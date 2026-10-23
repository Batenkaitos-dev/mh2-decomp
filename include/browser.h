/* browser.h: the lobby overlay's HTML browser for the KDDI "MMBB" portal:
 * its pages, modes, input, links, forms, URLs, dialogs and the page's draw
 * commands. The HTML parser and the page layout are in html.h; the image
 * decoders use zlib.h.
 *
 * The browser's globals (.bss):
 *   gBrowserWork  BrowserWork *: the work area, &gBrowserWorkMem (119488 bytes)
 *   gBrowserState  BrowserState *: &work->state
 *   gBrowserSub  BrowserSub *: &work->sub
 *   gMmbbData  MmbbData *: the MMBB data, kept across lobby visits in
 *               sub_main's gAccountData (0x5CAED0) (38850 bytes: the
 *               login id and password, the lobby server's user-id and
 *               connection-id ...)
 *   gBrowserTopMode  u8: the browser's top mode (BROWSER_TOP_*)
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
} UrlScheme;         /* 17 bytes */

/* The MMBB data (*gMmbbData: sub_main's gAccountData, 38850 bytes, kept
 * across lobby visits): what the portal's pages hand the game (the page's
 * <CSV> data: Mmbb_SetField's field numbers in brackets), used by the
 * lobby's login (userId and connectionId) and the server lists. */
/* Mmbb_SetField's field numbers, where the field is known (the others
 * are named by number in MmbbData). */
#define MMBB_FIELD_USER_ID 1
#define MMBB_FIELD_CONNECTION_ID 2
#define MMBB_FIELD_SESSION_URL 4
#define MMBB_FIELD_NEXT_URL 5
#define MMBB_FIELD_RESULT 6             /* the reply's result: "OK", or an error code */
#define MMBB_FIELD_SERVER_ADDR 7        /* a lobby server's "host:port" */
#define MMBB_FIELD_SERVER_ROW 8         /* a lobby server's "key,text,value" (LbsRow) */
#define MMBB_FIELD_ERROR_TEXT 11        /* the error's message, when the result isn't "OK" */
#define MMBB_FIELD_SERVER_USERS 12      /* a lobby server's users, as text */
#define MMBB_FIELD_SERVER_CAPACITY 13   /* a lobby server's capacity, as text */

typedef struct MmbbData {
    char userId[0x11];        /* 0x0000 [1]: the "user-id" (at most 14) */
    char connectionId[0xB];   /* 0x0011 [2]: the "connection-id" (10) */
    char sessionUrl[0x101];   /* 0x001C [4]: where a session reply posts the session's ids (Portal_HandleData) */
    char nextUrl[0x101];      /* 0x011D [5]: where the browser goes next */
    char result[0x11];        /* 0x021E [6]: "OK", or the error code (the error page shows it) */
    char serverAddr[40][0x62]; /* 0x022F [7]: each lobby server's "host:port" (LobbyNet_ConnectServer) */
    char serverRow[40][0x101]; /* 0x117F [8]: each lobby server's "id,name,value" (LbsRow's key, text and value) */
    char f9[40][0x101];       /* 0x39A7 [9] */
    char errorText[0x101];    /* 0x61CF [11]: the error's message */
    char serverUsers[40][0x21]; /* 0x62D0 [12]: each lobby server's users */
    char serverCapacity[40][0x21]; /* 0x67F8 [13]: each lobby server's capacity */
    char f14[0x11];           /* 0x6D20 [14] */
    char f15[0x101];          /* 0x6D31 [15] */
    char f16[40][3];          /* 0x6E32 [16] */
    char f17[40][5];          /* 0x6EAA [17] */
    char f18[40][0x101];      /* 0x6F72 [18] */
    u8 unk979A[0x28];         /* 0x979A */
} MmbbData;                   /* 38850 bytes */

/* The lobby servers of the portal's server-list page (gLobbyServers
 * (0x5C09D0), a view of it): a row per <CSV> entry, from the entry's
 * fields 8 ("key,text,value"), 12 and 13; Lbs_Sort sorts the rows by key,
 * and the entries' MMBB data with them. */
typedef struct LbsRow {
    char key[0xD];     /* 0x000: field 8's first item (at most 12 characters) */
    char text[0xF7];   /* 0x00D: its second (at most 244) */
    s32 value;         /* 0x104: its third ("0x..." hex, or decimal) */
    s32 capacity;      /* 0x108: field 13 as a number: the server's capacity (the lobby prints users/capacity) */
    s32 users;         /* 0x10C: field 12 as a number: its users */
    u32 free;          /* 0x110: capacity - users, set by the lobby before it sorts the servers by it */
} LbsRow;              /* 276 bytes */

typedef struct LbsList {
    LbsRow row[40];    /* 0x0000 */
    u8 unk2B20[8];     /* 0x2B20 */
    u8 count;          /* 0x2B28 (sub_main 0x5C34F8) */
} LbsList;

extern LbsList gLobbyServers;

typedef struct BrowserSub {
    u8 dlgStep;                 /* the sub-dialog's step (Dialog_Run's table) */
    u8 dlgMode;                 /* the dialog's step, Dialog_RunStep's table: BDLG_MODE_* */
    u8 dlgCursor;               /* the dialog's choice (Browser_OpenDialog's arg; 20 pixels a line) */
    u8 moveKind;                /* the last Browser_Move's kind (cleared when pressTimer ends) */
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
    s32 activeObj;              /* the page object in use: the open select list, or the text box the keyboard writes to (an obj[] index) */
} BrowserSub; /* 36 bytes */

typedef struct BrowserState {
    u8 prevPage;                /* 0x000: the page shown before (set from page) */
    u8 mode;                    /* 0x001: index into the running top mode's table of modes */
    u8 step;                    /* 0x002: index into the mode's table of steps */
    u8 sub;                     /* 0x003 */
    s32 x;                      /* 0x004 */
    s32 y;                      /* 0x008 */
    s32 pageW;                  /* 0x00C: the page's width (x scrolls up to pageW - PAGE_VIEW_W) */
    s32 pageH;                  /* 0x010: the page's height (y scrolls up to pageH - PAGE_VIEW_H) */
    u32 bgColor;                /* 0x014: the page's background colour (Html_SetBgColor; BROWSER_BLACK at start); Page_DrawObjBack draws bevels' lit edges grey on white (-1) */
    u16 style;                  /* 0x018: the page's style flags (PAGE_STYLE_*, Page_SetStyle; HTML_PAGE_STYLE_DEFAULT at start) */
    s16 marginTop;              /* 0x01A: the margins (Page_SetMargins) */
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
    u8 page;                    /* 0x02E: where the page on show is in its load: enum PageLoad */
    u8 prevPageMode;            /* 0x02F: pageMode before the last Page_SetDataMode */
    u8 pageMode;                /* 0x030: the page's <CSV> block kind, CSV_KIND_* (Page_SetDataMode); the session, server-list and result kinds are data pages without form fields (Page_IsDataPage) */
    u8 menuReached;             /* 0x031: 1 from the portal's menu page or the sign-up's done page (Page_SetKind) */
    u8 unk32;                   /* 0x032 */
    u8 fromTop;                 /* 0x033: 1 from the portal's top page until the login id dialog (Page_SetKind) */
    u8 plain;                   /* 0x034: 1: the portal's pages 1 and 2 (no frame colour, no back button) */
    s8 pageKind;                /* 0x035: what the page is to the game: enum PageKind (Page_SetKind; PAGE_KIND_NONE before) */
    s8 initMode;                /* 0x036: the argument of Browser_Init (1: ...) */
    u8 styled;                  /* 0x037: 1: Page_SetStyle ran for the page */
    u8 styleWait;               /* 0x038: 30 frames from Page_SetStyle (no reload meanwhile) */
    u8 forceStyle8;             /* 0x039: 1: the reply was status 14 or 2 (Page_SetStyle adds style flag 8) */
    s8 fade;                    /* 0x03A: the fade: 16 in, -16 out (no reload meanwhile) */
    char url[0x100];            /* 0x03B: the URL asked for (Browser_Go) */
    char baseUrl[0x100];        /* 0x13B: the page's base URL, that relative links resolve against (Html_SetBase) */
    u8 unk23B[0x33B - 0x23B];
    char msg[0x100];            /* 0x33B: the page's message (Page_SetMode) */
    char urlAnchor[0x100];      /* 0x43B: the URL's #fragment: the page scrolls to the anchor of that name (Portal_PageReady) */
    u8 unk53B[0x55D - 0x53B];
    char formLoginId[0x11];     /* 0x55D: the login id typed in the page's form (its field 1; Form_SetLoginId, Form_Submit) */
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
    u8 active;                  /* 0x000: update is called while set; a page object's draw-command sub-kind (PAGE_IMG_*, PAGE_SELECT_*), nonzero while its slot is used */
    u8 visible;                 /* 0x001: draw is called while set; a page object's: on the page view this frame (Page_DrawObj) */
    u8 kind;                    /* 0x002: a page object's kind (enum PageObjKind, from Draw_Cmd) */
    u8 layer;                   /* 0x003: the list it is in (PageObj_Alloc) */
    u8 size;                    /* 0x004: the text size (Draw_Cmd) */
    u8 sub;                     /* 0x005: a page object's own state: a button pressed, a radio button or check box checked (1), an image's load state (PAGE_IMG_REQUESTED, _LOADED, _FAILED), a cell's part (PAGE_CELL_*); Draw_Cmd first stores its maxlen there */
    s8 state;                   /* 0x006: a dialog object's state: BOBJ_STATE_*, or the dialog box's BDLG_BOX_*; its update sets it, its draw follows it */
    s8 glowDir;                 /* 0x007: the dialog box's glow: GLOW_RISING or GLOW_FALLING (DlgBox_Draw) */
    u8 unk008[0x00A - 0x008];
    s16 blink;                  /* 0x00A: the pointer object's: the focus frame's pulse, 0 to 255 by glow a frame; the dialog box's: a frame counter for the loading mark, 0 to 600 */
    s16 glow;                   /* 0x00C: the dialog box's: its glow, 0 to 255 in steps of 15; the pointer object's: blink's step, +15 or -15 (the focus frame is red while it rises) */
    s16 unkE;                   /* 0x00E: seen: only cleared, by the dialog objects' create functions */
    s16 unk10;                  /* 0x010: seen: only cleared, as unkE */
    u8 unk012[0x014 - 0x012];
    BrowserObjFunc update;      /* 0x014: called each frame while active (PageObj lists, browser_urls.c) */
    BrowserObjFunc draw;        /* 0x018: called each frame while visible */
    struct BrowserObj *prev;    /* 0x01C */
    struct BrowserObj *next;    /* 0x020 */
    u8 unk024[0x030 - 0x024];
    u8 timer;                   /* 0x030: a scroll bar's: frames an arrow has shown pressed */
    u8 unk031[0x034 - 0x031];
    f32 x;                      /* 0x034 */
    f32 y;                      /* 0x038 */
    u8 unk03C[0x040 - 0x03C];
    f32 w;                      /* 0x040 */
    f32 h;                      /* 0x044 */
    u8 unk048[0x04C - 0x048];
    s32 unk4C;                  /* 0x04C: seen: only cleared, as unkE */
    s32 unk50;                  /* 0x050: seen: only cleared, as unkE */
    u8 unk054[0x058 - 0x054];
    s32 color;                  /* 0x058: the draw command's color (Draw_Cmd); the draws test it signed: negative, none */
    u8 unk05C;                  /* 0x05C: seen: Page_DrawObj passes it to Page_DrawTextLine, which doesn't read it */
    u8 field;                   /* 0x05D: by kind: a text box's login field (1 the id, 2 the password, 3 ...; Form_Submit keeps their values); text's bold flag; a table's or cell's border width; an image button's kind (low 4 bits, 0x10 its second highlight, 0x80 no frame while it loads) */
    u8 group;                   /* 0x05E: a drop-down list's number, from 1, and its options' (Draw_Cmd) */
    u8 tex;                     /* 0x05F: texture slot; a drop-down option's index in its list (from 1) */
    char *s1;                   /* 0x060: href, name ... */
    char *s2;                   /* 0x064: value ... */
    char *s3;                   /* 0x068: the draw command's third string */
    u8 unk06C[0x070 - 0x06C];
} BrowserObj;
typedef char BrowserObj_size_check[(sizeof(BrowserObj) == 0x70) ? 1 : -1];

typedef struct BrowserNodeFlags {
    u8 serial; /* gBrowserWork->loadSerial when the node was made: tells one page load's images from another's */
    u8 flags; /* BTAG_*: BrowserTag_SetFlag sets, BrowserTag_TestFlag tests */
} BrowserNodeFlags;

/* BrowserNodeFlags.flags. */
#define BTAG_POSTED 0x1     /* the page was loaded by a post (BrowserReq_Post) */
#define BTAG_SJIS 0x4       /* the reply was Shift-JIS: a form is sent from it unconverted */

/* BrowserNode.status: a node's state in its list. The list walks stop at
 * the first free node. */
enum BrowserNodeStatus {
    BNODE_FREE = 0,
    BNODE_TAKEN = 1,        /* named (NameList_Take, NameCache_Take) */
    BNODE_CACHED = 2,       /* a cache node reused as it is (PageCache_Get, ImgCache_Get) */
    BNODE_LOADING = 3,      /* a cache node to fetch; a request started */
    BNODE_QUEUED = 4,       /* a request waiting for BrowserReq_UpdateAll */
    BNODE_DONE = 5,         /* a request finished (BrowserReq_TakeHttpResult) */
    BNODE_FAILED = 6
};

/* BrowserReqNode.type: what a request loads. */
enum BrowserReqType {
    BREQ_PAGE = 1,          /* a page (BrowserReq_StartPage) */
    BREQ_IMAGE = 2,         /* an image (BrowserReq_StartImage) */
    BREQ_DOWNLOAD = 3       /* a file into the download node (BrowserReq_StartDownload) */
};

/* BrowserReqNode.loadKind: how a page is loaded. */
enum BrowserLoadKind {
    BLOAD_PAGE = 1,         /* BrowserReq_Page */
    BLOAD_REFETCH = 2,      /* a memory:// page fetched again (PageCache_Get); no cache */
    BLOAD_POST = 3,         /* BrowserReq_Post: sent as POST */
    BLOAD_FORWARD = 4,      /* BrowserHist_Forward */
    BLOAD_BACK = 5,         /* BrowserHist_Back */
    BLOAD_RELOAD = 6        /* BrowserReq_Reload; no cache */
};

/* BrowserWork.lastNav: the last navigation, which BrowserHist_Undo undoes. */
enum BrowserNav {
    BNAV_NONE = 0,
    BNAV_PAGE = 1,
    BNAV_POST = 2,
    BNAV_FORWARD = 3,
    BNAV_BACK = 4,
    BNAV_RELOAD = 5         /* also the history moves with nowhere to go; images of older loads are dropped */
};

/* BrowserReqNode.result and BrowserResult.result: how a request ended. */
enum BrowserResultCode {
    BRES_OK = 0,
    BRES_NO_NETWORK = 1,
    BRES_FAILED = 2,        /* a bad URL, an unknown file type or another error */
    BRES_CONNECT = 4,       /* the connection failed */
    BRES_SEND = 5,          /* the request couldn't be sent */
    BRES_TIMEOUT = 6,
    BRES_IMAGE_FAILED = 8,  /* the image's texture couldn't be made (ImgCache_MakeTexture) */
    BRES_CANCELLED = 9,
    BRES_NOT_CACHED = 10,   /* a posted page that isn't in the cache (PageCache_Get) */
    BRES_REDIRECTED = 11,
    BRES_NEED_USER = 12,    /* the server asks for a user name */
    BRES_NEED_PROXY_USER = 13, /* the proxy asks for a user name */
    BRES_SERVER_ERROR = 14  /* an error status from the server */
};

/* Url_FileType's answers. */
#define URL_TYPE_NONE 0
#define URL_TYPE_HTML 1
#define URL_TYPE_PNG 2
#define URL_TYPE_BMP 3
#define URL_TYPE_MMBBIN 4

/* The lists' sizes (BrowserCache_Init). */
#define PAGE_CACHE_COUNT 3
#define PAGE_BUF_SIZE 0x8000    /* a cached page's buffer */
#define IMG_CACHE_COUNT 32
#define HIST_COUNT 32
#define REQ_COUNT 20
#define CONN_BUF_SIZE 0x4000    /* the reply buffer of each HttpReq slot */

/* The page's objects: BrowserWork.obj's length (Draw_Cmd takes the first
 * free one; the draws walk them in order). */
#define PAGE_OBJ_COUNT 500
/* The size of BrowserWork.cmdBuf, where Draw_Cmd keeps its objects'
 * strings; a command that doesn't fit is dropped. */
#define BROWSER_CMD_BUF_SIZE 32768

/* A page object's kind (BrowserObj.kind of the objects in BrowserWork.obj):
 * the draw command that made it (Draw_Cmd, from the Draw_* makers the
 * HTML layout calls). For these objects BrowserObj.active holds the
 * command's sub-kind (PAGE_IMG_*, PAGE_SELECT_*; nonzero: the slot is
 * used) and BrowserObj.sub the object's own state (a button pressed, a
 * box checked, an image loaded: PAGE_IMG_LOADED). */
enum PageObjKind {
    PAGE_OBJ_TITLE = 0,         /* the page's title (Draw_Title) */
    PAGE_OBJ_TEXT = 1,
    PAGE_OBJ_LINK = 2,
    PAGE_OBJ_SUBMIT = 3,        /* a form's submit button */
    PAGE_OBJ_RESET = 4,         /* a form's reset button */
    PAGE_OBJ_BUTTON = 5,        /* a page button (Draw_PageButton) */
    PAGE_OBJ_TEXT_INPUT = 6,
    PAGE_OBJ_PASSWORD = 7,      /* a password box: drawn as asterisks */
    PAGE_OBJ_SELECT = 8,        /* a drop-down list, and each of its options */
    PAGE_OBJ_RULE = 10,         /* a horizontal rule */
    PAGE_OBJ_RADIO = 11,
    PAGE_OBJ_CHECKBOX = 12,
    PAGE_OBJ_IMAGE = 13,        /* images, backgrounds and image inputs (PAGE_IMG_*) */
    PAGE_OBJ_ANCHOR = 14,       /* a named place in the page */
    PAGE_OBJ_TABLE = 15,
    PAGE_OBJ_CELL = 16,         /* a table cell: two objects, its border and its fill */
    PAGE_OBJ_STRUCK = 17,       /* struck-out text, underlined too when it is a link */
    PAGE_OBJ_UNDERLINED = 18,   /* underlined text; drawn as a link is */
    PAGE_OBJ_FORM = 19,         /* a form's start */
    PAGE_OBJ_HIDDEN = 20        /* a form's hidden value */
};

/* An image object's sub-kind (BrowserObj.active of a PAGE_OBJ_IMAGE). */
#define PAGE_IMG_BODY_BACK 1    /* the page's background image (Draw_BodyBackground) */
#define PAGE_IMG_IMAGE 2
#define PAGE_IMG_MAP 3          /* an image map */
#define PAGE_IMG_CELL_BACK 4    /* a table's or cell's background, tiled (Page_TileImage) */
#define PAGE_IMG_INPUT 5        /* a form's image button */
/* The images a page may ask for (BrowserState.imgCount); the others fail. */
#define PAGE_IMG_MAX 20
/* An image object's state (BrowserObj.sub; 0 before its request). */
#define PAGE_IMG_REQUESTED 1
#define PAGE_IMG_LOADED 2
#define PAGE_IMG_FAILED 3

/* A drop-down list's sub-kinds: Draw_Cmd's for its start, each option and
 * its end; then BrowserObj.active of the list object, as the draws read it. */
#define PAGE_SELECT_BEGIN 10
#define PAGE_SELECT_OPTION 20
#define PAGE_SELECT_END 30
#define PAGE_SELECT_CLOSED 11
#define PAGE_SELECT_PRESSED 12  /* its arrow drawn pressed until the press timer ends */
/* An option's BrowserObj.active: PAGE_SELECT_OPTION + 1 while its list is
 * being made, then PAGE_SELECT_OPTION + 2 once the list's end came. */
#define PAGE_SELECT_OPTION_OPEN 21
#define PAGE_SELECT_OPTION_DONE 22

/* A table cell's two objects (BrowserObj.sub of a PAGE_OBJ_CELL). */
#define PAGE_CELL_BORDER 1
#define PAGE_CELL_FILL 2

/* The browser's widget sprites (Page_DrawWidget, and Page_DrawMark for the
 * pointer and the boxes), from the browser's own textures (PageTex_Get). */
enum PageWidget {
    PAGE_WIDGET_POINTER = 0,        /* Page_DrawMark: the pointer */
    PAGE_WIDGET_POINTER_ACTIVE = 1, /* Page_DrawMark: the pointer over something it can act on */
    PAGE_WIDGET_IMGBTN_1 = 2,       /* 2 to 5: an image button's highlights (Page_DrawFocus, by its kind) */
    PAGE_WIDGET_IMGBTN_2 = 3,
    PAGE_WIDGET_IMGBTN_3 = 4,
    PAGE_WIDGET_IMGBTN_4 = 5,
    PAGE_WIDGET_STATUS_BAR = 6,
    PAGE_WIDGET_TEXT_BOX = 7,
    PAGE_WIDGET_BUTTON = 8,
    PAGE_WIDGET_BUTTON_PRESSED = 9,
    PAGE_WIDGET_ARROW_UP = 10,      /* 10 to 13: the scroll arrows */
    PAGE_WIDGET_ARROW_DOWN = 11,
    PAGE_WIDGET_ARROW_LEFT = 12,
    PAGE_WIDGET_ARROW_RIGHT = 13,
    PAGE_WIDGET_ARROW_UP_PRESSED = 14, /* 14 to 17: the same, pressed */
    PAGE_WIDGET_ARROW_DOWN_PRESSED = 15,
    PAGE_WIDGET_ARROW_LEFT_PRESSED = 16,
    PAGE_WIDGET_ARROW_RIGHT_PRESSED = 17,
    PAGE_WIDGET_VTRACK = 18,        /* the vertical scroll bar's track */
    PAGE_WIDGET_VTHUMB = 19,        /* and its thumb */
    PAGE_WIDGET_HTRACK = 20,        /* the horizontal scroll bar's track */
    PAGE_WIDGET_HTHUMB = 21,        /* and its thumb */
    PAGE_WIDGET_SELECT = 22,        /* a closed drop-down list */
    PAGE_WIDGET_SELECT_PRESSED = 23,
    PAGE_WIDGET_RADIO_OFF = 24,     /* Page_DrawMark: 24 to 27 the radio buttons and check boxes */
    PAGE_WIDGET_RADIO_ON = 25,
    PAGE_WIDGET_CHECK_OFF = 26,
    PAGE_WIDGET_CHECK_ON = 27
};

/* A dialog object's glow direction (BrowserObj.glowDir; DlgBox_Update). */
#define GLOW_RISING 1
#define GLOW_FALLING 2

/* Browser_Move's kinds (BrowserSub.moveKind): 1 to 4 the page up, down,
 * left and right; 5 and 6 the open drop-down list up and down. */
#define BMOVE_NONE 0
#define BMOVE_UP 1
#define BMOVE_DOWN 2
#define BMOVE_LEFT 3
#define BMOVE_RIGHT 4
#define BMOVE_LIST_UP 5
#define BMOVE_LIST_DOWN 6

/* The dialog box shown over the page (BrowserWork.dlgBoxKind; DlgBox_Update
 * picks the box object's state from it). */
#define BDLG_KIND_NONE 1            /* no box (the loading mark while images load) */
#define BDLG_KIND_LOADING 2         /* the loading mark */
#define BDLG_KIND_LOADING_SMALL 3   /* the loading mark, small, at the bottom right */
#define BDLG_KIND_MESSAGE 4         /* a message box: asks, then shows the network save's result or the error (errorKind) */
#define BDLG_KIND_YES_NO 5          /* a message box asking yes or no */
#define BDLG_KIND_CLEAR_LOGIN 6     /* a message box: keep or clear the kept login */
#define BDLG_KIND_REGISTER_ASK 7    /* yes or no to send the registration (BrowserDlg_DrawYesNo) */
#define BDLG_KIND_REGISTER_INCOMPLETE 8 /* one button: a registration field is empty (BrowserDlg_DrawOk) */
#define BDLG_KIND_EXIT 11           /* the leave dialog (BrowserDlg_DrawPortalTop) */
#define BDLG_KIND_CLOSE 12          /* frees the box */

/* The dialog's step (BrowserSub.dlgMode): Dialog_RunStep's table. */
#define BDLG_MODE_PAGE 0            /* no dialog: the page has the pad (Dialog_PageStep) */
#define BDLG_MODE_INPUT 1           /* the software keyboard (Dialog_InputStep) */
#define BDLG_MODE_MENU 2            /* the menu of three (Dialog_MenuStep) */
#define BDLG_MODE_CHOICE 3          /* a message box asks (Dialog_ChoiceStep; Browser_OpenDialog) */
#define BDLG_MODE_SAVE_LOGIN 4      /* the login's save and its result (Dialog_SaveLogin) */
#define BDLG_MODE_REGISTER 5        /* Dialog_Register */
#define BDLG_MODE_OK 6              /* Dialog_Ok */
#define BDLG_MODE_EXIT 7            /* Dialog_Exit */

/* The dialog objects' states: the pointer (BrowserWork.dlgPointerState), the
 * keyboard (dlgInputState), the menu (dlgMenuState), the background image
 * (dlgBackImgState). Each object's last state frees it. */
#define BDLG_POINTER_SHOWN 1
#define BDLG_POINTER_HIDDEN 2       /* a dialog has the pad */
#define BDLG_POINTER_CLOSE 3
#define BDLG_INPUT_OPEN 1           /* the keyboard is up */
#define BDLG_INPUT_IDLE 2
#define BDLG_INPUT_CLOSE 3
#define BDLG_MENU_HIDDEN 0
#define BDLG_MENU_SHOWN 1
#define BDLG_MENU_CLOSE 2
#define BDLG_BACK_FIND 1            /* waits for the page's background image to load */
#define BDLG_BACK_SHOWN 2
#define BDLG_BACK_NONE 3            /* no image, or it failed */
#define BDLG_BACK_CLOSE 4
/* The work area's sizes: the 512 KB buffer the page heap is on, the page
 * heap's records, and a URL's buffer (BrowserState.url, baseUrl). */
#define BROWSER_BUF_SIZE 0x80000
#define BROWSER_SMALL_BUF_SIZE 0x10000  /* buf64k */
#define BROWSER_CERT_SIZE 0x800         /* each of the two certificates' room (cert0, cert1) */
/* The browser's data files (Http_ReadBrdata, Http_ReadFile). */
#define BRDATA_FRAME_TEX 0x83           /* the frame's texture (Browser_LoadFrame) */
#define BRDATA_CERT0 0x84               /* the certificates (cert0, cert1) */
#define BRDATA_CERT1 0x85
#define BROWSER_HEAP_RECS 0x400

/* The lobby's image heap (gImgHeap): BrowserWork.buf64k, and its block
 * records (gImgBlocks). */
#define IMG_HEAP_SIZE 0x10000
#define IMG_HEAP_RECS 0x40
#define BROWSER_URL_LEN 0x100

/* The page's view: the screen area the page shows through, inside the
 * margins (x scrolls up to pageW - PAGE_VIEW_W, y up to pageH - PAGE_VIEW_H). */
#define PAGE_VIEW_W 584
#define PAGE_VIEW_H 380

/* BrowserWork.stickTilt and stick2Tilt: a stick tilted fully (Page_ScrollInput
 * scrolls twice as fast). */
#define BROWSER_STICK_FULL 3

/* The object lists (BrowserObj.layer; PageObj_Alloc), each drawn by its
 * dialog object's DrawAll function. */
#define BLAYER_BACK_IMAGE 0
#define BLAYER_PAGE 1                   /* the page and the page's objects */
#define BLAYER_BARS 2                   /* the scroll bars and the status line */
#define BLAYER_MENU 3
#define BLAYER_KEYBOARD 4
#define BLAYER_BOX 6
#define BLAYER_POINTER 8
#define BLAYER_COUNT 9

/* The page's texture slots (BrowserWork.tex; PageTex_Add). */
#define PAGE_TEX_SLOTS 24

/* The page object's state (dlgPageState), the scroll bars' (dlgVScrollState,
 * dlgHScrollState: the arrow pressed shows pressed for 4 frames) and the
 * status line's (dlgStatusState: what the line shows). */
#define BDLG_PAGE_SHOWN 3
#define BDLG_PAGE_CLOSE 5
#define BDLG_SCROLL_HIDDEN 1
#define BDLG_SCROLL_SHOWN 2
#define BDLG_SCROLL_FIRST_PRESSED 3     /* the up (or left) arrow pressed */
#define BDLG_SCROLL_SECOND_PRESSED 4    /* the down (or right) arrow pressed */
#define BDLG_SCROLL_CLOSE 5
#define BDLG_STATUS_TITLE 1             /* the page's title */
#define BDLG_STATUS_URL 2               /* a label and the page's URL */
#define BDLG_STATUS_TARGET 3            /* the text of what the pointer is on (the title object's s2) */
#define BDLG_STATUS_HIDDEN 4
#define BDLG_STATUS_CLOSE 5

/* A dialog object's own state (BrowserObj.state), which its update sets
 * from the work area's state for it and its draw follows. Every object has
 * the first two and the last; the scroll bars show an arrow pressed, the
 * status line shows BDLG_STATUS_TITLE, _URL or _TARGET (the same numbers),
 * and the dialog box is one of BDLG_BOX_*. */
#define BOBJ_STATE_HIDDEN 0
#define BOBJ_STATE_SHOWN 1
#define BOBJ_STATE_FIRST_PRESSED 2      /* a scroll bar's up (or left) arrow pressed */
#define BOBJ_STATE_SECOND_PRESSED 3     /* its down (or right) arrow pressed */
#define BOBJ_STATE_FREE 99              /* the draw frees the object and counts it */

/* The dialog box object's states (DlgBox_Update, from dlgBoxKind). */
#define BDLG_BOX_HIDDEN 0
#define BDLG_BOX_LOADING 1              /* the loading mark, glowing */
#define BDLG_BOX_LOADING_SMALL 2        /* the loading mark, small, at the bottom right */
#define BDLG_BOX_MESSAGE 3              /* BrowserDlg_DrawMsgBox */
#define BDLG_BOX_YES_NO 4               /* BrowserDlg_DrawYesNo */
#define BDLG_BOX_OK 5                   /* BrowserDlg_DrawOk */
#define BDLG_BOX_EXIT 7                 /* BrowserDlg_DrawPortalTop, the leave dialog */

/* The registration record the portal's sign-up form fills (gRegRecord;
 * a copy is kept in gLobbyPageData). Each of the form's registration text
 * boxes (a text box whose field is 4) is named by two digits, and the
 * keyboard writes its text into the record's field for that name; the
 * record must have every field filled before it is sent. What each field
 * holds isn't shown by the code. */
typedef struct RegRecord {
    s8 box01[0x41];             /* 0x000: box "01" */
    s8 box03[0xB];              /* 0x041: box "03": 8 characters */
    s8 box05[0x81];             /* 0x04C: box "05" */
    s8 box00[0x81];             /* 0x0CD: boxes "00" and "08" */
    s8 box02;                   /* 0x14E: box "02": a number, 1 to 99 */
    s8 box07[0x81];             /* 0x14F: box "07" */
} RegRecord;

extern RegRecord gRegRecord;

/* Where the page on show is in its load (BrowserState.page): its request,
 * the parse of its HTML (Html_LoadStep), then its images. */
enum PageLoad {
    PAGE_LOAD_NONE = 0,             /* nothing loaded yet (the browser's start) */
    PAGE_LOAD_REQUESTED = 1,        /* seen: only tested, with the request still out (BrowserWork.reqBusy) */
    PAGE_LOAD_STOPPED = 2,          /* the pad stopped the load before the page came */
    PAGE_LOAD_PARSING = 3,          /* the page came; its HTML is being parsed */
    PAGE_LOAD_PARSE_STOPPED = 4,    /* the pad stopped the parse: the history goes back a page */
    PAGE_LOAD_PARSING_ON = 5,       /* Html_LoadStep's other unfinished result (1): the small loading mark */
    PAGE_LOAD_PARSED = 6,           /* the parse ended, or failed */
    PAGE_LOAD_IMAGES = 8,           /* the page's images are still loading */
    PAGE_LOAD_IMAGES_STOPPED = 9,   /* the pad stopped the images' load (Portal_DialogStep) */
    PAGE_LOAD_DONE = 10             /* the page and its images are in */
};

/* What a portal page is to the game (BrowserState.pageKind; Page_SetKind,
 * from the keyword the HTML parser finds in the page; -1 none). */
enum PageKind {
    PAGE_KIND_NONE = -1,
    PAGE_KIND_TOP = 0,              /* the portal's top page */
    PAGE_KIND_MENU = 1,             /* the portal's menu page, reached after the login */
    PAGE_KIND_SIGNUP_FORM = 2,      /* the sign-up form */
    PAGE_KIND_SIGNUP_DONE = 3,      /* the sign-up's done page */
    PAGE_KIND_PASSWORD_FORM = 4,    /* the password change's form */
    PAGE_KIND_PASSWORD_DONE = 5,    /* the password change's done page */
    PAGE_KIND_SIGNUP_TOP = 6,       /* the sign-up's top page */
    PAGE_KIND_LOGIN_DONE = 7,       /* the login's done page */
    PAGE_KIND_MMBB_KEICAN = 8,      /* opens the dialog that keeps or clears the kept login */
    PAGE_KIND_MMBB_CANCAN = 9       /* the other MMBB page: Page_SetKind does nothing for it */
};

/* What the login dialog's answer keeps (BrowserWork.loginSave): the page's
 * kind times 10 plus the answer, LOGIN_SAVE(kind, answer). Browser_KeepLogin
 * copies the login id and password it says into the kept login. */
#define LOGIN_SAVE(kind, answer) ((kind) * 10 + (answer))
#define LOGIN_KEEP_NONE 0               /* keep nothing new */
#define LOGIN_KEEP_ID 1                 /* keep the login id; the password is cleared */
#define LOGIN_KEEP_ID_PASSWORD 2        /* keep the login id and the password */
#define LOGIN_CLEAR 2                   /* after PAGE_KIND_MMBB_KEICAN: clear the kept login */

/* Browser_GetText's groups: the browser's message tables. */
#define BTEXT_ERROR_PAGE 2          /* gBrowserErrorPages */
#define BTEXT_MENU 3                /* gBrowserMenuTexts: the menu of three */
#define BTEXT_STATUS 4              /* the status line's label */
#define BTEXT_NET_SAVE 5            /* gNetFileSaveTexts */
#define BTEXT_CONNECT_FAIL 6        /* gConnectFailTexts */
#define BTEXT_REGISTER 7            /* gRegisterConfirmTexts */
#define BTEXT_COMM_ERROR 8          /* gCommErrorTexts */

/* The page's style flags (BrowserState.style; Page_SetStyle). A page
 * sets them with its <GAME-STYLE> keywords (enum GameStyleKeyword). */
#define PAGE_STYLE_POINTER 0x1          /* the stick moves a free pointer; clear, the screen dims behind dialogs */
#define PAGE_STYLE_VSCROLL 0x2          /* the vertical scroll bar */
#define PAGE_STYLE_STATUS_BAR 0x4       /* the status bar */
#define PAGE_STYLE_BACK 0x8             /* seen: the cancel button (or Backspace) follows the page's message link or goes back; error replies force it (forceStyle8) */
#define PAGE_STYLE_FORWARD 0x10         /* the <GAME-STYLE> FORWARD keyword's bit; seen: with PAD_TRIANGLE the page goes forward */
#define PAGE_STYLE_CANCEL 0x20          /* the CANCEL keyword's bit; seen: tested with PAD_START on a styled page */
#define PAGE_STYLE_RELOAD 0x40          /* the RELOAD keyword's bit */
#define PAGE_STYLE_UNDERLINE_LINKS 0x80 /* links and underlined text are drawn with their line */
#define PAGE_STYLE_MSG_ICON 0x100       /* the message icon and its label, when the page has a message */
#define PAGE_STYLE_NO_MSG_ICON 0x200    /* seen: hides them, whatever PAGE_STYLE_MSG_ICON says */

/* The browser's colours (0xAARRGGBB). Black is 0xFF000001, not 0xFF000000:
 * a colour of 0 means "untinted" to the sprite draws (Page_DrawWidget). */
#define BROWSER_BLACK 0xFF000001
#define BROWSER_BEVEL_LIGHT 0xFFD0D0D0  /* a frame's lit edge */
#define BROWSER_BEVEL_DARK 0xFF606060   /* a frame's shaded edge */
#define BROWSER_CURSOR_RED 0xFFFE2B2C   /* the focus frame and the dialogs' cursor, while the glow brightens */
#define BROWSER_LIST_HOVER 0x600000FF   /* translucent blue: the drop-down list's row under the pointer */
#define BROWSER_LIST_CHOSEN 0x6000FF00  /* translucent green: its chosen option */
#define BROWSER_DIM 0x80010101          /* half-transparent black, over the page behind a dialog */

/* The open drop-down list (Page_DrawDropList): rows of 18 pixels, at most
 * 10 shown (a scroll bar beyond). */
#define DROPLIST_ROW_H 18
#define DROPLIST_ROWS 10
#define DROPLIST_PAGE (DROPLIST_ROWS - 1)   /* a page of the list: the rows shown less one */
#define DROPLIST_TRACK_H 144    /* its scroll bar's track: the rows' height less the two arrows */

/* BrowserWork.pressTimer's start: the frames a pressed arrow or button is
 * drawn pressed. */
#define BROWSER_PRESS_FRAMES 3

/* A node of the work area's name lists (heads BrowserWork.pageList, imgList,
 * histList and reqList; each head is a node too). The entries of a list extend it. */
typedef struct BrowserNode {
    struct BrowserNode *next; /* 0x000 */
    char name[0x100];      /* 0x004 */
    s32 status;            /* 0x104: enum BrowserNodeStatus */
    BrowserNodeFlags tag;          /* 0x108 */
} BrowserNode;                /* 0x10C */

/* An entry of the list BrowserWork.pageList. */
typedef struct BrowserPageNode {
    BrowserNode node; /* 0x000 */
    u8 *buf;       /* 0x10C: PAGE_BUF_SIZE bytes */
    s32 size;      /* 0x110: the page's length (BrowserReq_Poll); the download node: its buffer's size */
} BrowserPageNode;

/* An entry of the list BrowserWork.imgList. */
typedef struct BrowserImgNode {
    BrowserNode node; /* 0x000 */
    char *data;    /* 0x10C: the image data (a reply buffer) */
    s32 size;      /* 0x110: its size */
    u32 handles;   /* 0x114: two 16-bit handles (texture, palette) */
    u16 w;         /* 0x118 */
    u16 h;         /* 0x11A */
} BrowserImgNode;

/* An entry of the list BrowserWork.reqList. */
typedef struct BrowserReqNode {
    BrowserNode node; /* 0x000 */
    s8 loadKind;   /* 0x10C: enum BrowserLoadKind */
    s8 type;       /* 0x10D: enum BrowserReqType */
    char *referer; /* 0x110: the page the request comes from (BrowserHist_CurUrl), sent as HttpReq.referer */
    s32 cacheNode; /* 0x114: the BrowserPageNode or BrowserImgNode the request fills (the download node for BREQ_DOWNLOAD; 0 when the URL fails) */
    struct HttpReq *req; /* 0x118: made by HttpReq_Alloc */
    char *buf;     /* 0x11C: the reply (the connection buffer) */
    s32 len;       /* 0x120: its length */
    s8 result;     /* 0x124: enum BrowserResultCode */
    s8 errStep;     /* 0x125: the connection step that failed (0 none) */
} BrowserReqNode;

/* The result BrowserReq_Poll returns (BrowserWork.reqResult). */
typedef struct BrowserResult {
    void *data; /* 0x0 */
    s8 result;  /* 0x4: enum BrowserResultCode */
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
    s8 loginSave;               /* 0xE92: what the login dialog's answer keeps: LOGIN_SAVE(the page's kind, LOGIN_KEEP_* or LOGIN_CLEAR); Browser_KeepLogin */
    s8 keepLoginStep;           /* 0xE93 */
    s8 endReason;               /* 0xE94: why the browser ends: 1 network error, 2 left by the user, 3 the retry failed (>0: only the dialog boxes draw) */
    u8 netError;                /* 0xE95 */
    s8 endGoOn;                 /* 0xE96: 1: the end goes on to gMmbbData->nextUrl (PortalEnd_Start), 0: Browser_Exit(-1) */
    u8 formGoLink;              /* 0xE97: set: a form goes to its link object's URL and skips fields named URL (only cleared in the C so far) */
    s8 result;                  /* 0xE98: Browser_Update's result: 0 running, -1 ... */
    u8 atMmbbUrl;               /* 0xE99 */
    u8 loginPosted;             /* 0xE9A */
    s8 gameMenuNo;              /* 0xE9B */
    u8 firstPage;               /* 0xE9C: 1 until the first page is shown (Browser_GoStart only then) */
    u8 tries;                   /* 0xE9D: page tries: the second failure gives up (Browser_Retry, Browser_GoStart; 2: give up) */
    u8 startPosts;              /* 0xE9E: posts to gMmbbData->sessionUrl (more than 1 gives up) */
    u8 texFailOnce;             /* 0xE9F: 1: a failed image may fail the page once (Browser_Init's mode 1) */
    u8 texCount;                /* 0xEA0: page textures in texHandle/texW/texH */
    u8 cmdBuf[0x8000];          /* 0xEA1: the draw commands' strings (BROWSER_CMD_BUF_SIZE) */
    u8 unk8EA1[0x8EA4 - 0x8EA1];
    char *pageData;             /* 0x8EA4: the reply data: the page HTML (Html_StartLoad) */
    u8 *cmdPtr;                 /* 0x8EA8: write position in cmdBuf */
    u8 stickReleased;           /* 0x8EAC: 1: the left stick came back to neutral */
    u8 stickFirst;              /* 0x8EAD: 1: this is the first frame of a tilt (moves at once, then every 8 frames) */
    u8 reqBusy;                 /* 0x8EAE: 1 from a request (BrowserReq_Page, _Post) until its reply (BrowserReq_Poll) */
    u8 dlgBackImgState;         /* 0x8EAF: the page's background image object (DlgBackImg_Update): BDLG_BACK_* */
    u8 dlgPageState;            /* 0x8EB0: dialog kind 1's state (DlgPage_Update; 5 close) */
    u8 dlgVScrollState;         /* 0x8EB1: dialog kind 2's (DlgVScroll_Update) */
    u8 dlgHScrollState;         /* 0x8EB2: dialog kind 3's (DlgHScroll_Update) */
    u8 dlgStatusState;          /* 0x8EB3: dialog kind 4's (DlgStatus_Update) */
    u8 dlgMenuState;            /* 0x8EB4: the menu of three (DlgMenu_Update): BDLG_MENU_* */
    u8 dlgPointerState;         /* 0x8EB5: the pointer and focus (DlgPointer_Update): BDLG_POINTER_* */
    u8 dlgInputState;           /* 0x8EB6: the software keyboard (DlgInput_Update): BDLG_INPUT_* */
    u8 dlgBoxKind;              /* 0x8EB7: the dialog box shown (Browser_OpenDialog's kind): BDLG_KIND_* */
    u8 errorKind;               /* 0x8EB8: Browser_SetErrorState's kind */
    s8 cursorOnLink;            /* 0x8EB9: 1: the selection moved onto a link (restarts the cursor blink) */
    u8 unk8EBA[0x8EC0 - 0x8EBA];
    char kbdText[0x100];        /* 0x8EC0: the software keyboard's text */
    void *memPages;             /* 0x8FC0: the name lists (browser_cache.c: pages, images, history, requests): the page nodes' memory */
    void *memBufs;              /* 0x8FC4 */
    void *memImgs;              /* 0x8FC8 */
    void *memConnBufs;          /* 0x8FCC */
    void *memHist;              /* 0x8FD0 */
    void *memReqs;              /* 0x8FD4 */
    BrowserPageNode *pages;     /* 0x8FD8 */
    BrowserImgNode *imgs;       /* 0x8FDC */
    BrowserPageNode download;   /* 0x8FE0: the one node of downloads (BrowserReq_StartDownload) */
    BrowserNode *hist;          /* 0x90F4 */
    BrowserReqNode *reqs;       /* 0x90F8 */
    u8 *connBuf[8];             /* 0x90FC */
    BrowserNode pageList;       /* 0x911C */
    BrowserNode imgList;        /* 0x9228 */
    BrowserNode histList;       /* 0x9334 */
    BrowserNode *histCur;       /* 0x9440 */
    BrowserNode reqList;        /* 0x9444 */
    u8 lastNav;                 /* 0x9550: the last navigation, enum BrowserNav (BrowserHist_Undo undoes it) */
    u8 unk9551[0x9554 - 0x9551];
    BrowserResult reqResult;    /* 0x9554: BrowserReq_Poll result */
    u8 loadSerial;              /* 0x9564: counts the page loads; a node's tag keeps it (BrowserNodeFlags.serial) */
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
    s32 unk1A640;               /* 0x1A640: seen: only cleared, by PageObj_InitPool */
    s32 unk1A644;               /* 0x1A644: seen: only cleared, as unk1A640 */
    u16 padHeld;                /* 0x1A648: gPadState[0], copied each frame (Browser_ReadPad) */
    u16 padPrevHeld;            /* 0x1A64A: the buttons held the frame before (Browser_ReadPad) */
    u16 padPressed;             /* 0x1A64C */
    u16 padReleased;            /* 0x1A64E */
    u8 stickTilt;               /* 0x1A650: the left stick: tilt (0 to 3) */
    u8 stickDir;                /* 0x1A651: and direction (16 steps) */
    u8 stick2Tilt;              /* 0x1A652: the right stick */
    u8 stick2Dir;               /* 0x1A653 */
    u8 unk1A654[0x1A660 - 0x1A654];
    u8 *buf64k;                 /* 0x1A660: 65536 bytes (Mem_Alloc) */
    u8 *buf512k;                /* 0x1A664: 524288 bytes (Mem_Alloc) */
    void *mem64k;               /* 0x1A668: the Mem_Alloc handle of buf64k */
    void *mem512k;              /* 0x1A66C: the Mem_Alloc handle of buf512k */
    char formData[0x1000];      /* 0x1A670: the form's query: name=value pairs joined by & (BrowserForm_Add escapes them), cleared before a page's fields are added */
    char baseUrl[0x100];        /* 0x1B670: the base for relative URLs (Url_SetBase) */
    char urlLower[0x100];       /* 0x1B770: the URL in lower case (Url_SpecialScheme) */
    char errorText[0x200];      /* 0x1B870: a failed request's message (BrowserReq_Poll copies Browser_GetText(2, k) there: the result's data) */
    u8 errorPage[0x800];        /* 0x1BA70: the error page built in memory (Browser_BuildErrorPage), the memory:// page 0 */
    u8 *memPage[3];             /* 0x1C270: the pages of memory://N URLs (HttpReq_Open, URL_MEMORY); [0] is errorPage, [1] and [2] are cleared */
    u8 unk1C27C[0x1C27D - 0x1C27C];
    u8 exitArgs;                /* 0x1C27D: the arguments of mmbb://EXIT:n (n: 0-9) */
    s16 refreshTimer;           /* 0x1C27E: frames until the META REFRESH URL loads (Html_Refresh) */
    s16 pressTimer;             /* 0x1C280: 3 after a move (Browser_Move, form buttons): the pressed arrow or button is drawn pressed until it ends, and gBrowserSub->moveKind is then cleared */
    char savedId[0xB];          /* 0x1C282: the kept login id before Browser_SaveLogin wrote it (restored on failure) */
    char savedPass[9];          /* 0x1C28D: and its password */
    s8 dlgQuestion;             /* 0x1C296: the question the message box asked last: 1 dialog kind 4's, 2 kind 6's (BrowserDlg_DrawMsgBox; Browser_OpenDialog clears it) */
    u8 unk1C297[0x1C298 - 0x1C297];
    s32 loadState;              /* 0x1C298: 1 while a page is being parsed (Html_StartLoad) */
    s16 loadResult;             /* 0x1C29C: what Html_LoadStep returns a pointer to: 0, 1 parsing, 2 done, 3 failed */
    u8 unk1C29E[0x1C2A0 - 0x1C29E];
    char *loadSrc;              /* 0x1C2A0: the page's HTML */
    s32 loadPos;                /* 0x1C2A4 */
    u8 *cert0;                  /* 0x1C2A8: certificate BRDATA_CERT0 (func_8FD770), Http_Init passes it with Http_GetCertSize(0) */
    u8 *cert1;                  /* 0x1C2AC: certificate BRDATA_CERT1, BROWSER_CERT_SIZE bytes after cert0 (Http_GetCertSize(1)) */
    u8 unk1C2B0[0x1C2C0 - 0x1C2B0];
    u8 afsPartInfo[0x1000];     /* 0x1C2C0: the info area of AFS partition 5, the browser's data files (Http_LoadAfsPartition) */
} BrowserWork;
typedef char BrowserWork_size_check[(sizeof(BrowserWork) == 0x1D2C0) ? 1 : -1];

extern BrowserWork *gBrowserWork;
extern BrowserState *gBrowserState;
extern BrowserSub *gBrowserSub;
extern MmbbData *gMmbbData;
extern u8 gBrowserTopMode;
/* gBrowserTopMode: which pages the browser runs (gBrowserTopModes). */
#define BROWSER_TOP_PORTAL 0    /* the KDDI portal's pages (Browser_RunPortal) */
#define BROWSER_TOP_LOBBY 1     /* the pages of the lobby's leaving (Browser_RunLobby) */
#define BROWSER_TOP_ERROR 2     /* Browser_RunError: prints an error forever; seen: nothing in the C sets it */

/* The browser's mode (BrowserState.mode), the index into the running top
 * mode's table: gBrowserModes for the portal, its own for the
 * lobby's pages. */
enum BrowserMode {
    BMODE_START = 0,    /* the portal: its start steps (Browser_RunStart); the lobby's pages: Browser_LobbyInit */
    BMODE_RUN = 1,      /* the portal: the page steps (Browser_RunPage, enum PageStep); the lobby's pages: their steps (enum LobbyPageStep) */
    BMODE_END = 2,      /* the portal: the end steps (Browser_RunEnd); the lobby's pages: Browser_LobbyExit */
    BMODE_DONE = 3      /* nothing more (Browser_ModeDone) */
};

/* The portal's page steps (BrowserState.step in BMODE_RUN; the portal's step table,
 * Browser_SetStep). */
enum PageStep {
    PAGE_STEP_START = 0,      /* Portal_PageStart */
    PAGE_STEP_LOAD = 1,       /* Portal_LoadPage */
    PAGE_STEP_PARSE = 2,      /* Portal_ParsePage */
    PAGE_STEP_PARSING = 3,    /* Portal_ParseStep */
    PAGE_STEP_READY = 4,      /* Portal_PageReady */
    PAGE_STEP_DIALOG = 5,     /* Portal_DialogStep */
    PAGE_STEP_RESHOW = 6,     /* Portal_ReshowStep */
    PAGE_STEP_HISTORY = 7,    /* Portal_HistoryStep */
    PAGE_STEP_RELOAD = 8,     /* Portal_ReloadStep */
    PAGE_STEP_OPEN = 9,       /* Portal_OpenStep */
    PAGE_STEP_BACK = 10,      /* Portal_BackStep */
    PAGE_STEP_GAME_MENU = 11, /* Portal_GameMenuStep */
    PAGE_STEP_CANCEL = 12,    /* Portal_CancelStep */
    PAGE_STEP_WAIT_REQ = 13,  /* Portal_WaitReqStep: the request still out */
    PAGE_STEP_IMAGES = 14,    /* Portal_ImagesStep: the page's images still loading */
    PAGE_STEP_FAIL = 15       /* Portal_FailStep */
};

/* The portal's end steps (BrowserState.step in BMODE_END; gPortalEndSteps). */
enum PortalEndStep {
    PORTAL_END_START = 0,       /* PortalEnd_Start: posts the ids to the next URL, or closes the frame */
    PORTAL_END_DRAW_LAST = 1,   /* PortalEnd_DrawLast: the frame's objects a last time */
    PORTAL_END_FREE = 2,        /* PortalEnd_FreeObjs: the page's objects freed */
    PORTAL_END_FINISH = 3,      /* PortalEnd_Finish: the browser's result */
    PORTAL_END_WAIT_POST = 4    /* PortalEnd_WaitPost: the post's answer, then the frame closes */
};

/* The history move a step asks for (BrowserState.histDir). */
#define BHIST_NONE 0
#define BHIST_BACK 1
#define BHIST_FORWARD 2

/* The dialogs' step (BrowserSub.dlgStep; Dialog_Run's table). */
#define DLG_STEP_RESET 0        /* Dialog_Reset: the pointer centred, then DLG_STEP_RUN */
#define DLG_STEP_RUN 1          /* Dialog_RunStep: the dialog mode's step (BDLG_MODE_*) each frame */
#define DLG_STEP_STOPPED 2      /* Dialog_Stopped: nothing; set when the portal fails (Browser_Fail) */

/* The menu of three's lines (BrowserSub.dlgCursor in BDLG_MODE_MENU;
 * Dialog_MenuStep). It opens on BMENU_CLOSE. */
#define BMENU_BACK 1            /* the page before (Portal_Back) */
#define BMENU_LEAVE 2           /* leaves the portal (Browser_Fail) */
#define BMENU_CLOSE 3           /* closes the menu */
#define BMENU_LINES 3

/* The lobby's pages' steps (BrowserState.step in BMODE_RUN under
 * BROWSER_TOP_LOBBY; gBrowserLobbySteps). Steps 1 to 4 only go on. */
enum LobbyPageStep {
    LOBBY_PAGE_POST = 0,        /* Browser_LobbyStart: posts the ids to the MMBB data's next URL */
    LOBBY_PAGE_WAIT = 5,        /* Browser_LobbyWait: the reply */
    LOBBY_PAGE_END = 6          /* Browser_LobbyEnd: the last reply, then BROWSER_FAILED */
};

/* The names the ids are sent under, as form fields, to the lobby's pages
 * (BrowserForm_Add): the user id's and the connection id's. One object in
 * the game's .rodata, read in two parts. */
typedef union IdFieldNames {
    char raw[0x20];
    struct {
        char userId[8];
        char connectionId[0x18];
    } name;
} IdFieldNames;
void Browser_Init(u8 mode);
void LobbyKddi_CloseBrowser(void);
s32 Browser_Update(void);
void Browser_LobbyStep1(void);
void Browser_LobbyStep2(void);
void Browser_LobbyStep3(void);
void Browser_LobbyStep4(void);
void Browser_LobbyWait(void);
void Browser_LobbyEnd(void);
void Browser_LobbyExit(void);
void Browser_RunError(void);
void Browser_ModeDone(void);
extern char gLbsScheme[8]; /* the lobby server's URL scheme (URL_LBS) */
extern IdFieldNames gIdFieldNames;

/* gConnCheckState: the lobby's link, found lost while the browser shows a
 * page (the lobby's frame sets it on a receive timeout, the idle timer, the
 * server's close or a network error). */
#define CONN_LOST_NONE 0
#define CONN_LOST_FOUND 1       /* found lost: Browser_CheckConnection answers it */
#define CONN_LOST_SHOWN 2       /* the browser has gone to the failure (or cancel) step */
#define CONN_LOST_ENDED 3       /* the page ended with the link lost */

/* Browser_Update's result (BrowserWork.result; Browser_Exit sets it as the
 * browser ends). A page's game menu number (BrowserWork.gameMenuNo) is
 * returned as it is, from 2 on. */
#define BROWSER_RUNNING 0
#define BROWSER_DONE 1          /* the browser was left */
#define BROWSER_FAILED -1       /* it ended on an error (BrowserWork.endReason) */
#define BROWSER_LOGIN -100      /* the page posted the login (BrowserWork.loginPosted), or the lobby's pages ended after a quit to the portal */

/* Why the browser ends (BrowserWork.endReason; above 0 only the dialog
 * boxes draw). */
#define BROWSER_END_NONE 0
#define BROWSER_END_NET_ERROR 1     /* the network interface reported an error (Browser_CheckNetwork) */
#define BROWSER_END_LEAVE 2         /* the leave dialog: the page couldn't be shown (Page_Load's -2), or the leave was chosen */
#define BROWSER_END_RETRY_FAILED 3  /* a page failed again after its retry */

void NameList_Release(BrowserNode *list, BrowserNode *node);
BrowserNode *NameList_DropFirst(BrowserNode *list, BrowserNode *cur);
void BrowserCache_Init(void);
void BrowserCache_Shutdown(void);
char *BrowserHist_Forward(void);
s32 BrowserHist_CanForward(void);
char *BrowserHist_Back(void);
s32 BrowserHist_CanBack(void);
char *BrowserReq_Reload(void);
s32 BrowserReq_CanReload(void);
char *BrowserHist_CurUrl(void); /* the URL of the history's current entry */
void BrowserReq_Page(char **url);
void BrowserReq_Post(char **url);
void BrowserForm_Clear(void);
void BrowserReq_Image(char **url);
void ImgCache_FreeAll(BrowserNode *list);
void BrowserReq_TakeHttpResult(BrowserReqNode *reqNode);
void BrowserReq_StartPage(BrowserReqNode *reqNode);
void BrowserReq_StartImage(BrowserReqNode *reqNode);
void BrowserReq_StartDownload(BrowserReqNode *reqNode);
void BrowserReq_UpdateAll(void);
BrowserResult *BrowserReq_Poll(char **url);
void BrowserHist_Undo(void);
void BrowserReq_CancelAll(void);
void Draw_RectClipped(s32 color, f32 left, f32 top, f32 right, f32 bottom);
void Page_DrawMark(u8 kind, f32 x, f32 y); /* the pointer: PAGE_WIDGET_POINTER or _POINTER_ACTIVE */
void Page_DrawWidget(u8 kind, f32 x, f32 y, f32 w, f32 h, s32 color); /* one of the PAGE_WIDGET_* pieces */
void Page_DrawImage(u16 texSlot, f32 x, f32 y, f32 w, f32 h, s32 srcW, s32 srcH);
void BrowserDlg_DrawWindow(f32 x0, f32 y0, f32 x1, f32 y1);
void Page_DrawFrame(s32 color, s32 thickness, f32 x0, f32 y0, f32 x1, f32 y1);
void Page_DrawThinFrame(s32 color, f32 x0, f32 y0, f32 x1, f32 y1); /* a one-pixel frame */
void Page_DrawTextLine(u8 unused, u32 color, u8 bold, u8 size, f32 x, f32 y, char *text);
void Page_Printf(const char *fmt, ...);
void Page_DrawObjBack(BrowserObj *pageObj);
void Page_DrawObj(BrowserObj *pageObj);
void Page_DrawDropList(s32 listObjIdx);
void Page_DrawFocus(BrowserObj *cursorObj);
void BrowserDlg_DrawMsgBox(BrowserObj *dlgObj);
void BrowserDlg_DrawYesNo(BrowserObj *dlgObj);
void BrowserDlg_DrawOk(BrowserObj *dlgObj);
void BrowserDlg_DrawPortalTop(BrowserObj *dlgObj);
void Page_TileImage(BrowserObj *pageObj, u8 texSlot);
void Browser_DrawOverlay(void);
s32 Rect_ClipToScreen(f32 *x0, f32 *y0, f32 *x1, f32 *y1);
void Browser_SetRenderState(s32 state, u32 value);
void Prim_DrawTri(u32 color, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2);
void Prim_DrawRect(s32 color, f32 x1, f32 y1, f32 x2, f32 y2);
void Prim_DrawSprite(u32 color, u32 tex, s16 x, s16 y, s16 w, s16 h, s16 u1, s16 v1, s16 u2, s16 v2);
void Html_ResetLoad(void);
void Html_StartLoad(char *src);
void Html_StopLoad(void);
s16 *Html_LoadStep(s32 arg);
char *Browser_GetText(u32 group, s32 i);
void Browser_InitHeap(void);
void *Browser_Alloc(u32 size);
void Browser_Free(void *ptr);
void *Browser_Realloc(void *ptr, u32 size);
void Browser_ReadPad(void);
void Browser_SetUrl(BrowserWork *w, char *url);
void Url_SetBase(char *url);
void Url_ClearBase(void);
char *Url_Resolve(char *base, char *url);
void Str_ToLower(char *str);
s32 Url_SpecialScheme(const char *url);
s32 Url_GetScheme(const char *url);
s32 Url_FileType(char *url);
s32 Url_Escape(char *dst, u8 *src);
void Str_SjisToEuc(u8 *str);
void Str_EucToSjis(u8 *str);
void PageObj_InitPool(void);
BrowserObj *PageObj_Alloc(s32 layer, s32 atTail);
void PageObj_Free(BrowserObj *pageObj);
void PageObj_UpdateLayer(s32 layer);
void PageObj_DrawLayer(s32 layer);
void PageTex_ReleaseAll(void);
u32 PageTex_Get(s8 i);
void Browser_LoadFrame(s32 mode, u8 *step);
void PageObj_Nop(void);
extern UrlScheme gUrlSchemes[7]; /* the schemes the browser loads, then a zero entry */
extern char *gScriptSchemes[2]; /* the script scheme, then NULL */
extern char gScriptSchemes_0[12];

void DlgBackImg_DrawAll(void);
void DlgBackImg_Create(s32 mode);
void DlgBackImg_Update(BrowserObj *obj);
void DlgBackImg_Draw(BrowserObj *obj);
void DlgPage_DrawAll(void);
void DlgPage_Create(s32 mode);
void DlgPage_Update(BrowserObj *obj);
void DlgPage_Draw(BrowserObj *obj);
void DlgVScroll_DrawAll(void);
void DlgVScroll_Create(s32 mode);
void DlgVScroll_Update(BrowserObj *obj);
void DlgVScroll_Draw(BrowserObj *obj);
void DlgHScroll_DrawAll(void);
void DlgHScroll_Create(s32 mode);
void DlgHScroll_Update(BrowserObj *obj);
void DlgHScroll_Draw(BrowserObj *obj);
void DlgStatus_DrawAll(void);
void DlgStatus_Create(s32 mode);
void DlgStatus_Update(BrowserObj *obj);
void DlgStatus_Draw(BrowserObj *obj);
void DlgMenu_DrawAll(void);
void DlgMenu_Create(s32 mode);
void DlgMenu_Update(BrowserObj *obj);
void DlgMenu_Draw(BrowserObj *obj);
void Browser_OpenKeyboard(char *text, s16 max, s32 mode);
char *Browser_RunKeyboard(void);
void DlgInput_DrawAll(void);
void DlgInput_Create(s32 mode);
void DlgInput_Update(BrowserObj *obj);
void DlgInput_Draw(BrowserObj *obj);
void DlgPointer_DrawAll(void);
void DlgPointer_Create(s32 mode);
void DlgPointer_Update(BrowserObj *obj);
void DlgPointer_Draw(BrowserObj *obj);
void DlgBox_DrawAll(void);
void DlgBox_Create(s32 mode);
void DlgBox_Update(BrowserObj *obj);
void DlgBox_Draw(BrowserObj *obj);
void Browser_SaveLogin(void);
void Browser_SetErrorState(s32 kind);
void Browser_EmptyHook(void);
void Portal_FailStep(void);
void Browser_UpdateFrame(void);
void Browser_InitHook(void);
void Draw_Cmd(u8 kind, u8 sub, u16 x, u16 y, u16 x2, u16 y2, u32 color, u8 size, u8 field, u8 maxlen, u8 *s1, u8 *s2, u8 *s3);
s32 Page_Load(void);
void Page_RequestImages(void);
s8 Page_CheckImages(void);
s32 Browser_Timers(void);
void Page_CreateFrame(void);
void Browser_RunEnd(void);
void PortalEnd_Start(void);
void PortalEnd_DrawLast(void);
void PortalEnd_FreeObjs(void);
void PortalEnd_Finish(void);
void PortalEnd_WaitPost(void);
void Dialog_Run(void);
void Dialog_Reset(void);
void Dialog_RunStep(void);
void Dialog_PageStep(void);
void Dialog_InputStep(void);
void Dialog_ResetMsgState(void);
void Dialog_MenuStep(void);
void Dialog_ResetBoxState(void);
void Dialog_ChoiceStep(void);
void Dialog_SaveLogin(void);
void Dialog_Register(void);
void Dialog_Ok(void);
void Dialog_Exit(void);
void Dialog_NoStep(void);
extern char gPageCancelLink[32]; /* a page's message link that cancels the portal */
extern char gPortalTopUrl[48]; /* the URL of the portal's top page */

void Browser_GoForward(void);
void Browser_GoBack(void);
void Portal_Back(void);
void Page_ScrollInput(void);
void Page_DragScroll(void);
void Page_PointerScroll(void);
void Page_PointAt(void);
void Page_Clamp(void);
void Page_Click(void);
s32 Select_ListClick(s32 i);
void Link_Follow(s32 i);
void Form_Reset(s32 button);
void Form_Submit(s32 button);
s8 Portal_HandleData(void);
s32 Portal_SetIds(void);
void Dialog_Stopped(void);
void Browser_Reload(void);
void Browser_OpenPage(void);
void Browser_ShowPage(void);
void Browser_Fail(u8 page);
void Browser_OpenLeaveDialog(s32 unused);
void Browser_SetStep(s8 step);
void Browser_Go(char *url);
void Browser_ScrollBy(s32 dx, s32 dy);
void Browser_Move(s32 dx, s32 dy, u8 kind);
void Browser_Exit(s8 result);
void Dialog_Close(s32 yes);
void Browser_ListCursor(u8 max);
s8 Browser_GameStartError(void);
s32 Browser_GoStart(void);
s32 Browser_Retry(void);
void Browser_BuildErrorPage(char *buf, char *msg, char *code, s32 back);

void Page_New(void);
void Browser_RunStart(void);
void Page_AllocObjs(void);
void Page_FreeObjs(void);
void Portal_Start0(void);
void Portal_Start1(void);
void Browser_RunPage(void);
void Portal_ParseStep(void);
void Portal_PageReady(void);
void Portal_DialogStep(void);
void Portal_ReshowStep(void);
void Portal_HistoryStep(void);
void Portal_ReloadStep(void);
void Portal_OpenStep(void);
void Portal_BackStep(void);
void Portal_GameMenuStep(void);
void Portal_CancelStep(void);
void Portal_WaitReqStep(void);
void Portal_ImagesStep(void);

void Html_NewPage(void);
void Html_Refresh(s32 sec, char *url);
void Html_SetBgColor(u32 rgb);
void Html_SetBase(char *url);
void Draw_Title(u8 *text);
void Draw_Text(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *text, u8 bold);
void Draw_LinkText(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *href, u8 *text, u8 bold);
void Draw_Submit(u16 x, u16 y, u16 right, u16 bottom, u8 *src);
void Draw_Reset(u16 x, u16 y, u16 right, u16 bottom, u8 *src);
void Draw_InputImage(u16 x, u16 y, u16 right, u16 bottom, u8 *name, u8 *src, u8 *alt);
void Draw_PageButton(u16 x, u16 y, u16 right, u16 bottom, u8 *label);
void Draw_TextInput(u16 x, u16 y, u16 right, u16 bottom, u8 *name, u8 *value, u16 maxlen, u8 key);
void Draw_PasswordInput(u16 x, u16 y, u16 right, u16 bottom, u8 *name, u8 *value, u16 maxlen, u8 key);
void Draw_SelectBegin(u8 *action);
void Draw_Option(u8 *name, char *value, u8 selected);
void Draw_SelectEnd(u16 x, u16 y, u16 right, u16 bottom);
void Draw_Rule(u16 x, u16 y, u16 right, u16 bottom);
void Draw_Radio(u16 x, u16 y, u8 checked, u8 *name, u8 *value, u8 key);
void Draw_Checkbox(u16 x, u16 y, u8 checked, u8 *name, u8 *value, u8 key);
void Draw_BodyBackground(u8 *name);
void Draw_Image(u16 x, u16 y, u16 right, u16 bottom, u8 *src, u8 *alt);
void Draw_ImageMap(u16 x, u16 y, u16 right, u16 bottom, u8 *src, u8 *href, u8 *alt, u8 key);
void Draw_Anchor(u16 x, u16 y, u8 *name);
void Draw_Cell(u16 x, u16 y, u16 right, u16 bottom, u8 size, s32 color, u8 border, s32 bcolor, u8 *background);
void Draw_Table(u16 x, u16 y, u16 right, u16 bottom, u8 size, u32 color, u8 border, u8 *background);
void Draw_StruckText(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *text, u8 *href, u8 bold);
void Draw_UnderlinedText(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *text, u8 *href, u8 bold);
void Draw_FormBegin(u8 *action, u8 post);
void Draw_Hidden(u8 *name, u8 *value);
void Page_SetDataMode(u8 mode);
void Form_SetLoginId(u8 *id);
void Browser_OpenDialog(s8 kind, s8 arg);
void Page_SetKind(s8 kind);
void Page_Extend(s32 x, s32 y);
s32 Page_IsDataPage(void);

s32 PageObj_Activate(u16 idx, s32 sound);
void Select_Close(void);
void Page_Navigate(void);

void Prim_Sprite(Sprite *sprite);
void Prim_Rect(PrimLine *rect);
s32 Prim_Tri(PrimTri *tri);

#endif /* BROWSER_H */
