#ifndef YN_H
#define YN_H
#include "net_mode.h"
#include "net_save.h"
#include "window.h"
#include "prim.h"
#include "pad.h"

/*
 * The yn overlay, the network settings: the types its memory card checks
 * and its settings screens share.
 */

/* sub_main's memory card task, gCardTask at 0x6AF100 (the end of sub_main's
 * .bss; CardTask_* run it), as yn reads it. */
typedef struct McState {
    u8 unk0[0x14];
    s32 slot;       /* 0x14: the slot being checked */
    u8 unk18[0x1C];
    s32 type[2];    /* 0x34: per slot: -1 while checking, 1/2 a PS2 card, 3 ... */
    u8 unk3C[0x90];
    s32 busy;       /* 0xCC: nonzero while a card operation runs */
} McState;
extern McState gCardTask;
extern s32 gCardTaskTypes[2]; /* gCardTask.type, as its own symbol: YnCard_CheckCurrentSlot needs its own base */
#define MC_BUSY gCardTask.busy
#define MC_SLOT gCardTask.slot
#define MC_TYPE gCardTask.type

/* A memory card check in progress (YnCard_CheckSlots, YnCard_CheckCurrentSlot, YnCard_ListNetcnfDir). */
typedef struct YnCardCheck {
    s8 state;       /* 0x0: YnCard_CheckSlots and YnCard_CheckCurrentSlot */
    s8 loadState;   /* 0x1: YnCard_ListNetcnfDir */
    u8 okSlots;     /* 0x2: bit slot + 1 for each slot with card type 1 or 2 */
    u8 slots3;      /* 0x3: bit slot + 1 for each slot with card type 3 */
    s8 slot0Done;   /* 0x4: 1 once slot 0's card type is known (the check goes on to slot 1) */
    s8 slot;        /* 0x5: YnCard_SetSlot / YnCard_GetSlot */
} YnCardCheck;

/* One entry of a network configuration file, as Sony's netcnfif library
 * fills it (sceNetcnfifData_t, 4928 bytes). */
typedef struct NetcnfData {
    char attachIfc[0x100];      /* 0x000 */
    char attachDev[0x100];      /* 0x100 */
    char dhcpHostname[0x100];   /* 0x200 */
    char address[0x100];        /* 0x300 */
    char netmask[0x100];        /* 0x400 */
    char gateway[0x100];        /* 0x500 */
    char dns1Address[0x100];    /* 0x600 */
    char dns2Address[0x100];    /* 0x700 */
    char phoneNumbers[3][0x100]; /* 0x800 */
    char authName[0x100];       /* 0xB00 */
    char authKey[0x100];        /* 0xC00 */
    char peerName[0x100];       /* 0xD00 */
    char vendor[0x100];         /* 0xE00 */
    char product[0x100];        /* 0xF00 */
    char chatAdditional[0x100]; /* 0x1000 */
    char outsideNumber[0x100];  /* 0x1100 */
    char outsideDelay[0x100];   /* 0x1200 */
    s32 ifcType;                /* 0x1300 */
    s32 mtu;                    /* 0x1304 */
    s32 ifcIdleTimeout;         /* 0x1308 */
    s32 devType;                /* 0x130C */
    s32 phyConfig;              /* 0x1310 */
    s32 dialingType;            /* 0x1314 */
    s32 devIdleTimeout;         /* 0x1318 */
    s32 p0;                     /* 0x131C */
    u8 dhcp;                    /* 0x1320 */
    u8 dns1Nego;                /* 0x1321 */
    u8 dns2Nego;                /* 0x1322 */
    u8 fAuth;                   /* 0x1323 */
    u8 auth;                    /* 0x1324 */
    u8 pppoe;                   /* 0x1325 */
    u8 prcNego;                 /* 0x1326 */
    u8 accNego;                 /* 0x1327 */
    u8 accmNego;                /* 0x1328 */
    u8 p1[3];
    s32 p4[5];                  /* 0x132C */
} NetcnfData;

/* One entry of a configuration file's list (576 bytes). */
typedef struct NetcnfEntry {
    s32 type;
    s32 stat;
    char sysName[0x100];        /* 0x008 */
    char usrName[0x100];        /* 0x108 */
    u8 unk208[0x38];
} NetcnfEntry;

/* The network configuration files read from a memory card (105792 bytes, in
 * the work area at +0x1180): the lists of the three entry types (0: the
 * combinations, 1: provider settings, 2: device settings), their entries,
 * and the netcnfif call state. YnNetcnf_SetType selects the type. */
typedef struct YnNetcnf {
    NetcnfEntry list0[10];      /* 0x00000 */
    NetcnfEntry list1[30];      /* 0x01680 */
    NetcnfEntry list2[30];      /* 0x05A00 */
    NetcnfData data[10];        /* 0x09D80: the entries of type 0 */
    NetcnfData cur[3];          /* 0x15E00: one per type */
    s32 result;                 /* 0x197C0: the last call's result */
    u8 unk197C4[0x30C];
    s32 count[3];               /* 0x19AD0: entries per type */
    s32 unk19ADC;
    s32 type;                   /* 0x19AE0: the selected type */
    s32 savedType;              /* 0x19AE4 */
    NetcnfEntry *list;          /* 0x19AE8: the selected type's list */
    NetcnfData *curData;        /* 0x19AEC */
    s32 *countp;                /* 0x19AF0 */
    char path[0x40];            /* 0x19AF4: "mc%d:/BWNETCNF/BWNETDCNF" */
    char name[0x100];           /* 0x19B34: the selected entry's interface name (Yn_UpdateEntryList) */
    u8 unk19C34[0x10C];
} YnNetcnf;


/* A combination of the type 0 list with its device (20 bytes, 30 of them
 * at +0x254 of the work area). */
typedef struct YnCombo {
    s32 kind;                   /* Yn_GetDeviceKind: 1 or 2 when the device is known, else 0 */
    s32 device;                 /* Yn_FindDevice: index into sub_main's device table, -1/-2 */
    char *vendor;               /* 0x08 */
    char *product;              /* 0x0C */
    NetcnfData *data;           /* 0x10 */
} YnCombo;

/* The key fields Yn_CompareProxy compares (YnProxy's layout: it is passed YnProxy pointers). */
typedef struct YnEntryKey {
    s32 use;                    /* 0x000 */
    u16 httpPort;               /* 0x004 */
    u16 httpsPort;              /* 0x006 */
    char http[0x100];           /* 0x008 */
    char https[0x100];          /* 0x108 */
} YnEntryKey;

/* A network device (28 bytes): sub_main's list of the attached ones
 * (gNetDevs, 35 entries; NetDev_ClearList fills it), gNetDevCur the one
 * in use. NetDev_Identify copies type, driver and the names from the known
 * devices' table (gNetDevNames) row whose ids match the IOP's answer. */
typedef struct NetDevice {
    s32 type;                   /* 0x00: -1: unused; 1, 2, 3 */
    s32 driver;                 /* 0x04: the driver kind for type 1 (1, 5, 6: which IRX to unload) */
    s32 vendorId;               /* 0x08: matched against the IOP's answer (Net_GetDeviceState) */
    s32 productId;              /* 0x0C: the same */
    s32 status;                 /* 0x10: from the IOP; then type 1's link event (-1: none), types 2 and 3's PPP link state */
    char *vendor;               /* 0x14 */
    char *product;              /* 0x18 */
} NetDevice;
extern NetDevice gNetDevs[35];


/* A window frame, drawn by sub_main.s Window_DrawTextAlpha (20 bytes). */
typedef struct YnWindow {
    Window win;                 /* 0x00 */
    u32 unk10;                  /* 0x10: 0 in every entry */
} YnWindow;

/* A bar of button hints (Yn_DrawHelpBar, 14 bytes). */
typedef struct YnHelpBar {
    s16 count;
    s16 width;
    s16 items[5];               /* indices into gYnHelpHints */
} YnHelpBar;

/* One line of a screen: a text at (x, y); x == -1 centers it (Yn_CenterXSjis). */
typedef struct YnItem {
    s16 x;
    s16 y;
    char *text;
} YnItem;

/* The settings list shows YN_LIST_ROWS entries from YnWork.listTop. */
#define YN_LIST_ROWS 4

/* YnWork.triedSlots: a bit per memory card slot tried by the connect,
 * and YN_TRIED_FIRST once the first slot has been tried. */
#define YN_TRIED_FIRST 0x80

/* YnCombo.kind: whether the entry's device is attached (Yn_GetDeviceKind). */
#define YN_COMBO_ABSENT 0   /* not attached */
#define YN_COMBO_ETHERNET 1 /* attached, a LAN adaptor (NET_DEV_ETHERNET) */
#define YN_COMBO_OTHER 2    /* attached, another type */

/* YnWork.state while the settings list runs (Yn_Update). */
enum YnState {
    YN_STATE_LOAD_MODULES = 0, /* resets the device list, loads netcnf's modules */
    YN_STATE_OPEN = 1,         /* after 10 frames: opens netcnf, reads the proxy, picks slot 0 */
    YN_STATE_SCAN = 2,         /* Yn_ScanCards, then the list */
    YN_STATE_CONNECT = 3,      /* Yn_Connect, slot by slot (Yn_Init's autoConnect) */
    YN_STATE_LIST = 4,         /* Yn_UpdateEntryList */
    YN_STATE_CHOSEN = 5,       /* an entry was applied: quits, returns changed + 1 */
    YN_STATE_QUIT_ASK = 6,     /* the quit question */
    YN_STATE_ERROR_QUIT = 7,   /* a message that quits on either button (never set) */
    YN_STATE_MESSAGE = 8,      /* a message that goes back to the list (never set) */
    YN_STATE_FINISHED = 9,     /* the connect is over, gNetMode.ynResult is set */
    YN_STATE_WAIT_CARD = 10    /* rescans once the card goes (never set) */
};

/* YnWork.state while the proxy screen runs (Yn_UpdateProxy). */
enum YnProxyState {
    YN_PROXY_INIT = 0,     /* reads the proxy, sets up the use screen */
    YN_PROXY_USE = 1,      /* the use / don't use choice */
    YN_PROXY_FIELDS = 2,   /* the four fields (YnProxyField) */
    YN_PROXY_KEYBOARD = 3, /* a host name on the software keyboard */
    YN_PROXY_NUMBER = 4,   /* a port in the number editor */
    YN_PROXY_CONFIRM = 5,  /* the save question */
    YN_PROXY_SAVED = 6     /* the saved message; then returns */
};

/* YnWork.menuCursor on the proxy fields screen: the field chosen. */
enum YnProxyField {
    YN_FIELD_HTTP = 0,
    YN_FIELD_HTTP_PORT = 1,
    YN_FIELD_HTTPS = 2,
    YN_FIELD_HTTPS_PORT = 3,
    YN_FIELD_COUNT = 4
};

/* YnWork.step on the settings list (Yn_UpdateEntryList). */
enum YnListStep {
    YN_LIST_INIT = 0,         /* clears the cursors, reads the proxy */
    YN_LIST_INPUT = 1,        /* the cursor, the details, the slot switch */
    YN_LIST_MESSAGE = 5,      /* a message over the list, back to backStep */
    YN_LIST_ERROR = 6,        /* a message, then rescans the cards */
    YN_LIST_CARD_MESSAGE = 7, /* a message, then waits for the cards (never set) */
    YN_LIST_WAIT_CARD = 8,    /* waits for the card check, then rescans */
    YN_LIST_CHOOSE = 9        /* checks the cursor's entry and applies it */
};

/* YnWork.step while connecting with the card's settings (Yn_Connect). */
enum YnConnectStep {
    YN_CONNECT_STEP_START = 0,       /* slot 0, nothing tried yet */
    YN_CONNECT_STEP_CHECK_PROXY = 1, /* the proxy must be the saved one */
    YN_CONNECT_STEP_SCAN = 2,        /* scans the slot, looks for the saved settings */
    YN_CONNECT_STEP_NEXT_SLOT = 3,   /* the other slot, then scans again */
    YN_CONNECT_STEP_WAIT_NEXT = 4,   /* waits for PAD_CIRCLE before the next slot (never set) */
    YN_CONNECT_STEP_CONNECTED = 5    /* the connected message, closed by PAD_CIRCLE or a timer */
};

/* What Yn_Connect returns; Yn_Update turns each failure into a
 * NET_YN_ERR_* in gNetMode.error. */
enum YnConnectResult {
    YN_CONNECT_DONE = 1,            /* connected with the saved settings */
    YN_CONNECT_BUSY = 0,
    YN_CONNECT_ALL_TRIED = -1,      /* both slots tried without the saved settings */
    YN_CONNECT_NO_CARD = -2,        /* the slot's card went away during the scan */
    YN_CONNECT_DEVICE = -3,         /* the entry's device isn't attached or won't start */
    YN_CONNECT_PROXY_CHANGED = -4,  /* the proxy isn't the one saved with the settings */
    YN_CONNECT_OTHER_PS2 = -5,      /* the file fails netcnf's check (another console's) */
    YN_CONNECT_CARD_REMOVED = -6,   /* the card went away at the next-slot wait */
    YN_CONNECT_EMPTY_FILE = -7,     /* the settings file has no entries */
    YN_CONNECT_NO_MATCH = -8        /* the saved settings aren't in the file */
};

/* YnWork.scanStep: the card scan's steps (Yn_ScanCards). */
enum YnScanStep {
    YN_SCANSTEP_START = 0,       /* clears the scan's results */
    YN_SCANSTEP_CHECK_SLOTS = 1, /* which slots hold a card */
    YN_SCANSTEP_PICK_SLOT = 2,   /* the next slot with a card, from scanSlot */
    YN_SCANSTEP_NEXT_SLOT = 3,   /* the same after a slot without the directory */
    YN_SCANSTEP_LIST_DIR = 4,    /* looks for the settings directory */
    YN_SCANSTEP_READ_FILE = 5    /* reads the file's three lists, builds the combinations */
};

/* What Yn_ScanCards returns; YnWork.scanResult keeps the failures (0 when
 * the scan succeeded). */
enum YnScanResult {
    YN_SCAN_DONE = 1,            /* the combinations are built */
    YN_SCAN_BUSY = 0,
    YN_SCAN_NO_SETTINGS = -1,    /* no slot has a usable settings file */
    YN_SCAN_CARD_CHANGED = -2,   /* the card in the slot changed */
    YN_SCAN_OTHER_PS2 = -3,      /* the file fails netcnf's check (another console's) */
    YN_SCAN_SLOT_SWITCHED = -10  /* scanResult only: PAD_L1 switched the slot */
};

/* YnWork.msg: the message shown, an index into gYnMsgItems (the draw
 * functions' switches also handle indices this file never sets). */
enum YnMsg {
    YN_MSG_NONE = 0,
    YN_MSG_DEVICE_UNKNOWN = 4,     /* the entry's device isn't in the known devices' table */
    YN_MSG_DEVICE_ABSENT = 5,      /* the device isn't attached or won't start */
    YN_MSG_PROXY_CONFIRM = 0x18,   /* the proxy screen's save question */
    YN_MSG_ENTRY_DETAILS = 0x19,   /* the cursor entry's details (PAD_TRIANGLE) */
    YN_MSG_QUIT_ASK = 0x1A,        /* the quit question */
    YN_MSG_DIAL_UP = 0x1B,         /* a dial-up entry (phone numbers, PPP without PPPoE) */
    YN_MSG_FILE_ERROR = 0x1D,      /* the file fails netcnf's check */
    YN_MSG_PROXY_SAVED = 0x32,
    YN_MSG_NO_NAME = 0x33,         /* the entry has no name */
    YN_MSG_CONNECTED = 0x34
};

/* YnWork.msgCloseMode: which buttons close a message on the list. */
#define YN_MSG_CLOSE_CIRCLE 1 /* PAD_CIRCLE only */
#define YN_MSG_CLOSE_EITHER 2 /* PAD_CIRCLE or PAD_CROSS (so is any value but 1) */

/* YnWork.screen: the screen drawn, an index into gYnScreenItems. */
enum YnScreen {
    YN_SCREEN_NONE = 0,
    YN_SCREEN_LIST = 1,          /* the settings list (Yn_DrawEntryList) */
    YN_SCREEN_PROXY_USE = 0x16,  /* the proxy's use / don't use choice */
    YN_SCREEN_PROXY_FIELDS = 0x17
};

/* YnWork.help: the help line, an index into gYnHelpItems (plus helpVariant). */
enum YnHelp {
    YN_HELP_LIST = 1,          /* 1-3 by helpVariant */
    YN_HELP_PROXY_USE = 0x1F,
    YN_HELP_PROXY_HOST = 0x20, /* the HTTP proxy's host */
    YN_HELP_PROXY_PORT = 0x21, /* either port */
    YN_HELP_PROXY_HTTPS = 0x22 /* the HTTPS proxy's host */
};

/* YnWork.helpBar: the button hints drawn at the bottom, an index into
 * gYnHelpBars (Yn_DrawHelpBar). */
enum YnHelpBarKind {
    YN_HELPBAR_NONE = 0,
    YN_HELPBAR_MESSAGE = 1,     /* a message's buttons */
    YN_HELPBAR_PROXY_ON = 2,    /* the proxy's choice, set to use it */
    YN_HELPBAR_FIELDS = 4,      /* the proxy fields, the host or port missing */
    YN_HELPBAR_PROXY_OFF = 5,   /* the proxy's choice, set not to use it */
    YN_HELPBAR_FIELDS_DONE = 6, /* the proxy fields, ready to save */
    YN_HELPBAR_LIST = 7         /* the settings list */
};

/* Yn_DrawIcon's icons drawn by id: the arrows (by where the code draws
 * them: up and down above and below a port's digit, right and left at
 * the screen's edges where PAD_RIGHT and PAD_LEFT go). */
#define YN_ICON_ARROW_RIGHT 0x10
#define YN_ICON_ARROW_LEFT 0x16
#define YN_ICON_ARROW_DOWN 0x17
#define YN_ICON_ARROW_UP 0x18

/* sub_main's network state gNetMode: include/net_mode.h (yn reads unk13, unk14, trySlot, error). */

/* The overlay's work area (110336 bytes at 0xAD9C80, gYnWork points to it). */
typedef struct YnWork {
    u32 texPal;                 /* 0x000: the texture handle | the palette handle << 16 (released by Yn_Quit's path) */
    u8 unk004[0x008 - 0x004];
    s8 listDetail;              /* 0x008: the list's detail level: 2 from the list's first step (draws the entry's device line, help variant 2, msgCloseMode 4); the 16 bytes from here are cleared together */
    u8 unk009[0x00B - 0x009];
    s8 cursor;                  /* 0x00B: the settings list's cursor (an entry) */
    s8 listTop;                 /* 0x00C: its first entry shown (four shown) */
    s8 proxyCursor;             /* 0x00D: the proxy menu's cursor (0 use, 1 not: proxy.use = 1 - it) */
    s8 menuCursor;              /* 0x00E: the menu's cursor (of four, wraps) */
    u8 unk00F[0x017 - 0x00F];
    s8 quitChoice;              /* 0x017: the quit dialog's choice (left/right toggle; 0: quit) */
    s8 step;                    /* 0x018: the current screen's step */
    s8 backStep;                /* 0x019: the step to go back to (from a message) */
    s8 textMode;                /* 0x01A: Yn_GetTextMode() */
    u8 unk01B[0x01C - 0x01B];
    s8 entryCount;              /* 0x01C: the settings file's entries (YnNetcnf_GetCount) */
    u8 cardSlots;               /* 0x01D: memory card slots with a card (bit per slot; Yn_ScanCards) */
    u8 slot;                    /* 0x01E: the memory card slot in use */
    u8 unk1F;                   /* 0x01F: only cleared (at each card scan) */
    u8 unk020[0x021 - 0x020];
    u8 timer;                   /* 0x021 */
    u8 unk022[0x024 - 0x022];
    u8 scanStep;                /* 0x024: Yn_ScanCards' state (0-5) */
    u8 triedSlots;              /* 0x025: slots tried by the connect scan (bit per slot; YN_TRIED_FIRST after the first) */
    u8 changed;                 /* 0x026: 1: the chosen settings differ from the saved ones (gNetMode.ynResult = changed + 1) */
    u8 scanSlot;                /* 0x027: the slot Yn_ScanCards looks at */
    s32 msgTimer;               /* 0x028: 60 frames for the connect message */
    s32 repeatPad;              /* 0x02C */
    u32 repeat;                 /* 0x030: frames the same pad state has been held (Yn_UpdatePadRepeat) */
    u8 unk034[0x03C - 0x034];
    s8 numDigitPos;             /* 0x03C: the number editor: the digit's position (0 ones .. 4) */
    s8 numEditing;              /* 0x03D: 2 while a number is being edited */
    u16 numBackup;              /* 0x03E: the value before the edit (put back on cancel) */
    u16 *numTarget;             /* 0x040: the number being edited (Yn_BeginNumberEdit) */
    u16 numStep;                /* 0x044: the digit being changed (1, 10, .. 10000) */
    s8 unk46;                   /* 0x046: only written: 1 on the list, 16 on the proxy screen */
    s8 msgCloseMode;            /* 0x047: YN_MSG_CLOSE_CIRCLE: only PAD_CIRCLE closes the list's message; 2 or 4 (listDetail 2): either button */
    s8 unk48;                   /* 0x048: only written: 1 on the list, 19 on the proxy screen */
    s8 screen;                  /* 0x049: index into gYnScreenItems / gYnScreenItemCounts */
    u8 msg;                     /* 0x04A: index into gYnMsgItems / gYnMsgItemCounts */
    s8 help;                    /* 0x04B: index into gYnHelpItems / gYnHelpItemCounts */
    s8 helpVariant;             /* 0x04C: added to help for the help line (2 at most) */
    u8 unk04D[0x050 - 0x04D];
    char *editTarget;           /* 0x050: the string being edited with the keyboard (Yn_OpenKeyboard) */
    char editBuf[0x200];        /* 0x054 */
    YnCombo combos[30];         /* 0x254 */
    u8 unk4AC[0xECC - 0x4AC];
    YnProxy proxy;              /* 0xECC: copied from and to the NET file data (gNetSaveData.proxy and configProxy) */
    YnCardCheck card;           /* 0x10D4 */
    u8 unk10DA[0x1180 - 0x10DA];
    YnNetcnf netcnf;            /* 0x1180 */
    s32 state;                  /* 0x1AEC0: the main state machine (Yn_Update) */
    s32 initArg;                /* 0x1AEC4: Yn_Init's argument (nonzero: no wait for the circle) */
    s32 result;                 /* 0x1AEC8 */
    s32 nameScroll;             /* 0x1AECC: 1: the cursor entry's name is too long and scrolls */
    s32 nameOfs;                /* 0x1AED0: the scroll's offset in the name */
    s32 nameTimer;              /* 0x1AED4: frames to the next scroll step (60, 90, 40) */
    s32 nameAtEnd;              /* 0x1AED8: 1: the scroll reached the end (starts over) */
    s32 quitAsked;              /* 0x1AEDC: 1 while the quit dialog shows */
    s32 helpBar;                /* 0x1AEE0: the help bar's kind (Yn_DrawHelpBar) */
    s8 scanResult;              /* 0x1AEE4: the card scan's result (0, -1, -2 card changed, -3) */
    s8 prevSlot;                /* 0x1AEE5: the card slot before the scan */
    u8 okSlots;                 /* 0x1AEE6: slots with a readable card (bit per slot) */
    s8 listBusy;                /* 0x1AEE7: nonzero: the list takes no input (only cleared in the C) */
    s8 cardGone;                /* 0x1AEE8: the current card went away (Yn_IsCurrentCardGone) */
    u8 unk1AEE9[0x1AEEC - 0x1AEE9];
    s32 cardError[2];           /* 0x1AEEC: per memory card slot: the last load error (Yn_ScanCardsForConnect) */
    u8 unk1AEF4[0x1AF00 - 0x1AEF4];
} YnWork;
typedef char YnWork_size_check[(sizeof(YnWork) == 0x1AF00) ? 1 : -1];

void YnCard_Reset(YnCardCheck *check);
s32 YnCard_CheckSlots(YnCardCheck *check);
s32 YnCard_CheckCurrentSlot(YnCardCheck *check);
void YnCard_SetSlot(YnCardCheck *check, u8 slot);
s32 YnCard_GetSlot(YnCardCheck *check);
s32 YnCard_ListNetcnfDir(YnCardCheck *check);
s32 YnCard_MapError(s32 err);
void Yn_ResetStreams(void);
extern YnWork *gYnWork;

s32 Yn_Init(s32 arg);
void Yn_Draw(void);
void Yn_Quit(void);
s32 Yn_ScanCards(void);
s32 Yn_ScanCardsForConnect(void);
s32 Yn_Connect(void);
s32 Yn_UpdateEntryList(void);
s32 Yn_IsCardGone(void);
s32 Yn_IsCurrentCardGone(void);
void Yn_PreDraw(void);
void Yn_DrawScreen(void);
void Yn_DrawItems(void);
void Yn_DrawChoiceItems(s32 sel, s32 color);
void Yn_DrawEntryList(void);
void Yn_DrawPort(u16 *value, s16 x, s16 y);
void Yn_DrawTextField(u8 *text, char *label, s16 x, s16 y, s32 w);
void Yn_DrawMessage(void);
void Yn_DrawMessageText(void);
void Yn_DrawSlotMessage(s32 sel);
void Yn_DrawEntryDetails(void);
void Yn_AlignAddress(u8 *dst, u8 *src);
void Yn_DrawHelp(void);
void Yn_Print(s16 x, s16 y, char *text);
void Yn_SetColor(s8 color);
void Yn_SetFontSize(s32 w, s32 height);
void Yn_SetGlyphSize(f32 scale);
void Yn_SetTextMode(s8 mode);
s32 Yn_GetTextMode(void);
s16 Yn_CenterX(char *text, s32 w);
s16 Yn_CenterXSjis(char *str, s32 w);
void Yn_CopyForPrint(char *dst, u8 *src, s32 max);
void Yn_StrCopy(char *dst, char *src);
void Yn_UpdatePadRepeat(PadState *pad);
void Yn_PreUpdate(void);
void Yn_DrawTextLayer(void);
void Yn_ExportSettings(NetcnfData *data);
void Yn_ApplyEntry(NetcnfData *data);
void Yn_LoadProxy(void);
void Yn_DrawBackground(void);
void Yn_DrawChoiceCursor(void);
void Yn_DrawIcon(f32 scale, s32 id, s32 x, s32 y, s32 blink);
void Yn_DrawProxy(void);
void Yn_DrawHelpBar(s32 type);
s32 YnNetcnf_LoadModules(YnNetcnf *n);
s32 YnNetcnf_Open(YnNetcnf *n);
s32 YnNetcnf_Close(YnNetcnf *n);
char *YnNetcnf_FindUsrName(YnNetcnf *n, char *name);
void Yn_ConfigToIfc(YnConfig *cfg, NetcnfData *data, char *name);
void Yn_IfcToConfig(YnConfig *cfg, NetcnfData *data, char *name);
void Yn_ConfigToDev(YnConfig *cfg, NetcnfData *data);
void Yn_DevToConfig(YnConfig *cfg, NetcnfData *data);
void Yn_BuildCombos(YnNetcnf *n, YnCombo *combo);
s32 Yn_FindCombination(YnConfig *cfg, YnCombo *combo, s32 n);
s32 Yn_CompareProxy(YnEntryKey *edited, YnEntryKey *inUse);
s32 Yn_IsAddressSet(u8 *addr);
void YnNetcnf_SetType(YnNetcnf *n, s32 type);
s32 YnNetcnf_GetCount(YnNetcnf *n);
s32 YnNetcnf_GetList(YnNetcnf *n);
void YnNetcnf_LoadCombos(YnNetcnf *n);
s32 YnNetcnf_IsEmpty(YnNetcnf *n);
s32 YnNetcnf_CheckFile(YnNetcnf *n);
s32 YnNetcnf_LoadFirst(YnNetcnf *n);
void YnNetcnf_SetPath(YnNetcnf *n, s32 slot);
void Yn_ParseAddress(u8 *str, u8 *addr);
void Yn_FormatAddress(char *str, u8 *addr);
void Yn_ResetNetDevices(void);
s32 Yn_FindDevice(char *vendor, char *product);
s32 Yn_GetDeviceKind(char *vendor, char *product);
s32 Yn_SelectDevice(s32 i);
void Yn_Utf8ToSjis(char *dstp, char *srcp);
void Yn_SjisToUtf8(char *dstp, char *srcp);
void Yn_UnloadModule(s32 id);
void Yn_LoadModule(char *path, s32 argc, char *argv, void *buf, s32 tries);

#endif /* YN_H */
