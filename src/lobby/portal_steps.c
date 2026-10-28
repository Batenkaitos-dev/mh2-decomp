/*
 * The portal's start and its page steps.
 * - Page_New clears the page (its objects, its draw commands, the view);
 *   Page_AllocObjs and Page_FreeObjs take the page's objects from the
 *   pool and give them back.
 * - BMODE_START's two steps (Browser_RunStart): the start URL checked
 *   against the MMBB data's, then, once the frame's texture is in, the
 *   frame, the kept login and the first page.
 * - BMODE_RUN's steps (Browser_RunPage, enum PageStep): a page opened,
 *   loaded, parsed in time slices and made ready (the lobby server's data
 *   answered, the images asked for, the scroll set on the URL's anchor);
 *   then the page shown with its dialogs, and the steps a dialog or the
 *   pad leads to: the page shown again, the history moves, a reload, a
 *   URL opened, back, the game's menu, the cancel, the waits for a
 *   request or for the images before the leave dialog, and the failure.
 * The page's load state through these steps is BrowserState.page (enum
 * PageLoad).
 */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "pad.h"
#include "render_state.h"
#include "net_save.h"
#include "lobby_globals.h"
#pragma optimization_level 4
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "text.h"
#include "font.h"

void *memset(void *dst, s32 c, u32 n);
/* The portal's start steps and its page steps. The two tables keep their
 * address names, splat's names of the jump tables the code calls
 * through. */
extern void (*jtbl_007EEB10[])(void);
u32 strlen(const char *s);
s32 memcmp(const char *a, const char *b, u32 n);
s32 strcmp(const char *a, const char *b);
char *strncpy(char *dst, const char *src, u32 n);
/* Declared as s32 and each call cast to s8: the callers read the result
 * as a signed byte, and compile as the game's only so. */
extern void (*jtbl_007EEB20[])(void);
extern char gBrowserUrl[];     /* the start URL */
/* The file is compiled at optimization level 4 (the pragma above):
 * Portal_ParseStep compiles as the game's only at that level, the others
 * at it or the default. */

/* Clears the page: the keyboard's text, its objects, textures and draw
 * commands (an empty title first), the view (at 0, 0, the view's size),
 * its kind, the focus; then the page's render state. Reloads wait 30
 * frames. */
void Page_New(void)
{
    memset(gBrowserWork->kbdText, 0, sizeof(gBrowserWork->kbdText));
    memset(&gBrowserWork->url, 0, sizeof(gBrowserWork->url));
    memset(gBrowserWork->texHandle, 0, sizeof(gBrowserWork->texHandle));
    Page_FreeObjs();
    Page_AllocObjs();
    memset(gBrowserWork->cmdBuf, 0, sizeof(gBrowserWork->cmdBuf));
    gBrowserWork->cmdPtr = gBrowserWork->cmdBuf;
    *gBrowserWork->cmdBuf = 0;
    gBrowserWork->cmdPtr++;
    Draw_Title((u8 *)"");
    gBrowserState->styleWait = 30; /* frames */
    gBrowserState->imgCount = 0;
    gBrowserState->vScroll = 0;
    gBrowserState->hScroll = 0;
    gBrowserState->x = 0;
    gBrowserState->y = 0;
    gBrowserState->pageW = PAGE_VIEW_W;
    gBrowserState->pageH = PAGE_VIEW_H;
    gBrowserState->pageKind = PAGE_KIND_NONE;
    gBrowserWork->focus = 0;
    gBrowserWork->texCount = 0;
    /* opaque draws; the alpha test passes what differs from the reference */
    Browser_SetRenderState(RS_ALPHA_REF, BROWSER_BLACK);
    Browser_SetRenderState(RS_ALPHA_TEST, RS_CMP_NOTEQUAL);
    Browser_SetRenderState(RS_BLEND, RS_BLEND_OPAQUE);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
}

/* The portal's BMODE_START: its step's function. */
void Browser_RunStart(void)
{
    jtbl_007EEB10[gBrowserState->step]();
}

/* Takes the page's objects from the pool, empty and doing nothing, until
 * the pool runs out; clears the page's textures first. */
void Page_AllocObjs(void)
{
    u16 n;

    memset(gBrowserWork->texHandle, 0, sizeof(gBrowserWork->texHandle));
    memset(gBrowserWork->texW, 0, sizeof(gBrowserWork->texW));
    memset(gBrowserWork->texH, 0, sizeof(gBrowserWork->texH));
    for (n = 0; n < PAGE_OBJ_COUNT; n++) {
        if ((gBrowserWork->obj[n] = PageObj_Alloc(BLAYER_PAGE, 0)) == NULL) {
            return;
        }
        gBrowserWork->obj[n]->active = 0;
        gBrowserWork->obj[n]->tex = 0;
        gBrowserWork->obj[n]->update = (BrowserObjFunc)PageObj_Nop;
        gBrowserWork->obj[n]->draw = (BrowserObjFunc)PageObj_Nop;
        gBrowserWork->obj[n]->visible = 0;
        gBrowserWork->obj[n]->tex = 0;
        gBrowserWork->obj[n]->sub = 0;
    }
}

/* The start steps. */
void (*jtbl_007EEB10[4])(void) = {
    Portal_Start0, Portal_Start1, NULL, NULL,
};

/* Gives the page's objects back to the pool. */
void Page_FreeObjs(void)
{
    u16 n;

    for (n = 0; n < PAGE_OBJ_COUNT; n++) {
        if (gBrowserWork->obj[n] == NULL) {
            return;
        }
        PageObj_Free(gBrowserWork->obj[n]);
        gBrowserWork->obj[n] = NULL;
    }
}

/* BMODE_START's first step: marks whether the start URL is one of the
 * MMBB data's (the session URL or the next URL), then the draw commands start over. */
void Portal_Start0(void)
{
    gBrowserWork->result = BROWSER_RUNNING;
    if (strcmp(gBrowserUrl, gMmbbData->sessionUrl) == 0 || strcmp(gBrowserUrl, gMmbbData->nextUrl) == 0) {
        gBrowserWork->atMmbbUrl = 1;
    } else {
        gBrowserWork->atMmbbUrl = 0;
    }
    Font_ResetState();
    gBrowserWork->cmdPtr = gBrowserWork->cmdBuf;
    gBrowserState->step++;
    gBrowserState->sub = 0;
}

/* BMODE_START's second step: once the frame's texture is in (its load's
 * last step, 4), the frame and its bars, the start URL, the dialogs, and
 * the login fields cleared, then filled with the login id and password
 * kept in the NET file; then the first page is loaded. */
void Portal_Start1(void)
{
    Browser_LoadFrame(3, &gBrowserState->sub);
    if (gBrowserState->sub == 4) {
        gBrowserState->sub = 0;
        Page_CreateFrame();
        gBrowserWork->dlgHScrollState = BDLG_SCROLL_SHOWN;
        gBrowserWork->dlgVScrollState = BDLG_SCROLL_SHOWN;
        gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
        Browser_Go(gBrowserUrl);
        gBrowserState->pageH = 0;
        gBrowserState->y = 0;
        gBrowserState->prevPageMode = CSV_KIND_NONE;
        gBrowserState->pageMode = CSV_KIND_NONE;
        Dialog_Reset();
        memset(gBrowserState->loginId, 0, sizeof(gBrowserState->loginId));
        memset(gBrowserState->password, 0, sizeof(gBrowserState->password));
        memset(gBrowserState->formLoginId, 0, sizeof(gBrowserState->formLoginId));
        memset(gBrowserState->field2, 0, sizeof(gBrowserState->field2));
        memset(gBrowserState->loginIdSave, 0, sizeof(gBrowserState->loginIdSave));
        memset(gBrowserState->field3, 0, sizeof(gBrowserState->field3));
        gBrowserState->unk32 = 0;
        gBrowserState->menuReached = 0;
        strncpy((char *)gBrowserState->loginId, gNetSaveData.mmbbLoginId, sizeof(gNetSaveData.mmbbLoginId) - 1);
        strncpy((char *)gBrowserState->password, gNetSaveData.mmbbPassword, sizeof(gNetSaveData.mmbbPassword) - 1);
        gBrowserState->page = PAGE_LOAD_NONE;
        gBrowserWork->dlgInputState = BDLG_INPUT_IDLE;
        Browser_SetRenderState(RS_BG_COLOR, BROWSER_BLACK);
        Browser_Reload();
    }
}

/* The portal's BMODE_RUN: the page step's function (enum PageStep). */
void Browser_RunPage(void)
{
    jtbl_007EEB20[gBrowserState->step]();
}

/* PAGE_STEP_START: opens the page. The start URL, when it is one of the
 * MMBB data's (atMmbbUrl), is posted with the ids as form fields when the
 * portal opens at the MMBB data's back or next URL (else nothing happens);
 * any other URL is requested first, with reqBusy set. */
void Portal_PageStart(void)
{
    Browser_UpdateFrame();
    if (gBrowserWork->atMmbbUrl != 0) {
        switch (gPortalPage) {
        case PORTAL_PAGE_NEXT:
        case PORTAL_PAGE_BACK:
            Portal_SetIds();
            Browser_OpenPage();
            break;
        }
    } else {
        BrowserReq_Page(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        Browser_OpenPage();
    }
}

/* PAGE_STEP_LOAD: loads the page (Page_Load) and answers its result: -2,
 * no page to show (the leave dialog); -1, an error (the lobby server's
 * first page fails; a session data page is retried; else the history's
 * page is shown again); 0, still loading (on a page whose style allows
 * the cancel, Start or Cross stops it); 1, loaded (the parse next). */
void Portal_LoadPage(void)
{
    if (gBrowserState->initMode != 0 && gBrowserState->fade < 0) {
        Browser_UpdateFrame();
        return;
    }
    gBrowserWork->dlgBoxKind = BDLG_KIND_LOADING;
    Browser_UpdateFrame();
    switch ((s8)Page_Load()) {
    case -2:
        gBrowserWork->endReason = BROWSER_END_LEAVE;
        Browser_OpenLeaveDialog(0);
        gBrowserSub->dlgCursor = 1;
        gBrowserState->step++;
        break;
    case -1:
        if (gBrowserWork->firstPage != 0 && memcmp(gBrowserState->url, gLbsScheme, strlen(gLbsScheme)) == 0) {
            Browser_Fail(PAGE_LOAD_NONE);
            break;
        }
        switch (gBrowserState->pageMode) {
        case CSV_KIND_MENU:
        case CSV_KIND_PORTAL:
        default:
            break;
        case CSV_KIND_SESSION:
            Browser_Retry();
            return;
        }
        Browser_Go(BrowserHist_CurUrl());
        Browser_ShowPage();
        break;
    case 0:
    {
        BrowserState *state = gBrowserState;

        if (((state->style & PAGE_STYLE_CANCEL) >> 5) != 0) {
            BrowserWork *work = gBrowserWork;
            u16 pressed = work->padPressed;

            if (((pressed & PAD_START) || (pressed & PAD_CROSS)) && work->firstPage == 0 && state->plain == 0) {
                state->page = PAGE_LOAD_STOPPED;
                Browser_SetStep(PAGE_STEP_RESHOW);
            }
        }
    }
        break;
    case 1:
        gBrowserState->step++;
        gBrowserState->page = PAGE_LOAD_PARSING;
        break;
    case 2:
        return;
    }
}

/* PAGE_STEP_PARSE: starts the parse of the page loaded (into the draw
 * commands), from an empty base URL. */
void Portal_ParsePage(void)
{
    Browser_UpdateFrame();
    Url_ClearBase();
    Html_ResetLoad();
    Html_StartLoad(gBrowserWork->pageData);
    gBrowserWork->formGoLink = 0;
    gBrowserState->styled = 0;
    gBrowserState->page = PAGE_LOAD_PARSING;
    gBrowserState->step++;
}

/* The page steps. */
void (*jtbl_007EEB20[16])(void) = {
    Portal_PageStart, Portal_LoadPage, Portal_ParsePage, Portal_ParseStep,
    Portal_PageReady, Portal_DialogStep, Portal_ReshowStep, Portal_HistoryStep,
    Portal_ReloadStep, Portal_OpenStep, Portal_BackStep, Portal_GameMenuStep,
    Portal_CancelStep, Portal_WaitReqStep, Portal_ImagesStep, Portal_FailStep,
};

/* PAGE_STEP_PARSING: parses the page a slice a frame (Html_LoadStep) and
 * shows the loading mark. While it parses, on a styled page that allows
 * the cancel, Start stops it (PAGE_LOAD_PARSE_STOPPED, shown again). When
 * the parse ends, the same data page twice in a row is retried; else, as
 * when it fails, the next step. */
void Portal_ParseStep(void)
{
    u16 *result;

    Browser_UpdateFrame();
    result = (u16 *)Html_LoadStep(0);
    if (result == NULL) {
        return;
    }
    switch (*result) {
    case 0:
        gBrowserState->page = PAGE_LOAD_PARSING;
        gBrowserWork->dlgBoxKind = BDLG_KIND_LOADING;
        if (gBrowserState->styled != 0 && ((gBrowserState->style & PAGE_STYLE_CANCEL) >> 5) != 0 &&
            (gBrowserWork->padPressed & PAD_START) && gBrowserWork->firstPage == 0 && gBrowserState->plain == 0) {
            gBrowserState->page = PAGE_LOAD_PARSE_STOPPED;
            Html_StopLoad();
            gBrowserState->mode = BMODE_RUN;
            gBrowserState->step = PAGE_STEP_RESHOW;
        }
        break;
    case 1:
        gBrowserState->page = PAGE_LOAD_PARSING_ON;
        gBrowserWork->dlgBoxKind = BDLG_KIND_LOADING_SMALL;
        break;
    case 2:
        gBrowserState->page = PAGE_LOAD_PARSED;
        switch (gBrowserWork->dlgBoxKind) {
        case 0:
        case BDLG_KIND_NONE:
        case BDLG_KIND_LOADING:
        case BDLG_KIND_LOADING_SMALL:
        case BDLG_KIND_CLOSE:
            gBrowserWork->dlgBoxKind = BDLG_KIND_LOADING;
            break;
        }
        if ((gBrowserState->prevPageMode == CSV_KIND_SESSION && gBrowserState->pageMode == CSV_KIND_SESSION) ||
            (gBrowserState->prevPageMode == CSV_KIND_ACCOUNT && gBrowserState->pageMode == CSV_KIND_ACCOUNT)) {
            Browser_Retry();
        } else {
            gBrowserWork->tries = 0;
            gBrowserState->step++;
        }
        break;
    case 3:
        gBrowserState->page = PAGE_LOAD_PARSED;
        gBrowserWork->dlgBoxKind = BDLG_KIND_LOADING;
        gBrowserState->step++;
        break;
    }
}

/* Keeps a scroll position at most max; one within it is pulled back by
 * back pixels. Written as a function of the field's address because the
 * game's code computes that address for the store alone. */
static inline void Page_ClampScroll(s32 *pos, s32 max, s32 back)
{
    if (max < *pos) {
        *pos = max;
    } else {
        *pos -= back;
    }
}

/* PAGE_STEP_READY: the page parsed. Waits while a dialog asks or the login
 * is saved. A data page's data is answered first (Portal_HandleData: 0
 * waits, 1 draws and waits, 2 goes on, -1 and -2 an error, shown by
 * Browser_GameStartError, -9 no data). Then the scroll bars, whether the
 * page has a background image to wait for, its images asked for, the
 * view on the URL's anchor, and PAGE_STEP_DIALOG, fading in unless the
 * page is one of the MMBB data's. */
void Portal_PageReady(void)
{
    u16 i;
    BrowserObj *obj;
    u8 *boxKind;

    if (gBrowserState->vThumbLen == 0) {
        gBrowserState->vThumbLen = 1;
    }
    if (gBrowserState->hThumbLen == 0) {
        gBrowserState->hThumbLen = 1;
    }
    gBrowserState->vThumbY = 16;
    gBrowserState->hThumbX = 16;
    if (gBrowserSub->dlgMode == BDLG_MODE_CHOICE || gBrowserSub->dlgMode == BDLG_MODE_SAVE_LOGIN) {
        Browser_UpdateFrame();
        return;
    }
    if (gBrowserState->pageMode != CSV_KIND_NONE) {
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
                gBrowserState->pageMode = CSV_KIND_NONE;
                gBrowserState->plain = 0;
                return;
            default:
                gBrowserState->pageMode = CSV_KIND_NONE;
                gBrowserState->plain = 0;
                break;
            }
            break;
        case -9:
            gBrowserState->pageMode = CSV_KIND_NONE;
            break;
        }
    }
    gBrowserWork->dlgBackImgState = BDLG_BACK_NONE;
    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        obj = gBrowserWork->obj[i];
        if (obj == NULL || !obj->active) {
            break;
        }
        if (obj->kind == PAGE_OBJ_IMAGE && obj->active == PAGE_IMG_BODY_BACK) {
            gBrowserWork->dlgBackImgState = BDLG_BACK_FIND;
        }
    }
    boxKind = &gBrowserWork->dlgBoxKind;
    switch (*boxKind) {
    case BDLG_KIND_LOADING:
    case BDLG_KIND_LOADING_SMALL:
        *boxKind = BDLG_KIND_NONE;
        break;
    }
    Page_RequestImages();
    if (strlen(gBrowserState->urlAnchor) != 0) {
        for (i = 0; i < PAGE_OBJ_COUNT; i++) {
            obj = gBrowserWork->obj[i];
            if (obj == NULL || !obj->active) {
                break;
            }
            if (obj->kind == PAGE_OBJ_ANCHOR && strcmp(obj->s1, gBrowserState->urlAnchor) == 0) {
                gBrowserState->x = gBrowserWork->obj[i]->x;
                gBrowserState->y = gBrowserWork->obj[i]->y;
                if (gBrowserState->x < 0) {
                    gBrowserState->x = 0;
                }
                Page_ClampScroll(&gBrowserState->x, gBrowserState->pageW - PAGE_VIEW_W, 20);
                if (gBrowserState->y < 0) {
                    gBrowserState->y = 0;
                }
                Page_ClampScroll(&gBrowserState->y, gBrowserState->pageH - PAGE_VIEW_H, 40);
                break;
            }
        }
    }
    Browser_UpdateFrame();
    gBrowserState->step++;
    gBrowserWork->firstPage = 0;
    if (gBrowserWork->atMmbbUrl == 0) {
        gBrowserState->fade = 16; /* fades in */
    }
}

/* PAGE_STEP_DIALOG: the page on show, with its dialogs. While its images
 * load, on a page that allows the cancel, Start with no dialog up stops
 * them (PAGE_LOAD_IMAGES_STOPPED, shown again); an image that fails takes
 * the page back. */
void Portal_DialogStep(void)
{
    BrowserState *state = gBrowserState;
    u8 *page = &state->page;

    if (*page == PAGE_LOAD_IMAGES && state->styleWait == 0) {
        if (((state->style & PAGE_STYLE_CANCEL) >> 5) != 0 && (gBrowserWork->padPressed & PAD_START) &&
            state->plain == 0 && gBrowserWork->dlgBoxKind == BDLG_KIND_NONE && gBrowserWork->dlgInputState == BDLG_INPUT_IDLE) {
            *page = PAGE_LOAD_IMAGES_STOPPED;
            Browser_SetStep(PAGE_STEP_RESHOW);
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

/* PAGE_STEP_RESHOW: after a stopped load, parse or image load, the page
 * is shown again once what was running has ended. */
void Portal_ReshowStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case PAGE_LOAD_REQUESTED:
    case PAGE_LOAD_STOPPED:
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
    case PAGE_LOAD_PARSE_STOPPED:
        BrowserHist_Undo();
        Browser_ShowPage();
        break;
    case PAGE_LOAD_IMAGES:
    case PAGE_LOAD_IMAGES_STOPPED:
        if (Page_CheckImages() != 0) {
            Browser_ShowPage();
        }
        break;
    }
}

/* PAGE_STEP_HISTORY: once the page is in, the history move asked for
 * (histDir: 1 back, 2 forward). Each of these steps checks the images
 * while they load. */
void Portal_HistoryStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case PAGE_LOAD_IMAGES:
        Page_CheckImages();
        break;
    case PAGE_LOAD_DONE:
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

/* PAGE_STEP_RELOAD: once the page is in, reloads it. */
void Portal_ReloadStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case PAGE_LOAD_IMAGES:
        Page_CheckImages();
        break;
    case PAGE_LOAD_DONE:
        Browser_Reload();
        break;
    }
}

/* PAGE_STEP_OPEN: once the page is in, posts to the URL asked for. */
void Portal_OpenStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case PAGE_LOAD_IMAGES:
        Page_CheckImages();
        break;
    case PAGE_LOAD_DONE:
        Browser_SetUrl(gBrowserWork, gBrowserState->url);
        BrowserReq_Post(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        Browser_OpenPage();
        break;
    }
}

/* PAGE_STEP_BACK: once the page is in, goes back. */
void Portal_BackStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case PAGE_LOAD_IMAGES:
        Page_CheckImages();
        break;
    case PAGE_LOAD_DONE:
        Portal_Back();
        break;
    }
}

/* PAGE_STEP_GAME_MENU: once the page is in, the browser ends for the
 * game's menu (PORTAL_EXIT_GAME_MENU). */
void Portal_GameMenuStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case PAGE_LOAD_IMAGES:
        Page_CheckImages();
        break;
    case PAGE_LOAD_DONE:
        gPortalExit = PORTAL_EXIT_GAME_MENU;
        Browser_Fail(PAGE_LOAD_NONE);
        break;
    }
}

/* PAGE_STEP_CANCEL: once the page is in, the browser ends, cancelled
 * (PORTAL_EXIT_CANCEL). */
void Portal_CancelStep(void)
{
    Browser_UpdateFrame();
    switch (gBrowserState->page) {
    case PAGE_LOAD_IMAGES:
        Page_CheckImages();
        break;
    case PAGE_LOAD_DONE:
        gPortalExit = PORTAL_EXIT_CANCEL;
        Browser_Fail(PAGE_LOAD_NONE);
        break;
    }
}

/* PAGE_STEP_WAIT_REQ: once the request still out has its reply, the leave
 * dialog. */
void Portal_WaitReqStep(void)
{
    if (BrowserReq_Poll(&gBrowserWork->url) != 0) {
        gBrowserWork->reqBusy = 0;
        Browser_OpenLeaveDialog(0);
        gBrowserSub->dlgCursor = 1;
        gBrowserState->step = PAGE_STEP_DIALOG;
    }
}

/* PAGE_STEP_IMAGES: once the page's images are in, the leave dialog. */
void Portal_ImagesStep(void)
{
    u8 page;

    page = gBrowserState->page;
    switch (page) {
    case PAGE_LOAD_IMAGES:
        Page_CheckImages();
        break;
    case PAGE_LOAD_DONE:
        Browser_OpenLeaveDialog(0);
        gBrowserSub->dlgCursor = 1;
        gBrowserState->step = PAGE_STEP_DIALOG;
        break;
    }
}

/* PAGE_STEP_FAIL: once the request is answered, the portal fails (BMODE_END). */
void Portal_FailStep(void)
{
    if (BrowserReq_Poll(&gBrowserWork->url) != NULL) {
        gBrowserWork->reqBusy = 0;
        Browser_Fail(0);
        gBrowserState->mode = BMODE_END;
        gBrowserState->step = PORTAL_END_START;
    }
}
