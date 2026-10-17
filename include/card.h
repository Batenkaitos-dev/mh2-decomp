#ifndef CARD_H
#define CARD_H

#include "common.h"

/* The memory card task and its operations (unk_3DDBA0.c). */

/* NetMenu.mode: which state machine NetMenu_Update runs (NetMenu_Open's argument). The modes
 * 11-13, 15 and 16 end at once (done 1). */
enum NetMenuMode {
    NETMENU_MODE_BOOT = 0,              /* CardMenu_RunBoot: the boot load (select_boot.c) */
    NETMENU_MODE_SAVE_SYSTEM = 1,       /* CardMenu_RunSaveSystem: the settings (select_option.c, select_gallery.c) */
    NETMENU_MODE_SETUP = 2,             /* CardMenu_RunSetup: rewrites the system file of slot 0 (no caller seen) */
    NETMENU_MODE_SAVE_NEW = 3,          /* CardMenu_RunSaveNew: a new character (select_chara.c) */
    NETMENU_MODE_LOAD = 4,              /* CardMenu_RunLoad (select_chara.c) */
    NETMENU_MODE_SAVE = 5,              /* CardMenu_RunSave: offline in the town (lobby unk_70C350.c) */
    NETMENU_MODE_GAME_SAVE_TOWN = 6,    /* CardMenu_RunGameSaveNet: online in the town (unk_70C350.c) */
    NETMENU_MODE_GAME_SAVE_ENTRY = 7,   /* CardMenu_RunGameSaveNet (lobby unk_7C1540.c) */
    NETMENU_MODE_GAME_SAVE_LOGOUT = 8,  /* CardMenu_RunGameSaveNet (lobby unk_7D44B0.c) */
    NETMENU_MODE_SAVE_ASK = 9,          /* CardMenu_RunSave, asking first (lobby unk_7B91F0.c) */
    NETMENU_MODE_GAME_SAVE = 10,        /* CardMenu_RunGameSave: the save state switches to it when
                                         * a card is in use (NetMenu.hasSave) */
    NETMENU_MODE_SAVE_GALLERY = 14,     /* CardMenu_RunSaveSystem, asking with NETMENU_MSG_ASK_SAVE_GALLERY (no caller seen) */
    NETMENU_MODE_SAVE_B = 0x10,         /* runs as NETMENU_MODE_SAVE with NetMenu.openMode set; then
                                         * CardGame_AskRetry leads to CardGame_AskQuit (no caller seen) */
    NETMENU_MODE_GAME_SAVE_EXTRA = 0x11, /* CardMenu_RunGameSaveExtra (lobby unk_7D44B0.c) */
    NETMENU_MODE_DELETE_LOGIN_ID = 0x12 /* CardMenu_RunDeleteLoginId (select_option.c) */
};

/* The card menu's messages (NetMenu.msg; NetMenu_DrawMessage's id): each indexes the line lists
 * of gNetMenuMsgLines, and the ids from 0x40 on read on into gNetMenuMsgLines40. */
enum NetMenuMsg {
    NETMENU_MSG_CHECKING_BOTH            = 0x01, /* the notice that both card slots are being checked (boot) */
    NETMENU_MSG_AUTOLOADING              = 0x02, /* the notice that the save is loading (boot) */
    NETMENU_MSG_ASK_START_BAD_DATA       = 0x03, /* the prompt to start although the data is bad */
    NETMENU_MSG_AUTOLOADED               = 0x04, /* the notice that the boot load finished */
    NETMENU_MSG_ASK_START_NO_CARD        = 0x05, /* the prompt to start with no card */
    NETMENU_MSG_ASK_START_NO_SPACE       = 0x06, /* the prompt to start with a card too full to save */
    NETMENU_MSG_SLOT_LABELS              = 0x07, /* the two slots' labels (NetMenu_ChooseSlot) */
    NETMENU_MSG_CHOOSE_SAVE_SLOT         = 0x08, /* the prompt to choose the slot to save to */
    NETMENU_MSG_ASK_SAVE_SETTINGS        = 0x09, /* the prompt to save the settings */
    NETMENU_MSG_CHECKING                 = 0x0A, /* the notice that the card is being checked */
    NETMENU_MSG_NO_CARD                  = 0x0B, /* the error that there is no card */
    NETMENU_MSG_NO_SPACE                 = 0x0C, /* the error that the card has too little space */
    NETMENU_MSG_ASK_FORMAT               = 0x0D, /* the prompt to format the card */
    NETMENU_MSG_FORMATTING               = 0x0E, /* the notice that the card is being formatted */
    NETMENU_MSG_FORMAT_FAILED            = 0x0F, /* the error that the format failed */
    NETMENU_MSG_ASK_OVERWRITE            = 0x10, /* the prompt to overwrite the save */
    NETMENU_MSG_SAVING                   = 0x11, /* the notice that the save is being written */
    NETMENU_MSG_SAVED                    = 0x12, /* the notice that the save was written */
    NETMENU_MSG_SAVE_FAILED              = 0x13, /* the error that the save failed */
    NETMENU_MSG_BAD_DATA                 = 0x14, /* the error that the data is bad and was not loaded */
    NETMENU_MSG_CHOOSE_LOAD_SLOT         = 0x15, /* the prompt to choose the slot to load from */
    NETMENU_MSG_ASK_OVERWRITE_CHARA      = 0x16, /* the prompt to overwrite a used character slot */
    NETMENU_MSG_ASK_SAVE_HERE            = 0x17, /* the prompt to save to an empty character slot */
    NETMENU_MSG_SAVED_KEEP_CARD          = 0x18, /* the saved notice, with the card to be kept in */
    NETMENU_MSG_NO_GAME_DATA             = 0x19, /* the error that the card holds no save */
    NETMENU_MSG_NO_CHARA                 = 0x1A, /* the error that the card holds no character */
    NETMENU_MSG_SAVE_BAD_DATA            = 0x1B, /* the error that the card's data is bad, not saved */
    NETMENU_MSG_LOADING                  = 0x1C, /* the notice that the save is being read */
    NETMENU_MSG_ASK_SAVE                 = 0x1D, /* the prompt to save */
    NETMENU_MSG_SAVING_TO_SLOT           = 0x1F, /* the notice that the in-game save is being written */
    NETMENU_MSG_NO_CARD_LOAD             = 0x23, /* the error that there is no card (loading) */
    NETMENU_MSG_GAME_SAVE_CHECKING       = 0x24, /* the notice that the in-game save checks the card */
    NETMENU_MSG_GAME_CARD_MISSING        = 0x25, /* the error that the card the game started with is missing */
    NETMENU_MSG_CARD_CHANGED             = 0x26, /* the error that the card was changed or removed */
    NETMENU_MSG_ASK_QUIT_UNSAVED         = 0x27, /* the prompt to quit without saving */
    NETMENU_MSG_CHOOSE_CHARA_SLOT        = 0x2A, /* the prompt to choose a character slot */
    NETMENU_MSG_ASK_CREATE_DATA          = 0x2B, /* the prompt to create the save on this card */
    NETMENU_MSG_ASK_SAVE_NEW_CHARA       = 0x2C, /* the prompt to save the new character */
    NETMENU_MSG_ASK_CONTINUE_UNSAVED     = 0x2D, /* the prompt to go on without saving */
    NETMENU_MSG_ASK_SAVE_PROGRESS        = 0x2E, /* the prompt to save the game so far */
    NETMENU_MSG_ASK_START_NO_SPACE_BOTH  = 0x2F, /* the prompt to start with both cards too full */
    NETMENU_MSG_ASK_SAVE_GALLERY         = 0x3D, /* the prompt to save the gallery's state */
    NETMENU_MSG_OLD_DATA_FOUND           = 0x3F, /* the notice that an earlier game's save was found (%p, %t) */
    NETMENU_MSG_OLD_GAME_TITLES          = 0x40, /* the earlier games' titles, by gOldGameKind - 1 (%t) */
    NETMENU_MSG_SLOT_STATE_LABELS        = 0x41, /* the empty slot's and the bad slot's labels */
    NETMENU_MSG_ASK_START_LOAD_FAILED    = 0x42, /* the prompt to start after a failed load */
    NETMENU_MSG_ASK_QUIT_TO_MENU         = 0x43, /* the prompt to quit to the menu without saving */
    NETMENU_MSG_CHOOSE_NET_SLOT          = 0x44, /* the prompt to choose the slot with the NET file */
    NETMENU_MSG_NO_NET_FILE              = 0x45, /* the error that the card has no NET file */
    NETMENU_MSG_NET_LOAD_FAILED          = 0x46, /* the error that the NET file did not load */
    NETMENU_MSG_NET_NO_CARD              = 0x47, /* the error that there is no card (NET file) */
    NETMENU_MSG_ASK_DELETE_LOGIN_ID      = 0x48, /* the prompt to delete the login id, shown with it (%i, %q) */
    NETMENU_MSG_NET_SAVE_FAILED          = 0x49, /* the error that the NET file was not saved */
    NETMENU_MSG_NET_CARD_REMOVED         = 0x4A, /* the error that the card was removed (NET file) */
    NETMENU_MSG_NO_LOGIN_ID              = 0x4B  /* the notice that no login id is saved */
};

/* NetMenu_YesNo's answers (-1 while none is given). */
#define NETMENU_YES             0       /* circle with the cursor on yes */
#define NETMENU_NO              1       /* circle on no, or cross */
/* NetMenu_ChooseSlot's answer besides the port picked (0 or 1); -1 while none is given. */
#define NETMENU_SLOT_CANCEL     2       /* cross */

/* The card task's results (CardTask_Result, NetMenu.result[slot]). */
#define CARD_OK                 0       /* done */
#define CARD_BUSY               (-1)    /* still running */
#define CARD_NOT_TRIED          (-100)  /* NetMenu_Open's start value */
#define CARD_ERR_BAD_DATA       (-251)  /* the file's version or sum is wrong (set by the menu: SaveMenu_ReadBad) */
#define CARD_ERR_NO_SPACE       (-252)  /* no save file, and too little space for one (CardTask_SpaceStep) */
#define CARD_ERR_NO_FILE        (-253)  /* no save file, but space for one */
#define CARD_ERR_UNFORMATTED    (-254)  /* Card_GetInfo 3 */
#define CARD_ERR_NO_CARD        (-255)  /* no PS2 memory card (Card_GetInfo 0) */
#define CARD_ERR_FAILED         (-256)  /* a card command failed */

/* The card menus' states (NetMenu.state, net_menu.h; unk_3D54B0.c), one enum per state
 * machine. NetMenu_Update runs the machine of NetMenu.mode (a CardMenu_Run* function), and each
 * state runs the function of its name (CARD_SYS_ASK_SAVE: CardSys_AskSave). A DONE state ends
 * the menu with NetMenu.done 1, a QUIT state with 2 (left without saving). */
enum CardBootState {
    CARD_BOOT_START = 0,
    CARD_BOOT_FIND_MH = 1,
    CARD_BOOT_FIND_MHG = 2,
    CARD_BOOT_FIND_MHG2 = 3,            /* CardBoot_FindMHG too; no state sets it */
    CARD_BOOT_AUTO_LOAD = 4,
    CARD_BOOT_LOADED = 5,
    CARD_BOOT_SHOW_LOADED = 6,
    CARD_BOOT_SHOW_OLD_DATA = 7,
    CARD_BOOT_ASK_START = 8,
    CARD_BOOT_READ_PATCH = 9,
    CARD_BOOT_CHECK_PATCH = 10,
    CARD_BOOT_LOAD_PATCH = 11,
    CARD_BOOT_DONE = 12
};

enum CardSysState {
    CARD_SYS_START = 0,
    CARD_SYS_ASK_SAVE = 1,
    CARD_SYS_CHOOSE_SLOT = 2,
    CARD_SYS_READ_CHARACTERS = 3,
    CARD_SYS_CHECK_SYSTEM = 4,
    CARD_SYS_ASK_FORMAT = 5,
    CARD_SYS_FORMAT = 6,
    CARD_SYS_CHECK_SYSTEM_READ_PATCH = 7,
    CARD_SYS_CHECK_PATCH = 8,
    CARD_SYS_LOAD_PATCH = 9,
    CARD_SYS_ASK_OVERWRITE = 10,
    CARD_SYS_PREPARE_SAVE = 11,
    CARD_SYS_SAVE = 12,
    CARD_SYS_SHOW_SAVED = 13,
    CARD_SYS_SHOW_ERROR = 14,
    CARD_SYS_DONE = 15
};

enum CardNewState {
    CARD_NEW_START = 0,
    CARD_NEW_CHOOSE_SLOT = 1,
    CARD_NEW_ASK_SAVE = 2,
    CARD_NEW_CHECK_PATCH = 3,
    CARD_NEW_LOAD_PATCH = 4,
    CARD_NEW_CHOOSE_CHAR_SLOT = 5,
    CARD_NEW_CONFIRM_CHAR_SLOT = 6,
    CARD_NEW_ASK_FORMAT = 7,
    CARD_NEW_FORMAT = 8,
    CARD_NEW_PREPARE_SAVE = 9,
    CARD_NEW_SAVE_CHARACTER = 10,
    CARD_NEW_SAVE_SYSTEM = 11,
    CARD_NEW_SHOW_SAVED = 12,
    CARD_NEW_SHOW_ERROR = 13,
    CARD_NEW_DONE = 14,
    CARD_NEW_QUIT = 15
};

enum CardLoadState {
    CARD_LOAD_START = 0,
    CARD_LOAD_CHOOSE_SLOT = 1,
    CARD_LOAD_CHECK_SYSTEM = 2,
    CARD_LOAD_DONE_B = 3,               /* done too; no state sets it */
    CARD_LOAD_CHECK_PATCH = 4,
    CARD_LOAD_LOAD_PATCH = 5,
    CARD_LOAD_LOAD_CHARACTERS = 6,
    CARD_LOAD_SHOW_ERROR = 7,
    CARD_LOAD_WAIT = 8,
    CARD_LOAD_DONE = 9,
    CARD_LOAD_QUIT = 10
};

/* CardMenu_RunGameSave, CardMenu_RunGameSaveNet and CardMenu_RunGameSaveExtra share these. */
enum CardGameState {
    CARD_GAME_START = 0,
    CARD_GAME_CHECK_SYSTEM = 1,
    CARD_GAME_CHECK_CHARACTER = 2,
    CARD_GAME_PREPARE_SAVE = 3,
    CARD_GAME_SAVE_CHARACTER = 4,       /* CardGame_SaveCharacterExtra in CardMenu_RunGameSaveExtra */
    CARD_GAME_SAVE_SYSTEM = 5,
    CARD_GAME_SHOW_RESULT = 6,
    CARD_GAME_SHOW_ERROR = 7,
    CARD_GAME_ASK_RETRY = 8,
    CARD_GAME_ASK_QUIT = 9,
    CARD_GAME_DONE = 10,                /* the net mode: quitResult 1, then CARD_GAME_SAVE_EXTRA */
    CARD_GAME_QUIT = 11,                /* the net mode: quitResult 2, then CARD_GAME_SAVE_EXTRA */
    CARD_GAME_SAVE_EXTRA = 12           /* CardGame_SaveExtra; the net mode: LobbyCard_SaveNetFile, then done = quitResult */
};

enum CardSaveState {
    CARD_SAVE_START = 0,
    CARD_SAVE_CHOOSE_SLOT = 1,
    CARD_SAVE_ASK_SAVE = 2,
    CARD_SAVE_CHECK_SYSTEM = 3,
    CARD_SAVE_READ_CHARACTERS = 4,
    CARD_SAVE_CHOOSE_CHAR_SLOT = 5,
    CARD_SAVE_CONFIRM_CHAR_SLOT = 6,
    CARD_SAVE_ASK_FORMAT = 7,
    CARD_SAVE_FORMAT = 8,
    CARD_SAVE_PREPARE_SAVE = 9,
    CARD_SAVE_SAVE_CHARACTER = 10,
    CARD_SAVE_SAVE_SYSTEM = 11,
    CARD_SAVE_SHOW_SAVED = 12,
    CARD_SAVE_SHOW_ERROR = 13,
    CARD_SAVE_DONE = 14,
    CARD_SAVE_QUIT = 15
};

enum CardSetupState {
    CARD_SETUP_START = 0,
    CARD_SETUP_READ = 1,
    CARD_SETUP_SAVE = 2,
    CARD_SETUP_SHOW_RESULT = 3,
    CARD_SETUP_DONE = 4
};

enum CardLoginState {
    CARD_LOGIN_START = 0,
    CARD_LOGIN_CHOOSE_SLOT = 1,
    CARD_LOGIN_READ_NET_FILE = 2,
    CARD_LOGIN_ASK_DELETE = 3,
    CARD_LOGIN_SAVE = 4,
    CARD_LOGIN_SHOW_ERROR = 5,
    CARD_LOGIN_SHOW_RESULT = 6,
    CARD_LOGIN_DONE = 7,
    CARD_LOGIN_QUIT = 8
};


/* The card task (gCardTask, 0xF4 bytes). */
typedef struct CardTask {
    s32 step;             /* 0x00: the task step (the CardTask_*Step functions switch on it) */
    s32 subStep;          /* 0x04: the step within a card call (Card_ReadFile, Card_WriteFile, Card_GetInfo ...) */
    s32 fileNo;           /* 0x08: the file of the save operation being written (op->files[it]; CardTask_SaveStep) */
    s32 entryNo;          /* 0x0C: the directory entries found (Card_GetDir, from syncResult); the entry index (Card_DeleteDir) */
    s32 portCount;        /* 0x10: the ports to scan, 2 (Card_Init; CardTask_NextSlot) */
    s32 port;             /* 0x14: the memory card port (sceMcOpen, sceMcGetDir ...; cardState[it], the changed bit) */
    s32 syncCmd;          /* 0x18: the finished command (sceMcSync; Card_Sync) */
    s32 syncResult;       /* 0x1C: the result of the finished command (sceMcSync; Card_Sync): < 0 an error, an fd after sceMcOpen */
    s32 retries;        /* 0x20 */
    s32 result;         /* 0x24: -1 while busy */
    s32 type;           /* 0x28: card type, 2: PS2 card */
    s32 free;           /* 0x2C: free clusters */
    s32 format;         /* 0x30: nonzero when formatted */
    s32 cardState[3];   /* 0x34: per port (Card_PortState) */
    s32 cardFree[3];    /* 0x40: per port */
    s32 changed;        /* 0x4C: ports whose card changed, one bit each */
    char dir[0x22];     /* 0x50: directory name */
    char name[0x42];    /* 0x72: file name */
    s32 fd;             /* 0xB4: open file */
    s32 unkB8;          /* 0xB8: flag of the current file */
    s32 size;           /* 0xBC */
    s32 unkC0;          /* 0xC0: delete a directory with its files */
    s32 unkC4;          /* 0xC4 */
    s32 unkC8;          /* 0xC8 */
    s32 state;          /* 0xCC: index in gCardSteps */
    u8 *buf;            /* 0xD0 */
    s32 op;             /* 0xD4: index in gCardOps */
    u8 padD8[0xE0 - 0xD8];
    s32 unkE0;          /* 0xE0 */
    s32 mask;           /* 0xE4: files still to write, one bit each */
    u8 padE8[0xF4 - 0xE8];
} CardTask;

/* A file of a card operation (16 bytes). */
typedef struct McFileDesc {
    s32 flag;           /* 0x0: nonzero when used */
    char *name;         /* 0x4 */
    u8 *ptr;            /* 0x8 */
    s32 size;           /* 0xC */
} McFileDesc;

/* A card operation (gCardOps): the save's titles and its files
 * (files[0] is icon.sys, 1-3 the icons, 4-8 the data files). */
typedef struct CardOp {
    char *title1;       /* 0x00 */
    char *title2;       /* 0x04 */
    McFileDesc files[9]; /* 0x08 */
    u8 pad98[0x9C - 0x98];
    s32 unk9C;          /* 0x9C: to CardTask.unkC8 */
    s32 blocks;         /* 0xA0: card blocks needed */
} CardOp;

/* A memory card date and time (sceMcStDateTime; D_006AF0C8). */
typedef struct McDateTime {
    u8 wday;            /* 0x0: set by McDate_SetWeekday */
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;             /* 0x4 */
    u8 month;           /* 0x5 */
    u16 year;           /* 0x6 */
} McDateTime;

extern CardOp *gCardOps[];
extern void (*gCardSteps[])(CardTask *t);
extern CardTask gCardTask;
extern McDateTime D_006AF0C8;

#endif /* CARD_H */
