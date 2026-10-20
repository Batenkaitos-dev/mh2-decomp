/* The browser's mode and step changes, going to a URL, scrolling.
 * include/browser.h. */
#include "common.h"
#include "browser.h"
#include "lobby_menu.h"
void LobbySe_Play(s32 se);

void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
void Browser_SetRenderState(s32 state, u32 value); /* Browser_SetRenderState */
void Browser_SetUrl(BrowserWork *w, char *url);
void BrowserReq_CancelAll(void);

void func_00740D10(void)
{
}

/* Browser_Reload? mode 1, step 0. */
void Browser_Reload(void)
{
    gBrowserSub->activeObj = 0;
    gBrowserWork->refreshTimer = 0;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = 0;
    gBrowserState->sub = 0;
}

/* Browser_OpenPage: mode 1, step 1 (page 1). */
void Browser_OpenPage(void)
{
    gBrowserState->histDir = 0;
    gBrowserState->page = PAGE_LOAD_REQUESTED;
    gBrowserSub->activeObj = 0;
    gBrowserWork->refreshTimer = 0;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = 1;
    gBrowserState->sub = 0;
    if (gBrowserWork->atMmbbUrl == 0) {
        gBrowserState->fade = -0x10;
    }
    if (gBrowserWork->obj[0] == NULL) {
        gBrowserState->fade = 0;
        if (gBrowserState->initMode != 0) {
            gBrowserState->bgColor = 0xFF000001;
        }
    }
}

/* mode 1, step 5, page 0xA */
void Browser_ShowPage(void)
{
    gBrowserState->page = PAGE_LOAD_DONE;
    gBrowserWork->dlgBoxKind = BDLG_KIND_NONE;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = 5;
}

/* Browser_Error? mode 2 with page. */
void Browser_Fail(u8 page)
{
    gBrowserState->page = page;
    Browser_SetRenderState(0x14, 0xFF000001);
    gBrowserState->mode = BMODE_END;
    gBrowserState->step = 0;
    gBrowserSub->dlgStep = 2;
}

/* mode 1, step 5 */
void Browser_ShowList(s32 unused0)
{
    gBrowserSub->dlgMode = BDLG_MODE_EXIT;
    gBrowserState->prevPage = 0;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = 5;
    gBrowserState->sub = 0;
    gBrowserWork->dlgBoxKind = BDLG_KIND_EXIT;
    gBrowserWork->dlgPointerState = BDLG_POINTER_HIDDEN;
}

/* Browser_SetStep: steps 0xB-0xF always, others only when nothing is
 * pending (unk38, unk3A). */
void Browser_SetStep(s8 step)
{
    switch (step) {
    default:
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
        BrowserReq_CancelAll();
        gBrowserState->step = step;
    }
}

/* Browser_Go: the page at url. */
void Browser_Go(char *url)
{
    memset(gBrowserState->url, 0, 0x100);
    strcpy(gBrowserState->url, url);
    Browser_SetUrl(gBrowserWork, gBrowserState->url);
}

/* Browser_ScrollBy */
void Browser_ScrollBy(s32 dx, s32 dy)
{
    gBrowserState->x += dx;
    gBrowserState->y += dy;
    gBrowserSub->listCount = 0;
}

/* Browser_Move: scrolls the page (kinds 1-4) or moves the selection
 * (5, 6). */
void Browser_Move(s32 dx, s32 dy, u8 kind)
{
    switch (kind) {
    case 1:
    case 2:
    case 3:
    case 4:
        gBrowserState->x += dx;
        gBrowserState->y += dy;
        break;
    case 5:
    case 6:
        gBrowserSub->listTop += (u8)dy;
        break;
    case 0:
        break;
    }
    gBrowserWork->pressTimer = 3;
    gBrowserSub->moveKind = kind;
}

/* Browser_Exit: mode 3 with the result. */
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
