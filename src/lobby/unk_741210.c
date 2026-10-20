/* A list's cursor (the pad, the left stick or a USB keyboard's arrows),
 * and the portal's error codes. include/browser.h. */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "kbd.h"

void LobbySe_Play(s32 se); /* Se_Play? */
u32 strlen(const char *s);
char *strcpy(char *dst, const char *src);
s32 strcmp(const char *a, const char *b);   /* main: strcmp */
s32 sprintf(char *dst, const char *fmt, ...); /* main: sprintf */
char *Browser_GetText(u32 group, s32 i);
void Browser_BuildErrorPage(char *buf, char *msg, char *code, s32 back); /* Browser_BuildErrorPage */
void Browser_Go(char *url);                   /* Browser_Go */
void Browser_Reload(void);

/* The USB keyboard's state (sub_main): +0x668 the key pressed (HID usage:
 * 0x4F right, 0x50 left, 0x51 down, 0x52 up). */

/* Browser_ListCursor: moves the cursor of a list of max entries (1..max),
 * with a sound when it moves. */
void Browser_ListCursor(u8 max)
{
    u32 pressed;
    u8 *cur = &gBrowserSub->dlgCursor;
    u8 old = *cur;
    u8 tilt;
    s32 key;
    s32 dir;

    pressed = gBrowserWork->padPressed;
    if ((pressed & 0x800) || (pressed & 0x2000) ||
        ((tilt = gBrowserWork->stickTilt) != 0 && (dir = gBrowserWork->stickDir) >= 8 && dir <= 15) ||
        (key = gKbd->usbKey) == 0x50 || key == USB_KEY_UP) {
        (*cur)--;
    } else if ((pressed & 0x400) || (pressed & 0x1000) ||
               (tilt && (dir = gBrowserWork->stickDir) > 0 && dir <= 7) || key == USB_KEY_RIGHT || key == USB_KEY_DOWN) {
        (*cur)++;
    }
    cur = &gBrowserSub->dlgCursor;
    if (*cur <= 0) {
        *cur = 1;
    } else if (max < *cur) {
        *cur = max;
    }
    if (old != gBrowserSub->dlgCursor) {
        LobbySe_Play(1);
    }
}

/* Browser_GameStartError: when the portal's page gave an error code (the
 * MMBB data's f6, "102-4204" ...), builds the error page in memory with
 * its message (group 6 of Browser_GetText) and shows it ("memory://000").
 * -1: no code; 0: nothing to do (data mode 1); 1: the page is shown. */
s8 Browser_GameStartError(void)
{
    char url[0x100];
    s8 i;

    if (strlen(gMmbbData->result) < 8) {
        return -1;
    }
    switch (gBrowserState->pageMode) {
    case CSV_KIND_SESSION:
        return 0;
    case CSV_KIND_SERVERS:
    case CSV_KIND_RESULT:
        if (strcmp(gMmbbData->result, "102-0002") == 0 || strcmp(gMmbbData->result, "103-0002") == 0) {
            i = 0;
        } else if (strcmp(gMmbbData->result, "102-4000") == 0 || strcmp(gMmbbData->result, "103-4000") == 0) {
            i = 0;
        } else if (strcmp(gMmbbData->result, "102-4201") == 0 || strcmp(gMmbbData->result, "103-4201") == 0) {
            i = 0;
        } else if (strcmp(gMmbbData->result, "102-4204") == 0 || strcmp(gMmbbData->result, "103-4204") == 0) {
            i = 1;
        } else if (strcmp(gMmbbData->result, "102-4205") == 0 || strcmp(gMmbbData->result, "103-4205") == 0) {
            i = 0;
        } else if (strcmp(gMmbbData->result, "102-4206") == 0 || strcmp(gMmbbData->result, "103-4206") == 0) {
            i = 2;
        } else if (strcmp(gMmbbData->result, "102-4200") == 0) {
            i = 3;
        } else if (strcmp(gMmbbData->result, "102-8001") == 0) {
            i = 5;
        } else {
            i = 6;
        }
        strcpy(gMmbbData->errorText, Browser_GetText(6, i));
        Browser_BuildErrorPage((char *)gBrowserWork->memPage[0], gMmbbData->errorText, gMmbbData->result, 1);
        sprintf(url, "memory://%03d", 0);
        Browser_Go(url);
        Browser_Reload();
        return 1;
    default:
        return -1;
    }
}
