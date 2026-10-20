/*
 * The town's item box menu (ItemBoxMenu at gBoxWork, box_menu.h). BoxMenu_Open opens
 * it and BoxMenu_Update runs it: the menu list, then the sub-menu picked:
 * - BoxMenu_StoreFromPouch and BoxMenu_TakeToPouch move items between the pouch and
 *   the item box;
 * - BoxMenu_EquipMenu wears equipment: single pieces, the ten saved equipment sets
 *   (EquipSet_*: edit, wear, save, name) and the symbol colour, with the sets' stats
 *   and skills previews (gPreviewPlayer, gSkillPreview);
 * - BoxMenu_ArrangeMenu swaps and sorts the boxes, BoxMenu_SellMenu sells.
 * Their shared input helpers: BoxMenu_MoveCursor, BoxMenu_InputAmount,
 * BoxMenu_SortPrompt, BoxMenu_CloseStep and Box_YesNoInput. BoxMenu_Draw and the
 * other draw functions draw the menu from the same work.
 * The .data holds gBoxPartIdx (the set editor's rows to set entries); the .rodata the
 * jump tables and gBoxEmptyText.
 */
#include "common.h"
#include "sprite.h"
#include "m2c_macros.h"
#include "lobby_menu.h"
#include "lobby_globals.h"
#include "pad.h"
#include "item.h"
#include "save.h"
#include "scene.h"
#include "box_menu.h"
#include "player.h"
#include "sound.h"
#include "hud.h"
void Save_AddMoney(s32 n);
void Equip_Refresh(SaveData *save);
void Player_CalcSkills(struct Player *pl);
void SkillList_Build(struct Player *p, struct Player *q);
u8 SkillList_PageCount(void);
u8 Equip_PreviewSkills(Player *player, EquipItem *e);
u8 EquipSet_RemoveSlot(u16 slot, s32 check);
void Town_ReloadLocalPlayer(void);
void Sound_PlaySeFlat(u32 a, u32 b, s32 c);
void CamFocus_Start(struct Em *);
s32 StatusPage_HasSymbolColor(void);
void Shake_Start(u8 id, u8 flags, f32 amp);
/* end of prototypes */

/* Points gBoxWork at the item box menu's work, clears its state and sets it to the local
 * player (the HUD's set-up calls it). */
void BoxMenu_InitWork(void)
{
    gBoxWork = &gBoxWorkMem;
    gBoxWork->steps = 0;
    gBoxWork->menuKind = 0;
    gBoxWork->player = &gPlayers[gScene.localPlayer];
}

/* Opens the item box menu: clears its cursors and answers, shows help page 5 (the item
 * box's lines), starts the camera's focus and the menu window's slide-in (shake id 2),
 * counts the equipment set menu's entries, recomputes the player's skills and plays
 * SE_SYS_OPEN. Returns 1. */
s32 BoxMenu_Open(void)
{
    gBoxWork->steps = 0;
    gBoxWork->menuKind = 0;
    *(s32 *)gBoxWork->slots = 0;
    gBoxWork->pouch = 0;
    gBoxWork->yesNo = BOX_YESNO_NONE;
    gBoxWork->help = BOX_HELP_OFF;
    gBoxWork->blink = BOX_BLINK_RESTART;
    gBoxWork->skillPage = 0;
    gHelpPage = 5;
    gHelpShow = 0;
    gHelpLine = 0;
    gHelpColumn = 0;
    CamFocus_Start(0);
    gBoxWork->inputOn = 1;
    gBoxWork->setEntries = (StatusPage_HasSymbolColor() != 1) ? 4 : 5;
    Player_CalcSkills((struct Player *)gBoxWork->player);
    Shake_Start(2, SHAKE_AXIS_X, -200.0f);
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
    return 1;
}

s16 Box_GetPageCount(SaveData *save);
s16 Box_WrapIndex(SaveData *save, s16 idx);
void BoxMenu_MoveCursor(u16 *cur, u16 pad);
void Equip_SetEquipped(EquipIdx *e);
u8 BoxMenu_CloseStep(u16 pad, u8 step);
void BoxMenu_UpdateSkillPreview(void);
u8 EquipSet_DropUnfit(EquipIdx *out, EquipIdx *set, s32 flags);
s32 ItemBox_SlotHasItem(u16 slot);
u16 BoxMenu_SortPrompt(u16 pad, u8 kind);
s32 EquipBox_TryRemove(u16 slot);
static s32 BoxMenu_CanSplit(void);
s32 BoxMenu_CanTakeSeveral(void);
void BoxMenu_InputAmount(u16 pad, u8 noLimit);
void Equip_SwapSlots(u16 a, u16 b);
s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
void Chat_SetColor(s16 color);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Shake_Update(u8 id);
void CamFocus_End(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
u16 BoxMenu_StoreFromPouch(u16 pad);
u16 BoxMenu_TakeToPouch(u16 pad);
u16 BoxMenu_EquipMenu(u16 pad);
u16 BoxMenu_ArrangeMenu(u16 pad);
u16 BoxMenu_SellMenu(u16 pad);
extern u16 gChatRecipients;

/* The item box menu's update, with the frame's pad. PAD_START switches the menu's input off
 * and on (inputOn). State 0, the menu list (store, take, equipment, arrange, sell):
 * PAD_CROSS slides the window out (state 2); PAD_CIRCLE clears the sub-menu's work, opens
 * the entry (the store slides the pouch window in) and falls through, so the sub-menu runs
 * the same frame with no pad. State 1 runs the sub-menu, which returns the pad bits it
 * leaves: PAD_CROSS once it has closed, back to the list. State 2 waits for the slide-out.
 * Returns 0 once closed, else 1; `updated` is 1 only after a full update (BoxMenu_Draw tests
 * it). */
s32 BoxMenu_Update(u16 pad)
{
    u16 res;
    s32 buttons;

    gBoxWork->updated = 0;
    Chat_SetColor(-1);
    if (pad & PAD_START) {
        gBoxWork->inputOn ^= 1;
    }
    if (!(gBoxWork->inputOn & 1)) {
        pad = 0;
    }
    res = pad;
    switch (gBoxWork->state) {
    case BOX_STATE_LIST:
        Chat_SetWindowMode(1, 2);
        if (Shake_Update(2) == 1) {
            pad &= PAD_DPAD | PAD_CROSS;
        }
        buttons = pad;
        gHelpShow = 0;
        if (buttons & PAD_CROSS) {
            Shake_Start(2, SHAKE_SLIDE, -200.0f);
            CamFocus_End();
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            gBoxWork->state = BOX_STATE_CLOSING;
            break;
        }
        Menu_CursorUpDown(&gBoxWork->menu, pad, BOX_MENU_COUNT);
        if (!(buttons & PAD_CIRCLE)) {
            break;
        }
        pad = 0;
        gBoxWork->pick = 0;
        gBoxWork->kind = BOX_KIND_ITEMS;
        *(s32 *)gBoxWork->slots = gBoxWork->pouch = 0;
        gBoxWork->help = BOX_HELP_OFF;
        gBoxWork->levelSel = 0;
        gBoxWork->msg = -1;
        gBoxWork->prompt = 0;
        gBoxWork->cursorClass = BOX_CLASS_ARMOR;
        gBoxWork->set = 0;
        gBoxWork->part = 0;
        gBoxWork->state++;
        gBoxWork->step = 0;
        gHelpColumn = 0;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        switch (gBoxWork->menu) {
        case BOX_MENU_STORE:
            Shake_Start(SHAKE_POUCH_MENU, SHAKE_AXIS_X, 180.0f);
        }
    case BOX_STATE_SUB:
        Chat_SetWindowMode(2, 2);
        gHelpShow = 1; /* shows the help line */
        switch (gBoxWork->menu) {
        case BOX_MENU_STORE:
            res = BoxMenu_StoreFromPouch(pad);
            break;
        case BOX_MENU_TAKE:
            res = BoxMenu_TakeToPouch(pad);
            break;
        case BOX_MENU_EQUIP:
            res = BoxMenu_EquipMenu(pad);
            break;
        case BOX_MENU_ARRANGE:
            res = BoxMenu_ArrangeMenu(pad);
            break;
        case BOX_MENU_SELL:
            res = BoxMenu_SellMenu(pad);
            break;
        }
        if (res & PAD_CROSS) {
            Chat_SetColor(-1);
            gHelpShow = 0;
            gBoxWork->state = BOX_STATE_LIST;
        }
        break;
    case BOX_STATE_CLOSING:
        if (Shake_Update(2) == 0) {
            return 0;
        }
        break;
    }
    gBoxWork->updated = 1;
    return 1;
}

s16 Box_GetPageCount(SaveData *save);
s16 Box_WrapIndex(SaveData *save, s16 idx);

/* Moves an item box cursor, a slot index (page * 100 + row * 10 + column), by the pad:
 * PAD_R2 and PAD_L2 turn the page forward and back when the box has more than one
 * (Box_GetPageCount), the D-pad moves in the page's 10 by 10 grid, wrapping at its edges.
 * Plays SE_SYS_SCROLL when the cursor moved, SE_SYS_OPEN when the page turned. */
void BoxMenu_MoveCursor(u16 *cur, u16 pad)
{
    s16 idx;
    s16 page;
    u8 paged;
    s16 pos;
    s16 pageCount;
    u8 col;
    u8 row;

    idx = Box_WrapIndex(&gSave, *cur);
    paged = 0;
    page = idx / BOX_PAGE_SLOTS;
    pos = idx % BOX_PAGE_SLOTS;
    pageCount = Box_GetPageCount(&gSave);
    if (pageCount > 1 && (pad & (PAD_L2 | PAD_R2))) {
        paged = 1;
        if (pad & PAD_R2) {
            page++;
            if (page >= pageCount) {
                page = 0;
            }
        } else if (page == 0) {
            page = pageCount - 1;
        } else {
            page--;
        }
    }
    col = pos % 10;
    row = pos / 10;
    if (pad & PAD_LEFT) {
        if (col == 0) {
            col = 9;
        } else {
            col--;
        }
    }
    if (pad & PAD_RIGHT) {
        if (col >= 9) {
            col = 0;
        } else {
            col++;
        }
    }
    if (pad & PAD_UP) {
        if (row == 0) {
            row = 9;
        } else {
            row--;
        }
    }
    if (pad & PAD_DOWN) {
        if (row >= 9) {
            row = 0;
        } else {
            row++;
        }
    }
    idx = col % 10 + page * BOX_PAGE_SLOTS + row % 10 * 10;
    if (*cur != idx) {
        *cur = idx;
        if (paged == 0) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
        } else {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
        }
    }
}

u8 YesNo_Toggle(u16 pad, u8 cur, u16 mask);
void Pouch_Compact(u8 box);
s32 ItemBox_Fits(u16 item);
void Pouch_MoveCursor(u8 *cur, u16 buttons);
void PouchMenu_DecoCursor(u16 item, u16 pad);
u32 ItemBox_Add(u16 item, u32 count);
s16 Box_GetCapacity(SaveData *save);

/* The store sub-menu (pouch to item box), with the frame's pad; returns the pad bits it
 * leaves, PAD_CROSS once it has closed.
 * Step 0, the pouch cursor: PAD_SELECT asks whether to sort the pouch (prompt: PAD_LEFT and
 *   PAD_RIGHT pick, yes runs Pouch_Compact; PAD_CROSS moves to no, then closes it),
 *   PAD_SQUARE shows or hides the item's description, PAD_CIRCLE stores the whole stack
 *   (ItemBox_Add: what the box can't take stays in the pouch; refused when none fits) and
 *   goes to step 1, PAD_CROSS slides the pouch window out (step 2).
 * Step 1: the cursor blinks on the box slot that took the item (PAD_L2/PAD_R2 turn the page)
 *   until PAD_CIRCLE.
 * Step 2: waits for the slide-out. */
u16 BoxMenu_StoreFromPouch(u16 pad)
{
    SaveData *save = &gSave;
    u16 cannotStore;
    s32 buttons;
    ItemBoxMenu *menu;
    u8 *field;
    s32 left;
    ItemStack *stack;
    u32 i;
    s16 capacity;
    s32 keys;

    switch (gBoxWork->step) {
    case BOX_STORE_PICK:
        if (Shake_Update(SHAKE_POUCH_MENU) == 1) {
            gHelpShow = 0;
            pad &= PAD_DPAD | PAD_CROSS;
        }
        gHelpLine = 0x38;
        field = &gBoxWork->prompt;
        if (*field != 0) {
            keys = pad;
            if (keys & PAD_CROSS) {
                pad = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                field = &gBoxWork->yesNo;
                if (*field != BOX_YES) {
                    *field = BOX_YESNO_NONE;
                    /* Straight to the prompt's close, past the answer's handling: with an
                     * if/else instead, the compiler orders the branches differently. */
                    goto close;
                }
                *field = BOX_NO;
                break;
            } else if (!(keys & PAD_CIRCLE)) {
                gBoxWork->yesNo = YesNo_Toggle(pad, gBoxWork->yesNo, PAD_LEFT | PAD_RIGHT);
                break;
            }
            pad = 0;
            if (gBoxWork->yesNo == BOX_YES) {
                Pouch_Compact(1);
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                gBoxWork->pouch = gBoxWork->pouch % 10;
            } else {
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            }
            gBoxWork->yesNo = BOX_YESNO_NONE;
        close:
            gBoxWork->prompt = 0;
        } else if (pad & PAD_SELECT) {
            *field = 1;
            pad = 0;
            gBoxWork->yesNo = BOX_YES;
            gBoxWork->help = BOX_HELP_OFF;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, 0xF, 0);
            break;
        }
        if (pad & PAD_CROSS) {
            Shake_Start(SHAKE_POUCH_MENU, SHAKE_SLIDE, 180.0f);
            pad = 0;
            gBoxWork->step = BOX_STORE_CLOSING;
            gHelpShow = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        }
        gHelpLine = 0;
        cannotStore = 0;
        if (save->pouch[gBoxWork->pouch].item != 0) {
            if (ItemBox_Fits(save->pouch[gBoxWork->pouch].item) == 0) {
                cannotStore = 1;
                gHelpLine = 2;
            }
        } else {
            cannotStore = 1;
        }
        buttons = pad;
        if (buttons & PAD_SQUARE) {
            field = &gBoxWork->help;
            if (*field != 0) {
                *field = 0;
            } else {
                *field = 1;
                gHelpColumn = 0;
            }
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
        }
        Pouch_MoveCursor(&gBoxWork->pouch, pad);
        gBoxWork->pouchItem = save->pouch[gBoxWork->pouch].item;
        if (gBoxWork->help != BOX_HELP_OFF) {
            PouchMenu_DecoCursor(gBoxWork->pouchItem, pad);
        }
        if (buttons & PAD_CIRCLE) {
            if (cannotStore == 0) {
                left = ItemBox_Add(gBoxWork->pouchItem, save->pouch[gBoxWork->pouch].count);
                if (left == 0) {
                    save->pouch[gBoxWork->pouch].count = 0;
                    save->pouch[gBoxWork->pouch].item = 0;
                    gBoxWork->msg = 1;
                    Sound_PlaySeFlat(SE_SLOT_COMMON, 0x88, 0);
                } else {
                    save->pouch[gBoxWork->pouch].count = left;
                    gBoxWork->msg = 2;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                }
                menu = gBoxWork;
                gHelpLine = (s8)menu->msg;
                menu->help = BOX_HELP_OFF;
                gBoxWork->step++;
                gBoxWork->slots[0] = 0;
                capacity = Box_GetCapacity(save);
                for (i = 0, stack = save->itemBox; i < capacity; i++, stack++) {
                    if (stack->item == gBoxWork->pouchItem) {
                        gBoxWork->slots[0] = i;
                        break;
                    }
                }
            } else {
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
            }
        }
        break;
    case BOX_STORE_SHOW:
        buttons = pad;
        BoxMenu_MoveCursor(&gBoxWork->slots[0], buttons & (PAD_L2 | PAD_R2));
        menu = gBoxWork;
        gHelpLine = (s8)menu->msg;
        menu->blink++;
        if (buttons & PAD_CIRCLE) {
            gBoxWork->step = BOX_STORE_PICK;
            gBoxWork->help = BOX_HELP_OFF;
            gBoxWork->blink = BOX_BLINK_RESTART;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
        }
        pad = 0;
        break;
    case BOX_STORE_CLOSING:
        if (Shake_Update(SHAKE_POUCH_MENU) == 0) {
            return 0x40;
        }
        gHelpShow = 0;
        pad = 0;
        break;
    }
    return pad;
}

s32 Item_IsOnePerPouch(u16 item);
s32 SavePouch_Room(u16 item);

/* The item box menu's work as BoxMenu_TakeToPouch reads it, through TV: the same bytes as
 * ItemBoxMenu (step, sub, slots[0] as slot, pouch, atMax, amount, msg). Kept for the match:
 * through ItemBoxMenu the compiler reuses one load of gBoxWork, where the game's code loads it
 * again at each use. */
typedef struct TakeView {
    /* 0x00 */ u8 unk0[5];
    /* 0x05 */ u8 step;
    /* 0x06 */ u8 sub;
    /* 0x07 */ u8 unk7[0x15];
    /* 0x1C */ u16 slot;
    /* 0x1E */ u8 unk1E[4];
    /* 0x22 */ u8 pouch;
    /* 0x23 */ u8 atMax;
    /* 0x24 */ s16 amount;
    /* 0x26 */ u8 unk26[7];
    /* 0x2D */ s8 msg;
} TakeView;
#define TV ((TakeView *)gBoxWork)

/* The take sub-menu (item box to pouch), with the frame's pad; returns the pad bits it
 * leaves (PAD_CROSS at step 0 leaves the sub-menu).
 * Step 0, the box cursor (BoxMenu_SortPrompt first; PAD_SQUARE shows the description):
 *   PAD_CIRCLE on an item looks for its pouch slot, the one holding the same item (for an
 *   item the pouch holds once, Item_IsOnePerPouch, any slot of such an item). Found: refused
 *   when the pouch can't take more (help line 6); one moves when only one can
 *   (BoxMenu_CanTakeSeveral); else the amount is asked (step 2). Not found: step 1, the
 *   cursor on the first empty pouch slot.
 * Step 1, the pouch cursor: an empty slot takes the whole stack, or asks the amount when only
 *   part can move (BoxMenu_CanSplit); a filled slot asks whether to swap (step 3).
 * Step 2, the amount (BoxMenu_InputAmount): PAD_CIRCLE moves it, PAD_CROSS goes back to the
 *   step it came from (sub) and its help line (msg).
 * Step 3, the swap's yes/no: yes puts the box stack in the pouch slot and the slot's stack
 *   in the box (ItemBox_Add).
 * Step 4: shows the result's help line, the cursor blinking, until PAD_CIRCLE, then step 0. */
u16 BoxMenu_TakeToPouch(u16 pad)
{
    SaveData *save = &gSave;
    u8 *field;
    TakeView *menu = TV;
    s32 buttons;
    s32 unused;
    u32 i;
    ItemStack *pouchStack;
    ItemStack saved;

    switch (menu->step) {
    case BOX_TAKE_PICK:
        pad = BoxMenu_SortPrompt(pad, 0);
        menu = TV;
        if (gBoxWork->prompt != 0) {
            break;
        }
        buttons = pad;
        if (buttons & PAD_CROSS) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        gHelpLine = 3;
        if (buttons & PAD_SQUARE) {
            field = &((ItemBoxMenu *)menu)->help;
            if (*field != 0) {
                *field = 0;
            } else {
                *field = 1;
                gHelpColumn = 0;
            }
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
        }
        BoxMenu_MoveCursor(&TV->slot, pad);
        if (gBoxWork->help != BOX_HELP_OFF) {
            PouchMenu_DecoCursor(save->itemBox[TV->slot].item, pad);
        }
        if (!(buttons & PAD_CIRCLE)) {
            break;
        }
        if (ItemBox_SlotHasItem(TV->slot) == 0) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
            break;
        }
        gBoxWork->help = BOX_HELP_OFF;
        gHelpColumn = 0;
        if (Item_IsOnePerPouch(save->itemBox[TV->slot].item) != 1) {
            TV->pouch = 0;
          { u8 *fieldPtr;
            for (menu = TV, fieldPtr = &menu->pouch; menu->pouch < POUCH_SLOTS; menu = TV, fieldPtr = &menu->pouch) {
                if (save->pouch[menu->pouch].item == save->itemBox[menu->slot].item) {
                    break;
                }
                (*fieldPtr)++;
            } }
        } else {
            TV->pouch = 0;
            for (menu = TV; menu->pouch < POUCH_SLOTS; menu = TV) {
                if (Item_IsOnePerPouch(save->pouch[menu->pouch].item) == 1) {
                    break;
                }
                TV->pouch++;
            }
        }
        field = &TV->pouch;
        if (*field < POUCH_SLOTS) {
            if (gItemData[save->pouch[TV->pouch].item].max == ITEM_MAX_UNLIMITED || SavePouch_Room(save->itemBox[TV->slot].item) <= 0) {
                TV->msg = 6;
                gHelpLine = 6;
                TV->step = BOX_TAKE_RESULT;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
            } else if (BoxMenu_CanTakeSeveral() == 0) {
                save->pouch[TV->pouch].count++;
                save->itemBox[TV->slot].count--;
                if (save->itemBox[TV->slot].count == 0) {
                    save->itemBox[TV->slot].item = 0;
                }
                TV->msg = 7;
                gHelpLine = 7;
                TV->step = BOX_TAKE_RESULT;
                Sound_PlaySeFlat(SE_SLOT_COMMON, 0x88, 0);
            } else {
                gHelpLine = 5;
                TV->amount = 1;
                TV->atMax = 0;
                TV->step = BOX_TAKE_AMOUNT;
                TV->sub = 0;
                TV->msg = 3;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
            }
            break;
        }
        *field = 0;
        pad = 0;
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (save->pouch[i].item == 0) {
                TV->pouch = i;
                break;
            }
        }
        TV->step = BOX_TAKE_SLOT;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
    case BOX_TAKE_SLOT:
      { u8 *fieldPtr; s32 keys; TakeView *view;
        view = TV;
        fieldPtr = &((ItemBoxMenu *)view)->help;
        if (*fieldPtr != 0) {
            if (pad & (PAD_SQUARE | PAD_CROSS)) {
                *fieldPtr = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            }
            pad &= ~PAD_CROSS;
        } else {
            keys = pad;
            if (keys & PAD_CROSS) {
                gHelpColumn = 0;
                gHelpLine = 3;
                view->step = BOX_TAKE_PICK;
                pad = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                break;
            }
            if (keys & PAD_SQUARE) {
                *fieldPtr = 1;
                gHelpColumn = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
            }
        }
      }
        gHelpLine = 4;
        Pouch_MoveCursor(&TV->pouch, pad);
        if (gBoxWork->help != BOX_HELP_OFF) {
            PouchMenu_DecoCursor(save->pouch[TV->pouch].item, pad);
        }
        if (!(pad & PAD_CIRCLE)) {
            break;
        }
        gBoxWork->help = BOX_HELP_OFF;
        menu = TV;
        if (save->pouch[menu->pouch].item != 0) {
            gHelpLine = 8;
            menu->step = BOX_TAKE_SWAP;
            gBoxWork->yesNo = BOX_YES;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
            break;
        }
        if (BoxMenu_CanSplit() == 0) {
            save->pouch[TV->pouch].item = save->itemBox[TV->slot].item;
            save->pouch[TV->pouch].count = save->itemBox[TV->slot].count;
            save->itemBox[TV->slot].count = 0;
            save->itemBox[TV->slot].item = 0;
            TV->msg = 7;
            gHelpLine = 7;
            TV->step = BOX_TAKE_RESULT;
            Sound_PlaySeFlat(SE_SLOT_COMMON, 0x88, 0);
            break;
        }
        pad = 0;
        save->pouch[TV->pouch].count = 0;
        TV->amount = 1;
        TV->atMax = 0;
        TV->step = BOX_TAKE_AMOUNT;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        TV->sub = 1;
        TV->msg = 4;
    case BOX_TAKE_AMOUNT:
        buttons = pad;
        if (buttons & PAD_CROSS) {
            pad &= ~PAD_CROSS;
            menu = TV;
            gHelpLine = menu->msg;
            menu->step = menu->sub;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        gHelpLine = 5;
        BoxMenu_InputAmount(pad, TV->sub);
        if (!(buttons & PAD_CIRCLE)) {
            break;
        }
        save->pouch[TV->pouch].item = save->itemBox[TV->slot].item;
        save->pouch[TV->pouch].count += TV->amount;
        save->itemBox[TV->slot].count -= TV->amount;
        if (save->itemBox[TV->slot].count == 0) {
            save->itemBox[TV->slot].item = 0;
        }
        TV->msg = 7;
        gHelpLine = 7;
        TV->step = BOX_TAKE_RESULT;
        Sound_PlaySeFlat(SE_SLOT_COMMON, 0x88, 0);
        break;
    case BOX_TAKE_SWAP:
        buttons = pad;
        if (buttons & PAD_CROSS) {
            gHelpLine = 4;
            ((ItemBoxMenu *)menu)->yesNo = BOX_YESNO_NONE;
            TV->step = BOX_TAKE_SLOT;
            pad &= ~PAD_CROSS;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        gHelpLine = 8;
        gBoxWork->yesNo = YesNo_Toggle(pad, ((ItemBoxMenu *)menu)->yesNo, PAD_LEFT | PAD_RIGHT);
        if (!(buttons & PAD_CIRCLE)) {
            break;
        }
        pad = 0;
        field = &gBoxWork->yesNo;
        if (*field != 0) {
            gHelpLine = 4;
            *field = 0xFF;
            TV->step = BOX_TAKE_SLOT;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        menu = TV;
        /* The pouch slot by its byte offset: with an array index the compiler orders
         * the adds differently from the game's code. */
        pouchStack = (ItemStack *)((u8 *)save->pouch + menu->pouch * 4);
        saved = *pouchStack;
        *pouchStack = save->itemBox[menu->slot];
        save->itemBox[TV->slot].item = 0;
        save->itemBox[TV->slot].count = 0;
        ItemBox_Add(saved.item, saved.count);
        TV->msg = 7;
        gBoxWork->yesNo = BOX_YESNO_NONE;
        TV->step = BOX_TAKE_RESULT;
        Sound_PlaySeFlat(SE_SLOT_COMMON, 0x88, 0);
    case BOX_TAKE_RESULT:
        menu = TV;
        gHelpLine = menu->msg;
        ((ItemBoxMenu *)menu)->blink++;
        if (pad & PAD_CIRCLE) {
            TV->step = BOX_TAKE_PICK;
            gBoxWork->blink = BOX_BLINK_RESTART;
            gHelpColumn = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
        }
        pad = 0;
        break;
    }
    return pad;
}


s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);
s32 Equip_UnequipType(SaveData *save, s32 type);
s32 Equip_EquipSlot(SaveData *save, u16 idx);
s32 Box_YesNoInput(u16 *pad);
s8 BoxList_Build(u8 kind, u8 sorted);
u8 BoxMenu_ListStyles(u8 *out);
void Save_SetSymbolColor(u8 a);
void Player_SetColorWord(struct Player *pl, u8 idx);
void EquipSet_RecalcStats(EquipIdx *in, s32 b);
s32 EquipSet_RenameInput(u16 pad);
u16 BoxMenu_EquipListInput(u16 *sel, u16 pad, s32 *ok, s32 flags);
u16 BoxMenu_EquipPickInput(u16 *set, u16 pad, s32 *ok, s32 flags);
extern u8 gBoxPartIdx[];
extern s8 gLobbyTextDirty;

/*
 * The item box's equipment menu (BoxMenu_Update's entry 2), run once a frame with the pad's
 * new buttons. Step 0 is its list of entries (gBoxWork->kind: 4, or 5 once a symbol colour is
 * earned); PAD_CIRCLE opens one (step 1); PAD_CROSS is handed back, which leaves the menu.
 * - 0: put on, swap or take off single pieces (BoxMenu_EquipPickInput), after a yes/no.
 * - 1: edit an equipment set (SaveData.equipSets[gBoxWork->set]): the set is copied to
 *   gBoxWork->cur; each row (gBoxWork->part: weapon, head, body, arms, waist, legs) opens the
 *   box's pieces of that part (BoxMenu_EquipListInput), a set without a weapon opens on the
 *   weapon list, and the last row saves the set, after a yes/no when armour unfit for its weapon
 *   is dropped (EquipSet_DropUnfit). The saved set may then be named (EquipSet_RenameInput)
 *   and worn.
 * - 2: wear a set, after a yes/no (Equip_SetEquipped); the skill list compares the set with
 *   what the hunter wears.
 * - 3: save the worn equipment into a set, after a yes/no; it may then be named.
 * - 4: pick the symbol colour among those earned (BoxMenu_ListStyles); PAD_SQUARE shows or
 *   hides the colour's description.
 * On the set screens PAD_LEFT/PAD_RIGHT pick the set (EquipSet_RecalcStats rebuilds the
 * preview), PAD_SQUARE cycles the three info pages (gBoxWork->levelSel), and on the skills page
 * L1/R1 turn its pages. A change of equipment ends with BoxMenu_CloseStep, which reloads the
 * hunter. Returns pad, with the buttons the menu used cleared.
 */
u16 BoxMenu_EquipMenu(u16 pad)
{
    SaveData *save = &gSave;
    EquipSet *sets;
    EquipSet *setAtCursor;
    EquipSet *cur;
    s32 ok;
    s8 answer;
    u8 prevCursor;
    u16 boxSlot;
    u8 *editStep;
    u8 *setCursor;

    switch (gBoxWork->step) {
    case 0:
        if (pad & PAD_CROSS) {
            gHelpShow = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        Menu_CursorUpDown(&gBoxWork->kind, pad, gBoxWork->setEntries);
        gHelpShow = 1;
        gHelpLine = gBoxWork->kind + 0x33;
        if (pad & PAD_CIRCLE) {
            pad = 0;
            gBoxWork->help = BOX_HELP_OFF;
            gBoxWork->levelSel = 0;
            gBoxWork->msg = -1;
            gBoxWork->prompt = 0;
            gBoxWork->set = 0;
            /* SaveData.equipSets, reached by its offset: the compiler then orders the
             * adds as the game's code does. */
            setAtCursor = (EquipSet *)((u8 *)save + 0x25E4) + gBoxWork->set;
            switch (gBoxWork->kind) {
            case EQUIP_MENU_PIECES:
                BoxMenu_EquipPickInput(save->equipIdx, 0, &ok, 0);
                break;
            case EQUIP_MENU_WEAR_SET:
                EquipSet_RecalcStats((EquipIdx *)setAtCursor, (s32)gBoxWork->player);
                gHelpLine = 0x1F;
                break;
            case EQUIP_MENU_EDIT_SET:
                EquipSet_RecalcStats((EquipIdx *)setAtCursor, 0);
                gHelpLine = 0x20;
                break;
            case EQUIP_MENU_SAVE_SET:
                EquipSet_RecalcStats((EquipIdx *)setAtCursor, 0);
                gHelpLine = 0x28;
                break;
            case EQUIP_MENU_SYMBOL_COLOR:
                gHelpLine = 0x32;
                gBoxWork->help = BOX_HELP_OFF;
                gBoxWork->count = BoxMenu_ListStyles(gBoxWork->list);
                break;
            }
            gBoxWork->step++;
            gBoxWork->sub = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        }
        break;
    case 1:
        switch (gBoxWork->kind) {
        case EQUIP_MENU_PIECES:
            Chat_SetColor(0);
            switch (gBoxWork->sub) {
            case 0:
                pad = BoxMenu_EquipPickInput(save->equipIdx, pad, &ok, 0);
                if (pad & PAD_CROSS) {
                    gHelpLine = 0x33;
                    pad = 0;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                    break;
                }
                if (!(pad & PAD_CIRCLE)) {
                    break;
                }
                pad = 0;
                if (ok != 1) {
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                    break;
                }
                gBoxWork->sub++;
                gBoxWork->help = BOX_HELP_OFF;
                gBoxWork->yesNo = BOX_YES;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                /* falls through: the yes/no starts this frame */
            case 1:
                answer = Box_YesNoInput(&pad);
                if (answer < 0) {
                    gHelpLine = 0xA;
                    pad = 0;
                    gBoxWork->sub = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                    break;
                }
                if (gBoxWork->boxItem == NULL) {
                    gHelpLine = 0x10;
                } else if (gBoxWork->equipped == NULL) {
                    gHelpLine = 0x11;
                } else if (gBoxWork->cursorClass != BOX_CLASS_OTHER_WEAPON) {
                    gHelpLine = 0xF;
                } else {
                    gHelpLine = 0xB;
                }
                Menu_CursorLeftRight(&gBoxWork->levelSel, pad & (PAD_L1 | PAD_R1), gBoxWork->levelCount);
                if (answer > 0) {
                    if (gBoxWork->boxItem == NULL) {
                        ok = Equip_UnequipType(save, gBoxWork->equipped->type);
                    } else {
                        ok = Equip_EquipSlot(save, gBoxWork->slots[1]);
                    }
                    if (ok == 1) {
                        gHelpLine = 0x12;
                        gBoxWork->levelSel = 0;
                        gBoxWork->sub++;
                        gBoxWork->close = 0;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                    }
                }
                break;
            case 2:
                gHelpLine = 0x12;
                gBoxWork->close = BoxMenu_CloseStep(pad, gBoxWork->close);
                if (gBoxWork->close == 0) {
                    BoxMenu_EquipPickInput(save->equipIdx, 0, &ok, 0);
                    gBoxWork->sub = 0;
                }
                pad = 0;
                break;
            }
            break;
        case EQUIP_MENU_WEAR_SET:
            sets = save->equipSets;
            switch (gBoxWork->sub) {
            case 0:
                gHelpShow = 1;
                if (pad & PAD_CROSS) {
                    pad = 0;
                    gBoxWork->levelSel = 0;
                    gHelpLine = 0x35;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                    break;
                }
                setAtCursor = &sets[gBoxWork->set];
                if (pad & PAD_CIRCLE) {
                    pad = 0;
                    if (setAtCursor->idx[SAVE_EQUIP_WEAPON] == BOX_NONE) {
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                    } else {
                        gBoxWork->levelSel = 0;
                        gBoxWork->yesNo = BOX_YES;
                        gBoxWork->sub++;
                        gHelpLine = 0x22;
                        gBoxWork->levelSel = 0;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                        break;
                    }
                }
                gHelpLine = 0x1F;
                prevCursor = gBoxWork->set;
                Menu_CursorLeftRight(&gBoxWork->set, pad & (PAD_LEFT | PAD_RIGHT), EQUIP_SET_COUNT);
                setAtCursor = &sets[gBoxWork->set];
                if (prevCursor != gBoxWork->set) {
                    EquipSet_RecalcStats((EquipIdx *)setAtCursor, (s32)gBoxWork->player);
                    pad = 0;
                }
                if (pad & PAD_SQUARE) {
                    gBoxWork->levelSel++;
                    if (gBoxWork->levelSel > 2) {
                        gBoxWork->levelSel = 0;
                    }
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
                    pad = 0;
                }
                if (gBoxWork->levelSel == 2) {
                    Menu_CursorLeftRight(&gBoxWork->skillPage, pad & (PAD_L1 | PAD_R1), gBoxWork->skillPages);
                    gHelpShow = 0;
                }
                break;
            case 1:
                setAtCursor = &sets[gBoxWork->set];
                answer = Box_YesNoInput(&pad);
                gHelpLine = 0x22;
                if (answer > 0) {
                    Equip_SetEquipped((EquipIdx *)setAtCursor);
                    gHelpLine = 0x12;
                    gBoxWork->sub++;
                    gBoxWork->close = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                } else if (answer < 0) {
                    gHelpLine = 0x1F;
                    gBoxWork->sub = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                }
                break;
            case 2:
                gBoxWork->close = BoxMenu_CloseStep(pad, gBoxWork->close);
                if (gBoxWork->close == 0) {
                    EquipSet_RecalcStats((EquipIdx *)&sets[gBoxWork->set], (s32)gBoxWork->player);
                    gHelpLine = 0x1F;
                    gBoxWork->sub = 0;
                }
                pad = 0;
                break;
            }
            break;
        case EQUIP_MENU_EDIT_SET:
            sets = save->equipSets;
            switch (gBoxWork->sub) {
            case 0:
                gHelpShow = 1;
                if (pad & PAD_CROSS) {
                    pad = 0;
                    gBoxWork->levelSel = 0;
                    gHelpLine = 0x34;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                } else if (pad & PAD_CIRCLE) {
                    gBoxWork->sub++;
                    gBoxWork->help = BOX_HELP_OFF;
                    gBoxWork->levelSel = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    gBoxWork->cur = sets[gBoxWork->set];
                    gBoxWork->part = 0;
                    gBoxWork->pageCount = BoxList_Build(gBoxWork->part, 1);
                    gBoxWork->page = 0;
                    gBoxWork->row = 0;
                    if (gBoxWork->cur.idx[SAVE_EQUIP_WEAPON] != BOX_NONE) {
                        gBoxWork->newSet = 0;
                        gHelpLine = 0x2B;
                        gBoxWork->sub2 = 0;
                    } else {
                        gBoxWork->newSet = 1;
                        gHelpLine = 0x24;
                        gBoxWork->sub2 = 1;
                        gBoxWork->equipped = NULL;
                        BoxMenu_EquipListInput(sets[gBoxWork->set].idx, 0, &ok, 0);
                        EquipSet_RecalcStats((EquipIdx *)&gBoxWork->cur, 0);
                    }
                } else {
                    gHelpLine = 0x20;
                    prevCursor = gBoxWork->set;
                    Menu_CursorLeftRight(&gBoxWork->set, pad & (PAD_LEFT | PAD_RIGHT), EQUIP_SET_COUNT);
                    setAtCursor = &save->equipSets[gBoxWork->set];
                    if (prevCursor != gBoxWork->set) {
                        EquipSet_RecalcStats((EquipIdx *)setAtCursor, 0);
                        pad = 0;
                    }
                    if (pad & PAD_SQUARE) {
                        gBoxWork->levelSel++;
                        if (gBoxWork->levelSel > 2) {
                            gBoxWork->levelSel = 0;
                        }
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
                        pad = 0;
                    }
                    if (gBoxWork->levelSel == 2) {
                        Menu_CursorLeftRight(&gBoxWork->skillPage, pad & (PAD_L1 | PAD_R1), gBoxWork->skillPages);
                        gHelpShow = 0;
                    }
                }
                break;
            case 1:
                Chat_SetColor(0);
                cur = &gBoxWork->cur;
                EquipSet_RecalcStats((EquipIdx *)cur, 0);
                editStep = &gBoxWork->sub2;
                switch (*editStep) {
                case 0:
                    if (pad & PAD_CROSS) {
                        EquipSet_RecalcStats((EquipIdx *)&save->equipSets[gBoxWork->set], 0);
                        gHelpLine = 0x20;
                        gBoxWork->sub = 0;
                        gBoxWork->levelSel = 0;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                        pad = 0;
                        break;
                    }
                    gHelpShow = 1;
                    if (gBoxWork->part != BOX_PART_SAVE) {
                        gHelpLine = 0x2B;
                    } else {
                        gHelpLine = 0x2D;
                    }
                    if (pad & PAD_CIRCLE) {
                        pad = 0;
                        gBoxWork->levelSel = 0;
                        if (gBoxWork->part == BOX_PART_SAVE) {
                            if (EquipSet_DropUnfit((EquipIdx *)&save->equipSets[gBoxWork->set], (EquipIdx *)&gBoxWork->cur, 1) == 0) {
                                EquipSet_DropUnfit((EquipIdx *)&save->equipSets[gBoxWork->set], (EquipIdx *)&gBoxWork->cur, 0);
                                EquipSet_RecalcStats((EquipIdx *)&save->equipSets[gBoxWork->set], 0);
                                gBoxWork->sub++;
                                gHelpLine = 0x26;
                                gBoxWork->yesNo = BOX_YES;
                                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DONE, 0);
                            } else {
                                gBoxWork->sub2 = 2;
                                gHelpLine = 0x2C;
                                gBoxWork->yesNo = BOX_YES;
                                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                            }
                        } else {
                            gBoxWork->pageCount = BoxList_Build(gBoxWork->part, 1);
                            if (gBoxWork->pageCount == 0) {
                                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                            } else {
                                gBoxWork->sub2++;
                                gHelpLine = 0x23;
                                gBoxWork->help = BOX_HELP_OFF;
                                gBoxWork->row = 0;
                                gBoxWork->page = 0;
                                boxSlot = cur->idx[gBoxPartIdx[gBoxWork->part]];
                                gBoxWork->equipped = boxSlot != BOX_NONE ? &save->box[boxSlot] : NULL;
                                BoxMenu_EquipListInput(cur->idx, 0, &ok, 0);
                                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                            }
                        }
                    } else {
                        if (pad & PAD_SQUARE) {
                            gBoxWork->levelSel++;
                            if (gBoxWork->levelSel > 2) {
                                gBoxWork->levelSel = 0;
                            }
                            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
                            pad = 0;
                        }
                        if (gBoxWork->levelSel < 2) {
                            prevCursor = gBoxWork->part;
                            Menu_CursorUpDown(&gBoxWork->part, pad, BOX_PART_COUNT);
                            if (prevCursor != gBoxWork->part) {
                                gBoxWork->pageCount = BoxList_Build(gBoxWork->part, 1);
                                gBoxWork->row = 0;
                                gBoxWork->page = 0;
                            }
                        } else {
                            Menu_CursorLeftRight(&gBoxWork->skillPage, pad & (PAD_L1 | PAD_R1), gBoxWork->skillPages);
                            gHelpShow = 0;
                        }
                    }
                    break;
                case 1:
                    if (pad & PAD_CROSS) {
                        if (gBoxWork->newSet == 0) {
                            gHelpLine = 0x2B;
                            *editStep = 0;
                        } else {
                            gHelpLine = 0x20;
                            gBoxWork->sub = 0;
                        }
                        gBoxWork->levelSel = 0;
                        pad = 0;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                        break;
                    }
                    pad = BoxMenu_EquipListInput(cur->idx, pad, &ok, 0);
                    if (pad & PAD_CIRCLE) {
                        if (ok == 0) {
                            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                        } else {
                            if (gBoxWork->cursorClass != BOX_CLASS_ARMOR) {
                                cur->idx[SAVE_EQUIP_WEAPON] = gBoxWork->slots[1];
                                gBoxWork->newSet = 0;
                            } else if (gBoxWork->boxItem == gBoxWork->equipped) {
                                switch (gBoxWork->equipped->type) {
                                case EQUIP_LEGS:
                                    cur->idx[SAVE_EQUIP_LEGS] = BOX_NONE;
                                    break;
                                case EQUIP_HEAD:
                                    cur->idx[SAVE_EQUIP_HEAD] = BOX_NONE;
                                    break;
                                case EQUIP_BODY:
                                    cur->idx[SAVE_EQUIP_BODY] = BOX_NONE;
                                    break;
                                case EQUIP_ARMS:
                                    cur->idx[SAVE_EQUIP_ARMS] = BOX_NONE;
                                    break;
                                case EQUIP_WAIST:
                                    cur->idx[SAVE_EQUIP_WAIST] = BOX_NONE;
                                    break;
                                }
                            } else {
                                switch (gBoxWork->boxItem->type) {
                                case EQUIP_LEGS:
                                    cur->idx[SAVE_EQUIP_LEGS] = gBoxWork->slots[1];
                                    break;
                                case EQUIP_HEAD:
                                    cur->idx[SAVE_EQUIP_HEAD] = gBoxWork->slots[1];
                                    break;
                                case EQUIP_BODY:
                                    cur->idx[SAVE_EQUIP_BODY] = gBoxWork->slots[1];
                                    break;
                                case EQUIP_ARMS:
                                    cur->idx[SAVE_EQUIP_ARMS] = gBoxWork->slots[1];
                                    break;
                                case EQUIP_WAIST:
                                    cur->idx[SAVE_EQUIP_WAIST] = gBoxWork->slots[1];
                                    break;
                                }
                            }
                            EquipSet_RecalcStats((EquipIdx *)cur, 0);
                            gHelpLine = 0x2B;
                            gBoxWork->sub2 = 0;
                            gBoxWork->levelSel = 0;
                            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                        }
                    }
                    break;
                case 2:
                    answer = Box_YesNoInput(&pad);
                    if (answer != 0) {
                        if (answer > 0) {
                            EquipSet_DropUnfit((EquipIdx *)&save->equipSets[gBoxWork->set], (EquipIdx *)&gBoxWork->cur, 0);
                            EquipSet_RecalcStats((EquipIdx *)&save->equipSets[gBoxWork->set], 0);
                            gBoxWork->sub++;
                            gHelpLine = 0x26;
                            gBoxWork->yesNo = BOX_YES;
                            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DONE, 0);
                        } else {
                            gBoxWork->sub2 = 0;
                            gHelpLine = 0x2D;
                            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                        }
                    }
                    pad = 0;
                    break;
                }
                break;
            case 2:
                answer = Box_YesNoInput(&pad);
                gHelpLine = 0x26;
                if (answer != 0) {
                    if (answer > 0) {
                        gHelpLine = 0x2F;
                        gBoxWork->sub++;
                        gBoxWork->sub2 = 0;
                        gBoxWork->yesNo = BOX_YESNO_NONE;
                        EquipSet_RenameInput(0);
                    } else {
                        gHelpLine = 0x21;
                        gBoxWork->sub = 4;
                        gBoxWork->yesNo = BOX_YES;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                    }
                }
                pad = 0;
                break;
            case 3:
                if (EquipSet_RenameInput(pad) == 0) {
                    gBoxWork->sub++;
                    gBoxWork->yesNo = BOX_YES;
                    gHelpLine = 0x21;
                }
                pad = 0;
                break;
            case 4:
                answer = Box_YesNoInput(&pad);
                gHelpLine = 0x21;
                if (answer != 0) {
                    if (answer > 0) {
                        Equip_SetEquipped((EquipIdx *)&sets[gBoxWork->set]);
                        gBoxWork->close = 0;
                        gBoxWork->sub++;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    } else {
                        pad = 0;
                        gHelpLine = 0x20;
                        gBoxWork->sub = 0;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                    }
                    gBoxWork->yesNo = BOX_YESNO_NONE;
                }
                pad = 0;
                break;
            case 5:
                gBoxWork->close = BoxMenu_CloseStep(pad, gBoxWork->close);
                if (gBoxWork->close == 0) {
                    gHelpLine = 0x20;
                    gBoxWork->sub = 0;
                }
                pad = 0;
                break;
            }
            break;
        case EQUIP_MENU_SAVE_SET:
            sets = save->equipSets;
            switch (gBoxWork->sub) {
            case 0:
                gHelpShow = 1;
                if (pad & PAD_CROSS) {
                    gHelpLine = 0x36;
                    pad = 0;
                    gBoxWork->levelSel = 0;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                } else if (pad & PAD_CIRCLE) {
                    gHelpLine = 0x29;
                    pad = 0;
                    gBoxWork->yesNo = BOX_YES;
                    gBoxWork->levelSel = 0;
                    gBoxWork->sub++;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                } else {
                    gHelpLine = 0x28;
                    prevCursor = gBoxWork->set;
                    Menu_CursorLeftRight(&gBoxWork->set, pad & (PAD_LEFT | PAD_RIGHT), EQUIP_SET_COUNT);
                    setAtCursor = &save->equipSets[gBoxWork->set];
                    if (prevCursor != gBoxWork->set) {
                        EquipSet_RecalcStats((EquipIdx *)setAtCursor, 0);
                        pad = 0;
                    }
                    if (pad & PAD_SQUARE) {
                        gBoxWork->levelSel++;
                        if (gBoxWork->levelSel > 2) {
                            gBoxWork->levelSel = 0;
                        }
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
                        pad = 0;
                    }
                    if (gBoxWork->levelSel == 2) {
                        Menu_CursorLeftRight(&gBoxWork->skillPage, pad & (PAD_L1 | PAD_R1), gBoxWork->skillPages);
                        gHelpShow = 0;
                    }
                }
                break;
            case 1:
                answer = Box_YesNoInput(&pad);
                gHelpLine = 0x29;
                if (answer != 0) {
                    if (answer > 0) {
                        setCursor = &gBoxWork->set;
                        /* Only the six box slots: the set keeps its name. */
                        *(EquipIdx *)&sets[*setCursor] = *(EquipIdx *)save->equipIdx;
                        gBoxWork->cur = sets[*setCursor];
                        EquipSet_RecalcStats((EquipIdx *)&gBoxWork->cur, 0);
                        gHelpLine = 0x26;
                        gBoxWork->sub++;
                        gBoxWork->yesNo = BOX_YES;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DONE, 0);
                    } else {
                        gHelpLine = 0x28;
                        gBoxWork->sub--;
                        gBoxWork->yesNo = BOX_YESNO_NONE;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                    }
                }
                pad = 0;
                break;
            case 2:
                answer = Box_YesNoInput(&pad);
                gHelpLine = 0x26;
                if (answer != 0) {
                    if (answer > 0) {
                        gHelpLine = 0x2F;
                        gBoxWork->sub++;
                        gBoxWork->sub2 = 0;
                        EquipSet_RenameInput(0);
                    } else {
                        gBoxWork->sub = 4;
                        gHelpLine = 0x2A;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    }
                }
                pad = 0;
                break;
            case 3:
                if (EquipSet_RenameInput(pad) == 0) {
                    gHelpLine = 0x2A;
                    gBoxWork->sub++;
                }
                pad = 0;
                break;
            case 4:
                gHelpLine = 0x2A;
                gBoxWork->blink++;
                if (pad & PAD_CIRCLE) {
                    gHelpLine = 0x36;
                    gBoxWork->blink = BOX_BLINK_RESTART;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
                }
                pad = 0;
                break;
            }
            break;
        case EQUIP_MENU_SYMBOL_COLOR:
            if (pad & PAD_CROSS) {
                pad = 0;
                gHelpLine = 0x37;
                gBoxWork->step = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else {
                gBoxWork->count = BoxMenu_ListStyles(gBoxWork->list);
                Menu_CursorUpDown(&gBoxWork->pick, pad, gBoxWork->count);
                if (pad & PAD_SQUARE) {
                    gBoxWork->help ^= BOX_HELP_DETAIL;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
                }
                if (gBoxWork->help == BOX_HELP_OFF) {
                    gHelpLine = 0x32;
                } else {
                    gHelpLine = gBoxWork->list[gBoxWork->pick] + 0x3C;
                }
                if (pad & PAD_CIRCLE) {
                    Save_SetSymbolColor(gBoxWork->list[gBoxWork->pick]);
                    Player_SetColorWord((struct Player *)gBoxWork->player, gBoxWork->list[gBoxWork->pick]);
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    gLobbyTextDirty = 1;
                }
            }
            break;
        }
        break;
    }
    return pad;
}

void Player_RecomputeStats(struct Player *pl);
void Player_SetWeapon(struct Player *pl, EquipItem *e);
s32 Player_GetHpSkillBonus(struct Player *pl);
void Player_CalcAttackRemote(struct Player *pl);
void Player_CalcDefenseRemote(struct Player *pl);
void Player_CalcResistsRemote(struct Player *pl);
u8 SkillView_Init(void *pl, s32 b);

/*
 * Builds the stats preview of an equipment set in gPreviewPlayer: a copy of the local hunter
 * (Player_RecomputeStats first) wearing the set's pieces, less the armour unfit for its weapon
 * (EquipSet_DropUnfit on a copy: the set itself is left as it is). Then its skills, health
 * (100, plus the meal's and the health skills', at most 150), stamina (300 plus the meal's,
 * kept within 75-450), attack and defense with the meal's buffs, and resistances, as at a
 * quest's start; and the skill view's pages (gBoxWork->skillPages, skillPage kept below them).
 * compareTo is the hunter whose skills the skill list compares with (a Player *, 0 none).
 * A set without a weapon only clears the preview's pieces and the skill pages.
 */
void EquipSet_RecalcStats(EquipIdx *slots, s32 compareTo)
{
    EquipIdx fitted;
    s16 health;
    SaveData *save = &gSave;
    /* The cast on the object's address keeps the compiler reading through the
     * pointer, as the game's code does. */
    Player *pl = (Player *)&gPreviewPlayer;

    pl->active = 0;
    if (slots->idx[SAVE_EQUIP_WEAPON] == BOX_NONE) {
        pl->equip[EQUIP_SLOT_WEAPON].id = 0;
        pl->equip[EQUIP_HEAD].id = 0;
        pl->equip[EQUIP_BODY].id = 0;
        pl->equip[EQUIP_ARMS].id = 0;
        pl->equip[EQUIP_WAIST].id = 0;
        pl->equip[EQUIP_LEGS].id = 0;
        Player_CalcSkills(pl);
        gBoxWork->skillPages = 0;
        gBoxWork->skillPage = 0;
        return;
    }
    Player_RecomputeStats((struct Player *)gBoxWork->player);
    *(PlayerCopy *)pl = *(PlayerCopy *)gBoxWork->player;
    fitted = *slots;
    EquipSet_DropUnfit(&fitted, &fitted, 0);
    slots = &fitted;
    Player_SetWeapon(pl, &save->box[fitted.idx[SAVE_EQUIP_WEAPON]]);
    if (fitted.idx[SAVE_EQUIP_HEAD] != BOX_NONE) {
        pl->equip[EQUIP_HEAD] = save->box[slots->idx[SAVE_EQUIP_HEAD]];
    } else {
        pl->equip[EQUIP_HEAD].id = 0;
    }
    if (slots->idx[SAVE_EQUIP_BODY] != BOX_NONE) {
        pl->equip[EQUIP_BODY] = save->box[slots->idx[SAVE_EQUIP_BODY]];
    } else {
        pl->equip[EQUIP_BODY].id = 0;
    }
    if (slots->idx[SAVE_EQUIP_ARMS] != BOX_NONE) {
        pl->equip[EQUIP_ARMS] = save->box[slots->idx[SAVE_EQUIP_ARMS]];
    } else {
        pl->equip[EQUIP_ARMS].id = 0;
    }
    if (slots->idx[SAVE_EQUIP_WAIST] != BOX_NONE) {
        pl->equip[EQUIP_WAIST] = save->box[slots->idx[SAVE_EQUIP_WAIST]];
    } else {
        pl->equip[EQUIP_WAIST].id = 0;
    }
    if (slots->idx[SAVE_EQUIP_LEGS] != BOX_NONE) {
        pl->equip[EQUIP_LEGS] = save->box[slots->idx[SAVE_EQUIP_LEGS]];
    } else {
        pl->equip[EQUIP_LEGS].id = 0;
    }
    Player_CalcSkills(pl);
    health = gScene.mealHealth + 100 + (s16)Player_GetHpSkillBonus(pl);
    if (health > 150) {
        health = 150;
    }
    pl->hp = pl->hpMax = health;
    pl->staminaMax = gScene.mealStamina + 300;
    if (pl->staminaMax <= 0x4B) {
        pl->staminaMax = 0x4B;
    } else if (pl->staminaMax > 0x1C2) {
        pl->staminaMax = 0x1C2;
    }
    pl->attackDrug = gScene.mealAttack;
    pl->attackBuff = 0;
    Player_CalcAttackRemote(pl);
    pl->defenseDrug = gScene.mealDefense;
    pl->defenseBuff = 0;
    Player_CalcDefenseRemote(pl);
    Player_CalcResistsRemote(pl);
    gBoxWork->skillPages = SkillView_Init(pl, compareTo);
    if (gBoxWork->skillPage > gBoxWork->skillPages - 1) {
        gBoxWork->skillPage = gBoxWork->skillPages - 1;
    }
}

void Equip_SetEquipped(EquipIdx *e);
u8 BoxMenu_CloseStep(u16 pad, u8 step);
void BoxMenu_UpdateSkillPreview(void);
u8 EquipSet_DropUnfit(EquipIdx *out, EquipIdx *set, s32 flags);
s32 ItemBox_SlotHasItem(u16 slot);
s32 EquipBox_TryRemove(u16 slot);
static s32 BoxMenu_CanSplit(void);
s32 BoxMenu_CanTakeSeveral(void);
void BoxMenu_InputAmount(u16 pad, u8 noLimit);
void Equip_SwapSlots(u16 a, u16 b);
s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
s32 Field_ControlEnabled(void);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
u8 YesNo_Toggle(u16 pad, u8 cur, u16 mask);
void *memcpy(void *dst, const void *src, u32 n);
INCLUDE_RODATA("asm/lobby/nonmatchings/item_box_menu", gBoxEmptyText);
extern char gBoxEmptyText[]; /* an empty string */

/*
 * Names the equipment set gBoxWork->set, once a frame (its step is gBoxWork->sub2); called first
 * with pad 0 to start. Step 0 opens the software keyboard on an empty text and copies the set's
 * name into gBoxWork->cur.name; step 1 runs the keyboard and keeps a submitted text in
 * gBoxWork->cur.name; steps 2-3 close it and ask yes/no (the first PAD_CROSS moves to no, the
 * second answers no). Yes copies the name into the set; no opens the keyboard again.
 * Returns 1 while running, 0 when done; 0 at once (the naming is skipped) while the hunter
 * takes no field input (Field_ControlEnabled).
 */
s32 EquipSet_RenameInput(u16 pad)
{
    EquipSet *sets = gSave.equipSets;
    s8 answer;
    u8 name[0x14];
    s16 padHeld;
    s16 padPressed;

    switch (gBoxWork->sub2) {
    case 0:
        if (Field_ControlEnabled() != 1) {
            return 0;
        }
        gBoxWork->sub2++;
        memcpy(gBoxWork->cur.name, sets[gBoxWork->set].name, sizeof(gBoxWork->cur.name));
        Kbd_SetPos(0x3C, 76.0f);
        Chat_OpenKbd(2, 0xE, sizeof(gBoxWork->cur.name), gBoxEmptyText);
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
        /* falls through: the keyboard runs this frame */
    case 1:
        padHeld = *(s16 *)&gPadState[0].held;
        gHelpLine = 0x2F;
        padPressed = *(s16 *)&gPadState[0].pressed;
        name[0] = 0;
        answer = Kbd_Run((char *) name, padHeld, padPressed);
        if (answer != 0) {
            if (answer > 0) {
                memcpy(gBoxWork->cur.name, name, sizeof(gBoxWork->cur.name));
            }
            gBoxWork->sub2++;
        }
        break;
    case 2:
        Kbd_Cancel();
        gBoxWork->yesNo = BOX_YES;
        gBoxWork->sub2++;
        /* falls through: the yes/no starts this frame */
    case 3:
        answer = 0;
        if (pad & PAD_CROSS) {
            if (gBoxWork->yesNo == BOX_YES) {
                gBoxWork->yesNo = BOX_NO;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
            } else {
                answer = -1;
            }
        } else if (pad & PAD_CIRCLE) {
            answer = gBoxWork->yesNo == BOX_YES ? 1 : -1;
        } else {
            gBoxWork->yesNo = YesNo_Toggle(pad, gBoxWork->yesNo, PAD_LEFT | PAD_RIGHT);
        }
        gHelpLine = 0x27;
        if (answer != 0) {
            gBoxWork->yesNo = BOX_YESNO_NONE;
            if (answer > 0) {
                memcpy(sets[gBoxWork->set].name, gBoxWork->cur.name, sizeof(gBoxWork->cur.name));
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                return 0;
            }
            gBoxWork->sub2 = 0;
            gHelpLine = 0x2F;
        }
        break;
    }
    return 1;
}

/*
 * Wears an equipment set: SaveData.equipIdx takes the set's weapon, Equip_Refresh copies the
 * worn box slots into SaveData.equip, then equipIdx takes the whole set (its six box slots, not
 * the name). BoxMenu_CloseStep's reload (Town_ReloadLocalPlayer) refreshes SaveData.equip again.
 */
void Equip_SetEquipped(EquipIdx *set)
{
    gSave.equipIdx[SAVE_EQUIP_WEAPON] = set->idx[SAVE_EQUIP_WEAPON];
    Equip_Refresh(&gSave);
    *(EquipIdx *)gSave.equipIdx = *set;
}

/*
 * The end of an equipment change, one step a frame: step 0 plays the change's sound (the common
 * bank's 0x89), step 1 reloads the local hunter with the new equipment (Town_ReloadLocalPlayer)
 * and its skills, step 2 shows the "changed" help line with the cursor blinking until
 * PAD_CIRCLE. Returns the next step: 0 once closed.
 */
u8 BoxMenu_CloseStep(u16 pad, u8 step)
{
    switch (step) {
    case 0:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 0x89, 0);
        step++;
        break;
    case 1:
        Town_ReloadLocalPlayer();
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        step++;
        break;
    case 2:
        gHelpLine = 0x13;
        gBoxWork->blink++;
        if (pad & PAD_CIRCLE) {
            step = 0;
            gBoxWork->blink = BOX_BLINK_RESTART;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
        }
        break;
    }
    return step;
}

u8 Equip_LevelMark(struct EquipItem *e, u8 flags, struct EquipItem *other);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);

/*
 * The box cursor of the equipment screen: flags bit 0 clear, setIdx is SaveData.equipIdx and
 * the pieces go on the hunter; bit 0 set, setIdx is an equipment set's entries (its weapon is
 * chosen first). Moves the cursor over the equipment box (slots[1]), finds the piece of the
 * same kind in setIdx (equipped), and says whether the piece at the cursor can be taken
 * (*ok, canEquip): an armour piece must fit the hunter's gender and the class of the weapon
 * (blademaster or gunner), and the weapon already in use can't be picked again; the help line
 * says why not. PAD_SQUARE opens the skills preview (bit 0 set: the levels first, then the
 * skills), L1/R1 turn its pages or the levels; PAD_SELECT opens the sort prompt. Returns the
 * pad left for the caller (PAD_CROSS: back), 0 when the input was used here.
 */
u16 BoxMenu_EquipPickInput(u16 *setIdx, u16 pad, s32 *ok, s32 flags)
{
    SaveData *save = &gSave;
    u8 mode;
    u16 idx;
    u16 prev;
    u8 helpMode;
    u8 *helpPtr;
    u8 armorFlags;

    pad = BoxMenu_SortPrompt(pad, 1);
    if (gBoxWork->prompt != 0) {
        return pad;
    }
    mode = flags & 1;
    if (mode == 1) {
        helpPtr = &gBoxWork->help;
        helpMode = *helpPtr;
        if (helpMode == BOX_HELP_OFF) {
            if (!(pad & (PAD_CIRCLE | PAD_CROSS)) && (pad & PAD_SQUARE)) {
                *helpPtr = BOX_HELP_DETAIL;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
                return 0;
            }
        } else if (pad & PAD_CROSS) {
            *helpPtr = BOX_HELP_OFF;
            pad = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        } else if (!(pad & PAD_CIRCLE)) {
            if (helpMode == BOX_HELP_DETAIL) {
                if (pad & PAD_SQUARE) {
                    *helpPtr = BOX_HELP_SKILLS;
                    BoxMenu_UpdateSkillPreview();
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
                } else {
                    Menu_CursorLeftRight(&gBoxWork->levelSel, pad & (PAD_L1 | PAD_R1), gBoxWork->levelCount);
                }
            } else if (helpMode == BOX_HELP_SKILLS) {
                if (pad & PAD_SQUARE) {
                    *helpPtr = BOX_HELP_DETAIL;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
                } else {
                    Menu_CursorLeftRight(&gBoxWork->skillPage, pad & (PAD_L1 | PAD_R1), gBoxWork->skillPages);
                }
            }
            return 0;
        }
    }
    if (pad & PAD_CROSS) {
        return PAD_CROSS;
    }
    if (mode == 0) {
        gHelpLine = 0xA;
    } else if (setIdx[SAVE_EQUIP_WEAPON] != BOX_NONE) {
        gHelpLine = 0x23;
    } else {
        gHelpLine = 0x24;
    }
    prev = gBoxWork->slots[1];
    BoxMenu_MoveCursor(&gBoxWork->slots[1], pad);
    gBoxWork->boxItem = &save->box[gBoxWork->slots[1]];
    if (gBoxWork->boxItem->used != 0) {
        *ok = 1;
        gBoxWork->cursorClass = BOX_CLASS_ARMOR;
        gBoxWork->canEquip = 1;
        switch (gBoxWork->boxItem->type) {
        case EQUIP_LEGS:
            idx = setIdx[SAVE_EQUIP_LEGS];
            break;
        case EQUIP_HEAD:
            idx = setIdx[SAVE_EQUIP_HEAD];
            break;
        case EQUIP_BODY:
            idx = setIdx[SAVE_EQUIP_BODY];
            break;
        case EQUIP_ARMS:
            idx = setIdx[SAVE_EQUIP_ARMS];
            break;
        case EQUIP_WAIST:
            idx = setIdx[SAVE_EQUIP_WAIST];
            break;
        case EQUIP_BLADE:
        case EQUIP_GUN:
            idx = setIdx[SAVE_EQUIP_WEAPON];
            gBoxWork->cursorClass = BOX_CLASS_WEAPON;
            break;
        }
        if (idx != BOX_NONE) {
            gBoxWork->equipped = &save->box[idx];
        } else {
            gBoxWork->equipped = NULL;
        }
        if (mode == 0) {
            if (gBoxWork->equipped == gBoxWork->boxItem) {
                if (gBoxWork->cursorClass != BOX_CLASS_ARMOR) {
                    *ok = 0;
                    gHelpLine = 0xC;
                    gBoxWork->canEquip = 0;
                }
                gBoxWork->boxItem = NULL;
            } else if (gBoxWork->cursorClass == BOX_CLASS_ARMOR) {
                armorFlags = Equip_GetData(gBoxWork->boxItem)->armor.flags;
                if (!(armorFlags & ((save->gender == 0) ? (u8)ARMOR_FLAG_MALE : (u8)ARMOR_FLAG_FEMALE))) {
                    *ok = 0;
                    gHelpLine = 0xE;
                    gBoxWork->canEquip = 0;
                } else if (!(armorFlags & (u8)((save->box[setIdx[SAVE_EQUIP_WEAPON]].type == EQUIP_BLADE) ? ARMOR_FLAG_BLADEMASTER : ARMOR_FLAG_GUNNER))) {
                    *ok = 0;
                    gHelpLine = 0xD;
                    gBoxWork->canEquip = 0;
                }
            } else if (gBoxWork->equipped->type != gBoxWork->boxItem->type) {
                gBoxWork->cursorClass = BOX_CLASS_OTHER_WEAPON;
            }
        } else if (gBoxWork->cursorClass != BOX_CLASS_ARMOR) {
            if (gBoxWork->equipped == gBoxWork->boxItem) {
                *ok = 0;
                gHelpLine = 0xC;
                gBoxWork->boxItem = NULL;
            } else if (gBoxWork->equipped != NULL && gBoxWork->equipped->type != gBoxWork->boxItem->type) {
                gBoxWork->cursorClass = BOX_CLASS_OTHER_WEAPON;
            }
        } else if (setIdx[SAVE_EQUIP_WEAPON] != BOX_NONE) {
            if (gBoxWork->equipped == gBoxWork->boxItem) {
                gBoxWork->boxItem = NULL;
            } else {
                armorFlags = Equip_GetData(gBoxWork->boxItem)->armor.flags;
                if (!(armorFlags & ((save->gender == 0) ? (u8)ARMOR_FLAG_MALE : (u8)ARMOR_FLAG_FEMALE))) {
                    *ok = 0;
                    gHelpLine = 0xE;
                    gBoxWork->canEquip = 0;
                } else if (!(armorFlags & (u8)((save->box[setIdx[SAVE_EQUIP_WEAPON]].type == EQUIP_BLADE) ? ARMOR_FLAG_BLADEMASTER : ARMOR_FLAG_GUNNER))) {
                    *ok = 0;
                    gHelpLine = 0x25;
                    gBoxWork->canEquip = 0;
                }
            }
        } else {
            *ok = 0;
            gBoxWork->canEquip = 0;
        }
        gBoxWork->levelCount = Equip_LevelMark((struct EquipItem *)gBoxWork->boxItem, 0x10, (struct EquipItem *)gBoxWork->equipped);
        if (gBoxWork->levelSel >= gBoxWork->levelCount) {
            gBoxWork->levelSel = gBoxWork->levelCount - 1;
        }
    } else {
        *ok = 0;
        gBoxWork->canEquip = 0;
    }
    if (mode == 0) {
        if (gBoxWork->help != BOX_HELP_OFF) {
            if (prev != gBoxWork->slots[1] && gBoxWork->canEquip != 0) {
                BoxMenu_UpdateSkillPreview();
            }
            if (!(pad & PAD_CIRCLE)) {
                if (pad & (PAD_SQUARE | PAD_CROSS)) {
                    gBoxWork->help = BOX_HELP_OFF;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                } else {
                    Menu_CursorLeftRight(&gBoxWork->skillPage, pad & (PAD_L1 | PAD_R1), gBoxWork->skillPages);
                }
                pad = 0;
            }
        } else if (pad & PAD_SQUARE) {
            gBoxWork->help = BOX_HELP_OPEN;
            BoxMenu_UpdateSkillPreview();
            pad = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
        } else {
            Menu_CursorLeftRight(&gBoxWork->levelSel, pad & (PAD_L1 | PAD_R1), gBoxWork->levelCount);
        }
    }
    return pad;
}

/* Fills the skills preview (gSkillPreview) for the equipment at the box cursor, or, when the
 * cursor is on the worn piece, for taking it off (an empty piece of its type), and shows its
 * first page. */
void BoxMenu_UpdateSkillPreview(void)
{
    ItemBoxMenu *menu = gBoxWork;
    EquipItem *src = menu->boxItem;
    Player *player = menu->player;
    EquipItem item;

    if (src == NULL) {
        item.type = menu->equipped->type;
        item.id = 0;
    } else {
        item = *src;
    }
    gBoxWork->skillPages = Equip_PreviewSkills(player, &item);
    gBoxWork->skillPage = 0;
}


extern Player gSkillTryPlayer; /* a scratch Player: the equipment tried on */
void Player_RemoveUnwearable(struct Player *pl);
s32 Equip_DataWearable(struct Player *pl, EquipItem *e);
u8 Player_ArmorHasTorsoUp(Player *p);

/*
 * Tries the piece on a copy of the player (gSkillTryPlayer): a weapon of the other kind first
 * drops the armour the new class can't wear (Player_RemoveUnwearable). Fills gSkillPreview: the
 * skill list against the player's own, the piece, whether the player can wear it (weapons
 * always) and whether the copy's armour has torso up. Returns the list's page count. The
 * shops and the box menu call it.
 */
u8 Equip_PreviewSkills(Player *player, EquipItem *tried)
{
    Player *pl = player;
    Player *tryOn = (Player *)&gSkillTryPlayer; /* the cast keeps the address in a register */
    SkillPreview *preview;

    gSkillTryPlayer.equip[EQUIP_SLOT_WEAPON] = pl->equip[EQUIP_SLOT_WEAPON];
    tryOn->equip[EQUIP_LEGS] = pl->equip[EQUIP_LEGS];
    tryOn->equip[EQUIP_SLOT_FACE] = pl->equip[EQUIP_SLOT_FACE];
    tryOn->equip[EQUIP_HEAD] = pl->equip[EQUIP_HEAD];
    tryOn->equip[EQUIP_BODY] = pl->equip[EQUIP_BODY];
    tryOn->equip[EQUIP_ARMS] = pl->equip[EQUIP_ARMS];
    tryOn->equip[EQUIP_WAIST] = pl->equip[EQUIP_WAIST];
    if (tried->type == EQUIP_BLADE || tried->type == EQUIP_GUN) {
        tryOn->equip[EQUIP_SLOT_WEAPON] = *tried;
        if (pl->equip[EQUIP_SLOT_WEAPON].type != tried->type) {
            Player_RemoveUnwearable(tryOn);
        }
    } else {
        tryOn->equip[tried->type] = *tried;
    }
    Player_CalcSkills(tryOn);
    SkillList_Build(tryOn, player);
    preview = (SkillPreview *)&gSkillPreview; /* the cast keeps the address in a register */
    preview->compare = NULL;
    preview->item = *tried;
    preview->item.used = 1;
    preview->pages = SkillList_PageCount();
    if (tried->type == EQUIP_BLADE || tried->type == EQUIP_GUN) {
        preview->wearable = 1;
    } else {
        preview->wearable = Equip_DataWearable(tryOn, tried);
    }
    preview->torsoUp = Player_ArmorHasTorsoUp(tryOn);
    return preview->pages;
}

/* Fills gSkillPreview with the skills of pl (an equipment set tried on), compared with those of
 * player compareWith (0: listed alone), with no piece shown. Returns the page count. */
u8 SkillView_Init(void *pl, s32 compareWith)
{
    SkillPreview *view;

    Player_CalcSkills((struct Player *)pl);
    SkillList_Build((struct Player *)pl, (struct Player *)compareWith);
    view = (SkillPreview *)&gSkillPreview; /* the cast keeps the address in a register */
    view->compare = (struct Player *)compareWith;
    view->item.used = 0;
    view->pages = SkillList_PageCount();
    view->wearable = 1;
    return view->pages;
}

/*
 * Takes equipment box slot 'slot' out of the ten saved equipment sets: an armour piece leaves
 * its entry empty (BOX_NONE), a weapon empties the whole set. Returns the number of sets
 * changed. With testOnly bit 0 nothing changes, and it returns 1 at the first set that holds
 * the slot (0: in none).
 */
u8 EquipSet_RemoveSlot(u16 slot, s32 testOnly)
{
    SaveData *save = &gSave;
    EquipSet *set;
    s32 removed;
    s32 i;
    u32 j;

    removed = 0;
    set = gSave.equipSets;
    for (i = EQUIP_SET_COUNT; i != 0; i--, set++) {
        for (j = 0; j < SAVE_EQUIP_COUNT; j++) {
            if (slot == set->idx[j]) {
                if (!(testOnly & 1)) {
                    if (save->box[slot].type != EQUIP_GUN && save->box[slot].type != EQUIP_BLADE) {
                        set->idx[j] = BOX_NONE;
                    } else {
                        set->idx[SAVE_EQUIP_WAIST] = BOX_NONE;
                        set->idx[SAVE_EQUIP_ARMS] = BOX_NONE;
                        set->idx[SAVE_EQUIP_BODY] = BOX_NONE;
                        set->idx[SAVE_EQUIP_HEAD] = BOX_NONE;
                        set->idx[SAVE_EQUIP_LEGS] = BOX_NONE;
                        set->idx[SAVE_EQUIP_WEAPON] = BOX_NONE;
                    }
                    removed++;
                    break;
                } else {
                    return 1;
                }
            }
        }
    }
    return removed;
}

/* Counts the armour pieces of an equipment set that the class of its weapon can't wear
 * (ArmorData.flags' blademaster and gunner bits). Unless flags bit 0 is set or out is NULL,
 * empties their entries and copies the set to out. Returns the count. */
u8 EquipSet_DropUnfit(EquipIdx *out, EquipIdx *set, s32 flags)
{
    SaveData *save = &gSave;
    u8 armorFlags;
    u8 classBit;
    u32 i;
    u8 dropped;

    if (out == NULL) {
        flags |= 1;
    }
    classBit = (save->box[set->idx[SAVE_EQUIP_WEAPON]].type == EQUIP_BLADE) ? ARMOR_FLAG_BLADEMASTER : ARMOR_FLAG_GUNNER;
    dropped = 0;
    for (i = SAVE_EQUIP_LEGS; i < SAVE_EQUIP_COUNT; i++) {
        if (set->idx[i] != BOX_NONE) {
            armorFlags = Equip_GetData(&save->box[set->idx[i]])->armor.flags;
            if (!(armorFlags & classBit)) {
                if (!(flags & 1)) {
                    set->idx[i] = BOX_NONE;
                }
                dropped++;
            }
        }
    }
    if (!(flags & 1)) {
        *out = *set;
    }
    return dropped;
}

/* The ArmorData.flags bit of a hunter of this gender (SaveData.gender: 0 male):
 * ARMOR_FLAG_MALE or ARMOR_FLAG_FEMALE. */
static inline u8 Equip_GenderBit(u8 gender)
{
    return gender == 0 ? ARMOR_FLAG_MALE : ARMOR_FLAG_FEMALE;
}

s32 ItemBox_SlotHasItem(u16 slot);
s32 EquipBox_TryRemove(u16 slot);
static s32 BoxMenu_CanSplit(void);
s32 BoxMenu_CanTakeSeveral(void);
void BoxMenu_InputAmount(u16 pad, u8 noLimit);
void Equip_SwapSlots(u16 a, u16 b);
s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
s16 BoxList_MoveCursor(u8 *row, u8 *page, u16 pad);
u8 Equip_LevelMark(struct EquipItem *e, u8 flags, struct EquipItem *other);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);

/*
 * The box list of the equipment set editor (BoxList_MoveCursor over the pieces of the chosen
 * part): the box item at the cursor and whether it can go in the set (*ok): part 0, a weapon
 * other than the set's; else an armour piece of the hunter's gender and of the class of the
 * set's weapon (setIdx[0]). Sets the help line, and PAD_SQUARE opens the detail with its levels
 * (L1/R1). Returns the pad. gHelpLine (an s16) is cleared through a u16 pointer: the same
 * halfword store, in the form that builds the game's code.
 */
u16 BoxMenu_EquipListInput(u16 *setIdx, u16 pad, s32 *ok, s32 flags)
{
    SaveData *save = &gSave;
    ArmorData *data;
    u8 *helpPtr;
    ItemBoxMenu *menu;
    s32 buttons;

    *(u16 *)&gHelpLine = 0xFFFF;
    if (pad & (PAD_CIRCLE | PAD_CROSS)) {
        pad &= PAD_CIRCLE | PAD_CROSS;
    }
    gBoxWork->slots[1] = BoxList_MoveCursor(&gBoxWork->row, &gBoxWork->page, pad);
    gBoxWork->boxItem = &save->box[gBoxWork->slots[1]];
    *ok = 1;
    if (gBoxWork->part == 0) {
        gBoxWork->cursorClass = BOX_CLASS_WEAPON;
        if (gBoxWork->equipped == gBoxWork->boxItem) {
            *ok = 0;
            gHelpLine = 0xC;
        }
    } else {
        gBoxWork->cursorClass = BOX_CLASS_ARMOR;
        if (gBoxWork->equipped == gBoxWork->boxItem) {
            gHelpLine = 0x2E;
        } else {
            data = &Equip_GetData(gBoxWork->boxItem)->armor;
            if (!(data->flags & Equip_GenderBit(save->gender))) {
                *ok = 0;
                gHelpLine = 0xE;
            } else {
                if (!(data->flags & (u8)(save->box[*setIdx].type == EQUIP_BLADE ? ARMOR_FLAG_BLADEMASTER : ARMOR_FLAG_GUNNER))) {
                    *ok = 0;
                    gHelpLine = 0x25;
                }
            }
        }
    }
    gBoxWork->levelCount = Equip_LevelMark((struct EquipItem *)gBoxWork->boxItem, 0, (struct EquipItem *)NULL);
    if (gBoxWork->levelSel >= gBoxWork->levelCount) {
        gBoxWork->levelSel = gBoxWork->levelCount - 1;
    }
    menu = gBoxWork;
    helpPtr = &menu->help;
    if (*helpPtr == BOX_HELP_OFF) {
        if (pad & PAD_SQUARE) {
            *helpPtr = BOX_HELP_DETAIL;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
        }
    } else {
        buttons = pad;
        if (buttons & PAD_SQUARE) {
            *helpPtr = BOX_HELP_OFF;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        } else if (menu->boxItem != NULL) {
            Menu_CursorLeftRight(&menu->levelSel, buttons & (PAD_L1 | PAD_R1), menu->levelCount);
        }
    }
    return pad;
}

u16 BoxMenu_Input(u16 pad);

/*
 * The box menu's arrange screen. Step 0 picks the item box or the equipment box (kind; PAD_CROSS
 * goes back to the menu list). Step 1 moves the cursor (BoxMenu_Input): PAD_CIRCLE holds a slot
 * (held), a second PAD_CIRCLE on another slot swaps the two (for equipment, Equip_SwapSlots
 * moves the worn and saved-set entries with them); PAD_SELECT sorts the box. Returns the pad.
 * kind's pointer is taken first and the help line read through it. The swap reads the held slot
 * and the kind into heldSlot and curKind right after taking their pointers (held, heldSlot,
 * kindPtr, curKind, slots): in that order the compiler keeps slots in gBoxWork's register, as
 * the game's code does; with the reads later, gBoxWork gets a register of its own.
 */
u16 BoxMenu_ArrangeMenu(u16 pad)
{
    u8 *kind = &gBoxWork->kind;
    SaveData *save;
    u8 *kindPtr;
    u16 *slots;
    u16 *held;
    u16 cursorSlot;
    u8 curKind;
    u16 heldSlot;
    ItemStack stack;
    EquipItem item;

    gHelpLine = *kind + 0x14;
    switch (gBoxWork->step) {
    case 0:
        if (pad & PAD_CROSS) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        Menu_CursorUpDown(kind, pad, 2);
        if (pad & PAD_CIRCLE) {
            gBoxWork->help = BOX_HELP_OFF;
            gBoxWork->step++;
            gBoxWork->sub = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        } else {
            gHelpShow = 0;
        }
        break;
    case 1:
        pad = BoxMenu_SortPrompt(pad, *kind);
        if (gBoxWork->prompt != 0) {
            gBoxWork->sub = 0;
            break;
        }
        switch (gBoxWork->sub) {
        case 0:
            pad = BoxMenu_Input(pad);
            if (pad & PAD_CROSS) {
                gHelpShow = 0;
                pad &= ~PAD_CROSS;
                gBoxWork->step = 0;
            } else if (pad & PAD_CIRCLE) {
                gBoxWork->held = (&gBoxWork->slots[0])[gBoxWork->kind];
                gBoxWork->sub++;
                Sound_PlaySeFlat(SE_SLOT_COMMON, 0x8A, 0);
            }
            break;
        case 1:
            pad = BoxMenu_Input(pad);
            if (pad & PAD_CROSS) {
                pad &= ~PAD_CROSS;
                gBoxWork->sub = 0;
            } else if (pad & PAD_CIRCLE) {
                held = &gBoxWork->held;
                heldSlot = *held;
                kindPtr = &gBoxWork->kind;
                curKind = *kindPtr;
                slots = gBoxWork->slots;
                cursorSlot = slots[curKind];
                if (cursorSlot != heldSlot) {
                    save = &gSave;
                    if (curKind == 0) {
                        stack = save->itemBox[cursorSlot];
                        save->itemBox[cursorSlot] = save->itemBox[heldSlot];
                        save->itemBox[gBoxWork->held] = stack;
                    } else {
                        item = save->box[cursorSlot];
                        save->box[cursorSlot] = save->box[heldSlot];
                        save->box[*held] = item;
                        Equip_SwapSlots(*held, slots[*kindPtr]);
                    }
                    gBoxWork->sub = 0;
                    Sound_PlaySeFlat(SE_SLOT_COMMON, 0x8B, 0);
                } else {
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                }
            }
            break;
        }
        break;
    }
    return pad;
}

s32 Box_ClearSlot(SaveData *save, u16 idx);
s32 Equip_GetSellPrice(EquipItem *e);

/* ItemBoxMenu seen by BoxMenu_SellMenu: the ItemBoxMenu form changes the code (MWCC reuses the load of gBoxWork) */
typedef struct SellView {
    /* 0x00 */ u8 unk0[5];
    /* 0x05 */ u8 step;
    /* 0x06 */ u8 sub;
    /* 0x07 */ u8 unk7[2];
    /* 0x09 */ u8 kind;             /* 0 items, 1 equipment */
    /* 0x0A */ u8 unkA[0xE];
    /* 0x18 */ s32 price;
    /* 0x1C */ u16 slot[2];
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ s16 amount;
    /* 0x26 */ u8 unk26[7];
    /* 0x2D */ s8 msg;
} SellView;

/*
 * The box menu's sell screen. Step 0 picks the item box or the equipment box (kind; PAD_CROSS
 * goes back to the menu list). Step 1: sub 0 moves the cursor and PAD_CIRCLE picks the slot (an
 * item stack that can be split goes to sub 1, the amount; a piece of equipment can't be sold
 * while worn, and the help line warns when it is in a saved set); sub 2 asks yes or no with the
 * price, and yes takes the amount out of the box (equipment: also out of the sets) and adds the
 * price to the money. A message (msg) waits for PAD_CIRCLE before the cursor moves. Returns the
 * pad.
 */
u16 BoxMenu_SellMenu(u16 pad)
{
    SaveData *save = &gSave;
    s8 *msgPtr;
    u8 *subPtr;
    SellView *view = (SellView *)gBoxWork;
    s32 ok;
    s8 result;
    s32 buttons;
    s32 keys;

    switch (view->step) {
    case 0:
        buttons = pad;
        if (buttons & PAD_CROSS) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        Menu_CursorUpDown(&view->kind, pad, 2);
        if (buttons & PAD_CIRCLE) {
            view = (SellView *)gBoxWork;
            if (view->kind == BOX_KIND_ITEMS) {
                gHelpLine = 0x16;
            } else {
                gHelpLine = 0x17;
                if (EquipSet_RemoveSlot(view->slot[1], 1) != 0) {
                    gHelpLine = 0x1B;
                }
            }
            ((SellView *)gBoxWork)->msg = -1;
            ((SellView *)gBoxWork)->step++;
            ((SellView *)gBoxWork)->sub = 0;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        } else {
            gHelpShow = 0;
        }
        break;
    case 1:
        subPtr = &view->sub;
        switch (*subPtr) {
        case 0:
            msgPtr = &view->msg;
            if (*msgPtr >= 0) {
                if (pad & PAD_CIRCLE) {
                    *msgPtr = -1;
                    gBoxWork->blink = BOX_BLINK_RESTART;
                    pad = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
                } else {
                    ((ItemBoxMenu *)view)->blink++;
                    gHelpLine = ((SellView *)gBoxWork)->msg;
                    return 0;
                }
            }
            pad = BoxMenu_SortPrompt(pad, ((SellView *)gBoxWork)->kind);
            if (gBoxWork->prompt != 0) {
                break;
            }
            pad = BoxMenu_Input(pad);
            if (pad & PAD_CROSS) {
                pad = 0;
                gHelpShow = 0;
                ((SellView *)gBoxWork)->step = 0;
                break;
            }
            view = (SellView *)gBoxWork;
            if (view->kind == BOX_KIND_ITEMS) {
                gHelpLine = 0x16;
            } else {
                gHelpLine = 0x17;
                if (EquipSet_RemoveSlot(view->slot[1], 1) != 0) {
                    gHelpLine = 0x1B;
                }
            }
            if (pad & PAD_CIRCLE) {
                view = (SellView *)gBoxWork;
                if (view->kind == BOX_KIND_ITEMS) {
                    ok = ItemBox_SlotHasItem(view->slot[0]);
                    if (ok == 1) {
                        if (BoxMenu_CanSplit() == 0) {
                            gHelpLine = 0x19;
                            ((SellView *)gBoxWork)->sub = 2;
                            gBoxWork->yesNo = BOX_NO;
                            ((SellView *)gBoxWork)->price = gItemData[save->itemBox[((SellView *)gBoxWork)->slot[0]].item].value;
                        } else {
                            ((SellView *)gBoxWork)->amount = 1;
                            gHelpLine = 0x18;
                            ((SellView *)gBoxWork)->sub = 1;
                        }
                    }
                } else {
                    result = EquipBox_TryRemove(view->slot[1]);
                    if (result > 0) {
                        gHelpLine = 0x19;
                        ((SellView *)gBoxWork)->sub = 2;
                        gBoxWork->yesNo = BOX_NO;
                        ((SellView *)gBoxWork)->price = Equip_GetSellPrice(&save->box[((SellView *)gBoxWork)->slot[1]]);
                        ok = 1;
                    } else {
                        ok = 0;
                        if (result == 0) {
                            ((SellView *)gBoxWork)->msg = 0x1A;
                        }
                    }
                }
                if (ok == 1) {
                    gBoxWork->help = BOX_HELP_OFF;
                    ((SellView *)gBoxWork)->amount = 1;
                    gBoxWork->atMax = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                } else {
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                }
            }
            break;
        case 1:
            buttons = pad;
            if (buttons & PAD_CROSS) {
                gHelpLine = 0x16;
                pad &= ~PAD_CROSS;
                *subPtr = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                break;
            }
            gHelpLine = 0x18;
            BoxMenu_InputAmount(pad, 1);
            if (!(buttons & PAD_CIRCLE)) {
                break;
            }
            pad = 0;
            view = (SellView *)gBoxWork;
            view->price = view->amount * gItemData[save->itemBox[view->slot[view->kind]].item].value;
            ((SellView *)gBoxWork)->sub++;
            gBoxWork->yesNo = BOX_NO;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
            /* falls through: the amount is set, the yes/no follows at once */
        case 2:
            keys = pad;
            result = 0;
            if (keys & PAD_CROSS) {
                result = -1;
            } else {
                gBoxWork->yesNo = YesNo_Toggle(pad, gBoxWork->yesNo, PAD_LEFT | PAD_RIGHT);
                if (keys & PAD_CIRCLE) {
                    result = gBoxWork->yesNo == BOX_YES ? 1 : -1;
                }
            }
            gHelpLine = 0x19;
            if (result != 0) {
                if (result < 0) {
                    if (((SellView *)gBoxWork)->kind == BOX_KIND_ITEMS && BoxMenu_CanSplit() == 1) {
                        gHelpLine = 0x18;
                        ((SellView *)gBoxWork)->sub = 1;
                    } else {
                        view = (SellView *)gBoxWork;
                        gHelpLine = view->kind + 0x16;
                        view->sub = 0;
                    }
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                } else {
                    view = (SellView *)gBoxWork;
                    if (view->kind == BOX_KIND_ITEMS) {
                        if (gItemData[save->itemBox[view->slot[view->kind]].item].max == ITEM_MAX_UNLIMITED) {
                            save->itemBox[view->slot[view->kind]].item = 0;
                            view = (SellView *)gBoxWork;
                            save->itemBox[view->slot[view->kind]].count = 0;
                        } else {
                            save->itemBox[view->slot[view->kind]].count -= view->amount;
                            view = (SellView *)gBoxWork;
                            if (save->itemBox[view->slot[view->kind]].count == 0) {
                                save->itemBox[view->slot[view->kind]].item = 0;
                            }
                        }
                    } else {
                        EquipSet_RemoveSlot(view->slot[view->kind], 0);
                        view = (SellView *)gBoxWork;
                        Box_ClearSlot(save, view->slot[view->kind]);
                    }
                    Save_AddMoney(((SellView *)gBoxWork)->price);
                    view = (SellView *)gBoxWork;
                    gHelpLine = view->kind + 0x16;
                    view->sub = 0;
                    Sound_PlaySeFlat(SE_SLOT_COMMON, 0x83, 0);
                }
                pad = 0;
                gBoxWork->yesNo = BOX_YESNO_NONE;
            }
            break;
        }
        break;
    }
    return pad;
}

/* 1 when item box slot 'slot' holds an item that has an icon (ItemData.icon not 0xFF), else 0. */
s32 ItemBox_SlotHasItem(u16 slot)
{
    u16 item = gSave.itemBox[slot].item;

    if (item == 0) {
        return 0;
    }
    if (gItemData[item].icon != 0xFF) {
        return 1;
    }
    return 0;
}

/* Whether equipment box slot 'slot' can leave the box: -1 empty, 0 worn (in SaveData.equipIdx),
 * else 1. Its call of EquipSet_RemoveSlot only tests (bit 0), and the answer isn't used. */
s32 EquipBox_TryRemove(u16 slot)
{
    SaveData *save = &gSave;
    s32 slotIdx = slot;
    EquipItem *item = &save->box[slotIdx];

    if (item->used == 0) {
        return -1;
    }
    if (slotIdx == save->equipIdx[SAVE_EQUIP_LEGS] || slotIdx == save->equipIdx[SAVE_EQUIP_HEAD] ||
        slotIdx == save->equipIdx[SAVE_EQUIP_BODY] || slotIdx == save->equipIdx[SAVE_EQUIP_ARMS] ||
        slotIdx == save->equipIdx[SAVE_EQUIP_WAIST] || slotIdx == save->equipIdx[SAVE_EQUIP_WEAPON]) {
        return 0;
    }
    EquipSet_RemoveSlot(slot, 1);
    return 1;
}

/* 1 when the stack at the item box cursor can be split (an amount chosen): its item has a pouch
 * limit (not ITEM_MAX_UNLIMITED) above 1 and the stack holds more than one. Static: then its
 * caller BoxMenu_CanTakeSeveral keeps gSave's address in a register across the call, as the
 * game's code does. */
static s32 BoxMenu_CanSplit(void)
{
    SaveData *save = &gSave;
    u16 slot = gBoxWork->slots[0];
    s32 result;

    if (gItemData[gSave.itemBox[slot].item].max == ITEM_MAX_UNLIMITED) {
        result = 0;
    } else if (gItemData[save->itemBox[slot].item].max == 1) {
        result = 0;
    } else {
        result = save->itemBox[slot].count != 1;
    }
    return result;
}

/* 1 when the stack at the item box cursor can be split and the pouch slot (pouch) has room for
 * at least two more of its item. */
s32 BoxMenu_CanTakeSeveral(void)
{
    SaveData *save = &gSave;
    s32 result;

    if (BoxMenu_CanSplit() == 0) {
        result = 0;
    } else {
        u8 i = gBoxWork->pouch;
        result = gItemData[save->pouch[i].item].max - save->pouch[i].count >= 2;
    }
    return result;
}

/* Sets the amount to move (amount) with the pad: PAD_LEFT one, PAD_RIGHT the most, PAD_UP and
 * PAD_DOWN one more or one less, from 1 to the stack's count and, unless noLimit, the room in
 * the pouch slot; atMax is set at the most. */
void BoxMenu_InputAmount(u16 pad, u8 noLimit)
{
    SaveData *save = &gSave;
    u8 max = gSave.itemBox[gBoxWork->slots[0]].count;
    s16 oldAmount;
    s16 *amountPtr;
    s32 buttons;

    if (noLimit == 0) {
        u8 i = gBoxWork->pouch;
        s32 room = gItemData[save->pouch[i].item].max - save->pouch[i].count;

        if (room < max) {
            max = room;
        }
    }
    buttons = pad;
    gBoxWork->atMax = 0;
    amountPtr = &gBoxWork->amount;
    oldAmount = *amountPtr;
    if (buttons & PAD_LEFT) {
        *amountPtr = 1;
    } else if (buttons & PAD_RIGHT) {
        *amountPtr = max;
    } else if (buttons & PAD_UP) {
        if (*amountPtr >= max) {
            *amountPtr = max;
            oldAmount = -1;
        } else {
            (*amountPtr)++;
        }
    } else if (buttons & PAD_DOWN) {
        if (*amountPtr > 1) {
            (*amountPtr)--;
        } else {
            oldAmount = -1;
        }
    }
    if (oldAmount < 0) {
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
    } else if (oldAmount != gBoxWork->amount) {
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
    }
    if (gBoxWork->amount >= max) {
        gBoxWork->atMax = 1;
    }
}

/* After equipment box slots first and second swapped their items, swaps the two slot numbers
 * wherever they are kept: in the worn equipment (SaveData.equipIdx) and in the ten saved sets. */
void Equip_SwapSlots(u16 first, u16 second)
{
    SaveData *save = &gSave;
    EquipSet *set;
    u32 i;
    u32 j;
    s32 firstIdx = first;

    if (firstIdx == gSave.equipIdx[SAVE_EQUIP_LEGS]) {
        save->equipIdx[SAVE_EQUIP_LEGS] = second;
    } else if (second == save->equipIdx[SAVE_EQUIP_LEGS]) {
        save->equipIdx[SAVE_EQUIP_LEGS] = first;
    }
    if (firstIdx == save->equipIdx[SAVE_EQUIP_HEAD]) {
        save->equipIdx[SAVE_EQUIP_HEAD] = second;
    } else if (second == save->equipIdx[SAVE_EQUIP_HEAD]) {
        save->equipIdx[SAVE_EQUIP_HEAD] = first;
    }
    if (firstIdx == save->equipIdx[SAVE_EQUIP_BODY]) {
        save->equipIdx[SAVE_EQUIP_BODY] = second;
    } else if (second == save->equipIdx[SAVE_EQUIP_BODY]) {
        save->equipIdx[SAVE_EQUIP_BODY] = first;
    }
    if (firstIdx == save->equipIdx[SAVE_EQUIP_ARMS]) {
        save->equipIdx[SAVE_EQUIP_ARMS] = second;
    } else if (second == save->equipIdx[SAVE_EQUIP_ARMS]) {
        save->equipIdx[SAVE_EQUIP_ARMS] = first;
    }
    if (firstIdx == save->equipIdx[SAVE_EQUIP_WAIST]) {
        save->equipIdx[SAVE_EQUIP_WAIST] = second;
    } else if (second == save->equipIdx[SAVE_EQUIP_WAIST]) {
        save->equipIdx[SAVE_EQUIP_WAIST] = first;
    }
    if (firstIdx == save->equipIdx[SAVE_EQUIP_WEAPON]) {
        save->equipIdx[SAVE_EQUIP_WEAPON] = second;
    } else if (second == save->equipIdx[SAVE_EQUIP_WEAPON]) {
        save->equipIdx[SAVE_EQUIP_WEAPON] = first;
    }
    set = save->equipSets;
    for (i = EQUIP_SET_COUNT; i != 0; i--, set++) {
        for (j = 0; j < SAVE_EQUIP_COUNT; j++) {
            if (firstIdx == set->idx[j]) {
                set->idx[j] = second;
            } else if (second == set->idx[j]) {
                set->idx[j] = first;
            }
        }
    }
}

/* The box cursor's input on the item box (kind 0) or the equipment box (kind 1) pages: moves
 * the cursor (BoxMenu_MoveCursor); PAD_SQUARE opens or closes the item's description, or the
 * equipment's detail with its levels (PAD_LEFT, PAD_RIGHT). Returns the pad for the caller
 * (PAD_CROSS: back, PAD_CIRCLE: pick), 0 when the input was used here. */
u16 BoxMenu_Input(u16 pad)
{
    ItemBoxMenu *menu = gBoxWork;
    s32 keys;

    if (menu->kind == BOX_KIND_ITEMS) {
        s32 buttons;
        u8 *helpPtr;

        buttons = pad;
        if (buttons & PAD_CROSS) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            return PAD_CROSS;
        }
        if (buttons & PAD_SQUARE) {
            helpPtr = &menu->help;
            if (*helpPtr != BOX_HELP_OFF) {
                *helpPtr = BOX_HELP_OFF;
            } else {
                *helpPtr = BOX_HELP_OPEN;
                gHelpColumn = 0;
            }
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
        }
        BoxMenu_MoveCursor(&gBoxWork->slots[0], pad);
        if (gBoxWork->help != BOX_HELP_OFF) {
            PouchMenu_DecoCursor(gSave.itemBox[gBoxWork->slots[0]].item, pad);
        }
    } else {
        u8 *helpPtr;
        s32 buttons;

        helpPtr = &menu->help;
        if (*helpPtr != BOX_HELP_OFF) {
            buttons = pad;
            if (buttons & PAD_CIRCLE) {
                *helpPtr = BOX_HELP_OFF;
                return PAD_CIRCLE;
            }
            if (buttons & (PAD_SQUARE | PAD_CROSS)) {
                *helpPtr = BOX_HELP_OFF;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else {
                Menu_CursorLeftRight(&menu->levelSel, buttons & (PAD_LEFT | PAD_RIGHT), menu->levelCount);
            }
            return 0;
        }
        keys = pad;
        if (keys & PAD_CROSS) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            return PAD_CROSS;
        }
        BoxMenu_MoveCursor(&menu->slots[1], pad);
        if (!(keys & PAD_CIRCLE) && (keys & PAD_SQUARE)) {
            gBoxWork->boxItem = &gSave.box[gBoxWork->slots[1]];
            menu = gBoxWork;
            if (menu->boxItem->used != 0) {
                menu->help = BOX_HELP_DETAIL;
                gBoxWork->levelSel = 0;
                gBoxWork->levelCount = Equip_LevelMark((struct EquipItem *)gBoxWork->boxItem, 0, (struct EquipItem *)(0));
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
            } else {
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
            }
            return 0;
        }
    }
    return pad;
}

s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
void Box_ApplySort(u8 equip);

/* ItemBoxMenu seen by BoxMenu_SortPrompt: the ItemBoxMenu form changes the code (MWCC reuses the load of gBoxWork) */
typedef struct BoxCursors {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ u16 slot[2];
} BoxCursors;

/* The box menus' sort prompt: PAD_SELECT opens a yes/no (prompt, yesNo); yes sorts the item box
 * (kind 0) or the equipment box (Box_ApplySort) and puts the cursor at the same place on the
 * first page. Returns 0 while the prompt takes the input, else the pad. */
u16 BoxMenu_SortPrompt(u16 pad, u8 kind)
{
    u8 *answer;
    BoxCursors *cursors;

    if (gBoxWork->prompt != 0) {
        answer = &gBoxWork->yesNo;
        if (*answer == 0) {
            if (pad & PAD_CIRCLE) {
                Box_ApplySort(kind);
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                cursors = (BoxCursors *)gBoxWork;
                cursors->slot[kind] = cursors->slot[kind] % BOX_PAGE_SLOTS;
                gBoxWork->prompt = 0;
                gBoxWork->yesNo = BOX_YESNO_NONE;
            } else {
                gHelpLine = 0x38;
                if (pad & PAD_CROSS) {
                    *answer = 1;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                } else if (pad & PAD_RIGHT) {
                    *answer = 1;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                }
            }
        } else if (pad & (PAD_CIRCLE | PAD_CROSS)) {
            gBoxWork->prompt = 0;
            gBoxWork->yesNo = BOX_YESNO_NONE;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        } else {
            gHelpLine = 0x38;
            if (pad & PAD_LEFT) {
                *answer = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
            }
        }
        return 0;
    }
    if (pad & PAD_SELECT) {
        gBoxWork->prompt = 1;
        gBoxWork->yesNo = BOX_YES;
        gBoxWork->help = BOX_HELP_OFF;
        gHelpLine = 0x38;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, 0xF, 0);
        return 0;
    }
    return pad;
}

/* 1 when the hunter can wear piece item with the weapon in equipment box slot *slot: a weapon
 * always, an armour piece when it fits the hunter's gender and that weapon's class. */
s32 Equip_ArmorFits(EquipItem *item, u16 *slot)
{
    SaveData *save;
    u8 flags;
    u8 bit;

    if (item->type == EQUIP_BLADE || item->type == EQUIP_GUN) {
        return 1;
    }
    save = &gSave;
    flags = Equip_GetData(item)->armor.flags;
    bit = (gSave.gender == 0) ? ARMOR_FLAG_MALE : ARMOR_FLAG_FEMALE;
    if (!(flags & bit)) {
        return 0;
    }
    bit = (save->box[*slot].type == EQUIP_BLADE) ? ARMOR_FLAG_BLADEMASTER : ARMOR_FLAG_GUNNER;
    if (!(flags & bit)) {
        return 0;
    }
    return 1;
}

/*
 * The equipment menu's yes/no prompt: reads the pad (*pad, from which the
 * circle, cross, left and right buttons are then removed). With yes
 * highlighted, PAD_CIRCLE answers yes (returns 1), PAD_CROSS or PAD_RIGHT
 * moves to no; with no highlighted, PAD_CIRCLE or PAD_CROSS answers no
 * (returns -1), PAD_LEFT moves to yes. 0 while unanswered.
 */
s32 Box_YesNoInput(u16 *pad)
{
    s32 buttons;
    s32 buttonsOnNo;
    u16 raw;

    raw = *pad;
    *pad = raw & (u16)~(PAD_CIRCLE | PAD_CROSS | PAD_LEFT | PAD_RIGHT);
    if (gBoxWork->yesNo == BOX_YES) {
        buttons = raw & 0xFFFF;
        if (buttons & PAD_CIRCLE) {
            gBoxWork->yesNo = BOX_YESNO_NONE;
            return 1;
        }
        if (buttons & PAD_CROSS) {
            gBoxWork->yesNo = BOX_NO;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        }
        if (buttons & PAD_RIGHT) {
            gBoxWork->yesNo = BOX_NO;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
        }
    } else {
        buttonsOnNo = raw & 0xFFFF;
        if (buttonsOnNo & (PAD_CIRCLE | PAD_CROSS)) {
            gBoxWork->yesNo = BOX_YESNO_NONE;
            return -1;
        }
        if (buttonsOnNo & PAD_LEFT) {
            gBoxWork->yesNo = BOX_YES;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
        }
    }
    return 0;
}

/* The file's initialised data, in the game's order */
u8 gBoxPartIdx[8] = {
#include "lobby/007F1A00.inc"
};
