/*
 * The software keyboard's main unit, then the follow and orbit cameras
 * (sub_main 0x314E00-0x31C7E0).
 * - The keyboard (KbdEntry gKbdState, reached through gKbd; kbd.h): opening
 *   it for an entry kind (Kbd_Open, Kbd_SetLayout), its frame (Kbd_Run: the
 *   pad's buttons, the USB keyboard's keys, key repeat), the key sets and
 *   character modes (KBD_SET_*, KBD_MODE_*), typing kana and converting
 *   them (the converter's commands, ConvCmd_*), the phrase and gesture
 *   pages, drawing (Kbd_Draw) and the checks of what an entry kind
 *   accepts. unk_30EA30.c holds the rest: the USB keys, the text line and
 *   the conversion. Kbd_EnterChar and Kbd_CharIsSpecialKana share one
 *   full-width space; its "%s" is another copy than Kbd_DrawCandidateBox's.
 * - The camera controller (CamCtl gCamCtl; camera.h): its frame
 *   (CamCtl_Update runs each mode's handlers, CAM_MODE_*), the camera
 *   data's relocation, the follow camera behind the player (CamFollow_*)
 *   and the over-the-shoulder orbit (CamOrbit_*); the other modes are in
 *   unk_31E190.c.
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
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap); /* vsnprintf */
void Font_Printf(const char *fmt, ...);
extern s32 gTextOff;
extern char D_0046CC10[]; /* "%s" */
char *strcat(char *dst, char *src); /* strcat */
char *strcpy(char *dst, const char *src); /* strcpy */
s32 strlen(char *s); /* strlen */
void strncpy(char *dst, const char *src, u32 n);
void Vec_Copy(f32 *dst, f32 *src);
s32 ConvCmd_Open(void);
void Kbd_TypeHidKey(void);
void Kbd_OpenPhrases1(void);
void Kbd_OpenPhrases2(void);
s32 Str_InsertClip(char *dst, char *src, s32 pos, s32 max);
void Kbd_UpdateSymbolMode(void);
void Kbd_Convert(void);
void Kbd_PrevCand(void);
void Kbd_NextCand(void);
s32 Kbd_SetIsWide(void);
void Kbd_SetPhrasePage(u8 arg0);
s8 Kbd_LookupKey(void);
void Kbd_ClearUnk34(s8);
s32 Kbd_KeyRepeat(s16 held, s16 pressed);
void Kbd_TypeKey(KbdKey *key);
void Kbd_EnterChar(KbdKey *key);
void Kbd_InsertPhrase(void);
void Kbd_ToggleWidth(void);
void Kbd_DeleteBack(s32);
void Kbd_Submit(void);
void Str_ToWide(s8 *src, s8 *dst);
void Kbd_RefreshLayout(void);
s32 Kana_AddDakuten(void);
s32 Kana_AddHandakuten(void);
s32 Kbd_FindKind(u8 kind, u8 *outA, u8 *outB);
s32 Kbd_FindKindEx(u8 kind, u8 *outA, u8 *outB);
void Kbd_UpdateMode(void);
void Kbd_HomePageCursor(void);
void Kbd_PickLayout(void);
void Kbd_EnterGestures(void);
s32 Kbd_CharIsSpecialKana(KbdKey *p);
s32 Kbd_CharInSet5_Unk(KbdKey *p);
void Kbd_SaveKanaInput(void);
s32 Kbd_CharInSet4_Unk(KbdKey *p);
s32 Kbd_CharInRows01(KbdKey *arg0);
s32 Kbd_CharInRowsHigh(KbdKey *arg0);
s32 Kbd_CharIsPlain(KbdKey *p);
s32 CamFollow_IsBlownAway(void);
f32 AreaGrid2_GetFloorY(f32 *pos); /* the ground height at pos */
void Hud_SetCancel(void);
void Kbd_EmptyInit(void);
void Player_RequestGesture(u8 a);
void Mem_Fill(void *dst, u8 c, s32 n);
extern u8 gKbdDakutenSjis[];
extern u8 gKbdDakutenSjis_1;
extern u8 gKbdHandakutenSjis[];
extern u8 gKbdHandakutenSjis_1;
extern u8 gKbdSetModes[];
extern char D_0046CC08[]; /* a full-width space */
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
void Kbd_NextChatTarget(void);
void Kbd_ClearActive(void);
void Kbd_CursorRight(void);
void Kbd_CursorLeft(void);
void Kbd_ConvertOrCommit(void);
void Kbd_PressKey(KbdKey *key);
s32 Kbd_DoGesture(char *text, u16 *pos, s32 gesture);
void Kbd_KanaDakuten(void);
void Kbd_KanaHandakuten(void);
void Kbd_ToggleShift(void);
void Kbd_CyclePhrase(void);
void Kbd_CycleGesture(void);
extern s32 gKbdSettings;
s32 Kbd_ShiftPair(u8 i);
s32 Sjis_PrevCharSize(u8 *s, s32 pos);
void Kbd_CommitKana(void);
s32 Sjis_CharSizeAt(char *s, s32 pos);
s32 Kbd_KindIs8To10(void);
typedef struct Rect16 {
    s16 x, y, w, h;
    s32 color;
} Rect16;
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);
void Kbd_Printf(const char *fmt, ...);
void Kbd_ClearText(void);
void Kbd_ClearCands(void);


/* The size of the character that ends at byte pos of s (moving the cursor back). */
s32 Sjis_PrevCharSize(u8 *str, s32 pos)
{
    s32 i;
    s32 size = 0;
    s32 len = 0;

    while (*str != 0 && len < pos) {
        size = ((*str >= 0x80 && *str < 0xA0) || (*str >= 0xE0 && *str < 0x100)) ? 2 : 1;
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
        size = ((*scan >= 0x80 && *scan < 0xA0) || (*scan >= 0xE0 && *scan < 0x100)) ? 2 : 1;
        i += size;
        scan += size;
    }
    if (pos < i) {
        return -1;
    }
    lead = str[pos];
    return ((lead >= 0x80 && lead <= 0x9F) || (lead >= 0xE0 && lead < 0x100)) ? 2 : 1;
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

extern s32 gKbdSettings;
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
    idx = (long)(gKbd->col * 4);
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
    if ((code >= 0x80) && (code < 0xA0)) {
        return 0;
    }
    if ((code >= 0xE0) && (code < 0x100)) {
        return 0;
    }
    return 1;
}

/* Kbd_Open: opens the software keyboard: mode kind (the gKbdKindConfs entry), layout,
 * at most max bytes, starting from text (none: empty). */
extern u8 gKbdKindConfs[][0x38];
extern u8 gKbdDelay;
extern u8 gKbdReplayKey;
s32 Kbd_GetStartActive(void);
void Kbd_SetActive(void);
void Kbd_SetOpen(void);
void Kbd_ClearOpen(void);
void Kbd_ClearCands(void);
void Kbd_OpenConv(void);
void Kbd_BindStartKeys(void);
/* Called once with an argument it does not take. */
extern KbdLayout gKbdSetLayouts[20];
void Kbd_BindLayouts(s32 unused);
void Kbd_SetLayout(u8 k);
extern u8 gKbdKindIs0;
extern u8 gKbdOpenKey;
extern s8 gKbdPrompt[];
extern s16 gKbdChatTarget;
void sceUsbKbSetRepeat(s32 a, s32 b, s32 c);

void *memset(void *dst, s32 c, u32 n);
/*
 * Opens the software keyboard.
 * - kind picks the colours and the phrase and gesture lists
 *   (gKbdKindConfs[kind]); gKbdKindIs0 records kind 0, whose R2 cycle
 *   reaches the gestures page.
 * - layout is the entry kind (Kbd_SetLayout): the barred sets, the starting
 *   set and the characters accepted.
 * - max is the text's maximum length (at most 0xFF); text is the starting
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
    gKbd->kindConf = gKbdKindConfs[kind];
    Kbd_ClearOpen();
    Kbd_ClearActive();
    if (max > 0xFF) {
        max = 0xFF;
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
        memset(gKbd->text, 0, 0x100);
        strncpy(gKbd->text, text, 0xFF);
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
extern u8 gKbdKeySets[][2];

/*
 * Sets up entry kind entryKind: the barred key sets (gKbdLayoutFlags, plus
 * both phrase pages when the kind has no phrases); the starting key set
 * (gKbdKeySets[entryKind][0]: kinds 0, 0xC, 0xD and 0xE keep a current
 * two-byte set, kind 3 an allowed one and kind 0xB any; the set
 * Kbd_SaveKeySet saved for the kind wins; with Caps Lock on an alphabet set
 * swaps case); and the characters accepted (gKbdKeySets[entryKind][1]).
 * Then updates the mode, the cursor and the symbol mode.
 */
void Kbd_SetLayout(u8 entryKind)
{
    gKbd->kind = entryKind;
    gKbd->flags = gKbdLayoutFlags[entryKind];
    if (*(s32 *)(gKbd->kindConf + 0x30) == 0) {
        gKbd->flags |= KBD_SET_BIT(KBD_SET_PHRASE1);
        gKbd->flags |= KBD_SET_BIT(KBD_SET_PHRASE2);
    }
    switch (entryKind) {
    case 0:
    case 0xE:
    case 0xC:
    case 0xD:
        if (Kbd_SetIsWide() == 0) {
            gKbd->keySet = gKbdKeySets[entryKind][0];
        }
        break;
    case 3:
        if (gKbd->flags & (1 << gKbd->keySet)) {
            gKbd->keySet = gKbdKeySets[entryKind][0];
        }
        break;
    case 0xB:
        break;
    default:
        gKbd->keySet = gKbdKeySets[entryKind][0];
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
    gKbd->accept = gKbdKeySets[entryKind][1];
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

void Kbd_Update(void);
void Kbd_Space(s32);
void Kbd_PageUp(void);
void Kbd_PageDown(void);
void Kbd_PrevSegment(void);
void Kbd_NextSegment(void);
void Kbd_NextPhrasePage(void);
void sceUsbKbClearRbuf(s32);
void Kbd_MoveCursor(s16 keys, s16 held);
void Kbd_NextRow(s32 back);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

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
 * set saved. Entry kind 0xB then clears the text and stays open. Plays the
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
        if (gKbd->kind == 0xB) {
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

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_314E00", D_0046CAE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_314E00", D_0046CBB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_314E00", D_0046CC08);

s32 Kbd_CharBlocked(KbdKey *p);
s32 Kbd_DakutenKeyKind(KbdKey *arg0);
s32 Kana_SwapBeforeCursor(s8 *table);
extern void *gKanaDakutenTable; /* conversion tables for Kana_SwapBeforeCursor */
extern void *gKanaHandakutenTable;

/*
 * Enters key's character (ch), unless Kbd_CharBlocked. On a one-byte set it
 * commits any pending kana and inserts the character into the text at
 * textPos (up to maxLen). On a two-byte set a full-width space goes into the
 * text when no kana is pending; any other character goes into the kana at
 * kanaPos (up to 0x48 bytes), where a dakuten or handakuten key on the kana
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
                gKbd->kanaPos, 0x48);
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
        (flags & KBD_SET_BIT(KBD_SET_PHRASE2)) || *(s32 *)(gKbd->kindConf + 0x30) == 0) {
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
        *(s32 *)(gKbd->kindConf + 0x28) == 0) {
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

extern s16 gKbdChatTarget;
extern s8 gKbdPrompt[];
s32 Chat_NextTarget(s8 *out, s32 next);

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
    char buf[0x100];
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
    if (*(char **)(gKbd->keyInfo + 0xC) == NULL) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    strcpy(buf, *(char **)(gKbd->keyInfo + 0xC));
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
 * typed (entry kinds 0xC and 0xD excepted) or during a cycle it cancels the
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

    if (gKbd->kind != 0xC && gKbd->kind != 0xD &&
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

s32 Kbd_FindKind(u8 row, u8 *out1F, u8 *out1E);

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
    layout->cursors = gKbdGestureBinds[*(s32 *)(gKbd->kindConf + 0x28)].cursors;
    layout->grid = gKbdGestureBinds[*(s32 *)(gKbd->kindConf + 0x28)].grid;
    layout->extra = *(u8 **)(gKbd->kindConf + 0x2C);
    layout = &gKbdSetLayouts[KBD_SET_PHRASE1];
    layout->cursors = gKbdPhrase1Binds[*(s32 *)(gKbd->kindConf + 0x30)].cursors;
    layout->grid = gKbdPhrase1Binds[*(s32 *)(gKbd->kindConf + 0x30)].grid;
    layout->extra = *(u8 **)(gKbd->kindConf + 0x34);
    layout = &gKbdSetLayouts[KBD_SET_PHRASE2];
    layout->cursors = gKbdPhrase2Binds[*(s32 *)(gKbd->kindConf + 0x30)].cursors;
    layout->grid = gKbdPhrase2Binds[*(s32 *)(gKbd->kindConf + 0x30)].grid;
    layout->extra = *(u8 **)(gKbd->kindConf + 0x34);
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
    long k = (long)gKbd;

    gKbdKeysHira_4[0] = gKbdKeysSmallHira_4[0] = gKbdKeysKata_4[0] = gKbdKeysHalfKata_4[0] = gKbdKeysSmallKata_4[0] = gKbdKeysHalfSmallKata_4[0] = gKbdKeysWideUpper_4[0] =
        gKbdKeysWideLower_4[0] = gKbdKeysAsciiUpper_4[0] = gKbdKeysAsciiLower_4[0] = gKbdKeysSymbol_4[0] = gKbdKeysGesture_4[0] = gKbdKeysPhrase2_4[0] =
            gKbdPhrase1Binds[*(s32 *)(*(u8 **)((u8 *)k + 0x10) + 0x30)].key;
    gKbdKeysHira_5 = gKbdKeysSmallHira_5 = gKbdKeysKata_5 = gKbdKeysHalfKata_5 = gKbdKeysSmallKata_5 = gKbdKeysHalfSmallKata_5 = gKbdKeysWideUpper_5 =
        gKbdKeysWideLower_5 = gKbdKeysAsciiUpper_5 = gKbdKeysAsciiLower_5 = gKbdKeysSymbol_5 = gKbdKeysGesture_5 = gKbdKeysPhrase1_5 =
            gKbdPhrase2Binds[*(s32 *)(*(u8 **)((u8 *)k + 0x10) + 0x30)].key;
    gKbdKeysHira_6 = gKbdKeysSmallHira_6 = gKbdKeysKata_6 = gKbdKeysSmallKata_6 = gKbdKeysHalfKata_6 = gKbdKeysHalfSmallKata_6 = gKbdKeysWideUpper_6 =
        gKbdKeysWideLower_6 = gKbdKeysAsciiUpper_6 = gKbdKeysAsciiLower_6 = gKbdKeysSymbol_6 = gKbdKeysPhrase1_6 = gKbdKeysPhrase2_6 =
            gKbdGestureBinds[*(s32 *)(*(u8 **)((u8 *)k + 0x10) + 0x28)].key;
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
            dst[0] = 0x82;
            dst[1] = character + 0x1F;
            src++;
            dst += 2;
        } else if (character >= 'a' && character <= 'z') {
            dst[0] = 0x82;
            dst[1] = character + 0x20;
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

void Kbd_FillRect(s32 color, s16 x, s16 y, s16 w, s16 h);

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

s32 Sprite_Draw(Sprite *s);

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

extern u8 gKbdOpenKey;
extern u8 gKbdKindIs0;

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

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_314E00", D_0046CC10);

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
f32 Math_Sin(f32 x);
void Text_FlushPages(void);
void Text_DrawCurrentPage(void);
void Text_SetColor(s16 color);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Kbd_BindGlyphTex(void);
void Gfx_SetBlendAlphaAdd(s32 a);
void Gfx_SetTexFilter(s32 a);
s32 Kbd_ArrowsOnPage(void);
void Kbd_DrawLine(f32 x, s16 y); /* unit's own declaration: the definition takes a u32 y;
 * Kbd_Draw's call matches with s16 y */
void Kbd_DrawCandidateBox(f32 x, s16 y);
void Kbd_DrawSpecialKey(s16 dy, s32 color, f32 dx);

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
    Gfx_SetTexFilter(1);
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
    Gfx_SetTexFilter(0);
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
    Gfx_SetTexFilter(1);
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
                    code = 0xC5;
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
                code = 0xC5;
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
    Gfx_SetTexFilter(0);
    Kbd_DrawLine(2.0f + (14.0f + x) / gKbd->drawScale, y + 0);
    Kbd_DrawCandidateBox(2.0f + (14.0f + x) / gKbd->drawScale, y - 1);
    Text_DrawCurrentPage();
}

void Kbd_ToggleShift(void);

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

s32 Kana_SwapBeforeCursor(s8 *table);
extern void *gKanaDakutenTable;

s32 Kana_AddDakuten(void)
{
    return Kana_SwapBeforeCursor(gKanaDakutenTable);
}

extern void *gKanaHandakutenTable;

s32 Kana_AddHandakuten(void)
{
    return Kana_SwapBeforeCursor(gKanaHandakutenTable);
}

s8 Kbd_IsOpen(void)
{
    return gKbd->open;
}

void Kbd_ClearOpen(void);
void Kbd_ClearActive(void);
void Kbd_SaveKeySet(void);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

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
        if (*(s32 *)(gKbd->kindConf + 0x28) == 0) {
            return 1;
        }
        goto one;
    case KBD_MODE_PHRASE1:
    case KBD_MODE_PHRASE2:
        if (*(s32 *)(gKbd->kindConf + 0x30) == 0) {
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

extern u8 gKbdKindIs0;

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
    case 0xA:
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
    Kbd_SetPhrasePage(0xDU);
    Kbd_CommitKana();
    if (Kbd_CheckGestureText(gKbd->text, &gKbd->textPos) < 0) {
        gKbd->gestureMode = (u8) (gKbd->gestureMode | KBD_GESTURE_KANA);
    }
}

/*
 * Clears gKbdState.polled through its address (D_005AEAD7 is gKbdState +
 * 0x37) so that Kbd_ReadUsb reads the USB keyboard again; the frame's pad
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
        case 0x98:
        case 0x99:
        case 0xA8:
        case 0xB5:
        case 0xB6:
        case 0xB7:
        case 0xB8:
        case 0xB9:
        case 0xE0:
        case 0xE1:
        case 0xF3:
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
 * 0xF Kbd_CharInSet5_Unk's, 1, 2 and 5 Kbd_CharInSet4_Unk's, 7 all but
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
    } else if (key->glyph != 0xD1 && key->glyph != 0x7D) {
        switch (gKbd->kind) {
        case 0xF:
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
 * dot or the apostrophe (refused by entry kind 0xF), else 0.
 */
s32 Kbd_CharInSet5_Unk(KbdKey *key)
{
    s32 found = 0;

    switch (key->glyph) {
    case 0xE3:
    case 0xB5:
    case 0xB6:
    case 0xB9:
    case 0xF1:
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
    case 0xE3:
    case 0xB5:
    case 0xB6:
    case 0xB9:
        found = 1;
        break;
    }
    return found;
}

/* 1 when the key's glyph is a digit (glyph cells 0x00-0x04, 0x10-0x14). */
s32 Kbd_CharInRows01(KbdKey *key)
{
    u8 glyphCode;

    glyphCode = key->glyph;
    if (((s32) glyphCode >= 0) && ((s32) glyphCode < 5)) {
        return 1;
    }
    if (((s32) glyphCode >= 0x10) && ((s32) glyphCode < 0x15)) {
        return 1;
    }
    return 0;
}

/* 1 when the key's glyph is a letter (glyph cells 0x20-0x64 and 0x70). */
s32 Kbd_CharInRowsHigh(KbdKey *key)
{
    s32 glyphCode;

    glyphCode = key->glyph;
    if (((s32) glyphCode >= 0x20) && ((s32) glyphCode < 0x25)) {
        return 1;
    }
    if (((s32) glyphCode >= 0x30) && ((s32) glyphCode < 0x35)) {
        return 1;
    }
    if (((s32) glyphCode >= 0x40) && ((s32) glyphCode < 0x45)) {
        return 1;
    }
    if (((s32) glyphCode >= 0x50) && ((s32) glyphCode < 0x55)) {
        return 1;
    }
    if (((s32) glyphCode >= 0x60) && ((s32) glyphCode < 0x65)) {
        return 1;
    }
    if (glyphCode == 0x70) {
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
    case 0xE2:
    case 0xC5:
    case 0xF0:
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
    char buf[0x400];
    va_list args;

    if (gTextOff == 0) {
        va_start(args, fmt);
        vsnprintf(buf, 0x3FF, fmt, args);
        buf[0x3FF] = 0;
        Font_Printf(D_0046CC10, buf);
    }
}

/* Clears the whole camera controller gCamCtl. */
void CamCtl_Clear(void)
{
    Mem_Fill(&gCamCtl, 0U, 0x680);
}

/*
 * Drops any pending camera request and puts the follow camera back on the
 * default distance setting (CAM_DIST_DEFAULT).
 */
void CamReq_Reset(void)
{
    gCamCtl.request = 0;
    gCamCtl.requestArg = NULL;
    gCamCtl.follow.distNo = CAM_DIST_DEFAULT;
}

void CamCtl_LoadParams(CamCtl *c);

/*
 * Readies the camera controller for a scene: the next CamCtl_Update sets
 * the modes up again; the shake requests, the target object, the view flags
 * and the request are cleared; the local player becomes the one followed,
 * and CamCtl_LoadParams takes the grid parameters from the camera data.
 */
void CamCtl_Init(void)
{
    *(s32 *)&gCamCtl.inited = 0; /* a word store: inited and its padding */
    gCamCtl.shakeC.active = 0;
    gCamCtl.shakeB.active = 0;
    gCamCtl.shakeA.active = 0;
    gCamCtl.targetObj = NULL;
    gCamCtl.emInView = 0;
    gCamCtl.followView = 0;
    gCamCtl.request = 0;
    gCamCtl.requestArg = NULL;
    gCamCtl.player = &gPlayers[gScene.localPlayer];
    CamCtl_LoadParams(&gCamCtl);
}


/*
 * Keeps the loaded camera data hdr in gCamCtl.data (NULL: none) and turns
 * the file's offsets into pointers: each area's faces and transition table,
 * the grid cells, each cell's area list and the area pointers in the lists.
 */
void CamData_Relocate(CamDataHdr *hdr)
{
    CamArea *area;
    CamCell *cell;
    CamArea **areaList;
    s32 left;
    s32 listTotal;

    gCamCtl.data = hdr; /* the offsets in the file become pointers below */
    if (hdr == NULL) {
        return;
    }
    area = (CamArea *)(hdr + 1);
    hdr->areas = area;
    left = hdr->areaCount;
    if (left != 0) {
        do {
            if (area->trans != NULL) {
                area->faces = (CamFace *)((u8 *)area + (s32)area->faces);
                area->trans = (u8 *)area + (s32)area->trans;
                area = (CamArea *)((u8 *)((s32)(area + 1) + area->faceCount * 0x40) + 0x20);
            } else {
                area->faces = (CamFace *)((u8 *)area + (s32)area->faces);
                area = (CamArea *)((s32)(area + 1) + area->faceCount * 0x40);
            }
        } while (--left != 0);
    }
    cell = (CamCell *)((u8 *)hdr + (s32)hdr->cells);
    hdr->cells = cell;
    areaList = (CamArea **)((u8 *)hdr + (s32)hdr->areaLists);
    hdr->areaLists = areaList;
    listTotal = hdr->gridH;
    left = hdr->gridW * listTotal;
    listTotal = 0;
    if (left != 0) {
        do {
            if (cell->count != 0) {
                cell->areas = (CamArea **)((u8 *)hdr + (s32)cell->areas);
                listTotal += cell->count;
            }
            cell++;
        } while (--left != 0);
    }
    if (listTotal != 0) {
        do {
            *areaList = (CamArea *)((u8 *)hdr + (s32)*areaList);
            listTotal--;
            areaList++;
        } while (listTotal != 0);
    }
}

void CamCtl_ReadPad(void *c);
s32 Cam_FindPlayerArea(CamCtl *c);
s32 CamArea_TestObj(CamArea *area, Player *obj, u8 mask);
void CamCtl_RunMode(CamCtl *c);
void CamCtl_CheckEmInView(void);
void Camera_Update(void);

/*
 * The camera controller's frame. mode (CamCtl.mode) holds flags that switch
 * off the monster push (bit 0, Cam_PushOutOfEms) and the wall line test
 * (bit 1, Cam_LineHitDist). Nothing happens while the followed player is
 * inactive. On the first frame it finds the player's camera area and runs
 * each mode's init handler (gCamModeInit). Every frame it tracks the area
 * the player is in and picks areaMode from the area's kind (CAM_AREA_FOLLOW
 * the follow camera, the others the rail), runs the modes' update handlers
 * in order, and CamCtl_RunMode hands the current or the overriding view to
 * the camera. The target object request lasts one frame.
 */
void CamCtl_Update(s32 mode)
{
    CamCtl *ctl = (CamCtl *)&gCamCtl;
    CamView *view;
    Player *pl;
    u32 i;

    gCamCtl.mode = mode;
    gCamCtl.followView = 0;
    if ((pl = gCamCtl.player)->active == 0) {
        return;
    }
    CamCtl_ReadPad(ctl);
    ctl->unk3AC = 0;
    ctl->cellZ = 0;
    ctl->cellX = 0;
    ctl->areaChanged = 0;
    if (ctl->inited == 0) {
        ctl->inited++;
        ctl->area = NULL;
        ctl->prevAreaId = CAM_AREA_ID_NONE;
        ctl->areaId = CAM_AREA_ID_NONE;
        Cam_FindPlayerArea(ctl);
        ctl->areaId = ctl->area->id;
        ctl->areaMode = CAM_MODE_FOLLOW;
        if (ctl->area->kind == CAM_AREA_FOLLOW) {
            ctl->areaMode = CAM_MODE_FOLLOW;
        } else {
            ctl->areaMode = CAM_MODE_RAIL;
        }
        ctl->prevAreaMode = ctl->areaMode;
        view = ctl->modes;
        for (i = 0; i < CAM_MODE_COUNT; i++, view++) {
            view->kind = i;
            *(s32 *)&view->step = 0;
            gCamModeInit[i](ctl, view);
        }
    }
    if (ctl->areaId == 0) {
        if (Cam_FindPlayerArea(ctl) > 0) {
            ctl->prevAreaId = ctl->areaId;
            ctl->areaId = ctl->area->id;
            ctl->areaChanged = 1;
        }
    } else if (CamArea_TestObj(ctl->area, pl, 2) != 0) {
        ctl->prevAreaId = ctl->areaId;
        ctl->areaChanged = 1;
        if (Cam_FindPlayerArea(ctl) >= 0) {
            ctl->areaId = ctl->area->id;
        }
    }
    ctl->prevAreaMode = ctl->areaMode;
    switch (ctl->area->kind) {
    case CAM_AREA_FOLLOW:
        ctl->areaMode = CAM_MODE_FOLLOW;
        break;
    case CAM_AREA_FIXED:
    case CAM_AREA_PATH:
    case CAM_AREA_OFFSET:
        ctl->areaMode = CAM_MODE_RAIL;
        break;
    }
    view = ctl->modes;
    for (i = CAM_MODE_COUNT; i != 0; i--) {
        gCamModeUpdate[view->kind](ctl, view);
        view++;
    }
    CamCtl_RunMode(ctl);
    ctl->targetObj = 0;
    Camera_Update();
    if (ctl->followView != 0) {
        CamCtl_CheckEmInView();
    }
}

extern Camera *gCamera;
void CamScript_Update(CamCtl *c, CamView *v);
void Camera_SetRoll(f32 roll);
void Camera_StoreFov(f32 fov);
void Camera_Update(void);

/*
 * Runs only the camera script's mode (CamScript_Update), hands its eye,
 * target, roll and fov to the camera and updates it (seen: plsel).
 */
void CamCtl_Apply(void)
{
    CamView *view = &gCamCtl.modes[CAM_MODE_SCRIPT];

    CamScript_Update(&gCamCtl, view);
    Vec_Copy(gCamera->eye, view->eye);
    Vec_Copy(gCamera->at, view->at);
    Camera_SetRoll(view->roll);
    Camera_StoreFov(view->fov);
    Camera_Update();
}

s32 CamCtl_GetMode(void)
{
    return gCamCtl.mode;
}

extern CamDistSet gCamFollowDefaults;
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/*
 * The follow camera's init handler (CAM_MODE_FOLLOW; its work is
 * CamCtl.follow). Its settings are the camera area's in a CAM_AREA_FOLLOW
 * area and gCamFollowDefaults elsewhere, at setting distNo. The eye goes
 * behind the player, the goals start at the current eye and target, the
 * floor under the target is taken, and the follow delay is set to 3 frames.
 */
void CamFollow_Init(CamCtl *ctl, CamView *view)
{
    f32 ofsLocal[4];
    f32 ofsWorld[4];
    s32 angles[4];
    f32 mtx[16];
    CamFollow *work;
    Player *pl;

    pl = ctl->player;
    view->yawFrom = CAM_YAW_FROM_NONE;
    work = &ctl->follow;
    if (ctl->area->kind != CAM_AREA_FOLLOW) {
        work->set = &gCamFollowDefaults;
    } else {
        work->set = &ctl->area->follow;
    }
    work->dist = &work->set->dist[work->distNo];
    work->fov = work->fovGoal = work->set->fov;
    work->roll = work->set->roll;
    work->yaw = pl->rot[1] + 0x8000;
    angles[0] = 0;
    angles[1] = work->yaw;
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    ofsLocal[0] = 0.0f;
    ofsLocal[1] = work->dist->back;
    ofsLocal[2] = work->dist->up;
    Vec_Rotate(ofsWorld, ofsLocal, mtx);
    work->at[0] = pl->pos[0];
    work->at[1] = pl->pos[1] + work->dist->atY;
    work->at[2] = pl->pos[2];
    work->eye[0] = work->at[0] + ofsWorld[0];
    work->eye[1] = pl->pos[1] + ofsWorld[1];
    work->eye[2] = work->at[2] + ofsWorld[2];
    work->yawGoal = work->yaw;
    Vec_Copy(work->eyeGoal, work->eye);
    Vec_Copy(work->atGoal, work->at);
    work->floorY = AreaGrid2_GetFloorY(work->at);
    work->unk54 = -1;
    work->unk79 = 0;
    Vec_Copy(work->unk18, work->eye);
    *(s32 *)&work->hitDist = 0;
    work->unk58 = 0;
    work->unk60 = 0;
    work->unk73 = 0;
    work->heightState = CAM_HEIGHT_NORMAL;
    work->heightTimer = 0;
    work->unk75 = 0;
    work->blockedTimer = 0;
    work->turningBehind = 0;
    work->unk78 = 0;
    work->unk76 = 0;
    work->followDelay = 3;
}

typedef struct Vec3 {
    f32 x, y, z;
} Vec3;

extern CamDist gCamFallDist;
extern CamDist gCamBlockedDists[2];
extern s16 gCamOrbitExitBlend;
extern f32 gPlayerBodySpheres[3];
void CamFollow_UpdateCanFollow(CamFollow *w, Player *pl);
void CamFollow_GetKeys(CamCtl *c, CamFollow *o);
void CamFollow_UpdateBlocked(CamCtl *c, CamFollow *m);
s32 CamFollow_UpdateFallView(CamFollow *m);
f32 CamFollow_FloorY(f32 *pos, Player *obj);
s16 Vec_AngleXZ16(f32 *a, f32 *b);
s32 Angle_Bump(u32 a, f32 lo, f32 hi, f32 amp);
s32 CamFollow_ShouldTurnBehind(Player *pl, CamFollow *w);
f32 Cam_LineHitDist(f32 *from, f32 *to, f32 *dist);
void Cam_PushOutOfEms(f32 *pos, f32 *unused1);
void CamView_KeepTargetOnScreen(CamCtl *c, CamView *v);
void CamView_StartBlend(CamView *p, s32 n);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_Add(f32 *out, f32 *a, f32 *b);
f32 AreaGrid2_GetFloorY(f32 *pos);
f32 CamBlend_EaseStep(CamView *v);
u32 Player_TestFlag(void *obj, u32 flag);
/* unit's own declaration: s32 arguments and result (the definition's s16 and u8 ones give CamFollow_Update other code) */
s32 Player_IsState(void *obj, s32 a, s32 b);
f32 Math_Sqrtf(f32 x);
s32 AreaGrid_SweepSphere(f32 *from, f32 *to, f32 *hit, u16 flags, f32 radius);

void CamView_SetTransition(CamCtl *p, CamView *t, CamArea *area);
/*
 * The follow camera's update handler (CAM_MODE_FOLLOW; its work is
 * CamCtl.follow), active while areaMode is the follow camera.
 * Distance setting: CAM_DIST_FALL in the fall view, CAM_DIST_BLOCKED (with
 * a 70-degree fov) while blockedTimer runs, else distNo, which PAD_UP and
 * PAD_DOWN step (gScene.camControl swaps them); CAM_DIST_CROUCH replaces
 * CAM_DIST_DEFAULT while the player crouches.
 * Yaw goal: toward the battle target; from another mode's view (yawFrom);
 * turned by the D-pad's left and right; behind the player; following the
 * player's heading (gScene.camFollow); or swung back after a wall hit. When
 * the orbit view ends, the camera blends back at the setting nearest the
 * orbit eye's height.
 * It then places the eye goal around the player (high while he rides a
 * monster, low on ledges), keeps the eye above the floor and out of walls
 * (AreaGrid_SweepSphere) and monsters (Cam_PushOutOfEms), and the target at
 * most 30 degrees above the eye. After an area change it blends in from
 * CamCtl.out.
 */
void CamFollow_Update(CamCtl *ctl, CamView *view)
{
    Vec3 off;
    Vec3 rotOfs;
    s32 angles[3];
    Vec3 bodyCenter;
    Vec3 hit;
    Vec3 sweepFrom;
    Vec3 pos;
    Vec3 eyeLevelAt;
    Vec3 eyeHit;
    Vec3 probe;
    f32 maxDist;
    f32 mtx[16];
    CamFollow *work;
    Player *pl;
    CamArea *area;
    u16 distKind;
    u8 autoFollowNow;
    u16 prevDistNo;
    s32 i;
    s32 bestNo;
    f32 bestErr;
    f32 factor;
    f32 raiseLimit;
    f32 floorY;
    f32 dx;
    f32 goalDistSq;
    f32 eyeDistSq;
    f32 len;
    f32 goalDx;
    Vec3 toAt;
    f32 farDist;
    f32 curDx;
    f32 curDz;
    f32 curDist;
    f32 dz;
    f32 dxSq;
    f32 dzSq;
    f32 lenSq;
    f32 horiz;
    f32 goalDz;
    s16 turnStep;
    s32 fromMode;
    f32 *fromEye;
    CamDist *setting;
    s32 yawDiff;
    u32 yawGap;
    s32 eyeBlocked;

    if (ctl->areaMode != CAM_MODE_FOLLOW) {
        view->active = 0;
        view->step = 0;
        view->blend = 0;
        return;
    }
    work = &ctl->follow;
    view->active = 1;
    ctl->followView = 1;
    ctl->playerFade = 1.0f;
    pl = ctl->player;
    area = ctl->area;
    CamFollow_UpdateCanFollow(work, pl);
    CamFollow_GetKeys(ctl, work);
    Vec_Copy(work->unk18, work->eye);
    work->fovGoal = work->set->fov;
    if (area->kind != CAM_AREA_FOLLOW) {
        work->set = &gCamFollowDefaults;
    } else {
        work->set = &area->follow;
    }
    if (CamFollow_UpdateFallView(work) == 1) {
        distKind = CAM_DIST_FALL;
        work->dist = &gCamFallDist;
    } else if (work->blockedTimer != 0) {
        distKind = CAM_DIST_BLOCKED;
        work->dist = &gCamBlockedDists[0];
        work->fovGoal = 1.2217306f;
    } else {
        prevDistNo = work->distNo;
        if (gScene.camControl == 0) {
            if ((work->pressed & PAD_DOWN) && work->distNo > 0) {
                work->distNo--;
            }
            if ((work->pressed & PAD_UP) && work->distNo < CAM_DIST_MAX) {
                work->distNo++;
            }
        } else {
            if ((work->pressed & PAD_UP) && work->distNo > 0) {
                work->distNo--;
            }
            if ((work->pressed & PAD_DOWN) && work->distNo < CAM_DIST_MAX) {
                work->distNo++;
            }
        }
        if (prevDistNo != work->distNo) {
            work->unk76 = 8;
        }
        if (pl->action == 1 && work->distNo == CAM_DIST_DEFAULT) {
            distKind = CAM_DIST_CROUCH;
            work->dist = &work->set->dist[CAM_DIST_CROUCH];
        } else {
            work->dist = &work->set->dist[work->distNo];
            distKind = work->distNo;
        }
    }
    autoFollowNow = 0;
    if (ctl->targetObj != NULL) {
        work->wallHit = 0;
        work->yawGoal = pl->targetAngle + 0x8000;
    } else if ((fromMode = view->yawFrom) != CAM_YAW_FROM_NONE) {
        view->yawFrom = CAM_YAW_FROM_NONE;
        fromEye = ctl->modes[fromMode].eye;
        work->yaw = work->yawGoal = Vec_AngleXZ16(fromEye, fromEye + 3);
    } else if (ctl->prevAreaMode != CAM_MODE_FOLLOW && ctl->prevAreaMode != CAM_MODE_ORBIT) {
        work->yaw = work->yawGoal = Vec_AngleXZ16(ctl->out.eye, ctl->out.at);
    } else if (work->held & (PAD_LEFT | PAD_RIGHT)) {
        if (gScene.camControl != 2) {
            turnStep = 0x47E;
        } else {
            turnStep = -0x47E;
        }
        if (work->held & PAD_LEFT) {
            work->yawGoal += turnStep;
        }
        if (work->held & PAD_RIGHT) {
            work->yawGoal -= turnStep;
        }
        work->unk76 = 8;
    } else if (CamFollow_ShouldTurnBehind(pl, work) != 0) {
        work->yawGoal = pl->rot[1] + 0x8000;
        work->turningBehind = 1;
        work->unk76 = 0x14;
        autoFollowNow = work->unk78;
    } else if (work->heightState == CAM_HEIGHT_NORMAL && work->canFollow >= 0 && gScene.camFollow == 0) {
        yawDiff = (pl->rot[1] ^ 0x8000) - work->yawGoal & 0xFFFF;
        if (work->canFollow == 0) {
            work->yawGoal = work->yawGoal + Angle_Bump(yawDiff, 4096.0f, 26624.0f, 0.00021701389f);
        } else {
            work->yawGoal = work->yawGoal + Angle_Bump(yawDiff, 0.0f, 26624.0f, 0.00021701389f);
        }
        autoFollowNow = 1;
    } else if (work->held & PAD_L1) {
        if (Player_TestFlag(pl, 8) != 0) {
            work->yawGoal = work->yawGoal + Angle_Bump((pl->rot[1] ^ 0x8000) - work->yawGoal & 0xFFFF, 4096.0f, 26624.0f, 0.00013563369f);
        }
    } else if (work->wallHit != 0) {
        work->yawGoal += (s16)((s16)((pl->rot[1] ^ 0x8000) - work->yawGoal) >> 3);
        work->wallHit = 0;
    }
    work->unk78 = autoFollowNow;
    work->yaw += (s16)((s16)(work->yawGoal - work->yaw) >> 2);
    if (work->turningBehind != 0 && ((yawGap = (u16)(work->yawGoal - work->yaw)) < 0x1000 || yawGap > 0xF000)) {
        work->turningBehind = 0;
    }
    if (ctl->prevAreaMode == CAM_MODE_ORBIT && work->unk78 == 0) {
        CamView_StartBlend(view, gCamOrbitExitBlend);
        Vec_Diff(&off.x, ctl->modes[CAM_MODE_ORBIT].eye, pl->pos);
        setting = &work->set->dist[1];
        bestErr = 1e8f;
        bestNo = 0;
        for (i = 1; i < 4; i++, setting++) {
            factor = setting->back - off.y;
            if (bestErr > factor * factor) {
                bestErr = factor * factor;
                bestNo = i;
            }
        }
        work->distNo = bestNo;
        distKind = (u8)bestNo;
        work->dist = &work->set->dist[bestNo];
    }
    off.x = 0.0f;
    if (pl->rideState != 3 && pl->rideState != 0) {
        off.y = 400.0f;
        off.z = 400.0f;
        work->atGoal[1] = 100.0f + pl->pos[1];
    } else if (pl->rideState != 0 || Player_IsState(pl, 0, 0x11) != 0 ||
               Player_IsState(pl, 0, 0x1B) != 0 || Player_IsState(pl, 0, 0x1E) != 0 ||
               Player_IsState(pl, 0, 0x3A) != 0) {
        off.y = 20.0f;
        off.z = work->dist->up;
        work->atGoal[1] = 30.0f + pl->pos[1];
    } else {
        off.y = work->dist->back;
        off.z = work->dist->up;
        /* Kept for the match: an empty check of atY. It compiles to nothing but makes MWCC
         * load the add's two operands in the original's order (docs/mwcc-matching.md,
         * "Statements scheduled as if a check were still there"). */
        if (work->dist->atY < 0.0f || work->dist->atY > work->dist->up) {
        }
        work->atGoal[1] = pl->pos[1] + work->dist->atY;
    }
    work->atGoal[0] = pl->pos[0];
    work->atGoal[2] = pl->pos[2];
    angles[2] = 0;
    angles[0] = 0;
    angles[1] = work->yaw;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(&rotOfs.x, &off.x, mtx);
    work->eyeGoal[0] = work->atGoal[0] + rotOfs.x;
    work->eyeGoal[1] = pl->pos[1] + rotOfs.y;
    work->eyeGoal[2] = work->atGoal[2] + rotOfs.z;
    if (view->step == 0) {
        view->step++;
        Vec_Copy(work->eye, work->eyeGoal);
        Vec_Copy(work->at, work->atGoal);
        work->fov = work->fovGoal;
    }
    work->at[0] = work->atGoal[0];
    work->at[2] = work->atGoal[2];
    work->at[1] += 0.5f * (work->atGoal[1] - work->at[1]);
    if (CamFollow_IsBlownAway() == 1) {
        Vec_Add(&bodyCenter.x, pl->pos, gPlayerBodySpheres);
        if (AreaGrid_SweepSphere(&bodyCenter.x, work->at, &off.x, 4, 33.0f) != 0) {
            Vec_Copy(work->at, &off.x);
        }
    }
    work->fov += 0.05f * (work->fovGoal - work->fov);
    raiseLimit = 100.0f;
    floorY = AreaGrid2_GetFloorY(work->atGoal);
    if (-(floorY - work->floorY) < 210.0f) {
        work->unk54 = 0;
        work->floorY = floorY;
        if (work->unk79 != 0) {
            raiseLimit = 256.0f;
            work->unk79 = 0;
        }
    } else {
        switch (work->unk54) {
        case 0:
            work->unk54 = 1;
        case 1:
            raiseLimit = 256.0f;
            break;
        default:
            work->unk54 = 0;
            work->floorY = floorY;
            break;
        }
        work->unk79 = 0;
    }
    floorY = CamFollow_FloorY(work->eyeGoal, pl);
    if (floorY - work->eyeGoal[1] < raiseLimit) {
        floorY += work->dist->floorGap;
        if (floorY > work->eyeGoal[1]) {
            work->eyeGoal[1] = floorY;
            work->unk79 = 1;
        } else {
            work->unk54 = -1;
        }
    }
    if (work->heightState != CAM_HEIGHT_NORMAL) {
        work->unk75 = 0;
        work->eye[1] += 0.25f * (work->eyeGoal[1] - work->eye[1]);
        work->eye[0] = work->eyeGoal[0];
        work->eye[2] = work->eyeGoal[2];
    } else if (work->unk75 == 0) {
        eyeBlocked = AreaGrid_SweepSphere(work->eye, work->eyeGoal, &hit.x, CAM_SWEEP_MASK, 28.0f);
        work->wallHit = AreaGrid_SweepSphere(work->at, work->eyeGoal, &hit.x, CAM_SWEEP_MASK, 28.0f);
        if (eyeBlocked != 0 && work->wallHit != 0) {
            work->eye[0] = hit.x;
            work->eye[1] = hit.y;
            work->eye[2] = hit.z;
            work->unk75 = 1;
            work->unk76 = 8;
        } else {
            work->eye[1] += 0.125f * (work->eyeGoal[1] - work->eye[1]);
            work->eye[0] = work->eyeGoal[0];
            work->eye[2] = work->eyeGoal[2];
        }
    } else {
        Vec_Copy(&sweepFrom.x, work->at);
        if (work->eyeGoal[1] < sweepFrom.y) {
            sweepFrom.y = 0.5f * (sweepFrom.y + work->eyeGoal[1]);
        }
        if (AreaGrid_SweepSphere(&sweepFrom.x, work->eyeGoal, &hit.x, CAM_SWEEP_MASK, 28.0f) != 0) {
            if (work->unk76 != 0) {
                work->eye[0] = hit.x;
                work->eye[1] = hit.y;
                work->eye[2] = hit.z;
                work->unk18[0] = hit.x;
                work->unk18[1] = hit.y;
                work->unk18[2] = hit.z;
            } else {
                maxDist = work->dist->up;
                dx = work->eye[0] - work->at[0];
                dz = work->eye[2] - work->at[2];
                eyeDistSq = dx * dx + dz * dz;
                if (eyeDistSq > maxDist * maxDist) {
                    factor = maxDist / Math_Sqrtf(eyeDistSq);
                    pos.x = work->at[0] + dx * factor;
                    pos.y = work->eye[1];
                    pos.z = work->at[2] + dz * factor;
                    work->wallHit = AreaGrid_SweepSphere(work->at, &pos.x, &hit.x, CAM_SWEEP_MASK, 28.0f);
                    if (work->wallHit != 0) {
                        Vec_Copy(work->eye, &hit.x);
                    } else {
                        Vec_Copy(work->eye, &pos.x);
                    }
                }
                if (work->unk78 == 0) {
                    work->yawGoal = Vec_AngleXZ16(work->eye, work->at);
                }
            }
        } else {
            work->unk75 = 0;
        }
    }
    if (work->unk76 != 0) {
        work->unk76--;
    }
    if (work->unk75 == 0) {
        if (work->heightState == CAM_HEIGHT_NORMAL) {
            Cam_PushOutOfEms(work->eye, work->unk18);
            Cam_LineHitDist(work->eye, work->unk18, &work->hitDist);
        } else {
            eyeLevelAt.x = work->at[0];
            eyeLevelAt.y = work->eye[1];
            eyeLevelAt.z = work->at[2];
            if (AreaGrid_SweepSphere(&eyeLevelAt.x, work->eye, &eyeHit.x, CAM_SWEEP_MASK, 28.0f) != 0) {
                Vec_Copy(work->eye, &eyeHit.x);
                work->yawGoal += 0x400;
            }
        }
    }
    if (AreaGrid_SweepSphere(work->eye, work->eyeGoal, &probe.x, CAM_SWEEP_MASK, 28.0f) != 0) {
        Vec_Diff(&probe.x, work->eye, work->at);
        farDist = 150.0f + work->dist->up;
        if (probe.x * probe.x + probe.z * probe.z > farDist * farDist) {
            Vec_Copy(work->eye, work->eyeGoal);
            Vec_Copy(work->unk18, work->atGoal);
            Cam_PushOutOfEms(work->eye, work->unk18);
            Cam_LineHitDist(work->eye, work->unk18, &work->hitDist);
        }
    }
    if (work->unk75 != 0 && distKind > 0 && distKind < 5) {
        goalDx = work->atGoal[0] - work->eyeGoal[0];
        goalDz = work->atGoal[2] - work->eyeGoal[2];
        goalDistSq = goalDx * goalDx + goalDz * goalDz;
        curDx = work->at[0] - work->eye[0];
        curDz = work->at[2] - work->eye[2];
        curDist = curDx * curDx + curDz * curDz;
        if (goalDistSq > curDist) {
            curDist = Math_Sqrtf(curDist);
            if (curDist > 0.0f) {
                factor = Math_Sqrtf(goalDistSq) / curDist;
                work->at[0] = work->eye[0] + curDx * factor;
                work->at[2] = work->eye[2] + curDz * factor;
            }
        }
    }
    floorY = CamFollow_FloorY(work->eye, pl);
    floorY += work->dist->floorGap;
    if (floorY > work->eye[1]) {
        work->eye[1] = floorY;
    }
    if (work->eye[1] < work->at[1]) {
        Vec_Diff(&toAt.x, work->at, work->eye);
        dxSq = toAt.x * toAt.x;
        dzSq = toAt.z * toAt.z;
        lenSq = dxSq + toAt.y * toAt.y + dzSq;
        if (lenSq > 0.0f) {
            horiz = dxSq + dzSq;
            if (0.75f > horiz / lenSq) {
                horiz = Math_Sqrtf(horiz);
                horiz *= 0.57735f;
                work->at[1] = work->eye[1] + horiz;
            }
        }
    }
    CamView_SetTransition(ctl, view, area);
    CamFollow_UpdateBlocked(ctl, work);
    Vec_Copy(view->eye, work->eye);
    Vec_Copy(view->at, work->at);
    switch (view->blend) {
    case 0:
        view->roll = work->roll;
        view->fov = work->fov;
        break;
    case 1:
        Vec_Copy(view->eye0, ctl->out.eye);
        Vec_Copy(view->at0, ctl->out.at);
        view->roll0 = ctl->out.roll;
        view->fov0 = ctl->out.fov;
        view->blend++;
    case 2:
        factor = CamBlend_EaseStep(view);
        Vec_Lerp(view->eye, view->eye0, view->eye, factor);
        Vec_Lerp(view->at, view->at0, view->at, factor);
        view->roll = view->roll0 * factor + work->roll * (1.0f - factor);
        view->fov = view->fov0 * factor + work->fov * (1.0f - factor);
        break;
    }
    CamView_KeepTargetOnScreen(ctl, view);
}


/*
 * 1 while the follow camera is the view shown and is turning behind the
 * player (turningBehind), else 0.
 */
s32 CamCtl_IsTurningBehind(void)
{
    CamCtl *ctl = (CamCtl *)&gCamCtl;

    if (gCamCtl.override >= 0) {
        return 0;
    }
    if (ctl->areaMode != CAM_MODE_FOLLOW) {
        return 0;
    }
    return ctl->follow.turningBehind != 0;
}

/*
 * When obj is the local player, makes the follow camera turn toward the
 * player's battle target on its next frame (seen: the battle alert).
 */
void CamCtl_SetTargetObj(u8 *obj)
{
    CamCtl *ctl = (CamCtl *)&gCamCtl;

    if (*(u16 *)(obj + 0xC) == gScene.localPlayer) {
        ctl->targetObj = obj;
    }
}


/*
 * When obj is the local player: turns the follow camera to an eighth of a
 * turn off the player's front (heading + 0x2000), at the default distance
 * setting, and clears its wall hit (seen: Player_StateSpitRoastStart).
 */
void CamCtl_TurnToPlayerFront(u8 *obj)
{
    if (*(u16 *)(obj + 0xC) == gScene.localPlayer) {
        CamFollow *work = &gCamCtl.follow;

        work->wallHit = 0;
        work->yawGoal = *(s32 *)(obj + 0xA4) + 0x2000;
        work->distNo = CAM_DIST_DEFAULT;
    }
}

s32 Cam_PlayerFree(Player *pl);
s32 Player_OnMountedGun(struct Player *p);

/*
 * Gives the follow camera's work the camera's pad buttons while the follow
 * camera is current and the script view is off: all of them while the
 * player is free (Cam_PlayerFree), less the D-pad on the mounted gun with L1
 * held; else, unless the keyboard is open, only L1 (L1 and R1 with the bow
 * drawn, outside the hit and gesture groups). Otherwise none.
 */
void CamFollow_GetKeys(CamCtl *ctl, CamFollow *work)
{
    Player *pl;

    if (ctl->areaMode == CAM_MODE_FOLLOW && ctl->modes[CAM_MODE_SCRIPT].active == 0) {
        pl = ctl->player;
        if (Cam_PlayerFree(pl) == 1) {
            work->held = ctl->padHeld;
            work->pressed = ctl->padPressed;
            if (Player_OnMountedGun(pl) == 1 && (pl->input.held & PAD_L1)) {
                work->held &= (PAD_ALL & ~PAD_DPAD);
                work->pressed &= (PAD_ALL & ~PAD_DPAD);
            }
            return;
        }
        if ((s8)Kbd_IsOpen() == 0) {
            if (pl->weaponType == 0xA && pl->weaponOut != 0 && pl->stateGroup != 2 && pl->stateGroup != 6) {
                work->held = ctl->padHeld & (PAD_R1 | PAD_L1);
                work->pressed = ctl->padPressed & (PAD_R1 | PAD_L1);
                return;
            }
            work->held = ctl->padHeld & PAD_L1;
            work->pressed = ctl->padPressed & PAD_L1;
            return;        }
    }
    work->pressed = 0;
    work->held = 0;
}



/*
 * Makes the follow camera take its yaw from mode's view on its next update.
 * A negative mode or the script mode gives none (CAM_YAW_FROM_NONE).
 */
void CamFollow_SetYawFrom(s32 mode)
{
    CamView *view = &gCamCtl.modes[CAM_MODE_FOLLOW];

    if (mode == CAM_MODE_SCRIPT) {
        mode = -1;
    }
    if (mode >= 0) {
        view->yawFrom = mode;
    } else {
        view->yawFrom = -1;
    }
}

s32 AreaGrid_SweepSphere(f32 *from, f32 *to, f32 *hit, u16 flags, f32 radius);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
f32 Math_Atan2(f32 y, f32 x);

/*
 * The follow camera's blocked view: starts while the player is blown away
 * (offline) and the sweep from the target goal to the eye goal stops more
 * than 50 short. blockedTimer is set to CAM_BLOCKED_HOLD, held there while
 * he is blown away, then counts down; PAD_UP or PAD_DOWN ends it. distNo
 * becomes 1 and the yaw goal turns to the side the player came from.
 */
void CamFollow_UpdateBlocked(CamCtl *ctl, CamFollow *work)
{
    f32 delta[3];
    Player *pl;

    if (work->blockedTimer == CAM_BLOCKED_HOLD) {
        if (CamFollow_IsBlownAway() == 1) {
            return;
        }
        work->blockedTimer--;
        return;
    }
    if (work->blockedTimer != 0) {
        work->blockedTimer--;
        if (work->pressed & (PAD_UP | PAD_DOWN)) {
            work->blockedTimer = 0;
        }
    }
    if (CamFollow_IsBlownAway() == 1 && AreaGrid_SweepSphere(work->atGoal, work->eyeGoal, delta, CAM_SWEEP_MASK, 28.0f) != 0) {
        delta[0] -= work->eyeGoal[0];
        delta[2] -= work->eyeGoal[2];
        if (delta[0] * delta[0] + delta[2] * delta[2] > 2500.0f) {
            pl = ctl->player;
            work->blockedTimer = CAM_BLOCKED_HOLD;
            work->distNo = 1;
            Vec_Diff(delta, pl->prevPos, pl->pos);
            if (work->unk76 == 0) {
                work->yawGoal = RAD_TO_ANGLE16 * Math_Atan2(delta[0], delta[2]);
            }
        }
    }
}

/*
 * 1 while the follow camera uses its fall setting, else 0. The player's
 * kind each frame: 0 not airborne, 1 airborne, 2 dropping more than 10 a
 * frame, 3 dropping more than 300 above the floor. Four frames of kind 3
 * start the fall view (CAM_HEIGHT_FALL); it holds while the kind is 1 or 3
 * and for 15 frames after. A hit or a status ends it at once.
 */
s32 CamFollow_UpdateFallView(CamFollow *work)
{
    Player *pl = gCamCtl.player;
    u8 kind;
    f32 ground;

    if (pl->stateGroup == 2) {
        work->heightState = CAM_HEIGHT_NORMAL;
        return 0;
    }
    kind = 0;
    if (pl->action == 2) {
        kind = 1;
        if (pl->pos[1] < pl->prevPos[1] - 10.0f) {
            kind = 2;
            ground = AreaGrid2_GetFloorY(pl->pos);
            if (!(pl->pos[1] <= 300.0f + ground)) {
                kind = 3;
            }
        }
    }
    switch (work->heightState) {
    case CAM_HEIGHT_NORMAL:
        if (kind <= 2) {
            work->heightTimer = 4;
            break;
        }
        work->heightTimer -= 1;
        if (work->heightTimer > 0) {
            break;
        }
        work->heightState++;
    case CAM_HEIGHT_FALL:
        if (kind & 1) {
            work->heightTimer = 15;
            return 1;
        }
        work->heightTimer -= 1;
        if (work->heightTimer > 0) {
            return 1;
        }
        work->heightState = CAM_HEIGHT_NORMAL;
        break;
    }
    return 0;
}

/*
 * The floor height under pos; when the player pl is above that floor, the
 * floor found from 150 higher instead.
 */
f32 CamFollow_FloorY(f32 *pos, Player *pl)
{
    f32 probe[3];
    f32 floorY = AreaGrid2_GetFloorY(pos);

    if (pl->pos[1] <= floorY) {
        return floorY;
    }
    Vec_Copy(probe, pos);
    probe[1] += 150.0f;
    return AreaGrid2_GetFloorY(probe);
}

s32 Cam_PlayerCanFollow(Player *pl);
s32 Player_InBlownAway(struct Player *p);

/*
 * Updates canFollow from Cam_PlayerCanFollow (-1: the yaw doesn't follow
 * the player). A change from negative to 0 or 1 waits followDelay frames.
 * Also sets blownAway while offline and the player is blown away.
 */
void CamFollow_UpdateCanFollow(CamFollow *work, Player *pl)
{
    s8 newCanFollow = Cam_PlayerCanFollow(pl);

    if (newCanFollow >= 0) {
        if (work->canFollow < 0) {
            if (pl->weaponType == 0xA) {
                work->followDelay -= 1;
            } else if (newCanFollow == 1) {
                work->followDelay -= 1;
            }
            if (work->followDelay > 0) {
                newCanFollow = -1;
            }
        }
    } else {
        work->followDelay = 3;
    }
    work->canFollow = newCanFollow;
    work->blownAway = 0;
    if (gScene.online == 0 && Player_InBlownAway(pl) == 1) {
        work->blownAway = 1;
    }
}

s8 CamFollow_GetCanFollow(void)
{
    return gCamCtl.follow.canFollow;
}

s32 CamFollow_IsBlownAway(void)
{
    return gCamCtl.follow.blownAway != 0;
}

/*
 * The orbit camera's init handler: does nothing (CamOrbit_Update sets the
 * work up at step 0).
 */
void CamOrbit_Init(CamCtl *ctl, CamView *view)
{
}

void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
f32 CamBlend_EaseStep(CamView *v);
extern f32 gCamOrbitEyeOfs[3];
extern f32 gCamOrbitAimOfs[3];

/*
 * The orbit camera's update handler (CAM_MODE_ORBIT; its work is
 * CamCtl.orbit): an over-the-shoulder view that runs while the follow view
 * is active and follows the player's heading, and then sets areaMode to the
 * orbit. It starts with a 9-frame blend from the follow camera; its pitch
 * follows the player's aimWeight and its yaw eases to 0x800 to one side or
 * the other. The eye is placed around a target 170 above the player, kept
 * out of walls and above the floor; the target then moves to where the line
 * from the eye to a point ahead (gCamOrbitAimOfs) crosses the vertical
 * plane through the target. For the match, the reciprocal of the
 * denominator is taken before the numerator, and the floor gap is added in
 * a statement of its own (the original's order).
 */
void CamOrbit_Update(CamCtl *ctl, CamView *view)
{
    f32 pos[3];
    f32 rotOfs[3];
    f32 eyeOfs[3];
    f32 aimOfs[3];
    s32 angles[3];
    f32 hit[3];
    f32 mtx[16];
    f32 line[2][3];
    Player *pl;
    CamOrbit *orbitWork;
    CamFollow *followWork;
    f32 y;
    f32 factor;
    f32 planeX;
    f32 planeZ;
    f32 normalX;
    f32 normalZ;
    f32 normalY;
    f32 planeY;
    f32 dirX;
    f32 dirY;
    f32 dirZ;
    f32 floorY;
    u32 faceDiff;

    if (ctl->modes[CAM_MODE_FOLLOW].active == 0) {
        view->active = 0;
        *(u16 *)&view->step = 0;
        return;
    }
    pl = ctl->player;
    followWork = &ctl->follow;
    orbitWork = &ctl->orbit;
    switch (view->step) {
    case 0:
        if (followWork->unk78 == 0) {
            view->active = 0;
            *(u16 *)&view->step = 0;
            return;
        }
        Vec_Copy(orbitWork->unk18, ctl->modes[CAM_MODE_FOLLOW].eye);
        orbitWork->pitch = (pl->aimWeight + 100) * 34 - 0xB00;
        orbitWork->dir = 0x800;
        orbitWork->yaw = 0;
        view->step++;
        view->blend = 1;
        view->left = 9;
        view->rate = 1.0f / view->left;
        break;
    case 1:
        orbitWork->pitch += (s16)((s16)((pl->aimWeight + 100) * 34 - 0xB00 - orbitWork->pitch) >> 2);
        if (followWork->unk78 == 0) {
            view->active = 0;
            *(u16 *)&view->step = 0;
            return;
        }
        break;
    }
    view->active = 1;
    ctl->areaMode = CAM_MODE_ORBIT;
    ctl->followView = 0;
    faceDiff = ((pl->rot[1] ^ 0x8000) - (followWork->yaw + orbitWork->yaw)) & 0xFFFF;
    if (faceDiff > 0x100 && faceDiff < 0x7800) {
        orbitWork->dir = -0x800;
    } else if (faceDiff > 0x8800 && faceDiff < 0xFF00) {
        orbitWork->dir = 0x800;
    }
    if (orbitWork->dir < 0) {
        if (orbitWork->yaw > -0x800) {
            orbitWork->yaw += (s16)((s16)(-0x800 - orbitWork->yaw) * 6 >> 6);
        }
    } else if (orbitWork->yaw < 0x800) {
        orbitWork->yaw += (s16)((s16)(0x800 - orbitWork->yaw) * 6 >> 6);
    }
    switch (view->blend) {
    case 0:
        Vec_Copy(eyeOfs, gCamOrbitEyeOfs);
        Vec_Copy(aimOfs, gCamOrbitAimOfs);
        y = 170.0f;
        break;
    case 1:
        (view->blend)++;
    case 2:
        factor = CamBlend_EaseStep(view);
        eyeOfs[0] = 0.0f;
        eyeOfs[1] = 0.0f;
        eyeOfs[2] = followWork->dist->up;
        Vec_Lerp(eyeOfs, eyeOfs, gCamOrbitEyeOfs, factor);
        aimOfs[2] = 0.0f;
        aimOfs[1] = 0.0f;
        aimOfs[0] = 0.0f;
        Vec_Lerp(aimOfs, aimOfs, gCamOrbitAimOfs, factor);
        y = followWork->dist->atY * factor + 170.0f * (1.0f - factor);
        break;
    }
    orbitWork->at[0] = pos[0] = pl->pos[0];
    orbitWork->at[1] = pos[1] = pl->pos[1] + y;
    orbitWork->at[2] = pos[2] = pl->pos[2];
    angles[0] = orbitWork->pitch;
    angles[1] = followWork->yaw + orbitWork->yaw;
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(rotOfs, eyeOfs, mtx);
    Vec_Add(orbitWork->eye, orbitWork->at, rotOfs);
    if (AreaGrid_SweepSphere(pos, orbitWork->eye, hit, CAM_SWEEP_MASK, 28.0f) != 0) {
        Vec_Copy(orbitWork->eye, hit);
    }
    floorY = CamFollow_FloorY(orbitWork->eye, pl);
    floorY += followWork->dist->floorGap;
    if (floorY > orbitWork->eye[1]) {
        orbitWork->eye[1] = floorY;
    }
    angles[1] = followWork->yaw;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(rotOfs, aimOfs, mtx);
    planeX = orbitWork->at[0];
    planeZ = orbitWork->at[2];
    normalX = rotOfs[0];
    normalZ = rotOfs[2];
    normalY = 0.0f;
    planeY = 0.0f;
    Vec_Copy(line[0], orbitWork->eye);
    Vec_Add(line[1], orbitWork->at, rotOfs);
    dirX = line[1][0] - line[0][0];
    dirY = line[1][1] - line[0][1];
    dirZ = line[1][2] - line[0][2];
    factor = 1.0f / (dirX * normalX + dirY * normalY + dirZ * normalZ);
    factor = (normalX * (planeX - line[1][0]) + normalY * (planeY - line[1][1]) + normalZ * (planeZ - line[1][2])) * factor;
    line[1][0] += dirX * factor;
    line[1][1] += dirY * factor;
    line[1][2] += dirZ * factor;
    Vec_Copy(orbitWork->at, line[1]);
    view->roll = followWork->roll;
    view->fov = followWork->fov;
    Vec_Copy(view->eye, orbitWork->eye);
    Vec_Copy(view->at, orbitWork->at);
}

/* unk_314E00's .data (0x4154E0-0x4187B0) */
extern char D_0046A930[];
extern char D_0046A938[];
extern char D_0046A940[];
extern char D_0046A948[];
extern char D_0046A950[];
extern char D_0046A958[];
extern char D_0046A960[];
extern char D_0046A968[];
extern char D_0046A970[];
extern char D_0046A978[];
extern char D_0046A980[];
extern char D_0046A988[];
extern char D_0046A990[];
extern char D_0046A998[];
extern char D_0046A9A0[];
extern char D_0046A9A8[];
extern char D_0046A9B0[];
extern char D_0046A9B8[];
extern char D_0046A9C0[];
extern char D_0046A9C8[];
extern char D_0046A9D0[];
extern char D_0046A9D8[];
extern char D_0046A9E0[];
extern char D_0046A9E8[];
extern char D_0046A9F0[];
extern char D_0046A9F8[];
extern char D_0046AA00[];
extern char D_0046AA08[];
extern char D_0046AA10[];
extern char D_0046AA18[];
extern char D_0046AA20[];
extern char D_0046AA28[];
extern char D_0046AA30[];
extern char D_0046AA38[];
extern char D_0046AA40[];
extern char D_0046AA48[];
extern char D_0046AA50[];
extern char D_0046AA58[];
extern char D_0046AA60[];
extern char D_0046AA68[];
extern char D_0046AA70[];
extern char D_0046AA78[];
extern char D_0046AA80[];
extern char D_0046AA88[];
extern char D_0046AA90[];
extern char D_0046AA98[];
extern char D_0046AAA0[];
extern char D_0046AAA8[];
extern char D_0046AAB0[];
extern char D_0046AAB8[];
extern char D_0046AAC0[];
extern char D_0046AAC8[];
extern char D_0046AAD0[];
extern char D_0046AAD8[];
extern char D_0046AAE0[];
extern char D_0046AAE8[];
extern char D_0046AAF0[];
extern char D_0046AAF8[];
extern char D_0046AB00[];
extern char D_0046AB08[];
extern char D_0046AB10[];
extern char D_0046AB18[];
extern char D_0046AB20[];
extern char D_0046AB28[];
extern char D_0046AB30[];
extern char D_0046AB38[];
extern char D_0046AB40[];
extern char D_0046AB48[];
extern char D_0046AB50[];
extern char D_0046AB58[];
extern char D_0046AB60[];
extern char D_0046AB68[];
extern char D_0046AB70[];
extern char D_0046AB78[];
extern char D_0046AB80[];
extern char D_0046AB88[];
extern char D_0046AB90[];
extern char D_0046AB98[];
extern char D_0046ABA0[];
extern char D_0046ABA8[];
extern char D_0046ABB0[];
extern char D_0046ABB8[];
extern char D_0046ABC0[];
extern char D_0046ABC8[];
extern char D_0046ABD0[];
extern char D_0046ABD8[];
extern char D_0046ABE0[];
extern char D_0046ABE8[];
extern char D_0046ABF0[];
extern char D_0046ABF8[];
extern char D_0046AC00[];
extern char D_0046AC08[];
extern char D_0046AC10[];
extern char D_0046AC18[];
extern char D_0046AC20[];
extern char D_0046AC28[];
extern char D_0046AC30[];
extern char D_0046AC38[];
extern char D_0046AC40[];
extern char D_0046AC48[];
extern char D_0046AC50[];
extern char D_0046AC58[];
extern char D_0046AC60[];
extern char D_0046AC68[];
extern char D_0046AC70[];
extern char D_0046AC78[];
extern char D_0046AC80[];
extern char D_0046AC88[];
extern char D_0046AC90[];
extern char D_0046AC98[];
extern char D_0046ACA0[];
extern char D_0046ACA8[];
extern char D_0046ACB0[];
extern char D_0046ACB8[];
extern char D_0046ACC0[];
extern char D_0046ACC8[];
extern char D_0046ACD0[];
extern char D_0046ACD8[];
extern char D_0046ACE0[];
extern char D_0046ACE8[];
extern char D_0046ACF0[];
extern char D_0046ACF8[];
extern char D_0046AD00[];
extern char D_0046AD08[];
extern char D_0046AD10[];
extern char D_0046AD18[];
extern char D_0046AD20[];
extern char D_0046AD28[];
extern char D_0046AD30[];
extern char D_0046AD38[];
extern char D_0046AD40[];
extern char D_0046AD48[];
extern char D_0046AD50[];
extern char D_0046AD58[];
extern char D_0046AD60[];
extern char D_0046AD68[];
extern char D_0046AD70[];
extern char D_0046AD78[];
extern char D_0046AD80[];
extern char D_0046AD88[];
extern char D_0046AD90[];
extern char D_0046AD98[];
extern char D_0046ADA0[];
extern char D_0046ADA8[];
extern char D_0046ADB0[];
extern char D_0046ADB8[];
extern char D_0046ADC0[];
extern char D_0046ADC8[];
extern char D_0046ADD0[];
extern char D_0046ADD8[];
extern char D_0046ADE0[];
extern char D_0046ADE8[];
extern char D_0046ADF0[];
extern char D_0046ADF8[];
extern char D_0046AE00[];
extern char D_0046AE08[];
extern char D_0046AE10[];
extern char D_0046AE18[];
extern char D_0046AE20[];
extern char D_0046AE28[];
extern char D_0046AE30[];
extern char D_0046AE38[];
extern char D_0046AE40[];
extern char D_0046AE48[];
extern char D_0046AE50[];
extern char D_0046AE58[];
extern char D_0046AE60[];
extern char D_0046AE68[];
extern char D_0046AE70[];
extern char D_0046AE78[];
extern char D_0046AE80[];
extern char D_0046AE88[];
extern char D_0046AE90[];
extern char D_0046AE98[];
extern char D_0046AEA0[];
extern char D_0046AEA8[];
extern char D_0046AEB0[];
extern char D_0046AEB8[];
extern char D_0046AEC0[];
extern char D_0046AEC8[];
extern char D_0046AED0[];
extern char D_0046AED8[];
extern char D_0046AEE0[];
extern char D_0046AEE8[];
extern char D_0046AEF0[];
extern char D_0046AEF8[];
extern char D_0046AF00[];
extern char D_0046AF08[];
extern char D_0046AF10[];
extern char D_0046AF18[];
extern char D_0046AF20[];
extern char D_0046AF28[];
extern char D_0046AF30[];
extern char D_0046AF38[];
extern char D_0046AF40[];
extern char D_0046AF48[];
extern char D_0046AF50[];
extern char D_0046AF58[];
extern char D_0046AF60[];
extern char D_0046AF68[];
extern char D_0046AF70[];
extern char D_0046AF78[];
extern char D_0046AF80[];
extern char D_0046AF88[];
extern char D_0046AF90[];
extern char D_0046AF98[];
extern char D_0046AFA0[];
extern char D_0046AFA8[];
extern char D_0046AFB0[];
extern char D_0046AFB8[];
extern char D_0046AFC0[];
extern char D_0046AFC8[];
extern char D_0046AFD0[];
extern char D_0046AFD8[];
extern char D_0046AFE0[];
extern char D_0046AFE8[];
extern char D_0046AFF0[];
extern char D_0046AFF8[];
extern char D_0046B000[];
extern char D_0046B008[];
extern char D_0046B010[];
extern char D_0046B018[];
extern char D_0046B020[];
extern char D_0046B028[];
extern char D_0046B030[];
extern char D_0046B038[];
extern char D_0046B040[];
extern char D_0046B048[];
extern char D_0046B050[];
extern char D_0046B058[];
extern char D_0046B060[];
extern char D_0046B068[];
extern char D_0046B070[];
extern char D_0046B078[];
extern char D_0046B080[];
extern char D_0046B088[];
extern char D_0046B090[];
extern char D_0046B098[];
extern char D_0046B0A0[];
extern char D_0046B0A8[];
extern char D_0046B0B0[];
extern char D_0046B0B8[];
extern char D_0046B0C0[];
extern char D_0046B0C8[];
extern char D_0046B0D0[];
extern char D_0046B0D8[];
extern char D_0046B0E0[];
extern char D_0046B0E8[];
extern char D_0046B0F0[];
extern char D_0046B0F8[];
extern char D_0046B100[];
extern char D_0046B108[];
extern char D_0046B110[];
extern char D_0046B118[];
extern char D_0046B120[];
extern char D_0046B128[];
extern char D_0046B130[];
extern char D_0046B138[];
extern char D_0046B140[];
extern char D_0046B148[];
extern char D_0046B150[];
extern char D_0046B158[];
extern char D_0046B160[];
extern char D_0046B168[];
extern char D_0046B170[];
extern char D_0046B178[];
extern char D_0046B180[];
extern char D_0046B188[];
extern char D_0046B190[];
extern char D_0046B198[];
extern char D_0046B1A0[];
extern char D_0046B1A8[];
extern char D_0046B1B0[];
extern char D_0046B1B8[];
extern char D_0046B1C0[];
extern char D_0046B1C8[];
extern char D_0046B1D0[];
extern char D_0046B1D8[];
extern char D_0046B1E0[];
extern char D_0046B1E8[];
extern char D_0046B1F0[];
extern char D_0046B1F8[];
extern char D_0046B200[];
extern char D_0046B208[];
extern char D_0046B210[];
extern char D_0046B218[];
extern char D_0046B220[];
extern char D_0046B228[];
extern char D_0046B230[];
extern char D_0046B238[];
extern char D_0046B240[];
extern char D_0046B248[];
extern char D_0046B250[];
extern char D_0046B258[];
extern char D_0046B260[];
extern char D_0046B268[];
extern char D_0046B270[];
extern char D_0046B278[];
extern char D_0046B280[];
extern char D_0046B288[];
extern char D_0046B290[];
extern char D_0046B298[];
extern char D_0046B2A0[];
extern char D_0046B2A8[];
extern char D_0046B2B0[];
extern char D_0046B2B8[];
extern char D_0046B2C0[];
extern char D_0046B2C8[];
extern char D_0046B2D0[];
extern char D_0046B2D8[];
extern char D_0046B2E0[];
extern char D_0046B2E8[];
extern char D_0046B2F0[];
extern char D_0046B2F8[];
extern char D_0046B300[];
extern char D_0046B308[];
extern char D_0046B310[];
extern char D_0046B318[];
extern char D_0046B320[];
extern char D_0046B328[];
extern char D_0046B330[];
extern char D_0046B338[];
extern char D_0046B340[];
extern char D_0046B348[];
extern char D_0046B350[];
extern char D_0046B358[];
extern char D_0046B360[];
extern char D_0046B368[];
extern char D_0046B370[];
extern char D_0046B378[];
extern char D_0046B380[];
extern char D_0046B388[];
extern char D_0046B390[];
extern char D_0046B398[];
extern char D_0046B3A0[];
extern char D_0046B3A8[];
extern char D_0046B3B0[];
extern char D_0046B3B8[];
extern char D_0046B3C0[];
extern char D_0046B3C8[];
extern char D_0046B3D0[];
extern char D_0046B3D8[];
extern char D_0046B3E0[];
extern char D_0046B3E8[];
extern char D_0046B3F0[];
extern char D_0046B3F8[];
extern char D_0046B400[];
extern char D_0046B408[];
extern char D_0046B410[];
extern char D_0046B418[];
extern char D_0046B420[];
extern char D_0046B428[];
extern char D_0046B430[];
extern char D_0046B438[];
extern char D_0046B440[];
extern char D_0046B448[];
extern char D_0046B450[];
extern char D_0046B458[];
extern char D_0046B460[];
extern char D_0046B468[];
extern char D_0046B470[];
extern char D_0046B478[];
extern char D_0046B480[];
extern char D_0046B488[];
extern char D_0046B490[];
extern char D_0046B498[];
extern char D_0046B4A0[];
extern char D_0046B4A8[];
extern char D_0046B4B0[];
extern char D_0046B4B8[];
extern char D_0046B4C0[];
extern char D_0046B4C8[];
extern char D_0046B4D0[];
extern char D_0046B4D8[];
extern char D_0046B4E0[];
extern char D_0046B4E8[];
extern char D_0046B4F0[];
extern char D_0046B4F8[];
extern char D_0046B500[];
extern char D_0046B508[];
extern char D_0046B510[];
extern char D_0046B518[];
extern char D_0046B520[];
extern char D_0046B528[];
extern char D_0046B530[];
extern char D_0046B538[];
extern char D_0046B540[];
extern char D_0046B548[];
extern char D_0046B550[];
extern char D_0046B558[];
extern char D_0046B560[];
extern char D_0046B568[];
extern char D_0046B570[];
extern char D_0046B578[];
extern char D_0046B580[];
extern char D_0046B588[];
extern char D_0046B590[];
extern char D_0046B598[];
extern char D_0046B5A0[];
extern char D_0046B5A8[];
extern char D_0046B5B0[];
extern char D_0046B5B8[];
extern char D_0046B5C0[];
extern char D_0046B5C8[];
extern char D_0046B5D0[];
extern char D_0046B5D8[];
extern char D_0046B5E0[];
extern char D_0046B5E8[];
extern char D_0046B5F0[];
extern char D_0046B5F8[];
extern char D_0046B600[];
extern char D_0046B608[];
extern char D_0046B610[];
extern char D_0046B618[];
extern char D_0046B620[];
extern char D_0046B628[];
extern char D_0046B630[];
extern char D_0046B638[];
extern char D_0046B640[];
extern char D_0046B648[];
extern char D_0046B650[];
extern char D_0046B658[];
extern char D_0046B660[];
extern char D_0046B668[];
extern char D_0046B670[];
extern char D_0046B678[];
extern char D_0046B680[];
extern char D_0046B688[];
extern char D_0046B690[];
extern char D_0046B698[];
extern char D_0046B6A0[];
extern char D_0046B6A8[];
extern char D_0046B6B0[];
extern char D_0046B6B8[];
extern char D_0046B6C0[];
extern char D_0046B6C8[];
extern char D_0046B6D0[];
extern char D_0046B6D8[];
extern char D_0046B6E0[];
extern char D_0046B6E8[];
extern char D_0046B6F0[];
extern char D_0046B6F8[];
extern char D_0046B700[];
extern char D_0046B708[];
extern char D_0046B710[];
extern char D_0046B718[];
extern char D_0046B720[];
extern char D_0046B728[];
extern char D_0046B730[];
extern char D_0046B738[];
extern char D_0046B740[];
extern char D_0046B748[];
extern char D_0046B750[];
extern char D_0046B758[];
extern char D_0046B760[];
extern char D_0046B768[];
extern char D_0046B770[];
extern char D_0046B778[];
extern char D_0046B780[];
extern char D_0046B788[];
extern char D_0046B790[];
extern char D_0046B798[];
extern char D_0046B7A0[];
extern char D_0046B7A8[];
extern char D_0046B7B0[];
extern char D_0046B7B8[];
extern char D_0046B7C0[];
extern char D_0046B7C8[];
extern char D_0046B7D0[];
extern char D_0046B7D8[];
extern char D_0046B7E0[];
extern char D_0046B7E8[];
extern char D_0046B7F0[];
extern char D_0046B7F8[];
extern char D_0046B800[];
extern char D_0046B808[];
extern char D_0046B810[];
extern char D_0046B818[];
extern char D_0046B820[];
extern char D_0046B828[];
extern char D_0046B830[];
extern char D_0046B838[];
extern char D_0046B840[];
extern char D_0046B848[];
extern char D_0046B850[];
extern char D_0046B858[];
extern char D_0046B860[];
extern char D_0046B868[];
extern char D_0046B870[];
extern char D_0046B878[];
extern char D_0046B880[];
extern char D_0046B888[];
extern char D_0046B890[];
extern char D_0046B898[];
extern char D_0046B8A0[];
extern char D_0046B8A8[];
extern char D_0046B8B0[];
extern char D_0046B8B8[];
extern char D_0046B8C0[];
extern char D_0046B8C8[];
extern char D_0046B8D0[];
extern char D_0046B8D8[];
extern char D_0046B8E0[];
extern char D_0046B8E8[];
extern char D_0046B8F0[];
extern char D_0046B8F8[];
extern char D_0046B900[];
extern char D_0046B908[];
extern char D_0046B910[];
extern char D_0046B918[];
extern char D_0046B920[];
extern char D_0046B928[];
extern char D_0046B930[];
extern char D_0046B938[];
extern char D_0046B940[];
extern char D_0046B948[];
extern char D_0046B950[];
extern char D_0046B958[];
extern char D_0046B960[];
extern char D_0046B968[];
extern char D_0046B970[];
extern char D_0046B978[];
extern char D_0046B980[];
extern char D_0046B988[];
extern char D_0046B990[];
extern char D_0046B998[];
extern char D_0046B9A0[];
extern char D_0046B9A8[];
extern char D_0046B9B0[];
extern char D_0046B9B8[];
extern char D_0046B9C0[];
extern char D_0046B9C8[];
extern char D_0046B9D0[];
extern char D_0046B9D8[];
extern char D_0046B9E0[];
extern char D_0046B9E8[];
extern char D_0046B9F0[];
extern char D_0046B9F8[];
extern char D_0046BA00[];
extern char D_0046BA08[];
extern char D_0046BA10[];
extern char D_0046BA18[];
extern char D_0046BA20[];
extern char D_0046BA28[];
extern char D_0046BA30[];
extern char D_0046BA38[];
extern char D_0046BA40[];
extern char D_0046BA48[];
extern char D_0046BA50[];
extern char D_0046BA58[];
extern char D_0046BA60[];
extern char D_0046BA68[];
extern char D_0046BA70[];
extern char D_0046BA78[];
extern char D_0046BA80[];
extern char D_0046BA88[];
extern char D_0046BA90[];
extern char D_0046BA98[];
extern char D_0046BAA0[];
extern char D_0046BAA8[];
extern char D_0046BAB0[];
extern char D_0046BAB8[];
extern char D_0046BAC0[];
extern char D_0046BAC8[];
extern char D_0046BAD0[];
extern char D_0046BAD8[];
extern char D_0046BAE0[];
extern char D_0046BAE8[];
extern char D_0046BAF0[];
extern char D_0046BAF8[];
extern char D_0046BB00[];
extern char D_0046BB08[];
extern char D_0046BB10[];
extern char D_0046BB18[];
extern char D_0046BB20[];
extern char D_0046BB28[];
extern char D_0046BB30[];
extern char D_0046BB38[];
extern char D_0046BB40[];
extern char D_0046BB48[];
extern char D_0046BB50[];
extern char D_0046BB58[];
extern char D_0046BB60[];
extern char D_0046BB68[];
extern char D_0046BB70[];
extern char D_0046BB78[];
extern char D_0046BB80[];
extern char D_0046BB88[];
extern char D_0046BB90[];
extern char D_0046BB98[];
extern char D_0046BBA0[];
extern char D_0046BBA8[];
extern char D_0046BBB0[];
extern char D_0046BBB8[];
extern char D_0046BBC0[];
extern char D_0046BBC8[];
extern char D_0046BBD0[];
extern char D_0046BBD8[];
extern char D_0046BBE0[];
extern char D_0046BBE8[];
extern char D_0046BBF0[];
extern char D_0046BBF8[];
extern char D_0046BC00[];
extern char D_0046BC08[];
extern char D_0046BC10[];
extern char D_0046BC18[];
extern char D_0046BC20[];
extern char D_0046BC28[];
extern char D_0046BC30[];
extern char D_0046BC38[];
extern char D_0046BC40[];
extern char D_0046BC48[];
extern char D_0046BC50[];
extern char D_0046BC58[];
extern char D_0046BC60[];
extern char D_0046BC68[];
extern char D_0046BC70[];
extern char D_0046BC78[];
extern char D_0046BC80[];
extern char D_0046BC88[];
extern char D_0046BC90[];
extern char D_0046BC98[];
extern char D_0046BCA0[];
extern char D_0046BCA8[];
extern char D_0046BCB0[];
extern char D_0046BCB8[];
extern char D_0046BCC0[];
extern char D_0046BCC8[];
extern char D_0046BCD0[];
extern char D_0046BCD8[];
extern char D_0046BCE0[];
extern char D_0046BCE8[];
extern char D_0046BCF0[];
extern char D_0046BCF8[];
extern char D_0046BD00[];
extern char D_0046BD08[];
extern char D_0046BD10[];
extern char D_0046BD18[];
extern char D_0046BD20[];
extern char D_0046BD28[];
extern char D_0046BD30[];
extern char D_0046BD38[];
extern char D_0046BD40[];
extern char D_0046BD48[];
extern char D_0046BD50[];
extern char D_0046BD58[];
extern char D_0046BD60[];
extern char D_0046BD68[];
extern char D_0046BD70[];
extern char D_0046BD78[];
extern char D_0046BD80[];
extern char D_0046BD88[];
extern char D_0046BD90[];
extern char D_0046BD98[];
extern char D_0046BDA0[];
extern char D_0046BDA8[];
extern char D_0046BDB0[];
extern char D_0046BDB8[];
extern char D_0046BDC0[];
extern char D_0046BDC8[];
extern char D_0046BDD0[];
extern char D_0046BDD8[];
extern char D_0046BDE0[];
extern char D_0046BDE8[];
extern char D_0046BDF0[];
extern char D_0046BDF8[];
extern char D_0046BE00[];
extern char D_0046BE08[];
extern char D_0046BE10[];
extern char D_0046BE18[];
extern char D_0046BE20[];
extern char D_0046BE28[];
extern char D_0046BE30[];
extern char D_0046BE38[];
extern char D_0046BE40[];
extern char D_0046BE48[];
extern char D_0046BE50[];
extern char D_0046BE58[];
extern char D_0046BE60[];
extern char D_0046BE68[];
extern char D_0046BE70[];
extern char D_0046BE78[];
extern char D_0046BE80[];
extern char D_0046BE88[];
extern char D_0046BE90[];
extern char D_0046BE98[];
extern char D_0046BEA0[];
extern char D_0046BEA8[];
extern char D_0046BEB0[];
extern char D_0046BEB8[];
extern char D_0046BEC0[];
extern char D_0046BEC8[];
extern char D_0046BED0[];
extern char D_0046BED8[];
extern char D_0046BEE0[];
extern char D_0046BEE8[];
extern char D_0046BEF0[];
extern char D_0046BEF8[];
extern char D_0046BF00[];
extern char D_0046BF08[];
extern char D_0046BF10[];
extern char D_0046BF18[];
extern char D_0046BF20[];
extern char D_0046BF28[];
extern char D_0046BF30[];
extern char D_0046BF38[];
extern char D_0046BF40[];
extern char D_0046BF48[];
extern char D_0046BF50[];
extern char D_0046BF58[];
extern char D_0046BF60[];
extern char D_0046BF68[];
extern char D_0046BF70[];
extern char D_0046BF78[];
extern char D_0046BF80[];
extern char D_0046BF88[];
extern char D_0046BF90[];
extern char D_0046BF98[];
extern char D_0046BFA0[];
extern char D_0046BFA8[];
extern char D_0046BFB0[];
extern char D_0046BFB8[];
extern char D_0046BFC0[];
extern char D_0046BFC8[];
extern char D_0046BFD0[];
extern char D_0046BFD8[];
extern char D_0046BFE0[];
extern char D_0046BFE8[];
extern char D_0046BFF0[];
extern char D_0046BFF8[];
extern char D_0046C000[];
extern char D_0046C008[];
extern char D_0046C010[];
extern char D_0046C018[];
extern char D_0046C020[];
extern char D_0046C028[];
extern char D_0046C030[];
extern char D_0046C038[];
extern char D_0046C040[];
extern char D_0046C048[];
extern char D_0046C050[];
extern char D_0046C058[];
extern char D_0046C060[];
extern char D_0046C068[];
extern char D_0046C070[];
extern char D_0046C078[];
extern char D_0046C080[];
extern char D_0046C088[];
extern char D_0046C090[];
extern char D_0046C098[];
extern char D_0046C0A0[];
extern char D_0046C0A8[];
extern char D_0046C0B0[];
extern char D_0046C0B8[];
extern char D_0046C0C0[];
extern char D_0046C0C8[];
extern char D_0046C0D0[];
extern char D_0046C0D8[];
extern char D_0046C0E0[];
extern char D_0046C0E8[];
extern char D_0046C0F0[];
extern char D_0046C0F8[];
extern char D_0046C100[];
extern char D_0046C108[];
extern char D_0046C110[];
extern char D_0046C118[];
extern char D_0046C120[];
extern char D_0046C128[];
extern char D_0046C130[];
extern char D_0046C138[];
extern char D_0046C140[];
extern char D_0046C148[];
extern char D_0046C150[];
extern char D_0046C158[];
extern char D_0046C160[];
extern char D_0046C168[];
extern char D_0046C170[];
extern char D_0046C178[];
extern char D_0046C180[];
extern char D_0046C188[];
extern char D_0046C190[];
extern char D_0046C198[];
extern char D_0046C1A0[];
extern char D_0046C1A8[];
extern char D_0046C1B0[];
extern char D_0046C1B8[];
extern char D_0046C1C0[];
extern char D_0046C1C8[];
extern char D_0046C1D0[];
extern char D_0046C1E0[];
extern char D_0046C260[];
extern char D_0046C2E0[];
extern char D_0046C360[];
extern char D_0046C3E0[];
extern char D_0046C460[];
extern char D_0046C4E0[];
extern char D_0046C560[];
extern char D_0046C5E0[];
extern char D_0046C660[];
extern char D_0046C6E0[];
extern char D_0046C710[];
extern char D_0046C750[];
extern char D_0046C790[];
extern char D_0046C7D0[];
extern char D_0046C820[];
extern char D_0046C860[];
extern char D_0046C8A0[];
extern char D_0046C8E0[];
extern char D_0046C990[];
extern char D_0046C998[];
extern char D_0046C9A0[];
extern char D_0046C9A8[];
extern char D_0046C9B0[];
extern char D_0046C9B8[];
extern char D_0046C9C0[];
extern char D_0046C9C8[];
extern char D_0046C9D0[];
extern char D_0046C9D8[];
extern char D_0046C9E0[];
extern char D_0046C9E8[];
extern char D_0046C9F0[];
extern char D_0046C9F8[];
extern char D_0046CA00[];
extern char D_0046CA08[];
extern char D_0046CA10[];
extern char D_0046CA18[];
extern char D_0046CA20[];
extern char D_0046CA28[];
extern char D_0046CA30[];
extern char D_0046CA38[];
extern char D_0046CA40[];
extern char D_0046CA48[];
extern char D_0046CA50[];
extern char D_0046CA58[];
extern char D_0046CA60[];
extern char D_0046CA68[];
extern char D_0046CA70[];
extern char D_0046CA78[];
extern char D_0046CA80[];
extern char D_0046CA88[];
extern char D_0046CA90[];
extern char D_0046CA98[];
extern char D_0046CAA0[];
extern char D_0046CAA8[];
extern char D_0046CAB0[];
extern char D_0046CAB8[];
extern u32 D_0046CAE0[];
extern u32 D_0046CBB0[];
RomajiEntry gRomajiKanaTbl[240] = {
    {D_0046A930, {D_0046A938, D_0046A940, D_0046A948}},
    {D_0046A950, {D_0046A958, D_0046A960, D_0046A968}},
    {D_0046A970, {D_0046A978, D_0046A980, D_0046A988}},
    {D_0046A990, {D_0046A998, D_0046A9A0, D_0046A9A8}},
    {D_0046A9B0, {D_0046A9B8, D_0046A9C0, D_0046A9C8}},
    {D_0046A9D0, {D_0046A958, D_0046A960, D_0046A968}},
    {D_0046A9D8, {D_0046A978, D_0046A980, D_0046A988}},
    {D_0046A9E0, {D_0046A9E8, D_0046A9F0, D_0046A9F8}},
    {D_0046AA00, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {D_0046AA20, {D_0046AA28, D_0046AA30, D_0046AA38}},
    {D_0046AA40, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {D_0046AA60, {D_0046AA68, D_0046AA70, D_0046AA78}},
    {D_0046AA80, {D_0046A9E8, D_0046A9F0, D_0046A9F8}},
    {D_0046AA88, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {D_0046AA90, {D_0046AA28, D_0046AA30, D_0046AA38}},
    {D_0046AA98, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {D_0046AAA0, {D_0046AA68, D_0046AA70, D_0046AA78}},
    {D_0046AAA8, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {D_0046AAB0, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {D_0046AAB8, {D_0046AA08, D_0046AA10, D_0046AA18}},
    {D_0046AAC0, {D_0046AA48, D_0046AA50, D_0046AA58}},
    {D_0046AAC8, {D_0046AAD0, D_0046AAD8, D_0046AAE0}},
    {D_0046AAE8, {D_0046AAF0, D_0046AAF8, D_0046AB00}},
    {D_0046AB08, {D_0046AB10, D_0046AB18, D_0046AB20}},
    {D_0046AB28, {D_0046AB30, D_0046AB38, D_0046AB40}},
    {D_0046AB48, {D_0046AB50, D_0046AB58, D_0046AB60}},
    {D_0046AB68, {D_0046AB70, D_0046AB78, D_0046AB80}},
    {D_0046AB88, {D_0046AAF0, D_0046AAF8, D_0046AB00}},
    {D_0046AB90, {D_0046AB30, D_0046AB38, D_0046AB40}},
    {D_0046AB98, {D_0046AB70, D_0046AB78, D_0046AB80}},
    {D_0046ABA0, {D_0046ABA8, D_0046ABB0, D_0046ABB8}},
    {D_0046ABC0, {D_0046ABC8, D_0046ABD0, D_0046ABD8}},
    {D_0046ABE0, {D_0046ABE8, D_0046ABF0, D_0046ABF8}},
    {D_0046AC00, {D_0046AC08, D_0046AC10, D_0046AC18}},
    {D_0046AC20, {D_0046AC28, D_0046AC30, D_0046AC38}},
    {D_0046AC40, {D_0046AC48, D_0046AC50, D_0046AC58}},
    {D_0046AC60, {D_0046AC68, D_0046AC70, D_0046AC78}},
    {D_0046AC80, {D_0046AC88, D_0046AC90, D_0046AC98}},
    {D_0046ACA0, {D_0046ACA8, D_0046ACB0, D_0046ACB8}},
    {D_0046ACC0, {D_0046ACC8, D_0046ACD0, D_0046ACD8}},
    {D_0046ACE0, {D_0046ACE8, D_0046ACF0, D_0046ACF8}},
    {D_0046AD00, {D_0046AD08, D_0046AD10, D_0046AD18}},
    {D_0046AD20, {D_0046AD28, D_0046AD30, D_0046AD38}},
    {D_0046AD40, {D_0046AD48, D_0046AD50, D_0046AD58}},
    {D_0046AD60, {D_0046AD68, D_0046AD70, D_0046AD78}},
    {D_0046AD80, {D_0046AB30, D_0046AB38, D_0046AB40}},
    {D_0046AD88, {D_0046AD90, D_0046AD98, D_0046ADA0}},
    {D_0046ADA8, {D_0046ADB0, D_0046ADB8, D_0046ADC0}},
    {D_0046ADC8, {D_0046AD68, D_0046AD70, D_0046AD78}},
    {D_0046ADD0, {D_0046AD90, D_0046AD98, D_0046ADA0}},
    {D_0046ADD8, {D_0046ADE0, D_0046ADE8, D_0046ADF0}},
    {D_0046ADF8, {D_0046AE00, D_0046AE08, D_0046AE10}},
    {D_0046AE18, {D_0046AE20, D_0046AE28, D_0046AE30}},
    {D_0046AE38, {D_0046AE40, D_0046AE48, D_0046AE50}},
    {D_0046AE58, {D_0046AE60, D_0046AE68, D_0046AE70}},
    {D_0046AE78, {D_0046AE80, D_0046AE88, D_0046AE90}},
    {D_0046AE98, {D_0046AEA0, D_0046AEA8, D_0046AEB0}},
    {D_0046AEB8, {D_0046AEC0, D_0046AEC8, D_0046AED0}},
    {D_0046AED8, {D_0046AEE0, D_0046AEE8, D_0046AEF0}},
    {D_0046AEF8, {D_0046AF00, D_0046AF08, D_0046AF10}},
    {D_0046AF18, {D_0046AF20, D_0046AF28, D_0046AF30}},
    {D_0046AF38, {D_0046AF40, D_0046AF48, D_0046AF50}},
    {D_0046AF58, {D_0046AF60, D_0046AF68, D_0046AF70}},
    {D_0046AF78, {D_0046AF80, D_0046AF88, D_0046AF90}},
    {D_0046AF98, {D_0046AF20, D_0046AF28, D_0046AF30}},
    {D_0046AFA0, {D_0046AF60, D_0046AF68, D_0046AF70}},
    {D_0046AFA8, {D_0046AFB0, D_0046AFB8, D_0046AFC0}},
    {D_0046AFC8, {D_0046AFD0, D_0046AFD8, D_0046AFE0}},
    {D_0046AFE8, {D_0046AFF0, D_0046AFF8, D_0046B000}},
    {D_0046B008, {D_0046B010, D_0046B018, D_0046B020}},
    {D_0046B028, {D_0046B030, D_0046B038, D_0046B040}},
    {D_0046B048, {D_0046AFD0, D_0046AFD8, D_0046AFE0}},
    {D_0046B050, {D_0046AF20, D_0046AF28, D_0046AF30}},
    {D_0046B058, {D_0046B060, D_0046B068, D_0046B070}},
    {D_0046B078, {D_0046B080, D_0046B088, D_0046B090}},
    {D_0046B098, {D_0046B0A0, D_0046B0A8, D_0046B0B0}},
    {D_0046B0B8, {D_0046B0C0, D_0046B0C8, D_0046B0D0}},
    {D_0046B0D8, {D_0046B0E0, D_0046B0E8, D_0046B0F0}},
    {D_0046B0F8, {D_0046B060, D_0046B068, D_0046B070}},
    {D_0046B100, {D_0046B0A0, D_0046B0A8, D_0046B0B0}},
    {D_0046B108, {D_0046B0C0, D_0046B0C8, D_0046B0D0}},
    {D_0046B110, {D_0046B0E0, D_0046B0E8, D_0046B0F0}},
    {D_0046B118, {D_0046B120, D_0046B128, D_0046B130}},
    {D_0046B138, {D_0046B140, D_0046B148, D_0046B150}},
    {D_0046B158, {D_0046B160, D_0046B168, D_0046B170}},
    {D_0046B178, {D_0046B180, D_0046B188, D_0046B190}},
    {D_0046B198, {D_0046B1A0, D_0046B1A8, D_0046B1B0}},
    {D_0046B1B8, {D_0046B120, D_0046B128, D_0046B130}},
    {D_0046B1C0, {D_0046B160, D_0046B168, D_0046B170}},
    {D_0046B1C8, {D_0046B180, D_0046B188, D_0046B190}},
    {D_0046B1D0, {D_0046B1A0, D_0046B1A8, D_0046B1B0}},
    {D_0046B1D8, {D_0046B120, D_0046B128, D_0046B130}},
    {D_0046B1E0, {D_0046B140, D_0046B148, D_0046B150}},
    {D_0046B1E8, {D_0046B160, D_0046B168, D_0046B170}},
    {D_0046B1F0, {D_0046B180, D_0046B188, D_0046B190}},
    {D_0046B1F8, {D_0046B1A0, D_0046B1A8, D_0046B1B0}},
    {D_0046B200, {D_0046B208, D_0046B210, D_0046B218}},
    {D_0046B220, {D_0046B228, D_0046B230, D_0046B238}},
    {D_0046B240, {D_0046B248, D_0046B250, D_0046B258}},
    {D_0046B260, {D_0046B268, D_0046B270, D_0046B278}},
    {D_0046B280, {D_0046B288, D_0046B290, D_0046B298}},
    {D_0046B2A0, {D_0046B228, D_0046B230, D_0046B238}},
    {D_0046B2A8, {D_0046B248, D_0046B250, D_0046B258}},
    {D_0046B2B0, {D_0046B2B8, D_0046B2C0, D_0046B2C8}},
    {D_0046B2D0, {D_0046B2B8, D_0046B2C0, D_0046B2C8}},
    {D_0046B2D8, {D_0046B2E0, D_0046B2E8, D_0046B2F0}},
    {D_0046B2F8, {D_0046B300, D_0046B308, D_0046B310}},
    {D_0046B318, {D_0046B320, D_0046B328, D_0046B330}},
    {D_0046B338, {D_0046B340, D_0046B348, D_0046B350}},
    {D_0046B358, {D_0046B360, D_0046B368, D_0046B370}},
    {D_0046B378, {D_0046B380, D_0046B388, D_0046B390}},
    {D_0046B398, {D_0046B3A0, D_0046B3A8, D_0046B3B0}},
    {D_0046B3B8, {D_0046B3C0, D_0046B3C8, D_0046B3D0}},
    {D_0046B3D8, {D_0046B3E0, D_0046B3E8, D_0046B3F0}},
    {D_0046B3F8, {D_0046B400, D_0046B408, D_0046B410}},
    {D_0046B418, {D_0046B380, D_0046B388, D_0046B390}},
    {D_0046B420, {D_0046B3C0, D_0046B3C8, D_0046B3D0}},
    {D_0046B428, {D_0046B3E0, D_0046B3E8, D_0046B3F0}},
    {D_0046B430, {D_0046B400, D_0046B408, D_0046B410}},
    {D_0046B438, {D_0046B380, D_0046B388, D_0046B390}},
    {D_0046B440, {D_0046B3A0, D_0046B3A8, D_0046B3B0}},
    {D_0046B448, {D_0046B3C0, D_0046B3C8, D_0046B3D0}},
    {D_0046B450, {D_0046B3E0, D_0046B3E8, D_0046B3F0}},
    {D_0046B458, {D_0046B400, D_0046B408, D_0046B410}},
    {D_0046B460, {D_0046B468, D_0046B470, D_0046B478}},
    {D_0046B480, {D_0046B488, D_0046B490, D_0046B498}},
    {D_0046B4A0, {D_0046B4A8, D_0046B4B0, D_0046B4B8}},
    {D_0046B4C0, {D_0046B4C8, D_0046B4D0, D_0046B4D8}},
    {D_0046B4E0, {D_0046B4E8, D_0046B4F0, D_0046B4F8}},
    {D_0046B500, {D_0046B508, D_0046B510, D_0046B518}},
    {D_0046B520, {D_0046B528, D_0046B530, D_0046B538}},
    {D_0046B540, {D_0046B548, D_0046B550, D_0046B558}},
    {D_0046B560, {D_0046B568, D_0046B570, D_0046B578}},
    {D_0046B580, {D_0046B588, D_0046B590, D_0046B598}},
    {D_0046B5A0, {D_0046B5A8, D_0046B5B0, D_0046B5B8}},
    {D_0046B5C0, {D_0046B5C8, D_0046B5D0, D_0046B5D8}},
    {D_0046B5E0, {D_0046B5E8, D_0046B5F0, D_0046B5F8}},
    {D_0046B600, {D_0046B608, D_0046B610, D_0046B618}},
    {D_0046B620, {D_0046B628, D_0046B630, D_0046B638}},
    {D_0046B640, {D_0046B648, D_0046B650, D_0046B658}},
    {D_0046B660, {D_0046B668, D_0046B670, D_0046B678}},
    {D_0046B680, {D_0046B688, D_0046B690, D_0046B698}},
    {D_0046B6A0, {D_0046B6A8, D_0046B6B0, D_0046B6B8}},
    {D_0046B6C0, {D_0046B6C8, D_0046B6D0, D_0046B6D8}},
    {D_0046B6E0, {D_0046B6E8, D_0046B6F0, D_0046B6F8}},
    {D_0046B700, {D_0046B708, D_0046B710, D_0046B718}},
    {D_0046B720, {D_0046B728, D_0046B730, D_0046B738}},
    {D_0046B740, {D_0046B748, D_0046B750, D_0046B758}},
    {D_0046B760, {D_0046B768, D_0046B770, D_0046B778}},
    {D_0046B780, {D_0046B788, D_0046B790, D_0046B798}},
    {D_0046B7A0, {D_0046B7A8, D_0046B7B0, D_0046B7B8}},
    {D_0046B7C0, {D_0046B7C8, D_0046B7D0, D_0046B7D8}},
    {D_0046B7E0, {D_0046B7E8, D_0046B7F0, D_0046B7F8}},
    {D_0046B800, {D_0046B808, D_0046B810, D_0046B818}},
    {D_0046B820, {D_0046B828, D_0046B830, D_0046B838}},
    {D_0046B840, {D_0046B848, D_0046B850, D_0046B858}},
    {D_0046B860, {D_0046B868, D_0046B870, D_0046B878}},
    {D_0046B880, {D_0046B888, D_0046B890, D_0046B898}},
    {D_0046B8A0, {D_0046B848, D_0046B850, D_0046B858}},
    {D_0046B8A8, {D_0046B8B0, D_0046B8B8, D_0046B8C0}},
    {D_0046B8C8, {D_0046B8D0, D_0046B8D8, D_0046B8E0}},
    {D_0046B8E8, {D_0046B8F0, D_0046B8F8, D_0046B900}},
    {D_0046B908, {D_0046B910, D_0046B918, D_0046B920}},
    {D_0046B928, {D_0046B930, D_0046B938, D_0046B940}},
    {D_0046B948, {D_0046B950, D_0046B958, D_0046B960}},
    {D_0046B968, {D_0046B970, D_0046B978, D_0046B980}},
    {D_0046B988, {D_0046B990, D_0046B998, D_0046B9A0}},
    {D_0046B9A8, {D_0046B9B0, D_0046B9B8, D_0046B9C0}},
    {D_0046B9C8, {D_0046B9D0, D_0046B9D8, D_0046B9E0}},
    {D_0046B9E8, {D_0046B9F0, D_0046B9F8, D_0046BA00}},
    {D_0046BA08, {D_0046BA10, D_0046BA18, D_0046BA20}},
    {D_0046BA28, {D_0046BA30, D_0046BA38, D_0046BA40}},
    {D_0046BA48, {D_0046BA50, D_0046BA58, D_0046BA60}},
    {D_0046BA68, {D_0046BA70, D_0046BA78, D_0046BA80}},
    {D_0046BA88, {D_0046BA90, D_0046BA98, D_0046BAA0}},
    {D_0046BAA8, {D_0046BAB0, D_0046BAB8, D_0046BAC0}},
    {D_0046BAC8, {D_0046BAD0, D_0046BAD8, D_0046BAE0}},
    {D_0046BAE8, {D_0046BAF0, D_0046BAF8, D_0046BB00}},
    {D_0046BB08, {D_0046BB10, D_0046BB18, D_0046BB20}},
    {D_0046BB28, {D_0046BB30, D_0046BB38, D_0046BB40}},
    {D_0046BB48, {D_0046BB50, D_0046BB58, D_0046BB60}},
    {D_0046BB68, {D_0046BB10, D_0046BB18, D_0046BB20}},
    {D_0046BB70, {D_0046BB30, D_0046BB38, D_0046BB40}},
    {D_0046BB78, {D_0046BB80, D_0046BB88, D_0046BB90}},
    {D_0046BB98, {D_0046BBA0, D_0046BBA8, D_0046BBB0}},
    {D_0046BBB8, {D_0046BBC0, D_0046BBC8, D_0046BBD0}},
    {D_0046BBD8, {D_0046BBE0, D_0046BBE8, D_0046BBF0}},
    {D_0046BBF8, {D_0046BC00, D_0046BC08, D_0046BC10}},
    {D_0046BC18, {D_0046BC20, D_0046BC28, D_0046BC30}},
    {D_0046BC38, {D_0046BC40, D_0046BC48, D_0046BC50}},
    {D_0046BC58, {D_0046BC60, D_0046BC60, D_0046BC68}},
    {D_0046BC70, {D_0046BC78, D_0046BC80, D_0046BC88}},
    {D_0046BC90, {D_0046BC98, D_0046BCA0, D_0046BCA8}},
    {D_0046BCB0, {D_0046BCB8, D_0046BCC0, D_0046BCC8}},
    {D_0046BCD0, {D_0046BC40, D_0046BC48, D_0046BC50}},
    {D_0046BCD8, {D_0046BCE0, D_0046BCE8, D_0046BCF0}},
    {D_0046BCF8, {D_0046BC78, D_0046BC80, D_0046BC88}},
    {D_0046BD00, {D_0046BC98, D_0046BCA0, D_0046BCA8}},
    {D_0046BD08, {D_0046BD10, D_0046BD18, D_0046BD20}},
    {D_0046BD28, {D_0046BD30, D_0046BD38, D_0046BD40}},
    {D_0046BD48, {D_0046BD50, D_0046BD58, D_0046BD60}},
    {D_0046BD68, {D_0046BD70, D_0046BD78, D_0046BD80}},
    {D_0046BD88, {D_0046BD90, D_0046BD98, D_0046BDA0}},
    {D_0046BDA8, {D_0046BDB0, D_0046BDB8, D_0046BDC0}},
    {D_0046BDC8, {D_0046BDD0, D_0046BDD8, D_0046BDE0}},
    {D_0046BDE8, {D_0046BDF0, D_0046BDF8, D_0046BE00}},
    {D_0046BE08, {D_0046BE10, D_0046BE18, D_0046BE20}},
    {D_0046BE28, {D_0046BE30, D_0046BE38, D_0046BE40}},
    {D_0046BE48, {D_0046BE50, D_0046BE58, D_0046BE60}},
    {D_0046BE68, {D_0046BE70, D_0046BE78, D_0046BE80}},
    {D_0046BE88, {D_0046BE90, D_0046BE98, D_0046BEA0}},
    {D_0046BEA8, {D_0046BEB0, D_0046BEB8, D_0046BEC0}},
    {D_0046BEC8, {D_0046BED0, D_0046BED8, D_0046BEE0}},
    {D_0046BEE8, {D_0046BEF0, D_0046BEF8, D_0046BF00}},
    {D_0046BF08, {D_0046BF10, D_0046BF18, D_0046BF20}},
    {D_0046BF28, {D_0046BF30, D_0046BF38, D_0046BF40}},
    {D_0046BF48, {D_0046BF50, D_0046BF58, D_0046BF60}},
    {D_0046BF68, {D_0046BF70, D_0046BF78, D_0046BF80}},
    {D_0046BF88, {D_0046BF90, D_0046BF98, D_0046BFA0}},
    {D_0046BFA8, {D_0046BF50, D_0046BF58, D_0046BF60}},
    {D_0046BFB0, {D_0046BF70, D_0046BF78, D_0046BF80}},
    {D_0046BFB8, {D_0046BF90, D_0046BF98, D_0046BFA0}},
    {D_0046BFC0, {D_0046BFC8, D_0046BFD0, D_0046BFD8}},
    {D_0046BFE0, {D_0046BFE8, D_0046BFF0, D_0046BFF8}},
    {D_0046C000, {D_0046C008, D_0046C010, D_0046C018}},
    {D_0046C020, {D_0046C028, D_0046C030, D_0046C038}},
    {D_0046C040, {D_0046C048, D_0046C050, D_0046C058}},
    {D_0046C060, {D_0046C068, D_0046C070, D_0046C078}},
    {D_0046C080, {D_0046C088, D_0046C090, D_0046C098}},
    {D_0046C0A0, {D_0046C0A8, D_0046C0B0, D_0046C0B8}},
    {D_0046C0C0, {D_0046C0C8, D_0046C0D0, D_0046C0D8}},
    {D_0046C0E0, {D_0046C0E8, D_0046C0F0, D_0046C0F8}},
    {D_0046C100, {D_0046C108, D_0046C110, D_0046C118}},
    {D_0046C120, {D_0046C128, D_0046C130, D_0046C138}},
    {D_0046C140, {D_0046C128, D_0046C130, D_0046C148}},
    {D_0046C150, {D_0046C158, D_0046C160, D_0046C168}},
    {D_0046C170, {D_0046C178, D_0046C180, D_0046C188}},
    {D_0046C190, {D_0046C198, D_0046C1A0, D_0046C1A8}},
    {D_0046C1B0, {D_0046C1B8, D_0046C1C0, D_0046C1C8}},
    {D_0046C1D0, {D_0046C1B8, D_0046C1C0, D_0046C1C8}},
};
char *gKbdHidWideStrs[10] = {
    D_0046C1E0, D_0046C260, D_0046C2E0, D_0046C360,
    D_0046C3E0, D_0046C460, D_0046C4E0, D_0046C560,
    D_0046C5E0, D_0046C660,
};
char *D_00416408[2] = {
    D_0046C6E0, NULL,
};
char *gKbdHidChars[6] = {
    D_0046C710, D_0046C750, D_0046C790, D_0046C7D0,
    D_0046C820, D_0046C860,
};
char *D_00416428[2] = {
    D_0046C8A0, NULL,
};
char *D_00416430[4] = {
    D_0046C8E0, NULL, NULL, NULL,
};
u32 D_00416440[8] = {
#include "sub_main/00416440.inc"
};
u32 D_00416460[134] = {
#include "sub_main/00416460.inc"
};
char *gKanaNChars[3] = {
    D_0046C990, D_0046C998, D_0046C9A0,
};
char *gKbdSymbolCands[36] = {
    D_0046C9A8, D_0046C9B0, D_0046C9B8, D_0046C9C0,
    D_0046C9C8, D_0046C9D0, D_0046C9D8, D_0046C9E0,
    D_0046C9E8, D_0046C9F0, D_0046C9F8, D_0046CA00,
    D_0046CA08, D_0046CA10, D_0046CA18, D_0046CA20,
    D_0046CA28, D_0046CA30, D_0046CA38, D_0046CA40,
    D_0046CA48, D_0046CA50, D_0046CA58, D_0046CA60,
    D_0046CA68, D_0046CA70, D_0046CA78, D_0046CA80,
    D_0046CA88, D_0046CA90, D_0046CA98, D_0046CAA0,
    D_0046CAA8, D_0046CAB0, D_0046CAB8, NULL,
};
Tbl17 gKbdSetIsWide = {
#include "sub_main/00416720.inc"
};
u8 gKbdKindConfs[5][56] = {
#include "sub_main/00416740.inc"
};
u32 D_00416860[48] = {
#include "sub_main/00416860.inc"
};
PhraseEntry gPhraseEntries[PHRASE_COUNT] = {
#include "sub_main/00416920.inc"
};
u8 gKbdKeySets[16][2] = {
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
u32 D_00416B40[148] = {
#include "sub_main/00416B40.inc"
};
u32 D_00416D90[52] = {
#include "sub_main/00416D90.inc"
};
u32 D_00416E60[24] = {
#include "sub_main/00416E60.inc"
};
u32 D_00416EC0[24] = {
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
void *gKanaDakutenTable = (void *)D_0046CAE0;
void *gKanaHandakutenTable = (void *)D_0046CBB0;
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
