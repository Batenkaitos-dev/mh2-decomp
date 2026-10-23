/*
 * The software keyboard's input: the USB keyboard's keys (Kbd_ReadUsb,
 * Kbd_HandleKey and a handler per key), typing into the text or into the
 * kana being entered (Kbd_TypeHidKey, the romaji to kana table), the
 * cursors, the keys that start, step or commit the kana-kanji conversion,
 * and the drawing of the entry line. The state is the KbdEntry at gKbd;
 * the layouts are the key sets' tables (gKbdSetLayouts).
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
#include "area.h"
#include "camera.h"
#include "hud.h"
#include "player.h"
#include "shake.h"
#include "engine_math.h"
#include "mem_util.h"
#include "usb_kbd.h"
#include "libusbkb.h"

/* prototypes */
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap); /* vsnprintf */
void Font_Printf(const char *fmt, ...);
extern s32 gTextOff;
extern char D_0046CC10[]; /* "%s" */
char *strcat(char *dst, char *src); /* strcat */
s32 strlen(char *s); /* strlen */
char *strncat(char *dst, const char *src, u32 n); /* strncat */
void strncpy(char *dst, const char *src, u32 n);
s32 ConvCmd_NextCand(char *a, char *b);
s32 ConvCmd_CommitAll(u8 *a);
s32 ConvCmd_Open(void);
s32 ConvCmd_GrowSegment(char *a, char *b);
s32 ConvCmd_SegmentForm2(char *a, char *b);
s32 ConvCmd_SegmentForm1(char *a, char *b);
extern u8 gKbdDakutenSjis;
extern u8 gKbdDakutenSjis_1;
extern u8 gKbdHandakutenSjis;
extern u8 gKbdHandakutenSjis_1;
extern char D_0046CC08[]; /* a full-width space */
s32 strncmp(const char *a, const char *b, u32 n);
extern u8 gKbdPhrasePage;
extern u8 gKbdSavedKanaInput;
extern u8 gKbdState;
extern s8 D_005AEAD7;
/* end of prototypes */

typedef s8 Str256[256]; /* a text buffer */

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
char gRomajiKanaTbl_0_romaji[8] = {
#include "sub_main/0046A930.inc"
};
char gRomajiKanaTbl_0_kana_0[8] = {
#include "sub_main/0046A938.inc"
};
char gRomajiKanaTbl_0_kana_1[8] = {
#include "sub_main/0046A940.inc"
};
char gRomajiKanaTbl_0_kana_2[8] = {
#include "sub_main/0046A948.inc"
};
char gRomajiKanaTbl_1_romaji[8] = {
#include "sub_main/0046A950.inc"
};
char D_0046A958[8] = {
#include "sub_main/0046A958.inc"
};
char D_0046A960[8] = {
#include "sub_main/0046A960.inc"
};
char D_0046A968[8] = {
#include "sub_main/0046A968.inc"
};
char gRomajiKanaTbl_2_romaji[8] = {
#include "sub_main/0046A970.inc"
};
char D_0046A978[8] = {
#include "sub_main/0046A978.inc"
};
char D_0046A980[8] = {
#include "sub_main/0046A980.inc"
};
char D_0046A988[8] = {
#include "sub_main/0046A988.inc"
};
char gRomajiKanaTbl_3_romaji[8] = {
#include "sub_main/0046A990.inc"
};
char gRomajiKanaTbl_3_kana_0[8] = {
#include "sub_main/0046A998.inc"
};
char gRomajiKanaTbl_3_kana_1[8] = {
#include "sub_main/0046A9A0.inc"
};
char gRomajiKanaTbl_3_kana_2[8] = {
#include "sub_main/0046A9A8.inc"
};
char gRomajiKanaTbl_4_romaji[8] = {
#include "sub_main/0046A9B0.inc"
};
char gRomajiKanaTbl_4_kana_0[8] = {
#include "sub_main/0046A9B8.inc"
};
char gRomajiKanaTbl_4_kana_1[8] = {
#include "sub_main/0046A9C0.inc"
};
char gRomajiKanaTbl_4_kana_2[8] = {
#include "sub_main/0046A9C8.inc"
};
char gRomajiKanaTbl_5_romaji[8] = {
#include "sub_main/0046A9D0.inc"
};
char gRomajiKanaTbl_6_romaji[8] = {
#include "sub_main/0046A9D8.inc"
};
char gRomajiKanaTbl_7_romaji[8] = {
#include "sub_main/0046A9E0.inc"
};
char D_0046A9E8[8] = {
#include "sub_main/0046A9E8.inc"
};
char D_0046A9F0[8] = {
#include "sub_main/0046A9F0.inc"
};
char D_0046A9F8[8] = {
#include "sub_main/0046A9F8.inc"
};
char gRomajiKanaTbl_8_romaji[8] = {
#include "sub_main/0046AA00.inc"
};
char D_0046AA08[8] = {
#include "sub_main/0046AA08.inc"
};
char D_0046AA10[8] = {
#include "sub_main/0046AA10.inc"
};
char D_0046AA18[8] = {
#include "sub_main/0046AA18.inc"
};
char gRomajiKanaTbl_9_romaji[8] = {
#include "sub_main/0046AA20.inc"
};
char D_0046AA28[8] = {
#include "sub_main/0046AA28.inc"
};
char D_0046AA30[8] = {
#include "sub_main/0046AA30.inc"
};
char D_0046AA38[8] = {
#include "sub_main/0046AA38.inc"
};
char gRomajiKanaTbl_10_romaji[8] = {
#include "sub_main/0046AA40.inc"
};
char D_0046AA48[8] = {
#include "sub_main/0046AA48.inc"
};
char D_0046AA50[8] = {
#include "sub_main/0046AA50.inc"
};
char D_0046AA58[8] = {
#include "sub_main/0046AA58.inc"
};
char gRomajiKanaTbl_11_romaji[8] = {
#include "sub_main/0046AA60.inc"
};
char D_0046AA68[8] = {
#include "sub_main/0046AA68.inc"
};
char D_0046AA70[8] = {
#include "sub_main/0046AA70.inc"
};
char D_0046AA78[8] = {
#include "sub_main/0046AA78.inc"
};
char gRomajiKanaTbl_12_romaji[8] = {
#include "sub_main/0046AA80.inc"
};
char gRomajiKanaTbl_13_romaji[8] = {
#include "sub_main/0046AA88.inc"
};
char gRomajiKanaTbl_14_romaji[8] = {
#include "sub_main/0046AA90.inc"
};
char gRomajiKanaTbl_15_romaji[8] = {
#include "sub_main/0046AA98.inc"
};
char gRomajiKanaTbl_16_romaji[8] = {
#include "sub_main/0046AAA0.inc"
};
char gRomajiKanaTbl_17_romaji[8] = {
#include "sub_main/0046AAA8.inc"
};
char gRomajiKanaTbl_18_romaji[8] = {
#include "sub_main/0046AAB0.inc"
};
char gRomajiKanaTbl_19_romaji[8] = {
#include "sub_main/0046AAB8.inc"
};
char gRomajiKanaTbl_20_romaji[8] = {
#include "sub_main/0046AAC0.inc"
};
char gRomajiKanaTbl_21_romaji[8] = {
#include "sub_main/0046AAC8.inc"
};
char gRomajiKanaTbl_21_kana_0[8] = {
#include "sub_main/0046AAD0.inc"
};
char gRomajiKanaTbl_21_kana_1[8] = {
#include "sub_main/0046AAD8.inc"
};
char gRomajiKanaTbl_21_kana_2[8] = {
#include "sub_main/0046AAE0.inc"
};
char gRomajiKanaTbl_22_romaji[8] = {
#include "sub_main/0046AAE8.inc"
};
char D_0046AAF0[8] = {
#include "sub_main/0046AAF0.inc"
};
char D_0046AAF8[8] = {
#include "sub_main/0046AAF8.inc"
};
char D_0046AB00[8] = {
#include "sub_main/0046AB00.inc"
};
char gRomajiKanaTbl_23_romaji[8] = {
#include "sub_main/0046AB08.inc"
};
char gRomajiKanaTbl_23_kana_0[8] = {
#include "sub_main/0046AB10.inc"
};
char gRomajiKanaTbl_23_kana_1[8] = {
#include "sub_main/0046AB18.inc"
};
char gRomajiKanaTbl_23_kana_2[8] = {
#include "sub_main/0046AB20.inc"
};
char gRomajiKanaTbl_24_romaji[8] = {
#include "sub_main/0046AB28.inc"
};
char D_0046AB30[8] = {
#include "sub_main/0046AB30.inc"
};
char D_0046AB38[8] = {
#include "sub_main/0046AB38.inc"
};
char D_0046AB40[8] = {
#include "sub_main/0046AB40.inc"
};
char gRomajiKanaTbl_25_romaji[8] = {
#include "sub_main/0046AB48.inc"
};
char gRomajiKanaTbl_25_kana_0[8] = {
#include "sub_main/0046AB50.inc"
};
char gRomajiKanaTbl_25_kana_1[8] = {
#include "sub_main/0046AB58.inc"
};
char gRomajiKanaTbl_25_kana_2[8] = {
#include "sub_main/0046AB60.inc"
};
char gRomajiKanaTbl_26_romaji[8] = {
#include "sub_main/0046AB68.inc"
};
char D_0046AB70[8] = {
#include "sub_main/0046AB70.inc"
};
char D_0046AB78[8] = {
#include "sub_main/0046AB78.inc"
};
char D_0046AB80[8] = {
#include "sub_main/0046AB80.inc"
};
char gRomajiKanaTbl_27_romaji[8] = {
#include "sub_main/0046AB88.inc"
};
char gRomajiKanaTbl_28_romaji[8] = {
#include "sub_main/0046AB90.inc"
};
char gRomajiKanaTbl_29_romaji[8] = {
#include "sub_main/0046AB98.inc"
};
char gRomajiKanaTbl_30_romaji[8] = {
#include "sub_main/0046ABA0.inc"
};
char gRomajiKanaTbl_30_kana_0[8] = {
#include "sub_main/0046ABA8.inc"
};
char gRomajiKanaTbl_30_kana_1[8] = {
#include "sub_main/0046ABB0.inc"
};
char gRomajiKanaTbl_30_kana_2[8] = {
#include "sub_main/0046ABB8.inc"
};
char gRomajiKanaTbl_31_romaji[8] = {
#include "sub_main/0046ABC0.inc"
};
char gRomajiKanaTbl_31_kana_0[8] = {
#include "sub_main/0046ABC8.inc"
};
char gRomajiKanaTbl_31_kana_1[8] = {
#include "sub_main/0046ABD0.inc"
};
char gRomajiKanaTbl_31_kana_2[8] = {
#include "sub_main/0046ABD8.inc"
};
char gRomajiKanaTbl_32_romaji[8] = {
#include "sub_main/0046ABE0.inc"
};
char gRomajiKanaTbl_32_kana_0[8] = {
#include "sub_main/0046ABE8.inc"
};
char gRomajiKanaTbl_32_kana_1[8] = {
#include "sub_main/0046ABF0.inc"
};
char gRomajiKanaTbl_32_kana_2[8] = {
#include "sub_main/0046ABF8.inc"
};
char gRomajiKanaTbl_33_romaji[8] = {
#include "sub_main/0046AC00.inc"
};
char gRomajiKanaTbl_33_kana_0[8] = {
#include "sub_main/0046AC08.inc"
};
char gRomajiKanaTbl_33_kana_1[8] = {
#include "sub_main/0046AC10.inc"
};
char gRomajiKanaTbl_33_kana_2[8] = {
#include "sub_main/0046AC18.inc"
};
char gRomajiKanaTbl_34_romaji[8] = {
#include "sub_main/0046AC20.inc"
};
char gRomajiKanaTbl_34_kana_0[8] = {
#include "sub_main/0046AC28.inc"
};
char gRomajiKanaTbl_34_kana_1[8] = {
#include "sub_main/0046AC30.inc"
};
char gRomajiKanaTbl_34_kana_2[8] = {
#include "sub_main/0046AC38.inc"
};
char gRomajiKanaTbl_35_romaji[8] = {
#include "sub_main/0046AC40.inc"
};
char gRomajiKanaTbl_35_kana_0[8] = {
#include "sub_main/0046AC48.inc"
};
char gRomajiKanaTbl_35_kana_1[8] = {
#include "sub_main/0046AC50.inc"
};
char gRomajiKanaTbl_35_kana_2[8] = {
#include "sub_main/0046AC58.inc"
};
char gRomajiKanaTbl_36_romaji[8] = {
#include "sub_main/0046AC60.inc"
};
char gRomajiKanaTbl_36_kana_0[8] = {
#include "sub_main/0046AC68.inc"
};
char gRomajiKanaTbl_36_kana_1[8] = {
#include "sub_main/0046AC70.inc"
};
char gRomajiKanaTbl_36_kana_2[8] = {
#include "sub_main/0046AC78.inc"
};
char gRomajiKanaTbl_37_romaji[8] = {
#include "sub_main/0046AC80.inc"
};
char gRomajiKanaTbl_37_kana_0[8] = {
#include "sub_main/0046AC88.inc"
};
char gRomajiKanaTbl_37_kana_1[8] = {
#include "sub_main/0046AC90.inc"
};
char gRomajiKanaTbl_37_kana_2[8] = {
#include "sub_main/0046AC98.inc"
};
char gRomajiKanaTbl_38_romaji[8] = {
#include "sub_main/0046ACA0.inc"
};
char gRomajiKanaTbl_38_kana_0[8] = {
#include "sub_main/0046ACA8.inc"
};
char gRomajiKanaTbl_38_kana_1[8] = {
#include "sub_main/0046ACB0.inc"
};
char gRomajiKanaTbl_38_kana_2[8] = {
#include "sub_main/0046ACB8.inc"
};
char gRomajiKanaTbl_39_romaji[8] = {
#include "sub_main/0046ACC0.inc"
};
char gRomajiKanaTbl_39_kana_0[8] = {
#include "sub_main/0046ACC8.inc"
};
char gRomajiKanaTbl_39_kana_1[8] = {
#include "sub_main/0046ACD0.inc"
};
char gRomajiKanaTbl_39_kana_2[8] = {
#include "sub_main/0046ACD8.inc"
};
char gRomajiKanaTbl_40_romaji[8] = {
#include "sub_main/0046ACE0.inc"
};
char gRomajiKanaTbl_40_kana_0[8] = {
#include "sub_main/0046ACE8.inc"
};
char gRomajiKanaTbl_40_kana_1[8] = {
#include "sub_main/0046ACF0.inc"
};
char gRomajiKanaTbl_40_kana_2[8] = {
#include "sub_main/0046ACF8.inc"
};
char gRomajiKanaTbl_41_romaji[8] = {
#include "sub_main/0046AD00.inc"
};
char gRomajiKanaTbl_41_kana_0[8] = {
#include "sub_main/0046AD08.inc"
};
char gRomajiKanaTbl_41_kana_1[8] = {
#include "sub_main/0046AD10.inc"
};
char gRomajiKanaTbl_41_kana_2[8] = {
#include "sub_main/0046AD18.inc"
};
char gRomajiKanaTbl_42_romaji[8] = {
#include "sub_main/0046AD20.inc"
};
char gRomajiKanaTbl_42_kana_0[8] = {
#include "sub_main/0046AD28.inc"
};
char gRomajiKanaTbl_42_kana_1[8] = {
#include "sub_main/0046AD30.inc"
};
char gRomajiKanaTbl_42_kana_2[8] = {
#include "sub_main/0046AD38.inc"
};
char gRomajiKanaTbl_43_romaji[8] = {
#include "sub_main/0046AD40.inc"
};
char gRomajiKanaTbl_43_kana_0[8] = {
#include "sub_main/0046AD48.inc"
};
char gRomajiKanaTbl_43_kana_1[8] = {
#include "sub_main/0046AD50.inc"
};
char gRomajiKanaTbl_43_kana_2[8] = {
#include "sub_main/0046AD58.inc"
};
char gRomajiKanaTbl_44_romaji[8] = {
#include "sub_main/0046AD60.inc"
};
char D_0046AD68[8] = {
#include "sub_main/0046AD68.inc"
};
char D_0046AD70[8] = {
#include "sub_main/0046AD70.inc"
};
char D_0046AD78[8] = {
#include "sub_main/0046AD78.inc"
};
char gRomajiKanaTbl_45_romaji[8] = {
#include "sub_main/0046AD80.inc"
};
char gRomajiKanaTbl_46_romaji[8] = {
#include "sub_main/0046AD88.inc"
};
char D_0046AD90[8] = {
#include "sub_main/0046AD90.inc"
};
char D_0046AD98[8] = {
#include "sub_main/0046AD98.inc"
};
char D_0046ADA0[8] = {
#include "sub_main/0046ADA0.inc"
};
char gRomajiKanaTbl_47_romaji[8] = {
#include "sub_main/0046ADA8.inc"
};
char gRomajiKanaTbl_47_kana_0[8] = {
#include "sub_main/0046ADB0.inc"
};
char gRomajiKanaTbl_47_kana_1[8] = {
#include "sub_main/0046ADB8.inc"
};
char gRomajiKanaTbl_47_kana_2[8] = {
#include "sub_main/0046ADC0.inc"
};
char gRomajiKanaTbl_48_romaji[8] = {
#include "sub_main/0046ADC8.inc"
};
char gRomajiKanaTbl_49_romaji[8] = {
#include "sub_main/0046ADD0.inc"
};
char gRomajiKanaTbl_50_romaji[8] = {
#include "sub_main/0046ADD8.inc"
};
char gRomajiKanaTbl_50_kana_0[8] = {
#include "sub_main/0046ADE0.inc"
};
char gRomajiKanaTbl_50_kana_1[8] = {
#include "sub_main/0046ADE8.inc"
};
char gRomajiKanaTbl_50_kana_2[8] = {
#include "sub_main/0046ADF0.inc"
};
char gRomajiKanaTbl_51_romaji[8] = {
#include "sub_main/0046ADF8.inc"
};
char gRomajiKanaTbl_51_kana_0[8] = {
#include "sub_main/0046AE00.inc"
};
char gRomajiKanaTbl_51_kana_1[8] = {
#include "sub_main/0046AE08.inc"
};
char gRomajiKanaTbl_51_kana_2[8] = {
#include "sub_main/0046AE10.inc"
};
char gRomajiKanaTbl_52_romaji[8] = {
#include "sub_main/0046AE18.inc"
};
char gRomajiKanaTbl_52_kana_0[8] = {
#include "sub_main/0046AE20.inc"
};
char gRomajiKanaTbl_52_kana_1[8] = {
#include "sub_main/0046AE28.inc"
};
char gRomajiKanaTbl_52_kana_2[8] = {
#include "sub_main/0046AE30.inc"
};
char gRomajiKanaTbl_53_romaji[8] = {
#include "sub_main/0046AE38.inc"
};
char gRomajiKanaTbl_53_kana_0[8] = {
#include "sub_main/0046AE40.inc"
};
char gRomajiKanaTbl_53_kana_1[8] = {
#include "sub_main/0046AE48.inc"
};
char gRomajiKanaTbl_53_kana_2[8] = {
#include "sub_main/0046AE50.inc"
};
char gRomajiKanaTbl_54_romaji[8] = {
#include "sub_main/0046AE58.inc"
};
char gRomajiKanaTbl_54_kana_0[8] = {
#include "sub_main/0046AE60.inc"
};
char gRomajiKanaTbl_54_kana_1[8] = {
#include "sub_main/0046AE68.inc"
};
char gRomajiKanaTbl_54_kana_2[8] = {
#include "sub_main/0046AE70.inc"
};
char gRomajiKanaTbl_55_romaji[8] = {
#include "sub_main/0046AE78.inc"
};
char gRomajiKanaTbl_55_kana_0[8] = {
#include "sub_main/0046AE80.inc"
};
char gRomajiKanaTbl_55_kana_1[8] = {
#include "sub_main/0046AE88.inc"
};
char gRomajiKanaTbl_55_kana_2[8] = {
#include "sub_main/0046AE90.inc"
};
char gRomajiKanaTbl_56_romaji[8] = {
#include "sub_main/0046AE98.inc"
};
char gRomajiKanaTbl_56_kana_0[8] = {
#include "sub_main/0046AEA0.inc"
};
char gRomajiKanaTbl_56_kana_1[8] = {
#include "sub_main/0046AEA8.inc"
};
char gRomajiKanaTbl_56_kana_2[8] = {
#include "sub_main/0046AEB0.inc"
};
char gRomajiKanaTbl_57_romaji[8] = {
#include "sub_main/0046AEB8.inc"
};
char gRomajiKanaTbl_57_kana_0[8] = {
#include "sub_main/0046AEC0.inc"
};
char gRomajiKanaTbl_57_kana_1[8] = {
#include "sub_main/0046AEC8.inc"
};
char gRomajiKanaTbl_57_kana_2[8] = {
#include "sub_main/0046AED0.inc"
};
char gRomajiKanaTbl_58_romaji[8] = {
#include "sub_main/0046AED8.inc"
};
char gRomajiKanaTbl_58_kana_0[8] = {
#include "sub_main/0046AEE0.inc"
};
char gRomajiKanaTbl_58_kana_1[8] = {
#include "sub_main/0046AEE8.inc"
};
char gRomajiKanaTbl_58_kana_2[8] = {
#include "sub_main/0046AEF0.inc"
};
char gRomajiKanaTbl_59_romaji[8] = {
#include "sub_main/0046AEF8.inc"
};
char gRomajiKanaTbl_59_kana_0[8] = {
#include "sub_main/0046AF00.inc"
};
char gRomajiKanaTbl_59_kana_1[8] = {
#include "sub_main/0046AF08.inc"
};
char gRomajiKanaTbl_59_kana_2[8] = {
#include "sub_main/0046AF10.inc"
};
char gRomajiKanaTbl_60_romaji[8] = {
#include "sub_main/0046AF18.inc"
};
char D_0046AF20[8] = {
#include "sub_main/0046AF20.inc"
};
char D_0046AF28[8] = {
#include "sub_main/0046AF28.inc"
};
char D_0046AF30[8] = {
#include "sub_main/0046AF30.inc"
};
char gRomajiKanaTbl_61_romaji[8] = {
#include "sub_main/0046AF38.inc"
};
char gRomajiKanaTbl_61_kana_0[8] = {
#include "sub_main/0046AF40.inc"
};
char gRomajiKanaTbl_61_kana_1[8] = {
#include "sub_main/0046AF48.inc"
};
char gRomajiKanaTbl_61_kana_2[8] = {
#include "sub_main/0046AF50.inc"
};
char gRomajiKanaTbl_62_romaji[8] = {
#include "sub_main/0046AF58.inc"
};
char D_0046AF60[8] = {
#include "sub_main/0046AF60.inc"
};
char D_0046AF68[8] = {
#include "sub_main/0046AF68.inc"
};
char D_0046AF70[8] = {
#include "sub_main/0046AF70.inc"
};
char gRomajiKanaTbl_63_romaji[8] = {
#include "sub_main/0046AF78.inc"
};
char gRomajiKanaTbl_63_kana_0[8] = {
#include "sub_main/0046AF80.inc"
};
char gRomajiKanaTbl_63_kana_1[8] = {
#include "sub_main/0046AF88.inc"
};
char gRomajiKanaTbl_63_kana_2[8] = {
#include "sub_main/0046AF90.inc"
};
char gRomajiKanaTbl_64_romaji[8] = {
#include "sub_main/0046AF98.inc"
};
char gRomajiKanaTbl_65_romaji[8] = {
#include "sub_main/0046AFA0.inc"
};
char gRomajiKanaTbl_66_romaji[8] = {
#include "sub_main/0046AFA8.inc"
};
char gRomajiKanaTbl_66_kana_0[8] = {
#include "sub_main/0046AFB0.inc"
};
char gRomajiKanaTbl_66_kana_1[8] = {
#include "sub_main/0046AFB8.inc"
};
char gRomajiKanaTbl_66_kana_2[8] = {
#include "sub_main/0046AFC0.inc"
};
char gRomajiKanaTbl_67_romaji[8] = {
#include "sub_main/0046AFC8.inc"
};
char D_0046AFD0[8] = {
#include "sub_main/0046AFD0.inc"
};
char D_0046AFD8[8] = {
#include "sub_main/0046AFD8.inc"
};
char D_0046AFE0[8] = {
#include "sub_main/0046AFE0.inc"
};
char gRomajiKanaTbl_68_romaji[8] = {
#include "sub_main/0046AFE8.inc"
};
char gRomajiKanaTbl_68_kana_0[8] = {
#include "sub_main/0046AFF0.inc"
};
char gRomajiKanaTbl_68_kana_1[8] = {
#include "sub_main/0046AFF8.inc"
};
char gRomajiKanaTbl_68_kana_2[8] = {
#include "sub_main/0046B000.inc"
};
char gRomajiKanaTbl_69_romaji[8] = {
#include "sub_main/0046B008.inc"
};
char gRomajiKanaTbl_69_kana_0[8] = {
#include "sub_main/0046B010.inc"
};
char gRomajiKanaTbl_69_kana_1[8] = {
#include "sub_main/0046B018.inc"
};
char gRomajiKanaTbl_69_kana_2[8] = {
#include "sub_main/0046B020.inc"
};
char gRomajiKanaTbl_70_romaji[8] = {
#include "sub_main/0046B028.inc"
};
char gRomajiKanaTbl_70_kana_0[8] = {
#include "sub_main/0046B030.inc"
};
char gRomajiKanaTbl_70_kana_1[8] = {
#include "sub_main/0046B038.inc"
};
char gRomajiKanaTbl_70_kana_2[8] = {
#include "sub_main/0046B040.inc"
};
char gRomajiKanaTbl_71_romaji[8] = {
#include "sub_main/0046B048.inc"
};
char gRomajiKanaTbl_72_romaji[8] = {
#include "sub_main/0046B050.inc"
};
char gRomajiKanaTbl_73_romaji[8] = {
#include "sub_main/0046B058.inc"
};
char D_0046B060[8] = {
#include "sub_main/0046B060.inc"
};
char D_0046B068[8] = {
#include "sub_main/0046B068.inc"
};
char D_0046B070[8] = {
#include "sub_main/0046B070.inc"
};
char gRomajiKanaTbl_74_romaji[8] = {
#include "sub_main/0046B078.inc"
};
char gRomajiKanaTbl_74_kana_0[8] = {
#include "sub_main/0046B080.inc"
};
char gRomajiKanaTbl_74_kana_1[8] = {
#include "sub_main/0046B088.inc"
};
char gRomajiKanaTbl_74_kana_2[8] = {
#include "sub_main/0046B090.inc"
};
char gRomajiKanaTbl_75_romaji[8] = {
#include "sub_main/0046B098.inc"
};
char D_0046B0A0[8] = {
#include "sub_main/0046B0A0.inc"
};
char D_0046B0A8[8] = {
#include "sub_main/0046B0A8.inc"
};
char D_0046B0B0[8] = {
#include "sub_main/0046B0B0.inc"
};
char gRomajiKanaTbl_76_romaji[8] = {
#include "sub_main/0046B0B8.inc"
};
char D_0046B0C0[8] = {
#include "sub_main/0046B0C0.inc"
};
char D_0046B0C8[8] = {
#include "sub_main/0046B0C8.inc"
};
char D_0046B0D0[8] = {
#include "sub_main/0046B0D0.inc"
};
char gRomajiKanaTbl_77_romaji[8] = {
#include "sub_main/0046B0D8.inc"
};
char D_0046B0E0[8] = {
#include "sub_main/0046B0E0.inc"
};
char D_0046B0E8[8] = {
#include "sub_main/0046B0E8.inc"
};
char D_0046B0F0[8] = {
#include "sub_main/0046B0F0.inc"
};
char gRomajiKanaTbl_78_romaji[8] = {
#include "sub_main/0046B0F8.inc"
};
char gRomajiKanaTbl_79_romaji[8] = {
#include "sub_main/0046B100.inc"
};
char gRomajiKanaTbl_80_romaji[8] = {
#include "sub_main/0046B108.inc"
};
char gRomajiKanaTbl_81_romaji[8] = {
#include "sub_main/0046B110.inc"
};
char gRomajiKanaTbl_82_romaji[8] = {
#include "sub_main/0046B118.inc"
};
char D_0046B120[8] = {
#include "sub_main/0046B120.inc"
};
char D_0046B128[8] = {
#include "sub_main/0046B128.inc"
};
char D_0046B130[8] = {
#include "sub_main/0046B130.inc"
};
char gRomajiKanaTbl_83_romaji[8] = {
#include "sub_main/0046B138.inc"
};
char D_0046B140[8] = {
#include "sub_main/0046B140.inc"
};
char D_0046B148[8] = {
#include "sub_main/0046B148.inc"
};
char D_0046B150[8] = {
#include "sub_main/0046B150.inc"
};
char gRomajiKanaTbl_84_romaji[8] = {
#include "sub_main/0046B158.inc"
};
char D_0046B160[8] = {
#include "sub_main/0046B160.inc"
};
char D_0046B168[8] = {
#include "sub_main/0046B168.inc"
};
char D_0046B170[8] = {
#include "sub_main/0046B170.inc"
};
char gRomajiKanaTbl_85_romaji[8] = {
#include "sub_main/0046B178.inc"
};
char D_0046B180[8] = {
#include "sub_main/0046B180.inc"
};
char D_0046B188[8] = {
#include "sub_main/0046B188.inc"
};
char D_0046B190[8] = {
#include "sub_main/0046B190.inc"
};
char gRomajiKanaTbl_86_romaji[8] = {
#include "sub_main/0046B198.inc"
};
char D_0046B1A0[8] = {
#include "sub_main/0046B1A0.inc"
};
char D_0046B1A8[8] = {
#include "sub_main/0046B1A8.inc"
};
char D_0046B1B0[8] = {
#include "sub_main/0046B1B0.inc"
};
char gRomajiKanaTbl_87_romaji[8] = {
#include "sub_main/0046B1B8.inc"
};
char gRomajiKanaTbl_88_romaji[8] = {
#include "sub_main/0046B1C0.inc"
};
char gRomajiKanaTbl_89_romaji[8] = {
#include "sub_main/0046B1C8.inc"
};
char gRomajiKanaTbl_90_romaji[8] = {
#include "sub_main/0046B1D0.inc"
};
char gRomajiKanaTbl_91_romaji[8] = {
#include "sub_main/0046B1D8.inc"
};
char gRomajiKanaTbl_92_romaji[8] = {
#include "sub_main/0046B1E0.inc"
};
char gRomajiKanaTbl_93_romaji[8] = {
#include "sub_main/0046B1E8.inc"
};
char gRomajiKanaTbl_94_romaji[8] = {
#include "sub_main/0046B1F0.inc"
};
char gRomajiKanaTbl_95_romaji[8] = {
#include "sub_main/0046B1F8.inc"
};
char gRomajiKanaTbl_96_romaji[8] = {
#include "sub_main/0046B200.inc"
};
char gRomajiKanaTbl_96_kana_0[8] = {
#include "sub_main/0046B208.inc"
};
char gRomajiKanaTbl_96_kana_1[8] = {
#include "sub_main/0046B210.inc"
};
char gRomajiKanaTbl_96_kana_2[8] = {
#include "sub_main/0046B218.inc"
};
char gRomajiKanaTbl_97_romaji[8] = {
#include "sub_main/0046B220.inc"
};
char D_0046B228[8] = {
#include "sub_main/0046B228.inc"
};
char D_0046B230[8] = {
#include "sub_main/0046B230.inc"
};
char D_0046B238[8] = {
#include "sub_main/0046B238.inc"
};
char gRomajiKanaTbl_98_romaji[8] = {
#include "sub_main/0046B240.inc"
};
char D_0046B248[8] = {
#include "sub_main/0046B248.inc"
};
char D_0046B250[8] = {
#include "sub_main/0046B250.inc"
};
char D_0046B258[8] = {
#include "sub_main/0046B258.inc"
};
char gRomajiKanaTbl_99_romaji[8] = {
#include "sub_main/0046B260.inc"
};
char gRomajiKanaTbl_99_kana_0[8] = {
#include "sub_main/0046B268.inc"
};
char gRomajiKanaTbl_99_kana_1[8] = {
#include "sub_main/0046B270.inc"
};
char gRomajiKanaTbl_99_kana_2[8] = {
#include "sub_main/0046B278.inc"
};
char gRomajiKanaTbl_100_romaji[8] = {
#include "sub_main/0046B280.inc"
};
char gRomajiKanaTbl_100_kana_0[8] = {
#include "sub_main/0046B288.inc"
};
char gRomajiKanaTbl_100_kana_1[8] = {
#include "sub_main/0046B290.inc"
};
char gRomajiKanaTbl_100_kana_2[8] = {
#include "sub_main/0046B298.inc"
};
char gRomajiKanaTbl_101_romaji[8] = {
#include "sub_main/0046B2A0.inc"
};
char gRomajiKanaTbl_102_romaji[8] = {
#include "sub_main/0046B2A8.inc"
};
char gRomajiKanaTbl_103_romaji[8] = {
#include "sub_main/0046B2B0.inc"
};
char gSmallTsuHira[8] = {
#include "sub_main/0046B2B8.inc"
};
char gSmallTsuKata[8] = {
#include "sub_main/0046B2C0.inc"
};
char gSmallTsuHalf[8] = {
#include "sub_main/0046B2C8.inc"
};
char gRomajiKanaTbl_104_romaji[8] = {
#include "sub_main/0046B2D0.inc"
};
char gRomajiKanaTbl_105_romaji[8] = {
#include "sub_main/0046B2D8.inc"
};
char gRomajiKanaTbl_105_kana_0[8] = {
#include "sub_main/0046B2E0.inc"
};
char gRomajiKanaTbl_105_kana_1[8] = {
#include "sub_main/0046B2E8.inc"
};
char gRomajiKanaTbl_105_kana_2[8] = {
#include "sub_main/0046B2F0.inc"
};
char gRomajiKanaTbl_106_romaji[8] = {
#include "sub_main/0046B2F8.inc"
};
char gRomajiKanaTbl_106_kana_0[8] = {
#include "sub_main/0046B300.inc"
};
char gRomajiKanaTbl_106_kana_1[8] = {
#include "sub_main/0046B308.inc"
};
char gRomajiKanaTbl_106_kana_2[8] = {
#include "sub_main/0046B310.inc"
};
char gRomajiKanaTbl_107_romaji[8] = {
#include "sub_main/0046B318.inc"
};
char gRomajiKanaTbl_107_kana_0[8] = {
#include "sub_main/0046B320.inc"
};
char gRomajiKanaTbl_107_kana_1[8] = {
#include "sub_main/0046B328.inc"
};
char gRomajiKanaTbl_107_kana_2[8] = {
#include "sub_main/0046B330.inc"
};
char gRomajiKanaTbl_108_romaji[8] = {
#include "sub_main/0046B338.inc"
};
char gRomajiKanaTbl_108_kana_0[8] = {
#include "sub_main/0046B340.inc"
};
char gRomajiKanaTbl_108_kana_1[8] = {
#include "sub_main/0046B348.inc"
};
char gRomajiKanaTbl_108_kana_2[8] = {
#include "sub_main/0046B350.inc"
};
char gRomajiKanaTbl_109_romaji[8] = {
#include "sub_main/0046B358.inc"
};
char gRomajiKanaTbl_109_kana_0[8] = {
#include "sub_main/0046B360.inc"
};
char gRomajiKanaTbl_109_kana_1[8] = {
#include "sub_main/0046B368.inc"
};
char gRomajiKanaTbl_109_kana_2[8] = {
#include "sub_main/0046B370.inc"
};
char gRomajiKanaTbl_110_romaji[8] = {
#include "sub_main/0046B378.inc"
};
char D_0046B380[8] = {
#include "sub_main/0046B380.inc"
};
char D_0046B388[8] = {
#include "sub_main/0046B388.inc"
};
char D_0046B390[8] = {
#include "sub_main/0046B390.inc"
};
char gRomajiKanaTbl_111_romaji[8] = {
#include "sub_main/0046B398.inc"
};
char D_0046B3A0[8] = {
#include "sub_main/0046B3A0.inc"
};
char D_0046B3A8[8] = {
#include "sub_main/0046B3A8.inc"
};
char D_0046B3B0[8] = {
#include "sub_main/0046B3B0.inc"
};
char gRomajiKanaTbl_112_romaji[8] = {
#include "sub_main/0046B3B8.inc"
};
char D_0046B3C0[8] = {
#include "sub_main/0046B3C0.inc"
};
char D_0046B3C8[8] = {
#include "sub_main/0046B3C8.inc"
};
char D_0046B3D0[8] = {
#include "sub_main/0046B3D0.inc"
};
char gRomajiKanaTbl_113_romaji[8] = {
#include "sub_main/0046B3D8.inc"
};
char D_0046B3E0[8] = {
#include "sub_main/0046B3E0.inc"
};
char D_0046B3E8[8] = {
#include "sub_main/0046B3E8.inc"
};
char D_0046B3F0[8] = {
#include "sub_main/0046B3F0.inc"
};
char gRomajiKanaTbl_114_romaji[8] = {
#include "sub_main/0046B3F8.inc"
};
char D_0046B400[8] = {
#include "sub_main/0046B400.inc"
};
char D_0046B408[8] = {
#include "sub_main/0046B408.inc"
};
char D_0046B410[8] = {
#include "sub_main/0046B410.inc"
};
char gRomajiKanaTbl_115_romaji[8] = {
#include "sub_main/0046B418.inc"
};
char gRomajiKanaTbl_116_romaji[8] = {
#include "sub_main/0046B420.inc"
};
char gRomajiKanaTbl_117_romaji[8] = {
#include "sub_main/0046B428.inc"
};
char gRomajiKanaTbl_118_romaji[8] = {
#include "sub_main/0046B430.inc"
};
char gRomajiKanaTbl_119_romaji[8] = {
#include "sub_main/0046B438.inc"
};
char gRomajiKanaTbl_120_romaji[8] = {
#include "sub_main/0046B440.inc"
};
char gRomajiKanaTbl_121_romaji[8] = {
#include "sub_main/0046B448.inc"
};
char gRomajiKanaTbl_122_romaji[8] = {
#include "sub_main/0046B450.inc"
};
char gRomajiKanaTbl_123_romaji[8] = {
#include "sub_main/0046B458.inc"
};
char gRomajiKanaTbl_124_romaji[8] = {
#include "sub_main/0046B460.inc"
};
char gRomajiKanaTbl_124_kana_0[8] = {
#include "sub_main/0046B468.inc"
};
char gRomajiKanaTbl_124_kana_1[8] = {
#include "sub_main/0046B470.inc"
};
char gRomajiKanaTbl_124_kana_2[8] = {
#include "sub_main/0046B478.inc"
};
char gRomajiKanaTbl_125_romaji[8] = {
#include "sub_main/0046B480.inc"
};
char gRomajiKanaTbl_125_kana_0[8] = {
#include "sub_main/0046B488.inc"
};
char gRomajiKanaTbl_125_kana_1[8] = {
#include "sub_main/0046B490.inc"
};
char gRomajiKanaTbl_125_kana_2[8] = {
#include "sub_main/0046B498.inc"
};
char gRomajiKanaTbl_126_romaji[8] = {
#include "sub_main/0046B4A0.inc"
};
char gRomajiKanaTbl_126_kana_0[8] = {
#include "sub_main/0046B4A8.inc"
};
char gRomajiKanaTbl_126_kana_1[8] = {
#include "sub_main/0046B4B0.inc"
};
char gRomajiKanaTbl_126_kana_2[8] = {
#include "sub_main/0046B4B8.inc"
};
char gRomajiKanaTbl_127_romaji[8] = {
#include "sub_main/0046B4C0.inc"
};
char gRomajiKanaTbl_127_kana_0[8] = {
#include "sub_main/0046B4C8.inc"
};
char gRomajiKanaTbl_127_kana_1[8] = {
#include "sub_main/0046B4D0.inc"
};
char gRomajiKanaTbl_127_kana_2[8] = {
#include "sub_main/0046B4D8.inc"
};
char gRomajiKanaTbl_128_romaji[8] = {
#include "sub_main/0046B4E0.inc"
};
char gRomajiKanaTbl_128_kana_0[8] = {
#include "sub_main/0046B4E8.inc"
};
char gRomajiKanaTbl_128_kana_1[8] = {
#include "sub_main/0046B4F0.inc"
};
char gRomajiKanaTbl_128_kana_2[8] = {
#include "sub_main/0046B4F8.inc"
};
char gRomajiKanaTbl_129_romaji[8] = {
#include "sub_main/0046B500.inc"
};
char gRomajiKanaTbl_129_kana_0[8] = {
#include "sub_main/0046B508.inc"
};
char gRomajiKanaTbl_129_kana_1[8] = {
#include "sub_main/0046B510.inc"
};
char gRomajiKanaTbl_129_kana_2[8] = {
#include "sub_main/0046B518.inc"
};
char gRomajiKanaTbl_130_romaji[8] = {
#include "sub_main/0046B520.inc"
};
char gRomajiKanaTbl_130_kana_0[8] = {
#include "sub_main/0046B528.inc"
};
char gRomajiKanaTbl_130_kana_1[8] = {
#include "sub_main/0046B530.inc"
};
char gRomajiKanaTbl_130_kana_2[8] = {
#include "sub_main/0046B538.inc"
};
char gRomajiKanaTbl_131_romaji[8] = {
#include "sub_main/0046B540.inc"
};
char gRomajiKanaTbl_131_kana_0[8] = {
#include "sub_main/0046B548.inc"
};
char gRomajiKanaTbl_131_kana_1[8] = {
#include "sub_main/0046B550.inc"
};
char gRomajiKanaTbl_131_kana_2[8] = {
#include "sub_main/0046B558.inc"
};
char gRomajiKanaTbl_132_romaji[8] = {
#include "sub_main/0046B560.inc"
};
char gRomajiKanaTbl_132_kana_0[8] = {
#include "sub_main/0046B568.inc"
};
char gRomajiKanaTbl_132_kana_1[8] = {
#include "sub_main/0046B570.inc"
};
char gRomajiKanaTbl_132_kana_2[8] = {
#include "sub_main/0046B578.inc"
};
char gRomajiKanaTbl_133_romaji[8] = {
#include "sub_main/0046B580.inc"
};
char gRomajiKanaTbl_133_kana_0[8] = {
#include "sub_main/0046B588.inc"
};
char gRomajiKanaTbl_133_kana_1[8] = {
#include "sub_main/0046B590.inc"
};
char gRomajiKanaTbl_133_kana_2[8] = {
#include "sub_main/0046B598.inc"
};
char gRomajiKanaTbl_134_romaji[8] = {
#include "sub_main/0046B5A0.inc"
};
char gRomajiKanaTbl_134_kana_0[8] = {
#include "sub_main/0046B5A8.inc"
};
char gRomajiKanaTbl_134_kana_1[8] = {
#include "sub_main/0046B5B0.inc"
};
char gRomajiKanaTbl_134_kana_2[8] = {
#include "sub_main/0046B5B8.inc"
};
char gRomajiKanaTbl_135_romaji[8] = {
#include "sub_main/0046B5C0.inc"
};
char gRomajiKanaTbl_135_kana_0[8] = {
#include "sub_main/0046B5C8.inc"
};
char gRomajiKanaTbl_135_kana_1[8] = {
#include "sub_main/0046B5D0.inc"
};
char gRomajiKanaTbl_135_kana_2[8] = {
#include "sub_main/0046B5D8.inc"
};
char gRomajiKanaTbl_136_romaji[8] = {
#include "sub_main/0046B5E0.inc"
};
char gRomajiKanaTbl_136_kana_0[8] = {
#include "sub_main/0046B5E8.inc"
};
char gRomajiKanaTbl_136_kana_1[8] = {
#include "sub_main/0046B5F0.inc"
};
char gRomajiKanaTbl_136_kana_2[8] = {
#include "sub_main/0046B5F8.inc"
};
char gRomajiKanaTbl_137_romaji[8] = {
#include "sub_main/0046B600.inc"
};
char gRomajiKanaTbl_137_kana_0[8] = {
#include "sub_main/0046B608.inc"
};
char gRomajiKanaTbl_137_kana_1[8] = {
#include "sub_main/0046B610.inc"
};
char gRomajiKanaTbl_137_kana_2[8] = {
#include "sub_main/0046B618.inc"
};
char gRomajiKanaTbl_138_romaji[8] = {
#include "sub_main/0046B620.inc"
};
char gRomajiKanaTbl_138_kana_0[8] = {
#include "sub_main/0046B628.inc"
};
char gRomajiKanaTbl_138_kana_1[8] = {
#include "sub_main/0046B630.inc"
};
char gRomajiKanaTbl_138_kana_2[8] = {
#include "sub_main/0046B638.inc"
};
char gRomajiKanaTbl_139_romaji[8] = {
#include "sub_main/0046B640.inc"
};
char gRomajiKanaTbl_139_kana_0[8] = {
#include "sub_main/0046B648.inc"
};
char gRomajiKanaTbl_139_kana_1[8] = {
#include "sub_main/0046B650.inc"
};
char gRomajiKanaTbl_139_kana_2[8] = {
#include "sub_main/0046B658.inc"
};
char gRomajiKanaTbl_140_romaji[8] = {
#include "sub_main/0046B660.inc"
};
char gRomajiKanaTbl_140_kana_0[8] = {
#include "sub_main/0046B668.inc"
};
char gRomajiKanaTbl_140_kana_1[8] = {
#include "sub_main/0046B670.inc"
};
char gRomajiKanaTbl_140_kana_2[8] = {
#include "sub_main/0046B678.inc"
};
char gRomajiKanaTbl_141_romaji[8] = {
#include "sub_main/0046B680.inc"
};
char gRomajiKanaTbl_141_kana_0[8] = {
#include "sub_main/0046B688.inc"
};
char gRomajiKanaTbl_141_kana_1[8] = {
#include "sub_main/0046B690.inc"
};
char gRomajiKanaTbl_141_kana_2[8] = {
#include "sub_main/0046B698.inc"
};
char gRomajiKanaTbl_142_romaji[8] = {
#include "sub_main/0046B6A0.inc"
};
char gRomajiKanaTbl_142_kana_0[8] = {
#include "sub_main/0046B6A8.inc"
};
char gRomajiKanaTbl_142_kana_1[8] = {
#include "sub_main/0046B6B0.inc"
};
char gRomajiKanaTbl_142_kana_2[8] = {
#include "sub_main/0046B6B8.inc"
};
char gRomajiKanaTbl_143_romaji[8] = {
#include "sub_main/0046B6C0.inc"
};
char gRomajiKanaTbl_143_kana_0[8] = {
#include "sub_main/0046B6C8.inc"
};
char gRomajiKanaTbl_143_kana_1[8] = {
#include "sub_main/0046B6D0.inc"
};
char gRomajiKanaTbl_143_kana_2[8] = {
#include "sub_main/0046B6D8.inc"
};
char gRomajiKanaTbl_144_romaji[8] = {
#include "sub_main/0046B6E0.inc"
};
char gRomajiKanaTbl_144_kana_0[8] = {
#include "sub_main/0046B6E8.inc"
};
char gRomajiKanaTbl_144_kana_1[8] = {
#include "sub_main/0046B6F0.inc"
};
char gRomajiKanaTbl_144_kana_2[8] = {
#include "sub_main/0046B6F8.inc"
};
char gRomajiKanaTbl_145_romaji[8] = {
#include "sub_main/0046B700.inc"
};
char gRomajiKanaTbl_145_kana_0[8] = {
#include "sub_main/0046B708.inc"
};
char gRomajiKanaTbl_145_kana_1[8] = {
#include "sub_main/0046B710.inc"
};
char gRomajiKanaTbl_145_kana_2[8] = {
#include "sub_main/0046B718.inc"
};
char gRomajiKanaTbl_146_romaji[8] = {
#include "sub_main/0046B720.inc"
};
char gRomajiKanaTbl_146_kana_0[8] = {
#include "sub_main/0046B728.inc"
};
char gRomajiKanaTbl_146_kana_1[8] = {
#include "sub_main/0046B730.inc"
};
char gRomajiKanaTbl_146_kana_2[8] = {
#include "sub_main/0046B738.inc"
};
char gRomajiKanaTbl_147_romaji[8] = {
#include "sub_main/0046B740.inc"
};
char gRomajiKanaTbl_147_kana_0[8] = {
#include "sub_main/0046B748.inc"
};
char gRomajiKanaTbl_147_kana_1[8] = {
#include "sub_main/0046B750.inc"
};
char gRomajiKanaTbl_147_kana_2[8] = {
#include "sub_main/0046B758.inc"
};
char gRomajiKanaTbl_148_romaji[8] = {
#include "sub_main/0046B760.inc"
};
char gRomajiKanaTbl_148_kana_0[8] = {
#include "sub_main/0046B768.inc"
};
char gRomajiKanaTbl_148_kana_1[8] = {
#include "sub_main/0046B770.inc"
};
char gRomajiKanaTbl_148_kana_2[8] = {
#include "sub_main/0046B778.inc"
};
char gRomajiKanaTbl_149_romaji[8] = {
#include "sub_main/0046B780.inc"
};
char gRomajiKanaTbl_149_kana_0[8] = {
#include "sub_main/0046B788.inc"
};
char gRomajiKanaTbl_149_kana_1[8] = {
#include "sub_main/0046B790.inc"
};
char gRomajiKanaTbl_149_kana_2[8] = {
#include "sub_main/0046B798.inc"
};
char gRomajiKanaTbl_150_romaji[8] = {
#include "sub_main/0046B7A0.inc"
};
char gRomajiKanaTbl_150_kana_0[8] = {
#include "sub_main/0046B7A8.inc"
};
char gRomajiKanaTbl_150_kana_1[8] = {
#include "sub_main/0046B7B0.inc"
};
char gRomajiKanaTbl_150_kana_2[8] = {
#include "sub_main/0046B7B8.inc"
};
char gRomajiKanaTbl_151_romaji[8] = {
#include "sub_main/0046B7C0.inc"
};
char gRomajiKanaTbl_151_kana_0[8] = {
#include "sub_main/0046B7C8.inc"
};
char gRomajiKanaTbl_151_kana_1[8] = {
#include "sub_main/0046B7D0.inc"
};
char gRomajiKanaTbl_151_kana_2[8] = {
#include "sub_main/0046B7D8.inc"
};
char gRomajiKanaTbl_152_romaji[8] = {
#include "sub_main/0046B7E0.inc"
};
char gRomajiKanaTbl_152_kana_0[8] = {
#include "sub_main/0046B7E8.inc"
};
char gRomajiKanaTbl_152_kana_1[8] = {
#include "sub_main/0046B7F0.inc"
};
char gRomajiKanaTbl_152_kana_2[8] = {
#include "sub_main/0046B7F8.inc"
};
char gRomajiKanaTbl_153_romaji[8] = {
#include "sub_main/0046B800.inc"
};
char gRomajiKanaTbl_153_kana_0[8] = {
#include "sub_main/0046B808.inc"
};
char gRomajiKanaTbl_153_kana_1[8] = {
#include "sub_main/0046B810.inc"
};
char gRomajiKanaTbl_153_kana_2[8] = {
#include "sub_main/0046B818.inc"
};
char gRomajiKanaTbl_154_romaji[8] = {
#include "sub_main/0046B820.inc"
};
char gRomajiKanaTbl_154_kana_0[8] = {
#include "sub_main/0046B828.inc"
};
char gRomajiKanaTbl_154_kana_1[8] = {
#include "sub_main/0046B830.inc"
};
char gRomajiKanaTbl_154_kana_2[8] = {
#include "sub_main/0046B838.inc"
};
char gRomajiKanaTbl_155_romaji[8] = {
#include "sub_main/0046B840.inc"
};
char D_0046B848[8] = {
#include "sub_main/0046B848.inc"
};
char D_0046B850[8] = {
#include "sub_main/0046B850.inc"
};
char D_0046B858[8] = {
#include "sub_main/0046B858.inc"
};
char gRomajiKanaTbl_156_romaji[8] = {
#include "sub_main/0046B860.inc"
};
char gRomajiKanaTbl_156_kana_0[8] = {
#include "sub_main/0046B868.inc"
};
char gRomajiKanaTbl_156_kana_1[8] = {
#include "sub_main/0046B870.inc"
};
char gRomajiKanaTbl_156_kana_2[8] = {
#include "sub_main/0046B878.inc"
};
char gRomajiKanaTbl_157_romaji[8] = {
#include "sub_main/0046B880.inc"
};
char gRomajiKanaTbl_157_kana_0[8] = {
#include "sub_main/0046B888.inc"
};
char gRomajiKanaTbl_157_kana_1[8] = {
#include "sub_main/0046B890.inc"
};
char gRomajiKanaTbl_157_kana_2[8] = {
#include "sub_main/0046B898.inc"
};
char gRomajiKanaTbl_158_romaji[8] = {
#include "sub_main/0046B8A0.inc"
};
char gRomajiKanaTbl_159_romaji[8] = {
#include "sub_main/0046B8A8.inc"
};
char gRomajiKanaTbl_159_kana_0[8] = {
#include "sub_main/0046B8B0.inc"
};
char gRomajiKanaTbl_159_kana_1[8] = {
#include "sub_main/0046B8B8.inc"
};
char gRomajiKanaTbl_159_kana_2[8] = {
#include "sub_main/0046B8C0.inc"
};
char gRomajiKanaTbl_160_romaji[8] = {
#include "sub_main/0046B8C8.inc"
};
char gRomajiKanaTbl_160_kana_0[8] = {
#include "sub_main/0046B8D0.inc"
};
char gRomajiKanaTbl_160_kana_1[8] = {
#include "sub_main/0046B8D8.inc"
};
char gRomajiKanaTbl_160_kana_2[8] = {
#include "sub_main/0046B8E0.inc"
};
char gRomajiKanaTbl_161_romaji[8] = {
#include "sub_main/0046B8E8.inc"
};
char gRomajiKanaTbl_161_kana_0[8] = {
#include "sub_main/0046B8F0.inc"
};
char gRomajiKanaTbl_161_kana_1[8] = {
#include "sub_main/0046B8F8.inc"
};
char gRomajiKanaTbl_161_kana_2[8] = {
#include "sub_main/0046B900.inc"
};
char gRomajiKanaTbl_162_romaji[8] = {
#include "sub_main/0046B908.inc"
};
char gRomajiKanaTbl_162_kana_0[8] = {
#include "sub_main/0046B910.inc"
};
char gRomajiKanaTbl_162_kana_1[8] = {
#include "sub_main/0046B918.inc"
};
char gRomajiKanaTbl_162_kana_2[8] = {
#include "sub_main/0046B920.inc"
};
char gRomajiKanaTbl_163_romaji[8] = {
#include "sub_main/0046B928.inc"
};
char gRomajiKanaTbl_163_kana_0[8] = {
#include "sub_main/0046B930.inc"
};
char gRomajiKanaTbl_163_kana_1[8] = {
#include "sub_main/0046B938.inc"
};
char gRomajiKanaTbl_163_kana_2[8] = {
#include "sub_main/0046B940.inc"
};
char gRomajiKanaTbl_164_romaji[8] = {
#include "sub_main/0046B948.inc"
};
char gRomajiKanaTbl_164_kana_0[8] = {
#include "sub_main/0046B950.inc"
};
char gRomajiKanaTbl_164_kana_1[8] = {
#include "sub_main/0046B958.inc"
};
char gRomajiKanaTbl_164_kana_2[8] = {
#include "sub_main/0046B960.inc"
};
char gRomajiKanaTbl_165_romaji[8] = {
#include "sub_main/0046B968.inc"
};
char gRomajiKanaTbl_165_kana_0[8] = {
#include "sub_main/0046B970.inc"
};
char gRomajiKanaTbl_165_kana_1[8] = {
#include "sub_main/0046B978.inc"
};
char gRomajiKanaTbl_165_kana_2[8] = {
#include "sub_main/0046B980.inc"
};
char gRomajiKanaTbl_166_romaji[8] = {
#include "sub_main/0046B988.inc"
};
char gRomajiKanaTbl_166_kana_0[8] = {
#include "sub_main/0046B990.inc"
};
char gRomajiKanaTbl_166_kana_1[8] = {
#include "sub_main/0046B998.inc"
};
char gRomajiKanaTbl_166_kana_2[8] = {
#include "sub_main/0046B9A0.inc"
};
char gRomajiKanaTbl_167_romaji[8] = {
#include "sub_main/0046B9A8.inc"
};
char gRomajiKanaTbl_167_kana_0[8] = {
#include "sub_main/0046B9B0.inc"
};
char gRomajiKanaTbl_167_kana_1[8] = {
#include "sub_main/0046B9B8.inc"
};
char gRomajiKanaTbl_167_kana_2[8] = {
#include "sub_main/0046B9C0.inc"
};
char gRomajiKanaTbl_168_romaji[8] = {
#include "sub_main/0046B9C8.inc"
};
char gRomajiKanaTbl_168_kana_0[8] = {
#include "sub_main/0046B9D0.inc"
};
char gRomajiKanaTbl_168_kana_1[8] = {
#include "sub_main/0046B9D8.inc"
};
char gRomajiKanaTbl_168_kana_2[8] = {
#include "sub_main/0046B9E0.inc"
};
char gRomajiKanaTbl_169_romaji[8] = {
#include "sub_main/0046B9E8.inc"
};
char gRomajiKanaTbl_169_kana_0[8] = {
#include "sub_main/0046B9F0.inc"
};
char gRomajiKanaTbl_169_kana_1[8] = {
#include "sub_main/0046B9F8.inc"
};
char gRomajiKanaTbl_169_kana_2[8] = {
#include "sub_main/0046BA00.inc"
};
char gRomajiKanaTbl_170_romaji[8] = {
#include "sub_main/0046BA08.inc"
};
char gRomajiKanaTbl_170_kana_0[8] = {
#include "sub_main/0046BA10.inc"
};
char gRomajiKanaTbl_170_kana_1[8] = {
#include "sub_main/0046BA18.inc"
};
char gRomajiKanaTbl_170_kana_2[8] = {
#include "sub_main/0046BA20.inc"
};
char gRomajiKanaTbl_171_romaji[8] = {
#include "sub_main/0046BA28.inc"
};
char gRomajiKanaTbl_171_kana_0[8] = {
#include "sub_main/0046BA30.inc"
};
char gRomajiKanaTbl_171_kana_1[8] = {
#include "sub_main/0046BA38.inc"
};
char gRomajiKanaTbl_171_kana_2[8] = {
#include "sub_main/0046BA40.inc"
};
char gRomajiKanaTbl_172_romaji[8] = {
#include "sub_main/0046BA48.inc"
};
char gRomajiKanaTbl_172_kana_0[8] = {
#include "sub_main/0046BA50.inc"
};
char gRomajiKanaTbl_172_kana_1[8] = {
#include "sub_main/0046BA58.inc"
};
char gRomajiKanaTbl_172_kana_2[8] = {
#include "sub_main/0046BA60.inc"
};
char gRomajiKanaTbl_173_romaji[8] = {
#include "sub_main/0046BA68.inc"
};
char gRomajiKanaTbl_173_kana_0[8] = {
#include "sub_main/0046BA70.inc"
};
char gRomajiKanaTbl_173_kana_1[8] = {
#include "sub_main/0046BA78.inc"
};
char gRomajiKanaTbl_173_kana_2[8] = {
#include "sub_main/0046BA80.inc"
};
char gRomajiKanaTbl_174_romaji[8] = {
#include "sub_main/0046BA88.inc"
};
char gRomajiKanaTbl_174_kana_0[8] = {
#include "sub_main/0046BA90.inc"
};
char gRomajiKanaTbl_174_kana_1[8] = {
#include "sub_main/0046BA98.inc"
};
char gRomajiKanaTbl_174_kana_2[8] = {
#include "sub_main/0046BAA0.inc"
};
char gRomajiKanaTbl_175_romaji[8] = {
#include "sub_main/0046BAA8.inc"
};
char gRomajiKanaTbl_175_kana_0[8] = {
#include "sub_main/0046BAB0.inc"
};
char gRomajiKanaTbl_175_kana_1[8] = {
#include "sub_main/0046BAB8.inc"
};
char gRomajiKanaTbl_175_kana_2[8] = {
#include "sub_main/0046BAC0.inc"
};
char gRomajiKanaTbl_176_romaji[8] = {
#include "sub_main/0046BAC8.inc"
};
char gRomajiKanaTbl_176_kana_0[8] = {
#include "sub_main/0046BAD0.inc"
};
char gRomajiKanaTbl_176_kana_1[8] = {
#include "sub_main/0046BAD8.inc"
};
char gRomajiKanaTbl_176_kana_2[8] = {
#include "sub_main/0046BAE0.inc"
};
char gRomajiKanaTbl_177_romaji[8] = {
#include "sub_main/0046BAE8.inc"
};
char gRomajiKanaTbl_177_kana_0[8] = {
#include "sub_main/0046BAF0.inc"
};
char gRomajiKanaTbl_177_kana_1[8] = {
#include "sub_main/0046BAF8.inc"
};
char gRomajiKanaTbl_177_kana_2[8] = {
#include "sub_main/0046BB00.inc"
};
char gRomajiKanaTbl_178_romaji[8] = {
#include "sub_main/0046BB08.inc"
};
char D_0046BB10[8] = {
#include "sub_main/0046BB10.inc"
};
char D_0046BB18[8] = {
#include "sub_main/0046BB18.inc"
};
char D_0046BB20[8] = {
#include "sub_main/0046BB20.inc"
};
char gRomajiKanaTbl_179_romaji[8] = {
#include "sub_main/0046BB28.inc"
};
char D_0046BB30[8] = {
#include "sub_main/0046BB30.inc"
};
char D_0046BB38[8] = {
#include "sub_main/0046BB38.inc"
};
char D_0046BB40[8] = {
#include "sub_main/0046BB40.inc"
};
char gRomajiKanaTbl_180_romaji[8] = {
#include "sub_main/0046BB48.inc"
};
char gRomajiKanaTbl_180_kana_0[8] = {
#include "sub_main/0046BB50.inc"
};
char gRomajiKanaTbl_180_kana_1[8] = {
#include "sub_main/0046BB58.inc"
};
char gRomajiKanaTbl_180_kana_2[8] = {
#include "sub_main/0046BB60.inc"
};
char gRomajiKanaTbl_181_romaji[8] = {
#include "sub_main/0046BB68.inc"
};
char gRomajiKanaTbl_182_romaji[8] = {
#include "sub_main/0046BB70.inc"
};
char gRomajiKanaTbl_183_romaji[8] = {
#include "sub_main/0046BB78.inc"
};
char gRomajiKanaTbl_183_kana_0[8] = {
#include "sub_main/0046BB80.inc"
};
char gRomajiKanaTbl_183_kana_1[8] = {
#include "sub_main/0046BB88.inc"
};
char gRomajiKanaTbl_183_kana_2[8] = {
#include "sub_main/0046BB90.inc"
};
char gRomajiKanaTbl_184_romaji[8] = {
#include "sub_main/0046BB98.inc"
};
char gRomajiKanaTbl_184_kana_0[8] = {
#include "sub_main/0046BBA0.inc"
};
char gRomajiKanaTbl_184_kana_1[8] = {
#include "sub_main/0046BBA8.inc"
};
char gRomajiKanaTbl_184_kana_2[8] = {
#include "sub_main/0046BBB0.inc"
};
char gRomajiKanaTbl_185_romaji[8] = {
#include "sub_main/0046BBB8.inc"
};
char gRomajiKanaTbl_185_kana_0[8] = {
#include "sub_main/0046BBC0.inc"
};
char gRomajiKanaTbl_185_kana_1[8] = {
#include "sub_main/0046BBC8.inc"
};
char gRomajiKanaTbl_185_kana_2[8] = {
#include "sub_main/0046BBD0.inc"
};
char gRomajiKanaTbl_186_romaji[8] = {
#include "sub_main/0046BBD8.inc"
};
char gRomajiKanaTbl_186_kana_0[8] = {
#include "sub_main/0046BBE0.inc"
};
char gRomajiKanaTbl_186_kana_1[8] = {
#include "sub_main/0046BBE8.inc"
};
char gRomajiKanaTbl_186_kana_2[8] = {
#include "sub_main/0046BBF0.inc"
};
char gRomajiKanaTbl_187_romaji[8] = {
#include "sub_main/0046BBF8.inc"
};
char gRomajiKanaTbl_187_kana_0[8] = {
#include "sub_main/0046BC00.inc"
};
char gRomajiKanaTbl_187_kana_1[8] = {
#include "sub_main/0046BC08.inc"
};
char gRomajiKanaTbl_187_kana_2[8] = {
#include "sub_main/0046BC10.inc"
};
char gRomajiKanaTbl_188_romaji[8] = {
#include "sub_main/0046BC18.inc"
};
char gRomajiKanaTbl_188_kana_0[8] = {
#include "sub_main/0046BC20.inc"
};
char gRomajiKanaTbl_188_kana_1[8] = {
#include "sub_main/0046BC28.inc"
};
char gRomajiKanaTbl_188_kana_2[8] = {
#include "sub_main/0046BC30.inc"
};
char gRomajiKanaTbl_189_romaji[8] = {
#include "sub_main/0046BC38.inc"
};
char D_0046BC40[8] = {
#include "sub_main/0046BC40.inc"
};
char D_0046BC48[8] = {
#include "sub_main/0046BC48.inc"
};
char D_0046BC50[8] = {
#include "sub_main/0046BC50.inc"
};
char gRomajiKanaTbl_190_romaji[8] = {
#include "sub_main/0046BC58.inc"
};
char D_0046BC60[8] = {
#include "sub_main/0046BC60.inc"
};
char gRomajiKanaTbl_190_kana_2[8] = {
#include "sub_main/0046BC68.inc"
};
char gRomajiKanaTbl_191_romaji[8] = {
#include "sub_main/0046BC70.inc"
};
char D_0046BC78[8] = {
#include "sub_main/0046BC78.inc"
};
char D_0046BC80[8] = {
#include "sub_main/0046BC80.inc"
};
char D_0046BC88[8] = {
#include "sub_main/0046BC88.inc"
};
char gRomajiKanaTbl_192_romaji[8] = {
#include "sub_main/0046BC90.inc"
};
char D_0046BC98[8] = {
#include "sub_main/0046BC98.inc"
};
char D_0046BCA0[8] = {
#include "sub_main/0046BCA0.inc"
};
char D_0046BCA8[8] = {
#include "sub_main/0046BCA8.inc"
};
char gRomajiKanaTbl_193_romaji[8] = {
#include "sub_main/0046BCB0.inc"
};
char gRomajiKanaTbl_193_kana_0[8] = {
#include "sub_main/0046BCB8.inc"
};
char gRomajiKanaTbl_193_kana_1[8] = {
#include "sub_main/0046BCC0.inc"
};
char gRomajiKanaTbl_193_kana_2[8] = {
#include "sub_main/0046BCC8.inc"
};
char gRomajiKanaTbl_194_romaji[8] = {
#include "sub_main/0046BCD0.inc"
};
char gRomajiKanaTbl_195_romaji[8] = {
#include "sub_main/0046BCD8.inc"
};
char gRomajiKanaTbl_195_kana_0[8] = {
#include "sub_main/0046BCE0.inc"
};
char gRomajiKanaTbl_195_kana_1[8] = {
#include "sub_main/0046BCE8.inc"
};
char gRomajiKanaTbl_195_kana_2[8] = {
#include "sub_main/0046BCF0.inc"
};
char gRomajiKanaTbl_196_romaji[8] = {
#include "sub_main/0046BCF8.inc"
};
char gRomajiKanaTbl_197_romaji[8] = {
#include "sub_main/0046BD00.inc"
};
char gRomajiKanaTbl_198_romaji[8] = {
#include "sub_main/0046BD08.inc"
};
char gRomajiKanaTbl_198_kana_0[8] = {
#include "sub_main/0046BD10.inc"
};
char gRomajiKanaTbl_198_kana_1[8] = {
#include "sub_main/0046BD18.inc"
};
char gRomajiKanaTbl_198_kana_2[8] = {
#include "sub_main/0046BD20.inc"
};
char gRomajiKanaTbl_199_romaji[8] = {
#include "sub_main/0046BD28.inc"
};
char gRomajiKanaTbl_199_kana_0[8] = {
#include "sub_main/0046BD30.inc"
};
char gRomajiKanaTbl_199_kana_1[8] = {
#include "sub_main/0046BD38.inc"
};
char gRomajiKanaTbl_199_kana_2[8] = {
#include "sub_main/0046BD40.inc"
};
char gRomajiKanaTbl_200_romaji[8] = {
#include "sub_main/0046BD48.inc"
};
char gRomajiKanaTbl_200_kana_0[8] = {
#include "sub_main/0046BD50.inc"
};
char gRomajiKanaTbl_200_kana_1[8] = {
#include "sub_main/0046BD58.inc"
};
char gRomajiKanaTbl_200_kana_2[8] = {
#include "sub_main/0046BD60.inc"
};
char gRomajiKanaTbl_201_romaji[8] = {
#include "sub_main/0046BD68.inc"
};
char gRomajiKanaTbl_201_kana_0[8] = {
#include "sub_main/0046BD70.inc"
};
char gRomajiKanaTbl_201_kana_1[8] = {
#include "sub_main/0046BD78.inc"
};
char gRomajiKanaTbl_201_kana_2[8] = {
#include "sub_main/0046BD80.inc"
};
char gRomajiKanaTbl_202_romaji[8] = {
#include "sub_main/0046BD88.inc"
};
char gRomajiKanaTbl_202_kana_0[8] = {
#include "sub_main/0046BD90.inc"
};
char gRomajiKanaTbl_202_kana_1[8] = {
#include "sub_main/0046BD98.inc"
};
char gRomajiKanaTbl_202_kana_2[8] = {
#include "sub_main/0046BDA0.inc"
};
char gRomajiKanaTbl_203_romaji[8] = {
#include "sub_main/0046BDA8.inc"
};
char gRomajiKanaTbl_203_kana_0[8] = {
#include "sub_main/0046BDB0.inc"
};
char gRomajiKanaTbl_203_kana_1[8] = {
#include "sub_main/0046BDB8.inc"
};
char gRomajiKanaTbl_203_kana_2[8] = {
#include "sub_main/0046BDC0.inc"
};
char gRomajiKanaTbl_204_romaji[8] = {
#include "sub_main/0046BDC8.inc"
};
char gRomajiKanaTbl_204_kana_0[8] = {
#include "sub_main/0046BDD0.inc"
};
char gRomajiKanaTbl_204_kana_1[8] = {
#include "sub_main/0046BDD8.inc"
};
char gRomajiKanaTbl_204_kana_2[8] = {
#include "sub_main/0046BDE0.inc"
};
char gRomajiKanaTbl_205_romaji[8] = {
#include "sub_main/0046BDE8.inc"
};
char gRomajiKanaTbl_205_kana_0[8] = {
#include "sub_main/0046BDF0.inc"
};
char gRomajiKanaTbl_205_kana_1[8] = {
#include "sub_main/0046BDF8.inc"
};
char gRomajiKanaTbl_205_kana_2[8] = {
#include "sub_main/0046BE00.inc"
};
char gRomajiKanaTbl_206_romaji[8] = {
#include "sub_main/0046BE08.inc"
};
char gRomajiKanaTbl_206_kana_0[8] = {
#include "sub_main/0046BE10.inc"
};
char gRomajiKanaTbl_206_kana_1[8] = {
#include "sub_main/0046BE18.inc"
};
char gRomajiKanaTbl_206_kana_2[8] = {
#include "sub_main/0046BE20.inc"
};
char gRomajiKanaTbl_207_romaji[8] = {
#include "sub_main/0046BE28.inc"
};
char gRomajiKanaTbl_207_kana_0[8] = {
#include "sub_main/0046BE30.inc"
};
char gRomajiKanaTbl_207_kana_1[8] = {
#include "sub_main/0046BE38.inc"
};
char gRomajiKanaTbl_207_kana_2[8] = {
#include "sub_main/0046BE40.inc"
};
char gRomajiKanaTbl_208_romaji[8] = {
#include "sub_main/0046BE48.inc"
};
char gRomajiKanaTbl_208_kana_0[8] = {
#include "sub_main/0046BE50.inc"
};
char gRomajiKanaTbl_208_kana_1[8] = {
#include "sub_main/0046BE58.inc"
};
char gRomajiKanaTbl_208_kana_2[8] = {
#include "sub_main/0046BE60.inc"
};
char gRomajiKanaTbl_209_romaji[8] = {
#include "sub_main/0046BE68.inc"
};
char gRomajiKanaTbl_209_kana_0[8] = {
#include "sub_main/0046BE70.inc"
};
char gRomajiKanaTbl_209_kana_1[8] = {
#include "sub_main/0046BE78.inc"
};
char gRomajiKanaTbl_209_kana_2[8] = {
#include "sub_main/0046BE80.inc"
};
char gRomajiKanaTbl_210_romaji[8] = {
#include "sub_main/0046BE88.inc"
};
char gRomajiKanaTbl_210_kana_0[8] = {
#include "sub_main/0046BE90.inc"
};
char gRomajiKanaTbl_210_kana_1[8] = {
#include "sub_main/0046BE98.inc"
};
char gRomajiKanaTbl_210_kana_2[8] = {
#include "sub_main/0046BEA0.inc"
};
char gRomajiKanaTbl_211_romaji[8] = {
#include "sub_main/0046BEA8.inc"
};
char gRomajiKanaTbl_211_kana_0[8] = {
#include "sub_main/0046BEB0.inc"
};
char gRomajiKanaTbl_211_kana_1[8] = {
#include "sub_main/0046BEB8.inc"
};
char gRomajiKanaTbl_211_kana_2[8] = {
#include "sub_main/0046BEC0.inc"
};
char gRomajiKanaTbl_212_romaji[8] = {
#include "sub_main/0046BEC8.inc"
};
char gRomajiKanaTbl_212_kana_0[8] = {
#include "sub_main/0046BED0.inc"
};
char gRomajiKanaTbl_212_kana_1[8] = {
#include "sub_main/0046BED8.inc"
};
char gRomajiKanaTbl_212_kana_2[8] = {
#include "sub_main/0046BEE0.inc"
};
char gRomajiKanaTbl_213_romaji[8] = {
#include "sub_main/0046BEE8.inc"
};
char gRomajiKanaTbl_213_kana_0[8] = {
#include "sub_main/0046BEF0.inc"
};
char gRomajiKanaTbl_213_kana_1[8] = {
#include "sub_main/0046BEF8.inc"
};
char gRomajiKanaTbl_213_kana_2[8] = {
#include "sub_main/0046BF00.inc"
};
char gRomajiKanaTbl_214_romaji[8] = {
#include "sub_main/0046BF08.inc"
};
char gRomajiKanaTbl_214_kana_0[8] = {
#include "sub_main/0046BF10.inc"
};
char gRomajiKanaTbl_214_kana_1[8] = {
#include "sub_main/0046BF18.inc"
};
char gRomajiKanaTbl_214_kana_2[8] = {
#include "sub_main/0046BF20.inc"
};
char gRomajiKanaTbl_215_romaji[8] = {
#include "sub_main/0046BF28.inc"
};
char gRomajiKanaTbl_215_kana_0[8] = {
#include "sub_main/0046BF30.inc"
};
char gRomajiKanaTbl_215_kana_1[8] = {
#include "sub_main/0046BF38.inc"
};
char gRomajiKanaTbl_215_kana_2[8] = {
#include "sub_main/0046BF40.inc"
};
char gRomajiKanaTbl_216_romaji[8] = {
#include "sub_main/0046BF48.inc"
};
char D_0046BF50[8] = {
#include "sub_main/0046BF50.inc"
};
char D_0046BF58[8] = {
#include "sub_main/0046BF58.inc"
};
char D_0046BF60[8] = {
#include "sub_main/0046BF60.inc"
};
char gRomajiKanaTbl_217_romaji[8] = {
#include "sub_main/0046BF68.inc"
};
char D_0046BF70[8] = {
#include "sub_main/0046BF70.inc"
};
char D_0046BF78[8] = {
#include "sub_main/0046BF78.inc"
};
char D_0046BF80[8] = {
#include "sub_main/0046BF80.inc"
};
char gRomajiKanaTbl_218_romaji[8] = {
#include "sub_main/0046BF88.inc"
};
char D_0046BF90[8] = {
#include "sub_main/0046BF90.inc"
};
char D_0046BF98[8] = {
#include "sub_main/0046BF98.inc"
};
char D_0046BFA0[8] = {
#include "sub_main/0046BFA0.inc"
};
char gRomajiKanaTbl_219_romaji[8] = {
#include "sub_main/0046BFA8.inc"
};
char gRomajiKanaTbl_220_romaji[8] = {
#include "sub_main/0046BFB0.inc"
};
char gRomajiKanaTbl_221_romaji[8] = {
#include "sub_main/0046BFB8.inc"
};
char gRomajiKanaTbl_222_romaji[8] = {
#include "sub_main/0046BFC0.inc"
};
char gRomajiKanaTbl_222_kana_0[8] = {
#include "sub_main/0046BFC8.inc"
};
char gRomajiKanaTbl_222_kana_1[8] = {
#include "sub_main/0046BFD0.inc"
};
char gRomajiKanaTbl_222_kana_2[8] = {
#include "sub_main/0046BFD8.inc"
};
char gRomajiKanaTbl_223_romaji[8] = {
#include "sub_main/0046BFE0.inc"
};
char gRomajiKanaTbl_223_kana_0[8] = {
#include "sub_main/0046BFE8.inc"
};
char gRomajiKanaTbl_223_kana_1[8] = {
#include "sub_main/0046BFF0.inc"
};
char gRomajiKanaTbl_223_kana_2[8] = {
#include "sub_main/0046BFF8.inc"
};
char gRomajiKanaTbl_224_romaji[8] = {
#include "sub_main/0046C000.inc"
};
char gRomajiKanaTbl_224_kana_0[8] = {
#include "sub_main/0046C008.inc"
};
char gRomajiKanaTbl_224_kana_1[8] = {
#include "sub_main/0046C010.inc"
};
char gRomajiKanaTbl_224_kana_2[8] = {
#include "sub_main/0046C018.inc"
};
char gRomajiKanaTbl_225_romaji[8] = {
#include "sub_main/0046C020.inc"
};
char gRomajiKanaTbl_225_kana_0[8] = {
#include "sub_main/0046C028.inc"
};
char gRomajiKanaTbl_225_kana_1[8] = {
#include "sub_main/0046C030.inc"
};
char gRomajiKanaTbl_225_kana_2[8] = {
#include "sub_main/0046C038.inc"
};
char gRomajiKanaTbl_226_romaji[8] = {
#include "sub_main/0046C040.inc"
};
char gRomajiKanaTbl_226_kana_0[8] = {
#include "sub_main/0046C048.inc"
};
char gRomajiKanaTbl_226_kana_1[8] = {
#include "sub_main/0046C050.inc"
};
char gRomajiKanaTbl_226_kana_2[8] = {
#include "sub_main/0046C058.inc"
};
char gRomajiKanaTbl_227_romaji[8] = {
#include "sub_main/0046C060.inc"
};
char gRomajiKanaTbl_227_kana_0[8] = {
#include "sub_main/0046C068.inc"
};
char gRomajiKanaTbl_227_kana_1[8] = {
#include "sub_main/0046C070.inc"
};
char gRomajiKanaTbl_227_kana_2[8] = {
#include "sub_main/0046C078.inc"
};
char gRomajiKanaTbl_228_romaji[8] = {
#include "sub_main/0046C080.inc"
};
char gRomajiKanaTbl_228_kana_0[8] = {
#include "sub_main/0046C088.inc"
};
char gRomajiKanaTbl_228_kana_1[8] = {
#include "sub_main/0046C090.inc"
};
char gRomajiKanaTbl_228_kana_2[8] = {
#include "sub_main/0046C098.inc"
};
char gRomajiKanaTbl_229_romaji[8] = {
#include "sub_main/0046C0A0.inc"
};
char gRomajiKanaTbl_229_kana_0[8] = {
#include "sub_main/0046C0A8.inc"
};
char gRomajiKanaTbl_229_kana_1[8] = {
#include "sub_main/0046C0B0.inc"
};
char gRomajiKanaTbl_229_kana_2[8] = {
#include "sub_main/0046C0B8.inc"
};
char gRomajiKanaTbl_230_romaji[8] = {
#include "sub_main/0046C0C0.inc"
};
char gRomajiKanaTbl_230_kana_0[8] = {
#include "sub_main/0046C0C8.inc"
};
char gRomajiKanaTbl_230_kana_1[8] = {
#include "sub_main/0046C0D0.inc"
};
char gRomajiKanaTbl_230_kana_2[8] = {
#include "sub_main/0046C0D8.inc"
};
char gRomajiKanaTbl_231_romaji[8] = {
#include "sub_main/0046C0E0.inc"
};
char gRomajiKanaTbl_231_kana_0[8] = {
#include "sub_main/0046C0E8.inc"
};
char gRomajiKanaTbl_231_kana_1[8] = {
#include "sub_main/0046C0F0.inc"
};
char gRomajiKanaTbl_231_kana_2[8] = {
#include "sub_main/0046C0F8.inc"
};
char gRomajiKanaTbl_232_romaji[8] = {
#include "sub_main/0046C100.inc"
};
char gRomajiKanaTbl_232_kana_0[8] = {
#include "sub_main/0046C108.inc"
};
char gRomajiKanaTbl_232_kana_1[8] = {
#include "sub_main/0046C110.inc"
};
char gRomajiKanaTbl_232_kana_2[8] = {
#include "sub_main/0046C118.inc"
};
char gRomajiKanaTbl_233_romaji[8] = {
#include "sub_main/0046C120.inc"
};
char D_0046C128[8] = {
#include "sub_main/0046C128.inc"
};
char D_0046C130[8] = {
#include "sub_main/0046C130.inc"
};
char gRomajiKanaTbl_233_kana_2[8] = {
#include "sub_main/0046C138.inc"
};
char gRomajiKanaTbl_234_romaji[8] = {
#include "sub_main/0046C140.inc"
};
char gRomajiKanaTbl_234_kana_2[8] = {
#include "sub_main/0046C148.inc"
};
char gRomajiKanaTbl_235_romaji[8] = {
#include "sub_main/0046C150.inc"
};
char gRomajiKanaTbl_235_kana_0[8] = {
#include "sub_main/0046C158.inc"
};
char gRomajiKanaTbl_235_kana_1[8] = {
#include "sub_main/0046C160.inc"
};
char gRomajiKanaTbl_235_kana_2[8] = {
#include "sub_main/0046C168.inc"
};
char gRomajiKanaTbl_236_romaji[8] = {
#include "sub_main/0046C170.inc"
};
char gRomajiKanaTbl_236_kana_0[8] = {
#include "sub_main/0046C178.inc"
};
char gRomajiKanaTbl_236_kana_1[8] = {
#include "sub_main/0046C180.inc"
};
char gRomajiKanaTbl_236_kana_2[8] = {
#include "sub_main/0046C188.inc"
};
char gRomajiKanaTbl_237_romaji[8] = {
#include "sub_main/0046C190.inc"
};
char gRomajiKanaTbl_237_kana_0[8] = {
#include "sub_main/0046C198.inc"
};
char gRomajiKanaTbl_237_kana_1[8] = {
#include "sub_main/0046C1A0.inc"
};
char gRomajiKanaTbl_237_kana_2[8] = {
#include "sub_main/0046C1A8.inc"
};
char gRomajiKanaTbl_238_romaji[8] = {
#include "sub_main/0046C1B0.inc"
};
char D_0046C1B8[8] = {
#include "sub_main/0046C1B8.inc"
};
char D_0046C1C0[8] = {
#include "sub_main/0046C1C0.inc"
};
char D_0046C1C8[8] = {
#include "sub_main/0046C1C8.inc"
};
char gRomajiKanaTbl_239_romaji[16] = {
#include "sub_main/0046C1D0.inc"
};
char gKbdHidWideStrs_0[128] = {
#include "sub_main/0046C1E0.inc"
};
char gKbdHidWideStrs_1[128] = {
#include "sub_main/0046C260.inc"
};
char gKbdHidWideStrs_2[128] = {
#include "sub_main/0046C2E0.inc"
};
char gKbdHidWideStrs_3[128] = {
#include "sub_main/0046C360.inc"
};
char gKbdHidWideStrs_4[128] = {
#include "sub_main/0046C3E0.inc"
};
char gKbdHidWideStrs_5[128] = {
#include "sub_main/0046C460.inc"
};
char gKbdHidWideStrs_6[128] = {
#include "sub_main/0046C4E0.inc"
};
char gKbdHidWideStrs_7[128] = {
#include "sub_main/0046C560.inc"
};
char gKbdHidWideStrs_8[128] = {
#include "sub_main/0046C5E0.inc"
};
char gKbdHidWideStrs_9[128] = {
#include "sub_main/0046C660.inc"
};
char gKbdHidKeypadWideStrs_0[48] = {
#include "sub_main/0046C6E0.inc"
};
char gKbdHidChars_0[64] = {
#include "sub_main/0046C710.inc"
};
char gKbdHidChars_1[64] = {
#include "sub_main/0046C750.inc"
};
char gKbdHidChars_2[64] = {
#include "sub_main/0046C790.inc"
};
char gKbdHidChars_3[80] = {
#include "sub_main/0046C7D0.inc"
};
char gKbdHidChars_4[64] = {
#include "sub_main/0046C820.inc"
};
char gKbdHidChars_5[64] = {
#include "sub_main/0046C860.inc"
};
char gKbdHidNoRomaji_0[64] = {
#include "sub_main/0046C8A0.inc"
};
char gKbdHidKeypadChars_0[24] = {
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

/*
 * Kbd_Run's USB step: curKey moves to prevKey and usbKey becomes curKey.
 * repeatTimer is cleared when its top bit is set; on a new key (nonzero
 * and not prevKey) it is set to 158 and the key is handled
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
    kbd->repeatTimer = 158;
    Kbd_HandleKey();
    return;
clear:
    *timer = 0;
    goto back;
}

/*
 * Handles a newly pressed USB key (curKey, a HID usage code). Puts the
 * cursor off the grid (offGrid 1) and leaves pad input (Kbd_ClearActive)
 * when the pad was active. Ignores the codes below USB_KEY_A, Print
 * Screen to Pause, Insert, Num Lock, USB_KEY_NONUS_BACKSLASH to USB_KEY_KP_EQUAL_AS400, and USB_KEY_INTL6 on. Enter and
 * keypad Enter: Kbd_Decide. Escape, Backspace, Space (Kbd_Space(0)),
 * Caps Lock, Home, End, Delete, Page Up, Page Down and the arrows: their
 * Kbd_* handlers. Tab: Kbd_NextChatTarget. USB_KEY_LBRACKET and USB_KEY_RBRACKET (the '@' and
 * '[' keys of a Japanese keyboard): Kbd_DakutenKey, Kbd_HandakutenKey.
 * USB_KEY_GRAVE (Hankaku/Zenkaku): Kbd_ZenkakuKey. F1-F12: Kbd_FKey. The
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
    case USB_KEY_F1: case USB_KEY_F2: case USB_KEY_F3: case USB_KEY_F4: case USB_KEY_F5: case USB_KEY_F6:
    case USB_KEY_F7: case USB_KEY_F8: case USB_KEY_F9: case USB_KEY_F10: case USB_KEY_F11: case USB_KEY_F12:
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
extern u8 gKbdSetCharsets[];
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
 * The string is inserted at kanaPos (the kana kept to 72 bytes); a
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
        key = 50;
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
                if (gKbdHidNoRomaji[0][key] == 49) {
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
            if (buf[0] >= 65 && buf[0] < 91) {
                isUpper = 1;
            }
            if (isUpper == 0 && (buf[0] >= 97 && buf[0] < 123) == 0) {
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
        inserted = Str_InsertClip((char *)kbd->kana, buf, *pos, 72);
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
        narrowChar = gKbdHidKeypadChars[0][key - 84];
    } else {
        narrowChar = gKbdHidChars[kind + shift * 3][key];
    }
check:
    acceptBits = gKbd->accept;
    if ((acceptBits & KBD_ACCEPT_DIGITS) && (narrowChar < 48 || narrowChar > 57)) {
        return;
    }
    if ((acceptBits & 0x20) && (narrowChar < 48 || narrowChar > 57) && narrowChar != 45) {
        return;
    }
    if ((acceptBits & 0x40) && (narrowChar < 48 || narrowChar > 57) && (narrowChar < 65 || narrowChar > 90)) {
        return;
    }
    if ((acceptBits & 0x80) && (narrowChar < 48 || narrowChar > 57) && narrowChar != 45 && narrowChar != 42 && narrowChar != 35) {
        return;
    }
    if ((acceptBits & 2) && narrowChar == 37) {
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
            if (narrowChar < 32 || narrowChar > 126) {
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
                isLead = (cur[0] >= 128 && cur[0] < 160) || (cur[0] >= 224 && cur[0] < 256);
                if (isLead) {
                    if (cur[1] >= 96 && cur[1] < 122) {
                        dropChar = 1;
                    } else if (cur[1] >= 129 && cur[1] < 155) {
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
                        cur[76] = 0;
                        cur = (u8 *)gKbd + i;
                        strcat((char *)cur + 76, (char *)cur + charSize + 76);
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
                    gKbd->state = 22;
                }
            } else {
                gKbd->state = SE_SYS_REFUSE;
            }
        } else {
            Kbd_Convert();
        }
    } else {
        gKbd->state = 22;
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
        gKbd->state = 20;
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
    gKbd->state = 22;
}

/*
 * The dakuten key (USB_KEY_LBRACKET, the '@' key of a Japanese keyboard). Does
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

/*
 * The handakuten key (USB_KEY_RBRACKET, the '[' key of a Japanese keyboard).
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
    gKbd->state = 21;
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
        gKbd->state = 21;
        return;
    }
    fkeyIdx = key - 58;
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
    gKbd->state = 21;
}

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
 * when the kind has phrases (kindConf + 48) and the first phrase page
 * is not barred. Otherwise: F1, F2 the hiragana and katakana modes
 * (Kbd_FKeyRow); F3 and F8 half-width katakana (Kbd_FindKind, then
 * KBD_SET_HALF_SMALL_KATA and Kbd_ToggleShift), committing the kana; F4
 * the full-width alphabet and F5 the ASCII one (F5 commits the kana), in
 * capitals while Caps Lock is on (gKbdSettings), keeping the cursor; F6
 * Kbd_Confirm; F7 Kbd_ConvertForm2; F9, F10 the phrase pages
 * (Kbd_OpenPhrases1, Kbd_OpenPhrases2); F11 the gestures page with the
 * cursor on column 5, row 0 (refused while converting, when barred, or
 * when the kind has no gestures, kindConf + 40); F12 cycles the
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
            Kbd_DoGesture(gKbd->text, &gKbd->textPos, (u8)(key - 58));
        }
    } else if (Kbd_CtrlHeld() != 0) {
        Kbd_CtrlFKey(key);
    } else if (Kbd_ShiftHeld() != 0) {
        if (*(s32 *)(gKbd->kindConf + 48) != 0
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
                gKbd->state = 22;
                gKbd->col = x;
                gKbd->row = y;
                Kbd_LookupKey();
                return;
            }
            gKbd->state = 21;
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
                gKbd->state = 22;
                gKbd->col = x;
                gKbd->row = y;
                Kbd_LookupKey();
                return;
            }
            gKbd->state = 21;
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
                gKbd->keySet = 14;
                Kbd_ToggleShift();
                gKbd->state = 22;
                return;
            }
            gKbd->state = 21;
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
                || *(s32 *)(gKbd->kindConf + 40) == 0) {
                gKbd->state = 21;
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
            gKbd->state = 22;
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
            gKbd->state = 21;
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
    if (Kbd_FindKind(key - 58, &gKbd->mode, &gKbd->keySet) == 0) {
        gKbd->state = 22;
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
        gKbd->state = 21;
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
 * not converting, the kind has phrases (kindConf + 48) and
 * Kbd_FindKind allows the page: records it (Kbd_SetPhrasePage), commits
 * the kana, clears gestureMode, updates the mode, the page cursor, the
 * layout and the symbol mode; SE_SYS_SELECT. When the page is barred,
 * clears gestureMode and cyclePos. Otherwise the sound is SE_SYS_REFUSE.
 */
void Kbd_OpenPhrases1(void)
{
    s32 state = 21;

    if (gKbd->converting == 0 && *(s32 *)(gKbd->kindConf + 48) != 0) {
        if (Kbd_FindKind(4, &gKbd->mode, &gKbd->keySet) == 0) {
            Kbd_SetPhrasePage(4);
            Kbd_CommitKana();
            gKbd->gestureMode = 0;
            Kbd_UpdateMode();
            Kbd_HomePageCursor();
            Kbd_RefreshLayout();
            Kbd_UpdateSymbolMode();
            state = 22;
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
    s32 state = 21;

    if (gKbd->converting == 0 && *(s32 *)(gKbd->kindConf + 48) != 0) {
        if (Kbd_FindKind(5, &gKbd->mode, &gKbd->keySet) == 0) {
            Kbd_SetPhrasePage(5);
            Kbd_CommitKana();
            gKbd->gestureMode = 0;
            Kbd_UpdateMode();
            Kbd_HomePageCursor();
            Kbd_RefreshLayout();
            Kbd_UpdateSymbolMode();
            state = 22;
        } else {
            gKbd->gestureMode = 0;
            gKbd->cyclePos = 0;
        }
    }
    gKbd->state = state;
}

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
    s32 next = 21;
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
        next = 22;
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
        next = 22;
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
    s32 state = 21;
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
    state = 22;
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
    s32 state = 21;
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
        state = 22;
    }
end:
    gKbd->state = state;
}

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
                state = 22;
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
    gKbd->state = 22;
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
                state = 22;
            }
        } else {
            pos = kbd->textPos;
            if (pos < (u16)strlen(kbd->text)) {
                gKbd->textPos = strlen(kbd->text);
                state = 22;
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
                gKbd->state = 23;
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
            gKbd->state = 23;
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
        gKbd->state = 23;
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
        gKbd->state = 23;
    } else {
        gKbd->state = -1;
    }
}

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
 * The USB keyboard's katakana/hiragana key (USB_KEY_KANA). With Alt held,
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
 * The henkan (convert) key (USB_KEY_HENKAN). For entry kinds 8-10 outside the
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
 * The muhenkan key (USB_KEY_MUHENKAN). With a full-width key set (Kbd_SetIsWide)
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
 * The hankaku/zenkaku key (USB_KEY_GRAVE). Clears KBD_KEYF_CYCLING. Without
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
        lead = (((u8 *)src)[0] >= 128 && ((u8 *)src)[0] < 160) || (((u8 *)src)[0] >= 224 && ((u8 *)src)[0] < 256);
        if (lead) {
            return 0;
        }
        *out = *src;
        if (*out >= 65 && *out <= 90) {
            *out += 32;
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
            for (j = 126; gKbd->kanaPos < j; j--) {
                /* kana[j] = kana[j - 1] (kana is at 0x168): the game's
                 * code adds j to gKbd first. */
                kbdBytes = (u8 *)gKbd + j;
                kbdBytes[360] = kbdBytes[359];
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
 * when it is among the 264 characters of gKbdKind8Chars. Else 0. The
 * table search is written as a goto loop.
 */
s32 Kbd_CharAllowed(s8 *str)
{
    u32 i;
    s8 *entry;
    u8 firstChar;

    if (*(u8 *)((u8 *)str + 1) == 32) {
        firstChar = (u8) *(s8 *)((u8 *)str + 0);
        if (firstChar == 44) {
            return 0;
        }
        if (((s32) firstChar >= 32) && ((s32) firstChar < 127)) {
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
    if (i >= 264U) {
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
    char buf[256];
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
        memset(buf, 0, 256);
        strncpy(buf, gKbd->convDone, 255);
        strncat(buf, gKbd->cand, 255);
        strncat(buf, gKbd->convRest, 255);
        Str_ToWide((s8 *)buf, (s8 *)gKbd->wide);
    }
    added = Str_InsertClip(gKbd->text, (char *)gKbd->wide, gKbd->textPos,
                      gKbd->maxLen);
    memset(gKbd->kana, 0, 255);
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
    char tail[256];
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
            if ((code >= 128 && code <= 159) || (code >= 224 && code <= 255)) {
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
    strncpy(tail, dst + pos, 255);
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
    if (((s32) kind >= 8) && ((s32) kind < 11)) {
        return 1;
    }
    return 0;
}

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
void Font_SetPos(s32 x, s32 y); /* unit's own declaration: the definition has (s16, s16);
 * Kbd_DrawLine matches with s32, and is 457 words off with s16 */
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
    char buf[48];
    char conv[272];
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
    Font_SetSpacing(20, 20);
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
        Font_SetSpacing(4, 22);
        Font_SetPos(x + charW * (gKbd->textPos + gKbd->kanaPos - gKbd->firstShown), y);
        Kbd_Printf(TXT(0x0046C988) /* a full-width black square (a bar's cell) */);
    }
}
