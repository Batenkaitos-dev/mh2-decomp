#ifndef NET_MODE_H
#define NET_MODE_H

#include "common.h"

/*
 * sub_main's network mode state gNetMode (0x5D46A0, 0xE0 bytes): the online
 * mode's screens in the lobby (the connection and settings steps, the message
 * boxes, the memory card checks and the NET file save) and yn's network
 * configuration. Merged from the lobby's NetModeState views and yn's
 * YnNetState (round 15). Where a use reads a field with another type, it
 * reads `*(T *)&gNetMode.field`.
 */
typedef struct NetModeState {
    /* 0x00 */ u8 mode;
    /* 0x01 */ u8 step;             /* the mode's step */
    /* 0x02 */ u8 subStep;          /* a screen's step (NetSetup_ShowLoginError; 0xFF: none) */
    /* 0x03 */ u8 innerStep;        /* a step inside a screen's step (LobbyCard_*, Dnas_AuthStep); a count down */
    /* 0x04 */ s16 percent;         /* printed with a full-width percent sign (NetMode_PrintField); a message box's frames (unk_7D2C80.c) */
    /* 0x06 */ s16 timer;           /* a count down in frames; the card check's frames */
    /* 0x08 */ s16 sel3;            /* a choice of three */
    /* 0x0A */ s16 sel2;            /* a choice of two; the card task's result */
    /* 0x0C */ u8 padC[4];
    /* 0x10 */ s8 unk10;
    /* 0x11 */ s8 entry;            /* the entry requested (LobbyEntry_Init) */
    /* 0x12 */ u8 pad12;
    /* 0x13 */ u8 ynNoDraw;         /* nonzero: yn adds no draw callback; a failed connect opens no icon */
    /* 0x14 */ u8 ynResult;         /* set when yn finishes: YnWork.changed + 1 */
    /* 0x15 */ u8 pad15;
    /* 0x16 */ u8 noDraw;           /* bits: 1 skips func_007CF340, 2 skips NetMode_CallDrawScreen */
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
    /* 0x24 */ u8 unk24;
    /* 0x25 */ u8 layout0;          /* box 0's layout (NetMode_SetCursorX) */
    /* 0x26 */ u8 unk26;
    /* 0x27 */ u8 layout1;          /* box 1's */
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
    /* 0x60 */ s16 cursorX0;        /* box 0's cursor x */
    /* 0x62 */ s16 cursorX1;
    /* 0x64 */ u8 slot;             /* the memory card slot */
    /* 0x65 */ s8 slotUsed;         /* the slot used */
    /* 0x66 */ s16 cardFiles;       /* CardOp_SetFiles */
    /* 0x68 */ s8 saveStep;         /* the NET file save's step (unk_7D2C80.c) */
    /* 0x69 */ s8 saveNext;         /* the step after a message */
    /* 0x6A */ s8 cardMsg;          /* the message shown (NetMode_CardMsgError, NetModeMsg_Page3) */
    /* 0x6B */ s8 title;            /* the title text, 1-7 (NetMode_SetTitle; gNetModeTitleTexts) */
    /* 0x6C */ s32 card[2];         /* each slot's card state (LobbyCard_Select: 0-5, 3 has the file) */
    /* 0x74 */ s32 files;
    /* 0x78 */ s8 msg;
    /* 0x79 */ u8 yesNoOpen;        /* 1 while the yes/no box is open (NetMode_OpenYesNo) */
    /* 0x7A */ u8 trySlot;          /* yn: the memory card slot being tried */
    /* 0x7B */ u8 savedSlot;        /* slotUsed kept across the connect (NetSetup_Connect) */
    /* 0x7C */ s8 error[2];         /* yn: per slot, the message of the last failure */
    /* 0x7E */ u8 unk7E;
    /* 0x7F */ s8 netResult;        /* the DNAS or transfer step's outcome: 0 none, 1 running, 2 failed, 3 done */
    /* 0x80 */ s32 err;             /* the error code shown (after an error label) */
    /* 0x84 */ s32 size;            /* the size of the data sent (unk_7D44B0.c) */
    /* 0x88 */ u8 pad88[2];
    /* 0x8A */ u8 xferStep;         /* LobbyNetXfer_Run's step */
    /* 0x8B */ u8 xferSub;
    /* 0x8C */ s8 xferState;        /* the step of Dnas_StateMachine and NetXfer_Step (LobbyNetXfer_Run) */
    /* 0x8D */ u8 unk8D;
    /* 0x8E */ u8 pad8E[0x93 - 0x8E];
    /* 0x93 */ u8 icon2;            /* the icon drawn by part 0x40000 (NetMode_OpenIcon2; gNetModeIcons) */
    /* 0x94 */ s32 tex[16];         /* texture handles (Tex_Use) */
    /* 0xD4 */ s16 iconY;           /* the icon's y offset (written as an s32: NetMode_OpenFrame4E, NetMode_OpenIcon5) */
    /* 0xD6 */ u8 padD6[2];
    /* 0xD8 */ s8 wave;
    /* 0xD9 */ u8 padD9[3];
    /* 0xDC */ s8 cardsFound;       /* the cards found (Lobby_GetNetOption0, network option 0; 3: none) */
    /* 0xDD */ s8 bgmOn;            /* 1 while the network mode's BGM plays (NetMode_BgmOn, NetMode_BgmOff) */
    /* 0xDE */ u8 unkDE[2];
} NetModeState;
typedef char NetModeState_size_check[(sizeof(NetModeState) == 0xE0) ? 1 : -1];

extern NetModeState gNetMode;

#endif /* NET_MODE_H */
