/*
 * The item box menu's drawing, and the box draws other lobby menus share.
 * - BoxMenu_Draw draws the item box menu each frame (from the HUD's draw) while the town runs
 *   it, from its work (ItemBoxMenu, gBoxWork, box_menu.h), which BoxMenu_Open and
 *   BoxMenu_Update set: the action list (store, take, equip, arrange, sell), then the screen of
 *   the chosen action (BoxMenu_DrawStore ... BoxMenu_DrawSell), with the amount, the money, the
 *   help line and the yes/no answer.
 * - BoxMenu_DrawItemList draws a page of 100 slots of the item box (gSave.itemBox) or of the
 *   equipment box (gSave.box), with the cursor item's name, price and description; the lobby's
 *   equipment shops and workshops draw their lists with it. BoxMenu_DrawEquipGrid is the
 *   equipment set screen's smaller grid.
 * - The box list (BoxList, gBoxList): the equipment box slots of one class, seven to a page,
 *   for the equipment set screen.
 * - EquipDetail_Draw: the skill window of an item tried on (gSkillPreview), shared with the
 *   shops.
 * The item box menu's input and steps are the item box menu file's; the sheet helpers it draws
 * with are the menu sheets file's. The boxes' sort (Box_BuildSortedList, Box_ApplySort) and
 * the skill lists with an item (SkillList_DrawWithEquip, SkillList_DrawCompare) follow this
 * file.
 */
#include "common.h"
#include "window.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "sprite.h"
#include "lobby_globals.h"
#include "sound.h"
#include "texture.h"
#include "render_state.h"
#include "hud.h"
#include "prim.h"
#include "lobby_menu.h"
#include "area_draw.h"
#include "shake.h"
#include "text.h"
#include "box_skill_lists.h"
#include "chat_send.h"
#include "engine_math.h"
#include "equip_parts.h"
#include "font.h"
#include "member_card.h"
#include "menu_util.h"
#include "menu_widgets.h"
#include "name_tags.h"
#include "libc.h"

/* Compiled at optimization level 4: BoxMenu_DrawAmount and BoxMenu_DrawZenny match only
 * there, and every other function of the file matches at both levels. */
#pragma optimization_level 4

/* The icon tables are defined below as halfword arrays (the build's data takes that form), and
 * read as sprite.h's Icon records: */
extern s16 gBoxControlIcons[]; /* two sets of three Icons: the hint's places 0 and 1 */
extern s16 gBoxControlIconsB[];
extern s16 gBoxMenuIcon[];
extern Panel gEquipInfoWin;
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
extern ListPanel gEquipNonePanel;
extern Panel gEquipDetailWin[4];
#include "box_menu.h"
#include "lobby_labels.h"
extern ListPanel gBoxActionsPanel;
extern ListPanel gBoxListHeaders[2];

/* Draws spr (its position and size set by the caller) as the cursor's cell: the texels
 * (0, 0)-(32, 32) of the bound sheet at alpha, the colour pulsing with the frame counter
 * (a sine over 32 frames). */
void BoxMenu_DrawPulseSprite(Sprite *spr, s32 alpha)
{
    f32 wave;
    u32 phase;

    /* a colour that pulses with the frame counter */
    phase = (u16)((gFrameTick & 0x1F) << 11);
    wave = Math_Sin(ANGLE16_TO_RAD * phase);
    spr->color = ((u8)alpha << 24) | ((u8)((s8)(108.0f * wave) + 140) << 16) | ((u8)((s8)(6.0f * wave) + 248) << 8) | (u8)((s8)(38.0f * wave) + 214);
    *(u32 *)&spr->u0 = 0;
    *(u32 *)&spr->u1 = 0x200020;
    Sprite_Draw(spr);
}

extern u16 gFrameTick;

/* The arrange screen's held slot (ItemBoxMenu.held): a cell over its place on the box page's
 * grid of 10 by 10, its alpha pulsing with the frame counter. */
void BoxMenu_DrawGridCursor(void)
{
    Sprite cursor;
    s16 cell;
    f32 wave;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    cell = gBoxWork->held % BOX_PAGE_SLOTS;
    cursor.x = 313.0f + 28.8f * (cell % 10);
    cursor.y = (cell / 10) * 25 + 59;
    cursor.w = 32;
    cursor.h = 25;
    wave = Math_Sin(ANGLE16_TO_RAD * (u16)((gFrameTick & 0x1F) << 11));
    cursor.color = ((u8)((s8)(96.0f * wave) + 159) << 24) | 0xFF0000;
    *(u32 *)&cursor.u0 = 0;
    *(u32 *)&cursor.u1 = 0x200020;
    Sprite_Draw((struct Sprite *)&cursor);
}

/* BoxMenu_DrawItemList at x 306, the item box menu's place for it. */
void BoxMenu_DrawBoxPage(s16 cur, s32 flags)
{
    BoxMenu_DrawItemList(cur, (u32) flags, 306.0f);
}

extern Panel gBoxItemListWin;
void Window_DrawTextAlpha(Window *w, u8 *text, s32 alpha); /* unit's own declaration: alpha is an s32 here (the definition's is a u8), so the calls pass it unmasked, as the game's code does */
struct PrimLine;
struct Pt;
u8 Equip_GetRarity(u8 type, u16 id);
u32 Equip_Color(u8 rarity, s32 alpha, u8 alt); /* unit's own declaration: the definition takes (u8 rarity, u8 alpha, s32 alt); the item box's draws match only with alpha an s32, as the game passes it without masking it to a byte */
char *Equip_GetName(u8 type, u16 id);
extern u32 gBoxPageColors[];
extern u32 gItemColors[];
extern char *gItemNames[];

/* Draws page cur / 100 of the item box, or of the equipment box with BOX_LIST_EQUIP, at x
 * listX: a cell per slot in the page's colour (gBoxPageColors; a worn item's cell in another),
 * and each item's icon. With cur >= 0, the cursor's pulsing cell, the page bar, the cursor
 * item's name with its count (or the mark of an item without a limit), its price with
 * BOX_LIST_PRICE, and with BOX_LIST_DETAIL its description (items) or detail window (equipment,
 * its page in BOX_LIST_PAGE_MASK). BOX_LIST_DIM dims it all (alpha 96); BOX_LIST_MONEY adds
 * the money; BOX_LIST_SORTED draws the sorted copy (Box_BuildSortedList) without the cursor or
 * name; an item id (BOX_LIST_ITEM) keeps only its stacks bright and hides the cursor. For the
 * equipment, BOX_LIST_WORN_ALT draws worn items in the other colour, BOX_LIST_ARMOR the armour
 * and BOX_LIST_WEAPONS the weapons; each marks the worn items.
 * The item loops write the alpha byte as `(alpha % 256) << 24`: the compiler turns the unsigned
 * % 256 into a mask only after scheduling, so the mask stays in the register the game uses;
 * `(u8)alpha << 24` there allocates other registers. The equipment loop matches with the masked
 * alpha copied into alphaByte in its for. Equip_Color is declared with an s32 alpha for the
 * whole file, so alpha passes unmasked here and in BoxMenu_DrawEquipGrid (BoxMenu_DrawEquipPick
 * declares the definition's prototype in its own block). */
void BoxMenu_DrawItemList(s16 cur, u32 flags, f32 listX)
{
    SaveData *save;
    u8 equip;
    void *box;
    s32 price;
    u16 page;
    u16 idx;
    s16 cursorCell;
    Sprite sprite;
    char buf[48];
    ItemStack *stk;
    u16 item;
    ItemData *itemData;
    EquipItem *eqItem;
    PrimLine rect;
    s16 textColor;
    s32 cellSlot;
    u8 alt;
    u32 alpha;
    s32 dim;
    s32 mode;
    s32 iconCell;
    s32 noSave;
    f32 w;
    u8 max;
    u8 rare;
    s16 fullColor;
    s16 i;
    s32 iconSlot;
    u8 alphaByte;

    save = &gSave;
    equip = (flags & BOX_LIST_EQUIP) != 0;
    alpha = (u8)((flags & BOX_LIST_DIM) == 0 ? 255 : 96);
    if (!(flags & BOX_LIST_SORTED)) {
        if (equip == 0) {
            box = save->itemBox;
        } else {
            box = save->box;
        }
    } else {
        box = Box_BuildSortedList(equip);
        if (cur >= 0) {
            cur = cur % BOX_PAGE_SLOTS;
        }
    }
    gBoxItemListWin.x = listX;
    Window_DrawTextAlpha((Window *)&gBoxItemListWin, NULL, alpha);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    page = cur / BOX_PAGE_SLOTS;
    idx = page * BOX_PAGE_SLOTS;
    cursorCell = cur % BOX_PAGE_SLOTS;
    if (BOX_LIST_ITEM(flags) != 0) {
        cur = -1;
    }
    if (equip == 0) {
        for (i = 0; i < BOX_PAGE_SLOTS; i++) {
            rect.x0 = (s16)(153.0f + listX - 146.0f + 28.8f * (i % 10)) + 3;
            rect.x1 = 25.6f + rect.x0;
            rect.y0 = (i / 10) * 25 + 61;
            rect.y1 = rect.y0 + 21;
            rect.color = ((alpha % 256) << 24) | gBoxPageColors[page];
            Prim_FillRect(&rect);
        }
        sprite.w = 32;
        sprite.h = 25;
        stk = (ItemStack *)box + idx;
        for (i = 0; i < BOX_PAGE_SLOTS; i++, stk++) {
            item = stk->item;
            if (item != 0) {
                itemData = &gItemData[item];
                if (itemData->icon != 255) {
                    iconCell = itemData->icon + 1;
                    sprite.x = 153.0f + listX - 146.0f + 28.8f * (i % 10);
                    sprite.y = (i / 10) * 25 + 59;
                    sprite.u0 = ((iconCell & 7) << 5) + 1;
                    sprite.v0 = ((iconCell >> 3) << 5) + 1;
                    sprite.u1 = sprite.u0 + 30;
                    sprite.v1 = sprite.v0 + 30;
                    sprite.color = gItemColors[itemData->color];
                    sprite.color &= 0xFFFFFF;
                    if (BOX_LIST_ITEM(flags) == 0 || BOX_LIST_ITEM(flags) == item) {
                        sprite.color |= (alpha % 256) << 24;
                    } else {
                        sprite.color |= 0x50000000;
                    }
                    Sprite_Draw(&sprite);
                }
            }
        }
    } else {
        cellSlot = iconSlot = idx;
        for (i = 0, alphaByte = alpha; i < BOX_PAGE_SLOTS; i++, cellSlot++) {
            rect.color = (alphaByte << 24) | gBoxPageColors[page];
            if (!(flags & BOX_LIST_SORTED) && Equip_IsSlotEquipped(save, cellSlot) == 1) {
                rect.color = ((u8)alpha << 24) | 0x1078D4;
            }
            rect.x0 = (s16)(153.0f + listX - 146.0f + 28.8f * (i % 10)) + 3;
            rect.x1 = 25.6f + rect.x0;
            rect.y0 = (i / 10) * 25 + 61;
            rect.y1 = rect.y0 + 21;
            Prim_FillRect(&rect);
        }
        sprite.w = 32;
        sprite.h = 25;
        eqItem = (EquipItem *)box + idx;
        for (i = 0; i < BOX_PAGE_SLOTS; i++, iconSlot++, eqItem++) {
            if (eqItem->used) {
                sprite.x = 153.0f + listX - 146.0f + 28.8f * (i % 10);
                sprite.y = (i / 10) * 25 + 59;
                Equip_GetIconUV(eqItem, (struct Pt *)&sprite.u0, (struct Pt *)&sprite.u1);
                rare = Equip_GetRarity(eqItem->type, eqItem->id);
                if (flags & BOX_LIST_WORN_ALT) {
                    if (Equip_IsSlotEquipped(save, iconSlot) == 1) {
                        sprite.color = Equip_Color(rare, alpha, 1);
                        Sprite_Draw(&sprite);
                        *(u32 *)&sprite.u0 = 0x01200060;
                        *(u32 *)&sprite.u1 = 0x01400080;
                        sprite.color = ((u8)alpha << 24) | 0x909010;
                    } else {
                        sprite.color = Equip_Color(rare, alpha, 0);
                    }
                } else if (flags & BOX_LIST_ARMOR) {
                    alt = 0;
                    if (eqItem->type != EQUIP_BLADE && eqItem->type != EQUIP_GUN) {
                        alt = 1;
                    }
                    sprite.color = Equip_Color(rare, alpha, alt);
                    if (Equip_IsSlotEquipped(save, iconSlot) == 1) {
                        Sprite_Draw(&sprite);
                        *(u32 *)&sprite.u0 = 0x01200060;
                        *(u32 *)&sprite.u1 = 0x01400080;
                        if (alt == 0) {
                            sprite.color = ((u8)alpha << 24) | 0xFFFF00;
                        } else {
                            sprite.color = ((u8)alpha << 24) | 0x808010;
                        }
                    }
                } else if (flags & BOX_LIST_WEAPONS) {
                    alt = 0;
                    if (eqItem->type == EQUIP_BLADE || eqItem->type == EQUIP_GUN) {
                        alt = 1;
                    }
                    sprite.color = Equip_Color(rare, alpha, alt);
                    if (Equip_IsSlotEquipped(save, iconSlot) == 1) {
                        Sprite_Draw(&sprite);
                        *(u32 *)&sprite.u0 = 0x01200060;
                        *(u32 *)&sprite.u1 = 0x01400080;
                        if (alt == 0) {
                            sprite.color = ((u8)alpha << 24) | 0xFFFF00;
                        } else {
                            sprite.color = ((u8)alpha << 24) | 0x808010;
                        }
                    }
                } else {
                    sprite.color = Equip_Color(rare, alpha, 0);
                    if (!(flags & BOX_LIST_SORTED) && Equip_IsSlotEquipped(save, iconSlot) == 1) {
                        Sprite_Draw(&sprite);
                        *(u32 *)&sprite.u0 = 0x01200060;
                        *(u32 *)&sprite.u1 = 0x01400080;
                        sprite.color = ((u8)alpha << 24) | 0xFFFF00;
                    }
                }
                Sprite_Draw(&sprite);
            }
        }
    }
    if (cur >= 0) {
        if (!(flags & BOX_LIST_SORTED)) {
            sprite.x = 153.0f + listX - 146.0f + 28.8f * (cursorCell % 10);
            sprite.y = (cursorCell / 10) * 25 + 59;
            BoxMenu_DrawPulseSprite(&sprite, alpha);
        }
        Font_SetSpacing(18, 18);
        if (!(flags & BOX_LIST_DIM)) {
            textColor = 0;
            fullColor = 2;
        } else {
            textColor = 10;
            fullColor = 13;
        }
        Text_SetColor(textColor);
        BoxMenu_DrawPageTabs(listX, 27, page, flags, alpha, 0);
        if (equip == 0) {
            stk = (ItemStack *)box + cur;
            item = stk->item;
            if (!(flags & BOX_LIST_SORTED)) {
                if (item == 0) {
                    sprintf(buf, "%s", gItemNames[0]);
                    price = -1;
                } else {
                    max = gItemData[item].max;
                    switch (max) {
                    case 1:
                        sprintf(buf, "%s", gItemNames[item]);
                        break;
                    case ITEM_MAX_UNLIMITED:
                        sprintf(buf, "%s %s", gItemNames[item], gLobbyWords[LOBBY_WORD_INFINITY]);
                        break;
                    default:
                        if (stk->count >= max) {
                            sprintf(buf, "%s %s~C%02d%d~C%02d", gItemNames[item], gLobbyWords[LOBBY_WORD_TIMES], fullColor, stk->count, textColor);
                        } else {
                            sprintf(buf, "%s %s%d", gItemNames[item], gLobbyWords[LOBBY_WORD_TIMES], stk->count);
                        }
                        break;
                    }
                    price = gItemData[stk->item].value;
                }
            }
        } else {
            eqItem = (EquipItem *)box + cur;
            if (!eqItem->used) {
                sprintf(buf, "%s", gItemNames[0]);
                price = -1;
            } else {
                sprintf(buf, "%s", Equip_GetName(eqItem->type, eqItem->id));
                price = Equip_GetSellPrice(eqItem);
            }
        }
        if (!(flags & BOX_LIST_SORTED)) {
            Font_SetPos(5.0f + listX, 319);
            Text_Printf("%s", buf);
            if ((flags & BOX_LIST_PRICE) && price >= 0) {
                Font_SetSpacing(24, 18);
                Font_SetPos(198.0f + listX, 319);
                Text_PrintfColored("%7dz", price);
                Font_SetSpacing(18, 18);
            }
            if (flags & BOX_LIST_DETAIL) {
                if (equip == 0) {
                    stk = &save->itemBox[cur];
                    HelpWindow_Draw(HELP_PAGE_ITEMS, (u16)HELP_ITEM_LINE(stk->item), *(s8 *)&gHelpColumn);
                } else {
                    eqItem = &save->box[cur];
                    if (eqItem->used) {
                        EquipWindow_DrawLocal((struct EquipItem *)eqItem, listX, 184, flags & 0x47, (struct EquipItem *)(0));
                    }
                }
            }
        }
    } else if (BOX_LIST_ITEM(flags) != 0) {
        BoxMenu_DrawPageTabs(listX, 27, page, flags, alpha, 0);
    }
    if (flags & BOX_LIST_MONEY) {
        Menu_DrawMoney(160.0f + listX, 19, (flags & BOX_LIST_DIM) != 0);
    }
}

extern Panel gBoxPageTabsWin;

/* The page bar of a box list at (x, y), drawn only when the box has more than one page
 * (Box_GetPageCount): the tabs (gBoxPageTabs[kind], 0 the item list's, 1 the equipment
 * grid's, over their window when it has one), the arrows to either side, dim while the list is
 * (BOX_LIST_DIM or BOX_LIST_SORTED), and "page + 1 / pages" (gPageFmt). flags are the list's. */
void BoxMenu_DrawPageTabs(s16 x, s16 y, s16 page, u32 flags, s32 alpha, u8 kind)
{
    Sprite sprite;
    s32 arrowColor;
    BoxTab *layout;
    s16 pageCount;

    pageCount = Box_GetPageCount(&gSave);
    if (pageCount <= 1) {
        return;
    }
    layout = &gBoxPageTabs[kind];
    if (layout->framed != 0) {
        gBoxPageTabsWin.x = x;
        gBoxPageTabsWin.y = y + 2;
        Window_DrawTextAlpha((Window *)&gBoxPageTabsWin, NULL, alpha);
    } else {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        Tex_Prepare(1, TEX_SLOT_ICONS);
        Tex_Bind(TEX_SLOT_ICONS);
    }
    if (!(flags & (BOX_LIST_SORTED | BOX_LIST_DIM))) {
        sprite.color = -1;
        arrowColor = 0xFF20FF28;
    } else {
        sprite.color = 0x60FFFFFF;
        arrowColor = 0xA0606060;
    }
    sprite.y = y;
    sprite.w = layout->tabW;
    sprite.h = 20;
    sprite.x = x + layout->leftX;
    *(s32 *)&sprite.u0 = 121;
    *(s32 *)&sprite.u1 = 0x180090;
    Sprite_Draw(&sprite);
    sprite.x = x + layout->rightX;
    sprite.u0 = 169;
    sprite.u1 = 192;
    Sprite_Draw(&sprite);
    Menu_DrawArrow(x, y, layout->arrowW, 18, arrowColor, ARROW_MIRROR);
    Menu_DrawArrow(x + layout->rightArrowX, y, layout->arrowW, 18, arrowColor, 0);
    Font_SetSpacing(18, 18);
    Font_SetPos((s16)(x + layout->textX), y);
    Text_PrintfColored(gPageFmt, page + 1, pageCount);
}

extern Panel gBoxEquipGridWin;

/* The equipment set screen's grid of the equipment box: page cur / 100 at (gridX, y), a cell of
 * 25 by 17 per slot in the page's colour (a worn item's cell in another), the icons in their
 * rarity's colour and the worn items marked. With BOX_LIST_UNFIT_ALT, worn weapons, and items that
 * do not fit with the worn ones (Equip_ArmorFits), are drawn in the other colour. With
 * cur >= 0: the pulsing cursor and the item's name under the grid (not for the sorted copy,
 * BOX_LIST_SORTED), and the page bar. BOX_LIST_DIM dims it. It sets gBoxItemListWin.x (not the grid
 * window's) to y, as the game does. */
void BoxMenu_DrawEquipGrid(f32 gridX, s16 y, s16 cur, u32 flags)
{
    EquipItem *box;
    u16 page;
    s16 cursorCell;
    s32 sel;
    s16 i;
    s32 alt;
    EquipItem *eqItem;
    u32 *uvEnd;
    u16 idx;
    s32 dim;
    f32 cellsX;
    f32 baseX;
    s32 cellSlot;
    s32 iconSlot;
    s32 alpha;
    PrimLine rect;
    Sprite sprite;
    u8 rare;
    SaveData *save;
    u16 *equipped;

    save = &gSave;
    alpha = (u8)((flags & BOX_LIST_DIM) == 0 ? 255 : 96);
    if (!(flags & BOX_LIST_SORTED)) {
        box = save->box;
    } else {
        box = Box_BuildSortedList(1);
        if (cur >= 0) {
            cur = cur % BOX_PAGE_SLOTS;
        }
    }
    gBoxItemListWin.x = y;
    equipped = save->equipIdx;
    Window_DrawTextAlpha((Window *)&gBoxEquipGridWin, NULL, alpha);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    page = cur / BOX_PAGE_SLOTS;
    idx = page * BOX_PAGE_SLOTS;
    cellSlot = idx;
    cursorCell = cur % BOX_PAGE_SLOTS;
    for (i = 0; i < BOX_PAGE_SLOTS; i++, cellSlot++) {
        rect.color = ((u8)alpha << 24) | gBoxPageColors[page];
        if (!(flags & BOX_LIST_SORTED) && List6_Contains(save->equipIdx, cellSlot) == 1) {
            rect.color = ((u8)alpha << 24) | 0x1078D4;
        }
        rect.x0 = (s16)(117.0f + gridX - 116.0f + 25.0f * (i % 10)) + 1;
        rect.x1 = 1.0f + (22.2f + rect.x0);
        rect.y0 = y + (i / 10) * 17 + 1;
        rect.y1 = rect.y0 + 16;
        Prim_FillRect(&rect);
    }
    sprite.w = 27;
    sprite.h = 17;
    eqItem = &box[idx];
    /* The second corner's uv word is written through a pointer kept in a register, as the
     * game's code does; written as *(u32 *)&sprite.u1 the stores are scheduled otherwise. */
    uvEnd = (u32 *)&sprite.u1;
    baseX = 117.0f + gridX;
    cellsX = baseX - 116.0f;
    for (i = 0, iconSlot = idx; i < BOX_PAGE_SLOTS; i++, iconSlot++, eqItem++) {
        if (eqItem->used) {
            sprite.x = cellsX + 25.0f * (i % 10);
            sprite.y = y + (i / 10) * 17;
            Equip_GetIconUV(eqItem, (struct Pt *)&sprite.u0, (struct Pt *)&sprite.u1);
            rare = Equip_GetRarity(eqItem->type, eqItem->id);
            if (flags & BOX_LIST_UNFIT_ALT) {
                sprite.color = Equip_Color(rare, alpha, 0);
                if (List6_Contains(equipped, iconSlot) == 1) {
                    if (eqItem->type == EQUIP_BLADE || eqItem->type == EQUIP_GUN) {
                        sprite.color = Equip_Color(rare, alpha, 1);
                        Sprite_Draw(&sprite);
                        sprite.color = ((u8)alpha << 24) | 0x808010;
                    } else {
                        Sprite_Draw(&sprite);
                        sprite.color = ((u8)alpha << 24) | 0xFFFF00;
                    }
                    *(u32 *)&sprite.u0 = 0x01290061;
                    *uvEnd = 0x013F007F;
                } else if (!Equip_ArmorFits(eqItem, equipped)) {
                    sprite.color = Equip_Color(rare, alpha, 1);
                }
            } else {
                sprite.color = Equip_Color(rare, alpha, 0);
                if (!(flags & BOX_LIST_SORTED) && List6_Contains(equipped, iconSlot) == 1) {
                    Sprite_Draw(&sprite);
                    *(u32 *)&sprite.u0 = 0x01290061;
                    *uvEnd = 0x013F007F;
                    sprite.color = ((u8)alpha << 24) | 0xFFFF00;
                }
            }
            Sprite_Draw(&sprite);
        }
    }
    if (cur >= 0) {
        eqItem = &box[cur];
        Text_SetColor((u8)((flags & BOX_LIST_DIM) == 0 ? 0 : 10));
        if (!(flags & BOX_LIST_SORTED)) {
            sprite.x = (s16)(baseX - 116.0f + 25.0f * (cursorCell % 10)) - 1;
            sprite.y = y + (cursorCell / 10) * 17 - 1;
            sprite.h = 19;
            BoxMenu_DrawPulseSprite(&sprite, alpha);
            Font_SetSpacing(14, 18);
            Font_SetPos(gridX, y + 172);
            if (!eqItem->used) {
                Text_Draw(*(char * *)&gItemNames);
            } else {
                Text_Draw(Equip_GetName(eqItem->type, eqItem->id));
            }
        }
        BoxMenu_DrawPageTabs(146.0f + gridX, y + 172, page, flags, alpha, 1);
    }
}

extern s8 gBoxTypeKinds[];

/* Builds gBoxList: the equipment box slots of one class (kind below 6; a type's class is
 * gBoxTypeKinds[type]) in box order, or sorted by BoxEquip_SortKey (BoxList_InsertSorted),
 * with count, pages (7 entries a page) and lastCount. Returns the pages; -1 (and invalid set)
 * for a bad kind. */
s8 BoxList_Build(u8 kind, u8 sorted)
{
    BoxList *base = (BoxList *)&gBoxList; /* the cast keeps the address in a register */
    EquipItem *eqItem;
    u32 i;
    u32 found;
    u32 slotCount;
    u16 *out;

    if (kind > 5) {
        base->invalid = 1;
        return -1;
    }
    base->invalid = 0;
    base->kind = kind;
    base->sorted = sorted;
    eqItem = gSave.box;
    out = base->idx;
    slotCount = Box_GetPageCount(&gSave) * BOX_PAGE_SLOTS;
    for (i = 0, found = 0; i < slotCount; i++, eqItem++) {
        if (eqItem->used != 0 && gBoxTypeKinds[eqItem->type] == kind) {
            if (base->sorted == 0) {
                *out++ = i;
            } else {
                BoxList_InsertSorted(base->idx, i, found);
            }
            found++;
        }
    }
    base->count = found;
    if (found != 0) {
        base->pages = (found + 6) / 7;
        base->lastCount = found - (base->pages - 1) * 7;
    } else {
        base->lastCount = 0;
        base->pages = 0;
    }
    return base->pages;
}

/* Inserts the box slot idx into list, n entries sorted by BoxEquip_SortKey: before the first
 * entry with a greater key, the rest moving up one. */
void BoxList_InsertSorted(u16 *list, u16 idx, u16 n)
{
    SaveData *save = &gSave;
    u32 key;
    u16 moved;

    key = BoxEquip_SortKey(&save->box[idx]);
    while (n > 0) {
        if (key < BoxEquip_SortKey(&save->box[*list])) {
            while (n > 0) {
                moved = *list;
                n--;
                *list = idx;
                idx = moved;
                list++;
            }
            break;
        }
        n--;
        list++;
    }
    *list = idx;
}

/* Moves the box list's cursor (*row and *page, 7 rows a page) by the pad: PAD_LEFT and
 * PAD_RIGHT turn the page, PAD_UP and PAD_DOWN move the row, both wrapping, with their
 * sounds. Returns the box slot there, -1 for an empty list. */
s16 BoxList_MoveCursor(u8 *row, u8 *page, u16 pad)
{
    BoxList *base = (BoxList *)&gBoxList; /* the cast keeps the address in a register */
    s16 pageIdx;
    s16 rowIdx;
    s16 last;

    if (base->invalid != 0 || base->count == 0) {
        *row = 0;
        *page = 0;
        return -1;
    }
    rowIdx = *row;
    pageIdx = *page;
    if ((pad & (PAD_LEFT | PAD_RIGHT)) && base->pages > 1) {
        if (pad & PAD_LEFT) {
            if (--pageIdx < 0) {
                pageIdx = base->pages - 1;
            }
        } else if (++pageIdx >= base->pages) {
            pageIdx = 0;
        }
        if (pageIdx == base->pages - 1 && rowIdx >= base->lastCount) {
            *row = rowIdx = base->lastCount - 1;
        }
        *page = pageIdx;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
    } else if (pad & (PAD_UP | PAD_DOWN)) {
        last = pageIdx >= base->pages - 1 ? base->lastCount : 7;
        if (pad & PAD_UP) {
            if (--rowIdx < 0) {
                rowIdx = last - 1;
            }
        } else if (++rowIdx >= last) {
            rowIdx = 0;
        }
        *row = rowIdx;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
    }
    return base->idx[rowIdx + pageIdx * 7];
}

/* Points *list at page `page` of gBoxList (the last page when past it) and returns its entries:
 * 7, or lastCount on the last page; 0 for an empty or bad list. */
u16 BoxList_GetPage(u16 **list, u8 page)
{
    BoxList *base = (BoxList *)&gBoxList; /* the cast keeps the address in a register */
    s32 last;
    s16 entries;

    if (base->invalid != 0 || base->count == 0) {
        return 0;
    }
    last = base->pages - 1;
    entries = 7;
    if (page >= last) {
        page = last;
        entries = base->lastCount;
    }
    *list = &base->idx[page * 7];
    return entries;
}

/* The arrow over the skill list's column of the item's part (weapon, head, body, arms, waist,
 * legs: from x + 89, 32 apart), its colour pulsing with the frame counter. */
void SkillList_DrawPartArrow(s16 x, s16 y, EquipItem *item)
{
    f32 wave;
    u8 shade;

    switch (item->type) {
    default: /* EQUIP_BLADE, EQUIP_GUN: the weapon's column */
        x += 89;
        break;
    case EQUIP_HEAD:
        x += 121;
        break;
    case EQUIP_BODY:
        x += 153;
        break;
    case EQUIP_ARMS:
        x += 185;
        break;
    case EQUIP_WAIST:
        x += 217;
        break;
    case EQUIP_LEGS:
        x += 249;
        break;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    wave = Math_Sin(ANGLE16_TO_RAD * (u32)(u16)((gFrameTick & 0x1F) << 11));
    shade = (s8)(32.0f * wave) + 32;
    Menu_DrawArrow(x, y + 4, 32, 16, 0xFF000000 | (shade << 16) | (shade << 8) | (u8)((s8)(31.0f * wave) + 223), ARROW_MIRROR | ARROW_TEXRECT);
}

extern s16 gEquipDetailIcons[];
extern ListPanel gEquipDetailPanel;
void SkillList_DrawPage(s16 x, s16 y, s32 page, s32 mark); /* unit's own declaration: page and mark are s32 here (the definition's are u8s), so the call passes them unmasked, as the game's code does */
void SkillList_DrawPartArrow(s16 x, s16 y, EquipItem *item);
void Player_DrawSkillsPlain(Player *pl, s16 x, s16 y); /* unit's own declaration: x is an s16 here (the definition's is an s32); EquipDetail_Draw's call matches only so */

/* The skill window at (x, y) for pl, from gSkillPreview. If the item tried on does not fit, the
 * empty window (EquipDetail_DrawNone). Else the title, with "page / pages" of the skill list
 * when it has several (flags & 0xF the page; flag 0x80000000 picks the other title format and
 * adds the page icons), the skill list's page, the arrow over the item's part, then the active
 * skills: with the item (SkillList_DrawWithEquip), against the compared player
 * (SkillList_DrawCompare), or pl's own (Player_DrawSkillsPlain). */
void EquipDetail_Draw(Player *pl, s16 x, s16 y, u32 flags)
{
    SkillPreview *preview = (SkillPreview *)&gSkillPreview;
    char buf[64];
    s32 page;
    u32 arrows;
    s32 skillsY;

    if (preview->wearable == 0) {
        EquipDetail_DrawNone(x, y);
        return;
    }
    gEquipDetailPanel.x = x;
    gEquipDetailPanel.y = y;
    page = flags & 0xF;
    if (preview->pages > 1) {
        arrows = flags & 0x80000000;
        sprintf(buf, gStatusPageFmt[(u8)(arrows ? 10 : 13)], (u8)page + 1, preview->pages);
        ListPanel_Draw(&gEquipDetailPanel, 0, buf, -1);
        Menu_DrawFrame(&gEquipDetailPanel, 0);
        if (arrows) {
            Draw_Icons((Icon *)gEquipDetailIcons, 2);
        }
    } else {
        ListPanel_Draw(&gEquipDetailPanel, 0, gSkillDetailTitle, -1);
    }
    SkillList_DrawPage(x, y, page, preview->torsoUp);
    if (preview->item.used != 0) {
        SkillList_DrawPartArrow(x, y + 20, &preview->item);
    }
    gEquipDetailWin[0].x = x;
    gEquipDetailWin[0].y = skillsY = y + 232;
    Window_DrawText(&gEquipDetailWin[0], (s16 *)0, (u8 *)gActiveSkillsTitle);
    if (preview->item.used != 0) {
        SkillList_DrawWithEquip(pl, &preview->item, x, skillsY);
    } else if (preview->compare != NULL) {
        SkillList_DrawCompare(preview->compare, x, skillsY);
    } else {
        Player_DrawSkillsPlain(pl, x, skillsY);
    }
}

/* The skill window when the item tried on does not fit: the dim title and the empty active
 * skills window at (x, y). */
void EquipDetail_DrawNone(s16 x, s16 y)
{
    gEquipNonePanel.x = x;
    gEquipNonePanel.y = y;
    ListPanel_Draw(&gEquipNonePanel, 0, gSkillDetailTitleDim, -1);
    gEquipDetailWin[1].x = x;
    gEquipDetailWin[1].y = ((s16)(y)) + 232;
    Window_DrawText(&gEquipDetailWin[1], (s16 *)0, (u8 *)gActiveSkillsTitle);
}

/* The control hint (three button icons, the START button and the camera guide) in one of three
 * places: 0 the top left, under the action list; 1 the bottom right, while the menu is hidden
 * (START shows it again); 2 lower on the left, on the equipment screen. */
void BoxMenu_DrawControls(s32 where)
{
    s32 place;

    place = where & 0xFF;
    switch (place) {
    case 0:
        Gfx_DrawPulseGradient(0, 144, 250, 36, 0x80006000, 0);
        Draw_Icons((Icon *)gBoxControlIcons, 3);
        Text_SetColor(0);
        Font_SetSpacing(16, 14);
        Font_SetPos(40, 164);
        Text_Draw((char *) (gStartText));
        Font_SetSpacing(18, 18);
        Font_SetPos(60, 144);
        Text_Draw(gBoxControlTexts[BOX_CONTROL_TEXT_CAMERA]);
        Font_SetPos(110, 162);
        Text_Draw(gBoxControlTexts[BOX_CONTROL_TEXT_HIDE_MENU]);
        break;
    case 1:
        Gfx_DrawPulseGradient(420, 365, 220, 36, 0x80006000, 1);
        Draw_Icons((Icon *)gBoxControlIcons + 3, 3);
        Text_SetColor(0);
        Font_SetSpacing(16, 14);
        Font_SetPos(456, 384);
        Text_Draw((char *) (gStartText));
        Font_SetSpacing(18, 18);
        Font_SetPos(476, 364);
        Text_Draw(gBoxControlTexts[BOX_CONTROL_TEXT_CAMERA]);
        Font_SetPos(526, 382);
        Text_Draw(gMenuShowTexts[0]);
        break;
    case 2:
        Gfx_DrawPulseGradient(0, 232, 250, 36, 0x80006000, 0);
        Draw_Icons((Icon *)gBoxControlIconsB, 3);
        Text_SetColor(0);
        Font_SetSpacing(16, 14);
        Font_SetPos(40, 252);
        Text_Draw((char *) (gStartText));
        Font_SetSpacing(18, 18);
        Font_SetPos(60, 232);
        Text_Draw(gBoxControlTexts[BOX_CONTROL_TEXT_CAMERA]);
        Font_SetPos(110, 250);
        Text_Draw(gBoxControlTexts[BOX_CONTROL_TEXT_HIDE_MENU]);
        break;
    }
}

/* The action list (store, take, equip, arrange, sell: gBoxActionsPanel) with its cursor on
 * ItemBoxMenu.menu, sliding with its window (shake 2); its cursor grey when dimmed. The
 * control hint shows once the list stands still. */
void BoxMenu_DrawActions(s32 dimmed)
{
    s16 *pos;

    pos = Shake_GetOffset(2);
    if (Shake_Value(2) == 0) {
        BoxMenu_DrawControls(0);
    }
    if ((u8)dimmed == 0) {
        gBoxActionsPanel.cursorColor = 0xA9182;
    } else {
        gBoxActionsPanel.cursorColor = 0x808080;
    }
    ListPanel_Draw(&gBoxActionsPanel, pos, 0, gBoxWork->menu);
}

/* Draws the item box menu (the HUD's draw calls it every frame) while the town is in the item
 * box (TOWN_MODE_BOX_MENU) and BoxMenu_Update has run: with the menu's input off, only the hint
 * that START shows it; the help line, its icon and the mode name while they show; then by
 * ItemBoxMenu.state the action list, the chosen action's screen, or the list as it closes. */
void BoxMenu_Draw(void)
{
    u8 helpMode;
    u8 menuIdx;
    u8 menuState;

    if ((gBoxWork->updated != 0) && (Town_IsMode(29) != 0)) {
        if (!(gBoxWork->inputOn & 1)) {
            BoxMenu_DrawControls(1);
            return;
        }
        if ((gHelpShow != 0) && ((helpMode = gBoxWork->help, ((helpMode & BOX_HELP_HIDE_LINE) != 0)) || (helpMode == BOX_HELP_OFF))) {
            HelpWindow_DrawCurrent();
            if ((gBoxWork->blink & 0x1F) < 20) {
                Draw_Icons((Icon *)gBoxMenuIcon, 1);
            }
            if (gBoxWork->yesNo != BOX_YESNO_NONE) {
                BoxMenu_DrawYesNo();
            }
        }
        menuState = gBoxWork->state;
        switch (menuState) {
        case BOX_STATE_LIST:
            BoxMenu_DrawActions(0);
            return;
        case BOX_STATE_SUB:
            menuIdx = gBoxWork->menu;
            switch (menuIdx) {
            case BOX_MENU_STORE:
                BoxMenu_DrawStore();
                return;
            case BOX_MENU_TAKE:
                BoxMenu_DrawTake();
                return;
            case BOX_MENU_EQUIP:
                BoxMenu_DrawEquip();
                return;
            case BOX_MENU_ARRANGE:
                BoxMenu_DrawArrange();
                return;
            case BOX_MENU_SELL:
                BoxMenu_DrawSell();
                return;
            }
            break;
        case BOX_STATE_CLOSING:
            BoxMenu_DrawActions(1);
            break;
        }
    }
}

/* The store screen: step 0 the pouch with its cursor and help line (once the pouch window
 * stands still), or the sorted pouch while the sort prompt is up; step 1 the box, the stored
 * item's stacks bright, for the slot to store into; step 2 the pouch without a cursor. */
void BoxMenu_DrawStore(void)
{
    s32 cursorColor;
    s32 flags;
    u8 storeStep;

    BoxMenu_DrawActions(1);
    cursorColor = 0;
    storeStep = gBoxWork->step;
    flags = ITEM_LIST_SLIDE;
    switch (storeStep) {
    case BOX_STORE_PICK:
        flags = ITEM_LIST_SORTED;
        switch (gBoxWork->prompt) {
        case 0:
            if (Shake_Value(SHAKE_POUCH_MENU) == 0) {
                BoxMenu_DrawHelp(gBoxWork->help, gBoxWork->pouch);
            }
            cursorColor = 0xA9182;
            flags = ITEM_LIST_SLIDE | ITEM_LIST_SORT_HINT | ITEM_LIST_FRAME;
            break;
        }
        /* falls through: step 0 draws the pouch as step 2 does */
    case BOX_STORE_CLOSING:
        ItemList_DrawPouch((s32) gBoxWork->pouch, cursorColor, flags);
        break;
    case BOX_STORE_SHOW:
        BoxMenu_DrawBoxPage((s32) gBoxWork->slots[0], BOX_LIST_ITEM_FLAG(gBoxWork->pouchItem));
        break;
    }
}

/* The take screen: step 0 the box (with the cursor item's description when the help is on,
 * the sorted copy while the sort prompt is up, else the details and sort hints); step 1 the
 * box dimmed, the pouch with its cursor and the help line; step 2 the amount; steps 3 and 4
 * both lists dimmed. */
void BoxMenu_DrawTake(void)
{
    s32 flags;
    s32 step;

    BoxMenu_DrawActions(1);
    switch (step = gBoxWork->step) {
    case BOX_TAKE_PICK:
        if (gBoxWork->help != BOX_HELP_OFF) {
            flags = BOX_LIST_DETAIL;
        } else if (gBoxWork->prompt != 0) {
            flags = BOX_LIST_SORTED;
        } else {
            flags = 0;
            ButtonHint_DrawDetails();
            ButtonHint_DrawSort(0);
        }
        BoxMenu_DrawBoxPage(gBoxWork->slots[0], flags);
        break;
    case BOX_TAKE_SLOT:
        BoxMenu_DrawBoxPage(gBoxWork->slots[0], BOX_LIST_DIM);
        ItemList_DrawPouch(gBoxWork->pouch, 0xA9182, ITEM_LIST_FRAME);
        BoxMenu_DrawHelp(gBoxWork->help, gBoxWork->pouch);
        break;
    case BOX_TAKE_AMOUNT:
        BoxMenu_DrawAmount();
        BoxMenu_DrawBoxPage(gBoxWork->slots[0], BOX_LIST_DIM);
        ItemList_DrawPouch(gBoxWork->pouch, 0x808080, 0);
        break;
    case BOX_TAKE_SWAP:
    case BOX_TAKE_RESULT:
        BoxMenu_DrawBoxPage(gBoxWork->slots[0], BOX_LIST_DIM);
        ItemList_DrawPouch(gBoxWork->pouch, 0x808080, 0);
        break;
    }
}

extern ListWin gBoxEquipPanel;
extern ListPanel gBoxEquipRows;
extern u32 gPlayerColorWords[];
struct ListWin;

/* The equipment menu's draw (ItemBoxMenu.menu 2). Step 0: the menu list and the equipment
 * menu's entries (gBoxEquipPanel: 4, or 5 with the symbol colours). Step 1, by the entry (kind):
 * - 0, equipping from the box: the box's grid (BoxMenu_DrawEquipGrid) with the item at the
 *   cursor compared with the worn one (EquipCompare_Draw), or, with the details on (help), the
 *   item's skills (EquipDetail_Draw); the hints, or the guide line while the details show.
 * - 1-3, the equipment sets: the set as a list with its info page (BoxMenu_DrawEquipInfo,
 *   BoxMenu_DrawEquipList, EquipInfo_DrawPage); while a part is chosen (kind 1, sub 1), the
 *   box's items for that part (BoxMenu_DrawEquipPick); kind 2 compares the set with the player.
 * - 4, the symbol colours: a diamond mark in each colour (row 11 cycles, Color_Cycle8) and the
 *   colour's name, in gBoxEquipRows. */
void BoxMenu_DrawEquip(void)
{
    SaveData *save = &gSave;
    u32 flags;
    s16 pickSel;
    u8 cmpFlags;
    u32 i;
    s16 y;
    s16 rowStep;

    Font_SetSpacing(18, 18);
    switch (gBoxWork->step) {
    case 0:
        BoxMenu_DrawActions(1);
        gBoxEquipPanel.lines = gBoxWork->setEntries;
        ListPanel_DrawDim(&gBoxEquipPanel, 0, gBoxWork->kind);
        break;
    case 1:
        switch (gBoxWork->kind) {
        case EQUIP_MENU_PIECES:
            if (gChatSendTo == 0 || gChatWinTimer == 0) {
                BoxMenu_DrawControls(2);
            }
            switch (gBoxWork->sub) {
            case 0:
                if (gBoxWork->prompt != 0) {
                    flags = BOX_LIST_SORTED;
                } else {
                    flags = BOX_LIST_UNFIT_ALT;
                    if (gBoxWork->help == BOX_HELP_OFF) {
                        BoxMenu_DrawSkillHint();
                        ButtonHint_DrawSort(2);
                    } else {
                        Font_SetPos(540, 394);
                        Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_BACK]);
                    }
                }
                break;
            case 1:
            case 2:
                flags = BOX_LIST_UNFIT_ALT;
                break;
            }
            if (gBoxWork->help == BOX_HELP_OFF) {
                cmpFlags = gBoxWork->levelSel | 0x40;
                if (gBoxWork->cursorClass == BOX_CLASS_ARMOR) {
                    cmpFlags |= 0x10;
                    if (gBoxWork->canEquip == 0) {
                        cmpFlags |= 8;
                    }
                }
                if (gBoxWork->boxItem != NULL) {
                    if (gBoxWork->boxItem->used != 0) {
                        EquipCompare_Draw(gBoxWork->equipped, gBoxWork->boxItem, 306, 56, cmpFlags);
                    } else {
                        StatusPage_DrawBoxes(306, 56, 255);
                    }
                } else {
                    EquipCompare_Draw(gBoxWork->equipped, NULL, 306, 56, cmpFlags);
                }
            } else if (gBoxWork->canEquip != 0) {
                EquipDetail_Draw(gBoxWork->player, 306, 62, gBoxWork->skillPage | 0x80000000);
            } else {
                EquipDetail_DrawNone(306, 62);
            }
            BoxMenu_DrawEquipGrid(28.0f, 28, (s16)gBoxWork->slots[1], flags);
            break;
        case EQUIP_MENU_WEAR_SET:
            flags = 0;
            switch (gBoxWork->sub) {
            case 0:
                flags = 3;
                break;
            case 1:
                break;
            }
            BoxMenu_DrawActions(1);
            BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, flags, gBoxWork->player);
            break;
        case EQUIP_MENU_EDIT_SET:
            switch (gBoxWork->sub) {
            case 0:
                BoxMenu_DrawActions(1);
                BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, 3, 0);
                break;
            case 1:
                switch (gBoxWork->sub2) {
                case 0:
                    BoxMenu_DrawEquipInfo(gBoxWork->cur.idx, (s8)gBoxWork->part, 6, 0);
                    pickSel = -1;
                    flags = 1;
                    break;
                case 1:
                    BoxMenu_DrawEquipList(gBoxWork->cur.idx, gBoxWork->part, 14);
                    Font_SetSpacing(18, 18);
                    Text_SetColor(0);
                    Font_SetPos(504, 398);
                    if (gBoxWork->help == BOX_HELP_OFF) {
                        Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_DETAILS]);
                        EquipInfo_DrawPage(218, 1, 0, 0);
                    } else {
                        Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_BACK_GUIDE]);
                        EquipWindow_DrawLocal((struct EquipItem *)gBoxWork->boxItem, 306, 218, (gBoxWork->levelSel & BOX_LIST_PAGE_MASK) | 0x40, (struct EquipItem *)NULL);
                    }
                    flags = 0;
                    pickSel = (s16)gBoxWork->row;
                    if (*(u16 *)&gHelpLine == HELP_LINE_NONE) {
                        Font_SetSpacing(18, 18);
                        Text_SetColor(0);
                        Font_SetPos(306, 358);
                        Text_PrintfColored(gBoxMenuTexts[BOX_MENU_TEXT_PICK_PART_FMT], gEquipSlotTexts[gBoxWork->part]);
                    }
                    break;
                case 2:
                    BoxMenu_DrawEquipList(gBoxWork->cur.idx, gBoxWork->part, 4);
                    EquipInfo_DrawPage(218, 1, 0, 0);
                    pickSel = -1;
                    flags = 1;
                    break;
                }
                BoxMenu_DrawEquipPick((EquipIdx *)gBoxWork->cur.idx, pickSel, flags);
                break;
            case 2:
            case 3:
            case 4:
            case 5:
                BoxMenu_DrawActions(1);
                EquipInfo_DrawPage(198, BoxMenu_DrawEquipList(gBoxWork->cur.idx, -1, 0), 0, 0);
                break;
            }
            break;
        case EQUIP_MENU_SAVE_SET:
            BoxMenu_DrawActions(1);
            switch (gBoxWork->sub) {
            case 0:
                BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, 3, 0);
                break;
            case 1:
                BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, 0, 0);
                break;
            default:
                EquipInfo_DrawPage(198, BoxMenu_DrawEquipList(gBoxWork->cur.idx, -1, 0), 0, 0);
                break;
            }
            break;
        case EQUIP_MENU_SYMBOL_COLOR:
            BoxMenu_DrawActions(1);
            rowStep = gBoxEquipRows.charH + 4;
            y = gBoxEquipRows.y + rowStep + 2;
            for (i = 0; i < gBoxWork->count; i++) {
                Font_SetSpacing(20, 20);
                Font_SetPos(gBoxEquipRows.x - 1, y - 1);
                Font_SetColor(0x807F7F7F);
                Text_PrintfColored(gBoxMenuTexts[BOX_MENU_TEXT_MARK]);
                Font_SetSpacing(18, 18);
                Font_SetPos(gBoxEquipRows.x, y);
                if (i != 11) {
                    Font_SetColor(Color_Half(gPlayerColorWords[gBoxWork->list[i]]));
                } else {
                    Font_SetColor(Color_Half(Color_Cycle8()));
                }
                Text_PrintfColored(gBoxMenuTexts[BOX_MENU_TEXT_MARK]);
                Font_SetPos(gBoxEquipRows.x + 20, y);
                Font_SetColor(0x807F7F7F);
                Text_Draw(gSymbolColorNames[gBoxWork->list[i]]);
                y += rowStep;
            }
            gBoxEquipRows.lines = gBoxWork->count;
            ListPanel_Draw(&gBoxEquipRows, 0, gBoxMenuTexts[BOX_MENU_TEXT_COLOR_LIST], gBoxWork->pick);
            if (gBoxWork->help == BOX_HELP_OFF) {
                ButtonHint_DrawDetails();
            }
            break;
        }
        break;
    }
}

/* Returns the colour with each of its four bytes halved (alpha too): the font's half-scale
 * colour for a full-scale one (128 is full in the font's colours). */
u32 Color_Half(u32 color)
{
    ((u8 *)&color)[0] >>= 1;
    ((u8 *)&color)[1] >>= 1;
    ((u8 *)&color)[2] >>= 1;
    ((u8 *)&color)[3] >>= 1;
    return color;
}

/* Draws header panel `which` of gBoxListHeaders (0 above a set's list, 1 above its detail
 * page), framed when flags & 1, with the set's title: its name (EquipSet.name, the bytes at
 * equipSet + 12) and its number (gBoxWork->set + 1). */
void BoxMenu_DrawListHeader(u8 *equipSet, s32 flags, u8 which)
{
    ListPanel *panel;

    panel = &gBoxListHeaders[which];
    ListPanel_Draw(panel, 0, gBoxEmptyText, -1);
    if (flags & 1) {
        Menu_DrawFrame(panel, 0);
    }
    Font_SetPos(306, 28);
    Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_SET_TITLE_FMT], equipSet + 12, gBoxWork->set + 1);
}

extern u8 gBoxEquipListU[];
extern char **gArmorNameTables[];
/* The unit's own declaration: the box menu's draws pass alpha unmasked, as the game's code
 * does; the definition's prototype (u8 idx, u8 alpha, s32 alt) would add a mask at each call. */
u32 Equip_Color(u8 idx, s32 alpha, u8 alt); /* unit's own declaration: alpha is an s32 here (the definition's is a u8), as for the item list's draws above */

/* Compiled at optimization level 3 (a pragma pair in this level 4 file; most likely Capcom's
 * source file ended here): the game's code computes the colour ternaries into a register and
 * then masks them into nameCol, and keeps the name colour's `== 0 ||` branches, as level 3
 * does; level 4 writes them straight into nameCol. The armour names' y is written
 * i * 21 + 73: the compiler then steps it between the table walks as the game's code does. */
#pragma optimization_level 3
/* Draws an equipment set (slot: its six box indexes, EquipSet.idx) as a list under its header:
 * the weapon's icon and name, then the five armour pieces in the order head, body, arms,
 * waist, legs (icons from gBoxEquipListU, names from gArmorNameTables). part is the cursor's
 * row (0 the weapon, 1-5 the armour, 6 the register row, -1 none), marked by two arrows that
 * blink unless flags & 8; with flags & 8 only that row is lit. A name is grey when its row is
 * not lit, or when the set weapon's hunter type can't wear the piece (ARMOR_FLAG_BLADEMASTER,
 * ARMOR_FLAG_GUNNER). flags & 1 frames the header; flags & 4 adds the register row (its band,
 * rule and label). Returns 1 when the set has a weapon. */
u8 BoxMenu_DrawEquipList(u16 *slot, s32 part, s32 flags)
{
    EquipItem *weapon;
    u32 i;
    u8 has;
    u16 boxId;
    SaveData *save;
    s16 blink;
    s32 x;
    s16 cursorY;
    s16 y;
    u8 wearMask;
    u8 sel;
    char *name;
    u8 bit;
    u8 *id;
    u8 nameCol;
    u8 ids[6];
    u8 armorIds[5];
    PrimLine band;
    PrimLine rule;
    Sprite spr;
    ArmorData *armor[5];

    gBoxListHeaders[0].lines = 6;
    if (flags & 4) {
        gBoxListHeaders[0].lines = 7;
    }
    BoxMenu_DrawListHeader((u8 *)slot, flags, 0);
    if (slot[SAVE_EQUIP_WEAPON] != BOX_NONE) {
        save = &gSave;
        weapon = &save->box[slot[SAVE_EQUIP_WEAPON]];
        id = ids;
        if (slot[SAVE_EQUIP_HEAD] == BOX_NONE) {
            boxId = 0;
        } else {
            boxId = save->box[slot[SAVE_EQUIP_HEAD]].id;
        }
        ids[EQUIP_HEAD] = boxId;
        if (slot[SAVE_EQUIP_BODY] == BOX_NONE) {
            boxId = 0;
        } else {
            boxId = save->box[slot[SAVE_EQUIP_BODY]].id;
        }
        ids[EQUIP_BODY] = boxId;
        if (slot[SAVE_EQUIP_ARMS] == BOX_NONE) {
            boxId = 0;
        } else {
            boxId = save->box[slot[SAVE_EQUIP_ARMS]].id;
        }
        ids[EQUIP_ARMS] = boxId;
        if (slot[SAVE_EQUIP_WAIST] == BOX_NONE) {
            boxId = 0;
        } else {
            boxId = save->box[slot[SAVE_EQUIP_WAIST]].id;
        }
        ids[EQUIP_WAIST] = boxId;
        if (slot[SAVE_EQUIP_LEGS] == BOX_NONE) {
            boxId = 0;
        } else {
            boxId = save->box[slot[SAVE_EQUIP_LEGS]].id;
        }
        ids[EQUIP_LEGS] = boxId;
        has = 1;
    } else {
        weapon = NULL;
        id = NULL;
        has = 0;
    }
    if (part >= 0) {
        blink = 0;
        x = 324;
        if (!(flags & 8) && (gFrameTick & 0x10)) {
            blink = 5;
        }
        cursorY = part * (part != 6 ? 21 : 22) + 51;
        spr.w = 18;
        spr.h = 20;
        *(u32 *)&spr.u0 = TEX_UV(124, 110);
        *(u32 *)&spr.u1 = TEX_UV(112, 125);
        spr.x = x - blink + 1;
        spr.y = cursorY;
        spr.color = 0xFF000000;
        Sprite_Draw((struct Sprite *)&spr);
        spr.x -= 2;
        spr.y -= 1;
        spr.color = 0xFF00A3FF;
        Sprite_Draw((struct Sprite *)&spr);
        spr.u0 = 113;
        spr.u1 = 125;
        spr.x = blink + 576;
        spr.y = cursorY;
        spr.color = 0xFF000000;
        Sprite_Draw((struct Sprite *)&spr);
        spr.x -= 2;
        spr.y -= 1;
        spr.color = 0xFF00A3FF;
        Sprite_Draw((struct Sprite *)&spr);
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    spr.x = 342;
    spr.w = 32;
    spr.h = 20;
    spr.y = y = 52;
    if (weapon != NULL) {
        if (weapon->type == EQUIP_BLADE) {
            wearMask = ARMOR_FLAG_BLADEMASTER;
        } else {
            wearMask = ARMOR_FLAG_GUNNER;
        }
        sel = 255;
        if ((flags & 8) && part >= 0) {
            sel = 1 << part;
        }
        armorIds[0] = id[EQUIP_HEAD];
        armorIds[1] = id[EQUIP_BODY];
        armorIds[2] = id[EQUIP_ARMS];
        armorIds[3] = id[EQUIP_WAIST];
        armorIds[4] = id[EQUIP_LEGS];
        Equip_GetIconUV(weapon, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
        spr.color = Equip_Color(Equip_GetRarity(weapon->type, weapon->id), 255, !(sel & 1) ? 1 : 0);
        name = Equip_GetName(weapon->type, weapon->id);
    } else {
        wearMask = 0;
        armorIds[4] = 0;
        sel = 0;
        armorIds[3] = 0;
        armorIds[2] = 0;
        armorIds[1] = 0;
        armorIds[0] = 0;
        *(u32 *)&spr.u0 = TEX_UV(193, 225);
        *(u32 *)&spr.u1 = TEX_UV(223, 255);
        spr.color = Equip_Color(0, 255, 1);
        name = Equip_GetName(0, 0);
    }
    Sprite_Draw((struct Sprite *)&spr);
    armor[0] = &gArmorHeadData[armorIds[0]];
    armor[1] = &gArmorBodyData[armorIds[1]];
    armor[2] = &gArmorArmsData[armorIds[2]];
    armor[3] = &gArmorWaistData[armorIds[3]];
    armor[4] = &gArmorLegsData[armorIds[4]];
    spr.v0 = 289;
    spr.v1 = 319;
    for (i = 0, bit = 2; i < 5; i++) {
        y += 21;
        spr.y = y;
        spr.u0 = gBoxEquipListU[i];
        spr.u1 = spr.u0 + 32;
        spr.color = Equip_Color(armor[i]->rarity, 255, !(sel & bit));
        Sprite_Draw((struct Sprite *)&spr);
        bit += bit;
    }
    Font_SetSpacing(18, 18);
    Font_SetPos(373, 52);
    nameCol = (sel & 1) ? 0 : 10;
    Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_NAME_FMT], nameCol, name);
    for (i = 0, bit = 2; i < 5; i++) {
        nameCol = ((wearMask & armor[i]->flags) == 0 || (sel & bit) == 0) ? 10 : 0;
        Font_SetPos(373, i * 21 + 73);
        Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_NAME_FMT], nameCol, gArmorNameTables[i][armorIds[i]]);
        bit += bit;
    }
    if (flags & 4) {
        band.x0 = 308;
        band.x1 = 611;
        band.y0 = 181;
        band.y1 = 203;
        band.color = part != 6 ? 0x30A0E0C0 : 0x2C88FF88;
        Prim_FillRect(&band);
        rule.x0 = 308;
        rule.x1 = 611;
        rule.y0 = rule.y1 = 179;
        rule.color = 0xE0E0E0E0;
        Prim_DrawLine(&rule);
        Font_SetColor(part != 6 ? 0x807F7F7F : 0x807F212E);
        Font_SetPos(396, 183);
        Text_Draw(gBoxMenuTexts[BOX_MENU_TEXT_REGISTER]);
    }
    return has;
}
#pragma optimization_level 4

/* The info window under a set's list (gEquipInfoWin, moved to y). With an item (hasItem), page
 * 0 shows the skills title and gPreviewPlayer's skills coloured against player's
 * (SkillList_DrawCompare), page 1 the status title and gPreviewPlayer's stats against player's
 * (StatusPage_DrawStats). Without one, only the window with the page's empty title. player
 * may be NULL: nothing is compared. */
void EquipInfo_DrawPage(s16 y, s32 hasItem, s32 page, Player *player)
{
    s32 shownPage;
    s32 emptyPage;
    s32 text;

    text = hasItem;
    gEquipInfoWin.y = y;
    if (text != 0) {
        shownPage = page & 0xFF;
        switch (shownPage) {
        case 0:
            Window_DrawTextB2((Window *)&gEquipInfoWin, (u8 *)gBoxMenuTexts[BOX_MENU_TEXT_SKILLS_HEAD]);
            SkillList_DrawCompare(player, 306, y);
            return;
        case 1:
            Window_DrawTextB2((Window *)&gEquipInfoWin, (u8 *)gBoxMenuTexts[BOX_MENU_TEXT_STATUS_HEAD]);
            StatusPage_DrawStats(&gPreviewPlayer, player, 306, (((s16)(y)) + 20) & 0xFFFF);
            return;
        }
    } else {
        emptyPage = page & 0xFF;
        switch (emptyPage) {
        case 0:
            text = (s32)gBoxMenuTexts[BOX_MENU_TEXT_SKILLS_HEAD_ALT];
            break;
        case 1:
            text = (s32)gBoxMenuTexts[BOX_MENU_TEXT_STATUS_HEAD_ALT];
            break;
        }
        Window_DrawTextB2((Window *)&gEquipInfoWin, (u8 *)text);
    }
}

/* An equipment set's list and its info (slot: the set's six box indexes; part and flags as
 * for BoxMenu_DrawEquipList). On detail pages 0 and 1 (gBoxWork->levelSel): the list and
 * EquipInfo_DrawPage below it (at y 198, or 218 under the register row, flags & 4); on page
 * 2: the header alone and the set's skills (EquipDetail_Draw of gPreviewPlayer). flags & 2
 * adds the guide line at the bottom. */
void BoxMenu_DrawEquipInfo(u16 *slot, s32 part, s32 flags, Player *player)
{
    s16 winY;
    s16 guideY;
    u8 hasItem;

    winY = 198;
    if (flags & 4) {
        winY = 218;
    }
    switch (gBoxWork->levelSel) {
    case 0:
    case 1:
        hasItem = BoxMenu_DrawEquipList(slot, (s8)part, flags);
        EquipInfo_DrawPage(winY, hasItem, gBoxWork->levelSel, player);
        guideY = 398;
        break;
    case 2:
        BoxMenu_DrawListHeader((u8 *)slot, flags, 1);
        if (*slot != BOX_NONE) {
            EquipDetail_Draw(&gPreviewPlayer, 306, 62, gBoxWork->skillPage | 0x80000000);
        } else {
            EquipDetail_DrawNone(306, 62);
        }
        guideY = 394;
        break;
    }
    if (flags & 2) {
        Font_SetPos(504, guideY);
        Font_SetSpacing(18, 18);
        Text_SetColor(0);
        Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_DETAIL_SWITCH]);
    }
}

extern ListPanel gBoxEquipPickPanel;
/* The box's items for the set's part gBoxWork->part (0 the weapon, 1-5 head, body, arms,
 * waist, legs): page gBoxWork->page of BoxList_GetPage in gBoxEquipPickPanel, sel its cursor
 * row (-1 none). Each item has its icon, a mark when it is worn (SaveData.equipIdx), another
 * when it is the set's (cur), and its name, grey for armour the set weapon's hunter type can't
 * wear. The title names the part; "page/pages" shows when there are several. flags & 1 dims
 * it all (alpha 144, the dim colours). An empty page shows the empty text.
 * For the match: the empty text's colour has its own local (emptyCol), and alpha is declared
 * after y and entry, which gives the game's register use. Equip_Color is declared in the
 * block with its definition's prototype (u8 idx, u8 alpha, s32 alt), which loads the alpha
 * constants as the game's code does; the file's (u8, s32, u8) stays for the other functions. */
void BoxMenu_DrawEquipPick(EquipIdx *cur, s8 sel, u32 flags)
{
    EquipItem *item;
    u16 *list;
    u32 i;
    s16 rowCol;
    u16 wornSlot;
    ArmorData *armor;
    u16 setSlot;
    u8 mask;
    Sprite spr;
    SaveData *save;
    u16 *wornSlots;
    s16 baseCol;
    s16 emptyCol;
    char buf[64];
    ArmorData *armorTable;
    u16 count;
    s16 y;
    u16 *entry;
    u8 alpha;
    u32 Equip_Color(u8 idx, u8 alpha, s32 alt);

    if (gBoxWork->part < 6) {
        if (flags & 1) {
            emptyCol = 13;
            rowCol = 9;
            baseCol = 9;
            alpha = 144;
        } else {
            rowCol = 5;
            baseCol = 0;
            emptyCol = 2;
            alpha = 255;
        }
        sprintf(buf, gBoxMenuTexts[BOX_MENU_TEXT_PICK_FMT], rowCol, gEquipSlotTexts[gBoxWork->part]);
        ListPanel_Draw(&gBoxEquipPickPanel, 0, buf, sel);
        count = BoxList_GetPage(&list, gBoxWork->page);
        if (count == 0) {
            Text_SetColor(emptyCol);
            Font_SetPos(30, 48);
            Text_Draw(gBoxMenuTexts[BOX_MENU_TEXT_NO_EQUIP]);
            return;
        }
        if (gBoxWork->pageCount > 1 && !(flags & 1)) {
            Menu_DrawFrame(&gBoxEquipPickPanel, 0);
            Font_SetPos(210, 28);
            Text_SetColor(baseCol);
            Text_PrintfColored(gBoxMenuTexts[BOX_MENU_TEXT_PAGE_FMT], gBoxWork->page + 1, gBoxWork->pageCount);
        }
        entry = list;
        save = &gSave;
        wornSlots = gSave.equipIdx;
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        spr.x = 34;
        spr.w = 28;
        spr.h = 18;
        y = 47;
        if (gBoxWork->part == 0) {
            for (i = 0; i < count; i++) {
                item = &save->box[*entry];
                spr.y = y;
                Equip_GetIconUV(item, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
                spr.color = Equip_Color(Equip_GetRarity(item->type, item->id), alpha, 0);
                Sprite_Draw((struct Sprite *)&spr);
                if (*entry == wornSlots[SAVE_EQUIP_WEAPON]) {
                    *(u32 *)&spr.u0 = TEX_UV(96, 288);
                    *(u32 *)&spr.u1 = TEX_UV(128, 320);
                    spr.color = ((u8)alpha << 24) | 0xFFFF20;
                    Sprite_Draw((struct Sprite *)&spr);
                }
                if (*entry == cur->idx[SAVE_EQUIP_WEAPON]) {
                    *(u32 *)&spr.u0 = TEX_UV(0, 0);
                    *(u32 *)&spr.u1 = TEX_UV(32, 32);
                    spr.color = ((u8)alpha << 24) | 0xFFFF00;
                    Sprite_Draw((struct Sprite *)&spr);
                    rowCol = 10;
                } else {
                    rowCol = baseCol;
                }
                Text_SetColor(rowCol);
                Font_SetPos(66, y);
                Text_PrintfColored(gEquipNameTables[item->type][item->id]);
                y += 20;
                entry++;
            }
        } else {
        switch (gBoxWork->part) {
        case 1:
            wornSlot = wornSlots[SAVE_EQUIP_HEAD];
            armorTable = gArmorHeadData;
            setSlot = cur->idx[SAVE_EQUIP_HEAD];
            break;
        case 2:
            wornSlot = wornSlots[SAVE_EQUIP_BODY];
            armorTable = gArmorBodyData;
            setSlot = cur->idx[SAVE_EQUIP_BODY];
            break;
        case 3:
            wornSlot = wornSlots[SAVE_EQUIP_ARMS];
            armorTable = gArmorArmsData;
            setSlot = cur->idx[SAVE_EQUIP_ARMS];
            break;
        case 4:
            wornSlot = wornSlots[SAVE_EQUIP_WAIST];
            armorTable = gArmorWaistData;
            setSlot = cur->idx[SAVE_EQUIP_WAIST];
            break;
        case 5:
            wornSlot = wornSlots[SAVE_EQUIP_LEGS];
            armorTable = gArmorLegsData;
            setSlot = cur->idx[SAVE_EQUIP_LEGS];
            break;
        }
        mask = save->box[cur->idx[SAVE_EQUIP_WEAPON]].type == EQUIP_BLADE ? ARMOR_FLAG_BLADEMASTER : ARMOR_FLAG_GUNNER;
        for (i = 0; i < count; i++) {
            item = &save->box[*entry];
            armor = &armorTable[item->id];
            spr.y = y;
            Equip_GetIconUV(item, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
            spr.color = Equip_Color(Equip_GetRarity(item->type, item->id), alpha, 0);
            Sprite_Draw((struct Sprite *)&spr);
            if (*entry == wornSlot) {
                *(u32 *)&spr.u0 = TEX_UV(96, 288);
                *(u32 *)&spr.u1 = TEX_UV(128, 320);
                spr.color = ((u8)alpha << 24) | 0xFFFF20;
                Sprite_Draw((struct Sprite *)&spr);
            }
            if (*entry == setSlot) {
                *(u32 *)&spr.u0 = TEX_UV(0, 0);
                *(u32 *)&spr.u1 = TEX_UV(32, 32);
                spr.color = ((u8)alpha << 24) | 0xFFFF00;
                Sprite_Draw((struct Sprite *)&spr);
            }
            Text_SetColor((armor->flags & mask) == 0 ? 10 : baseCol);
            Font_SetPos(66, y);
            Text_PrintfColored(gEquipNameTables[item->type][item->id]);
            y += 20;
            entry++;
        }
        }
    }
}

extern ListWin gBoxListPanel;

/* The arrange menu's draw (ItemBoxMenu.menu 3). Step 0: the items or equipment choice. Step 1:
 * the box's list of that kind (BoxMenu_DrawBoxPage), with the details on (help), the swap
 * prompt (prompt) or the hints; while a slot is held (sub 1), the grid cursor on it when it
 * is on the page shown. */
void BoxMenu_DrawArrange(void)
{
    s32 listFlags;

    BoxMenu_DrawActions(1);
    switch (gBoxWork->step) {
    case 0:
        gBoxListPanel.y = 94;
        ListPanel_DrawDim(&gBoxListPanel, 0, gBoxWork->kind);
        break;
    case 1:
        if (gBoxWork->help != BOX_HELP_OFF) {
            if (gBoxWork->kind == BOX_KIND_ITEMS) {
                listFlags = BOX_LIST_DETAIL;
            } else {
                listFlags = (gBoxWork->levelSel & BOX_LIST_PAGE_MASK) | (BOX_LIST_DIM | BOX_LIST_EQUIP | BOX_LIST_DETAIL);
            }
        } else if (gBoxWork->prompt != 0) {
            listFlags = ((gBoxWork->kind & 1) << 4) | BOX_LIST_SORTED;
        } else {
            listFlags = (gBoxWork->kind & 1) << 4;
            ButtonHint_DrawDetails();
            ButtonHint_DrawSort(0);
        }
        BoxMenu_DrawBoxPage(((s16 *)gBoxWork->slots)[gBoxWork->kind], listFlags);
        if ((gBoxWork->help == BOX_HELP_OFF || gBoxWork->kind == BOX_KIND_ITEMS)
            && gBoxWork->sub == 1) {
            if (gBoxWork->held / BOX_PAGE_SLOTS
                == gBoxWork->slots[gBoxWork->kind] / BOX_PAGE_SLOTS) {
                BoxMenu_DrawGridCursor();
            }
        }
        break;
    }
}

/* The sell menu's draw (ItemBoxMenu.menu 4). Step 0: the items or equipment choice. Step 1:
 * the box's list with the prices and the money (worn equipment marked), with the details on,
 * the yes/no prompt or the hints; then the amount (sub 1, BoxMenu_DrawAmount) and the price
 * (sub 2, BoxMenu_DrawZenny). */
void BoxMenu_DrawSell(void)
{
    s32 listFlags;
    u8 boxKind;

    BoxMenu_DrawActions(1);
    switch (gBoxWork->step) {
    case 0:
        gBoxListPanel.y = 116;
        ListPanel_DrawDim(&gBoxListPanel, 0, gBoxWork->kind);
        break;
    case 1:
        boxKind = gBoxWork->kind;
        listFlags = boxKind == 0 ? (BOX_LIST_PRICE | BOX_LIST_MONEY) : (BOX_LIST_PRICE | BOX_LIST_WORN_ALT | BOX_LIST_MONEY | BOX_LIST_EQUIP);
        switch (gBoxWork->sub) {
        case 0:
            if (gBoxWork->help != BOX_HELP_OFF) {
                if (boxKind == 0) {
                    listFlags = BOX_LIST_PRICE | BOX_LIST_MONEY | BOX_LIST_DETAIL;
                } else {
                    listFlags = (gBoxWork->levelSel & BOX_LIST_PAGE_MASK) | (BOX_LIST_PRICE | BOX_LIST_WORN_ALT | BOX_LIST_MONEY | BOX_LIST_DIM | BOX_LIST_EQUIP | BOX_LIST_DETAIL);
                }
            } else if (gBoxWork->prompt != 0) {
                listFlags = ((boxKind & 1) << 4) | (BOX_LIST_SORTED | BOX_LIST_MONEY);
            } else if (gBoxWork->msg < 0) {
                ButtonHint_DrawDetails();
                ButtonHint_DrawSort(0);
            }
            break;
        case 1:
            BoxMenu_DrawAmount();
            break;
        case 2:
            BoxMenu_DrawZenny();
            break;
        }
        BoxMenu_DrawBoxPage(((s16 *)gBoxWork->slots)[gBoxWork->kind], listFlags);
        break;
    }
}

/* With helpMode 1, the help window with the description of the item in pouch slot pouchSlot;
 * otherwise the details button hint. */
void BoxMenu_DrawHelp(s32 helpMode, s32 pouchSlot)
{
    if ((helpMode & 0xFF) == 1) {
        HelpWindow_Draw(HELP_PAGE_ITEMS, HELP_ITEM_LINE(gSave.pouch[pouchSlot & 0xFF].item) & 0xFFFF, *(s8 *)&gHelpColumn);
        return;
    }
    ButtonHint_DrawDetails();
}

/* The amount to move (1-99, two full-width digits) with its up and down
 * arrows; 9999 means none. */
void BoxMenu_DrawAmount(void)
{
    char buf[5];

    if (gBoxWork->amount != 9999) {
        Font_SetSpacing(18, 18);
        Text_SetColor(0);
        Font_SetPos(570, 378);
        /* two full-width digits */
        buf[0] = 130;
        buf[1] = gBoxWork->amount / 10 + 79;
        buf[2] = 130;
        buf[3] = gBoxWork->amount % 10 + 79;
        buf[4] = 0;
        Text_Draw(buf);
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Tex_Prepare(1, TEX_SLOT_ICONS);
        Tex_Bind(TEX_SLOT_ICONS);
        if (gBoxWork->atMax == 0) {
            Menu_DrawArrow(576, 360, 24, 18, 0xFF20FF28, ARROW_TEXRECT);
        }
        if (gBoxWork->amount != 1) {
            Menu_DrawArrow(576, 398, 24, 18, 0xFF20FF28, ARROW_TEXRECT | ARROW_MIRROR);
        }
    }
}

/* The selling price (gBoxWork->price) in full-width digits followed by the zenny sign, or the
 * no-price text when it is 0. */
void BoxMenu_DrawZenny(void)
{
    u16 wide[16];
    char buf[16];
    u16 *dst;
    char *src;
    s32 sum;

    Font_SetSpacing(18, 18);
    Font_SetPos(306, 358);
    sum = gBoxWork->price;
    if (sum != 0) {
        /* the number in full-width digits */
        sprintf(buf, "%d", sum);
        dst = wide;
        src = buf;
        while (*src != 0) {
            *dst = ((*src + 31) << 8) | 0x82;
            src++;
            dst++;
        }
        dst[0] = 39554;
        dst[1] = 0;
        Text_SetColor(5);
        Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_PRICE_FMT], wide);
    } else {
        Text_SetColor(0);
        Text_Draw(gBoxMenuTexts[BOX_MENU_TEXT_TAKEN_FREE]);
    }
}

/* The yes/no answer (gYesNoTexts[gBoxWork->yesNo]) on the bottom line; the box menu's draw
 * calls it while an answer is shown (yesNo not BOX_YESNO_NONE). */
void BoxMenu_DrawYesNo(void)
{
    Font_SetSpacing(18, 18);
    Font_SetPos(432, 398);
    Text_Printf(gYesNoTexts[gBoxWork->yesNo]);
}

/* The box menu's .data: its windows, list panels, icons and colours, from
 * gBoxTypeKinds to the end of gBoxEquipPickPanel. */
s8 gBoxTypeKinds[8] = {
#include "lobby/007F1A08.inc"
};
Panel gBoxItemListWin = {
#include "lobby/007F1A10.inc"
};
u32 gBoxPageColors[8] = {
#include "lobby/007F1A30.inc"
};
BoxTab gBoxPageTabs[2] = {
#include "lobby/007F1A50.inc"
};
Panel gBoxPageTabsWin = {
#include "lobby/007F1A80.inc"
};
Panel gBoxEquipGridWin = {
#include "lobby/007F1AA0.inc"
};
ListPanel gEquipDetailPanel = {
#include "lobby/007F1AC0.inc"
};
ListPanel gEquipNonePanel = {
#include "lobby/007F1AE0.inc"
};
Panel gEquipDetailWin[4] = {
#include "lobby/007F1B00.inc"
};
s16 gEquipDetailIcons[8] = {
#include "lobby/007F1B60.inc"
};
/* The panels' row texts, which their data below points to. */
ListPanel gBoxActionsPanel = {
#include "lobby/007F1B70.inc"
};
ListWin gBoxListPanel = {
#include "lobby/007F1B90.inc"
};
ListWin gBoxEquipPanel = {
#include "lobby/007F1BB0.inc"
};
ListPanel gBoxListHeaders[2] = {
#include "lobby/007F1BD0.inc"
};
Panel gEquipInfoWin = {
#include "lobby/007F1C10.inc"
};
ListPanel gBoxEquipRows = {
#include "lobby/007F1C30.inc"
};
s16 gBoxMenuIcon[8] = {
#include "lobby/007F1C50.inc"
};
s16 gBoxControlIcons[24] = {
#include "lobby/007F1C60.inc"
};
s16 gBoxControlIconsB[12] = {
#include "lobby/007F1C90.inc"
};
u8 gBoxEquipListU[8] = {
#include "lobby/007F1CA8.inc"
};
ListPanel gBoxEquipPickPanel = {
#include "lobby/007F1CB0.inc"
};
