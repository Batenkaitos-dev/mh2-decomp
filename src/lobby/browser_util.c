/*
 * The browser's support: the page's HTML parsed in time slices
 * (Html_LoadStep), the browser's message tables (Browser_GetText, its
 * groups BTEXT_*), the page heap (an Arena on the work area's 512 KB
 * buffer), and the pad and sticks read each frame into the work area
 * (Browser_ReadPad). Then the message tables' data: each table lists its
 * texts, and the entries the game leaves empty point at zeroed objects of
 * their own. The work area is BrowserWork (browser.h); the URL code that
 * follows in the game is the next file's.
 */
#include "common.h"
#include "browser.h"
#include "pad.h"
#include "lobby_globals.h"
#include "padio.h"

long GetTimerSystemTime(void);                               /* the time, in bus clock ticks */
void TimerBusClock2USec(long ticks, u32 *sec, u32 *usec);    /* ticks to seconds and microseconds */
s32 Html_Parse(u8 *page, s32 maxTags, s32 inFrame); /* unit's own declaration: declared with s32 parameters, as Html_LoadStep passes them; with the definition's (u8 *page, u16 maxTags, u8 inFrame) the call narrows them and compiles differently */
void Arena_Init(Arena *h);
void *Arena_Alloc(Arena *h, s32 size);
void Arena_Free(Arena *h, void *ptr);
void *Arena_Realloc(Arena *h, void *ptr, s32 size);

/* The message tables, defined at the end of the file. */
extern char *gBrowserErrorPages[];
extern char *gBrowserMenuTexts[];
extern char *gBrowserStatusTexts[];
extern char *gNetFileSaveTexts[];
extern char *gConnectFailTexts[];
extern char *gRegisterConfirmTexts[];
extern char *gCommErrorTexts[];


/* Parses the page for up to 20 ms a frame; returns a pointer to the load's
 * result (BrowserWork.loadResult: Html_Parse's 0 or 1 while it goes on, 2
 * done, 3 failed), or NULL when no page is being parsed. arg is passed on to
 * Html_Parse as inFrame. */
s16 *Html_LoadStep(s32 arg)
{
    s32 result;
    long start;
    u32 sec;
    u32 usec;

    switch (gBrowserWork->loadState) {
    case 1:
        start = GetTimerSystemTime();
        do {
            result = (u16)Html_Parse((u8 *) gBrowserWork->loadSrc, 1, arg);
            if (result != 0 && result != 1) {
                break;
            }
            TimerBusClock2USec(GetTimerSystemTime() - start, &sec, &usec);
            usec /= 1000;
            sec %= 1000;
        } while (usec + sec * 1000 < 20);
        switch (result) {
        case 0:
            gBrowserWork->loadResult = 0;
            return &gBrowserWork->loadResult;
        case 1:
            gBrowserWork->loadResult = 1;
            return &gBrowserWork->loadResult;
        case 2:
            gBrowserWork->loadResult = 2;
            gBrowserWork->loadState = 0;
            return &gBrowserWork->loadResult;
        case 3:
            gBrowserWork->loadResult = 3;
            gBrowserWork->loadState = 0;
            return &gBrowserWork->loadResult;
        }
        break;
    case 0:
        break;
    }
    return NULL;
}

/* Returns text i of the message table of group (BTEXT_*); NULL for any
 * other group. */
char *Browser_GetText(u32 group, s32 i)
{
    switch (group) {
    case BTEXT_ERROR_PAGE:
        return gBrowserErrorPages[i];
    case BTEXT_MENU:
        return gBrowserMenuTexts[i];
    case BTEXT_STATUS:
        return gBrowserStatusTexts[i];
    case BTEXT_NET_SAVE:
        return gNetFileSaveTexts[i];
    case BTEXT_CONNECT_FAIL:
        return gConnectFailTexts[i];
    case BTEXT_REGISTER:
        return gRegisterConfirmTexts[i];
    case BTEXT_COMM_ERROR:
        return gCommErrorTexts[i];
    case 0:
    default:
        return NULL;
    }
}

/* Sets up the page heap: an Arena on the work area's 512 KB buffer
 * (buf512k), in blocks of 16 bytes, with BROWSER_HEAP_RECS records. */
void Browser_InitHeap(void)
{
    gBrowserWork->heap.align = 0x10;
    gBrowserWork->heap.base = (s32)gBrowserWork->buf512k;
    gBrowserWork->heap.size = BROWSER_BUF_SIZE;
    gBrowserWork->heap.recs = gBrowserWork->heapBlk;
    gBrowserWork->heap.nrecs = BROWSER_HEAP_RECS;
    gBrowserWork->heap.locked = 1;
    gBrowserWork->heap.unk16 = 1;
    Arena_Init(&gBrowserWork->heap);
}

/* Allocates size bytes from the page heap. */
void *Browser_Alloc(u32 size)
{
    return Arena_Alloc(&gBrowserWork->heap, size);
}

/* Frees ptr back to the page heap. */
void Browser_Free(void *ptr)
{
    Arena_Free(&gBrowserWork->heap, ptr);
}

/* Resizes ptr in the page heap to size bytes. */
void *Browser_Realloc(void *ptr, u32 size)
{
    return Arena_Realloc(&gBrowserWork->heap, ptr, size);
}

/* A stand-in for a function of the game's file that the game's linker
 * removed: the build compiles it and drops its code. Its body is unknown;
 * what shows is that the file compiled a two-argument floating-point call
 * before Browser_ReadPad, which changes that function's registers
 * (docs/notes/nm/Browser_ReadPad.md). */
static f64 Stripped_Sub(f64 lhs, f64 rhs)
{
    return lhs - rhs;
}

/*
 * Reads the first pad into the work area each frame: its buttons (held,
 * held the frame before, pressed, released), and each analog stick's tilt
 * (stickTilt, stick2Tilt: 0 to 3, past 70, 100 and 126 of 127) and, when
 * tilted, its direction (stickDir, stick2Dir: 16 steps of 22.5 degrees).
 *
 * Kept for the match (FAKEMATCH): the function matches only with
 * Stripped_Sub before it. The game's file had a function there, removed by
 * the game's linker, whose two-argument floating-point call changed how the
 * compiler assigns this function's registers; Stripped_Sub stands in for it.
 * The sticks are read at offset 0x2C of the pad record (PadInput.stickL)
 * and written through the work area's bytes at 0x1A650 (stickTilt), the
 * offset loaded again in each branch: the field accesses and a single
 * offset compile differently.
 */
void Browser_ReadPad(void)
{
    s32 i;
    s32 stickOfs;
    s32 tiltOfs;
    s32 outOfs;
    BrowserWork *work;
    f32 stickDeg;
    PadState *pad = (PadState *)(s32)gPadState;

    i = 0;
    stickOfs = 0;
    outOfs = 0;
    gBrowserWork->padHeld = pad->held;
    gBrowserWork->padPrevHeld = pad->prevHeld;
    gBrowserWork->padPressed = pad->pressed;
    gBrowserWork->padReleased = pad->released;
    do {
        if (((PadStick *)((u8 *)gPadPtr + stickOfs + 0x2C))->mag > 0x7E) {
            tiltOfs = 0x1A650;
            ((u8 *)gBrowserWork)[outOfs + tiltOfs] = 3;
        } else if (((PadStick *)((u8 *)gPadPtr + stickOfs + 0x2C))->mag > 0x64) {
            tiltOfs = 0x1A650;
            ((u8 *)gBrowserWork)[outOfs + tiltOfs] = 2;
        } else if (((PadStick *)((u8 *)gPadPtr + stickOfs + 0x2C))->mag > 0x46) {
            tiltOfs = 0x1A650;
            ((u8 *)gBrowserWork)[outOfs + tiltOfs] = 1;
        } else {
            tiltOfs = 0x1A650;
            ((u8 *)gBrowserWork)[outOfs + tiltOfs] = 0;
        }
        work = gBrowserWork;
        if (0 < *(u8 *)((char *)work + tiltOfs + outOfs)) {
            stickDeg = 101.25 - ((PadStick *)((u8 *)gPadPtr + stickOfs + 0x2C))->deg;
            if (stickDeg < 0.0f) {
                stickDeg += 360.0f;
            }
            if (360.0f <= stickDeg) {
                stickDeg -= 360.0f;
            }
            ((u8 *)work)[outOfs + 0x1A651] = (s32)(stickDeg / 22.5f);
        } else {
            ((u8 *)work)[outOfs + 0x1A651] = 0;
        }
        i++;
        stickOfs += sizeof(PadStick);
        outOfs += 2;
    } while (i < 2);
}

/* The message tables' texts (.data) and the zeroed objects their empty
 * entries point at. */
extern u32 gBrowserStatusTexts_0[];
extern u32 gBrowserStatusTexts_1[];
extern u32 gNetFileSaveTexts_10[];
extern u32 gRegisterConfirmTexts_5[];
extern u32 gRegisterConfirmTexts_6[];
char gBrowserErrorPages_0[0xC0] = {
#include "lobby/007ED950.inc"
};
char gBrowserErrorPages_1[0xA0] = {
#include "lobby/007EDA10.inc"
};
char gBrowserErrorPages_2[0x120] = {
#include "lobby/007EDAB0.inc"
};
char gBrowserErrorPages_3[0x120] = {
#include "lobby/007EDBD0.inc"
};
char gBrowserErrorPages_4[0x130] = {
#include "lobby/007EDCF0.inc"
};
char gBrowserErrorPages_5[0x100] = {
#include "lobby/007EDE20.inc"
};
char gBrowserErrorPages_6[0x80] = {
#include "lobby/007EDF20.inc"
};
char gBrowserErrorPages_7[0x110] = {
#include "lobby/007EDFA0.inc"
};
char gBrowserErrorPages_8[0xC0] = {
#include "lobby/007EE0B0.inc"
};
char *gBrowserErrorPages[10] = {
    gBrowserErrorPages_0, gBrowserErrorPages_1, gBrowserErrorPages_2, gBrowserErrorPages_3,
    gBrowserErrorPages_4, gBrowserErrorPages_5, gBrowserErrorPages_6, gBrowserErrorPages_7,
    gBrowserErrorPages_8, NULL,
};
char gBrowserMenuTexts_0[0xC] = {
#include "lobby/007EE198.inc"
};
char gBrowserMenuTexts_1[0xC] = {
#include "lobby/007EE1A8.inc"
};
char gBrowserMenuTexts_2[0xC] = {
#include "lobby/007EE1B8.inc"
};
char *gBrowserMenuTexts[3] = {
    gBrowserMenuTexts_0, gBrowserMenuTexts_1, gBrowserMenuTexts_2,
};
char *gBrowserStatusTexts[2] = {
    (char *)gBrowserStatusTexts_0, (char *)gBrowserStatusTexts_1,
};
char gNetFileSaveTexts_0[0x198] = {
#include "lobby/007EE1E0.inc"
};
char gNetFileSaveTexts_1[0x8] = {
#include "lobby/007EE378.inc"
};
char gNetFileSaveTexts_2[0x10] = {
#include "lobby/007EE380.inc"
};
char gNetFileSaveTexts_3[0xC0] = {
#include "lobby/007EE390.inc"
};
char gNetFileSaveTexts_4[0x130] = {
#include "lobby/007EE450.inc"
};
char gNetFileSaveTexts_5[0x8] = {
#include "lobby/007EE580.inc"
};
char gNetFileSaveTexts_6[0x8] = {
#include "lobby/007EE588.inc"
};
char gNetFileSaveTexts_7[0xF0] = {
#include "lobby/007EE590.inc"
};
char gNetFileSaveTexts_8[0x10] = {
#include "lobby/007EE680.inc"
};
char gNetFileSaveTexts_9[0x50] = {
#include "lobby/007EE690.inc"
};
char *gNetFileSaveTexts[12] = {
    gNetFileSaveTexts_0, gNetFileSaveTexts_1, gNetFileSaveTexts_2, gNetFileSaveTexts_3,
    gNetFileSaveTexts_4, gNetFileSaveTexts_5, gNetFileSaveTexts_6, gNetFileSaveTexts_7,
    gNetFileSaveTexts_8, gNetFileSaveTexts_9, (char *)gNetFileSaveTexts_10, NULL,
};
char gConnectFailTexts_0[0x20] = {
#include "lobby/007EE710.inc"
};
char gConnectFailTexts_1[0x80] = {
#include "lobby/007EE730.inc"
};
char gConnectFailTexts_2[0x30] = {
#include "lobby/007EE7B0.inc"
};
char gConnectFailTexts_3[0x60] = {
#include "lobby/007EE7E0.inc"
};
char gConnectFailTexts_4[0x60] = {
#include "lobby/007EE840.inc"
};
char gConnectFailTexts_5[0x30] = {
#include "lobby/007EE8A0.inc"
};
char gConnectFailTexts_6[0x20] = {
#include "lobby/007EE8D0.inc"
};
char *gConnectFailTexts[8] = {
    gConnectFailTexts_0, gConnectFailTexts_1, gConnectFailTexts_2, gConnectFailTexts_3,
    gConnectFailTexts_4, gConnectFailTexts_5, gConnectFailTexts_6, NULL,
};
char gRegisterConfirmTexts_0[0x20] = {
#include "lobby/007EE910.inc"
};
char gRegisterConfirmTexts_1[0x8] = {
#include "lobby/007EE930.inc"
};
char gRegisterConfirmTexts_2[0x8] = {
#include "lobby/007EE938.inc"
};
char gRegisterConfirmTexts_3[0x18] = {
#include "lobby/007EE940.inc"
};
char gRegisterConfirmTexts_4[0x8] = {
#include "lobby/007EE958.inc"
};
char *gRegisterConfirmTexts[8] = {
    gRegisterConfirmTexts_0, gRegisterConfirmTexts_1, gRegisterConfirmTexts_2, gRegisterConfirmTexts_3,
    gRegisterConfirmTexts_4, (char *)gRegisterConfirmTexts_5, (char *)gRegisterConfirmTexts_6, NULL,
};
char gCommErrorTexts_0[0x40] = {
#include "lobby/007EE980.inc"
};
char gCommErrorTexts_1[0x10] = {
#include "lobby/007EE9C0.inc"
};
char gCommErrorTexts_2[0x38] = {
#include "lobby/007EE9D0.inc"
};
char gCommErrorTexts_3[0x8] = {
#include "lobby/007EEA08.inc"
};
char gCommErrorTexts_4[0x10] = {
#include "lobby/007EEA10.inc"
};
char *gCommErrorTexts[8] = {
    gCommErrorTexts_0, gCommErrorTexts_1, gCommErrorTexts_2, gCommErrorTexts_3,
    gCommErrorTexts_4, NULL, NULL, NULL,
};
