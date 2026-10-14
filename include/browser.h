/* browser.h: the lobby overlay's HTML browser for the KDDI "MMBB" portal
 * (lobby 0x729AC0-0x744D70: pages, modes, input, links, forms, URLs and
 * the draw commands; the HTML parser and layout are in html.h, the image
 * decoders use zlib.h).
 *
 * The browser's globals (.bss; the names are still splat's):
 *   gBrowserWork  BrowserWork *: the work area, &gBrowserWorkMem (0x1D2C0 bytes)
 *   gBrowserState  BrowserState *: &work->state
 *   gBrowserSub  BrowserSub *: &work->sub
 *   gMmbbData  the persistent MMBB data in sub_main (0x5CAED0, 0x97C2
 *               bytes: the login id and password, the lobby server's
 *               user-id and connection-id ...), MmbbData *
 *   gBrowserTopMode  u8: the browser's top mode (0 the portal, 1 ...)
 * The lobby's first village range (0x7223C0-0x729AC0, unk_71F8A0.c ..
 * unk_7238B0.c) is browser code too: it reads the work area by byte
 * offsets (`u8 *gBrowserWork`).
 */
#ifndef BROWSER_H
#define BROWSER_H

#include "common.h"
#include "prim.h"              /* PrimTri, PrimLine (Prim_Tri, Prim_Rect) */
#include "sprite.h"            /* Sprite (Prim_Sprite: x, y, w, h) */
#include "arena.h"             /* Arena (BrowserWork.heap) */

/* URL schemes: Url_GetScheme's table (gUrlSchemes, UrlScheme entries). */
#define URL_OTHER 0
#define URL_FILE 1   /* "file://": a file on the disc (file://src/browser/DATA/...) */
#define URL_HTTP 2   /* "http://" */
#define URL_HTTPS 3  /* "https://" */
#define URL_MEMORY 4 /* "memory://%03d": a page held in memory */
#define URL_LBS 5    /* "lbs://": the lobby server's address, for the game */
#define URL_AFS 6    /* "afs://": a file of the game's AFS archive */

typedef struct UrlScheme {
    u8 id;           /* URL_* */
    char prefix[16]; /* "file://" ... */
} UrlScheme;         /* 0x11 bytes */

/* The MMBB data (gMmbbData = sub_main 0x5CAED0, 0x97C2 bytes, kept
 * across lobby visits): what the portal's pages hand the game (the page's
 * <CSV> data: Mmbb_SetField's field numbers in brackets), used by the
 * lobby's login (userId and connectionId) and the server lists. */
typedef struct MmbbData {
    char userId[0x11];        /* 0x0000 [1]: the "user-id" (at most 14) */
    char connectionId[0xB];   /* 0x0011 [2]: the "connection-id" (10) */
    char f4[0x101];           /* 0x001C [4] */
    char nextUrl[0x101];      /* 0x011D [5]: where the browser goes next */
    char f6[0x11];            /* 0x021E [6] */
    char f7[40][0x62];        /* 0x022F [7] */
    char f8[40][0x101];       /* 0x117F [8] */
    char f9[40][0x101];       /* 0x39A7 [9] */
    char f11[0x101];          /* 0x61CF [11] */
    char f12[40][0x21];       /* 0x62D0 [12] */
    char f13[40][0x21];       /* 0x67F8 [13] */
    char f14[0x11];           /* 0x6D20 [14] */
    char f15[0x101];          /* 0x6D31 [15] */
    char f16[40][3];          /* 0x6E32 [16] */
    char f17[40][5];          /* 0x6EAA [17] */
    char f18[40][0x101];      /* 0x6F72 [18] */
    u8 unk979A[0x28];         /* 0x979A */
} MmbbData;                   /* 0x97C2 bytes */

/* The lobby servers of a CRS-LBS-INFO-GET page (sub_main 0x5C09D0, a view
 * of it): a row per <CSV> entry, from the entry's fields 8 ("key,text,
 * value"), 12 and 13; Lbs_Sort sorts the rows by key, and the
 * entries' MMBB data with them. */
typedef struct LbsRow {
    char key[0xD];     /* 0x000: field 8's first item (at most 12 characters) */
    char text[0xF7];   /* 0x00D: its second (at most 0xF4) */
    s32 value;         /* 0x104: its third ("0x..." hex, or decimal) */
    s32 capacity;      /* 0x108: field 13 as a number: the server's capacity (the lobby prints users/capacity) */
    s32 users;         /* 0x10C: field 12 as a number: its users */
    u32 free;          /* 0x110: capacity - users, set by the lobby before it sorts the servers by it */
} LbsRow;              /* 0x114 bytes */

typedef struct LbsList {
    LbsRow row[40];    /* 0x0000 */
    u8 unk2B20[8];     /* 0x2B20 */
    u8 count;          /* 0x2B28 (sub_main 0x5C34F8) */
} LbsList;

extern LbsList gLobbyServers;

typedef struct BrowserSub {
    u8 dlgStep;                 /* the sub-dialog's step (Dialog_Run's table) */
    u8 dlgMode;                 /* the dialog's mode (3 from Browser_OpenDialog, 4, 7 ...) */
    u8 dlgCursor;               /* the dialog's choice (Browser_OpenDialog's arg; 20 pixels a line) */
    u8 moveKind;                /* the last Browser_Move's kind (cleared when moveTimer ends) */
    u8 listCount;               /* the open select list's options (0: none; a scroll bar over 10) */
    u8 listTop;                 /* its first option shown (1-based) */
    u8 listUp;                  /* 1: the list opens above its field */
    u8 unk7;
    s16 thumbTop;               /* the list's scroll thumb: top */
    s16 thumbBottom;            /* and bottom */
    s16 thumbLen;               /* and length */
    s16 stickRepeat;            /* frames the stick is held (a move every 8) */
    s16 cursorX;                /* the pointer's x */
    s16 cursorY;                /* and y */
    s32 dragX;                  /* where a thumb drag began: x */
    s32 dragY;                  /* and y */
    s32 dragOfs;                /* the pointer's offset in the thumb (0: no drag) */
    s32 listObj;                /* the select list's object (an obj[] index) */
} BrowserSub; /* 0x24 bytes */

typedef struct BrowserState {
    u8 prevPage;                /* 0x000: the page shown before (set from page) */
    u8 mode;                    /* 0x001: index into the running top mode's table of modes */
    u8 step;                    /* 0x002: index into the mode's table of steps */
    u8 sub;                     /* 0x003 */
    s32 x;                      /* 0x004 */
    s32 y;                      /* 0x008 */
    s32 pageW;                  /* 0x00C: the page's width (x scrolls up to pageW - 0x248) */
    s32 pageH;                  /* 0x010: the page's height (y scrolls up to pageH - 0x17C) */
    u32 color;                  /* 0x014: 0xFF000001 at start */
    u16 style;                  /* 0x018: the page's style flags (Page_SetStyle; 0x2E7 at start; 1, 2, 4, 8, 0x20 ...) */
    s16 marginTop;              /* 0x01A: the margins (Page_Margins) */
    s16 marginBottom;           /* 0x01C */
    s16 marginLeft;             /* 0x01E */
    s16 marginRight;            /* 0x020 */
    s16 vThumbLen;              /* 0x022: the vertical scroll bar's thumb: length (0x20490 / pageH) */
    s16 vThumbY;                /* 0x024: and position */
    s16 hThumbLen;              /* 0x026: the horizontal one's: length (0x4EB40 / pageW) */
    s16 hThumbX;                /* 0x028: and position */
    u8 vScroll;                 /* 0x02A: 1: the page is taller than the screen (vertical scroll bar) */
    u8 hScroll;                 /* 0x02B: 1: wider (horizontal scroll bar) */
    u8 histDir;                 /* 0x02C: the history move asked for: 1 back, 2 forward */
    u8 imgCount;                /* 0x02D: images requested for the page (at most 20) */
    u8 page;                    /* 0x02E: what the page on show is: 1, 8, 0xA ... */
    u8 prevPageMode;            /* 0x02F: pageMode before the last Page_SetDataMode */
    u8 pageMode;                /* 0x030: 1, 4, 5: data pages without form fields (Page_IsHidden) */
    u8 menuReached;             /* 0x031: 1 from CRS-MENU or COMP-SIGNUP (Page_SetKind) */
    u8 unk32;                   /* 0x032 */
    u8 fromTop;                 /* 0x033: 1 from CRS-TOP until the login id dialog (Page_SetKind) */
    u8 plain;                   /* 0x034: 1: the portal's pages 1 and 2 (no frame colour, no back button) */
    s8 pageKind;                /* 0x035: Page_SetKind's kind (-1 none) */
    s8 initMode;                /* 0x036: the argument of Browser_Init (1: ...) */
    u8 styled;                  /* 0x037: 1: Page_SetStyle ran for the page */
    u8 styleWait;               /* 0x038: 0x1E frames from Page_SetStyle (no reload meanwhile) */
    u8 forceStyle8;             /* 0x039: 1: the reply was status 14 or 2 (Page_SetStyle adds style flag 8) */
    s8 fade;                    /* 0x03A: the fade: 16 in, -16 out (no reload meanwhile) */
    char url[0x100];            /* 0x03B: the URL asked for (Browser_Go) */
    char url2[0x100];           /* 0x13B */
    u8 unk23B[0x33B - 0x23B];
    char msg[0x100];            /* 0x33B: the page's message (Page_SetMode) */
    char url3[0x100];           /* 0x43B */
    u8 unk53B[0x55D - 0x53B];
    char field1[0x11];          /* 0x55D: form field 1's value (Form_Submit) */
    char field2[0x11];          /* 0x56E: form field 2's */
    u8 loginId[0x11];           /* 0x57F: the MMBB login id (from the NET file) */
    u8 password[0x11];          /* 0x590 */
    char loginIdSave[0x11];     /* 0x5A1: a login id copy (restored into loginId) */
    char field3[0x11];          /* 0x5B2: form field 3's */
    s8 styleMode;               /* 0x5C3: Page_SetStyle's mode */
} BrowserState;
typedef char BrowserState_size_check[(sizeof(BrowserState) == 0x5C4) ? 1 : -1];

struct BrowserObj;
/* A page object's update or draw function (BrowserObj.update, .draw). */
typedef void (*BrowserObjFunc)(struct BrowserObj *o);

typedef struct BrowserObj {
    u8 active;                  /* 0x000: update is called while set */
    u8 visible;                 /* 0x001: draw is called while set */
    u8 kind;                    /* 0x002: the draw command's kind (Draw_Cmd: 1 text, 2 link, 6 text input ...) */
    u8 layer;                   /* 0x003: the list it is in (PageObj_Alloc) */
    u8 size;                    /* 0x004: the text size (Draw_Cmd) */
    u8 sub;                     /* 0x005: the command's sub-kind; a form's method; an option's index ... */
    s8 state;                   /* 0x006 */
    s8 boxStep;                 /* 0x007: the dialog box's step (DlgBox_Update) */
    u8 unk008[0x00A - 0x008];
    s16 blink;                  /* 0x00A: the cursor's blink alpha (0 to 0xFF) */
    s16 blinkStep;              /* 0x00C: its step (15, -15) */
    s16 unkE;                   /* 0x00E */
    s16 unk10;                  /* 0x010 */
    u8 unk012[0x014 - 0x012];
    BrowserObjFunc update;      /* 0x014: called each frame while active (PageObj lists, unk_730AE0.c) */
    BrowserObjFunc draw;        /* 0x018: called each frame while visible */
    struct BrowserObj *prev;    /* 0x01C */
    struct BrowserObj *next;    /* 0x020 */
    u8 unk024[0x030 - 0x024];
    u8 timer;                   /* 0x030 */
    u8 unk031[0x034 - 0x031];
    f32 x;                      /* 0x034 */
    f32 y;                      /* 0x038 */
    u8 unk03C[0x040 - 0x03C];
    f32 w;                      /* 0x040 */
    f32 h;                      /* 0x044 */
    u8 unk048[0x04C - 0x048];
    s32 unk4C;                  /* 0x04C */
    s32 unk50;                  /* 0x050 */
    u8 unk054[0x058 - 0x054];
    u32 color;                  /* 0x058: the draw command's color (Draw_Cmd) */
    u8 unk05C;                  /* 0x05C */
    u8 field;                   /* 0x05D: 1 the login id, 2 the password, 3 ... (Form_Submit keeps their values) */
    u8 group;                   /* 0x05E: a form's number; a radio button's group (Draw_Cmd kind 8) */
    u8 tex;                     /* 0x05F: texture slot; a radio button's index in its group */
    char *s1;                   /* 0x060: href, name ... */
    char *s2;                   /* 0x064: value ... */
    char *s3;                   /* 0x068: the draw command's third string */
    u8 unk06C[0x070 - 0x06C];
} BrowserObj;
typedef char BrowserObj_size_check[(sizeof(BrowserObj) == 0x70) ? 1 : -1];

typedef struct BrowserNodeFlags {
    u8 serial; /* gBrowserWork->serial when the node was made */
    u8 flags; /* NamePair_SetFlag sets, NamePair_TestFlag tests */
} BrowserNodeFlags;

/* A node of the work area's name lists (heads at gBrowserWork + 0x911C, 0x9228,
 * 0x9334, 0x9444; each head is a node too). The entries of a list extend it. */
typedef struct BrowserNode {
    struct BrowserNode *next; /* 0x000 */
    char name[0x100];      /* 0x004 */
    s32 used;              /* 0x104 */
    BrowserNodeFlags tag;          /* 0x108 */
} BrowserNode;                /* 0x10C */

/* An entry of the list at gBrowserWork + 0x911C. */
typedef struct BrowserPageNode {
    BrowserNode node; /* 0x000 */
    u8 *buf;       /* 0x10C: 0x8000 bytes */
    s32 unk110;    /* 0x110 */
} BrowserPageNode;

/* An entry of the list at gBrowserWork + 0x9228. */
typedef struct BrowserImgNode {
    BrowserNode node; /* 0x000 */
    char *data;    /* 0x10C: the image data (a reply buffer) */
    s32 size;      /* 0x110: its size */
    u32 handles;   /* 0x114: two 16-bit handles (texture, palette) */
    u16 w;         /* 0x118 */
    u16 h;         /* 0x11A */
} BrowserImgNode;

/* An entry of the list at gBrowserWork + 0x9444. */
typedef struct BrowserReqNode {
    BrowserNode node; /* 0x000 */
    s8 kind;       /* 0x10C: the request (1, 3: the current URL; 4, 5, 6: a page of the history) */
    s8 state;      /* 0x10D: 1 made, 2 sent */
    char *url;     /* 0x110 */
    s32 unk114;    /* 0x114 */
    struct HttpReq *req; /* 0x118: made by HttpReq_Alloc */
    char *buf;     /* 0x11C: the reply (the connection buffer) */
    s32 len;       /* 0x120: its length */
    s8 result;     /* 0x124: 0 ok, 2.. errors */
    s8 errStep;     /* 0x125: the connection step that failed (0 none) */
} BrowserReqNode;

/* The result BrowserReq_Poll returns (gBrowserWork + 0x9554). */
typedef struct BrowserResult {
    void *data; /* 0x0 */
    s8 code;    /* 0x4 */
    u16 w;      /* 0x6 */
    u16 h;      /* 0x8 */
    s32 size;   /* 0xC */
} BrowserResult;

typedef struct BrowserWork {
    char *url;                  /* 0x000: the page's URL (Browser_SetUrl) */
    BrowserState state;         /* 0x004: gBrowserState */
    BrowserSub sub;             /* 0x5C8: gBrowserSub */
    struct BrowserObj *obj[500];/* 0x5EC: the page's objects, in draw-command order */
    u8 unkDBC[0xDEC - 0xDBC];
    u8 freedObjs;               /* 0xDEC: objects freed by their update (state 99; cleared when the dialogs close) */
    u8 unkDED[0xDF0 - 0xDED];
    s32 texHandle[20];          /* 0xDF0: the page textures (0: none), for Prim_DrawSprite */
    u16 texW[20];               /* 0xE40: the page textures' widths (by BrowserObj.tex) */
    u16 texH[20];               /* 0xE68: and heights */
    u16 focus;                  /* 0xE90 */
    s8 formKind;                /* 0xE92 */
    s8 keepLoginStep;           /* 0xE93 */
    s8 endReason;               /* 0xE94: why the browser ends: 1 network error, 2 left by the user, 3 the retry failed (>0: only the dialog boxes draw) */
    u8 netError;                /* 0xE95 */
    s8 endGoOn;                 /* 0xE96: 1: the end goes on to gMmbbData->nextUrl (PortalEnd_Step0), 0: Browser_Exit(-1) */
    u8 formGoLink;              /* 0xE97: set: a form goes to its link object's URL and skips fields named URL (only cleared in the C so far) */
    s8 result;                  /* 0xE98: Browser_Update's result: 0 running, -1 ... */
    u8 atMmbbUrl;               /* 0xE99 */
    u8 loginPosted;             /* 0xE9A */
    s8 gameMenuNo;              /* 0xE9B */
    u8 firstPage;               /* 0xE9C: 1 until the first page is shown (Browser_GoStart only then) */
    u8 tries;                   /* 0xE9D: page tries: the second failure gives up (Browser_Retry, Browser_GoStart; 2: give up) */
    u8 startPosts;              /* 0xE9E: posts to gMmbbData->f4 (more than 1 gives up) */
    u8 texFailOnce;             /* 0xE9F: 1: a failed image may fail the page once (Browser_Init's mode 1) */
    u8 texCount;                /* 0xEA0: page textures in texHandle/texW/texH */
    u8 cmdBuf[0x8000];          /* 0xEA1: the draw commands of the page */
    u8 unk8EA1[0x8EA4 - 0x8EA1];
    char *pageData;             /* 0x8EA4: the reply data: the page HTML (Html_StartLoad) */
    u8 *cmdPtr;                 /* 0x8EA8: write position in cmdBuf */
    u8 stickReleased;           /* 0x8EAC: 1: the left stick came back to neutral */
    u8 stickFirst;              /* 0x8EAD: 1: this is the first frame of a tilt (moves at once, then every 8 frames) */
    u8 reqBusy;                 /* 0x8EAE: 1 from a request (BrowserReq_Page, _Post) until its reply (BrowserReq_Poll) */
    u8 dlgBackState;            /* 0x8EAF: dialog kind 0's state (DlgBack_Update: 1 look, 2 shown, 3 none, 4 close) */
    u8 dlgPageState;            /* 0x8EB0: dialog kind 1's state (DlgPage_Update; 5 close) */
    u8 dlgVScrollState;         /* 0x8EB1: dialog kind 2's (DlgVScroll_Update) */
    u8 dlgHScrollState;         /* 0x8EB2: dialog kind 3's (DlgHScroll_Update) */
    u8 dlgStatusState;          /* 0x8EB3: dialog kind 4's (DlgStatus_Update) */
    u8 dlgMsgState;             /* 0x8EB4: dialog kind 5's (DlgMsg_Update) */
    u8 dlgCursorState;          /* 0x8EB5: dialog kind 7's (DlgCursor_Update; 2 from Browser_OpenDialog) */
    u8 dlgInputState;           /* 0x8EB6: dialog kind 6's (DlgInput_Update) */
    u8 dlgBoxKind;              /* 0x8EB7: dialog kind 8's: the box shown (Browser_OpenDialog's kind; 1 none, 4 error, 0xB, 0xC end) */
    u8 errorKind;               /* 0x8EB8: Browser_SetErrorState's kind */
    s8 cursorOnLink;            /* 0x8EB9: 1: the selection moved onto a link (restarts the cursor blink) */
    u8 unk8EBA[0x8EC0 - 0x8EBA];
    char kbdText[0x100];        /* 0x8EC0: the software keyboard's text */
    void *memPages;             /* 0x8FC0: the name lists (unk_722360.c: pages, images, history, requests): the page nodes' memory */
    void *memBufs;              /* 0x8FC4 */
    void *memImgs;              /* 0x8FC8 */
    void *memConnBufs;          /* 0x8FCC */
    void *memHist;              /* 0x8FD0 */
    void *memReqs;              /* 0x8FD4 */
    BrowserPageNode *pages;     /* 0x8FD8 */
    BrowserImgNode *imgs;       /* 0x8FDC */
    BrowserPageNode dl;         /* 0x8FE0 */
    BrowserNode *hist;          /* 0x90F4 */
    BrowserReqNode *reqs;       /* 0x90F8 */
    u8 *connBuf[8];             /* 0x90FC */
    BrowserNode pageList;       /* 0x911C */
    BrowserNode imgList;        /* 0x9228 */
    BrowserNode histList;       /* 0x9334 */
    BrowserNode *histCur;       /* 0x9440 */
    BrowserNode reqList;        /* 0x9444 */
    u8 reqState;                /* 0x9550 */
    u8 unk9551[0x9554 - 0x9551];
    BrowserResult reqResult;    /* 0x9554: BrowserReq_Poll result */
    u8 serial;                  /* 0x9564 */
    u8 unk9565[0x9568 - 0x9565];
    ArenaBlk heapBlk[0x400];    /* 0x9568: the records of heap */
    Arena heap;                 /* 0xC568: the page heap, on buf512k */
    BrowserObj objPool[512];    /* 0xC590: PageObj_Alloc's pool */
    u32 tex[24];                /* 0x1A590: textures and palettes (Tex_Add) */
    s8 nTex;                    /* 0x1A5F0 */
    u8 unk1A5F1[0x1A5F4 - 0x1A5F1];
    struct BrowserObj *freeObj; /* 0x1A5F4: PageObj_Alloc's free list */
    struct BrowserObj *layerHead[9];/* 0x1A5F8 */
    struct BrowserObj *layerTail[9];/* 0x1A61C */
    s32 unk1A640;               /* 0x1A640 */
    s32 unk1A644;               /* 0x1A644 */
    u16 padHeld;                /* 0x1A648: gPadState[0], copied each frame (Browser_ReadPad) */
    u16 padUnk2;                /* 0x1A64A */
    u16 padPressed;             /* 0x1A64C */
    u16 padReleased;            /* 0x1A64E */
    u8 stickTilt;               /* 0x1A650: the left stick: tilt (0 to 3) */
    u8 stickDir;                /* 0x1A651: and direction (16 steps) */
    u8 stick2Tilt;              /* 0x1A652: the right stick */
    u8 stick2Dir;               /* 0x1A653 */
    u8 unk1A654[0x1A660 - 0x1A654];
    u8 *buf64k;                 /* 0x1A660: 0x10000 bytes (Mem_Alloc) */
    u8 *buf512k;                /* 0x1A664: 0x80000 bytes (Mem_Alloc) */
    void *mem64k;               /* 0x1A668: the Mem_Alloc handle of buf64k */
    void *mem512k;              /* 0x1A66C: the Mem_Alloc handle of buf512k */
    char formData[0x1000];      /* 0x1A670: the form's query: name=value pairs joined by & (BrowserForm_Add escapes them), cleared before a page's fields are added */
    char baseUrl[0x100];        /* 0x1B670: the base for relative URLs (Url_SetBase) */
    char urlLower[0x100];       /* 0x1B770: the URL in lower case (Url_SpecialScheme) */
    char errorText[0x200];      /* 0x1B870: a failed request's message (BrowserReq_Poll copies Browser_GetText(2, k) there: the result's data) */
    u8 unk1BA70[0x1C270 - 0x1BA70];
    u8 *memPage[3];             /* 0x1C270: the pages of memory://N URLs (HttpReq_Open, URL_MEMORY); [0] is the error page buffer at +0x1BA70 (Browser_BuildErrorPage), [1] and [2] are cleared */
    u8 unk1C27C[0x1C27D - 0x1C27C];
    u8 exitArgs;                /* 0x1C27D: the arguments of mmbb://EXIT:n (n: 0-9) */
    s16 refreshTimer;           /* 0x1C27E: frames until the META REFRESH URL loads (Html_Refresh) */
    s16 moveTimer;              /* 0x1C280: 3 after a move (Browser_Move, form buttons): gBrowserSub->moveKind is cleared when it ends */
    char savedId[0xB];          /* 0x1C282: the kept login id before Browser_SaveLogin wrote it (restored on failure) */
    char savedPass[9];          /* 0x1C28D: and its password */
    s8 dlgBoxReady;             /* 0x1C296: the dialog box's draw phase (unk_722360.c sets 1, 2; Browser_OpenDialog clears it) */
    u8 unk1C297[0x1C298 - 0x1C297];
    s32 loadState;              /* 0x1C298: 1 while a page is being parsed (Html_StartLoad) */
    s16 loadResult;             /* 0x1C29C: what Html_LoadStep returns a pointer to: 0, 1 parsing, 2 done, 3 failed */
    u8 unk1C29E[0x1C2A0 - 0x1C29E];
    char *loadSrc;              /* 0x1C2A0: the page's HTML */
    s32 loadPos;                /* 0x1C2A4 */
    u8 *cert0;                  /* 0x1C2A8: certificate 0x84 (func_8FD770), Http_Init passes it with Http_GetCertSize(0) */
    u8 *cert1;                  /* 0x1C2AC: certificate 0x85, 0x800 bytes after cert0 (Http_GetCertSize(1)) */
    u8 unk1C2B0[0x1D2C0 - 0x1C2B0];
} BrowserWork;
typedef char BrowserWork_size_check[(sizeof(BrowserWork) == 0x1D2C0) ? 1 : -1];

extern BrowserWork *gBrowserWork;
extern BrowserState *gBrowserState;
extern BrowserSub *gBrowserSub;
extern MmbbData *gMmbbData;
extern u8 gBrowserTopMode;

#endif /* BROWSER_H */
