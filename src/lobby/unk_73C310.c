/* The portal's pages: back and forward (the history), the dialogs, the
 * software keyboard, links and forms, the lobby server's data.
 * include/browser.h. */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "kbd.h"

char *BrowserHist_Forward(void);
s32 BrowserHist_CanForward(void);
char *BrowserHist_Back(void);
s32 BrowserHist_CanBack(void);

/* Compiled at optimization level 4 (Link_Follow matches only so; every
 * other function matches at 3 and 4). */
#pragma optimization_level 4
int strncmp(const char *, const char *, unsigned int);

s32 strcmp(const char *a, const char *b); /* main: strcmp */
s32 BrowserReq_CanReload(void);                       /* History_CanBack? */
char *BrowserReq_Reload(void);                     /* History_Back? */
s32 BrowserForm_Add(const char *name, char *value); /* Cookie_Set? */
void BrowserReq_Post(char **url);
void Browser_Go(char *url);
void Browser_OpenPage(void);
void Browser_ShowPage(void);
s32 strncmp(const char *a, const char *b, u32 n);
char *strcpy(char *dst, const char *src);
void BrowserForm_Clear(void);
void Browser_Fail(u8 page);
void Browser_Move(s32 dx, s32 dy, u8 kind);
void Browser_ScrollBy(s32 dx, s32 dy);

/* The right stick's 16 directions: a step across and down (.data, copied
 * to the stack whole). */
typedef struct StickDirs {
    s8 v[16];
} StickDirs __attribute__((aligned(16)));
extern StickDirs gPageScrollDx;
extern StickDirs gPageScrollDy;


/* Link_Follow's links: the portal's own (mmbb://...) and the registration
 * record's fields it checks. */
extern char D_00839110[];
extern u8 gLobbyPageData[];
u32 strlen(const char *s);
s32 memcmp(const char *a, const char *b, u32 n); /* main: strncmp */
char *strstr(const char *s, const char *sub);    /* main: strstr */
void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
void Browser_Reload(void);
void Browser_SetStep(s8 step);
void Browser_SetUrl(BrowserWork *w, char *url);
char *strcat(char *dst, const char *src);
char *BrowserHist_CurUrl(void); /* the page URL */

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

/* Portal_Back: the previous page of the history (or the page again). */
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

/* Page_ScrollInput: on a MOUSE page (style flag 1), scrolls by the
 * pointer at the view's edges, the directions held, the right stick
 * (faster when fully tilted) and the USB keyboard (Page Up/Down, Home,
 * End). */
void Page_ScrollInput(void)
{
    StickDirs dx = gPageScrollDx;
    StickDirs dy = gPageScrollDy;
    s8 my;
    s8 mx;
    u16 b;

    if (!(gBrowserState->style & PAGE_STYLE_POINTER)) {
        return;
    }
    if (gBrowserWork->stickTilt != 0) {
        if (gBrowserSub->cursorX <= gBrowserState->marginLeft) {
            Browser_ScrollBy(-20, 0);
        }
        if (gBrowserSub->cursorX >= 0x27C - gBrowserState->marginRight) {
            Browser_ScrollBy(20, 0);
        }
        if (gBrowserSub->cursorY <= gBrowserState->marginTop) {
            Browser_ScrollBy(0, -20);
        }
        if (gBrowserSub->cursorY >= 0x1BC - gBrowserState->marginBottom) {
            Browser_ScrollBy(0, 20);
        }
    }
    b = gBrowserWork->padHeld;
    if ((b & 0x2000) || (b & 0x1000) || (b & 0x800) || (b & 0x400) || (D_581AFC >= 0x4F && D_581AFC <= 0x52)) {
        if (!(gBrowserSub->cursorX >= gBrowserState->marginLeft)) {
            Browser_ScrollBy(-20, 0);
        }
        if (gBrowserSub->cursorX > 0x278 - gBrowserState->marginRight) {
            Browser_ScrollBy(20, 0);
        }
        if (!(gBrowserSub->cursorY >= gBrowserState->marginTop)) {
            Browser_ScrollBy(0, -20);
        }
        if (gBrowserSub->cursorY > 0x1B8 - gBrowserState->marginBottom) {
            Browser_ScrollBy(0, 20);
        }
    }
    mx = my = 0;
    if (gBrowserWork->stick2Tilt != 0) {
        u8 d = gBrowserWork->stick2Dir;

        mx += dx.v[d];
        my += dy.v[d];
        gBrowserSub->activeObj = 0;
        if (gBrowserWork->stick2Tilt == 3) {
            gBrowserState->x += mx * 4;
            gBrowserState->y += my * 4;
        } else {
            gBrowserState->x += mx * 2;
            gBrowserState->y += my * 2;
        }
    }
    if (gBrowserWork->padHeld & 8) {
        Browser_ScrollBy(0, -20);
    }
    if (gBrowserWork->padHeld & 0x80) {
        Browser_ScrollBy(0, 20);
    }
    if (gBrowserWork->padHeld & 4) {
        Browser_ScrollBy(-20, 0);
    }
    if (gBrowserWork->padHeld & 0x10) {
        Browser_ScrollBy(20, 0);
    }
    if (gKbd->usbKey == USB_KEY_PAGE_UP) {
        Browser_ScrollBy(0, -0x168);
    }
    if (gKbd->usbKey == USB_KEY_PAGE_DOWN) {
        Browser_ScrollBy(0, 0x168);
    }
    if (gKbd->usbKey == USB_KEY_HOME) {
        Browser_ScrollBy(0, -gBrowserState->pageH);
    }
    if (gKbd->usbKey == USB_KEY_END) {
        Browser_ScrollBy(0, gBrowserState->pageH);
    }
}

/* Page_DragScroll: dragging a scroll bar's thumb with the pointer (held
 * button 0x20). An open select list (unk4 items, more than 10) has its own
 * bar right of the list (unk6: the list opens upward): a press on the
 * thumb (unk8..unkA) notes where it was taken (unk14, unk18, unk1C), a
 * drag moves it and the first item shown (unk5). Else the page's bars:
 * the right one (x 0x249-0x27F) and the bottom one (y 0x17D-0x1BF), the
 * thumbs kept within their bars. */
void Page_DragScroll(void)
{
    BrowserObj *o;
    f32 top;
    f32 right;
    f32 left;
    s16 x;
    s32 lim;
    u8 n;

    n = gBrowserSub->listCount;
    if (n) {
        o = gBrowserWork->obj[gBrowserSub->activeObj];
        top = o->y - (f32)gBrowserState->y;
        left = o->x - (f32)gBrowserState->x;
        right = left + o->w;
        if (n > 10) {
            if (gBrowserWork->padPrevHeld & 0x20) {
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
                    if (gBrowserSub->thumbTop < 0x12) {
                        gBrowserSub->listTop = 1;
                        gBrowserSub->thumbTop = 0x12;
                        gBrowserSub->thumbBottom = gBrowserSub->thumbLen + 0x12;
                    } else {
                        gBrowserSub->listTop = gBrowserSub->listCount * (gBrowserSub->thumbTop - 0x12) / 144 + 1;
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
        if (gBrowserWork->padPrevHeld & 0x20) {
            if (gBrowserSub->cursorX > 0x248 && gBrowserSub->cursorX < 0x280 && gBrowserSub->cursorY > 0x10 &&
                gBrowserSub->cursorY < 0x17C && gBrowserSub->dragX > 0x248) {
                if (gBrowserSub->cursorY == gBrowserSub->dragY) {
                    return;
                }
                gBrowserState->vThumbY = gBrowserSub->cursorY - gBrowserSub->dragOfs;
                gBrowserState->y = gBrowserState->pageH * (gBrowserState->vThumbY - 0x10) / 348;
            }
            if (gBrowserSub->cursorX > 0x10 && gBrowserSub->cursorX < 0x24C && gBrowserSub->cursorY > 0x17C &&
                gBrowserSub->dragY > 0x17C) {
                if (gBrowserSub->cursorX == gBrowserSub->dragX) {
                    return;
                }
                gBrowserState->hThumbX = gBrowserSub->cursorX - gBrowserSub->dragOfs;
                gBrowserState->x = gBrowserState->pageW * (gBrowserState->hThumbX - 0x10) / 552;
            }
        } else {
            if (gBrowserSub->cursorX > 0x248 && gBrowserSub->cursorX < 0x280) {
                gBrowserSub->dragX = gBrowserSub->cursorX;
                gBrowserSub->dragY = gBrowserSub->cursorY;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserState->vThumbY;
            }
            if (gBrowserSub->cursorY > 0x17C && gBrowserSub->cursorY < 0x1C0) {
                gBrowserSub->dragX = gBrowserSub->cursorX;
                gBrowserSub->dragY = gBrowserSub->cursorY;
                gBrowserSub->dragOfs = gBrowserSub->cursorX - gBrowserState->hThumbX;
            }
        }
        if (gBrowserSub->cursorX < gBrowserState->marginLeft + 0x248 && gBrowserSub->cursorY < gBrowserState->marginTop + 0x17C) {
            return;
        }
        lim = 0x17C - gBrowserState->vThumbLen;
        if (lim < gBrowserState->vThumbY) {
            gBrowserState->vThumbY = lim;
            gBrowserState->y = gBrowserState->pageH - 0x17C;
        }
        if (gBrowserState->vThumbY < 0x10) {
            gBrowserState->vThumbY = 0x10;
            gBrowserState->y = 0;
        }
        lim = 0x24C - gBrowserState->hThumbLen;
        if (lim < gBrowserState->hThumbX) {
            gBrowserState->hThumbX = lim;
            gBrowserState->x = gBrowserState->pageW - 0x248;
        }
        if (gBrowserState->hThumbX < 0x10) {
            gBrowserState->hThumbX = 0x10;
            gBrowserState->x = 0;
        }
    }
}

/* Page_PointerScroll: the pointer (gBrowserSub->cursorX/unk12) on the
 * scroll bars: their arrows scroll by 20 (Browser_Move), the bars
 * themselves by a page (0x167 down, 0x200 across). */
void Page_PointerScroll(void)
{
    if (gBrowserSub->listCount != 0) {
        return;
    }
    if (gBrowserSub->cursorX > gBrowserState->marginLeft + 0x248 && 0x280 - gBrowserState->marginRight > gBrowserSub->cursorX) {
        if (gBrowserState->pageH == 0) {
            return;
        }
        if (gBrowserSub->cursorY >= gBrowserState->marginTop && gBrowserSub->cursorY < gBrowserState->marginTop + 0x10) {
            Browser_Move(0, -20, 1);
            gBrowserWork->dlgVScrollState = BDLG_SCROLL_FIRST_PRESSED;
            return;
        }
        if (gBrowserSub->cursorY > 0x17C && gBrowserState->marginTop + 0x17C > gBrowserSub->cursorY) {
            Browser_Move(0, 20, 2);
            gBrowserWork->dlgVScrollState = BDLG_SCROLL_SECOND_PRESSED;
            return;
        }
        if (gBrowserSub->cursorY > gBrowserState->marginTop + 0x10 && gBrowserState->marginTop + 0x17C > gBrowserSub->cursorY) {
            if (gBrowserState->vThumbY > gBrowserSub->cursorY) {
                gBrowserState->y -= 0x167;
            }
            if (gBrowserSub->cursorY > gBrowserState->vThumbY + gBrowserState->vThumbLen) {
                gBrowserState->y += 0x167;
            }
            gBrowserState->vThumbY = gBrowserState->y * 0x15C / gBrowserState->pageH + 16;
        }
    }
    if (gBrowserSub->cursorY > gBrowserState->marginTop + 0x17C && 0x1AC - gBrowserState->marginBottom > gBrowserSub->cursorY &&
        gBrowserState->pageW != 0) {
        if (gBrowserSub->cursorX >= gBrowserState->marginRight && gBrowserSub->cursorX < gBrowserState->marginRight + 0x10) {
            Browser_Move(-20, 0, 3);
            gBrowserWork->dlgHScrollState = BDLG_SCROLL_FIRST_PRESSED;
            return;
        }
        if (gBrowserSub->cursorX > 0x24C && gBrowserState->marginRight + 0x248 > gBrowserSub->cursorX) {
            Browser_Move(20, 0, 4);
            gBrowserWork->dlgHScrollState = BDLG_SCROLL_SECOND_PRESSED;
            return;
        }
        if (gBrowserSub->cursorX > gBrowserState->marginRight + 0x10 && gBrowserState->marginRight + 0x24C > gBrowserSub->cursorX) {
            if (gBrowserState->hThumbX > gBrowserSub->cursorX) {
                gBrowserState->x -= 0x200;
            }
            if (gBrowserSub->cursorX > gBrowserState->hThumbX + gBrowserState->hThumbLen) {
                gBrowserState->x += 0x200;
            }
            gBrowserState->hThumbX = gBrowserState->x * 0x228 / gBrowserState->pageW + 16;
        }
    }
}

/* Page_PointAt: on a MOUSE page, which object the pointer is over
 * (focus: links, fields, buttons, a clickable image), unless it is on
 * the scroll bars; none: dlgStatusState 1. */
void Page_PointAt(void)
{
    s32 i;
    BrowserObj *o;
    u8 a;
    f32 x;
    f32 y;
    f32 cx;
    f32 cy;

    if (((gBrowserState->style & PAGE_STYLE_STATUS_BAR) >> 2) != 0) {
        gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
    }
    if (((gBrowserState->style & PAGE_STYLE_VSCROLL) >> 1) != 0) {
        if (gBrowserSub->cursorX > 0x270 - gBrowserState->marginRight) {
            return;
        }
        if (((gBrowserState->style & PAGE_STYLE_STATUS_BAR) >> 2) != 0) {
            if (0x19C - gBrowserState->marginBottom < gBrowserSub->cursorY && gBrowserSub->cursorY < 0x1AC) {
                return;
            }
        } else if (gBrowserSub->cursorY > 0x1B0 - gBrowserState->marginBottom && gBrowserSub->cursorY < 0x1C0 - gBrowserState->marginBottom) {
            return;
        }
    }
    for (i = 0; i < 500; i++) {
        o = gBrowserWork->obj[i];
        if (o == NULL) {
            break;
        }
        if ((a = o->active) == 0) {
            continue;
        }
        x = o->x - (f32)gBrowserState->x;
        cx = gBrowserSub->cursorX;
        if (cx < x) {
            continue;
        }
        if (!(cx <= o->w + x)) {
            continue;
        }
        y = o->y - (f32)gBrowserState->y;
        cy = gBrowserSub->cursorY;
        if (cy <= y) {
            continue;
        }
        if (!(cy < o->h + y)) {
            continue;
        }
        switch (o->kind) {
        case 2:
        case 17:
        case 18:
            if ((u8)*o->s1 != 0) {
                gBrowserWork->focus = i;
                return;
            }
            break;
        case 13:
            if (a == 3 || a == 5) {
                if (o->sub != 0) {
                    gBrowserWork->focus = i;
                    return;
                }
            }
            break;
        case 6:
        case 7:
            gBrowserWork->focus = i;
            return;
        }
    }
    gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
}

/* Page_Clamp: the scroll position within the page (the view is 0x248 by
 * 0x17C), the cursor within the page's margins. */
void Page_Clamp(void)
{
    s32 lim;

    if (gBrowserState->y < 0) {
        gBrowserState->y = 0;
    }
    lim = gBrowserState->pageH - 0x17C;
    if (gBrowserState->y > lim) {
        gBrowserState->y = lim;
    }
    if (gBrowserState->x < 0) {
        gBrowserState->x = 0;
    }
    lim = gBrowserState->pageW - 0x248;
    if (gBrowserState->x > lim) {
        gBrowserState->x = lim;
    }
    if (gBrowserSub->cursorY < gBrowserState->marginTop) {
        gBrowserSub->cursorY = gBrowserState->marginTop;
    }
    if (gBrowserSub->cursorY > 0x1B6 - gBrowserState->marginBottom) {
        gBrowserSub->cursorY = 0x1B6 - gBrowserState->marginBottom;
    }
    if (gBrowserSub->cursorX < gBrowserState->marginLeft) {
        gBrowserSub->cursorX = gBrowserState->marginLeft;
    }
    if (gBrowserSub->cursorX > 0x276 - gBrowserState->marginRight) {
        gBrowserSub->cursorX = 0x276 - gBrowserState->marginRight;
    }
}

/* Browser_Idle: 1 when no key (Enter, the arrows), no direction or
 * button (held or pressed) and no stick is in use, else -1. */
s32 Browser_Idle(void)
{
    KbdEntry *kb = gKbd;
    s32 k;
    u16 b;

    k = kb->usbPrevKey;
    if (k == USB_KEY_ENTER || k == USB_KEY_KP_ENTER) {
        return -1;
    }
    if (k == USB_KEY_UP || k == USB_KEY_RIGHT || k == USB_KEY_LEFT || k == USB_KEY_DOWN) {
        return -1;
    }
    k = kb->usbKey;
    if (k == USB_KEY_UP || k == USB_KEY_RIGHT || k == USB_KEY_LEFT || k == USB_KEY_DOWN) {
        return -1;
    }
    b = gBrowserWork->padHeld;
    if ((b & 0x800) || (b & 0x2000) || (b & 0x400) || (b & 0x1000)) {
        return -1;
    }
    b = gBrowserWork->padPressed;
    if ((b & 0x800) || (b & 0x2000) || (b & 0x400) || (b & 0x1000)) {
        return -1;
    }
    if (gBrowserWork->stickTilt != 0) {
        return -1;
    }
    return 1;
}

void LobbySe_Play(s32 se);
void Browser_OpenKeyboard(char *text, s16 max, s32 mode);
void Select_Close(void);
s32 Select_ListClick(s32 i);
void Link_Follow(s32 i);
void Form_Submit(s32 button);
void Form_Reset(s32 button);
extern char D_00839140[]; /* "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp" */

/* Page_Click: a click of the pointer (gBrowserSub->cursorX, cursorY) on the
 * page: while a select list is open (sub.activeObj) it goes to
 * Select_ListClick; else the object under the pointer acts by kind: text
 * inputs (6, 7) open the software keyboard (a field 4 takes its mode from
 * the two digits of its name), links (2, 17, 18) and image buttons (13)
 * follow, submit (3) and reset (4) buttons, the portal's buttons (5), a
 * select (8) opens its list (10 rows at most, above the select when they
 * would not fit below), radio buttons (11) clear the others of the same
 * name, check boxes (12) toggle. The hit test reads gBrowserWork,
 * gBrowserState and gBrowserSub through read-only local views (w, st, sb);
 * the stores go through the globals.
 * MWCC keeps a const view as a local of its own (a plain copy of a global is
 * propagated away), so declared before o, in this order, the three are
 * numbered before the loop's object, as the game colours them (gBrowserWork
 * t1, gBrowserState t0, gBrowserSub a3, the object a2). */
void Page_Click(void)
{
    const BrowserWork *w;
    const BrowserState *st;
    const BrowserSub *sb;
    BrowserObj *o;
    s32 i;
    s32 j;
    s32 k;
    s32 mode;
    u8 a;
    f32 x0;
    f32 y0;
    f32 r;
    u8 *s;

    if ((s8)Browser_Idle() < 0) {
        return;
    }
    if (gBrowserSub->cursorY > 0x1AC - gBrowserState->marginBottom) {
        return;
    }
    if (gBrowserSub->cursorX < gBrowserState->marginLeft ||
        gBrowserSub->cursorY < gBrowserState->marginTop || gBrowserSub->cursorX > gBrowserState->marginLeft + 0x248 ||
        gBrowserSub->cursorY > gBrowserState->marginTop + 0x17C) {
        gBrowserSub->activeObj = 0;
        gBrowserSub->listCount = 0;
        return;
    }
    for (i = 0; i < 500; i++) {
        w = gBrowserWork;
        o = w->obj[i];
        if (o == NULL) {
            break;
        }
        if ((a = o->active) == 0) {
            continue;
        }
        st = gBrowserState;
        sb = gBrowserSub;
        x0 = o->x - (f32)st->x;
        if ((f32)sb->cursorX < x0 || !((f32)sb->cursorX <= o->w + x0)) {
            continue;
        }
        if (sb->activeObj != 0) {
            if (w->obj[sb->activeObj]->kind == 8) {
                gBrowserSub->activeObj = Select_ListClick(sb->activeObj);
            }
            return;
        }
        y0 = o->y - (f32)st->y;
        if ((f32)sb->cursorY <= y0 || !((f32)sb->cursorY < o->h + y0)) {
            continue;
        }
        if (st->styleWait == 0 && st->fade == 0) {
            switch (o->kind) {
            case 6:
            case 7:
            case 8:
            case 11:
            case 12:
                break;
            case 2:
            case 17:
            case 18:
                if (*(u8 *)o->s1 == 0) {
                    break;
                }
            case 3:
            case 4:
            case 5:
                LobbySe_Play(0);
                break;
            case 13:
                if (a == 3 || a == 5) {
                    LobbySe_Play(0);
                }
                break;
            }
        }
        o = gBrowserWork->obj[i];
        switch (o->kind) {
        case 6:
        case 7:
            gBrowserWork->dlgInputState = BDLG_INPUT_OPEN;
            LobbySe_Play(0);
            gBrowserSub->activeObj = i;
            gBrowserWork->focus = i;
            switch (gBrowserWork->obj[i]->kind) {
            case 6:
                if (gBrowserWork->obj[i]->field != 4) {
                    mode = 1;
                } else {
                    s = (u8 *)gBrowserWork->obj[i]->s1;
                    switch ((s[0] - '0') * 10 + (s[1] - '0')) {
                    case 0:
                    case 2:
                    case 3:
                    case 4:
                    case 8:
                    case 9:
                        mode = 4;
                        break;
                    case 1:
                    case 5:
                    case 6:
                        mode = 3;
                        break;
                    case 7:
                        mode = 2;
                        break;
                    }
                }
                Browser_OpenKeyboard(gBrowserWork->obj[i]->s2, gBrowserWork->obj[i]->sub, mode);
                break;
            case 7:
                Browser_OpenKeyboard(gBrowserWork->obj[i]->s2, gBrowserWork->obj[i]->sub, 5);
                break;
            }
            gBrowserWork->dlgInputState = BDLG_INPUT_OPEN;
            gBrowserSub->dlgMode = BDLG_MODE_INPUT;
            return;
        case 13:
            if (o->active == 2) {
                continue;
            }
            if (gBrowserWork->obj[i]->active == 3) {
                Link_Follow(i);
            }
            if (gBrowserWork->obj[i]->active == 5) {
                Form_Submit(i);
            }
            return;
        case 2:
        case 17:
        case 18:
            if (*(u8 *)o->s1 == 0) {
                return;
            }
            Link_Follow(i);
            continue;
        case 3:
            Form_Submit(i);
            continue;
        case 4:
            gBrowserWork->obj[i]->sub = 1;
            gBrowserWork->pressTimer = 3;
            Form_Reset(i);
            return;
        case 5:
            switch (gBrowserWork->obj[i]->field) {
            case 0:
                Browser_Go(D_00839140);
                gBrowserSub->dlgMode = BDLG_MODE_PAGE;
                gBrowserSub->activeObj = 0;
                gBrowserWork->obj[i]->sub = 1;
                gBrowserWork->pressTimer = 3;
                if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                    Browser_SetStep(PAGE_STEP_RELOAD);
                    return;
                }
                Browser_Reload();
                break;
            case 1:
                Browser_Fail(0);
                break;
            }
            return;
        case 8:
            LobbySe_Play(0);
            if (gBrowserSub->activeObj == i) {
                gBrowserSub->activeObj = 0;
                gBrowserSub->dlgCursor = 0;
            } else {
                gBrowserSub->activeObj = i;
                gBrowserSub->dlgCursor = gBrowserWork->obj[i]->sub;
                gBrowserSub->listCount = 0;
                for (j = gBrowserSub->activeObj + 1; j < 500; j++) {
                    o = gBrowserWork->obj[j];
                    if (o->kind != 8 || o->tex == 0) {
                        break;
                    }
                    gBrowserSub->listCount++;
                }
                if (gBrowserSub->listCount < 10) {
                    gBrowserSub->listTop = 1;
                } else if (gBrowserSub->listCount - gBrowserSub->dlgCursor >= 9) {
                    for (gBrowserSub->listTop = 1; gBrowserSub->listTop < 500; gBrowserSub->listTop++) {
                        if (gBrowserWork->obj[gBrowserSub->activeObj + gBrowserSub->listTop]->tex == gBrowserSub->dlgCursor) {
                            break;
                        }
                    }
                } else {
                    gBrowserSub->listTop = gBrowserSub->listCount - 9;
                }
                if (gBrowserSub->listCount < 10) {
                    if (gBrowserSub->listCount == 0) {
                        Select_Close();
                        return;
                    }
                    if (180.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) > 380.0f) {
                        gBrowserSub->listUp = 1;
                        gBrowserSub->cursorY = (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) - 9.0f;
                    } else {
                        gBrowserSub->listUp = 0;
                        gBrowserSub->cursorY = 9.0f + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y));
                    }
                    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - (f32)gBrowserState->x);
                } else {
                    if (180.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) > 380.0f) {
                        gBrowserSub->listUp = 1;
                        gBrowserSub->cursorY = (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) - 9.0f;
                    } else {
                        gBrowserSub->listUp = 0;
                        gBrowserSub->cursorY = 9.0f + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y));
                    }
                    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - (f32)gBrowserState->x);
                }
            }
            o = gBrowserWork->obj[i];
            r = o->w + (o->x - (f32)gBrowserState->x);
            if ((f32)gBrowserSub->cursorX > r - 16.0f && (f32)gBrowserSub->cursorX < r) {
                o->active = 12;
                gBrowserWork->pressTimer = 3;
            }
            return;
        case 11:
            LobbySe_Play(0);
            for (k = i + 1; k < 500; k++) {
                if (gBrowserWork->obj[k] == NULL || gBrowserWork->obj[k]->kind == 0x13) {
                    break;
                }
                if (gBrowserWork->obj[k]->kind == 11 && strcmp(gBrowserWork->obj[k]->s1, gBrowserWork->obj[i]->s1) == 0) {
                    gBrowserWork->obj[k]->sub = 0;
                }
            }
            for (j = i - 1; j > 0; j--) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->kind == 0x13) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == 11 && strcmp(gBrowserWork->obj[j]->s1, gBrowserWork->obj[i]->s1) == 0) {
                    gBrowserWork->obj[j]->sub = 0;
                }
            }
            gBrowserWork->obj[i]->sub = 1;
            return;
        case 12:
            LobbySe_Play(0);
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
    if (i == 500) {
        gBrowserSub->activeObj = 0;
    }
}


/* Select_ListClick: a click while object i's select list (kind 8) is
 * open: the scroll bar (its arrows, pages and thumb) keeps the list open
 * and returns i; an option is chosen (its text and value go to the
 * select, the list closes: 0); a click outside closes it (0). */
s32 Select_ListClick(s32 i)
{
    BrowserObj *o;
    f32 x0;
    f32 x1;
    f32 y0;
    f32 px;
    f32 top;
    s32 k;
    s32 m;

    o = gBrowserWork->obj[i];
    x0 = o->x - (f32)gBrowserState->x;
    y0 = o->y - (f32)gBrowserState->y;
    x1 = x0 + o->w;
    px = (f32)gBrowserSub->cursorX;
    if (px <= x0 || px >= x1) {
        gBrowserSub->listCount = 0;
        return 0;
    }
    if (gBrowserSub->listCount > 10 && px > x1 - 16.0f && px < x1) {
        if (gBrowserSub->listUp != 0) {
            if ((f32)gBrowserSub->cursorY < y0 - 162.0f && (f32)gBrowserSub->cursorY > y0 - 180.0f) {
                if (gBrowserSub->listTop > 1) {
                    Browser_Move(0, -1, 5);
                } else {
                    Browser_Move(0, 0, 5);
                }
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= y0 - 162.0f &&
                (f32)gBrowserSub->cursorY <= y0 + gBrowserSub->thumbTop - 180.0f) {
                if (gBrowserSub->listTop - 9 <= 0) {
                    gBrowserSub->listTop = 1;
                } else {
                    gBrowserSub->listTop -= 9;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * 144 / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = 1440 / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY > y0 + gBrowserSub->thumbTop - 180.0f &&
                (f32)gBrowserSub->cursorY < y0 + gBrowserSub->thumbBottom - 180.0f) {
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= y0 + gBrowserSub->thumbBottom - 180.0f && (f32)gBrowserSub->cursorY <= y0 - 20.0f) {
                gBrowserSub->listTop += 9;
                if (gBrowserSub->listCount < gBrowserSub->listTop + 10) {
                    gBrowserSub->listTop = gBrowserSub->listCount - 9;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * 144 / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = 1440 / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY < y0 - 1.0f && (f32)gBrowserSub->cursorY > y0 - 20.0f) {
                if (gBrowserSub->listCount - gBrowserSub->listTop >= 10) {
                    Browser_Move(0, 1, 6);
                } else {
                    Browser_Move(0, 0, 6);
                }
                return i;
            }
        } else {
            top = 20.0f + y0;
            if ((f32)gBrowserSub->cursorY > top && (f32)gBrowserSub->cursorY < 20.0f + top) {
                if (gBrowserSub->listTop > 1) {
                    Browser_Move(0, -1, 5);
                } else {
                    Browser_Move(0, 0, 5);
                }
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= 20.0f + top && (f32)gBrowserSub->cursorY <= top + gBrowserSub->thumbTop) {
                if (gBrowserSub->listTop - 9 <= 0) {
                    gBrowserSub->listTop = 1;
                } else {
                    gBrowserSub->listTop -= 9;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * 144 / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = 1440 / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY > top + gBrowserSub->thumbTop && (f32)gBrowserSub->cursorY < top + gBrowserSub->thumbBottom) {
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= top + gBrowserSub->thumbBottom && (f32)gBrowserSub->cursorY <= 180.0f + top - 18.0f) {
                gBrowserSub->listTop += 9;
                if (gBrowserSub->listCount < gBrowserSub->listTop + 10) {
                    gBrowserSub->listTop = gBrowserSub->listCount - 9;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * 144 / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = 1440 / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY > 162.0f + top && (f32)gBrowserSub->cursorY < 180.0f + top) {
                if (gBrowserSub->listCount - gBrowserSub->listTop >= 10) {
                    Browser_Move(0, 1, 6);
                } else {
                    Browser_Move(0, 0, 6);
                }
                return i;
            }
        }
    }
    if (gBrowserSub->listUp != 0) {
        if (gBrowserSub->listCount < 10) {
            for (k = 0; k <= gBrowserSub->listCount; k++) {
                if ((f32)gBrowserSub->cursorY < 20.0f + y0 - k * 18 &&
                    (f32)gBrowserSub->cursorY >= 20.0f + y0 - (k + 1) * 18) {
                    break;
                }
            }
            if (k == 0 || k > gBrowserSub->listCount) {
                return 0;
            }
            m = gBrowserSub->listCount - k;
        } else {
            for (k = 0; k <= 10; k++) {
                if ((f32)gBrowserSub->cursorY < 20.0f + y0 - k * 18 &&
                    (f32)gBrowserSub->cursorY >= 20.0f + y0 - (k + 1) * 18) {
                    break;
                }
            }
            if (gBrowserSub->listCount - gBrowserSub->dlgCursor >= 9) {
                if (k == 0 || k > 10) {
                    return 0;
                }
            } else if ((k == 0 || k >= 10) && gBrowserSub->listCount < 10 && gBrowserSub->listCount - gBrowserSub->listTop < 9) {
                return 0;
            }
            m = 10 - k;
        }
        LobbySe_Play(0);
        gBrowserSub->dlgCursor = i + gBrowserSub->listTop + m;
        gBrowserWork->obj[i]->s1 = gBrowserWork->obj[i + gBrowserSub->listTop + m]->s1;
        gBrowserWork->obj[i]->sub = gBrowserWork->obj[i + gBrowserSub->listTop + m]->tex;
        gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y);
    } else {
        if (gBrowserSub->listCount < 10) {
            for (m = 0; m <= gBrowserSub->listCount; m++) {
                if ((f32)gBrowserSub->cursorY >= 20.0f + y0 + m * 18 &&
                    (f32)gBrowserSub->cursorY < 20.0f + y0 + (m + 1) * 18) {
                    break;
                }
            }
            if (m < 0 || m > gBrowserSub->listCount) {
                return 0;
            }
        } else {
            for (m = 0; m <= 10; m++) {
                if ((f32)gBrowserSub->cursorY >= 20.0f + y0 + m * 18 &&
                    (f32)gBrowserSub->cursorY < 20.0f + y0 + (m + 1) * 18) {
                    break;
                }
            }
            if (gBrowserSub->listCount - gBrowserSub->dlgCursor >= 9) {
                if (m < 0 || m > 10) {
                    return 0;
                }
            } else if (m < 0 || m >= 10) {
                return 0;
            }
        }
        LobbySe_Play(0);
        gBrowserSub->dlgCursor = i + gBrowserSub->listTop + m;
        gBrowserWork->obj[i]->s1 = gBrowserWork->obj[i + gBrowserSub->listTop + m]->s1;
        gBrowserWork->obj[i]->sub = gBrowserWork->obj[i + gBrowserSub->listTop + m]->tex;
        gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y);
    }
    gBrowserSub->listCount = 0;
    return 0;
}

/* Link_Follow: following the link of object i (its s1): the portal's own
 * mmbb:// links (EXIT;n: the n-th ;-separated part of the rest is the
 * link; BB_DOD_KIYAKU: the terms; REGIST_CHECK?: the registration
 * record's fields all given; GAME_MENU_n: back to the game with menu n;
 * BUTTON_OK, BUTTON_NG; BACK_TO_MENU: the lobby server's page with the
 * login's cookies), an anchor (#name: the page scrolled to it) or a URL
 * (a #fragment kept apart in urlAnchor). */
void Link_Follow(s32 i)
{
    s32 n;
    s32 k;
    BrowserObj *o;
    char *p;
    char *q;
    s32 len;
    BrowserWork *w;
    s32 j;
    u8 c;
    s32 d;

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
        w = gBrowserWork;
        o = w->obj[i];
        p = o->s1 + strlen("mmbb://EXIT;");
        for (k = 0; k < w->exitArgs; k++) {
            q = strstr(p, ";");
            if (q == NULL) {
                gBrowserWork->obj[i]->s1[0] = 0;
                goto next;
            }
            p = q + 1;
            w = gBrowserWork;
        }
        for (j = 0; j < len; j++) {
            c = p[j];
            if (c == ';' || c == 0) {
                break;
            }
            gBrowserWork->obj[i]->s1[j] = c;
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
            for (k = strlen("mmbb://GAME_MENU_"); (d = (u8)gBrowserWork->obj[i]->s1[k]) >= '0' && d <= '9'; k++) {
                n = gBrowserWork->gameMenuNo * 10;
                gBrowserWork->gameMenuNo = n + (d - '0');
            }
            gPortalExit = PORTAL_EXIT_GAME_MENU;
            Browser_Fail(0);
        }
    } else if (memcmp(gBrowserWork->obj[i]->s1, "mmbb://BUTTON_OK", strlen("mmbb://BUTTON_OK")) == 0) {
        memcpy(gLobbyPageData, &gRegRecord, sizeof(gRegRecord));
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_GAME_MENU);
            return;
        }
        gPortalExit = PORTAL_EXIT_GAME_MENU;
        Browser_Fail(0);
    } else if (memcmp(gBrowserWork->obj[i]->s1, D_00839110, strlen(D_00839110)) == 0) {
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_CANCEL);
            return;
        }
        gPortalExit = PORTAL_EXIT_CANCEL;
        Browser_Fail(0);
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
        for (k = 0; k < 500; k++) {
            if (gBrowserWork->obj[k] == NULL) {
                return;
            }
            if (gBrowserWork->obj[k]->active != 0 && gBrowserWork->obj[k]->kind == 0xE &&
                strcmp(gBrowserWork->obj[k]->s1, gBrowserWork->obj[i]->s1) == 0) {
                gBrowserState->x = gBrowserWork->obj[k]->x;
                gBrowserState->y = gBrowserWork->obj[k]->y;
                if (gBrowserState->x < 0) {
                    gBrowserState->x = 0;
                }
                if (gBrowserState->pageW - 0x280 < gBrowserState->x) {
                    gBrowserState->x = gBrowserState->pageW - 0x280;
                } else {
                    gBrowserState->x = gBrowserState->x - 0x14;
                }
                if (gBrowserState->y < 0) {
                    gBrowserState->y = 0;
                }
                if (gBrowserState->y > gBrowserState->pageH - 0x1C0) {
                    gBrowserState->y = gBrowserState->y - 0x184;
                } else {
                    gBrowserState->y = gBrowserState->y - 0x28;
                }
                return;
            }
        }
    } else {
        gBrowserWork->texFailOnce = 1;
        if ((q = strstr(gBrowserWork->obj[i]->s1, "#")) != NULL) {
            memset(gBrowserState->url, 0, 0x100);
            memset(gBrowserState->urlAnchor, 0, 0x100);
            strcpy(gBrowserState->urlAnchor, q);
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

/* Form_Reset: the text fields (kinds 6, 7) of the <FORM> (kind 0x13)
 * holding the button at obj[button] emptied. */
void Form_Reset(s32 button)
{
    s32 i;
    s32 form;
    u8 *p;

    for (i = 0; i < 500; i++) {
        if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
            break;
        }
        switch (gBrowserWork->obj[i]->kind) {
        case 0x13:
            form = i;
            break;
        case 4:
            if (i == button) {
                goto found;
            }
            break;
        }
    }
found:
    for (i = form + 1; i < 500; i++) {
        if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
            break;
        }
        switch (gBrowserWork->obj[i]->kind) {
        case 6:
        case 7:
            p = (u8 *)gBrowserWork->obj[i]->s2;
            if (*p != 0) {
                do {
                    p--;
                } while (*p != 0);
                *++p = 0;
                gBrowserWork->obj[i]->s2 = (char *)p;
            }
            break;
        case 0x13:
            goto end;
        }
    }
end:;
}

/* Form_Submit: the <FORM> (kind 0x13) before the submit button at
 * obj[button]. A form without an action is left alone (unless formGoLink).
 * With formGoLink the first text field (kind 6) after it is the URL to go to.
 * Method 0 (GET): the action, "?" and name=value pairs joined by "&" (the
 * text fields 6 and 7, the checked boxes 11 and 12, 20, the selects 8 with
 * their chosen option; one pair per name) in 0xFF bytes at most, a last
 * "&" cut, and "?" cut when the page's and the query's schemes differ
 * (file:// and http(s)://). Otherwise (POST) the pairs are posted as
 * cookies (BrowserForm_Add) and the action opened. Fields 1..3 (the login's
 * id, password, ...) are kept in gBrowserState. The POST loop counts with
 * `found` and keeps the cookie's result in `method`: the game reuses the
 * two (new variables, in any order, give other saved registers). */
void Form_Submit(s32 button)
{
    char query[0x100];
    char last[0x100];
    s32 i;
    s32 found;
    s32 method;
    s32 form;
    s16 room;
    u8 n;
    u8 *p;
    u8 c;

    found = 0;
    method = 0;
    form = 0;
    if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
        return;
    }
    for (i = button; i > 0; i--) {
        if (gBrowserWork->obj[i]->kind == 0x13) {
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
        for (i = form; i < 500; i++) {
            if (gBrowserWork->obj[i]->kind == 6) {
                Browser_Go(gBrowserWork->obj[i]->s2);
                gBrowserSub->activeObj = 0;
                gBrowserWork->obj[button]->sub = 1;
                gBrowserWork->pressTimer = 3;
                Browser_Reload();
                return;
            }
        }
    }
    switch (gBrowserWork->obj[button]->kind) {
    case 3:
    case 4:
        gBrowserWork->obj[button]->sub = 1;
        gBrowserWork->pressTimer = 3;
        break;
    }
    if (method == 0) {
        memset(query, 0, 0x100);
        room = 0xFF;
        n = strlen(gBrowserWork->obj[form]->s1);
        if (n >= 0x100) {
            return;
        }
        memcpy(query, gBrowserWork->obj[form]->s1, n);
        room -= n;
        if (room > 0) {
            strcat(query, "?");
            room--;
        }
        memset(last, 0, 0x100);
        for (i = form + 1; i < 500; i++) {
            if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
                break;
            }
            switch (gBrowserWork->obj[i]->kind) {
            case 8:
                if (gBrowserWork->obj[i]->tex != 0) {
                    continue;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[i]->s2, last) == 0) {
                    continue;
                }
                n = strlen(gBrowserWork->obj[i]->s2) + 1 + strlen(gBrowserWork->obj[i + gBrowserWork->obj[i]->sub]->s2);
                if (n > room) {
                    return;
                }
                strcat(query, gBrowserWork->obj[i]->s2);
                strcat(query, "=");
                strcat(query, gBrowserWork->obj[i + gBrowserWork->obj[i]->sub]->s2);
                room -= n;
                memset(last, 0, 0x100);
                strcpy(last, gBrowserWork->obj[i]->s2);
                if (room > 0) {
                    strcat(query, "&");
                    room--;
                }
                break;
            case 6:
            case 7:
            case 20:
            case 12:
            case 11:
                if ((gBrowserWork->obj[i]->kind == 12 || gBrowserWork->obj[i]->kind == 11) && gBrowserWork->obj[i]->sub == 0) {
                    continue;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[i]->s1, last) == 0) {
                    continue;
                }
                n = strlen(gBrowserWork->obj[i]->s1) + 1 + strlen(gBrowserWork->obj[i]->s2);
                if (n > room) {
                    return;
                }
                strcat(query, gBrowserWork->obj[i]->s1);
                strcat(query, "=");
                strcat(query, gBrowserWork->obj[i]->s2);
                room -= n;
                memset(last, 0, 0x100);
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
                        memset(gBrowserState->field3, 0, 0x11);
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
            case 0x13:
                goto query_done;
            }
        }
    query_done:
        c = query[0];
        p = (u8 *)query;
        while (c != 0) {
            if (c == '&' && p[1] == 0) {
                *p = 0;
                break;
            }
            p++;
            c = *p;
        }
        p = (u8 *)query;
        if (p == NULL || *p == 0) {
            return;
        }
        if (strstr(BrowserHist_CurUrl(), "file://") != NULL && strstr((char *)p, "http://") == NULL &&
            strstr((char *)p, "https://") == NULL) {
            for (; *p != 0; p++) {
                if (*p == '?' && p[1] == 0) {
                    *p = 0;
                    break;
                }
            }
        }
        if ((strstr(BrowserHist_CurUrl(), "http://") != NULL || strstr(BrowserHist_CurUrl(), "https://") != NULL) &&
            strstr((char *)p, "file://") != NULL) {
            for (; *p != 0; p++) {
                if (*p == '?' && p[1] == 0) {
                    *p = 0;
                    break;
                }
            }
        }
        Browser_Go(query);
        gBrowserState->histDir = 0;
        gBrowserSub->activeObj = 0;
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_RELOAD);
            return;
        }
        Browser_Reload();
    } else {
        BrowserForm_Clear();
        memset(last, 0, 0x100);
        method = 0;
        for (found = form + 1; found < 500; found++) {
            if (gBrowserWork->obj[found] == NULL || gBrowserWork->obj[found]->active == 0) {
                break;
            }
            switch (gBrowserWork->obj[found]->kind) {
            case 8:
                if (gBrowserWork->obj[found]->tex != 0) {
                    continue;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                method = BrowserForm_Add(gBrowserWork->obj[found]->s2, gBrowserWork->obj[found + gBrowserWork->obj[found]->sub]->s2);
                memset(last, 0, 0x100);
                strcpy(last, gBrowserWork->obj[found]->s1);
                break;
            case 6:
            case 7:
                if (gBrowserWork->formGoLink != 0 && strstr(gBrowserWork->obj[found]->s1, "URL") != NULL) {
                    break;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                method = BrowserForm_Add(gBrowserWork->obj[found]->s1, gBrowserWork->obj[found]->s2);
                memset(last, 0, 0x100);
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
                        memset(gBrowserState->field3, 0, 0x11);
                        strcpy(gBrowserState->field3, gBrowserWork->obj[found]->s2);
                        break;
                    }
                    break;
                }
                break;
            case 12:
            case 11:
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                if (gBrowserWork->obj[found]->sub != 0) {
                    method = BrowserForm_Add(gBrowserWork->obj[found]->s1, gBrowserWork->obj[found]->s2);
                    memset(last, 0, 0x100);
                    strcpy(last, gBrowserWork->obj[found]->s1);
                }
                break;
            case 20:
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                method = BrowserForm_Add(gBrowserWork->obj[found]->s1, gBrowserWork->obj[found]->s2);
                memset(last, 0, 0x100);
                strcpy(last, gBrowserWork->obj[found]->s1);
                break;
            case 0x13:
                goto post_done;
            }
            if (method < 0) {
                return;
            }
        }
    post_done:
        gBrowserSub->activeObj = 0;
        gBrowserState->histDir = 0;
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

/* Portal_HandleData: acts on a data page's result (the MMBB data's f6,
 * "OK..." or an error code) by the page's data mode (unk30): 1 sets the
 * cookies connection-id and user-id (the id typed, if any) and goes to the
 * menu URL (1); 4 and 5 keep the id typed and leave with the server list
 * (Browser_Fail(5), 0); 6 and 7 give 2; -1 an error result, -2 a cookie
 * failed, -9 no data mode. The id's strcmp takes it through an int cast:
 * the game computes that argument first. */
s8 Portal_HandleData(void)
{
    s8 i;
    s32 r;

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
                r = BrowserForm_Add(gIdFieldNames.name.userId, gBrowserState->formLoginId);
            } else {
                r = BrowserForm_Add(gIdFieldNames.name.userId, gMmbbData->userId);
            }
            if (r < 0) {
                return -2;
            }
            break;
        }
        Browser_Go(gMmbbData->f4);
        BrowserReq_Post(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        gBrowserWork->startPosts++;
        Browser_OpenPage();
        gBrowserState->bgColor = 0xFF000001;
        return 1;
    case CSV_KIND_SERVERS:
    case CSV_KIND_RESULT:
        if ((u8)gBrowserState->formLoginId[0] != 0 && strcmp((char *)(u32)gBrowserState->formLoginId, gMmbbData->userId) != 0) {
            strcpy(gMmbbData->userId, gBrowserState->formLoginId);
        }
        for (i = 0; i < 40; i++) {
            if ((u8)gMmbbData->serverAddr[i][0] == 0) {
                break;
            }
        }
        Browser_Fail(5);
        return 0;
    case CSV_KIND_MENU:
    case CSV_KIND_PORTAL:
        return 2;
    default:
        return -9;
    }
}

/* Portal_SetIds: the cookies user-id (the id typed in the login field, if
 * any and different, else the MMBB data's) and connection-id; -1 when
 * one can't be set. */
s32 Portal_SetIds(void)
{
    s32 r;
    char *id = gBrowserState->formLoginId;

    if (*(u8 *)id != 0 && strcmp(id, gMmbbData->userId) != 0) {
        r = BrowserForm_Add(gIdFieldNames.name.userId, gBrowserState->formLoginId);
    } else {
        r = BrowserForm_Add(gIdFieldNames.name.userId, gMmbbData->userId);
    }
    if (r < 0) {
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

/* unk_73C310's .data (0x7EEBF0-0x7EEC10) */
StickDirs gPageScrollDx = {
#include "lobby/007EEBF0.inc"
};
StickDirs gPageScrollDy = {
#include "lobby/007EEC00.inc"
};
