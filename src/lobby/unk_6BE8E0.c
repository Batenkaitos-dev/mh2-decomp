#include "common.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "scene.h"
#include "lobby_menu.h"

extern u8 gMenuWork[];
void GameMenu_Back(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
/* The place information menu (gMenuWork: PlaceInfo_Open, unk_6BE630.c). */
typedef struct PlaceInfoMenu {
    WinSlide win;
    s8 step;                    /* PlaceInfo_Input */
} PlaceInfoMenu;

s32 PlaceInfo_Input(s32 arg)
{
    PlaceInfoMenu *m = (PlaceInfoMenu *)gMenuWork;
    u16 pad = arg;

    if (pad & 0x8000) {
        GameMenu_Back();
        return 0;
    }
    switch (m->step) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x136)) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 1:
        if (pad & 0x40) {
            return 0x40;
        }
        break;
    }
    return arg;
}

extern char *gAreaNameTexts[];
extern void *D_0080C25C;
u8 Area_GetSetIndex(u16 id);
void *LobbyArea_Name(u16 id)
{
    u8 k = Area_GetSetIndex(id);

    if (id == 0) {
        return D_0080C25C;
    }
    return gAreaNameTexts[k];
}

s32 LobbyWorld_GetSeason(void);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), PlaceInfo_DrawPlace 53 of 134 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
extern char D_00825F28[];
extern char *gTownInfoTexts[];
extern char *D_0080D034;
extern char *gSeasonNames[];
extern u32 gSeasonColors[];
/* NON_MATCHING: 42 words off: the original keeps ext(y) in s3 from the entry and adds 1
 * through a copy (move v0, s3); the x copies take other s-registers. */
s16 PlaceInfo_DrawPlace(s16 x, s16 y, s32 h)
{
    s32 k;
    s16 cy;
    s32 k4;
    s32 sx;
    s32 tx;

    k = LobbyWorld_GetSeason();
    cy = y;
    Area_GetSetIndex(gScene.area);
    Win_Draw(x, y, 0x12C, h, 0xBDB59F6C, 0xC0202020, 7);
    Gfx_DrawLine(x, y, 0x12C, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x, cy + 1);
    Text_PrintfColored(gInfoMenuLabels[0]);
    Font_SetPos(x, cy += 0x16);
    Text_PrintfColored(D_00825F28, gTownInfoTexts[0]);
    Font_SetColor(0x807F7F7F);
    sx = x;
    tx = sx + 0x6E;
    Font_SetPos(tx, cy);
    k4 = k * 4;
    Text_PrintfColored(D_00825F28, *(char **)((u8 *)gSeasonNames + k4));
    Icon_Draw(sx + 0x56, cy - 1, 0x14, 0x14, *(u32 *)((u8 *)gSeasonColors + k4), k + 0x2D);
    Font_SetPos(x, cy += 0x14);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(D_00825F28, D_0080D034);
    Font_SetPos(tx, cy);
    Font_SetColor(0x807F7F7F);
    Text_PrintfColored(D_00825F28, LobbyArea_Name(gScene.area));
    return cy + 0x14;
}

void Text_Printf(const char *fmt, ...);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
s8 *LobbyLoc_Text(void);
char *LobbySrv_MyName(void);
s32 LobbySrv_CurIndex(void);
s32 LobbySrv_ValueAt(s32 i);
u32 LevelList_SlotColor(u8 slot);
s32 LobbyLevel0_HasGroups(void);
extern char D_00825F30[];
extern char D_00825F38[];
extern char D_00825F40[];
extern char *D_0080D038;
extern char *gAreaPopTexts[];
extern char *gRecruitMsgLabelTexts[];
extern char *gLimitCondTexts[];
extern char *D_0080C4EC;
extern char *gNoValueDashTexts[];
extern char *gGroupLimitTexts[];
extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
extern u8 gLobbyLevel0Entries[];
extern u8 D_5C8086[];
extern LevelEntry gLobbyLevel1Entries[];
extern u8 D_5C41E7[];
extern u8 D_5C41E8[];
extern u8 D_5C8072[];
extern u8 D_5C8074[];
typedef struct {
    /* 0x000 */ u8 unk0[0xBBC];
    /* 0xBBC */ char names[64][24];
} RoomBBC;
void PlaceInfo_DrawOnline(s16 x, s16 y)
{
    u32 col;
    s16 cy;
    s32 sx;
    char *name;
    s32 ix;

    cy = PlaceInfo_DrawPlace(x, y, 0xE2);
    Font_SetPos(x, cy);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(D_00825F28, D_0080D038);
    sx = x;
    Font_SetPos(sx + 0x6E, cy);
    Font_SetColor(0x807F7F7F);
    Text_PrintfColored(D_00825F28, LobbyLoc_Text());
    Font_SetPos(sx + 0x6E, cy += 0x14);
    Text_PrintfColored(D_00825F28, LobbySrv_MyName());
    ix = (s16)(sx + 0x6E) - 0xE;
    LobbyUi_DrawIcon(ix, cy + 9, 0x14, LobbySrv_ValueAt(LobbySrv_CurIndex()) | 0xFF000000, 1);
    Font_SetPos(sx + 0x6E, cy += 0x14);
    if (gLobbyLevel0Sel != 0) {
        Text_PrintfColored(D_00825F28, gLobbyLevel0Entries + (gLobbyLevel0Sel - 1) * 0x164 + 0x1F);
        col = LevelList_SlotColor(D_5C8086[gLobbyLevel0Sel * 0x164]);
        col |= 0xFF000000;
        LobbyUi_DrawIcon(ix, cy + 9, 0x14, col, 2);
    }
    Font_SetPos(sx + 0x6E, cy += 0x14);
    if (gLobbyLevel1Sel != 0) {
        Text_PrintfColored(D_00825F28, gLobbyLevel1Entries[gLobbyLevel1Sel - 1].name);
        LobbyUi_DrawIcon(ix, cy + 9, 0x14, col, 3);
    }
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, gRecruitMsgLabelTexts[0]);
    if (gLobbyLevel1Sel != 0) {
        Font_SetPos(sx + 0x6E, cy);
        Text_Printf(D_00825F38, gLobbyLevel1Entries[gLobbyLevel1Sel - 1].text);
    } else {
        Font_SetPos(sx + 0x6E, cy);
        Text_Printf(D_00825F38, gNoValueDashTexts[0]);
    }
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, gLimitCondTexts[0]);
    Font_SetPos(sx + 0x6E, cy);
    if (LobbyLevel0_HasGroups() == 1 && gLobbyLevel1Sel != 0) {
        Text_Printf(D_00825F38, gGroupLimitTexts[D_5C41E7[gLobbyLevel1Sel * 0x164]]);
    } else {
        Text_Printf(D_00825F38, gNoValueDashTexts[0]);
    }
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, D_0080C4EC);
    name = ((RoomBBC *)((s32)(*(s32 *)&gLobbyWorkBuf + 0x6EF4) + 0x20))->names[D_5C41E8[gLobbyLevel1Sel * 0x164]];
    Font_SetPos(sx + 0x6E, cy);
    if (LobbyLevel0_HasGroups() == 1 && gLobbyLevel1Sel != 0) {
        Text_Printf(D_00825F38, name);
    } else {
        Text_Printf(D_00825F38, gNoValueDashTexts[0]);
    }
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, gAreaPopTexts[0]);
    Font_SetPos(sx + 0x6E, cy);
    Text_Printf(D_00825F40, *(u16 *)(D_5C8072 + gLobbyLevel0Sel * 0x164), *(u16 *)(D_5C8074 + gLobbyLevel0Sel * 0x164));
}

s16 PlaceInfo_DrawPlace(s16 x, s16 y, s32 h);
u8 Village_FacilityLevel(s32 kind);
extern char *D_0080D04C;
extern char *gTavernPortTexts[];
extern char *D_0080D054;
extern char *gSmithyMineTexts[];
extern char *D_0080D05C;
extern char *gTavernLevelTexts[];
extern char *gPortLevelTexts[];
extern char *gSmithyLevelTexts[];
extern char *gMineLevelTexts[];
void PlaceInfo_DrawVillage(s16 x, s16 y)
{
    s16 cy;
    s32 sx;

    cy = PlaceInfo_DrawPlace(x, y, 0xAA);
    Font_SetPos(x, cy);
    Text_Printf(D_00825F30, D_0080D04C);
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, gTavernPortTexts[0]);
    sx = x;
    Font_SetPos(sx + 0x3C, cy);
    Text_Printf(D_00825F38, gTavernLevelTexts[Village_FacilityLevel(1)]);
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, D_0080D054);
    Font_SetPos(sx + 0x3C, cy);
    Text_Printf(D_00825F38, gPortLevelTexts[Village_FacilityLevel(3)]);
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, gSmithyMineTexts[0]);
    Font_SetPos(sx + 0x3C, cy);
    Text_Printf(D_00825F38, gSmithyLevelTexts[Village_FacilityLevel(2)]);
    Font_SetPos(x, cy += 0x14);
    Text_Printf(D_00825F30, D_0080D05C);
    Font_SetPos(sx + 0x3C, cy);
    Text_Printf(D_00825F38, gMineLevelTexts[Village_FacilityLevel(0) / 3]);
}

/* unk_6BE8E0's .rodata (0x825F28-0x825F50) */
#pragma rodata on
char D_00825F28[0x8] = {
#include "lobby/00825F28.inc"
};
char D_00825F30[0x8] = {
#include "lobby/00825F30.inc"
};
char D_00825F38[0x8] = {
#include "lobby/00825F38.inc"
};
char D_00825F40[0x10] = {
#include "lobby/00825F40.inc"
};
#pragma rodata off
