/* The lobby pages' steps (top mode 1): waiting for the page's answer.
 * include/browser.h. */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"

BrowserResult *BrowserReq_Poll(char **url);
void Browser_Exit(s8 result);
void scePrintf(const char *fmt, ...);

extern char D_00839080[]; /* "BsGetterMode: error!" */

/* Lobby steps 1 to 4: on to the next. */
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

/* Lobby step 5: the page's answer: 14 retries once from the start, other
 * answers end in mode 2. */
void Browser_LobbyWait(void)
{
    BrowserResult *r = BrowserReq_Poll(&gBrowserWork->url);

    if (r != NULL) {
        gBrowserWork->reqBusy = 0;
        switch (r->code) { /* 0 failed, 14 retry ... */
        case 14:
            if (++gBrowserWork->tries != 2) {
                gBrowserWork->result = 0;
                gBrowserState->mode = 0;
                gBrowserState->step = 0;
            } else {
                gBrowserState->mode = 2;
            }
            break;
        case 0:
            gBrowserState->mode = 2;
            break;
        default:
            gBrowserState->mode = 2;
            break;
        }
    }
}

/* Lobby step 6: once answered, the end (result -1). */
void Browser_LobbyEnd(void)
{
    if (BrowserReq_Poll(&gBrowserWork->url) != NULL) {
        gBrowserWork->reqBusy = 0;
        gBrowserWork->result = -1;
        gBrowserState->mode = 3;
    }
}

/* The lobby pages' mode 2: the end, result 1 (-100 in the lobby's state 3). */
void Browser_LobbyExit(void)
{
    Browser_Exit(1);
    if (gPortalPage == 3) {
        gBrowserWork->result = -100;
    }
}

/* Browser_RunError: the top mode 2 (gBrowserTopModes[2]), an error, forever. */
void Browser_RunError(void)
{
    for (;;) {
        scePrintf(D_00839080);
    }
}

/* Browser_ModeDone: the last mode of the portal and of the lobby pages
 * (gBrowserModes[3], jtbl_007EEAE0[3]): nothing, once the browser is done. */
void Browser_ModeDone(void)
{
}

/* unk_732E90's .rodata (0x839080-0x839098) */
#pragma rodata on
char D_00839080[0x18] = {
#include "lobby/00839080.inc"
};
#pragma rodata off
