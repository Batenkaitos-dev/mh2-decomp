/*
 * The mail block list: the game menu's page that lists the users on the
 * mail block list (gMailBlockList, MAIL_BLOCK_LIST_LEN BlockedUser records)
 * and removes one of them.
 *
 * The page keeps its state in the menu work area (BlockListMenu in
 * gMenuWork). The game menu's mail page opens it and runs BlockList_Update
 * once a frame; the yes/no that confirms a removal (BlockList_AskRemove,
 * BlockList_RemoveInput) belongs to the mail menus.
 */
#include "common.h"
#include "sprite.h"
#include "lobby_text.h"
#include "em.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "pad.h"
#include "mail_block_list.h"
/* Built at optimisation level 4: the compiler gives these functions the
 * game's code only at this level. */
#pragma optimization_level 4

#include "stageobj.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "font.h"
#include "friend_add.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "libc.h"
extern u8 gMenuWork[];
extern BlockedUser gMailBlockList[];

/* Opens the block list: clears the menu work area, counts the blocked
 * users, starts the window's slide in from the right edge, sets the page's
 * draw and takes the pad from the hunter. */
void BlockList_Open(void)
{
    BlockListMenu *menu = (BlockListMenu *)gMenuWork;

    memset(menu, 0, MENU_WORK_SIZE);
    menu->count = IdList_Count((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN);
    WinSlide_Step(&menu->win, 640, 340);
    LobbyMenu_SetDraw(BlockList_Draw, 0, 0, 0);
    gFieldControl = 0;
}

/* Closes the block list: gives the hunter the pad back and clears the
 * menu's draw. */
void BlockList_Close(void)
{
    gFieldControl = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
}

/* The block list's input, once a frame: slides the window in; on the list,
 * PAD_CIRCLE opens the delete box for the user under the cursor, PAD_CROSS
 * goes back to the mail menu and PAD_START closes the game menu; in the
 * delete box PAD_CIRCLE asks whether to remove the user, and once answered
 * the list is counted again. Returns MENU_RUNNING while the page stays,
 * MENU_CANCEL or MENU_CLOSE when it closes. */
s32 BlockList_Update(u16 pad)
{
    BlockListMenu *menu = (BlockListMenu *)gMenuWork;
    s32 state;

    switch (state = menu->state) {
    case BLOCK_LIST_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 340)) {
        case MENU_DONE:
            menu->state++;
            break;
        }
        break;
    case BLOCK_LIST_LIST:
        if (pad & PAD_CIRCLE) {
            if (menu->count != 0) {
                menu->state = state + 1;
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else if (pad & PAD_CROSS) {
            BlockList_Close();
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & PAD_START) {
            BlockList_Close();
            return MENU_CLOSE;
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
        }
        break;
    case BLOCK_LIST_DELETE_BOX:
        if (pad & PAD_CIRCLE) {
            menu->state = state + 1;
            BlockList_AskRemove(&gMailBlockList[menu->cursor], gMailBlockList[menu->cursor].name);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_START) {
            BlockList_Close();
            return MENU_CLOSE;
        } else if (pad & PAD_CROSS) {
            menu->state = state - 1;
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case BLOCK_LIST_ASK_REMOVE:
        switch (BlockList_RemoveInput(pad)) {
        case MENU_CANCEL:
            menu->state--;
            break;
        case MENU_DONE:
            /* removed: count again and keep the cursor on the list */
            menu->count = IdList_Count((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN);
            if (menu->count > 0 && menu->cursor >= menu->count) {
                menu->cursor = menu->count - 1;
            }
            menu->state = BLOCK_LIST_LIST;
            break;
        case MENU_CLOSE:
            BlockList_Close();
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

/* unit's own declaration: as the callers use it; with the definition's void (s16 x0, s16 y0, s16 x1, s16 y1, s8 flag), the calls in the list's and the delete box's draws compile differently */
void Menu_DrawRowHighlight(s16 x, s16 y, s32 w, s32 h, s32 sel);
/* unit's own declaration: as the callers use it; with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind), the calls in the list's and the delete box's draws compile differently */
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
extern char gBlockListFmtString[];

/* Draws the list's window at (x, y): the title, the heads of the name and
 * id columns, and a row per blocked user (two icons, the name, the id
 * widened to full-width characters), with the cursor's bar on the selected
 * row (steady once the delete box has the input). With no one on the list
 * it draws only the line that says so, centred. The texts are entries of
 * the lobby's text tables: the title is gTownLabels[TOWN_LABEL_BLOCK_LIST], the
 * column heads gNetLabels[NET_LABEL_NAME] and gNetLabels[NET_LABEL_ID], the empty list's line
 * gEmptyListTexts[EMPTY_LIST_NONE_REGISTERED]. */
void BlockList_DrawList(BlockListMenu *menu, s16 x, s16 y)
{
    s16 rowY = y;
    s16 i;
    u8 steady;
    char idText[32];
    char idWide[32];

    Win_Draw(x, y, 280, 290, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 12, 264, 20, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 10, (s16)(y + 14) - 1);
    Text_PrintfColored(gTownLabels[TOWN_LABEL_BLOCK_LIST]);
    rowY += 36;
    menu->count = IdList_Count((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN);
    if (menu->count > 0) {
        if (menu->cursor >= menu->count) {
            menu->cursor = menu->count - 1;
        }
    } else {
        Font_SetPos(Text_CenterX(gEmptyListTexts[EMPTY_LIST_NONE_REGISTERED], x + 140, 18), rowY + 70);
        Font_SetColor(0x802B7F7F);
        Text_Printf(gBlockListFmtString, gEmptyListTexts[EMPTY_LIST_NONE_REGISTERED]);
        return;
    }
    Font_SetPos(x + 38, rowY);
    Text_PrintfColored(gBlockListFmtString, gNetLabels[NET_LABEL_NAME]);
    Font_SetPos(x + 140, rowY);
    Text_PrintfColored(gBlockListFmtString, gNetLabels[NET_LABEL_ID]);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < menu->count; i++) {
        rowY += 20;
        if (menu->cursor == i) {
            switch (menu->state) {
            case BLOCK_LIST_LIST:
                steady = 0;
                break;
            default:
                steady = 1;
                break;
            }
            Menu_DrawRowHighlight(x + 8, rowY - 1, 264, 20, (s8)steady);
        }
        Font_SetPos(x + 38, rowY);
        Text_PrintfColored(gBlockListFmtString, gMailBlockList[i].name);
        Font_SetPos(x + 140, rowY);
        sprintf(idText, gBlockListFmtString, &gMailBlockList[i]);
        Sjis_Widen(idText, idWide);
        Text_PrintfColored(gBlockListFmtString, idWide);
        Icon_Draw(x + 10, rowY - 1, 22, 20, -1, 85);
        Icon_Draw(x + 10, rowY - 1, 22, 20, -1, 88);
    }
}


/* Draws the delete box left of the list's window (x is the list's left
 * edge): its one option (gNetLabels[NET_LABEL_DELETE]) under the cursor's bar,
 * which pulses while the box has the input. */
void BlockList_DrawDelete(BlockListMenu *menu, s16 x, s16 y)
{
    s32 textY;
    s32 steady;
    s32 boxX;
    s32 textX;

    boxX = x - 110;
    Win_Draw(boxX, y, 110, 38, 0xBDB59F6C, 0xBB402010, 3);
    Font_SetColor(0x807F7F7F);
    switch (menu->state) {
    case BLOCK_LIST_DELETE_BOX:
        steady = 0;
        break;
    default:
        steady = 1;
        break;
    }
    textY = y;
    textX = (s16)boxX + 8;
    Menu_DrawRowHighlight(textX, (s16)(textY + 8) - 1, 94, 20, steady);
    Font_SetPos(textX, textY + 8);
    Text_PrintfColored(gNetLabels[NET_LABEL_DELETE]);
}

void Text_SetSize(s32 size);

/* The block list's draw (LobbyMenu_SetDraw): the list's window at the
 * window's slide position, and the delete box beside it while it is open. */
void BlockList_Draw(MenuTask *task)
{
    BlockListMenu *menu = (BlockListMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    BlockList_DrawList(menu, menu->win.pos, 54);
    switch (menu->state) {
    case BLOCK_LIST_DELETE_BOX:
        BlockList_DrawDelete(menu, menu->win.pos, 54);
        break;
    }
}

/* The format of the list's lines (a plain string). */
#pragma rodata on
char gBlockListFmtString[8] = {
#include "lobby/00825F20.inc"
};
#pragma rodata off
