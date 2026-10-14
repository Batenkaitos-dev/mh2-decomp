/* Going back to the start page, and the "game start error" page built in
 * memory. include/browser.h. */
#include "common.h"
#include "browser.h"

void *memcpy(void *dst, const void *src, u32 n);
u32 strlen(const char *s);
s32 strcmp(const char *a, const char *b); /* main: strcmp */
s32 Portal_SetIds(void);
void Browser_Reload(void);
void Browser_OpenPage(void);
void Browser_ShowList(s32 unused0);
void Browser_Go(char *url);                 /* Browser_Go */
s8 Portal_HandleData(void);

/* The URL the browser was started on (sub_main, just before the MMBB data). */
extern char gBrowserUrl[];
extern char gPortalUrlA[]; /* = gMmbbData->f4, addressed directly */

/* The error page's pieces (.data): its head, the end with and without a
 * link back to the menu ("mmbb://BACK_TO_MENU"), and "error number: ". */
extern char D_007EEC10[];
extern char D_007EED60[];
extern char D_007EEDA0[];
extern char D_007EEE10[];

/* Browser_GoStart: once (firstPage == 1), back to the start URL: a page
 * again when it is the MMBB data's f4, else a reload. */
s32 Browser_GoStart(void)
{
    u8 n;

    if (gBrowserWork->firstPage != 1) {
        return 0;
    }
    Browser_Go(gBrowserUrl);
    n = gBrowserWork->tries + 1;
    gBrowserWork->tries = n;
    if (n != 2) {
        if (strcmp(gBrowserUrl, gPortalUrlA) == 0) {
            Portal_SetIds();
            Browser_OpenPage();
        } else {
            Browser_Reload();
        }
        return 2;
    }
    return 0;
}

/* Browser_Retry: 1 while the lobby server's answer (Portal_HandleData) is 1
 * and this is not the second try; else back to step 5 with the list's
 * first entry. */
s32 Browser_Retry(void)
{
    BrowserWork *w = gBrowserWork;

    if (++w->tries != 2 && Portal_HandleData() == 1) {
        return 1;
    }
    gBrowserState->pageMode = 0;
    gBrowserWork->endReason = 3;
    Browser_ShowList(0);
    gBrowserSub->dlgCursor = 1;
    return 2;
}

/* Str_Append: copies n bytes to *p and moves *p to the copy's last byte
 * (the terminator of a string, for the next piece to overwrite). */
void *Str_Append(char **p, const void *src, s32 n)
{
    void *r = memcpy(*p, src, n);

    *p += n - 1;
    return r;
}

/* Browser_BuildErrorPage: the page shown when the game can't start,
 * with the message and the error code (and a link back to the menu). */
void Browser_BuildErrorPage(char *buf, char *msg, char *code, s32 back)
{
    char *p = buf;

    Str_Append(&p, D_007EEC10, 0x14C);
    Str_Append(&p, msg, strlen(msg) + 1);
    Str_Append(&p, D_007EEE10, 0x18);
    Str_Append(&p, code, strlen(code) + 1);
    if (back != 0) {
        Str_Append(&p, D_007EEDA0, 0x69);
        return;
    }
    Str_Append(&p, D_007EED60, 0x32);
}

/* unk_741690's .data (0x7EEC10-0x7EEE30) */
char D_007EEC10[0x150] = {
#include "lobby/007EEC10.inc"
};
char D_007EED60[0x40] = {
#include "lobby/007EED60.inc"
};
char D_007EEDA0[0x70] = {
#include "lobby/007EEDA0.inc"
};
char D_007EEE10[0x20] = {
#include "lobby/007EEE10.inc"
};
