/*
 * The software keyboard's input: the USB keyboard's keys (Kbd_ReadUsb,
 * Kbd_HandleKey and a handler per key), typing into the text or into the
 * kana being entered (Kbd_TypeHidKey, the romaji to kana table), the
 * cursors, and the kana-kanji conversion's segments and candidates
 * (gKbdCands, gKbdCandLines), and the drawing of the entry line. The
 * state is the KbdEntry at gKbd; the layouts are the key sets' tables
 * (gKbdSetLayouts).
 * The unit is built at optimization level 3, so that Kbd_ShiftFKey and
 * Kbd_CtrlFKey keep their key in the same register across Kbd_CtrlHeld
 * and Kbd_ShiftHeld, which Kbd_TypeHidKey calls too. Kbd_DrawCandidateBox
 * is a separate level-4 unit with its own copy of the "%s" format.
 */
#include "common.h"
#include "em.h"
#include "kbd.h"
#include "sound.h"
#include "text.h"

#include "ee_stdarg.h"
#include "sub_main/kbd_input.h"   /* the texts this file uses (tools/content_gen.py) */

/* prototypes */
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap); /* vsnprintf */
void Font_Printf(const char *fmt, ...);
extern s32 gTextOff;
extern char D_0046CC10[]; /* "%s" */
u8 *UsbKb_Poll(void);
char *strcat(char *dst, char *src); /* strcat */
s32 strcmp(char *a, char *b); /* strcmp */
char *strcpy(char *dst, const char *src); /* strcpy */
s32 strlen(char *s); /* strlen */
char *strncat(char *dst, const char *src, u32 n); /* strncat */
void strncpy(char *dst, const char *src, u32 n);
void Vec_Copy(f32 *dst, f32 *src);
s32 ConvCmd_PrevCand(char *a, char *b);
s32 ConvCmd_NextCand(char *a, char *b);
s32 ConvCmd_CommitAll(u8 *a);
s32 ConvCmd_CommitSegment(s32 a, u8 *b, char *c, char *d, s16 *e);
s32 ConvCmd_Open(void);
s32 ConvCmd_GrowSegment(char *a, char *b);
s32 ConvCmd_PrevSegment(char *a, char *b);
s32 ConvCmd_NextSegment(char *a, char *b);
s32 ConvCmd_SegmentForm2(char *a, char *b);
s32 ConvCmd_SegmentForm1(char *a, char *b);
void Kbd_TypeHidKey(void);
void Kbd_FKeyRow(u8 key);
void Kbd_OpenPhrases1(void);
void Kbd_OpenPhrases2(void);
void Kbd_CycleSegmentForm(s32 flag);
void Str_CutAt(char *s, s32 n);
void Kbd_MoveOffKey(s16 dir);
s32 Kbd_ArrowsOnPage(void);
void Kbd_CommitInput(void);
void Kbd_DeleteChar(void);
s32 Str_DropFront(char *s, s32 n);
s32 Kbd_RomajiToKana(s32 kind, char *s);
void Kbd_CommitKana(void);
s32 Str_InsertClip(char *dst, char *src, s32 pos, s32 max);
void Kbd_UpdateSymbolMode(void);
s32 CaseCands_CopyFirst(s8 *dst, u8 count);
s32 CaseCands_First(char *dst);
void Kbd_SkipSameCands(void);
s32 Sjis_CharSizeAt(char *s, s32 pos);
s32 Wide_ToUpper(char *dst, char *src);
s32 Wide_ToLower(char *dst, char *src);
s32 Wide_Capitalize(char *dst, char *src);
s32 Kbd_CountCands(void);
void Kbd_StartConvert(void);
void Kbd_Convert(void);
void Kbd_CommitConversion(void);
void Kbd_SetCandLine(s32 i);
void Kbd_SetSymbolCand(s32 i, s32 j);
void Kbd_TakeSymbolCand(void);
void Kbd_ListCandidates(s32 start, s32 dir);
void Kbd_InitCandList(void);
void Kbd_PrevCand(void);
void Kbd_NextCand(void);
void Kbd_NextSegment(void);
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
extern u8 gKbdDakutenSjis;
extern u8 gKbdDakutenSjis_1;
extern u8 gKbdHandakutenSjis;
extern u8 gKbdHandakutenSjis_1;
extern u8 gKbdSetModes[];
extern u8 gKbdCands[];
extern u8 gKbdCandText[];
extern u8 gKbdSegText[];
extern u8 gKbdCandLines[];
extern s8 gKbdCandLinesCut[];
extern char D_0046CC08[]; /* a full-width space */
s32 strncmp(const char *a, const char *b, u32 n);
extern s8 gCaseCands;
extern s8 gCaseCandCount;
extern u8 gKbdPhrasePage;
extern u8 gKbdSavedKanaInput;
extern u8 gKbdState;
extern s8 D_005AEAD7;
/* end of prototypes */

typedef s8 Str256[0x100]; /* a text buffer */


/* The size of a key cursor box. */
typedef struct BoxSize {
    f32 w;
    s16 h;
} BoxSize;
extern BoxSize gKbdBoxSizes[];


/* The cursor and grid tables of a layout variant. */
typedef struct KeyPair {
    KbdKeyPos *cursors;
    s8 *grid;
    s32 unk8;
} KeyPair;

/* This file's read-only data. */
#pragma rodata on
char D_0046A930[0x8] = {
#include "sub_main/0046A930.inc"
};
char D_0046A938[0x8] = {
#include "sub_main/0046A938.inc"
};
char D_0046A940[0x8] = {
#include "sub_main/0046A940.inc"
};
char D_0046A948[0x8] = {
#include "sub_main/0046A948.inc"
};
char D_0046A950[0x8] = {
#include "sub_main/0046A950.inc"
};
char D_0046A958[0x8] = {
#include "sub_main/0046A958.inc"
};
char D_0046A960[0x8] = {
#include "sub_main/0046A960.inc"
};
char D_0046A968[0x8] = {
#include "sub_main/0046A968.inc"
};
char D_0046A970[0x8] = {
#include "sub_main/0046A970.inc"
};
char D_0046A978[0x8] = {
#include "sub_main/0046A978.inc"
};
char D_0046A980[0x8] = {
#include "sub_main/0046A980.inc"
};
char D_0046A988[0x8] = {
#include "sub_main/0046A988.inc"
};
char D_0046A990[0x8] = {
#include "sub_main/0046A990.inc"
};
char D_0046A998[0x8] = {
#include "sub_main/0046A998.inc"
};
char D_0046A9A0[0x8] = {
#include "sub_main/0046A9A0.inc"
};
char D_0046A9A8[0x8] = {
#include "sub_main/0046A9A8.inc"
};
char D_0046A9B0[0x8] = {
#include "sub_main/0046A9B0.inc"
};
char D_0046A9B8[0x8] = {
#include "sub_main/0046A9B8.inc"
};
char D_0046A9C0[0x8] = {
#include "sub_main/0046A9C0.inc"
};
char D_0046A9C8[0x8] = {
#include "sub_main/0046A9C8.inc"
};
char D_0046A9D0[0x8] = {
#include "sub_main/0046A9D0.inc"
};
char D_0046A9D8[0x8] = {
#include "sub_main/0046A9D8.inc"
};
char D_0046A9E0[0x8] = {
#include "sub_main/0046A9E0.inc"
};
char D_0046A9E8[0x8] = {
#include "sub_main/0046A9E8.inc"
};
char D_0046A9F0[0x8] = {
#include "sub_main/0046A9F0.inc"
};
char D_0046A9F8[0x8] = {
#include "sub_main/0046A9F8.inc"
};
char D_0046AA00[0x8] = {
#include "sub_main/0046AA00.inc"
};
char D_0046AA08[0x8] = {
#include "sub_main/0046AA08.inc"
};
char D_0046AA10[0x8] = {
#include "sub_main/0046AA10.inc"
};
char D_0046AA18[0x8] = {
#include "sub_main/0046AA18.inc"
};
char D_0046AA20[0x8] = {
#include "sub_main/0046AA20.inc"
};
char D_0046AA28[0x8] = {
#include "sub_main/0046AA28.inc"
};
char D_0046AA30[0x8] = {
#include "sub_main/0046AA30.inc"
};
char D_0046AA38[0x8] = {
#include "sub_main/0046AA38.inc"
};
char D_0046AA40[0x8] = {
#include "sub_main/0046AA40.inc"
};
char D_0046AA48[0x8] = {
#include "sub_main/0046AA48.inc"
};
char D_0046AA50[0x8] = {
#include "sub_main/0046AA50.inc"
};
char D_0046AA58[0x8] = {
#include "sub_main/0046AA58.inc"
};
char D_0046AA60[0x8] = {
#include "sub_main/0046AA60.inc"
};
char D_0046AA68[0x8] = {
#include "sub_main/0046AA68.inc"
};
char D_0046AA70[0x8] = {
#include "sub_main/0046AA70.inc"
};
char D_0046AA78[0x8] = {
#include "sub_main/0046AA78.inc"
};
char D_0046AA80[0x8] = {
#include "sub_main/0046AA80.inc"
};
char D_0046AA88[0x8] = {
#include "sub_main/0046AA88.inc"
};
char D_0046AA90[0x8] = {
#include "sub_main/0046AA90.inc"
};
char D_0046AA98[0x8] = {
#include "sub_main/0046AA98.inc"
};
char D_0046AAA0[0x8] = {
#include "sub_main/0046AAA0.inc"
};
char D_0046AAA8[0x8] = {
#include "sub_main/0046AAA8.inc"
};
char D_0046AAB0[0x8] = {
#include "sub_main/0046AAB0.inc"
};
char D_0046AAB8[0x8] = {
#include "sub_main/0046AAB8.inc"
};
char D_0046AAC0[0x8] = {
#include "sub_main/0046AAC0.inc"
};
char D_0046AAC8[0x8] = {
#include "sub_main/0046AAC8.inc"
};
char D_0046AAD0[0x8] = {
#include "sub_main/0046AAD0.inc"
};
char D_0046AAD8[0x8] = {
#include "sub_main/0046AAD8.inc"
};
char D_0046AAE0[0x8] = {
#include "sub_main/0046AAE0.inc"
};
char D_0046AAE8[0x8] = {
#include "sub_main/0046AAE8.inc"
};
char D_0046AAF0[0x8] = {
#include "sub_main/0046AAF0.inc"
};
char D_0046AAF8[0x8] = {
#include "sub_main/0046AAF8.inc"
};
char D_0046AB00[0x8] = {
#include "sub_main/0046AB00.inc"
};
char D_0046AB08[0x8] = {
#include "sub_main/0046AB08.inc"
};
char D_0046AB10[0x8] = {
#include "sub_main/0046AB10.inc"
};
char D_0046AB18[0x8] = {
#include "sub_main/0046AB18.inc"
};
char D_0046AB20[0x8] = {
#include "sub_main/0046AB20.inc"
};
char D_0046AB28[0x8] = {
#include "sub_main/0046AB28.inc"
};
char D_0046AB30[0x8] = {
#include "sub_main/0046AB30.inc"
};
char D_0046AB38[0x8] = {
#include "sub_main/0046AB38.inc"
};
char D_0046AB40[0x8] = {
#include "sub_main/0046AB40.inc"
};
char D_0046AB48[0x8] = {
#include "sub_main/0046AB48.inc"
};
char D_0046AB50[0x8] = {
#include "sub_main/0046AB50.inc"
};
char D_0046AB58[0x8] = {
#include "sub_main/0046AB58.inc"
};
char D_0046AB60[0x8] = {
#include "sub_main/0046AB60.inc"
};
char D_0046AB68[0x8] = {
#include "sub_main/0046AB68.inc"
};
char D_0046AB70[0x8] = {
#include "sub_main/0046AB70.inc"
};
char D_0046AB78[0x8] = {
#include "sub_main/0046AB78.inc"
};
char D_0046AB80[0x8] = {
#include "sub_main/0046AB80.inc"
};
char D_0046AB88[0x8] = {
#include "sub_main/0046AB88.inc"
};
char D_0046AB90[0x8] = {
#include "sub_main/0046AB90.inc"
};
char D_0046AB98[0x8] = {
#include "sub_main/0046AB98.inc"
};
char D_0046ABA0[0x8] = {
#include "sub_main/0046ABA0.inc"
};
char D_0046ABA8[0x8] = {
#include "sub_main/0046ABA8.inc"
};
char D_0046ABB0[0x8] = {
#include "sub_main/0046ABB0.inc"
};
char D_0046ABB8[0x8] = {
#include "sub_main/0046ABB8.inc"
};
char D_0046ABC0[0x8] = {
#include "sub_main/0046ABC0.inc"
};
char D_0046ABC8[0x8] = {
#include "sub_main/0046ABC8.inc"
};
char D_0046ABD0[0x8] = {
#include "sub_main/0046ABD0.inc"
};
char D_0046ABD8[0x8] = {
#include "sub_main/0046ABD8.inc"
};
char D_0046ABE0[0x8] = {
#include "sub_main/0046ABE0.inc"
};
char D_0046ABE8[0x8] = {
#include "sub_main/0046ABE8.inc"
};
char D_0046ABF0[0x8] = {
#include "sub_main/0046ABF0.inc"
};
char D_0046ABF8[0x8] = {
#include "sub_main/0046ABF8.inc"
};
char D_0046AC00[0x8] = {
#include "sub_main/0046AC00.inc"
};
char D_0046AC08[0x8] = {
#include "sub_main/0046AC08.inc"
};
char D_0046AC10[0x8] = {
#include "sub_main/0046AC10.inc"
};
char D_0046AC18[0x8] = {
#include "sub_main/0046AC18.inc"
};
char D_0046AC20[0x8] = {
#include "sub_main/0046AC20.inc"
};
char D_0046AC28[0x8] = {
#include "sub_main/0046AC28.inc"
};
char D_0046AC30[0x8] = {
#include "sub_main/0046AC30.inc"
};
char D_0046AC38[0x8] = {
#include "sub_main/0046AC38.inc"
};
char D_0046AC40[0x8] = {
#include "sub_main/0046AC40.inc"
};
char D_0046AC48[0x8] = {
#include "sub_main/0046AC48.inc"
};
char D_0046AC50[0x8] = {
#include "sub_main/0046AC50.inc"
};
char D_0046AC58[0x8] = {
#include "sub_main/0046AC58.inc"
};
char D_0046AC60[0x8] = {
#include "sub_main/0046AC60.inc"
};
char D_0046AC68[0x8] = {
#include "sub_main/0046AC68.inc"
};
char D_0046AC70[0x8] = {
#include "sub_main/0046AC70.inc"
};
char D_0046AC78[0x8] = {
#include "sub_main/0046AC78.inc"
};
char D_0046AC80[0x8] = {
#include "sub_main/0046AC80.inc"
};
char D_0046AC88[0x8] = {
#include "sub_main/0046AC88.inc"
};
char D_0046AC90[0x8] = {
#include "sub_main/0046AC90.inc"
};
char D_0046AC98[0x8] = {
#include "sub_main/0046AC98.inc"
};
char D_0046ACA0[0x8] = {
#include "sub_main/0046ACA0.inc"
};
char D_0046ACA8[0x8] = {
#include "sub_main/0046ACA8.inc"
};
char D_0046ACB0[0x8] = {
#include "sub_main/0046ACB0.inc"
};
char D_0046ACB8[0x8] = {
#include "sub_main/0046ACB8.inc"
};
char D_0046ACC0[0x8] = {
#include "sub_main/0046ACC0.inc"
};
char D_0046ACC8[0x8] = {
#include "sub_main/0046ACC8.inc"
};
char D_0046ACD0[0x8] = {
#include "sub_main/0046ACD0.inc"
};
char D_0046ACD8[0x8] = {
#include "sub_main/0046ACD8.inc"
};
char D_0046ACE0[0x8] = {
#include "sub_main/0046ACE0.inc"
};
char D_0046ACE8[0x8] = {
#include "sub_main/0046ACE8.inc"
};
char D_0046ACF0[0x8] = {
#include "sub_main/0046ACF0.inc"
};
char D_0046ACF8[0x8] = {
#include "sub_main/0046ACF8.inc"
};
char D_0046AD00[0x8] = {
#include "sub_main/0046AD00.inc"
};
char D_0046AD08[0x8] = {
#include "sub_main/0046AD08.inc"
};
char D_0046AD10[0x8] = {
#include "sub_main/0046AD10.inc"
};
char D_0046AD18[0x8] = {
#include "sub_main/0046AD18.inc"
};
char D_0046AD20[0x8] = {
#include "sub_main/0046AD20.inc"
};
char D_0046AD28[0x8] = {
#include "sub_main/0046AD28.inc"
};
char D_0046AD30[0x8] = {
#include "sub_main/0046AD30.inc"
};
char D_0046AD38[0x8] = {
#include "sub_main/0046AD38.inc"
};
char D_0046AD40[0x8] = {
#include "sub_main/0046AD40.inc"
};
char D_0046AD48[0x8] = {
#include "sub_main/0046AD48.inc"
};
char D_0046AD50[0x8] = {
#include "sub_main/0046AD50.inc"
};
char D_0046AD58[0x8] = {
#include "sub_main/0046AD58.inc"
};
char D_0046AD60[0x8] = {
#include "sub_main/0046AD60.inc"
};
char D_0046AD68[0x8] = {
#include "sub_main/0046AD68.inc"
};
char D_0046AD70[0x8] = {
#include "sub_main/0046AD70.inc"
};
char D_0046AD78[0x8] = {
#include "sub_main/0046AD78.inc"
};
char D_0046AD80[0x8] = {
#include "sub_main/0046AD80.inc"
};
char D_0046AD88[0x8] = {
#include "sub_main/0046AD88.inc"
};
char D_0046AD90[0x8] = {
#include "sub_main/0046AD90.inc"
};
char D_0046AD98[0x8] = {
#include "sub_main/0046AD98.inc"
};
char D_0046ADA0[0x8] = {
#include "sub_main/0046ADA0.inc"
};
char D_0046ADA8[0x8] = {
#include "sub_main/0046ADA8.inc"
};
char D_0046ADB0[0x8] = {
#include "sub_main/0046ADB0.inc"
};
char D_0046ADB8[0x8] = {
#include "sub_main/0046ADB8.inc"
};
char D_0046ADC0[0x8] = {
#include "sub_main/0046ADC0.inc"
};
char D_0046ADC8[0x8] = {
#include "sub_main/0046ADC8.inc"
};
char D_0046ADD0[0x8] = {
#include "sub_main/0046ADD0.inc"
};
char D_0046ADD8[0x8] = {
#include "sub_main/0046ADD8.inc"
};
char D_0046ADE0[0x8] = {
#include "sub_main/0046ADE0.inc"
};
char D_0046ADE8[0x8] = {
#include "sub_main/0046ADE8.inc"
};
char D_0046ADF0[0x8] = {
#include "sub_main/0046ADF0.inc"
};
char D_0046ADF8[0x8] = {
#include "sub_main/0046ADF8.inc"
};
char D_0046AE00[0x8] = {
#include "sub_main/0046AE00.inc"
};
char D_0046AE08[0x8] = {
#include "sub_main/0046AE08.inc"
};
char D_0046AE10[0x8] = {
#include "sub_main/0046AE10.inc"
};
char D_0046AE18[0x8] = {
#include "sub_main/0046AE18.inc"
};
char D_0046AE20[0x8] = {
#include "sub_main/0046AE20.inc"
};
char D_0046AE28[0x8] = {
#include "sub_main/0046AE28.inc"
};
char D_0046AE30[0x8] = {
#include "sub_main/0046AE30.inc"
};
char D_0046AE38[0x8] = {
#include "sub_main/0046AE38.inc"
};
char D_0046AE40[0x8] = {
#include "sub_main/0046AE40.inc"
};
char D_0046AE48[0x8] = {
#include "sub_main/0046AE48.inc"
};
char D_0046AE50[0x8] = {
#include "sub_main/0046AE50.inc"
};
char D_0046AE58[0x8] = {
#include "sub_main/0046AE58.inc"
};
char D_0046AE60[0x8] = {
#include "sub_main/0046AE60.inc"
};
char D_0046AE68[0x8] = {
#include "sub_main/0046AE68.inc"
};
char D_0046AE70[0x8] = {
#include "sub_main/0046AE70.inc"
};
char D_0046AE78[0x8] = {
#include "sub_main/0046AE78.inc"
};
char D_0046AE80[0x8] = {
#include "sub_main/0046AE80.inc"
};
char D_0046AE88[0x8] = {
#include "sub_main/0046AE88.inc"
};
char D_0046AE90[0x8] = {
#include "sub_main/0046AE90.inc"
};
char D_0046AE98[0x8] = {
#include "sub_main/0046AE98.inc"
};
char D_0046AEA0[0x8] = {
#include "sub_main/0046AEA0.inc"
};
char D_0046AEA8[0x8] = {
#include "sub_main/0046AEA8.inc"
};
char D_0046AEB0[0x8] = {
#include "sub_main/0046AEB0.inc"
};
char D_0046AEB8[0x8] = {
#include "sub_main/0046AEB8.inc"
};
char D_0046AEC0[0x8] = {
#include "sub_main/0046AEC0.inc"
};
char D_0046AEC8[0x8] = {
#include "sub_main/0046AEC8.inc"
};
char D_0046AED0[0x8] = {
#include "sub_main/0046AED0.inc"
};
char D_0046AED8[0x8] = {
#include "sub_main/0046AED8.inc"
};
char D_0046AEE0[0x8] = {
#include "sub_main/0046AEE0.inc"
};
char D_0046AEE8[0x8] = {
#include "sub_main/0046AEE8.inc"
};
char D_0046AEF0[0x8] = {
#include "sub_main/0046AEF0.inc"
};
char D_0046AEF8[0x8] = {
#include "sub_main/0046AEF8.inc"
};
char D_0046AF00[0x8] = {
#include "sub_main/0046AF00.inc"
};
char D_0046AF08[0x8] = {
#include "sub_main/0046AF08.inc"
};
char D_0046AF10[0x8] = {
#include "sub_main/0046AF10.inc"
};
char D_0046AF18[0x8] = {
#include "sub_main/0046AF18.inc"
};
char D_0046AF20[0x8] = {
#include "sub_main/0046AF20.inc"
};
char D_0046AF28[0x8] = {
#include "sub_main/0046AF28.inc"
};
char D_0046AF30[0x8] = {
#include "sub_main/0046AF30.inc"
};
char D_0046AF38[0x8] = {
#include "sub_main/0046AF38.inc"
};
char D_0046AF40[0x8] = {
#include "sub_main/0046AF40.inc"
};
char D_0046AF48[0x8] = {
#include "sub_main/0046AF48.inc"
};
char D_0046AF50[0x8] = {
#include "sub_main/0046AF50.inc"
};
char D_0046AF58[0x8] = {
#include "sub_main/0046AF58.inc"
};
char D_0046AF60[0x8] = {
#include "sub_main/0046AF60.inc"
};
char D_0046AF68[0x8] = {
#include "sub_main/0046AF68.inc"
};
char D_0046AF70[0x8] = {
#include "sub_main/0046AF70.inc"
};
char D_0046AF78[0x8] = {
#include "sub_main/0046AF78.inc"
};
char D_0046AF80[0x8] = {
#include "sub_main/0046AF80.inc"
};
char D_0046AF88[0x8] = {
#include "sub_main/0046AF88.inc"
};
char D_0046AF90[0x8] = {
#include "sub_main/0046AF90.inc"
};
char D_0046AF98[0x8] = {
#include "sub_main/0046AF98.inc"
};
char D_0046AFA0[0x8] = {
#include "sub_main/0046AFA0.inc"
};
char D_0046AFA8[0x8] = {
#include "sub_main/0046AFA8.inc"
};
char D_0046AFB0[0x8] = {
#include "sub_main/0046AFB0.inc"
};
char D_0046AFB8[0x8] = {
#include "sub_main/0046AFB8.inc"
};
char D_0046AFC0[0x8] = {
#include "sub_main/0046AFC0.inc"
};
char D_0046AFC8[0x8] = {
#include "sub_main/0046AFC8.inc"
};
char D_0046AFD0[0x8] = {
#include "sub_main/0046AFD0.inc"
};
char D_0046AFD8[0x8] = {
#include "sub_main/0046AFD8.inc"
};
char D_0046AFE0[0x8] = {
#include "sub_main/0046AFE0.inc"
};
char D_0046AFE8[0x8] = {
#include "sub_main/0046AFE8.inc"
};
char D_0046AFF0[0x8] = {
#include "sub_main/0046AFF0.inc"
};
char D_0046AFF8[0x8] = {
#include "sub_main/0046AFF8.inc"
};
char D_0046B000[0x8] = {
#include "sub_main/0046B000.inc"
};
char D_0046B008[0x8] = {
#include "sub_main/0046B008.inc"
};
char D_0046B010[0x8] = {
#include "sub_main/0046B010.inc"
};
char D_0046B018[0x8] = {
#include "sub_main/0046B018.inc"
};
char D_0046B020[0x8] = {
#include "sub_main/0046B020.inc"
};
char D_0046B028[0x8] = {
#include "sub_main/0046B028.inc"
};
char D_0046B030[0x8] = {
#include "sub_main/0046B030.inc"
};
char D_0046B038[0x8] = {
#include "sub_main/0046B038.inc"
};
char D_0046B040[0x8] = {
#include "sub_main/0046B040.inc"
};
char D_0046B048[0x8] = {
#include "sub_main/0046B048.inc"
};
char D_0046B050[0x8] = {
#include "sub_main/0046B050.inc"
};
char D_0046B058[0x8] = {
#include "sub_main/0046B058.inc"
};
char D_0046B060[0x8] = {
#include "sub_main/0046B060.inc"
};
char D_0046B068[0x8] = {
#include "sub_main/0046B068.inc"
};
char D_0046B070[0x8] = {
#include "sub_main/0046B070.inc"
};
char D_0046B078[0x8] = {
#include "sub_main/0046B078.inc"
};
char D_0046B080[0x8] = {
#include "sub_main/0046B080.inc"
};
char D_0046B088[0x8] = {
#include "sub_main/0046B088.inc"
};
char D_0046B090[0x8] = {
#include "sub_main/0046B090.inc"
};
char D_0046B098[0x8] = {
#include "sub_main/0046B098.inc"
};
char D_0046B0A0[0x8] = {
#include "sub_main/0046B0A0.inc"
};
char D_0046B0A8[0x8] = {
#include "sub_main/0046B0A8.inc"
};
char D_0046B0B0[0x8] = {
#include "sub_main/0046B0B0.inc"
};
char D_0046B0B8[0x8] = {
#include "sub_main/0046B0B8.inc"
};
char D_0046B0C0[0x8] = {
#include "sub_main/0046B0C0.inc"
};
char D_0046B0C8[0x8] = {
#include "sub_main/0046B0C8.inc"
};
char D_0046B0D0[0x8] = {
#include "sub_main/0046B0D0.inc"
};
char D_0046B0D8[0x8] = {
#include "sub_main/0046B0D8.inc"
};
char D_0046B0E0[0x8] = {
#include "sub_main/0046B0E0.inc"
};
char D_0046B0E8[0x8] = {
#include "sub_main/0046B0E8.inc"
};
char D_0046B0F0[0x8] = {
#include "sub_main/0046B0F0.inc"
};
char D_0046B0F8[0x8] = {
#include "sub_main/0046B0F8.inc"
};
char D_0046B100[0x8] = {
#include "sub_main/0046B100.inc"
};
char D_0046B108[0x8] = {
#include "sub_main/0046B108.inc"
};
char D_0046B110[0x8] = {
#include "sub_main/0046B110.inc"
};
char D_0046B118[0x8] = {
#include "sub_main/0046B118.inc"
};
char D_0046B120[0x8] = {
#include "sub_main/0046B120.inc"
};
char D_0046B128[0x8] = {
#include "sub_main/0046B128.inc"
};
char D_0046B130[0x8] = {
#include "sub_main/0046B130.inc"
};
char D_0046B138[0x8] = {
#include "sub_main/0046B138.inc"
};
char D_0046B140[0x8] = {
#include "sub_main/0046B140.inc"
};
char D_0046B148[0x8] = {
#include "sub_main/0046B148.inc"
};
char D_0046B150[0x8] = {
#include "sub_main/0046B150.inc"
};
char D_0046B158[0x8] = {
#include "sub_main/0046B158.inc"
};
char D_0046B160[0x8] = {
#include "sub_main/0046B160.inc"
};
char D_0046B168[0x8] = {
#include "sub_main/0046B168.inc"
};
char D_0046B170[0x8] = {
#include "sub_main/0046B170.inc"
};
char D_0046B178[0x8] = {
#include "sub_main/0046B178.inc"
};
char D_0046B180[0x8] = {
#include "sub_main/0046B180.inc"
};
char D_0046B188[0x8] = {
#include "sub_main/0046B188.inc"
};
char D_0046B190[0x8] = {
#include "sub_main/0046B190.inc"
};
char D_0046B198[0x8] = {
#include "sub_main/0046B198.inc"
};
char D_0046B1A0[0x8] = {
#include "sub_main/0046B1A0.inc"
};
char D_0046B1A8[0x8] = {
#include "sub_main/0046B1A8.inc"
};
char D_0046B1B0[0x8] = {
#include "sub_main/0046B1B0.inc"
};
char D_0046B1B8[0x8] = {
#include "sub_main/0046B1B8.inc"
};
char D_0046B1C0[0x8] = {
#include "sub_main/0046B1C0.inc"
};
char D_0046B1C8[0x8] = {
#include "sub_main/0046B1C8.inc"
};
char D_0046B1D0[0x8] = {
#include "sub_main/0046B1D0.inc"
};
char D_0046B1D8[0x8] = {
#include "sub_main/0046B1D8.inc"
};
char D_0046B1E0[0x8] = {
#include "sub_main/0046B1E0.inc"
};
char D_0046B1E8[0x8] = {
#include "sub_main/0046B1E8.inc"
};
char D_0046B1F0[0x8] = {
#include "sub_main/0046B1F0.inc"
};
char D_0046B1F8[0x8] = {
#include "sub_main/0046B1F8.inc"
};
char D_0046B200[0x8] = {
#include "sub_main/0046B200.inc"
};
char D_0046B208[0x8] = {
#include "sub_main/0046B208.inc"
};
char D_0046B210[0x8] = {
#include "sub_main/0046B210.inc"
};
char D_0046B218[0x8] = {
#include "sub_main/0046B218.inc"
};
char D_0046B220[0x8] = {
#include "sub_main/0046B220.inc"
};
char D_0046B228[0x8] = {
#include "sub_main/0046B228.inc"
};
char D_0046B230[0x8] = {
#include "sub_main/0046B230.inc"
};
char D_0046B238[0x8] = {
#include "sub_main/0046B238.inc"
};
char D_0046B240[0x8] = {
#include "sub_main/0046B240.inc"
};
char D_0046B248[0x8] = {
#include "sub_main/0046B248.inc"
};
char D_0046B250[0x8] = {
#include "sub_main/0046B250.inc"
};
char D_0046B258[0x8] = {
#include "sub_main/0046B258.inc"
};
char D_0046B260[0x8] = {
#include "sub_main/0046B260.inc"
};
char D_0046B268[0x8] = {
#include "sub_main/0046B268.inc"
};
char D_0046B270[0x8] = {
#include "sub_main/0046B270.inc"
};
char D_0046B278[0x8] = {
#include "sub_main/0046B278.inc"
};
char D_0046B280[0x8] = {
#include "sub_main/0046B280.inc"
};
char D_0046B288[0x8] = {
#include "sub_main/0046B288.inc"
};
char D_0046B290[0x8] = {
#include "sub_main/0046B290.inc"
};
char D_0046B298[0x8] = {
#include "sub_main/0046B298.inc"
};
char D_0046B2A0[0x8] = {
#include "sub_main/0046B2A0.inc"
};
char D_0046B2A8[0x8] = {
#include "sub_main/0046B2A8.inc"
};
char D_0046B2B0[0x8] = {
#include "sub_main/0046B2B0.inc"
};
char gSmallTsuHira[0x8] = {
#include "sub_main/0046B2B8.inc"
};
char gSmallTsuKata[0x8] = {
#include "sub_main/0046B2C0.inc"
};
char gSmallTsuHalf[0x8] = {
#include "sub_main/0046B2C8.inc"
};
char D_0046B2D0[0x8] = {
#include "sub_main/0046B2D0.inc"
};
char D_0046B2D8[0x8] = {
#include "sub_main/0046B2D8.inc"
};
char D_0046B2E0[0x8] = {
#include "sub_main/0046B2E0.inc"
};
char D_0046B2E8[0x8] = {
#include "sub_main/0046B2E8.inc"
};
char D_0046B2F0[0x8] = {
#include "sub_main/0046B2F0.inc"
};
char D_0046B2F8[0x8] = {
#include "sub_main/0046B2F8.inc"
};
char D_0046B300[0x8] = {
#include "sub_main/0046B300.inc"
};
char D_0046B308[0x8] = {
#include "sub_main/0046B308.inc"
};
char D_0046B310[0x8] = {
#include "sub_main/0046B310.inc"
};
char D_0046B318[0x8] = {
#include "sub_main/0046B318.inc"
};
char D_0046B320[0x8] = {
#include "sub_main/0046B320.inc"
};
char D_0046B328[0x8] = {
#include "sub_main/0046B328.inc"
};
char D_0046B330[0x8] = {
#include "sub_main/0046B330.inc"
};
char D_0046B338[0x8] = {
#include "sub_main/0046B338.inc"
};
char D_0046B340[0x8] = {
#include "sub_main/0046B340.inc"
};
char D_0046B348[0x8] = {
#include "sub_main/0046B348.inc"
};
char D_0046B350[0x8] = {
#include "sub_main/0046B350.inc"
};
char D_0046B358[0x8] = {
#include "sub_main/0046B358.inc"
};
char D_0046B360[0x8] = {
#include "sub_main/0046B360.inc"
};
char D_0046B368[0x8] = {
#include "sub_main/0046B368.inc"
};
char D_0046B370[0x8] = {
#include "sub_main/0046B370.inc"
};
char D_0046B378[0x8] = {
#include "sub_main/0046B378.inc"
};
char D_0046B380[0x8] = {
#include "sub_main/0046B380.inc"
};
char D_0046B388[0x8] = {
#include "sub_main/0046B388.inc"
};
char D_0046B390[0x8] = {
#include "sub_main/0046B390.inc"
};
char D_0046B398[0x8] = {
#include "sub_main/0046B398.inc"
};
char D_0046B3A0[0x8] = {
#include "sub_main/0046B3A0.inc"
};
char D_0046B3A8[0x8] = {
#include "sub_main/0046B3A8.inc"
};
char D_0046B3B0[0x8] = {
#include "sub_main/0046B3B0.inc"
};
char D_0046B3B8[0x8] = {
#include "sub_main/0046B3B8.inc"
};
char D_0046B3C0[0x8] = {
#include "sub_main/0046B3C0.inc"
};
char D_0046B3C8[0x8] = {
#include "sub_main/0046B3C8.inc"
};
char D_0046B3D0[0x8] = {
#include "sub_main/0046B3D0.inc"
};
char D_0046B3D8[0x8] = {
#include "sub_main/0046B3D8.inc"
};
char D_0046B3E0[0x8] = {
#include "sub_main/0046B3E0.inc"
};
char D_0046B3E8[0x8] = {
#include "sub_main/0046B3E8.inc"
};
char D_0046B3F0[0x8] = {
#include "sub_main/0046B3F0.inc"
};
char D_0046B3F8[0x8] = {
#include "sub_main/0046B3F8.inc"
};
char D_0046B400[0x8] = {
#include "sub_main/0046B400.inc"
};
char D_0046B408[0x8] = {
#include "sub_main/0046B408.inc"
};
char D_0046B410[0x8] = {
#include "sub_main/0046B410.inc"
};
char D_0046B418[0x8] = {
#include "sub_main/0046B418.inc"
};
char D_0046B420[0x8] = {
#include "sub_main/0046B420.inc"
};
char D_0046B428[0x8] = {
#include "sub_main/0046B428.inc"
};
char D_0046B430[0x8] = {
#include "sub_main/0046B430.inc"
};
char D_0046B438[0x8] = {
#include "sub_main/0046B438.inc"
};
char D_0046B440[0x8] = {
#include "sub_main/0046B440.inc"
};
char D_0046B448[0x8] = {
#include "sub_main/0046B448.inc"
};
char D_0046B450[0x8] = {
#include "sub_main/0046B450.inc"
};
char D_0046B458[0x8] = {
#include "sub_main/0046B458.inc"
};
char D_0046B460[0x8] = {
#include "sub_main/0046B460.inc"
};
char D_0046B468[0x8] = {
#include "sub_main/0046B468.inc"
};
char D_0046B470[0x8] = {
#include "sub_main/0046B470.inc"
};
char D_0046B478[0x8] = {
#include "sub_main/0046B478.inc"
};
char D_0046B480[0x8] = {
#include "sub_main/0046B480.inc"
};
char D_0046B488[0x8] = {
#include "sub_main/0046B488.inc"
};
char D_0046B490[0x8] = {
#include "sub_main/0046B490.inc"
};
char D_0046B498[0x8] = {
#include "sub_main/0046B498.inc"
};
char D_0046B4A0[0x8] = {
#include "sub_main/0046B4A0.inc"
};
char D_0046B4A8[0x8] = {
#include "sub_main/0046B4A8.inc"
};
char D_0046B4B0[0x8] = {
#include "sub_main/0046B4B0.inc"
};
char D_0046B4B8[0x8] = {
#include "sub_main/0046B4B8.inc"
};
char D_0046B4C0[0x8] = {
#include "sub_main/0046B4C0.inc"
};
char D_0046B4C8[0x8] = {
#include "sub_main/0046B4C8.inc"
};
char D_0046B4D0[0x8] = {
#include "sub_main/0046B4D0.inc"
};
char D_0046B4D8[0x8] = {
#include "sub_main/0046B4D8.inc"
};
char D_0046B4E0[0x8] = {
#include "sub_main/0046B4E0.inc"
};
char D_0046B4E8[0x8] = {
#include "sub_main/0046B4E8.inc"
};
char D_0046B4F0[0x8] = {
#include "sub_main/0046B4F0.inc"
};
char D_0046B4F8[0x8] = {
#include "sub_main/0046B4F8.inc"
};
char D_0046B500[0x8] = {
#include "sub_main/0046B500.inc"
};
char D_0046B508[0x8] = {
#include "sub_main/0046B508.inc"
};
char D_0046B510[0x8] = {
#include "sub_main/0046B510.inc"
};
char D_0046B518[0x8] = {
#include "sub_main/0046B518.inc"
};
char D_0046B520[0x8] = {
#include "sub_main/0046B520.inc"
};
char D_0046B528[0x8] = {
#include "sub_main/0046B528.inc"
};
char D_0046B530[0x8] = {
#include "sub_main/0046B530.inc"
};
char D_0046B538[0x8] = {
#include "sub_main/0046B538.inc"
};
char D_0046B540[0x8] = {
#include "sub_main/0046B540.inc"
};
char D_0046B548[0x8] = {
#include "sub_main/0046B548.inc"
};
char D_0046B550[0x8] = {
#include "sub_main/0046B550.inc"
};
char D_0046B558[0x8] = {
#include "sub_main/0046B558.inc"
};
char D_0046B560[0x8] = {
#include "sub_main/0046B560.inc"
};
char D_0046B568[0x8] = {
#include "sub_main/0046B568.inc"
};
char D_0046B570[0x8] = {
#include "sub_main/0046B570.inc"
};
char D_0046B578[0x8] = {
#include "sub_main/0046B578.inc"
};
char D_0046B580[0x8] = {
#include "sub_main/0046B580.inc"
};
char D_0046B588[0x8] = {
#include "sub_main/0046B588.inc"
};
char D_0046B590[0x8] = {
#include "sub_main/0046B590.inc"
};
char D_0046B598[0x8] = {
#include "sub_main/0046B598.inc"
};
char D_0046B5A0[0x8] = {
#include "sub_main/0046B5A0.inc"
};
char D_0046B5A8[0x8] = {
#include "sub_main/0046B5A8.inc"
};
char D_0046B5B0[0x8] = {
#include "sub_main/0046B5B0.inc"
};
char D_0046B5B8[0x8] = {
#include "sub_main/0046B5B8.inc"
};
char D_0046B5C0[0x8] = {
#include "sub_main/0046B5C0.inc"
};
char D_0046B5C8[0x8] = {
#include "sub_main/0046B5C8.inc"
};
char D_0046B5D0[0x8] = {
#include "sub_main/0046B5D0.inc"
};
char D_0046B5D8[0x8] = {
#include "sub_main/0046B5D8.inc"
};
char D_0046B5E0[0x8] = {
#include "sub_main/0046B5E0.inc"
};
char D_0046B5E8[0x8] = {
#include "sub_main/0046B5E8.inc"
};
char D_0046B5F0[0x8] = {
#include "sub_main/0046B5F0.inc"
};
char D_0046B5F8[0x8] = {
#include "sub_main/0046B5F8.inc"
};
char D_0046B600[0x8] = {
#include "sub_main/0046B600.inc"
};
char D_0046B608[0x8] = {
#include "sub_main/0046B608.inc"
};
char D_0046B610[0x8] = {
#include "sub_main/0046B610.inc"
};
char D_0046B618[0x8] = {
#include "sub_main/0046B618.inc"
};
char D_0046B620[0x8] = {
#include "sub_main/0046B620.inc"
};
char D_0046B628[0x8] = {
#include "sub_main/0046B628.inc"
};
char D_0046B630[0x8] = {
#include "sub_main/0046B630.inc"
};
char D_0046B638[0x8] = {
#include "sub_main/0046B638.inc"
};
char D_0046B640[0x8] = {
#include "sub_main/0046B640.inc"
};
char D_0046B648[0x8] = {
#include "sub_main/0046B648.inc"
};
char D_0046B650[0x8] = {
#include "sub_main/0046B650.inc"
};
char D_0046B658[0x8] = {
#include "sub_main/0046B658.inc"
};
char D_0046B660[0x8] = {
#include "sub_main/0046B660.inc"
};
char D_0046B668[0x8] = {
#include "sub_main/0046B668.inc"
};
char D_0046B670[0x8] = {
#include "sub_main/0046B670.inc"
};
char D_0046B678[0x8] = {
#include "sub_main/0046B678.inc"
};
char D_0046B680[0x8] = {
#include "sub_main/0046B680.inc"
};
char D_0046B688[0x8] = {
#include "sub_main/0046B688.inc"
};
char D_0046B690[0x8] = {
#include "sub_main/0046B690.inc"
};
char D_0046B698[0x8] = {
#include "sub_main/0046B698.inc"
};
char D_0046B6A0[0x8] = {
#include "sub_main/0046B6A0.inc"
};
char D_0046B6A8[0x8] = {
#include "sub_main/0046B6A8.inc"
};
char D_0046B6B0[0x8] = {
#include "sub_main/0046B6B0.inc"
};
char D_0046B6B8[0x8] = {
#include "sub_main/0046B6B8.inc"
};
char D_0046B6C0[0x8] = {
#include "sub_main/0046B6C0.inc"
};
char D_0046B6C8[0x8] = {
#include "sub_main/0046B6C8.inc"
};
char D_0046B6D0[0x8] = {
#include "sub_main/0046B6D0.inc"
};
char D_0046B6D8[0x8] = {
#include "sub_main/0046B6D8.inc"
};
char D_0046B6E0[0x8] = {
#include "sub_main/0046B6E0.inc"
};
char D_0046B6E8[0x8] = {
#include "sub_main/0046B6E8.inc"
};
char D_0046B6F0[0x8] = {
#include "sub_main/0046B6F0.inc"
};
char D_0046B6F8[0x8] = {
#include "sub_main/0046B6F8.inc"
};
char D_0046B700[0x8] = {
#include "sub_main/0046B700.inc"
};
char D_0046B708[0x8] = {
#include "sub_main/0046B708.inc"
};
char D_0046B710[0x8] = {
#include "sub_main/0046B710.inc"
};
char D_0046B718[0x8] = {
#include "sub_main/0046B718.inc"
};
char D_0046B720[0x8] = {
#include "sub_main/0046B720.inc"
};
char D_0046B728[0x8] = {
#include "sub_main/0046B728.inc"
};
char D_0046B730[0x8] = {
#include "sub_main/0046B730.inc"
};
char D_0046B738[0x8] = {
#include "sub_main/0046B738.inc"
};
char D_0046B740[0x8] = {
#include "sub_main/0046B740.inc"
};
char D_0046B748[0x8] = {
#include "sub_main/0046B748.inc"
};
char D_0046B750[0x8] = {
#include "sub_main/0046B750.inc"
};
char D_0046B758[0x8] = {
#include "sub_main/0046B758.inc"
};
char D_0046B760[0x8] = {
#include "sub_main/0046B760.inc"
};
char D_0046B768[0x8] = {
#include "sub_main/0046B768.inc"
};
char D_0046B770[0x8] = {
#include "sub_main/0046B770.inc"
};
char D_0046B778[0x8] = {
#include "sub_main/0046B778.inc"
};
char D_0046B780[0x8] = {
#include "sub_main/0046B780.inc"
};
char D_0046B788[0x8] = {
#include "sub_main/0046B788.inc"
};
char D_0046B790[0x8] = {
#include "sub_main/0046B790.inc"
};
char D_0046B798[0x8] = {
#include "sub_main/0046B798.inc"
};
char D_0046B7A0[0x8] = {
#include "sub_main/0046B7A0.inc"
};
char D_0046B7A8[0x8] = {
#include "sub_main/0046B7A8.inc"
};
char D_0046B7B0[0x8] = {
#include "sub_main/0046B7B0.inc"
};
char D_0046B7B8[0x8] = {
#include "sub_main/0046B7B8.inc"
};
char D_0046B7C0[0x8] = {
#include "sub_main/0046B7C0.inc"
};
char D_0046B7C8[0x8] = {
#include "sub_main/0046B7C8.inc"
};
char D_0046B7D0[0x8] = {
#include "sub_main/0046B7D0.inc"
};
char D_0046B7D8[0x8] = {
#include "sub_main/0046B7D8.inc"
};
char D_0046B7E0[0x8] = {
#include "sub_main/0046B7E0.inc"
};
char D_0046B7E8[0x8] = {
#include "sub_main/0046B7E8.inc"
};
char D_0046B7F0[0x8] = {
#include "sub_main/0046B7F0.inc"
};
char D_0046B7F8[0x8] = {
#include "sub_main/0046B7F8.inc"
};
char D_0046B800[0x8] = {
#include "sub_main/0046B800.inc"
};
char D_0046B808[0x8] = {
#include "sub_main/0046B808.inc"
};
char D_0046B810[0x8] = {
#include "sub_main/0046B810.inc"
};
char D_0046B818[0x8] = {
#include "sub_main/0046B818.inc"
};
char D_0046B820[0x8] = {
#include "sub_main/0046B820.inc"
};
char D_0046B828[0x8] = {
#include "sub_main/0046B828.inc"
};
char D_0046B830[0x8] = {
#include "sub_main/0046B830.inc"
};
char D_0046B838[0x8] = {
#include "sub_main/0046B838.inc"
};
char D_0046B840[0x8] = {
#include "sub_main/0046B840.inc"
};
char D_0046B848[0x8] = {
#include "sub_main/0046B848.inc"
};
char D_0046B850[0x8] = {
#include "sub_main/0046B850.inc"
};
char D_0046B858[0x8] = {
#include "sub_main/0046B858.inc"
};
char D_0046B860[0x8] = {
#include "sub_main/0046B860.inc"
};
char D_0046B868[0x8] = {
#include "sub_main/0046B868.inc"
};
char D_0046B870[0x8] = {
#include "sub_main/0046B870.inc"
};
char D_0046B878[0x8] = {
#include "sub_main/0046B878.inc"
};
char D_0046B880[0x8] = {
#include "sub_main/0046B880.inc"
};
char D_0046B888[0x8] = {
#include "sub_main/0046B888.inc"
};
char D_0046B890[0x8] = {
#include "sub_main/0046B890.inc"
};
char D_0046B898[0x8] = {
#include "sub_main/0046B898.inc"
};
char D_0046B8A0[0x8] = {
#include "sub_main/0046B8A0.inc"
};
char D_0046B8A8[0x8] = {
#include "sub_main/0046B8A8.inc"
};
char D_0046B8B0[0x8] = {
#include "sub_main/0046B8B0.inc"
};
char D_0046B8B8[0x8] = {
#include "sub_main/0046B8B8.inc"
};
char D_0046B8C0[0x8] = {
#include "sub_main/0046B8C0.inc"
};
char D_0046B8C8[0x8] = {
#include "sub_main/0046B8C8.inc"
};
char D_0046B8D0[0x8] = {
#include "sub_main/0046B8D0.inc"
};
char D_0046B8D8[0x8] = {
#include "sub_main/0046B8D8.inc"
};
char D_0046B8E0[0x8] = {
#include "sub_main/0046B8E0.inc"
};
char D_0046B8E8[0x8] = {
#include "sub_main/0046B8E8.inc"
};
char D_0046B8F0[0x8] = {
#include "sub_main/0046B8F0.inc"
};
char D_0046B8F8[0x8] = {
#include "sub_main/0046B8F8.inc"
};
char D_0046B900[0x8] = {
#include "sub_main/0046B900.inc"
};
char D_0046B908[0x8] = {
#include "sub_main/0046B908.inc"
};
char D_0046B910[0x8] = {
#include "sub_main/0046B910.inc"
};
char D_0046B918[0x8] = {
#include "sub_main/0046B918.inc"
};
char D_0046B920[0x8] = {
#include "sub_main/0046B920.inc"
};
char D_0046B928[0x8] = {
#include "sub_main/0046B928.inc"
};
char D_0046B930[0x8] = {
#include "sub_main/0046B930.inc"
};
char D_0046B938[0x8] = {
#include "sub_main/0046B938.inc"
};
char D_0046B940[0x8] = {
#include "sub_main/0046B940.inc"
};
char D_0046B948[0x8] = {
#include "sub_main/0046B948.inc"
};
char D_0046B950[0x8] = {
#include "sub_main/0046B950.inc"
};
char D_0046B958[0x8] = {
#include "sub_main/0046B958.inc"
};
char D_0046B960[0x8] = {
#include "sub_main/0046B960.inc"
};
char D_0046B968[0x8] = {
#include "sub_main/0046B968.inc"
};
char D_0046B970[0x8] = {
#include "sub_main/0046B970.inc"
};
char D_0046B978[0x8] = {
#include "sub_main/0046B978.inc"
};
char D_0046B980[0x8] = {
#include "sub_main/0046B980.inc"
};
char D_0046B988[0x8] = {
#include "sub_main/0046B988.inc"
};
char D_0046B990[0x8] = {
#include "sub_main/0046B990.inc"
};
char D_0046B998[0x8] = {
#include "sub_main/0046B998.inc"
};
char D_0046B9A0[0x8] = {
#include "sub_main/0046B9A0.inc"
};
char D_0046B9A8[0x8] = {
#include "sub_main/0046B9A8.inc"
};
char D_0046B9B0[0x8] = {
#include "sub_main/0046B9B0.inc"
};
char D_0046B9B8[0x8] = {
#include "sub_main/0046B9B8.inc"
};
char D_0046B9C0[0x8] = {
#include "sub_main/0046B9C0.inc"
};
char D_0046B9C8[0x8] = {
#include "sub_main/0046B9C8.inc"
};
char D_0046B9D0[0x8] = {
#include "sub_main/0046B9D0.inc"
};
char D_0046B9D8[0x8] = {
#include "sub_main/0046B9D8.inc"
};
char D_0046B9E0[0x8] = {
#include "sub_main/0046B9E0.inc"
};
char D_0046B9E8[0x8] = {
#include "sub_main/0046B9E8.inc"
};
char D_0046B9F0[0x8] = {
#include "sub_main/0046B9F0.inc"
};
char D_0046B9F8[0x8] = {
#include "sub_main/0046B9F8.inc"
};
char D_0046BA00[0x8] = {
#include "sub_main/0046BA00.inc"
};
char D_0046BA08[0x8] = {
#include "sub_main/0046BA08.inc"
};
char D_0046BA10[0x8] = {
#include "sub_main/0046BA10.inc"
};
char D_0046BA18[0x8] = {
#include "sub_main/0046BA18.inc"
};
char D_0046BA20[0x8] = {
#include "sub_main/0046BA20.inc"
};
char D_0046BA28[0x8] = {
#include "sub_main/0046BA28.inc"
};
char D_0046BA30[0x8] = {
#include "sub_main/0046BA30.inc"
};
char D_0046BA38[0x8] = {
#include "sub_main/0046BA38.inc"
};
char D_0046BA40[0x8] = {
#include "sub_main/0046BA40.inc"
};
char D_0046BA48[0x8] = {
#include "sub_main/0046BA48.inc"
};
char D_0046BA50[0x8] = {
#include "sub_main/0046BA50.inc"
};
char D_0046BA58[0x8] = {
#include "sub_main/0046BA58.inc"
};
char D_0046BA60[0x8] = {
#include "sub_main/0046BA60.inc"
};
char D_0046BA68[0x8] = {
#include "sub_main/0046BA68.inc"
};
char D_0046BA70[0x8] = {
#include "sub_main/0046BA70.inc"
};
char D_0046BA78[0x8] = {
#include "sub_main/0046BA78.inc"
};
char D_0046BA80[0x8] = {
#include "sub_main/0046BA80.inc"
};
char D_0046BA88[0x8] = {
#include "sub_main/0046BA88.inc"
};
char D_0046BA90[0x8] = {
#include "sub_main/0046BA90.inc"
};
char D_0046BA98[0x8] = {
#include "sub_main/0046BA98.inc"
};
char D_0046BAA0[0x8] = {
#include "sub_main/0046BAA0.inc"
};
char D_0046BAA8[0x8] = {
#include "sub_main/0046BAA8.inc"
};
char D_0046BAB0[0x8] = {
#include "sub_main/0046BAB0.inc"
};
char D_0046BAB8[0x8] = {
#include "sub_main/0046BAB8.inc"
};
char D_0046BAC0[0x8] = {
#include "sub_main/0046BAC0.inc"
};
char D_0046BAC8[0x8] = {
#include "sub_main/0046BAC8.inc"
};
char D_0046BAD0[0x8] = {
#include "sub_main/0046BAD0.inc"
};
char D_0046BAD8[0x8] = {
#include "sub_main/0046BAD8.inc"
};
char D_0046BAE0[0x8] = {
#include "sub_main/0046BAE0.inc"
};
char D_0046BAE8[0x8] = {
#include "sub_main/0046BAE8.inc"
};
char D_0046BAF0[0x8] = {
#include "sub_main/0046BAF0.inc"
};
char D_0046BAF8[0x8] = {
#include "sub_main/0046BAF8.inc"
};
char D_0046BB00[0x8] = {
#include "sub_main/0046BB00.inc"
};
char D_0046BB08[0x8] = {
#include "sub_main/0046BB08.inc"
};
char D_0046BB10[0x8] = {
#include "sub_main/0046BB10.inc"
};
char D_0046BB18[0x8] = {
#include "sub_main/0046BB18.inc"
};
char D_0046BB20[0x8] = {
#include "sub_main/0046BB20.inc"
};
char D_0046BB28[0x8] = {
#include "sub_main/0046BB28.inc"
};
char D_0046BB30[0x8] = {
#include "sub_main/0046BB30.inc"
};
char D_0046BB38[0x8] = {
#include "sub_main/0046BB38.inc"
};
char D_0046BB40[0x8] = {
#include "sub_main/0046BB40.inc"
};
char D_0046BB48[0x8] = {
#include "sub_main/0046BB48.inc"
};
char D_0046BB50[0x8] = {
#include "sub_main/0046BB50.inc"
};
char D_0046BB58[0x8] = {
#include "sub_main/0046BB58.inc"
};
char D_0046BB60[0x8] = {
#include "sub_main/0046BB60.inc"
};
char D_0046BB68[0x8] = {
#include "sub_main/0046BB68.inc"
};
char D_0046BB70[0x8] = {
#include "sub_main/0046BB70.inc"
};
char D_0046BB78[0x8] = {
#include "sub_main/0046BB78.inc"
};
char D_0046BB80[0x8] = {
#include "sub_main/0046BB80.inc"
};
char D_0046BB88[0x8] = {
#include "sub_main/0046BB88.inc"
};
char D_0046BB90[0x8] = {
#include "sub_main/0046BB90.inc"
};
char D_0046BB98[0x8] = {
#include "sub_main/0046BB98.inc"
};
char D_0046BBA0[0x8] = {
#include "sub_main/0046BBA0.inc"
};
char D_0046BBA8[0x8] = {
#include "sub_main/0046BBA8.inc"
};
char D_0046BBB0[0x8] = {
#include "sub_main/0046BBB0.inc"
};
char D_0046BBB8[0x8] = {
#include "sub_main/0046BBB8.inc"
};
char D_0046BBC0[0x8] = {
#include "sub_main/0046BBC0.inc"
};
char D_0046BBC8[0x8] = {
#include "sub_main/0046BBC8.inc"
};
char D_0046BBD0[0x8] = {
#include "sub_main/0046BBD0.inc"
};
char D_0046BBD8[0x8] = {
#include "sub_main/0046BBD8.inc"
};
char D_0046BBE0[0x8] = {
#include "sub_main/0046BBE0.inc"
};
char D_0046BBE8[0x8] = {
#include "sub_main/0046BBE8.inc"
};
char D_0046BBF0[0x8] = {
#include "sub_main/0046BBF0.inc"
};
char D_0046BBF8[0x8] = {
#include "sub_main/0046BBF8.inc"
};
char D_0046BC00[0x8] = {
#include "sub_main/0046BC00.inc"
};
char D_0046BC08[0x8] = {
#include "sub_main/0046BC08.inc"
};
char D_0046BC10[0x8] = {
#include "sub_main/0046BC10.inc"
};
char D_0046BC18[0x8] = {
#include "sub_main/0046BC18.inc"
};
char D_0046BC20[0x8] = {
#include "sub_main/0046BC20.inc"
};
char D_0046BC28[0x8] = {
#include "sub_main/0046BC28.inc"
};
char D_0046BC30[0x8] = {
#include "sub_main/0046BC30.inc"
};
char D_0046BC38[0x8] = {
#include "sub_main/0046BC38.inc"
};
char D_0046BC40[0x8] = {
#include "sub_main/0046BC40.inc"
};
char D_0046BC48[0x8] = {
#include "sub_main/0046BC48.inc"
};
char D_0046BC50[0x8] = {
#include "sub_main/0046BC50.inc"
};
char D_0046BC58[0x8] = {
#include "sub_main/0046BC58.inc"
};
char D_0046BC60[0x8] = {
#include "sub_main/0046BC60.inc"
};
char D_0046BC68[0x8] = {
#include "sub_main/0046BC68.inc"
};
char D_0046BC70[0x8] = {
#include "sub_main/0046BC70.inc"
};
char D_0046BC78[0x8] = {
#include "sub_main/0046BC78.inc"
};
char D_0046BC80[0x8] = {
#include "sub_main/0046BC80.inc"
};
char D_0046BC88[0x8] = {
#include "sub_main/0046BC88.inc"
};
char D_0046BC90[0x8] = {
#include "sub_main/0046BC90.inc"
};
char D_0046BC98[0x8] = {
#include "sub_main/0046BC98.inc"
};
char D_0046BCA0[0x8] = {
#include "sub_main/0046BCA0.inc"
};
char D_0046BCA8[0x8] = {
#include "sub_main/0046BCA8.inc"
};
char D_0046BCB0[0x8] = {
#include "sub_main/0046BCB0.inc"
};
char D_0046BCB8[0x8] = {
#include "sub_main/0046BCB8.inc"
};
char D_0046BCC0[0x8] = {
#include "sub_main/0046BCC0.inc"
};
char D_0046BCC8[0x8] = {
#include "sub_main/0046BCC8.inc"
};
char D_0046BCD0[0x8] = {
#include "sub_main/0046BCD0.inc"
};
char D_0046BCD8[0x8] = {
#include "sub_main/0046BCD8.inc"
};
char D_0046BCE0[0x8] = {
#include "sub_main/0046BCE0.inc"
};
char D_0046BCE8[0x8] = {
#include "sub_main/0046BCE8.inc"
};
char D_0046BCF0[0x8] = {
#include "sub_main/0046BCF0.inc"
};
char D_0046BCF8[0x8] = {
#include "sub_main/0046BCF8.inc"
};
char D_0046BD00[0x8] = {
#include "sub_main/0046BD00.inc"
};
char D_0046BD08[0x8] = {
#include "sub_main/0046BD08.inc"
};
char D_0046BD10[0x8] = {
#include "sub_main/0046BD10.inc"
};
char D_0046BD18[0x8] = {
#include "sub_main/0046BD18.inc"
};
char D_0046BD20[0x8] = {
#include "sub_main/0046BD20.inc"
};
char D_0046BD28[0x8] = {
#include "sub_main/0046BD28.inc"
};
char D_0046BD30[0x8] = {
#include "sub_main/0046BD30.inc"
};
char D_0046BD38[0x8] = {
#include "sub_main/0046BD38.inc"
};
char D_0046BD40[0x8] = {
#include "sub_main/0046BD40.inc"
};
char D_0046BD48[0x8] = {
#include "sub_main/0046BD48.inc"
};
char D_0046BD50[0x8] = {
#include "sub_main/0046BD50.inc"
};
char D_0046BD58[0x8] = {
#include "sub_main/0046BD58.inc"
};
char D_0046BD60[0x8] = {
#include "sub_main/0046BD60.inc"
};
char D_0046BD68[0x8] = {
#include "sub_main/0046BD68.inc"
};
char D_0046BD70[0x8] = {
#include "sub_main/0046BD70.inc"
};
char D_0046BD78[0x8] = {
#include "sub_main/0046BD78.inc"
};
char D_0046BD80[0x8] = {
#include "sub_main/0046BD80.inc"
};
char D_0046BD88[0x8] = {
#include "sub_main/0046BD88.inc"
};
char D_0046BD90[0x8] = {
#include "sub_main/0046BD90.inc"
};
char D_0046BD98[0x8] = {
#include "sub_main/0046BD98.inc"
};
char D_0046BDA0[0x8] = {
#include "sub_main/0046BDA0.inc"
};
char D_0046BDA8[0x8] = {
#include "sub_main/0046BDA8.inc"
};
char D_0046BDB0[0x8] = {
#include "sub_main/0046BDB0.inc"
};
char D_0046BDB8[0x8] = {
#include "sub_main/0046BDB8.inc"
};
char D_0046BDC0[0x8] = {
#include "sub_main/0046BDC0.inc"
};
char D_0046BDC8[0x8] = {
#include "sub_main/0046BDC8.inc"
};
char D_0046BDD0[0x8] = {
#include "sub_main/0046BDD0.inc"
};
char D_0046BDD8[0x8] = {
#include "sub_main/0046BDD8.inc"
};
char D_0046BDE0[0x8] = {
#include "sub_main/0046BDE0.inc"
};
char D_0046BDE8[0x8] = {
#include "sub_main/0046BDE8.inc"
};
char D_0046BDF0[0x8] = {
#include "sub_main/0046BDF0.inc"
};
char D_0046BDF8[0x8] = {
#include "sub_main/0046BDF8.inc"
};
char D_0046BE00[0x8] = {
#include "sub_main/0046BE00.inc"
};
char D_0046BE08[0x8] = {
#include "sub_main/0046BE08.inc"
};
char D_0046BE10[0x8] = {
#include "sub_main/0046BE10.inc"
};
char D_0046BE18[0x8] = {
#include "sub_main/0046BE18.inc"
};
char D_0046BE20[0x8] = {
#include "sub_main/0046BE20.inc"
};
char D_0046BE28[0x8] = {
#include "sub_main/0046BE28.inc"
};
char D_0046BE30[0x8] = {
#include "sub_main/0046BE30.inc"
};
char D_0046BE38[0x8] = {
#include "sub_main/0046BE38.inc"
};
char D_0046BE40[0x8] = {
#include "sub_main/0046BE40.inc"
};
char D_0046BE48[0x8] = {
#include "sub_main/0046BE48.inc"
};
char D_0046BE50[0x8] = {
#include "sub_main/0046BE50.inc"
};
char D_0046BE58[0x8] = {
#include "sub_main/0046BE58.inc"
};
char D_0046BE60[0x8] = {
#include "sub_main/0046BE60.inc"
};
char D_0046BE68[0x8] = {
#include "sub_main/0046BE68.inc"
};
char D_0046BE70[0x8] = {
#include "sub_main/0046BE70.inc"
};
char D_0046BE78[0x8] = {
#include "sub_main/0046BE78.inc"
};
char D_0046BE80[0x8] = {
#include "sub_main/0046BE80.inc"
};
char D_0046BE88[0x8] = {
#include "sub_main/0046BE88.inc"
};
char D_0046BE90[0x8] = {
#include "sub_main/0046BE90.inc"
};
char D_0046BE98[0x8] = {
#include "sub_main/0046BE98.inc"
};
char D_0046BEA0[0x8] = {
#include "sub_main/0046BEA0.inc"
};
char D_0046BEA8[0x8] = {
#include "sub_main/0046BEA8.inc"
};
char D_0046BEB0[0x8] = {
#include "sub_main/0046BEB0.inc"
};
char D_0046BEB8[0x8] = {
#include "sub_main/0046BEB8.inc"
};
char D_0046BEC0[0x8] = {
#include "sub_main/0046BEC0.inc"
};
char D_0046BEC8[0x8] = {
#include "sub_main/0046BEC8.inc"
};
char D_0046BED0[0x8] = {
#include "sub_main/0046BED0.inc"
};
char D_0046BED8[0x8] = {
#include "sub_main/0046BED8.inc"
};
char D_0046BEE0[0x8] = {
#include "sub_main/0046BEE0.inc"
};
char D_0046BEE8[0x8] = {
#include "sub_main/0046BEE8.inc"
};
char D_0046BEF0[0x8] = {
#include "sub_main/0046BEF0.inc"
};
char D_0046BEF8[0x8] = {
#include "sub_main/0046BEF8.inc"
};
char D_0046BF00[0x8] = {
#include "sub_main/0046BF00.inc"
};
char D_0046BF08[0x8] = {
#include "sub_main/0046BF08.inc"
};
char D_0046BF10[0x8] = {
#include "sub_main/0046BF10.inc"
};
char D_0046BF18[0x8] = {
#include "sub_main/0046BF18.inc"
};
char D_0046BF20[0x8] = {
#include "sub_main/0046BF20.inc"
};
char D_0046BF28[0x8] = {
#include "sub_main/0046BF28.inc"
};
char D_0046BF30[0x8] = {
#include "sub_main/0046BF30.inc"
};
char D_0046BF38[0x8] = {
#include "sub_main/0046BF38.inc"
};
char D_0046BF40[0x8] = {
#include "sub_main/0046BF40.inc"
};
char D_0046BF48[0x8] = {
#include "sub_main/0046BF48.inc"
};
char D_0046BF50[0x8] = {
#include "sub_main/0046BF50.inc"
};
char D_0046BF58[0x8] = {
#include "sub_main/0046BF58.inc"
};
char D_0046BF60[0x8] = {
#include "sub_main/0046BF60.inc"
};
char D_0046BF68[0x8] = {
#include "sub_main/0046BF68.inc"
};
char D_0046BF70[0x8] = {
#include "sub_main/0046BF70.inc"
};
char D_0046BF78[0x8] = {
#include "sub_main/0046BF78.inc"
};
char D_0046BF80[0x8] = {
#include "sub_main/0046BF80.inc"
};
char D_0046BF88[0x8] = {
#include "sub_main/0046BF88.inc"
};
char D_0046BF90[0x8] = {
#include "sub_main/0046BF90.inc"
};
char D_0046BF98[0x8] = {
#include "sub_main/0046BF98.inc"
};
char D_0046BFA0[0x8] = {
#include "sub_main/0046BFA0.inc"
};
char D_0046BFA8[0x8] = {
#include "sub_main/0046BFA8.inc"
};
char D_0046BFB0[0x8] = {
#include "sub_main/0046BFB0.inc"
};
char D_0046BFB8[0x8] = {
#include "sub_main/0046BFB8.inc"
};
char D_0046BFC0[0x8] = {
#include "sub_main/0046BFC0.inc"
};
char D_0046BFC8[0x8] = {
#include "sub_main/0046BFC8.inc"
};
char D_0046BFD0[0x8] = {
#include "sub_main/0046BFD0.inc"
};
char D_0046BFD8[0x8] = {
#include "sub_main/0046BFD8.inc"
};
char D_0046BFE0[0x8] = {
#include "sub_main/0046BFE0.inc"
};
char D_0046BFE8[0x8] = {
#include "sub_main/0046BFE8.inc"
};
char D_0046BFF0[0x8] = {
#include "sub_main/0046BFF0.inc"
};
char D_0046BFF8[0x8] = {
#include "sub_main/0046BFF8.inc"
};
char D_0046C000[0x8] = {
#include "sub_main/0046C000.inc"
};
char D_0046C008[0x8] = {
#include "sub_main/0046C008.inc"
};
char D_0046C010[0x8] = {
#include "sub_main/0046C010.inc"
};
char D_0046C018[0x8] = {
#include "sub_main/0046C018.inc"
};
char D_0046C020[0x8] = {
#include "sub_main/0046C020.inc"
};
char D_0046C028[0x8] = {
#include "sub_main/0046C028.inc"
};
char D_0046C030[0x8] = {
#include "sub_main/0046C030.inc"
};
char D_0046C038[0x8] = {
#include "sub_main/0046C038.inc"
};
char D_0046C040[0x8] = {
#include "sub_main/0046C040.inc"
};
char D_0046C048[0x8] = {
#include "sub_main/0046C048.inc"
};
char D_0046C050[0x8] = {
#include "sub_main/0046C050.inc"
};
char D_0046C058[0x8] = {
#include "sub_main/0046C058.inc"
};
char D_0046C060[0x8] = {
#include "sub_main/0046C060.inc"
};
char D_0046C068[0x8] = {
#include "sub_main/0046C068.inc"
};
char D_0046C070[0x8] = {
#include "sub_main/0046C070.inc"
};
char D_0046C078[0x8] = {
#include "sub_main/0046C078.inc"
};
char D_0046C080[0x8] = {
#include "sub_main/0046C080.inc"
};
char D_0046C088[0x8] = {
#include "sub_main/0046C088.inc"
};
char D_0046C090[0x8] = {
#include "sub_main/0046C090.inc"
};
char D_0046C098[0x8] = {
#include "sub_main/0046C098.inc"
};
char D_0046C0A0[0x8] = {
#include "sub_main/0046C0A0.inc"
};
char D_0046C0A8[0x8] = {
#include "sub_main/0046C0A8.inc"
};
char D_0046C0B0[0x8] = {
#include "sub_main/0046C0B0.inc"
};
char D_0046C0B8[0x8] = {
#include "sub_main/0046C0B8.inc"
};
char D_0046C0C0[0x8] = {
#include "sub_main/0046C0C0.inc"
};
char D_0046C0C8[0x8] = {
#include "sub_main/0046C0C8.inc"
};
char D_0046C0D0[0x8] = {
#include "sub_main/0046C0D0.inc"
};
char D_0046C0D8[0x8] = {
#include "sub_main/0046C0D8.inc"
};
char D_0046C0E0[0x8] = {
#include "sub_main/0046C0E0.inc"
};
char D_0046C0E8[0x8] = {
#include "sub_main/0046C0E8.inc"
};
char D_0046C0F0[0x8] = {
#include "sub_main/0046C0F0.inc"
};
char D_0046C0F8[0x8] = {
#include "sub_main/0046C0F8.inc"
};
char D_0046C100[0x8] = {
#include "sub_main/0046C100.inc"
};
char D_0046C108[0x8] = {
#include "sub_main/0046C108.inc"
};
char D_0046C110[0x8] = {
#include "sub_main/0046C110.inc"
};
char D_0046C118[0x8] = {
#include "sub_main/0046C118.inc"
};
char D_0046C120[0x8] = {
#include "sub_main/0046C120.inc"
};
char D_0046C128[0x8] = {
#include "sub_main/0046C128.inc"
};
char D_0046C130[0x8] = {
#include "sub_main/0046C130.inc"
};
char D_0046C138[0x8] = {
#include "sub_main/0046C138.inc"
};
char D_0046C140[0x8] = {
#include "sub_main/0046C140.inc"
};
char D_0046C148[0x8] = {
#include "sub_main/0046C148.inc"
};
char D_0046C150[0x8] = {
#include "sub_main/0046C150.inc"
};
char D_0046C158[0x8] = {
#include "sub_main/0046C158.inc"
};
char D_0046C160[0x8] = {
#include "sub_main/0046C160.inc"
};
char D_0046C168[0x8] = {
#include "sub_main/0046C168.inc"
};
char D_0046C170[0x8] = {
#include "sub_main/0046C170.inc"
};
char D_0046C178[0x8] = {
#include "sub_main/0046C178.inc"
};
char D_0046C180[0x8] = {
#include "sub_main/0046C180.inc"
};
char D_0046C188[0x8] = {
#include "sub_main/0046C188.inc"
};
char D_0046C190[0x8] = {
#include "sub_main/0046C190.inc"
};
char D_0046C198[0x8] = {
#include "sub_main/0046C198.inc"
};
char D_0046C1A0[0x8] = {
#include "sub_main/0046C1A0.inc"
};
char D_0046C1A8[0x8] = {
#include "sub_main/0046C1A8.inc"
};
char D_0046C1B0[0x8] = {
#include "sub_main/0046C1B0.inc"
};
char D_0046C1B8[0x8] = {
#include "sub_main/0046C1B8.inc"
};
char D_0046C1C0[0x8] = {
#include "sub_main/0046C1C0.inc"
};
char D_0046C1C8[0x8] = {
#include "sub_main/0046C1C8.inc"
};
char D_0046C1D0[0x10] = {
#include "sub_main/0046C1D0.inc"
};
char D_0046C1E0[0x80] = {
#include "sub_main/0046C1E0.inc"
};
char D_0046C260[0x80] = {
#include "sub_main/0046C260.inc"
};
char D_0046C2E0[0x80] = {
#include "sub_main/0046C2E0.inc"
};
char D_0046C360[0x80] = {
#include "sub_main/0046C360.inc"
};
char D_0046C3E0[0x80] = {
#include "sub_main/0046C3E0.inc"
};
char D_0046C460[0x80] = {
#include "sub_main/0046C460.inc"
};
char D_0046C4E0[0x80] = {
#include "sub_main/0046C4E0.inc"
};
char D_0046C560[0x80] = {
#include "sub_main/0046C560.inc"
};
char D_0046C5E0[0x80] = {
#include "sub_main/0046C5E0.inc"
};
char D_0046C660[0x80] = {
#include "sub_main/0046C660.inc"
};
char D_0046C6E0[0x30] = {
#include "sub_main/0046C6E0.inc"
};
char D_0046C710[0x40] = {
#include "sub_main/0046C710.inc"
};
char D_0046C750[0x40] = {
#include "sub_main/0046C750.inc"
};
char D_0046C790[0x40] = {
#include "sub_main/0046C790.inc"
};
char D_0046C7D0[0x50] = {
#include "sub_main/0046C7D0.inc"
};
char D_0046C820[0x40] = {
#include "sub_main/0046C820.inc"
};
char D_0046C860[0x40] = {
#include "sub_main/0046C860.inc"
};
char D_0046C8A0[0x40] = {
#include "sub_main/0046C8A0.inc"
};
char D_0046C8E0[0x18] = {
#include "sub_main/0046C8E0.inc"
};
#pragma rodata off

/*
 * Reads the USB keyboard once per frame: nothing while polled is set (the
 * frame update clears it with Kbd_ClearPolled before calling this). Moves
 * usbKey and usbMods into usbPrevKey and usbPrevMods and clears usbMods,
 * usbKey and usbNewKey. When UsbKb_Poll returns a report, usbMods takes
 * its byte 0 (the modifier bits) and usbKey its byte 2 (the first key
 * code); usbNewKey is usbKey when that is nonzero and not usbPrevKey, and
 * usbModsDown holds the modifier bits newly set.
 */
void Kbd_ReadUsb(void)
{
    UsbKbReport *report;
    u8 usbKey;

    if (gKbd->polled == 0) {
        gKbd->polled = 1;
        gKbd->usbPrevKey = (u8) gKbd->usbKey;
        gKbd->usbPrevMods = (u8) gKbd->usbMods;
        gKbd->usbMods = 0U;
        gKbd->usbKey = 0U;
        gKbd->usbNewKey = 0U;
        report = (UsbKbReport *)UsbKb_Poll();
        if (report != NULL) {
            gKbd->usbMods = report->modifiers;
            gKbd->usbKey = (u8) report->keys[0];
            usbKey = gKbd->usbKey;
            if ((usbKey != 0) && (usbKey != gKbd->usbPrevKey)) {
                gKbd->usbNewKey = usbKey;
            }
            gKbd->usbModsDown = (s8) (gKbd->usbMods & ~gKbd->usbPrevMods);
        }
    }
}

void Kbd_HandleKey(void);

/*
 * Kbd_Run's USB step: curKey moves to prevKey and usbKey becomes curKey.
 * repeatTimer is cleared when its top bit is set; on a new key (nonzero
 * and not prevKey) it is set to 0x9E and the key is handled
 * (Kbd_HandleKey). Nothing else reads repeatTimer (seen). The game's code
 * places the clearing after the function's last block and jumps back;
 * the goto gives that layout.
 */
void Kbd_Update(void)
{
    KbdEntry *kbd;
    u8 *timer;

    gKbd->prevKey = gKbd->curKey;
    gKbd->curKey = gKbd->usbKey;
    timer = &gKbd->repeatTimer;
    if (*timer & 0x80) {
        goto clear;
    }
back:
    kbd = gKbd;
    if (kbd->curKey == 0) {
        return;
    }
    if (kbd->curKey == kbd->prevKey) {
        return;
    }
    kbd->repeatTimer = 0x9E;
    Kbd_HandleKey();
    return;
clear:
    *timer = 0;
    goto back;
}

/* Kbd_HandleKey: dispatches the keyboard's current key (USB HID usage code, curKey). */
void Kbd_Escape(void);
void Kbd_Space(s32 arg0);
void Kbd_Backspace(void);
void Kbd_DakutenKey(void);
void Kbd_HandakutenKey(void);
void Kbd_CapsLock(void);
void Kbd_FKey(u8 key);
void Kbd_CursorHome(void);
void Kbd_Delete(void);
void Kbd_CursorEnd(void);
void Kbd_CursorUp(void);
void Kbd_CursorDown(void);
void Kbd_KanaKey(void);
void Kbd_HenkanKey(void);
void Kbd_MuhenkanKey(void);
void Kbd_ZenkakuKey(void);
void Kbd_PageUp(void);
void Kbd_PageDown(void);
void Kbd_NextChatTarget(void);
void Kbd_ClearActive(void);
void Kbd_CursorRight(void);
void Kbd_CursorLeft(void);
void Kbd_Decide(void);

/*
 * Handles a newly pressed USB key (curKey, a HID usage code). Puts the
 * cursor off the grid (offGrid 1) and leaves pad input (Kbd_ClearActive)
 * when the pad was active. Ignores the codes below USB_KEY_A, Print
 * Screen to Pause, Insert, Num Lock, 0x64..0x86 and 0x8C on. Enter and
 * keypad Enter: Kbd_Decide. Escape, Backspace, Space (Kbd_Space(0)),
 * Caps Lock, Home, End, Delete, Page Up, Page Down and the arrows: their
 * Kbd_* handlers. Tab: Kbd_NextChatTarget. 0x2F and 0x30 (the '@' and
 * '[' keys of a Japanese keyboard): Kbd_DakutenKey, Kbd_HandakutenKey.
 * 0x35 (Hankaku/Zenkaku): Kbd_ZenkakuKey. F1-F12: Kbd_FKey. The
 * Katakana/Hiragana, Henkan and Muhenkan keys: Kbd_KanaKey,
 * Kbd_HenkanKey, Kbd_MuhenkanKey. Any other key is typed with
 * Kbd_TypeKey, which (active now being 0) calls Kbd_TypeHidKey.
 */
void Kbd_HandleKey(void)
{
    u8 key;

    gKbd->offGrid = 1;
    if (gKbd->active != 0) {
        Kbd_ClearActive();
    }
    key = gKbd->curKey;
    if (key < USB_KEY_A || (key >= USB_KEY_PRINT_SCREEN && key <= USB_KEY_PAUSE) || key == USB_KEY_NUM_LOCK || (key >= USB_KEY_NONUS_BACKSLASH && key <= USB_KEY_KP_EQUAL_AS400) || key >= USB_KEY_INTL6) {
        return;
    }
    switch (key) {
    case USB_KEY_INSERT:
        return;
    case USB_KEY_PAGE_UP:
        Kbd_PageUp();
        break;
    case USB_KEY_PAGE_DOWN:
        Kbd_PageDown();
        break;
    case USB_KEY_KP_ENTER:
    case USB_KEY_ENTER:
        Kbd_Decide();
        break;
    case USB_KEY_ESCAPE:
        Kbd_Escape();
        break;
    case USB_KEY_BACKSPACE:
        Kbd_Backspace();
        break;
    case USB_KEY_TAB:
        Kbd_NextChatTarget();
        break;
    case USB_KEY_SPACE:
        Kbd_Space(0);
        break;
    case USB_KEY_LBRACKET:
        Kbd_DakutenKey();
        break;
    case USB_KEY_RBRACKET:
        Kbd_HandakutenKey();
        break;
    case USB_KEY_GRAVE:
        Kbd_ZenkakuKey();
        break;
    case USB_KEY_CAPS_LOCK:
        Kbd_CapsLock();
        break;
    case USB_KEY_F1: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case USB_KEY_F6:
    case USB_KEY_F7: case 0x41: case 0x42: case 0x43: case 0x44: case USB_KEY_F12:
        Kbd_FKey(key);
        break;
    case USB_KEY_HOME:
        Kbd_CursorHome();
        break;
    case USB_KEY_DELETE:
        Kbd_Delete();
        break;
    case USB_KEY_END:
        Kbd_CursorEnd();
        break;
    case USB_KEY_RIGHT:
        Kbd_CursorRight();
        break;
    case USB_KEY_LEFT:
        Kbd_CursorLeft();
        break;
    case USB_KEY_DOWN:
        Kbd_CursorDown();
        break;
    case USB_KEY_UP:
        Kbd_CursorUp();
        break;
    case USB_KEY_KANA:
        Kbd_KanaKey();
        break;
    case USB_KEY_HENKAN:
        Kbd_HenkanKey();
        break;
    case USB_KEY_MUHENKAN:
        Kbd_MuhenkanKey();
        break;
    default:
        Kbd_TypeKey(gKbd->key);
        break;
    }
}



static s32 Kbd_ShiftHeld(void);
s32 Kbd_KanaInput(void);
s32 Kbd_ConvertRomaji(s32 kind);
s32 Kbd_CharAllowed(s8 *arg0);
s32 Str_InsertClip(char *dst, char *src, s32 pos, s32 max);
s32 Kbd_KindIs8To10(void);
extern u8 gKbdSetCharsets[];
extern char *gKbdHidKeypadWideStrs[];
extern char *gKbdHidNoRomaji[];
extern char *gKbdHidChars[];
extern char *gKbdHidWideStrs[];
extern char *gKbdHidKeypadChars[];
extern s32 gKbdShiftLetterPos;

/*
 * Types the USB key curKey (Kbd_TypeKey calls it while the pad is not
 * active). Types nothing in the gestures and phrase modes; commits the
 * kana first while converting. The Ro and Yen keys of a Japanese
 * keyboard (USB_KEY_INTL1, USB_KEY_INTL3) read table entries 0 and 1,
 * and Backslash reads the entry of USB_KEY_NONUS_HASH. Shift picks the
 * shifted tables; the keypad keys have tables of their own.
 * In the full-width sets and the half-width katakana sets a key goes into
 * the kana buffer as a two-byte string from gKbdHidWideStrs: the romaji
 * table (kind 4) under romaji input in the hiragana, katakana and symbol
 * modes or on a gestures page typing kana, else the table of the key
 * set's charset (gKbdSetCharsets). In the half-width katakana sets, kana
 * input types a one-byte kana straight into the text, and under romaji
 * input a key marked in gKbdHidNoRomaji commits the kana and types its
 * one-byte character from table 2.
 * Entry kind 8 refuses what Kbd_CharAllowed refuses, and accept bit 2
 * the full-width percent sign. A romaji-table letter gets mode + 1 (1 in
 * the symbol mode) as its second byte. A full-width space goes into the
 * text, and only for the Space key with no kana pending.
 * gKbdShiftLetterPos becomes kanaPos + 1 when Shift is held, and 0 when
 * typing without Shift at kanaPos 0 or with the cursor at or before it.
 * The string is inserted at kanaPos (the kana kept to 0x48 bytes); a
 * romaji-table key is then converted: in the half-width sets
 * Kbd_ConvertRomaji(2), committing the kana when it converts; else, only
 * while gKbdShiftLetterPos is 0, Kbd_ConvertRomaji with the mode (0 in
 * the symbol mode).
 * The other sets type one byte (a keypad key's from gKbdHidKeypadChars)
 * into the text at textPos, up to maxLen, after the accept filters
 * (digits; digits and '-'; digits and capitals; digits and "-*#"; no
 * '%'). '[', ']' and the half-width corner brackets are never typed.
 * Outside kinds 8-10 a backslash and braces are refused, the byte 0xA5
 * for kinds 1, 2 and 5, and with flags bit 3 (the symbol set barred)
 * also | ` $ < > ^; kinds 8-10 refuse ',' and bytes outside ' '..'~'.
 * Kind 15 refuses an apostrophe. After typing, blink is reset when
 * something was inserted, and the sound is SE_SYS_SELECT.
 */
void Kbd_TypeHidKey(void)
{
    char buf[4];
    u8 narrowStr[2];
    u8 shift;
    u8 kind;
    u8 key;
    s32 isKeypad;
    char *str;
    u8 narrowChar;
    s32 inserted;
    s32 isLetter;
    s32 isUpper;
    u8 acceptBits;
    u16 *pos;
    KbdEntry *kbd;

    isKeypad = 0;
    if (gKbd->mode == KBD_MODE_GESTURE || gKbd->mode == KBD_MODE_PHRASE1 || gKbd->mode == KBD_MODE_PHRASE2) {
        return;
    }
    if (gKbd->converting != 0) {
        Kbd_CommitKana();
    }
    key = gKbd->curKey;
    if (key >= USB_KEY_KP_SLASH && key < USB_KEY_NONUS_BACKSLASH) {
        isKeypad = 1;
    }
    if (key == USB_KEY_INTL1) {
        key = 0;
    }
    if (key == USB_KEY_INTL3) {
        key = 1;
    }
    if (key == USB_KEY_BACKSLASH) {
        key = 0x32;
    }
    shift = Kbd_ShiftHeld() != 0;
    kind = gKbdSetCharsets[gKbd->keySet];
    if (Kbd_SetIsWide() != 0 || gKbd->keySet == 6 || gKbd->keySet == 14) {
        if (isKeypad) {
            str = &gKbdHidKeypadWideStrs[0][(key - USB_KEY_KP_SLASH) * 2];
            goto use;
        }
        if (gKbd->keySet == 6 || gKbd->keySet == 14) {
            if (Kbd_KanaInput() == 0) {
                if (gKbdHidNoRomaji[0][key] == 0x31) {
                    Kbd_CommitKana();
                    kind = KBD_HID_ASCII;
                    narrowChar = gKbdHidChars[kind + shift * 3][key];
                    goto check;
                }
                kind = KBD_HID_ROMAJI;
                goto table;
            }
            narrowChar = gKbdHidChars[shift * 3][key];
            goto check;
        }
        if (Kbd_KanaInput() == 0 && (gKbd->mode <= 1U || gKbd->mode == KBD_MODE_SYMBOL || (gKbd->mode == KBD_MODE_GESTURE && (gKbd->gestureMode & KBD_GESTURE_KANA)))) {
            kind = 4;
        }
    table:
        str = &gKbdHidWideStrs[kind + shift * 5][key * 2];
    use:
        if (gKbd->kind == 8 && Kbd_CharAllowed((s8 *)str) == 0) {
            return;
        }
        if ((gKbd->accept & KBD_ACCEPT_NO_PERCENT) && strncmp(TXT(0x0046C8F8) /* a full-width percent sign */, str, 2) == 0) {
            return;
        }
        strncpy(buf, str, 2);
        if (kind == 4) {
            isLetter = 1;
            isUpper = 0;
            if (buf[0] >= 0x41 && buf[0] < 0x5B) {
                isUpper = 1;
            }
            if (isUpper == 0 && (buf[0] >= 0x61 && buf[0] < 0x7B) == 0) {
                isLetter = 0;
            }
            if (isLetter) {
                if (gKbd->mode == 3) {
                    buf[1] = 1;
                } else {
                    buf[1] = gKbd->mode + 1;
                }
            }
        }
        buf[2] = 0;
        if (strncmp(buf, TXT(0x0046C900) /* a full-width space */, 2) == 0) {
            if (gKbd->kana[0] != 0) {
                return;
            }
            if (key == USB_KEY_SPACE) {
                goto insert;
            }
            return;
        }
        if (shift == 0 && (kbd = gKbd, pos = &kbd->kanaPos, kbd->kanaPos == 0)) {
            gKbdShiftLetterPos = 0;
        } else if (shift) {
            kbd = gKbd;
            pos = &kbd->kanaPos;
            gKbdShiftLetterPos = kbd->kanaPos + 1;
        } else {
            kbd = gKbd;
            pos = &kbd->kanaPos;
            if (gKbdShiftLetterPos > kbd->kanaPos - 1) {
                gKbdShiftLetterPos = 0;
            }
        }
        inserted = Str_InsertClip((char *)kbd->kana, buf, *pos, 0x48);
        gKbd->kanaPos = gKbd->kanaPos + inserted;
        if (kind == 4) {
            if (gKbd->keySet == 6 || gKbd->keySet == 14) {
                if (Kbd_ConvertRomaji(KBD_KANA_HALF) != 0) {
                    Kbd_CommitKana();
                }
            } else if (gKbd->mode == 3) {
                if (gKbdShiftLetterPos == 0) {
                    Kbd_ConvertRomaji(KBD_KANA_HIRA);
                }
            } else if (gKbdShiftLetterPos == 0) {
                Kbd_ConvertRomaji(gKbd->mode);
            }
        }
        goto done;
    }
    if (isKeypad) {
        narrowChar = gKbdHidKeypadChars[0][key - 0x54];
    } else {
        narrowChar = gKbdHidChars[kind + shift * 3][key];
    }
check:
    acceptBits = gKbd->accept;
    if ((acceptBits & KBD_ACCEPT_DIGITS) && (narrowChar < 0x30 || narrowChar > 0x39)) {
        return;
    }
    if ((acceptBits & 0x20) && (narrowChar < 0x30 || narrowChar > 0x39) && narrowChar != 0x2D) {
        return;
    }
    if ((acceptBits & 0x40) && (narrowChar < 0x30 || narrowChar > 0x39) && (narrowChar < 0x41 || narrowChar > 0x5A)) {
        return;
    }
    if ((acceptBits & 0x80) && (narrowChar < 0x30 || narrowChar > 0x39) && narrowChar != 0x2D && narrowChar != 0x2A && narrowChar != 0x23) {
        return;
    }
    if ((acceptBits & 2) && narrowChar == 0x25) {
        return;
    }
    if (narrowChar != '[' && narrowChar != ']' && narrowChar != '\xA2' && narrowChar != '\xA3') {
        if (Kbd_KindIs8To10() == 0) {
            if (gKbd->kind == 1 || gKbd->kind == 5 || gKbd->kind == 2) {
                if (narrowChar == '\xA5') {
                    return;
                }
            }
            if (narrowChar == '\\') {
                return;
            }
            if (narrowChar == '{' || narrowChar == '}') {
                return;
            }
            if (gKbd->flags & KBD_SET_BIT(KBD_SET_SYMBOL)) {
                if (narrowChar == '|') {
                    return;
                }
                if (narrowChar == '`') {
                    return;
                }
                if (narrowChar == '$') {
                    return;
                }
                if (narrowChar == '<') {
                    return;
                }
                if (narrowChar == '>') {
                    return;
                }
                if (narrowChar == '|') {
                    return;
                }
                if (narrowChar == '^') {
                    return;
                }
                if (narrowChar == '`') {
                    return;
                }
            }
        } else {
            if (narrowChar == ',') {
                return;
            }
            if (narrowChar < 0x20 || narrowChar > 0x7E) {
                return;
            }
        }
        if (gKbd->kind == 15 && narrowChar == '\'') {
            return;
        }
        narrowStr[0] = narrowChar;
        narrowStr[1] = 0;
        strncpy(buf, (char *)narrowStr, 2);
insert:
        inserted = Str_InsertClip(gKbd->text, buf, gKbd->textPos, gKbd->maxLen);
        gKbd->textPos = gKbd->textPos + inserted;
done:
        if (inserted != 0) {
            gKbd->blink = 0;
        }
        gKbd->state = SE_SYS_SELECT;
    }
}


/* Nonzero while an Alt key is held (usbMods & USB_MOD_ALT). */
static s32 Kbd_AltHeld(void)
{
    return gKbd->usbMods & USB_MOD_ALT;
}

/* Nonzero while a Shift key is held (usbMods & USB_MOD_SHIFT). */
static s32 Kbd_ShiftHeld(void)
{
    return gKbd->usbMods & USB_MOD_SHIFT;
}

/* Nonzero while a Ctrl key is held (usbMods & USB_MOD_CTRL). */
static s32 Kbd_CtrlHeld(void)
{
    return gKbd->usbMods & USB_MOD_CTRL;
}

/*
 * Returns kanaInput: nonzero when the USB keys type kana directly (kana
 * input), 0 when letters go through the romaji table (Kbd_TypeHidKey).
 */
s32 Kbd_KanaInput(void)
{
    return gKbd->kanaInput;
}

/*
 * Toggles kanaInput (Alt with the Katakana/Hiragana key, Kbd_KanaKey)
 * and saves it (Kbd_SaveKanaInput).
 */
void Kbd_ToggleKanaInput(void)
{
    gKbd->kanaInput ^= 1;
    Kbd_SaveKanaInput();
}

void Kbd_ConvertOrCommit(void);
void Kbd_PressKey(KbdKey *key);
s32 Kbd_DoGesture(char *text, u16 *pos, s32 gesture);

/*
 * The Enter and keypad Enter keys. In the symbol mode converts or
 * commits (Kbd_ConvertOrCommit). On a phrase or gestures page
 * (Kbd_ArrowsOnPage) runs the key under the cursor: no key,
 * SE_SYS_REFUSE; a switch key, Kbd_PressKey; on the gestures page, its
 * gesture (Kbd_DoGesture); else, after Kbd_RefreshLayout, the submit
 * command key submits (Kbd_Submit) and another key inserts its phrase
 * (Kbd_InsertPhrase), during a phrase cycle also setting result to
 * KBD_RESULT_SUBMIT. Otherwise submits the text; for entry kind 8 it
 * then drops every two-byte character whose second byte is 0x60..0x79
 * or 0x81..0x9A (under lead byte 0x82 the full-width letters A-Z and
 * a-z; the first byte itself is not checked), stopping when
 * Sjis_CharSizeAt gives 0 or the end is passed, and puts textPos at the
 * end and kanaPos at 0. When it did not submit, convert or press a
 * switch key, the sound is SE_SYS_SEND if result is set, else
 * SE_SYS_SELECT.
 */
void Kbd_Decide(void)
{
    s32 done;
    KbdKey *cursorKey;
    u8 *textBytes;
    s32 charSize;
    s32 i;
    KbdEntry *kbd;
    u8 *cur;
    KbdChar *keyChar;
    s32 isLead;
    s32 dropChar;

    done = 0;
    if (gKbd->symbolMode != 0) {
        Kbd_ConvertOrCommit();
        done = 1;
    } else if (Kbd_ArrowsOnPage() == 1) {
        kbd = gKbd;
        cursorKey = kbd->key;
        keyChar = &cursorKey->ch;
        if (keyChar->code == 0) {
            kbd->state = SE_SYS_REFUSE;
            return;
        }
        if (keyChar->b[0] == 1) {
            Kbd_PressKey(cursorKey);
            done = 1;
        } else if (kbd->mode == KBD_MODE_GESTURE) {
            Kbd_DoGesture(kbd->text, &kbd->textPos, cursorKey->ch.b[1]);
        } else {
            Kbd_RefreshLayout();
            cursorKey = gKbd->key;
            if (cursorKey->ch.b[0] == 2 && cursorKey->ch.b[1] == 3) {
                done = 1;
                Kbd_Submit();
            } else {
                Kbd_InsertPhrase();
                if (gKbd->keyFlags & KBD_KEYF_CYCLING) {
                    gKbd->result = KBD_RESULT_SUBMIT;
                }
            }
        }
    } else {
        done = 1;
        Kbd_Submit();
        if (gKbd->kind == 8) {
            i = 0;
            textBytes = (u8 *)gKbd->text;
            while ((s8)textBytes[i] != 0) {
                cur = textBytes + i;
                dropChar = 0;
                isLead = (cur[0] >= 0x80 && cur[0] < 0xA0) || (cur[0] >= 0xE0 && cur[0] < 0x100);
                if (isLead) {
                    if (cur[1] >= 0x60 && cur[1] < 0x7A) {
                        dropChar = 1;
                    } else if (cur[1] >= 0x81 && cur[1] < 0x9B) {
                        dropChar = 1;
                    }
                    if (dropChar) {
                        charSize = Sjis_CharSizeAt(gKbd->text, i);
                        if (i >= (s32)strlen(gKbd->text) || charSize == 0) {
                            break;
                        }
                        /* text[i] and text[i + charSize] (text is at 0x4C),
                         * reached from gKbd + i: the game's code adds i to the
                         * pointer first. */
                        cur = (u8 *)gKbd + i;
                        cur[0x4C] = 0;
                        cur = (u8 *)gKbd + i;
                        strcat((char *)cur + 0x4C, (char *)cur + charSize + 0x4C);
                    } else {
                        i += 2;
                    }
                } else {
                    i += 1;
                }
            }
            gKbd->textPos = strlen(gKbd->text);
            gKbd->kanaPos = 0;
        }
    }
    if (done == 0) {
        if (gKbd->result != 0) {
            gKbd->state = SE_SYS_SEND;
        } else {
            gKbd->state = SE_SYS_SELECT;
        }
    }
}

void *memset(void *dst, s32 c, u32 n);
/*
 * The Escape key. While converting, leaves the conversion (converting
 * 0), resets blink and offGrid, and in the symbol mode empties the kana.
 * Else, with kana typed, empties it (kanaPos 0, its first 4 bytes
 * cleared). Else cancels the entry (result KBD_RESULT_CANCEL). The sound
 * is SE_SYS_CANCEL.
 */
void Kbd_Escape(void)
{
    u8 *convFlag = &gKbd->converting;

    if (*convFlag != 0) {
        *convFlag = 0;
        gKbd->blink = 0;
        gKbd->offGrid = 0;
        if (gKbd->symbolMode != 0) {
            gKbd->kanaPos = 0;
            gKbd->kana[0] = 0;
        }
    } else if (gKbd->kana[0] != 0) {
        gKbd->blink = 0;
        gKbd->kanaPos = 0;
        memset(gKbd->kana, 0, 4);
    } else {
        gKbd->result = KBD_RESULT_CANCEL;
    }
    gKbd->state = SE_SYS_CANCEL;
}

/*
 * The Space key (fromPad 0, from Kbd_HandleKey) and the pad's SQUARE
 * (fromPad 1, from Kbd_Run). While choosing a candidate (converting 1):
 * the next candidate, or the previous one with Shift held;
 * SE_SYS_SELECT. Otherwise, with kana typed, converts it (Kbd_Convert);
 * with none, the Space key is typed (Kbd_TypeKey; SE_SYS_SELECT when
 * the pad is active) and SQUARE is refused (SE_SYS_REFUSE).
 */
void Kbd_Space(s32 fromPad)
{
    if (gKbd->converting != 1) {
        if (gKbd->kana[0] == 0) {
            if (fromPad == 0) {
                Kbd_TypeKey(gKbd->key);
                if (gKbd->active == 1) {
                    gKbd->state = 0x16;
                }
            } else {
                gKbd->state = SE_SYS_REFUSE;
            }
        } else {
            Kbd_Convert();
        }
    } else {
        gKbd->state = 0x16;
        if (Kbd_ShiftHeld() != 0) {
            Kbd_PrevCand();
            return;
        }
        Kbd_NextCand();
    }
}

/*
 * The Backspace key. During a phrase or gesture cycle
 * (KBD_KEYF_CYCLING) cancels the entry (KBD_RESULT_CANCEL,
 * SE_SYS_CANCEL). While converting or without Ctrl, deletes back
 * (Kbd_DeleteBack). With Ctrl, drops everything before the cursor
 * (Str_DropFront) in the kana, when there is kana, else in the text,
 * moving that cursor to the start; SE_SYS_SELECT.
 */
void Kbd_Backspace(void)
{
    u16 *pos;

    if (gKbd->keyFlags & 2) {
        gKbd->result = -1;
        gKbd->state = 0x14;
        return;
    }
    if (gKbd->converting != 0 || Kbd_CtrlHeld() == 0) {
        Kbd_DeleteBack(1);
        return;
    }
    if (gKbd->kana[0] != 0) {
        pos = &gKbd->kanaPos;
        *pos = *pos - Str_DropFront((char *)gKbd->kana, gKbd->kanaPos);
    } else {
        pos = &gKbd->textPos;
        *pos = *pos - Str_DropFront(gKbd->text, gKbd->textPos);
    }
    gKbd->state = 0x16;
}

void Kbd_KanaDakuten(void);

/*
 * The dakuten key (USB 0x2F, the '@' key of a Japanese keyboard). Does
 * nothing while converting. Under kana input adds a dakuten
 * (Kbd_KanaDakuten); otherwise the key is typed (Kbd_TypeKey).
 */
void Kbd_DakutenKey(void)
{
    if (gKbd->converting == 0) {
        if (Kbd_KanaInput() != 0) {
            Kbd_KanaDakuten();
            return;
        }
        Kbd_TypeKey(gKbd->key);
    }
}

void Kbd_KanaHandakuten(void);

/*
 * The handakuten key (USB 0x30, the '[' key of a Japanese keyboard).
 * Does nothing while converting. Under kana input adds a handakuten
 * (Kbd_KanaHandakuten); otherwise the key is typed (Kbd_TypeKey).
 */
void Kbd_HandakutenKey(void)
{
    if (gKbd->converting == 0) {
        if (Kbd_KanaInput() != 0) {
            Kbd_KanaHandakuten();
            return;
        }
        Kbd_TypeKey(gKbd->key);
    }
}

void sceUsbKbSetLEDStatus(s32 a, s32 b);
/*
 * The Caps Lock key. On the alphabet sets, with Shift held, switches the
 * case unless accept bit 0 is set or the other case's set is barred: the
 * capitals sets (KBD_SET_WIDE_UPPER, KBD_SET_ASCII_UPPER) go to lower
 * case and turn the keyboard's LEDs off, the lower-case sets go to
 * capitals and turn the Caps Lock LED on (sceUsbKbSetLEDStatus). On the
 * kana, symbol, phrase and gestures sets, without Shift, switches to the
 * first set not barred of KBD_SET_WIDE_UPPER, KBD_SET_WIDE_LOWER,
 * KBD_SET_ASCII_UPPER and KBD_SET_ASCII_LOWER (nothing when all are)
 * and ends a phrase cycle; with Shift the set stays. Then updates the
 * mode, the page cursor and the layout; SE_SYS_SELECT. Does nothing on
 * sets 11, 12 and 16, or on the alphabet sets without Shift.
 */
void Kbd_CapsLock(void)
{
    switch (gKbd->keySet) {
    case KBD_SET_WIDE_UPPER:
    case KBD_SET_ASCII_UPPER:
        if (gKbd->accept & KBD_ACCEPT_FIXED_CASE) {
            return;
        }
        if (Kbd_ShiftHeld() == 0) {
            return;
        }
        if (gKbd->flags & (1 << (gKbd->keySet + 8))) {
            return;
        }
        gKbd->keySet |= 8;
        sceUsbKbSetLEDStatus(0, 0);
        break;
    case KBD_SET_WIDE_LOWER:
    case KBD_SET_ASCII_LOWER:
        if (gKbd->accept & 1) {
            return;
        }
        if (Kbd_ShiftHeld() == 0) {
            return;
        }
        if (gKbd->flags & (1 << (gKbd->keySet - 8))) {
            return;
        }
        gKbd->keySet ^= KBD_SET_SHIFTED;
        sceUsbKbSetLEDStatus(0, 2);
        break;
    case KBD_SET_HIRA:
    case KBD_SET_KATA:
    case KBD_SET_SYMBOL:
    case KBD_SET_PHRASE1:
    case KBD_SET_PHRASE2:
    case KBD_SET_HALF_KATA:
    case KBD_SET_SMALL_HIRA:
    case KBD_SET_SMALL_KATA:
    case KBD_SET_GESTURE:
    case KBD_SET_HALF_SMALL_KATA:
        if (Kbd_ShiftHeld() == 0) {
            s32 barred = gKbd->flags;
            if (!(barred & 4)) {
                gKbd->keySet = 2;
            } else if (!(barred & 0x400)) {
                gKbd->keySet = 10;
            } else if (!(barred & 0x80)) {
                gKbd->keySet = 7;
            } else if (!(barred & 0x8000)) {
                gKbd->keySet = 15;
            } else {
                return;
            }
            gKbd->keyFlags &= ~KBD_KEYF_CYCLING;
        }
        break;
    case KBD_SET_HALF_SYMBOL:
    case 12:
    default:
        return;
    }
    Kbd_UpdateMode();
    Kbd_HomePageCursor();
    Kbd_RefreshLayout();
    gKbd->state = SE_SYS_SELECT;
}

/*
 * Shift with F1..F12 (key, a USB code): inserts phrase number
 * key - USB_KEY_F1 of the first phrase page. Refused (SE_SYS_REFUSE)
 * while converting or with Ctrl held. Else sets KBD_KEYF_CYCLING and,
 * when Kbd_FindKind allows the page (else refused), records the page,
 * puts the cursor on cell number * 5 + 1 (column cell % 20, row
 * cell / 20), updates the mode, inserts that key's phrase
 * (Kbd_InsertPhrase, which replaces the text during a cycle) and
 * updates the symbol mode. The key stays in a0 across Kbd_CtrlHeld.
 */
void Kbd_ShiftFKey(u8 key)
{
    u8 fkeyIdx;
    u8 cell;

    if (gKbd->converting != 0 || Kbd_CtrlHeld() != 0) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    fkeyIdx = key - USB_KEY_F1;
    gKbd->keyFlags |= 2;
    if (Kbd_FindKind(KBD_MODE_PHRASE1, &gKbd->mode, &gKbd->keySet) == 0) {
        Kbd_SetPhrasePage(KBD_SET_PHRASE1);
        cell = fkeyIdx * 5;
        gKbd->col = (cell + 1) % 20;
        gKbd->row = (cell + 1) / 20;
        Kbd_UpdateMode();
        Kbd_InsertPhrase();
        Kbd_UpdateSymbolMode();
        return;
    }
    gKbd->state = 0x15;
}

/*
 * Ctrl with F1..F12: as Kbd_ShiftFKey, on the second phrase page;
 * refused while converting or with Shift held.
 */
void Kbd_CtrlFKey(u8 key)
{
    u8 fkeyIdx;
    u8 cell;

    if (gKbd->converting != 0 || Kbd_ShiftHeld() != 0) {
        gKbd->state = 0x15;
        return;
    }
    fkeyIdx = key - 0x3A;
    gKbd->keyFlags |= 2;
    if (Kbd_FindKind(KBD_MODE_PHRASE2, &gKbd->mode, &gKbd->keySet) == 0) {
        Kbd_SetPhrasePage(KBD_SET_PHRASE2);
        cell = fkeyIdx * 5;
        gKbd->col = (cell + 1) % 20;
        gKbd->row = (cell + 1) / 20;
        Kbd_UpdateMode();
        Kbd_InsertPhrase();
        Kbd_UpdateSymbolMode();
        return;
    }
    gKbd->state = 0x15;
}

void Kbd_ToggleShift(void);
void Kbd_CyclePhrase(void);
void Kbd_CycleGesture(void);
void Kbd_Confirm(void);
void Kbd_ConvertForm2(void);
extern s32 gKbdSettings;

/* Declared with an s32 parameter, as this file's callers use it: they pass
 * the set without masking it to a byte (with the definition's u8, Kbd_FKey
 * would mask it first). */
/* unit's own declaration: an s32 parameter, as the callers pass it */
s32 Kbd_ShiftPair(s32 i);

/*
 * F1..F12 (key, USB_KEY_F1..USB_KEY_F12). With Alt: when key 6 of the
 * layout has a character, does gesture key - USB_KEY_F1
 * (Kbd_DoGesture). With Ctrl: Kbd_CtrlFKey. With Shift: Kbd_ShiftFKey,
 * when the kind has phrases (kindConf + 0x30) and the first phrase page
 * is not barred. Otherwise: F1, F2 the hiragana and katakana modes
 * (Kbd_FKeyRow); F3 and F8 half-width katakana (Kbd_FindKind, then
 * KBD_SET_HALF_SMALL_KATA and Kbd_ToggleShift), committing the kana; F4
 * the full-width alphabet and F5 the ASCII one (F5 commits the kana), in
 * capitals while Caps Lock is on (gKbdSettings), keeping the cursor; F6
 * Kbd_Confirm; F7 Kbd_ConvertForm2; F9, F10 the phrase pages
 * (Kbd_OpenPhrases1, Kbd_OpenPhrases2); F11 the gestures page with the
 * cursor on column 5, row 0 (refused while converting, when barred, or
 * when the kind has no gestures, kindConf + 0x28); F12 cycles the
 * gestures on the gestures page, else the phrases. The keys that switch
 * sets end a phrase cycle (KBD_KEYF_CYCLING). Refusals: SE_SYS_REFUSE.
 */
void Kbd_FKey(u8 key)
{
    s8 pairSet;
    s32 fromSet;
    u8 x;
    u8 y;

    if (Kbd_AltHeld() != 0) {
        if (gKbd->layout->keys[6].ch.code != 0) {
            Kbd_DoGesture(gKbd->text, &gKbd->textPos, (u8)(key - 0x3A));
        }
    } else if (Kbd_CtrlHeld() != 0) {
        Kbd_CtrlFKey(key);
    } else if (Kbd_ShiftHeld() != 0) {
        if (*(s32 *)(gKbd->kindConf + 0x30) != 0
            && !(gKbd->flags & KBD_SET_BIT(KBD_SET_PHRASE1))) {
            Kbd_ShiftFKey(key);
        }
    } else {
        switch (key) {
        case USB_KEY_F1:
        case USB_KEY_F2:
            Kbd_FKeyRow(key);
            return;
        case USB_KEY_F3:
            if (Kbd_FindKind(KBD_SET_HALF_KATA, &gKbd->mode, &gKbd->keySet) == 0) {
                Kbd_CommitKana();
                gKbd->keyFlags &= ~2;
                gKbd->keySet = KBD_SET_HALF_SMALL_KATA;
                Kbd_ToggleShift();
                gKbd->state = SE_SYS_SELECT;
                return;
            }
            gKbd->state = SE_SYS_REFUSE;
            return;
        case USB_KEY_F4:
            fromSet = (gKbdSettings & 2) ? KBD_SET_WIDE_LOWER : 2;
            pairSet = Kbd_ShiftPair(fromSet);
            if (pairSet >= 0) {
                gKbd->keyFlags &= ~2;
                x = gKbd->col;
                y = gKbd->row;
                gKbd->keySet = pairSet;
                Kbd_UpdateMode();
                Kbd_HomePageCursor();
                Kbd_RefreshLayout();
                Kbd_UpdateSymbolMode();
                gKbd->state = 0x16;
                gKbd->col = x;
                gKbd->row = y;
                Kbd_LookupKey();
                return;
            }
            gKbd->state = 0x15;
            return;
        case USB_KEY_F5:
            fromSet = (gKbdSettings & USB_LED_CAPS_LOCK) ? KBD_SET_ASCII_LOWER : KBD_SET_ASCII_UPPER;
            pairSet = Kbd_ShiftPair(fromSet);
            if (pairSet >= 0) {
                Kbd_CommitKana();
                gKbd->keyFlags &= ~2;
                x = gKbd->col;
                y = gKbd->row;
                gKbd->keySet = pairSet;
                Kbd_UpdateMode();
                Kbd_HomePageCursor();
                Kbd_RefreshLayout();
                Kbd_UpdateSymbolMode();
                gKbd->state = 0x16;
                gKbd->col = x;
                gKbd->row = y;
                Kbd_LookupKey();
                return;
            }
            gKbd->state = 0x15;
            return;
        case USB_KEY_F6:
            Kbd_Confirm();
            return;
        case USB_KEY_F7:
            Kbd_ConvertForm2();
            return;
        case USB_KEY_F8:
            if (Kbd_FindKind(6, &gKbd->mode, &gKbd->keySet) == 0) {
                Kbd_CommitKana();
                gKbd->keyFlags &= ~2;
                gKbd->keySet = 0xE;
                Kbd_ToggleShift();
                gKbd->state = 0x16;
                return;
            }
            gKbd->state = 0x15;
            return;
        case USB_KEY_F9:
            gKbd->keyFlags &= ~2;
            Kbd_SetPhrasePage(4);
            Kbd_OpenPhrases1();
            return;
        case USB_KEY_F10:
            gKbd->keyFlags &= ~2;
            Kbd_SetPhrasePage(5);
            Kbd_OpenPhrases2();
            return;
        case USB_KEY_F11:
            if (gKbd->converting != 0 || (gKbd->flags & KBD_SET_BIT(KBD_SET_GESTURE))
                || *(s32 *)(gKbd->kindConf + 0x28) == 0) {
                gKbd->state = 0x15;
                return;
            }
            gKbd->keyFlags &= ~2;
            Kbd_SetPhrasePage(KBD_SET_GESTURE);
            gKbd->keySet = KBD_SET_GESTURE;
            Kbd_UpdateMode();
            gKbd->col = 5;
            gKbd->row = 0;
            Kbd_EnterGestures();
            Kbd_RefreshLayout();
            gKbd->state = 0x16;
            return;
        case USB_KEY_F12:
            switch (gKbd->keySet) {
            case KBD_SET_PHRASE1:
            case KBD_SET_PHRASE2:
            default:
                Kbd_CyclePhrase();
                return;
            case KBD_SET_GESTURE:
                Kbd_CycleGesture();
                return;
            }
        default:
            gKbd->state = 0x15;
            break;
        }
    }
}


/*
 * Switches to the mode of key - USB_KEY_F1 (F1 hiragana, F2 katakana;
 * Kbd_KanaKey passes those keys too) when Kbd_FindKind finds a set:
 * SE_SYS_SELECT, gestureMode and KBD_KEYF_CYCLING cleared, the mode,
 * the page cursor, the layout and the symbol mode updated, and in the
 * symbol mode Kbd_Convert. Else SE_SYS_REFUSE.
 */
void Kbd_FKeyRow(u8 key)
{
    if (Kbd_FindKind(key - 0x3A, &gKbd->mode, &gKbd->keySet) == 0) {
        gKbd->state = 0x16;
        gKbd->gestureMode = 0;
        gKbd->keyFlags &= (u8)~KBD_KEYF_CYCLING;
        Kbd_UpdateMode();
        Kbd_HomePageCursor();
        Kbd_RefreshLayout();
        Kbd_UpdateSymbolMode();
        if (gKbd->symbolMode != 0) {
            Kbd_Convert();
        }
    } else {
        gKbd->state = 0x15;
    }
}

/*
 * Empties the text and the kana buffer and sets textPos, kanaPos and
 * firstShown to 0.
 */
void Kbd_ClearText(void)
{
    gKbd->kana[0] = 0;
    gKbd->text[0] = 0;
    gKbd->kanaPos = 0;
    gKbd->textPos = 0;
    gKbd->firstShown = 0;
}

/*
 * Opens the first phrase page (F9, the page key, Kbd_CyclePhrase). When
 * not converting, the kind has phrases (kindConf + 0x30) and
 * Kbd_FindKind allows the page: records it (Kbd_SetPhrasePage), commits
 * the kana, clears gestureMode, updates the mode, the page cursor, the
 * layout and the symbol mode; SE_SYS_SELECT. When the page is barred,
 * clears gestureMode and cyclePos. Otherwise the sound is SE_SYS_REFUSE.
 */
void Kbd_OpenPhrases1(void)
{
    s32 state = 0x15;

    if (gKbd->converting == 0 && *(s32 *)(gKbd->kindConf + 0x30) != 0) {
        if (Kbd_FindKind(4, &gKbd->mode, &gKbd->keySet) == 0) {
            Kbd_SetPhrasePage(4);
            Kbd_CommitKana();
            gKbd->gestureMode = 0;
            Kbd_UpdateMode();
            Kbd_HomePageCursor();
            Kbd_RefreshLayout();
            Kbd_UpdateSymbolMode();
            state = 0x16;
        } else {
            gKbd->gestureMode = 0;
            gKbd->cyclePos = 0;
        }
    }
    gKbd->state = state;
}

/* As Kbd_OpenPhrases1, for the second phrase page (F10). */
void Kbd_OpenPhrases2(void)
{
    s32 state = 0x15;

    if (gKbd->converting == 0 && *(s32 *)(gKbd->kindConf + 0x30) != 0) {
        if (Kbd_FindKind(5, &gKbd->mode, &gKbd->keySet) == 0) {
            Kbd_SetPhrasePage(5);
            Kbd_CommitKana();
            gKbd->gestureMode = 0;
            Kbd_UpdateMode();
            Kbd_HomePageCursor();
            Kbd_RefreshLayout();
            Kbd_UpdateSymbolMode();
            state = 0x16;
        } else {
            gKbd->gestureMode = 0;
            gKbd->cyclePos = 0;
        }
    }
    gKbd->state = state;
}

s32 Kbd_KindIs8To10(void);

/*
 * F6. Entry kinds 8-10 outside the symbol mode commit the input
 * (Kbd_CommitInput), and the sound is then SE_SYS_REFUSE. Other kinds
 * need a full-width set (Kbd_SetIsWide) and kana typed, else refused.
 * Not converting: the symbol mode converts (Kbd_Convert) and returns;
 * else conversion starts (Kbd_StartConvert): case candidates are counted
 * and listed; otherwise the segment grows while ConvCmd_GrowSegment
 * gives a count (candCount the last), then with more than one candidate
 * ConvCmd_SegmentForm1 is applied and repeats skipped
 * (Kbd_SkipSameCands), else the next candidate is taken (candId); the
 * candidates are counted and listed (Kbd_InitCandList). While
 * converting, outside the symbol mode and without case candidates, the
 * same from the segment step. SE_SYS_SELECT when it acted, else
 * SE_SYS_REFUSE.
 */
void Kbd_Confirm(void)
{
    s32 next = 0x15;
    s32 count;

    if (Kbd_KindIs8To10() == 1) {
        if (gKbd->symbolMode == 0) {
            Kbd_CommitInput();
            goto end;
        }
    } else {
        if (Kbd_SetIsWide() == 0) {
            goto end;
        }
        if (gKbd->kana[0] == 0) {
            goto end;
        }
    }
    if (gKbd->converting == 0) {
        if (gKbd->symbolMode != 0) {
            Kbd_Convert();
            return;
        }
        Kbd_StartConvert();
        if (gKbd->caseCands != 0) {
            gKbd->candCount = 0;
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
        } else {
            while ((count = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest)) != 0) {
                gKbd->candCount = count;
            }
            if (gKbd->candCount > 1) {
                ConvCmd_SegmentForm1(gKbd->cand, gKbd->convRest);
                Kbd_SkipSameCands();
                gKbd->candCount = Kbd_CountCands();
                Kbd_InitCandList();
            } else {
                gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
                gKbd->candCount = Kbd_CountCands();
                Kbd_InitCandList();
            }
        }
        next = 0x16;
    } else if (gKbd->symbolMode == 0 && gKbd->caseCands == 0) {
        if (gKbd->candCount > 1) {
            ConvCmd_SegmentForm1(gKbd->cand, gKbd->convRest);
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
        } else {
            gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
        }
        next = 0x16;
    }
end:
    gKbd->state = next;
}

/*
 * The Muhenkan key (Kbd_MuhenkanKey passes useForm2 1 on the first
 * press, then 0 and 1 in turn). Same checks as Kbd_Confirm. Not
 * converting: the symbol mode converts (Kbd_Convert) and returns; else
 * conversion starts and, with case candidates, takes the first
 * (CaseCands_First) and goes to the end step; otherwise grows the
 * segment fully, and with useForm2 0 applies ConvCmd_SegmentForm1 (more
 * than one candidate) or takes the next candidate. Then, outside the
 * symbol mode (else refused): case candidates take the first again;
 * useForm2 1 applies ConvCmd_SegmentForm2 and skips repeats; 0 applies
 * ConvCmd_SegmentForm1 (a second time on a fresh start). The end step:
 * candCount 1, the list built, converting 2 (choosing segments),
 * SE_SYS_SELECT.
 */
void Kbd_CycleSegmentForm(s32 useForm2)
{
    s32 state = 0x15;
    s32 count;

    if (Kbd_KindIs8To10() == 1) {
        if (gKbd->symbolMode == 0) {
            Kbd_CommitInput();
            goto end;
        }
    } else if (Kbd_SetIsWide() == 0 || gKbd->kana[0] == 0) {
        goto end;
    }
    if (gKbd->converting == 0) {
        if (gKbd->symbolMode != 0) {
            Kbd_Convert();
            return;
        }
        Kbd_StartConvert();
        if (gKbd->caseCands != 0) {
            CaseCands_First(gKbd->cand);
            goto done;
        }
        if (useForm2 != 0) {
            count = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
            while (count != 0) {
                gKbd->candCount = count;
                count = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
            }
        } else {
            while ((count = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest)) != 0) {
                gKbd->candCount = count;
            }
            if (gKbd->candCount > 1) {
                ConvCmd_SegmentForm1(gKbd->cand, gKbd->convRest);
            } else {
                gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
            }
        }
    }
    if (gKbd->symbolMode != 0) {
        goto end;
    }
    if (gKbd->caseCands != 0) {
        CaseCands_First(gKbd->cand);
    } else if (useForm2 != 0) {
        ConvCmd_SegmentForm2(gKbd->cand, gKbd->convRest);
        Kbd_SkipSameCands();
    } else {
        ConvCmd_SegmentForm1(gKbd->cand, gKbd->convRest);
    }
done:
    gKbd->candCount = 1;
    Kbd_InitCandList();
    gKbd->converting = 2;
    state = 0x16;
end:
    gKbd->state = state;
}

/*
 * F7. Same checks as Kbd_Confirm. Not converting: the symbol mode
 * converts (Kbd_Convert) and returns; else conversion starts and the
 * segment is grown fully, or case candidates are counted and listed.
 * Then, outside the symbol mode and without case candidates, applies
 * ConvCmd_SegmentForm2, skips repeated candidates, counts and lists the
 * candidates: SE_SYS_SELECT. Otherwise SE_SYS_REFUSE (also after case
 * candidates were listed).
 */
void Kbd_ConvertForm2(void)
{
    s32 state = 0x15;
    s32 count;

    if (Kbd_KindIs8To10() == 1) {
        if (gKbd->symbolMode == 0) {
            Kbd_CommitInput();
            goto end;
        }
    } else if (Kbd_SetIsWide() == 0 || gKbd->kana[0] == 0) {
        goto end;
    }
    if (gKbd->converting == 0) {
        if (gKbd->symbolMode != 0) {
            Kbd_Convert();
            return;
        }
        Kbd_StartConvert();
        if (gKbd->caseCands == 0) {
            count = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
            while (count != 0) {
                gKbd->candCount = count;
                count = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
            }
        } else {
            gKbd->candCount = 0;
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
        }
    }
    if (gKbd->symbolMode == 0 && gKbd->caseCands == 0) {
        ConvCmd_SegmentForm2(gKbd->cand, gKbd->convRest);
        Kbd_SkipSameCands();
        gKbd->candCount = Kbd_CountCands();
        Kbd_InitCandList();
        state = 0x16;
    }
end:
    gKbd->state = state;
}

void Kbd_PrevSegment(void);

/*
 * The Home key. Before conversion puts the cursor at the start of the
 * kana, when there is kana, or of the text: SE_SYS_SELECT when it moved,
 * else no sound (KBD_SOUND_NONE). While converting steps back through
 * the segments (Kbd_PrevSegment) until convDone is empty, leaving the
 * sound alone. Resets blink.
 */
void Kbd_CursorHome(void)
{
    s32 state = KBD_SOUND_NONE;
    u16 *kanaPos;
    u16 *textPos;

    if (gKbd->converting == 0) {
        if (gKbd->kana[0] != 0) {
            kanaPos = &gKbd->kanaPos;
            if (*kanaPos != 0) {
                *kanaPos = 0;
                state = SE_SYS_SELECT;
            }
        } else {
            textPos = &gKbd->textPos;
            if (*textPos != 0) {
                *textPos = 0;
                state = 0x16;
            }
        }
    } else {
        for (;;) {
            if (gKbd->convDone[0] == 0) {
                gKbd->blink = 0;
                return;
            }
            Kbd_PrevSegment();
        }
    }
    gKbd->blink = 0;
    gKbd->state = state;
}

/*
 * The Delete key. Refused while converting. Else SE_SYS_SELECT, and
 * deletes the character at the cursor (Kbd_DeleteChar), or with Ctrl
 * cuts the kana, when there is kana, or the text at its cursor
 * (Str_CutAt).
 */
void Kbd_Delete(void)
{
    if (gKbd->converting != 0) {
        gKbd->state = SE_SYS_REFUSE;
        return;
    }
    gKbd->state = 0x16;
    if (Kbd_CtrlHeld() == 0) {
        Kbd_DeleteChar();
        return;
    }
    if (gKbd->kana[0] != 0) {
        Str_CutAt((char *)gKbd->kana, gKbd->kanaPos);
        return;
    }
    Str_CutAt(gKbd->text, gKbd->textPos);
}

/*
 * Ends str at byte len when len is inside it, and then resets the
 * keyboard cursor's blink.
 */
void Str_CutAt(char *str, s32 len)
{
    if (len < strlen(str)) {
        str[len] = 0;
        gKbd->blink = 0;
    }
}

/*
 * The End key. Before conversion puts the cursor at the end of the kana,
 * when there is kana, or of the text: SE_SYS_SELECT when it moved, else
 * no sound (KBD_SOUND_NONE). While converting steps on through the
 * segments (Kbd_NextSegment) until convRest is empty, leaving the sound
 * alone. Either way kanaPos ends at the kana's end and blink is reset.
 */
void Kbd_CursorEnd(void)
{
    KbdEntry *kbd = gKbd;
    s32 state = -1;
    s32 pos;
    char *kana;

    if (kbd->converting == 0) {
        kana = (char *)kbd->kana;
        if (kbd->kana[0] != 0) {
            pos = kbd->kanaPos;
            if ((u32)pos < (u32)strlen(kana)) {
                gKbd->kanaPos = strlen((char *)kbd->kana);
                state = 0x16;
            }
        } else {
            pos = kbd->textPos;
            if (pos < (u16)strlen(kbd->text)) {
                gKbd->textPos = strlen(kbd->text);
                state = 0x16;
            }
        }
    } else {
        for (;;) {
            if (gKbd->convRest[0] == 0) {
                gKbd->kanaPos = strlen((char *)gKbd->kana);
                gKbd->blink = 0;
                return;
            }
            Kbd_NextSegment();
        }
    }
    gKbd->kanaPos = strlen((char *)gKbd->kana);
    gKbd->blink = 0;
    gKbd->state = state;
}

/*
 * Steps the key cursor in direction dir (0 left, 1 right, wrapping
 * between columns 1 and 20; 2 up, 3 down, stopping at rows 0 and 3)
 * until it is on another key (keyIdx, Kbd_LookupKey). Sound
 * SE_SYS_SCROLL. The arrow keys use it on the phrase and gestures pages.
 */
void Kbd_MoveOffKey(s16 dir)
{
    s8 startIdx;
    u8 *colPtr;
    u8 *rowPtr;

    startIdx = gKbd->keyIdx;
    do {
        if (dir == KBD_DIR_LEFT) {
            colPtr = &gKbd->col;
            if (*colPtr < 2) {
                *colPtr = 20;
            } else {
                *colPtr = *colPtr - 1;
            }
        } else if (dir == 1) {
            gKbd->col += 1;
            colPtr = &gKbd->col;
            if (*colPtr > 20) {
                *colPtr = 1;
            }
        }
        if (dir == 2) {
            rowPtr = &gKbd->row;
            if (*rowPtr <= 0) {
                *rowPtr = 0;
                break;
            }
            *rowPtr = *rowPtr - 1;
        } else if (dir == 3) {
            rowPtr = &gKbd->row;
            if (*rowPtr == 3) {
                break;
            }
            *rowPtr = *rowPtr + 1;
        }
        Kbd_LookupKey();
    } while (startIdx == gKbd->keyIdx);
    gKbd->state = SE_SYS_SCROLL;
}

/*
 * 1 when the USB arrow keys and Enter work the key cursor (and the
 * cursor is drawn): the pad is not active, nothing is being converted,
 * and the mode is a phrase page, or the gestures page while it does not
 * type kana (KBD_GESTURE_KANA). Else 0.
 */
s32 Kbd_ArrowsOnPage(void)
{
    s32 curMode;

    if (gKbd->active == 0) {
        if (gKbd->converting != 0) {
            return 0;
        }
        curMode = gKbd->mode;
        if (curMode == KBD_MODE_GESTURE && !(gKbd->gestureMode & KBD_GESTURE_KANA)) {
            return 1;
        }
        if (curMode == 4 || curMode == 5) {
            return 1;
        }
    }
    return 0;
}


/*
 * The Right arrow (the pad's handling calls it too). On a phrase or
 * gestures page (Kbd_ArrowsOnPage) moves the key cursor right
 * (Kbd_MoveOffKey) and ends a cycle (KBD_KEYF_CYCLING). Before
 * conversion moves the cursor one character (2 bytes) in the kana, when
 * there is kana, else one character in the text (Sjis_CharSizeAt); with
 * Ctrl to the end. SE_SYS_SCROLL when it moved, no sound at the end
 * (KBD_SOUND_NONE). While converting: nothing for case candidates; with
 * the pad inactive and Shift not held, the next segment
 * (Kbd_NextSegment, SE_SYS_SCROLL); else lengthens the segment
 * (ConvCmd_GrowSegment): when that gives candidates, back to choosing a
 * candidate (converting 1), repeats skipped and the list rebuilt,
 * SE_SYS_SCROLL; else no sound.
 */
void Kbd_CursorRight(void)
{
    KbdEntry *kbd;
    s32 count;
    u8 *kanaBuf;
    u16 *pos;

    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(1);
        gKbd->keyFlags &= ~2;
        return;
    }
    kbd = gKbd;
    if (kbd->converting == 0) {
        kanaBuf = kbd->kana;
        if (*kanaBuf != 0) {
            if (kbd->kanaPos < (u32)strlen((char *)kanaBuf)) {
                if (Kbd_CtrlHeld() == 0) {
                    gKbd->kanaPos += 2;
                } else {
                    gKbd->kanaPos = strlen((char *)gKbd->kana);
                }
                gKbd->blink = 0;
                gKbd->state = 0x17;
                return;
            }
            kbd->state = KBD_SOUND_NONE;
            return;
        }
        if (kbd->textPos < (u16)strlen(kbd->text)) {
            if (Kbd_CtrlHeld() == 0) {
                pos = &gKbd->textPos;
                *pos = *pos + Sjis_CharSizeAt(gKbd->text, *pos);
            } else {
                gKbd->textPos = strlen(gKbd->text);
            }
            gKbd->blink = 0;
            gKbd->state = 0x17;
            return;
        }
        kbd->state = -1;
        return;
    }
    if (kbd->caseCands == 1) {
        kbd->state = -1;
        return;
    }
    if (kbd->active == 0 && Kbd_ShiftHeld() == 0) {
        gKbd->state = 0x17;
        Kbd_NextSegment();
        return;
    }
    count = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
    if (count > 0) {
        gKbd->converting = 1;
        gKbd->candCount = count;
        Kbd_SkipSameCands();
        gKbd->candCount = Kbd_CountCands();
        Kbd_InitCandList();
        gKbd->state = 0x17;
    } else {
        gKbd->state = -1;
    }
}


s32 Sjis_PrevCharSize(u8 *s, s32 pos);
s32 ConvCmd_ShrinkSegment(char *a, char *b);

/*
 * The left arrow key (also the pad's left). When the arrows move on the
 * key page (Kbd_ArrowsOnPage 1), moves the keyboard cursor left off its
 * key (Kbd_MoveOffKey) and clears KBD_KEYF_CYCLING. Before conversion,
 * moves the text cursor one character left: by 2 in kana while kana is
 * pending, else in text by Sjis_PrevCharSize; with Ctrl held, to the
 * start. Sound SE_SYS_SCROLL, or none when already at the start. While
 * converting: no sound for case candidates; with the keyboard not active
 * and Shift not held, goes to the previous segment (Kbd_PrevSegment);
 * otherwise shrinks the segment (ConvCmd_ShrinkSegment) and, when that
 * gives candidates, keeps the shown one, counts and lists them
 * (SE_SYS_SCROLL), else no sound.
 * `*pos = *pos - Sjis_PrevCharSize(...)` is written out: `*pos -= ...`
 * masks the result to u16 first.
 * (The existing comment calls this Backspace and says it deletes; it
 * only moves the cursor.)
 */
void Kbd_CursorLeft(void)
{
    s32 cands;

    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(KBD_DIR_LEFT);
        gKbd->keyFlags &= ~KBD_KEYF_CYCLING;
        return;
    }
    if (gKbd->converting == 0) {
        if (gKbd->kana[0] != 0) {
            if (gKbd->kanaPos != 0) {
                if (Kbd_CtrlHeld() == 0) {
                    gKbd->kanaPos -= 2;
                } else {
                    gKbd->kanaPos = 0;
                }
                gKbd->blink = 0;
                gKbd->state = SE_SYS_SCROLL;
            } else {
                gKbd->state = KBD_SOUND_NONE;
            }
        } else {
            if (gKbd->textPos != 0) {
                if (Kbd_CtrlHeld() == 0) {
                    u16 *pos = &gKbd->textPos;
                    *pos = *pos - Sjis_PrevCharSize((u8 *)gKbd->text, *pos);
                } else {
                    gKbd->textPos = 0;
                }
                gKbd->blink = 0;
                gKbd->state = SE_SYS_SCROLL;
            } else {
                gKbd->state = KBD_SOUND_NONE;
            }
        }
    } else {
        if (gKbd->caseCands == 1) {
            gKbd->state = KBD_SOUND_NONE;
            return;
        }
        if (gKbd->active == 0 && Kbd_ShiftHeld() == 0) {
            gKbd->state = SE_SYS_SCROLL;
            Kbd_PrevSegment();
            return;
        }
        cands = ConvCmd_ShrinkSegment(gKbd->cand, gKbd->convRest);
        if (cands > 0) {
            gKbd->converting = 1;
            gKbd->candCount = cands;
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
            gKbd->state = SE_SYS_SCROLL;
        } else {
            gKbd->state = KBD_SOUND_NONE;
        }
    }
}

/*
 * The up arrow key. When the arrows move on the key page, moves the
 * keyboard cursor up off its key and clears KBD_KEYF_CYCLING; otherwise,
 * while converting is 1, shows the previous candidate (Kbd_PrevCand) with
 * SE_SYS_SCROLL.
 */
void Kbd_CursorUp(void)
{
    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(KBD_DIR_UP);
        gKbd->keyFlags = (u8) (gKbd->keyFlags & (u8)~KBD_KEYF_CYCLING);
        return;
    }
    if (gKbd->converting == 1) {
        gKbd->state = SE_SYS_SCROLL;
        Kbd_PrevCand();
    }
}

/*
 * The down arrow key. When the arrows move on the key page, moves the
 * keyboard cursor down off its key and clears KBD_KEYF_CYCLING; otherwise,
 * while converting is 1, shows the next candidate (Kbd_NextCand) with
 * SE_SYS_SCROLL.
 */
void Kbd_CursorDown(void)
{
    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(KBD_DIR_DOWN);
        gKbd->keyFlags = (u8) (gKbd->keyFlags & (u8)~KBD_KEYF_CYCLING);
        return;
    }
    if (gKbd->converting == 1) {
        gKbd->state = SE_SYS_SCROLL;
        Kbd_NextCand();
    }
}

/*
 * The USB keyboard's katakana/hiragana key (HID 0x88). With Alt held,
 * toggles kana input (Kbd_ToggleKanaInput). Otherwise, unless the
 * hiragana or the katakana set is barred (KbdEntry.flags), switches to
 * the hiragana mode, or with Shift held the katakana mode, through
 * Kbd_FKeyRow as F1 or F2 would. Sound SE_SYS_SELECT; none when barred.
 */
void Kbd_KanaKey(void)
{
    s32 flags;

    if (Kbd_AltHeld() != 0) {
        Kbd_ToggleKanaInput();
        gKbd->state = SE_SYS_SELECT;
        return;
    }
    flags = gKbd->flags;
    if ((flags & KBD_SET_BIT(KBD_SET_KATA)) || (flags & KBD_SET_BIT(KBD_SET_HIRA))) {
        return;
    }
    if (Kbd_ShiftHeld() != 0) {
        Kbd_FKeyRow(USB_KEY_F2);
    } else {
        Kbd_FKeyRow(USB_KEY_F1);
    }
    gKbd->state = SE_SYS_SELECT;
}

/*
 * The henkan (convert) key (HID 0x8A). For entry kinds 8-10 outside the
 * symbol mode, commits the input (Kbd_CommitInput). Before conversion:
 * in the symbol mode, Kbd_Convert; else, with kana pending, starts the
 * conversion and lists the candidates (SE_SYS_SELECT). While converting
 * is 1: the next candidate, or with Shift held the previous one
 * (SE_SYS_SELECT). Nothing while choosing segments (converting 2).
 */
void Kbd_HenkanKey(void)
{
    if (Kbd_KindIs8To10() == 1 && gKbd->symbolMode == 0) {
        Kbd_CommitInput();
        return;
    }
    if (gKbd->converting == 0) {
        if (gKbd->symbolMode != 0) {
            Kbd_Convert();
        } else if (gKbd->kana[0] != 0) {
            Kbd_StartConvert();
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
            gKbd->state = SE_SYS_SELECT;
        }
    } else if (gKbd->converting == 1) {
        gKbd->state = SE_SYS_SELECT;
        if (Kbd_ShiftHeld() != 0) {
            Kbd_PrevCand();
        } else {
            Kbd_NextCand();
        }
    }
}

/*
 * The muhenkan key (HID 0x8B). With a full-width key set (Kbd_SetIsWide)
 * and kana pending: SE_SYS_SELECT, clears KBD_KEYF_CYCLING and runs
 * Kbd_CycleSegmentForm with cyclePos as its flag. The first press (when
 * converting is not 2) sets cyclePos to 1; each press while converting
 * is 2 flips it between 1 and 0, so the two forms alternate.
 */
void Kbd_MuhenkanKey(void)
{
    if (Kbd_SetIsWide() != 0 && gKbd->kana[0] != 0) {
        gKbd->state = SE_SYS_SELECT;
        gKbd->keyFlags &= (u8)~KBD_KEYF_CYCLING;
        if (gKbd->converting != 2) {
            gKbd->cyclePos = 1;
        } else {
            gKbd->cyclePos += 1;
            gKbd->cyclePos &= 1;
        }
        if (gKbd->cyclePos != 0) {
            Kbd_CycleSegmentForm(1);
        } else {
            Kbd_CycleSegmentForm(0);
        }
    }
}

void Kbd_CommitKana(void);
void Kbd_NextCandidate(void);

/*
 * With a full-width key set and kana pending: SE_SYS_SELECT, then commits
 * the kana into the text (Kbd_CommitKana), or while converting, runs
 * Kbd_NextCandidate.
 */
void Kbd_CommitInput(void)
{
    if (Kbd_SetIsWide() != 0 && gKbd->kana[0] != 0) {
        gKbd->state = SE_SYS_SELECT;
        if (gKbd->converting == 0) {
            Kbd_CommitKana();
        } else {
            Kbd_NextCandidate();
        }
    }
}

s32 Sjis_CharSizeAt(char *s, s32 pos);

/*
 * Deletes the character at the cursor (the Delete key without Ctrl; the
 * caller has set SE_SYS_SELECT). Only before conversion: while kana is
 * pending, the 2-byte pair at kanaPos in kana; else the character at
 * textPos in text (Sjis_CharSizeAt bytes). At the end of the string it
 * plays SE_SYS_REFUSE instead. Resets the cursor blink.
 */
void Kbd_DeleteChar(void)
{
    char *str;
    s32 charSize;
    s32 pos;

    if (gKbd->converting == 0) {
        str = (char *)gKbd->kana;
        if (gKbd->kana[0] != 0) {
            pos = gKbd->kanaPos;
            charSize = 2;
        } else {
            pos = gKbd->textPos;
            str = gKbd->text;
            charSize = Sjis_CharSizeAt(str, pos);
        }
        if (strlen(str) <= pos) {
            gKbd->state = SE_SYS_REFUSE;
            return;
        }
        if (charSize != 0) {
            str[pos] = 0;
            strcat(str + pos, str + pos + charSize);
            gKbd->blink = 0;
        }
    }
}

/*
 * The hankaku/zenkaku key (HID 0x35). Clears KBD_KEYF_CYCLING. Without
 * Alt, switches the width of the key set (Kbd_ToggleWidth). With Alt:
 * SE_SYS_REFUSE when both ASCII sets are barred; else toggles keyFlags
 * bit 1 (half-width): on, saves keySet in prevKeySet and switches to the
 * ASCII lower-case set, or to the capitals when lower case is barred;
 * off, restores prevKeySet. Then updates the mode, homes the page
 * cursor, refreshes the layout and the symbol mode, SE_SYS_SELECT.
 */
void Kbd_ZenkakuKey(void)
{
    s32 flags;

    gKbd->keyFlags &= (u8)~KBD_KEYF_CYCLING;
    if (Kbd_AltHeld() != 0) {
        flags = gKbd->flags;
        if ((flags & KBD_SET_BIT(KBD_SET_ASCII_UPPER)) && (flags & KBD_SET_BIT(KBD_SET_ASCII_LOWER))) {
            gKbd->state = SE_SYS_REFUSE;
        } else {
            gKbd->keyFlags ^= 1;
            if (gKbd->keyFlags & 1) {
                gKbd->prevKeySet = gKbd->keySet;
                if (gKbd->flags & KBD_SET_BIT(KBD_SET_ASCII_LOWER)) {
                    gKbd->keySet = KBD_SET_ASCII_UPPER;
                } else {
                    gKbd->keySet = KBD_SET_ASCII_LOWER;
                }
            } else {
                gKbd->keySet = gKbd->prevKeySet;
            }
            Kbd_UpdateMode();
            Kbd_HomePageCursor();
            Kbd_RefreshLayout();
            Kbd_UpdateSymbolMode();
            gKbd->state = SE_SYS_SELECT;
        }
    } else {
        Kbd_ToggleWidth();
    }
}

/*
 * Removes the first count bytes of str (the rest moves to the front) and
 * resets the cursor blink; returns count. Nothing when count is 0. Seen:
 * called with the kana or text cursor, so it deletes all before it.
 */
s32 Str_DropFront(char *str, s32 count)
{
    if (count == 0) {
        return 0;
    }
    *str = 0;
    strcat(str, str + count);
    gKbd->blink = 0;
    return count;
}

extern char gSmallTsuHira[];
extern char gSmallTsuKata[];
extern char gSmallTsuHalf[];

/*
 * Converts typed romaji into kana in place in KbdEntry.kana. typed is the
 * romaji to try, two bytes per typed character (the first the letter),
 * in a copy of the bytes that end at kanaPos; kind picks the kana of the
 * RomajiEntry (0 hiragana, 1 katakana, 2 half-width katakana).
 * Returns 0 when typed is empty, holds a Shift-JIS lead byte, or (its
 * capitals lowered) matches no gRomajiKanaTbl entry. On a match, the
 * romaji in kana becomes the entry's kana, the text after the cursor is
 * kept (first moved up a byte, written with raw offsets from gKbd:
 * kana[j] = kana[j - 1]), and kanaPos moves by the length change;
 * returns 1. When the pair before typed in the copy equals its first
 * pair (a doubled consonant), the pair before the romaji in kana becomes
 * the kind's small tsu (gSmallTsuHira, gSmallTsuKata, gSmallTsuHalf; the
 * half-width one is one byte, and the pad byte after it is removed).
 */
s32 Kbd_RomajiToKana(s32 kind, char *typed)
{
    s32 i;
    s32 len;
    s32 nChars;
    char buf[8];
    s32 j;
    s32 size;
    char *out;
    char *src;
    char *tblStr;
    u8 *kbdBytes;
    RomajiEntry *entry;
    KbdEntry *kbd;
    s32 lead;

    len = strlen(typed);
    if (len == 0) {
        return 0;
    }
    out = buf;
    for (src = typed; *src != 0; src += 2, out++) {
        lead = (((u8 *)src)[0] >= 0x80 && ((u8 *)src)[0] < 0xA0) || (((u8 *)src)[0] >= 0xE0 && ((u8 *)src)[0] < 0x100);
        if (lead) {
            return 0;
        }
        *out = *src;
        if (*out >= 0x41 && *out <= 0x5A) {
            *out += 0x20;
        }
    }
    *out = 0;
    nChars = strlen(buf);
    for (i = 0, entry = gRomajiKanaTbl; i < ROMAJI_TBL_COUNT; i++, entry++) {
        tblStr = entry->romaji;
        size = strlen(tblStr);
        if (size == nChars && strncmp(buf, tblStr, nChars) == 0) {
            kbd = gKbd;
            kbd->kana[gKbd->kanaPos - len] = 0;
            tblStr = gRomajiKanaTbl[i].kana[kind];
            for (j = 0x7E; gKbd->kanaPos < j; j--) {
                /* kana[j] = kana[j - 1] (kana is at 0x168): the game's
                 * code adds j to gKbd first. */
                kbdBytes = (u8 *)gKbd + j;
                kbdBytes[0x168] = kbdBytes[0x167];
            }
            strcat((char *)gKbd->kana, tblStr);
            strcat((char *)gKbd->kana, (char *)&gKbd->kana[gKbd->kanaPos + 1]);
            if (strncmp(typed, typed - 2, 2) == 0) {
                switch (kind) {
                case 0:
                    strncpy((char *)&gKbd->kana[gKbd->kanaPos - len - 2], gSmallTsuHira, 2);
                    break;
                case 1:
                    strncpy((char *)&gKbd->kana[gKbd->kanaPos - len - 2], gSmallTsuKata, 2);
                    break;
                case 2:
                    strncpy((char *)&gKbd->kana[gKbd->kanaPos - len - 2], gSmallTsuHalf, 1);
                    kbd = gKbd;
                    kbd->kana[gKbd->kanaPos - len - 1] = 0;
                    size = gKbd->kanaPos - len;
                    strcat((char *)&gKbd->kana[size - 1], (char *)&gKbd->kana[size]);
                    break;
                }
            }
            size = strlen(tblStr);
            gKbd->kanaPos = gKbd->kanaPos - (len - size);
            return 1;
        }
    }
    return 0;
}

extern char *gKanaNChars[];

/*
 * Converts the romaji just typed before the cursor in kana: copies up to
 * the 8 bytes before kanaPos (four typed characters) right-aligned into a
 * zeroed buffer and tries the last three, two, then one character with
 * Kbd_RomajiToKana(kind). Failing that, when the character before the
 * last is n or N and the last is not y or Y, that n's pair becomes the
 * kind's kana N (gKanaNChars). Returns 1 when something was converted,
 * else 0. kind: 0 hiragana, 1 katakana, 2 half-width katakana.
 */
s32 Kbd_ConvertRomaji(s32 kind)
{
    char buf[10];
    s32 pos;
    s32 skip;
    s32 copyLen;

    memset(buf, 0, 10);
    pos = gKbd->kanaPos;
    skip = 8 - pos;
    if (skip < 0) {
        skip = 0;
    }
    copyLen = 8 - skip;
    strncpy(&buf[skip], (char *)&gKbd->kana[pos - copyLen], copyLen);
    if (Kbd_RomajiToKana(kind, &buf[2]) != 0) {
        return 1;
    }
    if (Kbd_RomajiToKana(kind, &buf[4]) != 0) {
        return 1;
    }
    if (Kbd_RomajiToKana(kind, &buf[6]) != 0) {
        return 1;
    }
    if (buf[4] != 'N' && buf[4] != 'n') {
        return 0;
    }
    if (buf[6] == 'Y' || buf[6] == 'y') {
        return 0;
    }
    strncpy((char *)&gKbd->kana[gKbd->kanaPos - 4], gKanaNChars[kind], 2);
    return 1;
}

extern s8 gKbdKind8Chars;

/*
 * 1 when entry kind 8 accepts the key string str (two bytes, as the key
 * string tables hold a key): a one-byte character (its second byte a
 * space) when it is printable ASCII other than a comma; a two-byte one
 * when it is among the 0x108 characters of gKbdKind8Chars. Else 0. The
 * table search is written as a goto loop.
 */
s32 Kbd_CharAllowed(s8 *str)
{
    u32 i;
    s8 *entry;
    u8 firstChar;

    if (*(u8 *)((u8 *)str + 1) == 0x20) {
        firstChar = (u8) *(s8 *)((u8 *)str + 0);
        if (firstChar == 0x2C) {
            return 0;
        }
        if (((s32) firstChar >= 0x20) && ((s32) firstChar < 0x7F)) {
            return 1;
        }
        return 0;
    }
    i = 0;
    entry = &gKbdKind8Chars;
loop_10:
    if (strncmp((const char *) entry, (const char *) str, 2U) == 0) {
        return 1;
    }
    i += 1;
    entry += 2;
    if (i >= 0x108U) {
        return 0;
    }
    goto loop_10;
}

/*
 * Commits the pending kana into the text at the cursor; nothing when kana
 * is empty. Makes kana full-width (Str_ToWide into wide). While
 * converting, ends the conversion (ConvCmd_CommitAll; offGrid 0 when the
 * keyboard is active), and the text inserted is convDone, cand and
 * convRest made full-width. Inserts with Str_InsertClip (cut to maxLen),
 * moves textPos past it, clears kana, sets kanaPos 0 and resets the
 * cursor blink.
 */
void Kbd_CommitKana(void)
{
    char buf[0x100];
    s32 added;

    if (gKbd->kana[0] == 0) {
        return;
    }
    Str_ToWide((s8 *)gKbd->kana, (s8 *)gKbd->wide);
    if (gKbd->converting != 0) {
        ConvCmd_CommitAll(gKbd->wide);
        gKbd->converting = 0;
        if (gKbd->active == 1) {
            gKbd->offGrid = 0;
        }
        memset(buf, 0, 0x100);
        strncpy(buf, gKbd->convDone, 0xFF);
        strncat(buf, gKbd->cand, 0xFF);
        strncat(buf, gKbd->convRest, 0xFF);
        Str_ToWide((s8 *)buf, (s8 *)gKbd->wide);
    }
    added = Str_InsertClip(gKbd->text, (char *)gKbd->wide, gKbd->textPos,
                      gKbd->maxLen);
    memset(gKbd->kana, 0, 0xFF);
    gKbd->textPos = gKbd->textPos + added;
    gKbd->kanaPos = 0;
    gKbd->blink = 0;
}

/*
 * Inserts src into dst at byte pos, first cutting src (in place, at a
 * character boundary) so that dst and src together are at most max
 * bytes. Returns the bytes inserted (strlen of src after the cut).
 * FAKEMATCH: `len += k - len` for len = k is how the game's code computes
 * it (subu, addu); len = k, and for or while forms of the walk with it,
 * compile differently. Holding the total length in len before the test
 * puts len in a0 and max - dlen in a1, as the game's code does.
 */
s32 Str_InsertClip(char *dst, char *src, s32 pos, s32 max)
{
    char tail[0x100];
    s32 code;
    s32 dlen;
    char byte;
    s32 k;
    s32 len;

    dlen = strlen(dst);
    len = dlen + strlen(src);
    if (max < len) {
        k = len = 0;
        byte = src[k];
        while (byte != 0) {
            code = (u8)byte;
            if ((code >= 0x80 && code <= 0x9F) || (code >= 0xE0 && code <= 0xFF)) {
                k += 2;
            } else {
                k += 1;
            }
            if (k > max - dlen) {
                break;
            }
            len += k - len;
            byte = src[k];
        }
        src[len] = 0;
    }
    strncpy(tail, dst + pos, 0xFF);
    dst[pos] = 0;
    strcat(dst, src);
    strcat(dst, tail);
    return strlen(src);
}

/* 1 when the entry kind is 8, 9 or 10, else 0. */
s32 Kbd_KindIs8To10(void)
{
    u8 kind;

    kind = gKbd->kind;
    if (((s32) kind >= 8) && ((s32) kind < 0xB)) {
        return 1;
    }
    return 0;
}

s32 Kbd_KindIs8To10(void);

/*
 * Sets symbolMode: 5 (symbols are typed from the candidate list
 * gKbdSymbolCands) for entry kinds 8-10 on the symbol sets
 * (KBD_SET_SYMBOL, KBD_SET_HALF_SYMBOL), else 0.
 */
void Kbd_UpdateSymbolMode(void)
{
    s8 symMode = 0;

    if (Kbd_KindIs8To10() == 1) {
        switch (gKbd->keySet) {
        case KBD_SET_HALF_SYMBOL:
        case KBD_SET_SYMBOL:
            symMode = KBD_SYMBOLS_LISTED;
            break;
        }
    }
    gKbd->symbolMode = symMode;
}

/*
 * Before conversion, refreshes the layout and converts (Kbd_Convert);
 * while converting, commits the candidate (Kbd_CommitConversion) with
 * SE_SYS_SELECT.
 */
void Kbd_ConvertOrCommit(void)
{
    if (gKbd->converting == 0) {
        Kbd_RefreshLayout();
        Kbd_Convert();
        return;
    }
    Kbd_CommitConversion();
    gKbd->state = SE_SYS_SELECT;
}

void Font_SetSpacing(u8 a, u8 b);
void Text_SetColor(s16 color);
void Font_SetPos(s32 x, s32 y); /* unit's own declaration: the definition has (s16, s16);
 * Kbd_DrawLine matches with s32, and is 457 words off with s16 */
void Kbd_Printf(const char *fmt, ...);
s32 Sjis_CharSizeAt(char *s, s32 pos);
extern s16 gKbdChatTarget;
extern char gKbdPrompt[];

/*
 * Draws the text entry line at (x, y): the prompt gKbdPrompt in colour
 * gKbdChatTarget (when that is not negative), the text from firstShown
 * (asterisks for the password kinds 5 and 10), then the romaji being
 * typed made full-width or, while converting, convDone, cand (its column
 * kept in candCol) and convRest, and the cursor (a full-width black
 * square; not while converting, not while blink bit 0x20 is set).
 * The line holds 44 bytes, less the prompt (its length made even).
 * firstShown scrolls back when the cursor is less than 2 from the left
 * edge, and on (so the cursor ends 6 from the right) when the cursor or
 * the candidate's end passes width - 2. A two-byte character cut by an
 * edge is left out. Both colour calls before the text and the cursor
 * give colour 0 (the second written as keyFlags - keyFlags).
 * The romaji's and the candidate's widths are a new value,
 * len = width - off: an in-place `width -= off` is scheduled after the
 * strncpy's source address, the game's code before it. y is a typed local
 * copy of an int parameter, Font_SetPos is declared in this unit with
 * int parameters (its definition has s16 ones; with s16 this function
 * does not match), and offGrid and blink are read as u8.
 */
void Kbd_DrawLine(f32 x, u32 yIn)
{
    s32 width;
    s32 y = yIn;
    char buf[0x30];
    char conv[0x110];
    s32 *scrollRight;
    s32 cursorOff;
    s32 *scrollLeft;
    s32 first;
    KbdEntry *kbdLeft;
    s32 skip;
    s32 off;
    s32 i;
    KbdEntry *kbdRight;
    s32 len;
    f32 charW;

    charW = 10.0f;
    width = KBD_LINE_WIDTH;
    Font_SetSpacing(0x14, 0x14);
    if (gKbdChatTarget >= 0) {
        len = strlen(gKbdPrompt);
        if (len & 1) {
            len++;
        }
        Text_SetColor(gKbdChatTarget);
        Font_SetPos(x, y);
        Kbd_Printf("%s", gKbdPrompt);
        x += len * (s32)charW;
        width = KBD_LINE_WIDTH - len;
    }
    kbdLeft = gKbd;
    if (kbdLeft->converting == 0) {
        scrollLeft = &kbdLeft->firstShown;
        first = kbdLeft->firstShown;
        cursorOff = kbdLeft->textPos + kbdLeft->kanaPos - first;
    } else {
        first = kbdLeft->firstShown;
        scrollLeft = &kbdLeft->firstShown;
        cursorOff = kbdLeft->textPos + strlen(kbdLeft->convDone) - first;
    }
    if (cursorOff < 2) {
        if (width < first) {
            *scrollLeft -= width - 2;
            if (gKbd->firstShown < 0) {
                gKbd->firstShown = 0;
            }
        } else {
            *scrollLeft = 0;
        }
    }
    kbdRight = gKbd;
    if (kbdRight->converting == 0) {
        scrollRight = &kbdRight->firstShown;
        cursorOff = kbdRight->textPos + kbdRight->kanaPos - kbdRight->firstShown;
    } else {
        scrollRight = &kbdRight->firstShown;
        cursorOff = kbdRight->textPos + strlen(kbdRight->convDone) + strlen(kbdRight->cand) - *scrollRight;
    }
    if (width - 2 < cursorOff) {
        *scrollRight += cursorOff - width + 6;
    }
    len = strlen(gKbd->text);
    if (gKbd->firstShown < len) {
        if (gKbd->firstShown > 0 && Sjis_CharSizeAt(gKbd->text, gKbd->firstShown) < 0) {
            gKbd->firstShown--;
        }
        strncpy(buf, &gKbd->text[gKbd->firstShown], width);
        buf[width] = 0;
        if (gKbd->kind == 5 || gKbd->kind == 10) {
            len = strlen(buf);
            for (i = 0; i < len; i++) {
                buf[i] = '*';
            }
        }
        if (strlen(buf) >= width && Sjis_CharSizeAt(buf, width - 1) == 2) {
            buf[width - 1] = 0;
        }
        if (gKbd->offGrid == 1 && gKbd->converting == 0 && gKbd->keyFlags == 0) {
            Text_SetColor(0);
        } else {
            Text_SetColor(gKbd->keyFlags - gKbd->keyFlags);
        }
        Font_SetPos(x, y);
        Kbd_Printf("%s", buf);
    }
    if (gKbd->kana[0] != 0) {
        off = gKbd->textPos - gKbd->firstShown;
        skip = gKbd->firstShown - gKbd->textPos;
        if (off < 0) {
            off = 0;
        } else {
            skip = 0;
        }
        if (gKbd->converting == 0) {
            Str_ToWide((s8 *)gKbd->kana, (s8 *)conv);
            if (strlen(conv) > skip) {
                if (gKbd->firstShown > 0 && Sjis_CharSizeAt(conv, skip) < 0) {
                    skip--;
                }
                len = width - off;
                strncpy(buf, &conv[skip], len);
                buf[len] = 0;
                if (Sjis_CharSizeAt(buf, len - 1) == 2) {
                    buf[len - 1] = 0;
                }
                Text_SetColor(-2);
                Font_SetPos(x + off * charW, y);
                Kbd_Printf("%s", buf);
            }
        } else {
            if (strlen(gKbd->convDone) > skip) {
                strncpy(buf, &gKbd->convDone[skip], KBD_LINE_WIDTH);
                Text_SetColor(-2);
                Font_SetPos(x + off * charW, y);
                Kbd_Printf("%s", buf);
                off += strlen(buf);
                skip = 0;
            }
            if (strlen(gKbd->cand) > skip) {
                len = width - off;
                strncpy(buf, &gKbd->cand[skip], len);
                buf[len] = 0;
                if (Sjis_CharSizeAt(buf, len - 1) == 2) {
                    buf[len - 1] = 0;
                }
                gKbd->candCol = off;
                Text_SetColor(-3);
                Font_SetPos(x + off * charW, y);
                Kbd_Printf("%s", buf);
                off += strlen(buf);
            }
            if (off < width) {
                width -= off;
                strncpy(buf, gKbd->convRest, width);
                buf[width] = 0;
                if (Sjis_CharSizeAt(buf, width - 1) == 2) {
                    buf[width - 1] = 0;
                }
                Text_SetColor(-2);
                Font_SetPos(x + off * charW, y);
                Kbd_Printf("%s", buf);
            }
        }
    }
    if (gKbd->converting == 0 && gKbd->cursorShown != 0 && !(gKbd->blink & KBD_BLINK_HIDDEN)) {
        if (gKbd->offGrid == 1 && gKbd->converting == 0 && gKbd->keyFlags == 0) {
            Text_SetColor(0);
        } else {
            Text_SetColor(gKbd->keyFlags - gKbd->keyFlags);
        }
        Font_SetSpacing(4, 0x16);
        Font_SetPos(x + charW * (gKbd->textPos + gKbd->kanaPos - gKbd->firstShown), y);
        Kbd_Printf(TXT(0x0046C988) /* a full-width black square (a bar's cell) */);
    }
}

/*
 * 0 when every two-byte character of str is a full-width capital, small
 * letter, digit or symbol (0x8141-0x81AC); 1 when str is empty or holds
 * any other character. Characters are read lead byte high.
 */
s32 Str_NotWideAlnum(u8 *str)
{
    s32 bad = 1;
    s32 len;
    s32 i;
    u16 code;

    len = strlen((char *)str);
    if (len == 0) {
        return bad;
    }
    for (i = 0; i < len; i += 2, str += 2) {
        code = (u16)(str[0] << 8) | str[1];
        if ((code >= 0x8260 && code <= 0x8279) || (code >= 0x8281 && code <= 0x829A) || (code >= 0x824F && code <= 0x8258) ||
            (code >= 0x8141 && code <= 0x81AC)) {
            bad = 0;
        } else {
            return 1;
        }
    }
    return bad;
}

s32 memcmp(char *a, char *b, s32 n); /* memcmp */
void *memcpy(void *dst, const void *src, u32 n);

/*
 * Fills gCaseCands (four strings of 0x100 bytes) with src first, then its
 * full-width upper-case, lower-case and capitalised forms that differ
 * from it (the capitalised one is dropped when it equals the upper-case
 * one); gCaseCandCount is their number. Copies src to out; returns 0.
 */
s32 Kbd_MakeCaseCands(char *src, char *out)
{
    char orig[0x100];
    char forms[3][0x100];
    s32 i;
    s32 count;
    char *form;
    s32 len;

    memset(&gCaseCands, 0, 0x400);
    memset(orig, 0, 0x100);
    memset(forms, 0, 0x300);
    len = strlen(src);
    strncpy(orig, src, len);
    Wide_ToUpper(forms[0], src);
    Wide_ToLower(forms[1], src);
    Wide_Capitalize(forms[2], src);
    if (memcmp(forms[0], forms[2], len) == 0) {
        memset(forms[2], 0, len);
    }
    memcpy(&gCaseCands, orig, len);
    count = 0;
    for (i = 0, form = forms[0]; i < 3; i++, form += 0x100) {
        if (memcmp(form, orig, len) != 0 && *(u8 *)form != 0) {
            strncpy((char *)(&gCaseCands + (count + 1) * 0x100), form, len);
            count++;
        }
    }
    gCaseCandCount = count + 1;
    strncpy(out, orig, len);
    return 0;
}

/* Returns gCaseCandCount, the number of case candidates. */
s32 CaseCands_Count(void)
{
    return gCaseCandCount;
}

/*
 * Copies the first count strings of gCaseCands into the text of dst's
 * 0x101-byte candidate entries (gKbdCands); the id bytes are left as
 * they are. Returns 0.
 */
s32 CaseCands_CopyFirst(s8 *dst, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 0x101 + 1), (const char *)(&gCaseCands + (i << 8)), 0xFFU);
    }
    return 0;
}

/*
 * Copies count strings of gCaseCands, from start on, into the text of
 * dst's 0x101-byte candidate entries; returns 0. Kbd_ListCandidates uses
 * it when listing forward.
 */
s32 CaseCands_Copy(s8 *dst, u8 start, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 0x101 + 1), (const char *)(&gCaseCands + ((start + i) << 8)), 0xFFU);
    }
    return 0;
}

/*
 * The same as CaseCands_Copy; Kbd_ListCandidates uses it when listing
 * backward. Returns 0.
 */
s32 CaseCands_Copy2(s8 *dst, u8 start, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 0x101 + 1), (const char *)(&gCaseCands + ((start + i) << 8)), 0xFFU);
    }
    return 0;
}

/*
 * Makes the typed text, the first case candidate, the only one: sets
 * gCaseCandCount to 1 and copies gCaseCands' first string into dst.
 * Returns 0.
 */
s32 CaseCands_First(char *dst)
{
    gCaseCandCount = 1;
    strncpy(dst, (const char *) &gCaseCands, 0xFFU);
    return 0;
}

/*
 * Copies strlen(src) bytes of src to dst (no terminator is written) and
 * lowers the second byte by 0x21 for every two-byte character that is not
 * a full-width capital, digit or symbol, which makes full-width small
 * letters capitals. Returns 0.
 */
s32 Wide_ToUpper(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *cur;
    u16 code;

    len = strlen(src);
    strncpy(dst, src, len);
    i = 0;
    if (i < len) {
        cur = (u8 *)dst;
        do {
            code = (u16)(cur[0] << 8) | cur[1];
            if (!(code >= 0x8260 && code <= 0x8279) && !(code >= 0x824F && code <= 0x8258) && !(code >= 0x8141 && code <= 0x81AC)) {
                dst[i + 1] -= 0x21;
            }
            i += 2;
            cur += 2;
        } while (i < len);
    }
    return 0;
}

/*
 * The reverse of Wide_ToUpper: raises the second byte by 0x21 for every
 * two-byte character that is not a full-width small letter, digit or
 * symbol (capitals become small letters). Returns 0.
 */
s32 Wide_ToLower(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *cur;
    u16 code;

    len = strlen(src);
    strncpy(dst, src, len);
    i = 0;
    if (i < len) {
        cur = (u8 *)dst;
        do {
            code = (u16)(cur[0] << 8) | cur[1];
            if (!(code >= 0x8281 && code <= 0x829A) && !(code >= 0x824F && code <= 0x8258) && !(code >= 0x8141 && code <= 0x81AC)) {
                dst[i + 1] += 0x21;
            }
            i += 2;
            cur += 2;
        } while (i < len);
    }
    return 0;
}

/*
 * Wide_ToLower, then makes the first full-width small letter of dst a
 * capital. Returns 0.
 */
s32 Wide_Capitalize(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *cur;
    u16 code;

    Wide_ToLower(dst, src);
    len = strlen(src);
    i = 0;
    if (i < len) {
        cur = (u8 *)dst;
        do {
            code = (u16)(cur[0] << 8) | cur[1];
            if (code >= 0x8281 && code < 0x829B) {
                dst[i + 1] -= 0x21;
                return 0;
            }
            i += 2;
            cur += 2;
        } while (i < len);
    }
    return 0;
}

/*
 * Clears the conversion state: KbdEntry from candIdx up to usbKey (0x514
 * bytes: the candidate fields, kana, wide, convDone, cand and convRest)
 * and the candidate lines gKbdCandLines.
 */
void Kbd_ClearCands(void)
{
    memset(&gKbd->candIdx, 0, 0x514);
    memset(gKbdCandLines, 0, KBD_CAND_PAGE * KBD_CAND_LINE_SIZE);
}

/*
 * Brings the converter back to the candidate cand shows on entry: after
 * ConvCmd_SegmentForm2, steps ConvCmd_NextCand (candId takes its result)
 * until cand holds that text again. Stops when candId is 0, and sets
 * candId to 0 when the walk returns to its first candidate without
 * meeting the text (at most 256 steps). Nothing in the symbol mode or
 * for case candidates.
 */
void Kbd_SkipSameCands(void)
{
    char before[0x100];
    char first[0x100];
    s32 i;

    if (gKbd->symbolMode != 0) {
        return;
    }
    if (gKbd->caseCands != 0) {
        return;
    }
    strncpy(before, gKbd->cand, 0xFF);
    before[0xFE] = 0;
    ConvCmd_SegmentForm2(gKbd->cand, gKbd->convRest);
    gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
    strncpy(first, gKbd->cand, 0xFF);
    if (strcmp(gKbd->cand, before) == 0) {
        return;
    }
    if (gKbd->candId == 0) {
        return;
    }
    for (i = 0; i < 0xFF; i++) {
        gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
        if (strcmp(gKbd->cand, before) == 0) {
            return;
        }
        if (gKbd->candId == 0) {
            return;
        }
        if (strcmp(gKbd->cand, first) == 0) {
            gKbd->candId = 0;
            return;
        }
    }
}

/*
 * Returns the number of conversion candidates, at least 1. In the symbol
 * mode, 35 for entry kind 8, else 31; for case candidates,
 * CaseCands_Count. Otherwise steps ConvCmd_NextCand into local buffers,
 * counting, until the candidate is cand's text again (a full cycle) or
 * candId is 0, at most 256 steps; when the walk returns to its first
 * candidate without meeting cand's text, the count is 1 and candId 0.
 */
s32 Kbd_CountCands(void)
{
    char before[0x100];
    char first[0x100];
    char cand[0x100];
    char work[0x100];
    s32 count;
    s32 i;

    if (gKbd->symbolMode != 0) {
        if (gKbd->kind == KBD_KIND_CHECKED) {
            count = 0x23;
        } else {
            count = 0x1F;
        }
    } else if (gKbd->caseCands != 0) {
        return CaseCands_Count();
    } else {
        strncpy(before, gKbd->cand, 0xFF);
        count = 0;
        gKbd->candId = ConvCmd_NextCand(cand, work);
        strncpy(first, cand, 0xFF);
        count++;
        if (strcmp(cand, before) != 0 && gKbd->candId != 0) {
            for (i = 0; i < 0xFF; i++) {
                gKbd->candId = ConvCmd_NextCand(cand, work);
                count++;
                if (strcmp(cand, before) == 0) {
                    break;
                }
                if (gKbd->candId == 0) {
                    break;
                }
                if (strcmp(cand, first) == 0) {
                    count = 1;
                    gKbd->candId = 0;
                    break;
                }
            }
        }
    }
    if (count == 0) {
        count = 1;
    }
    return count;
}

s32 ConvCmd_Start(char *src, char *a, char *b);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C990);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046C9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CA98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CAA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CAA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_input", D_0046CAB8);

/*
 * Starts the conversion. In the symbol mode, kana becomes a space, cand
 * and convRest are emptied and the candidates counted. Otherwise, with
 * no kana pending (and not converting) plays SE_SYS_REFUSE and returns.
 * When the full-width kana is all full-width letters, digits or symbols
 * (Str_NotWideAlnum 0), the candidates are its case forms (caseCands 1,
 * Kbd_MakeCaseCands into cand). Else each typed n or N not followed by
 * y or Y becomes the kana N (gKanaNChars, picked by the pair's second
 * byte less 1), and ConvCmd_Start converts the full-width kana into cand
 * and convRest, its result candCount. Then the shown candidate is kept
 * (Kbd_SkipSameCands), convDone emptied, converting set to 1 and offGrid
 * to 1.
 */
void Kbd_StartConvert(void)
{
    s8 *cur;

    cur = (s8 *)gKbd->kana;
    gKbd->caseCands = 0;
    memset(gKbd->cand, 0, 0x100);
    memset(gKbd->convRest, 0, 0x100);
    if (gKbd->symbolMode != 0) {
        strcpy((char *)gKbd->kana, " ");
        gKbd->cand[0] = 0;
        gKbd->convRest[0] = 0;
        gKbd->candCount = Kbd_CountCands();
    } else {
        if (gKbd->converting == 0 && gKbd->kana[0] == 0) {
            gKbd->state = SE_SYS_REFUSE;
            return;
        }
        Str_ToWide((s8 *)gKbd->kana, (s8 *)gKbd->wide);
        if (Str_NotWideAlnum(gKbd->wide) == 0) {
            gKbd->caseCands = 1;
            Kbd_MakeCaseCands((char *)gKbd->wide, gKbd->cand);
        } else {
            for (; *cur != 0; cur += 2) {
                s8 next = cur[2];
                if (*cur == 'N' || *cur == 'n') {
                    if (next != 'Y' && next != 'y') {
                        strncpy((char *)cur, gKanaNChars[cur[1] - 1], 2);
                    }
                }
            }
            Str_ToWide((s8 *)gKbd->kana, (s8 *)gKbd->wide);
            gKbd->candCount = ConvCmd_Start((char *)gKbd->wide, gKbd->cand, gKbd->convRest);
        }
    }
    Kbd_SkipSameCands();
    gKbd->convDone[0] = 0;
    gKbd->converting = 1;
    gKbd->offGrid = 1;
}

/*
 * Outside the symbol mode and case candidates: with more than one
 * candidate, switches the segment to form 1 (ConvCmd_SegmentForm1),
 * keeps the shown candidate, counts and lists the candidates; with one
 * and candId 1, steps to the next candidate (ConvCmd_NextCand), counts
 * and lists them. Seen: Kbd_CommitInput calls it while converting.
 */
void Kbd_NextCandidate(void)
{
    if ((gKbd->symbolMode == 0) && (gKbd->caseCands == 0)) {
        if (gKbd->candCount > 1) {
            ConvCmd_SegmentForm1(gKbd->cand, gKbd->convRest);
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
            return;
        }
        if (gKbd->candId == 1) {
            gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
        }
    }
}

/*
 * For entry kinds 8-10 outside the symbol mode, submits the text
 * (Kbd_Submit). Otherwise SE_SYS_SELECT, starts the conversion
 * (Kbd_StartConvert), counts the candidates and lists them.
 */
void Kbd_Convert(void)
{
    if ((Kbd_KindIs8To10() == 1) && (gKbd->symbolMode == 0)) {
        Kbd_Submit();
        return;
    }
    gKbd->state = SE_SYS_SELECT;
    Kbd_StartConvert();
    gKbd->candCount = Kbd_CountCands();
    Kbd_InitCandList();
}

/*
 * While converting with a candidate in cand: inserts cand into the text
 * at textPos (Str_InsertClip, cut to maxLen), moves textPos past it,
 * clears kana and ends the conversion (converting 0, offGrid 0,
 * kanaPos 0).
 */
void Kbd_CommitConversion(void)
{
    if (gKbd->cand[0] != 0 && gKbd->converting != 0) {
        s32 added = Str_InsertClip(gKbd->text, gKbd->cand,
                              gKbd->textPos, gKbd->maxLen);
        gKbd->textPos = gKbd->textPos + added;
        memset(gKbd->kana, 0, 0xFF);
        gKbd->converting = 0;
        gKbd->offGrid = 0;
        gKbd->kanaPos = 0;
    }
}

/*
 * Copies candidate entry i's text (gKbdCands) into candidate line i
 * (gKbdCandLines, 0x100 bytes each) when it is not empty. A text longer
 * than 10 bytes is cut there and ends in three dots; the byte after them
 * is also cleared through gKbdCandLinesCut (gKbdCandLines + 13).
 */
void Kbd_SetCandLine(s32 i)
{
    s32 off = i << 8;
    s32 entryOff = off + i;
    u8 *line;

    if ((s8)gKbdCandText[entryOff] != 0) {
        line = gKbdCandLines + off;
        strcpy((char *)line, (char *)gKbdCands + entryOff + 1);
        if ((u32)strlen((char *)gKbdCandLines + off) > 10) {
            strcpy((char *)line + 10, "...");
            gKbdCandLinesCut[off] = 0;
        }
    }
}

extern char *gKbdSymbolCands[];

/*
 * Sets candidate entry line of gKbdCands to symbol first + line: its id
 * byte is that index, its text gKbdSymbolCands[first + line].
 */
void Kbd_SetSymbolCand(s32 line, s32 first)
{
    s32 symbol = line + first;
    u8 *entry = gKbdCands + line * 257;

    *entry = symbol;
    strcpy((char *)entry + 1, gKbdSymbolCands[symbol]);
}

/*
 * Takes the symbol under the candidate cursor (entry candIdx % 3 of
 * gKbdCands) as cand, and copies it into kana too.
 */
void Kbd_TakeSymbolCand(void)
{
    memset(gKbd->cand, 0, 0x100);
    strncpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1, 0xFF);
    strncpy((char *)gKbd->kana, gKbd->cand, 0xFF);
}

/*
 * Lists the candidate page that starts at candidate first: clears
 * gKbdCands' entries (0xFFFF bytes) and gKbdCandLines, then fills up to
 * 3 entries (candCount - first when fewer); dir 0 fills them forward,
 * else backward from the last. Symbols come from Kbd_SetSymbolCand (then
 * Kbd_TakeSymbolCand), case candidates from CaseCands_Copy or
 * CaseCands_Copy2, others from ConvCmd_NextCand (backward
 * ConvCmd_PrevCand) into each entry's id byte and text; the converter is
 * then stepped lines - 1 times the other way, so it rests on the page's
 * first entry (backward: its last). Sets the three lines; no sound when
 * there is only one candidate. The local copy of gKbd, through a cast,
 * keeps the game's code's registers.
 */
void Kbd_ListCandidates(s32 first, s32 dir)
{
    char buf[0x100];
    char rest[0x100];
    s32 count;
    s32 lines;
    s32 j;
    u8 *entry;
    KbdEntry *kbd;
    s32 i;

    memset(gKbdCands, 0, 0xFFFF);
    memset(gKbdCandLines, 0, KBD_CAND_PAGE * KBD_CAND_LINE_SIZE);
    if (dir == 0) {
        kbd = (KbdEntry *)gKbd; /* the cast keeps the original's registers */
        count = kbd->candCount;
        lines = count - first;
        if (count >= first + 3) {
            lines = 3;
        }
        if (kbd->symbolMode != 0) {
            for (j = 0; j < lines; j++) {
                Kbd_SetSymbolCand(j, first);
            }
            Kbd_TakeSymbolCand();
        } else if (kbd->caseCands != 0) {
            CaseCands_Copy((s8 *)gKbdCands, first & 0xFF, lines & 0xFF);
        } else {
            for (j = 0; j < lines; j++) {
                entry = gKbdCands + j * 0x101;
                *entry = ConvCmd_NextCand((char *)entry + 1, gKbd->convRest);
            }
            for (i = 0; i < lines - 1; i++) {
                ConvCmd_PrevCand(buf, rest);
            }
        }
        for (i = 0; i < 3; i++) {
            Kbd_SetCandLine(i);
        }
    } else {
        kbd = (KbdEntry *)gKbd; /* as above */
        lines = kbd->candCount - first;
        if (kbd->candCount >= first + 3) {
            lines = 3;
        }
        if (kbd->symbolMode != 0) {
            for (lines = lines - 1; lines >= 0; lines--) {
                Kbd_SetSymbolCand(lines, first);
            }
            Kbd_TakeSymbolCand();
        } else if (kbd->caseCands != 0) {
            CaseCands_Copy2((s8 *)gKbdCands, first & 0xFF, lines & 0xFF);
        } else {
            for (i = lines - 1; i >= 0; i--) {
                entry = gKbdCands + i * 0x101;
                *entry = ConvCmd_PrevCand((char *)entry + 1, gKbd->convRest);
            }
            for (i = 0; i < lines - 1; i++) {
                ConvCmd_NextCand(buf, rest);
            }
        }
        for (i = 2; i >= 0; i--) {
            Kbd_SetCandLine(i);
        }
    }
    if (gKbd->candCount == 1) {
        gKbd->state = KBD_SOUND_NONE;
    }
}


/*
 * Builds the first candidate page: candIdx and candPage 0; nothing more
 * with no candidates. Clears gKbdCands and gKbdCandLines and fills up to
 * 3 entries: the symbols from 0 (then Kbd_TakeSymbolCand), the first
 * case candidates, or cand as entry 0 (its id byte candId when there is
 * more than one) and the next ones from ConvCmd_NextCand, after which
 * the converter is stepped back to entry 0. Then sets the three lines.
 */
void Kbd_InitCandList(void)
{
    char candBuf[0x100];
    char restBuf[0x100];
    s32 extra;
    s32 i;

    gKbd->candIdx = 0;
    gKbd->candPage = 0;
    if (gKbd->candCount == 0) {
        return;
    }
    memset(gKbdCands, 0, 0xFFFF);
    memset(gKbdCandLines, 0, KBD_CAND_PAGE * KBD_CAND_LINE_SIZE);
    extra = (gKbd->candCount > 3) ? 2 : gKbd->candCount - 1;
    if (gKbd->symbolMode != 0) {
        for (i = 0; i < extra + 1; i++) {
            Kbd_SetSymbolCand(i, 0);
        }
        Kbd_TakeSymbolCand();
    } else if (gKbd->caseCands != 0) {
        CaseCands_CopyFirst((s8 *)gKbdCands, extra + 1);
    } else {
        strncpy((char *)gKbdCandText, gKbd->cand, 0xFF);
        if (gKbd->candCount > 1) {
            gKbdCands[0] = gKbd->candId;
        }
        for (i = 0; i < extra; i++) {
            gKbdCands[(i + 1) * 0x101] = ConvCmd_NextCand((char *)gKbdCands + (i + 1) * 0x101 + 1, gKbd->convRest);
        }
        for (i = 0; i < extra; i++) {
            ConvCmd_PrevCand(candBuf, restBuf);
        }
    }
    for (i = 0; i < 3; i++) {
        Kbd_SetCandLine(i);
    }
}

/*
 * Shows the previous conversion candidate (only while converting is 1).
 * candIdx goes down: below 0 it wraps to the last candidate and that
 * page is listed (Kbd_ListCandidates, backward); below the page's start
 * the previous page is listed; else the converter steps back
 * (ConvCmd_PrevCand). cand and candId then come from entry candIdx % 3.
 * counter points at candIdx, then at candPage, through casts that keep
 * the game's code's registers.
 */
void Kbd_PrevCand(void)
{
    char candBuf[0x100];
    char restBuf[0x100];
    s32 *counter;

    if (gKbd->converting != 1) {
        return;
    }
    gKbd->candIdx -= 1;
    if (gKbd->candIdx < 0) {
        counter = (s32 *)&gKbd->candIdx; /* the casts keep the original's registers */
        *counter = gKbd->candCount - 1;
        gKbd->candPage = (gKbd->candCount - 1) / 3;
        Kbd_ListCandidates(gKbd->candPage * 3, 1);
    } else {
        counter = (s32 *)&gKbd->candPage;
        if (gKbd->candIdx < *counter * 3) {
            *counter = *counter - 1;
            Kbd_ListCandidates(gKbd->candPage * 3, 1);
        } else {
            ConvCmd_PrevCand(candBuf, restBuf);
        }
    }
    memset(gKbd->cand, 0, 0x100);
    strcpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1);
    gKbd->candId = gKbdCands[(gKbd->candIdx % 3) * 257];
}

/* Next conversion candidate. */
/*
 * Shows the next conversion candidate (only while converting is 1).
 * candIdx goes up: past the last it wraps to 0 and the first page is
 * listed; past the page's end the next page is listed from candIdx; else
 * the converter steps on (ConvCmd_NextCand). cand and candId then come
 * from entry candIdx % 3. counter points at candIdx, then at candPage,
 * through casts that keep the game's code's registers.
 */
void Kbd_NextCand(void)
{
    char candBuf[0x100];
    char restBuf[0x100];
    s32 *counter;

    if (gKbd->converting != 1) {
        return;
    }
    gKbd->candIdx += 1;
    if (gKbd->candIdx > gKbd->candCount - 1) {
        counter = (s32 *)&gKbd->candIdx; /* the casts keep the original's registers */
        *counter = 0;
        gKbd->candPage = 0;
        Kbd_ListCandidates(0, 0);
    } else {
        counter = (s32 *)&gKbd->candPage;
        if (*counter * 3 + 3 <= gKbd->candIdx) {
            *counter = *counter + 1;
            Kbd_ListCandidates(gKbd->candIdx, 0);
        } else {
            ConvCmd_NextCand(candBuf, restBuf);
        }
    }
    memset(gKbd->cand, 0, 0x100);
    strcpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1);
    gKbd->candId = gKbdCands[(gKbd->candIdx % 3) * 257];
}

/*
 * Goes to the first candidate of the previous page by Kbd_PrevCand
 * steps; from the first page, to the first candidate of the last page.
 * SE_SYS_SCROLL when candIdx changed. Seen: USB Page Up, pad L2.
 */
void Kbd_PageUp(void)
{
    s32 start;
    s32 steps;
    s32 lastPageCount;

    start = gKbd->candIdx;
    if (start < 3) {
        lastPageCount = gKbd->candCount % 3;
        if (lastPageCount == 0) {
            steps = start % 3 + 3;
        } else {
            steps = start % 3 + lastPageCount;
        }
        for (; steps > 0; steps--) {
            Kbd_PrevCand();
        }
    } else {
        steps = start % 3 + 3;
        for (; steps > 0; steps--) {
            Kbd_PrevCand();
        }
    }
    if (start != gKbd->candIdx) {
        gKbd->state = SE_SYS_SCROLL;
    }
}

/*
 * Goes to the first candidate of the next page by Kbd_NextCand steps.
 * When fewer than 3 candidates remain from candIdx, steps that many,
 * which wraps to candidate 0. SE_SYS_SCROLL when candIdx changed. Seen:
 * USB Page Down, pad R2.
 */
void Kbd_PageDown(void)
{
    s32 start;
    s32 steps;

    start = gKbd->candIdx;
    steps = gKbd->candCount - start;
    if (steps < 3) {
        for (; steps > 0; steps--) {
            Kbd_NextCand();
        }
    } else {
        steps = start % 3;
        for (steps = 3 - steps; steps > 0; steps--) {
            Kbd_NextCand();
        }
    }
    if (start != gKbd->candIdx) {
        gKbd->state = SE_SYS_SCROLL;
    }
}

/*
 * Moves the conversion to the previous segment (while converting is 1,
 * outside the symbol mode); no sound when convDone is empty. Commits the
 * segment (ConvCmd_CommitSegment, its text into gKbdSegText), steps back
 * twice (ConvCmd_PrevSegment, the second result is candCount) and keeps
 * the shown candidate. When that count is not 0, cuts the new segment's
 * text (cand's length) off the end of convDone, counts and lists the
 * candidates, SE_SYS_SCROLL. count is reused for convDone's length.
 */
void Kbd_PrevSegment(void)
{
    s16 len;
    s32 count;

    if (gKbd->converting == 1) {
        if (gKbd->convDone[0] == 0) {
            gKbd->state = KBD_SOUND_NONE;
        } else if (gKbd->symbolMode == 0) {
            ConvCmd_CommitSegment(gKbd->candId, gKbdSegText, gKbd->cand,
                          gKbd->convRest, &len);
            ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            count = ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            gKbd->candCount = count;
            Kbd_SkipSameCands();
            if (count != 0) {
                count = strlen(gKbd->convDone);
                gKbd->convDone[count - strlen(gKbd->cand)] = 0;
                gKbd->candCount = Kbd_CountCands();
                Kbd_InitCandList();
                gKbd->state = SE_SYS_SCROLL;
            }
        }
    }
}

/*
 * Moves the conversion to the next segment (while converting is 1,
 * outside the symbol mode); no sound when convRest is empty. Commits the
 * segment (ConvCmd_CommitSegment, its text into gKbdSegText), steps back
 * once (ConvCmd_PrevSegment) then on (ConvCmd_NextSegment, its result
 * candCount), keeps the shown candidate, counts the candidates, appends
 * gKbdSegText to convDone, lists them, SE_SYS_SCROLL.
 */
void Kbd_NextSegment(void)
{
    s16 len;
    char *yomi;

    if (gKbd->converting == 1) {
        yomi = gKbd->convRest;
        if (*yomi == 0) {
            gKbd->state = KBD_SOUND_NONE;
        } else if (gKbd->symbolMode == 0) {
            ConvCmd_CommitSegment(gKbd->candId, gKbdSegText, gKbd->cand, yomi, &len);
            ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            gKbd->candCount = ConvCmd_NextSegment(gKbd->cand, gKbd->convRest);
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            strcat(gKbd->convDone, (char *)gKbdSegText);
            Kbd_InitCandList();
            gKbd->state = SE_SYS_SCROLL;
        }
    }
}
