/*
 * The chat's word filter and phrases: masking the filtered words of a chat line
 * (Chat_FilterWords, Chat_MaskWord), the phrase list (SysSave.phrases and their short names,
 * gPhraseEntries) and its keyboard (PhraseMenu_OpenKbd, PhraseMenu_KbdUpdate), and cutting a
 * Shift-JIS string to a width (Text_DrawTruncated, Sjis_Truncate).
 *
 * The file ends with two helpers of the HUD (the pouch bar's ends, the ammo icon's sprite). None
 * of these functions uses data of its own.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "player.h"
#include "pad.h"
#include "prim.h"
#include "syssave.h"
#include "save.h"
#include "hud.h"
#include "sound.h"
#include "texture.h"
#include "render_state.h"
#include "area_draw.h"
#include "kbd.h"
#include "model.h"
#include "text.h"
#include "window.h"
#include "chat_phrases.h"
#include "game_util.h"
/* The whole file is compiled at optimization level 4, as the game's code was: its functions
 * match only so. */
#pragma optimization_level 4

extern char gDefaultPhrases[][45];
void *memcpy(void *dst, const void *src, u32 n);
s8 Kbd_Run(char *buf, s16 held, s16 pressed);
char *strstr(char *s, char *w);
s32 strlen(char *s);

/* Masks the filtered words in a chat text: list A wherever found, list B
 * only as whole words (Chat_MaskWord's modes 0 and 2). */
void Chat_FilterWords(char *text)
{
    char **word = gChatNgWordsA;

    do {
        Chat_MaskWord(text, *word, 0);
        word++;
    } while (*word != 0);
    word = gChatNgWordsB;
    do {
        Chat_MaskWord(text, *word, 2);
        word++;
    } while (*word != 0);
}

/* Replaces each match of word in text by full-width asterisks (the Shift-JIS character
 * 0x8196, one per two bytes). Modes 0 and 1 mask every match; mode 2 only
 * one that starts the text or follows a symbol or digit, or ends the text
 * or is followed by one (Sjis_IsSymbolOrDigit). Returns 0. */
s32 Chat_MaskWord(char *text, char *word, s8 mode)
{
    s32 mask;
    s32 len;
    char *from = text;
    char *hit;
    char *end;

    while ((hit = strstr(from, word)) != NULL) {
        switch (mode) {
        case 0:
            len = strlen(word);
            mask = 1;
            break;
        case 1:
            len = strlen(word);
            mask = 1;
            break;
        case 2:
            mask = 0;
            len = strlen(word);
            if (hit == text) {
                mask = 1;
            } else if (Sjis_IsSymbolOrDigit((u8 *)hit - 2) == 1) {
                mask = 1;
            } else {
                end = from + strlen(from);
                if (hit + len == end) {
                    mask = 1;
                } else if (Sjis_IsSymbolOrDigit((u8 *)hit + len) == 1) {
                    mask = 1;
                }
            }
            break;
        }
        from = hit + 1;
        if (mask) {
            for (len >>= 1; len > 0; len--) {
                hit[0] = 129;
                hit[1] = 150;
                hit += 2;
            }
            from = hit;
        }
    }
    return 0;
}

/* Whether the Shift-JIS character at sjis is a full-width symbol (lead
 * byte 0x81) or a full-width digit (0x824F to 0x8258). */
s32 Sjis_IsSymbolOrDigit(u8 *sjis)
{
    if (sjis[0] == 129 && sjis[1] >= 64 && sjis[1] < 237) {
        return 1;
    }
    if (sjis[0] == 130 && sjis[1] >= 79 && sjis[1] < 89) {
        return 1;
    }
    return 0;
}

/* Filters a chat text (Chat_FilterWords) unless it holds only spaces
 * (half-width or full-width); returns 1 if it held anything else. */
s32 Chat_FilterIfNotBlank(u8 *text)
{
    u8 *cursor = text;
    u8 byte = *cursor;

    while (byte != 0) {
        if (byte == 32) {
            cursor++;
        } else if (byte == 129 && cursor[1] == 64) {
            cursor += 2;
        } else {
            Chat_FilterWords((char *)text);
            return 1;
        }
        byte = *cursor;
    }
    return 0;
}

/* Puts the default chat phrases back (gDefaultPhrases) and remakes their
 * names. */
void Phrase_ResetDefaults(void)
{
    memcpy(gSysSave.phrases[0], gDefaultPhrases, sizeof(gSysSave.phrases));
    Phrase_TruncateNames();
}

/* Makes each phrase's name: the phrase cut to PHRASE_NAME_LEN bytes. */
void Phrase_TruncateNames(void)
{
    PhraseEntry *entry = gPhraseEntries;
    s32 n = PHRASE_COUNT;

    do {
        Sjis_Truncate(entry->name, PHRASE_NAME_LEN, entry->text);
        entry++;
    } while (--n);
}

/* Sets phrase index to text (at most PHRASE_LEN - 1 bytes) and remakes its
 * name. */
void ChatPhrase_Set(char *text, u8 index)
{
    PhraseEntry *entry;
    s32 n;
    char *src;
    char *dst;

    src = text;
    entry = &gPhraseEntries[index];
    n = PHRASE_LEN - 1;
    dst = entry->text;

    do {
        if (*src == 0) {
            break;
        }
        *dst = *src;
        n--;
        src++;
        dst++;
    } while (n > 0);
    *dst = 0;
    Sjis_Truncate(entry->name, PHRASE_NAME_LEN, text);
}

/* Opens the software keyboard on phrase index to edit it, when the field
 * control allows; returns 1 if it opened. */
s32 PhraseMenu_OpenKbd(u8 index)
{
    if (Field_ControlEnabled() == 0) {
        return 0;
    }
    Kbd_SetPos(80, 80.0f);
    Chat_OpenKbd(2, 14, PHRASE_LEN - 1, gPhraseEntries[index].text);
    Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    return 1;
}

/* Runs the phrase keyboard for phrase index: PAD_SELECT cancels; an
 * entered text replaces the phrase. Returns 1 once the keyboard closed,
 * else 0. */
s8 PhraseMenu_KbdUpdate(u8 index)
{
    char buf[48];
    s8 result;

    if (gPadState[0].pressed & PAD_SELECT) {
        Kbd_Cancel();
        return 1;
    }
    buf[0] = 0;
    result = Kbd_Run(buf, gPadState[0].held, gPadState[0].pressed);
    if (result != 0) {
        if (result > 0) {
            ChatPhrase_Set(buf, index);
        }
        Kbd_Cancel();
        return 1;
    }
    return 0;
}

/* Draws src cut to n bytes (Sjis_Truncate). */
void Text_DrawTruncated(u8 n, char *src)
{
    char buf[48];

    Sjis_Truncate(buf, n, src);
    Text_Draw(buf);
}

/* Copies the Shift-JIS string src to dst cut at n bytes (a two-byte character is kept whole) and
 * ends it with half-width dots (0xA5) and a terminator. */
void Sjis_Truncate(char *dst, u8 n, char *src)
{
    char flags[48];
    s32 left = n + 1;
    char *start = dst;
    char *wide = flags;

    while (left > 0) {
        u8 lead;

        *dst = *src;
        lead = *dst;
        if ((lead >= 128 && lead <= 159) || (lead >= 224 && lead <= 255)) {
            dst[1] = src[1];
            left--;
            src += 2;
            wide[0] = 0;
            dst += 2;
            wide[1] = 1;
            wide += 2;
        } else {
            if (*src == 0) {
                return;
            }
            wide[0] = 0;
            dst++;
            src++;
            wide++;
        }
        left--;
    }
    if (flags[n - 2] == 0) {
        start[n - 1] = 165;
        start[n - 2] = 165;
    } else {
        start[n - 1] = 165;
        start[n - 2] = 165;
        start[n - 3] = 165;
    }
    start[n] = 0;
}

/* Draws the two end caps of the pouch bar, height high, at x and x + 21. */
void Hud_DrawPouchBarEnds(s16 x, s16 y, s16 height)
{
    Sprite spr;

    spr.w = 5;
    spr.x = x;
    spr.color = -1;
    spr.y = y;
    spr.h = height;
    *(s32 *)&spr.u0 = 0x200074;
    *(s32 *)&spr.u1 = 0x340079;
    Sprite_Draw(&spr);
    spr.x = x + 21;
    *(s32 *)&spr.u0 = 0x200078;
    *(s32 *)&spr.u1 = 0x340073;
    Sprite_Draw(&spr);
}

/* Sets spr to the ammo icon for the byte at +2 of equip: one icon for 10,
 * another for the rest. */
void Hud_AmmoIconSprite(u8 *equip, Sprite *spr)
{
    if (equip[2] != 10) {
        spr->x = 424;
        spr->y = 253;
        spr->w = 11;
        spr->h = 20;
        *(s32 *)&spr->u0 = 0xAD0015;
        *(s32 *)&spr->u1 = 0xC10020;
    } else {
        spr->x = 432;
        spr->y = 254;
        spr->w = 16;
        spr->h = 20;
        *(s32 *)&spr->u0 = 0xAD0004;
        *(s32 *)&spr->u1 = 0xC10014;
    }
}


