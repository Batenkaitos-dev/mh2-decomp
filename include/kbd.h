#ifndef KBD_H
#define KBD_H
#include "syssave.h"

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
    u8 *extra;          /* 0x0C: 16-byte entries */
    u8 nBoxes;          /* 0x10: the key boxes drawn (Kbd_Draw) */
    u8 nKeys;           /* 0x11: the keys with a glyph (Kbd_Draw) */
} KbdLayout;

typedef char KbdLayout_size_check[(sizeof(KbdLayout) == 0x14) ? 1 : -1];

/* An entry kind's settings (gKbdKindConfs[kind]; KbdEntry.kindConf, which
 * the code reads through byte offsets). */
typedef struct KbdKindConf {
    u32 colors[9];              /* 0x00: the keyboard's colours (KBD_COLW, KBD_PULSE) */
    u32 unk24;
    s32 gestureBinds;           /* 0x28: the gestures page's gKbdGestureBinds entry (0: the kind has no gestures) */
    PhraseEntry *gestures;      /* 0x2C: the gestures page's list (its KbdLayout.extra: Kbd_BindLayouts) */
    s32 phraseBinds;            /* 0x30: the phrase pages' gKbdPhrase1Binds and gKbdPhrase2Binds entry (0: no phrases) */
    PhraseEntry *phrases;       /* 0x34: the phrase pages' list (their KbdLayout.extra) */
} KbdKindConf;
typedef char KbdKindConf_size_check[(sizeof(KbdKindConf) == 0x38) ? 1 : -1];

/* An entry kind's key set and accepted characters (gKbdKeySets[kind]: Kbd_SetLayout). */
typedef struct KbdKindKeys {
    u8 keySet;                  /* 0x0: the starting key set (KbdEntry.keySet) */
    u8 accept;                  /* 0x1: KbdEntry.accept */
} KbdKindKeys;

/* The software keyboard and text entry of sub_main: the state
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
    u8 keySet;                  /* 0x01E: the keyboard (gKbdSetCharsets: its character set) */
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
    u8 gestureMode;             /* 0x035: 15 after Kbd_EnterGestures: the kind's extra list (kindConf + 52) is used; mode 13 then types and dakuten is refused; 0 on Kbd_PressKey, Kbd_NextRow, Kbd_FKeyRow, Kbd_OpenPhrases */
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
    s32 state;                  /* 0x14C: the next state (20 to 24) */
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
#define USB_KEY_F1 0x3A             /* F1; the function keys run on in order to USB_KEY_F12 */
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
#define USB_KEY_A 0x04              /* the first key; the codes below it are USB_KEY_NONE and the HID error codes */
#define USB_KEY_TAB 0x2B
#define USB_KEY_SPACE 0x2C
#define USB_KEY_LBRACKET 0x2F       /* the at-sign key of a Japanese keyboard: the dakuten key */
#define USB_KEY_RBRACKET 0x30       /* the left-bracket key of a Japanese keyboard: the handakuten key */
#define USB_KEY_BACKSLASH 0x31
#define USB_KEY_NONUS_HASH 0x32
#define USB_KEY_GRAVE 0x35          /* the Hankaku/Zenkaku key of a Japanese keyboard */
#define USB_KEY_CAPS_LOCK 0x39
#define USB_KEY_F2 0x3B
#define USB_KEY_F3 0x3C
#define USB_KEY_F4 0x3D
#define USB_KEY_F5 0x3E
#define USB_KEY_F8 0x41
#define USB_KEY_PRINT_SCREEN 0x46
#define USB_KEY_PAUSE 0x48
#define USB_KEY_INSERT 0x49
#define USB_KEY_HOME 0x4A
#define USB_KEY_PAGE_UP 0x4B
#define USB_KEY_DELETE 0x4C
#define USB_KEY_END 0x4D
#define USB_KEY_PAGE_DOWN 0x4E
#define USB_KEY_NUM_LOCK 0x53
#define USB_KEY_KP_SLASH 0x54       /* the first keypad key; the keypad runs from here to the key before USB_KEY_NONUS_BACKSLASH */
#define USB_KEY_NONUS_BACKSLASH 0x64 /* the first key after the keypad */
#define USB_KEY_KP_EQUAL_AS400 0x86
#define USB_KEY_INTL1 0x87          /* the Ro key of a Japanese keyboard */
#define USB_KEY_KANA 0x88           /* the Katakana/Hiragana key */
#define USB_KEY_INTL3 0x89          /* the Yen key of a Japanese keyboard */
#define USB_KEY_HENKAN 0x8A         /* the conversion key */
#define USB_KEY_MUHENKAN 0x8B       /* the no-conversion key */
#define USB_KEY_INTL6 0x8C          /* from here on the keys are ignored */
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
#define KBD_KEYF_HALF 1                 /* KbdEntry.keyFlags: the half-width set is on (Kbd_ZenkakuKey) */
#define KBD_BLINK_HIDDEN 0x20           /* KbdEntry.blink: the cursor is hidden while this bit is set */
#define KBD_KIND_CHECKED 8              /* KbdEntry.kind: the kind whose characters Kbd_CharAllowed checks */
#define KBD_LINE_WIDTH 0x2C             /* the bytes of text the entry line shows */
#define KBD_SYMBOLS_LISTED 5            /* KbdEntry.symbolMode: symbols are typed from the symbol candidates */

/* KbdEntry.accept's bits: what the text accepts. */
#define KBD_ACCEPT_FIXED_CASE 0x01      /* seen: Kbd_CapsLock doesn't switch the case */
#define KBD_ACCEPT_NO_PERCENT 0x02      /* no percent sign, full-width or ASCII */
#define KBD_ACCEPT_DIGITS 0x10          /* digits only */
#define KBD_ACCEPT_DIGITS_DASH 0x20     /* digits and '-' */
#define KBD_ACCEPT_DIGITS_UPPER 0x40    /* digits and capitals */
#define KBD_ACCEPT_DIAL 0x80            /* digits, '-', '*' and '#' */

/* Kbd_MoveOffKey's directions. */
#define KBD_DIR_LEFT 0
#define KBD_DIR_RIGHT 1
#define KBD_DIR_UP 2
#define KBD_DIR_DOWN 3

/* The USB keys' character tables (gKbdHidChars, gKbdHidWideStrs), by kind. */
#define KBD_HID_ASCII 2                 /* one-byte characters (+ shift * 3) */
#define KBD_HID_ROMAJI 4                /* the romaji table's two-byte strings (+ shift * 5) */

/* RomajiEntry.kana[]'s forms, which Kbd_ConvertRomaji takes. */
#define KBD_KANA_HIRA 0
#define KBD_KANA_KATA 1
#define KBD_KANA_HALF 2                 /* half-width katakana */
#define ROMAJI_TBL_COUNT 240            /* gRomajiKanaTbl's entries */
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
extern char *gKbdHidWideStrs[10];
extern char *gKbdHidKeypadWideStrs[2];
extern char *gKbdHidChars[6];
extern char *gKbdHidNoRomaji[2];
extern char *gKbdHidKeypadChars[4];
extern char *gKanaNChars[3];
extern char *gKbdSymbolCands[36];
extern PhraseEntry gPhraseEntries[PHRASE_COUNT];
extern u8 gKbdSetModes[32];

/* The conversion candidates (gKbdCandLines): a page shows
 * KBD_CAND_PAGE of them (Kbd_DrawCandidateBox), each a line of
 * KBD_CAND_LINE_SIZE bytes. */
#define KBD_CAND_PAGE 3
#define KBD_CAND_LINE_SIZE 0x100

/* The USB keyboard (libusbkb, through the UsbKb_* functions). */
/* What sceUsbKbGetInfo fills in (gUsbKbInfo, 144 bytes). */
typedef struct UsbKbInfo {
    s32 unk0;                   /* not read */
    s32 connectedCount;         /* 0x4: UsbKb_StepWaitConnect waits for nonzero */
    u8 connected[0x88];         /* 0x8: per keyboard, nonzero when it is connected (UsbKb_IsConnected) */
} UsbKbInfo;

/* The 9-byte report UsbKb_Poll packs (gUsbKbReport), as a USB keyboard's
 * boot report: the modifier keys (USB_MOD_*), the LEDs, then up to seven
 * key codes. */
typedef struct UsbKbReport {
    u8 modifiers;               /* 0x0 */
    u8 leds;                    /* 0x1 */
    u8 keys[7];                 /* 0x2 */
} UsbKbReport;

/* A keyboard's state as sceUsbKbRead returns it (gUsbKbData); UsbKb_Poll packs it into
 * the 9-byte report gUsbKbReport. */
typedef struct UsbKbData {
    s32 leds;                   /* 0x00: the report's byte 1 */
    s32 modifiers;              /* 0x04: the modifier keys, the report's byte 0 */
    s32 keyCount;               /* 0x08: the key codes in keys; 0: no report */
    u16 keys[7];                /* 0x0C: key codes, the report's bytes 2-8 */
} UsbKbData;

/* Whether the software keyboard is open (nonzero); the follow camera tests it. */
s32 Sjis_PrevCharSize(u8 *str, s32 pos);
s32 Sjis_CharSizeAt(char *str, s32 pos);
void Kbd_OpenConv(void);
void Kbd_KanaDakuten(void);
void Kbd_KanaHandakuten(void);
s32 Kbd_SetIsWide(void);
s32 Kbd_DoGesture(char *text, u16 *pos, s32 gesture);
void Kbd_SaveKeySet(void);
void Kbd_SetPhrasePage(u8 page);
s8 Kbd_LookupKey(void);
void Kbd_InitState(s8 language);
void Kbd_ClearUnk34(s8 language);
void Kbd_SetLayout(u8 entryKind);
s32 Kbd_KeyRepeat(s16 held, s16 pressed);
void Kbd_MoveCursor(s16 keys, s16 held);
void Kbd_TypeKey(KbdKey *key);
void Kbd_EnterChar(KbdKey *key);
void Kbd_CyclePhrase(void);
void Kbd_CycleGesture(void);
void Kbd_NextChatTarget(void);
void Kbd_InsertPhrase(void);
void Kbd_ToggleWidth(void);
void Kbd_DeleteBack(s32 unused);
void Kbd_Submit(void);
void Kbd_NextRow(s32 back);
void Kbd_BindLayouts(s32 unused);
void Kbd_BindStartKeys(void);
s32 Kana_SwapBeforeCursor(s8 *table);
void Str_ToWide(s8 *src, s8 *dst);
void Kbd_DrawSpecialKey(s16 dy, s32 color, f32 dx);
void Kbd_FillRect(s32 color, s16 x, s16 y, s16 right, s16 bottom);
s32 Kbd_KeysShown(void);
void Kbd_Draw(u8 unused);
void Kbd_PressKey(KbdKey *key);
void Kbd_RefreshLayout(void);
s32 Kbd_DakutenKeyKind(KbdKey *kbdKey);
s32 Kana_AddDakuten(void);
s32 Kana_AddHandakuten(void);
s8 Kbd_IsOpen(void);
void Kbd_Cancel(void);
void Kbd_Close(void);
void Kbd_SetPos(s32 posY, f32 posX);
void Kbd_GetPos(f32 *x, s16 *y);
void Kbd_SetSubmitWait(s32 wait);
void Kbd_SetActive(void);
void Kbd_ClearActive(void);
void Kbd_SetOpen(void);
void Kbd_ClearOpen(void);
s32 Kbd_FindKind(u8 firstSet, u8 *outMode, u8 *outKeySet);
s32 Kbd_FindKindEx(u8 kind, u8 *outMode, u8 *outKeySet);
void Kbd_UpdateMode(void);
void Kbd_ToggleShift(void);
void Kbd_NextPhrasePage(void);
void Kbd_HomePageCursor(void);
void Kbd_PickLayout(void);
void Kbd_EnterGestures(void);
void Kbd_ClearPolled(void);
s32 Kbd_CharIsSpecialKana(KbdKey *key);
s32 Kbd_CharBlocked(KbdKey *key);
s32 Kbd_CharInSet5_Unk(KbdKey *key);
s32 Kbd_CharInSet4_Unk(KbdKey *key);
s32 Kbd_CharInRows01(KbdKey *key);
s32 Kbd_CharInRowsHigh(KbdKey *key);
s32 Kbd_CharIsPlain(KbdKey *key);
void Kbd_SaveKanaInput(void);
void Kbd_Printf(const char *fmt, ...);

void Kbd_ReadUsb(void);
extern char gKbdHidKeypadChars_0[24];
extern char gKbdHidNoRomaji_0[64];
extern char gKbdHidChars_5[64];
extern char gKbdHidChars_4[64];
extern char gKbdHidChars_3[80];
extern char gKbdHidChars_2[64];
extern char gKbdHidChars_1[64];
extern char gKbdHidChars_0[64];
extern char gKbdHidKeypadWideStrs_0[48];
extern char gKbdHidWideStrs_9[128];
extern char gKbdHidWideStrs_8[128];
extern char gKbdHidWideStrs_7[128];
extern char gKbdHidWideStrs_6[128];
extern char gKbdHidWideStrs_5[128];
extern char gKbdHidWideStrs_4[128];
extern char gKbdHidWideStrs_3[128];
extern char gKbdHidWideStrs_2[128];
extern char gKbdHidWideStrs_1[128];
extern char gKbdHidWideStrs_0[128];
extern char gRomajiKanaTbl_239_romaji[16];
extern char D_0046C1C8[8];
extern char D_0046C1C0[8];
extern char D_0046C1B8[8];
extern char gRomajiKanaTbl_238_romaji[8];
extern char gRomajiKanaTbl_237_kana_2[8];
extern char gRomajiKanaTbl_237_kana_1[8];
extern char gRomajiKanaTbl_237_kana_0[8];
extern char gRomajiKanaTbl_237_romaji[8];
extern char gRomajiKanaTbl_236_kana_2[8];
extern char gRomajiKanaTbl_236_kana_1[8];
extern char gRomajiKanaTbl_236_kana_0[8];
extern char gRomajiKanaTbl_236_romaji[8];
extern char gRomajiKanaTbl_235_kana_2[8];
extern char gRomajiKanaTbl_235_kana_1[8];
extern char gRomajiKanaTbl_235_kana_0[8];
extern char gRomajiKanaTbl_235_romaji[8];
extern char gRomajiKanaTbl_234_kana_2[8];
extern char gRomajiKanaTbl_234_romaji[8];
extern char gRomajiKanaTbl_233_kana_2[8];
extern char D_0046C130[8];
extern char D_0046C128[8];
extern char gRomajiKanaTbl_233_romaji[8];
extern char gRomajiKanaTbl_232_kana_2[8];
extern char gRomajiKanaTbl_232_kana_1[8];
extern char gRomajiKanaTbl_232_kana_0[8];
extern char gRomajiKanaTbl_232_romaji[8];
extern char gRomajiKanaTbl_231_kana_2[8];
extern char gRomajiKanaTbl_231_kana_1[8];
extern char gRomajiKanaTbl_231_kana_0[8];
extern char gRomajiKanaTbl_231_romaji[8];
extern char gRomajiKanaTbl_230_kana_2[8];
extern char gRomajiKanaTbl_230_kana_1[8];
extern char gRomajiKanaTbl_230_kana_0[8];
extern char gRomajiKanaTbl_230_romaji[8];
extern char gRomajiKanaTbl_229_kana_2[8];
extern char gRomajiKanaTbl_229_kana_1[8];
extern char gRomajiKanaTbl_229_kana_0[8];
extern char gRomajiKanaTbl_229_romaji[8];
extern char gRomajiKanaTbl_228_kana_2[8];
extern char gRomajiKanaTbl_228_kana_1[8];
extern char gRomajiKanaTbl_228_kana_0[8];
extern char gRomajiKanaTbl_228_romaji[8];
extern char gRomajiKanaTbl_227_kana_2[8];
extern char gRomajiKanaTbl_227_kana_1[8];
extern char gRomajiKanaTbl_227_kana_0[8];
extern char gRomajiKanaTbl_227_romaji[8];
extern char gRomajiKanaTbl_226_kana_2[8];
extern char gRomajiKanaTbl_226_kana_1[8];
extern char gRomajiKanaTbl_226_kana_0[8];
extern char gRomajiKanaTbl_226_romaji[8];
extern char gRomajiKanaTbl_225_kana_2[8];
extern char gRomajiKanaTbl_225_kana_1[8];
extern char gRomajiKanaTbl_225_kana_0[8];
extern char gRomajiKanaTbl_225_romaji[8];
extern char gRomajiKanaTbl_224_kana_2[8];
extern char gRomajiKanaTbl_224_kana_1[8];
extern char gRomajiKanaTbl_224_kana_0[8];
extern char gRomajiKanaTbl_224_romaji[8];
extern char gRomajiKanaTbl_223_kana_2[8];
extern char gRomajiKanaTbl_223_kana_1[8];
extern char gRomajiKanaTbl_223_kana_0[8];
extern char gRomajiKanaTbl_223_romaji[8];
extern char gRomajiKanaTbl_222_kana_2[8];
extern char gRomajiKanaTbl_222_kana_1[8];
extern char gRomajiKanaTbl_222_kana_0[8];
extern char gRomajiKanaTbl_222_romaji[8];
extern char gRomajiKanaTbl_221_romaji[8];
extern char gRomajiKanaTbl_220_romaji[8];
extern char gRomajiKanaTbl_219_romaji[8];
extern char D_0046BFA0[8];
extern char D_0046BF98[8];
extern char D_0046BF90[8];
extern char gRomajiKanaTbl_218_romaji[8];
extern char D_0046BF80[8];
extern char D_0046BF78[8];
extern char D_0046BF70[8];
extern char gRomajiKanaTbl_217_romaji[8];
extern char D_0046BF60[8];
extern char D_0046BF58[8];
extern char D_0046BF50[8];
extern char gRomajiKanaTbl_216_romaji[8];
extern char gRomajiKanaTbl_215_kana_2[8];
extern char gRomajiKanaTbl_215_kana_1[8];
extern char gRomajiKanaTbl_215_kana_0[8];
extern char gRomajiKanaTbl_215_romaji[8];
extern char gRomajiKanaTbl_214_kana_2[8];
extern char gRomajiKanaTbl_214_kana_1[8];
extern char gRomajiKanaTbl_214_kana_0[8];
extern char gRomajiKanaTbl_214_romaji[8];
extern char gRomajiKanaTbl_213_kana_2[8];
extern char gRomajiKanaTbl_213_kana_1[8];
extern char gRomajiKanaTbl_213_kana_0[8];
extern char gRomajiKanaTbl_213_romaji[8];
extern char gRomajiKanaTbl_212_kana_2[8];
extern char gRomajiKanaTbl_212_kana_1[8];
extern char gRomajiKanaTbl_212_kana_0[8];
extern char gRomajiKanaTbl_212_romaji[8];
extern char gRomajiKanaTbl_211_kana_2[8];
extern char gRomajiKanaTbl_211_kana_1[8];
extern char gRomajiKanaTbl_211_kana_0[8];
extern char gRomajiKanaTbl_211_romaji[8];
extern char gRomajiKanaTbl_210_kana_2[8];
extern char gRomajiKanaTbl_210_kana_1[8];
extern char gRomajiKanaTbl_210_kana_0[8];
extern char gRomajiKanaTbl_210_romaji[8];
extern char gRomajiKanaTbl_209_kana_2[8];
extern char gRomajiKanaTbl_209_kana_1[8];
extern char gRomajiKanaTbl_209_kana_0[8];
extern char gRomajiKanaTbl_209_romaji[8];
extern char gRomajiKanaTbl_208_kana_2[8];
extern char gRomajiKanaTbl_208_kana_1[8];
extern char gRomajiKanaTbl_208_kana_0[8];
extern char gRomajiKanaTbl_208_romaji[8];
extern char gRomajiKanaTbl_207_kana_2[8];
extern char gRomajiKanaTbl_207_kana_1[8];
extern char gRomajiKanaTbl_207_kana_0[8];
extern char gRomajiKanaTbl_207_romaji[8];
extern char gRomajiKanaTbl_206_kana_2[8];
extern char gRomajiKanaTbl_206_kana_1[8];
extern char gRomajiKanaTbl_206_kana_0[8];
extern char gRomajiKanaTbl_206_romaji[8];
extern char gRomajiKanaTbl_205_kana_2[8];
extern char gRomajiKanaTbl_205_kana_1[8];
extern char gRomajiKanaTbl_205_kana_0[8];
extern char gRomajiKanaTbl_205_romaji[8];
extern char gRomajiKanaTbl_204_kana_2[8];
extern char gRomajiKanaTbl_204_kana_1[8];
extern char gRomajiKanaTbl_204_kana_0[8];
extern char gRomajiKanaTbl_204_romaji[8];
extern char gRomajiKanaTbl_203_kana_2[8];
extern char gRomajiKanaTbl_203_kana_1[8];
extern char gRomajiKanaTbl_203_kana_0[8];
extern char gRomajiKanaTbl_203_romaji[8];
extern char gRomajiKanaTbl_202_kana_2[8];
extern char gRomajiKanaTbl_202_kana_1[8];
extern char gRomajiKanaTbl_202_kana_0[8];
extern char gRomajiKanaTbl_202_romaji[8];
extern char gRomajiKanaTbl_201_kana_2[8];
extern char gRomajiKanaTbl_201_kana_1[8];
extern char gRomajiKanaTbl_201_kana_0[8];
extern char gRomajiKanaTbl_201_romaji[8];
extern char gRomajiKanaTbl_200_kana_2[8];
extern char gRomajiKanaTbl_200_kana_1[8];
extern char gRomajiKanaTbl_200_kana_0[8];
extern char gRomajiKanaTbl_200_romaji[8];
extern char gRomajiKanaTbl_199_kana_2[8];
extern char gRomajiKanaTbl_199_kana_1[8];
extern char gRomajiKanaTbl_199_kana_0[8];
extern char gRomajiKanaTbl_199_romaji[8];
extern char gRomajiKanaTbl_198_kana_2[8];
extern char gRomajiKanaTbl_198_kana_1[8];
extern char gRomajiKanaTbl_198_kana_0[8];
extern char gRomajiKanaTbl_198_romaji[8];
extern char gRomajiKanaTbl_197_romaji[8];
extern char gRomajiKanaTbl_196_romaji[8];
extern char gRomajiKanaTbl_195_kana_2[8];
extern char gRomajiKanaTbl_195_kana_1[8];
extern char gRomajiKanaTbl_195_kana_0[8];
extern char gRomajiKanaTbl_195_romaji[8];
extern char gRomajiKanaTbl_194_romaji[8];
extern char gRomajiKanaTbl_193_kana_2[8];
extern char gRomajiKanaTbl_193_kana_1[8];
extern char gRomajiKanaTbl_193_kana_0[8];
extern char gRomajiKanaTbl_193_romaji[8];
extern char D_0046BCA8[8];
extern char D_0046BCA0[8];
extern char D_0046BC98[8];
extern char gRomajiKanaTbl_192_romaji[8];
extern char D_0046BC88[8];
extern char D_0046BC80[8];
extern char D_0046BC78[8];
extern char gRomajiKanaTbl_191_romaji[8];
extern char gRomajiKanaTbl_190_kana_2[8];
extern char D_0046BC60[8];
extern char gRomajiKanaTbl_190_romaji[8];
extern char D_0046BC50[8];
extern char D_0046BC48[8];
extern char D_0046BC40[8];
extern char gRomajiKanaTbl_189_romaji[8];
extern char gRomajiKanaTbl_188_kana_2[8];
extern char gRomajiKanaTbl_188_kana_1[8];
extern char gRomajiKanaTbl_188_kana_0[8];
extern char gRomajiKanaTbl_188_romaji[8];
extern char gRomajiKanaTbl_187_kana_2[8];
extern char gRomajiKanaTbl_187_kana_1[8];
extern char gRomajiKanaTbl_187_kana_0[8];
extern char gRomajiKanaTbl_187_romaji[8];
extern char gRomajiKanaTbl_186_kana_2[8];
extern char gRomajiKanaTbl_186_kana_1[8];
extern char gRomajiKanaTbl_186_kana_0[8];
extern char gRomajiKanaTbl_186_romaji[8];
extern char gRomajiKanaTbl_185_kana_2[8];
extern char gRomajiKanaTbl_185_kana_1[8];
extern char gRomajiKanaTbl_185_kana_0[8];
extern char gRomajiKanaTbl_185_romaji[8];
extern char gRomajiKanaTbl_184_kana_2[8];
extern char gRomajiKanaTbl_184_kana_1[8];
extern char gRomajiKanaTbl_184_kana_0[8];
extern char gRomajiKanaTbl_184_romaji[8];
extern char gRomajiKanaTbl_183_kana_2[8];
extern char gRomajiKanaTbl_183_kana_1[8];
extern char gRomajiKanaTbl_183_kana_0[8];
extern char gRomajiKanaTbl_183_romaji[8];
extern char gRomajiKanaTbl_182_romaji[8];
extern char gRomajiKanaTbl_181_romaji[8];
extern char gRomajiKanaTbl_180_kana_2[8];
extern char gRomajiKanaTbl_180_kana_1[8];
extern char gRomajiKanaTbl_180_kana_0[8];
extern char gRomajiKanaTbl_180_romaji[8];
extern char D_0046BB40[8];
extern char D_0046BB38[8];
extern char D_0046BB30[8];
extern char gRomajiKanaTbl_179_romaji[8];
extern char D_0046BB20[8];
extern char D_0046BB18[8];
extern char D_0046BB10[8];
extern char gRomajiKanaTbl_178_romaji[8];
extern char gRomajiKanaTbl_177_kana_2[8];
extern char gRomajiKanaTbl_177_kana_1[8];
extern char gRomajiKanaTbl_177_kana_0[8];
extern char gRomajiKanaTbl_177_romaji[8];
extern char gRomajiKanaTbl_176_kana_2[8];
extern char gRomajiKanaTbl_176_kana_1[8];
extern char gRomajiKanaTbl_176_kana_0[8];
extern char gRomajiKanaTbl_176_romaji[8];
extern char gRomajiKanaTbl_175_kana_2[8];
extern char gRomajiKanaTbl_175_kana_1[8];
extern char gRomajiKanaTbl_175_kana_0[8];
extern char gRomajiKanaTbl_175_romaji[8];
extern char gRomajiKanaTbl_174_kana_2[8];
extern char gRomajiKanaTbl_174_kana_1[8];
extern char gRomajiKanaTbl_174_kana_0[8];
extern char gRomajiKanaTbl_174_romaji[8];
extern char gRomajiKanaTbl_173_kana_2[8];
extern char gRomajiKanaTbl_173_kana_1[8];
extern char gRomajiKanaTbl_173_kana_0[8];
extern char gRomajiKanaTbl_173_romaji[8];
extern char gRomajiKanaTbl_172_kana_2[8];
extern char gRomajiKanaTbl_172_kana_1[8];
extern char gRomajiKanaTbl_172_kana_0[8];
extern char gRomajiKanaTbl_172_romaji[8];
extern char gRomajiKanaTbl_171_kana_2[8];
extern char gRomajiKanaTbl_171_kana_1[8];
extern char gRomajiKanaTbl_171_kana_0[8];
extern char gRomajiKanaTbl_171_romaji[8];
extern char gRomajiKanaTbl_170_kana_2[8];
extern char gRomajiKanaTbl_170_kana_1[8];
extern char gRomajiKanaTbl_170_kana_0[8];
extern char gRomajiKanaTbl_170_romaji[8];
extern char gRomajiKanaTbl_169_kana_2[8];
extern char gRomajiKanaTbl_169_kana_1[8];
extern char gRomajiKanaTbl_169_kana_0[8];
extern char gRomajiKanaTbl_169_romaji[8];
extern char gRomajiKanaTbl_168_kana_2[8];
extern char gRomajiKanaTbl_168_kana_1[8];
extern char gRomajiKanaTbl_168_kana_0[8];
extern char gRomajiKanaTbl_168_romaji[8];
extern char gRomajiKanaTbl_167_kana_2[8];
extern char gRomajiKanaTbl_167_kana_1[8];
extern char gRomajiKanaTbl_167_kana_0[8];
extern char gRomajiKanaTbl_167_romaji[8];
extern char gRomajiKanaTbl_166_kana_2[8];
extern char gRomajiKanaTbl_166_kana_1[8];
extern char gRomajiKanaTbl_166_kana_0[8];
extern char gRomajiKanaTbl_166_romaji[8];
extern char gRomajiKanaTbl_165_kana_2[8];
extern char gRomajiKanaTbl_165_kana_1[8];
extern char gRomajiKanaTbl_165_kana_0[8];
extern char gRomajiKanaTbl_165_romaji[8];
extern char gRomajiKanaTbl_164_kana_2[8];
extern char gRomajiKanaTbl_164_kana_1[8];
extern char gRomajiKanaTbl_164_kana_0[8];
extern char gRomajiKanaTbl_164_romaji[8];
extern char gRomajiKanaTbl_163_kana_2[8];
extern char gRomajiKanaTbl_163_kana_1[8];
extern char gRomajiKanaTbl_163_kana_0[8];
extern char gRomajiKanaTbl_163_romaji[8];
extern char gRomajiKanaTbl_162_kana_2[8];
extern char gRomajiKanaTbl_162_kana_1[8];
extern char gRomajiKanaTbl_162_kana_0[8];
extern char gRomajiKanaTbl_162_romaji[8];
extern char gRomajiKanaTbl_161_kana_2[8];
extern char gRomajiKanaTbl_161_kana_1[8];
extern char gRomajiKanaTbl_161_kana_0[8];
extern char gRomajiKanaTbl_161_romaji[8];
extern char gRomajiKanaTbl_160_kana_2[8];
extern char gRomajiKanaTbl_160_kana_1[8];
extern char gRomajiKanaTbl_160_kana_0[8];
extern char gRomajiKanaTbl_160_romaji[8];
extern char gRomajiKanaTbl_159_kana_2[8];
extern char gRomajiKanaTbl_159_kana_1[8];
extern char gRomajiKanaTbl_159_kana_0[8];
extern char gRomajiKanaTbl_159_romaji[8];
extern char gRomajiKanaTbl_158_romaji[8];
extern char gRomajiKanaTbl_157_kana_2[8];
extern char gRomajiKanaTbl_157_kana_1[8];
extern char gRomajiKanaTbl_157_kana_0[8];
extern char gRomajiKanaTbl_157_romaji[8];
extern char gRomajiKanaTbl_156_kana_2[8];
extern char gRomajiKanaTbl_156_kana_1[8];
extern char gRomajiKanaTbl_156_kana_0[8];
extern char gRomajiKanaTbl_156_romaji[8];
extern char D_0046B858[8];
extern char D_0046B850[8];
extern char D_0046B848[8];
extern char gRomajiKanaTbl_155_romaji[8];
extern char gRomajiKanaTbl_154_kana_2[8];
extern char gRomajiKanaTbl_154_kana_1[8];
extern char gRomajiKanaTbl_154_kana_0[8];
extern char gRomajiKanaTbl_154_romaji[8];
extern char gRomajiKanaTbl_153_kana_2[8];
extern char gRomajiKanaTbl_153_kana_1[8];
extern char gRomajiKanaTbl_153_kana_0[8];
extern char gRomajiKanaTbl_153_romaji[8];
extern char gRomajiKanaTbl_152_kana_2[8];
extern char gRomajiKanaTbl_152_kana_1[8];
extern char gRomajiKanaTbl_152_kana_0[8];
extern char gRomajiKanaTbl_152_romaji[8];
extern char gRomajiKanaTbl_151_kana_2[8];
extern char gRomajiKanaTbl_151_kana_1[8];
extern char gRomajiKanaTbl_151_kana_0[8];
extern char gRomajiKanaTbl_151_romaji[8];
extern char gRomajiKanaTbl_150_kana_2[8];
extern char gRomajiKanaTbl_150_kana_1[8];
extern char gRomajiKanaTbl_150_kana_0[8];
extern char gRomajiKanaTbl_150_romaji[8];
extern char gRomajiKanaTbl_149_kana_2[8];
extern char gRomajiKanaTbl_149_kana_1[8];
extern char gRomajiKanaTbl_149_kana_0[8];
extern char gRomajiKanaTbl_149_romaji[8];
extern char gRomajiKanaTbl_148_kana_2[8];
extern char gRomajiKanaTbl_148_kana_1[8];
extern char gRomajiKanaTbl_148_kana_0[8];
extern char gRomajiKanaTbl_148_romaji[8];
extern char gRomajiKanaTbl_147_kana_2[8];
extern char gRomajiKanaTbl_147_kana_1[8];
extern char gRomajiKanaTbl_147_kana_0[8];
extern char gRomajiKanaTbl_147_romaji[8];
extern char gRomajiKanaTbl_146_kana_2[8];
extern char gRomajiKanaTbl_146_kana_1[8];
extern char gRomajiKanaTbl_146_kana_0[8];
extern char gRomajiKanaTbl_146_romaji[8];
extern char gRomajiKanaTbl_145_kana_2[8];
extern char gRomajiKanaTbl_145_kana_1[8];
extern char gRomajiKanaTbl_145_kana_0[8];
extern char gRomajiKanaTbl_145_romaji[8];
extern char gRomajiKanaTbl_144_kana_2[8];
extern char gRomajiKanaTbl_144_kana_1[8];
extern char gRomajiKanaTbl_144_kana_0[8];
extern char gRomajiKanaTbl_144_romaji[8];
extern char gRomajiKanaTbl_143_kana_2[8];
extern char gRomajiKanaTbl_143_kana_1[8];
extern char gRomajiKanaTbl_143_kana_0[8];
extern char gRomajiKanaTbl_143_romaji[8];
extern char gRomajiKanaTbl_142_kana_2[8];
extern char gRomajiKanaTbl_142_kana_1[8];
extern char gRomajiKanaTbl_142_kana_0[8];
extern char gRomajiKanaTbl_142_romaji[8];
extern char gRomajiKanaTbl_141_kana_2[8];
extern char gRomajiKanaTbl_141_kana_1[8];
extern char gRomajiKanaTbl_141_kana_0[8];
extern char gRomajiKanaTbl_141_romaji[8];
extern char gRomajiKanaTbl_140_kana_2[8];
extern char gRomajiKanaTbl_140_kana_1[8];
extern char gRomajiKanaTbl_140_kana_0[8];
extern char gRomajiKanaTbl_140_romaji[8];
extern char gRomajiKanaTbl_139_kana_2[8];
extern char gRomajiKanaTbl_139_kana_1[8];
extern char gRomajiKanaTbl_139_kana_0[8];
extern char gRomajiKanaTbl_139_romaji[8];
extern char gRomajiKanaTbl_138_kana_2[8];
extern char gRomajiKanaTbl_138_kana_1[8];
extern char gRomajiKanaTbl_138_kana_0[8];
extern char gRomajiKanaTbl_138_romaji[8];
extern char gRomajiKanaTbl_137_kana_2[8];
extern char gRomajiKanaTbl_137_kana_1[8];
extern char gRomajiKanaTbl_137_kana_0[8];
extern char gRomajiKanaTbl_137_romaji[8];
extern char gRomajiKanaTbl_136_kana_2[8];
extern char gRomajiKanaTbl_136_kana_1[8];
extern char gRomajiKanaTbl_136_kana_0[8];
extern char gRomajiKanaTbl_136_romaji[8];
extern char gRomajiKanaTbl_135_kana_2[8];
extern char gRomajiKanaTbl_135_kana_1[8];
extern char gRomajiKanaTbl_135_kana_0[8];
extern char gRomajiKanaTbl_135_romaji[8];
extern char gRomajiKanaTbl_134_kana_2[8];
extern char gRomajiKanaTbl_134_kana_1[8];
extern char gRomajiKanaTbl_134_kana_0[8];
extern char gRomajiKanaTbl_134_romaji[8];
extern char gRomajiKanaTbl_133_kana_2[8];
extern char gRomajiKanaTbl_133_kana_1[8];
extern char gRomajiKanaTbl_133_kana_0[8];
extern char gRomajiKanaTbl_133_romaji[8];
extern char gRomajiKanaTbl_132_kana_2[8];
extern char gRomajiKanaTbl_132_kana_1[8];
extern char gRomajiKanaTbl_132_kana_0[8];
extern char gRomajiKanaTbl_132_romaji[8];
extern char gRomajiKanaTbl_131_kana_2[8];
extern char gRomajiKanaTbl_131_kana_1[8];
extern char gRomajiKanaTbl_131_kana_0[8];
extern char gRomajiKanaTbl_131_romaji[8];
extern char gRomajiKanaTbl_130_kana_2[8];
extern char gRomajiKanaTbl_130_kana_1[8];
extern char gRomajiKanaTbl_130_kana_0[8];
extern char gRomajiKanaTbl_130_romaji[8];
extern char gRomajiKanaTbl_129_kana_2[8];
extern char gRomajiKanaTbl_129_kana_1[8];
extern char gRomajiKanaTbl_129_kana_0[8];
extern char gRomajiKanaTbl_129_romaji[8];
extern char gRomajiKanaTbl_128_kana_2[8];
extern char gRomajiKanaTbl_128_kana_1[8];
extern char gRomajiKanaTbl_128_kana_0[8];
extern char gRomajiKanaTbl_128_romaji[8];
extern char gRomajiKanaTbl_127_kana_2[8];
extern char gRomajiKanaTbl_127_kana_1[8];
extern char gRomajiKanaTbl_127_kana_0[8];
extern char gRomajiKanaTbl_127_romaji[8];
extern char gRomajiKanaTbl_126_kana_2[8];
extern char gRomajiKanaTbl_126_kana_1[8];
extern char gRomajiKanaTbl_126_kana_0[8];
extern char gRomajiKanaTbl_126_romaji[8];
extern char gRomajiKanaTbl_125_kana_2[8];
extern char gRomajiKanaTbl_125_kana_1[8];
extern char gRomajiKanaTbl_125_kana_0[8];
extern char gRomajiKanaTbl_125_romaji[8];
extern char gRomajiKanaTbl_124_kana_2[8];
extern char gRomajiKanaTbl_124_kana_1[8];
extern char gRomajiKanaTbl_124_kana_0[8];
extern char gRomajiKanaTbl_124_romaji[8];
extern char gRomajiKanaTbl_123_romaji[8];
extern char gRomajiKanaTbl_122_romaji[8];
extern char gRomajiKanaTbl_121_romaji[8];
extern char gRomajiKanaTbl_120_romaji[8];
extern char gRomajiKanaTbl_119_romaji[8];
extern char gRomajiKanaTbl_118_romaji[8];
extern char gRomajiKanaTbl_117_romaji[8];
extern char gRomajiKanaTbl_116_romaji[8];
extern char gRomajiKanaTbl_115_romaji[8];
extern char D_0046B410[8];
extern char D_0046B408[8];
extern char D_0046B400[8];
extern char gRomajiKanaTbl_114_romaji[8];
extern char D_0046B3F0[8];
extern char D_0046B3E8[8];
extern char D_0046B3E0[8];
extern char gRomajiKanaTbl_113_romaji[8];
extern char D_0046B3D0[8];
extern char D_0046B3C8[8];
extern char D_0046B3C0[8];
extern char gRomajiKanaTbl_112_romaji[8];
extern char D_0046B3B0[8];
extern char D_0046B3A8[8];
extern char D_0046B3A0[8];
extern char gRomajiKanaTbl_111_romaji[8];
extern char D_0046B390[8];
extern char D_0046B388[8];
extern char D_0046B380[8];
extern char gRomajiKanaTbl_110_romaji[8];
extern char gRomajiKanaTbl_109_kana_2[8];
extern char gRomajiKanaTbl_109_kana_1[8];
extern char gRomajiKanaTbl_109_kana_0[8];
extern char gRomajiKanaTbl_109_romaji[8];
extern char gRomajiKanaTbl_108_kana_2[8];
extern char gRomajiKanaTbl_108_kana_1[8];
extern char gRomajiKanaTbl_108_kana_0[8];
extern char gRomajiKanaTbl_108_romaji[8];
extern char gRomajiKanaTbl_107_kana_2[8];
extern char gRomajiKanaTbl_107_kana_1[8];
extern char gRomajiKanaTbl_107_kana_0[8];
extern char gRomajiKanaTbl_107_romaji[8];
extern char gRomajiKanaTbl_106_kana_2[8];
extern char gRomajiKanaTbl_106_kana_1[8];
extern char gRomajiKanaTbl_106_kana_0[8];
extern char gRomajiKanaTbl_106_romaji[8];
extern char gRomajiKanaTbl_105_kana_2[8];
extern char gRomajiKanaTbl_105_kana_1[8];
extern char gRomajiKanaTbl_105_kana_0[8];
extern char gRomajiKanaTbl_105_romaji[8];
extern char gRomajiKanaTbl_104_romaji[8];
extern char gRomajiKanaTbl_103_romaji[8];
extern char gRomajiKanaTbl_102_romaji[8];
extern char gRomajiKanaTbl_101_romaji[8];
extern char gRomajiKanaTbl_100_kana_2[8];
extern char gRomajiKanaTbl_100_kana_1[8];
extern char gRomajiKanaTbl_100_kana_0[8];
extern char gRomajiKanaTbl_100_romaji[8];
extern char gRomajiKanaTbl_99_kana_2[8];
extern char gRomajiKanaTbl_99_kana_1[8];
extern char gRomajiKanaTbl_99_kana_0[8];
extern char gRomajiKanaTbl_99_romaji[8];
extern char D_0046B258[8];
extern char D_0046B250[8];
extern char D_0046B248[8];
extern char gRomajiKanaTbl_98_romaji[8];
extern char D_0046B238[8];
extern char D_0046B230[8];
extern char D_0046B228[8];
extern char gRomajiKanaTbl_97_romaji[8];
extern char gRomajiKanaTbl_96_kana_2[8];
extern char gRomajiKanaTbl_96_kana_1[8];
extern char gRomajiKanaTbl_96_kana_0[8];
extern char gRomajiKanaTbl_96_romaji[8];
extern char gRomajiKanaTbl_95_romaji[8];
extern char gRomajiKanaTbl_94_romaji[8];
extern char gRomajiKanaTbl_93_romaji[8];
extern char gRomajiKanaTbl_92_romaji[8];
extern char gRomajiKanaTbl_91_romaji[8];
extern char gRomajiKanaTbl_90_romaji[8];
extern char gRomajiKanaTbl_89_romaji[8];
extern char gRomajiKanaTbl_88_romaji[8];
extern char gRomajiKanaTbl_87_romaji[8];
extern char D_0046B1B0[8];
extern char D_0046B1A8[8];
extern char D_0046B1A0[8];
extern char gRomajiKanaTbl_86_romaji[8];
extern char D_0046B190[8];
extern char D_0046B188[8];
extern char D_0046B180[8];
extern char gRomajiKanaTbl_85_romaji[8];
extern char D_0046B170[8];
extern char D_0046B168[8];
extern char D_0046B160[8];
extern char gRomajiKanaTbl_84_romaji[8];
extern char D_0046B150[8];
extern char D_0046B148[8];
extern char D_0046B140[8];
extern char gRomajiKanaTbl_83_romaji[8];
extern char D_0046B130[8];
extern char D_0046B128[8];
extern char D_0046B120[8];
extern char gRomajiKanaTbl_82_romaji[8];
extern char gRomajiKanaTbl_81_romaji[8];
extern char gRomajiKanaTbl_80_romaji[8];
extern char gRomajiKanaTbl_79_romaji[8];
extern char gRomajiKanaTbl_78_romaji[8];
extern char D_0046B0F0[8];
extern char D_0046B0E8[8];
extern char D_0046B0E0[8];
extern char gRomajiKanaTbl_77_romaji[8];
extern char D_0046B0D0[8];
extern char D_0046B0C8[8];
extern char D_0046B0C0[8];
extern char gRomajiKanaTbl_76_romaji[8];
extern char D_0046B0B0[8];
extern char D_0046B0A8[8];
extern char D_0046B0A0[8];
extern char gRomajiKanaTbl_75_romaji[8];
extern char gRomajiKanaTbl_74_kana_2[8];
extern char gRomajiKanaTbl_74_kana_1[8];
extern char gRomajiKanaTbl_74_kana_0[8];
extern char gRomajiKanaTbl_74_romaji[8];
extern char D_0046B070[8];
extern char D_0046B068[8];
extern char D_0046B060[8];
extern char gRomajiKanaTbl_73_romaji[8];
extern char gRomajiKanaTbl_72_romaji[8];
extern char gRomajiKanaTbl_71_romaji[8];
extern char gRomajiKanaTbl_70_kana_2[8];
extern char gRomajiKanaTbl_70_kana_1[8];
extern char gRomajiKanaTbl_70_kana_0[8];
extern char gRomajiKanaTbl_70_romaji[8];
extern char gRomajiKanaTbl_69_kana_2[8];
extern char gRomajiKanaTbl_69_kana_1[8];
extern char gRomajiKanaTbl_69_kana_0[8];
extern char gRomajiKanaTbl_69_romaji[8];
extern char gRomajiKanaTbl_68_kana_2[8];
extern char gRomajiKanaTbl_68_kana_1[8];
extern char gRomajiKanaTbl_68_kana_0[8];
extern char gRomajiKanaTbl_68_romaji[8];
extern char D_0046AFE0[8];
extern char D_0046AFD8[8];
extern char D_0046AFD0[8];
extern char gRomajiKanaTbl_67_romaji[8];
extern char gRomajiKanaTbl_66_kana_2[8];
extern char gRomajiKanaTbl_66_kana_1[8];
extern char gRomajiKanaTbl_66_kana_0[8];
extern char gRomajiKanaTbl_66_romaji[8];
extern char gRomajiKanaTbl_65_romaji[8];
extern char gRomajiKanaTbl_64_romaji[8];
extern char gRomajiKanaTbl_63_kana_2[8];
extern char gRomajiKanaTbl_63_kana_1[8];
extern char gRomajiKanaTbl_63_kana_0[8];
extern char gRomajiKanaTbl_63_romaji[8];
extern char D_0046AF70[8];
extern char D_0046AF68[8];
extern char D_0046AF60[8];
extern char gRomajiKanaTbl_62_romaji[8];
extern char gRomajiKanaTbl_61_kana_2[8];
extern char gRomajiKanaTbl_61_kana_1[8];
extern char gRomajiKanaTbl_61_kana_0[8];
extern char gRomajiKanaTbl_61_romaji[8];
extern char D_0046AF30[8];
extern char D_0046AF28[8];
extern char D_0046AF20[8];
extern char gRomajiKanaTbl_60_romaji[8];
extern char gRomajiKanaTbl_59_kana_2[8];
extern char gRomajiKanaTbl_59_kana_1[8];
extern char gRomajiKanaTbl_59_kana_0[8];
extern char gRomajiKanaTbl_59_romaji[8];
extern char gRomajiKanaTbl_58_kana_2[8];
extern char gRomajiKanaTbl_58_kana_1[8];
extern char gRomajiKanaTbl_58_kana_0[8];
extern char gRomajiKanaTbl_58_romaji[8];
extern char gRomajiKanaTbl_57_kana_2[8];
extern char gRomajiKanaTbl_57_kana_1[8];
extern char gRomajiKanaTbl_57_kana_0[8];
extern char gRomajiKanaTbl_57_romaji[8];
extern char gRomajiKanaTbl_56_kana_2[8];
extern char gRomajiKanaTbl_56_kana_1[8];
extern char gRomajiKanaTbl_56_kana_0[8];
extern char gRomajiKanaTbl_56_romaji[8];
extern char gRomajiKanaTbl_55_kana_2[8];
extern char gRomajiKanaTbl_55_kana_1[8];
extern char gRomajiKanaTbl_55_kana_0[8];
extern char gRomajiKanaTbl_55_romaji[8];
extern char gRomajiKanaTbl_54_kana_2[8];
extern char gRomajiKanaTbl_54_kana_1[8];
extern char gRomajiKanaTbl_54_kana_0[8];
extern char gRomajiKanaTbl_54_romaji[8];
extern char gRomajiKanaTbl_53_kana_2[8];
extern char gRomajiKanaTbl_53_kana_1[8];
extern char gRomajiKanaTbl_53_kana_0[8];
extern char gRomajiKanaTbl_53_romaji[8];
extern char gRomajiKanaTbl_52_kana_2[8];
extern char gRomajiKanaTbl_52_kana_1[8];
extern char gRomajiKanaTbl_52_kana_0[8];
extern char gRomajiKanaTbl_52_romaji[8];
extern char gRomajiKanaTbl_51_kana_2[8];
extern char gRomajiKanaTbl_51_kana_1[8];
extern char gRomajiKanaTbl_51_kana_0[8];
extern char gRomajiKanaTbl_51_romaji[8];
extern char gRomajiKanaTbl_50_kana_2[8];
extern char gRomajiKanaTbl_50_kana_1[8];
extern char gRomajiKanaTbl_50_kana_0[8];
extern char gRomajiKanaTbl_50_romaji[8];
extern char gRomajiKanaTbl_49_romaji[8];
extern char gRomajiKanaTbl_48_romaji[8];
extern char gRomajiKanaTbl_47_kana_2[8];
extern char gRomajiKanaTbl_47_kana_1[8];
extern char gRomajiKanaTbl_47_kana_0[8];
extern char gRomajiKanaTbl_47_romaji[8];
extern char D_0046ADA0[8];
extern char D_0046AD98[8];
extern char D_0046AD90[8];
extern char gRomajiKanaTbl_46_romaji[8];
extern char gRomajiKanaTbl_45_romaji[8];
extern char D_0046AD78[8];
extern char D_0046AD70[8];
extern char D_0046AD68[8];
extern char gRomajiKanaTbl_44_romaji[8];
extern char gRomajiKanaTbl_43_kana_2[8];
extern char gRomajiKanaTbl_43_kana_1[8];
extern char gRomajiKanaTbl_43_kana_0[8];
extern char gRomajiKanaTbl_43_romaji[8];
extern char gRomajiKanaTbl_42_kana_2[8];
extern char gRomajiKanaTbl_42_kana_1[8];
extern char gRomajiKanaTbl_42_kana_0[8];
extern char gRomajiKanaTbl_42_romaji[8];
extern char gRomajiKanaTbl_41_kana_2[8];
extern char gRomajiKanaTbl_41_kana_1[8];
extern char gRomajiKanaTbl_41_kana_0[8];
extern char gRomajiKanaTbl_41_romaji[8];
extern char gRomajiKanaTbl_40_kana_2[8];
extern char gRomajiKanaTbl_40_kana_1[8];
extern char gRomajiKanaTbl_40_kana_0[8];
extern char gRomajiKanaTbl_40_romaji[8];
extern char gRomajiKanaTbl_39_kana_2[8];
extern char gRomajiKanaTbl_39_kana_1[8];
extern char gRomajiKanaTbl_39_kana_0[8];
extern char gRomajiKanaTbl_39_romaji[8];
extern char gRomajiKanaTbl_38_kana_2[8];
extern char gRomajiKanaTbl_38_kana_1[8];
extern char gRomajiKanaTbl_38_kana_0[8];
extern char gRomajiKanaTbl_38_romaji[8];
extern char gRomajiKanaTbl_37_kana_2[8];
extern char gRomajiKanaTbl_37_kana_1[8];
extern char gRomajiKanaTbl_37_kana_0[8];
extern char gRomajiKanaTbl_37_romaji[8];
extern char gRomajiKanaTbl_36_kana_2[8];
extern char gRomajiKanaTbl_36_kana_1[8];
extern char gRomajiKanaTbl_36_kana_0[8];
extern char gRomajiKanaTbl_36_romaji[8];
extern char gRomajiKanaTbl_35_kana_2[8];
extern char gRomajiKanaTbl_35_kana_1[8];
extern char gRomajiKanaTbl_35_kana_0[8];
extern char gRomajiKanaTbl_35_romaji[8];
extern char gRomajiKanaTbl_34_kana_2[8];
extern char gRomajiKanaTbl_34_kana_1[8];
extern char gRomajiKanaTbl_34_kana_0[8];
extern char gRomajiKanaTbl_34_romaji[8];
extern char gRomajiKanaTbl_33_kana_2[8];
extern char gRomajiKanaTbl_33_kana_1[8];
extern char gRomajiKanaTbl_33_kana_0[8];
extern char gRomajiKanaTbl_33_romaji[8];
extern char gRomajiKanaTbl_32_kana_2[8];
extern char gRomajiKanaTbl_32_kana_1[8];
extern char gRomajiKanaTbl_32_kana_0[8];
extern char gRomajiKanaTbl_32_romaji[8];
extern char gRomajiKanaTbl_31_kana_2[8];
extern char gRomajiKanaTbl_31_kana_1[8];
extern char gRomajiKanaTbl_31_kana_0[8];
extern char gRomajiKanaTbl_31_romaji[8];
extern char gRomajiKanaTbl_30_kana_2[8];
extern char gRomajiKanaTbl_30_kana_1[8];
extern char gRomajiKanaTbl_30_kana_0[8];
extern char gRomajiKanaTbl_30_romaji[8];
extern char gRomajiKanaTbl_29_romaji[8];
extern char gRomajiKanaTbl_28_romaji[8];
extern char gRomajiKanaTbl_27_romaji[8];
extern char D_0046AB80[8];
extern char D_0046AB78[8];
extern char D_0046AB70[8];
extern char gRomajiKanaTbl_26_romaji[8];
extern char gRomajiKanaTbl_25_kana_2[8];
extern char gRomajiKanaTbl_25_kana_1[8];
extern char gRomajiKanaTbl_25_kana_0[8];
extern char gRomajiKanaTbl_25_romaji[8];
extern char D_0046AB40[8];
extern char D_0046AB38[8];
extern char D_0046AB30[8];
extern char gRomajiKanaTbl_24_romaji[8];
extern char gRomajiKanaTbl_23_kana_2[8];
extern char gRomajiKanaTbl_23_kana_1[8];
extern char gRomajiKanaTbl_23_kana_0[8];
extern char gRomajiKanaTbl_23_romaji[8];
extern char D_0046AB00[8];
extern char D_0046AAF8[8];
extern char D_0046AAF0[8];
extern char gRomajiKanaTbl_22_romaji[8];
extern char gRomajiKanaTbl_21_kana_2[8];
extern char gRomajiKanaTbl_21_kana_1[8];
extern char gRomajiKanaTbl_21_kana_0[8];
extern char gRomajiKanaTbl_21_romaji[8];
extern char gRomajiKanaTbl_20_romaji[8];
extern char gRomajiKanaTbl_19_romaji[8];
extern char gRomajiKanaTbl_18_romaji[8];
extern char gRomajiKanaTbl_17_romaji[8];
extern char gRomajiKanaTbl_16_romaji[8];
extern char gRomajiKanaTbl_15_romaji[8];
extern char gRomajiKanaTbl_14_romaji[8];
extern char gRomajiKanaTbl_13_romaji[8];
extern char gRomajiKanaTbl_12_romaji[8];
extern char D_0046AA78[8];
extern char D_0046AA70[8];
extern char D_0046AA68[8];
extern char gRomajiKanaTbl_11_romaji[8];
extern char D_0046AA58[8];
extern char D_0046AA50[8];
extern char D_0046AA48[8];
extern char gRomajiKanaTbl_10_romaji[8];
extern char D_0046AA38[8];
extern char D_0046AA30[8];
extern char D_0046AA28[8];
extern char gRomajiKanaTbl_9_romaji[8];
extern char D_0046AA18[8];
extern char D_0046AA10[8];
extern char D_0046AA08[8];
extern char gRomajiKanaTbl_8_romaji[8];
extern char D_0046A9F8[8];
extern char D_0046A9F0[8];
extern char D_0046A9E8[8];
extern char gRomajiKanaTbl_7_romaji[8];
extern char gRomajiKanaTbl_6_romaji[8];
extern char gRomajiKanaTbl_5_romaji[8];
extern char gRomajiKanaTbl_4_kana_2[8];
extern char gRomajiKanaTbl_4_kana_1[8];
extern char gRomajiKanaTbl_4_kana_0[8];
extern char gRomajiKanaTbl_4_romaji[8];
extern char gRomajiKanaTbl_3_kana_2[8];
extern char gRomajiKanaTbl_3_kana_1[8];
extern char gRomajiKanaTbl_3_kana_0[8];
extern char gRomajiKanaTbl_3_romaji[8];
extern char D_0046A988[8];
extern char D_0046A980[8];
extern char D_0046A978[8];
extern char gRomajiKanaTbl_2_romaji[8];
extern char D_0046A968[8];
extern char D_0046A960[8];
extern char D_0046A958[8];
extern char gRomajiKanaTbl_1_romaji[8];
extern char gRomajiKanaTbl_0_kana_2[8];
extern char gRomajiKanaTbl_0_kana_1[8];
extern char gRomajiKanaTbl_0_kana_0[8];
extern char gRomajiKanaTbl_0_romaji[8];
void Kbd_Update(void);
void Kbd_HandleKey(void);
void Kbd_TypeHidKey(void);
s32 Kbd_KanaInput(void);
void Kbd_Decide(void);
void Kbd_Escape(void);
void Kbd_Space(s32 fromPad);
void Kbd_Backspace(void);
void Kbd_DakutenKey(void);
void Kbd_HandakutenKey(void);
void Kbd_CapsLock(void);
void Kbd_FKey(u8 key);
void Kbd_FKeyRow(u8 key);
void Kbd_ClearText(void);
void Kbd_OpenPhrases1(void);
void Kbd_OpenPhrases2(void);
void Kbd_Confirm(void);
void Kbd_CycleSegmentForm(s32 useForm2);
void Kbd_ConvertForm2(void);
void Kbd_CursorHome(void);
void Kbd_Delete(void);
void Str_CutAt(char *str, s32 len);
void Kbd_CursorEnd(void);
void Kbd_MoveOffKey(s16 dir);
s32 Kbd_ArrowsOnPage(void);
void Kbd_CursorRight(void);
void Kbd_CursorLeft(void);
void Kbd_CursorUp(void);
void Kbd_CursorDown(void);
void Kbd_KanaKey(void);
void Kbd_HenkanKey(void);
void Kbd_MuhenkanKey(void);
void Kbd_CommitInput(void);
void Kbd_DeleteChar(void);
void Kbd_ZenkakuKey(void);
s32 Str_DropFront(char *str, s32 count);
s32 Kbd_RomajiToKana(s32 kind, char *typed);
s32 Kbd_ConvertRomaji(s32 kind);
s32 Kbd_CharAllowed(s8 *str);
void Kbd_CommitKana(void);
s32 Str_InsertClip(char *dst, char *src, s32 pos, s32 max);
s32 Kbd_KindIs8To10(void);
void Kbd_UpdateSymbolMode(void);
void Kbd_ConvertOrCommit(void);
s32 CaseCands_CopyFirst(s8 *dst, u8 count);
s32 CaseCands_First(char *dst);
s32 Wide_ToUpper(char *dst, char *src);
s32 Wide_ToLower(char *dst, char *src);
s32 Wide_Capitalize(char *dst, char *src);
void Kbd_ClearCands(void);
void Kbd_SkipSameCands(void);
s32 Kbd_CountCands(void);
void Kbd_StartConvert(void);
void Kbd_NextCandidate(void);
void Kbd_Convert(void);
void Kbd_CommitConversion(void);
void Kbd_SetCandLine(s32 i);
void Kbd_SetSymbolCand(s32 line, s32 first);
void Kbd_TakeSymbolCand(void);
void Kbd_ListCandidates(s32 first, s32 dir);
void Kbd_InitCandList(void);
void Kbd_PrevCand(void);
void Kbd_NextCand(void);
void Kbd_PageUp(void);
void Kbd_PageDown(void);
void Kbd_PrevSegment(void);
void Kbd_NextSegment(void);
extern char gSmallTsuHira[8];
extern char gSmallTsuKata[8];
extern char gSmallTsuHalf[8];

void Kbd_DrawCandidateBox(f32 x, s16 y);

#endif
