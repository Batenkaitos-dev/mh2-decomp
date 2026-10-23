#ifndef CHAT_PHRASES_H
#define CHAT_PHRASES_H

#include "common.h"
#include "sprite.h"

/*
 * The chat's word filter and phrases: masking the filtered words of a chat
 * line (Chat_FilterWords), the phrase list and its short names
 * (Phrase_ResetDefaults, ChatPhrase_Set) with the keyboard that edits a
 * phrase (PhraseMenu_OpenKbd, PhraseMenu_KbdUpdate), and cutting a Shift-JIS
 * string to a width (Sjis_Truncate, Text_DrawTruncated). Also two of the
 * HUD's helpers: the pouch bar's end caps and the ammo icon's sprite.
 */

void Chat_FilterWords(char *text);
s32 Chat_MaskWord(char *text, char *word, s8 mode);
s32 Sjis_IsSymbolOrDigit(u8 *sjis);
s32 Chat_FilterIfNotBlank(u8 *text);
void Phrase_ResetDefaults(void);
void Phrase_TruncateNames(void);
void ChatPhrase_Set(char *text, u8 index);
s32 PhraseMenu_OpenKbd(u8 index);
s8 PhraseMenu_KbdUpdate(u8 index);
void Text_DrawTruncated(u8 n, char *src);
void Sjis_Truncate(char *dst, u8 n, char *src);
void Hud_DrawPouchBarEnds(s16 x, s16 y, s16 height);
void Hud_AmmoIconSprite(u8 *equip, Sprite *spr);

#endif
