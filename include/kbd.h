#ifndef KBD_H
#define KBD_H

/* Where a key of a keyboard layout is drawn. */
typedef struct KbdKeyPos {
    f32 x;    /* 0x0 */
    s16 y;    /* 0x4 */
    u16 kind; /* 0x6: box size, index into gKbdBoxSizes */
} KbdKeyPos;

typedef char KbdKeyPos_size_check[(sizeof(KbdKeyPos) == 0x8) ? 1 : -1];

/* A key's character (KbdKey.ch): code 0 is no key. Otherwise b[0] says what the key does
 * (Kbd_Run, Kbd_Decide): 1 a key kind or mode switch, b[1] the kind (Kbd_PressKey); 2 a
 * command, b[1] the command (Kbd_PressCmdKey); 3 the phrase (Kbd_InsertPhrase); 4 a gesture,
 * b[1] the gesture (Kbd_DoGesture); else b[0..1] is the SJIS character it types (Kbd_EnterChar
 * copies it). */
typedef union KbdChar {
    u8 b[2];
    u16 code;
} KbdChar;

/* A key of a keyboard layout (4 bytes: KbdLayout.keys, KbdEntry.key). */
typedef struct KbdKey {
    u8 glyph;           /* 0x0: the glyph drawn on it (Kbd_Draw; the Kbd_CharIn* and
                           Kbd_CharIs* tests read it) */
    u8 font;            /* 0x1: bits 0-3: its gKbdGlyphs entry (Kbd_Draw) */
    KbdChar ch;         /* 0x2 */
} KbdKey;

typedef char KbdKey_size_check[(sizeof(KbdKey) == 0x4) ? 1 : -1];

/* A keyboard layout (the entries of gKbdSetLayouts and gKbdAltLayouts; the current one is
 * KbdEntry.layout). */
typedef struct KbdLayout {
    KbdKey *keys;       /* 0x00 */
    KbdKeyPos *cursors; /* 0x04: one per key */
    s8 *grid;           /* 0x08: the key at row * 4 + column */
    u8 *extra;          /* 0x0C: 0x10-byte entries */
    u8 nBoxes;          /* 0x10: the key boxes drawn (Kbd_Draw) */
    u8 nKeys;           /* 0x11: the keys with a glyph (Kbd_Draw) */
} KbdLayout;

typedef char KbdLayout_size_check[(sizeof(KbdLayout) == 0x14) ? 1 : -1];

/* The software keyboard and text entry of sub_main (unk_30EA30.c, unk_314E00.c): the state
 * gKbdState, reached through the pointer gKbd. Keys at +0x668..+0x671 are USB HID codes. */
typedef struct KbdEntry {
    KbdLayout *layout;          /* 0x000: the keyboard layout (Kbd_PickLayout) */
    KbdKeyPos *keyPos;          /* 0x004: the key under the cursor: where it is drawn, layout->cursors[keyIdx] (Kbd_LookupKey) */
    KbdKey *key;                /* 0x008: the key under the cursor, in layout->keys (Kbd_LookupKey; Kbd_TypeKey) */
    u8 *keyInfo;                /* 0x00C: the key under the cursor: its 16-byte entry (Kbd_LookupKey; +0xC a phrase, Kbd_InsertPhrase) */
    u8 *kindConf;               /* 0x010: the entry kind settings, gKbdKindConfs[kind] (Kbd_Open): the colours (KBD_COLW), +0x30 a phrase list, +0x34 an extra layout (Kbd_BindLayouts) */
    f32 drawScale;              /* 0x014: the horizontal draw scale, 1.0 (Kbd_Draw) */
    u16 frame;                  /* 0x018: counted up each frame (Kbd_Run) */
    s16 repeatKeys;             /* 0x01A: the buttons held for key repeat (Kbd_KeyRepeat) */
    s8 repeatWait;              /* 0x01C: the frames to the next repeat (Kbd_KeyRepeat: 10 after a press, then 4, 8 or 1) */
    u8 kind;                    /* 0x01D: the entry kind: 5 and 10 are passwords (drawn as asterisks) */
    u8 keySet;                  /* 0x01E: the keyboard (D_00416440: its character set) */
    u8 mode;                    /* 0x01F: the key mode (Kbd_FindKind) */
    s32 flags;                  /* 0x020: the modes not allowed (bits); 8 refuses some ASCII symbols */
    u8 col;                     /* 0x024: the cursor's column (1..20) */
    u8 row;                     /* 0x025: the cursor's row (0..3) */
    u8 offGrid;                 /* 0x026: 1 while the cursor is off the key grid (moved past row 0 or row 3; Kbd_ClearActive), 0 on it */
    u8 cursorShown;             /* 0x027: the cursor is shown */
    u8 blink;                   /* 0x028: counted up each frame; the cursor is hidden while bit 0x20 is set (Kbd_DrawLine); 0 when the cursor moves or the text changes */
    u8 unk29;
    u16 textPos;                /* 0x02A: the cursor in text */
    u16 kanaPos;                /* 0x02C: the cursor in kana */
    s8 keyIdx;                  /* 0x02E: the key index under the cursor (the layout table at +8; Kbd_LookupKey) */
    u8 converting;              /* 0x02F: 1 while converting, 2 while choosing segments */
    s8 active;                  /* 0x030: 1: the keyboard takes the input (Kbd_SetActive / Kbd_ClearActive) */
    s8 open;                    /* 0x031: Kbd_SetOpen */
    s8 result;                  /* 0x032: 1 submitted, -1 cancelled (Kbd_Submit, Kbd_Cancel, Kbd_Decide) */
    u8 accept;                  /* 0x033: what the text accepts: 0x10 digits, 0x20 digits and '-', 0x40 digits and capitals, 0x80 digits and "-*#"; 2 no '%' */
    s8 unk34;                   /* 0x034 */
    u8 gestureMode;             /* 0x035: 0xF after Kbd_EnterGestures: the kind's extra list (kindConf + 0x34) is used; mode 13 then types and dakuten is refused; 0 on Kbd_PressKey, Kbd_NextRow, Kbd_FKeyRow, Kbd_OpenPhrases */
    s8 symbolMode;              /* 0x036: nonzero in the symbol mode (Kbd_UpdateSymbolMode): the key and segment handling differ (Kbd_PressKey, Kbd_TypeKey, Kbd_NextSegment) */
    s8 polled;                  /* 0x037: the USB keyboard was read this frame (Kbd_ReadUsb) */
    s16 y;                      /* 0x038: Kbd_SetPos */
    u16 maxLen;                 /* 0x03A: the text's maximum length */
    s32 firstShown;             /* 0x03C: the first character shown */
    f32 x;                      /* 0x040: Kbd_SetPos */
    s32 submitWait;             /* 0x044: Kbd_SetSubmitWait */
    s16 cyclePos;               /* 0x048: the phrase list position (Kbd_CyclePhrase: +5 a page, wraps at 60) or the muhenkan cycle (Kbd_MuhenkanKey) */
    u8 prevKeySet;              /* 0x04A: keySet before Kbd_ZenkakuKey */
    u8 keyFlags;                /* 0x04B: 1 half-width (Kbd_ZenkakuKey); 2 a phrase or gesture is being cycled (Kbd_CyclePhrase, Kbd_CycleGesture), cleared by the cursor keys */
    char text[0x100];           /* 0x04C: the text */
    s32 state;                  /* 0x14C: the next state (0x14..0x18) */
    u8 caseCands;               /* 0x150: 1: the text to convert is wide alphanumerics, so the candidates are its case forms (Kbd_StartConvert, Kbd_MakeCaseCands) */
    u8 unk151[3];
    s32 candIdx;                /* 0x154: the candidate shown: gKbdCands[it % 3] (Kbd_PrevCand, Kbd_PageUp, Kbd_PageDown) */
    s32 candPage;               /* 0x158: the candidate page (3 a page: Kbd_ListCandidates(it * 3)) */
    s32 candId;                 /* 0x15C: the conversion candidate: ConvCmd_NextCand (Kbd_Confirm, Kbd_SkipSameCands), gKbdCands (Kbd_PrevCand); 0: none */
    s32 candCount;              /* 0x160: the number of conversion candidates (Kbd_CountCands) */
    s32 candCol;                /* 0x164: the column the conversion candidate is drawn at (x + 10 * it; Kbd_DrawCandidateBox) */
    u8 kana[0x100];             /* 0x168: the romaji being typed (Str_ToWide converts it into wide) */
    u8 wide[0x100];             /* 0x268 */
    char convDone[0x100];       /* 0x368: the converted part */
    char cand[0x100];           /* 0x468: the candidate */
    char convRest[0x100];       /* 0x568: the rest */
    u8 usbKey;                  /* 0x668: the key read (HID) */
    u8 usbPrevKey;              /* 0x669 */
    u8 usbNewKey;               /* 0x66A: usbKey when it changed */
    u8 usbMods;                 /* 0x66B: the modifier bits, USB_MOD_* */
    u8 usbPrevMods;             /* 0x66C */
    s8 usbModsDown;             /* 0x66D: the modifiers pressed this frame */
    u8 kanaInput;               /* 0x66E: Kbd_KanaInput */
    u8 repeatTimer;             /* 0x66F */
    u8 curKey;                  /* 0x670: the key being handled (Kbd_HandleKey) */
    u8 prevKey;                 /* 0x671 */
} KbdEntry;

typedef char KbdEntry_size_check[(sizeof(KbdEntry) == 0x674) ? 1 : -1];

extern KbdEntry *gKbd;

/*
 * The USB keyboard's codes, as KbdEntry's usb fields hold them: the HID
 * usage ids of the keys (usbKey, usbNewKey) and the modifier bits (usbMods;
 * each name covers the left and the right key).
 */
#define USB_MOD_CTRL 0x11
#define USB_MOD_SHIFT 0x22
#define USB_MOD_ALT 0x44
#define USB_KEY_NONE 0x00
#define USB_KEY_ENTER 0x28
#define USB_KEY_ESCAPE 0x29
#define USB_KEY_BACKSPACE 0x2A
#define USB_KEY_F1 0x3A             /* F1 to F12 are 0x3A to 0x45 */
#define USB_KEY_F6 0x3F
#define USB_KEY_F7 0x40
#define USB_KEY_F9 0x42
#define USB_KEY_F10 0x43
#define USB_KEY_F11 0x44
#define USB_KEY_F12 0x45
#define USB_KEY_RIGHT 0x4F
#define USB_KEY_LEFT 0x50
#define USB_KEY_DOWN 0x51
#define USB_KEY_UP 0x52
#define USB_KEY_KP_ENTER 0x58       /* the keypad's Enter */
#define USB_LED_CAPS_LOCK 0x02      /* UsbKbData.leds (gKbdSettings, its copy): Caps Lock is on */

/*
 * The keyboard's key sets (KbdEntry.keySet, an index of gKbdSetLayouts; seen
 * from each layout's key table). KBD_SET_SHIFTED is the bit of a set's
 * small-kana or lower-case pair (gKbdShiftSets); KBD_SET_BIT(set) is the
 * set's bit in KbdEntry.flags, which bars it.
 */
#define KBD_SET_HIRA 0
#define KBD_SET_KATA 1
#define KBD_SET_WIDE_UPPER 2            /* full-width capitals */
#define KBD_SET_SYMBOL 3
#define KBD_SET_PHRASE1 4               /* the phrase pages */
#define KBD_SET_PHRASE2 5
#define KBD_SET_HALF_KATA 6             /* half-width katakana */
#define KBD_SET_ASCII_UPPER 7
#define KBD_SET_SMALL_HIRA 8
#define KBD_SET_SMALL_KATA 9
#define KBD_SET_WIDE_LOWER 0xA
#define KBD_SET_HALF_SYMBOL 0xB
#define KBD_SET_GESTURE 0xD             /* the gestures page */
#define KBD_SET_HALF_SMALL_KATA 0xE
#define KBD_SET_ASCII_LOWER 0xF
#define KBD_SET_DIGIT 0x10
#define KBD_SET_SHIFTED 8
#define KBD_SET_BIT(set) (1 << (set))

/* The layouts after the sets' own (gKbdSetLayouts; Kbd_PickLayout). */
#define KBD_LAYOUT_ASCII_UPPER2 17      /* the ASCII capitals again: entry kind 7 */
#define KBD_LAYOUT_WIDE_UPPER_NOSYM 18  /* the full-width capitals without symbols: kinds 8-10 */
#define KBD_LAYOUT_WIDE_LOWER_NOSYM 19

/* The character modes (KbdEntry.mode, gKbdSetModes[keySet]); a mode key's
 * kind is its mode, and the mode's first set has the same number. */
#define KBD_MODE_HIRAGANA 0
#define KBD_MODE_KATAKANA 1
#define KBD_MODE_ALNUM 2
#define KBD_MODE_SYMBOL 3
#define KBD_MODE_PHRASE1 4
#define KBD_MODE_PHRASE2 5
#define KBD_MODE_GESTURE 13

/* The kinds of the switch keys after the modes (Kbd_PressKey, Kbd_FindKindEx). */
#define KBD_SWITCH_TO_HALF 6            /* the full-width sets' width key */
#define KBD_SWITCH_TO_FULL 7            /* the half-width sets' width key */
#define KBD_SWITCH_SHIFT 8              /* to the set's KBD_SET_SHIFTED pair */
#define KBD_SWITCH_UNSHIFT 9            /* back from it */

/* KbdChar.b[0]: what a key does (see KbdChar). */
#define KBD_ACT_SWITCH 1
#define KBD_ACT_COMMAND 2
#define KBD_ACT_PHRASE 3
#define KBD_ACT_GESTURE 4

/* KbdChar.b[1] of a KBD_ACT_COMMAND key (Kbd_PressCmdKey). */
#define KBD_CMD_BACKSPACE 0
#define KBD_CMD_CONVERT 1
#define KBD_CMD_SUBMIT 3

#define KBD_GESTURE_KANA 0xF            /* KbdEntry.gestureMode: the gestures page types kana */
#define KBD_KEYF_CYCLING 2              /* KbdEntry.keyFlags: a phrase or gesture cycle runs (R2) */
#define KBD_MARK_DAKUTEN 1              /* Kbd_DakutenKeyKind's results */
#define KBD_MARK_HANDAKUTEN 2
#define KBD_RESULT_SUBMIT 1             /* KbdEntry.result */
#define KBD_RESULT_CANCEL (-1)
#define KBD_SOUND_NONE (-1)             /* KbdEntry.state: no sound this frame */

/* The buttons the keyboard repeats (Kbd_KeyRepeat): all but L3, TRIANGLE,
 * SELECT and START. */
#define KBD_REPEAT_BUTTONS 0x3EFD

/* A romaji table entry (gRomajiKanaTbl, 240 of them; Kbd_RomajiToKana): the romaji and its kana
 * in three kinds (hiragana, katakana, half-width). */
typedef struct RomajiEntry {
    char *romaji;
    char *kana[3];
} RomajiEntry;

typedef char RomajiEntry_size_check[(sizeof(RomajiEntry) == 0x10) ? 1 : -1];

extern RomajiEntry gRomajiKanaTbl[];

/* The conversion candidates (gKbdCandLines): a page shows
 * KBD_CAND_PAGE of them (Kbd_DrawCandidateBox), each a line of
 * KBD_CAND_LINE_SIZE bytes. */
#define KBD_CAND_PAGE 3
#define KBD_CAND_LINE_SIZE 0x100

/* The USB keyboard (libusbkb; sub_main unk_2F6540.c's UsbKb_* functions). */
/* What sceUsbKbGetInfo fills in (gUsbKbInfo, 0x90 bytes). */
typedef struct UsbKbInfo {
    s32 unk0;                   /* not read */
    s32 connectedCount;         /* 0x4: UsbKb_StepWaitConnect waits for nonzero */
    u8 connected[0x88];         /* 0x8: per keyboard, nonzero when it is connected (UsbKb_IsConnected) */
} UsbKbInfo;

/* A keyboard's state as sceUsbKbRead returns it (gUsbKbData); UsbKb_Poll packs it into
 * the 9-byte report gUsbKbReport. */
typedef struct UsbKbData {
    s32 leds;                   /* 0x00: the report's byte 1 */
    s32 modifiers;              /* 0x04: the modifier keys, the report's byte 0 */
    s32 keyCount;               /* 0x08: the key codes in keys; 0: no report */
    u16 keys[7];                /* 0x0C: key codes, the report's bytes 2-8 */
} UsbKbData;

#endif
