/*
 * The village board (TOWN_MODE_VILLAGE_BOARD): a notice board of articles
 * in five categories. The player picks a category, pages through the
 * titles of the articles open to him (each has a story condition, a
 * Cond_Check id, and an optional event flag) and reads one; read articles
 * are marked in the save (gSave.boardRead, one bit per article, numbered
 * across the categories), and unread titles carry a mark.
 *
 * The board's work is a VillageBoardMenu in gMenuWork (lobby_menu.h). The
 * articles are gVillageBoardArticles' five lists (VillageBoardArticle),
 * gVillageBoardCounts the counts; their titles and texts are this file's
 * read-only data, named gVillageBoardTitle<category>_<index> and
 * gVillageBoardText<category>_<index>.
 */
#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "quest.h"
#include "menu_task.h"
#include "pad.h"

s32 Cond_Check(u16);
void CamReq_Cancel(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
s32 Save_TestEventFlag(s32 flag);
void LobbyMenu_Init(void);
extern s8 gMenuInputLock;
extern s8 gLobbyLevel1Sel;
extern s16 gVillageBoardCounts[];
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32, s32);
void VillageBoard_BuildPage(VillageBoardMenu *board, s16 page);
extern VillageBoardArticle *gVillageBoardArticles[];
s32 Menu_CountPages(s16 n, s16 per);
extern char gVillageBoardTextFmt[];
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
void CamReq_VillageBoard(void);
void VillageBoard_Draw(MenuTask *task);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
extern u8 gMenuWork;
void *memset(void *dst, s32 c, u32 n);

/* The articles' titles and texts. Some titles lie in the last bytes of
 * the text before them (gVillageBoardTitle0_9 and five more): an object of their
 * size on its own would be aligned to 16 bytes and move. */
#pragma rodata on
char gVillageBoardTitle0_0[0x10] = {
#include "lobby/0082EA90.inc"
};
char gVillageBoardText0_0[0x1D0] = {
#include "lobby/0082EAA0.inc"
};
char gVillageBoardTitle0_1[0x20] = {
#include "lobby/0082EC70.inc"
};
char gVillageBoardText0_1[0x178] = {
#include "lobby/0082EC90.inc"
};
char gVillageBoardTitle0_2[13] = {
#include "lobby/0082EE08.inc"
};
char gVillageBoardText0_2[0x188] = {
#include "lobby/0082EE20.inc"
};
char gVillageBoardTitle0_3[13] = {
#include "lobby/0082EFA8.inc"
};
char gVillageBoardText0_3[0x178] = {
#include "lobby/0082EFC0.inc"
};
char gVillageBoardTitle0_4[0xC] = {
#include "lobby/0082F138.inc"
};
char gVillageBoardText0_4[0x1B8] = {
#include "lobby/0082F150.inc"
};
char gVillageBoardTitle0_5[13] = {
#include "lobby/0082F308.inc"
};
char gVillageBoardText0_5[0x160] = {
#include "lobby/0082F320.inc"
};
char gVillageBoardTitle0_6[0x20] = {
#include "lobby/0082F480.inc"
};
char gVillageBoardText0_6[0x150] = {
#include "lobby/0082F4A0.inc"
};
char gVillageBoardTitle0_7[0x20] = {
#include "lobby/0082F5F0.inc"
};
char gVillageBoardText0_7[0x178] = {
#include "lobby/0082F610.inc"
};
char gVillageBoardTitle0_8[13] = {
#include "lobby/0082F788.inc"
};
char gVillageBoardText0_8[0x1A0] = {
#include "lobby/0082F7A0.inc"
};
char gVillageBoardText0_9[0x180] = {
#include "lobby/0082F940.inc"
};
char gVillageBoardTitle0_10[0x20] = {
#include "lobby/0082FAC0.inc"
};
char gVillageBoardText0_10[0x160] = {
#include "lobby/0082FAE0.inc"
};
char gVillageBoardTitle0_11[0x20] = {
#include "lobby/0082FC40.inc"
};
char gVillageBoardText0_11[0x130] = {
#include "lobby/0082FC60.inc"
};
char gVillageBoardTitle0_12[0x10] = {
#include "lobby/0082FD90.inc"
};
char gVillageBoardText0_12[0x150] = {
#include "lobby/0082FDA0.inc"
};
char gVillageBoardText0_13[0x188] = {
#include "lobby/0082FEF0.inc"
};
char gVillageBoardTitle0_14[8] = {
#include "lobby/00830078.inc"
};
u32 gVillageBoardTitle0_14Tail[4] = {
#include "lobby/00830080.inc"
};
char gVillageBoardText0_14[0x140] = {
#include "lobby/00830090.inc"
};
char gVillageBoardTitle0_15[0x20] = {
#include "lobby/008301D0.inc"
};
char gVillageBoardText0_15[0x130] = {
#include "lobby/008301F0.inc"
};
char gVillageBoardTitle0_16[0x20] = {
#include "lobby/00830320.inc"
};
char gVillageBoardText0_16[0x100] = {
#include "lobby/00830340.inc"
};
char gVillageBoardTitle0_17[0x20] = {
#include "lobby/00830440.inc"
};
char gVillageBoardText0_17[0x140] = {
#include "lobby/00830460.inc"
};
char gVillageBoardTitle0_18[0x10] = {
#include "lobby/008305A0.inc"
};
char gVillageBoardText0_18[0x180] = {
#include "lobby/008305B0.inc"
};
char gVillageBoardTitle4_0[0x20] = {
#include "lobby/00830730.inc"
};
char gVillageBoardText4_0[0x160] = {
#include "lobby/00830750.inc"
};
char gVillageBoardTitle4_1[0x10] = {
#include "lobby/008308B0.inc"
};
char gVillageBoardText4_1[0x170] = {
#include "lobby/008308C0.inc"
};
char gVillageBoardTitle4_2[0x20] = {
#include "lobby/00830A30.inc"
};
char gVillageBoardText4_2[0x140] = {
#include "lobby/00830A50.inc"
};
char gVillageBoardTitle4_3[0x20] = {
#include "lobby/00830B90.inc"
};
char gVillageBoardText4_3[0x140] = {
#include "lobby/00830BB0.inc"
};
char gVillageBoardTitle4_4[0x20] = {
#include "lobby/00830CF0.inc"
};
char gVillageBoardText4_4[0x190] = {
#include "lobby/00830D10.inc"
};
char gVillageBoardTitle4_5[0x20] = {
#include "lobby/00830EA0.inc"
};
char gVillageBoardText4_5[0x180] = {
#include "lobby/00830EC0.inc"
};
char gVillageBoardTitle4_6[0x20] = {
#include "lobby/00831040.inc"
};
char gVillageBoardText4_6[0xD0] = {
#include "lobby/00831060.inc"
};
char gVillageBoardTitle4_7[0x20] = {
#include "lobby/00831130.inc"
};
char gVillageBoardText4_7[0x170] = {
#include "lobby/00831150.inc"
};
char gVillageBoardTitle4_8[0x20] = {
#include "lobby/008312C0.inc"
};
char gVillageBoardText4_8[0x170] = {
#include "lobby/008312E0.inc"
};
char gVillageBoardTitle4_9[0x20] = {
#include "lobby/00831450.inc"
};
char gVillageBoardText4_9[0x128] = {
#include "lobby/00831470.inc"
};
char gVillageBoardTitle4_10[13] = {
#include "lobby/00831598.inc"
};
char gVillageBoardText4_10[0xD0] = {
#include "lobby/008315B0.inc"
};
char gVillageBoardTitle3_0[0x10] = {
#include "lobby/00831680.inc"
};
char gVillageBoardText3_0[0x190] = {
#include "lobby/00831690.inc"
};
char gVillageBoardTitle3_1[0x20] = {
#include "lobby/00831820.inc"
};
char gVillageBoardText3_1[0x1B0] = {
#include "lobby/00831840.inc"
};
char gVillageBoardText3_2[0x190] = {
#include "lobby/008319F0.inc"
};
char gVillageBoardTitle3_3[0x10] = {
#include "lobby/00831B80.inc"
};
char gVillageBoardText3_3[0x1A0] = {
#include "lobby/00831B90.inc"
};
char gVillageBoardText3_4[0x190] = {
#include "lobby/00831D30.inc"
};
char gVillageBoardTitle3_5[0x20] = {
#include "lobby/00831EC0.inc"
};
char gVillageBoardText3_5[0x150] = {
#include "lobby/00831EE0.inc"
};
char gVillageBoardTitle3_6[0x20] = {
#include "lobby/00832030.inc"
};
char gVillageBoardText3_6[0x188] = {
#include "lobby/00832050.inc"
};
char gVillageBoardTitle3_7[13] = {
#include "lobby/008321D8.inc"
};
char gVillageBoardText3_7[0x180] = {
#include "lobby/008321F0.inc"
};
char gVillageBoardTitle3_8[0x10] = {
#include "lobby/00832370.inc"
};
char gVillageBoardText3_8[0x1A0] = {
#include "lobby/00832380.inc"
};
char gVillageBoardTitle3_9[0x10] = {
#include "lobby/00832520.inc"
};
char gVillageBoardText3_9[0x180] = {
#include "lobby/00832530.inc"
};
char gVillageBoardTitle3_10[0x20] = {
#include "lobby/008326B0.inc"
};
char gVillageBoardText3_10[0x1B0] = {
#include "lobby/008326D0.inc"
};
char gVillageBoardTitle1_0[0x20] = {
#include "lobby/00832880.inc"
};
char gVillageBoardText1_0[0x190] = {
#include "lobby/008328A0.inc"
};
char gVillageBoardTitle1_1[0x20] = {
#include "lobby/00832A30.inc"
};
char gVillageBoardText1_1[0x1A0] = {
#include "lobby/00832A50.inc"
};
char gVillageBoardTitle1_2[0x20] = {
#include "lobby/00832BF0.inc"
};
char gVillageBoardText1_2[0x180] = {
#include "lobby/00832C10.inc"
};
char gVillageBoardTitle1_3[0x20] = {
#include "lobby/00832D90.inc"
};
char gVillageBoardText1_3[0x1A0] = {
#include "lobby/00832DB0.inc"
};
char gVillageBoardTitle1_4[0x10] = {
#include "lobby/00832F50.inc"
};
char gVillageBoardText1_4[0x160] = {
#include "lobby/00832F60.inc"
};
char gVillageBoardTitle1_5[0x20] = {
#include "lobby/008330C0.inc"
};
char gVillageBoardText1_5[0x1B0] = {
#include "lobby/008330E0.inc"
};
char gVillageBoardTitle1_6[0x20] = {
#include "lobby/00833290.inc"
};
char gVillageBoardText1_6[0x1B0] = {
#include "lobby/008332B0.inc"
};
char gVillageBoardTitle1_7[0x20] = {
#include "lobby/00833460.inc"
};
char gVillageBoardText1_7[0x1A0] = {
#include "lobby/00833480.inc"
};
char gVillageBoardTitle1_8[0x20] = {
#include "lobby/00833620.inc"
};
char gVillageBoardText1_8[0x190] = {
#include "lobby/00833640.inc"
};
char gVillageBoardTitle1_9[0x10] = {
#include "lobby/008337D0.inc"
};
char gVillageBoardText1_9[0x1C0] = {
#include "lobby/008337E0.inc"
};
char gVillageBoardTitle1_10[0x20] = {
#include "lobby/008339A0.inc"
};
char gVillageBoardText1_10[0x1A8] = {
#include "lobby/008339C0.inc"
};
char gVillageBoardTitle1_11[0xC] = {
#include "lobby/00833B68.inc"
};
char gVillageBoardText1_11[0x150] = {
#include "lobby/00833B80.inc"
};
char gVillageBoardTitle1_12[0x20] = {
#include "lobby/00833CD0.inc"
};
char gVillageBoardText1_12[0x1B0] = {
#include "lobby/00833CF0.inc"
};
char gVillageBoardText1_13[0x1D0] = {
#include "lobby/00833EA0.inc"
};
char gVillageBoardTitle1_14[0x20] = {
#include "lobby/00834070.inc"
};
char gVillageBoardText1_14[0x160] = {
#include "lobby/00834090.inc"
};
char gVillageBoardTitle1_15[0x20] = {
#include "lobby/008341F0.inc"
};
char gVillageBoardText1_15[0x1B0] = {
#include "lobby/00834210.inc"
};
char gVillageBoardTitle1_16[0x20] = {
#include "lobby/008343C0.inc"
};
char gVillageBoardText1_16[0x1B0] = {
#include "lobby/008343E0.inc"
};
char gVillageBoardTitle1_17[0x20] = {
#include "lobby/00834590.inc"
};
char gVillageBoardText1_17[0x170] = {
#include "lobby/008345B0.inc"
};
char gVillageBoardTitle1_18[0x20] = {
#include "lobby/00834720.inc"
};
char gVillageBoardText1_18[0x1B0] = {
#include "lobby/00834740.inc"
};
char gVillageBoardTitle1_19[0x20] = {
#include "lobby/008348F0.inc"
};
char gVillageBoardText1_19[0x1A0] = {
#include "lobby/00834910.inc"
};
char gVillageBoardTitle1_20[0x20] = {
#include "lobby/00834AB0.inc"
};
char gVillageBoardText1_20[0x1A0] = {
#include "lobby/00834AD0.inc"
};
char gVillageBoardTitle1_21[0x10] = {
#include "lobby/00834C70.inc"
};
char gVillageBoardText1_21[0x160] = {
#include "lobby/00834C80.inc"
};
char gVillageBoardTitle1_22[0x20] = {
#include "lobby/00834DE0.inc"
};
char gVillageBoardText1_22[0x180] = {
#include "lobby/00834E00.inc"
};
char gVillageBoardTitle1_23[0x20] = {
#include "lobby/00834F80.inc"
};
char gVillageBoardText1_23[0x1A0] = {
#include "lobby/00834FA0.inc"
};
char gVillageBoardTitle1_24[0x20] = {
#include "lobby/00835140.inc"
};
char gVillageBoardText1_24[0x1B0] = {
#include "lobby/00835160.inc"
};
char gVillageBoardTitle1_25[0x20] = {
#include "lobby/00835310.inc"
};
char gVillageBoardText1_25[0x1B8] = {
#include "lobby/00835330.inc"
};
char gVillageBoardTitle1_26[0xC] = {
#include "lobby/008354E8.inc"
};
char gVillageBoardText1_26[0x180] = {
#include "lobby/00835500.inc"
};
char gVillageBoardTitle2_0[0x20] = {
#include "lobby/00835680.inc"
};
char gVillageBoardText2_0[0x1A0] = {
#include "lobby/008356A0.inc"
};
char gVillageBoardTitle2_1[0x20] = {
#include "lobby/00835840.inc"
};
char gVillageBoardText2_1[0x1D0] = {
#include "lobby/00835860.inc"
};
char gVillageBoardTitle2_2[0x20] = {
#include "lobby/00835A30.inc"
};
char gVillageBoardText2_2[0x1B0] = {
#include "lobby/00835A50.inc"
};
char gVillageBoardTitle2_3[0x20] = {
#include "lobby/00835C00.inc"
};
char gVillageBoardText2_3[0x1D0] = {
#include "lobby/00835C20.inc"
};
char gVillageBoardTitle2_4[0x20] = {
#include "lobby/00835DF0.inc"
};
char gVillageBoardText2_4[0x1B0] = {
#include "lobby/00835E10.inc"
};
char gVillageBoardTitle2_5[0x20] = {
#include "lobby/00835FC0.inc"
};
char gVillageBoardText2_5[0x190] = {
#include "lobby/00835FE0.inc"
};
char gVillageBoardTitle2_6[0x20] = {
#include "lobby/00836170.inc"
};
char gVillageBoardText2_6[0x1B0] = {
#include "lobby/00836190.inc"
};
char gVillageBoardTitle2_7[0x20] = {
#include "lobby/00836340.inc"
};
char gVillageBoardText2_7[0x190] = {
#include "lobby/00836360.inc"
};
char gVillageBoardTitle2_8[0x20] = {
#include "lobby/008364F0.inc"
};
char gVillageBoardText2_8[0x1A0] = {
#include "lobby/00836510.inc"
};
char gVillageBoardTitle2_9[0x20] = {
#include "lobby/008366B0.inc"
};
char gVillageBoardText2_9[0x190] = {
#include "lobby/008366D0.inc"
};
char gVillageBoardTitle2_10[0x20] = {
#include "lobby/00836860.inc"
};
char gVillageBoardText2_10[0x1C0] = {
#include "lobby/00836880.inc"
};
char gVillageBoardTitle2_11[0x10] = {
#include "lobby/00836A40.inc"
};
char gVillageBoardText2_11[0x190] = {
#include "lobby/00836A50.inc"
};
char gVillageBoardTitle2_12[0x20] = {
#include "lobby/00836BE0.inc"
};
char gVillageBoardText2_12[0x190] = {
#include "lobby/00836C00.inc"
};
char gVillageBoardTitle2_13[0x20] = {
#include "lobby/00836D90.inc"
};
char gVillageBoardText2_13[0x178] = {
#include "lobby/00836DB0.inc"
};
char gVillageBoardTitle2_14[13] = {
#include "lobby/00836F28.inc"
};
char gVillageBoardText2_14[0x180] = {
#include "lobby/00836F40.inc"
};
char gVillageBoardText2_15[0x180] = {
#include "lobby/008370C0.inc"
};
char gVillageBoardTitle2_16[0x10] = {
#include "lobby/00837240.inc"
};
char gVillageBoardText2_16[0x148] = {
#include "lobby/00837250.inc"
};
char gVillageBoardTitle2_17[0xC] = {
#include "lobby/00837398.inc"
};
char gVillageBoardText2_17[0x158] = {
#include "lobby/008373B0.inc"
};
char gVillageBoardTitle2_18[0xC] = {
#include "lobby/00837508.inc"
};
char gVillageBoardText2_18[0x180] = {
#include "lobby/00837520.inc"
};
char gVillageBoardTitle2_19[0x10] = {
#include "lobby/008376A0.inc"
};
char gVillageBoardText2_19[0x1B0] = {
#include "lobby/008376B0.inc"
};
char gVillageBoardTitle2_20[0x20] = {
#include "lobby/00837860.inc"
};
char gVillageBoardText2_20[0x170] = {
#include "lobby/00837880.inc"
};
char gVillageBoardTitle2_21[0x10] = {
#include "lobby/008379F0.inc"
};
char gVillageBoardText2_21[0x1A0] = {
#include "lobby/00837A00.inc"
};
char gVillageBoardTitle2_22[0x10] = {
#include "lobby/00837BA0.inc"
};
char gVillageBoardText2_22[0x190] = {
#include "lobby/00837BB0.inc"
};
char gVillageBoardTitle2_23[0x10] = {
#include "lobby/00837D40.inc"
};
char gVillageBoardText2_23[0x190] = {
#include "lobby/00837D50.inc"
};
char gVillageBoardTitle2_24[0x10] = {
#include "lobby/00837EE0.inc"
};
char gVillageBoardText2_24[0x170] = {
#include "lobby/00837EF0.inc"
};
#pragma rodata off

/* The game built the next two functions at optimization level 4. */
#pragma optimization_level 4
/* Marks the village board's article n read: one bit per article in
 * gSave.boardRead. */
void VillageBoard_MarkRead(s16 n)
{
    u16 bit = n % 16;
    u16 mask = 1LL << bit;
    u16 word = n / 16;

    gSave.boardRead[word] |= mask;
}

/* Whether the village board's article n is marked read: 1 yes, 0 no. */
s32 VillageBoard_IsRead(u16 n)
{
    s32 bits = gSave.boardRead[(u16)(n / 16)];

    return (bits & (1LL << (u16)(n % 16))) != 0;
}
#pragma optimization_level reset

/* Opens the village board (TOWN_MODE_VILLAGE_BOARD): its window slides in
 * from the right, the board's camera is asked for, and the town ignores the
 * pad for 6 frames. */
void VillageBoard_Open(void)
{
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_VILLAGE_BOARD;
    memset(&gMenuWork, 0, sizeof(MenuWork));
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, &VillageBoard_Draw, 0, 0);
    gMenuInputLock = 6;
    WinSlide_Step((WinSlide *)&gMenuWork, VILLAGE_BOARD_X_OUT, VILLAGE_BOARD_X_IN);
    CamReq_VillageBoard();
    LobbySe_Play(LOBBY_SE_LIST_SELECT);
}

/* Closes the village board and cancels its camera. */
void VillageBoard_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
}

extern u8 gMenuWork;
u16 LobbyPad_Get(s32);
s32 WinSlide_Step(WinSlide *p, s16 from, s16 to);
s32 VillageBoard_TopInput(VillageBoardMenu *board, u16 pad);
s32 VillageBoard_ListInput(VillageBoardMenu *board, u16 pad);
s32 VillageBoard_ArticleInput(void *board, u16 pad);
void VillageBoard_MarkRead(s16 n);

/*
 * Runs the village board each frame, by its steps (enum VillageBoardStep):
 * the window slides in; the categories; the chosen category's titles, a
 * page at a time; the chosen article's window slides in (the article is
 * marked read as it is chosen); the article, until cross; and the window
 * slides out before the board closes.
 */
void VillageBoard_Update(void)
{
    VillageBoardMenu *board = (VillageBoardMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    switch (board->step) {
    case VILLAGE_BOARD_SLIDE_IN:
        switch (WinSlide_Step(&board->boardWin, VILLAGE_BOARD_X_OUT, VILLAGE_BOARD_X_IN)) {
        case MENU_DONE:
            board->step++;
        }
        break;
    case VILLAGE_BOARD_CATEGORIES:
        switch (VillageBoard_TopInput(board, pad)) {
        case MENU_DONE:
            board->step++;
            board->cursor = 0;
            VillageBoard_BuildPage(board, 0);
            break;
        case MENU_CANCEL:
            board->step = VILLAGE_BOARD_SLIDE_OUT;
            WinSlide_Step(&board->boardWin, VILLAGE_BOARD_X_IN, VILLAGE_BOARD_X_OUT);
            break;
        }
        break;
    case VILLAGE_BOARD_TITLES:
        switch (VillageBoard_ListInput(board, pad)) {
        case MENU_DONE:
            board->step++;
            VillageBoard_MarkRead(board->categoryFirst + board->rows[board->cursor]);
            WinSlide_Step(&board->articleWin, VILLAGE_BOARD_ARTICLE_Y_OUT, VILLAGE_BOARD_ARTICLE_Y_IN);
            LobbySe_Play(LOBBY_SE_LIST_SELECT);
            break;
        case MENU_CANCEL:
            board->step = VILLAGE_BOARD_CATEGORIES;
            break;
        }
        break;
    case VILLAGE_BOARD_ARTICLE_IN:
        switch (WinSlide_Step(&board->articleWin, VILLAGE_BOARD_ARTICLE_Y_OUT, VILLAGE_BOARD_ARTICLE_Y_IN)) {
        case MENU_DONE:
            board->step++;
        }
        break;
    case VILLAGE_BOARD_ARTICLE:
        switch (VillageBoard_ArticleInput(board, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            board->step = VILLAGE_BOARD_TITLES;
            break;
        }
        break;
    case VILLAGE_BOARD_SLIDE_OUT:
        switch (WinSlide_Step(&board->boardWin, VILLAGE_BOARD_X_IN, VILLAGE_BOARD_X_OUT)) {
        case MENU_DONE:
            VillageBoard_Close();
        }
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/village_board", gVillageBoardTextFmt);

/* The article's input: cross goes back to the titles (MENU_CANCEL);
 * MENU_RUNNING before. The board isn't read. */
s32 VillageBoard_ArticleInput(void *board, u16 pad)
{
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return MENU_CANCEL;
    }
    return MENU_RUNNING;
}

/* The titles' input: circle opens the cursor's article (MENU_DONE), cross
 * goes back to the categories (MENU_CANCEL), left and right turn the page,
 * up and down move the cursor; MENU_RUNNING otherwise. */
s32 VillageBoard_ListInput(VillageBoardMenu *board, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return MENU_CANCEL;
    }
    if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (board->pageCount > 1) {
            board->page = Menu_WrapLeftRight(board->page, board->pageCount);
            VillageBoard_BuildPage(board, board->page);
        }
    } else {
        board->cursor = Menu_WrapUpDown(board->cursor, board->rowCount);
    }
    return MENU_RUNNING;
}

/* Whether an article's condition (VillageBoardArticle.cond, a Cond_Check
 * id) is met: 1 if so. */
s32 VillageBoard_CondCheck(s32 condId)
{
    return Cond_Check((u16)condId);
}

/* Whether an article's event flag is set; an article without a flag (0)
 * passes. */
s32 VillageBoard_TestFlag(s32 flag)
{
    if (flag != 0) {
        return Save_TestEventFlag(flag);
    }
    return 1;
}

/*
 * Builds page `page` of the chosen category's titles: of the category's
 * articles whose condition and flag are met, those on the page
 * (VILLAGE_BOARD_PAGE_ROWS a page) go into rows by their index in the
 * category. Sets the page count, and brings the cursor back onto the page.
 */
void VillageBoard_BuildPage(VillageBoardMenu *board, s16 page)
{
    VillageBoardArticle *article;
    s16 i = 0;
    s16 shownCount = 0;
    s16 pageFirst;
    s16 pageLast;
    s16 articleCount;

    board->rowCount = 0;
    pageFirst = page * VILLAGE_BOARD_PAGE_ROWS;
    pageLast = pageFirst + VILLAGE_BOARD_PAGE_ROWS - 1;
    article = gVillageBoardArticles[board->category];
    articleCount = gVillageBoardCounts[board->category];
    for (; i < articleCount; i++, article++) {
        if (VillageBoard_CondCheck(article->cond) == 1 && VillageBoard_TestFlag(article->eventFlag) == 1) {
            if (shownCount >= pageFirst && shownCount <= pageLast) {
                board->rows[board->rowCount] = i;
                board->rowCount++;
            }
            shownCount++;
        }
    }
    board->page = page;
    board->pageCount = Menu_CountPages(shownCount, VILLAGE_BOARD_PAGE_ROWS);
    if (board->cursor >= board->rowCount) {
        board->cursor = board->rowCount - 1;
    }
}

/* The categories' input: circle takes the cursor's category (MENU_DONE)
 * and counts the articles of the categories before it, the first of its
 * read marks; cross closes the board (MENU_CANCEL); up and down move the
 * cursor; MENU_RUNNING otherwise. */
s32 VillageBoard_TopInput(VillageBoardMenu *board, u16 pad)
{
    s16 i;

    if (pad & PAD_CIRCLE) {
        board->categoryFirst = 0;
        for (i = 0; i < board->category; i++) {
            board->categoryFirst += gVillageBoardCounts[i];
        }
        LobbySe_Play(LOBBY_SE_LIST_SELECT);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return MENU_CANCEL;
    }
    board->category = Menu_WrapUpDown(board->category, VILLAGE_BOARD_CATEGORY_COUNT);
    return MENU_RUNNING;
}


void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);

/* Draws the categories at (x, y): the board's title bar and panel, the
 * heading, the five categories' names with the arrows on the cursor's, and
 * the help box. */
void VillageBoard_DrawTop(VillageBoardMenu *board, s16 x, s16 y)
{
    s32 i;
    s16 rowY = y;
    s16 centerX;
    s32 left;
    s16 textX;
    char *label;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(VILLAGE_BOARD_X_IN, 0x12, 0xC8, 0xFFB0B000, gVillageBoardTitles[VILLAGE_BOARD_TITLE], 2, 1);
    Icon_Draw(x, y, 0x14E, 0x11A, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    left = x;
    Font_SetPos(left + 0x28, rowY += 0x16);
    Text_PrintfColored(gVillageBoardTitles[VILLAGE_BOARD_TOP_HELP]);
    Menu_DrawDashLine(left + 0x14, rowY + 0x2C, 0x126, 0xFF000000);
    rowY += 0x3E;
    centerX = left + 0xA7;
    for (i = 0; i < VILLAGE_BOARD_CATEGORY_COUNT; i++) {
        label = gVillageBoardTexts[i];
        textX = Text_CenterX(label, centerX, 0x12);
        if (board->category == i) {
            Menu_DrawArrowsRev(textX, rowY, 0x12, label, 0xFF008080);
        }
        Font_SetPos(textX, rowY);
        Text_PrintfColored(label);
        rowY += 0x16;
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gVillageBoardTitles[VILLAGE_BOARD_TOP_PROMPT]);
}

void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
s32 sprintf(char *buf, char *fmt, ...);
s32 VillageBoard_IsRead(u16 n);

/*
 * Draws the titles at (x, y): the title bar and panel, the page number, the
 * category's name in brackets and its subtitle, then the page's titles,
 * each with ICON_UNREAD_MARK beside it while unread and the arrows on the
 * cursor's, and the help box.
 */
void VillageBoard_DrawArticles(VillageBoardMenu *board, s16 x, s16 y)
{
    s32 left;
    s32 i;
    s16 rowY = y;
    char *label;
    char line[128];
    VillageBoardArticle *articles;
    s16 centerX;
    s16 textX;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(VILLAGE_BOARD_X_IN, 0x12, 0xC8, 0xFFB0B000, gVillageBoardTitles[VILLAGE_BOARD_TITLE], 2, 1);
    Icon_Draw(x, y, 0x14E, 0x11A, 0xFFE0D8C8, 1);
    left = x;
    centerX = left + 0xA7;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(left + 0xFE, rowY + 4, board->page, board->pageCount, 1);
    sprintf(line, gPriceInfoTexts[PRICE_INFO_BRACKET_FMT], gVillageBoardTexts[board->category]);
    Font_SetPos(Text_CenterX(line, centerX, 0x12), rowY += 0x18);
    Text_PrintfColored(gVillageBoardTextFmt, line);
    label = gVillageBoardTexts[VILLAGE_BOARD_CATEGORY_COUNT + board->category];
    Font_SetPos(Text_CenterX(label, centerX, 0x12), rowY += 0x16);
    Text_PrintfColored(gVillageBoardTextFmt, label);
    Menu_DrawDashLine(left + 0x14, rowY + 0x16, 0x126, 0xFF000000);
    articles = gVillageBoardArticles[board->category];
    rowY += 0x28;
    for (i = 0; i < board->rowCount; i++) {
        sprintf(line, "%s%s%s", gLobbyWords[LOBBY_WORD_WIDE_SPACE], articles[board->rows[i]].title, gLobbyWords[LOBBY_WORD_WIDE_SPACE]);
        label = line;
        textX = Text_CenterX(line, centerX, 0x12);
        Font_SetPos(textX, rowY);
        Text_PrintfColored(line);
        if (VillageBoard_IsRead(board->categoryFirst + board->rows[i]) == 0) {
            Icon_Draw(textX, rowY, 0x10, 0x14, 0xFFC03000, ICON_UNREAD_MARK);
        }
        if (board->cursor == i) {
            Menu_DrawArrowsRev(textX, rowY, 0x12, label, 0xFF008080);
        }
        rowY += 0x16;
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gVillageBoardTitles[VILLAGE_BOARD_ARTICLE_PROMPT]);
}

/* Draws the open article at (x, y): once its window is in place, the
 * category's label bar; then the article's panel, its title centred over a
 * line, and its text. */
void VillageBoard_DrawArticle(VillageBoardMenu *board, s16 x, s16 y)
{
    VillageBoardArticle *article;
    char *title;
    s16 rowY = y;
    s16 centerX;
    s32 left = x;

    centerX = left + 0x118;
    if (board->step == VILLAGE_BOARD_ARTICLE) {
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(0x17C, 0x30, 0xF0, 0xFFB0B000, gVillageBoardTexts[board->category], 2, 1);
    }
    Icon_Draw(x, y, 0x230, 0x140, 0xFFE0D8C8, 2);
    article = gVillageBoardArticles[board->category];
    article += board->rows[board->cursor];
    Font_SetColor(0x80101010);
    title = article->title;
    Font_SetSpacing(0x16, 0x16);
    Font_SetPos(Text_CenterX(title, centerX, 0x16), rowY += 0x1E);
    Text_PrintfColored(gVillageBoardTextFmt, title);
    Menu_DrawDashLine(left + 0x32, rowY + 0x1A, 0x1CC, 0xFF000000);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(left + 0x1E, rowY + 0x28);
    Text_PrintfStyled(4, gVillageBoardTextFmt, article->text);
}

extern u8 gMenuWork;
void Text_SetSize(s32);
void VillageBoard_DrawTop(VillageBoardMenu *board, s16 x, s16 y);
void VillageBoard_DrawArticles(VillageBoardMenu *board, s16 x, s16 y);
void VillageBoard_DrawArticle(VillageBoardMenu *board, s16 x, s16 y);
/* The village board's draw: the categories, the titles or the article by
 * the step, at its window's slide. */
void VillageBoard_Draw(MenuTask *task)
{
    VillageBoardMenu *board = (VillageBoardMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    switch (board->step) {
    case VILLAGE_BOARD_SLIDE_IN:
    case VILLAGE_BOARD_CATEGORIES:
    case VILLAGE_BOARD_SLIDE_OUT:
        VillageBoard_DrawTop(board, board->boardWin.pos, 0x38);
        break;
    case VILLAGE_BOARD_TITLES:
        VillageBoard_DrawArticles(board, board->boardWin.pos, 0x38);
        break;
    case VILLAGE_BOARD_ARTICLE_IN:
    case VILLAGE_BOARD_ARTICLE:
        VillageBoard_DrawArticle(board, 0x28, board->articleWin.pos);
        break;
    }
}

/* The articles of each category, the category table and the number of
 * articles in each (with a closing 0). */
extern char gVillageBoardTitle0_0[];
extern char gVillageBoardText0_0[];
extern char gVillageBoardTitle0_1[];
extern char gVillageBoardText0_1[];
extern char gVillageBoardTitle0_2[];
extern char gVillageBoardText0_2[];
extern char gVillageBoardTitle0_3[];
extern char gVillageBoardText0_3[];
extern char gVillageBoardTitle0_4[];
extern char gVillageBoardText0_4[];
extern char gVillageBoardTitle0_5[];
extern char gVillageBoardText0_5[];
extern char gVillageBoardTitle0_6[];
extern char gVillageBoardText0_6[];
extern char gVillageBoardTitle0_7[];
extern char gVillageBoardText0_7[];
extern char gVillageBoardTitle0_8[];
extern char gVillageBoardText0_8[];
extern u32 gVillageBoardTitle0_9[];
extern char gVillageBoardText0_9[];
extern char gVillageBoardTitle0_10[];
extern char gVillageBoardText0_10[];
extern char gVillageBoardTitle0_11[];
extern char gVillageBoardText0_11[];
extern char gVillageBoardTitle0_12[];
extern char gVillageBoardText0_12[];
extern u32 gVillageBoardTitle0_13[];
extern char gVillageBoardText0_13[];
extern char gVillageBoardTitle0_14[];
extern char gVillageBoardText0_14[];
extern char gVillageBoardTitle0_15[];
extern char gVillageBoardText0_15[];
extern char gVillageBoardTitle0_16[];
extern char gVillageBoardText0_16[];
extern char gVillageBoardTitle0_17[];
extern char gVillageBoardText0_17[];
extern char gVillageBoardTitle0_18[];
extern char gVillageBoardText0_18[];
extern char gVillageBoardTitle4_0[];
extern char gVillageBoardText4_0[];
extern char gVillageBoardTitle4_1[];
extern char gVillageBoardText4_1[];
extern char gVillageBoardTitle4_2[];
extern char gVillageBoardText4_2[];
extern char gVillageBoardTitle4_3[];
extern char gVillageBoardText4_3[];
extern char gVillageBoardTitle4_4[];
extern char gVillageBoardText4_4[];
extern char gVillageBoardTitle4_5[];
extern char gVillageBoardText4_5[];
extern char gVillageBoardTitle4_6[];
extern char gVillageBoardText4_6[];
extern char gVillageBoardTitle4_7[];
extern char gVillageBoardText4_7[];
extern char gVillageBoardTitle4_8[];
extern char gVillageBoardText4_8[];
extern char gVillageBoardTitle4_9[];
extern char gVillageBoardText4_9[];
extern char gVillageBoardTitle4_10[];
extern char gVillageBoardText4_10[];
extern char gVillageBoardTitle3_0[];
extern char gVillageBoardText3_0[];
extern char gVillageBoardTitle3_1[];
extern char gVillageBoardText3_1[];
extern u32 gVillageBoardTitle3_2[];
extern char gVillageBoardText3_2[];
extern char gVillageBoardTitle3_3[];
extern char gVillageBoardText3_3[];
extern u32 gVillageBoardTitle3_4[];
extern char gVillageBoardText3_4[];
extern char gVillageBoardTitle3_5[];
extern char gVillageBoardText3_5[];
extern char gVillageBoardTitle3_6[];
extern char gVillageBoardText3_6[];
extern char gVillageBoardTitle3_7[];
extern char gVillageBoardText3_7[];
extern char gVillageBoardTitle3_8[];
extern char gVillageBoardText3_8[];
extern char gVillageBoardTitle3_9[];
extern char gVillageBoardText3_9[];
extern char gVillageBoardTitle3_10[];
extern char gVillageBoardText3_10[];
extern char gVillageBoardTitle1_0[];
extern char gVillageBoardText1_0[];
extern char gVillageBoardTitle1_1[];
extern char gVillageBoardText1_1[];
extern char gVillageBoardTitle1_2[];
extern char gVillageBoardText1_2[];
extern char gVillageBoardTitle1_3[];
extern char gVillageBoardText1_3[];
extern char gVillageBoardTitle1_4[];
extern char gVillageBoardText1_4[];
extern char gVillageBoardTitle1_5[];
extern char gVillageBoardText1_5[];
extern char gVillageBoardTitle1_6[];
extern char gVillageBoardText1_6[];
extern char gVillageBoardTitle1_7[];
extern char gVillageBoardText1_7[];
extern char gVillageBoardTitle1_8[];
extern char gVillageBoardText1_8[];
extern char gVillageBoardTitle1_9[];
extern char gVillageBoardText1_9[];
extern char gVillageBoardTitle1_10[];
extern char gVillageBoardText1_10[];
extern char gVillageBoardTitle1_11[];
extern char gVillageBoardText1_11[];
extern char gVillageBoardTitle1_12[];
extern char gVillageBoardText1_12[];
extern u32 gVillageBoardTitle1_13[];
extern char gVillageBoardText1_13[];
extern char gVillageBoardTitle1_14[];
extern char gVillageBoardText1_14[];
extern char gVillageBoardTitle1_15[];
extern char gVillageBoardText1_15[];
extern char gVillageBoardTitle1_16[];
extern char gVillageBoardText1_16[];
extern char gVillageBoardTitle1_17[];
extern char gVillageBoardText1_17[];
extern char gVillageBoardTitle1_18[];
extern char gVillageBoardText1_18[];
extern char gVillageBoardTitle1_19[];
extern char gVillageBoardText1_19[];
extern char gVillageBoardTitle1_20[];
extern char gVillageBoardText1_20[];
extern char gVillageBoardTitle1_21[];
extern char gVillageBoardText1_21[];
extern char gVillageBoardTitle1_22[];
extern char gVillageBoardText1_22[];
extern char gVillageBoardTitle1_23[];
extern char gVillageBoardText1_23[];
extern char gVillageBoardTitle1_24[];
extern char gVillageBoardText1_24[];
extern char gVillageBoardTitle1_25[];
extern char gVillageBoardText1_25[];
extern char gVillageBoardTitle1_26[];
extern char gVillageBoardText1_26[];
extern char gVillageBoardTitle2_0[];
extern char gVillageBoardText2_0[];
extern char gVillageBoardTitle2_1[];
extern char gVillageBoardText2_1[];
extern char gVillageBoardTitle2_2[];
extern char gVillageBoardText2_2[];
extern char gVillageBoardTitle2_3[];
extern char gVillageBoardText2_3[];
extern char gVillageBoardTitle2_4[];
extern char gVillageBoardText2_4[];
extern char gVillageBoardTitle2_5[];
extern char gVillageBoardText2_5[];
extern char gVillageBoardTitle2_6[];
extern char gVillageBoardText2_6[];
extern char gVillageBoardTitle2_7[];
extern char gVillageBoardText2_7[];
extern char gVillageBoardTitle2_8[];
extern char gVillageBoardText2_8[];
extern char gVillageBoardTitle2_9[];
extern char gVillageBoardText2_9[];
extern char gVillageBoardTitle2_10[];
extern char gVillageBoardText2_10[];
extern char gVillageBoardTitle2_11[];
extern char gVillageBoardText2_11[];
extern char gVillageBoardTitle2_12[];
extern char gVillageBoardText2_12[];
extern char gVillageBoardTitle2_13[];
extern char gVillageBoardText2_13[];
extern char gVillageBoardTitle2_14[];
extern char gVillageBoardText2_14[];
extern u32 gVillageBoardTitle2_15[];
extern char gVillageBoardText2_15[];
extern char gVillageBoardTitle2_16[];
extern char gVillageBoardText2_16[];
extern char gVillageBoardTitle2_17[];
extern char gVillageBoardText2_17[];
extern char gVillageBoardTitle2_18[];
extern char gVillageBoardText2_18[];
extern char gVillageBoardTitle2_19[];
extern char gVillageBoardText2_19[];
extern char gVillageBoardTitle2_20[];
extern char gVillageBoardText2_20[];
extern char gVillageBoardTitle2_21[];
extern char gVillageBoardText2_21[];
extern char gVillageBoardTitle2_22[];
extern char gVillageBoardText2_22[];
extern char gVillageBoardTitle2_23[];
extern char gVillageBoardText2_23[];
extern char gVillageBoardTitle2_24[];
extern char gVillageBoardText2_24[];
VillageBoardArticle gVillageBoardArticles_0[19] = {
#include "lobby/007EB2C0.inc"
};
VillageBoardArticle gVillageBoardArticles_4[11] = {
#include "lobby/007EB3F0.inc"
};
VillageBoardArticle gVillageBoardArticles_3[11] = {
#include "lobby/007EB4A0.inc"
};
VillageBoardArticle gVillageBoardArticles_1[27] = {
#include "lobby/007EB550.inc"
};
VillageBoardArticle gVillageBoardArticles_2[25] = {
#include "lobby/007EB700.inc"
};
VillageBoardArticle *gVillageBoardArticles[6] = {
    gVillageBoardArticles_0, gVillageBoardArticles_1, gVillageBoardArticles_2, gVillageBoardArticles_3,
    gVillageBoardArticles_4, NULL,
};
s16 gVillageBoardCounts[6] = {
#include "lobby/007EB8A8.inc"
};
