/* The browser's page loading in time slices, its message tables, its page
 * heap and its pad input; then Url_Compare and Url_Copy. include/browser.h. */
#include "common.h"
#include "browser.h"
#include "pad.h"
#include "lobby_globals.h"
int strncmp(const char *, const char *, unsigned int);

s32 strncmp(const char *a, const char *b, u32 n);
char *strncpy(char *dst, const char *src, u32 n); /* main: strncpy */
long GetTimerSystemTime(void);                               /* main: the time, in clock ticks */
void TimerBusClock2USec(long ticks, u32 *sec, u32 *usec);    /* main: ticks to seconds and microseconds */
s32 Html_Parse(u8 *page, s32 maxTags, s32 inFrame);   /* unk_741900.c */ /* unit's own declaration: with the definition's s32 (u8 *page, u16 maxTags, u8 inFrame) (lobby/unk_741900.c), Html_LoadStep 2 of 146 words differ */
void Arena_Init(Arena *h);                     /* sub_main: Heap_Init */
struct Heap;
void *Arena_Alloc(Arena *h, s32 size);          /* sub_main: Heap_Alloc */
void Arena_Free(Arena *h, void *p);            /* sub_main: Heap_Free */
void *Arena_Realloc(Arena *h, void *p, s32 size);   /* sub_main: Heap_Realloc? */

/* The message tables (.data). */
extern char *gBrowserErrorPages[];
extern char *gBrowserMenuTexts[];
extern char *gBrowserTexts4[];
extern char *gNetFileSaveTexts[];
extern char *gConnectFailTexts[];
extern char *gRegisterConfirmTexts[];
extern char *gCommErrorTexts[];

/* The main module's pad record (0x24BBEC points to it): the analog sticks. */
typedef struct MainStick {
    s16 mag;   /* tilt */
    s16 angle; /* degrees */
    s32 unk4;
    s32 unk8;
} MainStick;

typedef struct MainPad {
    u8 unk0[0x30];
    MainStick stick[2];
} MainPad;

#define gPadPtr (*(MainPad **)&gPadPtr)

/* Html_LoadStep: parses the page for up to 20 ms; returns the load result
 * (0 or 1: not finished, 2 done, 3 failed), or NULL when no page is being
 * loaded. */
s16 *Html_LoadStep(s32 arg)
{
    s32 r;
    long start;
    u32 sec;
    u32 usec;

    switch (gBrowserWork->loadState) {
    case 1:
        start = GetTimerSystemTime();
        do {
            r = (u16)Html_Parse((u8 *) gBrowserWork->loadSrc, 1, arg);
            if (r != 0 && r != 1) {
                break;
            }
            TimerBusClock2USec(GetTimerSystemTime() - start, &sec, &usec);
            usec /= 1000;
            sec %= 1000;
        } while (usec + sec * 1000 < 20);
        switch (r) {
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

/* Browser_GetText: text i of the message table of group (2..8). */
char *Browser_GetText(u32 group, s32 i)
{
    switch (group) {
    case 2:
        return gBrowserErrorPages[i];
    case 3:
        return gBrowserMenuTexts[i];
    case 4:
        return gBrowserTexts4[i];
    case 5:
        return gNetFileSaveTexts[i];
    case 6:
        return gConnectFailTexts[i];
    case 7:
        return gRegisterConfirmTexts[i];
    case 8:
        return gCommErrorTexts[i];
    case 0:
    default:
        return NULL;
    }
}

/* Browser_InitHeap: the page heap on buf512k, 0x400 records. */
void Browser_InitHeap(void)
{
    gBrowserWork->heap.align = 0x10;
    gBrowserWork->heap.base = (s32)gBrowserWork->buf512k;
    gBrowserWork->heap.size = 0x80000;
    gBrowserWork->heap.recs = gBrowserWork->heapBlk;
    gBrowserWork->heap.nrecs = 0x400;
    gBrowserWork->heap.locked = 1;
    gBrowserWork->heap.unk16 = 1;
    Arena_Init(&gBrowserWork->heap);
}

/* Browser_Alloc */
void *Browser_Alloc(u32 size)
{
    return Arena_Alloc(&gBrowserWork->heap, size);
}

/* Browser_Free */
void Browser_Free(void *p)
{
    Arena_Free(&gBrowserWork->heap, p);
}

/* Browser_Realloc? */
void *Browser_Realloc(void *p, u32 size)
{
    return Arena_Realloc(&gBrowserWork->heap, p, size);
}

/* Stripped_Sub: a stand-in for a function of this file that the game's
 * linker dead-stripped (tools/mwcc_cc.py compiles it and drops its code). Its
 * body is unknown: what shows is that the file compiled a two-argument runtime
 * call before Browser_ReadPad, which changes that function's registers
 * (docs/notes/nm/Browser_ReadPad.md). */
static f64 Stripped_Sub(f64 a, f64 b)
{
    return a - b;
}

/* Browser_ReadPad: the buttons of the first pad, and each analog stick's
 * tilt (0 to 3) and direction (16 steps). FAKEMATCH: it matches only with
 * Stripped_Sub before it. Once a function earlier in the file has compiled a
 * two-argument runtime call (dpsub, dpadd, _dpflt, __divdi3...), MWCC models
 * its own litodp call as reading a1 too, so a1 stays busy from the entry and
 * pad and o take a2, as in the game; nothing left in lobby calls one. The
 * conversions are left to the compiler. Also contrived: the stick array's
 * offset (0x1A650) assigned to o in each branch, which the game computes into
 * a2 in each branch and adds to w before k at the join. */
void Browser_ReadPad(void)
{
    s32 i;
    s32 off;
    s32 o;
    s32 k;
    BrowserWork *w;
    f32 a;
    PadState *pad = (PadState *)(s32)gPadState;

    i = 0;
    off = 0;
    k = 0;
    gBrowserWork->padHeld = pad->held;
    gBrowserWork->padUnk2 = pad->prevHeld;
    gBrowserWork->padPressed = pad->pressed;
    gBrowserWork->padReleased = pad->released;
    do {
        if (((MainStick *)((u8 *)gPadPtr + off + 0x30))->mag > 0x7E) {
            o = 0x1A650;
            ((u8 *)gBrowserWork)[k + o] = 3;
        } else if (((MainStick *)((u8 *)gPadPtr + off + 0x30))->mag > 0x64) {
            o = 0x1A650;
            ((u8 *)gBrowserWork)[k + o] = 2;
        } else if (((MainStick *)((u8 *)gPadPtr + off + 0x30))->mag > 0x46) {
            o = 0x1A650;
            ((u8 *)gBrowserWork)[k + o] = 1;
        } else {
            o = 0x1A650;
            ((u8 *)gBrowserWork)[k + o] = 0;
        }
        w = gBrowserWork;
        if (0 < *(u8 *)((char *)w + o + k)) {
            a = 101.25 - ((MainStick *)((u8 *)gPadPtr + off + 0x30))->angle;
            if (a < 0.0f) {
                a += 360.0f;
            }
            if (360.0f <= a) {
                a -= 360.0f;
            }
            ((u8 *)w)[k + 0x1A651] = (s32)(a / 22.5f);
        } else {
            ((u8 *)w)[k + 0x1A651] = 0;
        }
        i++;
        off += sizeof(MainStick);
        k += 2;
    } while (i < 2);
}

/* Url_Compare */
s32 Url_Compare(const char *a, const char *b)
{
    return strncmp(a, b, 0x100);
}

/* Url_Copy: at most 0xFF characters, always terminated. */
void Url_Copy(char *dst, const char *src)
{
    strncpy(dst, src, 0xFF);
    dst[0xFF] = 0;
}

/* unk_7303D0's .data (0x7ED950-0x7EEA40) */
extern u32 D_00895230[];
extern u32 D_00895238[];
extern u32 D_00895240[];
extern u32 D_00895248[];
extern u32 D_00895250[];
char D_007ED950[0xC0] = {
#include "lobby/007ED950.inc"
};
char D_007EDA10[0xA0] = {
#include "lobby/007EDA10.inc"
};
char D_007EDAB0[0x120] = {
#include "lobby/007EDAB0.inc"
};
char D_007EDBD0[0x120] = {
#include "lobby/007EDBD0.inc"
};
char D_007EDCF0[0x130] = {
#include "lobby/007EDCF0.inc"
};
char D_007EDE20[0x100] = {
#include "lobby/007EDE20.inc"
};
char D_007EDF20[0x80] = {
#include "lobby/007EDF20.inc"
};
char D_007EDFA0[0x110] = {
#include "lobby/007EDFA0.inc"
};
char D_007EE0B0[0xC0] = {
#include "lobby/007EE0B0.inc"
};
char *gBrowserErrorPages[10] = {
    D_007ED950, D_007EDA10, D_007EDAB0, D_007EDBD0,
    D_007EDCF0, D_007EDE20, D_007EDF20, D_007EDFA0,
    D_007EE0B0, NULL,
};
char D_007EE198[0xC] = {
#include "lobby/007EE198.inc"
};
char D_007EE1A8[0xC] = {
#include "lobby/007EE1A8.inc"
};
char D_007EE1B8[0xC] = {
#include "lobby/007EE1B8.inc"
};
char *gBrowserMenuTexts[3] = {
    D_007EE198, D_007EE1A8, D_007EE1B8,
};
char *gBrowserTexts4[2] = {
    (char *)D_00895230, (char *)D_00895238,
};
char D_007EE1E0[0x198] = {
#include "lobby/007EE1E0.inc"
};
char D_007EE378[0x8] = {
#include "lobby/007EE378.inc"
};
char D_007EE380[0x10] = {
#include "lobby/007EE380.inc"
};
char D_007EE390[0xC0] = {
#include "lobby/007EE390.inc"
};
char D_007EE450[0x130] = {
#include "lobby/007EE450.inc"
};
char D_007EE580[0x8] = {
#include "lobby/007EE580.inc"
};
char D_007EE588[0x8] = {
#include "lobby/007EE588.inc"
};
char D_007EE590[0xF0] = {
#include "lobby/007EE590.inc"
};
char D_007EE680[0x10] = {
#include "lobby/007EE680.inc"
};
char D_007EE690[0x50] = {
#include "lobby/007EE690.inc"
};
char *gNetFileSaveTexts[12] = {
    D_007EE1E0, D_007EE378, D_007EE380, D_007EE390,
    D_007EE450, D_007EE580, D_007EE588, D_007EE590,
    D_007EE680, D_007EE690, (char *)D_00895240, NULL,
};
char D_007EE710[0x20] = {
#include "lobby/007EE710.inc"
};
char D_007EE730[0x80] = {
#include "lobby/007EE730.inc"
};
char D_007EE7B0[0x30] = {
#include "lobby/007EE7B0.inc"
};
char D_007EE7E0[0x60] = {
#include "lobby/007EE7E0.inc"
};
char D_007EE840[0x60] = {
#include "lobby/007EE840.inc"
};
char D_007EE8A0[0x30] = {
#include "lobby/007EE8A0.inc"
};
char D_007EE8D0[0x20] = {
#include "lobby/007EE8D0.inc"
};
char *gConnectFailTexts[8] = {
    D_007EE710, D_007EE730, D_007EE7B0, D_007EE7E0,
    D_007EE840, D_007EE8A0, D_007EE8D0, NULL,
};
char D_007EE910[0x20] = {
#include "lobby/007EE910.inc"
};
char D_007EE930[0x8] = {
#include "lobby/007EE930.inc"
};
char D_007EE938[0x8] = {
#include "lobby/007EE938.inc"
};
char D_007EE940[0x18] = {
#include "lobby/007EE940.inc"
};
char D_007EE958[0x8] = {
#include "lobby/007EE958.inc"
};
char *gRegisterConfirmTexts[8] = {
    D_007EE910, D_007EE930, D_007EE938, D_007EE940,
    D_007EE958, (char *)D_00895248, (char *)D_00895250, NULL,
};
char D_007EE980[0x40] = {
#include "lobby/007EE980.inc"
};
char D_007EE9C0[0x10] = {
#include "lobby/007EE9C0.inc"
};
char D_007EE9D0[0x38] = {
#include "lobby/007EE9D0.inc"
};
char D_007EEA08[0x8] = {
#include "lobby/007EEA08.inc"
};
char D_007EEA10[0x10] = {
#include "lobby/007EEA10.inc"
};
char *gCommErrorTexts[8] = {
    D_007EE980, D_007EE9C0, D_007EE9D0, D_007EEA08,
    D_007EEA10, NULL, NULL, NULL,
};
