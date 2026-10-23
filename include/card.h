#ifndef CARD_H
#define CARD_H

#include "common.h"
#include "net_menu.h"

/* The memory card: the card menus' modes, messages and states (gCardMenu, NetMenu), the card
 * task and its operations (gCardTask, gCardOps), the save files and the patch. */

/* NetMenu.mode: which state machine NetMenu_Update runs (NetMenu_Open's argument). The modes
 * 11-13, 15 and 16 end at once (done 1). */
enum NetMenuMode {
    NETMENU_MODE_BOOT = 0,              /* CardMenu_RunBoot: the boot load (select_boot.c) */
    NETMENU_MODE_SAVE_SYSTEM = 1,       /* CardMenu_RunSaveSystem: the settings (select_option.c, select_gallery.c) */
    NETMENU_MODE_SETUP = 2,             /* CardMenu_RunSetup: rewrites the system file of slot 0 (no caller seen) */
    NETMENU_MODE_SAVE_NEW = 3,          /* CardMenu_RunSaveNew: a new character (select_chara.c) */
    NETMENU_MODE_LOAD = 4,              /* CardMenu_RunLoad (select_chara.c) */
    NETMENU_MODE_SAVE = 5,              /* CardMenu_RunSave: offline in the town (the lobby's TownNetMenu_Update) */
    NETMENU_MODE_GAME_SAVE_TOWN = 6,    /* CardMenu_RunGameSaveNet: online in the town (TownNetMenu_Update) */
    NETMENU_MODE_GAME_SAVE_ENTRY = 7,   /* CardMenu_RunGameSaveNet (the lobby's LobbyEntry_Fetch) */
    NETMENU_MODE_GAME_SAVE_LOGOUT = 8,  /* CardMenu_RunGameSaveNet (the lobby's LobbyLogin_SaveAndExit) */
    NETMENU_MODE_SAVE_ASK = 9,          /* CardMenu_RunSave, asking first (the lobby's LobbyScreen_LoadBanks) */
    NETMENU_MODE_GAME_SAVE = 10,        /* CardMenu_RunGameSave: the save state switches to it when
                                         * a card is in use (NetMenu.hasSave) */
    NETMENU_MODE_SAVE_GALLERY = 14,     /* CardMenu_RunSaveSystem, asking with NETMENU_MSG_ASK_SAVE_GALLERY (no caller seen) */
    NETMENU_MODE_SAVE_B = 0x10,         /* runs as NETMENU_MODE_SAVE with NetMenu.openMode set; then
                                         * CardGame_AskRetry leads to CardGame_AskQuit (no caller seen) */
    NETMENU_MODE_GAME_SAVE_EXTRA = 0x11, /* CardMenu_RunGameSaveExtra (the lobby's LobbyPatch_Install) */
    NETMENU_MODE_DELETE_LOGIN_ID = 0x12 /* CardMenu_RunDeleteLoginId (select_option.c) */
};

/* The card menu's messages (NetMenu.msg; NetMenu_DrawMessage's id): each indexes the line lists
 * of gNetMenuMsgLines, and the ids from NETMENU_MSG_LINES40 on read on into gNetMenuMsgLines40. */
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

#define NETMENU_MSG_LINES40     0x40    /* the first message whose lines are in gNetMenuMsgLines40 */

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
#define CARD_ERR_NO_SPACE       (-252)  /* no save file, and too little space for one (CardTask_FindStep) */
#define CARD_ERR_NO_FILE        (-253)  /* no save file, but space for one */
#define CARD_ERR_UNFORMATTED    (-254)  /* Card_GetInfo 3 */
#define CARD_ERR_NO_CARD        (-255)  /* no PS2 memory card (Card_GetInfo 0) */
#define CARD_ERR_FAILED         (-256)  /* a card command failed */

#define CARD_PORTS      2       /* the memory card ports the task polls (CardTask.portCount) */
#define MC_TYPE_PS2     2       /* CardTask.type of a PS2 memory card (sceMcGetInfo) */

/* sceMcOpen's modes. */
#define MC_OPEN_READ    0x0001
#define MC_OPEN_WRITE   0x0002
#define MC_OPEN_CREATE  0x0200

/* The card in a port (Card_GetInfo's result, CardTask.cardState, Card_PortState). */
#define CARD_STATE_NONE         0       /* no PS2 memory card */
#define CARD_STATE_SAME         1       /* the card seen before */
#define CARD_STATE_NEW          2       /* a card put in since the last look (CardTask.changed is set) */
#define CARD_STATE_UNFORMATTED  3

/* The card calls' results (Card_ReadFile, Card_WriteFile, Card_GetDir ...): each call runs over
 * several frames, one libmc command and its sceMcSync at a time. */
#define CARD_CALL_DONE          0
#define CARD_CALL_FAILED        1
#define CARD_CALL_NO_DIR        2       /* Card_GetDir: the listing failed, no such directory (the task reports CARD_ERR_NO_FILE) */
#define CARD_CALL_BUSY          (-1)

/* Card_GetDirEntry's results: whether the task's file (or directory) is on the card. */
#define CARD_ENTRY_FOUND        0
#define CARD_ENTRY_MISSING      1
#define CARD_ENTRY_NO_CARD      2       /* sceMcSync gave -5 or below -10: the tasks report CARD_ERR_NO_CARD */
#define CARD_ENTRY_FAILED       3       /* sceMcSync gave -6: the tasks report CARD_ERR_FAILED */

/* The results of the SaveMenu_* load and save functions, which each run over several frames
 * (NetMenu.saveStep). */
#define SAVE_STEP_RUNNING       0
#define SAVE_STEP_DONE          1
#define SAVE_STEP_FAILED        (-1)    /* NetMenu.result[slot] has the card error */

/* NetMenu.charRead: what the read of a character slot's file found. */
#define CHAR_READ_NONE          0       /* not read, or no hunter in it */
#define CHAR_READ_OK            1
#define CHAR_READ_BAD           0x80    /* bad data (CARD_ERR_BAD_DATA): the slot is cleared */

/* gOldGameKind: the earlier game whose save the boot found on the card (CardBoot_FindMH,
 * CardBoot_FindMHG). While it is set, a new character starts with extra items in its box. */
#define OLD_GAME_NONE   0
#define OLD_GAME_MH     1       /* the first Monster Hunter */
#define OLD_GAME_MHG    2       /* Monster Hunter G */

/* The card operations (gCardOps, CardTask_Begin's argument): what is read or written. */
#define CARD_OP_SAVE        0       /* this game's save: the system file, the characters, the patch */
#define CARD_OP_NET_FILE    1       /* the NET file (the login id) */
#define CARD_OP_OLD_MH      2       /* the first Monster Hunter's save, looked for at boot */
#define CARD_OP_OLD_MHG     3       /* Monster Hunter G's save, looked for at boot */

/* A card operation's files (CardOp.files, CardTask_SelectFile's argument): 0 icon.sys and 1-3
 * the icons, then the data files. */
#define CARD_OP_FILE_SYSTEM     4
#define CARD_OP_FILE_CHAR(n)    (5 + (n))   /* character n's file, n 0-2 */
#define CARD_OP_FILE_PATCH      8
#define CARD_OP_FILES           9

/* The card file being read or written (NetMenu.fileKind, gCardFileKind), one bit each; the
 * card's state flags (CardState_SetFlag) take the same bits. */
#define CARD_FILE_SYSTEM    0x01
#define CARD_FILE_CHAR(n)   (2 << (n))  /* character n's file, n 0-2 */
#define CARD_FILE_CHAR0     0x02
#define CARD_FILE_CHAR1     0x04
#define CARD_FILE_CHAR2     0x08
#define CARD_FILE_PATCH     0x10
#define CARD_FILE_ALL       0x1F

/* The files' sizes and versions (SaveFile_Encrypt, SaveFile_Decrypt). A file opens with
 * CARD_FILE_HEAD random bytes, over whose first words the version, the key and the sum of the
 * body are written; the body is XORed with a stream from the key. */
#define CARD_FILE_HEAD          0x40
#define CARD_FILE_STAMP         0x40    /* the date stamp after the head (two words, Card_GetDate) */
#define CARD_FILE_SYSTEM_SIZE   0x550
#define CARD_FILE_CHAR_SIZE     0x8050
#define CARD_FILE_PATCH_SIZE    0x10080
#define CARD_FILE_SYSTEM_VER    0x100
#define CARD_FILE_VER           1       /* the characters' and the patch's */
#define CARD_FILE_BODY          0x50    /* the body, after the stamp: the system file's is a SysFileBody, a character's a SaveData */
#define CARD_FILE_SYSTEM_BODY_SIZE  0x500   /* the bodies' space, cleared before they are written */
#define CARD_FILE_CHAR_BODY_SIZE    0x8000
#define CARD_CHAR_SLOTS         3       /* the character files of a save, one per character slot */

/* The card image (NetMenu.imageBuf): the save's files one after another, each in its size
 * rounded up to 64 bytes (SaveBuf_Offset, CardTask_StartSave). */
#define CARD_IMAGE_ROUND(size)  (((size) + 63) / 64 * 64)
#define CARD_IMAGE_SYSTEM_SIZE  CARD_IMAGE_ROUND(CARD_FILE_SYSTEM_SIZE)
#define CARD_IMAGE_CHAR_SIZE    CARD_IMAGE_ROUND(CARD_FILE_CHAR_SIZE)
#define CARD_IMAGE_FILE_SYSTEM  0       /* SaveBuf_Offset's kinds of file */
#define CARD_IMAGE_FILE_CHAR    1
#define CARD_IMAGE_FILE_PATCH   2

/* The card menus' buffers in gLoadBuf (CardMenu_InitBuffers): the save icons, the card image,
 * the file read and the save slots, each at its offset. */
#define CARD_BUF_SIZE       0x6AFE8     /* cleared at the set-up */
#define CARD_BUF_IMAGE      0x20000     /* NetMenu.imageBuf */
#define CARD_BUF_READ       0x40400     /* NetMenu.readBuf */
#define CARD_BUF_SLOTS      0x60800     /* NetMenu.slots */
#define CARD_ICONS_SAVE     0x1714      /* the save's icons (a data file, LoadCompressedFile) */
#define CARD_ICONS_NET_FILE 0x1715      /* the NET file's icons */

/* The NET file as the card holds it (CARD_OP_NET_FILE, read into NetMenu.readBuf): the login id
 * and the password the network login saved; the card menu's last mode deletes them. */
#define NET_FILE_ID_LEN         11
#define NET_FILE_PASSWORD_LEN   9
typedef struct NetFile {
    u8 pad0[0x960];
    char loginId[NET_FILE_ID_LEN];              /* 0x960: empty when none is saved */
    char password[NET_FILE_PASSWORD_LEN];       /* 0x96B */
} NetFile;

/* The card menus' states (NetMenu.state), one enum per state
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


/* The card task's steps (CardTask.state, gCardSteps): CardTask_Update runs one each frame. */
enum CardStep {
    CARD_STEP_IDLE = 0,         /* CardTask_IdleStep */
    CARD_STEP_POLL = 1,         /* CardTask_PollStep (CardTask_StartPoll) */
    CARD_STEP_READ = 2,         /* CardTask_ReadStep (CardTask_StartRead) */
    CARD_STEP_FIND = 3,         /* CardTask_FindStep (CardTask_StartFind) */
    CARD_STEP_SAVE = 4,         /* CardTask_SaveStep (CardTask_StartSave) */
    CARD_STEP_FORMAT = 5,       /* CardTask_FormatStep (CardTask_StartFormat) */
    CARD_STEP_STUB = 6,         /* CardTask_StubStep: no code starts it */
    CARD_STEP_DELETE_SAVE = 7,  /* CardTask_DeleteSaveStep: no code starts it */
    CARD_STEP_DELETE_DIR = 8,   /* CardTask_DeleteDirStep: no code starts it */
    CARD_STEP_LIST = 9,         /* CardTask_ListStep (CardTask_StartList) */
    CARD_STEP_COUNT = 10
};

/* The card task (gCardTask, 244 bytes). */
typedef struct CardTask {
    s32 step;             /* 0x00: the task step (the CardTask_*Step functions switch on it) */
    s32 subStep;          /* 0x04: the step within a card call (Card_ReadFile, Card_WriteFile, Card_GetInfo ...) */
    s32 fileNo;           /* 0x08: the file of the save operation being written (op->files[it]; CardTask_SaveStep) */
    s32 entryNo;          /* 0x0C: the directory entries found (Card_GetDir, from syncResult); the entry index (Card_DeleteDir) */
    s32 portCount;        /* 0x10: the ports to scan, 2 (Card_Init; CardTask_PollStep) */
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
    s32 fileFlag;       /* 0xB8: the current file's McFileDesc.flag (CARD_OPFILE_*) */
    s32 size;           /* 0xBC */
    s32 deleteFiles;    /* 0xC0: Card_DeleteDir deletes the directory's files first (only Card_Init's clear sets it, to 0) */
    s32 readFound;      /* 0xC4: CardTask_FindStep reads the file it finds (CardTask_StartFind); Card_DeleteDir reuses it: the directory itself was deleted */
    s32 copyProtect;    /* 0xC8: from CardOp.copyProtect: Card_SetFileInfo marks the files not to be copied */
    s32 state;          /* 0xCC: the step that runs (CARD_STEP_*, gCardSteps) */
    u8 *buf;            /* 0xD0 */
    s32 op;             /* 0xD4: index in gCardOps */
    u8 padD8[0xE0 - 0xD8];
    s32 writing;        /* 0xE0: 1 from a save or a format until CardTask_End; no reader seen */
    s32 mask;           /* 0xE4: files still to write, one bit each */
    u8 padE8[0xF4 - 0xE8];
} CardTask;

/* McFileDesc.flag (CardTask.fileFlag): how CardTask_SaveStep writes the file. */
#define CARD_OPFILE_NONE    0       /* no such file: skipped */
#define CARD_OPFILE_KEEP    1       /* an existing file of the same size is kept (the icons) */
#define CARD_OPFILE_WRITE   2       /* always written */

/* A file of a card operation (16 bytes). */
typedef struct McFileDesc {
    s32 flag;           /* 0x0: CARD_OPFILE_*; nonzero when used */
    char *name;         /* 0x4 */
    u8 *ptr;            /* 0x8 */
    s32 size;           /* 0xC */
} McFileDesc;

/* A card operation (gCardOps): the save's titles and its files
 * (files[0] is icon.sys, 1-3 the icons, 4-8 the data files). */
typedef struct CardOp {
    char *title1;       /* 0x00 */
    char *title2;       /* 0x04 */
    McFileDesc files[CARD_OP_FILES]; /* 0x08 */
    u8 pad98[0x9C - 0x98];
    s32 copyProtect;    /* 0x9C: nonzero: the files are marked not to be copied (only the game save's operation) */
    s32 blocks;         /* 0xA0: card blocks needed */
} CardOp;

/* A memory card date and time (sceMcStDateTime). */
typedef struct McDateTime {
    u8 wday;            /* 0x0: set by McDate_SetWeekday */
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;             /* 0x4 */
    u8 month;           /* 0x5 */
    u16 year;           /* 0x6 */
} McDateTime;

/* A card directory entry as sceMcGetDir fills it (gCardDirEntry, gCardFileAttr; 64 bytes). */
typedef struct McDirEntry {
    McDateTime created;     /* 0x00 */
    McDateTime modified;    /* 0x08: Card_GetDate takes it as the date stamp of a file written */
    u32 size;               /* 0x10: the file's size in bytes */
    u16 attr;               /* 0x14: the attributes (MC_ATTR_*) */
    u8 pad16[0x20 - 0x16];
    u8 name[32];            /* 0x20: the name (bytes: Card_DeleteDir tests its first as unsigned) */
} McDirEntry;

#define MC_ATTR_NO_COPY     0x0008  /* the file may not be copied (Card_SetFileInfo) */
#define MC_INFO_ATTR        4       /* sceMcSetFileInfo's flag: write only the attributes */

/* The save's icon.sys (CardOp.files[0], 964 bytes), which the PS2's browser reads: the
 * title and the save's three icon files. */
typedef struct McIconSys {
    u8 pad000[6];
    u16 titleBreak;         /* 0x006: the byte of the title where its second line starts */
    u8 pad008[0xC0 - 0x8];
    char title[0x44];       /* 0x0C0: the two lines of the title, as one string */
    char icons[3][0x40];    /* 0x104: the icon files (CardOp.files[1..3]) */
    u8 pad1C4[0x3C4 - 0x1C4];
} McIconSys;

/*
 * The patch: a file of the card (CARD_FILE_PATCH) or of the network that changes the game's code
 * and data when it is loaded. Patch_Init clears the patch area (ram_areas.h: PATCH_DATA,
 * PATCH_RECS, PATCH_HEADER), Patch_CopyFile and Patch_LoadStep fill it, and PatchExecCS applies
 * the records of each part of the game as it is loaded (Patch_ExecAll).
 */
#define PATCH_AREA_SIZE     0x10000     /* PATCH_DATA's and PATCH_RECS's size */
#define PATCH_VERSION_LEN   10          /* PatchHeader.version, compared with gVersionString */
#define PATCH_MAGIC_LEN     8           /* PatchRecords.magic, compared with gPatchMagic */

/* The patch's header (PATCH_HEADER, 64 bytes). */
typedef struct PatchHeader {
    char version[PATCH_VERSION_LEN];    /* 0x00: the patch's version; a newer one than gVersionString is loaded */
    char number[4];                     /* 0x0A: a 4-digit number, "1000" after Patch_Init */
    u8 pad0E[0x14 - 0x0E];
    s32 recsSize;                       /* 0x14: the records' size in bytes; 0 when there is no patch */
    u8 pad18[0x40 - 0x18];
} PatchHeader;

/* The patch records (PATCH_RECS): the magic and the count, then the records. A record is its id
 * ((mode << 4) | sub, PatchExecCS), the destination address and the length, then the bytes
 * (padded to 4) or, with PATCH_LEN_INDIRECT set in the length, the address of the bytes. */
typedef struct PatchRecords {
    char magic[PATCH_MAGIC_LEN];        /* 0x0: gPatchMagic */
    s32 count;                          /* 0x8 */
} PatchRecords;
#define PATCH_LEN_INDIRECT  0x80000000

/* A patch file as the card holds it: the save file's head, the header and the data. The records
 * are read into the buffer after it (Patch_BeginFile). */
typedef struct PatchFile {
    u8 fileHead[CARD_FILE_HEAD];        /* 0x00 */
    PatchHeader header;                 /* 0x40 */
    u8 data[PATCH_AREA_SIZE];           /* 0x80 */
} PatchFile;

/* Patch_LoadStep's results (gPatchResult). */
#define PATCH_LOADING   0       /* still loading */
#define PATCH_NONE      1       /* nothing to load: no records, an older version or a bad magic */
#define PATCH_LOADED    2
#define PATCH_FAILED    (-1)    /* the transfer failed */

u8 Card_GetSeed(void);
void CardState_SetFlag(s32 files);
s32 SaveBuf_Offset(s32 addr, s32 kind, s32 n);
void SaveMenu_CopySystem(NetMenu *menu, u8 *data, s32 save);
s32 SaveMenu_SaveSystem(NetMenu *menu);
s32 SaveMenu_LoadSystem(NetMenu *menu, s32 mode);
s32 SaveMenu_SaveExtra(NetMenu *menu);
void Save_CopyToSlot(s32 slot);
void SaveMenu_CopyCharacter(NetMenu *menu, s32 data, s32 save, s32 slot);
s32 SaveMenu_SaveCharacter(NetMenu *menu);
s32 SaveMenu_LoadCharacter(NetMenu *menu);
s32 SaveFile_IsCharUsed(NetMenu *menu, s32 data, s32 save, s32 slot);
s32 SaveMenu_CheckCharacter(NetMenu *menu);
s32 Card_IsSystemSaved(void);
s32 CardMenu_IsTexOn(void);
s32 Card_SlotReady(s32 i);
s32 Card_ReadyCount(void);
void Card_Lock(void);
void Card_Unlock(void);
void Card_Init(void);
s32 Card_Sync(CardTask *task);
s32 Card_GetInfo(CardTask *task);
s32 Card_GetDirEntry(CardTask *task);
s32 Card_ReadFile(CardTask *task);
s32 Card_MkDir(CardTask *task);
s32 Card_CreateFile(CardTask *task);
s32 Card_WriteFile(CardTask *task);
s32 Card_SetFileInfo(CardTask *task);
s32 Card_Delete(CardTask *task);
s32 Card_Format(CardTask *task);
s32 Card_DeleteDir(CardTask *task);
s32 Card_GetDir(CardTask *task);
void CardTask_Begin(s32 i);
void CardTask_Update(void);
void CardTask_End(void);
void CardTask_EndIfDone(CardTask *task);
void CardTask_StartPoll(void);
void CardTask_StartRead(s32 port, u8 *buf);
void CardTask_StartFind(s32 port, u8 *buf, s32 readFound);
void CardTask_StartSave(s32 port, u8 *buf);
void CardTask_SelectFile(s32 fileArg, s32 clear);
void CardTask_StartFormat(s32 port);
s32 CardTask_Result(void);
s32 Card_PortState(s32 i);
void Card_ClearChanged(void);
s32 Card_IsChanged(s32 i);
s32 CardOp_SetFiles(s32 *data);
void Card_GetDate(void *dst);
void McDate_SetWeekday(McDateTime *date);
void CardOp_MakeIconSys(CardTask *task);
void Em_BonePos(void *obj, s16 bone, f32 *out);
extern CardOp *gCardOps[];
extern void (*gCardSteps[])(CardTask *t);
extern CardTask gCardTask;

void Loading_CaptionStub(s32 unused);
s32 TownReload_CheckStub(s32 unused);
void TownReload_MarkStub(s32 unused);
void LobbyDay_SetNoon(void);
void Patch_Init(void);
void PatchExecCS(u8 mode, u8 sub);
void Patch_BeginLoad(PatchHeader *hdr, u8 *src, u8 *dst);
void Patch_ExecAll(void);
void CardMenu_DrawSlots(NetMenu *menu, s32 mode, u8 selectMode, s32 unused, s32 flag);
s32 SaveMenu_SlotUsed(s32 i); /* the save slot is used */
void SaveFile_Encrypt(u8 *buf);
void SaveFile_Decrypt(NetMenu *menu, u16 *words);
s32 SaveMenu_SystemDateMatches(NetMenu *menu);
s32 SaveMenu_ReadBad(NetMenu *menu);
void SysSave_MergeFlags(u8 *buf);
void Patch_InstallFile(u8 *buf);
void SaveExtra_CopyFile(s32 mode, s32 addr);
void Patch_CopyFile(s32 mode, s32 addr);
void Patch_BeginFile(u8 *buf);
void Save_ClearCopy(void);
void SaveMenu_CopyAll(NetMenu *menu);
void SaveMenu_ClearCharacter(NetMenu *menu);
void SaveMenu_CopyCharacterSlot(NetMenu *menu, s32 i);
void CardMenu_InitBuffers(NetMenu *menu);
s16 NetMenu_DrawMessageY(s16 y, s32 id);
s16 NetMenu_DrawMessage(s32 x, s16 y, s32 id);
s32 NetMenu_YesNo(NetMenu *menu, s32 x, s16 y, u8 *cursor);
s32 NetMenu_CardChanged(s32 slot);
void NetMenu_Open(u8 mode);
void NetMenu_Draw(void);
void CardNew_ChooseCharSlot(NetMenu *menu);
void CardGame_AskQuit(NetMenu *menu);
void CardSave_PrepareSave(NetMenu *menu);
void CardSetup_Start(NetMenu *menu);
void CardMenu_RunSetup(NetMenu *menu);
void CardMenu_InitNetBuffers(NetMenu *menu);
extern f32 gLobbyDayFrac;

#endif /* CARD_H */
