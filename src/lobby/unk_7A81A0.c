/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_labels.h"

void Font_SetPos(s16, s16);
void Font_SetColor(s32 color);
void Text_Printf(const char *fmt, ...);
void Text_SetSize(s32);
s32 strlen(const char *s);
s32 Str_Length(char *s);
extern s32 gSpotTakenMask;
extern s32 D_008BEB28;
extern s32 gTownMode;

void Lobby_DrawPressButton(void) {
    if (gFrameTick & 0x20) {
        Text_SetSize(5);
        Font_SetPos(0xC8, 0x17C);
        Font_SetColor(0x807F7F28);
        Text_Printf(gMiscTexts[MISC_TEXT_PRESS_BUTTON]);
    }
}

s32 Lobby_Strlen(char *s) {
    return strlen(s);
}

s32 Lobby_StrLength(char *s) {
    return Str_Length(s);
}

void Str_FromInt(s32 n, char *dst);
char *strcat(char *, const char *);
char *strcpy(char *, const char *);
char *strrchr(const char *, s32);
s32 sprintf(char *, const char *, ...);
extern char gTextFillBuf[];
extern char gTextFill2Buf[];
extern char gTextFillFmtBuf[];
extern char D_008428A0[];

char *Text_FillDollar(char *fmt, s32 n) {
    char *p;

    strcpy(gTextFillBuf, fmt);
    p = strrchr(gTextFillBuf, 0x24);
    if (p == 0) {
        return fmt;
    }
    Str_FromInt(n, p);
    strcat(gTextFillBuf, strrchr(fmt, 0x24) + 1);
    return gTextFillBuf;
}

char *Text_FillDollar2(s32 n) {
    char *p;

    strcpy(gTextFill2Buf, gTextFillBuf);
    p = strrchr(gTextFill2Buf, 0x24);
    if (p == 0) {
        return gTextFill2Buf;
    }
    Str_FromInt(n, p);
    strcat(gTextFill2Buf, strrchr(gTextFillBuf, 0x24) + 1);
    return gTextFill2Buf;
}

char *Text_FillDollarFmt(char *fmt, char *name) {
    char *p;

    strcpy(gTextFillFmtBuf, fmt);
    p = strrchr(gTextFillFmtBuf, 0x24);
    if (p == 0) {
        return fmt;
    }
    sprintf(p, D_008428A0, name);
    strcat(gTextFillFmtBuf, strrchr(fmt, 0x24) + 1);
    return gTextFillFmtBuf;
}

s32 Area_IsTownHall(s32 area)
{
    switch (area & 0xFFFF) {
    case 0x86:
    case 0x87:
    case 0x88:
        return 1;
    }
    return 0;
}

s32 Area_IsHouse(u16 area)
{
    switch (area & 0xFFFF) {
    case 0x56:
    case 0xAD:
    case 0xAE:
    case 0xAF:
        return 1;
    }
    return 0;
}

s32 Lobby_HrAtLeast31(void) {
    return (s32) gSave.hr >= 0x1F;
}

void ActSpot_ClearAll(void) {
    D_008BEB28 = 0;
    gSpotTakenMask = 0;
}

extern s32 gSpotTakenMask;
extern s32 D_008BEB28;
void ActSpot_Release(s32 slot) {
    s32 bit;

    bit = slot & 0xFF;
    if (bit < 0x1F) {
        gSpotTakenMask &= ~(1 << bit);
        return;
    }
    D_008BEB28 &= ~(1 << (bit % 32));
}

extern s32 gSpotTakenMask;
extern s32 D_008BEB28;
void ActSpot_Take(u8 slot) {
    s32 bit;

    bit = slot & 0xFF;
    if (bit < 0x1F) {
        gSpotTakenMask |= 1 << bit;
        return;
    }
    D_008BEB28 |= 1 << (bit % 32);
}

s32 ActSpot_IsFree(u32 n)
{
    u32 *w = &((u32 *)&gSpotTakenMask)[n >= 31];

    switch (*w & (1 << n)) {
    default:
        return 0;
    case 0:
        return 1;
    }
}

s32 MyHouse_InOwnHouse(void);
s32 Game_IsOnline(void);
s8 MyHouse_LocalSlot(void);
/* unit's own declaration: with the definition's u8 (s32 k) (sub_main/equip_save.c), MyHouse_Level: 1 of 33 words differ */
s32 Village_FacilityLevel(s32);
typedef struct PlayerRec11C {
    u8 pad0[0xF1];
    u8 unkF1;
    u8 padF2[0x11C - 0xF2];
} PlayerRec11C;
extern u8 gNetUsers[];

u8 MyHouse_Level(void)
{
    if (MyHouse_InOwnHouse() == 1 || Game_IsOnline() == 0) {
        return Village_FacilityLevel(5);
    }
    return ((PlayerRec11C *)gNetUsers)[MyHouse_LocalSlot()].unkF1;
}

s32 Town_InMode46(void) {
    return gTownMode == 0x46;
}
