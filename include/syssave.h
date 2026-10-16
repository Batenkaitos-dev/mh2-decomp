#ifndef SYSSAVE_H
#define SYSSAVE_H

#include "common.h"
#include "save.h"

/*
 * The system data (gSysSave, sub_main 0x6A9E50, 0x3C74 bytes): the options,
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
    s8 chatBeep;                /* 0xC: 0 beeps at every chat message; else only for messages to all or in area 0x83 (lobby Chat_Beep); HudState.unkA3 */
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
    u8 netOptions[16];          /* 0x010: the network options (select_option.c's network page; lobby Lobby_GetNetOption0/1); was gNetOptions, an alias */
    char phrases[PHRASE_COUNT][PHRASE_LEN];     /* 0x020: the chat phrases (Phrase_ResetDefaults copies the defaults; the phrase menu gPhraseEntries) */
    s32 unk458;                 /* 0x458: set by func_003BDA10; 0 hides the character editor option 0x11, the lobby tests it with event flag 0x7C */
    u8 lastSlot;                /* 0x45C: the character slot last saved or loaded (gChrEdit.slot) */
    u8 unk45D;                  /* 0x45D: set to 3 when a character is saved */
    u8 unk45E;                  /* 0x45E-0x461: flags ORed with the card copy's (SysSave_Exchange) */
    u8 unk45F;
    u8 unk460;
    u8 unk461;
    u8 pad462[2];
    SaveData save;              /* 0x464: the character as loaded (Save_LoadSlot loads a slot into it; the character editor builds a new one in it) */
    u8 pad3C5C[2];
    u8 oldGameKind;             /* 0x3C5E: copied from the system file (gOldGameKind) */
    u8 pad3C5F[0x3C64 - 0x3C5F];
    s32 unk3C64[2][2];          /* 0x3C64: flags ORed with the card copy's (SysSave_Exchange) */
} SysSave;                      /* 0x3C74 bytes */

extern SysSave gSysSave;

#endif /* SYSSAVE_H */
