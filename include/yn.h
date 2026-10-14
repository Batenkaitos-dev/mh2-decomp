#ifndef YN_H
#define YN_H
#include "net_mode.h"
#include "window.h"
#include "prim.h"

/*
 * The yn overlay (network settings): types shared by
 * src/yn/yn_card.c and src/yn/yn_main.c.
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
extern s32 D_6AF134[2];     /* gCardTask.type */
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
 * fills it (sceNetcnfifData_t, 0x1340 bytes). */
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

/* One entry of a configuration file's list (0x240 bytes). */
typedef struct NetcnfEntry {
    s32 type;
    s32 stat;
    char sysName[0x100];        /* 0x008 */
    char usrName[0x100];        /* 0x108 */
    u8 unk208[0x38];
} NetcnfEntry;

/* The network configuration files read from a memory card (0x19D40 bytes, in
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

/* The settings being edited, in the game's own form (0xA20 bytes). */
typedef struct YnConfig {
    s32 ifcType;                /* 0x000 */
    u8 unk4;
    u8 noPppoe;                 /* 0x005: 0: PPPoE (authName, authKey) */
    u8 fixedIp;                 /* 0x006: 0: DHCP (dhcpHostname), else address/netmask/gateway */
    u8 dnsSet;                  /* 0x007: dns1/dns2 given */
    u8 phyConfig;               /* 0x008: 0 auto, 1-4 the ethernet modes */
    u8 ifcType3;                /* 0x009: 1: interface type 3 (Yn_ConfigToIfc) */
    u8 unkA[2];
    char authName[0x200];       /* 0x00C */
    char authKey[0x200];        /* 0x20C */
    char dhcpHostname[0x200];   /* 0x40C */
    u8 address[4];              /* 0x60C */
    u8 netmask[4];              /* 0x610 */
    u8 gateway[4];              /* 0x614 */
    u8 dns1[4];                 /* 0x618 */
    u8 dns2[4];                 /* 0x61C */
    char name[0x200];           /* 0x620: the entry's name (Shift-JIS here, EUC-JP in the file) */
    char vendor[0x100];         /* 0x820 */
    char product[0x100];        /* 0x920 */
} YnConfig;

/* A combination of the type 0 list with its device (0x14 bytes, 30 of them
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

/* A network device (0x1C bytes): sub_main's list of the attached ones
 * (gNetDevs, 35 at 0x59DE20; NetDev_ClearList fills it), gNetDevCur the one
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

/* The proxy settings (0x208 bytes). */
typedef struct YnProxy {
    s32 use;                    /* 0x000 */
    u16 httpPort;               /* 0x004 */
    u16 httpsPort;              /* 0x006 */
    char http[0x100];           /* 0x008 */
    char https[0x100];          /* 0x108 */
} YnProxy;

/* A window frame, drawn by sub_main.s Window_DrawTextAlpha (0x14 bytes). */
typedef struct YnWindow {
    Window win;                 /* 0x00 */
    u32 unk10;                  /* 0x10: 0 in every entry */
} YnWindow;

/* A bar of button hints (Yn_DrawHelpBar, 0xE bytes). */
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

/* sub_main's network state gNetMode: include/net_mode.h (yn reads unk13, unk14, trySlot, error). */

/* The overlay's work area (0x1AF00 bytes at 0xAD9C80, gYnWork points to it). */
typedef struct YnWork {
    u32 texPal;                 /* 0x000: the texture handle | the palette handle << 16 (released by Yn_Quit's path) */
    u8 unk004[0x008 - 0x004];
    s8 unk8;                    /* 0x008 */
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
    u8 unk1F;                   /* 0x01F */
    u8 unk020[0x021 - 0x020];
    u8 timer;                   /* 0x021 */
    u8 unk022[0x024 - 0x022];
    u8 scanStep;                /* 0x024: Yn_ScanCards' state (0-5) */
    u8 triedSlots;              /* 0x025: slots tried by the connect scan (bit per slot; 0x80 after the first) */
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
    s8 unk46;                   /* 0x046 */
    s8 unk47;                   /* 0x047 */
    s8 unk48;                   /* 0x048 */
    s8 screen;                  /* 0x049: index into gYnScreenItems / gYnScreenItemCounts */
    u8 msg;                     /* 0x04A: index into gYnMsgItems / gYnMsgItemCounts */
    s8 help;                    /* 0x04B: index into gYnHelpItems / gYnHelpItemCounts */
    s8 helpVariant;             /* 0x04C: added to help for the help line (2 at most) */
    u8 unk04D[0x050 - 0x04D];
    char *editTarget;           /* 0x050: the string being edited with the keyboard (Yn_OpenKeyboard) */
    char editBuf[0x200];        /* 0x054 */
    YnCombo combos[30];         /* 0x254 */
    u8 unk4AC[0xECC - 0x4AC];
    YnProxy proxy;              /* 0xECC: copied from and to sub_main (D_5BE6C4, D_5BF400) */
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

extern YnWork *gYnWork;

#endif /* YN_H */
