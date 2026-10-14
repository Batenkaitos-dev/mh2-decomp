/* The portal's page steps (mode 1, steps 3 to 15): the page shown, its
 * dialogs (page 8), the pages 9 and 10. include/browser.h. */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"

/* Compiled at optimization level 4 (Portal_ParseStep matches only so;
 * the others at 3 and 4). */
#pragma optimization_level 4

void Browser_UpdateFrame(void);
s8 Page_CheckImages(void);
void Browser_GoForward(void);
void Browser_GoBack(void);
void Portal_Back(void);
s32 Browser_Timers(void);
void BrowserHist_Cancel(void);
/* the calls cast to s8: this unit's code takes the value as s8 */
s32 Page_Load(void);
void Browser_Reload(void);
void Browser_Fail(u8 page);
void Browser_ShowPage(void);
void Browser_OpenPage(void);
void Browser_SetStep(s8 step);
void Browser_SetUrl(BrowserWork *w, char *url);
void BrowserReq_Post(char **url);
s16 *Html_LoadStep(s32 arg);
void Html_StopLoad(void);
s32 Browser_Retry(void);
s8 Portal_HandleData(void);        /* the lobby server's answer (unk_73C310.c) */
s8 Browser_GameStartError(void);
void Page_RequestImages(void);
u32 strlen(const char *s);
s32 strcmp(const char *a, const char *b); /* main: strcmp */

/* Portal step 3: parses the page in 20 ms slices (Html_LoadStep): the
 * page's state for the dialogs (dlgBoxKind), and when done or failed the
 * next step; a MOUSE page may be cancelled while it loads (page 4, mode
 * 1 step 6). */
void Portal_ParseStep(void)
{
    u16 *r;

    Browser_UpdateFrame();
    r = (u16 *)Html_LoadStep(0);
    if (r == NULL) {
        return;
    }
    switch (*r) {
    case 0:
        gBrowserState->page = 3;
        gBrowserWork->dlgBoxKind = 2;
        if (gBrowserState->styled != 0 && ((gBrowserState->style & 0x20) >> 5) != 0 &&
            (gBrowserWork->padPressed & 0x8000) && gBrowserWork->firstPage == 0 && gBrowserState->plain == 0) {
            gBrowserState->page = 4;
            Html_StopLoad();
            gBrowserState->mode = 1;
            gBrowserState->step = 6;
        }
        break;
    case 1:
        gBrowserState->page = 5;
        gBrowserWork->dlgBoxKind = 3;
        break;
    case 2:
        gBrowserState->page = 6;
        switch (gBrowserWork->dlgBoxKind) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 12:
            gBrowserWork->dlgBoxKind = 2;
            break;
        }
        if ((gBrowserState->prevPageMode == 1 && gBrowserState->pageMode == 1) || (gBrowserState->prevPageMode == 2 && gBrowserState->pageMode == 2)) {
            Browser_Retry();
        } else {
            gBrowserWork->tries = 0;
            gBrowserState->step++;
        }
        break;
    case 3:
        gBrowserState->page = 6;
        gBrowserWork->dlgBoxKind = 2;
        gBrowserState->step++;
        break;
    }
}

/* Page_ClampScroll: a scroll position no further than max, else pulled
 * back (the game computes the field's address for the store alone). */
static inline void Page_ClampScroll(s32 *p, s32 max, s32 back)
{
    if (max < *p) {
        *p = max;
    } else {
        *p -= back;
    }
}

/* Portal step 4: the page parsed: the scroll bars' sizes, whether it has
 * dialog boxes (dlgBackState 1, else 3), its images asked for, the scroll
 * position on the anchor named by the URL (url3), then step 5. While the
 * lobby server's answer is awaited (unk30), its result first (an error
 * page for a game-start error). */
void Portal_PageReady(void)
{
    u16 i;
    BrowserObj *p;
    u8 *q;

    if (gBrowserState->vThumbLen == 0) {
        gBrowserState->vThumbLen = 1;
    }
    if (gBrowserState->hThumbLen == 0) {
        gBrowserState->hThumbLen = 1;
    }
    gBrowserState->vThumbY = 16;
    gBrowserState->hThumbX = 16;
    if (gBrowserSub->dlgMode == 3 || gBrowserSub->dlgMode == 4) {
        Browser_UpdateFrame();
        return;
    }
    if (gBrowserState->pageMode != 0) {
        switch (Portal_HandleData()) {
        case 0:
            return;
        case 1:
            Browser_UpdateFrame();
            return;
        case 2:
            break;
        case -1:
        case -2:
            gBrowserWork->atMmbbUrl = 0;
            switch (Browser_GameStartError()) {
            case 1:
                gBrowserState->pageMode = 0;
                gBrowserState->plain = 0;
                return;
            default:
                gBrowserState->pageMode = 0;
                gBrowserState->plain = 0;
                break;
            }
            break;
        case -9:
            gBrowserState->pageMode = 0;
            break;
        }
    }
    gBrowserWork->dlgBackState = 3;
    for (i = 0; i < 500; i++) {
        p = gBrowserWork->obj[i];
        if (p == NULL || !p->active) {
            break;
        }
        if (p->kind == 13 && p->active == 1) {
            gBrowserWork->dlgBackState = 1;
        }
    }
    q = &gBrowserWork->dlgBoxKind;
    switch (*q) {
    case 2:
    case 3:
        *q = 1;
        break;
    }
    Page_RequestImages();
    if (strlen(gBrowserState->url3) != 0) {
        for (i = 0; i < 500; i++) {
            p = gBrowserWork->obj[i];
            if (p == NULL || !p->active) {
                break;
            }
            if (p->kind == 14 && strcmp(p->s1, gBrowserState->url3) == 0) {
                gBrowserState->x = gBrowserWork->obj[i]->x;
                gBrowserState->y = gBrowserWork->obj[i]->y;
                if (gBrowserState->x < 0) {
                    gBrowserState->x = 0;
                }
                Page_ClampScroll(&gBrowserState->x, gBrowserState->pageW - 0x248, 20);
                if (gBrowserState->y < 0) {
                    gBrowserState->y = 0;
                }
                Page_ClampScroll(&gBrowserState->y, gBrowserState->pageH - 0x17C, 40);
                break;
            }
        }
    }
    Browser_UpdateFrame();
    gBrowserState->step++;
    gBrowserWork->firstPage = 0;
    if (gBrowserWork->atMmbbUrl == 0) {
        gBrowserState->fade = 16;
    }
}

/* Portal step 5: the page's dialog (page 8), or the page itself; the
 * cancel button on a dialog's first stage goes to step 6 (page 9). */
void Portal_DialogStep(void)
{
    BrowserState *st = gBrowserState;
    u8 *page = &st->page;

    if (*page == 8 && st->styleWait == 0) {
        if (((st->style & 0x20) >> 5) != 0 && (gBrowserWork->padPressed & 0x8000) &&
            st->plain == 0 && gBrowserWork->dlgBoxKind == 1 && gBrowserWork->dlgInputState == 2) {
            *page = 9;
            Browser_SetStep(6);
            return;
        }
        if (Page_CheckImages() == -1) {
            Portal_Back();
            gBrowserState->fade = 0;
            return;
        }
    }
    Browser_UpdateFrame();
    Browser_Timers();
}

/* Portal step 6: the page (re)loaded after a dialog. */
void Portal_ReshowStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case 1:
    case 2:
        switch ((s8)Page_Load()) {
        case -1:
            Browser_ShowPage();
            break;
        case 0:
            break;
        case 1:
            Browser_ShowPage();
            break;
        }
        break;
    case 4:
        BrowserHist_Cancel();
        Browser_ShowPage();
        break;
    case 8:
    case 9:
        if (Page_CheckImages() != 0) {
            Browser_ShowPage();
        }
        break;
    }
}

/* Portal step 7: a dialog's answer on page 10: 1 back (Browser_GoBack),
 * 2 forward (Browser_GoForward)? */
void Portal_HistoryStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case 8:
        Page_CheckImages();
        break;
    case 10:
        switch (gBrowserState->histDir) {
        case 1:
            Browser_GoBack();
            break;
        case 2:
            Browser_GoForward();
        }
        break;
    }
}

/* Portal step 8: reload. */
void Portal_ReloadStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case 8:
        Page_CheckImages();
        break;
    case 10:
        Browser_Reload();
        break;
    }
}

/* Portal step 9: open the URL asked for. */
void Portal_OpenStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case 8:
        Page_CheckImages();
        break;
    case 10:
        Browser_SetUrl(gBrowserWork, gBrowserState->url);
        BrowserReq_Post(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        Browser_OpenPage();
        break;
    }
}

/* Portal step 10: back. */
void Portal_BackStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case 8:
        Page_CheckImages();
        break;
    case 10:
        Portal_Back();
        break;
    }
}

/* Portal step 11: leave the browser for the game's menu (sub_main
 * 0x5C3500 = 1). */
void Portal_GameMenuStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case 8:
        Page_CheckImages();
        break;
    case 10:
        gPortalExit = 1;
        Browser_Fail(0);
        break;
    }
}

/* Portal step 12: leave, cancelled (0x5C3500 = 2). */
void Portal_CancelStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case 8:
        Page_CheckImages();
        break;
    case 10:
        gPortalExit = 2;
        Browser_Fail(0);
        break;
    }
}
