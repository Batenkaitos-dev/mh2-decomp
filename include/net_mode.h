#ifndef NET_MODE_H
#define NET_MODE_H

#include "common.h"
#include "task.h"
#include "sprite.h"

/*
 * sub_main's network mode state gNetMode (0x5D46A0, 224 bytes): the online
 * mode's screens in the lobby (the connection and settings steps, the message
 * boxes, the memory card checks and the NET file save) and yn's network
 * configuration. Where a use reads a field with another type, it reads
 * `*(T *)&gNetMode.field`.
 */
typedef struct NetModeState {
    /* 0x00 */ u8 mode;
    /* 0x01 */ u8 step;             /* the mode's step */
    /* 0x02 */ u8 subStep;          /* a screen's step (NetSetup_ShowLoginError; NET_SUBSTEP_NONE: none) */
    /* 0x03 */ u8 innerStep;        /* a step inside a screen's step (LobbyCard_*, Dnas_AuthStep); a count down */
    /* 0x04 */ s16 percent;         /* printed with a full-width percent sign (NetMode_PrintField); the NET file save also counts a message's frames down in it */
    /* 0x06 */ s16 timer;           /* a count down in frames; the card check's frames */
    /* 0x08 */ s16 sel3;            /* a choice of three */
    /* 0x0A */ s16 sel2;            /* a choice of two; the card task's result */
    /* 0x0C */ u8 padC[4];
    /* 0x10 */ s8 savedTaken;       /* 1 once the saved set-up is taken (NetSetup_UseSaved); cleared for the settings menu; no reader seen */
    /* 0x11 */ s8 entry;            /* the entry requested (LobbyEntry_Start) */
    /* 0x12 */ u8 pad12;
    /* 0x13 */ u8 ynNoDraw;         /* nonzero: yn adds no draw callback; a failed connect opens no icon */
    /* 0x14 */ u8 ynResult;         /* set when yn finishes: YnWork.changed + 1 */
    /* 0x15 */ u8 pad15;
    /* 0x16 */ u8 noDraw;           /* NET_NO_DRAW_* bits */
    /* 0x17 */ u8 pad17;
    /* 0x18 */ u8 yesNo;            /* 0 yes, 1 no (NetMode_ChooseYesNo) */
    /* 0x19 */ u8 rowWait;          /* frames to the next row step (NetMode_DrawScreen) */
    /* 0x1A */ u8 row;              /* a list row, 0-15 (NetMode_ListEntryUV) */
    /* 0x1B */ u8 pad1B;
    /* 0x1C */ s32 boxes;           /* a bit per box shown */
    /* 0x20 */ u8 unk20;
    /* 0x21 */ s8 nA;               /* the first message queue's length */
    /* 0x22 */ s8 nB;               /* the second's */
    /* 0x23 */ u8 icon;             /* the icon drawn by part 0x20 (NetMode_OpenIcon; gNetModeIcons) */
    /* 0x24 */ u8 menuSet;          /* which of the menus' three sprite sets is drawn (gNetMenuRows); nothing writes it */
    /* 0x25 */ u8 layout0;          /* menu 0's layout: where its cursor's rows are (NetMode_SetCursorY) */
    /* 0x26 */ u8 unk26;            /* NetMode_OpenMenu's second argument; nothing reads it */
    /* 0x27 */ u8 layout1;          /* menu 1's (NetMode_OpenMenu) */
    /* 0x28 */ u8 anim0;            /* frame counters of the animations */
    /* 0x29 */ u8 anim0Frame;
    /* 0x2A */ u8 blink;
    /* 0x2B */ u8 blinkOn;
    /* 0x2C */ u8 anim1;
    /* 0x2D */ u8 anim1Frame;
    /* 0x2E */ s8 openW;            /* the box opening animation's steps */
    /* 0x2F */ s8 openH;
    /* 0x30 */ u8 idA[8];
    /* 0x38 */ u8 kindA[8];
    /* 0x40 */ u8 argA[8];
    /* 0x48 */ u8 idB[8];
    /* 0x50 */ u8 kindB[8];
    /* 0x58 */ u8 argB[8];
    /* 0x60 */ s16 cursorY0;        /* menu 0's cursor: its y offset for the choice */
    /* 0x62 */ s16 cursorY1;        /* menu 1's */
    /* 0x64 */ u8 slot;             /* the memory card slot */
    /* 0x65 */ s8 slotUsed;         /* the slot used */
    /* 0x66 */ s16 cardFiles;       /* CardOp_SetFiles */
    /* 0x68 */ s8 saveStep;         /* the NET file save's step */
    /* 0x69 */ s8 saveNext;         /* the step after a message */
    /* 0x6A */ s8 cardMsg;          /* the message shown (NetMode_CardMsgError, NetModeMsg_Page3) */
    /* 0x6B */ s8 title;            /* the title text, NET_TITLE_* (NetMode_SetTitle; gNetModeTitleTexts) */
    /* 0x6C */ s32 card[2];         /* each slot's card state, enum NetCardState (LobbyCard_Select) */
    /* 0x74 */ s32 files;
    /* 0x78 */ s8 msg;              /* the message shown, enum NetModeMsg */
    /* 0x79 */ u8 yesNoOpen;        /* 1 while the yes/no box is open (NetMode_OpenYesNo) */
    /* 0x7A */ u8 trySlot;          /* yn: the memory card slot being tried */
    /* 0x7B */ u8 savedSlot;        /* slotUsed kept across the connect (NetSetup_Connect) */
    /* 0x7C */ s8 error[2];         /* yn: per slot, the message of the last failure */
    /* 0x7E */ u8 unk7E;
    /* 0x7F */ s8 netResult;        /* the DNAS or transfer step's outcome: 0 none, 1 running, 2 failed, 3 done */
    /* 0x80 */ s32 err;             /* the error code shown (after an error label) */
    /* 0x84 */ s32 size;            /* the size of the data sent (the lobby's data transfer) */
    /* 0x88 */ u8 pad88[2];
    /* 0x8A */ u8 xferStep;         /* LobbyPatch_Install's step */
    /* 0x8B */ u8 xferSub;
    /* 0x8C */ s8 xferState;        /* the step of Dnas_StateMachine and NetXfer_Step (LobbyPatch_Install) */
    /* 0x8D */ u8 unk8D;
    /* 0x8E */ u8 pad8E[0x93 - 0x8E];
    /* 0x93 */ u8 icon2;            /* the icon drawn by part 0x40000 (NetMode_OpenIcon2; gNetModeIcons) */
    /* 0x94 */ s32 tex[16];         /* texture handles (Tex_Use) */
    /* 0xD4 */ s16 iconY;           /* the icon's y offset (written as an s32: NetMode_OpenFrameTop, NetMode_OpenIcon5) */
    /* 0xD6 */ u8 padD6[2];
    /* 0xD8 */ s8 wave;
    /* 0xD9 */ u8 blinkCount;       /* counts frames for a blink (odd frames lit) */
    /* 0xDA */ u8 animTick;         /* 0-3, then animFrame steps (NetMode_StepAnim) */
    /* 0xDB */ u8 animFrame;        /* an icon's frame, 0-2 */
    /* 0xDC */ s8 cardSearch;       /* which slots the set-up searches, NET_CARD_SEARCH_* (Lobby_GetNetOption0, network option 0) */
    /* 0xDD */ s8 bgmOn;            /* 1 while the network mode's BGM plays (NetMode_BgmOn, NetMode_BgmOff) */
    /* 0xDE */ u8 unkDE[2];
} NetModeState;
typedef char NetModeState_size_check[(sizeof(NetModeState) == 0xE0) ? 1 : -1];

/* gNetMode.error[slot]: why yn's connect with a slot's settings failed
 * (Yn_Update, from Yn_Connect's result); the online set-up turns the two
 * slots' errors into its message. */
enum NetYnError {
    NET_YN_ERR_NONE = 0,
    NET_YN_ERR_OTHER_PS2 = 0x1E,   /* the file fails netcnf's check (another console's) */
    NET_YN_ERR_NO_SETTINGS = 0x1F, /* the saved settings or proxy aren't on the card */
    NET_YN_ERR_NO_CARD = 0x20,
    NET_YN_ERR_CARD_REMOVED = 0x21,
    NET_YN_ERR_DEVICE = 0x22,      /* the settings' device isn't attached or won't start */
    NET_YN_ERR_EMPTY_FILE = 0x2D,  /* the settings file has no entries */
    NET_YN_ERR_NOT_TRIED = 0x7F    /* set by the online set-up before yn runs without drawing */
};

#define NET_TEX_SLOT 163            /* the network screen's first texture slot (gTexHandles) */
#define NET_TEX_SLOTS 14            /* the slots it frees */
#define NET_TEX_COUNT 16            /* NetModeState.tex */
#define NET_TEX_LETTERS 11          /* gNetMode.tex: the capital letters' font (TEX_SLOT_LOAD_LETTERS; NetMode_DrawBigText) */
#define NET_FILE_CARD_PACK 5909     /* the card task's pack: the NET file's icons and the card files' list (CardOp_SetFiles) */
#define NET_FILE_CARD_PACK_ALT 5908 /* loaded instead in LobbyCard_LoadFiles' other modes */
#define NET_LOBBY_TEX_SLOT 2        /* sub_main's lobby set: its first slot */
#define NET_LOBBY_TEX_SLOTS 10

/* gNetMode.noDraw's bits: the draws the network screen skips. */
#define NET_NO_DRAW_HOOK 1          /* NetMode_EmptyDrawHook */
#define NET_NO_DRAW_SCREEN 2        /* NetMode_CallDrawScreen */

/*
 * gNetMode.boxes: a bit for each box of the network screen that is shown.
 * NetMode_DrawScreen draws them (NetMode_DrawPart) in the order of the
 * screen's layers; NetMode_CloseBoxes clears them. The two menus' bits
 * (MENU0 and MENU1) draw the same sprites: menu 0's are never opened.
 */
#define NET_BOX_FRAME 0x1           /* the frame (sprite set NET_SPR_FRAME; NetMode_OpenFrame) */
#define NET_BOX_PANEL 0x2           /* sprite set NET_SPR_PANEL; nothing opens it */
#define NET_BOX_MENU0_ROWS 0x4      /* menu 0's rows, placed by the choice (NetMode_MenuRowsY) */
#define NET_BOX_MENU0_LIT 0x8       /* menu 0's chosen row */
#define NET_BOX_MENU0_CURSOR 0x10   /* menu 0's cursor; closing it opens NET_BOX_MENU0_PICKED */
#define NET_BOX_ICON 0x20           /* the icon (NetMode_OpenIcon) */
#define NET_BOX_PROGRESS 0x40       /* the progress bar (NetMode_OpenProgress) */
#define NET_BOX_PERCENT 0x80        /* the percentage (NetMode_OpenPercent) */
#define NET_BOX_STATUS_MSG 0x100    /* the transfer's messages, one blinking (NetMode_OpenStatus) */
#define NET_BOX_STATUS 0x200        /* the transfer's two status sprites, faded by its result */
#define NET_BOX_SPARE 0x400         /* sprite set NET_SPR_SPARE; nothing opens it */
#define NET_BOX_MENU1_ROWS 0x800    /* menu 1's rows (NetMode_OpenMenu) */
#define NET_BOX_MENU1_LIT 0x1000    /* menu 1's chosen row */
#define NET_BOX_MENU1_CURSOR 0x2000 /* menu 1's cursor; closing it opens NET_BOX_MENU1_PICKED */
#define NET_BOX_MENU0_PICKED 0x4000 /* the chosen row growing across the screen (NetBox_GrowPicked) */
#define NET_BOX_MENU1_PICKED 0x8000 /* the same for menu 1 */
#define NET_BOX_STATUS_NONE 0x10000 /* opened with the status boxes; draws nothing */
#define NET_BOX_ERROR 0x20000       /* the error's messages (NetMode_OpenError) */
#define NET_BOX_ICON2 0x40000       /* the second icon (NetMode_OpenIcon2) */
#define NET_BOX_TITLE 0x80000       /* the title (NetMode_SetTitle) */
#define NET_BOX_YES_NO_LARGE 0x100000 /* a larger yes/no bar; nothing opens it */
#define NET_BOX_SLOT_CHOICE 0x200000 /* the memory card slot's choice (NetMode_PickSlot) */
#define NET_BOX_YES_NO 0x400000     /* the yes/no bar (NetMode_OpenYesNo) */
#define NET_BOX_PROGRESS2 0x800000  /* the second progress bar (NetMode_OpenProgress2) */

/* The sprite sets of the network screen (gNetModeSprDefs: each a table of
 * NetSprDef; NetMode_DrawPart's boxes). */
/* A sprite of the network screen's sprite sets and of the login screen's
 * (gLoginFrameSprites), drawn by NetMode_DrawSprites. */
typedef struct NetSprDef {
    s16 tex;                    /* NetModeState.tex[] */
    s16 unk02;                  /* no reader seen */
    s16 x, y, w, h;
    s16 u0, v0, u1, v1;
    u32 color;
} NetSprDef;

/* a menu item's box on the login screen (gLoginBoxSprites) */
typedef struct LoginBox {
    s16 x, y, w, h;
} LoginBox;
#define NET_SPR_FRAME 0             /* the frame, one sprite */
#define NET_SPR_PANEL 1             /* two sprites */
#define NET_SPR_CURSOR 2            /* the menus' cursor, one sprite (its texture rows step with gNetMode.row) */
#define NET_SPR_BAR 8               /* the progress bar, four sprites: the bar is the fourth */
#define NET_SPR_STATUS 10           /* the transfer's two status sprites */
#define NET_SPR_SPARE 11            /* two sprites */

/* The kinds of a queued message (gNetMode.kindA, kindB): the drawer
 * NetModeMsg_Flush calls for it. */
enum NetModeMsgKind {
    NETMODE_KIND_PAGE = 0,          /* a part of a message page (LobbyMsg_DrawPage) */
    NETMODE_KIND_NONE = 1,          /* draws nothing (LobbyMsg_DrawNone) */
    NETMODE_KIND_FIELD = 2,         /* a field, NET_FIELD_* (NetMode_PrintField) */
    NETMODE_KIND_PAGE4 = 3,         /* a part of page 4 (LobbyMsg_DrawPage4) */
    NETMODE_KIND_NONE_ALT = 4,      /* draws nothing (LobbyMsg_DrawNoneAlt) */
    NETMODE_KIND_LIST = 5,          /* a list of choices (LobbyMsg_DrawList) */
    NETMODE_KIND_COUNTDOWN = 6      /* the countdown, then page 6's part 1 */
};

/* NetModeMsg_Queue's queues: the first is drawn under the boxes, the second
 * over them. */
#define NETMODE_QUEUE_BACK 0        /* the first queue, as a page part */
#define NETMODE_QUEUE_FRONT 1       /* the second queue, as a page part */
#define NETMODE_QUEUE_LIST 5        /* the second queue, as a list */
#define NETMODE_QUEUE_LEN 8         /* each queue's room */

/* The fields of the network screen (NetMode_PrintField, gNetFieldPos);
 * fields 0, 3 and 4 print nothing, and 8 and 9 print the version too. */
#define NET_FIELD_LOGIN_ID 1        /* the login id */
#define NET_FIELD_PASSWORD 2        /* the password, as stars */
#define NET_FIELD_PERCENT 5         /* gNetMode.percent, with a percent sign */
#define NET_FIELD_ERROR_CODE 6      /* gNetMode.err, after its label */
#define NET_FIELD_VERSION 7         /* the version, a.bcd */

/* gNetMode.netResult: the outcome of the DNAS check or the patch's install
 * (NetSetup_Dnas, LobbyPatch_Install set it). */
#define NET_RESULT_NONE 0
#define NET_RESULT_RUNNING 1
#define NET_RESULT_FAILED 2
#define NET_RESULT_DONE 3

/* gNetMode.title: the network screen's titles (NetMode_SetTitle). */
#define NET_TITLE_NONE 0
#define NET_TITLE_CARD 1            /* the memory card's choice and read (LobbyCard_Select) */
#define NET_TITLE_CONNECT 2         /* yn's connection with the saved network settings */
#define NET_TITLE_LOGIN 3           /* the connection to the network (NetSetup_Login) */
#define NET_TITLE_PROXY 4           /* yn's proxy settings */
#define NET_TITLE_ACCOUNT 5         /* the login ID's deletion (NetSetup_DeleteAccount) */
#define NET_TITLE_MENU 6            /* the online menu (NetSetup_SettingsMenu's first list) */
#define NET_TITLE_SETTINGS 7        /* the network settings (its second list) */

/* gNetMode.cardSearch: which memory card slots the set-up searches for its
 * file (network option 0). */
#define NET_CARD_SEARCH_BOTH 0      /* slot 1, then slot 2 */
#define NET_CARD_SEARCH_SLOT1 1
#define NET_CARD_SEARCH_SLOT2 2
#define NET_CARD_SEARCH_ASK 3       /* the player picks the slot */

/* gNetMode.card[slot]: what the card task found in a slot (from its
 * CARD_ERR_* results). */
enum NetCardState {
    NET_CARD_UNCHECKED = -1,
    NET_CARD_NONE = 0,              /* no memory card (CARD_ERR_NO_CARD) */
    NET_CARD_FULL = 1,              /* no file, and no space for one (CARD_ERR_NO_SPACE) */
    NET_CARD_NO_FILE = 2,           /* no file (CARD_ERR_NO_FILE, CARD_ERR_UNFORMATTED); after a failed save */
    NET_CARD_HAS_FILE = 3,          /* the file is there */
    NET_CARD_BAD_DATA = 4,          /* the file's data is wrong (CARD_ERR_BAD_DATA, LobbyCard_CheckNetValues) */
    NET_CARD_ERROR = 5              /* another error */
};

/*
 * gNetMode.msg: the network screen's messages (NetModeMsg_Page1), named by
 * the cases that show them. The two slots' yn results (NetYnError) pick
 * NETMODE_MSG_YN_*: one message when both slots failed the same way, and
 * one for each pair of different failures (first slot 1's, then slot 2's).
 */
enum NetModeMsg {
    NETMODE_MSG_NONE = 0,
    NETMODE_MSG_ASK_QUIT = 3,       /* with yes/no: yes quits the set-up */
    NETMODE_MSG_CHECKING_CARDS = 4, /* while both slots are searched */
    NETMODE_MSG_NO_CARDS = 5,       /* no memory card in either slot */
    NETMODE_MSG_CARD_REMOVED = 6,   /* the card left its slot during a question */
    NETMODE_MSG_NO_FILE_BOTH = 7,   /* neither card has the file: with yes/no, yes creates it */
    NETMODE_MSG_NO_FILE = 8,        /* the card has no file: with yes/no, yes creates it */
    NETMODE_MSG_PICK_SLOT_NEW = 9,  /* the slot choice for a new file */
    NETMODE_MSG_READING = 10,
    NETMODE_MSG_READ_FAILED = 11,
    NETMODE_MSG_SAVING = 12,
    NETMODE_MSG_SAVED = 13,
    NETMODE_MSG_SAVE_FAILED = 14,
    NETMODE_MSG_NO_SPACE = 15,
    NETMODE_MSG_ASK_FORMAT = 16,    /* the card is unformatted: with yes/no, yes formats it */
    NETMODE_MSG_FORMATTING = 17,
    NETMODE_MSG_FORMAT_FAILED = 18,
    NETMODE_MSG_NO_CARD = 26,       /* no memory card in the slot */
    NETMODE_MSG_BAD_DATA_BOTH = 27,
    NETMODE_MSG_BAD_DATA = 28,
    NETMODE_MSG_YN_TRYING = 29,     /* while yn tries the saved settings without its screens */
    NETMODE_MSG_YN_FAILED = 31,     /* yn failed, for another reason or a mix */
    NETMODE_MSG_YN_NO_CARD = 32,    /* NET_YN_ERR_NO_CARD (both slots, or slot 1 with another failure) */
    NETMODE_MSG_YN_CARD_REMOVED = 33,
    NETMODE_MSG_YN_DEVICE = 34,
    NETMODE_MSG_PICK_SLOT = 35,     /* the slot choice for the file to read */
    NETMODE_MSG_CARD_ERROR_BOTH = 36,
    NETMODE_MSG_CARD_ERROR = 37,
    NETMODE_MSG_ASK_DELETE_ID = 38, /* with yes/no: the login ID's deletion */
    NETMODE_MSG_LOADED = 40,        /* the file is read: the set-up goes on */
    NETMODE_MSG_ASK_SETUP_GUIDE = 41, /* with yes/no: yes starts yn's set-up guide */
    NETMODE_MSG_YN_CONNECTED = 42,  /* a slot's saved settings connected */
    NETMODE_MSG_YN_OTHER_PS2 = 43,
    NETMODE_MSG_YN_NO_SETTINGS = 44,
    NETMODE_MSG_YN_EMPTY_FILE = 46,
    NETMODE_MSG_CHECKING_CARD = 47, /* while one slot is searched */
    NETMODE_MSG_ASK_OVERWRITE = 48, /* the new file's slot already has one: with yes/no */
    NETMODE_MSG_YN_NO_CARD_EMPTY = 49,
    NETMODE_MSG_YN_NO_CARD_OTHER_PS2 = 50,
    NETMODE_MSG_YN_EMPTY_NO_CARD = 51,
    NETMODE_MSG_YN_EMPTY_OTHER_PS2 = 52,
    NETMODE_MSG_YN_OTHER_PS2_NO_CARD = 53,
    NETMODE_MSG_YN_OTHER_PS2_EMPTY = 54,
    NETMODE_MSG_YN_NO_CARD_NO_SETTINGS = 55,
    NETMODE_MSG_YN_EMPTY_NO_SETTINGS = 56,
    NETMODE_MSG_YN_NO_SETTINGS_NO_CARD = 57,
    NETMODE_MSG_YN_NO_SETTINGS_OTHER_PS2 = 58,
    NETMODE_MSG_YN_NO_SETTINGS_EMPTY = 59,
    NETMODE_MSG_YN_OTHER_PS2_NO_SETTINGS = 60
};

/* NetMode_RunSetup's modes (gNetMode.mode, NetMode_SetMode). */
enum NetSetupMode {
    NET_SETUP_START = 0,            /* the music on (NetMode_StartSetup) */
    NET_SETUP_CARD = 1,             /* the memory card's choice and read (LobbyCard_Select) */
    NET_SETUP_CHECK_SAVED = 2,      /* whether a saved set-up exists (NetSetup_CheckSaved) */
    NET_SETUP_CONNECT = 3,          /* the connection (NetSetup_Connect) */
    NET_SETUP_LOGIN = 4,            /* the login (NetSetup_Login) */
    NET_SETUP_USE_SAVED = 5,        /* the saved set-up taken (NetSetup_UseSaved) */
    NET_SETUP_SAVE_NET_FILE = 6,    /* the NET file saved (NetSetup_SaveNetFile) */
    NET_SETUP_DNAS = 7,             /* the DNAS check (NetSetup_Dnas) */
    NET_SETUP_PORTAL = 8,           /* the KDDI portal (LobbyKddi_Portal) */
    NET_SETUP_SETTINGS = 9,         /* the settings menu (NetSetup_SettingsMenu) */
    NET_SETUP_LEAVE = 10,           /* on to the lobby's scene (NetSetup_Leave) */
    NET_SETUP_DISCONNECT = 11,      /* the disconnect (NetSetup_Disconnect) */
    NET_SETUP_DISCONNECT_2 = 12,    /* the same; nothing sets it */
    NET_SETUP_QUIT = 13             /* back to gm_sub (NetSetup_Quit) */
};

extern NetModeState gNetMode;

s32 LobbyCard_Select(void);
s32 NetSetup_Connect(void);
void NetSetup_ResetLobby(void);
s32 NetSetup_Login(void);
s32 LobbyKddi_Portal(void);
s32 NetSetup_UseSaved(void);
void NetMode_ClearBuffers(void);
s32 NetSetup_Leave(void);
s32 NetSetup_Disconnect(void);
s32 NetSetup_Quit(void);
void NetSetup_ToSettings(void);
s32 NetSetup_CheckSaved(void);
u8 NetMode_ErrorMsg(s32 code);
void NetMode_ResetAnim(void);
s32 NetSetup_SettingsMenu(void);
s32 NetSetup_ShowLoginError(void);
void LobbyLogin_FreeTextures(void);
void NetMode_ResetStep(void);
s32 LobbyKddi_PortalScene(void);
void LobbyLogin_DrawMenu(void);
void LoginWave_Clear(void);
void LoginWave_Mark(s32 screen);
s32 LoginWave_Update(void);
void LobbyLogin_DrawFrameBase(void);
void LoginMenu_PlayCursorSe(void);
void LoginMenu_PlayDecideSe(void);
s32 LoginPad_Triggered(void);
s32 LoginPad_Held(s8 port);
s32 LoginPad_DecidePressed(s32 port);
s32 LoginPad_CancelPressed(s32 port);
s32 LoginPad_StartHeld(s32 port);
s32 LoginPad_AnyStartHeld(void);
void LoginSe_Decide(void);
void LoginSe_Cancel(void); /* lobby: plays SE_SYS_CANCEL */
void LoginSe_Alert(void);
void LoginSe_Cursor(void);
/* LoginPad_Decide's and LoginPad_DecideEx's sounds. */
#define LOGIN_SE_NONE 0
#define LOGIN_SE_DECIDE 1           /* LoginSe_Decide */
#define LOGIN_SE_ALERT 2            /* LoginSe_Alert */
/* gNetSaveData.mmbbLoginId[0] without a login ID. */
#define LOGIN_ID_EMPTY 0
s32 LoginPad_Decide(s32 sound);
s32 LoginPad_DecideEx(s32 sound, s32 idRow);
s32 LoginPad_Cancel(void);
s32 NetMode_ChooseYesNo(void);

void NetModeMsg_Queue(s32 where, s32 page, s32 part);
void NetModeMsg_Page0(s32 part);
void NetMode_SetCursorY(s8 which);
void NetModeMsg_Flush(s8 which);
void NetMode_DrawScreen(s32 withText);
void NetMode_DrawPart(s32 part);
void NetMode_DrawTitle(void);
void NetMode_DrawBigText(char *str, s32 x, s32 y, s32 size);
s32 NetMode_WaveTitleTex(void); /* nonzero while the net mode's texture slot 9 is in use: the loading mark is then the lobby's waving title */
void LobbyLogin_DrawWaveTitle(s32 unused); /* the lobby's; its definition takes no argument */
extern char *gFullWidthDigits[4]; /* entry 0: the full-width digits 0-9 */
extern u32 gLoginItemColors[8]; /* the menu items' colours ([0] unused) */
extern u32 gLoginLitColors[8]; /* the lit item's colours ([0] unused) */
char **LobbyMsg_GetLines(s32 page, s32 part);
s32 LobbyMsg_DrawPart(s32 size, s32 page, s32 tag, s32 y, s32 lineH);
s32 LobbyMsg_Format(s32 page, char *src, char *dst);
s32 LobbyMsg_CenterX(char *str, s32 size);
void LobbyMsg_Print(char *str, s32 x, s32 y, s32 color, s32 size);
void LobbyMsg_DrawPage(s32 page, s32 part);
void LobbyMsg_DrawNoneAlt(s32 unused, s32 unused2);
void LobbyMsg_DrawPage4(s32 page, s32 part);
void LobbyMsg_DrawPage5(s32 part);
void LobbyMsg_DrawNone(s32 unused, s32 unused2);
void LobbyLogin_DrawBlinkOn(void);
void LobbyMsg_DrawIcon(s32 kind, s16 x, s16 y, s32 big);
void LobbyMsg_DrawList(s32 page, s32 part);
extern char *gNetModeTitleTexts[8];

void NetMode_PlayBgm(void);
s32 NetOverlay_Request(s32 kind); /* the lobby */
s32 NetOverlay_Ready(s32 unused);
s32 NetSetup_DeleteAccount(s32 mode);
s32 LoginPad_StartHeldLong(s16 *heldFrames);
s32 NetSetup_Dnas(void);
s32 NetSetup_SaveNetFile(void);
void LobbyPatch_ResetInstall(void);
s32 LobbyPatch_Install(void);

void NetMode_EmptyDrawHook(s32 unused);
void NetMode_CallDrawScreen(s32 withText);
void NetMode_Begin(DrawSlot *ds);
s32 NetMode_FadeRunning(void);
void NetMode_FadeReset(void);
s32 NetMode_FadeStart(s32 unused, s32 frames, s16 fadeOut);
void NetMode_ReleaseTextures(s32 unused);
void NetMode_EmptyResetHook(void);
void NetMode_EmptyFadeHook(s32 unused);
void NetMode_BeginCardTask(s32 mode);
void NetMode_EmptyInitHook(void);
void NetMode_PrintField(s32 unused, s32 field);
void NetMode_CloseAll(void);
void NetMode_ResetToFrame(void);

u8 Lobby_GetNetOption0(void);
s32 Lobby_GetNetOption1(void);
void NetMode_Init(void);
void NetMode_OpenFrameTop(void);
void NetMode_OpenIcon5(void);
void NetMode_OpenIcon7(void);
void NetMode_ReloadFrame(void);
void NetMode_SetTitle(s32 title);
void NetMode_LoadTextures(s32 mode);
void NetMode_BindTextures(void);
void NetSetupTask_Run(void *task);
void NetSetupTask_Init(TaskSlot *task);
void NetSetupTask_Step(TaskSlot *task);
void NetSetupTask_Quit(TaskSlot *task);
void NetSetupTask_StartScene(TaskSlot *task);
void Lobby_SetSysBusy(void);
void Lobby_ClearSysBusy(void);
void NetMode_SetMode(s8 mode);
void NetMode_Clear(void);
s32 NetMode_RunSetup(void);
void NetMode_BgmOn(void);
void NetMode_BgmOff(void);
s32 NetMode_StartSetup(void);
void NetModeMsg_Page1(s32 id);
void NetModeMsg_Queue5(s32 arg, s32 id);
void NetModeMsg_Page3(s32 id);
void NetMode_OpenYesNo(s8 cursor);
void NetMode_OpenYesNoAtNo(void);
void NetMode_PickSlot(void);
void NetMode_CloseYesNoBox(void);

void NetMode_CloseBoxes(s32 bits);
void NetMode_OpenFrame(void);
void NetMode_OpenIcon(void);
void NetMode_OpenIconN(s32 icon);
void NetMode_OpenIcon2(s8 icon);
void NetMode_OpenMenu(s8 layout, s8 arg);
void NetMode_OpenProgress(s32 unused);
void NetMode_OpenProgress2(void);
void NetMode_OpenPercent(void);
void NetMode_OpenStatus(void);
void NetMode_OpenError(void);
void NetMode_OpenYesNoBar(void);
void NetMode_CloseYesNo(void);
void NetMode_OpenSlotChoice(void);
void NetMode_EmptyMsgHook(u8 unused);
void NetModeMsg_QueuePage4(u8 part);
void NetModeMsg_QueueField(u8 field);
void NetModeMsg_QueueCountdown(u8 layout);
void NetOverlay_Load(s32 id);
s32 NetOverlay_Busy(void);
s32 NetKbd_Run(void);
void NetKbd_Open(s32 x, s32 y, void *text, s32 maxChars, s32 kind);
char *NetKbd_GetText(void);
void NetKbd_SetActive(s32 on);
void NetKbd_Draw(void);
void NetMode_EmptyDrawEnd(void);

#endif /* NET_MODE_H */
