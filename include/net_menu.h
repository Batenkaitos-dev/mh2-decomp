#ifndef NET_MENU_H
#define NET_MENU_H

#include "common.h"

/*
 * The memory card menu (gCardMenu, 0xBC bytes):
 * one state machine per mode (CardMenu_Run*), for loading, saving and the NET file.
 * NetMenu_SetState sets the state; each state draws a message box and moves on.
 */
typedef struct NetMenu {
    u8 state;                   /* 0x00 */
    u8 sub;                     /* 0x01: the state's sub-state */
    u8 charLoaded;              /* 0x2: 1 (or 2) once a character was loaded or read into the slots (the save checks the slot then) */
    s8 slotSel;                 /* 0x03: the highlighted slot of CardMenu_DrawSlotFrames */
    s16 timer;                  /* 0x04 */
    s16 charNo;                 /* 0x06: a character slot: the one being read (CardSave_ReadCharacters: slots + it * 0x37F8), the choice that sets charIndex (CardNew_ChooseSlot, CardLoad_CheckPatch) */
    u8 pad8[8];
    u16 badVersion;             /* 0x10: set by SaveFile_Decrypt */
    u16 badSum;                 /* 0x12: set by SaveFile_Decrypt */
    u8 pad14[4];
    s32 slot;                   /* 0x18 */
    u8 pad1C[0x2C - 0x1C];
    u8 layers[4];               /* 0x2C: what each of the 4 layers draws (NetMenu_Draw) */
    u8 showCursor;              /* 0x30: the cursor box is shown */
    u8 cursor;                  /* 0x31 */
    u8 showButton;              /* 0x32: the confirm button icon blinks: blinkPhase while the box is shown (NetMenu_Draw) */
    u8 hasSave;                 /* 0x33: a character was saved or loaded (card in use) */
    s32 mode;                   /* 0x34: the mode (NetMenu_Open), which state machine runs */
    s32 done;                   /* 0x38: NetMenu_Update returns it: 1 done, 2 quit without saving */
    u16 buttons;                /* 0x3C: gPadState[0].pressed, each frame */
    u16 held;                   /* 0x3E: gPadState[0].held */
    s32 result[2];              /* 0x40: per slot: 0 ok, -1 busy, -251..-256 errors */
    s32 fileBlocks;             /* 0x48: the save size in card blocks (CardOp_SetFiles) */
    s16 boxX;                   /* 0x4C: the cursor box x, y, w, h (Menu_DrawGlowBar) */
    s16 boxY;                   /* 0x4E */
    s16 boxW;                   /* 0x50 */
    s16 boxH;                   /* 0x52 */
    s16 buttonX;                /* 0x54: the button icon x, y (Hud_DrawButtonIconWhite) */
    s16 buttonY;                /* 0x56 */
    s16 blinkTimer;             /* 0x58: frames to the next blink (15) */
    s16 blinkPhase;             /* 0x5A: toggled every 15 frames */
    s16 msg;                    /* 0x5C: message id */
    u8 pad5E[2];
    s32 fileDates[4][2];        /* 0x60: the files' date stamps (Card_GetDate's pair): [0] the system file's, [1..3] the three characters' */
    u8 pad80[4];
    s32 filesToWrite;           /* 0x84: the card files the save writes, CARD_FILE_* bits (CardState_SetFlag) */
    s32 fileKind;               /* 0x88: the card file: 1 system, 2/4/8 character 0-2, 0x10 patch */
    u8 *iconBuf;                /* 0x8C..0x98: buffers in gLoadBuf: the save icons (file 0x1714/0x1715, CardOp_SetFiles) */
    s32 imageBuf;               /* 0x90: the save image (SaveMenu_CopyAll; an integer: SaveBuf_Offset takes it) */
    u8 *readBuf;                /* 0x94: the file read from the card */
    struct SaveData *slots;     /* 0x98: the save slots of the three characters (SaveData[3]) */
    u8 pad9C;
    u8 charRead[3];             /* 0x9D: per character slot: 0 none, 1 read, 0x80 bad data, cleared (CardSave_ReadCharacters; SaveMenu_FreeCharSlots switches on it) */
    u8 padA0[2];
    u8 charIndex;               /* 0xA2: the character being read or saved */
    u8 saveStep;                /* 0xA3: the step of the SaveMenu_* load and save functions (SaveMenu.step); each state clears it before the first call */
    u8 padA4[4];
    s32 readDate[2];            /* 0xA8: the date stamp of the character file just read (SaveMenu_LoadCharacter), stored to fileDates[charIndex + 1] (SaveMenu_CharDateMatches) */
    u8 menuTexOn;               /* 0xB0: 1 in the modes that load gCardMenuTex (NetMenu_Open), which NetMenu_Draw then draws */
    s8 oldSlot;                  /* 0xB1: the slot with Monster Hunter (G) data, -1 none */
    u8 padB2[2];
    s32 openMode;               /* 0xB4: NETMENU_MODE_SAVE_B when opened with it (it runs as NETMENU_MODE_SAVE), else 0 */
    s32 quitResult;             /* 0xB8: the online in-game saves (CardMenu_RunGameSaveNet): the done to set once the lobby has saved its NET file (LobbyCard_SaveNetFile) */
} NetMenu;

typedef char NetMenu_size_check[(sizeof(NetMenu) == 0xBC) ? 1 : -1];

extern NetMenu gCardMenu;

#endif /* NET_MENU_H */
