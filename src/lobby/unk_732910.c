/* The browser's frame: Browser_Update, the top modes' dispatchers and the
 * lobby server check. include/browser.h. */
#include "common.h"
#include "browser.h"
int strncmp(const char *, const char *, unsigned int);

u32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, u32 n);
void Callbacks_Clear(void);
void Text_Reset(void);
void Loading_Request(void);
s32 NetIf_GetError(void);                  /* sub_main: the network state? */
void Browser_ReadPad(void);
void BrowserReq_UpdateAll(void);
void Dialog_Run(void);
void BrowserReq_CancelAll(void);
void Browser_SetStep(s8 step);
void Browser_Fail(u8 page);
void Browser_ShowList(s32 unused0);
void Browser_SetUrl(BrowserWork *w, char *url);
s32 BrowserForm_Add(const char *name, char *value); /* Cookie_Set? */
void BrowserReq_Post(char **url);

extern u8 gConnCheckState;                   /* the lobby connection's state */

/* Strings that the rest of this unit (to 0x740D10) uses too, in .rodata
 * before unk_732E90.c's: "lbs://", "user-id", "connection-id". They stay
 * in the data assembly while their other users are assembly. */
extern char D_00839058[];
extern char D_00839060[];
extern char D_00839068[];
extern char gBrowserUrl[];                 /* the start URL */

/* The mode tables (.data): the top mode's (gBrowserTopMode: 0 the portal, 1 the
 * lobby's pages, 2 an error), the portal's modes (gBrowserState->mode) and
 * the lobby pages' steps. */
extern void (*gBrowserTopModes[])(void);
extern void (*gBrowserModes[])(void);
extern void (*gBrowserLobbySteps[])(void);
extern void (*jtbl_007EEAE0[])(void); /* the lobby pages' modes */
void BrowserForm_Clear(void);

/* Browser_CheckConnection: while the lobby connection is being checked
 * (gConnCheckState == 1), the page's step (0xF, 0xC) or a failure; then 2. */
void Browser_CheckConnection(void)
{
    switch (gConnCheckState) {
    case 0:
        break;
    case 1:
        switch (gBrowserState->page) {
        case 1:
            Browser_SetStep(0xF);
            break;
        case 8:
            Browser_SetStep(0xC);
            break;
        default:
            Browser_Fail(0);
            gBrowserState->mode = 2;
            gBrowserState->step = 0;
            break;
        }
        gConnCheckState = 2;
        break;
    case 2:
    case 3:
        break;
    }
}

/* Browser_CheckNetwork: when the network is lost (NetIf_GetError 1 or 2)
 * while a page is shown, and the page isn't the lobby server's
 * ("lbs://"), the browser shows its error (step 0xD or 0xE) or leaves. */
void Browser_CheckNetwork(void)
{
    if (gBrowserSub->dlgMode == 4) {
        return;
    }
    if (gBrowserState->mode != 1) {
        return;
    }
    if (gConnCheckState != 0) {
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
        if (strncmp(gBrowserUrl, D_00839058, strlen(D_00839058)) == 0) {
            return;
        }
        gBrowserWork->endReason = 1;
        gBrowserWork->dlgInputState = 2;
        gBrowserWork->tries = 2;
        switch (gBrowserTopMode) {
        case 0:
            if (gBrowserWork->reqBusy != 0 && gBrowserState->page == 1) {
                Browser_SetStep(0xD);
                return;
            }
            if (gBrowserState->page == 8) {
                Browser_SetStep(0xE);
                return;
            }
            Browser_ShowList(0);
            gBrowserSub->dlgCursor = 1;
            return;
        case 1:
            if (gBrowserState->step == 5) {
                BrowserReq_CancelAll();
                gBrowserState->mode = 1;
                gBrowserState->step = 6;
                return;
            }
            gBrowserWork->result = -1;
            gBrowserState->mode = 3;
            break;
        }
        break;
    case 0:
    case 3:
        break;
    }
}

/* Browser_Update: one frame of the browser; its result (0 while it runs). */
s32 Browser_Update(void)
{
    Browser_ReadPad();
    BrowserReq_UpdateAll();
    Browser_CheckNetwork();
    gBrowserTopModes[gBrowserTopMode]();
    Text_Reset();
    return gBrowserWork->result;
}

/* Browser_RunPortal: top mode 0, the portal's mode gBrowserState->mode. */
void Browser_RunPortal(void)
{
    Callbacks_Clear();
    gBrowserModes[gBrowserState->mode]();
    Dialog_Run();
}

/* Browser_RunLobby: top mode 1, the lobby pages' mode gBrowserState->mode. */
void Browser_RunLobby(void)
{
    jtbl_007EEAE0[gBrowserState->mode]();
}

/* Browser_LobbyInit: the lobby pages' mode 0 (jtbl_007EEAE0[0]): clears
 * the request buffer, then mode 1 from step 0. */
void Browser_LobbyInit(void)
{
    BrowserForm_Clear();
    gBrowserState->mode++;
    gBrowserState->step = 0;
    gBrowserWork->result = 0;
}

/* Browser_RunLobbyStep: top mode 1, mode 1: the step's function. */
void Browser_RunLobbyStep(void)
{
    gBrowserLobbySteps[gBrowserState->step]();
    Loading_Request();
}

/* Browser_LobbyStart: with the MMBB data's user-id, connection-id (both
 * set as cookies?) and next URL, goes to that URL (step 5); else mode 2. */
void Browser_LobbyStart(void)
{
    if (*(u8 *)gMmbbData->userId != 0 && BrowserForm_Add(D_00839060, gMmbbData->userId) >= 0 &&
        *(u8 *)gMmbbData->connectionId != 0 &&
        BrowserForm_Add(D_00839068, gMmbbData->connectionId) >= 0 && *(u8 *)gMmbbData->nextUrl != 0) {
        Browser_SetUrl(gBrowserWork, gMmbbData->nextUrl);
        BrowserReq_Post(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        gBrowserWork->result = 0;
        gBrowserState->step = 5;
        return;
    }
    gBrowserState->mode = 2;
}

/* unk_732910's .data (0x7EEAC0-0x7EEB10) */
void Browser_LobbyEnd(void);
void Browser_LobbyExit(void);
void Browser_LobbyStep1(void);
void Browser_LobbyStep2(void);
void Browser_LobbyStep3(void);
void Browser_LobbyStep4(void);
void Browser_LobbyWait(void);
void Browser_ModeDone(void);
void Browser_RunEnd(void);
void Browser_RunError(void);
void Browser_RunPage(void);
void Browser_RunStart(void);
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

/* unk_732910's .rodata (0x839058-0x839080) */
#pragma rodata on
char D_00839058[0x8] = {
#include "lobby/00839058.inc"
};
char D_00839060[0x20] = {
#include "lobby/00839060.inc"
};
#pragma rodata off
