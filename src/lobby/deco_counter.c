/* The workshop's decorations counter, the third of its counters: the
 * player makes decorations from materials and money, attaches an owned
 * decoration to a piece of the equipment box, or removes one from a piece
 * for a fee.
 *
 * Its work is a DecoWork at gMenuWork + FORGE_WORK_OFFSET, after the
 * workshop's own header; the workshop's update calls Deco_Input, then
 * Deco_Confirm (wear the changed piece?), and draws with Deco_Draw and
 * Deco_DrawEquipPrompt. The recipes are gDecoRecipes (DecoRecipe: the
 * decoration and four materials, some hidden until a material has been
 * obtained). A decoration's attach fee is its DecoData price, the removal
 * fee a tenth of it, and making one costs half its item price. The box
 * screens reuse the lobby's equipment box list, the equipment comparison
 * and the skills window, as the armour and weapon forges do. The screens,
 * their steps and their detail views are lobby_forge.h's DECO_* names. */
#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "lobby_globals.h"
#include "save.h"
#include "scene.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_forge.h"
#include "pad.h"
#include "chat_log.h"
#include "item.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "box_menu.h"
#include "camera.h"
#include "text.h"
#include "equip_parts.h"
#include "font.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "town_frame.h"
#include "libc.h"
#include "window.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4
extern s8 gMenuInputLock;
/* end of prototypes */

extern u8 gLobbyTextDirty;
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
extern LobbyMenu *gLobbyMenu;
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: declared with s16 coordinates as the counter's draws pass them; with the definition's s32 coordinates and s16 kind the calls compile differently */
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: declared with s16 coordinates as the draws pass them; with the definition's s32 ones the calls compile differently */
/* The workshop opens the counter with it (Workshop_OpenCounter); it has
 * nothing to set up: Deco_Input starts at the top menu. */
void Deco_Open(void)
{
}

/*
 * The decorations counter's input, once a frame while the workshop shows
 * it: the top menu (make, attach, remove), then the chosen screen. Returns
 * MENU_DONE when an attach or a removal has been chosen (the workshop then
 * asks whether to wear the piece), MENU_CANCEL when the top menu is left,
 * else MENU_RUNNING.
 */
s32 Deco_Input(u8 *task, u16 pad)
{
    DecoWork *counter = (DecoWork *)(task + FORGE_WORK_OFFSET);

    switch (counter->mode) {
    case DECO_MODE_MENU:
        switch (Deco_MenuInput(counter, pad)) {
        case MENU_DONE:
            switch (counter->choice) {
            case DECO_CHOICE_MAKE:
                counter->mode = DECO_MODE_MAKE;
                Deco_OpenMake(counter);
                break;
            case DECO_CHOICE_ATTACH:
                counter->mode = DECO_MODE_ATTACH;
                Deco_OpenAttach(counter);
                break;
            case DECO_CHOICE_REMOVE:
                counter->mode = DECO_MODE_REMOVE;
                Deco_OpenRemove(counter);
                break;
            }
            CamFocus_SetAside(1);
            Chat_SetColor(0);
            break;
        case MENU_CANCEL:
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        break;
    case DECO_MODE_MAKE:
        switch (Deco_MakeInput(counter, pad)) {
        case MENU_DONE:
            counter->mode = DECO_MODE_MENU;
            return MENU_DONE;
        case MENU_CANCEL:
            counter->mode = DECO_MODE_MENU;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case DECO_MODE_ATTACH:
        switch (Deco_AttachInput(counter, pad)) {
        case MENU_DONE:
            counter->mode = DECO_MODE_MENU;
            return MENU_DONE;
        case MENU_CANCEL:
            counter->mode = DECO_MODE_MENU;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case DECO_MODE_REMOVE:
        switch (Deco_RemoveInput(counter, pad)) {
        case MENU_DONE:
            counter->mode = DECO_MODE_MENU;
            return MENU_DONE;
        case MENU_CANCEL:
            counter->mode = DECO_MODE_MENU;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

s32 Forge_CheckCondition(s16 cond);
/*
 * Whether a decoration recipe is shown: it is hidden while one of its
 * materials has a condition that holds and that material has never been
 * obtained. Returns 1 shown, 0 hidden.
 */
s32 Deco_IsRecipeShown(DecoRecipe *recipe)
{
    s16 i;

    for (i = 0; i < DECO_RECIPE_MATS; i++) {
        if (recipe->need[i].cond != 0 && Forge_CheckCondition(recipe->need[i].cond) == 1 && Save_IsItemObtained(&gSave, recipe->need[i].item) == 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * Fills the make list with the shown recipes of a page (seven per page):
 * their gDecoRecipes indices, the row count, the page and the page count,
 * and keeps the cursor inside the list.
 */
void Deco_LoadRecipes(DecoWork *counter, s16 page)
{
    DecoRecipe *recipe;
    s32 i;
    s16 shown;
    u16 product;
    s16 first = page * FORGE_LIST_ROWS;
    s16 last = first + FORGE_LIST_ROWS - 1;

    memset(counter->recipes, 0, sizeof(counter->recipes));
    counter->count = 0;
    shown = 0;
    i = 0;
    recipe = gDecoRecipes;
    product = gDecoRecipes[0].deco;
    while (product != 0) {
        if (Deco_IsRecipeShown(recipe) == 1) {
            if (shown >= first && shown <= last) {
                counter->recipes[counter->count] = i;
                counter->count++;
            }
            shown++;
        }
        recipe++;
        i++;
        product = recipe->deco;
    }
    counter->page = page;
    counter->pages = Menu_CountPages(shown, FORGE_LIST_ROWS);
    if (counter->cursor >= counter->count) {
        counter->cursor = counter->count - 1;
    }
}

/*
 * Opens the make screen: the first page of recipes with the cursor at the
 * top, no detail view, and the window starting its slide in.
 */
void Deco_OpenMake(DecoWork *counter)
{
    counter->step = DECO_MAKE_SLIDE_IN;
    counter->cursor = 0;
    counter->view = DECO_MAKE_VIEW_LIST;
    counter->page = 0;
    counter->viewPages = DECO_DETAIL_PAGES;
    Deco_LoadRecipes(counter, 0);
    WinSlide_Step(&counter->win, 640, 296);
}

/*
 * Whether n of a recipe's decoration can be made: enough money for n times
 * its price, enough of each material for n, room for n more of the
 * decoration, and n under 100. Returns 1 yes, 0 no.
 */
s32 Deco_CanMake(s32 recipeIdx, s32 quantity)
{
    DecoRecipe *recipe = &gDecoRecipes[recipeIdx];
    s32 i;
    s32 price;

    price = Item_HalfPrice(recipe->deco);
    if (gSave.money < price * quantity) {
        return 0;
    }
    for (i = 0; i < DECO_RECIPE_MATS; i++) {
        if (recipe->need[i].item != 0 && Items_HaveEnough(recipe->need[i].item, recipe->need[i].count * quantity) == 0) {
            return 0;
        }
    }
    if (Items_Room(recipe->deco) < quantity) {
        return 0;
    }
    if (quantity < DECO_MAKE_LIMIT) {
        return 1;
    }
    return 0;
}

/*
 * The make screen's steps: the window slides in, the recipe list, the
 * quantity, then the price question (the detail view and its pages still
 * change). A yes makes the decorations and goes back to the list. Returns
 * MENU_CANCEL when the list is left, else MENU_RUNNING.
 */
s32 Deco_MakeInput(DecoWork *counter, u16 pad)
{
    switch (counter->step) {
    case DECO_MAKE_SLIDE_IN:
        switch (WinSlide_Step(&counter->win, 640, 296)) {
        case MENU_DONE:
            counter->step++;
            break;
        }
        break;
    case DECO_MAKE_LIST:
        switch (Deco_RecipeListInput(counter, pad)) {
        case MENU_DONE:
            counter->step++;
            counter->quantity = 1;
            counter->view = DECO_MAKE_VIEW_INFO;
            break;
        case MENU_CANCEL:
            counter->step = DECO_MAKE_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case DECO_MAKE_QUANTITY:
        switch (Deco_QuantityInput(counter, pad)) {
        case MENU_DONE:
            counter->step++;
            counter->viewPage = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case MENU_CANCEL:
            counter->step = DECO_MAKE_LIST;
            break;
        }
        break;
    case DECO_MAKE_ASK:
        if (pad & PAD_SQUARE) {
            if (counter->view == DECO_MAKE_VIEW_INFO) {
                counter->view = DECO_MAKE_VIEW_DETAILS;
            } else {
                counter->view = DECO_MAKE_VIEW_INFO;
            }
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        } else if (pad & PAD_R1) {
            if (counter->view == DECO_MAKE_VIEW_DETAILS && counter->viewPages > 1) {
                if (++counter->viewPage >= counter->viewPages) {
                    counter->viewPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (counter->view == DECO_MAKE_VIEW_DETAILS && counter->viewPages > 1) {
                if (--counter->viewPage < 0) {
                    counter->viewPage = counter->viewPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(LOBBY_SE_SELECT)) {
            case MENU_DONE:
                counter->step = DECO_MAKE_LIST;
                Deco_Make(counter);
                break;
            case MENU_CANCEL:
                counter->step = DECO_MAKE_LIST;
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Makes the chosen quantity of the recipe's decoration: takes the money
 * and the materials, shows the made notice and gives the decorations, with
 * the notice that some went to the item box when the pouch is full.
 */
void Deco_Make(DecoWork *counter)
{
    DecoRecipe *recipe = &gDecoRecipes[counter->recipes[counter->cursor]];
    s32 price;
    s32 i;

    price = Item_HalfPrice(recipe->deco);
    Save_AddMoney(-(price * counter->quantity));
    LobbySe_Play(LOBBY_SE_MONEY);
    for (i = 0; i < DECO_RECIPE_MATS; i++) {
        Items_Take(recipe->need[i].item, recipe->need[i].count * counter->quantity);
    }
    Notice_Push(NOTICE_KIND_ITEM, 18, recipe->deco);
    if (Items_Give(recipe->deco, counter->quantity) != 0) {
        Menu_PushNoticeOnce(LOBBY_NOTICE_SENT_TO_BOX);
    }
}

/*
 * The quantity step: up and down add or take one (up only while that many
 * can be made), left goes back to one, right jumps to the most that can be
 * made; square switches the detail view and R1/L1 turn its pages. Returns
 * MENU_DONE on circle, MENU_CANCEL on cross, else MENU_RUNNING.
 */
s32 Deco_QuantityInput(DecoWork *counter, u16 pad)
{
    s32 recipeIdx = counter->recipes[counter->cursor];
    s32 count;
    s16 oldQuantity;

    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_UP) {
        if (Deco_CanMake(recipeIdx, counter->quantity + 1) == 1) {
            counter->quantity++;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_DOWN) {
        if (counter->quantity > 1) {
            counter->quantity--;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_LEFT) {
        if (counter->quantity > 1) {
            counter->quantity = 1;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_RIGHT) {
        count = counter->quantity;
        oldQuantity = count;
        for (; count <= DECO_MAKE_LIMIT; count++) {
            if (Deco_CanMake(recipeIdx, count) == 0) {
                counter->quantity = count - 1;
                break;
            }
        }
        if (counter->quantity != oldQuantity) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_SQUARE) {
        if (counter->view == DECO_MAKE_VIEW_INFO) {
            counter->view = DECO_MAKE_VIEW_DETAILS;
        } else {
            counter->view = DECO_MAKE_VIEW_INFO;
        }
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & PAD_R1) {
        if (counter->view == DECO_MAKE_VIEW_DETAILS && counter->viewPages > 1) {
            if (++counter->viewPage >= counter->viewPages) {
                counter->viewPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (counter->view == DECO_MAKE_VIEW_DETAILS && counter->viewPages > 1) {
            if (--counter->viewPage < 0) {
                counter->viewPage = counter->viewPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    }
    return MENU_RUNNING;
}

/*
 * The recipe list: circle chooses a recipe that can be made once (a
 * refusal sound otherwise), square cycles the recipe's two detail views
 * and R1/L1 turn their pages, left and right turn the list's page, up and
 * down move the cursor; cross closes the detail view, or leaves the list.
 * Returns MENU_DONE when a recipe is chosen, MENU_CANCEL when the list is
 * left, else MENU_RUNNING.
 */
s32 Deco_RecipeListInput(DecoWork *counter, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        s32 idx = counter->recipes[counter->cursor];

        if (counter->count != 0 && Deco_CanMake(idx, 1) == 1) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        if (counter->count != 0) {
            LobbySe_Play(LOBBY_SE_REFUSE);
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        if (counter->view == DECO_MAKE_VIEW_LIST) {
            return MENU_CANCEL;
        }
        counter->view = DECO_MAKE_VIEW_LIST;
    } else if (pad & PAD_SQUARE) {
        if (counter->count != 0) {
            if (++counter->view > DECO_MAKE_VIEW_DETAILS) {
                counter->view = DECO_MAKE_VIEW_INFO;
            }
            counter->viewPage = 0;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & PAD_R1) {
        if (counter->view == DECO_MAKE_VIEW_DETAILS && counter->viewPages > 1) {
            if (++counter->viewPage >= counter->viewPages) {
                counter->viewPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (counter->view == DECO_MAKE_VIEW_DETAILS && counter->viewPages > 1) {
            if (--counter->viewPage < 0) {
                counter->viewPage = counter->viewPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (counter->pages > 1) {
            counter->page = Menu_WrapLeftRight(counter->page, counter->pages);
            Deco_LoadRecipes(counter, counter->page);
        }
    } else if (pad & (PAD_UP | PAD_DOWN)) {
        if (counter->count > 1) {
            counter->cursor = Menu_WrapUpDownSe(counter->cursor, counter->count, LOBBY_SE_CURSOR);
        }
    }
    return MENU_RUNNING;
}

/*
 * Opens the remove screen: the equipment box at its first page and slot,
 * no detail view, and the window starting its slide in.
 */
void Deco_OpenRemove(DecoWork *counter)
{
    counter->step = DECO_REMOVE_SLIDE_IN;
    counter->row = 0;
    counter->col = 0;
    counter->page = 0;
    counter->pages = Box_GetPageCount(&gSave);
    counter->view = DECO_BOX_VIEW_GRID;
    counter->equip = gSave.box[counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col];
    WinSlide_Step(&counter->win, 640, 300);
}

/*
 * The remove screen's steps: the window slides in, the player picks a box
 * piece that holds decorations, then one of its decorations, then answers
 * the fee question (the detail pages still turn). A yes builds the piece
 * without that decoration and returns MENU_DONE; leaving the box returns
 * MENU_CANCEL; else MENU_RUNNING.
 */
s32 Deco_RemoveInput(DecoWork *counter, u16 pad)
{
    switch (counter->step) {
    case DECO_REMOVE_SLIDE_IN:
        switch (WinSlide_Step(&counter->win, 640, 300)) {
        case MENU_DONE:
            counter->step++;
            break;
        }
        break;
    case DECO_REMOVE_BOX:
        switch (Deco_BoxInput(counter, pad)) {
        case MENU_DONE:
            counter->step++;
            Deco_OpenRemovePanel(counter);
            break;
        case MENU_CANCEL:
            counter->step = DECO_REMOVE_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case DECO_REMOVE_PANEL:
        switch (Deco_ChooseRemove(counter, pad)) {
        case MENU_DONE:
            counter->step++;
            counter->view = DECO_VIEW_COMPARE;
            Deco_CountPanelViewPages(counter);
            counter->viewPage = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case MENU_CANCEL:
            counter->step = DECO_REMOVE_BOX;
            counter->viewPage = 0;
            break;
        }
        break;
    case DECO_REMOVE_ASK:
        if (pad & PAD_R1) {
            if (counter->viewPages > 1) {
                if (++counter->viewPage >= counter->viewPages) {
                    counter->viewPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (counter->viewPages > 1) {
                if (--counter->viewPage < 0) {
                    counter->viewPage = counter->viewPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(LOBBY_SE_SELECT)) {
            case MENU_DONE:
                counter->step = DECO_REMOVE_SLIDE_IN;
                counter->changed = counter->equip;
                Equip_RemoveDecoration(&counter->changed, counter->cursor);
                return MENU_DONE;
            case MENU_CANCEL:
                counter->step = DECO_REMOVE_PANEL;
                counter->view = DECO_VIEW_NONE;
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Lists the chosen piece's three decoration slots for the removal panel,
 * counts the decorations, puts the cursor on the first and starts the
 * panel's slide in.
 */
void Deco_OpenRemovePanel(DecoWork *counter)
{
    s32 decoCount;
    s32 i;

    memset(counter->list, 0, sizeof(counter->list));
    decoCount = 0;
    for (i = 0; i < 3; i++) {
        counter->list[i] = counter->equip.deco[i];
        if (counter->list[i] != 0) {
            decoCount++;
        }
    }
    counter->cursor = 0;
    counter->count = decoCount;
    counter->viewPage = 0;
    WinSlide_Step(&counter->win2, 640, 296);
    Deco_CountPanelViewPages(counter);
}

/*
 * The removal panel: after its slide in, circle picks the decoration at
 * the cursor when its fee can be paid, square cycles the detail views
 * (only the decoration's details when the fee can't be paid), R1/L1 turn
 * the view's pages and up and down move the cursor. Returns MENU_DONE when
 * a decoration is picked, MENU_CANCEL when the panel is left, else
 * MENU_RUNNING.
 */
s32 Deco_ChooseRemove(DecoWork *counter, u16 pad)
{
    s8 changed = 0;

    switch (counter->panelStep) {
    case DECO_PANEL_SLIDE_IN:
        switch (WinSlide_Step(&counter->win2, 640, 296)) {
        case MENU_DONE:
            counter->panelStep++;
            break;
        }
        break;
    case DECO_PANEL_CHOOSE:
        if (pad & PAD_CIRCLE) {
            if (Deco_GetRemoveFee(counter->list[counter->cursor]) <= gSave.money) {
                counter->panelStep = DECO_PANEL_SLIDE_IN;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
            LobbySe_Play(LOBBY_SE_REFUSE);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            if (counter->view == DECO_VIEW_NONE) {
                counter->panelStep = DECO_PANEL_SLIDE_IN;
                return MENU_CANCEL;
            }
            counter->view = DECO_VIEW_NONE;
        } else if (pad & PAD_SQUARE) {
            if (Deco_GetRemoveFee(counter->list[counter->cursor]) <= gSave.money) {
                if (++counter->view >= DECO_VIEW_COUNT) {
                    counter->view = DECO_VIEW_DETAILS;
                }
            } else {
                counter->view ^= 1;
            }
            Deco_CountPanelViewPages(counter);
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        } else if (pad & PAD_R1) {
            if (counter->view != DECO_VIEW_NONE && counter->viewPages > 1) {
                if (++counter->viewPage >= counter->viewPages) {
                    counter->viewPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (counter->view != DECO_VIEW_NONE && counter->viewPages > 1) {
                if (--counter->viewPage < 0) {
                    counter->viewPage = counter->viewPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & (PAD_UP | PAD_DOWN)) {
            if (counter->view < DECO_VIEW_COMPARE) {
                counter->cursor = Menu_WrapUpDownSe(counter->cursor, counter->count, LOBBY_SE_CURSOR);
                changed = 1;
            }
        }
        if (changed != 0) {
            Deco_CountPanelViewPages(counter);
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Fills the attach list with a page (seven per page) of the decorations
 * the player owns, in item id order, with the page and page count, and
 * keeps the cursor inside the list.
 */
void Deco_LoadOwned(DecoWork *counter, s16 page)
{
    s32 first;
    s32 last;
    s32 owned;
    u16 item;

    memset(counter->list, 0, sizeof(counter->list));
    first = page * FORGE_LIST_ROWS;
    last = first + FORGE_LIST_ROWS - 1;
    counter->count = 0;
    owned = 0;
    for (item = 1; item < ITEM_ID_COUNT; item++) {
        if (gItemData[item].deco != 0 && Items_HaveEnough(item, 1) == 1) {
            if (first <= owned && owned <= last) {
                counter->list[counter->count] = item;
                counter->count++;
            }
            owned++;
        }
    }
    if (counter->cursor >= counter->count) {
        counter->cursor = counter->count - 1;
    }
    counter->decoPage = page;
    counter->decoPages = Menu_CountPages(owned, FORGE_LIST_ROWS);
}

/*
 * Opens the attach screen: the equipment box at its first page and slot,
 * no detail view, and the window starting its slide in.
 */
void Deco_OpenAttach(DecoWork *counter)
{
    counter->step = DECO_ATTACH_SLIDE_IN;
    counter->row = 0;
    counter->col = 0;
    counter->page = 0;
    counter->pages = Box_GetPageCount(&gSave);
    counter->view = DECO_BOX_VIEW_GRID;
    counter->equip = gSave.box[counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col];
    WinSlide_Step(&counter->win, 640, 300);
}

/*
 * The attach screen's steps: the window slides in, the player picks a box
 * piece with a free slot, the decoration list slides in, the player picks
 * a decoration, then answers the fee question (the detail pages still
 * turn). Keeps the piece as it would be with the decoration for the
 * previews. Returns MENU_DONE on a yes, MENU_CANCEL when the box is left,
 * else MENU_RUNNING.
 */
s32 Deco_AttachInput(DecoWork *counter, u16 pad)
{
    switch (counter->step) {
    case DECO_ATTACH_SLIDE_IN:
        switch (WinSlide_Step(&counter->win, 640, 300)) {
        case MENU_DONE:
            counter->step++;
            break;
        }
        break;
    case DECO_ATTACH_BOX:
        switch (Deco_BoxInput(counter, pad)) {
        case MENU_DONE:
            counter->step++;
            counter->view = DECO_VIEW_NONE;
            counter->cursor = 0;
            counter->viewPage = 0;
            counter->viewPages = DECO_DETAIL_PAGES;
            Deco_LoadOwned(counter, 0);
            WinSlide_Step(&counter->win2, 640, 296);
            counter->changed = counter->equip;
            Equip_AddDecoration(&counter->changed, counter->list[counter->cursor]);
            break;
        case MENU_CANCEL:
            counter->step = DECO_ATTACH_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case DECO_ATTACH_LIST_SLIDE_IN:
        switch (WinSlide_Step(&counter->win2, 640, 296)) {
        case MENU_DONE:
            counter->step++;
            break;
        }
        break;
    case DECO_ATTACH_LIST:
        switch (Deco_ChooseAttach(counter, pad)) {
        case MENU_DONE:
            counter->step++;
            counter->viewPage = 0;
            counter->view = DECO_VIEW_COMPARE;
            Deco_CountPanelViewPages(counter);
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case MENU_CANCEL:
            counter->step = DECO_ATTACH_BOX;
            counter->view = DECO_VIEW_NONE;
            counter->viewPage = 0;
            break;
        }
        break;
    case DECO_ATTACH_ASK:
        if (pad & PAD_R1) {
            if (counter->viewPages > 1) {
                if (++counter->viewPage >= counter->viewPages) {
                    counter->viewPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (counter->viewPages > 1) {
                if (--counter->viewPage < 0) {
                    counter->viewPage = counter->viewPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(LOBBY_SE_SELECT)) {
            case MENU_DONE:
                counter->step = DECO_ATTACH_SLIDE_IN;
                return MENU_DONE;
            case MENU_CANCEL:
                counter->step = DECO_ATTACH_LIST;
                counter->view = DECO_VIEW_NONE;
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Half an item's buy price, at least 1: the price of making one
 * decoration.
 */
s32 Item_HalfPrice(u16 item)
{
    s32 price = gItemData[item].price >> 1;

    if (price == 0) {
        price = 1;
    }
    return price;
}

/*
 * A decoration's attach fee, its DecoData price; 100 for an item without
 * decoration data.
 */
s32 Deco_GetAttachFee(u16 item)
{
    DecoData *data = Item_GetDecoration(item);

    if (data == NULL) {
        return DECO_NO_DATA_FEE;
    }
    return data->price;
}

/*
 * The fee for removing a decoration: a tenth of its attach fee, at least
 * 1.
 */
s32 Deco_GetRemoveFee(u16 item)
{
    s32 fee = Deco_GetAttachFee(item) / 10;

    if (fee == 0) {
        fee = 1;
    }
    return fee;
}

/*
 * Why a decoration can't be attached to a piece, as bits:
 * DECO_ATTACH_NO_FIT the piece's kind doesn't take it,
 * DECO_ATTACH_NO_SLOTS too few free slots, DECO_ATTACH_NO_MONEY the fee is
 * more than the money held. Returns 0 when it can be attached.
 */
s32 Deco_CheckAttach(EquipItem *piece, u16 item)
{
    s32 why = 0;
    s32 cost;

    if (Deco_FitsEquip(piece, item) == 0) {
        why |= DECO_ATTACH_NO_FIT;
    }
    if (Equip_FreeSlots(piece) < Item_GetDecoration(item)->size) {
        why |= DECO_ATTACH_NO_SLOTS;
    }
    cost = Deco_GetAttachFee(item);
    if (gSave.money < cost) {
        why |= DECO_ATTACH_NO_MONEY;
    }
    return why;
}

/*
 * The decoration list: circle picks the decoration at the cursor when it
 * can be attached (a refusal sound otherwise), square cycles the detail
 * views (only the decoration's details when it can't be attached), R1/L1
 * turn the view's pages, left and right turn the list's page, up and down
 * move the cursor; cross closes the view, or leaves the list. Keeps the
 * piece as it would be with the decoration at the cursor. Returns
 * MENU_DONE when picked, MENU_CANCEL when left, else MENU_RUNNING.
 */
s32 Deco_ChooseAttach(DecoWork *counter, u16 pad)
{
    s8 changed = 0;

    if (pad & PAD_CIRCLE) {
        if (counter->count != 0) {
            if (Deco_CheckAttach(&counter->equip, counter->list[counter->cursor]) == 0) {
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
            LobbySe_Play(LOBBY_SE_REFUSE);
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        if (counter->view == DECO_VIEW_NONE) {
            return MENU_CANCEL;
        }
        counter->view = DECO_VIEW_NONE;
    } else if (pad & PAD_SQUARE) {
        if (counter->count != 0) {
            if (Deco_CheckAttach(&counter->equip, counter->list[counter->cursor]) != 0) {
                counter->view ^= 1;
            } else if (++counter->view >= DECO_VIEW_COUNT) {
                counter->view = DECO_VIEW_NONE;
            }
            counter->viewPage = 0;
            Deco_CountPanelViewPages(counter);
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & PAD_R1) {
        if (counter->count != 0 && counter->view != DECO_VIEW_NONE && counter->viewPages > 1) {
            if (++counter->viewPage >= counter->viewPages) {
                counter->viewPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (counter->count != 0 && counter->view != DECO_VIEW_NONE && counter->viewPages > 1) {
            if (--counter->viewPage < 0) {
                counter->viewPage = counter->viewPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (counter->count != 0 && counter->view < DECO_VIEW_COMPARE) {
            counter->decoPage = Menu_WrapLeftRight(counter->decoPage, counter->decoPages);
            Deco_LoadOwned(counter, counter->decoPage);
            changed = 1;
        }
    } else if (pad & (PAD_UP | PAD_DOWN)) {
        if (counter->count != 0 && counter->view < DECO_VIEW_COMPARE) {
            counter->cursor = Menu_WrapUpDownSe(counter->cursor, counter->count, LOBBY_SE_CURSOR);
            changed = 1;
        }
    }
    if (changed != 0) {
        Deco_CountPanelViewPages(counter);
    }
    counter->changed = counter->equip;
    Equip_AddDecoration(&counter->changed, counter->list[counter->cursor]);
    return MENU_RUNNING;
}

/*
 * The equipment box of the attach and remove screens: circle picks the
 * piece at the cursor when it has a free slot (attach) or a decoration
 * (remove), else shows the matching notice; square opens and cycles the
 * piece's detail views, R1/L1 turn their pages, R2/L2 turn the box page,
 * the d-pad moves over the page's ten by ten slots; cross closes the view,
 * or leaves. Keeps a copy of the piece at the cursor. Returns MENU_DONE
 * when picked, MENU_CANCEL when left, else MENU_RUNNING.
 */
s32 Deco_BoxInput(DecoWork *counter, u16 pad)
{
    s16 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    s32 count;
    s16 newSlot;

    if (pad & PAD_CIRCLE) {
        if (gSave.box[slot].used != 0) {
            if (counter->mode == DECO_MODE_REMOVE) {
                count = Equip_CountDecos(&gSave.box[slot]);
            } else {
                count = Equip_FreeSlots(&gSave.box[slot]);
            }
            if (count > 0) {
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
            if (counter->mode == DECO_MODE_REMOVE) {
                Menu_PushNoticeOnce(LOBBY_NOTICE_NO_DECO);
            } else {
                Menu_PushNoticeOnce(LOBBY_NOTICE_NO_FREE_SLOT);
            }
            LobbySe_Play(LOBBY_SE_REFUSE);
        } else {
            LobbySe_Play(LOBBY_SE_REFUSE);
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        if (counter->view == DECO_BOX_VIEW_GRID) {
            return MENU_CANCEL;
        }
        counter->view = DECO_BOX_VIEW_GRID;
    } else if (pad & PAD_SQUARE) {
        if (counter->view == DECO_BOX_VIEW_GRID) {
            if (gSave.box[slot].id != 0) {
                counter->view++;
            }
            counter->viewPage = 0;
            counter->viewPages = Equip_LevelMark(&gSave.box[slot], 0, NULL);
        } else {
            if (++counter->view >= DECO_BOX_VIEW_COUNT) {
                counter->view = DECO_BOX_VIEW_DETAIL;
            }
            Deco_CountBoxViewPages(counter);
        }
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & PAD_R1) {
        if (counter->view != DECO_BOX_VIEW_GRID && counter->viewPages > 1) {
            if (++counter->viewPage >= counter->viewPages) {
                counter->viewPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (counter->view != DECO_BOX_VIEW_GRID && counter->viewPages > 1) {
            if (--counter->viewPage < 0) {
                counter->viewPage = counter->viewPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_R2) {
        if (counter->view == DECO_BOX_VIEW_GRID && counter->pages > 1) {
            if (++counter->page >= counter->pages) {
                counter->page = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L2) {
        if (counter->view == DECO_BOX_VIEW_GRID && counter->pages > 1) {
            if (--counter->page < 0) {
                counter->page = counter->pages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (counter->view == DECO_BOX_VIEW_GRID) {
            counter->col = Menu_WrapLeftRight(counter->col, BOX_PAGE_COLS);
        }
    } else {
        if (counter->view == DECO_BOX_VIEW_GRID) {
            counter->row = Menu_WrapUpDown(counter->row, BOX_PAGE_ROWS);
        }
    }
    newSlot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    if (slot != newSlot) {
        counter->equip = gSave.box[newSlot];
        Deco_CountBoxViewPages(counter);
    }
    return MENU_RUNNING;
}

/*
 * Recounts the box screen's detail view pages for the piece at the cursor:
 * the skill pages in the skills view, the piece's detail pages in the
 * details view.
 */
void Deco_CountBoxViewPages(DecoWork *counter)
{
    s32 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;

    switch (counter->view) {
    case DECO_BOX_VIEW_SKILLS:
        counter->viewPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &counter->equip);
        counter->viewPage = 0;
        break;
    case DECO_BOX_VIEW_DETAIL:
        counter->viewPages = Equip_LevelMark(&gSave.box[slot], 0, NULL);
        break;
    }
}

/*
 * Resets the attach or removal panel's detail view to its first page and
 * recounts its pages: the decoration's details (three pages), the compare
 * view of the piece before and after the change, or the skills the player
 * would have with the changed piece.
 */
void Deco_CountPanelViewPages(DecoWork *counter)
{
    EquipItem result;

    memset(&result, 0, sizeof(result));
    switch (counter->view) {
    case DECO_VIEW_SKILLS:
        result = counter->equip;
        if (counter->mode == DECO_MODE_REMOVE) {
            Equip_RemoveDecoration(&result, counter->cursor);
            counter->viewPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &result);
        } else {
            counter->viewPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &counter->changed);
        }
        counter->viewPage = 0;
        break;
    case DECO_VIEW_DETAILS:
        counter->viewPage = 0;
        counter->viewPages = DECO_DETAIL_PAGES;
        break;
    case DECO_VIEW_COMPARE:
        result = counter->equip;
        if (counter->mode == DECO_MODE_REMOVE) {
            Equip_RemoveDecoration(&result, counter->cursor);
        } else {
            Equip_AddDecoration(&result, counter->list[counter->cursor]);
        }
        counter->viewPages = Equip_LevelMark(&counter->equip, 16, &result);
        counter->viewPage = 0;
        break;
    default:
        counter->viewPages = 0;
        counter->viewPage = 0;
        break;
    }
}

/*
 * The counter's top menu: up and down move over make, attach and remove.
 * Returns MENU_DONE on circle (with the choice's sound), MENU_CANCEL on
 * cross, else MENU_RUNNING.
 */
s32 Deco_MenuInput(DecoWork *counter, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        switch (counter->choice) {
        case DECO_CHOICE_MAKE:
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            break;
        default:
            LobbySe_Play(LOBBY_SE_SELECT);
            break;
        }
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    counter->choice = Menu_WrapUpDownSe(counter->choice, DECO_CHOICE_COUNT, LOBBY_SE_CURSOR);
    return MENU_RUNNING;
}

/*
 * Attaches the chosen decoration to the chosen box piece: takes the
 * decoration and the fee, and when the piece is to be worn, wears it and
 * rebuilds the local player and its skills. Plays the attach sound, shows
 * the done notice and keeps the changed piece as the one shown.
 */
void Deco_Attach(DecoWork *counter)
{
    s32 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    u16 deco = counter->list[counter->cursor];

    Items_Take(deco, 1);
    Equip_AddDecoration(&gSave.box[slot], deco);
    Save_AddMoney(-Deco_GetAttachFee(deco));
    LobbySe_Play(LOBBY_SE_MONEY);
    if (counter->wear != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, slot);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
    }
    LobbySe_Play(LOBBY_SE_DECO_ATTACH);
    Menu_PushNotice(LOBBY_NOTICE_DECO_ATTACHED);
    counter->equip = counter->changed;
}

/*
 * Removes the decoration at the cursor from the chosen box piece and takes
 * the fee (the decoration is lost); when the piece is to be worn, wears it
 * and rebuilds the local player and its skills. Plays the removal sound,
 * shows the done notice and keeps the changed piece as the one shown.
 */
void Deco_Remove(DecoWork *counter)
{
    s32 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    u16 deco = counter->list[counter->cursor];

    Equip_RemoveDecoration(&gSave.box[slot], counter->cursor);
    Save_AddMoney(-Deco_GetRemoveFee(deco));
    LobbySe_Play(LOBBY_SE_MONEY);
    if (counter->wear != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, slot);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
    }
    LobbySe_Play(LOBBY_SE_DECO_REMOVE);
    Menu_PushNotice(LOBBY_NOTICE_DECO_REMOVED);
    counter->equip = counter->changed;
}

/*
 * The wear question after an attach or a removal, the workshop's confirm
 * step: a changed piece that can't be worn is changed without asking, and
 * one already worn is changed and stays worn; otherwise the player is asked
 * whether to wear it (with a warning when the worn weapon's type would
 * change), and R1/L1 turn the compare view's pages. Then makes the change
 * (Deco_Attach or Deco_Remove) and goes back to the box. Returns MENU_DONE
 * once the change is made, else MENU_RUNNING.
 */
s32 Deco_WearAskInput(DecoWork *counter, u16 pad)
{
    s32 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    EquipItem worn;

    memset(&worn, 0, sizeof(worn));
    switch (counter->mode) {
    case DECO_WEAR_SETUP:
        if (Equip_CanWear(&gSave, &counter->changed) == 0) {
            counter->mode = DECO_WEAR_APPLY;
            counter->wear = 0;
        } else if (Equip_IsSlotEquipped(&gSave, slot) == 1) {
            counter->mode = DECO_WEAR_APPLY;
            counter->wear = 1;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            if (counter->changed.type == EQUIP_BLADE || counter->changed.type == EQUIP_GUN) {
                if (counter->changed.type != gSave.equip[SAVE_EQUIP_WEAPON].type) {
                    counter->mode = DECO_WEAR_ASK_CHANGE;
                } else {
                    counter->mode = DECO_WEAR_ASK;
                }
            } else {
                counter->mode = DECO_WEAR_ASK;
            }
            counter->viewPage = 0;
            if (gSave.box[slot].type == EQUIP_GUN || gSave.box[slot].type == EQUIP_BLADE) {
                worn = gSave.equip[SAVE_EQUIP_WEAPON];
            } else {
                Equip_CopyEquipped(&worn, counter->changed.type);
            }
            counter->viewPages = Equip_LevelMark(&worn, 16, &counter->changed);
        }
        break;
    case DECO_WEAR_ASK:
    case DECO_WEAR_ASK_CHANGE:
        if (pad & PAD_R1) {
            if (counter->viewPages > 1) {
                if (++counter->viewPage >= counter->viewPages) {
                    counter->viewPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (counter->viewPages > 1) {
                if (--counter->viewPage < 0) {
                    counter->viewPage = counter->viewPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                counter->mode = DECO_WEAR_APPLY;
                counter->wear = 1;
                break;
            case MENU_CANCEL:
                counter->mode = DECO_WEAR_APPLY;
                counter->wear = 0;
                break;
            }
        }
        break;
    case DECO_WEAR_APPLY:
        switch (counter->choice) {
        case DECO_CHOICE_ATTACH:
            counter->mode = DECO_MODE_ATTACH;
            counter->step = DECO_ATTACH_SLIDE_IN;
            Deco_Attach(counter);
            break;
        case DECO_CHOICE_REMOVE:
            counter->mode = DECO_MODE_REMOVE;
            counter->step = DECO_REMOVE_SLIDE_IN;
            Deco_Remove(counter);
            break;
        }
        counter->view = DECO_BOX_VIEW_GRID;
        counter->viewPage = 0;
        WinSlide_Step(&counter->win, 640, 300);
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * The workshop's confirm step for the decorations counter: runs
 * Deco_WearAskInput after an attach or a removal. Returns its result,
 * MENU_DONE after a make.
 */
s32 Deco_Confirm(u8 *task, u16 pad)
{
    DecoWork *counter = (DecoWork *)(task + FORGE_WORK_OFFSET);

    switch (counter->choice) {
    case DECO_CHOICE_ATTACH:
        return Deco_WearAskInput(counter, pad);
    case DECO_CHOICE_REMOVE:
        return Deco_WearAskInput(counter, pad);
    default:
        return MENU_DONE;
    }
}

/*
 * Draws the attach screen's box step: the title and the money, then the
 * equipment box (with the piece's details in the details view) or the
 * skills window, and a message with the piece's free slots, or why none
 * can be chosen.
 */
void Deco_DrawAttachEquip(DecoWork *counter, s32 x, s16 y)
{
    s32 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    s32 freeSlots;
    char *msg;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 170, 0xFFB0B000, gDecoMenuTexts[DECO_TITLE_ATTACH], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (counter->step) {
    case DECO_ATTACH_SLIDE_IN:
    case DECO_ATTACH_BOX:
        switch (counter->view) {
        case DECO_BOX_VIEW_SKILLS:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, y, counter->viewPage);
            break;
        default:
            if (counter->view == DECO_BOX_VIEW_GRID) {
                BoxMenu_DrawItemList(slot, 16, x);
            } else {
                BoxMenu_DrawItemList(slot, (counter->viewPage & 7) | 0x78, x);
            }
            freeSlots = Equip_FreeSlots(&counter->equip);
            if (freeSlots != 0) {
                msg = Text_FillDollar(gForgeTexts[FORGE_TEXT_ATTACH_PICK_PIECE_SLOTS], freeSlots);
            } else if (counter->equip.used != 0) {
                msg = gForgeTexts[FORGE_TEXT_ATTACH_NO_FREE_SLOT];
            } else {
                msg = gForgeTexts[FORGE_TEXT_ATTACH_PICK_PIECE];
            }
            Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, msg);
            break;
        }
        break;
    }
}

void ItemInfo_Draw(s16 x, s16 y, s32 w, u8 a, u16 item, s32 b, s8 c, u32 color); /* unit's own declaration: declared with the types the counter's draws pass (an s32 width, a u16 item); with the definition's (s16 w, s32 item, u16 extra) the calls compile differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);






/*
 * Draws the owned decorations panel: the page, seven rows with each
 * decoration's fee (dimmed when it can't be attached), the cursor, and the
 * decoration's info at the bottom; the message under it says why the
 * decoration at the cursor can't be attached, or asks for a choice.
 */
void Deco_DrawAttachList(DecoWork *counter, s16 x, s16 y)
{
    u32 color;
    s16 rowY = y;
    s32 top;
    s32 left;
    u16 selItem = 0;
    s16 i;
    u16 item;
    s32 freeSlots;
    DecoData *data;
    char *msg;
    s32 textX;
    s32 cursorX;

    Win_Draw(x, y, 332, 290, 0xBDB59F6C, 0x80202020, 2);
    if (counter->list[0] == 0) {
        Font_SetSpacing(18, 18);
        Font_SetPos(Text_CenterX(gForgeTexts[FORGE_TEXT_NO_DECO_HELD], x + 166, 18), 174);
        Text_Printf(gForgeTexts[FORGE_TEXT_NO_DECO_HELD]);
    } else {
        Font_SetColor(0x807F7F7F);
        top = rowY;
        left = x;
        Menu_DrawPageArrows(left + 240, top + 10, counter->decoPage, counter->decoPages, 0xFF106060);
        rowY += 30;
        Font_SetSpacing(20, 20);
        textX = left + 50;
        cursorX = left + 10;
        for (i = 0; i < FORGE_LIST_ROWS; i++) {
            item = counter->list[i];
            Font_SetPos(textX, rowY);
            if (i == counter->cursor) {
                selItem = item;
                Menu_DrawRowHighlight(cursorX, rowY - 1, 312, 24, 0);
            }
            if (item != 0) {
                if (Deco_CheckAttach(&counter->equip, item) == 0) {
                    color = 0x807F7F7F;
                } else {
                    color = 0x8042352E;
                }
                Menu_DrawItemPriceRow(textX, rowY, item, color, Deco_GetAttachFee(item));
            }
            rowY += 22;
        }
        Gfx_DrawLine(left + 20, top + 186, 292, 2, 0xBDB59F6C);
        if (selItem != 0) {
            Font_SetColor(0x807F7F7F);
            ItemInfo_Draw((s16)(left + 50) - 56, top + 190, 332, counter->view, (s32)selItem, 0, counter->viewPage, 0x807F7F7F);
        }
    }
    if (selItem != 0) {
        freeSlots = Equip_FreeSlots(&counter->equip);
        data = Item_GetDecoration(selItem);
        if (Deco_FitsEquip(&counter->equip, selItem) == 0) {
            msg = gForgeTexts[FORGE_TEXT_ATTACH_NOT_FOR_PIECE];
        } else if (freeSlots < data->size) {
            msg = gForgeTexts[FORGE_TEXT_ATTACH_TOO_FEW_SLOTS];
        } else if (gSave.money < data->price) {
            msg = gForgeTexts[FORGE_TEXT_ATTACH_NO_MONEY];
        } else {
            msg = gForgeTexts[FORGE_TEXT_ATTACH_PICK_DECO];
        }
    } else {
        msg = gForgeTexts[FORGE_TEXT_ATTACH_PICK_DECO];
    }
    Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, msg);
}

/*
 * Draws the attach screen once a piece is chosen: the decoration list, the
 * compare view of the piece with the decoration, or the skills window, and
 * at the question step the fee question with the yes/no cursor.
 */
void Deco_DrawAttachConfirm(DecoWork *counter, s16 x, s16 y)
{
    EquipItem changed;

    memset(&changed, 0, sizeof(changed));
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 170, 0xFFB0B000, gDecoMenuTexts[DECO_TITLE_ATTACH], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (counter->view) {
    case DECO_VIEW_NONE:
    case DECO_VIEW_DETAILS:
        Deco_DrawAttachList(counter, x, y);
        break;
    case DECO_VIEW_COMPARE:
        changed = counter->equip;
        Equip_AddDecoration(&changed, counter->list[counter->cursor]);
        EquipCompare_Draw(&counter->equip, &changed, 310, y + 4, counter->viewPage | 0x50);
        break;
    case DECO_VIEW_SKILLS:
        EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, 54, counter->viewPage);
        return;
    }
    switch (counter->step) {
    case DECO_ATTACH_ASK:
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020,
                      Text_FillDollar(gForgeTexts[FORGE_TEXT_ATTACH_FEE_ASK], Deco_GetAttachFee(counter->list[counter->cursor])));
        Font_SetPos(444, 400);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

/*
 * Draws the remove screen's box step: the title and the money, the
 * equipment box (with the piece's details in the details view) or the
 * skills window, and the message asking for a piece, or saying it holds no
 * decoration.
 */
void Deco_DrawRemoveEquip(DecoWork *counter, s32 x, s16 y)
{
    s32 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    char *msg;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 170, 0xFFB0B000, gDecoMenuTexts[DECO_TITLE_REMOVE], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (counter->step) {
    case DECO_REMOVE_SLIDE_IN:
    case DECO_REMOVE_BOX:
        switch (counter->view) {
        case DECO_BOX_VIEW_SKILLS:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, y, counter->viewPage);
            break;
        default:
            if (counter->view == DECO_BOX_VIEW_GRID) {
                BoxMenu_DrawItemList(slot, 16, x);
            } else {
                BoxMenu_DrawItemList(slot, (counter->viewPage & 7) | 0x78, x);
            }
            if (Equip_CountDecos(&counter->equip) != 0) {
                msg = gForgeTexts[FORGE_TEXT_REMOVE_PICK_PIECE];
            } else if (counter->equip.used != 0) {
                msg = gForgeTexts[FORGE_TEXT_REMOVE_NONE_ATTACHED];
            } else {
                msg = gForgeTexts[FORGE_TEXT_REMOVE_PICK_PIECE];
            }
            Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, msg);
            break;
        }
        break;
    }
}

u8 Equip_GetRarity(u8 type, u16 id);
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
const char gDecoTextFmt[3] = {
#include "lobby/0082E980.inc"
};

/*
 * Draws the removal panel: the piece's icon, rarity and name, its three
 * decoration slots with each removal fee (dimmed when it can't be paid),
 * the cursor, and the info of the decoration at the cursor.
 */
void Deco_DrawRemovePanel(DecoWork *counter, s16 x, s16 y)
{
    s16 rowY = y;
    s16 left;
    s16 top;
    s16 i;
    u16 item;
    s32 selItem;
    s32 fee;

    Win_Draw(x, y, 332, 290, 0xBDB59F6C, 0x80202020, 2);
    left = x + 12;
    top = y;
    y += 14;
    Lobby_DrawEquipIcon(left, y, 60, 60, &counter->equip);
    Item_DrawRarity(left + 12, y + 60, Equip_GetRarity(counter->equip.type, counter->equip.id));
    Font_SetPos(left + 78, y + 20);
    Text_PrintfColored(gDecoTextFmt, Equip_GetName(counter->equip.type, counter->equip.id));
    Gfx_DrawLine(left, top + 94, 308, 2, 0xBDB59F6C);
    rowY += 106;
    Font_SetSpacing(20, 20);
    for (i = 0; i < 3; i++) {
        item = counter->list[i];
        Font_SetPos(left + 20, rowY);
        if (i == counter->cursor) {
            selItem = item;
            Menu_DrawRowHighlight(x + 10, rowY - 1, 312, 24, 0);
        }
        if (item != 0) {
            fee = Deco_GetRemoveFee(item);
            Menu_DrawItemPriceRow(left + 40, rowY, item, (gSave.money < fee) ? 0x8042352E : 0x807F7F7F, fee);
        }
        rowY += 22;
    }
    Gfx_DrawLine(left, rowY, 308, 2, 0xBDB59F6C);
    Font_SetColor(0x807F7F7F);
    ItemInfo_Draw(left - 16, rowY + 12, 332, counter->view, selItem, 0, counter->viewPage, 0x807F7F7F);
}

/*
 */
void Deco_DrawRemove(DecoWork *counter, s16 x, s16 y)
{
    EquipItem piece;

    memset(&piece, 0, sizeof(piece));
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 170, 0xFFB0B000, gDecoMenuTexts[DECO_TITLE_REMOVE], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (counter->view) {
    case DECO_VIEW_NONE:
    case DECO_VIEW_DETAILS:
        Deco_DrawRemovePanel(counter, x, y);
        break;
    case DECO_VIEW_COMPARE:
        piece = counter->equip;
        Equip_RemoveDecoration(&piece, counter->cursor);
        EquipCompare_Draw(&counter->equip, &piece, 310, y + 4, counter->viewPage | 0x50);
        break;
    case DECO_VIEW_SKILLS:
        EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, y, counter->viewPage);
        return;
    }
    switch (counter->step) {
    case DECO_REMOVE_PANEL:
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_REMOVE_PICK_DECO]);
        break;
    case DECO_REMOVE_ASK:
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020,
                      Text_FillDollar(gForgeTexts[FORGE_TEXT_REMOVE_FEE_ASK], Deco_GetRemoveFee(counter->list[counter->cursor])));
        Font_SetPos(444, 400);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

/*
 * Draws the make list: the page, each recipe's decoration with its price
 * (dimmed when it can't be made once), the cursor, and the info of the
 * decoration at the cursor; a message when no recipe is shown.
 */
void Deco_DrawMakeList(DecoWork *counter, s16 x, s16 y)
{
    s16 left;
    s32 top;
    DecoRecipe *recipe;
    u32 color;
    s16 rowY = y;
    s16 i;
    u16 selItem;
    char *msg;

    Icon_Draw(x, y, 340, 290, 0xFFFFDDDD, 0);
    top = rowY;
    left = x;
    Icon_Draw(left + 110, top + 45, 120, 200, 0x60FFFFFF, 36);
    Font_SetSpacing(20, 20);
    if (counter->count == 0) {
        msg = gForgeTexts[FORGE_TEXT_NO_DECO_TO_MAKE];
        Font_SetPos(Text_CenterX(msg, left + 170, 20), top + 120);
        Font_SetColor(0x8000052C);
        Text_PrintfColored(gDecoTextFmt, msg);
        return;
    }
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(left + 250, top + 14, counter->page, counter->pages, 1);
    Font_SetSpacing(20, 20);
    rowY += 30;
    for (i = 0; i < counter->count; i++) {
        recipe = &gDecoRecipes[(s16)counter->recipes[i]];
        if (Deco_CanMake((s16)counter->recipes[i], 1) == 1) {
            color = 0x80101010;
        } else {
            color = 0x8042352E;
        }
        if (i == counter->cursor) {
            selItem = recipe->deco;
            Menu_DrawCursorRow(left + 70, rowY, 250, 50, 0xC0106060, 0xFF106060);
        }
        Menu_DrawItemPriceRow(left + 70, rowY, recipe->deco, color, Item_HalfPrice(recipe->deco));
        rowY += 22;
    }
    Menu_DrawDashLine(left + 20, top + 186, 300, 0xFF000000);
    ItemInfo_Draw(left + 10, top + 195, 332, counter->view, (s32)selItem, 0, -1, 0x80101010);
}

/*
 * Draws the narrow make list shown beside a recipe's details: the page and
 * each recipe's decoration with its price, dimmed when it can't be made
 * once.
 */
void Deco_DrawRecipeList(DecoWork *counter, s16 x, s16 y)
{
    DecoRecipe *recipe;
    s32 color;
    s16 rowY = y;
    s16 i;
    s16 recipeIdx;

    Icon_Draw(x, y, 290, 180, 0xFFFFDDDD, 0);
    Icon_Draw(x + 100, y + 15, 90, 150, 0x60FFFFFF, 36);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 200, y + 12, counter->page, counter->pages, 1);
    Font_SetSpacing(17, 17);
    rowY += 30;
    for (i = 0; i < counter->count; i++) {
        recipeIdx = counter->recipes[i];
        recipe = &gDecoRecipes[recipeIdx];
        if (Deco_CanMake(recipeIdx, 1) == 1) {
            color = 0x80101010;
        } else {
            color = 0x8042352E;
        }
        if (i == counter->cursor) {
            Menu_DrawCursorRow((s16)(x + 60) - 6, rowY, 200, 50, 0xC0106060, 0xFF106060);
        }
        Menu_DrawItemPriceRow(x + 60, rowY, recipe->deco, color, Item_HalfPrice(recipe->deco));
        rowY += 20;
    }
}

/*
 * Draws the chosen recipe's panel: the decoration's info or details, then
 * its materials, each for one or (at the quantity and question steps) for
 * the chosen quantity.
 */
void Deco_DrawRecipeDetail(DecoWork *counter, s16 x, s16 y)
{
    DecoRecipe *recipe = &gDecoRecipes[counter->recipes[counter->cursor]];
    s16 rowY = y;
    s32 product = recipe->deco;
    s16 i;

    Win_Draw(x, y, 332, 290, 0xBDB59F6C, 0x80202020, 2);
    ItemInfo_Draw(x - 6, rowY += 24, 332, counter->view - 1, product, 0, counter->viewPage, 0x807F7F7F);
    Gfx_DrawLine(x + 20, rowY += 110, 292, 2, 0xBDB59F6C);
    rowY += 14;
    Font_SetSpacing(18, 18);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 14, rowY);
    if (counter->step == DECO_MAKE_QUANTITY || counter->step == DECO_MAKE_ASK) {
        Text_PrintfColored(gForgeTexts[FORGE_TEXT_USED_MATERIALS_HEAD]);
    } else {
        Text_PrintfColored(gForgeTexts[FORGE_TEXT_MATERIALS_HEAD]);
    }
    Font_SetColor(0x807F7F7F);
    x += 34;
    for (i = 0; i < DECO_RECIPE_MATS; i++) {
        if (recipe->need[i].item != 0) {
            if (counter->step == DECO_MAKE_QUANTITY || counter->step == DECO_MAKE_ASK) {
                Items_DrawNeedLineAfter(x, rowY += 20, recipe->need[i].item, recipe->need[i].count * counter->quantity, 0x807F7F7F);
            } else {
                Items_DrawNeedLine(x, rowY += 20, recipe->need[i].item, recipe->need[i].count, 0x807F7F7F);
            }
        }
    }
}

/*
 * Draws the make screen: the title and the money, the make list or the
 * recipe list with the recipe's panel, then the message, the quantity with
 * its up and down marks, or the price question with the yes/no cursor.
 */
void Deco_DrawMake(DecoWork *counter, s16 x, s16 y)
{
    char buf[16];
    char wide[16];
    s32 recipeIdx;
    s32 fee;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 170, 0xFFB0B000, gDecoMenuTexts[DECO_TITLE_MAKE], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (counter->step) {
    case DECO_MAKE_ASK:
    case DECO_MAKE_QUANTITY:
        Deco_DrawRecipeList(counter, 0, y);
        Deco_DrawRecipeDetail(counter, x, y);
        break;
    default:
        switch (counter->view) {
        case DECO_MAKE_VIEW_LIST:
            Deco_DrawMakeList(counter, x, y);
            break;
        case DECO_MAKE_VIEW_INFO:
        case DECO_MAKE_VIEW_DETAILS:
            Deco_DrawRecipeList(counter, 0, y);
            Deco_DrawRecipeDetail(counter, x, y);
            break;
        }
        break;
    }
    switch (counter->step) {
    case DECO_MAKE_ASK:
        fee = Item_HalfPrice(gDecoRecipes[counter->recipes[counter->cursor]].deco);
        fee *= counter->quantity;
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, Text_FillDollar(gShopPromptTexts[SHOP_PROMPT_PRICE_ASK], fee));
        Font_SetPos(434, 402);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    case DECO_MAKE_QUANTITY:
        recipeIdx = counter->recipes[counter->cursor];
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_MAKE_COUNT_ASK]);
        Font_SetPos(523, 388);
        sprintf(buf, "%02d", counter->quantity);
        Sjis_Widen(buf, wide);
        Text_PrintfColored(wide);
        if (Deco_CanMake(recipeIdx, counter->quantity + 1) == 1 && counter->quantity < DECO_MAKE_LIMIT - 1) {
            Icon_DrawMode(524, 366, 36, 22, 0xFF00FF00, 41, 2);
        }
        if (counter->quantity > 1) {
            Icon_DrawMode(525, 406, 36, 22, 0xFF00FF00, 40, 2);
        }
        break;
    default:
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_MAKE_PICK_DECO]);
        break;
    }
}

/*
 * Draws the decorations counter for the workshop: the top menu, or the
 * make, attach or remove screen at its step, its window at the slide's
 * position.
 */
void Deco_Draw(u8 *task)
{
    DecoWork *counter = (DecoWork *)(task + FORGE_WORK_OFFSET);

    switch (counter->mode) {
    case DECO_MODE_MENU:
        Font_SetSpacing(20, 20);
        Menu_DrawLabelList(460, gDecoMenuTexts, DECO_CHOICE_COUNT, counter->choice, 0xFFB0B000);
        break;
    case DECO_MODE_MAKE:
        Deco_DrawMake(counter, counter->win.pos, 52);
        break;
    case DECO_MODE_ATTACH:
        switch (counter->step) {
        case DECO_ATTACH_LIST_SLIDE_IN:
        case DECO_ATTACH_LIST:
        case DECO_ATTACH_ASK:
            Deco_DrawAttachConfirm(counter, counter->win2.pos, 52);
            break;
        default:
            Deco_DrawAttachEquip(counter, (s16)(counter->win.pos + 12), 52);
            break;
        }
        break;
    case DECO_MODE_REMOVE:
        switch (counter->step) {
        case DECO_REMOVE_SLIDE_IN:
        case DECO_REMOVE_BOX:
            Deco_DrawRemoveEquip(counter, (s16)(counter->win.pos + 12), 52);
            break;
        case DECO_REMOVE_PANEL:
        case DECO_REMOVE_ASK:
            Deco_DrawRemove(counter, counter->win2.pos, 52);
            break;
        }
        break;
    }
}

/* The workshop's second draw layer for this counter (Workshop_Draw2):
 * nothing is drawn on it. */
void Deco_Draw2(u8 *unused0)
{
}

/*
 * Draws the wear question after an attach or a removal: the compare view
 * of the worn piece of that kind against the changed piece, the question
 * (or the warning that the weapon kind changes) and the yes/no cursor.
 * choice is not used.
 */
void Deco_DrawEquipAsk(DecoWork *counter, s16 choice)
{
    s32 slot = counter->row * BOX_PAGE_COLS + counter->page * BOX_PAGE_SLOTS + counter->col;
    EquipItem worn;
    char *msg;

    memset(&worn, 0, sizeof(worn));
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 140, 0xFFB0B000, gDecoMenuTexts[DECO_TITLE_ATTACH], 2, 1);
    switch (counter->mode) {
    case DECO_WEAR_ASK:
        msg = gForgeTexts[FORGE_TEXT_EQUIP_ASK];
        break;
    case DECO_WEAR_ASK_CHANGE:
        msg = gForgeTexts[FORGE_TEXT_WEAPON_CHANGE_WARN];
        break;
    default:
        return;
    }
    if (gSave.box[slot].type == EQUIP_GUN || gSave.box[slot].type == EQUIP_BLADE) {
        worn = gSave.equip[SAVE_EQUIP_WEAPON];
    } else {
        Equip_CopyEquipped(&worn, counter->changed.type);
    }
    EquipCompare_Draw(&worn, &counter->changed, 310, 56, counter->viewPage | 0x50);
    Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, msg);
    Font_SetPos(324, 400);
    Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
}

/*
 * Draws the workshop's confirm step for the decorations counter: the wear
 * question after an attach or a removal.
 */
void Deco_DrawEquipPrompt(u8 *task)
{
    DecoWork *counter = (DecoWork *)(task + FORGE_WORK_OFFSET);
    s16 menuChoice = counter->choice;

    switch (menuChoice) {
    case DECO_CHOICE_ATTACH:
    case DECO_CHOICE_REMOVE:
        Deco_DrawEquipAsk(counter, menuChoice);
        break;
    }
}

/* The file's initialised data, in the game's order */
DecoRecipe gDecoRecipes[123] = {
#include "lobby/007E8AB0.inc"
};
