#include "common.h"
#include "m2c_macros.h"
#include "pad.h"
#include "save.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_menu.h"


void Shop_BuildSellList(ItemListWin *list, s16 page);
s32 WinSlide_Step(WinSlide *, s16, s16);
s32 Village_Is3924Kind4(void);
s32 Game_IsOnline(void);

extern u8 gMenuWork[];
u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8, s8);
void ItemShop_SetRows(u16 pad);
void Shop_StartBuy(ItemListWin *list, u16 kind);
void CamFocus_SetAside(s8);
void LobbySe_Play(s32);
void Shop_StartSell(ItemListWin *list);
s32 Menu_PushNoticeOnce(s32 arg0);
void Gallery_Open(void);
struct Em *Npc_Find(u8 kind);
struct OfferTask;
void OfferMenu_Open(struct OfferTask *t);
void Talk_Say(s32 arg0, s8 arg1);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Shop_BuyStep(u16 pad, ItemListWin *list);
s32 Shop_SellStep(u16 pad, ItemListWin *list);
s32 Talk_Update(void);
void TownMenu_Close(void);

void ItemShop_Update(void)
{
    ShopMenu *m = (ShopMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    s32 state;

    Chat_SetWindowMode(1, 2);
    state = m->state;
    switch (state) {
    case 0:
        m->state = state + 1;
        ItemShop_SetRows(pad);
        break;
    case 1:
        if (pad & 0x20) {
            switch (m->items[m->cur]) {
            case 1:
                m->state = 2;
                Shop_StartBuy(&m->list, m->npc);
                CamFocus_SetAside(1);
                LobbySe_Play(0x17);
                break;
            case 2:
                m->state = 3;
                Shop_StartSell(&m->list);
                CamFocus_SetAside(1);
                LobbySe_Play(8);
                break;
            case 3:
                if (gLobby->questTaken != 0) {
                    Menu_PushNoticeOnce(1);
                    break;
                }
                CamFocus_SetAside(1);
                Gallery_Open();
                break;
            case 4:
                OfferMenu_Open((struct OfferTask *) (Npc_Find(m->npc)));
                LobbySe_Play(8);
                break;
            }
        } else if (pad & 0x40) {
            m->state = 4;
            Talk_Say(1, 0);
            LobbySe_Play(4);
        } else {
            m->cur = Menu_WrapUpDown(m->cur, m->n);
        }
        break;
    case 2:
        switch (Shop_BuyStep(pad, &m->list)) {
        case 0:
        case 3:
            m->state = 1;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 3:
        switch (Shop_SellStep(pad, &m->list)) {
        case 0:
        case 3:
            m->state = 1;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
        case 3:
            TownMenu_Close();
            break;
        }
        break;
    }
}

s32 Item_GetSellPrice(s32 arg0);
s32 Menu_CountPages(s16 n, s16 per);
void *memset(void *, s32, u32);

void Shop_BuildSellList(ItemListWin *list, s16 page)
{
    s32 base = page * 7;
    s32 i;
    s32 idx;
    u16 item;

    list->n = 0;
    memset(list->ent, 0, 0x1C);
    for (i = 0; i < 7; i++) {
        idx = base + i;
        if (idx < 20) {
            item = gSave.pouch[idx].item;
            list->ent[i].id = item;
            list->ent[i].unk2 = Item_GetSellPrice(item);
            list->n++;
        }
    }
    if (list->cur >= list->n) {
        list->cur = list->n - 1;
    }
    list->page = page;
    list->pages = Menu_CountPages(20, 7);
}

void Shop_StartSell(ItemListWin *list) {
    list->page = 0;
    list->infoMode = 0;
    list->cur = 0;
    list->color = 0xBDB59F6C;
    Shop_BuildSellList(list, 0);
    WinSlide_Step(&list->win, 0x280, 0x10E);
}

void LobbySe_Play(s32);
void Shop_Sell(ItemListWin *list);
s32 Shop_SellAmountInput(u16 pad, ItemListWin *list);
s32 Item_IsDeco(u16 id);
s32 Shop_SellListInput(u16 pad, ItemListWin *list);
s32 Menu_YesNoInput(void);
void Menu_SetYesNo(s8 arg0);
extern u8 gMenuInputLock;

s32 Shop_SellStep(u16 pad, ItemListWin *list)
{
    switch (list->state) {
    case 0:
        switch (WinSlide_Step(&list->win, 0x280, 0x10E)) {
        case 0:
            list->state++;
            break;
        }
        break;
    case 1:
        switch (Shop_SellListInput(pad, list)) {
        case 0:
            list->state++;
            list->count = 1;
            break;
        case 3:
            list->state = 0;
            return 3;
        }
        break;
    case 2:
        switch (Shop_SellAmountInput(pad, list)) {
        case 0:
            list->state++;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case 3:
            list->state = 1;
            break;
        }
        break;
    case 3:
        if (pad & 0x200) {
            if (list->ent[list->cur].id != 0) {
                list->infoMode ^= 1;
                LobbySe_Play(0x39);
            }
        } else if (pad & 0x80) {
            if (list->ent[list->cur].id != 0 && list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
                if (++list->infoPage >= 3) {
                    list->infoPage = 0;
                }
                LobbySe_Play(8);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case 0:
                list->state = 1;
                Shop_Sell(list);
                Shop_BuildSellList(list, list->page);
                break;
            case 3:
                list->state = 1;
                break;
            }
        }
        break;
    }
    return 2;
}

u16 SavePouch_Add(u16, s16); /* unit's own declaration: the definition (sub_main unk_3B4C80.c) returns u8; with it Shop_Sell differs (1 of 51 words) */
s32 SavePouch_ClearSlot(s16);
void Save_AddMoney(s32 n);

void Shop_Sell(ItemListWin *list)
{
    s16 cur = list->cur;
    u16 price = list->ent[cur].unk2;
    u16 id = list->ent[cur].id;
    s32 count = list->count;
    s32 money = price * count;

    if (gItemData[id].max != 0xFF) {
        if (SavePouch_Add(id, -count) == 4) {
            list->ent[list->cur].id = 0;
        }
    } else {
        SavePouch_ClearSlot(cur + list->page * 7);
    }
    LobbySe_Play(10);
    Save_AddMoney(money);
}

s32 Shop_SellAmountInput(u16 pad, ItemListWin *list)
{
    s32 SavePouch_Count(u16 item);
    u16 id = list->ent[list->cur].id;
    s16 max = SavePouch_Count(id);

    if (gItemData[id].max == 0xFF) {
        max = 1;
    }
    if (pad & 0x20) {
        LobbySe_Play(8);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x200) {
        if (list->ent[list->cur].id != 0) {
            list->infoMode ^= 1;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0x80) {
        if (list->ent[list->cur].id != 0 && list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
            if (++list->infoPage >= 3) {
                list->infoPage = 0;
            }
            LobbySe_Play(8);
        }
    } else if (pad & 0x2000) {
        if (list->count < max) {
            list->count++;
            LobbySe_Play(1);
        }
    } else if (pad & 0x1000) {
        if (list->count > 1) {
            list->count--;
            LobbySe_Play(1);
        }
    } else if (pad & 0x800) {
        if (list->count > 1) {
            list->count = 1;
            LobbySe_Play(1);
        }
    } else if (pad & 0x400) {
        if (list->count < max) {
            list->count = max;
            LobbySe_Play(1);
        }
    }
    return 2;
}

s32 Item_IsDeco(u16 id)
{
    return gItemData[id].deco != 0;
}

void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);

s32 Shop_SellListInput(u16 pad, ItemListWin *list)
{
    if (pad & 0x20) {
        if (list->ent[list->cur].id != 0) {
            LobbySe_Play(8);
            return 0;
        }
        LobbySe_Play(9);
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        if (list->ent[list->cur].id != 0) {
            list->infoMode ^= 1;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0x80) {
        if (list->ent[list->cur].id != 0 && list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
            if (++list->infoPage >= 3) {
                list->infoPage = 0;
            }
            LobbySe_Play(8);
        }
    } else if (pad & 0xC00) {
        list->page = Menu_WrapLeftRight(list->page, list->pages);
        Shop_BuildSellList(list, list->page);
    } else {
        list->cur = Menu_WrapUpDown(list->cur, list->n);
    }
    return 2;
}

struct Em *Npc_Find(u8 kind);
s16 OfferMenu_BuildList(struct OfferTask *t, s32 b);
extern char *D_0080D104[];
extern char *gShopBuySellTexts[];
extern u8 gMenuWork[];

void ItemShop_SetRows(u16 pad)
{
    ShopMenu *c = (ShopMenu *)gMenuWork;

    c->lines = D_0080D104;
    c->items[0] = 1;
    c->items[1] = 2;
    c->n = 2;
    c->color = 0xBDB59F6C;
    switch (c->npc) {
    case 0x39:
        c->n = 3;
        c->items[2] = 3;
        break;
    default:
        if (OfferMenu_BuildList((struct OfferTask *) (Npc_Find(c->npc)), 0) != 0) {
            c->n = 3;
            c->items[2] = 4;
            c->lines = gShopBuySellTexts;
        }
        break;
    }
}

s32 PspLink_Count(void);
s32 Save_TestEventFlag(s32 flag);
/* unit's own declaration: the definition (unk_6CB690.c) returns s16; with it Shop_PickStockList, Shop_PickByTimer and Shop_BuildStock differ (31 of 45, 12 of 22, 171 of 242 words) */
s32 Village_GetTimerState(s32);
extern u16 *gShopStockNpc12Psp[];
extern u16 *gShopStockNpc12Timer[];
extern u16 *D_0080BA04;

u16 * Shop_PickStockList(void)
{
    s32 n;
    s32 idx;
    u16 *ret;

    n = PspLink_Count();
    idx = 0;
    if (Save_TestEventFlag(0x7E) == 0) {
        n = 0;
    }
    if (Village_GetTimerState(3) != 0) {
        if (n != 0) {
            ret = D_0080BA04;
        } else {
            ret = gShopStockNpc12Timer[0];
        }
        return ret;
    }
    if (n != 0) {
        n %= 3;
        idx = n + 1;
    }
    return gShopStockNpc12Psp[idx];
}

/* unit's own declaration: the definition (unk_6CB690.c) returns s16; with it Shop_PickStockList, Shop_PickByTimer and Shop_BuildStock differ (31 of 45, 12 of 22, 171 of 242 words) */
s32 Village_GetTimerState(s32);

u16 *Shop_PickByTimer(u16 **p, s32 x)
{
    if (Game_IsOnline() == 0 && Village_GetTimerState(x) != 0) {
        p++;
    }
    return *p;
}

u8 Village_FacilityLevel(s32 sub);
s32 Shop_GetBuyPrice(s32 arg0);
s32 Cond_Check(u16 cond);
typedef struct CondItems {
    u16 cond;
    u16 *items;
} CondItems;
extern CondItems gShopCondStockNpc07[];
extern CondItems gShopCondStockNpc08[];
extern u16 *gShopStockNpc0A[];
extern u16 gShopStockNpc0B[];
extern u16 gShopStockNpc0BLv2[];
extern u16 gShopStockNpc0BTimer[];
extern u16 gShopStockNpc0C[];
extern u16 gShopStockNpc0CLv2[];
extern CondItems gShopCondStockNpc28[];
extern u16 *gShopStockNpc2C[];
extern u16 *D_0080BAC4;
extern u16 *gShopStockNpc2D[];
extern u16 *D_0080BB34;
extern u16 *gShopStockNpc2E[];
extern u16 *D_0080BBAC;
extern CondItems gShopCondStockNpc39[];
extern CondItems gShopCondStockNpc42[];
extern u16 gShopStockNpc50[];
extern u16 gShopStockNpc50Ev122[];
extern CondItems gShopCondStockNpc5D[];
extern CondItems gShopCondStockNpc6A[];

/* fills the list with page 'page' (7 entries) of the items of list->kind */
void Shop_BuildStock(ItemListWin *list, s16 page)
{
    u16 *items = NULL;
    s32 first = page * 7;
    s32 last = first + 6;
    s32 i;
    CondItems *cl;
    s32 k;
    u16 id;

    list->n = 0;
    list->page = page;
    memset(list->ent, 0, sizeof(list->ent));
    cl = NULL;
    switch (list->kind) {
    case 7:
        cl = gShopCondStockNpc07;
        break;
    case 8:
        cl = gShopCondStockNpc08;
        break;
    case 10:
        items = Shop_PickByTimer(gShopStockNpc0A, 0);
        break;
    case 11:
        if (Village_GetTimerState(0) != 0) {
            items = gShopStockNpc0BTimer;
        } else if (Village_FacilityLevel(3) == 2) {
            items = gShopStockNpc0BLv2;
        } else {
            items = gShopStockNpc0B;
        }
        break;
    case 0x12:
        items = Shop_PickStockList();
        break;
    case 0x28:
        cl = gShopCondStockNpc28;
        break;
    case 0x2C:
        if (gLobby->townEvent == 1) {
            items = D_0080BAC4;
        } else {
            items = gShopStockNpc2C[0];
        }
        break;
    case 0x2D:
        if (gLobby->townEvent == 3) {
            items = D_0080BB34;
        } else {
            items = gShopStockNpc2D[0];
        }
        break;
    case 0x2E:
        if (gLobby->townEvent == 2) {
            items = D_0080BBAC;
        } else {
            items = gShopStockNpc2E[0];
        }
        break;
    case 0x39:
        cl = gShopCondStockNpc39;
        break;
    case 0x42:
        cl = gShopCondStockNpc42;
        break;
    case 0x50:
        if (Save_TestEventFlag(0x122) == 1) {
            items = gShopStockNpc50Ev122;
        } else {
            items = gShopStockNpc50;
        }
        break;
    case 0x5D:
        cl = gShopCondStockNpc5D;
        break;
    case 0x6A:
        cl = gShopCondStockNpc6A;
        break;
    case 0xC:
    case 0xD:
        if (Village_FacilityLevel(2) == 2) {
            items = gShopStockNpc0CLv2;
        } else {
            items = gShopStockNpc0C;
        }
        break;
    default:
        cl = NULL; /* already NULL: the game still jumps through this block */
        break;
    }
    for (; cl != NULL; cl++) {
        if (Cond_Check(cl->cond) == 1) {
            items = cl->items;
            break;
        }
    }
    if (items != NULL) {
        k = 0;
        for (i = 0; (id = *items) != 0; items++, i++) {
            if (i >= first && i <= last) {
                list->ent[k].id = id;
                list->ent[k].unk2 = Shop_GetBuyPrice(id);
                k++;
                list->n++;
            }
        }
    }
    if (list->cur >= list->n) {
        list->cur = list->n - 1;
    }
    /* the game reads a register it never set: i is s1, still the caller's, when the kind has no stock list (items NULL) */
    list->pages = Menu_CountPages(i, 7);
}

s16 SavePouch_HasFreeSlot(void);
void Shop_BuildStock(ItemListWin *list, s16 page);

void Shop_StartBuy(ItemListWin *list, u16 kind)
{
    list->page = 0;
    list->color = 0xBDB59F6C;
    list->infoMode = 0;
    list->cur = 0;
    list->kind = kind;
    Shop_BuildStock(list, 0);
    WinSlide_Step(&list->win, 0x280, 0x10E);
    list->unk3D = SavePouch_HasFreeSlot();
    switch (list->kind) {
    case 0xD:
    case 0xC:
        list->unk38 = 0;
        break;
    case 7:
    case 0x39:
    case 0x42:
    case 0x50:
    case 0x5D:
        list->unk38 = 2;
        break;
    case 0xB:
    case 0x2D:
        list->unk38 = 3;
        break;
    case 8:
    case 0x2C:
        list->unk38 = 4;
        break;
    default:
        list->unk38 = 1;
        break;
    }
}

void Shop_Buy(ItemListWin *list);
s32 Shop_BuyAmountInput(u16 pad, ItemListWin *list);
s32 Shop_BuyListInput(u16 pad, ItemListWin *list);
s32 Menu_YesNoInput(void);
void Menu_SetYesNo(s8 arg0);
extern u8 gMenuInputLock;

s32 Shop_BuyStep(u16 pad, ItemListWin *list)
{
    switch (list->state) {
    case 0:
        switch (WinSlide_Step(&list->win, 0x280, 0x10E)) {
        case 0:
            list->state++;
            break;
        }
        break;
    case 1:
        switch (Shop_BuyListInput(pad, list)) {
        case 0:
            list->state++;
            list->count = 1;
            break;
        case 3:
            list->state = 0;
            return 3;
        }
        break;
    case 2:
        switch (Shop_BuyAmountInput(pad, list)) {
        case 0:
            list->state++;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case 3:
            list->state = 1;
            break;
        }
        break;
    case 3:
        if (pad & 0x200) {
            list->infoMode ^= 1;
            LobbySe_Play(0x39);
        } else {
            switch (Menu_YesNoInput()) {
            case 0:
                list->state = 1;
                Shop_Buy(list);
                break;
            case 3:
                list->state = 1;
                break;
            }
        }
        break;
    }
    return 2;
}

static s32 Shop_IsUnlockItem(s32 x);
static s32 Shop_IsOnceOnlyItem(u16 arg0);
u16 SavePouch_Add(u16, s16); /* unit's own declaration: the definition (sub_main unk_3B4C80.c) returns u8; with it Shop_Sell differs (1 of 51 words) */
void Save_AddMoney(s32 n);
void Save_SetItemObtained(SaveData *save, u16 item);
u8 Village_FacilityLevel(s32 sub);
void Village_SetFacilityLevel(s32 sub, u8 lvl);
s32 Shop_GetBuyPrice(s32 arg0);
s32 Menu_PushNoticeOnce(s32 arg0);
s32 LobbyText_Update(void);

void Shop_Buy(ItemListWin *list)
{
    s32 cost;
    u16 id = list->ent[list->cur].id;

    cost = Shop_GetBuyPrice(id);
    cost *= list->count;

    LobbySe_Play(10);
    Save_AddMoney(-cost);
    if (Shop_IsUnlockItem(id) == 1) {
        Save_SetItemObtained(&gSave, id);
        switch (id) {
        case 0x26D:
            Menu_PushNoticeOnce(5);
            break;
        case 0x26E:
            Menu_PushNoticeOnce(6);
            break;
        case 0x26F:
            Village_SetFacilityLevel(4, Village_FacilityLevel(4) + 1);
            if (Village_FacilityLevel(4) == 2) {
                Menu_PushNoticeOnce(8);
            } else {
                Menu_PushNoticeOnce(7);
            }
            break;
        case 0x270:
            Village_SetFacilityLevel(4, Village_FacilityLevel(4) + 1);
            if (Village_FacilityLevel(4) == 2) {
                Menu_PushNoticeOnce(8);
            } else {
                Menu_PushNoticeOnce(7);
            }
            break;
        }
        if (Game_IsOnline() == 1) {
            LobbyText_Update();
        }
    } else if (Shop_IsOnceOnlyItem(id) == 1) {
        Save_SetItemObtained(&gSave, id);
    } else {
        SavePouch_Add(id, list->count);
    }
    list->unk3D = SavePouch_HasFreeSlot();
}

s32 Shop_CanBuy(u16 item, s16 n);

s32 Shop_BuyAmountInput(u16 pad, ItemListWin *list)
{
    s32 k;
    s16 old;
    u16 id = list->ent[list->cur].id;

    if (pad & 0x20) {
        LobbySe_Play(8);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x200) {
        list->infoMode ^= 1;
        LobbySe_Play(0x39);
    } else if (pad & 0x2000) {
        if (Shop_CanBuy(id, list->count + 1) == 1) {
            list->count++;
            LobbySe_Play(1);
        }
    } else if (pad & 0x1000) {
        if (list->count > 1) {
            list->count--;
            LobbySe_Play(1);
        }
    } else if (pad & 0x800) {
        if (list->count > 1) {
            list->count = 1;
            LobbySe_Play(1);
        }
    } else if (pad & 0x400) {
        k = list->count;
        old = k;
        for (; k <= 100; k++) {
            if (Shop_CanBuy(id, k) == 0) {
                list->count = k - 1;
                break;
            }
        }
        if (list->count != old) {
            LobbySe_Play(1);
        }
    }
    return 2;
}

u8 Equip_GetClass(EquipItem *e);
void Shop_BuildStock(ItemListWin *list, s16 page);
s32 Shop_CanBuy(u16 item, s16 n);
s32 Menu_PushNoticeOnce(s32 arg0);

/* The item type read as gItemData[id].type: through a byte offset (id * 0x18) MWCC
 * shares id's mask with the 0xAC test, the game masks id again there. */
s32 Shop_BuyListInput(u16 pad, ItemListWin *list)
{
    u16 id;
    u8 cls = Equip_GetClass((EquipItem *) gSave.equip);
    u8 type;

    if (pad & 0x20) {
        id = list->ent[list->cur].id;
        if (Shop_CanBuy(id, 1) == 1) {
            type = gItemData[id].type;
            if (type == 3 && cls != 0xA) {
                Menu_PushNoticeOnce(0xE);
            } else if (type == 2 && cls != 5 && cls != 1) {
                Menu_PushNoticeOnce(0);
            } else if (id == 0xAC && gSave.equip[0].type != 6) {
                Menu_PushNoticeOnce(0xD);
            }
            LobbySe_Play(8);
            return 0;
        }
        LobbySe_Play(9);
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        list->infoMode ^= 1;
        LobbySe_Play(0x39);
    } else if (pad & 0x80) {
        if (list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
            if (++list->infoPage >= 3) {
                list->infoPage = 0;
            }
            LobbySe_Play(8);
        }
    } else if (pad & 0xC00) {
        if (list->pages > 1) {
            list->page = Menu_WrapLeftRight(list->page, list->pages);
            Shop_BuildStock(list, list->page);
        }
    } else {
        list->cur = Menu_WrapUpDown(list->cur, list->n);
    }
    return 2;
}

static s32 Shop_IsUnlockItem(s32 x)
{
    switch (x) {
    case 0x26D:
    case 0x26E:
    case 0x26F:
    case 0x270:
        return 1;
    }
    return 0;
}

static s32 Shop_IsOnceOnlyItem(u16 item)
{
    switch (item & 0xFFFF) {
    case 0x2C1:
    case 0x2C2:
        return 1;
    }
    return 0;
}

s32 Shop_GetBuyPrice(s32 item)
{
    s32 ret;

    ret = gItemData[item].price;
    if ((Game_IsOnline() == 1) && (Village_Is3924Kind4() == 1)) {
        ret = ret >> 1;
    }
    return ret;
}

s32 Item_GetSellPrice(s32 item)
{
    return gItemData[item].value;
}

s32 SavePouch_Space(u16 item);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 Item_IsRecordOnly(u16 item);

s32 Shop_CanBuy(u16 item, s16 n)
{
    s32 SavePouch_Count(u16 item);
    s16 space;
    s32 price;
    u32 money;

    if (Item_IsRecordOnly(item) == 1) {
        if (n > 1) {
            return 0;
        }
        if (Save_IsItemObtained(&gSave, item) == 1) {
            return 0;
        }
    } else if (Shop_IsUnlockItem(item) == 1 || Shop_IsOnceOnlyItem(item) == 1) {
        if (Save_IsItemObtained(&gSave, item) == 1 || n > 1) {
            return 0;
        }
    } else if (gItemData[item].max == 0xFF) {
        if (n > 1 || SavePouch_Count(item) != 0 || SavePouch_Space(item) == -1) {
            return 0;
        }
    } else {
        space = SavePouch_Space(item);
        if (space == -1) {
            return 0;
        }
        if (space < n) {
            return 0;
        }
    }
    price = Shop_GetBuyPrice(item);
    money = gSave.money;
    return (s32)money >= price * n;
}

void Font_SetPos(s16 x, s16 y);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Sjis_Widen(char *, char *);
s32 sprintf(char *buf, char *fmt, ...);
/* unit's own declaration: as the file's second declaration of it (s8 in sub_main unk_314E00.c) */
s32 Kbd_IsOpen(void);
s16 Lobby_DrawSprite155(s16 x, s16 y, s32 i);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
struct DecoData *Item_GetDecoration(u16 item);
s32 ItemBox_Total(u16 item);
s32 ItemBox_Room(u16 item);
void Deco_DrawDetails(u16 item, s16 x, s16 y, s8 attr, u8 dim);
extern char *gButtonGuideTexts[];
extern char *gNeededSlotsTexts[];
extern char *D_0080C09C;
extern char *gCrossMarkTexts[];
extern char *D_0080C04C;
extern char *gItemNames[];
extern s32 gTownMode;

/*
 * The item info panel: the name, rarity and icon, then (mode 0) how many the
 * pouch and the box hold, or (mode 1) the item's details (Deco_DrawDetails).
 * The game calls SavePouch_Count without an argument: no prototype was in
 * scope (an implicit declaration), so the file's other callers declare it in
 * their own blocks. SavePouch_Count reads a0 there, which holds item & 0xFFFF,
 * left by Shop_IsOnceOnlyItem(item) (which keeps a0); ItemInfo_Draw stays recompiled.
 */
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color)
{
    char num[16];
    char wide[16];
    s16 off;
    s32 total;
    s32 lx;
    s16 ty;
    s32 right;
    s32 have;
    u8 *deco;
    s32 k;
    s32 ly;

    lx = x;
    off = 0;
    total = 0;
    ty = y + 0x44;
    Font_SetSpacing(18, 18);
    right = (s16)(lx + w);
    Font_SetPos(right - 0x3C, ty);
    Font_SetColor(color);
    Text_PrintfColored(gButtonGuideTexts[0]);
    if (Kbd_IsOpen() == 0 && (gPadState[0].pressed & 0x200)) {
        off = 2;
    }
    Lobby_DrawSprite155((s16)(right - 0x3C) - 0x1A, ty - 2 + off, 3);
    Font_SetSpacing(18, 18);
    y += 8;
    Item_DrawIcon(lx + 10, y, 0x36, item);
    k = item * 0x18;
    Item_DrawRarity((s16)(lx + 10) + 8, y + 0x36, (&gItemData[0].rarity)[k]);
    if (mode == 0) {
        Font_SetPos(lx + 0x50, y);
        Font_SetColor(color);
        Text_PrintfColored("%s", gItemNames[item]);
        if (Item_IsDeco(item) == 1) {
            deco = (u8 *)Item_GetDecoration(item);
            Font_SetPos(lx + 0x50, y += 0x17);
            sprintf(num, "%d", *deco);
            Sjis_Widen(num, wide);
            Text_PrintfColored("%s%s", gNeededSlotsTexts[0], wide);
        }
        Font_SetPos(lx + 0x50, y += 0x17);
        Text_PrintfColored("%s%s", D_0080C09C, gCrossMarkTexts[0]);
        Font_SetSpacing(0x1C, 0x14);
        if (Shop_IsUnlockItem(item) == 1) {
            if (Save_IsItemObtained(&gSave, item) == 1) {
                have = 1;
            } else {
                have = 0;
            }
        } else if (Item_IsRecordOnly(item) == 1 || Shop_IsOnceOnlyItem(item) == 1) {
            if (Save_IsItemObtained(&gSave, item) == 1) {
                have = 1;
            } else {
                have = 0;
            }
        } else {
            have = SavePouch_Count();
            total = ItemBox_Total(item);
            if (total > 999) {
                total = 999;
            }
        }
        if (extra > 0) {
            have += extra;
        }
        if ((&gItemData[0].max)[k] == 0xFF) {
            Font_SetColor(color);
            ly = y;
            Font_SetPos(lx + 0x98, ly - 1);
            if (have == 0) {
                Text_PrintfColored("%2d", have);
            } else {
                Text_Printf("%s", D_0080C04C);
            }
            Font_SetColor(color);
            Font_SetPos((s16)(lx + 0x98) + 0x1C, ly - 1);
            if (total == 0) {
                Text_PrintfColored("(%3d)", total);
            } else {
                s16 bx = lx + 0x98;

                Font_SetPos(bx + 0x1C, ly - 1);
                Text_Printf("(", D_0080C04C);
                Font_SetPos(bx + 0x23, ly - 1);
                Text_Printf(" %s", D_0080C04C);
                Font_SetColor(color);
                Font_SetPos(bx + 0x1C, ly - 1);
                Text_Printf("    )");
            }
        } else {
            ly = y;
            Font_SetPos(lx + 0x98, ly - 1);
            if (have >= (&gItemData[0].max)[k]) {
                Text_Printf("~C02%2d", have);
                Font_SetPos(lx + 0x98, ly - 1);
                Font_SetColor(color);
                Text_PrintfColored("  (%3d)", total);
                if (ItemBox_Room(item) == 0) {
                    Font_SetPos(lx + 0x98, ly - 1);
                    Font_SetColor(0x807F212E);
                    Text_PrintfColored("   %3d", total);
                }
            } else {
                Font_SetColor(color);
                Text_PrintfColored("%2d(%3d)", have, total);
                if (ItemBox_Room(item) == 0 && ItemBox_Total(item) != 0) {
                    Font_SetPos(lx + 0x98, ly - 1);
                    Text_Printf("   ~C02%3d", total);
                }
            }
        }
    } else if (color == 0x807F7F7F) {
        if (gTownMode == 9) {
            Deco_DrawDetails(item, lx + 0x50, y, attr, 0);
        } else {
            Deco_DrawDetails(item, lx + 0x50, y, attr | 0x10, 0);
        }
    } else {
        if (gTownMode == 9) {
            Deco_DrawDetails(item, lx + 0x50, y, attr, 1);
        } else {
            Deco_DrawDetails(item, lx + 0x50, y, attr | 0x10, 1);
        }
    }
}

s16 Rarity_Color(u8 i);
s32 sprintf(char *buf, char *fmt, ...);
extern char *gRarePrefixTexts[];
void Font_SetPos(s16 x, s16 y);
void Font_SetSpacing(u8 a, u8 b);
void Text_DrawShadow(s16 x, s16 y, s32 c1, s32 c2, char *s); /* unit's own declaration: the third and fourth parameters s32 (the definition has s16); with the definition's type Item_DrawRarity by 14 of 39 words differ */

/* "<label><rarity + 1>" at x, y in the colour of the rarity */
void Item_DrawRarity(s16 x, s16 y, u8 rarity)
{
    char buf[32];
    s16 color;

    Font_SetPos(x, y);
    Font_SetSpacing(18, 18);
    color = Rarity_Color(rarity);
    sprintf(buf, "%s%d", gRarePrefixTexts[0], rarity + 1);
    Text_DrawShadow(x, y, 1, color, buf);
}

typedef struct SprUV {
    s16 u;
    s16 v;
} SprUV;
typedef struct SprRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    s32 color;
    SprUV uv0;
    SprUV uv1;
} SprRect;
struct Sprite;
s32 Sprite_Draw(struct Sprite *s);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
extern u32 gItemColors[];

void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);

/* r.x is stored before icon + 1 (a statement of its own) and the other fields. */
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item)
{
    s32 k;
    SprRect r;
    s16 *u1;
    s16 *v1;
    s32 *col;
    s16 *u0;
    s16 *v0;
    s32 icon;

    k = item * 0x18;
    icon = ((u8 *)&gItemData[0].icon)[k];
    r.x = x;
    icon = icon + 1;
    r.h = size;
    r.y = y;
    r.w = 1.25f * size;
    u0 = &r.uv0.u;
    v0 = &r.uv0.v;
    u1 = &r.uv1.u;
    v1 = &r.uv1.v;
    col = &r.color;
    *u0 = (icon & 7) * 32 + 1;
    *v0 = (icon >> 3) * 32 + 1;
    *u1 = (icon & 7) * 32 + 31;
    *v1 = (icon >> 3) * 32 + 31;
    *col = gItemColors[((u8 *)&gItemData[0].color)[k]];
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    Sprite_Draw((struct Sprite *)&r);
    if (((u8 *)&gItemData[0].flags2)[k] & 1) {
        *u0 = 0x80;
        *v0 = 0xE0;
        *u1 = *u0 + 0x20;
        *v1 = *v0 + 0x20;
        *col = -1;
        Sprite_Draw((struct Sprite *)&r);
    }
}

void Font_SetColor(s32 color);
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Sjis_Widen(char *, char *);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
char *Text_FillDollar(char *fmt, s32 n);
extern char *D_0080D164;
extern char *gPriceConfirmTexts[];
extern char *D_0080D184[];
extern char *D_0080D188[];
extern char *gItemNames[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;

/* the shop's buy window */
void Shop_DrawBuy(void *m)
{
    ItemListWin *list = m;
    s32 price;
    s16 x = *(s16 *)list;
    u16 id;
    s16 y = 0x31;
    s16 i;
    s32 item;
    char num[16];
    char buf[32];

    y += 10;
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x10E, 0x12, 0xA0, 0xFFB0B000, D_0080D104[0], 2, 1);
    switch (list->unk38) {
    case 0:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFDDDD, 0);
        Icon_Draw(x + 0x73, 0x66, 0x78, 0xC8, 0x60FFFFFF, 0x24);
        break;
    case 1:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFD3EDDC, 0);
        Icon_Draw(x + 0x64, 0x7F, 0x96, 0x96, 0x60FFFFFF, 0x25);
        break;
    case 3:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFFFDD, 0);
        Icon_Draw(x + 0x50, 0x7F, 0xBE, 0x96, 0x60FFFFFF, 0x21);
        break;
    case 4:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFFFDD, 0);
        Icon_Draw(x + 0x64, 0x7F, 0x96, 0x96, 0x60FFFFFF, 0x22);
        break;
    default:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFFFDD, 0);
        Icon_Draw(x + 0x64, 0x7F, 0x96, 0x96, 0x60FFFFFF, 0x26);
        break;
    }
    Menu_DrawMoney(0x1E0, 0x10, 0);
    Font_SetColor(0x80000000);
    Menu_DrawPageNum(x + 0xFA, y, list->page, list->pages, 1);
    Font_SetSpacing(20, 20);
    for (i = 0; i < list->n; i++) {
        id = list->ent[i].id;
        price = Shop_GetBuyPrice(id);
        if (Shop_CanBuy(id, 1) == 1) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Font_SetSpacing(20, 20);
        y += 0x18;
        Font_SetPos(x + 0x2E, y);
        Text_PrintfColored("%s", gItemNames[id]);
        Font_SetSpacing(20, 20);
        Font_SetPos(x + 0xF8, y);
        Text_PrintfColored("% 7dz", price);
        Item_DrawIcon(x + 0x10, y - 1, 0x18, id);
        if (list->cur == i) {
            Menu_DrawCursorRow(x + 0x2E, y, 0xFA, 0x32, 0xC0106060, 0xFF106060);
        }
    }
    id = list->ent[list->cur].id;
    Menu_DrawDashLine(x + 0x14, 0xFB, 0x136, 0xFF000000);
    if (Item_IsDeco(id) != 0) {
        ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, item = id, 0, list->infoPage, 0x80101010);
    } else {
        ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, item = id, 0, -1, 0x80101010);
    }
    Font_SetSpacing(18, 18);
    switch (list->state) {
    case 0:
    case 1:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010,
                      list->unk3D == 0 ? D_0080D188[0] : D_0080D184[0]);
        break;
    case 2:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010, D_0080D164);
        Font_SetPos(0x1FB, 0x184);
        sprintf(num, "%02d", list->count);
        Sjis_Widen(num, buf);
        Text_PrintfColored(buf);
        if (Shop_CanBuy(id, list->count + 1) == 1 && list->count < 99) {
            Icon_DrawMode(0x1FC, 0x16E, 0x24, 0x16, 0xFF00FF00, 0x29, 2);
        }
        if (list->count > 1) {
            Icon_DrawMode(0x1FD, 0x196, 0x24, 0x16, 0xFF00FF00, 0x28, 2);
        }
        break;
    case 3:
        price = Shop_GetBuyPrice(item);
        price *= list->count;
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010,
                      Text_FillDollar(gPriceConfirmTexts[0], price));
        Font_SetPos(0x1A2, 0x192);
        Text_Printf(gYesNoTexts[*((u8 *)gLobbyMenu + 0x3C)]);
        break;
    }
}

void Font_SetColor(s32 color);
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Win_Draw(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, s16 kind); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), Shop_DrawSell 31 of 340 words, Win_DrawText 43 of 46 words differ */
u32 Color_PulseAlpha(u32 color);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Sjis_Widen(char *, char *);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
char *Text_FillDollar(char *fmt, s32 n);
extern char *gSellLabelTexts[];
extern char *gSellCountTexts[];
extern char *D_0080D174;
extern char *D_0080D18C[];
extern char *gItemNames[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;

/* the pouch's sell window */
void Shop_DrawSell(void *m)
{
    s32 SavePouch_Count(u16 item);
    ItemListWin *list = m;
    s32 price;
    s16 x = *(s16 *)list + 4;
    u16 id;
    s16 y = 0x31;
    s16 i;
    s16 have;
    char num[16];
    char buf[32];

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x10E, 0x12, 0xA0, 0xFFB0B000, gSellLabelTexts[0], 2, 1);
    Font_SetColor(0x807F7F7F);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    Menu_DrawPageNum(x + 0xF0, 0x3B, list->page, list->pages, 1);
    Win_Draw(x, y, 0x15E, 0x132, list->color, 0x80202020, 2);
    y += 10;
    Font_SetColor(0x80000000);
    Font_SetSpacing(20, 20);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < list->n; i++) {
        id = list->ent[i].id;
        price = list->ent[i].unk2;
        y += 0x18;
        if (list->cur == i) {
            Gfx_DrawLine(x + 0x14, y - 1, 0x136, 0x14, Color_PulseAlpha(0x8080));
        }
        if (id == 0) {
            Font_SetPos(x + 0x14, y);
            Text_PrintfColored("------------------------------");
        } else {
            Font_SetSpacing(20, 20);
            Font_SetPos(x + 0x2E, y);
            Text_PrintfColored("%s", gItemNames[id]);
            Font_SetSpacing(20, 20);
            Font_SetPos(x + 0xF8, y);
            Text_PrintfColored("% 7dz", price);
            Item_DrawIcon(x + 0x10, y - 1, 0x18, id);
        }
    }
    id = list->ent[list->cur].id;
    Gfx_DrawLine(x + 0x14, 0xFB, 0x136, 2, list->color);
    if (id != 0) {
        if (Item_IsDeco(id) != 0) {
            ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, id, 0, list->infoPage, 0x807F7F7F);
        } else {
            ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, id, 0, -1, 0x807F7F7F);
        }
    }
    Font_SetSpacing(18, 18);
    switch (list->state) {
    case 0:
    case 1:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010, D_0080D18C[0]);
        break;
    case 2:
        have = SavePouch_Count(id);
        if (gItemData[id].max == 0xFF) {
            have = 1;
        }
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010, gSellCountTexts[0]);
        Font_SetPos(0x1E9, 0x184);
        sprintf(num, "%02d", list->count);
        Sjis_Widen(num, buf);
        Text_PrintfColored(buf);
        if (list->count < have) {
            Icon_DrawMode(0x1EA, 0x16E, 0x24, 0x16, 0xFF00FF00, 0x29, 2);
        }
        if (list->count > 1) {
            Icon_DrawMode(0x1EB, 0x196, 0x24, 0x16, 0xFF00FF00, 0x28, 2);
        }
        break;
    case 3:
        price = list->ent[list->cur].unk2;
        price *= list->count;
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010,
                      Text_FillDollar(D_0080D174, price));
        Font_SetPos(0x190, 0x192);
        Text_Printf(gYesNoTexts[*((u8 *)gLobbyMenu + 0x3C)]);
        break;
    }
}

void Text_SetSize(s32);
void Shop_DrawBuy(void *arg0);
void Shop_DrawSell(void *arg0);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
extern u8 gMenuWork[];

void ItemShop_Draw(u8 *t)
{
    ShopMenu *c = (ShopMenu *)gMenuWork;
    ItemListWin *m = &c->list;

    Text_SetSize(*(s32 *)(t + 0x18));
    switch (c->state) {
    case 1:
        Menu_DrawLabelList(0x1CC, c->lines, c->n, c->cur, 0xFFB0B000);
        break;
    case 2:
        Shop_DrawBuy(m);
        break;
    case 3:
        Shop_DrawSell(m);
        break;
    }
}

void Text_Printf(const char *fmt, ...);
void Win_Draw(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, s16 kind); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), Shop_DrawSell 31 of 340 words, Win_DrawText 43 of 46 words differ */
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);

void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s)
{
    Win_Draw(x, y, w, h, c1, c2, 0);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(x + 10, y + 12);
    if (s != NULL) {
        Text_Printf(s);
    }
}

/* unit's own declaration: with the definition's s8 (void) (sub_main/unk_314E00.c), ItemInfo_Draw: 411 of 462 words differ, LobbyPad_Read: 33 of 47 words differ */
s32 Kbd_IsOpen(void);
extern u8 gMenuInputLock;

u16 LobbyPad_Read(s8 port)
{
    u16 pad = 0;

    if (gMenuInputLock != 0) {
        return 0;
    }
    if (Kbd_IsOpen() == 0) {
        pad = (gPadState[port].repeat & 0x3C00) | gPadState[port].pressed;
    }
    if (pad != 0) {
        gMenuInputLock = 3;
    }
    return pad;
}

/* unk_7A1120's .data (0x80B1A0-0x80DC70) */
extern char D_0083BA90[];
extern char D_0083BA98[];
extern char D_0083BAA0[];
extern char D_0083BAA8[];
extern char D_0083BAB0[];
extern char D_0083BAB8[];
extern char D_0083BAC0[];
extern char D_0083BAC8[];
extern char D_0083BAD0[];
extern char D_0083BAE0[];
extern char D_0083BAF8[];
extern char D_0083BB00[];
extern char D_0083BB08[];
extern char D_0083BB10[];
extern char D_0083BB18[];
extern char D_0083BB28[];
extern char D_0083BB30[];
extern char D_0083BB38[];
extern char D_0083BB40[];
extern char D_0083BB48[];
extern char D_0083BB50[];
extern char D_0083BB58[];
extern char D_0083BB60[];
extern char D_0083BB68[];
extern char D_0083BB70[];
extern char D_0083BB78[];
extern char D_0083BB88[];
extern char D_0083BB98[];
extern char D_0083BBA8[];
extern char D_0083BBC0[];
extern char D_0083BBD8[];
extern char D_0083BBE0[];
extern char D_0083BBF0[];
extern char D_0083BC00[];
extern char D_0083BC10[];
extern char D_0083BC18[];
extern char D_0083BC28[];
extern char D_0083BC38[];
extern char D_0083BC48[];
extern char D_0083BC50[];
extern char D_0083BC60[];
extern char D_0083BC70[];
extern char D_0083BC80[];
extern char D_0083BC90[];
extern char D_0083BCA0[];
extern char D_0083BCB0[];
extern char D_0083BCC8[];
extern char D_0083BCD8[];
extern char D_0083BCE8[];
extern char D_0083BD00[];
extern char D_0083BD18[];
extern char D_0083BD28[];
extern char D_0083BD40[];
extern char D_0083BD58[];
extern char D_0083BD68[];
extern char D_0083BD78[];
extern char D_0083BD90[];
extern char D_0083BDB0[];
extern char D_0083BDD0[];
extern char D_0083BDF0[];
extern char D_0083BE08[];
extern char D_0083BE18[];
extern char D_0083BE28[];
extern char D_0083BE38[];
extern char D_0083BE48[];
extern char D_0083BE58[];
extern char D_0083BE68[];
extern char D_0083BE70[];
extern char D_0083BE78[];
extern char D_0083BE90[];
extern char D_0083BEA0[];
extern char D_0083BEB0[];
extern char D_0083BEC0[];
extern char D_0083BED0[];
extern char D_0083BEE0[];
extern char D_0083BEF0[];
extern char D_0083BF00[];
extern char D_0083BF10[];
extern char D_0083BF20[];
extern char D_0083BF30[];
extern char D_0083BF40[];
extern char D_0083BF50[];
extern char D_0083BF60[];
extern char D_0083BF70[];
extern char D_0083BF80[];
extern char D_0083BF90[];
extern char D_0083BFA0[];
extern char D_0083BFB0[];
extern char D_0083BFD0[];
extern char D_0083BFE8[];
extern char D_0083BFF8[];
extern char D_0083C008[];
extern char D_0083C018[];
extern char D_0083C030[];
extern char D_0083C050[];
extern char D_0083C060[];
extern char D_0083C078[];
extern char D_0083C080[];
extern char D_0083C088[];
extern char D_0083C098[];
extern char D_0083C0A0[];
extern char D_0083C0B8[];
extern char D_0083C0D0[];
extern char D_0083C0E8[];
extern char D_0083C100[];
extern char D_0083C120[];
extern char D_0083C150[];
extern char D_0083C170[];
extern char D_0083C180[];
extern char D_0083C1A0[];
extern char D_0083C1A8[];
extern char D_0083C1B8[];
extern char D_0083C1D0[];
extern char D_0083C1F0[];
extern char D_0083C208[];
extern char D_0083C218[];
extern char D_0083C228[];
extern char D_0083C240[];
extern char D_0083C258[];
extern char D_0083C260[];
extern char D_0083C270[];
extern char D_0083C280[];
extern char D_0083C290[];
extern char D_0083C2A0[];
extern char D_0083C2A8[];
extern char D_0083C2B8[];
extern char D_0083C2C0[];
extern char D_0083C2D0[];
extern char D_0083C2E0[];
extern char D_0083C2F0[];
extern char D_0083C300[];
extern char D_0083C310[];
extern char D_0083C320[];
extern char D_0083C328[];
extern char D_0083C340[];
extern char D_0083C370[];
extern char D_0083C3A0[];
extern char D_0083C3C8[];
extern char D_0083C3D0[];
extern char D_0083C3E0[];
extern char D_0083C400[];
extern char D_0083C440[];
extern char D_0083C470[];
extern char D_0083C498[];
extern char D_0083C4B0[];
extern char D_0083C4C8[];
extern char D_0083C4D8[];
extern char D_0083C4E8[];
extern char D_0083C500[];
extern char D_0083C520[];
extern char D_0083C540[];
extern char D_0083C560[];
extern char D_0083C580[];
extern char D_0083C598[];
extern char D_0083C5A8[];
extern char D_0083C5B8[];
extern char D_0083C5C8[];
extern char D_0083C5D0[];
extern char D_0083C5D8[];
extern char D_0083C5E0[];
extern char D_0083C5F0[];
extern char D_0083C610[];
extern char D_0083C630[];
extern char D_0083C660[];
extern char D_0083C690[];
extern char D_0083C6A8[];
extern char D_0083C6B8[];
extern char D_0083C6C8[];
extern char D_0083C6D8[];
extern char D_0083C6E8[];
extern char D_0083C6F8[];
extern char D_0083C700[];
extern char D_0083C710[];
extern char D_0083C730[];
extern char D_0083C760[];
extern char D_0083C790[];
extern char D_0083C7C0[];
extern char D_0083C810[];
extern char D_0083C830[];
extern char D_0083C860[];
extern char D_0083C890[];
extern char D_0083C8B0[];
extern char D_0083C8D0[];
extern char D_0083C8E0[];
extern char D_0083C8F0[];
extern char D_0083C908[];
extern char D_0083C918[];
extern char D_0083C928[];
extern char D_0083C930[];
extern char D_0083C940[];
extern char D_0083C950[];
extern char D_0083C960[];
extern char D_0083C980[];
extern char D_0083C9A0[];
extern char gHunterNotesTexts_8[];
extern char D_0083C9D0[];
extern char D_0083C9E8[];
extern char D_0083C9F0[];
extern char D_0083C9F8[];
extern char D_0083CA08[];
extern char D_0083CA10[];
extern char D_0083CA28[];
extern char D_0083CA38[];
extern char D_0083CA40[];
extern char D_0083CA60[];
extern char D_0083CA70[];
extern char D_0083CA90[];
extern char D_0083CAB0[];
extern char D_0083CAC8[];
extern char D_0083CAD8[];
extern char D_0083CAE8[];
extern char D_0083CAF0[];
extern char D_0083CB00[];
extern char D_0083CB10[];
extern char D_0083CB20[];
extern char D_0083CB30[];
extern char D_0083CB38[];
extern char D_0083CB50[];
extern char D_0083CB80[];
extern char D_0083CBD0[];
extern char D_0083CC00[];
extern char D_0083CC20[];
extern char D_0083CC30[];
extern char D_0083CC40[];
extern char D_0083CC70[];
extern char D_0083CCC0[];
extern char D_0083CD10[];
extern char D_0083CD40[];
extern char D_0083CD90[];
extern char D_0083CDB0[];
extern char D_0083CDC8[];
extern char D_0083CDD8[];
extern char D_0083CDE8[];
extern char D_0083CDF8[];
extern char D_0083CE08[];
extern char D_0083CE18[];
extern char D_0083CE30[];
extern char D_0083CE60[];
extern char D_0083CE80[];
extern char D_0083CEB0[];
extern char D_0083CEC8[];
extern char D_0083CED8[];
extern char D_0083CEF0[];
extern char D_0083CF08[];
extern char D_0083CF18[];
extern char D_0083CF20[];
extern char D_0083CF28[];
extern char D_0083CF30[];
extern char D_0083CF38[];
extern char D_0083CF40[];
extern char D_0083CF48[];
extern char D_0083CF58[];
extern char D_0083CF60[];
extern char D_0083CF68[];
extern char D_0083CF80[];
extern char D_0083CFC0[];
extern char D_0083CFD0[];
extern char D_0083CFE0[];
extern char D_0083CFE8[];
extern char D_0083D000[];
extern char D_0083D018[];
extern char D_0083D028[];
extern char D_0083D030[];
extern char D_0083D040[];
extern char D_0083D050[];
extern char D_0083D060[];
extern char D_0083D070[];
extern char D_0083D088[];
extern char D_0083D0A0[];
extern char D_0083D0B8[];
extern char D_0083D0C8[];
extern char D_0083D0D8[];
extern char D_0083D0E8[];
extern char D_0083D0F8[];
extern char D_0083D100[];
extern char D_0083D118[];
extern char D_0083D128[];
extern char D_0083D130[];
extern char D_0083D140[];
extern char D_0083D158[];
extern char D_0083D168[];
extern char D_0083D170[];
extern char D_0083D180[];
extern char D_0083D188[];
extern char D_0083D190[];
extern char D_0083D198[];
extern char D_0083D1B0[];
extern char D_0083D220[];
extern char D_0083D280[];
extern char D_0083D2B0[];
extern char D_0083D310[];
extern char D_0083D318[];
extern char D_0083D328[];
extern char D_0083D338[];
extern char D_0083D340[];
extern char D_0083D350[];
extern char D_0083D360[];
extern char D_0083D370[];
extern char D_0083D378[];
extern char D_0083D380[];
extern char D_0083D388[];
extern char D_0083D398[];
extern char D_0083D3A8[];
extern char D_0083D3B8[];
extern char D_0083D3C8[];
extern char D_0083D3D8[];
extern char D_0083D3E8[];
extern char D_0083D3F8[];
extern char D_0083D400[];
extern char D_0083D420[];
extern char D_0083D440[];
extern char D_0083D458[];
extern char D_0083D468[];
extern char D_0083D478[];
extern char D_0083D488[];
extern char D_0083D498[];
extern char D_0083D4A8[];
extern char D_0083D4B8[];
extern char D_0083D4C0[];
extern char D_0083D4D0[];
extern char D_0083D500[];
extern char D_0083D520[];
extern char D_0083D540[];
extern char D_0083D550[];
extern char D_0083D560[];
extern char D_0083D570[];
extern char D_0083D590[];
extern char D_0083D5A0[];
extern char D_0083D5A8[];
extern char D_0083D5B0[];
extern char D_0083D5B8[];
extern char D_0083D5C8[];
extern char D_0083D5D8[];
extern char D_0083D5E8[];
extern char D_0083D600[];
extern char D_0083D620[];
extern char D_0083D680[];
extern char D_0083D700[];
extern char D_0083D718[];
extern char D_0083D720[];
extern char D_0083D728[];
extern char D_0083D730[];
extern char D_0083D738[];
extern char D_0083D740[];
extern char D_0083D748[];
extern char D_0083D750[];
extern char D_0083D758[];
extern char D_0083D760[];
extern char D_0083D768[];
extern char D_0083D770[];
extern char D_0083D778[];
extern char D_0083D780[];
extern char D_0083D788[];
extern char D_0083D790[];
extern char D_0083D7A0[];
extern char D_0083D7B8[];
extern char D_0083D7C0[];
extern char D_0083D7E0[];
extern char D_0083D800[];
extern char D_0083D820[];
extern char D_0083D838[];
extern char D_0083D840[];
extern char D_0083D850[];
extern char D_0083D860[];
extern char D_0083D880[];
extern char D_0083D898[];
extern char D_0083D8A8[];
extern char D_0083D8B8[];
extern char D_0083D8C8[];
extern char D_0083D8D8[];
extern char D_0083D8E8[];
extern char D_0083D8F8[];
extern char D_0083D908[];
extern char D_0083D918[];
extern char D_0083D928[];
extern char D_0083D940[];
extern char D_0083D960[];
extern char D_0083D978[];
extern char D_0083D988[];
extern char D_0083D9A0[];
extern char D_0083D9D0[];
extern char D_0083DA00[];
extern char D_0083DA50[];
extern char D_0083DAA0[];
extern char D_0083DAD0[];
extern char D_0083DAF0[];
extern char D_0083DB20[];
extern char D_0083DB70[];
extern char D_0083DBB0[];
extern char D_0083DBE0[];
extern char D_0083DC30[];
extern char D_0083DC78[];
extern char D_0083DC88[];
extern char D_0083DC98[];
extern char D_0083DCA0[];
extern char D_0083DCB0[];
extern char D_0083DCC0[];
extern char D_0083DCE0[];
extern char D_0083DD00[];
extern char D_0083DD10[];
extern char D_0083DD20[];
extern char D_0083DD30[];
extern char D_0083DD40[];
extern char D_0083DD50[];
extern char D_0083DD60[];
extern char D_0083DD70[];
extern char D_0083DD80[];
extern char D_0083DD90[];
extern char D_0083DDA0[];
extern char D_0083DDC0[];
extern char D_0083DDD8[];
extern char D_0083DDF0[];
extern char D_0083DE30[];
extern char D_0083DE70[];
extern char D_0083DEB0[];
extern char D_0083DEE0[];
extern char D_0083DF30[];
extern char D_0083DF90[];
extern char D_0083DFB0[];
extern char D_0083DFD0[];
extern char D_0083DFE8[];
extern char D_0083DFF8[];
extern char D_0083E008[];
extern char D_0083E020[];
extern char D_0083E038[];
extern char D_0083E050[];
extern char D_0083E070[];
extern char D_0083E090[];
extern char D_0083E0B0[];
extern char D_0083E0D0[];
extern char D_0083E0F0[];
extern char D_0083E108[];
extern char D_0083E118[];
extern char D_0083E130[];
extern char D_0083E150[];
extern char D_0083E170[];
extern char D_0083E1C0[];
extern char D_0083E260[];
extern char D_0083E2C0[];
extern char D_0083E350[];
extern char D_0083E3B0[];
extern char D_0083E420[];
extern char D_0083E490[];
extern char D_0083E530[];
extern char D_0083E590[];
extern char D_0083E610[];
extern char D_0083E670[];
extern char D_0083E6A0[];
extern char D_0083E6F0[];
extern char D_0083E740[];
extern char D_0083E790[];
extern char D_0083E7E0[];
extern char D_0083E830[];
extern char D_0083E880[];
extern char D_0083E8D0[];
extern char D_0083E930[];
extern char D_0083E9A0[];
extern char D_0083E9D0[];
extern char D_0083EA00[];
extern char D_0083EA30[];
extern char D_0083EA50[];
extern char D_0083EA90[];
extern char D_0083EAB0[];
extern char D_0083EB00[];
extern char D_0083EB70[];
extern char D_0083EBC0[];
extern char D_0083EC00[];
extern char D_0083EC30[];
extern char D_0083EC60[];
extern char D_0083EC80[];
extern char D_0083EC98[];
extern char D_0083ECA8[];
extern char D_0083ECC0[];
extern char D_0083ECD8[];
extern char D_0083ECF0[];
extern char D_0083ED10[];
extern char D_0083ED30[];
extern char D_0083ED50[];
extern char D_0083ED70[];
extern char D_0083ED90[];
extern char D_0083EDA8[];
extern char D_0083EDB0[];
extern char D_0083EDC0[];
extern char D_0083EDE0[];
extern char D_0083EE00[];
extern char D_0083EE38[];
extern char D_0083EE50[];
extern char D_0083EE68[];
extern char D_0083EE80[];
extern char D_0083EE98[];
extern char D_0083EEA8[];
extern char D_0083EEC0[];
extern char D_0083EED8[];
extern char D_0083EEE8[];
extern char D_0083EEF8[];
extern char D_0083EF08[];
extern char D_0083EF18[];
extern char D_0083EF28[];
extern char D_0083EF38[];
extern char D_0083EF48[];
extern char D_0083EF58[];
extern char D_0083EF68[];
extern char D_0083EF78[];
extern char D_0083EF90[];
extern char D_0083EFB0[];
extern char D_0083EFD0[];
extern char D_0083EFF0[];
extern char D_0083F010[];
extern char D_0083F030[];
extern char D_0083F050[];
extern char D_0083F070[];
extern char D_0083F090[];
extern char D_0083F0B0[];
extern char D_0083F0D0[];
extern char D_0083F0F0[];
extern char D_0083F110[];
extern char D_0083F130[];
extern char D_0083F140[];
extern char D_0083F150[];
extern char D_0083F170[];
extern char D_0083F190[];
extern char D_0083F1B0[];
extern char D_0083F1D0[];
extern char D_0083F1F0[];
extern char D_0083F210[];
extern char D_0083F230[];
extern char D_0083F250[];
extern char D_0083F270[];
extern char D_0083F290[];
extern char D_0083F2B0[];
extern char D_0083F2D0[];
extern char D_0083F2E8[];
extern char D_0083F300[];
extern char D_0083F318[];
extern char D_0083F328[];
extern char D_0083F338[];
extern char D_0083F348[];
extern char D_0083F358[];
extern char D_0083F368[];
extern char D_0083F370[];
extern char D_0083F380[];
extern char D_0083F3A0[];
extern char D_0083F3C0[];
extern char D_0083F3D8[];
extern char D_0083F3E8[];
extern char D_0083F3F0[];
extern char D_0083F3F8[];
extern char D_0083F400[];
extern char D_0083F408[];
extern char D_0083F410[];
extern char D_0083F430[];
extern char D_0083F450[];
extern char D_0083F470[];
extern char D_0083F480[];
extern char D_0083F4A0[];
extern char D_0083F4B0[];
extern char D_0083F4C0[];
extern char D_0083F4E0[];
extern char D_0083F4F0[];
extern char D_0083F510[];
extern char D_0083F530[];
extern char D_0083F540[];
extern char D_0083F550[];
extern char D_0083F560[];
extern char D_0083F570[];
extern char D_0083F590[];
extern char D_0083F5A8[];
extern char D_0083F5C0[];
extern char D_0083F5E0[];
extern char D_0083F600[];
extern char D_0083F618[];
extern char D_0083F620[];
extern char D_0083F650[];
extern char D_0083F670[];
extern char D_0083F6A0[];
extern char D_0083F6D0[];
extern char D_0083F700[];
extern char D_0083F720[];
extern char D_0083F750[];
extern char D_0083F770[];
extern char D_0083F7A0[];
extern char D_0083F7C0[];
extern char D_0083F810[];
extern char D_0083F830[];
extern char D_0083F860[];
extern char D_0083F880[];
extern char D_0083F8A0[];
extern char D_0083F8B8[];
extern char D_0083F8D0[];
extern char D_0083F8F0[];
extern char D_0083F910[];
extern char D_0083F940[];
extern char D_0083F970[];
extern char D_0083F9A0[];
extern char D_0083F9C0[];
extern char D_0083F9E0[];
extern char D_0083FA00[];
extern char D_0083FA30[];
extern char D_0083FA78[];
extern char D_0083FA90[];
extern char D_0083FAB0[];
extern char D_0083FAE0[];
extern char D_0083FB10[];
extern char D_0083FB40[];
extern char D_0083FB70[];
extern char D_0083FB90[];
extern char D_0083FBB0[];
extern char D_0083FBE0[];
extern char D_0083FC10[];
extern char D_0083FC30[];
extern char D_0083FC70[];
extern char D_0083FCA0[];
extern char D_0083FCB0[];
extern char D_0083FCD0[];
extern char D_0083FD20[];
extern char D_0083FD40[];
extern char D_0083FD60[];
extern char D_0083FD80[];
extern char D_0083FDB0[];
extern char D_0083FDD0[];
extern char D_0083FE10[];
extern char D_0083FE50[];
extern char D_0083FE80[];
extern char D_0083FEB0[];
extern char D_0083FED0[];
extern char D_0083FF10[];
extern char D_0083FF50[];
extern char D_0083FF90[];
extern char D_0083FFC0[];
extern char D_00840000[];
extern char D_00840040[];
extern char D_00840080[];
extern char D_008400B0[];
extern char D_008400E0[];
extern char D_00840120[];
extern char D_00840160[];
extern char D_00840190[];
extern char D_008401A8[];
extern char D_008401B0[];
extern char D_008401B8[];
extern char D_008401C0[];
extern char D_008401D0[];
extern char D_008401D8[];
extern char D_008401E0[];
extern char D_008401F8[];
extern char D_00840208[];
extern char D_00840220[];
extern char D_00840240[];
extern char D_00840258[];
extern char D_00840268[];
extern char D_00840270[];
extern char D_00840280[];
extern char D_00840288[];
extern char D_00840298[];
extern char D_008402A8[];
extern char D_008402B8[];
extern char D_008402C8[];
extern char D_008402D8[];
extern char D_008402E0[];
extern char D_008402F8[];
extern char D_00840308[];
extern char D_00840318[];
extern char D_00840328[];
extern char D_00840338[];
extern char D_00840348[];
extern char D_00840360[];
extern char D_00840378[];
extern char D_00840390[];
extern char D_008403B0[];
extern char D_008403D0[];
extern char D_008403F0[];
extern char D_00840410[];
extern char D_00840430[];
extern char D_00840450[];
extern char D_00840470[];
extern char D_00840490[];
extern char D_008404B0[];
extern char D_008404D0[];
extern char D_008404F0[];
extern char D_00840510[];
extern char D_00840528[];
extern char D_00840538[];
extern char D_00840550[];
extern char D_00840568[];
extern char D_00840578[];
extern char D_00840588[];
extern char D_008405A0[];
extern char D_008405D0[];
extern char D_008405F0[];
extern char D_00840620[];
extern char D_00840638[];
extern char D_00840648[];
extern char D_00840658[];
extern char D_00840668[];
extern char D_00840678[];
extern char D_00840688[];
extern char D_008406A0[];
extern char D_008406C0[];
extern char D_008406D8[];
extern char D_008406E8[];
extern char D_008406F8[];
extern char D_00840708[];
extern char D_00840718[];
extern char D_00840728[];
extern char D_00840740[];
extern char D_00840760[];
extern char D_00840778[];
extern char D_00840788[];
extern char D_00840798[];
extern char D_008407A8[];
extern char D_008407C0[];
extern char D_008407D8[];
extern char D_008407E8[];
extern char D_008407F8[];
extern char D_00840808[];
extern char D_00840818[];
extern char D_00840828[];
extern char D_00840838[];
extern char D_00840850[];
extern char D_00840870[];
extern char D_00840888[];
extern char D_00840898[];
extern char D_008408A8[];
extern char D_008408B8[];
extern char D_008408C8[];
extern char D_008408D0[];
extern char D_008408E0[];
extern char D_008408F0[];
extern char D_00840900[];
extern char D_00840910[];
extern char D_00840920[];
extern char D_00840930[];
extern char D_00840940[];
extern char D_00840950[];
extern char D_00840990[];
extern char D_008409B0[];
extern char D_008409E0[];
extern char D_00840A10[];
extern char D_00840A30[];
extern char D_00840A70[];
extern char D_00840AB0[];
extern char D_00840AD0[];
extern char D_00840AF0[];
extern char D_00840B10[];
extern char D_00840B30[];
extern char D_00840B50[];
extern char D_00840B70[];
extern char D_00840B90[];
extern char D_00840BB0[];
extern char D_00840BF0[];
extern char D_00840C40[];
extern char D_00840C80[];
extern char D_00840CC0[];
extern char D_00840CF0[];
extern char D_00840D20[];
extern char D_00840D50[];
extern char D_00840DB0[];
extern char D_00840E10[];
extern char D_00840E40[];
extern char D_00840E70[];
extern char D_00840EA0[];
extern char D_00840EC0[];
extern char D_00840F00[];
extern char D_00840F60[];
extern char D_00840F90[];
extern char D_00840FD0[];
extern char D_00841010[];
extern char D_00841048[];
extern char D_00841058[];
extern char D_00841068[];
extern char D_00841078[];
extern char D_00841088[];
extern char D_00841098[];
extern char D_008410B0[];
extern char D_008410C8[];
extern char D_008410E0[];
extern char D_00841100[];
extern char D_00841130[];
extern char D_00841160[];
extern char D_008411B0[];
extern char D_00841240[];
extern char D_00841290[];
extern char D_008412F8[];
extern char D_00841310[];
extern char D_00841330[];
extern char D_00841340[];
extern char D_00841350[];
extern char D_00841370[];
extern char D_00841390[];
extern char D_008413B0[];
extern char D_008413D0[];
extern char D_008413F0[];
extern char D_00841410[];
extern char D_00841420[];
extern char D_00841430[];
extern char D_00841450[];
extern char D_00841470[];
extern char D_00841490[];
extern char D_008414A8[];
extern char D_008414B8[];
extern char D_008414C8[];
extern char D_008414D8[];
extern char D_008414F0[];
extern char D_00841508[];
extern char D_00841510[];
extern char D_00841520[];
extern char D_00841530[];
extern char D_00841540[];
extern char D_00841548[];
extern char D_00841550[];
extern char D_00841558[];
extern char D_00841560[];
extern char D_00841580[];
extern char D_008415A0[];
extern char D_008415C0[];
extern char D_008415E0[];
extern char D_00841600[];
extern char D_00841620[];
extern char D_00841640[];
extern char D_00841660[];
extern char D_00841680[];
extern char D_008416A0[];
extern char D_008416C0[];
extern char D_008416D8[];
extern char D_008416F0[];
extern char D_00841710[];
extern char D_00841728[];
extern char D_00841738[];
extern char D_00841750[];
extern char D_00841770[];
extern char D_00841790[];
extern char D_008417B0[];
extern char D_008417C8[];
extern char D_008417E0[];
extern char D_00841800[];
extern char D_00841820[];
extern char D_00841838[];
extern char D_00841850[];
extern char D_00841860[];
extern char D_00841870[];
extern char D_00841888[];
extern char D_00841890[];
extern char D_008418A0[];
extern char D_008418A8[];
extern char D_008418B0[];
extern char D_008418B8[];
extern char D_008418C8[];
extern char D_008418D8[];
extern char D_008418E8[];
extern char D_008418F8[];
extern char D_00841908[];
extern char D_00841918[];
extern char D_00841920[];
extern char D_00841930[];
extern char D_00841940[];
extern char D_00841960[];
extern char D_00841980[];
extern char D_008419A0[];
extern char D_008419D0[];
extern char D_00841A00[];
extern char D_00841A70[];
extern char D_00841AA0[];
extern char D_00841AD0[];
extern char D_00841B40[];
extern char D_00841B70[];
extern char D_00841BA0[];
extern char D_00841C10[];
extern char D_00841C70[];
extern char D_00841CE0[];
extern char D_00841D20[];
extern char D_00841D78[];
extern char D_00841D88[];
extern char D_00841D98[];
extern char D_00841DA8[];
extern char D_00841DB8[];
extern char D_00841DD0[];
extern char D_00841DE8[];
extern char D_00841E00[];
extern char D_00841E18[];
extern char D_00841E30[];
extern char D_00841E60[];
extern char D_00841E80[];
extern char D_00841EA0[];
extern char D_00841EA8[];
extern char D_00841EB0[];
extern char D_00841EC0[];
extern char D_00841F00[];
extern char D_00841F60[];
extern char D_00841FB0[];
extern char D_00841FE0[];
extern char D_00841FF0[];
extern char D_00841FF8[];
extern char D_00842000[];
extern char D_00842008[];
extern char D_00842010[];
extern char D_00842018[];
extern char D_00842020[];
extern char D_00842030[];
extern char D_00842040[];
extern char D_00842050[];
extern char D_00842060[];
extern char D_00842070[];
extern char D_00842080[];
extern char D_00842090[];
extern char D_008420A0[];
extern char D_008420A8[];
extern char D_008420C0[];
extern char D_008420E0[];
extern char D_00842100[];
extern char D_00842120[];
extern char D_00842140[];
extern char D_00842160[];
extern char D_00842170[];
extern char D_00842190[];
extern char D_008421B0[];
extern char D_008421D0[];
extern char D_008421F0[];
extern char D_00842210[];
extern char D_00842230[];
extern char D_00842260[];
extern char D_00842290[];
extern char D_008422C0[];
extern char D_008422F0[];
extern char D_00842320[];
extern char D_00842340[];
extern char D_00842360[];
extern char D_00842380[];
extern char D_008423A0[];
extern char D_008423C0[];
extern char D_008423E0[];
extern char D_00842400[];
extern char D_00842420[];
extern char D_00842440[];
extern char D_00842470[];
extern char D_00842490[];
extern char D_008424B0[];
extern char D_008424D0[];
extern char D_008424E0[];
extern char D_00842500[];
extern char D_00842530[];
extern char D_00842560[];
extern char D_00842580[];
extern char D_008425B0[];
extern char D_008425D8[];
extern char D_008425E8[];
extern char D_00842600[];
extern char D_00842620[];
extern char D_00842660[];
extern char D_00842680[];
extern char D_008426A0[];
extern char D_008426D0[];
extern char D_00842700[];
extern char D_00842720[];
extern char D_00842750[];
extern char D_00842770[];
extern char D_00842790[];
extern char D_008427B0[];
extern char D_008427D0[];
extern char D_008427E8[];
extern char D_008427F8[];
extern char D_00842810[];
extern char D_00842840[];
extern char D_00842860[];
extern char D_00842880[];
extern char D_008428A0[];
extern char D_008428B0[];
extern char D_008428C8[];
extern char D_008428D8[];
extern char D_008428E8[];
extern char D_00842900[];
extern char D_00842920[];
extern char D_00842930[];
extern char D_00842940[];
extern char D_00842958[];
extern char D_00842968[];
extern char D_00842978[];
extern char D_00842988[];
extern char D_00842990[];
extern char D_008429A0[];
extern char D_008429B8[];
extern char D_008429C8[];
extern char D_008429D8[];
extern char D_008429E8[];
extern char D_008429F8[];
extern char D_00842A10[];
extern char D_00842A28[];
extern char D_00842A38[];
extern char D_00842A40[];
extern char D_00842A58[];
extern char D_00842A70[];
extern char D_00842A88[];
extern char D_00842A98[];
extern char D_00842AB0[];
extern char D_00842AD0[];
extern char D_00842AF0[];
extern char D_00842AF8[];
extern char D_00842B08[];
extern char D_00842B18[];
extern char D_00842B28[];
extern char D_00842B40[];
extern char D_00842B60[];
extern char D_00842C00[];
extern char D_00842C30[];
extern char D_00842C60[];
extern char D_00842C80[];
extern char D_00842D00[];
extern char D_00842D30[];
extern char D_00842D50[];
extern char D_00842D90[];
extern char D_00842DC0[];
extern char D_00842DF0[];
extern char D_00842E10[];
extern char D_00842E40[];
extern char D_00842EC0[];
extern char D_00842F00[];
extern char D_00842F30[];
extern char D_00842F60[];
extern char D_00842FC0[];
extern char D_00842FE0[];
extern char D_00843090[];
extern char D_00843130[];
extern char D_008431C0[];
extern char D_00843260[];
extern char D_00843290[];
extern char D_008432D0[];
extern char D_008432F0[];
extern char D_00843340[];
extern char D_008433E0[];
extern char D_00843480[];
extern char D_008434B0[];
extern char D_008434F0[];
extern char D_00843520[];
extern char D_008435B0[];
extern char D_00843660[];
extern char D_008436F0[];
extern char D_00843730[];
extern char D_00843770[];
extern char D_008437B0[];
extern char D_00843800[];
extern char D_00843840[];
extern char D_00843880[];
extern char D_008438C0[];
extern char D_008438F0[];
extern char D_00843980[];
extern char D_00843988[];
extern char D_00843998[];
extern char D_008439A8[];
extern char D_008439B0[];
extern char D_008439B8[];
extern char D_008439C0[];
extern char D_008439C8[];
extern char D_008439D8[];
extern char D_008439E0[];
extern char D_008439E8[];
extern char D_008439F0[];
extern char D_008439F8[];
extern char D_00843A00[];
extern char D_00843A08[];
extern char D_00843A18[];
extern char D_00843A20[];
extern char D_00843A28[];
extern char D_00843A40[];
extern char D_00843A60[];
extern char D_00843A78[];
extern char D_00843A88[];
extern char D_00843AA0[];
extern char D_00843AB8[];
extern char D_00843AC8[];
extern char D_00843AE0[];
extern char D_00843AF8[];
extern char D_00843B00[];
extern char D_00843B10[];
extern char D_00843B20[];
extern char D_00843B30[];
extern char D_00843B40[];
extern char D_00843B60[];
extern char D_00843B80[];
extern char D_00843BA0[];
extern char D_00843BC0[];
extern char D_00843BE0[];
extern char D_00843C00[];
extern char D_00843C20[];
extern char D_00843C40[];
extern char D_00843C48[];
extern char D_00843C58[];
extern char D_00843C68[];
extern char D_00843C80[];
extern char D_00843CA0[];
extern char D_00843CC0[];
extern char D_00843CE0[];
extern char D_00843D10[];
extern char D_00843D30[];
extern char D_00843D40[];
extern char D_00843D50[];
extern char D_00843D60[];
extern char D_00843D70[];
extern char D_00843D88[];
extern char D_00843D98[];
extern char D_00843DB0[];
extern char D_00843DD0[];
extern char D_00843DE8[];
extern char D_00843DF0[];
extern char D_00843DF8[];
extern char D_00843E10[];
extern char D_00843E28[];
extern char D_00843E30[];
extern char D_00843E40[];
extern char D_00843E50[];
extern char D_00843E60[];
extern char D_00843E70[];
extern char D_00843E80[];
extern char D_00843E90[];
extern char D_00843EB0[];
extern char D_00843ED0[];
extern char D_00843EE0[];
extern char D_00843EF0[];
extern char D_00843EF8[];
extern char D_00843F08[];
extern char D_00843F18[];
extern char D_00843F28[];
extern char D_00843F38[];
extern char D_00843F48[];
extern char D_00843F58[];
extern char D_00843F68[];
extern char D_00843F70[];
extern char D_00843F88[];
extern char D_00843F90[];
extern char D_00843FA0[];
extern char D_00843FB0[];
extern char D_00843FC0[];
extern char D_00843FD0[];
extern char D_00843FE0[];
extern char D_00844000[];
extern char D_00844020[];
extern char D_00844040[];
extern char D_00844080[];
extern char D_008440C0[];
extern char D_008440F0[];
extern char D_00844120[];
extern char D_00844160[];
extern char D_008441C0[];
extern char D_008441E0[];
extern char D_00844220[];
extern char D_00844260[];
extern char D_008442B0[];
extern char D_008442F0[];
extern char D_00844320[];
extern char D_00844340[];
extern char D_00844380[];
extern char D_008443C0[];
extern char D_008443E0[];
extern char D_00844430[];
extern char D_00844460[];
extern char D_00844490[];
extern char D_008444C0[];
extern char D_008444F0[];
extern char D_00844520[];
extern char D_00844550[];
extern char D_00844570[];
extern char D_008445D0[];
extern char D_00844620[];
extern char D_00844650[];
extern char D_00844670[];
extern char D_00844690[];
extern char D_008446C0[];
extern char D_008446F0[];
extern char D_00844730[];
extern char D_00844760[];
extern char D_00844790[];
extern char D_008447C0[];
extern char D_008447F0[];
extern char D_00844820[];
extern char D_00844860[];
extern char D_008448A0[];
extern char D_00844910[];
extern char D_00844950[];
extern char D_00844980[];
extern char D_008449E0[];
extern char D_00844A10[];
extern char D_00844A40[];
extern char D_00844A70[];
extern char D_00844AA0[];
extern char D_00844AE0[];
extern char D_00844B50[];
extern char D_00844BA0[];
extern char D_00844BD0[];
extern char D_00844BF0[];
extern char D_00844C30[];
extern char D_00844C50[];
extern char D_00844CA0[];
extern char D_00844D00[];
extern char D_00844D60[];
extern char D_00844D90[];
extern char D_00844DD0[];
extern char D_00844E10[];
extern char D_00844E30[];
extern char D_00844E50[];
extern char D_00844E70[];
extern char D_00844EA0[];
extern char D_00844EC0[];
extern char D_00844EE0[];
extern char D_00844F00[];
extern char D_00844F30[];
extern char D_00844F60[];
extern char D_00844F80[];
extern char D_00844FD0[];
extern char D_00845030[];
extern char D_00845090[];
extern char D_008450F0[];
extern char D_00845120[];
extern char D_00845180[];
extern char D_008451E0[];
extern char D_00845240[];
extern char D_00845270[];
extern char D_008452A0[];
extern char D_008452D0[];
extern char D_00845310[];
extern char D_00845370[];
extern char D_008453A0[];
extern char D_008453D0[];
extern char D_00845400[];
extern char D_00845460[];
extern char D_008454B0[];
extern char D_008454E0[];
extern char D_00845510[];
extern char D_00845540[];
extern char D_008455A0[];
extern char D_008456B0[];
extern char D_00845740[];
extern char D_00845770[];
extern char D_008457A0[];
extern char D_008457F0[];
extern char D_00845840[];
extern char D_00845870[];
extern char D_008458B0[];
extern char D_008458E0[];
extern char D_00845910[];
extern char D_00845930[];
extern char D_00845980[];
extern char D_008459B0[];
extern char D_008459E0[];
extern char D_00845A70[];
extern char D_00845B10[];
extern char D_00845B70[];
extern char D_00845C30[];
extern char D_00845CC0[];
extern char D_00845D90[];
extern char D_00845E90[];
extern char D_00845EB0[];
extern char D_00845ED0[];
extern char D_00845F00[];
extern char D_00845F20[];
extern char D_00845F40[];
extern char D_00845F70[];
extern char D_00845FD0[];
extern char D_00846030[];
extern char D_00846090[];
extern char D_008460F0[];
extern char D_00846170[];
extern char D_008461A0[];
extern char D_008461F0[];
extern char D_00846250[];
extern char D_00846280[];
extern char D_008462D0[];
extern char D_00846310[];
extern char D_00846350[];
extern char D_00846370[];
extern char D_008463A0[];
extern char D_00846400[];
extern char D_00846430[];
extern char D_00846480[];
extern char D_008464B0[];
extern char D_008464F0[];
extern char D_008465A0[];
extern char D_00846620[];
extern char D_00846650[];
extern char D_008466A0[];
u32 gShopCondStockNpc07_8[24] = {
#include "lobby/0080B1A0.inc"
};
u32 gShopCondStockNpc07_7[24] = {
#include "lobby/0080B200.inc"
};
u32 gShopCondStockNpc07_6[24] = {
#include "lobby/0080B260.inc"
};
u32 gShopCondStockNpc07_5[24] = {
#include "lobby/0080B2C0.inc"
};
u32 gShopCondStockNpc07_4[28] = {
#include "lobby/0080B320.inc"
};
u32 gShopCondStockNpc07_3[28] = {
#include "lobby/0080B390.inc"
};
u32 gShopCondStockNpc07_2[28] = {
#include "lobby/0080B400.inc"
};
u32 gShopCondStockNpc07_1[32] = {
#include "lobby/0080B470.inc"
};
u32 gShopCondStockNpc07_0[32] = {
#include "lobby/0080B4F0.inc"
};
CondItems gShopCondStockNpc07[10] = {
#include "lobby/0080B570.inc"
};
u32 gShopCondStockNpc08_5[8] = {
#include "lobby/0080B5C0.inc"
};
u32 gShopCondStockNpc08_4[12] = {
#include "lobby/0080B5E0.inc"
};
u32 gShopCondStockNpc08_3[12] = {
#include "lobby/0080B610.inc"
};
u32 gShopCondStockNpc08_2[12] = {
#include "lobby/0080B640.inc"
};
u32 gShopCondStockNpc08_1[12] = {
#include "lobby/0080B670.inc"
};
u32 gShopCondStockNpc08_0[12] = {
#include "lobby/0080B6A0.inc"
};
CondItems gShopCondStockNpc08[6] = {
#include "lobby/0080B6D0.inc"
};
u16 gShopStockNpc0A_0[24] = {
#include "lobby/0080B700.inc"
};
u16 gShopStockNpc0A_1[28] = {
#include "lobby/0080B730.inc"
};
u16 *gShopStockNpc0A[2] = {
    gShopStockNpc0A_0, gShopStockNpc0A_1,
};
u16 gShopStockNpc0B[16] = {
#include "lobby/0080B770.inc"
};
u16 gShopStockNpc0BLv2[24] = {
#include "lobby/0080B790.inc"
};
u16 gShopStockNpc0BTimer[24] = {
#include "lobby/0080B7C0.inc"
};
u16 gShopStockNpc0C[32] = {
#include "lobby/0080B7F0.inc"
};
u16 gShopStockNpc0CLv2[32] = {
#include "lobby/0080B830.inc"
};
u16 gShopStockNpc12Psp_0[24] = {
#include "lobby/0080B870.inc"
};
u16 gShopStockNpc12Psp_1[32] = {
#include "lobby/0080B8A0.inc"
};
u16 gShopStockNpc12Psp_2[32] = {
#include "lobby/0080B8E0.inc"
};
u16 gShopStockNpc12Psp_3[32] = {
#include "lobby/0080B920.inc"
};
u16 gShopStockNpc12Timer_0[32] = {
#include "lobby/0080B960.inc"
};
u16 gShopStockNpc12Timer_1[40] = {
#include "lobby/0080B9A0.inc"
};
u16 *gShopStockNpc12Psp[4] = {
    gShopStockNpc12Psp_0, gShopStockNpc12Psp_1, gShopStockNpc12Psp_2, gShopStockNpc12Psp_3,
};
u16 *gShopStockNpc12Timer[2] = {
    gShopStockNpc12Timer_0, gShopStockNpc12Timer_1,
};
u32 gShopCondStockNpc28_2[2] = {
#include "lobby/0080BA08.inc"
};
u32 gShopCondStockNpc28_1[4] = {
#include "lobby/0080BA10.inc"
};
u32 gShopCondStockNpc28_0[4] = {
#include "lobby/0080BA20.inc"
};
CondItems gShopCondStockNpc28[4] = {
#include "lobby/0080BA30.inc"
};
u16 gShopStockNpc2C_0[24] = {
#include "lobby/0080BA50.inc"
};
u16 gShopStockNpc2C_1[32] = {
#include "lobby/0080BA80.inc"
};
u16 *gShopStockNpc2C[4] = {
    gShopStockNpc2C_0, gShopStockNpc2C_1, NULL, NULL,
};
u16 gShopStockNpc2D_0[24] = {
#include "lobby/0080BAD0.inc"
};
u16 gShopStockNpc2D_1[24] = {
#include "lobby/0080BB00.inc"
};
u16 *gShopStockNpc2D[4] = {
    gShopStockNpc2D_0, gShopStockNpc2D_1, NULL, NULL,
};
u16 gShopStockNpc2E_0[24] = {
#include "lobby/0080BB40.inc"
};
u16 gShopStockNpc2E_1[28] = {
#include "lobby/0080BB70.inc"
};
u16 *gShopStockNpc2E[2] = {
    gShopStockNpc2E_0, gShopStockNpc2E_1,
};
u32 gShopCondStockNpc39_3[24] = {
#include "lobby/0080BBB0.inc"
};
u32 gShopCondStockNpc39_2[28] = {
#include "lobby/0080BC10.inc"
};
u32 gShopCondStockNpc39_1[28] = {
#include "lobby/0080BC80.inc"
};
u32 gShopCondStockNpc39_0[32] = {
#include "lobby/0080BCF0.inc"
};
CondItems gShopCondStockNpc39[4] = {
#include "lobby/0080BD70.inc"
};
u32 gShopCondStockNpc42_0[14] = {
#include "lobby/0080BD90.inc"
};
CondItems gShopCondStockNpc42[1] = {
    {0, (void *)gShopCondStockNpc42_0},
};
u16 gShopStockNpc50[72] = {
#include "lobby/0080BDD0.inc"
};
u16 gShopStockNpc50Ev122[72] = {
#include "lobby/0080BE60.inc"
};
u32 gShopCondStockNpc5D_0[20] = {
#include "lobby/0080BEF0.inc"
};
CondItems gShopCondStockNpc5D[2] = {
    {0, (void *)gShopCondStockNpc5D_0},
    {0, NULL},
};
u32 gShopCondStockNpc6A_1[28] = {
#include "lobby/0080BF50.inc"
};
u32 gShopCondStockNpc6A_0[28] = {
#include "lobby/0080BFC0.inc"
};
CondItems gShopCondStockNpc6A[2] = {
#include "lobby/0080C030.inc"
};
char *D_0080C040[2] = {
    D_0083BA90, D_0083BA98,
};
char *gPriceMarks[2] = {
    D_0083BAA0, D_0083BAA8,
};
char *gCrossMarkTexts[2] = {
    D_0083BAB0, D_0083BAB8,
};
char *gWideSpaceTexts[2] = {
    D_0083BAC0, D_0083BAC8,
};
char *gPointMarkTexts[8] = {
    D_0083BAD0, D_0083BAE0, D_0083BAF8, D_0083BB00,
    D_0083BB08, D_0083BB10, D_0083BB18, D_0083BB28,
};
char *gRarePrefixTexts[4] = {
    D_0083BB30, D_0083BB38, D_0083BB40, D_0083BB48,
};
char *gNoneSetTexts[2] = {
    D_0083BB50, D_0083BB58,
};
char *gHeadcountTexts[2] = {
    D_0083BB60, D_0083BB68,
};
char *gButtonGuideTexts[30] = {
    D_0083BB70, D_0083BB78, D_0083BB88, D_0083BB98,
    D_0083BBA8, D_0083BBC0, D_0083BBD8, D_0083BBE0,
    D_0083BBF0, D_0083BC00, D_0083BC10, D_0083BC18,
    D_0083BC28, D_0083BC38, D_0083BC48, D_0083BC50,
    D_0083BC60, D_0083BC70, D_0083BC80, D_0083BC90,
    D_0083BCA0, D_0083BCB0, D_0083BCC8, D_0083BCD8,
    D_0083BCE8, D_0083BD00, D_0083BD18, D_0083BD28,
    D_0083BD40, D_0083BD58,
};
char *gUnknownNameTexts[2] = {
    D_0083BD68, D_0083BD78,
};
char *gWantedTexts[4] = {
    D_0083BD90, D_0083BDB0, D_0083BDD0, D_0083BDF0,
};
char *gQuestHallTexts[8] = {
    D_0083BE08, D_0083BE18, D_0083BE28, D_0083BB50,
    D_0083BE38, D_0083BE48, D_0083BE58, D_0083BE68,
};
char *gClearMarkTexts[4] = {
    D_0083BE70, D_0083BE78, D_0083BE90, D_0083BEA0,
};
char *gTargetSelectTexts[2] = {
    D_0083BEB0, D_0083BEC0,
};
char *gLandNameTexts[2] = {
    D_0083BED0, D_0083BEE0,
};
char *gAreaNameLabelTexts[2] = {
    D_0083BEF0, D_0083BF00,
};
char *gIngredientSelectTexts[2] = {
    D_0083BF10, D_0083BF20,
};
char *gFishingBaitTexts[2] = {
    D_0083BF30, D_0083BF40,
};
char *gPickaxeSelectTexts[2] = {
    D_0083BF50, D_0083BF60,
};
char *gQuestHallNameTexts[6] = {
    D_0083BF70, D_0083BF80, D_0083BF90, D_0083BFA0,
    D_0083BFB0, D_0083BFD0,
};
char *gArenaTournamentTexts[2] = {
    D_0083BFE8, D_0083BFF8,
};
char *gQuestTakerTexts[2] = {
    D_0083C008, D_0083C018,
};
char *gWantedFeeTexts[2] = {
    D_0083C030, D_0083C050,
};
char *gZennyReceivedTexts[2] = {
    D_0083C060, D_0083C078,
};
char *gCloseLabelTexts[2] = {
    D_0083C080, D_0083C088,
};
char *gGoneOutTexts[2] = {
    D_0083C098, D_0083C0A0,
};
char *gMyHouseLabelTexts[2] = {
    D_0083C0B8, D_0083C0D0,
};
char *gNeededSlotsTexts[2] = {
    D_0083C0E8, D_0083C100,
};
char *gPickaxeGiveTexts[2] = {
    D_0083C120, D_0083C150,
};
char *gIngredientListTexts[6] = {
    D_0083C170, D_0083C180, D_0083C1A0, D_0083BBD8,
    D_0083C1A8, D_0083C1B8,
};
char *gHostChoosingFoodTexts[2] = {
    D_0083C1D0, D_0083C1F0,
};
char *gHunterDojoTexts[2] = {
    D_0083C208, D_0083C218,
};
char *gLimitOnTexts[2] = {
    D_0083C228, D_0083C240,
};
char *gAreaNameTexts[16] = {
    D_0083C258, D_0083C260, D_0083C270, D_0083C280,
    D_0083C290, D_0083C2A0, D_0083C2A8, D_0083C2B8,
    D_0083C2B8, D_0083C2B8, D_0083C2C0, D_0083C2D0,
    D_0083C2D0, D_0083C2D0, D_0083C2D0, D_0083C2E0,
};
char *gBoardShipTexts[2] = {
    D_0083C2F0, D_0083C300,
};
char *gTownBoardTexts[3] = {
    D_0083C310, D_0083C320, D_0083C328,
};
char *gTownBoardHelpTexts[4] = {
    D_0083C340, D_0083C370, D_0083C3A0, D_0083C3C8,
};
char *gUpDownArrowTexts[4] = {
    D_0083C3D0, NULL, NULL, NULL,
};
char *gJumboBoardTitleTexts[2] = {
    D_0083C3E0, D_0083C400,
};
char *gBoardPromptTexts[2] = {
    D_0083C440, D_0083C470,
};
char *gJumboBoardTexts[12] = {
    D_0083C498, D_0083C4B0, D_0083C4C8, D_0083C4D8,
    D_0083C4E8, D_0083C500, D_0083C520, D_0083C540,
    D_0083C560, D_0083C580, NULL, NULL,
};
char *gElderDragonInfoTexts[4] = {
    D_0083C598, D_0083C5A8, D_0083BC60, D_0083C598,
};
char *gTimeOfDayTexts[2] = {
    D_0083C5B8, D_0083C5C8,
};
char *gNightTexts[2] = {
    D_0083C5D0, D_0083C5D8,
};
char *gHairChangeTexts[2] = {
    D_0083C5E0, D_0083C5F0,
};
char *gHairColorTexts[2] = {
    D_0083C610, D_0083C630,
};
char *gHairShowHintTexts[4] = {
    D_0083C660, NULL, NULL, NULL,
};
char *gProfileMenuTexts[2] = {
    D_0083C690, D_0083C6A8,
};
char *gHuntRecordTexts[2] = {
    D_0083C6B8, D_0083C6C8,
};
char *gWeaponCollectTexts[2] = {
    D_0083C6D8, D_0083C6E8,
};
char *gTitleLabelTexts[2] = {
    D_0083C6F8, D_0083C700,
};
char *gAncientScoreTexts[4] = {
    D_0083C710, NULL, NULL, NULL,
};
char *gCheckItemPromptTexts[2] = {
    D_0083C730, D_0083C760,
};
char *gTitleGenrePromptTexts[2] = {
    D_0083C790, D_0083C7C0,
};
char *gTitleConfirmTexts[2] = {
    D_0083C810, D_0083C830,
};
char *gArmorGenrePromptTexts[2] = {
    D_0083C860, D_0083C890,
};
char *gNoArmorFoundTexts[4] = {
    D_0083C8B0, NULL, NULL, NULL,
};
char *gHouseAccessTexts[4] = {
    D_0083C8D0, D_0083C8E0, D_0083C8F0, D_0083C908,
};
char *gHunterNotesTexts[18] = {
    D_0083C918, D_0083C928, D_0083C930, D_0083C940,
    D_0083C950, D_0083C960, D_0083C980, D_0083C9A0,
    gHunterNotesTexts_8, D_0083C9D0, D_0083C9E8, D_0083C9F0,
    D_0083C9F8, D_0083CA08, D_0083CA10, D_0083CA28,
    D_0083CA38, D_0083C960,
};
char *gUnknownItemTexts[2] = {
    D_0083C980, D_0083CA40,
};
char *gHuntCountTexts[2] = {
    D_0083CA60, D_0083CA70,
};
char *gMinSizeTexts[2] = {
    D_0083CA90, D_0083CAB0,
};
char *gSongSelectTexts[2] = {
    D_0083CAC8, D_0083CAD8,
};
char *gShortDashTexts[2] = {
    D_0083CAE8, NULL,
};
char *gDeliveryTexts[10] = {
    D_0083CAF0, D_0083CB00, D_0083CB10, D_0083CB20,
    D_0083CB30, D_0083CB38, D_0083CB50, D_0083CB80,
    D_0083CBD0, D_0083CC00,
};
char *gQuestPeriodTexts[2] = {
    D_0083CC20, D_0083CC30,
};
char *gQuestOrderPromptTexts[2] = {
    D_0083CC40, D_0083CC70,
};
char *gQuestBusyTexts[2] = {
    D_0083CCC0, D_0083CD10,
};
char *gQuestOrderedTexts[4] = {
    D_0083CD40, NULL, NULL, NULL,
};
char *gDojoMenuTexts[4] = {
    D_0083CD90, D_0083CDB0, D_0083C300, D_0083CDC8,
};
char *gDojoExchangeTexts[6] = {
    D_0083CDD8, D_0083CDE8, D_0083CDF8, D_0083CE08,
    D_0083CE18, D_0083CE30,
};
char *gExchangeCountTexts[2] = {
    D_0083CE60, D_0083CE80,
};
char *gDojoBasicsTexts[4] = {
    D_0083CEB0, D_0083CEC8, D_0083CED8, D_0083CEF0,
};
char *gGroupBoardTexts[2] = {
    D_0083CF08, D_0083CF18,
};
char *gRecruitMsgTexts[2] = {
    D_0083CF20, D_0083BB60,
};
char *gInTownTexts[2] = {
    D_0083CF28, D_0083CF30,
};
char *gHrLabelTexts[2] = {
    D_0083CF38, D_0083CF40,
};
char *gConnStateTexts[2] = {
    D_0083CF48, D_0083CF58,
};
char *gLimitedTexts[2] = {
    D_0083CF60, D_0083CF68,
};
char *gLongDashTexts[2] = {
    D_0083CF80, D_0083CFC0,
};
char *gRecruitMsgColonTexts[2] = {
    D_0083CFD0, D_0083CFE0,
};
char *gCancelLabelTexts[4] = {
    D_0083CB38, D_0083CFE8, D_0083D000, D_0083D018,
};
char *gGroupBoardTitleTexts[2] = {
    D_0083CF08, D_0083D028,
};
char *gLimitCondTexts[2] = {
    D_0083D030, D_0083D040,
};
char *gRecruitMsgLabelTexts[2] = {
    D_0083CFD0, D_0083D050,
};
char *gSearchStartTexts[2] = {
    D_0083D060, D_0083D070,
};
char *gLocationLabelTexts[2] = {
    D_0083D088, D_0083D0A0,
};
char *gCheckingStatusTexts[2] = {
    D_0083D0B8, D_0083D0C8,
};
char *gOnQuestStatusTexts[4] = {
    D_0083D0D8, D_0083D0E8, D_0083BC80, D_0083D0F8,
};
char *gOnlineMarkTexts[2] = {
    D_0083D100, D_0083D118,
};
char *gTitleColonTexts[2] = {
    D_0083D128, D_0083D130,
};
char *gSymbolColorTexts[2] = {
    D_0083D140, D_0083D158,
};
char *gWinsMarkTexts[2] = {
    D_0083D168, D_0083D170,
};
char *gMailComposeTexts[6] = {
    D_0083D180, D_0083D188, D_0083D190, D_0083BC18,
    D_0083D198, D_0083D1B0,
};
char *gGroupJoinPromptTexts[2] = {
    D_0083D220, D_0083D280,
};
char *gGroupSearchHelpTexts[2] = {
    D_0083D2B0, NULL,
};
char *gMemberActTexts[3] = {
    D_0083D310, D_0083D318, D_0083D328,
};
char *gMailActTexts[3] = {
    D_0083D338, D_0083D318, D_0083D340,
};
char *gGroupLimitTexts[8] = {
    D_0083BB50, D_0083D350, D_0083D360, D_0083BB50,
    D_0083D370, D_0083D378, NULL, NULL,
};
char *gGroupLimitNewTexts[4] = {
    D_0083D380, D_0083BB50, D_0083D350, D_0083D388,
};
char *gGroupLimitExpTexts[4] = {
    D_0083D380, D_0083BB50, D_0083D360, D_0083D398,
};
char *gArenaEntryTexts[8] = {
    D_0083D3A8, D_0083D3B8, D_0083D3C8, D_0083D3D8,
    D_0083D170, D_0083D3E8, D_0083D3F8, D_0083D400,
};
char *gMonsterStatusTexts[2] = {
    D_0083D420, D_0083D440,
};
char *gMonsterRecordTexts[2] = {
    D_0083D458, D_0083D468,
};
char *gWinCountTexts[2] = {
    D_0083D478, D_0083D488,
};
char *gBattleTimeTexts[2] = {
    D_0083D498, D_0083D4A8,
};
char *gMonsterListTexts[10] = {
    D_0083D4B8, D_0083CF30, D_0083D4C0, D_0083D380,
    D_0083D4D0, D_0083D500, D_0083D520, D_0083D540,
    D_0083D550, D_0083D560,
};
char *gMonsterActionTexts[2] = {
    D_0083D570, D_0083D590,
};
char *D_0080C640[10] = {
    D_0083CF30, D_0083D4C0, D_0083D5A0, D_0083D5A8,
    D_0083D4B8, D_0083D5B0, D_0083D5B8, D_0083D5C8,
    D_0083D5D8, D_0083D5E8,
};
char *gTimeLeftSecTexts[2] = {
    D_0083D600, D_0083D620,
};
char *gMonsterFeedTexts[14] = {
    D_0083D680, D_0083D700, NULL, NULL,
    D_0083D718, D_0083D720, D_0083D728, D_0083D730,
    D_0083D738, D_0083D740, D_0083D748, D_0083D750,
    D_0083D758, D_0083D760,
};
char *gStarMarkTexts[2] = {
    D_0083D768, D_0083D770,
};
char *gMemberCountTexts[6] = {
    D_0083D778, D_0083D780, D_0083D788, D_0083D790,
    D_0083D7A0, D_0083D7B8,
};
char *gNoCombineTexts[2] = {
    D_0083D7C0, D_0083D7E0,
};
char *gCharaNameLabelTexts[2] = {
    D_0083D800, D_0083D820,
};
char *gWeaponArmorTexts[2] = {
    D_0083CF58, D_0083D838,
};
char *gEquipGroupTexts[12] = {
    D_0083D840, D_0083D850, D_0083D860, D_0083D880,
    D_0083D898, D_0083D8A8, D_0083D8B8, D_0083D8C8,
    D_0083D8D8, D_0083D8E8, NULL, NULL,
};
char *gEquipLimitTexts[8] = {
    D_0083D8F8, D_0083D908, D_0083D918, D_0083D928,
    D_0083D940, D_0083D960, D_0083D978, D_0083D988,
};
char *gEquipNowTexts[4] = {
    D_0083D9A0, D_0083D9D0, NULL, NULL,
};
char *gEquipBuyPromptTexts[12] = {
    D_0083DA00, D_0083DA50, D_0083DAA0, D_0083DAD0,
    D_0083DAF0, D_0083DB20, D_0083DB70, D_0083DBB0,
    D_0083DBE0, D_0083DC30, NULL, NULL,
};
char *gSmithyMenuTexts[4] = {
    D_0083DC78, D_0083DC88, D_0083DC98, D_0083C300,
};
char *gSmithyMakeTexts[4] = {
    D_0083DCA0, D_0083DCB0, D_0083C300, D_0083DCC0,
};
char *gNoArmorToMakeTexts[4] = {
    D_0083DCE0, NULL, NULL, NULL,
};
char *gWeaponForgeTexts[2] = {
    D_0083DCA0, D_0083DD00,
};
char *gArmorMakeTexts[2] = {
    D_0083DCB0, D_0083DD10,
};
char *gDecoMenuTexts[8] = {
    D_0083DD20, D_0083DD30, D_0083DD40, D_0083DD50,
    D_0083DD60, D_0083DD70, D_0083DD80, D_0083DD90,
};
char *gWeaponClassTexts[8] = {
    D_0083D840, D_0083D850, D_0083D860, D_0083D880,
    D_0083D898, NULL, NULL, NULL,
};
char *gDecoMakeTexts[4] = {
    D_0083DDA0, D_0083DDC0, D_0083C630, D_0083DDD8,
};
char *gCannotUpgradeTexts[2] = {
    D_0083DDD8, D_0083DDF0,
};
char *gUpgradeKindTexts[2] = {
    D_0083DE30, D_0083DE70,
};
char *gNeedMaterialsTexts[2] = {
    D_0083DEB0, D_0083DAD0,
};
char *gWeaponChangeWarnTexts[2] = {
    D_0083DEE0, D_0083DF30,
};
char *D_0080C820[2] = {
    D_0083DF90, D_0083DFB0,
};
char *gMaterialsHeaderTexts[2] = {
    D_0083DFD0, D_0083DFE8,
};
char *gSmithyLabelTexts[26] = {
    D_0083DFF8, D_0083E008, D_0083E020, D_0083E038,
    D_0083E050, D_0083E070, D_0083E090, D_0083E0B0,
    D_0083E0D0, D_0083E0F0, D_0083E108, D_0083E118,
    D_0083E130, D_0083E150, D_0083E170, D_0083E1C0,
    D_0083E260, D_0083E2C0, D_0083E350, D_0083E3B0,
    D_0083E420, D_0083E490, D_0083E530, D_0083E590,
    D_0083E610, D_0083E670,
};
char *gModFeeSilencerTexts[2] = {
    D_0083E6A0, D_0083E6F0,
};
char *gModFeeShieldTexts[2] = {
    D_0083E740, D_0083E790,
};
char *gWeaponMakePromptTexts[2] = {
    D_0083E7E0, D_0083E830,
};
char *gDecoAttachPromptTexts[2] = {
    D_0083E880, D_0083E8D0,
};
char *gNoFreeSlotTexts[2] = {
    D_0083E930, D_0083E9A0,
};
char *gSlotShortTexts[2] = {
    D_0083E9D0, D_0083EA00,
};
char *gDecoChoosePromptTexts[2] = {
    D_0083EA30, D_0083EA50,
};
char *gNoDecoTexts[2] = {
    D_0083EA90, D_0083EAB0,
};
char *gNoDecoAttachedTexts[2] = {
    D_0083EB00, D_0083EB70,
};
char *gDecoRemoveWarnTexts[2] = {
    D_0083EBC0, D_0083EC00,
};
char *gDecoMakePromptTexts[2] = {
    D_0083EC30, D_0083DDC0,
};
char *gNoDecoToMakeTexts[4] = {
    D_0083EC60, D_0083EC80, NULL, NULL,
};
char *gIngredientABTexts[2] = {
    D_0083EC98, D_0083ECA8,
};
char *gMealHealthTexts[2] = {
    D_0083ECC0, D_0083ECD8,
};
char *gMealStaminaTexts[2] = {
    D_0083ECF0, D_0083ED10,
};
char *gMealAttackTexts[2] = {
    D_0083ED30, D_0083ED50,
};
char *gMealDefenseTexts[4] = {
    D_0083ED70, D_0083ED90, D_0083EDA8, D_0083EDB0,
};
char *gMealPromptTexts[2] = {
    D_0083EDC0, D_0083EDE0,
};
char *gMealPartyPromptTexts[2] = {
    D_0083EE00, D_0083EE38,
};
char *gMealCourseTexts[4] = {
    D_0083EE50, D_0083EE68, NULL, NULL,
};
char *gMealNameTexts[16] = {
    D_0083EE80, D_0083EE98, D_0083EEA8, D_0083EEC0,
    D_0083EED8, D_0083EEE8, D_0083EEF8, D_0083EF08,
    D_0083EF18, D_0083EF28, D_0083EF38, D_0083EF48,
    D_0083EF58, D_0083EF68, D_0083EF78, D_0083EF90,
};
char *gMealTasteTexts_0[4] = {
    D_0083EFB0, D_0083EFD0, D_0083EFF0, D_0083F010,
};
char *gMealTasteTexts_1[4] = {
    D_0083F030, D_0083F050, D_0083F070, D_0083F090,
};
char *gMealTasteTexts_2[4] = {
    D_0083F0B0, D_0083F0D0, D_0083F0F0, D_0083F110,
};
char *gMealTasteTexts_3[4] = {
    D_0083F130, D_0083F130, D_0083F130, D_0083F130,
};
char *gMealTasteTexts_4[4] = {
    D_0083F140, D_0083F150, D_0083F170, D_0083F190,
};
char *gMealTasteTexts_5[4] = {
    D_0083F1B0, D_0083F1D0, D_0083F1F0, D_0083F210,
};
char *gMealTasteTexts_6[4] = {
    D_0083F230, D_0083F250, D_0083F270, D_0083F290,
};
char **gMealTasteTexts[8] = {
    gMealTasteTexts_0, gMealTasteTexts_1, gMealTasteTexts_2, gMealTasteTexts_3,
    gMealTasteTexts_4, gMealTasteTexts_5, gMealTasteTexts_6, NULL,
};
char *D_0080CA20[6] = {
    D_0083F2B0, D_0083F2D0, D_0083F2E8, D_0083F300,
    D_0083F318, NULL,
};
char *gCameraGuideTexts[2] = {
    D_0083F328, D_0083F338,
};
char *gMenuShowTexts[2] = {
    D_0083F348, NULL,
};
char *D_0080CA48[2] = {
    D_0083F358, D_0083F368,
};
char *D_0080CA50[8] = {
    D_0083F370, D_0083F380, D_0083F3A0, D_0083F3C0,
    D_0083F3D8, NULL, NULL, NULL,
};
char *gEquipSlotTexts[8] = {
    D_0083CF58, D_0083F3E8, D_0083F3F0, D_0083F3F8,
    D_0083F400, D_0083F408, NULL, NULL,
};
char *gPriceNoteTexts[6] = {
    D_0083F410, D_0083F430, D_0083F450, D_0083F470,
    D_0083F480, D_0083F4A0,
};
char *gBackGuideTexts[2] = {
    D_0083F4B0, D_0083F4C0,
};
char *gDiamondMarkTexts[2] = {
    D_0083F4E0, D_0083F4F0,
};
char *gColorRowFmtTexts[2] = {
    D_0083F510, D_0083F530,
};
char *gRegisterLabelTexts[2] = {
    D_0083F540, D_0083F550,
};
char *gStatusHeadTexts[2] = {
    D_0083F560, D_0083F570,
};
char *gStatusHeadAltTexts[80] = {
    D_0083F590, D_0083F5A8, D_0083F5C0, D_0083F5E0,
    D_0083F600, D_0083F618, NULL, NULL,
    D_0083F620, D_0083F650, D_0083F670, D_0083F6A0,
    D_0083F6D0, D_0083F700, D_0083F720, D_0083F750,
    D_0083F770, gHunterNotesTexts_8, D_0083F7A0, D_0083F7C0,
    D_0083F810, D_0083F830, D_0083F860, D_0083F880,
    D_0083F8A0, D_0083F8B8, D_0083F8D0, D_0083F8F0,
    D_0083F910, D_0083F940, D_0083F970, D_0083F9A0,
    D_0083F9C0, D_0083F9E0, D_0083FA00, D_0083FA30,
    D_0083F880, D_0083FA78, D_0083FA90, D_0083FAB0,
    D_0083FAE0, D_0083FB10, D_0083FB40, D_0083FB70,
    D_0083FB90, D_0083FBB0, D_0083FBE0, D_0083FC10,
    D_0083FC30, D_0083FC70, D_0083FCA0, D_0083FCB0,
    D_0083FCD0, D_0083FD20, D_0083FD40, D_0083FD60,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_0083FD80, D_0083FDB0,
    D_0083FDD0, D_0083FE10, D_0083FE50, D_0083FE80,
    D_0083FEB0, gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_0083FED0, D_0083FF10, D_0083FF50, D_0083FF90,
    D_0083FFC0, D_00840000, D_00840040, D_00840080,
    D_008400B0, D_008400E0, D_00840120, D_00840160,
};
char *gAreaChatTexts[4] = {
    D_00840190, D_008401A8, D_008401B0, D_008401B8,
};
char *gArenaQuestTexts[2] = {
    D_0083BFE8, D_0083C050,
};
char *gUnknownQuestTexts[2] = {
    D_008401C0, D_008401D0,
};
char *D_0080CC30[44] = {
    D_008401D8, D_008401E0, D_008401F8, D_00840208,
    D_00840220, D_00840240, D_00840258, D_00840268,
    D_0083D7A0, D_00840270, D_00840280, D_00840288,
    D_00840298, D_0083D3D8, D_008402A8, D_008402B8,
    D_008402C8, D_008402D8, D_008402E0, D_008402F8,
    D_00840308, D_00840318, D_00840328, D_00840338,
    D_00840348, D_00840360, D_00840378, D_00840390,
    D_008403B0, D_008403D0, D_008403F0, D_00840410,
    D_00840430, D_00840450, D_00840470, D_00840490,
    D_008404B0, D_008404D0, D_008404F0, D_00840510,
    D_00840528, D_00840538, D_00840550, D_00840568,
};
char *gArenaTeamTexts[8] = {
    D_00840578, D_00840588, D_008405A0, D_008405D0,
    D_008405F0, D_00840620, NULL, NULL,
};
char *gInfoIndexTexts[8] = {
    D_00840638, D_0083C328, D_00840648, D_00840658,
    D_00840668, NULL, NULL, NULL,
};
char *gQuestKindTexts[2] = {
    D_0083BFA0, D_00840678,
};
char *gHuntInfoHeadTexts[2] = {
    D_00840688, D_008406A0,
};
char *gMainTargetTexts[2] = {
    D_008406C0, D_008406D8,
};
char *gMainPointsTexts[2] = {
    D_008406E8, D_008406F8,
};
char *gSubAPointsTexts[2] = {
    D_00840708, D_00840718,
};
char *gSubBPointsTexts[2] = {
    D_00840728, D_00840740,
};
char *gSubTargetBTexts[2] = {
    D_00840760, D_008402B8,
};
char *gContractFeeTexts[2] = {
    D_00840778, D_0083D3D8,
};
char *gDestinationTexts[2] = {
    D_00840788, D_00840798,
};
char *gClearCondTexts[2] = {
    D_00840328, D_00840338,
};
char *gClientLabelTexts[2] = {
    D_0083CB20, D_008407A8,
};
char *gTimeAttackTexts[2] = {
    D_008407C0, D_008407D8,
};
char *gHighScoreTexts[2] = {
    D_008407E8, D_008407F8,
};
char *gNoValueDashTexts[2] = {
    D_0083C5B8, D_00840808,
};
char *gRewardLabelTexts[2] = {
    D_00840818, D_008402E0,
};
char *gPlayerCountTexts[2] = {
    D_00840828, D_00840838,
};
char *gCondDecideTexts[2] = {
    D_008402F8, D_00840638,
};
char *gMonsterEcologyTexts[2] = {
    D_00840850, D_00840870,
};
char *gOfficialEventTexts[4] = {
    D_00840888, D_00840898, D_008408A8, D_008408B8,
};
char *gGameMenuTexts1[12] = {
    D_0083F358, D_008408C8, D_0083BC80, D_008408D0,
    D_008408E0, D_0083BC50, D_0083BC10, D_008408F0,
    D_00840900, NULL, NULL, NULL,
};
char *gGameMenuTexts2[12] = {
    D_0083F358, D_008408C8, D_0083BC80, D_00840910,
    D_008408D0, D_008408E0, D_0083BC50, D_0083BC10,
    D_008408F0, D_00840900, NULL, NULL,
};
char *gGameMenuTexts2Dim[12] = {
    D_0083F358, D_00840920, D_0083BC80, D_00840910,
    D_008408D0, D_008408E0, D_0083BC50, D_0083BC10,
    D_008408F0, D_00840930, NULL, NULL,
};
char *gGameMenuTexts0[12] = {
    D_0083F358, D_008408C8, D_0083BC80, D_00840910,
    D_008408D0, D_0083BCC8, D_008408F0, D_00840940,
    D_00840900, NULL, NULL, NULL,
};
char *gGameMenuTexts0Dim[52] = {
    D_0083F358, D_00840920, D_0083BC80, D_00840910,
    D_008408D0, D_0083BCC8, D_008408F0, D_00840940,
    D_00840930, NULL, NULL, NULL,
    D_00840950, D_00840990, D_008409B0, D_008409E0,
    D_00840A10, D_00840A30, D_00840A70, D_00840AB0,
    D_00840AD0, D_00840AF0, D_00840B10, D_00840B30,
    gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_00840B50, D_00840B70, D_00840B90, D_00840BB0,
    D_00840BF0, D_00840C40, D_00840C80, D_00840CC0,
    D_00840CF0, D_00840D20, gHunterNotesTexts_8, D_00840B30,
    D_00840D50, D_00840DB0, D_00840E10, D_00840E40,
    D_00840E70, D_00840EA0, D_00840EC0, D_00840F00,
    D_00840F60, D_00840F90, D_00840FD0, D_00841010,
};
char *gSearchPanel_7[2] = {
    D_0083BC18, D_00841048,
};
char *gNameSearchTexts[2] = {
    D_00841058, D_00841068,
};
char *gMailboxMenuTexts[4] = {
    D_00841078, D_00841088, D_00841098, D_0083C1B8,
};
char *gQuitMenuTexts[2] = {
    D_008410B0, D_008410C8,
};
char *gQuitMenuDimTexts[2] = {
    D_008410B0, D_008410E0,
};
char *gQuitSaveTexts[4] = {
    D_00841100, D_00841130, NULL, NULL,
};
char *gQuitConfirmTexts[4] = {
    D_00841160, D_008411B0, D_00841240, D_00841290,
};
char *gSpeakerListTexts[2] = {
    D_008412F8, D_00841310,
};
char *gSpeakerHeadTexts[2] = {
    D_00841330, D_00841340,
};
char *gNoSpeakerTexts[2] = {
    D_00841350, D_00841370,
};
char *gMutedListTexts[2] = {
    D_00841390, D_008413B0,
};
char *gMutedLabelTexts[4] = {
    D_008413D0, D_008413F0, NULL, NULL,
};
char *gChatMenuTexts[6] = {
    D_0083BCD8, D_0083BCC8, D_00841410, D_00841420,
    D_00841430, D_00841450,
};
char *gChatStateTexts[2] = {
    D_00841470, D_00841490,
};
char *gChatUserActTexts[4][3] = {
    {D_0083BC80, D_008414A8, D_008414B8},
    {D_0083BC80, D_008414C8, D_008414D8},
    {D_0083BC80, D_008414F0, D_008414D8},
    {D_008414D8, NULL, NULL},
};
/* The member card's restriction texts, by the restricted flag. */
char *gRestrictTexts[2] = {
    D_0083C218, D_0083C228,
};
char *gTownInfoTexts[6] = {
    D_00841508, D_00841510, D_0083D088, D_0083CFD0,
    D_0083D030, D_0083D040,
};
char *gAreaPopTexts[2] = {
    D_00841520, D_00841530,
};
char *gTavernPortTexts[2] = {
    D_00841540, D_00841548,
};
char *gSmithyMineTexts[2] = {
    D_00841550, D_00841558,
};
char *gTavernLevelTexts[4] = {
    D_00841560, D_00841580, D_008415A0, NULL,
};
char *gPortLevelTexts[4] = {
    D_008415C0, D_008415E0, D_00841600, NULL,
};
char *gSmithyLevelTexts[4] = {
    D_00841620, D_00841640, D_00841660, NULL,
};
char *gMineLevelTexts[4] = {
    D_00841680, D_008416A0, D_008416C0, NULL,
};
char *gHunterStateTexts[12] = {
    D_008416D8, D_00841750, D_00841770, D_00841790,
    D_008417B0, D_00841710, D_00841728, D_008416F0,
    D_00841738, NULL, NULL, NULL,
};
/* The member status texts, by status (MemberCard_DrawStatus). */
char *gMemberStatusTexts[12] = {
    D_008417C8, D_008417E0, D_00841800, D_00841820,
    D_00841838, D_00841850, D_00841860, D_00841870,
    D_00841888, NULL, NULL, NULL,
};
/* A two-entry label list from its second entry on (D_0080D104, an alias)
 * runs into gSellLabelTexts. */
char *D_0080D100[2] = {
    D_00841890, D_008418A0,
};
char *gSellLabelTexts[2] = {
    D_008418A8, D_008418B0,
};
char *gShopBuySellTexts[4] = {
    D_008418A0, D_008418A8, D_0083C300, NULL,
};
char *gItemShopMenuTexts[4] = {
    D_00841890, D_008418A0, D_008418A8, D_0083C300,
};
char *gRecipeTexts[6] = {
    D_008418B8, D_008418C8, D_008418D8, D_008418E8,
    D_008418F8, D_00841908,
};
char *gUnknownLabelTexts[2] = {
    D_00841918, D_00841920,
};
char *gCombineFeeTexts[4] = {
    D_00841930, NULL, NULL, NULL,
};
char *gCombineCountTexts[2] = {
    D_00841940, D_00841960,
};
char *gSellCountTexts[2] = {
    D_00841980, D_0083C630,
};
char *gPriceConfirmTexts[2] = {
    D_0083C630, D_008419A0,
};
char *gCombinePromptTexts[2] = {
    D_008419D0, D_00841A00,
};
char *gRecipePromptTexts[6] = {
    D_00841A70, D_00841AA0, D_00841AD0, D_00841B40,
    D_00841B70, D_00841BA0,
};
char *gCoursePromptTexts[2] = {
    D_00841C10, D_00841C70,
};
char *gSongHelpTexts[4] = {
    D_00841CE0, D_00841D20, NULL, NULL,
};
char *gMemberCardPanels_0[10] = {
    D_00841D78, D_00841D88, D_00841D98, D_00841DA8,
    gHunterNotesTexts_8, D_00841DB8, D_00841DD0, D_00841DE8,
    D_00841E00, D_00841510,
};
char *gStatusTitleTexts[2] = {
    D_00841E18, D_00841E30,
};
char *gFriendRegTexts[4] = {
    D_00841E60, D_00841E80, NULL, NULL,
};
char *gBedMenuTexts[6] = {
    D_00841EA0, D_00841EA8, D_00841EB0, D_00841EC0,
    D_00841F00, D_00841F60,
};
char *gLivingCostTexts[2] = {
    D_00841FB0, NULL,
};
char *gMonsterClassTexts[76] = {
    D_00841FE0, D_00841FF0, D_00841FF8, gHunterNotesTexts_8,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00842000, D_00841FF8,
    D_00842008, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0, D_00841FF0,
    gHunterNotesTexts_8, D_00841FF0, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_00842000, D_00842008, D_00841FF0, gHunterNotesTexts_8,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0, D_00842000,
    D_00842000, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00842000,
    gHunterNotesTexts_8, D_00841FF8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_00841FF8, D_00841FF0, D_00842000, D_00842000,
    D_00842000, D_00841FF0, D_00841FF0, D_00841FF0,
    D_00841FF0, D_00841FF0, D_00842008, D_00841FF0,
    D_00842010, D_00841FF0, D_00841FF8, D_00842018,
    D_00842018, D_00842018, D_00841FF8, D_00842010,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF8, D_00841FF8,
    D_00841FF8, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0,
    D_00841FF8, D_00841FF8, gHunterNotesTexts_8, D_00842010,
    D_00842018, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF8,
    gHunterNotesTexts_8, gHunterNotesTexts_8, NULL, NULL,
};
char *gGalleryCourseTexts[20] = {
    D_00842020, D_00842030, D_00842040, D_00842050,
    D_00842060, D_00842070, D_00842080, D_00842090,
    D_008420A0, D_008420A8, NULL, NULL,
    D_008420C0, D_008420E0, D_00842100, D_00842120,
    D_00842140, D_00842160, D_00841350, D_00842170,
};
char *gLobbyNoticeTexts[44] = {
    D_00842190, D_008421B0, D_008421D0, D_008421F0,
    D_00842210, D_00842230, D_00842260, D_00842290,
    D_008422C0, D_008422F0, D_00842320, D_00842340,
    D_00842360, D_00842380, D_008423A0, D_008423C0,
    D_008423E0, D_00842400, D_00842420, D_00842440,
    D_00842470, D_00842490, D_008424B0, D_008424D0,
    D_008424E0, D_00842500, D_00842530, D_00842560,
    D_00842580, D_008425B0, D_008425D8, D_008425E8,
    D_00842600, D_00842620, D_00842660, D_00842680,
    D_008426A0, D_008426D0, D_00842700, D_00842720,
    D_00842750, NULL, NULL, NULL,
};
char *gLobbyEventTexts[12] = {
    D_00842770, D_00842790, D_008427B0, D_008427D0,
    D_008427E8, D_008427F8, D_00842810, D_00842840,
    D_00842860, D_00842880, D_008428A0, D_008428B0,
};
char *gSpotActionTexts[28] = {
    D_008428C8, D_008428D8, D_008428E8, D_00842900,
    D_00842920, D_00842930, D_00842940, D_00842958,
    D_00842968, D_00842978, D_008428D8, D_00842988,
    D_00842990, D_008429A0, D_008429B8, D_008429C8,
    D_0083CF08, D_008429D8, D_008429E8, D_008429F8,
    D_00842A10, D_00842A28, D_00842A38, D_00842A40,
    D_00842A58, D_00842A70, D_00842A88, NULL,
};
char *gStandUpTexts[2] = {
    D_00842A98, D_00842AB0,
};
char *gSaveGameTexts[2] = {
    D_00842AD0, NULL,
};
char *gLobbyTitleTexts[8] = {
    D_00842AF0, D_00842AF8, D_00842B08, D_00842B18,
    D_00842B28, NULL, NULL, NULL,
};
char *gLobbyHelpTexts[68] = {
    D_00842B40, D_00842B60, D_00842C00, D_00842C30,
    D_00842C60, D_00842C80, D_00842D00, D_00842D30,
    D_00842D50, D_00842D90, D_00842DC0, D_00842DF0,
    D_00840B10, D_00842E10, D_00842E40, D_00842EC0,
    D_00842F00, D_00842F30, D_00842F60, D_00842FC0,
    D_00842FE0, D_00843090, D_00843130, D_008431C0,
    D_00843260, D_00843290, D_008432D0, D_008432F0,
    D_00843340, D_008433E0, gHunterNotesTexts_8, D_00843480,
    D_008434B0, D_008434F0, D_00843520, D_008435B0,
    D_00843660, D_008436F0, D_00843730, D_00843770,
    D_008437B0, D_00843800, D_00840BF0, D_00843840,
    D_00843880, D_00840CC0, D_008438C0, D_008438F0,
    D_0083CF30, D_0083CF58, D_0083CF38, D_00843980,
    D_0083CF40, D_00843988, D_00843998, D_008439A8,
    D_0083BB60, D_0083CF28, D_0083C950, D_0083CF48,
    D_008439B0, D_008439B8, D_008439C0, D_008439C8,
    D_008439D8, D_008439E0, D_008439E8, D_008439F0,
};
char *gMemberInfoTexts[58] = {
    D_008439F8, D_00843A00, D_00843A08, D_0083D130,
    D_00843A18, D_0083D088, D_0083D170, D_0083BC80,
    D_00843A20, D_00843A28, D_00843A40, D_00843A60,
    D_00843A78, D_00843A88, D_00843AA0, D_00843AB8,
    D_00843AC8, D_00843AE0, D_00843AF8, D_00843998,
    D_00843B00, D_00843B10, D_00840318, D_00840828,
    D_008402B8, D_00840240, D_00843B20, D_00843B30,
    D_00843B40, D_00843B60, D_00843B80, D_00843BA0,
    D_00843BC0, D_00843BE0, D_00843C00, D_00843C20,
    D_00840318, D_00843C40, D_00843C48, D_00843C58,
    D_00843C68, D_00843C80, D_00843CA0, D_00843CC0,
    D_0083D060, D_00843CE0, D_00843D10, D_00843D30,
    D_00843D40, D_00843D50, D_00841920, D_00843D60,
    D_00843D70, D_00843D88, D_00843D98, D_00843DB0,
    D_00843DD0, D_00843DE8,
};
char *gNoSaveTexts[2] = {
    D_00843DF0, D_00843DF8,
};
char *gNetStatusTexts[40] = {
    D_00843E10, D_00843E28, D_00843E30, D_0083C2E0,
    D_00843E40, D_0083C5B8, D_00843E50, D_00843E60,
    D_00843E70, D_00843E80, NULL, NULL,
    D_00843E90, D_00843EB0, D_00843ED0, D_00843EE0,
    D_0083CFE0, D_00843EF0, D_00843EF8, D_0083BC80,
    D_00843F08, D_0083D318, D_00843F18, D_00843F28,
    D_00843F38, D_00843A88, D_00843F48, D_00843F58,
    D_00843F68, D_00843F70, D_00843F88, D_0083D328,
    D_00843F90, D_00843FA0, D_008402C8, D_00843FB0,
    D_00841410, D_00843FC0, D_00843FD0, NULL,
};
LobbyMsgDef gLobbyMsgTexts[152] = {
#include "lobby/0080D7B0.inc"
};

/* unk_7A1120's .rodata (0x83BA90-0x8466D0) */
#pragma rodata on
char D_0083BA90[0x8] = {
#include "lobby/0083BA90.inc"
};
char D_0083BA98[0x8] = {
#include "lobby/0083BA98.inc"
};
char D_0083BAA0[0x8] = {
#include "lobby/0083BAA0.inc"
};
char D_0083BAA8[0x8] = {
#include "lobby/0083BAA8.inc"
};
char D_0083BAB0[0x8] = {
#include "lobby/0083BAB0.inc"
};
char D_0083BAB8[0x8] = {
#include "lobby/0083BAB8.inc"
};
char D_0083BAC0[0x8] = {
#include "lobby/0083BAC0.inc"
};
char D_0083BAC8[0x8] = {
#include "lobby/0083BAC8.inc"
};
char D_0083BAD0[0x10] = {
#include "lobby/0083BAD0.inc"
};
char D_0083BAE0[0x18] = {
#include "lobby/0083BAE0.inc"
};
char D_0083BAF8[0x8] = {
#include "lobby/0083BAF8.inc"
};
char D_0083BB00[0x8] = {
#include "lobby/0083BB00.inc"
};
char D_0083BB08[0x8] = {
#include "lobby/0083BB08.inc"
};
char D_0083BB10[0x8] = {
#include "lobby/0083BB10.inc"
};
char D_0083BB18[0xC] = {
#include "lobby/0083BB18.inc"
};
char D_0083BB28[0x8] = {
#include "lobby/0083BB28.inc"
};
char D_0083BB30[0x8] = {
#include "lobby/0083BB30.inc"
};
char D_0083BB38[0x8] = {
#include "lobby/0083BB38.inc"
};
char D_0083BB40[0x8] = {
#include "lobby/0083BB40.inc"
};
char D_0083BB48[0x8] = {
#include "lobby/0083BB48.inc"
};
char D_0083BB50[0x8] = {
#include "lobby/0083BB50.inc"
};
char D_0083BB58[0x8] = {
#include "lobby/0083BB58.inc"
};
char D_0083BB60[0x8] = {
#include "lobby/0083BB60.inc"
};
char D_0083BB68[0x8] = {
#include "lobby/0083BB68.inc"
};
char D_0083BB70[0x8] = {
#include "lobby/0083BB70.inc"
};
char D_0083BB78[0xC] = {
#include "lobby/0083BB78.inc"
};
char D_0083BB88[0xC] = {
#include "lobby/0083BB88.inc"
};
char D_0083BB98[0xC] = {
#include "lobby/0083BB98.inc"
};
char D_0083BBA8[15] = {
#include "lobby/0083BBA8.inc"
};
char D_0083BBC0[0x18] = {
#include "lobby/0083BBC0.inc"
};
char D_0083BBD8[0x8] = {
#include "lobby/0083BBD8.inc"
};
char D_0083BBE0[0x10] = {
#include "lobby/0083BBE0.inc"
};
char D_0083BBF0[0x10] = {
#include "lobby/0083BBF0.inc"
};
char D_0083BC00[0x10] = {
#include "lobby/0083BC00.inc"
};
char D_0083BC10[0x8] = {
#include "lobby/0083BC10.inc"
};
char D_0083BC18[15] = {
#include "lobby/0083BC18.inc"
};
char D_0083BC28[15] = {
#include "lobby/0083BC28.inc"
};
char D_0083BC38[13] = {
#include "lobby/0083BC38.inc"
};
char D_0083BC48[0x8] = {
#include "lobby/0083BC48.inc"
};
char D_0083BC50[0x10] = {
#include "lobby/0083BC50.inc"
};
char D_0083BC60[0x10] = {
#include "lobby/0083BC60.inc"
};
char D_0083BC70[0x10] = {
#include "lobby/0083BC70.inc"
};
char D_0083BC80[0x10] = {
#include "lobby/0083BC80.inc"
};
char D_0083BC90[0x10] = {
#include "lobby/0083BC90.inc"
};
char D_0083BCA0[0x10] = {
#include "lobby/0083BCA0.inc"
};
char D_0083BCB0[0x18] = {
#include "lobby/0083BCB0.inc"
};
char D_0083BCC8[0xC] = {
#include "lobby/0083BCC8.inc"
};
char D_0083BCD8[13] = {
#include "lobby/0083BCD8.inc"
};
char D_0083BCE8[0xC] = {
#include "lobby/0083BCE8.inc"
};
char D_0083BD00[0x18] = {
#include "lobby/0083BD00.inc"
};
char D_0083BD18[0xC] = {
#include "lobby/0083BD18.inc"
};
char D_0083BD28[13] = {
#include "lobby/0083BD28.inc"
};
char D_0083BD40[0x18] = {
#include "lobby/0083BD40.inc"
};
char D_0083BD58[15] = {
#include "lobby/0083BD58.inc"
};
char D_0083BD68[13] = {
#include "lobby/0083BD68.inc"
};
char D_0083BD78[0xC] = {
#include "lobby/0083BD78.inc"
};
char D_0083BD90[0x20] = {
#include "lobby/0083BD90.inc"
};
char D_0083BDB0[0x20] = {
#include "lobby/0083BDB0.inc"
};
char D_0083BDD0[0x20] = {
#include "lobby/0083BDD0.inc"
};
char D_0083BDF0[0x18] = {
#include "lobby/0083BDF0.inc"
};
char D_0083BE08[13] = {
#include "lobby/0083BE08.inc"
};
char D_0083BE18[0xC] = {
#include "lobby/0083BE18.inc"
};
char D_0083BE28[13] = {
#include "lobby/0083BE28.inc"
};
char D_0083BE38[0xC] = {
#include "lobby/0083BE38.inc"
};
char D_0083BE48[0xC] = {
#include "lobby/0083BE48.inc"
};
char D_0083BE58[15] = {
#include "lobby/0083BE58.inc"
};
char D_0083BE68[0x8] = {
#include "lobby/0083BE68.inc"
};
char D_0083BE70[0x8] = {
#include "lobby/0083BE70.inc"
};
char D_0083BE78[0xC] = {
#include "lobby/0083BE78.inc"
};
char D_0083BE90[0x10] = {
#include "lobby/0083BE90.inc"
};
char D_0083BEA0[0x10] = {
#include "lobby/0083BEA0.inc"
};
char D_0083BEB0[0x10] = {
#include "lobby/0083BEB0.inc"
};
char D_0083BEC0[0x10] = {
#include "lobby/0083BEC0.inc"
};
char D_0083BED0[0x10] = {
#include "lobby/0083BED0.inc"
};
char D_0083BEE0[0x10] = {
#include "lobby/0083BEE0.inc"
};
char D_0083BEF0[0x10] = {
#include "lobby/0083BEF0.inc"
};
char D_0083BF00[0x10] = {
#include "lobby/0083BF00.inc"
};
char D_0083BF10[0x10] = {
#include "lobby/0083BF10.inc"
};
char D_0083BF20[0x10] = {
#include "lobby/0083BF20.inc"
};
char D_0083BF30[0x10] = {
#include "lobby/0083BF30.inc"
};
char D_0083BF40[0x10] = {
#include "lobby/0083BF40.inc"
};
char D_0083BF50[0x10] = {
#include "lobby/0083BF50.inc"
};
char D_0083BF60[0x10] = {
#include "lobby/0083BF60.inc"
};
char D_0083BF70[0x10] = {
#include "lobby/0083BF70.inc"
};
char D_0083BF80[0x10] = {
#include "lobby/0083BF80.inc"
};
char D_0083BF90[0x10] = {
#include "lobby/0083BF90.inc"
};
char D_0083BFA0[0x10] = {
#include "lobby/0083BFA0.inc"
};
char D_0083BFB0[0x20] = {
#include "lobby/0083BFB0.inc"
};
char D_0083BFD0[0x18] = {
#include "lobby/0083BFD0.inc"
};
char D_0083BFE8[0xC] = {
#include "lobby/0083BFE8.inc"
};
char D_0083BFF8[15] = {
#include "lobby/0083BFF8.inc"
};
char D_0083C008[0xC] = {
#include "lobby/0083C008.inc"
};
char D_0083C018[0xC] = {
#include "lobby/0083C018.inc"
};
char D_0083C030[0x20] = {
#include "lobby/0083C030.inc"
};
char D_0083C050[0x10] = {
#include "lobby/0083C050.inc"
};
char D_0083C060[0x18] = {
#include "lobby/0083C060.inc"
};
char D_0083C078[0x8] = {
#include "lobby/0083C078.inc"
};
char D_0083C080[0x8] = {
#include "lobby/0083C080.inc"
};
char D_0083C088[0xC] = {
#include "lobby/0083C088.inc"
};
char D_0083C098[0x8] = {
#include "lobby/0083C098.inc"
};
char D_0083C0A0[0x18] = {
#include "lobby/0083C0A0.inc"
};
char D_0083C0B8[13] = {
#include "lobby/0083C0B8.inc"
};
char D_0083C0D0[0x18] = {
#include "lobby/0083C0D0.inc"
};
char D_0083C0E8[15] = {
#include "lobby/0083C0E8.inc"
};
char D_0083C100[0x20] = {
#include "lobby/0083C100.inc"
};
char D_0083C120[0x30] = {
#include "lobby/0083C120.inc"
};
char D_0083C150[0x20] = {
#include "lobby/0083C150.inc"
};
char D_0083C170[0x10] = {
#include "lobby/0083C170.inc"
};
char D_0083C180[0x20] = {
#include "lobby/0083C180.inc"
};
char D_0083C1A0[0x8] = {
#include "lobby/0083C1A0.inc"
};
char D_0083C1A8[0xC] = {
#include "lobby/0083C1A8.inc"
};
char D_0083C1B8[0xC] = {
#include "lobby/0083C1B8.inc"
};
char D_0083C1D0[0x20] = {
#include "lobby/0083C1D0.inc"
};
char D_0083C1F0[0x18] = {
#include "lobby/0083C1F0.inc"
};
char D_0083C208[0xC] = {
#include "lobby/0083C208.inc"
};
char D_0083C218[0xC] = {
#include "lobby/0083C218.inc"
};
char D_0083C228[0xC] = {
#include "lobby/0083C228.inc"
};
char D_0083C240[0x18] = {
#include "lobby/0083C240.inc"
};
char D_0083C258[0x8] = {
#include "lobby/0083C258.inc"
};
char D_0083C260[0x10] = {
#include "lobby/0083C260.inc"
};
char D_0083C270[0x10] = {
#include "lobby/0083C270.inc"
};
char D_0083C280[0x10] = {
#include "lobby/0083C280.inc"
};
char D_0083C290[0x10] = {
#include "lobby/0083C290.inc"
};
char D_0083C2A0[0x8] = {
#include "lobby/0083C2A0.inc"
};
char D_0083C2A8[0xC] = {
#include "lobby/0083C2A8.inc"
};
char D_0083C2B8[0x8] = {
#include "lobby/0083C2B8.inc"
};
char D_0083C2C0[0x10] = {
#include "lobby/0083C2C0.inc"
};
char D_0083C2D0[0x10] = {
#include "lobby/0083C2D0.inc"
};
char D_0083C2E0[0x10] = {
#include "lobby/0083C2E0.inc"
};
char D_0083C2F0[0x10] = {
#include "lobby/0083C2F0.inc"
};
char D_0083C300[0x10] = {
#include "lobby/0083C300.inc"
};
char D_0083C310[0x10] = {
#include "lobby/0083C310.inc"
};
char D_0083C320[0x8] = {
#include "lobby/0083C320.inc"
};
char D_0083C328[0xC] = {
#include "lobby/0083C328.inc"
};
char D_0083C340[0x30] = {
#include "lobby/0083C340.inc"
};
char D_0083C370[0x30] = {
#include "lobby/0083C370.inc"
};
char D_0083C3A0[0x28] = {
#include "lobby/0083C3A0.inc"
};
char D_0083C3C8[0x8] = {
#include "lobby/0083C3C8.inc"
};
char D_0083C3D0[0x10] = {
#include "lobby/0083C3D0.inc"
};
char D_0083C3E0[0x20] = {
#include "lobby/0083C3E0.inc"
};
char D_0083C400[0x40] = {
#include "lobby/0083C400.inc"
};
char D_0083C440[0x30] = {
#include "lobby/0083C440.inc"
};
char D_0083C470[0x28] = {
#include "lobby/0083C470.inc"
};
char D_0083C498[13] = {
#include "lobby/0083C498.inc"
};
char D_0083C4B0[0x18] = {
#include "lobby/0083C4B0.inc"
};
char D_0083C4C8[13] = {
#include "lobby/0083C4C8.inc"
};
char D_0083C4D8[0xC] = {
#include "lobby/0083C4D8.inc"
};
char D_0083C4E8[0xC] = {
#include "lobby/0083C4E8.inc"
};
char D_0083C500[0x20] = {
#include "lobby/0083C500.inc"
};
char D_0083C520[0x20] = {
#include "lobby/0083C520.inc"
};
char D_0083C540[0x20] = {
#include "lobby/0083C540.inc"
};
char D_0083C560[0x20] = {
#include "lobby/0083C560.inc"
};
char D_0083C580[0x18] = {
#include "lobby/0083C580.inc"
};
char D_0083C598[15] = {
#include "lobby/0083C598.inc"
};
char D_0083C5A8[15] = {
#include "lobby/0083C5A8.inc"
};
char D_0083C5B8[0xC] = {
#include "lobby/0083C5B8.inc"
};
char D_0083C5C8[0x8] = {
#include "lobby/0083C5C8.inc"
};
char D_0083C5D0[0x8] = {
#include "lobby/0083C5D0.inc"
};
char D_0083C5D8[0x8] = {
#include "lobby/0083C5D8.inc"
};
char D_0083C5E0[0x10] = {
#include "lobby/0083C5E0.inc"
};
char D_0083C5F0[0x20] = {
#include "lobby/0083C5F0.inc"
};
char D_0083C610[0x20] = {
#include "lobby/0083C610.inc"
};
char D_0083C630[0x30] = {
#include "lobby/0083C630.inc"
};
char D_0083C660[0x30] = {
#include "lobby/0083C660.inc"
};
char D_0083C690[0x18] = {
#include "lobby/0083C690.inc"
};
char D_0083C6A8[15] = {
#include "lobby/0083C6A8.inc"
};
char D_0083C6B8[0xC] = {
#include "lobby/0083C6B8.inc"
};
char D_0083C6C8[13] = {
#include "lobby/0083C6C8.inc"
};
char D_0083C6D8[0xC] = {
#include "lobby/0083C6D8.inc"
};
char D_0083C6E8[0xC] = {
#include "lobby/0083C6E8.inc"
};
char D_0083C6F8[0x8] = {
#include "lobby/0083C6F8.inc"
};
char D_0083C700[0x10] = {
#include "lobby/0083C700.inc"
};
char D_0083C710[0x20] = {
#include "lobby/0083C710.inc"
};
char D_0083C730[0x30] = {
#include "lobby/0083C730.inc"
};
char D_0083C760[0x30] = {
#include "lobby/0083C760.inc"
};
char D_0083C790[0x30] = {
#include "lobby/0083C790.inc"
};
char D_0083C7C0[0x50] = {
#include "lobby/0083C7C0.inc"
};
char D_0083C810[0x20] = {
#include "lobby/0083C810.inc"
};
char D_0083C830[0x30] = {
#include "lobby/0083C830.inc"
};
char D_0083C860[0x30] = {
#include "lobby/0083C860.inc"
};
char D_0083C890[0x20] = {
#include "lobby/0083C890.inc"
};
char D_0083C8B0[0x20] = {
#include "lobby/0083C8B0.inc"
};
char D_0083C8D0[0x10] = {
#include "lobby/0083C8D0.inc"
};
char D_0083C8E0[0x10] = {
#include "lobby/0083C8E0.inc"
};
char D_0083C8F0[0x18] = {
#include "lobby/0083C8F0.inc"
};
char D_0083C908[0xC] = {
#include "lobby/0083C908.inc"
};
char D_0083C918[15] = {
#include "lobby/0083C918.inc"
};
char D_0083C928[0x8] = {
#include "lobby/0083C928.inc"
};
char D_0083C930[0x10] = {
#include "lobby/0083C930.inc"
};
char D_0083C940[0x10] = {
#include "lobby/0083C940.inc"
};
char D_0083C950[0x10] = {
#include "lobby/0083C950.inc"
};
char D_0083C960[0x20] = {
#include "lobby/0083C960.inc"
};
char D_0083C980[0x20] = {
#include "lobby/0083C980.inc"
};
char D_0083C9A0[0x20] = {
#include "lobby/0083C9A0.inc"
};
#pragma explicit_zero_data on
char gHunterNotesTexts_8[0x10] = {
#include "lobby/0083C9C0.inc"
};
#pragma explicit_zero_data reset
char D_0083C9D0[0x18] = {
#include "lobby/0083C9D0.inc"
};
char D_0083C9E8[0x8] = {
#include "lobby/0083C9E8.inc"
};
char D_0083C9F0[0x8] = {
#include "lobby/0083C9F0.inc"
};
char D_0083C9F8[0xC] = {
#include "lobby/0083C9F8.inc"
};
char D_0083CA08[0x8] = {
#include "lobby/0083CA08.inc"
};
char D_0083CA10[0x18] = {
#include "lobby/0083CA10.inc"
};
char D_0083CA28[0xC] = {
#include "lobby/0083CA28.inc"
};
char D_0083CA38[0x8] = {
#include "lobby/0083CA38.inc"
};
char D_0083CA40[0x20] = {
#include "lobby/0083CA40.inc"
};
char D_0083CA60[0x10] = {
#include "lobby/0083CA60.inc"
};
char D_0083CA70[0x20] = {
#include "lobby/0083CA70.inc"
};
char D_0083CA90[0x20] = {
#include "lobby/0083CA90.inc"
};
char D_0083CAB0[0x18] = {
#include "lobby/0083CAB0.inc"
};
char D_0083CAC8[0xC] = {
#include "lobby/0083CAC8.inc"
};
char D_0083CAD8[0xC] = {
#include "lobby/0083CAD8.inc"
};
char D_0083CAE8[0x8] = {
#include "lobby/0083CAE8.inc"
};
char D_0083CAF0[0x10] = {
#include "lobby/0083CAF0.inc"
};
char D_0083CB00[0x10] = {
#include "lobby/0083CB00.inc"
};
char D_0083CB10[0x10] = {
#include "lobby/0083CB10.inc"
};
char D_0083CB20[0x10] = {
#include "lobby/0083CB20.inc"
};
char D_0083CB30[0x8] = {
#include "lobby/0083CB30.inc"
};
char D_0083CB38[0xC] = {
#include "lobby/0083CB38.inc"
};
char D_0083CB50[0x30] = {
#include "lobby/0083CB50.inc"
};
char D_0083CB80[0x50] = {
#include "lobby/0083CB80.inc"
};
char D_0083CBD0[0x30] = {
#include "lobby/0083CBD0.inc"
};
char D_0083CC00[0x20] = {
#include "lobby/0083CC00.inc"
};
char D_0083CC20[0x10] = {
#include "lobby/0083CC20.inc"
};
char D_0083CC30[0x10] = {
#include "lobby/0083CC30.inc"
};
char D_0083CC40[0x30] = {
#include "lobby/0083CC40.inc"
};
char D_0083CC70[0x50] = {
#include "lobby/0083CC70.inc"
};
char D_0083CCC0[0x50] = {
#include "lobby/0083CCC0.inc"
};
char D_0083CD10[0x30] = {
#include "lobby/0083CD10.inc"
};
char D_0083CD40[0x50] = {
#include "lobby/0083CD40.inc"
};
char D_0083CD90[0x20] = {
#include "lobby/0083CD90.inc"
};
char D_0083CDB0[0x18] = {
#include "lobby/0083CDB0.inc"
};
char D_0083CDC8[0xC] = {
#include "lobby/0083CDC8.inc"
};
char D_0083CDD8[15] = {
#include "lobby/0083CDD8.inc"
};
char D_0083CDE8[13] = {
#include "lobby/0083CDE8.inc"
};
char D_0083CDF8[0xC] = {
#include "lobby/0083CDF8.inc"
};
char D_0083CE08[0xC] = {
#include "lobby/0083CE08.inc"
};
char D_0083CE18[0xC] = {
#include "lobby/0083CE18.inc"
};
char D_0083CE30[0x30] = {
#include "lobby/0083CE30.inc"
};
char D_0083CE60[0x20] = {
#include "lobby/0083CE60.inc"
};
char D_0083CE80[0x30] = {
#include "lobby/0083CE80.inc"
};
char D_0083CEB0[0x18] = {
#include "lobby/0083CEB0.inc"
};
char D_0083CEC8[0xC] = {
#include "lobby/0083CEC8.inc"
};
char D_0083CED8[0xC] = {
#include "lobby/0083CED8.inc"
};
char D_0083CEF0[0x18] = {
#include "lobby/0083CEF0.inc"
};
char D_0083CF08[15] = {
#include "lobby/0083CF08.inc"
};
char D_0083CF18[0x8] = {
#include "lobby/0083CF18.inc"
};
char D_0083CF20[0x8] = {
#include "lobby/0083CF20.inc"
};
char D_0083CF28[0x8] = {
#include "lobby/0083CF28.inc"
};
char D_0083CF30[0x8] = {
#include "lobby/0083CF30.inc"
};
char D_0083CF38[0x8] = {
#include "lobby/0083CF38.inc"
};
char D_0083CF40[0x8] = {
#include "lobby/0083CF40.inc"
};
char D_0083CF48[0xC] = {
#include "lobby/0083CF48.inc"
};
char D_0083CF58[0x8] = {
#include "lobby/0083CF58.inc"
};
char D_0083CF60[0x8] = {
#include "lobby/0083CF60.inc"
};
char D_0083CF68[0xC] = {
#include "lobby/0083CF68.inc"
};
char D_0083CF80[0x40] = {
#include "lobby/0083CF80.inc"
};
char D_0083CFC0[0x10] = {
#include "lobby/0083CFC0.inc"
};
char D_0083CFD0[0x10] = {
#include "lobby/0083CFD0.inc"
};
char D_0083CFE0[0x8] = {
#include "lobby/0083CFE0.inc"
};
char D_0083CFE8[13] = {
#include "lobby/0083CFE8.inc"
};
char D_0083D000[0x18] = {
#include "lobby/0083D000.inc"
};
char D_0083D018[13] = {
#include "lobby/0083D018.inc"
};
char D_0083D028[0x8] = {
#include "lobby/0083D028.inc"
};
char D_0083D030[0x10] = {
#include "lobby/0083D030.inc"
};
char D_0083D040[0x10] = {
#include "lobby/0083D040.inc"
};
char D_0083D050[0x10] = {
#include "lobby/0083D050.inc"
};
char D_0083D060[0x10] = {
#include "lobby/0083D060.inc"
};
char D_0083D070[0x18] = {
#include "lobby/0083D070.inc"
};
char D_0083D088[0xC] = {
#include "lobby/0083D088.inc"
};
char D_0083D0A0[0x18] = {
#include "lobby/0083D0A0.inc"
};
char D_0083D0B8[0xC] = {
#include "lobby/0083D0B8.inc"
};
char D_0083D0C8[15] = {
#include "lobby/0083D0C8.inc"
};
char D_0083D0D8[15] = {
#include "lobby/0083D0D8.inc"
};
char D_0083D0E8[15] = {
#include "lobby/0083D0E8.inc"
};
char D_0083D0F8[0x8] = {
#include "lobby/0083D0F8.inc"
};
char D_0083D100[0x18] = {
#include "lobby/0083D100.inc"
};
char D_0083D118[15] = {
#include "lobby/0083D118.inc"
};
char D_0083D128[0x8] = {
#include "lobby/0083D128.inc"
};
char D_0083D130[0x10] = {
#include "lobby/0083D130.inc"
};
char D_0083D140[0x18] = {
#include "lobby/0083D140.inc"
};
char D_0083D158[0xC] = {
#include "lobby/0083D158.inc"
};
char D_0083D168[0x8] = {
#include "lobby/0083D168.inc"
};
char D_0083D170[0x10] = {
#include "lobby/0083D170.inc"
};
char D_0083D180[0x8] = {
#include "lobby/0083D180.inc"
};
char D_0083D188[0x8] = {
#include "lobby/0083D188.inc"
};
char D_0083D190[0x8] = {
#include "lobby/0083D190.inc"
};
char D_0083D198[15] = {
#include "lobby/0083D198.inc"
};
char D_0083D1B0[0x70] = {
#include "lobby/0083D1B0.inc"
};
char D_0083D220[0x60] = {
#include "lobby/0083D220.inc"
};
char D_0083D280[0x30] = {
#include "lobby/0083D280.inc"
};
char D_0083D2B0[0x60] = {
#include "lobby/0083D2B0.inc"
};
char D_0083D310[0x8] = {
#include "lobby/0083D310.inc"
};
char D_0083D318[13] = {
#include "lobby/0083D318.inc"
};
char D_0083D328[0xC] = {
#include "lobby/0083D328.inc"
};
char D_0083D338[0x8] = {
#include "lobby/0083D338.inc"
};
char D_0083D340[0x10] = {
#include "lobby/0083D340.inc"
};
char D_0083D350[0x10] = {
#include "lobby/0083D350.inc"
};
char D_0083D360[0x10] = {
#include "lobby/0083D360.inc"
};
char D_0083D370[0x8] = {
#include "lobby/0083D370.inc"
};
char D_0083D378[0x8] = {
#include "lobby/0083D378.inc"
};
char D_0083D380[0x8] = {
#include "lobby/0083D380.inc"
};
char D_0083D388[15] = {
#include "lobby/0083D388.inc"
};
char D_0083D398[15] = {
#include "lobby/0083D398.inc"
};
char D_0083D3A8[15] = {
#include "lobby/0083D3A8.inc"
};
char D_0083D3B8[0xC] = {
#include "lobby/0083D3B8.inc"
};
char D_0083D3C8[0xC] = {
#include "lobby/0083D3C8.inc"
};
char D_0083D3D8[0xC] = {
#include "lobby/0083D3D8.inc"
};
char D_0083D3E8[0xC] = {
#include "lobby/0083D3E8.inc"
};
char D_0083D3F8[0x8] = {
#include "lobby/0083D3F8.inc"
};
char D_0083D400[0x20] = {
#include "lobby/0083D400.inc"
};
char D_0083D420[0x20] = {
#include "lobby/0083D420.inc"
};
char D_0083D440[0x18] = {
#include "lobby/0083D440.inc"
};
char D_0083D458[0xC] = {
#include "lobby/0083D458.inc"
};
char D_0083D468[0xC] = {
#include "lobby/0083D468.inc"
};
char D_0083D478[0xC] = {
#include "lobby/0083D478.inc"
};
char D_0083D488[0xC] = {
#include "lobby/0083D488.inc"
};
char D_0083D498[0xC] = {
#include "lobby/0083D498.inc"
};
char D_0083D4A8[15] = {
#include "lobby/0083D4A8.inc"
};
char D_0083D4B8[0x8] = {
#include "lobby/0083D4B8.inc"
};
char D_0083D4C0[0x10] = {
#include "lobby/0083D4C0.inc"
};
char D_0083D4D0[0x30] = {
#include "lobby/0083D4D0.inc"
};
char D_0083D500[0x20] = {
#include "lobby/0083D500.inc"
};
char D_0083D520[0x20] = {
#include "lobby/0083D520.inc"
};
char D_0083D540[0x10] = {
#include "lobby/0083D540.inc"
};
char D_0083D550[0x10] = {
#include "lobby/0083D550.inc"
};
char D_0083D560[0x10] = {
#include "lobby/0083D560.inc"
};
char D_0083D570[0x20] = {
#include "lobby/0083D570.inc"
};
char D_0083D590[0x10] = {
#include "lobby/0083D590.inc"
};
char D_0083D5A0[0x8] = {
#include "lobby/0083D5A0.inc"
};
char D_0083D5A8[0x8] = {
#include "lobby/0083D5A8.inc"
};
char D_0083D5B0[0x8] = {
#include "lobby/0083D5B0.inc"
};
char D_0083D5B8[0xC] = {
#include "lobby/0083D5B8.inc"
};
char D_0083D5C8[0xC] = {
#include "lobby/0083D5C8.inc"
};
char D_0083D5D8[0xC] = {
#include "lobby/0083D5D8.inc"
};
char D_0083D5E8[13] = {
#include "lobby/0083D5E8.inc"
};
char D_0083D600[0x20] = {
#include "lobby/0083D600.inc"
};
char D_0083D620[0x60] = {
#include "lobby/0083D620.inc"
};
char D_0083D680[0x80] = {
#include "lobby/0083D680.inc"
};
char D_0083D700[0x18] = {
#include "lobby/0083D700.inc"
};
char D_0083D718[0x8] = {
#include "lobby/0083D718.inc"
};
char D_0083D720[0x8] = {
#include "lobby/0083D720.inc"
};
char D_0083D728[0x8] = {
#include "lobby/0083D728.inc"
};
char D_0083D730[0x8] = {
#include "lobby/0083D730.inc"
};
char D_0083D738[0x8] = {
#include "lobby/0083D738.inc"
};
char D_0083D740[0x8] = {
#include "lobby/0083D740.inc"
};
char D_0083D748[0x8] = {
#include "lobby/0083D748.inc"
};
char D_0083D750[0x8] = {
#include "lobby/0083D750.inc"
};
char D_0083D758[0x8] = {
#include "lobby/0083D758.inc"
};
char D_0083D760[0x8] = {
#include "lobby/0083D760.inc"
};
char D_0083D768[0x8] = {
#include "lobby/0083D768.inc"
};
char D_0083D770[0x8] = {
#include "lobby/0083D770.inc"
};
char D_0083D778[0x8] = {
#include "lobby/0083D778.inc"
};
char D_0083D780[0x8] = {
#include "lobby/0083D780.inc"
};
char D_0083D788[0x8] = {
#include "lobby/0083D788.inc"
};
char D_0083D790[0x10] = {
#include "lobby/0083D790.inc"
};
char D_0083D7A0[0x18] = {
#include "lobby/0083D7A0.inc"
};
char D_0083D7B8[0x8] = {
#include "lobby/0083D7B8.inc"
};
char D_0083D7C0[0x20] = {
#include "lobby/0083D7C0.inc"
};
char D_0083D7E0[0x20] = {
#include "lobby/0083D7E0.inc"
};
char D_0083D800[0x20] = {
#include "lobby/0083D800.inc"
};
char D_0083D820[0x18] = {
#include "lobby/0083D820.inc"
};
char D_0083D838[0x8] = {
#include "lobby/0083D838.inc"
};
char D_0083D840[0x10] = {
#include "lobby/0083D840.inc"
};
char D_0083D850[0x10] = {
#include "lobby/0083D850.inc"
};
char D_0083D860[0x20] = {
#include "lobby/0083D860.inc"
};
char D_0083D880[0x18] = {
#include "lobby/0083D880.inc"
};
char D_0083D898[13] = {
#include "lobby/0083D898.inc"
};
char D_0083D8A8[0xC] = {
#include "lobby/0083D8A8.inc"
};
char D_0083D8B8[0xC] = {
#include "lobby/0083D8B8.inc"
};
char D_0083D8C8[0xC] = {
#include "lobby/0083D8C8.inc"
};
char D_0083D8D8[0xC] = {
#include "lobby/0083D8D8.inc"
};
char D_0083D8E8[0xC] = {
#include "lobby/0083D8E8.inc"
};
char D_0083D8F8[13] = {
#include "lobby/0083D8F8.inc"
};
char D_0083D908[0xC] = {
#include "lobby/0083D908.inc"
};
char D_0083D918[0xC] = {
#include "lobby/0083D918.inc"
};
char D_0083D928[0xC] = {
#include "lobby/0083D928.inc"
};
char D_0083D940[0x20] = {
#include "lobby/0083D940.inc"
};
char D_0083D960[0x18] = {
#include "lobby/0083D960.inc"
};
char D_0083D978[13] = {
#include "lobby/0083D978.inc"
};
char D_0083D988[13] = {
#include "lobby/0083D988.inc"
};
char D_0083D9A0[0x30] = {
#include "lobby/0083D9A0.inc"
};
char D_0083D9D0[0x30] = {
#include "lobby/0083D9D0.inc"
};
char D_0083DA00[0x50] = {
#include "lobby/0083DA00.inc"
};
char D_0083DA50[0x50] = {
#include "lobby/0083DA50.inc"
};
char D_0083DAA0[0x30] = {
#include "lobby/0083DAA0.inc"
};
char D_0083DAD0[0x20] = {
#include "lobby/0083DAD0.inc"
};
char D_0083DAF0[0x30] = {
#include "lobby/0083DAF0.inc"
};
char D_0083DB20[0x50] = {
#include "lobby/0083DB20.inc"
};
char D_0083DB70[0x40] = {
#include "lobby/0083DB70.inc"
};
char D_0083DBB0[0x30] = {
#include "lobby/0083DBB0.inc"
};
char D_0083DBE0[0x50] = {
#include "lobby/0083DBE0.inc"
};
char D_0083DC30[0x48] = {
#include "lobby/0083DC30.inc"
};
char D_0083DC78[0xC] = {
#include "lobby/0083DC78.inc"
};
char D_0083DC88[0xC] = {
#include "lobby/0083DC88.inc"
};
char D_0083DC98[0x8] = {
#include "lobby/0083DC98.inc"
};
char D_0083DCA0[0x10] = {
#include "lobby/0083DCA0.inc"
};
char D_0083DCB0[0x10] = {
#include "lobby/0083DCB0.inc"
};
char D_0083DCC0[0x20] = {
#include "lobby/0083DCC0.inc"
};
char D_0083DCE0[0x20] = {
#include "lobby/0083DCE0.inc"
};
char D_0083DD00[0x10] = {
#include "lobby/0083DD00.inc"
};
char D_0083DD10[0x10] = {
#include "lobby/0083DD10.inc"
};
char D_0083DD20[0x10] = {
#include "lobby/0083DD20.inc"
};
char D_0083DD30[0x10] = {
#include "lobby/0083DD30.inc"
};
char D_0083DD40[0x10] = {
#include "lobby/0083DD40.inc"
};
char D_0083DD50[0x10] = {
#include "lobby/0083DD50.inc"
};
char D_0083DD60[0x10] = {
#include "lobby/0083DD60.inc"
};
char D_0083DD70[0x10] = {
#include "lobby/0083DD70.inc"
};
char D_0083DD80[0x10] = {
#include "lobby/0083DD80.inc"
};
char D_0083DD90[0x10] = {
#include "lobby/0083DD90.inc"
};
char D_0083DDA0[0x20] = {
#include "lobby/0083DDA0.inc"
};
char D_0083DDC0[0x18] = {
#include "lobby/0083DDC0.inc"
};
char D_0083DDD8[15] = {
#include "lobby/0083DDD8.inc"
};
char D_0083DDF0[0x40] = {
#include "lobby/0083DDF0.inc"
};
char D_0083DE30[0x40] = {
#include "lobby/0083DE30.inc"
};
char D_0083DE70[0x40] = {
#include "lobby/0083DE70.inc"
};
char D_0083DEB0[0x30] = {
#include "lobby/0083DEB0.inc"
};
char D_0083DEE0[0x50] = {
#include "lobby/0083DEE0.inc"
};
char D_0083DF30[0x60] = {
#include "lobby/0083DF30.inc"
};
char D_0083DF90[0x20] = {
#include "lobby/0083DF90.inc"
};
char D_0083DFB0[0x20] = {
#include "lobby/0083DFB0.inc"
};
char D_0083DFD0[0x18] = {
#include "lobby/0083DFD0.inc"
};
char D_0083DFE8[13] = {
#include "lobby/0083DFE8.inc"
};
char D_0083DFF8[13] = {
#include "lobby/0083DFF8.inc"
};
char D_0083E008[15] = {
#include "lobby/0083E008.inc"
};
char D_0083E020[0x18] = {
#include "lobby/0083E020.inc"
};
char D_0083E038[13] = {
#include "lobby/0083E038.inc"
};
char D_0083E050[0x20] = {
#include "lobby/0083E050.inc"
};
char D_0083E070[0x20] = {
#include "lobby/0083E070.inc"
};
char D_0083E090[0x20] = {
#include "lobby/0083E090.inc"
};
char D_0083E0B0[0x20] = {
#include "lobby/0083E0B0.inc"
};
char D_0083E0D0[0x20] = {
#include "lobby/0083E0D0.inc"
};
char D_0083E0F0[0x18] = {
#include "lobby/0083E0F0.inc"
};
char D_0083E108[13] = {
#include "lobby/0083E108.inc"
};
char D_0083E118[15] = {
#include "lobby/0083E118.inc"
};
char D_0083E130[0x20] = {
#include "lobby/0083E130.inc"
};
char D_0083E150[0x20] = {
#include "lobby/0083E150.inc"
};
char D_0083E170[0x50] = {
#include "lobby/0083E170.inc"
};
char D_0083E1C0[0xA0] = {
#include "lobby/0083E1C0.inc"
};
char D_0083E260[0x60] = {
#include "lobby/0083E260.inc"
};
char D_0083E2C0[0x90] = {
#include "lobby/0083E2C0.inc"
};
char D_0083E350[0x60] = {
#include "lobby/0083E350.inc"
};
char D_0083E3B0[0x70] = {
#include "lobby/0083E3B0.inc"
};
char D_0083E420[0x70] = {
#include "lobby/0083E420.inc"
};
char D_0083E490[0xA0] = {
#include "lobby/0083E490.inc"
};
char D_0083E530[0x60] = {
#include "lobby/0083E530.inc"
};
char D_0083E590[0x80] = {
#include "lobby/0083E590.inc"
};
char D_0083E610[0x60] = {
#include "lobby/0083E610.inc"
};
char D_0083E670[0x30] = {
#include "lobby/0083E670.inc"
};
char D_0083E6A0[0x50] = {
#include "lobby/0083E6A0.inc"
};
char D_0083E6F0[0x50] = {
#include "lobby/0083E6F0.inc"
};
char D_0083E740[0x50] = {
#include "lobby/0083E740.inc"
};
char D_0083E790[0x50] = {
#include "lobby/0083E790.inc"
};
char D_0083E7E0[0x50] = {
#include "lobby/0083E7E0.inc"
};
char D_0083E830[0x50] = {
#include "lobby/0083E830.inc"
};
char D_0083E880[0x50] = {
#include "lobby/0083E880.inc"
};
char D_0083E8D0[0x60] = {
#include "lobby/0083E8D0.inc"
};
char D_0083E930[0x70] = {
#include "lobby/0083E930.inc"
};
char D_0083E9A0[0x30] = {
#include "lobby/0083E9A0.inc"
};
char D_0083E9D0[0x30] = {
#include "lobby/0083E9D0.inc"
};
char D_0083EA00[0x30] = {
#include "lobby/0083EA00.inc"
};
char D_0083EA30[0x20] = {
#include "lobby/0083EA30.inc"
};
char D_0083EA50[0x40] = {
#include "lobby/0083EA50.inc"
};
char D_0083EA90[0x20] = {
#include "lobby/0083EA90.inc"
};
char D_0083EAB0[0x50] = {
#include "lobby/0083EAB0.inc"
};
char D_0083EB00[0x70] = {
#include "lobby/0083EB00.inc"
};
char D_0083EB70[0x50] = {
#include "lobby/0083EB70.inc"
};
char D_0083EBC0[0x40] = {
#include "lobby/0083EBC0.inc"
};
char D_0083EC00[0x30] = {
#include "lobby/0083EC00.inc"
};
char D_0083EC30[0x30] = {
#include "lobby/0083EC30.inc"
};
char D_0083EC60[0x20] = {
#include "lobby/0083EC60.inc"
};
char D_0083EC80[0x18] = {
#include "lobby/0083EC80.inc"
};
char D_0083EC98[0xC] = {
#include "lobby/0083EC98.inc"
};
char D_0083ECA8[0xC] = {
#include "lobby/0083ECA8.inc"
};
char D_0083ECC0[0x18] = {
#include "lobby/0083ECC0.inc"
};
char D_0083ECD8[15] = {
#include "lobby/0083ECD8.inc"
};
char D_0083ECF0[0x20] = {
#include "lobby/0083ECF0.inc"
};
char D_0083ED10[0x20] = {
#include "lobby/0083ED10.inc"
};
char D_0083ED30[0x20] = {
#include "lobby/0083ED30.inc"
};
char D_0083ED50[0x20] = {
#include "lobby/0083ED50.inc"
};
char D_0083ED70[0x20] = {
#include "lobby/0083ED70.inc"
};
char D_0083ED90[0x18] = {
#include "lobby/0083ED90.inc"
};
char D_0083EDA8[0x8] = {
#include "lobby/0083EDA8.inc"
};
char D_0083EDB0[0x10] = {
#include "lobby/0083EDB0.inc"
};
char D_0083EDC0[0x20] = {
#include "lobby/0083EDC0.inc"
};
char D_0083EDE0[0x20] = {
#include "lobby/0083EDE0.inc"
};
char D_0083EE00[0x38] = {
#include "lobby/0083EE00.inc"
};
char D_0083EE38[0xC] = {
#include "lobby/0083EE38.inc"
};
char D_0083EE50[0x18] = {
#include "lobby/0083EE50.inc"
};
char D_0083EE68[15] = {
#include "lobby/0083EE68.inc"
};
char D_0083EE80[0x18] = {
#include "lobby/0083EE80.inc"
};
char D_0083EE98[15] = {
#include "lobby/0083EE98.inc"
};
char D_0083EEA8[15] = {
#include "lobby/0083EEA8.inc"
};
char D_0083EEC0[0x18] = {
#include "lobby/0083EEC0.inc"
};
char D_0083EED8[15] = {
#include "lobby/0083EED8.inc"
};
char D_0083EEE8[15] = {
#include "lobby/0083EEE8.inc"
};
char D_0083EEF8[15] = {
#include "lobby/0083EEF8.inc"
};
char D_0083EF08[15] = {
#include "lobby/0083EF08.inc"
};
char D_0083EF18[13] = {
#include "lobby/0083EF18.inc"
};
char D_0083EF28[13] = {
#include "lobby/0083EF28.inc"
};
char D_0083EF38[0xC] = {
#include "lobby/0083EF38.inc"
};
char D_0083EF48[15] = {
#include "lobby/0083EF48.inc"
};
char D_0083EF58[13] = {
#include "lobby/0083EF58.inc"
};
char D_0083EF68[15] = {
#include "lobby/0083EF68.inc"
};
char D_0083EF78[15] = {
#include "lobby/0083EF78.inc"
};
char D_0083EF90[0x20] = {
#include "lobby/0083EF90.inc"
};
char D_0083EFB0[0x20] = {
#include "lobby/0083EFB0.inc"
};
char D_0083EFD0[0x20] = {
#include "lobby/0083EFD0.inc"
};
char D_0083EFF0[0x20] = {
#include "lobby/0083EFF0.inc"
};
char D_0083F010[0x20] = {
#include "lobby/0083F010.inc"
};
char D_0083F030[0x20] = {
#include "lobby/0083F030.inc"
};
char D_0083F050[0x20] = {
#include "lobby/0083F050.inc"
};
char D_0083F070[0x20] = {
#include "lobby/0083F070.inc"
};
char D_0083F090[0x20] = {
#include "lobby/0083F090.inc"
};
char D_0083F0B0[0x20] = {
#include "lobby/0083F0B0.inc"
};
char D_0083F0D0[0x20] = {
#include "lobby/0083F0D0.inc"
};
char D_0083F0F0[0x20] = {
#include "lobby/0083F0F0.inc"
};
char D_0083F110[0x20] = {
#include "lobby/0083F110.inc"
};
char D_0083F130[0x10] = {
#include "lobby/0083F130.inc"
};
char D_0083F140[0x10] = {
#include "lobby/0083F140.inc"
};
char D_0083F150[0x20] = {
#include "lobby/0083F150.inc"
};
char D_0083F170[0x20] = {
#include "lobby/0083F170.inc"
};
char D_0083F190[0x20] = {
#include "lobby/0083F190.inc"
};
char D_0083F1B0[0x20] = {
#include "lobby/0083F1B0.inc"
};
char D_0083F1D0[0x20] = {
#include "lobby/0083F1D0.inc"
};
char D_0083F1F0[0x20] = {
#include "lobby/0083F1F0.inc"
};
char D_0083F210[0x20] = {
#include "lobby/0083F210.inc"
};
char D_0083F230[0x20] = {
#include "lobby/0083F230.inc"
};
char D_0083F250[0x20] = {
#include "lobby/0083F250.inc"
};
char D_0083F270[0x20] = {
#include "lobby/0083F270.inc"
};
char D_0083F290[0x20] = {
#include "lobby/0083F290.inc"
};
char D_0083F2B0[0x20] = {
#include "lobby/0083F2B0.inc"
};
char D_0083F2D0[0x18] = {
#include "lobby/0083F2D0.inc"
};
char D_0083F2E8[15] = {
#include "lobby/0083F2E8.inc"
};
char D_0083F300[0x18] = {
#include "lobby/0083F300.inc"
};
char D_0083F318[13] = {
#include "lobby/0083F318.inc"
};
char D_0083F328[15] = {
#include "lobby/0083F328.inc"
};
char D_0083F338[0xC] = {
#include "lobby/0083F338.inc"
};
char D_0083F348[0xC] = {
#include "lobby/0083F348.inc"
};
char D_0083F358[0xC] = {
#include "lobby/0083F358.inc"
};
char D_0083F368[0x8] = {
#include "lobby/0083F368.inc"
};
char D_0083F370[0x10] = {
#include "lobby/0083F370.inc"
};
char D_0083F380[0x20] = {
#include "lobby/0083F380.inc"
};
char D_0083F3A0[0x20] = {
#include "lobby/0083F3A0.inc"
};
char D_0083F3C0[0x18] = {
#include "lobby/0083F3C0.inc"
};
char D_0083F3D8[15] = {
#include "lobby/0083F3D8.inc"
};
char D_0083F3E8[0x8] = {
#include "lobby/0083F3E8.inc"
};
char D_0083F3F0[0x8] = {
#include "lobby/0083F3F0.inc"
};
char D_0083F3F8[0x8] = {
#include "lobby/0083F3F8.inc"
};
char D_0083F400[0x8] = {
#include "lobby/0083F400.inc"
};
char D_0083F408[0x8] = {
#include "lobby/0083F408.inc"
};
char D_0083F410[0x20] = {
#include "lobby/0083F410.inc"
};
char D_0083F430[0x20] = {
#include "lobby/0083F430.inc"
};
char D_0083F450[0x20] = {
#include "lobby/0083F450.inc"
};
char D_0083F470[0x10] = {
#include "lobby/0083F470.inc"
};
char D_0083F480[0x20] = {
#include "lobby/0083F480.inc"
};
char D_0083F4A0[0x10] = {
#include "lobby/0083F4A0.inc"
};
char D_0083F4B0[0x10] = {
#include "lobby/0083F4B0.inc"
};
char D_0083F4C0[0x20] = {
#include "lobby/0083F4C0.inc"
};
char D_0083F4E0[0x10] = {
#include "lobby/0083F4E0.inc"
};
char D_0083F4F0[0x20] = {
#include "lobby/0083F4F0.inc"
};
char D_0083F510[0x20] = {
#include "lobby/0083F510.inc"
};
char D_0083F530[0x10] = {
#include "lobby/0083F530.inc"
};
char D_0083F540[0x10] = {
#include "lobby/0083F540.inc"
};
char D_0083F550[0x10] = {
#include "lobby/0083F550.inc"
};
char D_0083F560[0x10] = {
#include "lobby/0083F560.inc"
};
char D_0083F570[0x20] = {
#include "lobby/0083F570.inc"
};
char D_0083F590[0x18] = {
#include "lobby/0083F590.inc"
};
char D_0083F5A8[13] = {
#include "lobby/0083F5A8.inc"
};
char D_0083F5C0[0x20] = {
#include "lobby/0083F5C0.inc"
};
char D_0083F5E0[0x20] = {
#include "lobby/0083F5E0.inc"
};
char D_0083F600[0x18] = {
#include "lobby/0083F600.inc"
};
char D_0083F618[0x8] = {
#include "lobby/0083F618.inc"
};
char D_0083F620[0x30] = {
#include "lobby/0083F620.inc"
};
char D_0083F650[0x20] = {
#include "lobby/0083F650.inc"
};
char D_0083F670[0x30] = {
#include "lobby/0083F670.inc"
};
char D_0083F6A0[0x30] = {
#include "lobby/0083F6A0.inc"
};
char D_0083F6D0[0x30] = {
#include "lobby/0083F6D0.inc"
};
char D_0083F700[0x20] = {
#include "lobby/0083F700.inc"
};
char D_0083F720[0x30] = {
#include "lobby/0083F720.inc"
};
char D_0083F750[0x20] = {
#include "lobby/0083F750.inc"
};
char D_0083F770[0x30] = {
#include "lobby/0083F770.inc"
};
char D_0083F7A0[0x20] = {
#include "lobby/0083F7A0.inc"
};
char D_0083F7C0[0x50] = {
#include "lobby/0083F7C0.inc"
};
char D_0083F810[0x20] = {
#include "lobby/0083F810.inc"
};
char D_0083F830[0x30] = {
#include "lobby/0083F830.inc"
};
char D_0083F860[0x20] = {
#include "lobby/0083F860.inc"
};
char D_0083F880[0x20] = {
#include "lobby/0083F880.inc"
};
char D_0083F8A0[0x18] = {
#include "lobby/0083F8A0.inc"
};
char D_0083F8B8[13] = {
#include "lobby/0083F8B8.inc"
};
char D_0083F8D0[0x20] = {
#include "lobby/0083F8D0.inc"
};
char D_0083F8F0[0x20] = {
#include "lobby/0083F8F0.inc"
};
char D_0083F910[0x30] = {
#include "lobby/0083F910.inc"
};
char D_0083F940[0x30] = {
#include "lobby/0083F940.inc"
};
char D_0083F970[0x30] = {
#include "lobby/0083F970.inc"
};
char D_0083F9A0[0x20] = {
#include "lobby/0083F9A0.inc"
};
char D_0083F9C0[0x20] = {
#include "lobby/0083F9C0.inc"
};
char D_0083F9E0[0x20] = {
#include "lobby/0083F9E0.inc"
};
char D_0083FA00[0x30] = {
#include "lobby/0083FA00.inc"
};
char D_0083FA30[0x48] = {
#include "lobby/0083FA30.inc"
};
char D_0083FA78[13] = {
#include "lobby/0083FA78.inc"
};
char D_0083FA90[0x20] = {
#include "lobby/0083FA90.inc"
};
char D_0083FAB0[0x30] = {
#include "lobby/0083FAB0.inc"
};
char D_0083FAE0[0x30] = {
#include "lobby/0083FAE0.inc"
};
char D_0083FB10[0x30] = {
#include "lobby/0083FB10.inc"
};
char D_0083FB40[0x30] = {
#include "lobby/0083FB40.inc"
};
char D_0083FB70[0x20] = {
#include "lobby/0083FB70.inc"
};
char D_0083FB90[0x20] = {
#include "lobby/0083FB90.inc"
};
char D_0083FBB0[0x30] = {
#include "lobby/0083FBB0.inc"
};
char D_0083FBE0[0x30] = {
#include "lobby/0083FBE0.inc"
};
char D_0083FC10[0x20] = {
#include "lobby/0083FC10.inc"
};
char D_0083FC30[0x40] = {
#include "lobby/0083FC30.inc"
};
char D_0083FC70[0x30] = {
#include "lobby/0083FC70.inc"
};
char D_0083FCA0[0x10] = {
#include "lobby/0083FCA0.inc"
};
char D_0083FCB0[0x20] = {
#include "lobby/0083FCB0.inc"
};
char D_0083FCD0[0x50] = {
#include "lobby/0083FCD0.inc"
};
char D_0083FD20[0x20] = {
#include "lobby/0083FD20.inc"
};
char D_0083FD40[0x20] = {
#include "lobby/0083FD40.inc"
};
char D_0083FD60[0x20] = {
#include "lobby/0083FD60.inc"
};
char D_0083FD80[0x30] = {
#include "lobby/0083FD80.inc"
};
char D_0083FDB0[0x20] = {
#include "lobby/0083FDB0.inc"
};
char D_0083FDD0[0x40] = {
#include "lobby/0083FDD0.inc"
};
char D_0083FE10[0x40] = {
#include "lobby/0083FE10.inc"
};
char D_0083FE50[0x30] = {
#include "lobby/0083FE50.inc"
};
char D_0083FE80[0x30] = {
#include "lobby/0083FE80.inc"
};
char D_0083FEB0[0x20] = {
#include "lobby/0083FEB0.inc"
};
char D_0083FED0[0x40] = {
#include "lobby/0083FED0.inc"
};
char D_0083FF10[0x40] = {
#include "lobby/0083FF10.inc"
};
char D_0083FF50[0x40] = {
#include "lobby/0083FF50.inc"
};
char D_0083FF90[0x30] = {
#include "lobby/0083FF90.inc"
};
char D_0083FFC0[0x40] = {
#include "lobby/0083FFC0.inc"
};
char D_00840000[0x40] = {
#include "lobby/00840000.inc"
};
char D_00840040[0x40] = {
#include "lobby/00840040.inc"
};
char D_00840080[0x30] = {
#include "lobby/00840080.inc"
};
char D_008400B0[0x30] = {
#include "lobby/008400B0.inc"
};
char D_008400E0[0x40] = {
#include "lobby/008400E0.inc"
};
char D_00840120[0x40] = {
#include "lobby/00840120.inc"
};
char D_00840160[0x30] = {
#include "lobby/00840160.inc"
};
char D_00840190[0x18] = {
#include "lobby/00840190.inc"
};
char D_008401A8[0x8] = {
#include "lobby/008401A8.inc"
};
char D_008401B0[0x8] = {
#include "lobby/008401B0.inc"
};
char D_008401B8[0x8] = {
#include "lobby/008401B8.inc"
};
char D_008401C0[0x10] = {
#include "lobby/008401C0.inc"
};
char D_008401D0[0x8] = {
#include "lobby/008401D0.inc"
};
char D_008401D8[0x8] = {
#include "lobby/008401D8.inc"
};
char D_008401E0[0x18] = {
#include "lobby/008401E0.inc"
};
char D_008401F8[0xC] = {
#include "lobby/008401F8.inc"
};
char D_00840208[0xC] = {
#include "lobby/00840208.inc"
};
char D_00840220[0x20] = {
#include "lobby/00840220.inc"
};
char D_00840240[0x18] = {
#include "lobby/00840240.inc"
};
char D_00840258[0xC] = {
#include "lobby/00840258.inc"
};
char D_00840268[0x8] = {
#include "lobby/00840268.inc"
};
char D_00840270[0x10] = {
#include "lobby/00840270.inc"
};
char D_00840280[0x8] = {
#include "lobby/00840280.inc"
};
char D_00840288[0xC] = {
#include "lobby/00840288.inc"
};
char D_00840298[0xC] = {
#include "lobby/00840298.inc"
};
char D_008402A8[0xC] = {
#include "lobby/008402A8.inc"
};
char D_008402B8[0xC] = {
#include "lobby/008402B8.inc"
};
char D_008402C8[13] = {
#include "lobby/008402C8.inc"
};
char D_008402D8[0x8] = {
#include "lobby/008402D8.inc"
};
char D_008402E0[0x18] = {
#include "lobby/008402E0.inc"
};
char D_008402F8[0xC] = {
#include "lobby/008402F8.inc"
};
char D_00840308[13] = {
#include "lobby/00840308.inc"
};
char D_00840318[13] = {
#include "lobby/00840318.inc"
};
char D_00840328[0xC] = {
#include "lobby/00840328.inc"
};
char D_00840338[0xC] = {
#include "lobby/00840338.inc"
};
char D_00840348[13] = {
#include "lobby/00840348.inc"
};
char D_00840360[0x18] = {
#include "lobby/00840360.inc"
};
char D_00840378[15] = {
#include "lobby/00840378.inc"
};
char D_00840390[0x20] = {
#include "lobby/00840390.inc"
};
char D_008403B0[0x20] = {
#include "lobby/008403B0.inc"
};
char D_008403D0[0x20] = {
#include "lobby/008403D0.inc"
};
char D_008403F0[0x20] = {
#include "lobby/008403F0.inc"
};
char D_00840410[0x20] = {
#include "lobby/00840410.inc"
};
char D_00840430[0x20] = {
#include "lobby/00840430.inc"
};
char D_00840450[0x20] = {
#include "lobby/00840450.inc"
};
char D_00840470[0x20] = {
#include "lobby/00840470.inc"
};
char D_00840490[0x20] = {
#include "lobby/00840490.inc"
};
char D_008404B0[0x20] = {
#include "lobby/008404B0.inc"
};
char D_008404D0[0x20] = {
#include "lobby/008404D0.inc"
};
char D_008404F0[0x20] = {
#include "lobby/008404F0.inc"
};
char D_00840510[0x18] = {
#include "lobby/00840510.inc"
};
char D_00840528[0xC] = {
#include "lobby/00840528.inc"
};
char D_00840538[13] = {
#include "lobby/00840538.inc"
};
char D_00840550[0x18] = {
#include "lobby/00840550.inc"
};
char D_00840568[0xC] = {
#include "lobby/00840568.inc"
};
char D_00840578[0xC] = {
#include "lobby/00840578.inc"
};
char D_00840588[0xC] = {
#include "lobby/00840588.inc"
};
char D_008405A0[0x30] = {
#include "lobby/008405A0.inc"
};
char D_008405D0[0x20] = {
#include "lobby/008405D0.inc"
};
char D_008405F0[0x30] = {
#include "lobby/008405F0.inc"
};
char D_00840620[0x18] = {
#include "lobby/00840620.inc"
};
char D_00840638[15] = {
#include "lobby/00840638.inc"
};
char D_00840648[13] = {
#include "lobby/00840648.inc"
};
char D_00840658[15] = {
#include "lobby/00840658.inc"
};
char D_00840668[15] = {
#include "lobby/00840668.inc"
};
char D_00840678[13] = {
#include "lobby/00840678.inc"
};
char D_00840688[13] = {
#include "lobby/00840688.inc"
};
char D_008406A0[0x20] = {
#include "lobby/008406A0.inc"
};
char D_008406C0[0x18] = {
#include "lobby/008406C0.inc"
};
char D_008406D8[15] = {
#include "lobby/008406D8.inc"
};
char D_008406E8[13] = {
#include "lobby/008406E8.inc"
};
char D_008406F8[15] = {
#include "lobby/008406F8.inc"
};
char D_00840708[13] = {
#include "lobby/00840708.inc"
};
char D_00840718[15] = {
#include "lobby/00840718.inc"
};
char D_00840728[13] = {
#include "lobby/00840728.inc"
};
char D_00840740[0x20] = {
#include "lobby/00840740.inc"
};
char D_00840760[0x18] = {
#include "lobby/00840760.inc"
};
char D_00840778[0xC] = {
#include "lobby/00840778.inc"
};
char D_00840788[0xC] = {
#include "lobby/00840788.inc"
};
char D_00840798[0xC] = {
#include "lobby/00840798.inc"
};
char D_008407A8[0xC] = {
#include "lobby/008407A8.inc"
};
char D_008407C0[0x18] = {
#include "lobby/008407C0.inc"
};
char D_008407D8[0xC] = {
#include "lobby/008407D8.inc"
};
char D_008407E8[13] = {
#include "lobby/008407E8.inc"
};
char D_008407F8[15] = {
#include "lobby/008407F8.inc"
};
char D_00840808[15] = {
#include "lobby/00840808.inc"
};
char D_00840818[0xC] = {
#include "lobby/00840818.inc"
};
char D_00840828[0xC] = {
#include "lobby/00840828.inc"
};
char D_00840838[13] = {
#include "lobby/00840838.inc"
};
char D_00840850[0x20] = {
#include "lobby/00840850.inc"
};
char D_00840870[0x18] = {
#include "lobby/00840870.inc"
};
char D_00840888[13] = {
#include "lobby/00840888.inc"
};
char D_00840898[13] = {
#include "lobby/00840898.inc"
};
char D_008408A8[0xC] = {
#include "lobby/008408A8.inc"
};
char D_008408B8[0xC] = {
#include "lobby/008408B8.inc"
};
char D_008408C8[0x8] = {
#include "lobby/008408C8.inc"
};
char D_008408D0[0x10] = {
#include "lobby/008408D0.inc"
};
char D_008408E0[0x10] = {
#include "lobby/008408E0.inc"
};
char D_008408F0[0x10] = {
#include "lobby/008408F0.inc"
};
char D_00840900[0x10] = {
#include "lobby/00840900.inc"
};
char D_00840910[0x10] = {
#include "lobby/00840910.inc"
};
char D_00840920[0x10] = {
#include "lobby/00840920.inc"
};
char D_00840930[0x10] = {
#include "lobby/00840930.inc"
};
char D_00840940[0x10] = {
#include "lobby/00840940.inc"
};
char D_00840950[0x40] = {
#include "lobby/00840950.inc"
};
char D_00840990[0x20] = {
#include "lobby/00840990.inc"
};
char D_008409B0[0x30] = {
#include "lobby/008409B0.inc"
};
char D_008409E0[0x30] = {
#include "lobby/008409E0.inc"
};
char D_00840A10[0x20] = {
#include "lobby/00840A10.inc"
};
char D_00840A30[0x40] = {
#include "lobby/00840A30.inc"
};
char D_00840A70[0x40] = {
#include "lobby/00840A70.inc"
};
char D_00840AB0[0x20] = {
#include "lobby/00840AB0.inc"
};
char D_00840AD0[0x20] = {
#include "lobby/00840AD0.inc"
};
char D_00840AF0[0x20] = {
#include "lobby/00840AF0.inc"
};
char D_00840B10[0x20] = {
#include "lobby/00840B10.inc"
};
char D_00840B30[0x20] = {
#include "lobby/00840B30.inc"
};
char D_00840B50[0x20] = {
#include "lobby/00840B50.inc"
};
char D_00840B70[0x20] = {
#include "lobby/00840B70.inc"
};
char D_00840B90[0x20] = {
#include "lobby/00840B90.inc"
};
char D_00840BB0[0x40] = {
#include "lobby/00840BB0.inc"
};
char D_00840BF0[0x50] = {
#include "lobby/00840BF0.inc"
};
char D_00840C40[0x40] = {
#include "lobby/00840C40.inc"
};
char D_00840C80[0x40] = {
#include "lobby/00840C80.inc"
};
char D_00840CC0[0x30] = {
#include "lobby/00840CC0.inc"
};
char D_00840CF0[0x30] = {
#include "lobby/00840CF0.inc"
};
char D_00840D20[0x30] = {
#include "lobby/00840D20.inc"
};
char D_00840D50[0x60] = {
#include "lobby/00840D50.inc"
};
char D_00840DB0[0x60] = {
#include "lobby/00840DB0.inc"
};
char D_00840E10[0x30] = {
#include "lobby/00840E10.inc"
};
char D_00840E40[0x30] = {
#include "lobby/00840E40.inc"
};
char D_00840E70[0x30] = {
#include "lobby/00840E70.inc"
};
char D_00840EA0[0x20] = {
#include "lobby/00840EA0.inc"
};
char D_00840EC0[0x40] = {
#include "lobby/00840EC0.inc"
};
char D_00840F00[0x60] = {
#include "lobby/00840F00.inc"
};
char D_00840F60[0x30] = {
#include "lobby/00840F60.inc"
};
char D_00840F90[0x40] = {
#include "lobby/00840F90.inc"
};
char D_00840FD0[0x40] = {
#include "lobby/00840FD0.inc"
};
char D_00841010[0x38] = {
#include "lobby/00841010.inc"
};
char D_00841048[13] = {
#include "lobby/00841048.inc"
};
char D_00841058[0xC] = {
#include "lobby/00841058.inc"
};
char D_00841068[0xC] = {
#include "lobby/00841068.inc"
};
char D_00841078[15] = {
#include "lobby/00841078.inc"
};
char D_00841088[0xC] = {
#include "lobby/00841088.inc"
};
char D_00841098[0xC] = {
#include "lobby/00841098.inc"
};
char D_008410B0[0x18] = {
#include "lobby/008410B0.inc"
};
char D_008410C8[15] = {
#include "lobby/008410C8.inc"
};
char D_008410E0[0x20] = {
#include "lobby/008410E0.inc"
};
char D_00841100[0x30] = {
#include "lobby/00841100.inc"
};
char D_00841130[0x30] = {
#include "lobby/00841130.inc"
};
char D_00841160[0x50] = {
#include "lobby/00841160.inc"
};
char D_008411B0[0x90] = {
#include "lobby/008411B0.inc"
};
char D_00841240[0x50] = {
#include "lobby/00841240.inc"
};
char D_00841290[0x68] = {
#include "lobby/00841290.inc"
};
char D_008412F8[13] = {
#include "lobby/008412F8.inc"
};
char D_00841310[0x20] = {
#include "lobby/00841310.inc"
};
char D_00841330[0x10] = {
#include "lobby/00841330.inc"
};
char D_00841340[0x10] = {
#include "lobby/00841340.inc"
};
char D_00841350[0x20] = {
#include "lobby/00841350.inc"
};
char D_00841370[0x20] = {
#include "lobby/00841370.inc"
};
char D_00841390[0x20] = {
#include "lobby/00841390.inc"
};
char D_008413B0[0x20] = {
#include "lobby/008413B0.inc"
};
char D_008413D0[0x20] = {
#include "lobby/008413D0.inc"
};
char D_008413F0[0x20] = {
#include "lobby/008413F0.inc"
};
char D_00841410[0x10] = {
#include "lobby/00841410.inc"
};
char D_00841420[0x10] = {
#include "lobby/00841420.inc"
};
char D_00841430[0x20] = {
#include "lobby/00841430.inc"
};
char D_00841450[0x20] = {
#include "lobby/00841450.inc"
};
char D_00841470[0x20] = {
#include "lobby/00841470.inc"
};
char D_00841490[0x18] = {
#include "lobby/00841490.inc"
};
char D_008414A8[0xC] = {
#include "lobby/008414A8.inc"
};
char D_008414B8[13] = {
#include "lobby/008414B8.inc"
};
char D_008414C8[13] = {
#include "lobby/008414C8.inc"
};
char D_008414D8[0xC] = {
#include "lobby/008414D8.inc"
};
char D_008414F0[0x18] = {
#include "lobby/008414F0.inc"
};
char D_00841508[0x8] = {
#include "lobby/00841508.inc"
};
char D_00841510[0x10] = {
#include "lobby/00841510.inc"
};
char D_00841520[0x10] = {
#include "lobby/00841520.inc"
};
char D_00841530[0x10] = {
#include "lobby/00841530.inc"
};
char D_00841540[0x8] = {
#include "lobby/00841540.inc"
};
char D_00841548[0x8] = {
#include "lobby/00841548.inc"
};
char D_00841550[0x8] = {
#include "lobby/00841550.inc"
};
char D_00841558[0x8] = {
#include "lobby/00841558.inc"
};
char D_00841560[0x20] = {
#include "lobby/00841560.inc"
};
char D_00841580[0x20] = {
#include "lobby/00841580.inc"
};
char D_008415A0[0x20] = {
#include "lobby/008415A0.inc"
};
char D_008415C0[0x20] = {
#include "lobby/008415C0.inc"
};
char D_008415E0[0x20] = {
#include "lobby/008415E0.inc"
};
char D_00841600[0x20] = {
#include "lobby/00841600.inc"
};
char D_00841620[0x20] = {
#include "lobby/00841620.inc"
};
char D_00841640[0x20] = {
#include "lobby/00841640.inc"
};
char D_00841660[0x20] = {
#include "lobby/00841660.inc"
};
char D_00841680[0x20] = {
#include "lobby/00841680.inc"
};
char D_008416A0[0x20] = {
#include "lobby/008416A0.inc"
};
char D_008416C0[0x18] = {
#include "lobby/008416C0.inc"
};
char D_008416D8[14] = {
#include "lobby/008416D8.inc"
};
char D_008416F0[0x20] = {
#include "lobby/008416F0.inc"
};
char D_00841710[0x18] = {
#include "lobby/00841710.inc"
};
char D_00841728[14] = {
#include "lobby/00841728.inc"
};
char D_00841738[0xC] = {
#include "lobby/00841738.inc"
};
char D_00841750[0x20] = {
#include "lobby/00841750.inc"
};
char D_00841770[0x20] = {
#include "lobby/00841770.inc"
};
char D_00841790[0x20] = {
#include "lobby/00841790.inc"
};
char D_008417B0[0x18] = {
#include "lobby/008417B0.inc"
};
char D_008417C8[0xC] = {
#include "lobby/008417C8.inc"
};
char D_008417E0[0x20] = {
#include "lobby/008417E0.inc"
};
char D_00841800[0x20] = {
#include "lobby/00841800.inc"
};
char D_00841820[0x18] = {
#include "lobby/00841820.inc"
};
char D_00841838[14] = {
#include "lobby/00841838.inc"
};
char D_00841850[0x10] = {
#include "lobby/00841850.inc"
};
char D_00841860[0x10] = {
#include "lobby/00841860.inc"
};
char D_00841870[0x18] = {
#include "lobby/00841870.inc"
};
char D_00841888[0x8] = {
#include "lobby/00841888.inc"
};
char D_00841890[0x10] = {
#include "lobby/00841890.inc"
};
char D_008418A0[0x8] = {
#include "lobby/008418A0.inc"
};
char D_008418A8[0x8] = {
#include "lobby/008418A8.inc"
};
char D_008418B0[0x8] = {
#include "lobby/008418B0.inc"
};
char D_008418B8[13] = {
#include "lobby/008418B8.inc"
};
char D_008418C8[13] = {
#include "lobby/008418C8.inc"
};
char D_008418D8[13] = {
#include "lobby/008418D8.inc"
};
char D_008418E8[0xC] = {
#include "lobby/008418E8.inc"
};
char D_008418F8[0xC] = {
#include "lobby/008418F8.inc"
};
char D_00841908[0xC] = {
#include "lobby/00841908.inc"
};
char D_00841918[0x8] = {
#include "lobby/00841918.inc"
};
char D_00841920[0x10] = {
#include "lobby/00841920.inc"
};
char D_00841930[0x10] = {
#include "lobby/00841930.inc"
};
char D_00841940[0x20] = {
#include "lobby/00841940.inc"
};
char D_00841960[0x20] = {
#include "lobby/00841960.inc"
};
char D_00841980[0x20] = {
#include "lobby/00841980.inc"
};
char D_008419A0[0x30] = {
#include "lobby/008419A0.inc"
};
char D_008419D0[0x30] = {
#include "lobby/008419D0.inc"
};
char D_00841A00[0x70] = {
#include "lobby/00841A00.inc"
};
char D_00841A70[0x30] = {
#include "lobby/00841A70.inc"
};
char D_00841AA0[0x30] = {
#include "lobby/00841AA0.inc"
};
char D_00841AD0[0x70] = {
#include "lobby/00841AD0.inc"
};
char D_00841B40[0x30] = {
#include "lobby/00841B40.inc"
};
char D_00841B70[0x30] = {
#include "lobby/00841B70.inc"
};
char D_00841BA0[0x70] = {
#include "lobby/00841BA0.inc"
};
char D_00841C10[0x60] = {
#include "lobby/00841C10.inc"
};
char D_00841C70[0x70] = {
#include "lobby/00841C70.inc"
};
char D_00841CE0[0x40] = {
#include "lobby/00841CE0.inc"
};
char D_00841D20[0x58] = {
#include "lobby/00841D20.inc"
};
char D_00841D78[0xC] = {
#include "lobby/00841D78.inc"
};
char D_00841D88[0xC] = {
#include "lobby/00841D88.inc"
};
char D_00841D98[0xC] = {
#include "lobby/00841D98.inc"
};
char D_00841DA8[0xC] = {
#include "lobby/00841DA8.inc"
};
char D_00841DB8[0xC] = {
#include "lobby/00841DB8.inc"
};
char D_00841DD0[0x18] = {
#include "lobby/00841DD0.inc"
};
char D_00841DE8[0xC] = {
#include "lobby/00841DE8.inc"
};
char D_00841E00[0x18] = {
#include "lobby/00841E00.inc"
};
char D_00841E18[13] = {
#include "lobby/00841E18.inc"
};
char D_00841E30[0x30] = {
#include "lobby/00841E30.inc"
};
char D_00841E60[0x20] = {
#include "lobby/00841E60.inc"
};
char D_00841E80[0x20] = {
#include "lobby/00841E80.inc"
};
char D_00841EA0[0x8] = {
#include "lobby/00841EA0.inc"
};
char D_00841EA8[0x8] = {
#include "lobby/00841EA8.inc"
};
char D_00841EB0[0x10] = {
#include "lobby/00841EB0.inc"
};
char D_00841EC0[0x40] = {
#include "lobby/00841EC0.inc"
};
char D_00841F00[0x60] = {
#include "lobby/00841F00.inc"
};
char D_00841F60[0x50] = {
#include "lobby/00841F60.inc"
};
char D_00841FB0[0x30] = {
#include "lobby/00841FB0.inc"
};
char D_00841FE0[0x10] = {
#include "lobby/00841FE0.inc"
};
char D_00841FF0[0x8] = {
#include "lobby/00841FF0.inc"
};
char D_00841FF8[0x8] = {
#include "lobby/00841FF8.inc"
};
char D_00842000[0x8] = {
#include "lobby/00842000.inc"
};
char D_00842008[0x8] = {
#include "lobby/00842008.inc"
};
char D_00842010[0x8] = {
#include "lobby/00842010.inc"
};
char D_00842018[0x8] = {
#include "lobby/00842018.inc"
};
char D_00842020[0x10] = {
#include "lobby/00842020.inc"
};
char D_00842030[0x10] = {
#include "lobby/00842030.inc"
};
char D_00842040[0x10] = {
#include "lobby/00842040.inc"
};
char D_00842050[0x10] = {
#include "lobby/00842050.inc"
};
char D_00842060[0x10] = {
#include "lobby/00842060.inc"
};
char D_00842070[0x10] = {
#include "lobby/00842070.inc"
};
char D_00842080[0x10] = {
#include "lobby/00842080.inc"
};
char D_00842090[0x10] = {
#include "lobby/00842090.inc"
};
char D_008420A0[0x8] = {
#include "lobby/008420A0.inc"
};
char D_008420A8[13] = {
#include "lobby/008420A8.inc"
};
char D_008420C0[0x20] = {
#include "lobby/008420C0.inc"
};
char D_008420E0[0x20] = {
#include "lobby/008420E0.inc"
};
char D_00842100[0x20] = {
#include "lobby/00842100.inc"
};
char D_00842120[0x20] = {
#include "lobby/00842120.inc"
};
char D_00842140[0x20] = {
#include "lobby/00842140.inc"
};
char D_00842160[0x10] = {
#include "lobby/00842160.inc"
};
char D_00842170[0x20] = {
#include "lobby/00842170.inc"
};
char D_00842190[0x20] = {
#include "lobby/00842190.inc"
};
char D_008421B0[0x20] = {
#include "lobby/008421B0.inc"
};
char D_008421D0[0x20] = {
#include "lobby/008421D0.inc"
};
char D_008421F0[0x20] = {
#include "lobby/008421F0.inc"
};
char D_00842210[0x20] = {
#include "lobby/00842210.inc"
};
char D_00842230[0x30] = {
#include "lobby/00842230.inc"
};
char D_00842260[0x30] = {
#include "lobby/00842260.inc"
};
char D_00842290[0x30] = {
#include "lobby/00842290.inc"
};
char D_008422C0[0x30] = {
#include "lobby/008422C0.inc"
};
char D_008422F0[0x30] = {
#include "lobby/008422F0.inc"
};
char D_00842320[0x20] = {
#include "lobby/00842320.inc"
};
char D_00842340[0x20] = {
#include "lobby/00842340.inc"
};
char D_00842360[0x20] = {
#include "lobby/00842360.inc"
};
char D_00842380[0x20] = {
#include "lobby/00842380.inc"
};
char D_008423A0[0x20] = {
#include "lobby/008423A0.inc"
};
char D_008423C0[0x20] = {
#include "lobby/008423C0.inc"
};
char D_008423E0[0x20] = {
#include "lobby/008423E0.inc"
};
char D_00842400[0x20] = {
#include "lobby/00842400.inc"
};
char D_00842420[0x20] = {
#include "lobby/00842420.inc"
};
char D_00842440[0x30] = {
#include "lobby/00842440.inc"
};
char D_00842470[0x20] = {
#include "lobby/00842470.inc"
};
char D_00842490[0x20] = {
#include "lobby/00842490.inc"
};
char D_008424B0[0x20] = {
#include "lobby/008424B0.inc"
};
char D_008424D0[0x10] = {
#include "lobby/008424D0.inc"
};
char D_008424E0[0x20] = {
#include "lobby/008424E0.inc"
};
char D_00842500[0x30] = {
#include "lobby/00842500.inc"
};
char D_00842530[0x30] = {
#include "lobby/00842530.inc"
};
char D_00842560[0x20] = {
#include "lobby/00842560.inc"
};
char D_00842580[0x30] = {
#include "lobby/00842580.inc"
};
char D_008425B0[0x28] = {
#include "lobby/008425B0.inc"
};
char D_008425D8[13] = {
#include "lobby/008425D8.inc"
};
char D_008425E8[13] = {
#include "lobby/008425E8.inc"
};
char D_00842600[0x20] = {
#include "lobby/00842600.inc"
};
char D_00842620[0x40] = {
#include "lobby/00842620.inc"
};
char D_00842660[0x20] = {
#include "lobby/00842660.inc"
};
char D_00842680[0x20] = {
#include "lobby/00842680.inc"
};
char D_008426A0[0x30] = {
#include "lobby/008426A0.inc"
};
char D_008426D0[0x30] = {
#include "lobby/008426D0.inc"
};
char D_00842700[0x20] = {
#include "lobby/00842700.inc"
};
char D_00842720[0x30] = {
#include "lobby/00842720.inc"
};
char D_00842750[0x20] = {
#include "lobby/00842750.inc"
};
char D_00842770[0x20] = {
#include "lobby/00842770.inc"
};
char D_00842790[0x20] = {
#include "lobby/00842790.inc"
};
char D_008427B0[0x20] = {
#include "lobby/008427B0.inc"
};
char D_008427D0[0x18] = {
#include "lobby/008427D0.inc"
};
char D_008427E8[0xC] = {
#include "lobby/008427E8.inc"
};
char D_008427F8[13] = {
#include "lobby/008427F8.inc"
};
char D_00842810[0x30] = {
#include "lobby/00842810.inc"
};
char D_00842840[0x20] = {
#include "lobby/00842840.inc"
};
char D_00842860[0x20] = {
#include "lobby/00842860.inc"
};
char D_00842880[0x20] = {
#include "lobby/00842880.inc"
};
char D_008428A0[0x10] = {
#include "lobby/008428A0.inc"
};
char D_008428B0[0x18] = {
#include "lobby/008428B0.inc"
};
char D_008428C8[13] = {
#include "lobby/008428C8.inc"
};
char D_008428D8[0xC] = {
#include "lobby/008428D8.inc"
};
char D_008428E8[0xC] = {
#include "lobby/008428E8.inc"
};
char D_00842900[0x20] = {
#include "lobby/00842900.inc"
};
char D_00842920[0x10] = {
#include "lobby/00842920.inc"
};
char D_00842930[0x10] = {
#include "lobby/00842930.inc"
};
char D_00842940[0x18] = {
#include "lobby/00842940.inc"
};
char D_00842958[0xC] = {
#include "lobby/00842958.inc"
};
char D_00842968[0xC] = {
#include "lobby/00842968.inc"
};
char D_00842978[15] = {
#include "lobby/00842978.inc"
};
char D_00842988[0x8] = {
#include "lobby/00842988.inc"
};
char D_00842990[0x10] = {
#include "lobby/00842990.inc"
};
char D_008429A0[0x18] = {
#include "lobby/008429A0.inc"
};
char D_008429B8[0xC] = {
#include "lobby/008429B8.inc"
};
char D_008429C8[0xC] = {
#include "lobby/008429C8.inc"
};
char D_008429D8[15] = {
#include "lobby/008429D8.inc"
};
char D_008429E8[0xC] = {
#include "lobby/008429E8.inc"
};
char D_008429F8[15] = {
#include "lobby/008429F8.inc"
};
char D_00842A10[0x18] = {
#include "lobby/00842A10.inc"
};
char D_00842A28[0xC] = {
#include "lobby/00842A28.inc"
};
char D_00842A38[0x8] = {
#include "lobby/00842A38.inc"
};
char D_00842A40[0x18] = {
#include "lobby/00842A40.inc"
};
char D_00842A58[0xC] = {
#include "lobby/00842A58.inc"
};
char D_00842A70[0x18] = {
#include "lobby/00842A70.inc"
};
char D_00842A88[0xC] = {
#include "lobby/00842A88.inc"
};
char D_00842A98[0xC] = {
#include "lobby/00842A98.inc"
};
char D_00842AB0[0x20] = {
#include "lobby/00842AB0.inc"
};
char D_00842AD0[0x20] = {
#include "lobby/00842AD0.inc"
};
char D_00842AF0[0x8] = {
#include "lobby/00842AF0.inc"
};
char D_00842AF8[0xC] = {
#include "lobby/00842AF8.inc"
};
char D_00842B08[0xC] = {
#include "lobby/00842B08.inc"
};
char D_00842B18[0xC] = {
#include "lobby/00842B18.inc"
};
char D_00842B28[0xC] = {
#include "lobby/00842B28.inc"
};
char D_00842B40[0x20] = {
#include "lobby/00842B40.inc"
};
char D_00842B60[0xA0] = {
#include "lobby/00842B60.inc"
};
char D_00842C00[0x30] = {
#include "lobby/00842C00.inc"
};
char D_00842C30[0x30] = {
#include "lobby/00842C30.inc"
};
char D_00842C60[0x20] = {
#include "lobby/00842C60.inc"
};
char D_00842C80[0x80] = {
#include "lobby/00842C80.inc"
};
char D_00842D00[0x30] = {
#include "lobby/00842D00.inc"
};
char D_00842D30[0x20] = {
#include "lobby/00842D30.inc"
};
char D_00842D50[0x40] = {
#include "lobby/00842D50.inc"
};
char D_00842D90[0x30] = {
#include "lobby/00842D90.inc"
};
char D_00842DC0[0x30] = {
#include "lobby/00842DC0.inc"
};
char D_00842DF0[0x20] = {
#include "lobby/00842DF0.inc"
};
char D_00842E10[0x30] = {
#include "lobby/00842E10.inc"
};
char D_00842E40[0x80] = {
#include "lobby/00842E40.inc"
};
char D_00842EC0[0x40] = {
#include "lobby/00842EC0.inc"
};
char D_00842F00[0x30] = {
#include "lobby/00842F00.inc"
};
char D_00842F30[0x30] = {
#include "lobby/00842F30.inc"
};
char D_00842F60[0x60] = {
#include "lobby/00842F60.inc"
};
char D_00842FC0[0x20] = {
#include "lobby/00842FC0.inc"
};
char D_00842FE0[0xB0] = {
#include "lobby/00842FE0.inc"
};
char D_00843090[0xA0] = {
#include "lobby/00843090.inc"
};
char D_00843130[0x90] = {
#include "lobby/00843130.inc"
};
char D_008431C0[0xA0] = {
#include "lobby/008431C0.inc"
};
char D_00843260[0x30] = {
#include "lobby/00843260.inc"
};
char D_00843290[0x40] = {
#include "lobby/00843290.inc"
};
char D_008432D0[0x20] = {
#include "lobby/008432D0.inc"
};
char D_008432F0[0x50] = {
#include "lobby/008432F0.inc"
};
char D_00843340[0xA0] = {
#include "lobby/00843340.inc"
};
char D_008433E0[0xA0] = {
#include "lobby/008433E0.inc"
};
char D_00843480[0x30] = {
#include "lobby/00843480.inc"
};
char D_008434B0[0x40] = {
#include "lobby/008434B0.inc"
};
char D_008434F0[0x30] = {
#include "lobby/008434F0.inc"
};
char D_00843520[0x90] = {
#include "lobby/00843520.inc"
};
char D_008435B0[0xB0] = {
#include "lobby/008435B0.inc"
};
char D_00843660[0x90] = {
#include "lobby/00843660.inc"
};
char D_008436F0[0x40] = {
#include "lobby/008436F0.inc"
};
char D_00843730[0x40] = {
#include "lobby/00843730.inc"
};
char D_00843770[0x40] = {
#include "lobby/00843770.inc"
};
char D_008437B0[0x50] = {
#include "lobby/008437B0.inc"
};
char D_00843800[0x40] = {
#include "lobby/00843800.inc"
};
char D_00843840[0x40] = {
#include "lobby/00843840.inc"
};
char D_00843880[0x40] = {
#include "lobby/00843880.inc"
};
char D_008438C0[0x30] = {
#include "lobby/008438C0.inc"
};
char D_008438F0[0x90] = {
#include "lobby/008438F0.inc"
};
char D_00843980[0x8] = {
#include "lobby/00843980.inc"
};
char D_00843988[0xC] = {
#include "lobby/00843988.inc"
};
char D_00843998[0xC] = {
#include "lobby/00843998.inc"
};
char D_008439A8[0x8] = {
#include "lobby/008439A8.inc"
};
char D_008439B0[0x8] = {
#include "lobby/008439B0.inc"
};
char D_008439B8[0x8] = {
#include "lobby/008439B8.inc"
};
char D_008439C0[0x8] = {
#include "lobby/008439C0.inc"
};
char D_008439C8[0xC] = {
#include "lobby/008439C8.inc"
};
char D_008439D8[0x8] = {
#include "lobby/008439D8.inc"
};
char D_008439E0[0x8] = {
#include "lobby/008439E0.inc"
};
char D_008439E8[0x8] = {
#include "lobby/008439E8.inc"
};
char D_008439F0[0x8] = {
#include "lobby/008439F0.inc"
};
char D_008439F8[0x8] = {
#include "lobby/008439F8.inc"
};
char D_00843A00[0x8] = {
#include "lobby/00843A00.inc"
};
char D_00843A08[0xC] = {
#include "lobby/00843A08.inc"
};
char D_00843A18[0x8] = {
#include "lobby/00843A18.inc"
};
char D_00843A20[0x8] = {
#include "lobby/00843A20.inc"
};
char D_00843A28[13] = {
#include "lobby/00843A28.inc"
};
char D_00843A40[0x20] = {
#include "lobby/00843A40.inc"
};
char D_00843A60[0x18] = {
#include "lobby/00843A60.inc"
};
char D_00843A78[0xC] = {
#include "lobby/00843A78.inc"
};
char D_00843A88[0xC] = {
#include "lobby/00843A88.inc"
};
char D_00843AA0[0x18] = {
#include "lobby/00843AA0.inc"
};
char D_00843AB8[0xC] = {
#include "lobby/00843AB8.inc"
};
char D_00843AC8[13] = {
#include "lobby/00843AC8.inc"
};
char D_00843AE0[0x18] = {
#include "lobby/00843AE0.inc"
};
char D_00843AF8[0x8] = {
#include "lobby/00843AF8.inc"
};
char D_00843B00[0x10] = {
#include "lobby/00843B00.inc"
};
char D_00843B10[0x10] = {
#include "lobby/00843B10.inc"
};
char D_00843B20[0x10] = {
#include "lobby/00843B20.inc"
};
char D_00843B30[0x10] = {
#include "lobby/00843B30.inc"
};
char D_00843B40[0x20] = {
#include "lobby/00843B40.inc"
};
char D_00843B60[0x20] = {
#include "lobby/00843B60.inc"
};
char D_00843B80[0x20] = {
#include "lobby/00843B80.inc"
};
char D_00843BA0[0x20] = {
#include "lobby/00843BA0.inc"
};
char D_00843BC0[0x20] = {
#include "lobby/00843BC0.inc"
};
char D_00843BE0[0x20] = {
#include "lobby/00843BE0.inc"
};
char D_00843C00[0x20] = {
#include "lobby/00843C00.inc"
};
char D_00843C20[0x20] = {
#include "lobby/00843C20.inc"
};
char D_00843C40[0x8] = {
#include "lobby/00843C40.inc"
};
char D_00843C48[15] = {
#include "lobby/00843C48.inc"
};
char D_00843C58[13] = {
#include "lobby/00843C58.inc"
};
char D_00843C68[15] = {
#include "lobby/00843C68.inc"
};
char D_00843C80[0x20] = {
#include "lobby/00843C80.inc"
};
char D_00843CA0[0x20] = {
#include "lobby/00843CA0.inc"
};
char D_00843CC0[0x20] = {
#include "lobby/00843CC0.inc"
};
char D_00843CE0[0x30] = {
#include "lobby/00843CE0.inc"
};
char D_00843D10[0x20] = {
#include "lobby/00843D10.inc"
};
char D_00843D30[0x10] = {
#include "lobby/00843D30.inc"
};
char D_00843D40[0x10] = {
#include "lobby/00843D40.inc"
};
char D_00843D50[0x10] = {
#include "lobby/00843D50.inc"
};
char D_00843D60[0x10] = {
#include "lobby/00843D60.inc"
};
char D_00843D70[0x18] = {
#include "lobby/00843D70.inc"
};
char D_00843D88[13] = {
#include "lobby/00843D88.inc"
};
char D_00843D98[15] = {
#include "lobby/00843D98.inc"
};
char D_00843DB0[0x20] = {
#include "lobby/00843DB0.inc"
};
char D_00843DD0[0x18] = {
#include "lobby/00843DD0.inc"
};
char D_00843DE8[0x8] = {
#include "lobby/00843DE8.inc"
};
char D_00843DF0[0x8] = {
#include "lobby/00843DF0.inc"
};
char D_00843DF8[0xC] = {
#include "lobby/00843DF8.inc"
};
char D_00843E10[0x18] = {
#include "lobby/00843E10.inc"
};
char D_00843E28[0x8] = {
#include "lobby/00843E28.inc"
};
char D_00843E30[0x10] = {
#include "lobby/00843E30.inc"
};
char D_00843E40[0x10] = {
#include "lobby/00843E40.inc"
};
char D_00843E50[0x10] = {
#include "lobby/00843E50.inc"
};
char D_00843E60[0x10] = {
#include "lobby/00843E60.inc"
};
char D_00843E70[0x10] = {
#include "lobby/00843E70.inc"
};
char D_00843E80[0x10] = {
#include "lobby/00843E80.inc"
};
char D_00843E90[0x20] = {
#include "lobby/00843E90.inc"
};
char D_00843EB0[0x20] = {
#include "lobby/00843EB0.inc"
};
char D_00843ED0[0x10] = {
#include "lobby/00843ED0.inc"
};
char D_00843EE0[0x10] = {
#include "lobby/00843EE0.inc"
};
char D_00843EF0[0x8] = {
#include "lobby/00843EF0.inc"
};
char D_00843EF8[0xC] = {
#include "lobby/00843EF8.inc"
};
char D_00843F08[0xC] = {
#include "lobby/00843F08.inc"
};
char D_00843F18[15] = {
#include "lobby/00843F18.inc"
};
char D_00843F28[0xC] = {
#include "lobby/00843F28.inc"
};
char D_00843F38[0xC] = {
#include "lobby/00843F38.inc"
};
char D_00843F48[15] = {
#include "lobby/00843F48.inc"
};
char D_00843F58[15] = {
#include "lobby/00843F58.inc"
};
char D_00843F68[0x8] = {
#include "lobby/00843F68.inc"
};
char D_00843F70[0x18] = {
#include "lobby/00843F70.inc"
};
char D_00843F88[0x8] = {
#include "lobby/00843F88.inc"
};
char D_00843F90[0x10] = {
#include "lobby/00843F90.inc"
};
char D_00843FA0[0x10] = {
#include "lobby/00843FA0.inc"
};
char D_00843FB0[0x10] = {
#include "lobby/00843FB0.inc"
};
char D_00843FC0[0x10] = {
#include "lobby/00843FC0.inc"
};
char D_00843FD0[0x10] = {
#include "lobby/00843FD0.inc"
};
char D_00843FE0[0x20] = {
#include "lobby/00843FE0.inc"
};
char D_00844000[0x20] = {
#include "lobby/00844000.inc"
};
char D_00844020[0x20] = {
#include "lobby/00844020.inc"
};
char D_00844040[0x40] = {
#include "lobby/00844040.inc"
};
char D_00844080[0x40] = {
#include "lobby/00844080.inc"
};
char D_008440C0[0x30] = {
#include "lobby/008440C0.inc"
};
char D_008440F0[0x30] = {
#include "lobby/008440F0.inc"
};
char D_00844120[0x40] = {
#include "lobby/00844120.inc"
};
char D_00844160[0x60] = {
#include "lobby/00844160.inc"
};
char D_008441C0[0x20] = {
#include "lobby/008441C0.inc"
};
char D_008441E0[0x40] = {
#include "lobby/008441E0.inc"
};
char D_00844220[0x40] = {
#include "lobby/00844220.inc"
};
char D_00844260[0x50] = {
#include "lobby/00844260.inc"
};
char D_008442B0[0x40] = {
#include "lobby/008442B0.inc"
};
char D_008442F0[0x30] = {
#include "lobby/008442F0.inc"
};
char D_00844320[0x20] = {
#include "lobby/00844320.inc"
};
char D_00844340[0x40] = {
#include "lobby/00844340.inc"
};
char D_00844380[0x40] = {
#include "lobby/00844380.inc"
};
char D_008443C0[0x20] = {
#include "lobby/008443C0.inc"
};
char D_008443E0[0x50] = {
#include "lobby/008443E0.inc"
};
char D_00844430[0x30] = {
#include "lobby/00844430.inc"
};
char D_00844460[0x30] = {
#include "lobby/00844460.inc"
};
char D_00844490[0x30] = {
#include "lobby/00844490.inc"
};
char D_008444C0[0x30] = {
#include "lobby/008444C0.inc"
};
char D_008444F0[0x30] = {
#include "lobby/008444F0.inc"
};
char D_00844520[0x30] = {
#include "lobby/00844520.inc"
};
char D_00844550[0x20] = {
#include "lobby/00844550.inc"
};
char D_00844570[0x60] = {
#include "lobby/00844570.inc"
};
char D_008445D0[0x50] = {
#include "lobby/008445D0.inc"
};
char D_00844620[0x30] = {
#include "lobby/00844620.inc"
};
char D_00844650[0x20] = {
#include "lobby/00844650.inc"
};
char D_00844670[0x20] = {
#include "lobby/00844670.inc"
};
char D_00844690[0x30] = {
#include "lobby/00844690.inc"
};
char D_008446C0[0x30] = {
#include "lobby/008446C0.inc"
};
char D_008446F0[0x40] = {
#include "lobby/008446F0.inc"
};
char D_00844730[0x30] = {
#include "lobby/00844730.inc"
};
char D_00844760[0x30] = {
#include "lobby/00844760.inc"
};
char D_00844790[0x30] = {
#include "lobby/00844790.inc"
};
char D_008447C0[0x30] = {
#include "lobby/008447C0.inc"
};
char D_008447F0[0x30] = {
#include "lobby/008447F0.inc"
};
char D_00844820[0x40] = {
#include "lobby/00844820.inc"
};
char D_00844860[0x40] = {
#include "lobby/00844860.inc"
};
char D_008448A0[0x70] = {
#include "lobby/008448A0.inc"
};
char D_00844910[0x40] = {
#include "lobby/00844910.inc"
};
char D_00844950[0x30] = {
#include "lobby/00844950.inc"
};
char D_00844980[0x60] = {
#include "lobby/00844980.inc"
};
char D_008449E0[0x30] = {
#include "lobby/008449E0.inc"
};
char D_00844A10[0x30] = {
#include "lobby/00844A10.inc"
};
char D_00844A40[0x30] = {
#include "lobby/00844A40.inc"
};
char D_00844A70[0x30] = {
#include "lobby/00844A70.inc"
};
char D_00844AA0[0x40] = {
#include "lobby/00844AA0.inc"
};
char D_00844AE0[0x70] = {
#include "lobby/00844AE0.inc"
};
char D_00844B50[0x50] = {
#include "lobby/00844B50.inc"
};
char D_00844BA0[0x30] = {
#include "lobby/00844BA0.inc"
};
char D_00844BD0[0x20] = {
#include "lobby/00844BD0.inc"
};
char D_00844BF0[0x40] = {
#include "lobby/00844BF0.inc"
};
char D_00844C30[0x20] = {
#include "lobby/00844C30.inc"
};
char D_00844C50[0x50] = {
#include "lobby/00844C50.inc"
};
char D_00844CA0[0x60] = {
#include "lobby/00844CA0.inc"
};
char D_00844D00[0x60] = {
#include "lobby/00844D00.inc"
};
char D_00844D60[0x30] = {
#include "lobby/00844D60.inc"
};
char D_00844D90[0x40] = {
#include "lobby/00844D90.inc"
};
char D_00844DD0[0x40] = {
#include "lobby/00844DD0.inc"
};
char D_00844E10[0x20] = {
#include "lobby/00844E10.inc"
};
char D_00844E30[0x20] = {
#include "lobby/00844E30.inc"
};
char D_00844E50[0x20] = {
#include "lobby/00844E50.inc"
};
char D_00844E70[0x30] = {
#include "lobby/00844E70.inc"
};
char D_00844EA0[0x20] = {
#include "lobby/00844EA0.inc"
};
char D_00844EC0[0x20] = {
#include "lobby/00844EC0.inc"
};
char D_00844EE0[0x20] = {
#include "lobby/00844EE0.inc"
};
char D_00844F00[0x30] = {
#include "lobby/00844F00.inc"
};
char D_00844F30[0x30] = {
#include "lobby/00844F30.inc"
};
char D_00844F60[0x20] = {
#include "lobby/00844F60.inc"
};
char D_00844F80[0x50] = {
#include "lobby/00844F80.inc"
};
char D_00844FD0[0x60] = {
#include "lobby/00844FD0.inc"
};
char D_00845030[0x60] = {
#include "lobby/00845030.inc"
};
char D_00845090[0x60] = {
#include "lobby/00845090.inc"
};
char D_008450F0[0x30] = {
#include "lobby/008450F0.inc"
};
char D_00845120[0x60] = {
#include "lobby/00845120.inc"
};
char D_00845180[0x60] = {
#include "lobby/00845180.inc"
};
char D_008451E0[0x60] = {
#include "lobby/008451E0.inc"
};
char D_00845240[0x30] = {
#include "lobby/00845240.inc"
};
char D_00845270[0x30] = {
#include "lobby/00845270.inc"
};
char D_008452A0[0x30] = {
#include "lobby/008452A0.inc"
};
char D_008452D0[0x40] = {
#include "lobby/008452D0.inc"
};
char D_00845310[0x60] = {
#include "lobby/00845310.inc"
};
char D_00845370[0x30] = {
#include "lobby/00845370.inc"
};
char D_008453A0[0x30] = {
#include "lobby/008453A0.inc"
};
char D_008453D0[0x30] = {
#include "lobby/008453D0.inc"
};
char D_00845400[0x60] = {
#include "lobby/00845400.inc"
};
char D_00845460[0x50] = {
#include "lobby/00845460.inc"
};
char D_008454B0[0x30] = {
#include "lobby/008454B0.inc"
};
char D_008454E0[0x30] = {
#include "lobby/008454E0.inc"
};
char D_00845510[0x30] = {
#include "lobby/00845510.inc"
};
char D_00845540[0x60] = {
#include "lobby/00845540.inc"
};
char D_008455A0[0x110] = {
#include "lobby/008455A0.inc"
};
char D_008456B0[0x90] = {
#include "lobby/008456B0.inc"
};
char D_00845740[0x30] = {
#include "lobby/00845740.inc"
};
char D_00845770[0x30] = {
#include "lobby/00845770.inc"
};
char D_008457A0[0x50] = {
#include "lobby/008457A0.inc"
};
char D_008457F0[0x50] = {
#include "lobby/008457F0.inc"
};
char D_00845840[0x30] = {
#include "lobby/00845840.inc"
};
char D_00845870[0x40] = {
#include "lobby/00845870.inc"
};
char D_008458B0[0x30] = {
#include "lobby/008458B0.inc"
};
char D_008458E0[0x30] = {
#include "lobby/008458E0.inc"
};
char D_00845910[0x20] = {
#include "lobby/00845910.inc"
};
char D_00845930[0x50] = {
#include "lobby/00845930.inc"
};
char D_00845980[0x30] = {
#include "lobby/00845980.inc"
};
char D_008459B0[0x30] = {
#include "lobby/008459B0.inc"
};
char D_008459E0[0x90] = {
#include "lobby/008459E0.inc"
};
char D_00845A70[0xA0] = {
#include "lobby/00845A70.inc"
};
char D_00845B10[0x60] = {
#include "lobby/00845B10.inc"
};
char D_00845B70[0xC0] = {
#include "lobby/00845B70.inc"
};
char D_00845C30[0x90] = {
#include "lobby/00845C30.inc"
};
char D_00845CC0[0xD0] = {
#include "lobby/00845CC0.inc"
};
char D_00845D90[0x100] = {
#include "lobby/00845D90.inc"
};
char D_00845E90[0x20] = {
#include "lobby/00845E90.inc"
};
char D_00845EB0[0x20] = {
#include "lobby/00845EB0.inc"
};
char D_00845ED0[0x30] = {
#include "lobby/00845ED0.inc"
};
char D_00845F00[0x20] = {
#include "lobby/00845F00.inc"
};
char D_00845F20[0x20] = {
#include "lobby/00845F20.inc"
};
char D_00845F40[0x30] = {
#include "lobby/00845F40.inc"
};
char D_00845F70[0x60] = {
#include "lobby/00845F70.inc"
};
char D_00845FD0[0x60] = {
#include "lobby/00845FD0.inc"
};
char D_00846030[0x60] = {
#include "lobby/00846030.inc"
};
char D_00846090[0x60] = {
#include "lobby/00846090.inc"
};
char D_008460F0[0x80] = {
#include "lobby/008460F0.inc"
};
char D_00846170[0x30] = {
#include "lobby/00846170.inc"
};
char D_008461A0[0x50] = {
#include "lobby/008461A0.inc"
};
char D_008461F0[0x60] = {
#include "lobby/008461F0.inc"
};
char D_00846250[0x30] = {
#include "lobby/00846250.inc"
};
char D_00846280[0x50] = {
#include "lobby/00846280.inc"
};
char D_008462D0[0x40] = {
#include "lobby/008462D0.inc"
};
char D_00846310[0x40] = {
#include "lobby/00846310.inc"
};
char D_00846350[0x20] = {
#include "lobby/00846350.inc"
};
char D_00846370[0x30] = {
#include "lobby/00846370.inc"
};
char D_008463A0[0x60] = {
#include "lobby/008463A0.inc"
};
char D_00846400[0x30] = {
#include "lobby/00846400.inc"
};
char D_00846430[0x50] = {
#include "lobby/00846430.inc"
};
char D_00846480[0x30] = {
#include "lobby/00846480.inc"
};
char D_008464B0[0x40] = {
#include "lobby/008464B0.inc"
};
char D_008464F0[0xB0] = {
#include "lobby/008464F0.inc"
};
char D_008465A0[0x80] = {
#include "lobby/008465A0.inc"
};
char D_00846620[0x30] = {
#include "lobby/00846620.inc"
};
char D_00846650[0x50] = {
#include "lobby/00846650.inc"
};
char D_008466A0[0x30] = {
#include "lobby/008466A0.inc"
};
#pragma rodata off
