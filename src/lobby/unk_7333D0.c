/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

#include "browser.h"
#include "lobby_globals.h"

void *memset(void *dst, s32 c, u32 n);
u32 strlen(const char *s);
s32 memcmp(const char *a, const char *b, u32 n); /* main: strncmp */
s32 strcmp(const char *a, const char *b);        /* main: strcmp */
char *strncpy(char *dst, const char *src, u32 n); /* main: strncpy */
void Font_ResetState(void);
void PageObj_Free(BrowserObj *o);                    /* PageObj_Free */
void Html_ResetLoad(void);
void Html_StartLoad(char *src);
void Url_ClearBase(void);
void Browser_UpdateFrame(void);
/* the calls cast to s8: this unit's code takes the value as s8 */
s32 Page_Load(void);                               /* Page_Load */
char *BrowserHist_CurUrl(void);
void Browser_Go(char *url);
void Browser_Fail(u8 page);
void Browser_ShowPage(void);
void Browser_ShowList(s32 unused0);
void Browser_SetStep(s8 step);
void Browser_Reload(void);
void Browser_SetRenderState(s32 state, u32 value);
s32 Browser_Retry(void);
void Browser_LoadFrame(s32 mode, u8 *step);
void Page_CreateFrame(void);
void Dialog_Reset(void);
void BrowserReq_Page(char **url);
s32 Portal_SetIds(void);
void Browser_OpenPage(void);
extern void (*jtbl_007EEB20[])(void); /* the portal's page steps */

extern char gBrowserUrl[];     /* the start URL */
extern char gMmbbLoginId[];     /* sub_main: the MMBB login id kept in the NET file? */
extern char D_5BE6BB[];     /* sub_main: its password? */
extern char D_00839058[];   /* "lbs://" */

/* Page_FreeObjs: returns the page's objects to the pool. */
void Page_FreeObjs(void)
{
    u16 n;

    for (n = 0; n < 500; n++) {
        if (gBrowserWork->obj[n] == NULL) {
            return;
        }
        PageObj_Free(gBrowserWork->obj[n]);
        gBrowserWork->obj[n] = NULL;
    }
}

/* Portal mode 0, step 0: is the start URL one of the MMBB data's (f4 or
 * the next URL)? Then the draw commands from the start. */
void Portal_Start0(void)
{
    gBrowserWork->result = 0;
    if (strcmp(gBrowserUrl, gMmbbData->f4) == 0 || strcmp(gBrowserUrl, gMmbbData->nextUrl) == 0) {
        gBrowserWork->atMmbbUrl = 1;
    } else {
        gBrowserWork->atMmbbUrl = 0;
    }
    Font_ResetState();
    gBrowserWork->cmdPtr = gBrowserWork->cmdBuf;
    gBrowserState->step++;
    gBrowserState->sub = 0;
}

/* Portal mode 0, step 1: once the frame's texture is loaded (sub 4), the
 * first page: the start URL, the login fields filled from the NET file. */
void Portal_Start1(void)
{
    Browser_LoadFrame(3, &gBrowserState->sub);
    if (gBrowserState->sub == 4) {
        gBrowserState->sub = 0;
        Page_CreateFrame();
        gBrowserWork->dlgHScrollState = 2;
        gBrowserWork->dlgVScrollState = 2;
        gBrowserWork->dlgStatusState = 1;
        Browser_Go(gBrowserUrl);
        gBrowserState->pageH = 0;
        gBrowserState->y = 0;
        gBrowserState->prevPageMode = 0;
        gBrowserState->pageMode = 0;
        Dialog_Reset();
        memset(gBrowserState->loginId, 0, 0x11);
        memset(gBrowserState->password, 0, 0x11);
        memset(gBrowserState->field1, 0, 0x11);
        memset(gBrowserState->field2, 0, 0x11);
        memset(gBrowserState->loginIdSave, 0, 0x11);
        memset(gBrowserState->field3, 0, 0x11);
        gBrowserState->unk32 = 0;
        gBrowserState->menuReached = 0;
        strncpy((char *)gBrowserState->loginId, gMmbbLoginId, 0xA);
        strncpy((char *)gBrowserState->password, D_5BE6BB, 8);
        gBrowserState->page = 0;
        gBrowserWork->dlgInputState = 2;
        Browser_SetRenderState(0x14, 0xFF000001);
        Browser_Reload();
    }
}

/* Browser_RunPage: portal mode 1, the page's step gBrowserState->step. */
void Browser_RunPage(void)
{
    jtbl_007EEB20[gBrowserState->step]();
}

/* Portal_PageStart: portal mode 1, step 0 (jtbl_007EEB20[0]): opens the
 * page. The start URL, when it is one of the MMBB data's (atMmbbUrl), gets the
 * cookies first in the lobby's states 1 and 2; any other URL goes
 * to BrowserReq_Page first, with reqBusy set. */
void Portal_PageStart(void)
{
    Browser_UpdateFrame();
    if (gBrowserWork->atMmbbUrl != 0) {
        switch (gPortalPage) {
        case 2:
        case 1:
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

/* Portal mode 1, step 1: loads the page (Page_Load) and reacts to its
 * result: -2 no connection (the list), -1 an error (the lobby server's
 * page fails; else retried or the error page), 0 loading (cancel with
 * the cancel button), 1 loaded (next step). */
void Portal_LoadPage(void)
{
    if (gBrowserState->initMode != 0 && gBrowserState->fade < 0) {
        Browser_UpdateFrame();
        return;
    }
    gBrowserWork->dlgBoxKind = 2;
    Browser_UpdateFrame();
    switch ((s8)Page_Load()) {
    case -2:
        gBrowserWork->endReason = 2;
        Browser_ShowList(0);
        gBrowserSub->dlgCursor = 1;
        gBrowserState->step++;
        break;
    case -1:
        if (gBrowserWork->firstPage != 0 && memcmp(gBrowserState->url, D_00839058, strlen(D_00839058)) == 0) {
            Browser_Fail(0);
            break;
        }
        switch (gBrowserState->pageMode) {
        case 6:
        case 7:
        default:
            break;
        case 1:
            Browser_Retry();
            return;
        }
        Browser_Go(BrowserHist_CurUrl());
        Browser_ShowPage();
        break;
    case 0:
    {
        BrowserState *st = gBrowserState;

        if (((st->style & 0x20) >> 5) != 0) {
            BrowserWork *w = gBrowserWork;
            u16 pressed = w->padPressed;

            if (((pressed & 0x8000) || (pressed & 0x40)) && w->firstPage == 0 && st->plain == 0) {
                st->page = 2;
                Browser_SetStep(6);
            }
        }
    }
        break;
    case 1:
        gBrowserState->step++;
        gBrowserState->page = 3;
        break;
    case 2:
        return;
    }
}

/* Portal mode 1, step 2: parses the page loaded (into the draw commands). */
void Portal_ParsePage(void)
{
    Browser_UpdateFrame();
    Url_ClearBase();
    Html_ResetLoad();
    Html_StartLoad(gBrowserWork->pageData);
    gBrowserWork->formGoLink = 0;
    gBrowserState->styled = 0;
    gBrowserState->page = 3;
    gBrowserState->step++;
}

/* unk_7333D0's .data (0x7EEB20-0x7EEB60) */
void Portal_BackStep(void);
void Portal_CancelStep(void);
void Portal_DialogStep(void);
void Portal_FailStep(void);
void Portal_GameMenuStep(void);
void Portal_HistoryStep(void);
void Portal_OpenStep(void);
void Portal_PageReady(void);
void Portal_ParseStep(void);
void Portal_ReloadStep(void);
void Portal_ReshowStep(void);
void Portal_WaitReqStep(void);
void Portal_ImagesStep(void);
void (*jtbl_007EEB20[16])(void) = {
    Portal_PageStart, Portal_LoadPage, Portal_ParsePage, Portal_ParseStep,
    Portal_PageReady, Portal_DialogStep, Portal_ReshowStep, Portal_HistoryStep,
    Portal_ReloadStep, Portal_OpenStep, Portal_BackStep, Portal_GameMenuStep,
    Portal_CancelStep, Portal_WaitReqStep, Portal_ImagesStep, Portal_FailStep,
};
