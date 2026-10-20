#ifndef SYSSAVE_H
#define SYSSAVE_H

#include "common.h"
#include "save.h"

/*
 * The system data (gSysSave, 0x3C74 bytes): the options,
 * the network options, the chat phrases and the flags saved in the system
 * file (SaveMenu_CopySystem, SysSave_Exchange), and a copy of the loaded
 * character (Save_LoadSlot).
 */

/* The options screen's bytes (select_option.c; Options_Apply copies them
 * into gScene and the HUD). */
typedef struct SysOptions {
    s8 soundMode;               /* 0x0: 0 mono, 1 stereo, 2 surround (SysSettings.soundMode) */
    s8 musicVolume;             /* 0x1: SysSettings.musicVolume */
    s8 seVolume;                /* 0x2: SysSettings.seVolume */
    s8 vibration;               /* 0x3: 0 off (Pad vibration; was gVibrationOn) */
    s8 aimReverse;              /* 0x4: gScene.aimReverse, HudState.optAimReverse */
    s8 equipView;               /* 0x5: nonzero shows the equipment in the character editor (ChrEdit.unk42, Chara_EquipViewOn) */
    s8 chatSize;                /* 0x6: the chat window's size, the index into gChatTextSizes and gChatLogIcons */
    s8 screenX;                 /* 0x7: the screen offset (Display_SetOffset) */
    s8 screenY;                 /* 0x8 */
    s8 camControl;              /* 0x9: gScene.camControl */
    s8 camFollow;               /* 0xA: gScene.camFollow */
    s8 brightness;              /* 0xB: SysSettings.brightness; reset to 0x80 */
    s8 chatBeep;                /* 0xC: 0 beeps at every chat message; else only for messages to all or in AREA_HUB_0x83 (lobby Chat_Beep); HudState.unkA3 */
    u8 unk0D[3];
} SysOptions;

/* The chat phrases (SysSave.phrases): 24 of 0x2C bytes and a terminator;
 * the phrase menu shows each cut to PHRASE_NAME_LEN bytes (gPhraseEntries). */
#define PHRASE_COUNT    24
#define PHRASE_LEN      0x2D
#define PHRASE_NAME_LEN 10

/* A chat phrase in the phrase menu (gPhraseEntries, PHRASE_COUNT of them):
 * its name (the phrase cut to PHRASE_NAME_LEN bytes, Phrase_TruncateNames)
 * and the phrase itself (in SysSave.phrases). */
typedef struct PhraseEntry {
    char name[12];
    char *text;
} PhraseEntry;

typedef char PhraseEntry_size_check[(sizeof(PhraseEntry) == 0x10) ? 1 : -1];

typedef struct SysSave {
    SysOptions options;         /* 0x000 */
    u8 netOptions[16];          /* 0x010: the network options (the options screen's network page; the lobby's Lobby_GetNetOption0/1) */
    char phrases[PHRASE_COUNT][PHRASE_LEN];     /* 0x020: the chat phrases (Phrase_ResetDefaults copies the defaults; the phrase menu gPhraseEntries) */
    s32 bonusUnlocked;          /* 0x458: set by SysSave_UnlockBonus, or ORed in from the system file (SysSave_Exchange);
                                 * unlocks the character editor's last hair style (option 0x11), and the lobby and select
                                 * test it with event flag 0x7C */
    u8 lastSlot;                /* 0x45C: the character slot last saved or loaded (gChrEdit.slot) */
    u8 charSaved;               /* 0x45D: 3 once a character was saved (the PrepareSave states); kept in the system file (SysSave_Exchange); no reader seen */
    u8 savedFlags[4];           /* 0x45E: flag bytes kept in the system file and ORed with its copy on a load, like
                                 * bonusUnlocked (SysSave_Exchange, SysSave_MergeFlags); no code reads them */
    u8 pad462[2];
    SaveData save;              /* 0x464: the character as loaded (Save_LoadSlot loads a slot into it; the character editor builds a new one in it) */
    s8 charsLoaded;             /* 0x3C5C: the characters read from the card: cleared by the card menus' set-up, counted up
                                 * by the loads (CardSave_ReadCharacters, CardSave_CountFree); no code reads it */
    u8 pad3C5D;
    s8 oldGameKind;             /* 0x3C5E: the system file's record of an earlier game's save (SaveMenu_CopySystem); while
                                 * it is 0, the boot shows the message of the save found (gOldGameKind) */
    u8 pad3C5F;
    SaveData *cardSlots;        /* 0x3C60: the three save slots the card menus read and write (gCardSlots, the same word):
                                 * gLobbySaveBuf in the lobby, else gSaveBuf */
    s32 eventFlags[2][2];       /* 0x3C64: the system data's event flags, a bit each (EventFlag_Set, EventFlag_Test),
                                 * ORed with the card copy's (SysSave_Exchange) */
} SysSave;                      /* 0x3C74 bytes */

/* The system file's body as the card holds it (at CARD_FILE_BODY in the file; SysSave_Exchange):
 * the SysSave fields the file keeps. */
typedef struct SysFileBody {
    SysOptions options;         /* 0x00: SysSave.options */
    u8 netOptions[16];          /* 0x10: SysSave.netOptions */
    s32 bonusUnlocked;          /* 0x20 */
    u8 lastSlot;                /* 0x24 */
    u8 charSaved;               /* 0x25 */
    u8 savedFlags[4];           /* 0x26 */
    u8 oldGameKind;             /* 0x2A: SysSave.oldGameKind (OLD_GAME_*) */
    u8 pad2B[0x30 - 0x2B];
    s32 eventFlags[2][2];       /* 0x30 */
    char phrases[PHRASE_COUNT][PHRASE_LEN];     /* 0x40: SysSave.phrases */
} SysFileBody;

extern SysSave gSysSave;

#endif /* SYSSAVE_H */
