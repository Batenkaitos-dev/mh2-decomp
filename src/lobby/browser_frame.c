/*
 * The browser's frame: its start and end, the frame it runs each frame,
 * and the lobby's own pages.
 * - Browser_Init sets up the work area (BrowserWork), its two heaps, the
 *   certificates, the caches and the page state. Under BROWSER_TOP_LOBBY
 *   it keeps the page objects and textures it has; otherwise it starts
 *   from an empty page. LobbyKddi_CloseBrowser frees it all.
 * - Browser_Update runs one frame: the pad, the requests, the check of the
 *   network and of the lobby's link (Browser_CheckNetwork), then the top
 *   mode's function (gBrowserTopModes, BROWSER_TOP_*). The portal's modes
 *   (gBrowserModes) and the lobby's pages' modes (their own table) are
 *   enum BrowserMode.
 * - The lobby's pages post the user id and the connection id to the MMBB
 *   data's next URL and wait for the reply (gBrowserLobbySteps, enum
 *   LobbyPageStep).
 * The URL scheme of the lobby's server (gLbsScheme) and the names of the
 * two id fields (gIdFieldNames) are .rodata of this file that the portal's
 * page steps use too.
 */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "kbd.h"
#include "net_mode.h"
#include "nethttp.h"
#include "text.h"
#include "draw_helpers.h"
#include "font.h"
#include "frame.h"
#include "main.h"
#include "mem_util.h"
#include "libkernl.h"

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void Page_SetMargins(s32 a, s32 b); /* unit's own declaration: the definition takes three
 * arguments and reads none; Browser_Init passes two, and with a third the call compiles differently */
extern u8 gBrowserWorkMem[];                 /* the work area */
extern MmbbData gAccountData;               /* sub_main: the MMBB data */
extern u8 gLobbyPageData[];                 /* the registration record kept */
int strncmp(const char *, const char *, unsigned int);
u32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, u32 n);
s32 BrowserForm_Add(const char *name, char *value);
extern u8 gConnCheckState;                   /* the lobby's link, lost while a page shows: CONN_LOST_* */
extern char gBrowserUrl[];                 /* the start URL */
/* The mode tables: the top modes' (BROWSER_TOP_*), the portal's and the
 * lobby's pages' modes (enum BrowserMode), and the lobby's pages' steps. */
extern void (*gBrowserTopModes[])(void);
extern void (*gBrowserModes[])(void);
extern void (*gBrowserLobbySteps[])(void);
/* The lobby's pages' modes. The table keeps its address name: the port's
 * recompiler takes a table it calls through as a jump table, and finds it
 * by that name. */
extern void (*jtbl_007EEAE0[])(void);
extern char gBrowserErrorMsg[];              /* the debug message Browser_RunError prints */

/*
 * Starts the browser: the work area and its two heaps (64 KB and 512 KB),
 * the two certificates (read from the browser's data files into 4 KB
 * that start 10240 bytes below gMemAreas), the caches and the HTTP
 * layer. Under
 * BROWSER_TOP_LOBBY only the page state is cleared: the page objects and
 * textures stay. Otherwise the page starts empty, with the default style
 * and margins, a black background and the registration record kept; the
 * MMBB data is cleared when the portal opens at its top page. mode is
 * kept as BrowserState.initMode (1 for the lobby's web pages).
 */
void Browser_Init(u8 mode)
{
    u8 *certBuf;
    u16 i;

    NetOverlay_Request(0);
    gBrowserWork = (BrowserWork *)gBrowserWorkMem;
    memset(gBrowserWork, 0, sizeof(BrowserWork));
    Http_LoadAfsPartition();
    certBuf = gMemAreas - 10240;
    memset(certBuf, 0, 2 * BROWSER_CERT_SIZE);
    gBrowserWork->cert0 = certBuf;
    gBrowserWork->cert1 = certBuf + BROWSER_CERT_SIZE;
    Http_ReadBrdata(gBrowserWork->cert0, BRDATA_CERT0);
    Http_ReadBrdata(gBrowserWork->cert1, BRDATA_CERT1);
    gBrowserWork->reqBusy = 0;
    gKbd->usbKey = 0;
    gKbd->usbPrevKey = 0;
    gBrowserWork->dlgQuestion = 0;
    gBrowserWork->mem64k = Mem_Alloc(BROWSER_SMALL_BUF_SIZE, 1);
    gBrowserWork->buf64k = Mem_GetAddr(gBrowserWork->mem64k);
    memset(gBrowserWork->buf64k, 0, BROWSER_SMALL_BUF_SIZE);
    gBrowserWork->mem512k = Mem_Alloc(BROWSER_BUF_SIZE, 1);
    gBrowserWork->buf512k = Mem_GetAddr(gBrowserWork->mem512k);
    memset(gBrowserWork->buf512k, 0, BROWSER_BUF_SIZE);
    gBrowserWork->endGoOn = 0;
    if (gBrowserTopMode == BROWSER_TOP_LOBBY) {
        BrowserCache_Init();
        Html_ResetLoad();
        Http_Init();
        memset(&gBrowserWork->state, 0, sizeof(BrowserState));
        gBrowserWork->tries = 0;
        gBrowserWork->endReason = 0;
        gBrowserWork->netError = 0;
        gMmbbData = &gAccountData;
        gBrowserState = &gBrowserWork->state;
        memset(&gBrowserWork->sub, 0, sizeof(BrowserSub));
        gBrowserSub = &gBrowserWork->sub;
        return;
    }
    BrowserCache_Init();
    Html_ResetLoad();
    PageTex_ReleaseAll();
    PageObj_InitPool();
    Http_Init();
    memset(&gBrowserWork->state, 0, sizeof(BrowserState));
    if (gPortalPage == PORTAL_PAGE_TOP) {
        memset(&gAccountData, 0, sizeof(MmbbData));
    }
    memset(&gBrowserWork->sub, 0, sizeof(BrowserSub));
    gMmbbData = &gAccountData;
    gBrowserState = &gBrowserWork->state;
    gBrowserSub = &gBrowserWork->sub;
    Font_ResetState();
    Browser_InitHook();
    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        gBrowserWork->obj[i] = NULL;
    }
    gBrowserWork->tries = 0;
    gBrowserWork->texFailOnce = 1;
    gBrowserWork->startPosts = 0;
    gBrowserWork->firstPage = 1;
    gBrowserState->style = HTML_PAGE_STYLE_DEFAULT;
    if (mode == 1) {
        gBrowserState->initMode = 1;
        gBrowserState->bgColor = BROWSER_BLACK;
    } else {
        gBrowserState->initMode = 0;
        gBrowserState->bgColor = BROWSER_BLACK;
    }
    Page_SetMargins(0, 0);
    gBrowserWork->loginPosted = 0;
    gBrowserWork->gameMenuNo = 0;
    gBrowserWork->focus = 0;
    gBrowserWork->endReason = 0;
    gBrowserWork->netError = 0;
    gBrowserWork->formGoLink = 0;
    gBrowserWork->pageData = 0;
    gBrowserWork->texCount = 0;
    gBrowserWork->freedObjs = 0;
    memset(gBrowserWork->texHandle, 0, sizeof(gBrowserWork->texHandle));
    memset(gBrowserWork->texW, 0, sizeof(gBrowserWork->texW));
    memset(gBrowserWork->texH, 0, sizeof(gBrowserWork->texH));
    memset(gBrowserWork->cmdBuf, 0, sizeof(gBrowserWork->cmdBuf));
    gBrowserWork->cmdPtr = gBrowserWork->cmdBuf;
    gBrowserWork->result = 0;
    memcpy(&gRegRecord, gLobbyPageData, sizeof(gRegRecord));
    gBrowserWork->exitArgs = 0;
    gBrowserWork->memPage[0] = gBrowserWork->errorPage;
    gBrowserWork->memPage[1] = 0;
    gBrowserWork->memPage[2] = 0;
}

/* Ends the browser: the caches, the HTTP layer and the two heaps; then
 * loads the game's sub-overlay back. */
void LobbyKddi_CloseBrowser(void)
{
    BrowserCache_Shutdown();
    Http_Term();
    Mem_Free(gBrowserWork->mem512k);
    Mem_Free(gBrowserWork->mem64k);
    Mem_Compact();
    Overlay_LoadGmSub();
}

/*
 * Answers the lobby's link found lost (CONN_LOST_FOUND) while a page
 * shows: a page still requested goes to its failure step, a page loading
 * its images to the cancel step, any other to the end with the failure
 * page; then CONN_LOST_SHOWN.
 */
void Browser_CheckConnection(void)
{
    switch (gConnCheckState) {
    case CONN_LOST_NONE:
        break;
    case CONN_LOST_FOUND:
        switch (gBrowserState->page) {
        case PAGE_LOAD_REQUESTED:
            Browser_SetStep(PAGE_STEP_FAIL);
            break;
        case PAGE_LOAD_IMAGES:
            Browser_SetStep(PAGE_STEP_CANCEL);
            break;
        default:
            Browser_Fail(PAGE_LOAD_NONE);
            gBrowserState->mode = BMODE_END;
            gBrowserState->step = 0;
            break;
        }
        gConnCheckState = CONN_LOST_SHOWN;
        break;
    case CONN_LOST_SHOWN:
    case CONN_LOST_ENDED:
        break;
    }
}

/*
 * Each frame of a running page (BMODE_RUN), unless the login is being
 * saved: answers the lobby's link found lost (Browser_CheckConnection),
 * or, the first time the network interface reports an error (1 or 2),
 * ends the browser on it, unless the current URL is the lobby server's.
 * The portal goes to the step waiting for its request or its images, or
 * opens the leave dialog; the lobby's pages wait for the reply they are
 * waiting for, or end with BROWSER_FAILED.
 */
void Browser_CheckNetwork(void)
{
    if (gBrowserSub->dlgMode == BDLG_MODE_SAVE_LOGIN) {
        return;
    }
    if (gBrowserState->mode != BMODE_RUN) {
        return;
    }
    if (gConnCheckState != CONN_LOST_NONE) {
        Browser_CheckConnection();
        return;
    }
    if (gBrowserWork->netError != 0) {
        return;
    }
    switch (NetIf_GetError()) {
    case 1:
    case 2:
        gBrowserWork->netError = 1;
        gBrowserWork->endGoOn = 0;
        if (strncmp(gBrowserUrl, gLbsScheme, strlen(gLbsScheme)) == 0) {
            return;
        }
        gBrowserWork->endReason = BROWSER_END_NET_ERROR;
        gBrowserWork->dlgInputState = BDLG_INPUT_IDLE;
        gBrowserWork->tries = 2; /* no retry */
        switch (gBrowserTopMode) {
        case BROWSER_TOP_PORTAL:
            if (gBrowserWork->reqBusy != 0 && gBrowserState->page == PAGE_LOAD_REQUESTED) {
                Browser_SetStep(PAGE_STEP_WAIT_REQ);
                return;
            }
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                Browser_SetStep(PAGE_STEP_IMAGES);
                return;
            }
            Browser_OpenLeaveDialog(0);
            gBrowserSub->dlgCursor = 1;
            return;
        case BROWSER_TOP_LOBBY:
            if (gBrowserState->step == LOBBY_PAGE_WAIT) {
                BrowserReq_CancelAll();
                gBrowserState->mode = BMODE_RUN;
                gBrowserState->step = LOBBY_PAGE_END;
                return;
            }
            gBrowserWork->result = BROWSER_FAILED;
            gBrowserState->mode = BMODE_DONE;
            break;
        }
        break;
    case 0:
    case 3:
        break;
    }
}

/* Runs one frame of the browser; returns its result (BROWSER_RUNNING while
 * it runs). */
s32 Browser_Update(void)
{
    Browser_ReadPad();
    BrowserReq_UpdateAll();
    Browser_CheckNetwork();
    gBrowserTopModes[gBrowserTopMode]();
    Text_Reset();
    return gBrowserWork->result;
}

/* BROWSER_TOP_PORTAL's frame: the portal's mode, then the dialogs. */
void Browser_RunPortal(void)
{
    Callbacks_Clear();
    gBrowserModes[gBrowserState->mode]();
    Dialog_Run();
}

/* BROWSER_TOP_LOBBY's frame: the lobby's pages' mode. */
void Browser_RunLobby(void)
{
    jtbl_007EEAE0[gBrowserState->mode]();
}

/* The lobby's pages' BMODE_START: clears the form's fields, then
 * BMODE_RUN from its first step. */
void Browser_LobbyInit(void)
{
    BrowserForm_Clear();
    gBrowserState->mode++;
    gBrowserState->step = 0;
    gBrowserWork->result = 0;
}

/* The lobby's pages' BMODE_RUN: the step's function, then the loading
 * mark. */
void Browser_RunLobbyStep(void)
{
    gBrowserLobbySteps[gBrowserState->step]();
    Loading_Request();
}

/* LOBBY_PAGE_POST: with the MMBB data's user id, connection id and next
 * URL all set, posts the two ids as form fields to that URL and waits
 * (LOBBY_PAGE_WAIT); else BMODE_END. */
void Browser_LobbyStart(void)
{
    if (*(u8 *)gMmbbData->userId != 0 && BrowserForm_Add(gIdFieldNames.name.userId, gMmbbData->userId) >= 0 &&
        *(u8 *)gMmbbData->connectionId != 0 &&
        BrowserForm_Add(gIdFieldNames.name.connectionId, gMmbbData->connectionId) >= 0 && *(u8 *)gMmbbData->nextUrl != 0) {
        Browser_SetUrl(gBrowserWork, gMmbbData->nextUrl);
        BrowserReq_Post(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        gBrowserWork->result = 0;
        gBrowserState->step = LOBBY_PAGE_WAIT;
        return;
    }
    gBrowserState->mode = BMODE_END;
}

/* The mode and step tables. */
void (*gBrowserTopModes[4])(void) = {
    Browser_RunPortal, Browser_RunLobby, Browser_RunError, NULL,
};
void (*gBrowserModes[4])(void) = {
    Browser_RunStart, Browser_RunPage, Browser_RunEnd, Browser_ModeDone,
};
void (*jtbl_007EEAE0[4])(void) = {
    Browser_LobbyInit, Browser_RunLobbyStep, Browser_LobbyExit, Browser_ModeDone,
};
void (*gBrowserLobbySteps[8])(void) = {
    Browser_LobbyStart, Browser_LobbyStep1, Browser_LobbyStep2, Browser_LobbyStep3,
    Browser_LobbyStep4, Browser_LobbyWait, Browser_LobbyEnd, NULL,
};

/* The lobby server's URL scheme and the id fields' names. */
#pragma rodata on
char gLbsScheme[8] = {
#include "lobby/00839058.inc"
};
IdFieldNames gIdFieldNames = {
#include "lobby/00839060.inc"
};
#pragma rodata off

/* The lobby's pages' steps 1 to 4: each goes on to the next. */
void Browser_LobbyStep1(void)
{
    gBrowserState->step++;
}

void Browser_LobbyStep2(void)
{
    gBrowserState->step++;
}

void Browser_LobbyStep3(void)
{
    gBrowserState->step++;
}

void Browser_LobbyStep4(void)
{
    gBrowserState->step++;
}

/* LOBBY_PAGE_WAIT: the reply. A server error starts over once (a second
 * one ends); any other reply goes to BMODE_END. */
void Browser_LobbyWait(void)
{
    BrowserResult *reply = BrowserReq_Poll(&gBrowserWork->url);

    if (reply != NULL) {
        gBrowserWork->reqBusy = 0;
        switch (reply->result) {
        case BRES_SERVER_ERROR:
            if (++gBrowserWork->tries != 2) {
                gBrowserWork->result = BROWSER_RUNNING;
                gBrowserState->mode = BMODE_START;
                gBrowserState->step = 0;
            } else {
                gBrowserState->mode = BMODE_END;
            }
            break;
        case BRES_OK:
            gBrowserState->mode = BMODE_END;
            break;
        default:
            gBrowserState->mode = BMODE_END;
            break;
        }
    }
}

/* LOBBY_PAGE_END: once the reply comes, the browser ends with
 * BROWSER_FAILED. */
void Browser_LobbyEnd(void)
{
    if (BrowserReq_Poll(&gBrowserWork->url) != NULL) {
        gBrowserWork->reqBusy = 0;
        gBrowserWork->result = BROWSER_FAILED;
        gBrowserState->mode = BMODE_DONE;
    }
}

/* The lobby's pages' BMODE_END: the browser ends with BROWSER_DONE, or
 * BROWSER_LOGIN after the quit menu chose the portal (PORTAL_PAGE_QUIT). */
void Browser_LobbyExit(void)
{
    Browser_Exit(BROWSER_DONE);
    if (gPortalPage == PORTAL_PAGE_QUIT) {
        gBrowserWork->result = BROWSER_LOGIN;
    }
}

/* BROWSER_TOP_ERROR's frame: prints its debug message forever. */
void Browser_RunError(void)
{
    for (;;) {
        scePrintf(gBrowserErrorMsg);
    }
}

/* BMODE_DONE, of the portal and of the lobby's pages: nothing, once the
 * browser is done. */
void Browser_ModeDone(void)
{
}

/* Browser_RunError's message. */
#pragma rodata on
char gBrowserErrorMsg[24] = {
#include "lobby/00839080.inc"
};
#pragma rodata off
