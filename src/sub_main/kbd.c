/*
 * The software keyboard's main unit.
 * - The keyboard (KbdEntry gKbdState, reached through gKbd; kbd.h): opening
 *   it for an entry kind (Kbd_Open, Kbd_SetLayout), its frame (Kbd_Run: the
 *   pad's buttons, the USB keyboard's keys, key repeat), the key sets and
 *   character modes (KBD_SET_*, KBD_MODE_*), typing kana and converting
 *   them (the converter's commands, ConvCmd_*), the phrase and gesture
 *   pages, drawing (Kbd_Draw) and the checks of what an entry kind
 *   accepts. The keyboard's other unit holds the rest: the USB keys, the
 *   text line and the conversion. Kbd_EnterChar and Kbd_CharIsSpecialKana share one
 *   full-width space; its "%s" is another copy than Kbd_DrawCandidateBox's.
 * The drawing (Kbd_DrawCursorBox to Kbd_Draw) lies among the key handlers,
 * and their tables interleave in the data: the key tables and page binds
 * that Kbd_BindStartKeys reads lie between Kbd_Draw's box sizes and
 * glyphs and its pulse and key-position tables, so the drawing can't
 * stand as a file of its own.
 * The data part holds the romaji to kana table (gRomajiKanaTbl): pointers
 * to the romaji and kana strings, declared by address below.
 */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "kbd.h"
#include "scene.h"
#include "syssave.h"
#include "player.h"
#include "camera.h"
#include "pad.h"
#include "sound.h"
#pragma optimization_level 4

#include "ee_stdarg.h"
#include "model.h"
#include "render_state.h"
#include "bgm.h"
#include "chat_box.h"
#include "hud.h"
#include "prim.h"
#include "shake.h"
#include "text.h"
#include "engine_math.h"
#include "font.h"
#include "libusbkb.h"
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap); /* vsnprintf */
void Font_Printf(const char *fmt, ...);
extern s32 gTextOff;
char *strcat(char *dst, char *src); /* strcat */
char *strcpy(char *dst, const char *src); /* strcpy */
s32 strlen(char *s); /* strlen */
void strncpy(char *dst, const char *src, u32 n);
s32 ConvCmd_Open(void);
extern u8 gKbdDakutenSjis[];
extern u8 gKbdDakutenSjis_1;
extern u8 gKbdHandakutenSjis[];
extern u8 gKbdHandakutenSjis_1;
s32 strncmp(const char *a, const char *b, u32 n);
extern u8 gKbdPhrasePage;
extern u8 gKbdSavedKanaInput;
extern u8 gKbdState;
extern s8 D_005AEAD7;
typedef struct BoxSize {
    f32 w;
    s16 h;
} BoxSize;
extern BoxSize gKbdBoxSizes[];
typedef struct KeyPair {
    KbdKeyPos *cursors;
    s8 *grid;
    KbdKey key;         /* the key that switches to this page (Kbd_BindStartKeys puts it at keys 4-6) */
} KeyPair;
extern s32 gKbdSettings;
s32 Kbd_ShiftPair(u8 i);
typedef struct Rect16 {
    s16 x, y, w, h;
    s32 color;
} Rect16;
struct PrimLine;

/* The size of the character that ends at byte pos of s (moving the cursor back). */
s32 Sjis_PrevCharSize(u8 *str, s32 pos)
{
    s32 i;
    s32 size = 0;
    s32 len = 0;

    while (*str != 0 && len < pos) {
        size = ((*str >= 128 && *str < 160) || (*str >= 224 && *str < 256)) ? 2 : 1;
        len += size;
        for (i = 0; i < size; i++) {
            str++;
        }
    }
    return size;
}

/* The size of the character at byte pos of s (0 for an empty s, -1 when pos is inside a character). */
s32 Sjis_CharSizeAt(char *str, s32 pos)
{
    u8 *scan;
    s32 i;
    u8 lead;
    s32 size;

    if (strlen(str) == 0) {
        return 0;
    }
    i = 0;
    scan = (u8 *)str;
    while (*scan != 0 && i < pos) {
        size = ((*scan >= 128 && *scan < 160) || (*scan >= 224 && *scan < 256)) ? 2 : 1;
        i += size;
        scan += size;
    }
    if (pos < i) {
        return -1;
    }
    lead = str[pos];
    return ((lead >= 128 && lead <= 159) || (lead >= 224 && lead < 256)) ? 2 : 1;
}

/* Starts the kana-kanji converter for a new entry (ConvCmd_Open). */
void Kbd_OpenConv(void)
{
    ConvCmd_Open();
}

/*
 * The dakuten key (with kana input on); nothing while converting. On the
 * hiragana, katakana and half-width katakana sets, and on the gestures page
 * typing kana (KBD_GESTURE_KANA), adds a dakuten to the kana before the
 * cursor (Kana_AddDakuten). If that kana takes none, or on another set, it
 * types the key under the cursor.
 */
void Kbd_KanaDakuten(void)
{
    u8 keySet;

    if (gKbd->converting == 0) {
        keySet = gKbd->keySet;
        if ((keySet > 1U) && (keySet != KBD_SET_HALF_KATA)) {
            if (gKbd->mode == KBD_MODE_GESTURE) {
                if (!(gKbd->gestureMode & KBD_GESTURE_KANA)) {
                    goto block_6;
                }
                goto block_7;
            }
block_6:
            Kbd_TypeKey(gKbd->key);
            return;
        }
block_7:
        if (Kana_AddDakuten() == 0) {
            Kbd_TypeKey(gKbd->key);
        }
    }
}

/*
 * The handakuten key: as Kbd_KanaDakuten, with a handakuten
 * (Kana_AddHandakuten).
 */
void Kbd_KanaHandakuten(void)
{
    u8 keySet;

    if (gKbd->converting == 0) {
        keySet = gKbd->keySet;
        if ((keySet > 1U) && (keySet != KBD_SET_HALF_KATA)) {
            if (gKbd->mode == KBD_MODE_GESTURE) {
                if (!(gKbd->gestureMode & KBD_GESTURE_KANA)) {
                    goto block_6;
                }
                goto block_7;
            }
block_6:
            Kbd_TypeKey(gKbd->key);
            return;
        }
block_7:
        if (Kana_AddHandakuten() == 0) {
            Kbd_TypeKey(gKbd->key);
        }
    }
}

typedef struct Tbl17 {
    s8 v[17];
} Tbl17;

extern Tbl17 gKbdSetIsWide;

/*
 * 1 when the current key set types two-byte (full-width) characters
 * (gKbdSetIsWide[keySet]), else 0. The game copies the table to the stack
 * first.
 */
s32 Kbd_SetIsWide(void)
{
    Tbl17 isWide = gKbdSetIsWide;

    return isWide.v[gKbd->keySet];
}

/*
 * The test Kbd_EnterGestures runs on the text and its cursor: a negative
 * result makes the gestures page type kana (KBD_GESTURE_KANA). It always
 * returns 0.
 */
s32 Kbd_CheckGestureText(char *text, u16 *pos)
{
    return 0;
}

/*
 * Asks the local player for gesture number gesture, counting from 0
 * (Player_RequestGesture counts from 1), and sets the HUD's cancel flag.
 * The text and its cursor are not used. Returns 0.
 */
s32 Kbd_DoGesture(char *text, u16 *pos, s32 gesture)
{
    Player_RequestGesture(((gesture & 0xFF) + 1) & 0xFF);
    Hud_SetCancel();
    return 0;
}

extern u8 gKbdSavedKind;
extern u8 gKbdSavedKeySet;

/*
 * Saves the entry kind and its key set for the next Kbd_SetLayout of the
 * same kind (on a submit, Kbd_Cancel and Kbd_Close). The phrase and gesture
 * pages are saved as hiragana. With Caps Lock on (USB_LED_CAPS_LOCK), an
 * alphabet set is saved as its other case (Kbd_ShiftPair), which
 * Kbd_SetLayout swaps back.
 */
void Kbd_SaveKeySet(void)
{
    gKbdSavedKind = gKbd->kind;
    if (gKbdSettings & USB_LED_CAPS_LOCK) {
        switch (gKbd->keySet) {
        case KBD_SET_WIDE_UPPER:
        case KBD_SET_ASCII_UPPER:
        case KBD_SET_ASCII_LOWER:
        case KBD_SET_WIDE_LOWER:
            if ((s8)Kbd_ShiftPair(gKbd->keySet) >= 0) {
                gKbdSavedKeySet = Kbd_ShiftPair(gKbd->keySet);
            } else {
                gKbdSavedKeySet = gKbd->keySet;
            }
            break;
        case KBD_SET_PHRASE1:
        case KBD_SET_PHRASE2:
        case KBD_SET_GESTURE:
            gKbdSavedKeySet = KBD_SET_HIRA;
            break;
        default:
            gKbdSavedKeySet = gKbd->keySet;
            break;
        }
    } else {
        switch (gKbd->keySet) {
        case KBD_SET_PHRASE1:
        case KBD_SET_PHRASE2:
        case KBD_SET_GESTURE:
            gKbdSavedKeySet = KBD_SET_HIRA;
            break;
        default:
            gKbdSavedKeySet = gKbd->keySet;
            break;
        }
    }
}

void Kbd_SetPhrasePage(u8 page)
{
    gKbdPhrasePage = page;
}

extern s8 D_00418743[];

/*
 * Looks up the key under the cursor (col 0..20, row 0..3; the layout's grid
 * index is col * 4 + row): sets keyIdx and points keyPos and key at the
 * key's entries. On the phrase pages keyInfo becomes the key's phrase entry
 * (page 2's entries follow page 1's 12); it is NULL when the layout has no
 * list. Returns the key index.
 */
s8 Kbd_LookupKey(void)
{
    u8 *list;
    s32 idx;

    /* the grid index is col * 4 + row; the (long) cast is kept for the match (register allocation) */
    idx = (MWCC_LONG)(gKbd->col * 4);
    idx += gKbd->row;
    gKbd->keyIdx = gKbd->layout->grid[idx];
    gKbd->keyPos = gKbd->layout->cursors + gKbd->keyIdx;
    gKbd->key = gKbd->layout->keys + gKbd->keyIdx;
    list = gKbd->layout->extra;
    if (list != NULL) {
        switch (gKbd->keySet) {
        case KBD_SET_PHRASE1:
            gKbd->keyInfo = list + D_00418743[gKbd->keyIdx] * 16;
            break;
        case KBD_SET_PHRASE2:
            gKbd->keyInfo = list + (D_00418743[gKbd->keyIdx] + 12) * 16;
            break;
        }
    } else {
        gKbd->keyInfo = NULL;
    }
    return gKbd->keyIdx;
}

/*
 * Sets the keyboard up once at start-up: points gKbd at gKbdState, clears
 * polled and the saved kana-input toggle, and clears unk34.
 */
void Kbd_InitState(s8 language)
{
    gKbd = (KbdEntry *)&gKbdState;
    gKbd->polled = 0;
    gKbdSavedKanaInput = 0;
    Kbd_ClearUnk34(language);
    Kbd_EmptyInit();
}

/* Clears unk34; both branches store 0, language only picks the branch. */
void Kbd_ClearUnk34(s8 language)
{
    if (language == 0) {
        gKbd->unk34 = language;
    } else {
        gKbd->unk34 = 0;
    }
}

/*
 * 1 when byte is a one-byte Shift-JIS character, 0 when it is the first
 * byte of a two-byte one (0x80..0x9F or 0xE0..0xFF).
 */
static s32 Sjis_IsSingleByte(u8 byte)
{
    s32 code;

    code = byte & 0xFF;
    if ((code >= 128) && (code < 160)) {
        return 0;
    }
    if ((code >= 224) && (code < 256)) {
        return 0;
    }
    return 1;
}

/* Kbd_Open: opens the software keyboard: mode kind (the gKbdKindConfs entry), layout,
 * at most max bytes, starting from text (none: empty). */
extern KbdKindConf gKbdKindConfs[];
extern u8 gKbdDelay;
extern u8 gKbdReplayKey;
/* Called once with an argument it does not take (see Portability in the h1 notes). */
extern KbdLayout gKbdSetLayouts[20];
extern u8 gKbdKindIs0;
extern u8 gKbdOpenKey;
extern s8 gKbdPrompt[];
extern s16 gKbdChatTarget;

void *memset(void *dst, s32 c, u32 n);
/*
 * Opens the software keyboard.
 * - kind picks the colours and the phrase and gesture lists
 *   (gKbdKindConfs[kind]); gKbdKindIs0 records kind 0, whose R2 cycle
 *   reaches the gestures page.
 * - layout is the entry kind (Kbd_SetLayout): the barred sets, the starting
 *   set and the characters accepted.
 * - max is the text's maximum length (at most 255); text is the starting
 *   text (NULL: empty), cut so that no two-byte character is split.
 * A USB key held at the open, other than the Enter keys, Escape and
 * Backspace, is replayed as a new key once the one-frame delay ends
 * (gKbdReplayKey, Kbd_Run). Entry kinds 1 to 5 take the pad at once unless
 * Enter opened them; the others when Kbd_SetStartActive allowed it. Also
 * sets the USB keyboard's key repeat, restores the kana-input toggle and
 * clears the chat prompt and target.
 */
void Kbd_Open(s32 kind, u8 layout, u16 max, char *text)
{
    u8 *end;
    u8 *scan;
    u8 openKey;

    if (kind == 0) {
        gKbdKindIs0 = 1;
    } else {
        gKbdKindIs0 = 0;
    }
    gKbd->kindConf = (u8 *)&gKbdKindConfs[kind];
    Kbd_ClearOpen();
    Kbd_ClearActive();
    if (max > 255) {
        max = 255;
    }
    gKbd->maxLen = max;
    gKbdDelay = 1;
    openKey = gKbd->usbKey;
    switch (openKey) {
    case USB_KEY_ENTER:
    case USB_KEY_ESCAPE:
    case USB_KEY_BACKSPACE:
    case USB_KEY_KP_ENTER:
        gKbdReplayKey = 0;
        break;
    default:
        gKbdReplayKey = openKey;
        break;
    }
    gKbd->gestureMode = 0;
    gKbd->keyFlags = 0;
    gKbd->cyclePos = 0;
    gKbd->symbolMode = 0;
    Kbd_SetLayout(layout);
    Kbd_PickLayout();
    Kbd_BindLayouts(0);
    Kbd_BindStartKeys();
    gKbd->col = 1;
    gKbd->row = 0;
    Kbd_LookupKey();
    gKbd->converting = 0;
    gKbd->result = 0;
    gKbd->symbolMode = 0;
    gKbd->offGrid = 0;
    gKbd->cursorShown = 1;
    gKbd->blink = 0;
    gKbd->kanaPos = 0;
    gKbd->firstShown = 0;
    if (text == NULL) {
        gKbd->textPos = 0;
        gKbd->text[0] = 0;
    } else {
        memset(gKbd->text, 0, 256);
        strncpy(gKbd->text, text, 255);
        end = (u8 *)&gKbd->text[254];
        scan = (u8 *)gKbd->text;
        for (;;) {
            if (scan >= end) {
            if (end == scan && Sjis_IsSingleByte(*scan) == 0) {
                gKbd->text[254] = 0;
            } else {
                gKbd->text[255] = 0;
            }
            break;
            }
            if (Sjis_IsSingleByte(*scan) != 0) {
                scan++;
            } else {
                scan += 2;
            }
        }
        gKbd->textPos = strlen(text);
    }
    Kbd_KeyRepeat(0, 0);
    switch (layout) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        openKey = gKbd->usbKey;
        if (openKey != USB_KEY_ENTER && openKey != USB_KEY_KP_ENTER) {
            Kbd_SetActive();
        }
        break;
    default:
        if (Kbd_GetStartActive() != 0) {
            Kbd_SetActive();
        }
        break;
    }
    Kbd_SetOpen();
    Kbd_ClearCands();
    Kbd_OpenConv();
    gKbdOpenKey = gKbd->usbKey;
    memset(&gKbd->usbKey, 0, 10);
    sceUsbKbSetRepeat(0, 10, 4);
    gKbd->kanaInput = gKbdSavedKanaInput;
    gKbd->prevKeySet = gKbd->keySet;
    memset(gKbdPrompt, 0, 4);
    gKbdChatTarget = -1;
}

extern s32 gKbdLayoutFlags[];
extern KbdKindKeys gKbdKeySets[];

/*
 * Sets up entry kind entryKind: the barred key sets (gKbdLayoutFlags, plus
 * both phrase pages when the kind has no phrases); the starting key set
 * (gKbdKeySets[entryKind].keySet: kinds 0, 12, 13 and 14 keep a current
 * two-byte set, kind 3 an allowed one and kind 11 any; the set
 * Kbd_SaveKeySet saved for the kind wins; with Caps Lock on an alphabet set
 * swaps case); and the characters accepted (gKbdKeySets[entryKind].accept).
 * Then updates the mode, the cursor and the symbol mode.
 */
void Kbd_SetLayout(u8 entryKind)
{
    gKbd->kind = entryKind;
    gKbd->flags = gKbdLayoutFlags[entryKind];
    if (*(s32 *)(gKbd->kindConf + 48) == 0) {
        gKbd->flags |= KBD_SET_BIT(KBD_SET_PHRASE1);
        gKbd->flags |= KBD_SET_BIT(KBD_SET_PHRASE2);
    }
    switch (entryKind) {
    case 0:
    case 14:
    case 12:
    case 13:
        if (Kbd_SetIsWide() == 0) {
            gKbd->keySet = gKbdKeySets[entryKind].keySet;
        }
        break;
    case 3:
        if (gKbd->flags & (1 << gKbd->keySet)) {
            gKbd->keySet = gKbdKeySets[entryKind].keySet;
        }
        break;
    case 11:
        break;
    default:
        gKbd->keySet = gKbdKeySets[entryKind].keySet;
        break;
    }
    if (gKbdSavedKind == gKbd->kind) {
        gKbd->keySet = gKbdSavedKeySet;
    }
    if (gKbdSettings & USB_LED_CAPS_LOCK) {
        switch (gKbd->keySet) {
        case KBD_SET_WIDE_UPPER:
        case KBD_SET_ASCII_UPPER:
        case KBD_SET_ASCII_LOWER:
        case KBD_SET_WIDE_LOWER:
            if ((s8)Kbd_ShiftPair(gKbd->keySet) >= 0) {
                gKbd->keySet = Kbd_ShiftPair(gKbd->keySet);
            }
            break;
        }
    }
    gKbd->accept = gKbdKeySets[entryKind].accept;
    Kbd_UpdateMode();
    Kbd_HomePageCursor();
    Kbd_UpdateSymbolMode();
}

/*
 * Key repeat for the keyboard's buttons (KBD_REPEAT_BUTTONS). A new press
 * is returned at once and waits 10 frames; a held button then repeats every
 * 4 frames for R2 or while converting, every 8 for L1, R3 and R1, else every
 * frame. Kbd_KeyRepeat(0, 0) resets it. Returns the buttons to act on.
 */
s32 Kbd_KeyRepeat(s16 held, s16 pressed)
{
    s8 *timer;
    s32 repeated;
    s32 keys;

    pressed &= KBD_REPEAT_BUTTONS;
    if (pressed) {
        gKbd->repeatKeys = pressed;
        gKbd->repeatWait = 10;
        return pressed;
    }
    keys = held & KBD_REPEAT_BUTTONS;
    if ((s16)keys == 0) {
        return gKbd->repeatKeys = 0;
    }
    repeated = 0;
    --gKbd->repeatWait;
    timer = &gKbd->repeatWait;
    if (*timer <= 0) {
        if (gKbd->converting == 0) {
            if ((s16)keys & PAD_R2) {
                *timer = 4;
            } else if (((s16)keys & PAD_L1) || ((s16)keys & PAD_R3) || ((s16)keys & PAD_R1)) {
                *timer = 8;
            } else {
                *timer = 1;
            }
        } else {
            *timer = 4;
        }
        repeated = gKbd->repeatKeys &= (s16)keys;
    }
    return repeated;
}

/*
 * A command key (KBD_ACT_COMMAND, the command in ch.b[1]): KBD_CMD_BACKSPACE
 * deletes back, KBD_CMD_CONVERT starts the conversion (or types the key
 * while converting), KBD_CMD_SUBMIT submits.
 */
void Kbd_PressCmdKey(KbdKey *key)
{
    u8 cmd;

    cmd = key->ch.b[1];
    switch (cmd) {
    case KBD_CMD_BACKSPACE:
        Kbd_DeleteBack(1);
        break;
    case KBD_CMD_CONVERT:
        if (gKbd->converting != 0) {
            Kbd_TypeKey(key);
            break;
        }
        Kbd_Convert();
        break;
    case KBD_CMD_SUBMIT:
        Kbd_Submit();
        break;
    }
}

/*
 * The keyboard's frame; callers pass gPadState[0]'s held and pressed
 * buttons. After the gKbdDelay frames it replays gKbdReplayKey, handles the
 * USB key and takes the repeated buttons (Kbd_KeyRepeat). While the
 * keyboard is inactive the first button only activates it. Otherwise the
 * D-pad moves the cursor; CIRCLE runs the key under the cursor by its
 * action (ch.b[0]); START or TRIANGLE submits, CROSS deletes back, SQUARE
 * types a space; when not converting, L2 shifts, R1 turns the phrase page,
 * L1 switches the mode, R2 cycles the phrases or gestures and R3 the chat
 * target; while converting, L1 and R1 move between segments and L2 and R2
 * page the candidates. On a submit the text is copied to dst and the key
 * set saved. Entry kind 11 then clears the text and stays open. Plays the
 * frame's sound (KbdEntry.state). Returns KBD_RESULT_SUBMIT,
 * KBD_RESULT_CANCEL, or 0 while the keyboard is open.
 */
s8 Kbd_Run(char *dst, s16 held, s16 pressed)
{
    KbdKey *key;
    s16 keys;
    s8 *resultPtr;

    gKbd->frame += 1;
    gKbd->submitWait -= 1;
    gKbd->blink += 1;
    if (*(s8 *)&gKbdDelay <= 0) {
        *(s8 *)&gKbdDelay = 0;
        if (gKbdReplayKey != 0) {
            memset(&gKbd->usbKey, 0, 10);
            gKbd->kanaInput = gKbdSavedKanaInput;
            gKbd->prevKeySet = gKbd->keySet;
            gKbd->usbNewKey = gKbd->usbKey = gKbdReplayKey;
            gKbdReplayKey = 0;
        }
    } else {
        *(s8 *)&gKbdDelay -= 1;
        return 0;
    }
    gKbd->state = KBD_SOUND_NONE;
    Kbd_Update();
    keys = Kbd_KeyRepeat(held, pressed);
    if ((held != 0 || keys != 0) && gKbd->active == 0) {
        Kbd_SetActive();
        Kbd_KeyRepeat(0, 0);
        Sound_PlaySeFlat(0, SE_SYS_SELECT, 0);
        return 0;
    }
    if (keys & PAD_DPAD) {
        gKbd->state = SE_SYS_SCROLL;
        Kbd_MoveCursor(keys, held);
        gKbd->keyFlags &= (u8)~KBD_KEYF_CYCLING;
    }
    if (keys & PAD_CIRCLE) {
        key = gKbd->key;
        if (gKbd->converting == 0) {
            if (gKbd->active == 1 && gKbd->offGrid == 1) {
                gKbd->state = SE_SYS_REFUSE;
            } else if (key->ch.code == 0) {
                gKbd->state = SE_SYS_REFUSE;
            } else {
                switch (key->ch.b[0]) {
                case 0:
                    break;
                case KBD_ACT_SWITCH:
                    Kbd_PressKey(key);
                    break;
                case KBD_ACT_COMMAND:
                    Kbd_PressCmdKey(key);
                    break;
                case KBD_ACT_PHRASE:
                    Kbd_InsertPhrase();
                    if (gKbd->keyFlags & KBD_KEYF_CYCLING) {
                        Kbd_Submit();
                    }
                    break;
                case KBD_ACT_GESTURE:
                    Kbd_DoGesture(gKbd->text, &gKbd->textPos, key->ch.b[1]);
                    break;
                default:
                    goto other;
                }
            }
        } else {
        other:
            Kbd_TypeKey(key);
            gKbd->blink = 0;
        }
    } else if (pressed & PAD_START) {
        Kbd_Submit();
    } else if (keys & PAD_CROSS) {
        Kbd_DeleteBack(0);
    } else if (keys & PAD_SQUARE) {
        Kbd_Space(1);
    } else if (pressed & PAD_TRIANGLE) {
        Kbd_Submit();
    } else if ((pressed & PAD_L2) && gKbd->converting == 0) {
        Kbd_ToggleShift();
    } else if ((keys & PAD_R1) && gKbd->converting == 0) {
        Kbd_NextPhrasePage();
    } else if (gKbd->converting == 0) {
        if (keys & PAD_L1) {
            Kbd_NextRow(0);
        } else if (keys & PAD_R2) {
            switch (gKbd->keySet) {
            default:
            case KBD_SET_PHRASE1:
            case KBD_SET_PHRASE2:
                Kbd_CyclePhrase();
                break;
            case KBD_SET_GESTURE:
                Kbd_CycleGesture();
                break;
            }
        } else if (keys & PAD_R3) {
            Kbd_NextChatTarget();
        }
    } else {
        if (keys & PAD_L1) {
            Kbd_PrevSegment();
        }
        if (keys & PAD_R1) {
            Kbd_NextSegment();
        }
        if (keys & PAD_L2) {
            Kbd_PageUp();
        }
        if (keys & PAD_R2) {
            Kbd_PageDown();
        }
    }
    if (gKbd->result != 0) {
        if (gKbd->result == KBD_RESULT_SUBMIT) {
            strcpy(dst, gKbd->text);
            Kbd_SaveKeySet();
        }
        if (gKbd->kind == 11) {
            gKbd->blink = 0;
            gKbd->kanaPos = 0;
            gKbd->firstShown = 0;
            gKbd->textPos = 0;
            gKbd->text[0] = 0;
            resultPtr = &gKbd->result;
            if (*resultPtr != KBD_RESULT_CANCEL) {
                *resultPtr = 0;
                Kbd_ClearCands();
            }
        } else {
            sceUsbKbSetRepeat(0, 0, 0);
            sceUsbKbClearRbuf(0);
        }
    }
    if (gKbd->state != KBD_SOUND_NONE) {
        Sound_PlaySeFlat(0, gKbd->state, 0);
    }
    return gKbd->result;
}

/*
 * Moves the cursor by the D-pad buttons in keys (held is not used). On the
 * grid LEFT and RIGHT step the column (0..20, wrapping) and UP and DOWN the
 * row, until the key under the cursor changes; UP from row 0 or DOWN from
 * row 3 leaves the grid for the text line (offGrid), and back. On the text
 * line LEFT and RIGHT move the text cursor. While converting LEFT and RIGHT
 * move the text cursor and UP and DOWN pick the candidate.
 */
void Kbd_MoveCursor(s16 keys, s16 held)
{
    u8 *upRow;
    u8 *downRow;
    s8 startKey = gKbd->keyIdx;

    if (gKbd->offGrid == 0) {
        if (gKbd->converting != 0) {
            goto text;
        }
        do {
            if (keys & PAD_LEFT) {
                if (gKbd->col <= 0) {
                    gKbd->col = 20;
                } else {
                    gKbd->col -= 1;
                }
            } else if (keys & PAD_RIGHT) {
                gKbd->col += 1;
                if (gKbd->col > 20) {
                    gKbd->col = 0;
                }
            }
            if (keys & PAD_UP) {
                upRow = &gKbd->row;
                if (*upRow == 0) {
                    gKbd->offGrid = 1;
                    gKbd->blink = 0;
                    break;
                }
                *upRow -= 1;
            } else if (keys & PAD_DOWN) {
                downRow = &gKbd->row;
                if (*downRow == 3) {
                    gKbd->offGrid = 1;
                    gKbd->blink = 0;
                    break;
                }
                *downRow += 1;
            }
            Kbd_LookupKey();
        } while (startKey == gKbd->keyIdx);
    } else if (gKbd->converting != 0) {
        text:
        if (keys & PAD_RIGHT) {
            Kbd_CursorRight();
        } else if (keys & PAD_LEFT) {
            Kbd_CursorLeft();
        } else if (keys & PAD_UP) {
            Kbd_PrevCand();
        } else if (keys & PAD_DOWN) {
            Kbd_NextCand();
        }
    } else {
        if (keys & PAD_UP) {
            gKbd->row = 3;
            gKbd->offGrid = 0;
            Kbd_LookupKey();
        } else if (keys & PAD_DOWN) {
            gKbd->row = 0;
            gKbd->offGrid = 0;
            Kbd_LookupKey();
        } else if (keys & PAD_LEFT) {
            Kbd_CursorLeft();
        } else if (keys & PAD_RIGHT) {
            Kbd_CursorRight();
        }
    }
}

/*
 * Types key: with the pad, in the symbol mode it converts or commits, else
 * it enters the key's character when converting or on the grid; without
 * the pad, it types the USB keyboard's key (Kbd_TypeHidKey).
 */
void Kbd_TypeKey(KbdKey *key)
{
    if (gKbd->active == 1) {
        if (gKbd->symbolMode != 0) {
            Kbd_ConvertOrCommit();
            Kbd_KeyRepeat(0, 0);
            return;
        }
        if (gKbd->converting != 0) {
            Kbd_EnterChar(key);
            return;
        }
        if (gKbd->offGrid == 0) {
            Kbd_EnterChar(key);
        }
    } else {
        Kbd_TypeHidKey();
    }
}

const char gKanaDakutenTable_0[205] = {
#include "sub_main/0046CAE0.inc"
};
const char gKanaHandakutenTable_0[81] = {
#include "sub_main/0046CBB0.inc"
};
const char D_0046CC08[3] = {
#include "sub_main/0046CC08.inc"
};

extern void *gKanaDakutenTable; /* conversion tables for Kana_SwapBeforeCursor */
extern void *gKanaHandakutenTable;

/*
 * Enters key's character (ch), unless Kbd_CharBlocked. On a one-byte set it
 * commits any pending kana and inserts the character into the text at
 * textPos (up to maxLen). On a two-byte set a full-width space goes into the
 * text when no kana is pending; any other character goes into the kana at
 * kanaPos (up to 72 bytes), where a dakuten or handakuten key on the kana
 * sets first tries to mark the kana before the cursor. While converting, a
 * two-byte key commits the conversion and types nothing.
 */
void Kbd_EnterChar(KbdKey *key)
{
    s8 done;
    s32 added;
    u32 set;
    char buf[4];

    done = 0;
    if (Kbd_CharBlocked(key) == 1) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    if (Kbd_SetIsWide() == 0) {
        strncpy(buf, (char *)key->ch.b, 1);
        buf[1] = 0;
        if (gKbd->kana[0] != 0) {
            Kbd_CommitKana();
            Kbd_KeyRepeat(0, 0);
        }
    single:
        added = Str_InsertClip(gKbd->text, buf,
            gKbd->textPos, gKbd->maxLen);
        gKbd->textPos = gKbd->textPos + added;
        gKbd->state = SE_SYS_SELECT;
        return;
    }
    strncpy(buf, (char *)key->ch.b, 2);
    buf[2] = 0;
    if (strncmp(buf, D_0046CC08, 2) == 0) {
        if (gKbd->converting != 0) {
            goto clear;
        }
        if (gKbd->kana[0] == 0) {
            goto single;
        }
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    set = gKbd->keySet;
    if (!(set <= 1 || set == 6 || set - 8 <= 1
          || (gKbd->mode == KBD_MODE_GESTURE && (gKbd->gestureMode & KBD_GESTURE_KANA)))) {
        goto skip;
    }
    switch (Kbd_DakutenKeyKind(key)) {
    case 0:
        break;
    case KBD_MARK_DAKUTEN:
        if (Kana_SwapBeforeCursor(gKanaDakutenTable)) {
            done = 1;
        }
        break;
    case KBD_MARK_HANDAKUTEN:
        if (Kana_SwapBeforeCursor(gKanaHandakutenTable)) {
            done = 1;
        }
        break;
    }
skip:
    if (done == 0) {
        if (gKbd->converting != 0) {
        clear:
            Kbd_CommitKana();
            Kbd_KeyRepeat(0, 0);
        } else {
            added = Str_InsertClip((char *)gKbd->kana, buf,
                gKbd->kanaPos, 72);
            gKbd->kanaPos = gKbd->kanaPos + added;
        }
    }
    gKbd->state = SE_SYS_SELECT;
}

/*
 * R2 off the gestures page: types the phrases in turn. The first press
 * opens a phrase page and starts the cycle (KBD_KEYF_CYCLING); each later
 * one moves the cursor 5 columns on, wrapping, and Kbd_InsertPhrase
 * replaces the text with the phrase under the cursor. Refused while
 * converting, when a phrase page is barred or the kind has no phrases.
 */
void Kbd_CyclePhrase(void)
{
    s16 *cyclePosPtr;
    s32 flags;
    u8 *colPtr;

    if (gKbd->converting != 0 || ((flags = gKbd->flags) & KBD_SET_BIT(KBD_SET_PHRASE1)) ||
        (flags & KBD_SET_BIT(KBD_SET_PHRASE2)) || *(s32 *)(gKbd->kindConf + 48) == 0) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    if (!(gKbd->keyFlags & KBD_KEYF_CYCLING)) {
        gKbd->offGrid = 0;
        gKbd->keyFlags |= KBD_KEYF_CYCLING;
        gKbd->cyclePos = 1;
        switch (gKbd->keySet) {
        case KBD_SET_PHRASE1:
        default:
            Kbd_OpenPhrases1();
            break;
        case KBD_SET_PHRASE2:
            Kbd_OpenPhrases2();
            break;
        }
    } else {
        gKbd->cyclePos += 5;
        cyclePosPtr = &gKbd->cyclePos;
        if (*cyclePosPtr >= 60) {
            *cyclePosPtr = 1;
        }
        gKbd->col = (gKbd->cyclePos + 1) % 20;
        gKbd->row = (gKbd->cyclePos + 1) / 20;
        colPtr = &gKbd->col;
        if (*colPtr > 19 || gKbd->row >= 3) {
            *colPtr = 5;
            gKbd->row = 0;
            gKbd->cyclePos = 1;
        }
    }
    Kbd_InsertPhrase();
}

/*
 * R2 on the gestures page: steps the cursor through the gesture keys, as
 * Kbd_CyclePhrase does. Refused while converting, when the gestures set is
 * barred or the kind has no gestures.
 */
void Kbd_CycleGesture(void)
{
    s16 *cyclePosPtr;
    u8 *colPtr;

    if (gKbd->converting != 0 || (gKbd->flags & KBD_SET_BIT(KBD_SET_GESTURE)) ||
        *(s32 *)(gKbd->kindConf + 40) == 0) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    if (!(gKbd->keyFlags & KBD_KEYF_CYCLING)) {
        gKbd->offGrid = 0;
        gKbd->keyFlags |= KBD_KEYF_CYCLING;
        gKbd->cyclePos = 1;
        gKbd->keySet = KBD_SET_GESTURE;
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_EnterGestures();
        Kbd_RefreshLayout();
    } else {
        gKbd->cyclePos += 5;
        cyclePosPtr = &gKbd->cyclePos;
        if (*cyclePosPtr >= 60) {
            *cyclePosPtr = 1;
        }
        gKbd->col = (gKbd->cyclePos + 1) % 20;
        gKbd->row = (gKbd->cyclePos + 1) / 20;
        colPtr = &gKbd->col;
        if (*colPtr > 19 || gKbd->row >= 3) {
            *colPtr = 5;
            gKbd->row = 0;
            gKbd->cyclePos = 1;
        }
    }
    Kbd_RefreshLayout();
    gKbd->state = SE_SYS_SELECT;
}


/*
 * R3 (and the USB Tab key): steps to the next chat target and shows its
 * name as the prompt (gKbdPrompt). Refused when chat targets are off
 * (gKbdChatTarget < 0) or the prompt stays empty.
 */
void Kbd_NextChatTarget(void)
{
    s8 name[32];
    s32 same;

    gKbdChatTarget = Chat_NextTarget(name, 1);
    gKbd->blink = 0;
    if (name[0] == 0 && gKbdPrompt[0] == 0) {
        same = 1;
    } else {
        same = 0;
    }
    strncpy((char *)gKbdPrompt, (char *)name, 4);
    gKbdChatTarget = Chat_NextTarget(gKbdPrompt, 0);
    if (gKbdChatTarget < 0 || same) {
        gKbd->state = SE_SYS_REFUSE;
    } else {
        gKbd->state = SE_SYS_SCROLL;
    }
}

extern u8 D_00418739[];

/*
 * Inserts the phrase of the key under the cursor into the text at textPos
 * (up to maxLen); during a cycle (KBD_KEYF_CYCLING) it replaces the text.
 * Nothing on the row of the page's mode key or when the key has no entry.
 */
void Kbd_InsertPhrase(void)
{
    char buf[256];
    s32 added;

    if (gKbd->row == D_00418739[0]) {
        return;
    }
    Kbd_RefreshLayout();
    if (gKbd->keyInfo == NULL) {
        return;
    }
    if (gKbd->keyFlags & KBD_KEYF_CYCLING) {
        Kbd_ClearText();
    }
    if (*(char **)(gKbd->keyInfo + 12) == NULL) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    strcpy(buf, *(char **)(gKbd->keyInfo + 12));
    added = Str_InsertClip(gKbd->text, buf, gKbd->textPos,
                      gKbd->maxLen);
    gKbd->textPos = gKbd->textPos + added;
    gKbd->state = SE_SYS_SELECT;
}

extern s8 gKbdWidthPairs[];

/*
 * The key set of the other width for set (gKbdWidthPairs), or -1 when there
 * is none or it is barred.
 */
static s32 Kbd_WidthPair(u8 set)
{
    s32 pair = gKbdWidthPairs[set];

    if (pair >= 0 && !(gKbd->flags & (1 << (s8)pair))) {
        return pair;
    }
    return -1;
}

/*
 * Switches to the other width's key set (Kbd_WidthPair), committing any
 * pending kana and keeping the cursor; refused when there is none.
 */
void Kbd_ToggleWidth(void)
{
    s32 state = SE_SYS_REFUSE;
    s8 pair;
    u8 savedCol;
    u8 savedRow;

    pair = Kbd_WidthPair(gKbd->keySet);
    if (pair >= 0) {
        savedCol = gKbd->col;
        savedRow = gKbd->row;
        gKbd->keySet = pair;
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_CommitKana();
        Kbd_RefreshLayout();
        Kbd_UpdateSymbolMode();
        gKbd->col = savedCol;
        gKbd->row = savedRow;
        state = SE_SYS_SELECT;
    }
    gKbd->state = state;
}

/*
 * Backspace (CROSS, the backspace key, the USB Backspace key). With nothing
 * typed (entry kinds 12 and 13 excepted) or during a cycle it cancels the
 * keyboard (KBD_RESULT_CANCEL). Otherwise it deletes the character before
 * the cursor: two bytes of the pending kana, or one character of the text.
 * While converting it leaves the conversion instead. The parameter is not
 * used. The kana is cut through a u8 store for the match (instruction
 * order).
 */
void Kbd_DeleteBack(s32 unused)
{
    u8 *convPtr;
    char *kana;
    s16 *pos;
    s32 size;
    s32 cursor;
    char *text;

    if (gKbd->kind != 12 && gKbd->kind != 13 &&
        gKbd->kana[0] == 0 && (u8)gKbd->text[0] == 0) {
        gKbd->result = KBD_RESULT_CANCEL;
        gKbd->state = SE_SYS_CANCEL;
        return;
    }
    if (gKbd->keyFlags & KBD_KEYF_CYCLING) {
        gKbd->result = KBD_RESULT_CANCEL;
        gKbd->state = SE_SYS_CANCEL;
        return;
    }
    convPtr = &gKbd->converting;
    if (*convPtr == 0) {
        kana = (char *)gKbd->kana;
        if (*(u8 *)kana != 0) {
            cursor = gKbd->kanaPos;
            if (cursor == 0) {
                gKbd->state = SE_SYS_REFUSE;
                return;
            }
            *(u8 *)(kana + cursor - 2) = 0;
            strcat(kana, kana + cursor);
            gKbd->kanaPos -= 2;
            pos = (s16 *)&gKbd->kanaPos;
            if (*pos <= 0) {
                *pos = 0;
                Kbd_KeyRepeat(0, 0);
            }
        } else {
            cursor = gKbd->textPos;
            text = gKbd->text;
            if (cursor == 0) {
                gKbd->state = SE_SYS_REFUSE;
                return;
            }
            size = Sjis_PrevCharSize((u8 *)text, cursor);
            *(text + cursor - size) = 0;
            strcat(text, text + cursor);
            gKbd->textPos = gKbd->textPos - size;
            pos = (s16 *)&gKbd->textPos;
            if (*pos <= 0) {
                *pos = 0;
                Kbd_KeyRepeat(0, 0);
            }
        }
        gKbd->blink = 0;
        gKbd->state = SE_SYS_SELECT;
        return;
    }
    *convPtr = 0;
    gKbd->blink = 0;
    gKbd->offGrid = 0;
    if (gKbd->symbolMode != 0) {
        gKbd->kanaPos = 0;
        gKbd->kana[0] = 0;
    }
    gKbd->state = SE_SYS_SELECT;
}

/*
 * Submits the text (START, TRIANGLE, the submit key). With kana pending it
 * commits the kana instead. Refused while submitWait (Kbd_SetSubmitWait) is
 * positive. Otherwise sets KBD_RESULT_SUBMIT, with SE_SYS_SEND, or
 * SE_SYS_CANCEL for an empty text.
 */
void Kbd_Submit(void)
{
    s32 *wait;

    if (gKbd->kana[0] != 0) {
        Kbd_CommitKana();
        gKbdPhrasePage = 0;
        gKbd->state = SE_SYS_SELECT;
        return;
    }
    wait = &gKbd->submitWait;
    if (*wait != 0) {
        if (*wait < 0) {
            *wait = 0;
        } else {
            gKbd->state = SE_SYS_REFUSE;
            return;
        }
    }
    gKbd->result = KBD_RESULT_SUBMIT;
    if (gKbd->kana[0] == 0 && (u8)gKbd->text[0] == 0) {
        gKbd->state = SE_SYS_CANCEL;
    } else {
        gKbd->state = SE_SYS_SEND;
    }
}

/*
 * L1: switches to the next (back: previous) of the four character modes
 * that Kbd_FindKind allows; from a phrase or gestures page it starts at
 * hiragana (back: symbols) and sends the cursor home. When none is allowed
 * the mode and set are restored and the press refused. back is always 0.
 */
void Kbd_NextRow(s32 back)
{
    s32 i;
    s32 found;
    u8 savedSet;
    s8 tryMode;
    u8 savedMode;

    savedMode = gKbd->mode;
    savedSet = gKbd->keySet;
    found = 0;

    if (savedMode == KBD_MODE_GESTURE || savedMode == KBD_MODE_PHRASE1 || savedMode == KBD_MODE_PHRASE2) {
        if (back) {
            tryMode = 0;
        } else {
            tryMode = 3;
        }
    } else {
        tryMode = savedMode;
    }
    for (i = 0; i < 4; i++) {
        if (back) {
            tryMode--;
        } else {
            tryMode++;
        }
        tryMode = (tryMode + 4) % 4;
        if (Kbd_FindKind(tryMode, &gKbd->mode, &gKbd->keySet) == 0 && tryMode != savedMode) {
            found = 1;
            break;
        }
    }
    if (!found) {
        gKbd->keySet = savedSet;
        gKbd->mode = savedMode;
        gKbd->state = SE_SYS_REFUSE;
    } else {
        gKbd->cyclePos = 0;
        gKbd->gestureMode = 0;
        gKbd->keyFlags &= ~KBD_KEYF_CYCLING;
        gKbd->state = SE_SYS_SELECT;
        switch (savedMode) {
        case KBD_MODE_GESTURE:
        case KBD_MODE_PHRASE1:
        case KBD_MODE_PHRASE2:
            gKbd->col = 1;
            gKbd->row = 0;
            break;
        }
    }
    if (gKbd->keySet == KBD_SET_GESTURE) {
        Kbd_EnterGestures();
    }
    Kbd_UpdateMode();
    Kbd_HomePageCursor();
    Kbd_RefreshLayout();
    Kbd_UpdateSymbolMode();
    if (gKbd->symbolMode != 0) {
        Kbd_Convert();
        Kbd_KeyRepeat(0, 0);
    }
}

extern KeyPair gKbdGestureBinds[];
extern KeyPair gKbdPhrase1Binds[];
extern KeyPair gKbdPhrase2Binds[];

/* Binds the cursors and grids of the phrases and gestures pages for the keyboard kind. */
/*
 * Points the gestures page and the two phrase pages of gKbdSetLayouts at
 * the cursors and grids of the entry kind's binds (gKbdGestureBinds,
 * gKbdPhrase1Binds, gKbdPhrase2Binds, picked by kindConf +0x28 and +0x30,
 * entry 0 being none) and at its gesture and phrase lists (kindConf +0x2C
 * and +0x34). The parameter is not used: the caller passes 0.
 */
void Kbd_BindLayouts(s32 unused)
{
    KbdLayout *layout;

    layout = &gKbdSetLayouts[KBD_SET_GESTURE];
    layout->cursors = gKbdGestureBinds[*(s32 *)(gKbd->kindConf + 40)].cursors;
    layout->grid = gKbdGestureBinds[*(s32 *)(gKbd->kindConf + 40)].grid;
    layout->extra = *(u8 **)(gKbd->kindConf + 44);
    layout = &gKbdSetLayouts[KBD_SET_PHRASE1];
    layout->cursors = gKbdPhrase1Binds[*(s32 *)(gKbd->kindConf + 48)].cursors;
    layout->grid = gKbdPhrase1Binds[*(s32 *)(gKbd->kindConf + 48)].grid;
    layout->extra = *(u8 **)(gKbd->kindConf + 52);
    layout = &gKbdSetLayouts[KBD_SET_PHRASE2];
    layout->cursors = gKbdPhrase2Binds[*(s32 *)(gKbd->kindConf + 48)].cursors;
    layout->grid = gKbdPhrase2Binds[*(s32 *)(gKbd->kindConf + 48)].grid;
    layout->extra = *(u8 **)(gKbd->kindConf + 52);
}

extern KbdKey gKbdKeysHira_4[];
extern KbdKey gKbdKeysHira_5;
extern KbdKey gKbdKeysHira_6;
extern KbdKey gKbdKeysKata_4[];
extern KbdKey gKbdKeysKata_5;
extern KbdKey gKbdKeysKata_6;
extern KbdKey gKbdKeysWideUpper_4[];
extern KbdKey gKbdKeysWideUpper_5;
extern KbdKey gKbdKeysWideUpper_6;
extern KbdKey gKbdKeysSmallHira_4[];
extern KbdKey gKbdKeysSmallHira_5;
extern KbdKey gKbdKeysSmallHira_6;
extern KbdKey gKbdKeysSmallKata_4[];
extern KbdKey gKbdKeysSmallKata_5;
extern KbdKey gKbdKeysSmallKata_6;
extern KbdKey gKbdKeysWideLower_4[];
extern KbdKey gKbdKeysWideLower_5;
extern KbdKey gKbdKeysWideLower_6;
extern KbdKey gKbdKeysHalfKata_4[];
extern KbdKey gKbdKeysHalfKata_5;
extern KbdKey gKbdKeysHalfKata_6;
extern KbdKey gKbdKeysHalfSmallKata_4[];
extern KbdKey gKbdKeysHalfSmallKata_5;
extern KbdKey gKbdKeysHalfSmallKata_6;
extern KbdKey gKbdKeysSymbol_4[];
extern KbdKey gKbdKeysSymbol_5;
extern KbdKey gKbdKeysSymbol_6;
extern KbdKey gKbdKeysGesture_4[];
extern KbdKey gKbdKeysGesture_5;
extern KbdKey gKbdKeysPhrase1_5;
extern KbdKey gKbdKeysPhrase1_6;
extern KbdKey gKbdKeysPhrase2_4[];
extern KbdKey gKbdKeysPhrase2_6;
extern KbdKey gKbdKeysAsciiUpper_4[];
extern KbdKey gKbdKeysAsciiUpper_5;
extern KbdKey gKbdKeysAsciiUpper_6;
extern KbdKey gKbdKeysAsciiLower_4[];
extern KbdKey gKbdKeysAsciiLower_5;
extern KbdKey gKbdKeysAsciiLower_6;

/*
 * Copies the three page keys into keys 4-6 of every key table: key 4 opens
 * the first phrase page, key 5 the second and key 6 the gestures page, each
 * from the entry kind's binds (gKbdPhrase1Binds, gKbdPhrase2Binds,
 * gKbdGestureBinds), or a blank key when the kind has no such page. gKbd is
 * held in a 64-bit local and kindConf read at its offset for the match: the
 * game spills it with sq/lq.
 */
void Kbd_BindStartKeys(void)
{
    MWCC_LONG k = (MWCC_LONG)gKbd;

    gKbdKeysHira_4[0] = gKbdKeysSmallHira_4[0] = gKbdKeysKata_4[0] = gKbdKeysHalfKata_4[0] = gKbdKeysSmallKata_4[0] = gKbdKeysHalfSmallKata_4[0] = gKbdKeysWideUpper_4[0] =
        gKbdKeysWideLower_4[0] = gKbdKeysAsciiUpper_4[0] = gKbdKeysAsciiLower_4[0] = gKbdKeysSymbol_4[0] = gKbdKeysGesture_4[0] = gKbdKeysPhrase2_4[0] =
            gKbdPhrase1Binds[*(s32 *)(*(u8 **)((u8 *)k + 16) + 48)].key;
    gKbdKeysHira_5 = gKbdKeysSmallHira_5 = gKbdKeysKata_5 = gKbdKeysHalfKata_5 = gKbdKeysSmallKata_5 = gKbdKeysHalfSmallKata_5 = gKbdKeysWideUpper_5 =
        gKbdKeysWideLower_5 = gKbdKeysAsciiUpper_5 = gKbdKeysAsciiLower_5 = gKbdKeysSymbol_5 = gKbdKeysGesture_5 = gKbdKeysPhrase1_5 =
            gKbdPhrase2Binds[*(s32 *)(*(u8 **)((u8 *)k + 16) + 48)].key;
    gKbdKeysHira_6 = gKbdKeysSmallHira_6 = gKbdKeysKata_6 = gKbdKeysSmallKata_6 = gKbdKeysHalfKata_6 = gKbdKeysHalfSmallKata_6 = gKbdKeysWideUpper_6 =
        gKbdKeysWideLower_6 = gKbdKeysAsciiUpper_6 = gKbdKeysAsciiLower_6 = gKbdKeysSymbol_6 = gKbdKeysPhrase1_6 = gKbdKeysPhrase2_6 =
            gKbdGestureBinds[*(s32 *)(*(u8 **)((u8 *)k + 16) + 40)].key;
}

/*
 * Replaces the two-byte character before the kana cursor with its pair in
 * table (a string of 4-byte entries: a character, then its replacement).
 * Returns 1 when it replaced one (the cursor blink restarts), else 0.
 */
s32 Kana_SwapBeforeCursor(s8 *table)
{
    s8 *prev;
    u16 code;
    s32 i;
    s32 count;
    s32 pos;
    u16 from;

    pos = gKbd->kanaPos;
    if (pos <= 1) {
        return 0;
    }
    prev = (s8 *)&gKbd->kana[pos - 2];
    code = (prev[0] << 8) | (u8)prev[1];
    count = (u32)strlen((char *)table) / 4;
    for (i = 0; i < count; i++) {
        from = (u16)(*table++ << 8);
        from |= (u8)*table++;
        if (from == code) {
            prev[0] = *table++;
            prev[1] = *table++;
            gKbd->blink = 0;
            return 1;
        }
        table += 2;
    }
    return 0;
}

/* Copies the 2-byte characters of src to dst, turning ASCII letters into full-width (Shift-JIS) ones. */
void Str_ToWide(s8 *src, s8 *dst)
{
    s8 character;

    while ((character = *src++) != 0) {
        if (character >= 'A' && character <= 'Z') {
            dst[0] = 130;
            dst[1] = character + 31;
            src++;
            dst += 2;
        } else if (character >= 'a' && character <= 'z') {
            dst[0] = 130;
            dst[1] = character + 32;
            src++;
            dst += 2;
        } else {
            dst[0] = character;
            dst[1] = *src;
            src++;
            dst += 2;
        }
    }
    *dst = 0;
}

/*
 * Fills the box of the key under the cursor with color; (dx, dy) is the
 * keyboard's position, and the box's x and width are scaled by drawScale.
 */
void Kbd_DrawCursorBox(s16 dy, s32 color, f32 dx)
{
    KbdKeyPos *box = gKbd->keyPos;
    f32 scale = gKbd->drawScale;

    Kbd_FillRect(color, (s16)(s32)(dx + box->x * scale), dy + box->y, (s16)(s32)(dx + scale * (box->x + gKbdBoxSizes[box->kind].w)),
                  dy + (box->y + gKbdBoxSizes[box->kind].h));
}

typedef struct KeyPos {
    u8 row;
    u8 col;
} KeyPos;
extern KeyPos gKbdSpecialKeyPos[];

/*
 * Fills the box of the current mode's key (gKbdSpecialKeyPos[mode]) with
 * color, at the keyboard's position (dx, dy). Nothing is drawn when the
 * cursor is on that key.
 */
void Kbd_DrawSpecialKey(s16 dy, s32 color, f32 dx)
{
    KbdLayout *curLayout;
    KbdKeyPos *box;
    f32 scale;
    s32 posOffset;
    s32 gridIdx;
    u8 keyCol;
    u8 keyRow;
    s32 curCol;
    s32 curRow;

    curCol = gKbd->col;
    curRow = gKbd->row;
    gridIdx = (curCol << 2) + curRow;
    posOffset = gKbd->mode * sizeof(KeyPos); /* byte offset of the mode's KeyPos */
    keyCol = (&gKbdSpecialKeyPos->row)[posOffset];
    keyRow = (&gKbdSpecialKeyPos->col)[posOffset];
    switch (gKbd->mode) {
    case KBD_MODE_PHRASE1:
        if (gKbd->layout->grid[gridIdx] == 4 && keyRow == curRow) {
            return;
        }
        break;
    case KBD_MODE_PHRASE2:
        if (gKbd->layout->grid[gridIdx] == 5 && keyRow == curRow) {
            return;
        }
        break;
    case KBD_MODE_GESTURE:
        if (gKbd->layout->grid[gridIdx] == 6 && keyRow == curRow) {
            return;
        }
        break;
    default:
        if (gKbd->active == 1 && gKbd->offGrid == 0 &&
            keyCol == curCol && keyRow == curRow) {
            return;
        }
        break;
    }
    gridIdx = (keyCol << 2) + keyRow;
    curLayout = gKbd->layout;
    scale = gKbd->drawScale;
    box = &curLayout->cursors[curLayout->grid[gridIdx]];
    Kbd_FillRect(color, (s16)(s32)(dx + box->x * scale), dy + box->y, (s16)(s32)(dx + scale * (box->x + gKbdBoxSizes[box->kind].w)),
                  dy + (box->y + gKbdBoxSizes[box->kind].h));
}

/* Fills the rectangle from (x, y) to (right, bottom) with color. */
void Kbd_FillRect(s32 color, s16 x, s16 y, s16 right, s16 bottom)
{
    Rect16 rect;

    rect.color = color;
    rect.x = x;
    rect.y = y;
    rect.w = right;
    rect.h = bottom;
    Prim_FillRect((struct PrimLine *)&rect);
}

/*
 * Draws width x height pixels at (x, y), tinted color, from the texel
 * rectangle (texLeft, texTop)-(texRight, texBottom) of the bound texture.
 */
void Kbd_DrawSprite(u32 color, s16 x, s16 y, s16 width, s16 height, s16 texLeft, s16 texTop, s16 texRight, s16 texBottom)
{
    Sprite sprite;

    sprite.color = color;
    sprite.x = x;
    sprite.y = y;
    sprite.w = width;
    sprite.h = height;
    sprite.u0 = texLeft;
    sprite.v0 = texTop;
    sprite.u1 = texRight;
    sprite.v1 = texBottom;
    Sprite_Draw(&sprite);
}


/*
 * 1 when the whole keyboard is drawn: while it takes the input or shows a
 * phrase or gestures page, or when F9, F10 or F12 opened it (F11 for entry
 * kind 0; gKbdOpenKey). Otherwise only the text line is drawn: 0.
 * Chat_DrawKbdBox sizes its frame by it.
 */
s32 Kbd_KeysShown(void)
{
    if ((gKbd->active ^ 1) && gKbd->layout->extra == NULL) {
        switch (gKbdOpenKey) {
        case USB_KEY_F11:
            if (gKbdKindIs0 == 0) {
                goto no; /* an if/return pair would become a set-on-condition */
            }
            return 1;
        no:
            return 0;
        case USB_KEY_F9:
        case USB_KEY_F10:
        case USB_KEY_F12:
            return 1;
        default:
            return 0;
        }
    }
    return 1;
}

const char D_0046CC10[3] = {
#include "sub_main/0046CC10.inc"
};

typedef struct KbdColor {
    u32 rgb : 24;
    u32 a : 8;
} KbdColor;
typedef struct KbdGlyph {
    u8 w;
    u8 scale;
    u8 h;
    u8 pad;
} KbdGlyph;
extern KbdGlyph gKbdGlyphs[];
extern u8 gKbdWidthKeys[];
extern u8 gKbdWidthKeys_1;
extern u32 gKbdPulseAmp[];
extern u32 gKbdLinePulseAmp[];
extern s8 gKbdPhraseKeyOrder[];
extern s32 gKbdInactive;
extern s32 gKbdCursorHideFrames;
void Kbd_DrawLine(f32 x, s16 y); /* unit's own declaration: the definition takes a u32 y;
 * Kbd_Draw's call matches with s16 y */

#define KBD_COLW(i) (((u32 *)gKbd->kindConf)[i])
#define KBD_COLOR(i) (((KbdColor *)gKbd->kindConf)[i])
#define KBD_PULSE(v, i, amp) v = KBD_COLW(i) & 0xFFFFFF; v |= (((KBD_COLW(i) >> 24) & 0xFF) + (s8)(sin * (amp))) << 24

/*
 * Draws the software keyboard at (gKbd->x, gKbd->y); the parameter is not
 * used. While the keyboard doesn't take the input and shows no phrase or
 * gestures page, only a strip is drawn: the panel, the text line and the
 * mode and width keys. Otherwise, in order: the panel; the text line,
 * pulsing while the cursor is on it; the key boxes; on a phrase or gestures
 * page the twelve phrase texts; the pulsing cursor box (unless
 * gKbdCursorHideFrames counts down); the mode key's box; the key glyphs
 * from the glyph texture (a blocked key as the blank glyph). Then the text
 * and the conversion candidates.
 * Kept for the match: the grid is indexed (col << 2) + row so the bytes
 * load in the original's order; the box loop counts with i and walks box;
 * a glyph's width is an s32 holding the (s16) of its byte; the glyph
 * loops' declaration orders give their registers and stack slots.
 */
void Kbd_Draw(u8 unused)
{
    KbdLayout *curLayout;
    KbdKey *kbdKey;
    f32 x;
    f32 sin;
    s16 y;
    u32 i;
    KbdKeyPos *box;
    s8 modeKey;
    s8 cursorKey;
    u32 color;

    x = gKbd->x;
    y = gKbd->y;
    sin = Math_Sin(ANGLE16_TO_RAD * (u16)((gKbd->frame & 0x3F) << 10));
    gKbdInactive = gKbd->active ^ 1;
    Text_FlushPages();
    gKbd->drawScale = 1.0f;
    x *= gKbd->drawScale;
    if (gKbdInactive != 0 && gKbd->layout->extra == 0) {
        switch (gKbdOpenKey) {
        case USB_KEY_F9:
        case USB_KEY_F10:
        case USB_KEY_F11:
        case USB_KEY_F12:
            gKbdOpenKey = 0;
            return;
        default:
            Kbd_FillRect(KBD_COLW(0), (s32)(x - 56.0f * gKbd->drawScale), y - 4, (s32)(x + 488.0f * gKbd->drawScale), y + 24);
            break;
        }
    } else {
        Kbd_FillRect(KBD_COLW(0), (s32)x, y - 4, (s32)(x + 488.0f * gKbd->drawScale), y + 100);
    }
    if (gKbd->active == 1 && gKbd->offGrid == 1 && gKbd->converting == 0) {
        KBD_PULSE(color, 2, gKbdLinePulseAmp[0]);
        Kbd_FillRect(color, (s32)(x + 6.0f * gKbd->drawScale), (s32)y,
                     (s32)(x + 488.0f * gKbd->drawScale - 6.0f * gKbd->drawScale), y + 20);
    } else {
        Kbd_FillRect(KBD_COLW(1), (s32)(x + 6.0f * gKbd->drawScale), (s32)y,
                     (s32)(x + 488.0f * gKbd->drawScale - 6.0f * gKbd->drawScale), y + 20);
    }
    curLayout = gKbd->layout;
    box = curLayout->cursors;
    for (i = curLayout->nBoxes; i != 0; i--) {
        if (gKbdInactive != 0 && gKbd->layout->extra == 0) {
            Kbd_DrawSpecialKey(y - gKbd->mode * 17 - 26, KBD_COLW(1), x - 56.0f * gKbd->drawScale);
            Kbd_DrawSpecialKey(y - gKbd->mode * 17 - 26, KBD_COLW(1), x - 30.0f * gKbd->drawScale);
            break;
        }
        Kbd_FillRect(KBD_COLW(1), (s32)(x + box->x * gKbd->drawScale), y + box->y,
                     (s32)(x + gKbd->drawScale * (box->x + gKbdBoxSizes[box->kind].w)), y + box->y + gKbdBoxSizes[box->kind].h);
        box++;
    }
    Kbd_BindGlyphTex();
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    curLayout = gKbd->layout;
    if (curLayout->extra != 0 && gKbd->gestureMode == 0) {
        s8 cur;
        s8 *order;

        cur = curLayout->grid[(u32)((gKbd->col << 2) + gKbd->row)];
        box = curLayout->cursors + 13;
        Font_SetSpacing(20, 16);
        for (i = 0, order = gKbdPhraseKeyOrder; i < 12; i++, order++, box++) {
            Font_SetPos(box->x + x / gKbd->drawScale, y + box->y);
            if (cur == i + 13 && (gKbd->offGrid == 0 || gKbd->active != 1)) {
                Text_SetColor(1);
            } else {
                Text_SetColor(0);
            }
            switch (gKbd->keySet) {
            default:
            case KBD_SET_PHRASE1:
                Kbd_Printf(D_0046CC10, gKbd->layout->extra + *order * 16);
                break;
            case KBD_SET_PHRASE2:
                Kbd_Printf(D_0046CC10, gKbd->layout->extra + (*order + 12) * 16);
                break;
            }
        }
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    if ((gKbdInactive == 0 || gKbd->layout->extra != 0) && gKbdCursorHideFrames == 0 &&
        (gKbd->offGrid == 0 || Kbd_ArrowsOnPage() == 1)) {
        kbdKey = gKbd->key;
        Gfx_SetBlendAlphaAdd(1);
        if (Kbd_CharBlocked(kbdKey) == 1) {
            KBD_PULSE(color, 7, gKbdPulseAmp[0]);
        } else {
            KBD_PULSE(color, 6, gKbdPulseAmp[0]);
        }
        Kbd_DrawCursorBox(y, color, x);
    }
    if (gKbdCursorHideFrames > 0) {
        gKbdCursorHideFrames--;
    }
    Gfx_SetBlendAlphaAdd(1);
    if (gKbdInactive != 0 && gKbd->layout->extra == 0) {
        Kbd_DrawSpecialKey(y - gKbd->mode * 17 - 26, KBD_COLW(8), x - 56.0f * gKbd->drawScale);
        Kbd_DrawSpecialKey(y - gKbd->mode * 17 - 26, KBD_COLW(8), x - 30.0f * gKbd->drawScale);
    } else {
        KBD_PULSE(color, 8, gKbdPulseAmp[0]);
        Kbd_DrawSpecialKey(y, color, x);
    }
    Gfx_SetBlendAlphaAdd(0);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    curLayout = gKbd->layout;
    box = curLayout->cursors;
    kbdKey = curLayout->keys;
    cursorKey = curLayout->grid[(u32)((gKbd->col << 2) + gKbd->row)];
    modeKey = curLayout->grid[(u32)((gKbdSpecialKeyPos[gKbd->mode].row << 2) + gKbdSpecialKeyPos[gKbd->mode].col)];
    if (gKbdInactive != 0 && curLayout->extra == 0) {
        for (i = 0; i < gKbd->layout->nKeys; i++, box++, kbdKey++) {
            if (i >= 13 || i == modeKey || i == 7) {
                s32 glyphW;
                s32 drawW;
                s16 pad;
                s16 glyphH;
                u8 code;
                u8 glyphIdx;

                if (i >= 13) {
                    break;
                }
                if (Kbd_CharBlocked(kbdKey) == 1) {
                    code = 197;
                    color = KBD_COLW(4);
                    glyphIdx = 0;
                } else {
                    color = KBD_COLW(3);
                    code = kbdKey->glyph;
                    glyphIdx = kbdKey->font & 0xF;
                }
                glyphW = (s16)gKbdGlyphs[glyphIdx].w;
                glyphH = gKbdGlyphs[glyphIdx].h;
                drawW = (glyphW * (s16)gKbdGlyphs[glyphIdx].scale) >> 4;
                pad = (s16)(gKbd->drawScale * gKbdBoxSizes[box->kind].w) - drawW;
                if (i == cursorKey) {
                    if (gKbdCursorHideFrames == 0 && (gKbd->offGrid == 0 || Kbd_ArrowsOnPage() == 1)) {
                        color = KBD_COLW(5);
                    } else if (cursorKey == modeKey && i < 4) {
                        color = KBD_COLW(5);
                    } else {
                        color = KBD_COLW(3);
                    }
                } else if (i == modeKey) {
                    color = KBD_COLW(5);
                }
                if (i == 7) {
                    s16 widthKeyW = gKbdGlyphs[0].w;
                    s16 widthKeyScale = gKbdGlyphs[0].scale;
                    s16 widthKeyH = gKbdGlyphs[0].h;

                    switch (gKbd->keySet) {
                    case KBD_SET_HALF_KATA:
                    case KBD_SET_ASCII_UPPER:
                    case KBD_SET_HALF_SYMBOL:
                    case KBD_SET_HALF_SMALL_KATA:
                    case KBD_SET_ASCII_LOWER:
                    case KBD_SET_DIGIT:
                        code = gKbdWidthKeys[0];
                        break;
                    default:
                        code = gKbdWidthKeys_1;
                        break;
                    }
                    Kbd_DrawSprite(((s32 *)gKbd->kindConf)[5], x + -20.0f * gKbd->drawScale, y + 3,
                                   (widthKeyW * widthKeyScale) >> 4, widthKeyH,
                                   ((code & 0xF) << 4) + 1, (code & 0xF0) + 1,
                                   ((code & 0xF) << 4) - 1 + widthKeyW, (code & 0xF0) + 16);
                } else {
                    Kbd_DrawSprite(color, (f32)(pad >> 1) + (x + (box->x - 56.0f) * gKbd->drawScale), y + 3, drawW, glyphH,
                                   ((code & 0xF) << 4) + 1, (code & 0xF0) + 1,
                                   ((code & 0xF) << 4) - 1 + glyphW, (code & 0xF0) + 16);
                }
            }
        }
    } else {
        for (i = 0; i < gKbd->layout->nKeys; i++, box++, kbdKey++) {
            u8 code;
            u8 glyphIdx;
            s32 drawW;
            s32 glyphW;
            s16 glyphH;
            s16 pad;

            if (Kbd_CharBlocked(kbdKey) == 1) {
                code = 197;
                color = KBD_COLW(4);
                glyphIdx = 0;
            } else {
                code = kbdKey->glyph;
                color = KBD_COLW(3);
                glyphIdx = kbdKey->font & 0xF;
            }
            glyphW = (s16)gKbdGlyphs[glyphIdx].w;
            glyphH = gKbdGlyphs[glyphIdx].h;
            drawW = (glyphW * (s16)gKbdGlyphs[glyphIdx].scale) >> 4;
            pad = (s16)(gKbd->drawScale * gKbdBoxSizes[box->kind].w) - drawW;
            if (i == cursorKey) {
                if (gKbdCursorHideFrames == 0 && (gKbd->offGrid == 0 || Kbd_ArrowsOnPage() == 1)) {
                    color = KBD_COLW(5);
                } else if (cursorKey == modeKey && i < 4) {
                    color = KBD_COLW(5);
                } else {
                    color = KBD_COLW(3);
                }
            } else if (i == modeKey) {
                color = KBD_COLW(5);
            }
            Kbd_DrawSprite(color, (f32)(pad >> 1) + (x + box->x * gKbd->drawScale), y + box->y + 16 - glyphH, drawW, glyphH,
                           ((code & 0xF) << 4) + 1, (code & 0xF0) + 1,
                           ((code & 0xF) << 4) - 1 + glyphW, (code & 0xF0) + 16);
        }
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Kbd_DrawLine(2.0f + (14.0f + x) / gKbd->drawScale, y + 0);
    Kbd_DrawCandidateBox(2.0f + (14.0f + x) / gKbd->drawScale, y - 1);
    Text_DrawCurrentPage();
}

/*
 * Runs a switch key (KBD_ACT_SWITCH; its kind in ch.b[1]). When
 * Kbd_FindKindEx refuses the kind, SE_SYS_REFUSE. A mode key switches to
 * the key set Kbd_FindKindEx chose (a phrase or gestures page puts the
 * cursor on its home key); the page keys open those pages; the width keys
 * toggle the width and the shift keys the shift.
 */
void Kbd_PressKey(KbdKey *key)
{
    if (Kbd_FindKindEx(key->ch.b[1], &gKbd->mode, &gKbd->keySet) != 0) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    gKbd->cyclePos = 0;
    gKbd->gestureMode = 0;
    Kbd_UpdateSymbolMode();
    switch (key->ch.b[1]) {
    case KBD_MODE_GESTURE:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_EnterGestures();
        Kbd_RefreshLayout();
        gKbd->state = SE_SYS_SELECT;
        break;
    case KBD_MODE_SYMBOL:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        if (gKbd->symbolMode != 0) {
            Kbd_ConvertOrCommit();
            Kbd_KeyRepeat(0, 0);
        } else {
            gKbd->state = SE_SYS_SELECT;
        }
        Kbd_RefreshLayout();
        break;
    case KBD_MODE_PHRASE1:
        Kbd_OpenPhrases1();
        break;
    case KBD_MODE_PHRASE2:
        Kbd_OpenPhrases2();
        break;
    case KBD_MODE_HIRAGANA:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_RefreshLayout();
        gKbd->state = SE_SYS_SELECT;
        break;
    case KBD_MODE_KATAKANA:
    case KBD_MODE_ALNUM:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_RefreshLayout();
        gKbd->state = SE_SYS_SELECT;
        break;
    case KBD_SWITCH_TO_HALF:
    case KBD_SWITCH_TO_FULL:
        Kbd_ToggleWidth();
        break;
    case KBD_SWITCH_SHIFT:
    case KBD_SWITCH_UNSHIFT:
        Kbd_ToggleShift();
        break;
    case 10:
    case 11:
    case 12:
        break;
    }
}

void Kbd_RefreshLayout(void)
{
    Kbd_PickLayout();
    Kbd_LookupKey();
}

/*
 * KBD_MARK_DAKUTEN when the key types the dakuten mark, KBD_MARK_HANDAKUTEN
 * the handakuten mark, else 0.
 */
s32 Kbd_DakutenKeyKind(KbdKey *kbdKey)
{
    u8 trailByte;

    trailByte = kbdKey->ch.b[1];
    if ((trailByte == gKbdDakutenSjis_1) && (kbdKey->ch.b[0] == gKbdDakutenSjis[0])) {
        return KBD_MARK_DAKUTEN;
    }
    if ((trailByte == gKbdHandakutenSjis_1) && (kbdKey->ch.b[0] == gKbdHandakutenSjis[0])) {
        return KBD_MARK_HANDAKUTEN;
    }
    return 0;
}

extern void *gKanaDakutenTable;

s32 Kana_AddDakuten(void)
{
    return Kana_SwapBeforeCursor(gKanaDakutenTable);
}


s32 Kana_AddHandakuten(void)
{
    return Kana_SwapBeforeCursor(gKanaHandakutenTable);
}

s8 Kbd_IsOpen(void)
{
    return gKbd->open;
}

/*
 * When the keyboard is open: closes it with KBD_RESULT_CANCEL and saves the
 * key set; plays SE_SYS_CANCEL unless the last Kbd_Run played a sound.
 */
void Kbd_Cancel(void)
{
    if ((s8)Kbd_IsOpen() != 0) {
        Kbd_ClearOpen();
        Kbd_ClearActive();
        gKbd->result = KBD_RESULT_CANCEL;
        if (gKbd->state == KBD_SOUND_NONE) {
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        Kbd_SaveKeySet();
    }
}

/* Kbd_Cancel without the sound. */
void Kbd_Close(void)
{
    if (Kbd_IsOpen() != 0) {
        Kbd_ClearOpen();
        Kbd_ClearActive();
        gKbd->result = KBD_RESULT_CANCEL;
        Kbd_SaveKeySet();
    }
}

void Kbd_SetPos(s32 posY, f32 posX)
{
    gKbd->x = posX;
    gKbd->y = (s16) posY;
}

/*
 * Writes the keyboard's position to *x and *y, or (0, 0) without a keyboard
 * (gKbd NULL).
 */
void Kbd_GetPos(f32 *x, s16 *y)
{
    if (gKbd != 0) {
        *x = gKbd->x;
        *y = gKbd->y;
        return;
    }
    *x = 0.0f;
    *y = 0;
}

void Kbd_SetSubmitWait(s32 wait)
{
    gKbd->submitWait = wait;
}

void Kbd_SetActive(void)
{
    gKbd->active = 1;
    gKbd->offGrid = 0;
}

void Kbd_ClearActive(void)
{
    gKbd->active = 0;
    gKbd->offGrid = 1;
}

void Kbd_SetOpen(void)
{
    gKbd->open = 1;
}

void Kbd_ClearOpen(void)
{
    gKbd->open = 0;
}

/*
 * Chooses the key set a mode key switches to. firstSet is the key's kind (a
 * KBD_MODE_*, which is also the mode's first set). It takes the first set,
 * in this order, that KbdEntry.flags doesn't bar: hiragana, small hiragana;
 * katakana, small, half-width, half-width small; the alphabets (capitals
 * first while Caps Lock is on), then the digits; the symbols, then the
 * half-width ones; other kinds only themselves. Writes the first set to
 * *outMode and the set taken to *outKeySet (either may be NULL) and returns
 * 0; returns 1 when every set is barred, or for a page the kind lacks.
 */
s32 Kbd_FindKind(u8 firstSet, u8 *outMode, u8 *outKeySet)
{
    u8 dummy;
    u8 secondAlt;
    u8 lastAlt;
    u8 firstAlt;
    u8 thirdAlt;
    s32 mask;

    if (outMode == NULL) {
        outMode = &dummy;
    }
    if (outKeySet == NULL) {
        outKeySet = &dummy;
    }
    switch (firstSet) {
    case KBD_MODE_HIRAGANA:
        secondAlt = firstSet;
        lastAlt = firstSet;
        firstAlt = firstSet | 8;
        thirdAlt = firstSet;
        break;
    case KBD_MODE_KATAKANA:
        secondAlt = firstSet + 5;
        lastAlt = firstSet + 5;
        firstAlt = firstSet | 8;
        thirdAlt = firstSet + 13;
        break;
    case KBD_MODE_ALNUM:
        if (gKbdSettings & USB_LED_CAPS_LOCK) {
            secondAlt = firstSet + 5;
            lastAlt = 16;
            firstAlt = firstSet | 8;
            thirdAlt = firstSet + 13;
        } else {
            firstAlt = 2;
            firstSet = 10;
            secondAlt = 15;
            lastAlt = 16;
            thirdAlt = 7;
        }
        break;
    case KBD_MODE_SYMBOL:
        secondAlt = firstSet + 8;
        lastAlt = firstSet + 8;
        firstAlt = firstSet;
        thirdAlt = firstSet;
        break;
    case KBD_MODE_GESTURE:
        if (*(s32 *)(gKbd->kindConf + 40) == 0) {
            return 1;
        }
        goto one;
    case KBD_MODE_PHRASE1:
    case KBD_MODE_PHRASE2:
        if (*(s32 *)(gKbd->kindConf + 48) == 0) {
            return 1;
        }
    default:
    one:
        thirdAlt = firstSet;
        firstAlt = firstSet;
        lastAlt = firstSet;
        secondAlt = firstSet;
        break;
    }
    mask = gKbd->flags;
    if (mask & (1 << firstSet)) {
        if (mask & (1 << firstAlt)) {
            if (mask & (1 << secondAlt)) {
                if (mask & (1 << thirdAlt)) {
                    if (!(mask & (1 << lastAlt))) {
                        *outMode = firstSet;
                        *outKeySet = lastAlt;
                        return 0;
                    }
                } else {
                    *outMode = firstSet;
                    *outKeySet = thirdAlt;
                    return 0;
                }
            } else {
                *outMode = firstSet;
                *outKeySet = secondAlt;
                return 0;
            }
        } else {
            *outMode = firstSet;
            *outKeySet = firstAlt;
            return 0;
        }
    } else {
        *outMode = firstSet;
        *outKeySet = firstSet;
        return 0;
    }
    return 1;
}

/*
 * Kbd_FindKind for the mode keys; for the switch keys, 0 when the key works
 * on the key set (*outKeySet, or keySet when it is NULL), else 1: a width
 * key needs an allowed width pair, KBD_SWITCH_SHIFT an unshifted set and
 * KBD_SWITCH_UNSHIFT a shifted one with an allowed pair.
 */
s32 Kbd_FindKindEx(u8 kind, u8 *outMode, u8 *outKeySet)
{
    u8 set;

    if (kind <= KBD_MODE_PHRASE2 || kind == KBD_MODE_GESTURE) {
        return Kbd_FindKind(kind, outMode, outKeySet);
    }
    if (outKeySet == NULL) {
        set = gKbd->keySet;
    } else {
        set = *outKeySet;
    }
    switch (kind) {
    case KBD_SWITCH_TO_HALF:
    case KBD_SWITCH_TO_FULL:
        if ((s8)Kbd_WidthPair(set) >= 0) {
            return 0;
        }
        break;
    case KBD_SWITCH_SHIFT:
        if ((s8)Kbd_ShiftPair(set) >= 0 && !(gKbd->keySet & KBD_SET_SHIFTED)) {
            return 0;
        }
        break;
    case KBD_SWITCH_UNSHIFT:
        if ((s8)Kbd_ShiftPair(set) >= 0 && (gKbd->keySet & KBD_SET_SHIFTED)) {
            return 0;
        }
        break;
    }
    return 1;
}

void Kbd_UpdateMode(void)
{
    gKbd->mode = (u8) gKbdSetModes[gKbd->keySet];
}

extern s8 gKbdShiftSets[];

/*
 * The shift pair of key set set (gKbdShiftSets), or -1 when it has none or
 * it is barred.
 */
s32 Kbd_ShiftPair(u8 set)
{
    s32 pair = gKbdShiftSets[set];

    if (pair >= 0 && !(gKbd->flags & (1 << (s8)pair))) {
        return pair;
    }
    return -1;
}

/*
 * Switches keySet to its shift pair, keeping the cursor, with SE_SYS_SELECT;
 * SE_SYS_REFUSE when there is none.
 */
void Kbd_ToggleShift(void)
{
    s8 pair;
    u8 savedCol;
    u8 savedRow;

    pair = Kbd_ShiftPair(gKbd->keySet);
    if (pair >= 0) {
        savedCol = gKbd->col;
        savedRow = gKbd->row;
        gKbd->keySet = pair;
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_RefreshLayout();
        Kbd_UpdateSymbolMode();
        gKbd->state = SE_SYS_SELECT;
        gKbd->col = savedCol;
        gKbd->row = savedRow;
        Kbd_LookupKey();
        return;
    }
    gKbd->state = SE_SYS_REFUSE;
}


/*
 * Steps through the phrase and gestures pages: from another set, the first
 * phrase page; then by the last page opened (gKbdPhrasePage): the first,
 * the second, then the gestures page for entry kind 0 (gKbdKindIs0) or the
 * first again. Leaving a page during a cycle clears the text and ends it.
 */
void Kbd_NextPhrasePage(void)
{
    switch (gKbd->keySet) {
    case KBD_SET_GESTURE:
    case KBD_SET_PHRASE1:
    case KBD_SET_PHRASE2:
        break;
    default:
        Kbd_OpenPhrases1();
        return;
    }
    switch (gKbdPhrasePage) {
    case 0:
        Kbd_OpenPhrases1();
        break;
    case KBD_SET_PHRASE1:
        if (gKbd->keyFlags & KBD_KEYF_CYCLING) {
            gKbd->cyclePos = 1;
            Kbd_ClearText();
            gKbd->keyFlags &= (u8)~KBD_KEYF_CYCLING;
        }
        Kbd_OpenPhrases2();
        break;
    case KBD_SET_PHRASE2:
        if (gKbd->keyFlags & KBD_KEYF_CYCLING) {
            gKbd->cyclePos = 1;
            Kbd_ClearText();
            gKbd->keyFlags &= (u8)~KBD_KEYF_CYCLING;
        }
        if (gKbdKindIs0 == 0) {
            gKbd->keySet = KBD_SET_PHRASE1;
            Kbd_OpenPhrases1();
        } else {
            gKbd->keySet = KBD_SET_GESTURE;
            Kbd_UpdateMode();
            gKbd->col = 5;
            gKbd->row = 0;
            Kbd_EnterGestures();
            Kbd_RefreshLayout();
            gKbd->state = SE_SYS_SELECT;
        }
        break;
    case KBD_SET_GESTURE:
        if (gKbd->keyFlags & KBD_KEYF_CYCLING) {
            gKbd->cyclePos = 1;
            Kbd_ClearText();
            gKbd->keyFlags &= (u8)~KBD_KEYF_CYCLING;
        }
        Kbd_OpenPhrases1();
        break;
    }
}

/*
 * On a phrase or gestures page, puts the cursor on the page's home key
 * (column 5, row 0).
 */
void Kbd_HomePageCursor(void)
{
    switch (gKbd->mode) {
    case KBD_MODE_GESTURE:
    case KBD_MODE_PHRASE1:
    case KBD_MODE_PHRASE2:
        gKbd->col = 5;
        gKbd->row = 0;
        break;
    }
}

extern KbdLayout gKbdAltLayouts[];
extern KbdLayout gKbdAltLayouts_1;
extern KbdLayout gKbdAltLayouts_2;
extern KbdLayout gKbdAltLayouts_3;

/*
 * Sets layout for the entry kind and the key set: kind 7 the second ASCII
 * capitals layout; kinds 8-10 the alphabets without their symbol keys and
 * a blank symbol page; kind 6 the ASCII capitals; others the set's own. On
 * the gestures page typing kana it takes the hiragana layout, or the ASCII
 * lower case or capitals when hiragana is barred.
 */
void Kbd_PickLayout(void)
{
    u8 *set;

    switch (gKbd->kind) {
    case 7:
        gKbd->layout = &gKbdSetLayouts[KBD_LAYOUT_ASCII_UPPER2];
        break;
    case 8:
    case 9:
    case 10:
        set = &gKbd->keySet;
        switch (*set) {
        case KBD_SET_WIDE_UPPER:
            gKbd->layout = &gKbdSetLayouts[KBD_LAYOUT_WIDE_UPPER_NOSYM];
            break;
        case KBD_SET_WIDE_LOWER:
            gKbd->layout = &gKbdSetLayouts[KBD_LAYOUT_WIDE_LOWER_NOSYM];
            break;
        case KBD_SET_ASCII_UPPER:
            gKbd->layout = gKbdAltLayouts;
            break;
        case KBD_SET_ASCII_LOWER:
            gKbd->layout = &gKbdAltLayouts_1;
            break;
        case KBD_SET_SYMBOL:
            *set = KBD_SET_HALF_SYMBOL;
        case KBD_SET_HALF_SYMBOL:
            gKbd->layout = &gKbdAltLayouts_2;
            break;
        default:
            gKbd->layout = &gKbdSetLayouts[*set];
            break;
        }
        break;
    case 6:
        gKbd->layout = &gKbdAltLayouts_3;
        break;
    default:
        gKbd->layout = &gKbdSetLayouts[gKbd->keySet];
        break;
    }
    if (gKbd->mode == KBD_MODE_GESTURE && (gKbd->gestureMode & KBD_GESTURE_KANA)) {
        if (!(gKbd->flags & KBD_SET_BIT(KBD_SET_HIRA))) {
            gKbd->layout = &gKbdSetLayouts[KBD_SET_HIRA];
        } else if (!(gKbd->flags & KBD_SET_BIT(KBD_SET_ASCII_LOWER))) {
            gKbd->layout = &gKbdSetLayouts[KBD_SET_ASCII_LOWER];
        } else {
            gKbd->layout = &gKbdSetLayouts[KBD_SET_ASCII_UPPER];
        }
    }
}

/*
 * Prepares the gestures page: records it as the last page opened and
 * commits the kana; sets KBD_GESTURE_KANA when Kbd_CheckGestureText is negative
 * (it never is).
 */
void Kbd_EnterGestures(void)
{
    Kbd_SetPhrasePage(13U);
    Kbd_CommitKana();
    if (Kbd_CheckGestureText(gKbd->text, &gKbd->textPos) < 0) {
        gKbd->gestureMode = (u8) (gKbd->gestureMode | KBD_GESTURE_KANA);
    }
}

/*
 * Clears gKbdState.polled, through a symbol of its own for that byte as
 * the original addresses it, so that Kbd_ReadUsb reads the USB keyboard
 * again; the frame's pad
 * update calls it.
 */
void Kbd_ClearPolled(void)
{
    D_005AEAD7 = 0;
}

/*
 * 1 when the key types a full-width space or, on the hiragana and katakana
 * sets, one of their eleven punctuation marks (by glyph); else 0. Entry
 * kinds 8-10 refuse these (Kbd_CharBlocked).
 */
s32 Kbd_CharIsSpecialKana(KbdKey *key)
{
    char buf[4];
    s32 found = 0;

    if (gKbd->keySet == KBD_SET_HIRA || gKbd->keySet == KBD_SET_KATA) {
        switch (key->glyph) {
        case 152:
        case 153:
        case 168:
        case 181:
        case 182:
        case 183:
        case 184:
        case 185:
        case 224:
        case 225:
        case 243:
            found = 1;
            break;
        }
    }
    strncpy(buf, (char *)key->ch.b, 2);
    buf[3] = 0;
    if (strncmp(buf, D_0046CC08, 2) == 0) {
        found = 1;
    }
    return found;
}

/*
 * 1 when the key can't be used now (Kbd_Draw shows the blank glyph): no
 * character; a switch key Kbd_FindKindEx refuses; for entry kinds 8-10 the
 * convert key and Kbd_CharIsSpecialKana's characters; for the other kinds
 * any key but backspace and submit whose character the kind refuses (kind
 * 15 Kbd_CharInSet5_Unk's, 1, 2 and 5 Kbd_CharInSet4_Unk's, 7 all but
 * digits, the hash and the asterisk, 6 all but digits and letters).
 */
s32 Kbd_CharBlocked(KbdKey *key)
{
    s32 blocked = 0;

    if (key->ch.code == 0) {
        blocked = 1;
    } else if (key->ch.b[0] == KBD_ACT_SWITCH) {
        if (Kbd_FindKindEx(key->ch.b[1], NULL, NULL) != 0) {
            blocked = 1;
        }
    } else if (Kbd_KindIs8To10() == 1) {
        if (key->ch.b[0] == KBD_ACT_COMMAND && key->ch.b[1] == 1) {
            blocked = 1;
        } else if (Kbd_CharIsSpecialKana(key) != 0) {
            blocked = 1;
        }
    } else if (key->glyph != 209 && key->glyph != 125) {
        switch (gKbd->kind) {
        case 15:
            if (Kbd_CharInSet5_Unk(key) != 0) {
                blocked = 1;
            }
            break;
        case 1:
        case 5:
        case 2:
            if (Kbd_CharInSet4_Unk(key) != 0) {
                blocked = 1;
            }
            break;
        case 7:
            if (Kbd_CharIsPlain(key) != 0) {
                blocked = 1;
            }
            break;
        case 6:
            if (Kbd_CharInRows01(key) == 0 && Kbd_CharInRowsHigh(key) == 0) {
                blocked = 1;
            }
            break;
        }
    }
    return blocked;
}

/*
 * 1 when the key's glyph is the percent sign, a corner bracket, the middle
 * dot or the apostrophe (refused by entry kind 15), else 0.
 */
s32 Kbd_CharInSet5_Unk(KbdKey *key)
{
    s32 found = 0;

    switch (key->glyph) {
    case 227:
    case 181:
    case 182:
    case 185:
    case 241:
        found = 1;
        break;
    }
    return found;
}

/*
 * 1 when the key's glyph is the percent sign, a corner bracket or the
 * middle dot (refused by entry kinds 1, 2 and 5), else 0.
 */
s32 Kbd_CharInSet4_Unk(KbdKey *key)
{
    s32 found = 0;

    switch (key->glyph) {
    case 227:
    case 181:
    case 182:
    case 185:
        found = 1;
        break;
    }
    return found;
}

/* 1 when the key's glyph is a digit (glyph cells 0-4, 16-20). */
s32 Kbd_CharInRows01(KbdKey *key)
{
    u8 glyphCode;

    glyphCode = key->glyph;
    if (((s32) glyphCode >= 0) && ((s32) glyphCode < 5)) {
        return 1;
    }
    if (((s32) glyphCode >= 16) && ((s32) glyphCode < 21)) {
        return 1;
    }
    return 0;
}

/* 1 when the key's glyph is a letter (glyph cells 32-100 and 112). */
s32 Kbd_CharInRowsHigh(KbdKey *key)
{
    s32 glyphCode;

    glyphCode = key->glyph;
    if (((s32) glyphCode >= 32) && ((s32) glyphCode < 37)) {
        return 1;
    }
    if (((s32) glyphCode >= 48) && ((s32) glyphCode < 53)) {
        return 1;
    }
    if (((s32) glyphCode >= 64) && ((s32) glyphCode < 69)) {
        return 1;
    }
    if (((s32) glyphCode >= 80) && ((s32) glyphCode < 85)) {
        return 1;
    }
    if (((s32) glyphCode >= 96) && ((s32) glyphCode < 101)) {
        return 1;
    }
    if (glyphCode == 112) {
        return 1;
    }
    return 0;
}

/*
 * 1 when entry kind 7 refuses the key: its glyph is none of the digits, the
 * hash, the asterisk and the blank glyph; else 0.
 */
s32 Kbd_CharIsPlain(KbdKey *key)
{
    switch (key->glyph) {
    case 226:
    case 197:
    case 240:
        return 0;
    }
    if (Kbd_CharInRows01(key) != 1) {
        goto yes; /* a plain if/return pair becomes a set-on-condition */
    }
    return 0;
yes:
    return 1;
}

void Kbd_SaveKanaInput(void)
{
    gKbdSavedKanaInput = gKbd->kanaInput;
}

/* printf to the screen (Font_Printf with "%s"), unless gTextOff. */
void Kbd_Printf(const char *fmt, ...)
{
    char buf[1024];
    va_list args;

    if (gTextOff == 0) {
        va_start(args, fmt);
        vsnprintf(buf, 1023, fmt, args);
        buf[1023] = 0;
        Font_Printf(D_0046CC10, buf);
    }
}

/* This file's initialised data. */
extern char gKanaNChars_0[];
extern char gKanaNChars_1[];
extern char gKanaNChars_2[];
extern char gKbdSymbolCands_0[];
extern char gKbdSymbolCands_1[];
extern char gKbdSymbolCands_2[];
extern char gKbdSymbolCands_3[];
extern char gKbdSymbolCands_4[];
extern char gKbdSymbolCands_5[];
extern char gKbdSymbolCands_6[];
extern char gKbdSymbolCands_7[];
extern char gKbdSymbolCands_8[];
extern char gKbdSymbolCands_9[];
extern char gKbdSymbolCands_10[];
extern char gKbdSymbolCands_11[];
extern char gKbdSymbolCands_12[];
extern char gKbdSymbolCands_13[];
extern char gKbdSymbolCands_14[];
extern char gKbdSymbolCands_15[];
extern char gKbdSymbolCands_16[];
extern char gKbdSymbolCands_17[];
extern char gKbdSymbolCands_18[];
extern char gKbdSymbolCands_19[];
extern char gKbdSymbolCands_20[];
extern char gKbdSymbolCands_21[];
extern char gKbdSymbolCands_22[];
extern char gKbdSymbolCands_23[];
extern char gKbdSymbolCands_24[];
extern char gKbdSymbolCands_25[];
extern char gKbdSymbolCands_26[];
extern char gKbdSymbolCands_27[];
extern char gKbdSymbolCands_28[];
extern char gKbdSymbolCands_29[];
extern char gKbdSymbolCands_30[];
extern char gKbdSymbolCands_31[];
extern char gKbdSymbolCands_32[];
extern char gKbdSymbolCands_33[];
extern char gKbdSymbolCands_34[];
RomajiEntry gRomajiKanaTbl[240] = {
    {gRomajiKanaTbl_0_romaji, {gRomajiKanaTbl_0_kana_0, gRomajiKanaTbl_0_kana_1, gRomajiKanaTbl_0_kana_2}},
    {gRomajiKanaTbl_1_romaji, {D_0046A958, D_0046A960, D_0046A968}},
    {gRomajiKanaTbl_2_romaji, {D_0046A978, D_0046A980, D_0046A988}},
    {gRomajiKanaTbl_3_romaji, {gRomajiKanaTbl_3_kana_0, gRomajiKanaTbl_3_kana_1, gRomajiKanaTbl_3_kana_2}},
    {gRomajiKanaTbl_4_romaji, {gRomajiKanaTbl_4_kana_0, gRomajiKanaTbl_4_kana_1, gRomajiKanaTbl_4_kana_2}},
    {gRomajiKanaTbl_5_romaji, {D_0046A958, D_0046A960, D_0046A968}},
    {gRomajiKanaTbl_6_romaji, {D_0046A978, D_0046A980, D_0046A988}},
    {gRomajiKanaTbl_7_romaji, {D_0046A9E8, D_0046A9F0, D_0046A9F8}},
    {gRomajiKanaTbl_8_romaji, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {gRomajiKanaTbl_9_romaji, {D_0046AA28, D_0046AA30, D_0046AA38}},
    {gRomajiKanaTbl_10_romaji, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {gRomajiKanaTbl_11_romaji, {D_0046AA68, D_0046AA70, D_0046AA78}},
    {gRomajiKanaTbl_12_romaji, {D_0046A9E8, D_0046A9F0, D_0046A9F8}},
    {gRomajiKanaTbl_13_romaji, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {gRomajiKanaTbl_14_romaji, {D_0046AA28, D_0046AA30, D_0046AA38}},
    {gRomajiKanaTbl_15_romaji, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {gRomajiKanaTbl_16_romaji, {D_0046AA68, D_0046AA70, D_0046AA78}},
    {gRomajiKanaTbl_17_romaji, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {gRomajiKanaTbl_18_romaji, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {gRomajiKanaTbl_19_romaji, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {gRomajiKanaTbl_20_romaji, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {gRomajiKanaTbl_21_romaji, {gRomajiKanaTbl_21_kana_0, gRomajiKanaTbl_21_kana_1, gRomajiKanaTbl_21_kana_2}},
    {gRomajiKanaTbl_22_romaji, {D_0046AAF0, D_0046AAF8, D_0046AB00}},
    {gRomajiKanaTbl_23_romaji, {gRomajiKanaTbl_23_kana_0, gRomajiKanaTbl_23_kana_1, gRomajiKanaTbl_23_kana_2}},
    {gRomajiKanaTbl_24_romaji, {D_0046AB30, D_0046AB38, D_0046AB40}},
    {gRomajiKanaTbl_25_romaji, {gRomajiKanaTbl_25_kana_0, gRomajiKanaTbl_25_kana_1, gRomajiKanaTbl_25_kana_2}},
    {gRomajiKanaTbl_26_romaji, {D_0046AB70, D_0046AB78, D_0046AB80}},
    {gRomajiKanaTbl_27_romaji, {D_0046AAF0, D_0046AAF8, D_0046AB00}},
    {gRomajiKanaTbl_28_romaji, {D_0046AB30, D_0046AB38, D_0046AB40}},
    {gRomajiKanaTbl_29_romaji, {D_0046AB70, D_0046AB78, D_0046AB80}},
    {gRomajiKanaTbl_30_romaji, {gRomajiKanaTbl_30_kana_0, gRomajiKanaTbl_30_kana_1, gRomajiKanaTbl_30_kana_2}},
    {gRomajiKanaTbl_31_romaji, {gRomajiKanaTbl_31_kana_0, gRomajiKanaTbl_31_kana_1, gRomajiKanaTbl_31_kana_2}},
    {gRomajiKanaTbl_32_romaji, {gRomajiKanaTbl_32_kana_0, gRomajiKanaTbl_32_kana_1, gRomajiKanaTbl_32_kana_2}},
    {gRomajiKanaTbl_33_romaji, {gRomajiKanaTbl_33_kana_0, gRomajiKanaTbl_33_kana_1, gRomajiKanaTbl_33_kana_2}},
    {gRomajiKanaTbl_34_romaji, {gRomajiKanaTbl_34_kana_0, gRomajiKanaTbl_34_kana_1, gRomajiKanaTbl_34_kana_2}},
    {gRomajiKanaTbl_35_romaji, {gRomajiKanaTbl_35_kana_0, gRomajiKanaTbl_35_kana_1, gRomajiKanaTbl_35_kana_2}},
    {gRomajiKanaTbl_36_romaji, {gRomajiKanaTbl_36_kana_0, gRomajiKanaTbl_36_kana_1, gRomajiKanaTbl_36_kana_2}},
    {gRomajiKanaTbl_37_romaji, {gRomajiKanaTbl_37_kana_0, gRomajiKanaTbl_37_kana_1, gRomajiKanaTbl_37_kana_2}},
    {gRomajiKanaTbl_38_romaji, {gRomajiKanaTbl_38_kana_0, gRomajiKanaTbl_38_kana_1, gRomajiKanaTbl_38_kana_2}},
    {gRomajiKanaTbl_39_romaji, {gRomajiKanaTbl_39_kana_0, gRomajiKanaTbl_39_kana_1, gRomajiKanaTbl_39_kana_2}},
    {gRomajiKanaTbl_40_romaji, {gRomajiKanaTbl_40_kana_0, gRomajiKanaTbl_40_kana_1, gRomajiKanaTbl_40_kana_2}},
    {gRomajiKanaTbl_41_romaji, {gRomajiKanaTbl_41_kana_0, gRomajiKanaTbl_41_kana_1, gRomajiKanaTbl_41_kana_2}},
    {gRomajiKanaTbl_42_romaji, {gRomajiKanaTbl_42_kana_0, gRomajiKanaTbl_42_kana_1, gRomajiKanaTbl_42_kana_2}},
    {gRomajiKanaTbl_43_romaji, {gRomajiKanaTbl_43_kana_0, gRomajiKanaTbl_43_kana_1, gRomajiKanaTbl_43_kana_2}},
    {gRomajiKanaTbl_44_romaji, {D_0046AD68, D_0046AD70, D_0046AD78}},
    {gRomajiKanaTbl_45_romaji, {D_0046AB30, D_0046AB38, D_0046AB40}},
    {gRomajiKanaTbl_46_romaji, {D_0046AD90, D_0046AD98, D_0046ADA0}},
    {gRomajiKanaTbl_47_romaji, {gRomajiKanaTbl_47_kana_0, gRomajiKanaTbl_47_kana_1, gRomajiKanaTbl_47_kana_2}},
    {gRomajiKanaTbl_48_romaji, {D_0046AD68, D_0046AD70, D_0046AD78}},
    {gRomajiKanaTbl_49_romaji, {D_0046AD90, D_0046AD98, D_0046ADA0}},
    {gRomajiKanaTbl_50_romaji, {gRomajiKanaTbl_50_kana_0, gRomajiKanaTbl_50_kana_1, gRomajiKanaTbl_50_kana_2}},
    {gRomajiKanaTbl_51_romaji, {gRomajiKanaTbl_51_kana_0, gRomajiKanaTbl_51_kana_1, gRomajiKanaTbl_51_kana_2}},
    {gRomajiKanaTbl_52_romaji, {gRomajiKanaTbl_52_kana_0, gRomajiKanaTbl_52_kana_1, gRomajiKanaTbl_52_kana_2}},
    {gRomajiKanaTbl_53_romaji, {gRomajiKanaTbl_53_kana_0, gRomajiKanaTbl_53_kana_1, gRomajiKanaTbl_53_kana_2}},
    {gRomajiKanaTbl_54_romaji, {gRomajiKanaTbl_54_kana_0, gRomajiKanaTbl_54_kana_1, gRomajiKanaTbl_54_kana_2}},
    {gRomajiKanaTbl_55_romaji, {gRomajiKanaTbl_55_kana_0, gRomajiKanaTbl_55_kana_1, gRomajiKanaTbl_55_kana_2}},
    {gRomajiKanaTbl_56_romaji, {gRomajiKanaTbl_56_kana_0, gRomajiKanaTbl_56_kana_1, gRomajiKanaTbl_56_kana_2}},
    {gRomajiKanaTbl_57_romaji, {gRomajiKanaTbl_57_kana_0, gRomajiKanaTbl_57_kana_1, gRomajiKanaTbl_57_kana_2}},
    {gRomajiKanaTbl_58_romaji, {gRomajiKanaTbl_58_kana_0, gRomajiKanaTbl_58_kana_1, gRomajiKanaTbl_58_kana_2}},
    {gRomajiKanaTbl_59_romaji, {gRomajiKanaTbl_59_kana_0, gRomajiKanaTbl_59_kana_1, gRomajiKanaTbl_59_kana_2}},
    {gRomajiKanaTbl_60_romaji, {D_0046AF20, D_0046AF28, D_0046AF30}},
    {gRomajiKanaTbl_61_romaji, {gRomajiKanaTbl_61_kana_0, gRomajiKanaTbl_61_kana_1, gRomajiKanaTbl_61_kana_2}},
    {gRomajiKanaTbl_62_romaji, {D_0046AF60, D_0046AF68, D_0046AF70}},
    {gRomajiKanaTbl_63_romaji, {gRomajiKanaTbl_63_kana_0, gRomajiKanaTbl_63_kana_1, gRomajiKanaTbl_63_kana_2}},
    {gRomajiKanaTbl_64_romaji, {D_0046AF20, D_0046AF28, D_0046AF30}},
    {gRomajiKanaTbl_65_romaji, {D_0046AF60, D_0046AF68, D_0046AF70}},
    {gRomajiKanaTbl_66_romaji, {gRomajiKanaTbl_66_kana_0, gRomajiKanaTbl_66_kana_1, gRomajiKanaTbl_66_kana_2}},
    {gRomajiKanaTbl_67_romaji, {D_0046AFD0, D_0046AFD8, D_0046AFE0}},
    {gRomajiKanaTbl_68_romaji, {gRomajiKanaTbl_68_kana_0, gRomajiKanaTbl_68_kana_1, gRomajiKanaTbl_68_kana_2}},
    {gRomajiKanaTbl_69_romaji, {gRomajiKanaTbl_69_kana_0, gRomajiKanaTbl_69_kana_1, gRomajiKanaTbl_69_kana_2}},
    {gRomajiKanaTbl_70_romaji, {gRomajiKanaTbl_70_kana_0, gRomajiKanaTbl_70_kana_1, gRomajiKanaTbl_70_kana_2}},
    {gRomajiKanaTbl_71_romaji, {D_0046AFD0, D_0046AFD8, D_0046AFE0}},
    {gRomajiKanaTbl_72_romaji, {D_0046AF20, D_0046AF28, D_0046AF30}},
    {gRomajiKanaTbl_73_romaji, {D_0046B060, D_0046B068, D_0046B070}},
    {gRomajiKanaTbl_74_romaji, {gRomajiKanaTbl_74_kana_0, gRomajiKanaTbl_74_kana_1, gRomajiKanaTbl_74_kana_2}},
    {gRomajiKanaTbl_75_romaji, {D_0046B0A0, D_0046B0A8, D_0046B0B0}},
    {gRomajiKanaTbl_76_romaji, {D_0046B0C0, D_0046B0C8, D_0046B0D0}},
    {gRomajiKanaTbl_77_romaji, {D_0046B0E0, D_0046B0E8, D_0046B0F0}},
    {gRomajiKanaTbl_78_romaji, {D_0046B060, D_0046B068, D_0046B070}},
    {gRomajiKanaTbl_79_romaji, {D_0046B0A0, D_0046B0A8, D_0046B0B0}},
    {gRomajiKanaTbl_80_romaji, {D_0046B0C0, D_0046B0C8, D_0046B0D0}},
    {gRomajiKanaTbl_81_romaji, {D_0046B0E0, D_0046B0E8, D_0046B0F0}},
    {gRomajiKanaTbl_82_romaji, {D_0046B120, D_0046B128, D_0046B130}},
    {gRomajiKanaTbl_83_romaji, {D_0046B140, D_0046B148, D_0046B150}},
    {gRomajiKanaTbl_84_romaji, {D_0046B160, D_0046B168, D_0046B170}},
    {gRomajiKanaTbl_85_romaji, {D_0046B180, D_0046B188, D_0046B190}},
    {gRomajiKanaTbl_86_romaji, {D_0046B1A0, D_0046B1A8, D_0046B1B0}},
    {gRomajiKanaTbl_87_romaji, {D_0046B120, D_0046B128, D_0046B130}},
    {gRomajiKanaTbl_88_romaji, {D_0046B160, D_0046B168, D_0046B170}},
    {gRomajiKanaTbl_89_romaji, {D_0046B180, D_0046B188, D_0046B190}},
    {gRomajiKanaTbl_90_romaji, {D_0046B1A0, D_0046B1A8, D_0046B1B0}},
    {gRomajiKanaTbl_91_romaji, {D_0046B120, D_0046B128, D_0046B130}},
    {gRomajiKanaTbl_92_romaji, {D_0046B140, D_0046B148, D_0046B150}},
    {gRomajiKanaTbl_93_romaji, {D_0046B160, D_0046B168, D_0046B170}},
    {gRomajiKanaTbl_94_romaji, {D_0046B180, D_0046B188, D_0046B190}},
    {gRomajiKanaTbl_95_romaji, {D_0046B1A0, D_0046B1A8, D_0046B1B0}},
    {gRomajiKanaTbl_96_romaji, {gRomajiKanaTbl_96_kana_0, gRomajiKanaTbl_96_kana_1, gRomajiKanaTbl_96_kana_2}},
    {gRomajiKanaTbl_97_romaji, {D_0046B228, D_0046B230, D_0046B238}},
    {gRomajiKanaTbl_98_romaji, {D_0046B248, D_0046B250, D_0046B258}},
    {gRomajiKanaTbl_99_romaji, {gRomajiKanaTbl_99_kana_0, gRomajiKanaTbl_99_kana_1, gRomajiKanaTbl_99_kana_2}},
    {gRomajiKanaTbl_100_romaji, {gRomajiKanaTbl_100_kana_0, gRomajiKanaTbl_100_kana_1, gRomajiKanaTbl_100_kana_2}},
    {gRomajiKanaTbl_101_romaji, {D_0046B228, D_0046B230, D_0046B238}},
    {gRomajiKanaTbl_102_romaji, {D_0046B248, D_0046B250, D_0046B258}},
    {gRomajiKanaTbl_103_romaji, {gSmallTsuHira, gSmallTsuKata, gSmallTsuHalf}},
    {gRomajiKanaTbl_104_romaji, {gSmallTsuHira, gSmallTsuKata, gSmallTsuHalf}},
    {gRomajiKanaTbl_105_romaji, {gRomajiKanaTbl_105_kana_0, gRomajiKanaTbl_105_kana_1, gRomajiKanaTbl_105_kana_2}},
    {gRomajiKanaTbl_106_romaji, {gRomajiKanaTbl_106_kana_0, gRomajiKanaTbl_106_kana_1, gRomajiKanaTbl_106_kana_2}},
    {gRomajiKanaTbl_107_romaji, {gRomajiKanaTbl_107_kana_0, gRomajiKanaTbl_107_kana_1, gRomajiKanaTbl_107_kana_2}},
    {gRomajiKanaTbl_108_romaji, {gRomajiKanaTbl_108_kana_0, gRomajiKanaTbl_108_kana_1, gRomajiKanaTbl_108_kana_2}},
    {gRomajiKanaTbl_109_romaji, {gRomajiKanaTbl_109_kana_0, gRomajiKanaTbl_109_kana_1, gRomajiKanaTbl_109_kana_2}},
    {gRomajiKanaTbl_110_romaji, {D_0046B380, D_0046B388, D_0046B390}},
    {gRomajiKanaTbl_111_romaji, {D_0046B3A0, D_0046B3A8, D_0046B3B0}},
    {gRomajiKanaTbl_112_romaji, {D_0046B3C0, D_0046B3C8, D_0046B3D0}},
    {gRomajiKanaTbl_113_romaji, {D_0046B3E0, D_0046B3E8, D_0046B3F0}},
    {gRomajiKanaTbl_114_romaji, {D_0046B400, D_0046B408, D_0046B410}},
    {gRomajiKanaTbl_115_romaji, {D_0046B380, D_0046B388, D_0046B390}},
    {gRomajiKanaTbl_116_romaji, {D_0046B3C0, D_0046B3C8, D_0046B3D0}},
    {gRomajiKanaTbl_117_romaji, {D_0046B3E0, D_0046B3E8, D_0046B3F0}},
    {gRomajiKanaTbl_118_romaji, {D_0046B400, D_0046B408, D_0046B410}},
    {gRomajiKanaTbl_119_romaji, {D_0046B380, D_0046B388, D_0046B390}},
    {gRomajiKanaTbl_120_romaji, {D_0046B3A0, D_0046B3A8, D_0046B3B0}},
    {gRomajiKanaTbl_121_romaji, {D_0046B3C0, D_0046B3C8, D_0046B3D0}},
    {gRomajiKanaTbl_122_romaji, {D_0046B3E0, D_0046B3E8, D_0046B3F0}},
    {gRomajiKanaTbl_123_romaji, {D_0046B400, D_0046B408, D_0046B410}},
    {gRomajiKanaTbl_124_romaji, {gRomajiKanaTbl_124_kana_0, gRomajiKanaTbl_124_kana_1, gRomajiKanaTbl_124_kana_2}},
    {gRomajiKanaTbl_125_romaji, {gRomajiKanaTbl_125_kana_0, gRomajiKanaTbl_125_kana_1, gRomajiKanaTbl_125_kana_2}},
    {gRomajiKanaTbl_126_romaji, {gRomajiKanaTbl_126_kana_0, gRomajiKanaTbl_126_kana_1, gRomajiKanaTbl_126_kana_2}},
    {gRomajiKanaTbl_127_romaji, {gRomajiKanaTbl_127_kana_0, gRomajiKanaTbl_127_kana_1, gRomajiKanaTbl_127_kana_2}},
    {gRomajiKanaTbl_128_romaji, {gRomajiKanaTbl_128_kana_0, gRomajiKanaTbl_128_kana_1, gRomajiKanaTbl_128_kana_2}},
    {gRomajiKanaTbl_129_romaji, {gRomajiKanaTbl_129_kana_0, gRomajiKanaTbl_129_kana_1, gRomajiKanaTbl_129_kana_2}},
    {gRomajiKanaTbl_130_romaji, {gRomajiKanaTbl_130_kana_0, gRomajiKanaTbl_130_kana_1, gRomajiKanaTbl_130_kana_2}},
    {gRomajiKanaTbl_131_romaji, {gRomajiKanaTbl_131_kana_0, gRomajiKanaTbl_131_kana_1, gRomajiKanaTbl_131_kana_2}},
    {gRomajiKanaTbl_132_romaji, {gRomajiKanaTbl_132_kana_0, gRomajiKanaTbl_132_kana_1, gRomajiKanaTbl_132_kana_2}},
    {gRomajiKanaTbl_133_romaji, {gRomajiKanaTbl_133_kana_0, gRomajiKanaTbl_133_kana_1, gRomajiKanaTbl_133_kana_2}},
    {gRomajiKanaTbl_134_romaji, {gRomajiKanaTbl_134_kana_0, gRomajiKanaTbl_134_kana_1, gRomajiKanaTbl_134_kana_2}},
    {gRomajiKanaTbl_135_romaji, {gRomajiKanaTbl_135_kana_0, gRomajiKanaTbl_135_kana_1, gRomajiKanaTbl_135_kana_2}},
    {gRomajiKanaTbl_136_romaji, {gRomajiKanaTbl_136_kana_0, gRomajiKanaTbl_136_kana_1, gRomajiKanaTbl_136_kana_2}},
    {gRomajiKanaTbl_137_romaji, {gRomajiKanaTbl_137_kana_0, gRomajiKanaTbl_137_kana_1, gRomajiKanaTbl_137_kana_2}},
    {gRomajiKanaTbl_138_romaji, {gRomajiKanaTbl_138_kana_0, gRomajiKanaTbl_138_kana_1, gRomajiKanaTbl_138_kana_2}},
    {gRomajiKanaTbl_139_romaji, {gRomajiKanaTbl_139_kana_0, gRomajiKanaTbl_139_kana_1, gRomajiKanaTbl_139_kana_2}},
    {gRomajiKanaTbl_140_romaji, {gRomajiKanaTbl_140_kana_0, gRomajiKanaTbl_140_kana_1, gRomajiKanaTbl_140_kana_2}},
    {gRomajiKanaTbl_141_romaji, {gRomajiKanaTbl_141_kana_0, gRomajiKanaTbl_141_kana_1, gRomajiKanaTbl_141_kana_2}},
    {gRomajiKanaTbl_142_romaji, {gRomajiKanaTbl_142_kana_0, gRomajiKanaTbl_142_kana_1, gRomajiKanaTbl_142_kana_2}},
    {gRomajiKanaTbl_143_romaji, {gRomajiKanaTbl_143_kana_0, gRomajiKanaTbl_143_kana_1, gRomajiKanaTbl_143_kana_2}},
    {gRomajiKanaTbl_144_romaji, {gRomajiKanaTbl_144_kana_0, gRomajiKanaTbl_144_kana_1, gRomajiKanaTbl_144_kana_2}},
    {gRomajiKanaTbl_145_romaji, {gRomajiKanaTbl_145_kana_0, gRomajiKanaTbl_145_kana_1, gRomajiKanaTbl_145_kana_2}},
    {gRomajiKanaTbl_146_romaji, {gRomajiKanaTbl_146_kana_0, gRomajiKanaTbl_146_kana_1, gRomajiKanaTbl_146_kana_2}},
    {gRomajiKanaTbl_147_romaji, {gRomajiKanaTbl_147_kana_0, gRomajiKanaTbl_147_kana_1, gRomajiKanaTbl_147_kana_2}},
    {gRomajiKanaTbl_148_romaji, {gRomajiKanaTbl_148_kana_0, gRomajiKanaTbl_148_kana_1, gRomajiKanaTbl_148_kana_2}},
    {gRomajiKanaTbl_149_romaji, {gRomajiKanaTbl_149_kana_0, gRomajiKanaTbl_149_kana_1, gRomajiKanaTbl_149_kana_2}},
    {gRomajiKanaTbl_150_romaji, {gRomajiKanaTbl_150_kana_0, gRomajiKanaTbl_150_kana_1, gRomajiKanaTbl_150_kana_2}},
    {gRomajiKanaTbl_151_romaji, {gRomajiKanaTbl_151_kana_0, gRomajiKanaTbl_151_kana_1, gRomajiKanaTbl_151_kana_2}},
    {gRomajiKanaTbl_152_romaji, {gRomajiKanaTbl_152_kana_0, gRomajiKanaTbl_152_kana_1, gRomajiKanaTbl_152_kana_2}},
    {gRomajiKanaTbl_153_romaji, {gRomajiKanaTbl_153_kana_0, gRomajiKanaTbl_153_kana_1, gRomajiKanaTbl_153_kana_2}},
    {gRomajiKanaTbl_154_romaji, {gRomajiKanaTbl_154_kana_0, gRomajiKanaTbl_154_kana_1, gRomajiKanaTbl_154_kana_2}},
    {gRomajiKanaTbl_155_romaji, {D_0046B848, D_0046B850, D_0046B858}},
    {gRomajiKanaTbl_156_romaji, {gRomajiKanaTbl_156_kana_0, gRomajiKanaTbl_156_kana_1, gRomajiKanaTbl_156_kana_2}},
    {gRomajiKanaTbl_157_romaji, {gRomajiKanaTbl_157_kana_0, gRomajiKanaTbl_157_kana_1, gRomajiKanaTbl_157_kana_2}},
    {gRomajiKanaTbl_158_romaji, {D_0046B848, D_0046B850, D_0046B858}},
    {gRomajiKanaTbl_159_romaji, {gRomajiKanaTbl_159_kana_0, gRomajiKanaTbl_159_kana_1, gRomajiKanaTbl_159_kana_2}},
    {gRomajiKanaTbl_160_romaji, {gRomajiKanaTbl_160_kana_0, gRomajiKanaTbl_160_kana_1, gRomajiKanaTbl_160_kana_2}},
    {gRomajiKanaTbl_161_romaji, {gRomajiKanaTbl_161_kana_0, gRomajiKanaTbl_161_kana_1, gRomajiKanaTbl_161_kana_2}},
    {gRomajiKanaTbl_162_romaji, {gRomajiKanaTbl_162_kana_0, gRomajiKanaTbl_162_kana_1, gRomajiKanaTbl_162_kana_2}},
    {gRomajiKanaTbl_163_romaji, {gRomajiKanaTbl_163_kana_0, gRomajiKanaTbl_163_kana_1, gRomajiKanaTbl_163_kana_2}},
    {gRomajiKanaTbl_164_romaji, {gRomajiKanaTbl_164_kana_0, gRomajiKanaTbl_164_kana_1, gRomajiKanaTbl_164_kana_2}},
    {gRomajiKanaTbl_165_romaji, {gRomajiKanaTbl_165_kana_0, gRomajiKanaTbl_165_kana_1, gRomajiKanaTbl_165_kana_2}},
    {gRomajiKanaTbl_166_romaji, {gRomajiKanaTbl_166_kana_0, gRomajiKanaTbl_166_kana_1, gRomajiKanaTbl_166_kana_2}},
    {gRomajiKanaTbl_167_romaji, {gRomajiKanaTbl_167_kana_0, gRomajiKanaTbl_167_kana_1, gRomajiKanaTbl_167_kana_2}},
    {gRomajiKanaTbl_168_romaji, {gRomajiKanaTbl_168_kana_0, gRomajiKanaTbl_168_kana_1, gRomajiKanaTbl_168_kana_2}},
    {gRomajiKanaTbl_169_romaji, {gRomajiKanaTbl_169_kana_0, gRomajiKanaTbl_169_kana_1, gRomajiKanaTbl_169_kana_2}},
    {gRomajiKanaTbl_170_romaji, {gRomajiKanaTbl_170_kana_0, gRomajiKanaTbl_170_kana_1, gRomajiKanaTbl_170_kana_2}},
    {gRomajiKanaTbl_171_romaji, {gRomajiKanaTbl_171_kana_0, gRomajiKanaTbl_171_kana_1, gRomajiKanaTbl_171_kana_2}},
    {gRomajiKanaTbl_172_romaji, {gRomajiKanaTbl_172_kana_0, gRomajiKanaTbl_172_kana_1, gRomajiKanaTbl_172_kana_2}},
    {gRomajiKanaTbl_173_romaji, {gRomajiKanaTbl_173_kana_0, gRomajiKanaTbl_173_kana_1, gRomajiKanaTbl_173_kana_2}},
    {gRomajiKanaTbl_174_romaji, {gRomajiKanaTbl_174_kana_0, gRomajiKanaTbl_174_kana_1, gRomajiKanaTbl_174_kana_2}},
    {gRomajiKanaTbl_175_romaji, {gRomajiKanaTbl_175_kana_0, gRomajiKanaTbl_175_kana_1, gRomajiKanaTbl_175_kana_2}},
    {gRomajiKanaTbl_176_romaji, {gRomajiKanaTbl_176_kana_0, gRomajiKanaTbl_176_kana_1, gRomajiKanaTbl_176_kana_2}},
    {gRomajiKanaTbl_177_romaji, {gRomajiKanaTbl_177_kana_0, gRomajiKanaTbl_177_kana_1, gRomajiKanaTbl_177_kana_2}},
    {gRomajiKanaTbl_178_romaji, {D_0046BB10, D_0046BB18, D_0046BB20}},
    {gRomajiKanaTbl_179_romaji, {D_0046BB30, D_0046BB38, D_0046BB40}},
    {gRomajiKanaTbl_180_romaji, {gRomajiKanaTbl_180_kana_0, gRomajiKanaTbl_180_kana_1, gRomajiKanaTbl_180_kana_2}},
    {gRomajiKanaTbl_181_romaji, {D_0046BB10, D_0046BB18, D_0046BB20}},
    {gRomajiKanaTbl_182_romaji, {D_0046BB30, D_0046BB38, D_0046BB40}},
    {gRomajiKanaTbl_183_romaji, {gRomajiKanaTbl_183_kana_0, gRomajiKanaTbl_183_kana_1, gRomajiKanaTbl_183_kana_2}},
    {gRomajiKanaTbl_184_romaji, {gRomajiKanaTbl_184_kana_0, gRomajiKanaTbl_184_kana_1, gRomajiKanaTbl_184_kana_2}},
    {gRomajiKanaTbl_185_romaji, {gRomajiKanaTbl_185_kana_0, gRomajiKanaTbl_185_kana_1, gRomajiKanaTbl_185_kana_2}},
    {gRomajiKanaTbl_186_romaji, {gRomajiKanaTbl_186_kana_0, gRomajiKanaTbl_186_kana_1, gRomajiKanaTbl_186_kana_2}},
    {gRomajiKanaTbl_187_romaji, {gRomajiKanaTbl_187_kana_0, gRomajiKanaTbl_187_kana_1, gRomajiKanaTbl_187_kana_2}},
    {gRomajiKanaTbl_188_romaji, {gRomajiKanaTbl_188_kana_0, gRomajiKanaTbl_188_kana_1, gRomajiKanaTbl_188_kana_2}},
    {gRomajiKanaTbl_189_romaji, {D_0046BC40, D_0046BC48, D_0046BC50}},
    {gRomajiKanaTbl_190_romaji, {D_0046BC60, D_0046BC60, gRomajiKanaTbl_190_kana_2}},
    {gRomajiKanaTbl_191_romaji, {D_0046BC78, D_0046BC80, D_0046BC88}},
    {gRomajiKanaTbl_192_romaji, {D_0046BC98, D_0046BCA0, D_0046BCA8}},
    {gRomajiKanaTbl_193_romaji, {gRomajiKanaTbl_193_kana_0, gRomajiKanaTbl_193_kana_1, gRomajiKanaTbl_193_kana_2}},
    {gRomajiKanaTbl_194_romaji, {D_0046BC40, D_0046BC48, D_0046BC50}},
    {gRomajiKanaTbl_195_romaji, {gRomajiKanaTbl_195_kana_0, gRomajiKanaTbl_195_kana_1, gRomajiKanaTbl_195_kana_2}},
    {gRomajiKanaTbl_196_romaji, {D_0046BC78, D_0046BC80, D_0046BC88}},
    {gRomajiKanaTbl_197_romaji, {D_0046BC98, D_0046BCA0, D_0046BCA8}},
    {gRomajiKanaTbl_198_romaji, {gRomajiKanaTbl_198_kana_0, gRomajiKanaTbl_198_kana_1, gRomajiKanaTbl_198_kana_2}},
    {gRomajiKanaTbl_199_romaji, {gRomajiKanaTbl_199_kana_0, gRomajiKanaTbl_199_kana_1, gRomajiKanaTbl_199_kana_2}},
    {gRomajiKanaTbl_200_romaji, {gRomajiKanaTbl_200_kana_0, gRomajiKanaTbl_200_kana_1, gRomajiKanaTbl_200_kana_2}},
    {gRomajiKanaTbl_201_romaji, {gRomajiKanaTbl_201_kana_0, gRomajiKanaTbl_201_kana_1, gRomajiKanaTbl_201_kana_2}},
    {gRomajiKanaTbl_202_romaji, {gRomajiKanaTbl_202_kana_0, gRomajiKanaTbl_202_kana_1, gRomajiKanaTbl_202_kana_2}},
    {gRomajiKanaTbl_203_romaji, {gRomajiKanaTbl_203_kana_0, gRomajiKanaTbl_203_kana_1, gRomajiKanaTbl_203_kana_2}},
    {gRomajiKanaTbl_204_romaji, {gRomajiKanaTbl_204_kana_0, gRomajiKanaTbl_204_kana_1, gRomajiKanaTbl_204_kana_2}},
    {gRomajiKanaTbl_205_romaji, {gRomajiKanaTbl_205_kana_0, gRomajiKanaTbl_205_kana_1, gRomajiKanaTbl_205_kana_2}},
    {gRomajiKanaTbl_206_romaji, {gRomajiKanaTbl_206_kana_0, gRomajiKanaTbl_206_kana_1, gRomajiKanaTbl_206_kana_2}},
    {gRomajiKanaTbl_207_romaji, {gRomajiKanaTbl_207_kana_0, gRomajiKanaTbl_207_kana_1, gRomajiKanaTbl_207_kana_2}},
    {gRomajiKanaTbl_208_romaji, {gRomajiKanaTbl_208_kana_0, gRomajiKanaTbl_208_kana_1, gRomajiKanaTbl_208_kana_2}},
    {gRomajiKanaTbl_209_romaji, {gRomajiKanaTbl_209_kana_0, gRomajiKanaTbl_209_kana_1, gRomajiKanaTbl_209_kana_2}},
    {gRomajiKanaTbl_210_romaji, {gRomajiKanaTbl_210_kana_0, gRomajiKanaTbl_210_kana_1, gRomajiKanaTbl_210_kana_2}},
    {gRomajiKanaTbl_211_romaji, {gRomajiKanaTbl_211_kana_0, gRomajiKanaTbl_211_kana_1, gRomajiKanaTbl_211_kana_2}},
    {gRomajiKanaTbl_212_romaji, {gRomajiKanaTbl_212_kana_0, gRomajiKanaTbl_212_kana_1, gRomajiKanaTbl_212_kana_2}},
    {gRomajiKanaTbl_213_romaji, {gRomajiKanaTbl_213_kana_0, gRomajiKanaTbl_213_kana_1, gRomajiKanaTbl_213_kana_2}},
    {gRomajiKanaTbl_214_romaji, {gRomajiKanaTbl_214_kana_0, gRomajiKanaTbl_214_kana_1, gRomajiKanaTbl_214_kana_2}},
    {gRomajiKanaTbl_215_romaji, {gRomajiKanaTbl_215_kana_0, gRomajiKanaTbl_215_kana_1, gRomajiKanaTbl_215_kana_2}},
    {gRomajiKanaTbl_216_romaji, {D_0046BF50, D_0046BF58, D_0046BF60}},
    {gRomajiKanaTbl_217_romaji, {D_0046BF70, D_0046BF78, D_0046BF80}},
    {gRomajiKanaTbl_218_romaji, {D_0046BF90, D_0046BF98, D_0046BFA0}},
    {gRomajiKanaTbl_219_romaji, {D_0046BF50, D_0046BF58, D_0046BF60}},
    {gRomajiKanaTbl_220_romaji, {D_0046BF70, D_0046BF78, D_0046BF80}},
    {gRomajiKanaTbl_221_romaji, {D_0046BF90, D_0046BF98, D_0046BFA0}},
    {gRomajiKanaTbl_222_romaji, {gRomajiKanaTbl_222_kana_0, gRomajiKanaTbl_222_kana_1, gRomajiKanaTbl_222_kana_2}},
    {gRomajiKanaTbl_223_romaji, {gRomajiKanaTbl_223_kana_0, gRomajiKanaTbl_223_kana_1, gRomajiKanaTbl_223_kana_2}},
    {gRomajiKanaTbl_224_romaji, {gRomajiKanaTbl_224_kana_0, gRomajiKanaTbl_224_kana_1, gRomajiKanaTbl_224_kana_2}},
    {gRomajiKanaTbl_225_romaji, {gRomajiKanaTbl_225_kana_0, gRomajiKanaTbl_225_kana_1, gRomajiKanaTbl_225_kana_2}},
    {gRomajiKanaTbl_226_romaji, {gRomajiKanaTbl_226_kana_0, gRomajiKanaTbl_226_kana_1, gRomajiKanaTbl_226_kana_2}},
    {gRomajiKanaTbl_227_romaji, {gRomajiKanaTbl_227_kana_0, gRomajiKanaTbl_227_kana_1, gRomajiKanaTbl_227_kana_2}},
    {gRomajiKanaTbl_228_romaji, {gRomajiKanaTbl_228_kana_0, gRomajiKanaTbl_228_kana_1, gRomajiKanaTbl_228_kana_2}},
    {gRomajiKanaTbl_229_romaji, {gRomajiKanaTbl_229_kana_0, gRomajiKanaTbl_229_kana_1, gRomajiKanaTbl_229_kana_2}},
    {gRomajiKanaTbl_230_romaji, {gRomajiKanaTbl_230_kana_0, gRomajiKanaTbl_230_kana_1, gRomajiKanaTbl_230_kana_2}},
    {gRomajiKanaTbl_231_romaji, {gRomajiKanaTbl_231_kana_0, gRomajiKanaTbl_231_kana_1, gRomajiKanaTbl_231_kana_2}},
    {gRomajiKanaTbl_232_romaji, {gRomajiKanaTbl_232_kana_0, gRomajiKanaTbl_232_kana_1, gRomajiKanaTbl_232_kana_2}},
    {gRomajiKanaTbl_233_romaji, {D_0046C128, D_0046C130, gRomajiKanaTbl_233_kana_2}},
    {gRomajiKanaTbl_234_romaji, {D_0046C128, D_0046C130, gRomajiKanaTbl_234_kana_2}},
    {gRomajiKanaTbl_235_romaji, {gRomajiKanaTbl_235_kana_0, gRomajiKanaTbl_235_kana_1, gRomajiKanaTbl_235_kana_2}},
    {gRomajiKanaTbl_236_romaji, {gRomajiKanaTbl_236_kana_0, gRomajiKanaTbl_236_kana_1, gRomajiKanaTbl_236_kana_2}},
    {gRomajiKanaTbl_237_romaji, {gRomajiKanaTbl_237_kana_0, gRomajiKanaTbl_237_kana_1, gRomajiKanaTbl_237_kana_2}},
    {gRomajiKanaTbl_238_romaji, {D_0046C1B8, D_0046C1C0, D_0046C1C8}},
    {gRomajiKanaTbl_239_romaji, {D_0046C1B8, D_0046C1C0, D_0046C1C8}},
};
char *gKbdHidWideStrs[10] = {
    gKbdHidWideStrs_0, gKbdHidWideStrs_1, gKbdHidWideStrs_2, gKbdHidWideStrs_3,
    gKbdHidWideStrs_4, gKbdHidWideStrs_5, gKbdHidWideStrs_6, gKbdHidWideStrs_7,
    gKbdHidWideStrs_8, gKbdHidWideStrs_9,
};
char *gKbdHidKeypadWideStrs[2] = {
    gKbdHidKeypadWideStrs_0, NULL,
};
char *gKbdHidChars[6] = {
    gKbdHidChars_0, gKbdHidChars_1, gKbdHidChars_2, gKbdHidChars_3,
    gKbdHidChars_4, gKbdHidChars_5,
};
char *gKbdHidNoRomaji[2] = {
    gKbdHidNoRomaji_0, NULL,
};
char *gKbdHidKeypadChars[4] = {
    gKbdHidKeypadChars_0, NULL, NULL, NULL,
};
u8 gKbdSetCharsets[32] = {
#include "sub_main/00416440.inc"
};
char gKbdKind8Chars[536] = {
#include "sub_main/00416460.inc"
};
char *gKanaNChars[3] = {
    gKanaNChars_0, gKanaNChars_1, gKanaNChars_2,
};
char *gKbdSymbolCands[36] = {
    gKbdSymbolCands_0, gKbdSymbolCands_1, gKbdSymbolCands_2, gKbdSymbolCands_3,
    gKbdSymbolCands_4, gKbdSymbolCands_5, gKbdSymbolCands_6, gKbdSymbolCands_7,
    gKbdSymbolCands_8, gKbdSymbolCands_9, gKbdSymbolCands_10, gKbdSymbolCands_11,
    gKbdSymbolCands_12, gKbdSymbolCands_13, gKbdSymbolCands_14, gKbdSymbolCands_15,
    gKbdSymbolCands_16, gKbdSymbolCands_17, gKbdSymbolCands_18, gKbdSymbolCands_19,
    gKbdSymbolCands_20, gKbdSymbolCands_21, gKbdSymbolCands_22, gKbdSymbolCands_23,
    gKbdSymbolCands_24, gKbdSymbolCands_25, gKbdSymbolCands_26, gKbdSymbolCands_27,
    gKbdSymbolCands_28, gKbdSymbolCands_29, gKbdSymbolCands_30, gKbdSymbolCands_31,
    gKbdSymbolCands_32, gKbdSymbolCands_33, gKbdSymbolCands_34, NULL,
};
Tbl17 gKbdSetIsWide = {
#include "sub_main/00416720.inc"
};
extern PhraseEntry gKbdGestureEntries[12];
KbdKindConf gKbdKindConfs[5] = {
#include "sub_main/00416740.inc"
};
PhraseEntry gKbdGestureEntries[12] = {
#include "sub_main/00416860.inc"
};
PhraseEntry gPhraseEntries[PHRASE_COUNT] = {
#include "sub_main/00416920.inc"
};
KbdKindKeys gKbdKeySets[16] = {
#include "sub_main/00416AA0.inc"
};
s32 gKbdLayoutFlags[16] = {
#include "sub_main/00416AC0.inc"
};
s8 gKbdWidthPairs[32] = {
#include "sub_main/00416B00.inc"
};
BoxSize gKbdBoxSizes[4] = {
#include "sub_main/00416B20.inc"
};
KbdKeyPos gKbdSetCursors[74] = {
#include "sub_main/00416B40.inc"
};
KbdKeyPos gKbdBindCursors[26] = {
#include "sub_main/00416D90.inc"
};
s8 gKbdSetGrid[21][4] = {
#include "sub_main/00416E60.inc"
};
s8 gKbdBindGrid[21][4] = {
#include "sub_main/00416EC0.inc"
};
KbdGlyph gKbdGlyphs[8] = {
#include "sub_main/00416F20.inc"
};
u8 gKbdWidthKeys[16] = {
#include "sub_main/00416F40.inc"
};
KbdKey gKbdKeysHira[4] = {
#include "sub_main/00416F50.inc"
};
KbdKey gKbdKeysHira_4[72] = {
#include "sub_main/00416F60.inc"
};
KbdKey gKbdKeysKata[4] = {
#include "sub_main/00417080.inc"
};
KbdKey gKbdKeysKata_4[72] = {
#include "sub_main/00417090.inc"
};
KbdKey gKbdKeysWideUpper[4] = {
#include "sub_main/004171B0.inc"
};
KbdKey gKbdKeysWideUpper_4[72] = {
#include "sub_main/004171C0.inc"
};
KbdKey gKbdKeysWideUpperNoSym[76] = {
#include "sub_main/004172E0.inc"
};
KbdKey gKbdKeysSmallHira[4] = {
#include "sub_main/00417410.inc"
};
KbdKey gKbdKeysSmallHira_4[72] = {
#include "sub_main/00417420.inc"
};
KbdKey gKbdKeysSmallKata[4] = {
#include "sub_main/00417540.inc"
};
KbdKey gKbdKeysSmallKata_4[72] = {
#include "sub_main/00417550.inc"
};
KbdKey gKbdKeysWideLower[4] = {
#include "sub_main/00417670.inc"
};
KbdKey gKbdKeysWideLower_4[72] = {
#include "sub_main/00417680.inc"
};
KbdKey gKbdKeysWideLowerNoSym[76] = {
#include "sub_main/004177A0.inc"
};
KbdKey gKbdKeysHalfKata[4] = {
#include "sub_main/004178D0.inc"
};
KbdKey gKbdKeysHalfKata_4[72] = {
#include "sub_main/004178E0.inc"
};
KbdKey gKbdKeysHalfSmallKata[4] = {
#include "sub_main/00417A00.inc"
};
KbdKey gKbdKeysHalfSmallKata_4[72] = {
#include "sub_main/00417A10.inc"
};
KbdKey gKbdKeysSymbol[4] = {
#include "sub_main/00417B30.inc"
};
KbdKey gKbdKeysSymbol_4[72] = {
#include "sub_main/00417B40.inc"
};
KbdKey gKbdKeysBlank[76] = {
#include "sub_main/00417C60.inc"
};
KbdKey gKbdKeysGesture[4] = {
#include "sub_main/00417D90.inc"
};
KbdKey gKbdKeysGesture_4[24] = {
#include "sub_main/00417DA0.inc"
};
KbdKey gKbdKeysPhrase1[28] = {
#include "sub_main/00417E00.inc"
};
KbdKey gKbdKeysPhrase2[4] = {
#include "sub_main/00417E70.inc"
};
KbdKey gKbdKeysPhrase2_4[24] = {
#include "sub_main/00417E80.inc"
};
KbdKey gKbdKeysAsciiUpper[4] = {
#include "sub_main/00417EE0.inc"
};
KbdKey gKbdKeysAsciiUpper_4[72] = {
#include "sub_main/00417EF0.inc"
};
KbdKey gKbdKeysAsciiUpperNoSym[76] = {
#include "sub_main/00418010.inc"
};
KbdKey gKbdKeysAsciiLower[4] = {
#include "sub_main/00418140.inc"
};
KbdKey gKbdKeysAsciiLower_4[72] = {
#include "sub_main/00418150.inc"
};
KbdKey gKbdKeysAsciiLowerNoSym[76] = {
#include "sub_main/00418270.inc"
};
KbdKey gKbdKeysDigit[76] = {
#include "sub_main/004183A0.inc"
};
KeyPair gKbdPhrase1Binds[2] = {
#include "sub_main/004184D0.inc"
};
KeyPair gKbdPhrase2Binds[2] = {
#include "sub_main/004184F0.inc"
};
KeyPair gKbdGestureBinds[2] = {
#include "sub_main/00418510.inc"
};
KbdLayout gKbdSetLayouts[20] = {
#include "sub_main/00418530.inc"
};
KbdLayout gKbdAltLayouts[4] = {
#include "sub_main/004186C0.inc"
};
void *gKanaDakutenTable = (void *)gKanaDakutenTable_0;
void *gKanaHandakutenTable = (void *)gKanaHandakutenTable_0;
u32 gKbdPulseAmp[2] = {
#include "sub_main/00418720.inc"
};
u32 gKbdLinePulseAmp[2] = {
#include "sub_main/00418728.inc"
};
KeyPos gKbdSpecialKeyPos[16] = {
#include "sub_main/00418730.inc"
};
s8 gKbdPhraseKeyOrder[16] = {
#include "sub_main/00418750.inc"
};
u8 gKbdDakutenSjis[8] = {
#include "sub_main/00418760.inc"
};
u8 gKbdHandakutenSjis[8] = {
#include "sub_main/00418768.inc"
};
u8 gKbdSetModes[32] = {
#include "sub_main/00418770.inc"
};
s8 gKbdShiftSets[32] = {
#include "sub_main/00418790.inc"
};
