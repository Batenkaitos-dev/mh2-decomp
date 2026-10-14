/*
 * The software keyboard, continued: the unit that holds Kbd_EnterChar (0x3166D0) to the end
 * (Kbd_EnterChar and Kbd_CharIsSpecialKana share one full-width space; its "%s" is another copy than
 * Kbd_DrawCandidateBox's). Its .rodata starts with the kana tables of gKanaDakutenTable.
 */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "kbd.h"
#include "scene.h"
#include "syssave.h"
#include "player.h"
#include "camera.h"
#pragma optimization_level 4

#include "ee_stdarg.h"
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
void func_003634B0(void);
void Player_RequestAct(u8 a);
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
s32 Sjis_PrevCharSize(u8 *s, s32 pos)
{
    s32 i;
    s32 size = 0;
    s32 len = 0;

    while (*s != 0 && len < pos) {
        size = ((*s >= 0x80 && *s < 0xA0) || (*s >= 0xE0 && *s < 0x100)) ? 2 : 1;
        len += size;
        for (i = 0; i < size; i++) {
            s++;
        }
    }
    return size;
}

/* The size of the character at byte pos of s (0 for an empty s, -1 when pos is inside a character). */
s32 Sjis_CharSizeAt(char *s, s32 pos)
{
    u8 *p;
    s32 i;
    u8 c;
    s32 size;

    if (strlen(s) == 0) {
        return 0;
    }
    i = 0;
    p = (u8 *)s;
    while (*p != 0 && i < pos) {
        size = ((*p >= 0x80 && *p < 0xA0) || (*p >= 0xE0 && *p < 0x100)) ? 2 : 1;
        i += size;
        p += size;
    }
    if (pos < i) {
        return -1;
    }
    c = s[pos];
    return ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c < 0x100)) ? 2 : 1;
}

void Kbd_OpenConv(void)
{
    ConvCmd_Open();
}

void Kbd_KanaDakuten(void)
{
    u8 keySet;

    if (gKbd->converting == 0) {
        keySet = gKbd->keySet;
        if ((keySet > 1U) && (keySet != 6)) {
            if (gKbd->mode == 0xD) {
                if (!(gKbd->gestureMode & 0xF)) {
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

void Kbd_KanaHandakuten(void)
{
    u8 keySet;

    if (gKbd->converting == 0) {
        keySet = gKbd->keySet;
        if ((keySet > 1U) && (keySet != 6)) {
            if (gKbd->mode == 0xD) {
                if (!(gKbd->gestureMode & 0xF)) {
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

s32 Kbd_SetIsWide(void)
{
    Tbl17 t = gKbdSetIsWide;

    return t.v[gKbd->keySet];
}

s32 func_003151C0(char *text, u16 *pos)
{
    return 0;
}

s32 Kbd_DoGesture(char *text, u16 *pos, s32 gesture)
{
    Player_RequestAct(((gesture & 0xFF) + 1) & 0xFF);
    Hud_SetCancel();
    return 0;
}

extern s32 gKbdSettings;
extern u8 gKbdSavedKind;
extern u8 gKbdSavedKeySet;

void Kbd_SaveKeySet(void)
{
    gKbdSavedKind = gKbd->kind;
    if (gKbdSettings & 2) {
        switch (gKbd->keySet) {
        case 2:
        case 7:
        case 0xF:
        case 0xA:
            if ((s8)Kbd_ShiftPair(gKbd->keySet) >= 0) {
                gKbdSavedKeySet = Kbd_ShiftPair(gKbd->keySet);
            } else {
                gKbdSavedKeySet = gKbd->keySet;
            }
            break;
        case 4:
        case 5:
        case 0xD:
            gKbdSavedKeySet = 0;
            break;
        default:
            gKbdSavedKeySet = gKbd->keySet;
            break;
        }
    } else {
        switch (gKbd->keySet) {
        case 4:
        case 5:
        case 0xD:
            gKbdSavedKeySet = 0;
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

/* Looks up the key under the cursor (col: 1..20, row: 0..3) in the layout and points keyPos,
 * key and keyInfo at its entries; returns the key number. */
s8 Kbd_LookupKey(void)
{
    u8 *p;
    s32 idx;

    /* the (long) cast (found by the permuter) gives the original registers: row * 4 + column in v1 */
    idx = (long)(gKbd->col * 4);
    idx += gKbd->row;
    gKbd->keyIdx = gKbd->layout->grid[idx];
    gKbd->keyPos = gKbd->layout->cursors + gKbd->keyIdx;
    gKbd->key = gKbd->layout->keys + gKbd->keyIdx;
    p = gKbd->layout->extra;
    if (p != NULL) {
        switch (gKbd->keySet) {
        case 4:
            gKbd->keyInfo = p + D_00418743[gKbd->keyIdx] * 16;
            break;
        case 5:
            gKbd->keyInfo = p + (D_00418743[gKbd->keyIdx] + 12) * 16;
            break;
        }
    } else {
        gKbd->keyInfo = NULL;
    }
    return gKbd->keyIdx;
}

void Kbd_InitState(s8 arg0)
{
    gKbd = (KbdEntry *)&gKbdState;
    gKbd->polled = 0;
    gKbdSavedKanaInput = 0;
    Kbd_ClearUnk34(arg0);
    func_003634B0();
}

void Kbd_ClearUnk34(s8 arg0)
{
    if (arg0 == 0) {
        gKbd->unk34 = arg0;
    } else {
        gKbd->unk34 = 0;
    }
}

static s32 Sjis_IsSingleByte(u8 ch)
{
    s32 c;

    c = ch & 0xFF;
    if ((c >= 0x80) && (c < 0xA0)) {
        return 0;
    }
    if ((c >= 0xE0) && (c < 0x100)) {
        return 0;
    }
    return 1;
}

/* Kbd_Open: opens the software keyboard: mode kind (the gKbdKindConfs entry), layout,
 * at most max bytes, starting from text (none: empty). */
extern u8 gKbdKindConfs[][0x38];
extern u8 gKbdDelay;
extern u8 gKbdModeReq;
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
extern u8 gKbdOpenMode;
extern s8 gKbdPrompt[];
extern s16 gKbdChatTarget;
void sceUsbKbSetRepeat(s32 a, s32 b, s32 c);

void *memset(void *dst, s32 c, u32 n);
void Kbd_Open(s32 kind, u8 layout, u16 max, char *text)
{
    u8 *end;
    u8 *s;
    u8 k;

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
    k = gKbd->usbKey;
    switch (k) {
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x58:
        gKbdModeReq = 0;
        break;
    default:
        gKbdModeReq = k;
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
        s = (u8 *)gKbd->text;
        for (;;) {
            if (s >= end) {
            if (end == s && Sjis_IsSingleByte(*s) == 0) {
                gKbd->text[254] = 0;
            } else {
                gKbd->text[255] = 0;
            }
            break;
            }
            if (Sjis_IsSingleByte(*s) != 0) {
                s++;
            } else {
                s += 2;
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
        k = gKbd->usbKey;
        if (k != 0x28 && k != 0x58) {
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
    gKbdOpenMode = gKbd->usbKey;
    memset(&gKbd->usbKey, 0, 10);
    sceUsbKbSetRepeat(0, 10, 4);
    gKbd->kanaInput = gKbdSavedKanaInput;
    gKbd->prevKeySet = gKbd->keySet;
    memset(gKbdPrompt, 0, 4);
    gKbdChatTarget = -1;
}

extern s32 gKbdLayoutFlags[];
extern u8 gKbdKeySets[][2];

/* Switches the keyboard to layout k: its flags (gKbdLayoutFlags), the case key
 * (gKbdKeySets[k][0] unless kept; the one saved by Kbd_SaveKeySet when back on
 * the saved layout) and accept (gKbdKeySets[k][1]). */
void Kbd_SetLayout(u8 k)
{
    gKbd->kind = k;
    gKbd->flags = gKbdLayoutFlags[k];
    if (*(s32 *)(gKbd->kindConf + 0x30) == 0) {
        gKbd->flags |= 0x10;
        gKbd->flags |= 0x20;
    }
    switch (k) {
    case 0:
    case 0xE:
    case 0xC:
    case 0xD:
        if (Kbd_SetIsWide() == 0) {
            gKbd->keySet = gKbdKeySets[k][0];
        }
        break;
    case 3:
        if (gKbd->flags & (1 << gKbd->keySet)) {
            gKbd->keySet = gKbdKeySets[k][0];
        }
        break;
    case 0xB:
        break;
    default:
        gKbd->keySet = gKbdKeySets[k][0];
        break;
    }
    if (gKbdSavedKind == gKbd->kind) {
        gKbd->keySet = gKbdSavedKeySet;
    }
    if (gKbdSettings & 2) {
        switch (gKbd->keySet) {
        case 2:
        case 7:
        case 0xF:
        case 0xA:
            if ((s8)Kbd_ShiftPair(gKbd->keySet) >= 0) {
                gKbd->keySet = Kbd_ShiftPair(gKbd->keySet);
            }
            break;
        }
    }
    gKbd->accept = gKbdKeySets[k][1];
    Kbd_UpdateMode();
    Kbd_HomePageCursor();
    Kbd_UpdateSymbolMode();
}

/*
 * Key repeat for the keyboard (buttons masked with 0x3EFD): a new press is returned at
 * once and arms the timer (repeatWait) for 10 frames; a held button repeats when the timer
 * runs out (4 frames for 0x10 or while converting, 8 for 0x8/0x1/0x80, else 1).
 * The held set is repeatKeys; releasing everything clears it.
 */
s32 Kbd_KeyRepeat(s16 held, s16 pressed)
{
    s8 *timer;
    s32 r;
    s32 keys;

    pressed &= 0x3EFD;
    if (pressed) {
        gKbd->repeatKeys = pressed;
        gKbd->repeatWait = 10;
        return pressed;
    }
    keys = held & 0x3EFD;
    if ((s16)keys == 0) {
        return gKbd->repeatKeys = 0;
    }
    r = 0;
    --gKbd->repeatWait;
    timer = &gKbd->repeatWait;
    if (*timer <= 0) {
        if (gKbd->converting == 0) {
            if ((s16)keys & 0x10) {
                *timer = 4;
            } else if (((s16)keys & 0x8) || ((s16)keys & 0x1) || ((s16)keys & 0x80)) {
                *timer = 8;
            } else {
                *timer = 1;
            }
        } else {
            *timer = 4;
        }
        r = gKbd->repeatKeys &= (s16)keys;
    }
    return r;
}

void Kbd_PressCmdKey(KbdKey *p)
{
    u8 key;

    key = p->ch.b[1];
    switch (key) {
    case 0:
        Kbd_DeleteBack(1);
        break;
    case 1:
        if (gKbd->converting != 0) {
            Kbd_TypeKey(p);
            break;
        }
        Kbd_Convert();
        break;
    case 3:
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

/* The keyboard's frame update: waits out the gKbdDelay delay, takes the repeated keys,
 * moves the cursor, runs the pressed key or button, and ends with the result in result
 * (1: the text is copied to dst). */
s8 Kbd_Run(char *dst, s16 held, s16 pressed)
{
    KbdKey *key;
    s16 keys;
    s8 *res;

    gKbd->frame += 1;
    gKbd->submitWait -= 1;
    gKbd->blink += 1;
    if (*(s8 *)&gKbdDelay <= 0) {
        *(s8 *)&gKbdDelay = 0;
        if (gKbdModeReq != 0) {
            memset(&gKbd->usbKey, 0, 10);
            gKbd->kanaInput = gKbdSavedKanaInput;
            gKbd->prevKeySet = gKbd->keySet;
            gKbd->usbNewKey = gKbd->usbKey = gKbdModeReq;
            gKbdModeReq = 0;
        }
    } else {
        *(s8 *)&gKbdDelay -= 1;
        return 0;
    }
    gKbd->state = -1;
    Kbd_Update();
    keys = Kbd_KeyRepeat(held, pressed);
    if ((held != 0 || keys != 0) && gKbd->active == 0) {
        Kbd_SetActive();
        Kbd_KeyRepeat(0, 0);
        Sound_PlaySeFlat(0, 0x16, 0);
        return 0;
    }
    if (keys & 0x3C00) {
        gKbd->state = 0x17;
        Kbd_MoveCursor(keys, held);
        gKbd->keyFlags &= 0xFD;
    }
    if (keys & 0x20) {
        key = gKbd->key;
        if (gKbd->converting == 0) {
            if (gKbd->active == 1 && gKbd->offGrid == 1) {
                gKbd->state = 0x15;
            } else if (key->ch.code == 0) {
                gKbd->state = 0x15;
            } else {
                switch (key->ch.b[0]) {
                case 0:
                    break;
                case 1:
                    Kbd_PressKey(key);
                    break;
                case 2:
                    Kbd_PressCmdKey(key);
                    break;
                case 3:
                    Kbd_InsertPhrase();
                    if (gKbd->keyFlags & 2) {
                        Kbd_Submit();
                    }
                    break;
                case 4:
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
    } else if (pressed & 0x8000) {
        Kbd_Submit();
    } else if (keys & 0x40) {
        Kbd_DeleteBack(0);
    } else if (keys & 0x200) {
        Kbd_Space(1);
    } else if (pressed & 0x100) {
        Kbd_Submit();
    } else if ((pressed & 4) && gKbd->converting == 0) {
        Kbd_ToggleShift();
    } else if ((keys & 0x80) && gKbd->converting == 0) {
        Kbd_NextPhrasePage();
    } else if (gKbd->converting == 0) {
        if (keys & 8) {
            Kbd_NextRow(0);
        } else if (keys & 0x10) {
            switch (gKbd->keySet) {
            default:
            case 4:
            case 5:
                Kbd_CyclePhrase();
                break;
            case 13:
                Kbd_CycleGesture();
                break;
            }
        } else if (keys & 1) {
            Kbd_NextChatTarget();
        }
    } else {
        if (keys & 8) {
            Kbd_PrevSegment();
        }
        if (keys & 0x80) {
            Kbd_NextSegment();
        }
        if (keys & 4) {
            Kbd_PageUp();
        }
        if (keys & 0x10) {
            Kbd_PageDown();
        }
    }
    if (gKbd->result != 0) {
        if (gKbd->result == 1) {
            strcpy(dst, gKbd->text);
            Kbd_SaveKeySet();
        }
        if (gKbd->kind == 0xB) {
            gKbd->blink = 0;
            gKbd->kanaPos = 0;
            gKbd->firstShown = 0;
            gKbd->textPos = 0;
            gKbd->text[0] = 0;
            res = &gKbd->result;
            if (*res != -1) {
                *res = 0;
                Kbd_ClearCands();
            }
        } else {
            sceUsbKbSetRepeat(0, 0, 0);
            sceUsbKbClearRbuf(0);
        }
    }
    if (gKbd->state != -1) {
        Sound_PlaySeFlat(0, gKbd->state, 0);
    }
    return gKbd->result;
}

/* Moves the keyboard cursor by the pressed keys (0x400 right, 0x800 left, 0x1000 down,
 * 0x2000 up): on the key grid (col 0-20, row 0-3) until the key under it
 * (keyIdx) changes, leaving it for the buttons (offGrid) past the top or bottom row; while
 * converting or on the buttons, the text and button moves. */
void Kbd_MoveCursor(s16 keys, s16 held)
{
    u8 *row;
    u8 *dn;
    s8 key = gKbd->keyIdx;

    if (gKbd->offGrid == 0) {
        if (gKbd->converting != 0) {
            goto text;
        }
        do {
            if (keys & 0x800) {
                if (gKbd->col <= 0) {
                    gKbd->col = 20;
                } else {
                    gKbd->col -= 1;
                }
            } else if (keys & 0x400) {
                gKbd->col += 1;
                if (gKbd->col > 20) {
                    gKbd->col = 0;
                }
            }
            if (keys & 0x2000) {
                row = &gKbd->row;
                if (*row == 0) {
                    gKbd->offGrid = 1;
                    gKbd->blink = 0;
                    break;
                }
                *row -= 1;
            } else if (keys & 0x1000) {
                dn = &gKbd->row;
                if (*dn == 3) {
                    gKbd->offGrid = 1;
                    gKbd->blink = 0;
                    break;
                }
                *dn += 1;
            }
            Kbd_LookupKey();
        } while (key == gKbd->keyIdx);
    } else if (gKbd->converting != 0) {
        text:
        if (keys & 0x400) {
            Kbd_CursorRight();
        } else if (keys & 0x800) {
            Kbd_CursorLeft();
        } else if (keys & 0x2000) {
            Kbd_PrevCand();
        } else if (keys & 0x1000) {
            Kbd_NextCand();
        }
    } else {
        if (keys & 0x2000) {
            gKbd->row = 3;
            gKbd->offGrid = 0;
            Kbd_LookupKey();
        } else if (keys & 0x1000) {
            gKbd->row = 0;
            gKbd->offGrid = 0;
            Kbd_LookupKey();
        } else if (keys & 0x800) {
            Kbd_CursorLeft();
        } else if (keys & 0x400) {
            Kbd_CursorRight();
        }
    }
}

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

/* Enters one key's character (arg0 + 2) into the text field. */
void Kbd_EnterChar(KbdKey *key)
{
    s8 done;
    s32 n;
    u32 m;
    char buf[4];

    done = 0;
    if (Kbd_CharBlocked(key) == 1) {
        gKbd->state = 0x15;
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
        n = Str_InsertClip(gKbd->text, buf,
            gKbd->textPos, gKbd->maxLen);
        gKbd->textPos = gKbd->textPos + n;
        gKbd->state = 0x16;
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
        gKbd->state = 0x15;
        return;
    }
    m = gKbd->keySet;
    if (!(m <= 1 || m == 6 || m - 8 <= 1
          || (gKbd->mode == 0xD && (gKbd->gestureMode & 0xF)))) {
        goto skip;
    }
    switch (Kbd_DakutenKeyKind(key)) {
    case 0:
        break;
    case 1:
        if (Kana_SwapBeforeCursor(gKanaDakutenTable)) {
            done = 1;
        }
        break;
    case 2:
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
            n = Str_InsertClip((char *)gKbd->kana, buf,
                gKbd->kanaPos, 0x48);
            gKbd->kanaPos = gKbd->kanaPos + n;
        }
    }
    gKbd->state = 0x16;
}

void Kbd_CyclePhrase(void)
{
    s16 *count;
    s32 flags;
    u8 *row;

    if (gKbd->converting != 0 || ((flags = gKbd->flags) & 0x10) ||
        (flags & 0x20) || *(s32 *)(gKbd->kindConf + 0x30) == 0) {
        gKbd->state = 0x15;
        return;
    }
    if (!(gKbd->keyFlags & 2)) {
        gKbd->offGrid = 0;
        gKbd->keyFlags |= 2;
        gKbd->cyclePos = 1;
        switch (gKbd->keySet) {
        case 4:
        default:
            Kbd_OpenPhrases1();
            break;
        case 5:
            Kbd_OpenPhrases2();
            break;
        }
    } else {
        gKbd->cyclePos += 5;
        count = &gKbd->cyclePos;
        if (*count >= 60) {
            *count = 1;
        }
        gKbd->col = (gKbd->cyclePos + 1) % 20;
        gKbd->row = (gKbd->cyclePos + 1) / 20;
        row = &gKbd->col;
        if (*row > 19 || gKbd->row >= 3) {
            *row = 5;
            gKbd->row = 0;
            gKbd->cyclePos = 1;
        }
    }
    Kbd_InsertPhrase();
}

void Kbd_CycleGesture(void)
{
    s16 *count;
    u8 *row;

    if (gKbd->converting != 0 || (gKbd->flags & 0x2000) ||
        *(s32 *)(gKbd->kindConf + 0x28) == 0) {
        gKbd->state = 0x15;
        return;
    }
    if (!(gKbd->keyFlags & 2)) {
        gKbd->offGrid = 0;
        gKbd->keyFlags |= 2;
        gKbd->cyclePos = 1;
        gKbd->keySet = 0xD;
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_EnterGestures();
        Kbd_RefreshLayout();
    } else {
        gKbd->cyclePos += 5;
        count = &gKbd->cyclePos;
        if (*count >= 60) {
            *count = 1;
        }
        gKbd->col = (gKbd->cyclePos + 1) % 20;
        gKbd->row = (gKbd->cyclePos + 1) / 20;
        row = &gKbd->col;
        if (*row > 19 || gKbd->row >= 3) {
            *row = 5;
            gKbd->row = 0;
            gKbd->cyclePos = 1;
        }
    }
    Kbd_RefreshLayout();
    gKbd->state = 0x16;
}

extern s16 gKbdChatTarget;
extern s8 gKbdPrompt[];
s32 Chat_NextTarget(s8 *out, s32 next);

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
        gKbd->state = 0x15;
    } else {
        gKbd->state = 0x17;
    }
}

extern u8 D_00418739[];

void Kbd_InsertPhrase(void)
{
    char buf[0x100];
    s32 n;

    if (gKbd->row == D_00418739[0]) {
        return;
    }
    Kbd_RefreshLayout();
    if (gKbd->keyInfo == NULL) {
        return;
    }
    if (gKbd->keyFlags & 2) {
        Kbd_ClearText();
    }
    if (*(char **)(gKbd->keyInfo + 0xC) == NULL) {
        gKbd->state = 0x15;
        return;
    }
    strcpy(buf, *(char **)(gKbd->keyInfo + 0xC));
    n = Str_InsertClip(gKbd->text, buf, gKbd->textPos,
                      gKbd->maxLen);
    gKbd->textPos = gKbd->textPos + n;
    gKbd->state = 0x16;
}

extern s8 gKbdWidthPairs[];

static s32 Kbd_WidthPair(u8 i)
{
    s32 v = gKbdWidthPairs[i];

    if (v >= 0 && !(gKbd->flags & (1 << (s8)v))) {
        return v;
    }
    return -1;
}

void Kbd_ToggleWidth(void)
{
    s32 state = 0x15;
    s8 k;
    u8 a;
    u8 b;

    k = Kbd_WidthPair(gKbd->keySet);
    if (k >= 0) {
        a = gKbd->col;
        b = gKbd->row;
        gKbd->keySet = k;
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_CommitKana();
        Kbd_RefreshLayout();
        Kbd_UpdateSymbolMode();
        gKbd->col = a;
        gKbd->row = b;
        state = 0x16;
    }
    gKbd->state = state;
}

/* Backspace: deletes the character before the cursor (or leaves the conversion). The
 * kana store goes through u8 (a char store adds n + kana in the other order). */
void Kbd_DeleteBack(s32 unused)
{
    u8 *mode;
    char *kana;
    s16 *pos;
    s32 size;
    s32 n;
    char *text;

    if (gKbd->kind != 0xC && gKbd->kind != 0xD &&
        gKbd->kana[0] == 0 && (u8)gKbd->text[0] == 0) {
        gKbd->result = -1;
        gKbd->state = 0x14;
        return;
    }
    if (gKbd->keyFlags & 2) {
        gKbd->result = -1;
        gKbd->state = 0x14;
        return;
    }
    mode = &gKbd->converting;
    if (*mode == 0) {
        kana = (char *)gKbd->kana;
        if (*(u8 *)kana != 0) {
            n = gKbd->kanaPos;
            if (n == 0) {
                gKbd->state = 0x15;
                return;
            }
            *(u8 *)(kana + n - 2) = 0;
            strcat(kana, kana + n);
            gKbd->kanaPos -= 2;
            pos = (s16 *)&gKbd->kanaPos;
            if (*pos <= 0) {
                *pos = 0;
                Kbd_KeyRepeat(0, 0);
            }
        } else {
            n = gKbd->textPos;
            text = gKbd->text;
            if (n == 0) {
                gKbd->state = 0x15;
                return;
            }
            size = Sjis_PrevCharSize((u8 *)text, n);
            *(text + n - size) = 0;
            strcat(text, text + n);
            gKbd->textPos = gKbd->textPos - size;
            pos = (s16 *)&gKbd->textPos;
            if (*pos <= 0) {
                *pos = 0;
                Kbd_KeyRepeat(0, 0);
            }
        }
        gKbd->blink = 0;
        gKbd->state = 0x16;
        return;
    }
    *mode = 0;
    gKbd->blink = 0;
    gKbd->offGrid = 0;
    if (gKbd->symbolMode != 0) {
        gKbd->kanaPos = 0;
        gKbd->kana[0] = 0;
    }
    gKbd->state = 0x16;
}

void Kbd_Submit(void)
{
    s32 *p;

    if (gKbd->kana[0] != 0) {
        Kbd_CommitKana();
        gKbdPhrasePage = 0;
        gKbd->state = 0x16;
        return;
    }
    p = &gKbd->submitWait;
    if (*p != 0) {
        if (*p < 0) {
            *p = 0;
        } else {
            gKbd->state = 0x15;
            return;
        }
    }
    gKbd->result = 1;
    if (gKbd->kana[0] == 0 && (u8)gKbd->text[0] == 0) {
        gKbd->state = 0x14;
    } else {
        gKbd->state = 0x18;
    }
}

s32 Kbd_FindKind(u8 row, u8 *out1F, u8 *out1E);

/* Moves the cursor to the next (back: previous) of the four rows that
 * Kbd_FindKind accepts; when none, restores the cursor (state 0x15). */
void Kbd_NextRow(s32 back)
{
    s32 i;
    s32 found;
    u8 key;
    s8 cur;
    u8 row;

    row = gKbd->mode;
    key = gKbd->keySet;
    found = 0;

    if (row == 13 || row == 4 || row == 5) {
        if (back) {
            cur = 0;
        } else {
            cur = 3;
        }
    } else {
        cur = row;
    }
    for (i = 0; i < 4; i++) {
        if (back) {
            cur--;
        } else {
            cur++;
        }
        cur = (cur + 4) % 4;
        if (Kbd_FindKind(cur, &gKbd->mode, &gKbd->keySet) == 0 && cur != row) {
            found = 1;
            break;
        }
    }
    if (!found) {
        gKbd->keySet = key;
        gKbd->mode = row;
        gKbd->state = 0x15;
    } else {
        gKbd->cyclePos = 0;
        gKbd->gestureMode = 0;
        gKbd->keyFlags &= ~2;
        gKbd->state = 0x16;
        switch (row) {
        case 13:
        case 4:
        case 5:
            gKbd->col = 1;
            gKbd->row = 0;
            break;
        }
    }
    if (gKbd->keySet == 13) {
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
void Kbd_BindLayouts(s32 unused) /* unused: the caller passes 0 */
{
    KbdLayout *l;

    l = &gKbdSetLayouts[13];
    l->cursors = gKbdGestureBinds[*(s32 *)(gKbd->kindConf + 0x28)].cursors;
    l->grid = gKbdGestureBinds[*(s32 *)(gKbd->kindConf + 0x28)].grid;
    l->extra = *(u8 **)(gKbd->kindConf + 0x2C);
    l = &gKbdSetLayouts[4];
    l->cursors = gKbdPhrase1Binds[*(s32 *)(gKbd->kindConf + 0x30)].cursors;
    l->grid = gKbdPhrase1Binds[*(s32 *)(gKbd->kindConf + 0x30)].grid;
    l->extra = *(u8 **)(gKbd->kindConf + 0x34);
    l = &gKbdSetLayouts[5];
    l->cursors = gKbdPhrase2Binds[*(s32 *)(gKbd->kindConf + 0x30)].cursors;
    l->grid = gKbdPhrase2Binds[*(s32 *)(gKbd->kindConf + 0x30)].grid;
    l->extra = *(u8 **)(gKbd->kindConf + 0x34);
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

/* Copies the three switch keys (to the first and second phrases pages and the gestures page:
 * the key after each bind's grid, or a blank key when the keyboard kind has no such page)
 * to keys 4-6 of every keyboard page. The context pointer is a 64-bit local: the game spills it
 * with sq/lq (an s32 or pointer local, or the global itself, is spilled with sw). */
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

/* Replaces the character before the kana cursor by its pair in table (4-byte entries: from, to). */
s32 Kana_SwapBeforeCursor(s8 *table)
{
    s8 *p;
    u16 code;
    s32 i;
    s32 n;
    s32 pos;
    u16 from;

    pos = gKbd->kanaPos;
    if (pos <= 1) {
        return 0;
    }
    p = (s8 *)&gKbd->kana[pos - 2];
    code = (p[0] << 8) | (u8)p[1];
    n = (u32)strlen((char *)table) / 4;
    for (i = 0; i < n; i++) {
        from = (u16)(*table++ << 8);
        from |= (u8)*table++;
        if (from == code) {
            p[0] = *table++;
            p[1] = *table++;
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
    s8 c;

    while ((c = *src++) != 0) {
        if (c >= 'A' && c <= 'Z') {
            dst[0] = 0x82;
            dst[1] = c + 0x1F;
            src++;
            dst += 2;
        } else if (c >= 'a' && c <= 'z') {
            dst[0] = 0x82;
            dst[1] = c + 0x20;
            src++;
            dst += 2;
        } else {
            dst[0] = c;
            dst[1] = *src;
            src++;
            dst += 2;
        }
    }
    *dst = 0;
}

void Kbd_FillRect(s32 color, s16 x, s16 y, s16 w, s16 h);

void Kbd_DrawCursorBox(s16 dy, s32 color, f32 dx)
{
    KbdKeyPos *c = gKbd->keyPos;
    f32 scale = gKbd->drawScale;

    Kbd_FillRect(color, (s16)(s32)(dx + c->x * scale), dy + c->y, (s16)(s32)(dx + scale * (c->x + gKbdBoxSizes[c->kind].w)),
                  dy + (c->y + gKbdBoxSizes[c->kind].h));
}

typedef struct KeyPos {
    u8 row;
    u8 col;
} KeyPos;
extern KeyPos gKbdSpecialKeyPos[];

/* Draws the box of the special key of the mode, unless the cursor is on it. */
void Kbd_DrawSpecialKey(s16 dy, s32 color, f32 dx)
{
    KbdLayout *l;
    KbdKeyPos *c;
    f32 scale;
    s32 ofs;
    s32 cur;
    u8 row;
    u8 col;
    s32 kc;
    s32 kr;

    kc = gKbd->col;
    kr = gKbd->row;
    cur = (kc << 2) + kr;
    ofs = gKbd->mode * sizeof(KeyPos); /* byte offset of the mode's KeyPos */
    row = (&gKbdSpecialKeyPos->row)[ofs];
    col = (&gKbdSpecialKeyPos->col)[ofs];
    switch (gKbd->mode) {
    case 4:
        if (gKbd->layout->grid[cur] == 4 && col == kr) {
            return;
        }
        break;
    case 5:
        if (gKbd->layout->grid[cur] == 5 && col == kr) {
            return;
        }
        break;
    case 0xD:
        if (gKbd->layout->grid[cur] == 6 && col == kr) {
            return;
        }
        break;
    default:
        if (gKbd->active == 1 && gKbd->offGrid == 0 &&
            row == kc && col == kr) {
            return;
        }
        break;
    }
    cur = (row << 2) + col;
    l = gKbd->layout;
    scale = gKbd->drawScale;
    c = &l->cursors[l->grid[cur]];
    Kbd_FillRect(color, (s16)(s32)(dx + c->x * scale), dy + c->y, (s16)(s32)(dx + scale * (c->x + gKbdBoxSizes[c->kind].w)),
                  dy + (c->y + gKbdBoxSizes[c->kind].h));
}

void Kbd_FillRect(s32 color, s16 x, s16 y, s16 w, s16 h)
{
    Rect16 r;

    r.color = color;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    Prim_FillRect((struct PrimLine *)&r);
}

s32 Sprite_Draw(Sprite *s);

void Kbd_DrawSprite(u32 color, s16 x, s16 y, s16 w, s16 h, s16 u0, s16 v0, s16 u1, s16 v1)
{
    Sprite s;

    s.color = color;
    s.x = x;
    s.y = y;
    s.w = w;
    s.h = h;
    s.u0 = u0;
    s.v0 = v0;
    s.u1 = u1;
    s.v1 = v1;
    Sprite_Draw(&s);
}

extern u8 gKbdOpenMode;
extern u8 gKbdKindIs0;

s32 Kbd_KeysShown(void)
{
    if ((gKbd->active ^ 1) && gKbd->layout->extra == NULL) {
        switch (gKbdOpenMode) {
        case 0x44:
            if (gKbdKindIs0 == 0) {
                goto no; /* an if/return pair would become a set-on-condition */
            }
            return 1;
        no:
            return 0;
        case 0x42:
        case 0x43:
        case 0x45:
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
extern s32 D_005AEA58;
f32 Math_Sin(f32 x);
void Text_FlushPages(void);
void Text_DrawCurrentPage(void);
void Text_SetColor(s16 color);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Tex_Bind148(void);
void Gfx_SetBlendAlphaAdd(s32 a);
void Gfx_SetState63(s32 a);
s32 Kbd_ArrowsOnPage(void);
void Kbd_DrawLine(f32 x, s16 y); /* unit's own declaration: the definition has
 * u32 y_in; Kbd_Draw matches with s16 y (with u32 it was 40 words further off) */
void Kbd_DrawCandidateBox(f32 x, s16 y);
void Kbd_DrawSpecialKey(s16 dy, s32 color, f32 dx);

#define KBD_COLW(i) (((u32 *)gKbd->kindConf)[i])
#define KBD_COLOR(i) (((KbdColor *)gKbd->kindConf)[i])
#define KBD_PULSE(v, i, amp) v = KBD_COLW(i) & 0xFFFFFF; v |= (((KBD_COLW(i) >> 24) & 0xFF) + (s8)(sin * (amp))) << 24

/* Draws the software keyboard: the panel, the keys, the page characters, the cursor, the
 * special key of the mode, the key glyphs, then the text line and the candidates.
 * The grid is indexed (col << 2) + row, so the bytes load in the original's order. The box loop
 * counts with i and walks c, so that the page loop's i and c are later webs of both; the page
 * loop sets p after i = 0, in the for's init. A glyph's width is an s32 holding the (s16) of
 * its byte, read like its scale (the glyph loops' saved registers come from it, and from their
 * declaration orders, which also give the stack slots); its texel column is (code & 0xF) << 4.
 * The pulsing colour is set in two statements (KBD_PULSE) so that it is computed before the
 * call's arguments, as in the original. */
void Kbd_Draw(u8 a)
{
    KbdLayout *l;
    KbdKey *k;
    f32 x;
    f32 sin;
    s16 y;
    u32 i;
    KbdKeyPos *c;
    s8 spec;
    s8 sel;
    u32 color;

    x = gKbd->x;
    y = gKbd->y;
    sin = Math_Sin(0.0000958738f * (u16)((gKbd->frame & 0x3F) << 10));
    gKbdInactive = gKbd->active ^ 1;
    Text_FlushPages();
    gKbd->drawScale = 1.0f;
    x *= gKbd->drawScale;
    if (gKbdInactive != 0 && gKbd->layout->extra == 0) {
        switch (gKbdOpenMode) {
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
            gKbdOpenMode = 0;
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
    l = gKbd->layout;
    c = l->cursors;
    for (i = l->nBoxes; i != 0; i--) {
        if (gKbdInactive != 0 && gKbd->layout->extra == 0) {
            Kbd_DrawSpecialKey(y - gKbd->mode * 17 - 26, KBD_COLW(1), x - 56.0f * gKbd->drawScale);
            Kbd_DrawSpecialKey(y - gKbd->mode * 17 - 26, KBD_COLW(1), x - 30.0f * gKbd->drawScale);
            break;
        }
        Kbd_FillRect(KBD_COLW(1), (s32)(x + c->x * gKbd->drawScale), y + c->y,
                     (s32)(x + gKbd->drawScale * (c->x + gKbdBoxSizes[c->kind].w)), y + c->y + gKbdBoxSizes[c->kind].h);
        c++;
    }
    Tex_Bind148();
    Gfx_SetState63(1);
    l = gKbd->layout;
    if (l->extra != 0 && gKbd->gestureMode == 0) {
        s8 cur;
        s8 *p;

        cur = l->grid[(u32)((gKbd->col << 2) + gKbd->row)];
        c = l->cursors + 13;
        Font_SetSpacing(20, 16);
        for (i = 0, p = gKbdPhraseKeyOrder; i < 12; i++, p++, c++) {
            Font_SetPos(c->x + x / gKbd->drawScale, y + c->y);
            if (cur == i + 13 && (gKbd->offGrid == 0 || gKbd->active != 1)) {
                Text_SetColor(1);
            } else {
                Text_SetColor(0);
            }
            switch (gKbd->keySet) {
            default:
            case 4:
                Kbd_Printf(D_0046CC10, gKbd->layout->extra + *p * 16);
                break;
            case 5:
                Kbd_Printf(D_0046CC10, gKbd->layout->extra + (*p + 12) * 16);
                break;
            }
        }
    }
    Gfx_SetState63(0);
    if ((gKbdInactive == 0 || gKbd->layout->extra != 0) && D_005AEA58 == 0 &&
        (gKbd->offGrid == 0 || Kbd_ArrowsOnPage() == 1)) {
        k = gKbd->key;
        Gfx_SetBlendAlphaAdd(1);
        if (Kbd_CharBlocked(k) == 1) {
            KBD_PULSE(color, 7, gKbdPulseAmp[0]);
        } else {
            KBD_PULSE(color, 6, gKbdPulseAmp[0]);
        }
        Kbd_DrawCursorBox(y, color, x);
    }
    if (D_005AEA58 > 0) {
        D_005AEA58--;
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
    Gfx_SetState63(1);
    l = gKbd->layout;
    c = l->cursors;
    k = l->keys;
    sel = l->grid[(u32)((gKbd->col << 2) + gKbd->row)];
    spec = l->grid[(u32)((gKbdSpecialKeyPos[gKbd->mode].row << 2) + gKbdSpecialKeyPos[gKbd->mode].col)];
    if (gKbdInactive != 0 && l->extra == 0) {
        for (i = 0; i < gKbd->layout->nKeys; i++, c++, k++) {
            if (i >= 13 || i == spec || i == 7) {
                s32 gw;
                s32 sw;
                s16 pad;
                s16 gh;
                u8 code;
                u8 g;

                if (i >= 13) {
                    break;
                }
                if (Kbd_CharBlocked(k) == 1) {
                    code = 0xC5;
                    color = KBD_COLW(4);
                    g = 0;
                } else {
                    color = KBD_COLW(3);
                    code = k->glyph;
                    g = k->font & 0xF;
                }
                gw = (s16)gKbdGlyphs[g].w;
                gh = gKbdGlyphs[g].h;
                sw = (gw * (s16)gKbdGlyphs[g].scale) >> 4;
                pad = (s16)(gKbd->drawScale * gKbdBoxSizes[c->kind].w) - sw;
                if (i == sel) {
                    if (D_005AEA58 == 0 && (gKbd->offGrid == 0 || Kbd_ArrowsOnPage() == 1)) {
                        color = KBD_COLW(5);
                    } else if (sel == spec && i < 4) {
                        color = KBD_COLW(5);
                    } else {
                        color = KBD_COLW(3);
                    }
                } else if (i == spec) {
                    color = KBD_COLW(5);
                }
                if (i == 7) {
                    s16 w = gKbdGlyphs[0].w;
                    s16 sc = gKbdGlyphs[0].scale;
                    s16 h = gKbdGlyphs[0].h;

                    switch (gKbd->keySet) {
                    case 6:
                    case 7:
                    case 0xB:
                    case 0xE:
                    case 0xF:
                    case 0x10:
                        code = gKbdWidthKeys[0];
                        break;
                    default:
                        code = gKbdWidthKeys_1;
                        break;
                    }
                    Kbd_DrawSprite(((s32 *)gKbd->kindConf)[5], x + -20.0f * gKbd->drawScale, y + 3,
                                   (w * sc) >> 4, h,
                                   ((code & 0xF) << 4) + 1, (code & 0xF0) + 1,
                                   ((code & 0xF) << 4) - 1 + w, (code & 0xF0) + 16);
                } else {
                    Kbd_DrawSprite(color, (f32)(pad >> 1) + (x + (c->x - 56.0f) * gKbd->drawScale), y + 3, sw, gh,
                                   ((code & 0xF) << 4) + 1, (code & 0xF0) + 1,
                                   ((code & 0xF) << 4) - 1 + gw, (code & 0xF0) + 16);
                }
            }
        }
    } else {
        for (i = 0; i < gKbd->layout->nKeys; i++, c++, k++) {
            u8 code;
            u8 g;
            s32 sw;
            s32 gw;
            s16 gh;
            s16 pad;

            if (Kbd_CharBlocked(k) == 1) {
                code = 0xC5;
                color = KBD_COLW(4);
                g = 0;
            } else {
                code = k->glyph;
                color = KBD_COLW(3);
                g = k->font & 0xF;
            }
            gw = (s16)gKbdGlyphs[g].w;
            gh = gKbdGlyphs[g].h;
            sw = (gw * (s16)gKbdGlyphs[g].scale) >> 4;
            pad = (s16)(gKbd->drawScale * gKbdBoxSizes[c->kind].w) - sw;
            if (i == sel) {
                if (D_005AEA58 == 0 && (gKbd->offGrid == 0 || Kbd_ArrowsOnPage() == 1)) {
                    color = KBD_COLW(5);
                } else if (sel == spec && i < 4) {
                    color = KBD_COLW(5);
                } else {
                    color = KBD_COLW(3);
                }
            } else if (i == spec) {
                color = KBD_COLW(5);
            }
            Kbd_DrawSprite(color, (f32)(pad >> 1) + (x + c->x * gKbd->drawScale), y + c->y + 16 - gh, sw, gh,
                           ((code & 0xF) << 4) + 1, (code & 0xF0) + 1,
                           ((code & 0xF) << 4) - 1 + gw, (code & 0xF0) + 16);
        }
    }
    Gfx_SetState63(0);
    Kbd_DrawLine(2.0f + (14.0f + x) / gKbd->drawScale, y + 0);
    Kbd_DrawCandidateBox(2.0f + (14.0f + x) / gKbd->drawScale, y - 1);
    Text_DrawCurrentPage();
}

void Kbd_ToggleShift(void);

/* A key pressed: Kbd_FindKindEx moves the cursor (then state 0x15), else the
 * key's action by kind (key->ch.b[1]). */
void Kbd_PressKey(KbdKey *key)
{
    if (Kbd_FindKindEx(key->ch.b[1], &gKbd->mode, &gKbd->keySet) != 0) {
        gKbd->state = 0x15;
        return;
    }
    gKbd->cyclePos = 0;
    gKbd->gestureMode = 0;
    Kbd_UpdateSymbolMode();
    switch (key->ch.b[1]) {
    case 13:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_EnterGestures();
        Kbd_RefreshLayout();
        gKbd->state = 0x16;
        break;
    case 3:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        if (gKbd->symbolMode != 0) {
            Kbd_ConvertOrCommit();
            Kbd_KeyRepeat(0, 0);
        } else {
            gKbd->state = 0x16;
        }
        Kbd_RefreshLayout();
        break;
    case 4:
        Kbd_OpenPhrases1();
        break;
    case 5:
        Kbd_OpenPhrases2();
        break;
    case 0:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_RefreshLayout();
        gKbd->state = 0x16;
        break;
    case 1:
    case 2:
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_RefreshLayout();
        gKbd->state = 0x16;
        break;
    case 6:
    case 7:
        Kbd_ToggleWidth();
        break;
    case 8:
    case 9:
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

s32 Kbd_DakutenKeyKind(KbdKey *p)
{
    u8 key;

    key = p->ch.b[1];
    if ((key == gKbdDakutenSjis_1) && (p->ch.b[0] == gKbdDakutenSjis[0])) {
        return 1;
    }
    if ((key == gKbdHandakutenSjis_1) && (p->ch.b[0] == gKbdHandakutenSjis[0])) {
        return 2;
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

void Kbd_Cancel(void)
{
    if ((s8)Kbd_IsOpen() != 0) {
        Kbd_ClearOpen();
        Kbd_ClearActive();
        gKbd->result = -1;
        if (gKbd->state == -1) {
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        Kbd_SaveKeySet();
    }
}

void Kbd_Close(void)
{
    if (Kbd_IsOpen() != 0) {
        Kbd_ClearOpen();
        Kbd_ClearActive();
        gKbd->result = -1;
        Kbd_SaveKeySet();
    }
}

void Kbd_SetPos(s32 n, f32 f)
{
    gKbd->x = f;
    gKbd->y = (s16) n;
}

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

/* The first of kind and its alternatives (by kind: kind | 8, +5, +8, +13, 16) whose bit is
 * clear in the mask flags: kind into outA and it into outB, 0; 1 when all are set (or kind
 * 4, 5, 0xD is off). Kind 2 uses 10 and its alternatives without bit 1 of gKbdSettings. */
s32 Kbd_FindKind(u8 kind, u8 *outA, u8 *outB)
{
    u8 dummy;
    u8 k2;
    u8 k4;
    u8 k1;
    u8 k3;
    s32 mask;

    if (outA == NULL) {
        outA = &dummy;
    }
    if (outB == NULL) {
        outB = &dummy;
    }
    switch (kind) {
    case 0:
        k2 = kind;
        k4 = kind;
        k1 = kind | 8;
        k3 = kind;
        break;
    case 1:
        k2 = kind + 5;
        k4 = kind + 5;
        k1 = kind | 8;
        k3 = kind + 13;
        break;
    case 2:
        if (gKbdSettings & 2) {
            k2 = kind + 5;
            k4 = 16;
            k1 = kind | 8;
            k3 = kind + 13;
        } else {
            k1 = 2;
            kind = 10;
            k2 = 15;
            k4 = 16;
            k3 = 7;
        }
        break;
    case 3:
        k2 = kind + 8;
        k4 = kind + 8;
        k1 = kind;
        k3 = kind;
        break;
    case 13:
        if (*(s32 *)(gKbd->kindConf + 0x28) == 0) {
            return 1;
        }
        goto one;
    case 4:
    case 5:
        if (*(s32 *)(gKbd->kindConf + 0x30) == 0) {
            return 1;
        }
    default:
    one:
        k3 = kind;
        k1 = kind;
        k4 = kind;
        k2 = kind;
        break;
    }
    mask = gKbd->flags;
    if (mask & (1 << kind)) {
        if (mask & (1 << k1)) {
            if (mask & (1 << k2)) {
                if (mask & (1 << k3)) {
                    if (!(mask & (1 << k4))) {
                        *outA = kind;
                        *outB = k4;
                        return 0;
                    }
                } else {
                    *outA = kind;
                    *outB = k3;
                    return 0;
                }
            } else {
                *outA = kind;
                *outB = k2;
                return 0;
            }
        } else {
            *outA = kind;
            *outB = k1;
            return 0;
        }
    } else {
        *outA = kind;
        *outB = kind;
        return 0;
    }
    return 1;
}

/* Kbd_FindKind for the kinds up to 5 and 0xD; for 6 to 9 whether the key (outB, or keySet) is
 * unavailable (1) or not (0). */
s32 Kbd_FindKindEx(u8 kind, u8 *outA, u8 *outB)
{
    u8 key;

    if (kind <= 5 || kind == 0xD) {
        return Kbd_FindKind(kind, outA, outB);
    }
    if (outB == NULL) {
        key = gKbd->keySet;
    } else {
        key = *outB;
    }
    switch (kind) {
    case 6:
    case 7:
        if ((s8)Kbd_WidthPair(key) >= 0) {
            return 0;
        }
        break;
    case 8:
        if ((s8)Kbd_ShiftPair(key) >= 0 && !(gKbd->keySet & 8)) {
            return 0;
        }
        break;
    case 9:
        if ((s8)Kbd_ShiftPair(key) >= 0 && (gKbd->keySet & 8)) {
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

s32 Kbd_ShiftPair(u8 i)
{
    s32 v = gKbdShiftSets[i];

    if (v >= 0 && !(gKbd->flags & (1 << (s8)v))) {
        return v;
    }
    return -1;
}

void Kbd_ToggleShift(void)
{
    s8 k;
    u8 a;
    u8 b;

    k = Kbd_ShiftPair(gKbd->keySet);
    if (k >= 0) {
        a = gKbd->col;
        b = gKbd->row;
        gKbd->keySet = k;
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_RefreshLayout();
        Kbd_UpdateSymbolMode();
        gKbd->state = 0x16;
        gKbd->col = a;
        gKbd->row = b;
        Kbd_LookupKey();
        return;
    }
    gKbd->state = 0x15;
}

extern u8 gKbdKindIs0;

void Kbd_NextPhrasePage(void)
{
    switch (gKbd->keySet) {
    case 0xD:
    case 4:
    case 5:
        break;
    default:
        Kbd_OpenPhrases1();
        return;
    }
    switch (gKbdPhrasePage) {
    case 0:
        Kbd_OpenPhrases1();
        break;
    case 4:
        if (gKbd->keyFlags & 2) {
            gKbd->cyclePos = 1;
            Kbd_ClearText();
            gKbd->keyFlags &= 0xFD;
        }
        Kbd_OpenPhrases2();
        break;
    case 5:
        if (gKbd->keyFlags & 2) {
            gKbd->cyclePos = 1;
            Kbd_ClearText();
            gKbd->keyFlags &= 0xFD;
        }
        if (gKbdKindIs0 == 0) {
            gKbd->keySet = 4;
            Kbd_OpenPhrases1();
        } else {
            gKbd->keySet = 0xD;
            Kbd_UpdateMode();
            gKbd->col = 5;
            gKbd->row = 0;
            Kbd_EnterGestures();
            Kbd_RefreshLayout();
            gKbd->state = 0x16;
        }
        break;
    case 0xD:
        if (gKbd->keyFlags & 2) {
            gKbd->cyclePos = 1;
            Kbd_ClearText();
            gKbd->keyFlags &= 0xFD;
        }
        Kbd_OpenPhrases1();
        break;
    }
}

void Kbd_HomePageCursor(void)
{
    switch (gKbd->mode) {
    case 0xD:
    case 4:
    case 5:
        gKbd->col = 5;
        gKbd->row = 0;
        break;
    }
}

extern KbdLayout gKbdAltLayouts[];
extern KbdLayout gKbdAltLayouts_1;
extern KbdLayout gKbdAltLayouts_2;
extern KbdLayout gKbdAltLayouts_3;

/* Picks the keyboard layout for the entry kind and the key set (keySet). */
void Kbd_PickLayout(void)
{
    u8 *set;

    switch (gKbd->kind) {
    case 7:
        gKbd->layout = &gKbdSetLayouts[17];
        break;
    case 8:
    case 9:
    case 0xA:
        set = &gKbd->keySet;
        switch (*set) {
        case 2:
            gKbd->layout = &gKbdSetLayouts[18];
            break;
        case 0xA:
            gKbd->layout = &gKbdSetLayouts[19];
            break;
        case 7:
            gKbd->layout = gKbdAltLayouts;
            break;
        case 0xF:
            gKbd->layout = &gKbdAltLayouts_1;
            break;
        case 3:
            *set = 0xB;
        case 0xB:
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
    if (gKbd->mode == 0xD && (gKbd->gestureMode & 0xF)) {
        if (!(gKbd->flags & 1)) {
            gKbd->layout = &gKbdSetLayouts[0];
        } else if (!(gKbd->flags & 0x8000)) {
            gKbd->layout = &gKbdSetLayouts[15];
        } else {
            gKbd->layout = &gKbdSetLayouts[7];
        }
    }
}

void Kbd_EnterGestures(void)
{
    Kbd_SetPhrasePage(0xDU);
    Kbd_CommitKana();
    if (func_003151C0(gKbd->text, &gKbd->textPos) < 0) {
        gKbd->gestureMode = (u8) (gKbd->gestureMode | 0xF);
    }
}

void func_00319FE0(void)
{
    D_005AEAD7 = 0;
}

/* 1 when the code at p is one of eleven kana (keyboard modes 0 and 1) or its second
 * character is a full-width space (D_0046CC08). */
s32 Kbd_CharIsSpecialKana(KbdKey *p)
{
    char buf[4];
    s32 r = 0;

    if (gKbd->keySet == 0 || gKbd->keySet == 1) {
        switch (p->glyph) {
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
            r = 1;
            break;
        }
    }
    strncpy(buf, (char *)p->ch.b, 2);
    buf[3] = 0;
    if (strncmp(buf, D_0046CC08, 2) == 0) {
        r = 1;
    }
    return r;
}

/* 1 when the character (entry p of a key table) cannot be typed in the current mode. */
s32 Kbd_CharBlocked(KbdKey *p)
{
    s32 bad = 0;

    if (p->ch.code == 0) {
        bad = 1;
    } else if (p->ch.b[0] == 1) {
        if (Kbd_FindKindEx(p->ch.b[1], NULL, NULL) != 0) {
            bad = 1;
        }
    } else if (Kbd_KindIs8To10() == 1) {
        if (p->ch.b[0] == 2 && p->ch.b[1] == 1) {
            bad = 1;
        } else if (Kbd_CharIsSpecialKana(p) != 0) {
            bad = 1;
        }
    } else if (p->glyph != 0xD1 && p->glyph != 0x7D) {
        switch (gKbd->kind) {
        case 0xF:
            if (Kbd_CharInSet5_Unk(p) != 0) {
                bad = 1;
            }
            break;
        case 1:
        case 5:
        case 2:
            if (Kbd_CharInSet4_Unk(p) != 0) {
                bad = 1;
            }
            break;
        case 7:
            if (Kbd_CharIsPlain(p) != 0) {
                bad = 1;
            }
            break;
        case 6:
            if (Kbd_CharInRows01(p) == 0 && Kbd_CharInRowsHigh(p) == 0) {
                bad = 1;
            }
            break;
        }
    }
    return bad;
}

s32 Kbd_CharInSet5_Unk(KbdKey *p)
{
    s32 r = 0;

    switch (p->glyph) {
    case 0xE3:
    case 0xB5:
    case 0xB6:
    case 0xB9:
    case 0xF1:
        r = 1;
        break;
    }
    return r;
}

s32 Kbd_CharInSet4_Unk(KbdKey *p)
{
    s32 r = 0;

    switch (p->glyph) {
    case 0xE3:
    case 0xB5:
    case 0xB6:
    case 0xB9:
        r = 1;
        break;
    }
    return r;
}

s32 Kbd_CharInRows01(KbdKey *p)
{
    u8 c;

    c = p->glyph;
    if (((s32) c >= 0) && ((s32) c < 5)) {
        return 1;
    }
    if (((s32) c >= 0x10) && ((s32) c < 0x15)) {
        return 1;
    }
    return 0;
}

s32 Kbd_CharInRowsHigh(KbdKey *p)
{
    s32 c;

    c = p->glyph;
    if (((s32) c >= 0x20) && ((s32) c < 0x25)) {
        return 1;
    }
    if (((s32) c >= 0x30) && ((s32) c < 0x35)) {
        return 1;
    }
    if (((s32) c >= 0x40) && ((s32) c < 0x45)) {
        return 1;
    }
    if (((s32) c >= 0x50) && ((s32) c < 0x55)) {
        return 1;
    }
    if (((s32) c >= 0x60) && ((s32) c < 0x65)) {
        return 1;
    }
    if (c == 0x70) {
        return 1;
    }
    return 0;
}

/* Whether the character code at p is a plain one (not 0xE2/0xC5/0xF0 nor one of Kbd_CharInRows01's). */
s32 Kbd_CharIsPlain(KbdKey *p)
{
    switch (p->glyph) {
    case 0xE2:
    case 0xC5:
    case 0xF0:
        return 0;
    }
    if (Kbd_CharInRows01(p) != 1) {
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
    va_list ap;

    if (gTextOff == 0) {
        va_start(ap, fmt);
        vsnprintf(buf, 0x3FF, fmt, ap);
        buf[0x3FF] = 0;
        Font_Printf(D_0046CC10, buf);
    }
}

void CamCtl_Clear(void)
{
    Mem_Fill(&gCamCtl, 0U, 0x680);
}

void CamReq_Reset(void)
{
    gCamCtl.request = 0;
    gCamCtl.requestArg = NULL;
    gCamCtl.follow.distNo = 2;
}

void CamCtl_LoadParams(CamCtl *c);

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


/* Keeps the loaded camera data h in gCamCtl.data and turns its offsets into pointers: each
 * area's faces and transition table (the areas from h + 0x30: 0x300 bytes, then faceCount
 * faces of 0x40 bytes, then 0x20 more bytes when trans is set), the cells' area lists and the
 * area pointers they list. */
void CamData_Relocate(CamDataHdr *h)
{
    CamArea *p;
    CamCell *pair;
    CamArea **q;
    s32 n;
    s32 sum;

    gCamCtl.data = h; /* the offsets in the file become pointers below */
    if (h == NULL) {
        return;
    }
    p = (CamArea *)(h + 1);
    h->areas = p;
    n = h->areaCount;
    if (n != 0) {
        do {
            if (p->trans != NULL) {
                p->faces = (CamFace *)((u8 *)p + (s32)p->faces);
                p->trans = (u8 *)p + (s32)p->trans;
                p = (CamArea *)((u8 *)((s32)(p + 1) + p->faceCount * 0x40) + 0x20);
            } else {
                p->faces = (CamFace *)((u8 *)p + (s32)p->faces);
                p = (CamArea *)((s32)(p + 1) + p->faceCount * 0x40);
            }
        } while (--n != 0);
    }
    pair = (CamCell *)((u8 *)h + (s32)h->cells);
    h->cells = pair;
    q = (CamArea **)((u8 *)h + (s32)h->areaLists);
    h->areaLists = q;
    sum = h->gridH;
    n = h->gridW * sum;
    sum = 0;
    if (n != 0) {
        do {
            if (pair->count != 0) {
                pair->areas = (CamArea **)((u8 *)h + (s32)pair->areas);
                sum += pair->count;
            }
            pair++;
        } while (--n != 0);
    }
    if (sum != 0) {
        do {
            *q = (CamArea *)((u8 *)h + (s32)*q);
            sum--;
            q++;
        } while (sum != 0);
    }
}

void CamCtl_ReadPad(void *c);
s32 Cam_FindPlayerArea(CamCtl *c);
s32 CamArea_TestObj(CamArea *area, Player *obj, u8 mask);
void CamCtl_RunMode(CamCtl *c);
void CamCtl_CheckEmInView(void);
void Camera_Update(void);

/* The camera script's frame (mode a): starts the script on the first frame
 * (its six entries through gCamModeInit), steps it, runs each entry's update
 * (gCamModeUpdate by kind) and the camera. */
void CamCtl_Update(s32 mode)
{
    CamCtl *c = (CamCtl *)&gCamCtl;
    CamView *e;
    Player *pl;
    u32 i;

    gCamCtl.mode = mode;
    gCamCtl.followView = 0;
    if ((pl = gCamCtl.player)->active == 0) {
        return;
    }
    CamCtl_ReadPad(c);
    c->unk3AC = 0;
    c->cellZ = 0;
    c->cellX = 0;
    c->areaChanged = 0;
    if (c->inited == 0) {
        c->inited++;
        c->area = NULL;
        c->prevAreaId = 0xFF;
        c->areaId = 0xFF;
        Cam_FindPlayerArea(c);
        c->areaId = c->area->id;
        c->areaMode = 0;
        if (c->area->kind == 0) {
            c->areaMode = 0;
        } else {
            c->areaMode = 2;
        }
        c->prevAreaMode = c->areaMode;
        e = c->modes;
        for (i = 0; i < 6; i++, e++) {
            e->kind = i;
            *(s32 *)&e->step = 0;
            gCamModeInit[i](c, e);
        }
    }
    if (c->areaId == 0) {
        if (Cam_FindPlayerArea(c) > 0) {
            c->prevAreaId = c->areaId;
            c->areaId = c->area->id;
            c->areaChanged = 1;
        }
    } else if (CamArea_TestObj(c->area, pl, 2) != 0) {
        c->prevAreaId = c->areaId;
        c->areaChanged = 1;
        if (Cam_FindPlayerArea(c) >= 0) {
            c->areaId = c->area->id;
        }
    }
    c->prevAreaMode = c->areaMode;
    switch (c->area->kind) {
    case 0:
        c->areaMode = 0;
        break;
    case 1:
    case 2:
    case 3:
        c->areaMode = 2;
        break;
    }
    e = c->modes;
    for (i = 6; i != 0; i--) {
        gCamModeUpdate[e->kind](c, e);
        e++;
    }
    CamCtl_RunMode(c);
    c->targetObj = 0;
    Camera_Update();
    if (c->followView != 0) {
        CamCtl_CheckEmInView();
    }
}

extern Camera *gCamera;
void CamScript_Update(CamCtl *c, CamView *v);
void Camera_SetRoll(f32 roll);
void Camera_StoreFov(f32 fov);
void Camera_Update(void);

/* Computes the view from the controller state and hands it to the camera. */
void CamCtl_Apply(void)
{
    CamView *v = &gCamCtl.modes[5];

    CamScript_Update(&gCamCtl, v);
    Vec_Copy(gCamera->eye, v->eye);
    Vec_Copy(gCamera->at, v->at);
    Camera_SetRoll(v->roll);
    Camera_StoreFov(v->fov);
    Camera_Update();
}

s32 CamCtl_GetMode(void)
{
    return gCamCtl.mode;
}

extern CamDistSet gCamMode1Defaults;
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/* Starts the camera behind the player (the work c->follow, the settings from the camera area). */
void CamFollow_Init(CamCtl *c, CamView *m)
{
    f32 in[4];
    f32 ofs[4];
    s32 ang[4];
    f32 mtx[16];
    CamFollow *w;
    Player *pl;

    pl = c->player;
    m->yawFrom = 0xFF;
    w = &c->follow;
    if (c->area->kind != 0) {
        w->set = &gCamMode1Defaults;
    } else {
        w->set = &c->area->follow;
    }
    w->dist = &w->set->dist[w->distNo];
    w->fov = w->fovGoal = w->set->fov;
    w->roll = w->set->roll;
    w->yaw = pl->rot[1] + 0x8000;
    ang[0] = 0;
    ang[1] = w->yaw;
    ang[2] = 0;
    Mtx_FromAnglesXYZ(ang, mtx);
    in[0] = 0.0f;
    in[1] = w->dist->back;
    in[2] = w->dist->up;
    Vec_Rotate(ofs, in, mtx);
    w->at[0] = pl->pos[0];
    w->at[1] = pl->pos[1] + w->dist->atY;
    w->at[2] = pl->pos[2];
    w->eye[0] = w->at[0] + ofs[0];
    w->eye[1] = pl->pos[1] + ofs[1];
    w->eye[2] = w->at[2] + ofs[2];
    w->yawGoal = w->yaw;
    Vec_Copy(w->eyeGoal, w->eye);
    Vec_Copy(w->atGoal, w->at);
    w->floorY = AreaGrid2_GetFloorY(w->at);
    w->unk54 = -1;
    w->unk79 = 0;
    Vec_Copy(w->unk18, w->eye);
    *(s32 *)&w->hitDist = 0;
    w->unk58 = 0;
    w->unk60 = 0;
    w->unk73 = 0;
    w->heightState = 0;
    w->heightTimer = 0;
    w->unk75 = 0;
    w->blockedTimer = 0;
    w->turningBehind = 0;
    w->unk78 = 0;
    w->unk76 = 0;
    w->followDelay = 3;
}

typedef struct Vec3 {
    f32 x, y, z;
} Vec3;

extern CamDist D_0041C970;
extern CamDist D_0041C930[2];
extern s16 D_0041CDC8;
extern f32 D_0041D2D0[3];
void CamFollow_UpdateCanFollow(CamFollow *w, Player *pl);
void CamFollow_GetKeys(CamCtl *c, CamFollow *o);
void CamFollow_UpdateBlocked(CamCtl *c, CamFollow *m);
s32 CamFollow_PlayerHeightKind(CamFollow *m);
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
/* unit's own declaration: with the definition's s16 (void *obj, u8 state, u8 sub) (sub_main/unk_3A1070.c), CamFollow_Update: 609 of 1014 words differ */
s32 Player_IsState(void *obj, s32 a, s32 b);
f32 Math_Sqrtf(f32 x);
s32 AreaGrid_SweepSphere(f32 *from, f32 *to, f32 *hit, u16 flags, f32 radius);

void CamView_SetTransition(CamCtl *p, CamView *t, CamArea *area);
/* The follow camera's update handler (camera mode 1): picks the distance setting (L2/R2
 * steps it), turns toward the player's back or by the pad, keeps the eye above the floor
 * and out of walls, and blends the view in. */
void CamFollow_Update(CamCtl *c, CamView *v)
{
    Vec3 off;
    Vec3 goal;
    s32 angles[3];
    Vec3 base;
    Vec3 hit;
    Vec3 at1;
    Vec3 pos;
    Vec3 at2;
    Vec3 hit2;
    Vec3 e;
    f32 r;
    f32 mtx[16];
    CamFollow *w;
    Player *pl;
    CamArea *area;
    u16 mode;
    u8 keep;
    u16 old;
    s32 i;
    s32 best;
    f32 bestD;
    f32 t;
    f32 lim;
    f32 floorY;
    f32 dx;
    f32 gd;
    f32 dd;
    f32 len;
    f32 gx;
    Vec3 d;
    f32 r2;
    f32 ax;
    f32 az;
    f32 l;
    f32 dz;
    f32 x2;
    f32 z2;
    f32 s2;
    f32 h2;
    f32 gz;
    s16 step;
    s32 k;
    f32 *p;
    CamDist *dp;
    s32 diff;
    u32 dy;
    s32 hitA;

    if (c->areaMode != 0) {
        v->active = 0;
        v->step = 0;
        v->blend = 0;
        return;
    }
    w = &c->follow;
    v->active = 1;
    c->followView = 1;
    c->playerFade = 1.0f;
    pl = c->player;
    area = c->area;
    CamFollow_UpdateCanFollow(w, pl);
    CamFollow_GetKeys(c, w);
    Vec_Copy(w->unk18, w->eye);
    w->fovGoal = w->set->fov;
    if (area->kind != 0) {
        w->set = &gCamMode1Defaults;
    } else {
        w->set = &area->follow;
    }
    if (CamFollow_PlayerHeightKind(w) == 1) {
        mode = 7;
        w->dist = &D_0041C970;
    } else if (w->blockedTimer != 0) {
        mode = 6;
        w->dist = &D_0041C930[0];
        w->fovGoal = 1.2217306f;
    } else {
        old = w->distNo;
        if (gScene.camControl == 0) {
            if ((w->pressed & 0x1000) && w->distNo > 0) {
                w->distNo--;
            }
            if ((w->pressed & 0x2000) && w->distNo < 3) {
                w->distNo++;
            }
        } else {
            if ((w->pressed & 0x2000) && w->distNo > 0) {
                w->distNo--;
            }
            if ((w->pressed & 0x1000) && w->distNo < 3) {
                w->distNo++;
            }
        }
        if (old != w->distNo) {
            w->unk76 = 8;
        }
        if (pl->action == 1 && w->distNo == 2) {
            mode = 4;
            w->dist = &w->set->dist[4];
        } else {
            w->dist = &w->set->dist[w->distNo];
            mode = w->distNo;
        }
    }
    keep = 0;
    if (c->targetObj != NULL) {
        w->wallHit = 0;
        w->yawGoal = pl->targetAngle + 0x8000;
    } else if ((k = v->yawFrom) != 0xFF) {
        v->yawFrom = 0xFF;
        p = c->modes[k].eye;
        w->yaw = w->yawGoal = Vec_AngleXZ16(p, p + 3);
    } else if (c->prevAreaMode != 0 && c->prevAreaMode != 1) {
        w->yaw = w->yawGoal = Vec_AngleXZ16(c->out.eye, c->out.at);
    } else if (w->held & 0xC00) {
        if (gScene.camControl != 2) {
            step = 0x47E;
        } else {
            step = -0x47E;
        }
        if (w->held & 0x800) {
            w->yawGoal += step;
        }
        if (w->held & 0x400) {
            w->yawGoal -= step;
        }
        w->unk76 = 8;
    } else if (CamFollow_ShouldTurnBehind(pl, w) != 0) {
        w->yawGoal = pl->rot[1] + 0x8000;
        w->turningBehind = 1;
        w->unk76 = 0x14;
        keep = w->unk78;
    } else if (w->heightState == 0 && w->canFollow >= 0 && gScene.camFollow == 0) {
        diff = (pl->rot[1] ^ 0x8000) - w->yawGoal & 0xFFFF;
        if (w->canFollow == 0) {
            w->yawGoal = w->yawGoal + Angle_Bump(diff, 4096.0f, 26624.0f, 0.00021701389f);
        } else {
            w->yawGoal = w->yawGoal + Angle_Bump(diff, 0.0f, 26624.0f, 0.00021701389f);
        }
        keep = 1;
    } else if (w->held & 8) {
        if (Player_TestFlag(pl, 8) != 0) {
            w->yawGoal = w->yawGoal + Angle_Bump((pl->rot[1] ^ 0x8000) - w->yawGoal & 0xFFFF, 4096.0f, 26624.0f, 0.00013563369f);
        }
    } else if (w->wallHit != 0) {
        w->yawGoal += (s16)((s16)((pl->rot[1] ^ 0x8000) - w->yawGoal) >> 3);
        w->wallHit = 0;
    }
    w->unk78 = keep;
    w->yaw += (s16)((s16)(w->yawGoal - w->yaw) >> 2);
    if (w->turningBehind != 0 && ((dy = (u16)(w->yawGoal - w->yaw)) < 0x1000 || dy > 0xF000)) {
        w->turningBehind = 0;
    }
    if (c->prevAreaMode == 1 && w->unk78 == 0) {
        CamView_StartBlend(v, D_0041CDC8);
        Vec_Diff(&off.x, c->modes[1].eye, pl->pos);
        dp = &w->set->dist[1];
        bestD = 1e8f;
        best = 0;
        for (i = 1; i < 4; i++, dp++) {
            t = dp->back - off.y;
            if (bestD > t * t) {
                bestD = t * t;
                best = i;
            }
        }
        w->distNo = best;
        mode = (u8)best;
        w->dist = &w->set->dist[best];
    }
    off.x = 0.0f;
    if (pl->rideState != 3 && pl->rideState != 0) {
        off.y = 400.0f;
        off.z = 400.0f;
        w->atGoal[1] = 100.0f + pl->pos[1];
    } else if (pl->rideState != 0 || Player_IsState(pl, 0, 0x11) != 0 ||
               Player_IsState(pl, 0, 0x1B) != 0 || Player_IsState(pl, 0, 0x1E) != 0 ||
               Player_IsState(pl, 0, 0x3A) != 0) {
        off.y = 20.0f;
        off.z = w->dist->up;
        w->atGoal[1] = 30.0f + pl->pos[1];
    } else {
        off.y = w->dist->back;
        off.z = w->dist->up;
        /* An empty check of the setting's target height (from 0 to the eye height), compiled
         * out (docs/mwcc-matching.md, "Statements scheduled as if a check were still there"): without
         * it the two loads of the add.s come in the other order (f0/f1 swapped). */
        if (w->dist->atY < 0.0f || w->dist->atY > w->dist->up) { /* compiled-out check */
        }
        w->atGoal[1] = pl->pos[1] + w->dist->atY;
    }
    w->atGoal[0] = pl->pos[0];
    w->atGoal[2] = pl->pos[2];
    angles[2] = 0;
    angles[0] = 0;
    angles[1] = w->yaw;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(&goal.x, &off.x, mtx);
    w->eyeGoal[0] = w->atGoal[0] + goal.x;
    w->eyeGoal[1] = pl->pos[1] + goal.y;
    w->eyeGoal[2] = w->atGoal[2] + goal.z;
    if (v->step == 0) {
        v->step++;
        Vec_Copy(w->eye, w->eyeGoal);
        Vec_Copy(w->at, w->atGoal);
        w->fov = w->fovGoal;
    }
    w->at[0] = w->atGoal[0];
    w->at[2] = w->atGoal[2];
    w->at[1] += 0.5f * (w->atGoal[1] - w->at[1]);
    if (CamFollow_IsBlownAway() == 1) {
        Vec_Add(&base.x, pl->pos, D_0041D2D0);
        if (AreaGrid_SweepSphere(&base.x, w->at, &off.x, 4, 33.0f) != 0) {
            Vec_Copy(w->at, &off.x);
        }
    }
    w->fov += 0.05f * (w->fovGoal - w->fov);
    lim = 100.0f;
    floorY = AreaGrid2_GetFloorY(w->atGoal);
    if (-(floorY - w->floorY) < 210.0f) {
        w->unk54 = 0;
        w->floorY = floorY;
        if (w->unk79 != 0) {
            lim = 256.0f;
            w->unk79 = 0;
        }
    } else {
        switch (w->unk54) {
        case 0:
            w->unk54 = 1;
        case 1:
            lim = 256.0f;
            break;
        default:
            w->unk54 = 0;
            w->floorY = floorY;
            break;
        }
        w->unk79 = 0;
    }
    floorY = CamFollow_FloorY(w->eyeGoal, pl);
    if (floorY - w->eyeGoal[1] < lim) {
        floorY += w->dist->floorGap;
        if (floorY > w->eyeGoal[1]) {
            w->eyeGoal[1] = floorY;
            w->unk79 = 1;
        } else {
            w->unk54 = -1;
        }
    }
    if (w->heightState != 0) {
        w->unk75 = 0;
        w->eye[1] += 0.25f * (w->eyeGoal[1] - w->eye[1]);
        w->eye[0] = w->eyeGoal[0];
        w->eye[2] = w->eyeGoal[2];
    } else if (w->unk75 == 0) {
        hitA = AreaGrid_SweepSphere(w->eye, w->eyeGoal, &hit.x, 0x8009, 28.0f);
        w->wallHit = AreaGrid_SweepSphere(w->at, w->eyeGoal, &hit.x, 0x8009, 28.0f);
        if (hitA != 0 && w->wallHit != 0) {
            w->eye[0] = hit.x;
            w->eye[1] = hit.y;
            w->eye[2] = hit.z;
            w->unk75 = 1;
            w->unk76 = 8;
        } else {
            w->eye[1] += 0.125f * (w->eyeGoal[1] - w->eye[1]);
            w->eye[0] = w->eyeGoal[0];
            w->eye[2] = w->eyeGoal[2];
        }
    } else {
        Vec_Copy(&at1.x, w->at);
        if (w->eyeGoal[1] < at1.y) {
            at1.y = 0.5f * (at1.y + w->eyeGoal[1]);
        }
        if (AreaGrid_SweepSphere(&at1.x, w->eyeGoal, &hit.x, 0x8009, 28.0f) != 0) {
            if (w->unk76 != 0) {
                w->eye[0] = hit.x;
                w->eye[1] = hit.y;
                w->eye[2] = hit.z;
                w->unk18[0] = hit.x;
                w->unk18[1] = hit.y;
                w->unk18[2] = hit.z;
            } else {
                r = w->dist->up;
                dx = w->eye[0] - w->at[0];
                dz = w->eye[2] - w->at[2];
                dd = dx * dx + dz * dz;
                if (dd > r * r) {
                    t = r / Math_Sqrtf(dd);
                    pos.x = w->at[0] + dx * t;
                    pos.y = w->eye[1];
                    pos.z = w->at[2] + dz * t;
                    w->wallHit = AreaGrid_SweepSphere(w->at, &pos.x, &hit.x, 0x8009, 28.0f);
                    if (w->wallHit != 0) {
                        Vec_Copy(w->eye, &hit.x);
                    } else {
                        Vec_Copy(w->eye, &pos.x);
                    }
                }
                if (w->unk78 == 0) {
                    w->yawGoal = Vec_AngleXZ16(w->eye, w->at);
                }
            }
        } else {
            w->unk75 = 0;
        }
    }
    if (w->unk76 != 0) {
        w->unk76--;
    }
    if (w->unk75 == 0) {
        if (w->heightState == 0) {
            Cam_PushOutOfEms(w->eye, w->unk18);
            Cam_LineHitDist(w->eye, w->unk18, &w->hitDist);
        } else {
            at2.x = w->at[0];
            at2.y = w->eye[1];
            at2.z = w->at[2];
            if (AreaGrid_SweepSphere(&at2.x, w->eye, &hit2.x, 0x8009, 28.0f) != 0) {
                Vec_Copy(w->eye, &hit2.x);
                w->yawGoal += 0x400;
            }
        }
    }
    if (AreaGrid_SweepSphere(w->eye, w->eyeGoal, &e.x, 0x8009, 28.0f) != 0) {
        Vec_Diff(&e.x, w->eye, w->at);
        r2 = 150.0f + w->dist->up;
        if (e.x * e.x + e.z * e.z > r2 * r2) {
            Vec_Copy(w->eye, w->eyeGoal);
            Vec_Copy(w->unk18, w->atGoal);
            Cam_PushOutOfEms(w->eye, w->unk18);
            Cam_LineHitDist(w->eye, w->unk18, &w->hitDist);
        }
    }
    if (w->unk75 != 0 && mode > 0 && mode < 5) {
        gx = w->atGoal[0] - w->eyeGoal[0];
        gz = w->atGoal[2] - w->eyeGoal[2];
        gd = gx * gx + gz * gz;
        ax = w->at[0] - w->eye[0];
        az = w->at[2] - w->eye[2];
        l = ax * ax + az * az;
        if (gd > l) {
            l = Math_Sqrtf(l);
            if (l > 0.0f) {
                t = Math_Sqrtf(gd) / l;
                w->at[0] = w->eye[0] + ax * t;
                w->at[2] = w->eye[2] + az * t;
            }
        }
    }
    floorY = CamFollow_FloorY(w->eye, pl);
    floorY += w->dist->floorGap;
    if (floorY > w->eye[1]) {
        w->eye[1] = floorY;
    }
    if (w->eye[1] < w->at[1]) {
        Vec_Diff(&d.x, w->at, w->eye);
        x2 = d.x * d.x;
        z2 = d.z * d.z;
        s2 = x2 + d.y * d.y + z2;
        if (s2 > 0.0f) {
            h2 = x2 + z2;
            if (0.75f > h2 / s2) {
                h2 = Math_Sqrtf(h2);
                h2 *= 0.57735f;
                w->at[1] = w->eye[1] + h2;
            }
        }
    }
    CamView_SetTransition(c, v, area);
    CamFollow_UpdateBlocked(c, w);
    Vec_Copy(v->eye, w->eye);
    Vec_Copy(v->at, w->at);
    switch (v->blend) {
    case 0:
        v->roll = w->roll;
        v->fov = w->fov;
        break;
    case 1:
        Vec_Copy(v->eye0, c->out.eye);
        Vec_Copy(v->at0, c->out.at);
        v->roll0 = c->out.roll;
        v->fov0 = c->out.fov;
        v->blend++;
    case 2:
        t = CamBlend_EaseStep(v);
        Vec_Lerp(v->eye, v->eye0, v->eye, t);
        Vec_Lerp(v->at, v->at0, v->at, t);
        v->roll = v->roll0 * t + w->roll * (1.0f - t);
        v->fov = v->fov0 * t + w->fov * (1.0f - t);
        break;
    }
    CamView_KeepTargetOnScreen(c, v);
}


s32 CamCtl_IsTurningBehind(void)
{
    CamCtl *c = (CamCtl *)&gCamCtl;

    if (gCamCtl.override >= 0) {
        return 0;
    }
    if (c->areaMode != 0) {
        return 0;
    }
    return c->follow.turningBehind != 0;
}

void CamCtl_SetTargetObj(u8 *o)
{
    CamCtl *c = (CamCtl *)&gCamCtl;

    if (*(u16 *)(o + 0xC) == gScene.localPlayer) {
        c->targetObj = o;
    }
}


void CamCtl_TurnToObj_Unk(u8 *o)
{
    if (*(u16 *)(o + 0xC) == gScene.localPlayer) {
        CamFollow *c = &gCamCtl.follow;

        c->wallHit = 0;
        c->yawGoal = *(s32 *)(o + 0xA4) + 0x2000;
        c->distNo = 2;
    }
}

s32 Cam_PlayerFree(Player *pl);
s32 func_003ACE10(struct Player *p);

/* The camera's pad keys given to the follow camera o, filtered by the player state. */
void CamFollow_GetKeys(CamCtl *c, CamFollow *o)
{
    Player *pl;

    if (c->areaMode == 0 && c->modes[5].active == 0) {
        pl = c->player;
        if (Cam_PlayerFree(pl) == 1) {
            o->held = c->padHeld;
            o->pressed = c->padPressed;
            if (func_003ACE10(pl) == 1 && (pl->input.held & 8)) {
                o->held &= 0xC3FF;
                o->pressed &= 0xC3FF;
            }
            return;
        }
        if ((s8)Kbd_IsOpen() == 0) {
            if (pl->weaponType == 0xA && pl->weaponOut != 0 && pl->stateGroup != 2 && pl->stateGroup != 6) {
                o->held = c->padHeld & 0x88;
                o->pressed = c->padPressed & 0x88;
                return;
            }
            o->held = c->padHeld & 8;
            o->pressed = c->padPressed & 8;
            return;        }
    }
    o->pressed = 0;
    o->held = 0;
}



void CamFollow_SetYawFrom(s32 v)
{
    CamView *w = &gCamCtl.modes[0];

    if (v == 5) {
        v = -1;
    }
    if (v >= 0) {
        w->yawFrom = v;
    } else {
        w->yawFrom = -1;
    }
}

s32 AreaGrid_SweepSphere(f32 *from, f32 *to, f32 *hit, u16 flags, f32 radius);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
f32 Math_Atan2(f32 y, f32 x);

void CamFollow_UpdateBlocked(CamCtl *c, CamFollow *m)
{
    f32 d[3];
    Player *pl;

    if (m->blockedTimer == 0xF) {
        if (CamFollow_IsBlownAway() == 1) {
            return;
        }
        m->blockedTimer--;
        return;
    }
    if (m->blockedTimer != 0) {
        m->blockedTimer--;
        if (m->pressed & 0x3000) {
            m->blockedTimer = 0;
        }
    }
    if (CamFollow_IsBlownAway() == 1 && AreaGrid_SweepSphere(m->atGoal, m->eyeGoal, d, 0x8009, 28.0f) != 0) {
        d[0] -= m->eyeGoal[0];
        d[2] -= m->eyeGoal[2];
        if (d[0] * d[0] + d[2] * d[2] > 2500.0f) {
            pl = c->player;
            m->blockedTimer = 0xF;
            m->distNo = 1;
            Vec_Diff(d, pl->prevPos, pl->pos);
            if (m->unk76 == 0) {
                m->yawGoal = 10430.378f * Math_Atan2(d[0], d[2]);
            }
        }
    }
}

s32 CamFollow_PlayerHeightKind(CamFollow *m)
{
    Player *pl = gCamCtl.player;
    u8 kind;
    f32 ground;

    if (pl->stateGroup == 2) {
        m->heightState = 0;
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
    switch (m->heightState) {
    case 0:
        if (kind <= 2) {
            m->heightTimer = 4;
            break;
        }
        m->heightTimer -= 1;
        if (m->heightTimer > 0) {
            break;
        }
        m->heightState++;
    case 1:
        if (kind & 1) {
            m->heightTimer = 15;
            return 1;
        }
        m->heightTimer -= 1;
        if (m->heightTimer > 0) {
            return 1;
        }
        m->heightState = 0;
        break;
    }
    return 0;
}

/* The ground height at pos; when it is below obj->0xB0, the height seen from 150 higher. */
f32 CamFollow_FloorY(f32 *pos, Player *obj)
{
    f32 v[3];
    f32 h = AreaGrid2_GetFloorY(pos);

    if (obj->pos[1] <= h) {
        return h;
    }
    Vec_Copy(v, pos);
    v[1] += 150.0f;
    return AreaGrid2_GetFloorY(v);
}

s32 Cam_PlayerCanFollow(Player *pl);
s32 Player_InBlownAway(struct Player *p);

void CamFollow_UpdateCanFollow(CamFollow *w, Player *pl)
{
    s8 v = Cam_PlayerCanFollow(pl);

    if (v >= 0) {
        if (w->canFollow < 0) {
            if (pl->weaponType == 0xA) {
                w->followDelay -= 1;
            } else if (v == 1) {
                w->followDelay -= 1;
            }
            if (w->followDelay > 0) {
                v = -1;
            }
        }
    } else {
        w->followDelay = 3;
    }
    w->canFollow = v;
    w->blownAway = 0;
    if (gScene.online == 0 && Player_InBlownAway(pl) == 1) {
        w->blownAway = 1;
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

void CamOrbit_Init(CamCtl *c, CamView *v)
{
}

void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
f32 CamBlend_EaseStep(CamView *v);
extern f32 D_0041CDD0[3];
extern f32 D_0041CDE0[3];

/*
 * The orbit camera's update handler (mode 1, its work c->orbit): while the follow view is
 * active and the follow camera's unk78 holds, it turns its pitch toward the player's
 * aimWeight and its yaw (+-0x800 by where the player faces) around the player, raised by
 * 170 (or blended to the follow camera's distance setting), keeps the eye out of walls
 * (AreaGrid_SweepSphere) and above the ground, and moves the target onto the plane through it.
 * NON_MATCHING: 24 words: the scheduling of the line-plane intersection's madd chains (the
 * game issues the denominator's division before the numerator's madd) and the operand
 * order of the ground-height add.
 */
void CamOrbit_Update(CamCtl *c, CamView *v)
{
    f32 pos[3];
    f32 r[3];
    f32 off[3];
    f32 off2[3];
    s32 ang[3];
    f32 hit[3];
    f32 mtx[16];
    f32 ab[2][3];
    Player *pl;
    CamOrbit *w;
    CamFollow *m;
    f32 y;
    f32 t;
    f32 px;
    f32 pz;
    f32 nx;
    f32 nz;
    f32 ny;
    f32 py;
    f32 d0;
    f32 d1;
    f32 d2;
    f32 h;
    u32 dd;

    if (c->modes[0].active == 0) {
        v->active = 0;
        *(u16 *)&v->step = 0;
        return;
    }
    pl = c->player;
    m = &c->follow;
    w = &c->orbit;
    switch (v->step) {
    case 0:
        if (m->unk78 == 0) {
            v->active = 0;
            *(u16 *)&v->step = 0;
            return;
        }
        Vec_Copy(w->unk18, c->modes[0].eye);
        w->pitch = (pl->aimWeight + 100) * 34 - 0xB00;
        w->dir = 0x800;
        w->yaw = 0;
        v->step++;
        v->blend = 1;
        v->left = 9;
        v->rate = 1.0f / v->left;
        break;
    case 1:
        w->pitch += (s16)((s16)((pl->aimWeight + 100) * 34 - 0xB00 - w->pitch) >> 2);
        if (m->unk78 == 0) {
            v->active = 0;
            *(u16 *)&v->step = 0;
            return;
        }
        break;
    }
    v->active = 1;
    c->areaMode = 1;
    c->followView = 0;
    dd = ((pl->rot[1] ^ 0x8000) - (m->yaw + w->yaw)) & 0xFFFF;
    if (dd > 0x100 && dd < 0x7800) {
        w->dir = -0x800;
    } else if (dd > 0x8800 && dd < 0xFF00) {
        w->dir = 0x800;
    }
    if (w->dir < 0) {
        if (w->yaw > -0x800) {
            w->yaw += (s16)((s16)(-0x800 - w->yaw) * 6 >> 6);
        }
    } else if (w->yaw < 0x800) {
        w->yaw += (s16)((s16)(0x800 - w->yaw) * 6 >> 6);
    }
    switch (v->blend) {
    case 0:
        Vec_Copy(off, D_0041CDD0);
        Vec_Copy(off2, D_0041CDE0);
        y = 170.0f;
        break;
    case 1:
        (v->blend)++;
    case 2:
        t = CamBlend_EaseStep(v);
        off[0] = 0.0f;
        off[1] = 0.0f;
        off[2] = m->dist->up;
        Vec_Lerp(off, off, D_0041CDD0, t);
        off2[2] = 0.0f;
        off2[1] = 0.0f;
        off2[0] = 0.0f;
        Vec_Lerp(off2, off2, D_0041CDE0, t);
        y = m->dist->atY * t + 170.0f * (1.0f - t);
        break;
    }
    w->at[0] = pos[0] = pl->pos[0];
    w->at[1] = pos[1] = pl->pos[1] + y;
    w->at[2] = pos[2] = pl->pos[2];
    ang[0] = w->pitch;
    ang[1] = m->yaw + w->yaw;
    ang[2] = 0;
    Mtx_FromAnglesXYZ(ang, mtx);
    Vec_Rotate(r, off, mtx);
    Vec_Add(w->eye, w->at, r);
    if (AreaGrid_SweepSphere(pos, w->eye, hit, 0x8009, 28.0f) != 0) {
        Vec_Copy(w->eye, hit);
    }
    h = CamFollow_FloorY(w->eye, pl);
    h += m->dist->floorGap;
    if (h > w->eye[1]) {
        w->eye[1] = h;
    }
    ang[1] = m->yaw;
    Mtx_FromAnglesXYZ(ang, mtx);
    Vec_Rotate(r, off2, mtx);
    px = w->at[0];
    pz = w->at[2];
    nx = r[0];
    nz = r[2];
    ny = 0.0f;
    py = 0.0f;
    Vec_Copy(ab[0], w->eye);
    Vec_Add(ab[1], w->at, r);
    d0 = ab[1][0] - ab[0][0];
    d1 = ab[1][1] - ab[0][1];
    d2 = ab[1][2] - ab[0][2];
    t = 1.0f / (d0 * nx + d1 * ny + d2 * nz);
    t = (nx * (px - ab[1][0]) + ny * (py - ab[1][1]) + nz * (pz - ab[1][2])) * t;
    ab[1][0] += d0 * t;
    ab[1][1] += d1 * t;
    ab[1][2] += d2 * t;
    Vec_Copy(w->at, ab[1]);
    v->roll = m->roll;
    v->fov = m->fov;
    Vec_Copy(v->eye, w->eye);
    Vec_Copy(v->at, w->at);
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
u32 gQuestListEntries[96] = {
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
