/*
 * The browser's error page and the moves around it: the error page shown
 * when the portal's page answers with a result code; going back to the
 * start page after the first page failed; the retry of a data page's
 * reply; and the error page itself, built in memory page 0 from its pieces
 * (its head, the message, the code's label, the code, its end) and opened
 * as memory://000.
 */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"

u32 strlen(const char *s);
char *strcpy(char *dst, const char *src);
s32 strcmp(const char *a, const char *b);
s32 sprintf(char *dst, const char *fmt, ...);
void *memcpy(void *dst, const void *src, u32 n);

/* The URL the browser was started on. */
extern char gBrowserUrl[];
extern MmbbData gAccountData; /* the MMBB data (gMmbbData points to it) */

/* The error page's pieces: its head, its end without and with a link back
 * to the portal's menu, and the label before the error code. */
extern char gErrorPageHead[];
extern char gErrorPageEnd[];
extern char gErrorPageEndBack[];
extern char gErrorPageCodeLabel[];

/* A result code is at least this long (the error codes are "NNN-NNNN"). */
#define RESULT_CODE_MIN_LEN 8

/* When the portal's page gave a result code (the MMBB data's result, 8
 * characters or more), builds an error page in memory page 0 with the
 * code's message (a BTEXT_CONNECT_FAIL text, chosen by the code) and shows
 * it. Returns -1 without a code or on another kind of page, 0 on a session
 * page (CSV_KIND_SESSION), 1 when the error page is shown. */
s8 Browser_GameStartError(void)
{
    char url[BROWSER_URL_LEN];
    s8 msg;

    if (strlen(gMmbbData->result) < RESULT_CODE_MIN_LEN) {
        return -1;
    }
    switch (gBrowserState->pageMode) {
    case CSV_KIND_SESSION:
        return 0;
    case CSV_KIND_SERVERS:
    case CSV_KIND_RESULT:
        if (strcmp(gMmbbData->result, "102-0002") == 0 || strcmp(gMmbbData->result, "103-0002") == 0) {
            msg = 0;
        } else if (strcmp(gMmbbData->result, "102-4000") == 0 || strcmp(gMmbbData->result, "103-4000") == 0) {
            msg = 0;
        } else if (strcmp(gMmbbData->result, "102-4201") == 0 || strcmp(gMmbbData->result, "103-4201") == 0) {
            msg = 0;
        } else if (strcmp(gMmbbData->result, "102-4204") == 0 || strcmp(gMmbbData->result, "103-4204") == 0) {
            msg = 1;
        } else if (strcmp(gMmbbData->result, "102-4205") == 0 || strcmp(gMmbbData->result, "103-4205") == 0) {
            msg = 0;
        } else if (strcmp(gMmbbData->result, "102-4206") == 0 || strcmp(gMmbbData->result, "103-4206") == 0) {
            msg = 2;
        } else if (strcmp(gMmbbData->result, "102-4200") == 0) {
            msg = 3;
        } else if (strcmp(gMmbbData->result, "102-8001") == 0) {
            msg = 5;
        } else {
            msg = 6;
        }
        strcpy(gMmbbData->errorText, Browser_GetText(BTEXT_CONNECT_FAIL, msg));
        Browser_BuildErrorPage((char *)gBrowserWork->memPage[0], gMmbbData->errorText, gMmbbData->result, 1);
        sprintf(url, "memory://%03d", 0);
        Browser_Go(url);
        Browser_Reload();
        return 1;
    default:
        return -1;
    }
}

/* After the first page failed (firstPage 1), goes back to the start URL
 * (gBrowserUrl): opens it with the ids set again when it is the MMBB data's
 * session URL, else reloads it, and returns 2. Returns 0 on any other page,
 * or when this was the second try. */
s32 Browser_GoStart(void)
{
    u8 tryCount;

    if (gBrowserWork->firstPage != 1) {
        return 0;
    }
    Browser_Go(gBrowserUrl);
    tryCount = gBrowserWork->tries + 1;
    gBrowserWork->tries = tryCount;
    if (tryCount != 2) {
        if (strcmp(gBrowserUrl, gAccountData.sessionUrl) == 0) {
            Portal_SetIds();
            Browser_OpenPage();
        } else {
            Browser_Reload();
        }
        return 2;
    }
    return 0;
}

/* Handles the page's data again (Portal_HandleData): 1 when that worked
 * and this isn't the second try. Otherwise ends with
 * BROWSER_END_RETRY_FAILED: the leave dialog, its cursor on the first
 * line; returns 2. */
s32 Browser_Retry(void)
{
    BrowserWork *work = gBrowserWork;

    if (++work->tries != 2 && Portal_HandleData() == 1) {
        return 1;
    }
    gBrowserState->pageMode = CSV_KIND_NONE;
    gBrowserWork->endReason = BROWSER_END_RETRY_FAILED;
    Browser_OpenLeaveDialog(0);
    gBrowserSub->dlgCursor = 1;
    return 2;
}

/* Copies n bytes to *cursor and moves *cursor to the copy's last byte (a
 * string's terminator, which the next piece overwrites). Returns memcpy's
 * result. */
void *Str_Append(char **cursor, const void *src, s32 n)
{
    void *copy = memcpy(*cursor, src, n);

    *cursor += n - 1;
    return copy;
}

/* Builds the error page in buf: the head, the message, the code's label,
 * the error code, and the end, with a link back to the portal's menu when
 * `back` is set. Each piece's length counts its terminator. */
void Browser_BuildErrorPage(char *buf, char *msg, char *code, s32 back)
{
    char *out = buf;

    Str_Append(&out, gErrorPageHead, 332);
    Str_Append(&out, msg, strlen(msg) + 1);
    Str_Append(&out, gErrorPageCodeLabel, 24);
    Str_Append(&out, code, strlen(code) + 1);
    if (back != 0) {
        Str_Append(&out, gErrorPageEndBack, 105);
        return;
    }
    Str_Append(&out, gErrorPageEnd, 50);
}

/* The error page's pieces. */
char gErrorPageHead[336] = {
#include "lobby/007EEC10.inc"
};
char gErrorPageEnd[64] = {
#include "lobby/007EED60.inc"
};
char gErrorPageEndBack[112] = {
#include "lobby/007EEDA0.inc"
};
char gErrorPageCodeLabel[32] = {
#include "lobby/007EEE10.inc"
};
