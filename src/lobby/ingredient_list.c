/*
 * The ingredient list of the player's house (TOWN_MODE_INGREDIENTS): when
 * the player talks to the house's cook (NPC_HOUSE_COOK), the cook greets
 * the player and then a window lists the ingredients the player holds (the
 * items of gKitchenIngredients' groups, seven a page) with how many of each
 * are held and the selected one's description. Nothing is chosen here: the
 * list only shows.
 *
 * Its state is the kitchen's menu work (gMenuWork as a KitchenMenu). The
 * kitchen (Kitchen_Update, TOWN_MODE_KITCHEN) builds its pages with the same
 * IngredientList_BuildPage and draws them in the same window,
 * IngredientList_DrawWindow, to choose a dish's two ingredients.
 *
 * The file is compiled at optimization level 4, as the game's code is.
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "item.h"
#include "menu_task.h"
#include "pad.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "text.h"
#include "ingredient_list.h"
#include "font.h"
#include "item_shop.h"
#include "kitchen.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "talk.h"
#include "libc.h"
#pragma optimization_level 4

extern u8 gMenuWork[];

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u16 gLobbyEnterRand;
extern s8 gMenuInputLock;

/* Opens the ingredient list at the house's cook: the menu work cleared,
 * its draw set, the input held for 6 frames, and the cook says one of
 * COOK_TALK_GREET_COUNT greetings, picked by gLobbyEnterRand (the random
 * value drawn on entering the lobby). The cook isn't used. */
void IngredientList_Open(struct Em *cook)
{
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_INGREDIENTS;
    memset(gMenuWork, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(IngredientList_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    Talk_Say(((s32) ((s32) gLobbyEnterRand >> 1) % COOK_TALK_GREET_COUNT) + COOK_TALK_GREET_FIRST, 0);
}

/* Closes the list: back to TOWN_MODE_FREE, the draw removed and the
 * camera's focus ended. */
void IngredientList_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/* The list's frame, by its step: the greeting, the line before the list
 * (cancel there goes to the goodbye), the window sliding in with the
 * first page, the list, the window sliding out, and the goodbye before
 * closing. In the list cross leaves, square shows or hides the
 * description, left and right turn the page, up and down move the
 * cursor. */
void IngredientList_Update(void)
{
    KitchenMenu *menu = (KitchenMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (menu->step) {
    case INGREDIENT_STEP_GREET:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->step++;
            Talk_Say(COOK_TALK_LIST, 0);
            break;
        }
        break;
    case INGREDIENT_STEP_PROMPT:
        switch (Talk_Update()) {
        case MENU_DONE:
            menu->step++;
            WinSlide_Step(&menu->win, 640, 280);
            IngredientList_BuildPage(menu, 0);
            CamFocus_SetAside(1);
            break;
        case MENU_CANCEL:
            menu->step = INGREDIENT_STEP_BYE;
            Talk_Say(COOK_TALK_BYE, 0);
            break;
        }
        break;
    case INGREDIENT_STEP_OPEN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case 0:
            menu->step++;
            break;
        }
        break;
    case INGREDIENT_STEP_LIST:
        if (pad & PAD_CIRCLE) {
            /* circle does nothing here: the empty branch keeps the game's
             * order of tests */
        } else if (pad & PAD_CROSS) {
            menu->step++;
            WinSlide_Step(&menu->win, 280, 640);
            CamFocus_SetAside(0);
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_SQUARE) {
            if (menu->count != 0) {
                menu->showInfo ^= 1;
                LobbySe_Play(LOBBY_SE_DESCRIPTION);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (menu->pages > 1) {
                menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
                IngredientList_BuildPage(menu, menu->page);
            }
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
        }
        break;
    case INGREDIENT_STEP_SHUT:
        switch (WinSlide_Step(&menu->win, 280, 640)) {
        case 0:
            menu->step++;
            Talk_Say(COOK_TALK_BYE, 0);
            break;
        }
        break;
    case INGREDIENT_STEP_BYE:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            IngredientList_Close();
            break;
        }
        break;
    }
}

/* Fills the menu with page `page` of the ingredients the player holds
 * (each item of each gKitchenIngredients group with at least one held),
 * INGREDIENT_ROWS a page, and sets the count, the page and the number of
 * pages. The cursor is pulled back onto the page. The kitchen uses it
 * too. */
void IngredientList_BuildPage(KitchenMenu *menu, s16 page)
{
    s16 first;
    s16 last;
    s32 i;
    KitchenIngredient *group;
    s16 held = 0;
    s32 j;

    menu->count = 0;
    first = page * INGREDIENT_ROWS;
    last = first + INGREDIENT_ROWS - 1;
    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (group = gKitchenIngredients[i], j = 0; group[j].item != 0; j++) {
            if (Items_HaveEnough(group[j].item, 1) == 1) {
                if (held >= first && held <= last) {
                    menu->items[menu->count] = group[j].item;
                    menu->count++;
                }
                held++;
            }
        }
    }
    menu->page = page;
    menu->pages = Menu_CountPages(held, INGREDIENT_ROWS);
    if (menu->cursor >= menu->count) {
        menu->cursor = menu->count - 1;
    }
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: the definition takes s32 x and y and an s16 kind; declared as the calls here pass them, since with the definition's types IngredientList_DrawWindow's call compiles differently */
void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 n, u32 color);

/* Draws the list's window at (x, y): the title bar, the page arrows, a row
 * per ingredient with how many are held (dimmed when fewer than one for
 * each diner), the cursor's bar and, under a rule, the selected
 * ingredient's description; with nothing held, one centred line instead.
 * In the kitchen, while the two ingredients are chosen
 * (KITCHEN_STEP_PICK_FIRST, _PICK_SECOND), the ingredients already picked
 * get a bar too. */
void IngredientList_DrawWindow(KitchenMenu *menu, s16 x, s16 y)
{
    s16 rowY = y;
    s16 i;
    u16 item;
    u16 selItem;

    Win_Draw(x, y, 330, 294, 0xBDB59F6C, 0xBB402010, 8);
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 200, 0xFFB0B000, gTownLabels[TOWN_LABEL_INGREDIENT_LIST], 2, 1);
    if (menu->count == 0) {
        Font_SetSpacing(18, 18);
        Font_SetPos(Text_CenterX(gTownLabels[TOWN_LABEL_NO_INGREDIENTS], x + 165, 18), rowY + 100);
        Font_SetColor(0x802B7F7F);
        Text_PrintfColored(gTownLabels[TOWN_LABEL_NO_INGREDIENTS]);
        Gfx_DrawLine(x + 20, (s16)(rowY + 190) + 2, 290, 2, 0xBDB59F6C);
        return;
    }
    Menu_DrawPageArrows(x + 240, rowY + 10, menu->page, menu->pages, 0xBDB59F6C);
    Font_SetColor(0x807F7F7F);
    rowY += 30;
    Font_SetSpacing(20, 20);
    for (i = 0; i < menu->count; i++) {
        item = menu->items[i];
        if (gTownMode == TOWN_MODE_KITCHEN) {
            switch (menu->step) {
            case KITCHEN_STEP_PICK_FIRST:
            case KITCHEN_STEP_PICK_SECOND:
                if (menu->firstPick == menu->items[i] || menu->secondPick == item) {
                    Menu_DrawRowHighlight(x + 10, rowY - 1, 310, 24, 1);
                }
                break;
            }
        }
        if (i == menu->cursor) {
            selItem = item;
            Menu_DrawRowHighlight(x + 10, rowY - 1, 310, 24, 0);
        }
        Menu_DrawItemCountRow(x + 50, rowY, item, Items_HaveEnough(item, menu->diners) != 0 ? 0x807F7F7F : 0x80404040);
        rowY += 22;
    }
    rowY = y + 190;
    Gfx_DrawLine(x + 20, rowY + 2, 290, 2, 0xBDB59F6C);
    ItemInfo_Draw((s16)(x + 50) - 50, rowY + 4, 330, menu->showInfo, selItem, 0, -1, 0x807F7F7F);
}

/* The list's draw callback: the window, except while the cook talks. */
void IngredientList_Draw(MenuTask *task)
{
    KitchenMenu *menu = (KitchenMenu *)gMenuWork;

    switch (menu->step) {
    case INGREDIENT_STEP_PROMPT:
    case INGREDIENT_STEP_BYE:
    case INGREDIENT_STEP_GREET:
        return;
    default:
        Text_SetSize(task->textSize);
        IngredientList_DrawWindow(menu, menu->win.pos, 58);
        break;
    }
}
