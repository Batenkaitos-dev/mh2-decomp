/*
 * The software keyboard and text entry: the state at *gKbd (usbMods
 * pairs of bits, kanaInput a toggle), the layouts at gKbdSetLayouts.
 * A unit from 0x30EA30 to at least 0x314580 (level 3): Kbd_ShiftFKey and
 * Kbd_CtrlFKey keep their key in a0 across the static Kbd_CtrlHeld/Kbd_ShiftHeld,
 * which Kbd_TypeHidKey calls too. Kbd_DrawCandidateBox (unk_314AD0.c) is a level-4
 * unit with its own "%s"; the rest is unk_314E00.c.
 */
#include "common.h"
#include "em.h"
#include "kbd.h"

#include "ee_stdarg.h"
#include "sub_main/unk_30EA30.h"   /* the texts this file uses (tools/content_gen.py) */

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
void Kbd_ConfirmStep_Unk(s32 flag);
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
void Player_RequestAct(u8 a);
void Mem_Fill(void *dst, u8 c, s32 n);
extern u8 gKbdDakutenSjis;
extern u8 gKbdDakutenSjis_1;
extern u8 gKbdHandakutenSjis;
extern u8 gKbdHandakutenSjis_1;
extern u8 gKbdSetModes[];
extern u8 gKbdCands[];
extern u8 D_0059E211[];
extern u8 D_005AE210[];
extern u8 gKbdCandLines[];
extern s8 D_005AE31D[];
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

/* unk_30EA30's .rodata (0x46A930-0x46C8F8) */
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
char D_0046B2B8[0x8] = {
#include "sub_main/0046B2B8.inc"
};
char D_0046B2C0[0x8] = {
#include "sub_main/0046B2C0.inc"
};
char D_0046B2C8[0x8] = {
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

void Kbd_ReadUsb(void)
{
    s8 *data;
    u8 usbKey;

    if (gKbd->polled == 0) {
        gKbd->polled = 1;
        gKbd->usbPrevKey = (u8) gKbd->usbKey;
        gKbd->usbPrevMods = (u8) gKbd->usbMods;
        gKbd->usbMods = 0U;
        gKbd->usbKey = 0U;
        gKbd->usbNewKey = 0U;
        data = (s8 *)UsbKb_Poll();
        if (data != NULL) {
            gKbd->usbMods = *(u8 *)((u8 *)data + 0);
            gKbd->usbKey = (u8) *(u8 *)((u8 *)data + 2);
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
 * Keeps the previous key (curKey into prevKey) and takes the new one from usbKey; clears
 * the repeatTimer timer when its top bit is set, and on a new key sets it to 0x9E and calls
 * Kbd_HandleKey. The game places the clearing after the function's last block and
 * jumps back (the goto gives that layout).
 */
void Kbd_Update(void)
{
    KbdEntry *w;
    u8 *f;

    gKbd->prevKey = gKbd->curKey;
    gKbd->curKey = gKbd->usbKey;
    f = &gKbd->repeatTimer;
    if (*f & 0x80) {
        goto clear;
    }
back:
    w = gKbd;
    if (w->curKey == 0) {
        return;
    }
    if (w->curKey == w->prevKey) {
        return;
    }
    w->repeatTimer = 0x9E;
    Kbd_HandleKey();
    return;
clear:
    *f = 0;
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

void Kbd_HandleKey(void)
{
    u8 key;

    gKbd->offGrid = 1;
    if (gKbd->active != 0) {
        Kbd_ClearActive();
    }
    key = gKbd->curKey;
    if (key < 4 || (key >= 0x46 && key <= 0x48) || key == 0x53 || (key >= 0x64 && key <= 0x86) || key >= 0x8C) {
        return;
    }
    switch (key) {
    case 0x49:
        return;
    case 0x4B:
        Kbd_PageUp();
        break;
    case 0x4E:
        Kbd_PageDown();
        break;
    case 0x58:
    case 0x28:
        Kbd_Decide();
        break;
    case 0x29:
        Kbd_Escape();
        break;
    case 0x2A:
        Kbd_Backspace();
        break;
    case 0x2B:
        Kbd_NextChatTarget();
        break;
    case 0x2C:
        Kbd_Space(0);
        break;
    case 0x2F:
        Kbd_DakutenKey();
        break;
    case 0x30:
        Kbd_HandakutenKey();
        break;
    case 0x35:
        Kbd_ZenkakuKey();
        break;
    case 0x39:
        Kbd_CapsLock();
        break;
    case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:
        Kbd_FKey(key);
        break;
    case 0x4A:
        Kbd_CursorHome();
        break;
    case 0x4C:
        Kbd_Delete();
        break;
    case 0x4D:
        Kbd_CursorEnd();
        break;
    case 0x4F:
        Kbd_CursorRight();
        break;
    case 0x50:
        Kbd_CursorLeft();
        break;
    case 0x51:
        Kbd_CursorDown();
        break;
    case 0x52:
        Kbd_CursorUp();
        break;
    case 0x88:
        Kbd_KanaKey();
        break;
    case 0x8A:
        Kbd_HenkanKey();
        break;
    case 0x8B:
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
extern u8 D_00416440[];
extern char *D_00416408[];
extern char *D_00416428[];
extern char *gKbdHidChars[];
extern char *gKbdHidWideStrs[];
extern char *D_00416430[];
extern s32 D_0059E200;

void Kbd_TypeHidKey(void)
{
    char buf[4];
    u8 ch[2];
    u8 shift;
    u8 kind;
    u8 key;
    s32 num;
    char *str;
    u8 c;
    s32 n;
    s32 alpha;
    s32 up;
    u8 f;
    u16 *pos;
    KbdEntry *k;

    num = 0;
    if (gKbd->mode == 13 || gKbd->mode == 4 || gKbd->mode == 5) {
        return;
    }
    if (gKbd->converting != 0) {
        Kbd_CommitKana();
    }
    key = gKbd->curKey;
    if (key >= 0x54 && key < 0x64) {
        num = 1;
    }
    if (key == 0x87) {
        key = 0;
    }
    if (key == 0x89) {
        key = 1;
    }
    if (key == 0x31) {
        key = 0x32;
    }
    shift = Kbd_ShiftHeld() != 0;
    kind = D_00416440[gKbd->keySet];
    if (Kbd_SetIsWide() != 0 || gKbd->keySet == 6 || gKbd->keySet == 14) {
        if (num) {
            str = &D_00416408[0][(key - 0x54) * 2];
            goto use;
        }
        if (gKbd->keySet == 6 || gKbd->keySet == 14) {
            if (Kbd_KanaInput() == 0) {
                if (D_00416428[0][key] == 0x31) {
                    Kbd_CommitKana();
                    kind = 2;
                    c = gKbdHidChars[kind + shift * 3][key];
                    goto check;
                }
                kind = 4;
                goto table;
            }
            c = gKbdHidChars[shift * 3][key];
            goto check;
        }
        if (Kbd_KanaInput() == 0 && (gKbd->mode <= 1U || gKbd->mode == 3 || (gKbd->mode == 13 && (gKbd->gestureMode & 0xF)))) {
            kind = 4;
        }
    table:
        str = &gKbdHidWideStrs[kind + shift * 5][key * 2];
    use:
        if (gKbd->kind == 8 && Kbd_CharAllowed((s8 *)str) == 0) {
            return;
        }
        if ((gKbd->accept & 2) && strncmp(TXT(0x0046C8F8) /* a full-width percent sign */, str, 2) == 0) {
            return;
        }
        strncpy(buf, str, 2);
        if (kind == 4) {
            alpha = 1;
            up = 0;
            if (buf[0] >= 0x41 && buf[0] < 0x5B) {
                up = 1;
            }
            if (up == 0 && (buf[0] >= 0x61 && buf[0] < 0x7B) == 0) {
                alpha = 0;
            }
            if (alpha) {
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
            if (key == 0x2C) {
                goto insert;
            }
            return;
        }
        if (shift == 0 && (k = gKbd, pos = &k->kanaPos, k->kanaPos == 0)) {
            D_0059E200 = 0;
        } else if (shift) {
            k = gKbd;
            pos = &k->kanaPos;
            D_0059E200 = k->kanaPos + 1;
        } else {
            k = gKbd;
            pos = &k->kanaPos;
            if (D_0059E200 > k->kanaPos - 1) {
                D_0059E200 = 0;
            }
        }
        n = Str_InsertClip((char *)k->kana, buf, *pos, 0x48);
        gKbd->kanaPos = gKbd->kanaPos + n;
        if (kind == 4) {
            if (gKbd->keySet == 6 || gKbd->keySet == 14) {
                if (Kbd_ConvertRomaji(2) != 0) {
                    Kbd_CommitKana();
                }
            } else if (gKbd->mode == 3) {
                if (D_0059E200 == 0) {
                    Kbd_ConvertRomaji(0);
                }
            } else if (D_0059E200 == 0) {
                Kbd_ConvertRomaji(gKbd->mode);
            }
        }
        goto done;
    }
    if (num) {
        c = D_00416430[0][key - 0x54];
    } else {
        c = gKbdHidChars[kind + shift * 3][key];
    }
check:
    f = gKbd->accept;
    if ((f & 0x10) && (c < 0x30 || c > 0x39)) {
        return;
    }
    if ((f & 0x20) && (c < 0x30 || c > 0x39) && c != 0x2D) {
        return;
    }
    if ((f & 0x40) && (c < 0x30 || c > 0x39) && (c < 0x41 || c > 0x5A)) {
        return;
    }
    if ((f & 0x80) && (c < 0x30 || c > 0x39) && c != 0x2D && c != 0x2A && c != 0x23) {
        return;
    }
    if ((f & 2) && c == 0x25) {
        return;
    }
    if (c != '[' && c != ']' && c != '\xA2' && c != '\xA3') {
        if (Kbd_KindIs8To10() == 0) {
            if (gKbd->kind == 1 || gKbd->kind == 5 || gKbd->kind == 2) {
                if (c == '\xA5') {
                    return;
                }
            }
            if (c == '\\') {
                return;
            }
            if (c == '{' || c == '}') {
                return;
            }
            if (gKbd->flags & 8) {
                if (c == '|') {
                    return;
                }
                if (c == '`') {
                    return;
                }
                if (c == '$') {
                    return;
                }
                if (c == '<') {
                    return;
                }
                if (c == '>') {
                    return;
                }
                if (c == '|') {
                    return;
                }
                if (c == '^') {
                    return;
                }
                if (c == '`') {
                    return;
                }
            }
        } else {
            if (c == ',') {
                return;
            }
            if (c < 0x20 || c > 0x7E) {
                return;
            }
        }
        if (gKbd->kind == 15 && c == '\'') {
            return;
        }
        ch[0] = c;
        ch[1] = 0;
        strncpy(buf, (char *)ch, 2);
insert:
        n = Str_InsertClip(gKbd->text, buf, gKbd->textPos, gKbd->maxLen);
        gKbd->textPos = gKbd->textPos + n;
done:
        if (n != 0) {
            gKbd->blink = 0;
        }
        gKbd->state = 0x16;
    }
}


static s32 Kbd_AltHeld(void)
{
    return gKbd->usbMods & 0x44;
}

static s32 Kbd_ShiftHeld(void)
{
    return gKbd->usbMods & 0x22;
}

static s32 Kbd_CtrlHeld(void)
{
    return gKbd->usbMods & 0x11;
}

s32 Kbd_KanaInput(void)
{
    return gKbd->kanaInput;
}

void Kbd_ToggleKanaInput(void)
{
    gKbd->kanaInput ^= 1;
    Kbd_SaveKanaInput();
}

void Kbd_ConvertOrCommit(void);
void Kbd_PressKey(KbdKey *key);
s32 Kbd_DoGesture(char *text, u16 *pos, s32 gesture);

/* Confirms the keyboard's current key (gKbd->key): with nothing there the next
 * state is 0x15; otherwise the key's action, or (from Kbd_ArrowsOnPage != 1) the end of
 * entry, which drops the two-byte characters Sjis_CharSizeAt rejects from the text. */
void Kbd_Decide(void)
{
    s32 done;
    KbdKey *e;
    u8 *s;
    s32 n;
    s32 i;
    KbdEntry *w;
    u8 *p;
    KbdChar *k;
    s32 lead;
    s32 kana;

    done = 0;
    if (gKbd->symbolMode != 0) {
        Kbd_ConvertOrCommit();
        done = 1;
    } else if (Kbd_ArrowsOnPage() == 1) {
        w = gKbd;
        e = w->key;
        k = &e->ch;
        if (k->code == 0) {
            w->state = 0x15;
            return;
        }
        if (k->b[0] == 1) {
            Kbd_PressKey(e);
            done = 1;
        } else if (w->mode == 0xD) {
            Kbd_DoGesture(w->text, &w->textPos, e->ch.b[1]);
        } else {
            Kbd_RefreshLayout();
            e = gKbd->key;
            if (e->ch.b[0] == 2 && e->ch.b[1] == 3) {
                done = 1;
                Kbd_Submit();
            } else {
                Kbd_InsertPhrase();
                if (gKbd->keyFlags & 2) {
                    gKbd->result = 1;
                }
            }
        }
    } else {
        done = 1;
        Kbd_Submit();
        if (gKbd->kind == 8) {
            i = 0;
            s = (u8 *)gKbd->text;
            while ((s8)s[i] != 0) {
                p = s + i;
                kana = 0;
                lead = (p[0] >= 0x80 && p[0] < 0xA0) || (p[0] >= 0xE0 && p[0] < 0x100);
                if (lead) {
                    if (p[1] >= 0x60 && p[1] < 0x7A) {
                        kana = 1;
                    } else if (p[1] >= 0x81 && p[1] < 0x9B) {
                        kana = 1;
                    }
                    if (kana) {
                        n = Sjis_CharSizeAt(gKbd->text, i);
                        if (i >= (s32)strlen(gKbd->text) || n == 0) {
                            break;
                        }
                        p = (u8 *)gKbd + i;
                        p[0x4C] = 0;
                        p = (u8 *)gKbd + i;
                        strcat((char *)p + 0x4C, (char *)p + n + 0x4C);
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
            gKbd->state = 0x18;
        } else {
            gKbd->state = 0x16;
        }
    }
}

void *memset(void *dst, s32 c, u32 n);
void Kbd_Escape(void)
{
    u8 *mode = &gKbd->converting;

    if (*mode != 0) {
        *mode = 0;
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
        gKbd->result = -1;
    }
    gKbd->state = 0x14;
}

void Kbd_Space(s32 arg0)
{
    if (gKbd->converting != 1) {
        if (gKbd->kana[0] == 0) {
            if (arg0 == 0) {
                Kbd_TypeKey(gKbd->key);
                if (gKbd->active == 1) {
                    gKbd->state = 0x16;
                }
            } else {
                gKbd->state = 0x15;
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
/* The case key (keySet) of the keyboard: 2/7 and 10/15 shift up/down when
 * allowed by flags; others pick the first allowed of 2, 10, 7, 15; then
 * redraw and state 0x16. */
void Kbd_CapsLock(void)
{
    switch (gKbd->keySet) {
    case 2:
    case 7:
        if (gKbd->accept & 1) {
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
    case 10:
    case 15:
        if (gKbd->accept & 1) {
            return;
        }
        if (Kbd_ShiftHeld() == 0) {
            return;
        }
        if (gKbd->flags & (1 << (gKbd->keySet - 8))) {
            return;
        }
        gKbd->keySet ^= 8;
        sceUsbKbSetLEDStatus(0, 2);
        break;
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
    case 13:
    case 14:
        if (Kbd_ShiftHeld() == 0) {
            s32 fl = gKbd->flags;
            if (!(fl & 4)) {
                gKbd->keySet = 2;
            } else if (!(fl & 0x400)) {
                gKbd->keySet = 10;
            } else if (!(fl & 0x80)) {
                gKbd->keySet = 7;
            } else if (!(fl & 0x8000)) {
                gKbd->keySet = 15;
            } else {
                return;
            }
            gKbd->keyFlags &= ~2;
        }
        break;
    case 11:
    case 12:
    default:
        return;
    }
    Kbd_UpdateMode();
    Kbd_HomePageCursor();
    Kbd_RefreshLayout();
    gKbd->state = 0x16;
}

void Kbd_ShiftFKey(u8 key)
{
    u8 k;
    u8 pos;

    if (gKbd->converting != 0 || Kbd_CtrlHeld() != 0) {
        gKbd->state = 0x15;
        return;
    }
    k = key - 0x3A;
    gKbd->keyFlags |= 2;
    if (Kbd_FindKind(4, &gKbd->mode, &gKbd->keySet) == 0) {
        Kbd_SetPhrasePage(4);
        pos = k * 5;
        gKbd->col = (pos + 1) % 20;
        gKbd->row = (pos + 1) / 20;
        Kbd_UpdateMode();
        Kbd_InsertPhrase();
        Kbd_UpdateSymbolMode();
        return;
    }
    gKbd->state = 0x15;
}

void Kbd_CtrlFKey(u8 key)
{
    u8 k;
    u8 pos;

    if (gKbd->converting != 0 || Kbd_ShiftHeld() != 0) {
        gKbd->state = 0x15;
        return;
    }
    k = key - 0x3A;
    gKbd->keyFlags |= 2;
    if (Kbd_FindKind(5, &gKbd->mode, &gKbd->keySet) == 0) {
        Kbd_SetPhrasePage(5);
        pos = k * 5;
        gKbd->col = (pos + 1) % 20;
        gKbd->row = (pos + 1) / 20;
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
void Kbd_FKeyStep_Unk(void);
extern s32 gKbdSettings;

/* s32 here (no andi before the calls); the definition, in unk_314E00.c, takes a u8 */
/* unit's own declaration: with the definition's s32 (u8 i) (sub_main/unk_314E00.c), Kbd_FKey: 80 of 332 words differ */
s32 Kbd_ShiftPair(s32 i);

/* The keys 0x3A..0x45 of the keyboard: with usbMods' 0x44 bits set, types key - 0x3A
 * (Kbd_DoGesture); with 0x11, Kbd_CtrlFKey; with 0x22, Kbd_ShiftFKey (when allowed);
 * otherwise the key's own action (0x3A/0x3B Kbd_FKeyRow, 0x3C/0x41 Kbd_FindKind kind 6,
 * 0x3D/0x3E the pages of Kbd_ShiftPair, 0x44 kind 0xD, 0x45 by the current kind), else the
 * next state is 0x15. */
void Kbd_FKey(u8 key)
{
    s8 r;
    s32 kind;
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
            && !(gKbd->flags & 0x10)) {
            Kbd_ShiftFKey(key);
        }
    } else {
        switch (key) {
        case 0x3A:
        case 0x3B:
            Kbd_FKeyRow(key);
            return;
        case 0x3C:
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
        case 0x3D:
            kind = (gKbdSettings & 2) ? 10 : 2;
            r = Kbd_ShiftPair(kind);
            if (r >= 0) {
                gKbd->keyFlags &= ~2;
                x = gKbd->col;
                y = gKbd->row;
                gKbd->keySet = r;
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
        case 0x3E:
            kind = (gKbdSettings & 2) ? 15 : 7;
            r = Kbd_ShiftPair(kind);
            if (r >= 0) {
                Kbd_CommitKana();
                gKbd->keyFlags &= ~2;
                x = gKbd->col;
                y = gKbd->row;
                gKbd->keySet = r;
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
        case 0x3F:
            Kbd_Confirm();
            return;
        case 0x40:
            Kbd_FKeyStep_Unk();
            return;
        case 0x41:
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
        case 0x42:
            gKbd->keyFlags &= ~2;
            Kbd_SetPhrasePage(4);
            Kbd_OpenPhrases1();
            return;
        case 0x43:
            gKbd->keyFlags &= ~2;
            Kbd_SetPhrasePage(5);
            Kbd_OpenPhrases2();
            return;
        case 0x44:
            if (gKbd->converting != 0 || (gKbd->flags & 0x2000)
                || *(s32 *)(gKbd->kindConf + 0x28) == 0) {
                gKbd->state = 0x15;
                return;
            }
            gKbd->keyFlags &= ~2;
            Kbd_SetPhrasePage(0xD);
            gKbd->keySet = 0xD;
            Kbd_UpdateMode();
            gKbd->col = 5;
            gKbd->row = 0;
            Kbd_EnterGestures();
            Kbd_RefreshLayout();
            gKbd->state = 0x16;
            return;
        case 0x45:
            switch (gKbd->keySet) {
            case 4:
            case 5:
            default:
                Kbd_CyclePhrase();
                return;
            case 13:
                Kbd_CycleGesture();
                return;
            }
        default:
            gKbd->state = 0x15;
            break;
        }
    }
}


void Kbd_FKeyRow(u8 key)
{
    if (Kbd_FindKind(key - 0x3A, &gKbd->mode, &gKbd->keySet) == 0) {
        gKbd->state = 0x16;
        gKbd->gestureMode = 0;
        gKbd->keyFlags &= 0xFD;
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

void Kbd_ClearText(void)
{
    gKbd->kana[0] = 0;
    gKbd->text[0] = 0;
    gKbd->kanaPos = 0;
    gKbd->textPos = 0;
    gKbd->firstShown = 0;
}

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

/* Confirms the text entry: converts the typed kana (cand into convRest) and
 * sets the next state (0x16 when done, else 0x15). */
void Kbd_Confirm(void)
{
    s32 next = 0x15;
    s32 v;

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
            while ((v = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest)) != 0) {
                gKbd->candCount = v;
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

void Kbd_ConfirmStep_Unk(s32 flag)
{
    s32 state = 0x15;
    s32 r;

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
        if (flag != 0) {
            r = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
            while (r != 0) {
                gKbd->candCount = r;
                r = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
            }
        } else {
            while ((r = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest)) != 0) {
                gKbd->candCount = r;
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
    } else if (flag != 0) {
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

void Kbd_FKeyStep_Unk(void)
{
    s32 state = 0x15;
    s32 r;

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
            r = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
            while (r != 0) {
                gKbd->candCount = r;
                r = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
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

void Kbd_CursorHome(void)
{
    s32 state = -1;
    u16 *kanaPos;
    u16 *textPos;

    if (gKbd->converting == 0) {
        if (gKbd->kana[0] != 0) {
            kanaPos = &gKbd->kanaPos;
            if (*kanaPos != 0) {
                *kanaPos = 0;
                state = 0x16;
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

void Kbd_Delete(void)
{
    if (gKbd->converting != 0) {
        gKbd->state = 0x15;
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

/* Cuts s off after n characters. */
void Str_CutAt(char *s, s32 n)
{
    if (n < strlen(s)) {
        s[n] = 0;
        gKbd->blink = 0;
    }
}

/* Moves the cursor to the end of the text (text, or the kana buffer kana). */
void Kbd_CursorEnd(void)
{
    KbdEntry *w = gKbd;
    s32 state = -1;
    s32 pos;
    char *kana;

    if (w->converting == 0) {
        kana = (char *)w->kana;
        if (w->kana[0] != 0) {
            pos = w->kanaPos;
            if ((u32)pos < (u32)strlen(kana)) {
                gKbd->kanaPos = strlen((char *)w->kana);
                state = 0x16;
            }
        } else {
            pos = w->textPos;
            if (pos < (u16)strlen(w->text)) {
                gKbd->textPos = strlen(w->text);
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

/* Moves the keyboard cursor (0 left, 1 right, 2 up, 3 down) until it leaves the current key. */
void Kbd_MoveOffKey(s16 dir)
{
    s8 key;
    u8 *row;
    u8 *col;

    key = gKbd->keyIdx;
    do {
        if (dir == 0) {
            row = &gKbd->col;
            if (*row < 2) {
                *row = 20;
            } else {
                *row = *row - 1;
            }
        } else if (dir == 1) {
            gKbd->col += 1;
            row = &gKbd->col;
            if (*row > 20) {
                *row = 1;
            }
        }
        if (dir == 2) {
            col = &gKbd->row;
            if (*col <= 0) {
                *col = 0;
                break;
            }
            *col = *col - 1;
        } else if (dir == 3) {
            col = &gKbd->row;
            if (*col == 3) {
                break;
            }
            *col = *col + 1;
        }
        Kbd_LookupKey();
    } while (key == gKbd->keyIdx);
    gKbd->state = 0x17;
}

s32 Kbd_ArrowsOnPage(void)
{
    s32 c;

    if (gKbd->active == 0) {
        if (gKbd->converting != 0) {
            return 0;
        }
        c = gKbd->mode;
        if (c == 0xD && !(gKbd->gestureMode & 0xF)) {
            return 1;
        }
        if (c == 4 || c == 5) {
            return 1;
        }
    }
    return 0;
}


/* Moves the keyboard's cursor right: one character (to the end with the key
 * of Kbd_CtrlHeld) in the text (text, cursor textPos) or in the kana
 * buffer (kana, cursor kanaPos, by 2); at the end, -1. While converting, ends the
 * conversion (ConvCmd_GrowSegment) or, with the key flags clear, Kbd_NextSegment. */
void Kbd_CursorRight(void)
{
    KbdEntry *w;
    s32 n;
    u8 *s;
    u16 *p;

    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(1);
        gKbd->keyFlags &= ~2;
        return;
    }
    w = gKbd;
    if (w->converting == 0) {
        s = w->kana;
        if (*s != 0) {
            if (w->kanaPos < (u32)strlen((char *)s)) {
                if (Kbd_CtrlHeld() == 0) {
                    gKbd->kanaPos += 2;
                } else {
                    gKbd->kanaPos = strlen((char *)gKbd->kana);
                }
                gKbd->blink = 0;
                gKbd->state = 0x17;
                return;
            }
            w->state = -1;
            return;
        }
        if (w->textPos < (u16)strlen(w->text)) {
            if (Kbd_CtrlHeld() == 0) {
                p = &gKbd->textPos;
                *p = *p + Sjis_CharSizeAt(gKbd->text, *p);
            } else {
                gKbd->textPos = strlen(gKbd->text);
            }
            gKbd->blink = 0;
            gKbd->state = 0x17;
            return;
        }
        w->state = -1;
        return;
    }
    if (w->caseCands == 1) {
        w->state = -1;
        return;
    }
    if (w->active == 0 && Kbd_ShiftHeld() == 0) {
        gKbd->state = 0x17;
        Kbd_NextSegment();
        return;
    }
    n = ConvCmd_GrowSegment(gKbd->cand, gKbd->convRest);
    if (n > 0) {
        gKbd->converting = 1;
        gKbd->candCount = n;
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

/* Backspace: before conversion, deletes the character before the cursor
 * (kanaPos in kana, or textPos in text); after it, takes the conversion back.
 * `*p = *p - f()`: `*p -= f()` masks the result to u16 first. */
void Kbd_CursorLeft(void)
{
    s32 n;

    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(0);
        gKbd->keyFlags &= ~2;
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
                gKbd->state = 0x17;
            } else {
                gKbd->state = -1;
            }
        } else {
            if (gKbd->textPos != 0) {
                if (Kbd_CtrlHeld() == 0) {
                    u16 *p = &gKbd->textPos;
                    *p = *p - Sjis_PrevCharSize((u8 *)gKbd->text, *p);
                } else {
                    gKbd->textPos = 0;
                }
                gKbd->blink = 0;
                gKbd->state = 0x17;
            } else {
                gKbd->state = -1;
            }
        }
    } else {
        if (gKbd->caseCands == 1) {
            gKbd->state = -1;
            return;
        }
        if (gKbd->active == 0 && Kbd_ShiftHeld() == 0) {
            gKbd->state = 0x17;
            Kbd_PrevSegment();
            return;
        }
        n = ConvCmd_ShrinkSegment(gKbd->cand, gKbd->convRest);
        if (n > 0) {
            gKbd->converting = 1;
            gKbd->candCount = n;
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
            gKbd->state = 0x17;
        } else {
            gKbd->state = -1;
        }
    }
}

void Kbd_CursorUp(void)
{
    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(2);
        gKbd->keyFlags = (u8) (gKbd->keyFlags & 0xFD);
        return;
    }
    if (gKbd->converting == 1) {
        gKbd->state = 0x17;
        Kbd_PrevCand();
    }
}

void Kbd_CursorDown(void)
{
    if (Kbd_ArrowsOnPage() == 1) {
        Kbd_MoveOffKey(3);
        gKbd->keyFlags = (u8) (gKbd->keyFlags & 0xFD);
        return;
    }
    if (gKbd->converting == 1) {
        gKbd->state = 0x17;
        Kbd_NextCand();
    }
}

void Kbd_KanaKey(void)
{
    s32 flags;

    if (Kbd_AltHeld() != 0) {
        Kbd_ToggleKanaInput();
        gKbd->state = 0x16;
        return;
    }
    flags = gKbd->flags;
    if ((flags & 2) || (flags & 1)) {
        return;
    }
    if (Kbd_ShiftHeld() != 0) {
        Kbd_FKeyRow(0x3B);
    } else {
        Kbd_FKeyRow(0x3A);
    }
    gKbd->state = 0x16;
}

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
            gKbd->state = 0x16;
        }
    } else if (gKbd->converting == 1) {
        gKbd->state = 0x16;
        if (Kbd_ShiftHeld() != 0) {
            Kbd_PrevCand();
        } else {
            Kbd_NextCand();
        }
    }
}

void Kbd_MuhenkanKey(void)
{
    if (Kbd_SetIsWide() != 0 && gKbd->kana[0] != 0) {
        gKbd->state = 0x16;
        gKbd->keyFlags &= 0xFD;
        if (gKbd->converting != 2) {
            gKbd->cyclePos = 1;
        } else {
            gKbd->cyclePos += 1;
            gKbd->cyclePos &= 1;
        }
        if (gKbd->cyclePos != 0) {
            Kbd_ConfirmStep_Unk(1);
        } else {
            Kbd_ConfirmStep_Unk(0);
        }
    }
}

void Kbd_CommitKana(void);
void Kbd_NextCandidate(void);

void Kbd_CommitInput(void)
{
    if (Kbd_SetIsWide() != 0 && gKbd->kana[0] != 0) {
        gKbd->state = 0x16;
        if (gKbd->converting == 0) {
            Kbd_CommitKana();
        } else {
            Kbd_NextCandidate();
        }
    }
}

s32 Sjis_CharSizeAt(char *s, s32 pos);

/* Deletes the character at the cursor (2 bytes in the kana buffer). */
void Kbd_DeleteChar(void)
{
    char *s;
    s32 n;
    s32 pos;

    if (gKbd->converting == 0) {
        s = (char *)gKbd->kana;
        if (gKbd->kana[0] != 0) {
            pos = gKbd->kanaPos;
            n = 2;
        } else {
            pos = gKbd->textPos;
            s = gKbd->text;
            n = Sjis_CharSizeAt(s, pos);
        }
        if (strlen(s) <= pos) {
            gKbd->state = 0x15;
            return;
        }
        if (n != 0) {
            s[pos] = 0;
            strcat(s + pos, s + pos + n);
            gKbd->blink = 0;
        }
    }
}

void Kbd_ZenkakuKey(void)
{
    s32 flags;

    gKbd->keyFlags &= 0xFD;
    if (Kbd_AltHeld() != 0) {
        flags = gKbd->flags;
        if ((flags & 0x80) && (flags & 0x8000)) {
            gKbd->state = 0x15;
        } else {
            gKbd->keyFlags ^= 1;
            if (gKbd->keyFlags & 1) {
                gKbd->prevKeySet = gKbd->keySet;
                if (gKbd->flags & 0x8000) {
                    gKbd->keySet = 7;
                } else {
                    gKbd->keySet = 0xF;
                }
            } else {
                gKbd->keySet = gKbd->prevKeySet;
            }
            Kbd_UpdateMode();
            Kbd_HomePageCursor();
            Kbd_RefreshLayout();
            Kbd_UpdateSymbolMode();
            gKbd->state = 0x16;
        }
    } else {
        Kbd_ToggleWidth();
    }
}

/* Drops the first n characters of s; returns n. */
s32 Str_DropFront(char *s, s32 n)
{
    if (n == 0) {
        return 0;
    }
    *s = 0;
    strcat(s, s + n);
    gKbd->blink = 0;
    return n;
}

extern char D_0046B2B8[];
extern char D_0046B2C0[];
extern char D_0046B2C8[];

s32 Kbd_RomajiToKana(s32 kind, char *s)
{
    s32 i;
    s32 len;
    s32 n;
    char buf[8];
    s32 j;
    s32 t;
    char *d;
    char *p;
    char *r;
    u8 *q;
    RomajiEntry *e;
    KbdEntry *w;
    s32 lead;

    len = strlen(s);
    if (len == 0) {
        return 0;
    }
    d = buf;
    for (p = s; *p != 0; p += 2, d++) {
        lead = (((u8 *)p)[0] >= 0x80 && ((u8 *)p)[0] < 0xA0) || (((u8 *)p)[0] >= 0xE0 && ((u8 *)p)[0] < 0x100);
        if (lead) {
            return 0;
        }
        *d = *p;
        if (*d >= 0x41 && *d <= 0x5A) {
            *d += 0x20;
        }
    }
    *d = 0;
    n = strlen(buf);
    for (i = 0, e = gRomajiKanaTbl; i < 0xF0; i++, e++) {
        r = e->romaji;
        t = strlen(r);
        if (t == n && strncmp(buf, r, n) == 0) {
            w = gKbd;
            w->kana[gKbd->kanaPos - len] = 0;
            r = gRomajiKanaTbl[i].kana[kind];
            for (j = 0x7E; gKbd->kanaPos < j; j--) {
                q = (u8 *)gKbd + j;
                q[0x168] = q[0x167];
            }
            strcat((char *)gKbd->kana, r);
            strcat((char *)gKbd->kana, (char *)&gKbd->kana[gKbd->kanaPos + 1]);
            if (strncmp(s, s - 2, 2) == 0) {
                switch (kind) {
                case 0:
                    strncpy((char *)&gKbd->kana[gKbd->kanaPos - len - 2], D_0046B2B8, 2);
                    break;
                case 1:
                    strncpy((char *)&gKbd->kana[gKbd->kanaPos - len - 2], D_0046B2C0, 2);
                    break;
                case 2:
                    strncpy((char *)&gKbd->kana[gKbd->kanaPos - len - 2], D_0046B2C8, 1);
                    w = gKbd;
                    w->kana[gKbd->kanaPos - len - 1] = 0;
                    t = gKbd->kanaPos - len;
                    strcat((char *)&gKbd->kana[t - 1], (char *)&gKbd->kana[t]);
                    break;
                }
            }
            t = strlen(r);
            gKbd->kanaPos = gKbd->kanaPos - (len - t);
            return 1;
        }
    }
    return 0;
}

extern char *gKanaNChars[];

s32 Kbd_ConvertRomaji(s32 kind)
{
    char buf[10];
    s32 pos;
    s32 skip;
    s32 n;

    memset(buf, 0, 10);
    pos = gKbd->kanaPos;
    skip = 8 - pos;
    if (skip < 0) {
        skip = 0;
    }
    n = 8 - skip;
    strncpy(&buf[skip], (char *)&gKbd->kana[pos - n], n);
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

extern s8 D_00416460;

s32 Kbd_CharAllowed(s8 *str)
{
    u32 i;
    s8 *p;
    u8 c;

    if (*(u8 *)((u8 *)str + 1) == 0x20) {
        c = (u8) *(s8 *)((u8 *)str + 0);
        if (c == 0x2C) {
            return 0;
        }
        if (((s32) c >= 0x20) && ((s32) c < 0x7F)) {
            return 1;
        }
        return 0;
    }
    i = 0;
    p = &D_00416460;
loop_10:
    if (strncmp((const char *) p, (const char *) str, 2U) == 0) {
        return 1;
    }
    i += 1;
    p += 2;
    if (i >= 0x108U) {
        return 0;
    }
    goto loop_10;
}

/* Commits the kana buffer (kana) into the text at the cursor. */
void Kbd_CommitKana(void)
{
    char buf[0x100];
    s32 n;

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
    n = Str_InsertClip(gKbd->text, (char *)gKbd->wide, gKbd->textPos,
                      gKbd->maxLen);
    memset(gKbd->kana, 0, 0xFF);
    gKbd->textPos = gKbd->textPos + n;
    gKbd->kanaPos = 0;
    gKbd->blink = 0;
}

/* FAKEMATCH: `len += k - len` for len = k: the original computes k - len and adds it (subu, addu); len = k
 * and the for/while forms of the walk with it give 18 words. The total length in len before the test (found
 * by the permuter) puts len in a0 and max - dlen in a1. */
s32 Str_InsertClip(char *dst, char *src, s32 pos, s32 max)
{
    char tail[0x100];
    s32 u;
    s32 dlen;
    char c;
    s32 k;
    s32 len;

    dlen = strlen(dst);
    len = dlen + strlen(src);
    if (max < len) {
        k = len = 0;
        c = src[k];
        while (c != 0) {
            u = (u8)c;
            if ((u >= 0x80 && u <= 0x9F) || (u >= 0xE0 && u <= 0xFF)) {
                k += 2;
            } else {
                k += 1;
            }
            if (k > max - dlen) {
                break;
            }
            len += k - len;
            c = src[k];
        }
        src[len] = 0;
    }
    strncpy(tail, dst + pos, 0xFF);
    dst[pos] = 0;
    strcat(dst, src);
    strcat(dst, tail);
    return strlen(src);
}

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

void Kbd_UpdateSymbolMode(void)
{
    s8 v = 0;

    if (Kbd_KindIs8To10() == 1) {
        switch (gKbd->keySet) {
        case 0xB:
        case 3:
            v = 5;
            break;
        }
    }
    gKbd->symbolMode = v;
}

void Kbd_ConvertOrCommit(void)
{
    if (gKbd->converting == 0) {
        Kbd_RefreshLayout();
        Kbd_Convert();
        return;
    }
    Kbd_CommitConversion();
    gKbd->state = 0x16;
}

void Font_SetSpacing(u8 a, u8 b);
void Text_SetColor(s16 color);
void Font_SetPos(s32 x, s32 y); /* unit's own declaration: the definition has (s16, s16);
 * Kbd_DrawLine matches with s32, and is 457 words off with s16 */
void Kbd_Printf(const char *fmt, ...);
s32 Sjis_CharSizeAt(char *s, s32 pos);
extern s16 gKbdChatTarget;
extern char gKbdPrompt[];

/* Draws the text entry line at (x, y): the prompt gKbdPrompt (colour gKbdChatTarget, when set), the
 * text from its first shown character (asterisks for a password), the romaji being typed or the
 * conversion (converted part, candidate at unk164, rest), and the cursor. The romaji's width is
 * a new value n = w - off, as for the candidate: MWCC schedules an in-place `w -= off` after the
 * strncpy's source address, the game before it. y is a typed local copy of an int parameter,
 * Font_SetPos is declared with int parameters above, KbdEntry.unk26/unk28 are u8 (lbu here). */
void Kbd_DrawLine(f32 x, u32 y_in)
{
    s32 w;
    s32 y = y_in;
    char buf[0x30];
    char conv[0x110];
    s32 *p2;
    s32 d;
    s32 *scroll;
    s32 s;
    KbdEntry *k;
    s32 skip;
    s32 off;
    s32 i;
    KbdEntry *k2;
    s32 n;
    f32 cw;

    cw = 10.0f;
    w = 0x2C;
    Font_SetSpacing(0x14, 0x14);
    if (gKbdChatTarget >= 0) {
        n = strlen(gKbdPrompt);
        if (n & 1) {
            n++;
        }
        Text_SetColor(gKbdChatTarget);
        Font_SetPos(x, y);
        Kbd_Printf("%s", gKbdPrompt);
        x += n * (s32)cw;
        w = 0x2C - n;
    }
    k = gKbd;
    if (k->converting == 0) {
        scroll = &k->firstShown;
        s = k->firstShown;
        d = k->textPos + k->kanaPos - s;
    } else {
        s = k->firstShown;
        scroll = &k->firstShown;
        d = k->textPos + strlen(k->convDone) - s;
    }
    if (d < 2) {
        if (w < s) {
            *scroll -= w - 2;
            if (gKbd->firstShown < 0) {
                gKbd->firstShown = 0;
            }
        } else {
            *scroll = 0;
        }
    }
    k2 = gKbd;
    if (k2->converting == 0) {
        p2 = &k2->firstShown;
        d = k2->textPos + k2->kanaPos - k2->firstShown;
    } else {
        p2 = &k2->firstShown;
        d = k2->textPos + strlen(k2->convDone) + strlen(k2->cand) - *p2;
    }
    if (w - 2 < d) {
        *p2 += d - w + 6;
    }
    n = strlen(gKbd->text);
    if (gKbd->firstShown < n) {
        if (gKbd->firstShown > 0 && Sjis_CharSizeAt(gKbd->text, gKbd->firstShown) < 0) {
            gKbd->firstShown--;
        }
        strncpy(buf, &gKbd->text[gKbd->firstShown], w);
        buf[w] = 0;
        if (gKbd->kind == 5 || gKbd->kind == 10) {
            n = strlen(buf);
            for (i = 0; i < n; i++) {
                buf[i] = '*';
            }
        }
        if (strlen(buf) >= w && Sjis_CharSizeAt(buf, w - 1) == 2) {
            buf[w - 1] = 0;
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
                n = w - off;
                strncpy(buf, &conv[skip], n);
                buf[n] = 0;
                if (Sjis_CharSizeAt(buf, n - 1) == 2) {
                    buf[n - 1] = 0;
                }
                Text_SetColor(-2);
                Font_SetPos(x + off * cw, y);
                Kbd_Printf("%s", buf);
            }
        } else {
            if (strlen(gKbd->convDone) > skip) {
                strncpy(buf, &gKbd->convDone[skip], 0x2C);
                Text_SetColor(-2);
                Font_SetPos(x + off * cw, y);
                Kbd_Printf("%s", buf);
                off += strlen(buf);
                skip = 0;
            }
            if (strlen(gKbd->cand) > skip) {
                n = w - off;
                strncpy(buf, &gKbd->cand[skip], n);
                buf[n] = 0;
                if (Sjis_CharSizeAt(buf, n - 1) == 2) {
                    buf[n - 1] = 0;
                }
                gKbd->candCol = off;
                Text_SetColor(-3);
                Font_SetPos(x + off * cw, y);
                Kbd_Printf("%s", buf);
                off += strlen(buf);
            }
            if (off < w) {
                w -= off;
                strncpy(buf, gKbd->convRest, w);
                buf[w] = 0;
                if (Sjis_CharSizeAt(buf, w - 1) == 2) {
                    buf[w - 1] = 0;
                }
                Text_SetColor(-2);
                Font_SetPos(x + off * cw, y);
                Kbd_Printf("%s", buf);
            }
        }
    }
    if (gKbd->converting == 0 && gKbd->cursorShown != 0 && !(gKbd->blink & 0x20)) {
        if (gKbd->offGrid == 1 && gKbd->converting == 0 && gKbd->keyFlags == 0) {
            Text_SetColor(0);
        } else {
            Text_SetColor(gKbd->keyFlags - gKbd->keyFlags);
        }
        Font_SetSpacing(4, 0x16);
        Font_SetPos(x + cw * (gKbd->textPos + gKbd->kanaPos - gKbd->firstShown), y);
        Kbd_Printf(TXT(0x0046C988) /* a full-width black square (a bar's cell) */);
    }
}

/* 1 when s is empty or holds a character that is not a full-width letter, digit or symbol. */
s32 Str_NotWideAlnum(u8 *s)
{
    s32 bad = 1;
    s32 len;
    s32 i;
    u16 c;

    len = strlen((char *)s);
    if (len == 0) {
        return bad;
    }
    for (i = 0; i < len; i += 2, s += 2) {
        c = (u16)(s[0] << 8) | s[1];
        if ((c >= 0x8260 && c <= 0x8279) || (c >= 0x8281 && c <= 0x829A) || (c >= 0x824F && c <= 0x8258) ||
            (c >= 0x8141 && c <= 0x81AC)) {
            bad = 0;
        } else {
            return 1;
        }
    }
    return bad;
}

s32 memcmp(char *a, char *b, s32 n); /* memcmp */
void *memcpy(void *dst, const void *src, u32 n);

/* Fills the candidate list gCaseCands with src and its uppercase/lowercase/capitalised forms
 * (the different ones); copies src to out. */
s32 Kbd_MakeCaseCands(char *src, char *out)
{
    char orig[0x100];
    char forms[3][0x100];
    s32 i;
    s32 n;
    char *p;
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
    n = 0;
    for (i = 0, p = forms[0]; i < 3; i++, p += 0x100) {
        if (memcmp(p, orig, len) != 0 && *(u8 *)p != 0) {
            strncpy((char *)(&gCaseCands + (n + 1) * 0x100), p, len);
            n++;
        }
    }
    gCaseCandCount = n + 1;
    strncpy(out, orig, len);
    return 0;
}

s32 CaseCands_Count(void)
{
    return gCaseCandCount;
}

/* Copies the first count strings of gCaseCands into dst (lines of 0x101 bytes). */
s32 CaseCands_CopyFirst(s8 *dst, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 0x101 + 1), (const char *)(&gCaseCands + (i << 8)), 0xFFU);
    }
    return 0;
}

/* Copies count strings of gCaseCands from start on into dst (lines of 0x101 bytes). */
s32 CaseCands_Copy(s8 *dst, u8 start, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 0x101 + 1), (const char *)(&gCaseCands + ((start + i) << 8)), 0xFFU);
    }
    return 0;
}

/* The same as CaseCands_Copy. */
s32 CaseCands_Copy2(s8 *dst, u8 start, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 0x101 + 1), (const char *)(&gCaseCands + ((start + i) << 8)), 0xFFU);
    }
    return 0;
}

s32 CaseCands_First(char *dst)
{
    gCaseCandCount = 1;
    strncpy(dst, (const char *) &gCaseCands, 0xFFU);
    return 0;
}

/* Copies src to dst and lowers the second byte of every full-width character that is not an
 * uppercase letter, a digit or a symbol by 0x21 (lowercase letters to uppercase). */
s32 Wide_ToUpper(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *p;
    u16 c;

    len = strlen(src);
    strncpy(dst, src, len);
    i = 0;
    if (i < len) {
        p = (u8 *)dst;
        do {
            c = (u16)(p[0] << 8) | p[1];
            if (!(c >= 0x8260 && c <= 0x8279) && !(c >= 0x824F && c <= 0x8258) && !(c >= 0x8141 && c <= 0x81AC)) {
                dst[i + 1] -= 0x21;
            }
            i += 2;
            p += 2;
        } while (i < len);
    }
    return 0;
}

/* The reverse of Wide_ToUpper: raises the second byte by 0x21 (uppercase letters to lowercase). */
s32 Wide_ToLower(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *p;
    u16 c;

    len = strlen(src);
    strncpy(dst, src, len);
    i = 0;
    if (i < len) {
        p = (u8 *)dst;
        do {
            c = (u16)(p[0] << 8) | p[1];
            if (!(c >= 0x8281 && c <= 0x829A) && !(c >= 0x824F && c <= 0x8258) && !(c >= 0x8141 && c <= 0x81AC)) {
                dst[i + 1] += 0x21;
            }
            i += 2;
            p += 2;
        } while (i < len);
    }
    return 0;
}

/* Wide_ToLower, then makes the first full-width lowercase letter of dst uppercase. */
s32 Wide_Capitalize(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *p;
    u16 c;

    Wide_ToLower(dst, src);
    len = strlen(src);
    i = 0;
    if (i < len) {
        p = (u8 *)dst;
        do {
            c = (u16)(p[0] << 8) | p[1];
            if (c >= 0x8281 && c < 0x829B) {
                dst[i + 1] -= 0x21;
                return 0;
            }
            i += 2;
            p += 2;
        } while (i < len);
    }
    return 0;
}

void Kbd_ClearCands(void)
{
    memset(&gKbd->candIdx, 0, 0x514);
    memset(gKbdCandLines, 0, 0x300);
}

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

/* The number of conversion candidates (at least 1). */
s32 Kbd_CountCands(void)
{
    char before[0x100];
    char first[0x100];
    char cand[0x100];
    char work[0x100];
    s32 n;
    s32 i;

    if (gKbd->symbolMode != 0) {
        if (gKbd->kind == 8) {
            n = 0x23;
        } else {
            n = 0x1F;
        }
    } else if (gKbd->caseCands != 0) {
        return CaseCands_Count();
    } else {
        strncpy(before, gKbd->cand, 0xFF);
        n = 0;
        gKbd->candId = ConvCmd_NextCand(cand, work);
        strncpy(first, cand, 0xFF);
        n++;
        if (strcmp(cand, before) != 0 && gKbd->candId != 0) {
            for (i = 0; i < 0xFF; i++) {
                gKbd->candId = ConvCmd_NextCand(cand, work);
                n++;
                if (strcmp(cand, before) == 0) {
                    break;
                }
                if (gKbd->candId == 0) {
                    break;
                }
                if (strcmp(cand, first) == 0) {
                    n = 1;
                    gKbd->candId = 0;
                    break;
                }
            }
        }
    }
    if (n == 0) {
        n = 1;
    }
    return n;
}

s32 ConvCmd_Start(char *src, char *a, char *b);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C990);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046C9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CA98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CAA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CAA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_30EA30", D_0046CAB8);

void Kbd_StartConvert(void)
{
    s8 *s;

    s = (s8 *)gKbd->kana;
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
            gKbd->state = 0x15;
            return;
        }
        Str_ToWide((s8 *)gKbd->kana, (s8 *)gKbd->wide);
        if (Str_NotWideAlnum(gKbd->wide) == 0) {
            gKbd->caseCands = 1;
            Kbd_MakeCaseCands((char *)gKbd->wide, gKbd->cand);
        } else {
            for (; *s != 0; s += 2) {
                s8 c = s[2];
                if (*s == 'N' || *s == 'n') {
                    if (c != 'Y' && c != 'y') {
                        strncpy((char *)s, gKanaNChars[s[1] - 1], 2);
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

void Kbd_Convert(void)
{
    if ((Kbd_KindIs8To10() == 1) && (gKbd->symbolMode == 0)) {
        Kbd_Submit();
        return;
    }
    gKbd->state = 0x16;
    Kbd_StartConvert();
    gKbd->candCount = Kbd_CountCands();
    Kbd_InitCandList();
}

void Kbd_CommitConversion(void)
{
    if (gKbd->cand[0] != 0 && gKbd->converting != 0) {
        s32 n = Str_InsertClip(gKbd->text, gKbd->cand,
                              gKbd->textPos, gKbd->maxLen);
        gKbd->textPos = gKbd->textPos + n;
        memset(gKbd->kana, 0, 0xFF);
        gKbd->converting = 0;
        gKbd->offGrid = 0;
        gKbd->kanaPos = 0;
    }
}

/* Copies candidate i's text into line i (0x100 bytes each at gKbdCandLines), cut to 10
 * characters and "..." when it is longer. */
void Kbd_SetCandLine(s32 i)
{
    s32 off = i << 8;
    s32 k = off + i;
    u8 *line;

    if ((s8)D_0059E211[k] != 0) {
        line = gKbdCandLines + off;
        strcpy((char *)line, (char *)gKbdCands + k + 1);
        if ((u32)strlen((char *)gKbdCandLines + off) > 10) {
            strcpy((char *)line + 10, "...");
            D_005AE31D[off] = 0;
        }
    }
}

extern char *gKbdSymbolCands[];

void Kbd_SetSymbolCand(s32 i, s32 j)
{
    s32 k = i + j;
    u8 *p = gKbdCands + i * 257;

    *p = k;
    strcpy((char *)p + 1, gKbdSymbolCands[k]);
}

void Kbd_TakeSymbolCand(void)
{
    memset(gKbd->cand, 0, 0x100);
    strncpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1, 0xFF);
    strncpy((char *)gKbd->kana, gKbd->cand, 0xFF);
}

/* Fills the three candidate lines from entry a on: going forward (dir 0) or backward. */
void Kbd_ListCandidates(s32 a, s32 dir)
{
    char buf[0x100];
    char tmp[0x100];
    s32 cnt;
    s32 n;
    s32 j;
    u8 *p;
    KbdEntry *w;
    s32 i;

    memset(gKbdCands, 0, 0xFFFF);
    memset(gKbdCandLines, 0, 0x300);
    if (dir == 0) {
        w = (KbdEntry *)gKbd; /* the cast keeps the original's registers */
        cnt = w->candCount;
        n = cnt - a;
        if (cnt >= a + 3) {
            n = 3;
        }
        if (w->symbolMode != 0) {
            for (j = 0; j < n; j++) {
                Kbd_SetSymbolCand(j, a);
            }
            Kbd_TakeSymbolCand();
        } else if (w->caseCands != 0) {
            CaseCands_Copy((s8 *)gKbdCands, a & 0xFF, n & 0xFF);
        } else {
            for (j = 0; j < n; j++) {
                p = gKbdCands + j * 0x101;
                *p = ConvCmd_NextCand((char *)p + 1, gKbd->convRest);
            }
            for (i = 0; i < n - 1; i++) {
                ConvCmd_PrevCand(buf, tmp);
            }
        }
        for (i = 0; i < 3; i++) {
            Kbd_SetCandLine(i);
        }
    } else {
        w = (KbdEntry *)gKbd; /* as above */
        n = w->candCount - a;
        if (w->candCount >= a + 3) {
            n = 3;
        }
        if (w->symbolMode != 0) {
            for (n = n - 1; n >= 0; n--) {
                Kbd_SetSymbolCand(n, a);
            }
            Kbd_TakeSymbolCand();
        } else if (w->caseCands != 0) {
            CaseCands_Copy2((s8 *)gKbdCands, a & 0xFF, n & 0xFF);
        } else {
            for (i = n - 1; i >= 0; i--) {
                p = gKbdCands + i * 0x101;
                *p = ConvCmd_PrevCand((char *)p + 1, gKbd->convRest);
            }
            for (i = 0; i < n - 1; i++) {
                ConvCmd_NextCand(buf, tmp);
            }
        }
        for (i = 2; i >= 0; i--) {
            Kbd_SetCandLine(i);
        }
    }
    if (gKbd->candCount == 1) {
        gKbd->state = -1;
    }
}


/* Builds the candidate list (entries of 0x101 bytes at gKbdCands) and draws its first lines. */
void Kbd_InitCandList(void)
{
    char b[0x100];
    char a[0x100];
    s32 n;
    s32 i;

    gKbd->candIdx = 0;
    gKbd->candPage = 0;
    if (gKbd->candCount == 0) {
        return;
    }
    memset(gKbdCands, 0, 0xFFFF);
    memset(gKbdCandLines, 0, 0x300);
    n = (gKbd->candCount > 3) ? 2 : gKbd->candCount - 1;
    if (gKbd->symbolMode != 0) {
        for (i = 0; i < n + 1; i++) {
            Kbd_SetSymbolCand(i, 0);
        }
        Kbd_TakeSymbolCand();
    } else if (gKbd->caseCands != 0) {
        CaseCands_CopyFirst((s8 *)gKbdCands, n + 1);
    } else {
        strncpy((char *)D_0059E211, gKbd->cand, 0xFF);
        if (gKbd->candCount > 1) {
            gKbdCands[0] = gKbd->candId;
        }
        for (i = 0; i < n; i++) {
            gKbdCands[(i + 1) * 0x101] = ConvCmd_NextCand((char *)gKbdCands + (i + 1) * 0x101 + 1, gKbd->convRest);
        }
        for (i = 0; i < n; i++) {
            ConvCmd_PrevCand(b, a);
        }
    }
    for (i = 0; i < 3; i++) {
        Kbd_SetCandLine(i);
    }
}

/* Previous conversion candidate. */
void Kbd_PrevCand(void)
{
    char b[0x100];
    char a[0x100];
    s32 *p;

    if (gKbd->converting != 1) {
        return;
    }
    gKbd->candIdx -= 1;
    if (gKbd->candIdx < 0) {
        p = (s32 *)&gKbd->candIdx; /* the casts keep the original's registers */
        *p = gKbd->candCount - 1;
        gKbd->candPage = (gKbd->candCount - 1) / 3;
        Kbd_ListCandidates(gKbd->candPage * 3, 1);
    } else {
        p = (s32 *)&gKbd->candPage;
        if (gKbd->candIdx < *p * 3) {
            *p = *p - 1;
            Kbd_ListCandidates(gKbd->candPage * 3, 1);
        } else {
            ConvCmd_PrevCand(b, a);
        }
    }
    memset(gKbd->cand, 0, 0x100);
    strcpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1);
    gKbd->candId = gKbdCands[(gKbd->candIdx % 3) * 257];
}

/* Next conversion candidate. */
/* The raw offsets are kept here: KbdEntry fields change the registers. */
void Kbd_NextCand(void)
{
    char b[0x100];
    char a[0x100];
    s32 *p;

    if (gKbd->converting != 1) {
        return;
    }
    gKbd->candIdx += 1;
    if (gKbd->candIdx > gKbd->candCount - 1) {
        p = (s32 *)&gKbd->candIdx; /* the casts keep the original's registers */
        *p = 0;
        gKbd->candPage = 0;
        Kbd_ListCandidates(0, 0);
    } else {
        p = (s32 *)&gKbd->candPage;
        if (*p * 3 + 3 <= gKbd->candIdx) {
            *p = *p + 1;
            Kbd_ListCandidates(gKbd->candIdx, 0);
        } else {
            ConvCmd_NextCand(b, a);
        }
    }
    memset(gKbd->cand, 0, 0x100);
    strcpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1);
    gKbd->candId = gKbdCands[(gKbd->candIdx % 3) * 257];
}

void Kbd_PageUp(void)
{
    s32 start;
    s32 n;
    s32 m;

    start = gKbd->candIdx;
    if (start < 3) {
        m = gKbd->candCount % 3;
        if (m == 0) {
            n = start % 3 + 3;
        } else {
            n = start % 3 + m;
        }
        for (; n > 0; n--) {
            Kbd_PrevCand();
        }
    } else {
        n = start % 3 + 3;
        for (; n > 0; n--) {
            Kbd_PrevCand();
        }
    }
    if (start != gKbd->candIdx) {
        gKbd->state = 0x17;
    }
}

void Kbd_PageDown(void)
{
    s32 start;
    s32 n;

    start = gKbd->candIdx;
    n = gKbd->candCount - start;
    if (n < 3) {
        for (; n > 0; n--) {
            Kbd_NextCand();
        }
    } else {
        n = start % 3;
        for (n = 3 - n; n > 0; n--) {
            Kbd_NextCand();
        }
    }
    if (start != gKbd->candIdx) {
        gKbd->state = 0x17;
    }
}

void Kbd_PrevSegment(void)
{
    s16 len;
    s32 n;

    if (gKbd->converting == 1) {
        if (gKbd->convDone[0] == 0) {
            gKbd->state = -1;
        } else if (gKbd->symbolMode == 0) {
            ConvCmd_CommitSegment(gKbd->candId, D_005AE210, gKbd->cand,
                          gKbd->convRest, &len);
            ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            n = ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            gKbd->candCount = n;
            Kbd_SkipSameCands();
            if (n != 0) {
                n = strlen(gKbd->convDone);
                gKbd->convDone[n - strlen(gKbd->cand)] = 0;
                gKbd->candCount = Kbd_CountCands();
                Kbd_InitCandList();
                gKbd->state = 0x17;
            }
        }
    }
}

void Kbd_NextSegment(void)
{
    s16 len;
    char *yomi;

    if (gKbd->converting == 1) {
        yomi = gKbd->convRest;
        if (*yomi == 0) {
            gKbd->state = -1;
        } else if (gKbd->symbolMode == 0) {
            ConvCmd_CommitSegment(gKbd->candId, D_005AE210, gKbd->cand, yomi, &len);
            ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            gKbd->candCount = ConvCmd_NextSegment(gKbd->cand, gKbd->convRest);
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            strcat(gKbd->convDone, (char *)D_005AE210);
            Kbd_InitCandList();
            gKbd->state = 0x17;
        }
    }
}
