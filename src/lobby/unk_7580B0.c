#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "lobby_state.h"
#include "player.h"
#include "capsule.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "village_cond.h"

typedef struct EquipSell {
    /* 0x00 */ u8 unk0[0x1A];
    /* 0x1A */ s16 row;
    /* 0x1C */ s16 col;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ char *msg;
    /* 0x24 */ u8 unk24[2];
    /* 0x26 */ s16 box;
    /* 0x28 */ s16 page;
    /* 0x2A */ s16 pages;
    /* 0x2C */ u8 unk2C[0xB4];
    /* 0xE0 */ s32 price;
} EquipSell;
typedef struct BoxPages {
    /* 0x00 */ u8 unk0[0x1E];
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ s16 pages;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ s16 page;
} BoxPages;

extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
extern char *gYesNoTexts[];
void EquipShop_DrawTitle(void);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(const char *, ...);
void EquipCompare_Draw(EquipItem *a, EquipItem *b, s32 c, s16 y, u8 flags);
void EquipShop_DrawPage(s16 x, s16 y);
void EquipShop_DrawList(void);
void EquipShop_DrawPreview(void);
void EquipShop_DrawPageDetail(void);
void EquipWindow_DrawLocal(struct EquipItem *, s16 x, s16 y, u8 flags, struct EquipItem *);
extern s8 gMenuInputLock;
void Menu_SetYesNo(s8 a);
s32 Equip_GetSellPrice(EquipItem *item);
char *Text_FillDollar(char *arg0, s32 arg1);
void EquipShop_DrawPageSmall(void);
void EquipShop_DrawNote(void);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawLabelBar(s16 x, s16 y, s16 c, u32 color, char *label, u8 t1, u8 t2);
extern u8 gEquipPreview[];
s16 Box_GetPageCount(SaveData *save);
s16 Box_GetCapacity(SaveData *save);
extern s8 gLobbyTextDirty;
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
void Save_AddMoney(s32 n);
void LobbySe_Play(s32 se);
struct Player;
void Player_SetColorWord(struct Player *pl, u8 idx);
void Menu_PushNotice(s32 msg);
s32 Equip_EquipSlot(SaveData *save, u16 idx);
void Player_SetupFromSave(struct Player *player, struct SaveData *save);
void Player_CalcSkills(struct Player *player);
void Town_ReloadLocalPlayer(void);
s32 Equip_CanWear(struct SaveData *save, struct EquipItem *e);
s32 Menu_YesNoInput(void);
s32 Box_ClearSlot(SaveData *save, u16 idx);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 EquipSet_RemoveSlot(u16 slot, s32 a);
void EquipDetail_Draw(Player *, s16, s16, u32);
void *memset(void *p, s32 c, u32 n);
u8 Equip_GetWearMask(struct SaveData *save, struct EquipItem *e);
s32 Equip_TestFlag(struct EquipItem *e, s16 bit);
void Equip_SetFlag(struct EquipItem *e, s16 bit);
s32 Game_IsOnline(void);
s32 Cond_Check(u16 id);
void EquipShop_LoadCompare(u8 type);
void EquipShop_BuildPage(void);
s32 Menu_CountPages(s16 n, s16 per);
u16 Box_FindFreeSlot(void *unused0);
void Text_SetSize(s32 size);
void EquipShop_DrawMenu(void);
void EquipBox_DrawSellNote(s16 x, s16 y);
void EquipBox_DrawSellPrice(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void BoxMenu_DrawItemList(s16 slot, u32 flags, f32 x);
u32 Equip_GetPrice(u8 type, u16 id);
s32 Forge_IsRandomWeapon(s16 id);
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, EquipItem *e) (lobby/name_tags.c), EquipShop_DrawLine 100 of 112 words, EquipShop_DrawPage 6 of 176 words, EquipShop_DrawPageSmall 98 of 162 words differ */
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
u8 Equip_GetRarity(u8 type, u16 id);
char *Equip_GetName(u8 type, u16 id);

u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
void EquipPreview_Hide(u8 hide);
s32 EquipShop_InputTop(u16 pad);
s32 WinSlide_Step(WinSlide *w, s16 from, s16 to);
void EquipPreview_Reset(void);
void EquipShop_OpenList(void);
void Chat_SetColor(s16 color);
void EquipBox_FirstPage(void);
struct Em;
void OfferMenu_Open(struct Em *t);
void CamFocus_SetAside(s8 on);
void Talk_Say(s32 talk, s8 c);
s32 EquipShop_InputList(u16 pad);
void EquipShop_TryOn(void);
s32 EquipShop_InputBuy(u16 pad);
s32 EquipShop_InputSell(u16 pad);
s32 EquipBox_SellMenu(u16 pad);
s32 Talk_Update(void);
void EquipShop_Close(void);

/* The equipment shop's frame: its states (+0xC) around the menus (0 the
 * choice, 1 opening, 2 the list, 3 buying, 4 the box, 5 selling, 6 the
 * farewell talk, 7 closing). */
void EquipShop_Update(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(2, 2);
    EquipPreview_Hide(1);
    switch (s->state) {
    case 0:
        switch (EquipShop_InputTop(pad)) {
        case 0:
            s->cursor = 0;
            s->page = 0;
            s->state++;
            WinSlide_Step(&s->win, 0x280, 0x118);
            EquipPreview_Reset();
            s->selling = 1;
            if (s->unk10 == 0) {
                EquipShop_OpenList();
                Chat_SetColor(0);
            } else if (s->unk10 == 1) {
                EquipBox_FirstPage();
            } else {
                OfferMenu_Open((struct Em *) s->npc);
                break;
            }
            CamFocus_SetAside(1);
            break;
        case 3:
            Talk_Say(1, 0);
            s->state = 6;
            break;
        }
        break;
    case 1:
        switch (WinSlide_Step(&s->win, 0x280, 0x118)) {
        case 0:
            if (s->unk10 == 0) {
                s->state = 2;
            } else {
                s->state = 4;
            }
            break;
        }
        break;
    case 2:
        switch (EquipShop_InputList(pad)) {
        case 0:
            s->state++;
            s->view = 2;
            EquipShop_TryOn();
            Chat_SetColor(-1);
            break;
        case 3:
            s->cursor = s->kind;
            if (s->unk10 == 0) {
                s->cursor = s->kind;
                s->canWear = 2;
            } else {
                s->cursor = s->unk10;
            }
            s->state = 0;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case 3:
        switch (EquipShop_InputBuy(pad)) {
        case 0:
            s->state = 2;
            Chat_SetColor(0);
            EquipShop_OpenList();
            break;
        case 3:
            s->state = 2;
            Chat_SetColor(0);
            break;
        }
        break;
    case 4:
        switch (EquipShop_InputSell(pad)) {
        case 0:
            s->state++;
            break;
        case 3:
            s->cursor = s->unk10;
            s->state = 0;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 5:
        switch (EquipBox_SellMenu(pad)) {
        case 0:
            s->state = 4;
            EquipBox_FirstPage();
            break;
        case 3:
            s->state = 4;
            s->detailPages = 0;
            break;
        }
        break;
    case 6:
        switch (Talk_Update()) {
        case 0:
            s->state++;
            break;
        }
        break;
    case 7:
        EquipShop_Close();
        break;
    }
}

s32 Menu_WrapUpDownSe(s32 arg0, s32 arg1, s32 arg2);

/* The equipment shop's three selection steps (+0xD): the first choice
 * (+0x10; 1 and 2 leave with 0), the tab (+0x11), the kind (+0x12); pad
 * 0x20 confirms, 0x40 goes back (3 from the first step); else 2. */
s32 EquipShop_InputTop(u16 pad)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    switch (s->canWear) {
    case 0:
        if (pad & 0x20) {
            s->unk10 = s->cursor;
            s->cursor = 0;
            LobbySe_Play(0x1B);
            if (s->unk10 == 2) {
                return 0;
            }
            if (s->unk10 == 1) {
                return 0;
            }
            s->canWear++;
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else if (s->hasOffers == 0) {
            s->cursor = Menu_WrapUpDownSe(s->cursor, 2, 2);
        } else {
            s->cursor = Menu_WrapUpDownSe(s->cursor, 3, 2);
        }
        break;
    case 1:
        if (pad & 0x20) {
            s->tab = s->cursor;
            s->cursor = 0;
            s->canWear++;
            LobbySe_Play(0x1B);
        } else if (pad & 0x40) {
            s->cursor = s->unk10;
            s->canWear--;
            LobbySe_Play(4);
        } else {
            s->cursor = Menu_WrapUpDownSe(s->cursor, 2, 2);
        }
        break;
    case 2:
        if (pad & 0x20) {
            s->kind = s->cursor;
            s->cursor = 0;
            s->canWear = 0;
            LobbySe_Play(0x1C);
            return 0;
        } else if (pad & 0x40) {
            s->cursor = s->tab;
            s->canWear--;
            LobbySe_Play(4);
        } else {
            s->cursor = Menu_WrapUpDownSe(s->cursor, 5, 2);
        }
        break;
    }
    return 2;
}

s32 EquipShop_IsListed(u8 type, s32 id)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    EquipItem e;

    if (id == 0) {
        return 0;
    }
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.id = id;
    if (!((1 << pl->gender) & Equip_GetWearMask(&gSave, &e))) {
        return 0;
    }
    if (Equip_TestFlag(&e, 1) == 0) {
        Equip_SetFlag(&e, 2);
        Equip_SetFlag(&e, 1);
    }
    return 1;
}

extern s32 **gEquipShopLists[];

s32 *EquipShop_GetList(s32 i)
{
    s32 rank;

    if (Game_IsOnline() == 1) {
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1) {
            rank = 3;
        } else if (Cond_Check(COND_HR_GATE_1_HR_17) == 1) {
            rank = 2;
        } else if (Cond_Check(COND_HR_GATE_0_HR_11) == 1) {
            rank = 1;
        } else {
            rank = 0;
        }
    } else {
        if (Cond_Check(0x3E) == 1) {
            rank = 2;
        } else if (Cond_Check(0x37) == 1) {
            rank = 1;
        } else {
            rank = 0;
        }
    }
    return gEquipShopLists[rank][i];
}

extern u8 gEquipShopTypesTab0[];
extern u8 gEquipShopTypesTab1[];

void EquipShop_BuildPage(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    s32 *list = EquipShop_GetList(s->kind + s->tab * 5);
    s32 k;
    u8 type;
    u8 n;

    if (s->tab == 0) {
        type = gEquipShopTypesTab0[s->kind];
    } else {
        type = gEquipShopTypesTab1[s->kind];
    }
    memset(s->line, 0, sizeof(s->line));
    k = n = 0;
    for (; *list != 0; list++) {
        if (EquipShop_IsListed(type, *list) == 1) {
            k++;
            if (s->page * 7 < k) {
                s->line[n].item.type = type;
                s->line[n].item.id = *list;
                s->line[n].item.used = 1;
                s->line[n].price = Equip_GetPrice(type, *list);
                if (s->line[n].price <= (u32)gSave.money && (u16)Box_FindFreeSlot(&gSave) != 0xFFFF) {
                    s->line[n].noBuy = 0;
                } else {
                    s->line[n].noBuy = 1;
                }
                n++;
                if (n >= 7) {
                    break;
                }
            }
        }
    }
    s->lines = n;
    if (s->cursor >= s->lines) {
        s->cursor = s->lines - 1;
    }
}

/* The equipped item of the type (the save's equip[]: the weapon for types
 * 6 and 7, then the armour of types 0, 2-5) copied to +0xBC for the
 * comparison; nothing for type 1. */
void EquipShop_LoadCompare(u8 type)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    memset(&s->worn, 0, sizeof(EquipItem));
    s->worn.type = type;
    switch (type) {
    case 6:
    case 7:
        s->worn = gSave.equip[0];
        break;
    case 2:
        s->worn = gSave.equip[2];
        break;
    case 3:
        s->worn = gSave.equip[3];
        break;
    case 4:
        s->worn = gSave.equip[4];
        break;
    case 5:
        s->worn = gSave.equip[5];
        break;
    case 0:
        s->worn = gSave.equip[1];
        break;
    }
    s->worn.used = 1;
}

extern u8 gEquipShopTypesTab0[];
extern u8 gEquipShopTypesTab1[];

void EquipShop_OpenList(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    s32 *list = EquipShop_GetList(s->kind + s->tab * 5);
    u8 type;

    if (s->tab == 0) {
        type = gEquipShopTypesTab0[s->kind];
    } else {
        type = gEquipShopTypesTab1[s->kind];
    }
    s->detailPage = 0;
    s->view = 0;
    EquipShop_LoadCompare(type);
    s->count = 0;
    for (; *list != 0; list++) {
        if (EquipShop_IsListed(type, *list) == 1) {
            s->count++;
        }
    }
    s->pages = Menu_CountPages(s->count, 7);
    EquipShop_BuildPage();
    if ((u16)Box_FindFreeSlot(&gSave) == 0xFFFF) {
        Menu_PushNotice(0xB);
    }
}

u8 Equip_PreviewSkills(Player *player, struct EquipItem *l);
u8 Equip_LevelMark(struct EquipItem *e, u8 flags, struct EquipItem *other);
void EquipPreview_Free(void);
s32 Equip_GetModelId(u8 type, u16 id, u8 x); /* unit's own declaration: with the definition's u16 (u16 kind, u16 id, u16 female) (sub_main/model_load.c), EquipShop_TryOn 4 of 127 words, EquipShop_InputList 1 of 218 words differ */
void EquipPreview_Load(u8 type, s32 n, u8 x); /* unit's own declaration: with the definition's void (u16 kind, u16 id, u8 female) (sub_main/model_load.c), EquipShop_TryOn 4 of 127 words, EquipShop_InputList 1 of 218 words differ */

/* The detail of the line under the cursor for the view shown: 3 the
 * wearer's check (Equip_PreviewSkills), 2 the comparison with the box item at
 * +0xBC, 1 the item itself, whose model is loaded when it changed (+0xD4).
 * The game clears sizeof(EquipItem) bytes at the pointer's own address. */
void EquipShop_TryOn(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    EquipItem *e = &s->line[s->cursor].item;
    EquipItem *pe;
    u8 x;

    memset(&pe, 0, sizeof(EquipItem));
    switch (s->view) {
    case 3:
        s->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &s->line[s->cursor].item);
        s->detailPage = 0;
        break;
    case 2:
        if (e->type != 6 && e->type != 7) {
            s->detailPages = Equip_LevelMark(&s->worn, 0x50, e);
        } else {
            s->detailPages = Equip_LevelMark(&s->worn, 0x40, e);
        }
        break;
    case 1:
        s->detailPages = Equip_LevelMark(&s->line[s->cursor].item, 0x40, NULL);
        if (s->selling == 0) {
            pe = &s->line[s->cursor].item;
            if (pe->type != s->shown.type || pe->id != s->shown.id) {
                EquipPreview_Free();
                s->shown = *pe;
                x = gSave.gender;
                EquipPreview_Load(pe->type, Equip_GetModelId(pe->type, pe->id, x), x);
            }
        }
        break;
    }
}

void EquipPreview_Hide(u8 hide)
{
    u8 *p = gEquipPreview;

    if (hide == 0) {
        *p = 1;
    } else {
        *p = 0;
    }
}

void EquipShop_Choose(void);
s32 Menu_WrapLeftRight(s32 cur, s32 n);

/* The shop list's input: 0x20 buys the line under the cursor (0), 0x40
 * closes the detail or the list (3), 0x200 the next detail view, pages and
 * lines, 0x80/0x8 the detail's pages, 0x4000 the model preview; the detail
 * is redrawn when the page or line changed. Else 2. */
s32 EquipShop_InputList(u16 pad)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    EquipItem *e;
    s16 cur;
    s16 pg;
    u8 x;

    pg = s->page;
    cur = s->cursor;
    if (pad & 0x20) {
        if (s->line[cur].noBuy == 0) {
            EquipShop_Choose();
            LobbySe_Play(0x1B);
            return 0;
        }
        LobbySe_Play(9);
    } else if (pad & 0x40) {
        LobbySe_Play(5);
        if (s->view != 0) {
            s->view = 0;
            s->detailPage = 0;
        } else {
            return 3;
        }
    } else if (pad & 0x200) {
        LobbySe_Play(0x39);
        if (++s->view >= 4) {
            s->view = 1;
        }
        EquipShop_TryOn();
        s->detailPage = 0;
    } else if (pad & 0xC00) {
        s->page = Menu_WrapLeftRight(pg, s->pages);
        if (pg != s->page) {
            EquipShop_BuildPage();
        }
    } else if (pad & 0x3000) {
        s->cursor = Menu_WrapUpDownSe(cur, s->lines, 2);
    } else if (pad & 0x80) {
        if (s->view != 0 && s->detailPages > 1) {
            if (++s->detailPage >= s->detailPages) {
                s->detailPage = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (s->view != 0 && s->detailPages > 1) {
            if (--s->detailPage < 0) {
                s->detailPage = s->detailPages - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x4000) {
        if (s->view == 1) {
            if (s->selling == 0) {
                s->selling = 1;
                EquipPreview_Hide(1);
            } else {
                s->selling = 0;
                e = &s->line[s->cursor].item;
                if (e->type != s->shown.type || e->id != s->shown.id) {
                    EquipPreview_Free();
                    s->shown = *e;
                    x = gSave.gender;
                    EquipPreview_Load(e->type, Equip_GetModelId(e->type, e->id, x), x);
                }
            }
        }
    }
    if (pg != s->page || cur != s->cursor) {
        EquipShop_TryOn();
    }
    return 2;
}

void EquipBox_FirstPage(void)
{
    BoxPages *s = (BoxPages *)gMenuWork;

    s->page = 0;
    s->unk1E = 0;
    s->pages = Box_GetPageCount(&gSave);
}

s32 Box_IsSlotFree(SaveData *save, s32 idx);
s32 Equip_IsSlotEquipped(SaveData *save, s32 idx);
void EquipBox_SellPrice(void);
s32 Menu_WrapUpDown(s32 cur, s32 n);

/* The box of the equipment shop (10 by 10 per box): 0x20 chooses a slot
 * to sell (0; not free and not worn), 0x40 closes the detail or the box
 * (3), 0x200 the detail, 0x80/0x8 its pages, 0x10/0x4 the boxes, the pad
 * moves the cursor. Else 2. */
s32 EquipShop_InputSell(u16 pad)
{
    EquipSell *s = (EquipSell *)gMenuWork;
    s32 slot = s->col + (s->row * 10 + s->box * 100);

    if (pad & 0x20) {
        if (Box_IsSlotFree(&gSave, slot) == 0 && Equip_IsSlotEquipped(&gSave, slot) == 0) {
            EquipBox_SellPrice();
            LobbySe_Play(0x1B);
            return 0;
        }
        LobbySe_Play(9);
    } else if (pad & 0x40) {
        if (s->unk1E != 0) {
            s->unk1E = 0;
            s->page = 0;
            s->pages = 0;
            LobbySe_Play(4);
        } else {
            LobbySe_Play(4);
            return 3;
        }
    } else if (pad & 0x200) {
        /* the slot's item id (gSave.box[slot].id) */
        if (gSave.box[slot].id != 0) {
            s->unk1E ^= 1;
            LobbySe_Play(0x39);
            if (s->unk1E != 0) {
                s->pages = Equip_LevelMark(&gSave.box[slot], 0x40, NULL);
            } else {
                s->pages = 0;
                s->page = 0;
            }
        }
    } else if (pad & 0x80) {
        if (s->unk1E != 0 && s->pages > 1) {
            if (++s->page >= s->pages) {
                s->page = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (s->unk1E != 0 && s->pages > 1) {
            if (--s->page < 0) {
                s->page = s->pages - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x10) {
        if (*(s16 *)s->unk24 > 1) {
            if (++s->box >= *(s16 *)s->unk24) {
                s->box = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x4) {
        if (*(s16 *)s->unk24 > 1) {
            if (--s->box < 0) {
                s->box = *(s16 *)s->unk24 - 1;
            }
            LobbySe_Play(1);
        }
    } else if (s->unk1E == 0) {
        s->col = Menu_WrapLeftRight(s->col, 10);
        s->row = Menu_WrapUpDown(s->row, 10);
    }
    return 2;
}

void EquipShop_Choose(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    s->price = s->line[s->cursor].price;
    s->item = s->line[s->cursor].item;
    s->item.used = 1;
    Menu_SetYesNo(0);
    gMenuInputLock = 9;
    if (Equip_CanWear(&gSave, &s->item) == 0) {
        s->msg = gEquipShopTexts[EQUIP_SHOP_TEXT_CANNOT_WEAR];
        s->canWear = 0;
    } else {
        s->msg = Text_FillDollar((char *) gEquipShopTexts[EQUIP_SHOP_TEXT_PRICE_ASK], s->price);
        s->canWear = 1;
    }
}

void EquipShop_Buy(u8 equip)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    u16 slot;

    slot = Box_AddItem(&gSave, s->item.type, s->item.id, 0);
    Save_AddMoney(-s->price);
    LobbySe_Play(10);
    if (equip == 0) {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(0xF);
        return;
    }
    LobbySe_Play(0x12);
    Equip_EquipSlot(&gSave, slot);
    Player_SetupFromSave(&gPlayers[gScene.localPlayer], &gSave);
    Player_CalcSkills(&gPlayers[gScene.localPlayer]);
    Town_ReloadLocalPlayer();
    gLobbyTextDirty = 1;
    Menu_PushNotice(0x17);
}

s32 LobbyMenu_YesNoInput(s32 msg);
void LobbyText_Open(s32 a);

/* The buying steps (+0xD) of the equipment shop: confirm the price, then
 * whether to wear it at once (when it can be worn; gEquipShopTexts[EQUIP_SHOP_TEXT_WEAPON_CHANGE_WARN] when the
 * weapon's kind changes), each with the detail's pages. 0 bought, 3
 * cancelled, else 2. */
s32 EquipShop_InputBuy(u16 pad)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    switch (s->canWear) {
    case 0:
        if (pad & 0x80) {
            if (s->detailPages > 1) {
                if (++s->detailPage >= s->detailPages) {
                    s->detailPage = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (s->detailPages > 1) {
                if (--s->detailPage < 0) {
                    s->detailPage = s->detailPages - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x1B)) {
            case 0:
                s->canWear++;
                LobbyText_Open(1);
                s->msg = Text_FillDollar((char *) gEquipShopTexts[EQUIP_SHOP_TEXT_PRICE_ASK], s->price);
                break;
            case 3:
                return 3;
            }
        }
        break;
    case 1:
        if (pad & 0x80) {
            if (s->detailPages > 1) {
                if (++s->detailPage >= s->detailPages) {
                    s->detailPage = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (s->detailPages > 1) {
                if (--s->detailPage < 0) {
                    s->detailPage = s->detailPages - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x13)) {
            case 0:
                if (Equip_CanWear(&gSave, &s->item) == 1) {
                    s->canWear++;
                    s->msg = gEquipShopTexts[EQUIP_SHOP_TEXT_EQUIP_ASK];
                    Menu_SetYesNo(0);
                    gMenuInputLock = 9;
                    switch (s->worn.type) {
                    case 6:
                    case 7:
                        if (s->worn.type != s->item.type) {
                            s->msg = gEquipShopTexts[EQUIP_SHOP_TEXT_WEAPON_CHANGE_WARN];
                        }
                        break;
                    }
                } else {
                    EquipShop_Buy(0);
                    s->canWear = 0;
                    return 0;
                }
                break;
            case 3:
                s->canWear = 0;
                return 3;
            }
        }
        break;
    case 2:
        if (pad & 0x80) {
            if (s->detailPages > 1) {
                if (++s->detailPage >= s->detailPages) {
                    s->detailPage = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (s->detailPages > 1) {
                if (--s->detailPage < 0) {
                    s->detailPage = s->detailPages - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x1B)) {
            case 0:
                EquipShop_Buy(1);
                s->canWear = 0;
                return 0;
            case 3:
                EquipShop_Buy(0);
                s->canWear = 0;
                return 0;
            }
        }
        break;
    }
    return 2;
}

void EquipBox_SellPrice(void)
{
    EquipSell *s = (EquipSell *)gMenuWork;
    s32 slot = s->col + (s->row * 10 + s->box * 100);

    Menu_SetYesNo(0);
    gMenuInputLock = 9;
    s->price = Equip_GetSellPrice(&gSave.box[slot]);
    s->msg = Text_FillDollar((char *) gEquipShopTexts[EQUIP_SHOP_TEXT_PRICE_ASK], s->price);
}

s32 EquipBox_SellMenu(u16 pad)
{
    EquipSell *s = (EquipSell *)gMenuWork;
    s32 slot = s->col + (s->row * 10 + s->box * 100);


    if (pad & 0x80) {
        if (s->pages > 1) {
            if (++s->page >= s->pages) {
                s->page = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 8) {
        if (s->pages > 1) {
            if (--s->page < 0) {
                s->page = s->pages - 1;
            }
            LobbySe_Play(1);
        }
    } else {
        switch (Menu_YesNoInput()) {
        case 0:
            Save_AddMoney(s->price);
            Box_ClearSlot(&gSave, slot);
            LobbySe_Play(10);
            if ((u8)(s32)EquipSet_RemoveSlot(slot, 1)) {
                EquipSet_RemoveSlot(slot, 0);
            }
            return 0;
        case 3:
            return 3;
        }
    }
    return 2;
}


void EquipShop_DrawMenu(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(0x14, 0x14);
    switch (s->canWear) {
    case 0:
        if (s->hasOffers == 0) {
            Menu_DrawLabelList(0x1CC, &gShopMenuTexts[SHOP_MENU_TEXT_BUY], 2, s->cursor, 0xFFB0B000);
        } else {
            Menu_DrawLabelList(0x1CC, gShopBuySellTexts, 3, s->cursor, 0xFFB0B000);
        }
        break;
    case 1:
        Menu_DrawLabelList(0x1CC, gWeaponArmorTexts, 2, s->cursor, 0xFFB0B000);
        break;
    case 2:
        if (s->tab == 0) {
            Menu_DrawLabelList(0x1A4, &gEquipGroupTexts[s->tab * 5], 5, s->cursor, 0xFFB0B000);
        } else {
            Menu_DrawLabelList(0x1CC, &gEquipGroupTexts[s->tab * 5], 5, s->cursor, 0xFFB0B000);
        }
        break;
    }
}

s32 Box_CountEquip(u8 type, u16 id)
{
    EquipItem *e = gSave.box;
    s32 i;
    s32 n = (s16)Box_GetCapacity(&gSave);
    s32 count = 0;

    for (i = 0; i < n; i++, e++) {
        if (e->type == type && e->id == id) {
            count++;
        }
    }
    return count;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7580B0", D_0083AF90);

extern char D_0083AF90[];
void Equip_DrawWearer(EquipItem *e);

/* A line of the equipment shop: the icon (with the price mark unless the
 * price is fixed), the name, the count owned and the wearer. y steps
 * inside the calls' arguments, as the game's code does (made as statements,
 * MWCC extends y before the add). */
void EquipShop_DrawLine(s16 x, s16 y, EquipItem *e)
{
    s32 x2;

    if (Forge_IsRandomWeapon(e->id)) {
        Lobby_DrawEquipIcon(x, y, 0x3C, 0x3C, e);
    } else {
        Lobby_DrawEquipIcon(x, y, 0x3C, 0x3C, e);
        Item_DrawRarity(x + 12, y + 0x3C, Equip_GetRarity(e->type, e->id));
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    x2 = x + 0x4E;
    Font_SetPos(x2, y);
    if (Forge_IsRandomWeapon(e->id)) {
        Text_PrintfColored(D_0083AF90, gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME]);
    } else {
        Text_PrintfColored(D_0083AF90, Equip_GetName(e->type, e->id));
    }
    Font_SetPos(x2, y += 0x16);
    Text_PrintfColored("%s%s%3d", gCountLabels[COUNT_LABEL_HELD], gLobbyWords[LOBBY_WORD_TIMES], Box_CountEquip(e->type, e->id));
    Font_SetPos(x2, y += 0x16);
    Equip_DrawWearer(e);
}

void EquipShop_DrawNote(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    char *msg;

    switch (s->line[s->cursor].item.type) {
    case 6:
    case 7:
        msg = gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_WEAPON];
        break;
    default:
        msg = gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_ARMOR];
        break;
    }
    Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0x80202020, msg);
}

void EquipShop_DrawFrame(s16 x, s16 y)
{
    EquipShop_DrawTitle();
    Menu_DrawMoney(0x1E0, 0x10, 0);
    Icon_Draw(x, y, 0x154, 0x122, -0x2223, 0);
    Icon_Draw(x + 0x6E, y + 0x2D, 0x78, 0xC8, 0x60FFFFFF, 0x24);
}

void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
extern u16 gFrameTick;

/* The equipment shop's page at (x, y): the page counter, each line (the
 * icon blinking when Equip_TestFlag says so, the cursor frame, the name and
 * price, greyed when it can't be bought), then the line under the cursor
 * and the note. */
void EquipShop_DrawPage(s16 x, s16 y)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    s16 y0 = y;
    ShopLine *l;
    s16 i;

    EquipShop_DrawFrame(x, y);
    y += 0x1A;
    Font_SetColor(0x80101010);
    Menu_DrawPageNum((s16)(x + 0x28) + 0xC8, y - 0x16, s->page, s->pages, 1);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(x + 0x28, y);
    for (i = 0; i < 7; i++) {
        l = &s->line[i];
        if (l->item.id == 0) {
            break;
        }
        if (Equip_TestFlag(&l->item, 2) == 1 && (gFrameTick & 0x10)) {
            Icon_Draw((s16)(x + 0x28) - 0x12, y, 0x10, 0x14, 0xFFC03000, 0x63);
        } else {
            Lobby_DrawEquipIcon((s16)(x + 0x28) - 0x18, y, 0x14, 0x14, &l->item);
        }
        if (i == s->cursor) {
            Menu_DrawCursorRow(x + 0x28, y, 0x112, 0x32, 0xC0106060, 0xFF106060);
        }
        if (l->noBuy != 0) {
            Font_SetColor(0x8042352E);
        } else {
            Font_SetColor(0x80101010);
        }
        Text_PrintfColored("%-20s %6d%s", Equip_GetName(l->item.type, l->item.id), l->price, gLobbyWords[LOBBY_WORD_ZENNY]);
        Font_SetPos(x + 0x28, y += 0x16);
    }
    Menu_DrawDashLine(x + 0x14, y0 + 0xBA, 0x12C, 0xFF000000);
    if (s->cursor < 0 || s->cursor >= 7) {} /* compiled-out check */
    EquipShop_DrawLine(x + 0x10, y0 + 0xC8, &s->line[s->cursor].item);
    EquipShop_DrawNote();
}


void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 color);

/* The equipment shop's page in the small layout: the lines at fixed
 * places, the cursor bar with its blinking arrow. */
void EquipShop_DrawPageSmall(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    s16 d;
    ShopLine *l;
    s16 y;
    s16 i;

    Icon_Draw(0, 0x32, 0x122, 0xB4, -0x2223, 0);
    Icon_Draw(0x64, 0x41, 0x5A, 0x96, 0x60FFFFFF, 0x24);
    y = 0x52;
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(0xC8, 0x3E, s->page, s->pages, 1);
    Font_SetSpacing(0x11, 0x11);
    Font_SetPos(0x28, y);
    for (i = 0; i < 7; i++) {
        l = &s->line[i];
        if (l->item.id == 0) {
            break;
        }
        if (Equip_TestFlag(&l->item, 2) == 1 && (gFrameTick & 0x10)) {
            Icon_Draw(0x14, y, 0x10, 0x14, 0xFFC03000, 0x63);
        } else {
            Lobby_DrawEquipIcon(0xE, y, 0x14, 0x14, &l->item);
        }
        if (i == s->cursor) {
            d = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(0x20, y + 0xC, 0xF8, 7, 0x99106060, 0x6F);
            Menu_DrawCursorIcon(-2 - d, y - 2, 0x16, 0xFF20C0C0);
        }
        if (l->noBuy != 0) {
            Font_SetColor(0x8042352E);
        } else {
            Font_SetColor(0x80101010);
        }
        Text_PrintfColored("%-20s", Equip_GetName(l->item.type, l->item.id));
        Font_SetPos(0xDA, y);
        Text_PrintfColored("%6d%s", l->price, gLobbyWords[LOBBY_WORD_ZENNY]);
        Font_SetPos(0x28, y += 0x14);
    }
}

typedef struct GradBox {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u32 col[4];
} GradBox;
extern u32 gHeaderBoxColors[4];
extern s16 gMenuOffsetX;
extern s16 gMenuOffsetY;
struct PrimGradRect;
s32 Prim_FillGradRect(struct PrimGradRect *);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), Equip_DrawPreviewHelp 35 of 152 words differ */
s16 Lobby_DrawButtonIcon(s16, s16, s32);

/* The window at (gMenuOffsetX, gMenuOffsetY) with a gradient; k 0 adds a
 * header with two icons and labels, k 1 a line of text. */
void Equip_DrawPreviewHelp(u32 color, s8 k)
{
    GradBox b;
    s16 x;
    s16 y;

    b.x0 = gMenuOffsetX + 0x136;
    b.y0 = gMenuOffsetY + 0x36;
    b.x1 = b.x0 + 0x132;
    b.y1 = b.y0 + 0x96;
    b.col[0] = gHeaderBoxColors[0];
    b.col[1] = gHeaderBoxColors[1];
    b.col[2] = gHeaderBoxColors[2];
    b.col[3] = gHeaderBoxColors[3];
    Font_SetColor(0x807F7F7F);
    if (k == 0) {
        x = gMenuOffsetX + 0x136;
        y = gMenuOffsetY + 0x18;
        Win_Draw(x, y, 0x9A, 0x14, color, 0xC0202020, 6);
        Lobby_DrawButtonIcon(x + 4, y - 1, 0xF);
        Lobby_DrawButtonIcon(x + 0x4C, y - 1, 0xE);
        Font_SetSpacing(0x11, 0x11);
        Font_SetPos(x + 0x1C, y + 1);
        Text_PrintfColored(D_0083AF90, gTownLabels[TOWN_LABEL_MOVE]);
        Font_SetPos(x + 0x64, y + 1);
        Text_PrintfColored(D_0083AF90, gTownLabels[TOWN_LABEL_ZOOM]);
    }
    Prim_FillGradRect((struct PrimGradRect *)&b);
    Win_Draw(gMenuOffsetX + 0x136, gMenuOffsetY + 0x36, 0x132, 0x96, color, 0, 6);
    if (k == 1) {
        Font_SetPos(gMenuOffsetX + 0x17C, gMenuOffsetY + 0x6E);
        Font_SetSpacing(0x12, 0x12);
        Text_Printf(gForgeTexts[FORGE_TEXT_SELECT_SHOWS_EQUIP]);
    }
}

void EquipShop_DrawList(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    Equip_DrawPreviewHelp(0xBDB59F6C, s->selling);
    if (s->selling == 0) {
        EquipPreview_Hide(0);
    }
    EquipShop_DrawPageSmall();
    EquipShop_DrawNote();
}

void EquipShop_DrawPreview(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    ShopLine *l = &s->line[s->cursor];

    EquipShop_DrawPageSmall();
    if (l->item.type != 6 && l->item.type != 7) {
        EquipCompare_Draw(&s->worn, &l->item, 0x136, 0x36, s->detailPage | 0x50);
    } else {
        EquipCompare_Draw(&s->worn, &l->item, 0x136, 0x36, s->detailPage | 0x40);
    }
    EquipShop_DrawNote();
}

void EquipShop_DrawPageDetail(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    EquipShop_DrawPageSmall();
    EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, 0x3A, s->detailPage);
}

void EquipShop_DrawTitle(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0xA, 0x12, 0xA0, 0xFFB0B000, gEquipGroupTexts[s->tab * 5 + s->kind], 2, 1);
}

void EquipShop_Draw(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    ShopLine *l = &s->line[s->cursor];

    EquipShop_DrawTitle();
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (s->view) {
    case 0:
        EquipShop_DrawPage(s->win.pos + 0xC, 0x34);
        break;
    case 1:
        EquipShop_DrawList();
        EquipWindow_DrawLocal(&l->item, 0x136, 0xD8, s->detailPage | 0x40, NULL);
        break;
    case 2:
        EquipShop_DrawPreview();
        break;
    case 3:
        EquipShop_DrawPageDetail();
        break;
    }
}

void EquipShop_DrawItem(void)
{
    EquipShop *s = (EquipShop *)gMenuWork;

    EquipShop_DrawTitle();
    Menu_DrawMoney(0x1E0, 0x10, 0);
    if (s->msg != NULL) {
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xC0101010, s->msg);
    }
    if ((s8)s->canWear == 2) {
        Font_SetPos(0x144, 0x194);
        Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
    } else {
        Font_SetPos(0x1B0, 0x194);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    }
    EquipCompare_Draw(&s->worn, &s->item, 0x136, 0x36, s->detailPage | 0x50);
}


void Equip_DrawWearer(EquipItem *e)
{
    u8 flags;
    s32 i;

    if (e->type != 7 && e->type != 6) {
        flags = Equip_GetWearMask(&gSave, e);
        i = 0xFF;
        if ((flags & 3) != 3) {
            if (flags & 1) {
                switch (flags & 0xC) {
                case 4:
                    i = 7;
                    break;
                case 8:
                    i = 5;
                    break;
                default:
                    i = 3;
                    break;
                }
            } else {
                switch (flags & 0xC) {
                case 4:
                    i = 6;
                    break;
                case 8:
                    i = 4;
                    break;
                default:
                    i = 2;
                    break;
                }
            }
        } else if ((flags & 0xC) != 0xC) {
            if (flags & 4) {
                i = 1;
            } else {
                i = 0;
            }
        }
        if (i != 0xFF) {
            Font_SetColor(0x807F7F28);
            Text_PrintfColored("%-16s", gEquipLimitTexts[i]);
        }
    }
}


void EquipBox_DrawSellNote(s16 x, s16 y)
{
    EquipSell *s = (EquipSell *)gMenuWork;
    s32 slot = s->col + (s->row * 10 + s->box * 100);

    Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xC0101010, (u8)(s32)EquipSet_RemoveSlot(slot, 1) ? gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_SELL_IN_SET] : gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_SELL]);
    if (s->unk1E != 0) {
        BoxMenu_DrawItemList(slot, (s->page & 7) | 0x10002F8, x + 12);
    } else {
        BoxMenu_DrawItemList(slot, 0x1000290, x + 12);
        Font_SetSpacing(0x12, 0x12);
        Font_SetColor(0x807F7F7F);
        Font_SetPos(0x1FE, 0x194);
        Text_Printf(gGameMenuLabels[MENU_LABEL_DETAILS_GUIDE]);
    }
}

void EquipBox_DrawSellPrice(s16 x, s16 y)
{
    EquipSell *s = (EquipSell *)gMenuWork;
    s32 slot = s->col + (s->row * 10 + s->box * 100);

    Equip_GetSellPrice(&gSave.box[slot]);
    if (s->msg != NULL) {
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xC0101010, s->msg);
    }
    Font_SetPos(0x1B0, 0x194);
    Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    if (s->unk1E != 0) {
        BoxMenu_DrawItemList(slot, (s->page & 7) | 0x10002F8, x + 12);
    } else {
        BoxMenu_DrawItemList(slot, 0x1000290, x + 12);
    }
}

void EquipShop_DrawTask(u8 *task)
{
    EquipShop *s = (EquipShop *)gMenuWork;
    s16 x;

    Text_SetSize(*(s32 *)(task + 0x18));
    x = s->win.pos;
    switch (s->state) {
    case 0:
        EquipShop_DrawMenu();
        break;
    case 1:
        if (s->unk10 == 0) {
            EquipShop_DrawFrame(x, 0x34);
        } else {
            EquipBox_DrawSellNote(x + 0xE, 0x34);
        }
        break;
    case 2:
        EquipShop_Draw();
        break;
    case 3:
        EquipShop_DrawItem();
        break;
    case 4:
        EquipBox_DrawSellNote(x + 0xE, 0x34);
        break;
    case 5:
        EquipBox_DrawSellPrice(x + 0xE, 0x34);
        break;
    }
}

/* unk_7580B0's .data (0x7F1858-0x7F1990) */
void NpcSetA_Run(u8 *task);
void NpcSetA_Effect(u8 *task);
void NpcSetB_Run(u8 *task);
void NpcSetB_Effect(u8 *task);
void NpcSetB_Nop(void);
void NpcSetC_Run(u8 *task);
void NpcSetC_Effect(u8 *task);
void NpcSetC_Nop(void);
u8 gEquipShopTypesTab0[8] = {
#include "lobby/007F1858.inc"
};
u8 gEquipShopTypesTab1[16] = {
#include "lobby/007F1860.inc"
};
u32 gHeaderBoxColors[4] = {
#include "lobby/007F1870.inc"
};
void *gNpcHandlers_3[8] = {
    NpcSetA_Effect, NpcSetA_Effect, NpcSetA_Effect, NpcSetA_Run,
    NpcSetA_Effect, NULL, NULL, NULL,
};
void *gNpcHandlers_2[8] = {
    NpcSetB_Effect, NpcSetB_Effect, NpcSetB_Nop, NpcSetB_Run,
    NpcSetB_Nop, NULL, NULL, NULL,
};
u32 gNpcHandlers_1[16] = {
#include "lobby/007F18C0.inc"
};
/* The lobby's own monster hit tables, which the push-apart and camera code
 * read while gScene.inLobby is set in place of the game's gEmBoneHits and
 * gEmPushSpheres: every one of the four kinds has the same one-capsule bone
 * list and the same one-sphere push list ({x, y, z, radius}; a radius of -1
 * ends it). */
HitShape gEmBoneHitsLobbyList[2] = {
#include "lobby/007F1900.inc"
};
HitShape *gEmBoneHitsLobby[4] = {
#include "lobby/007F1950.inc"
};
f32 gEmPushSpheresLobbyList[8] = {
#include "lobby/007F1960.inc"
};
f32 *gEmPushSpheresLobby[4] = {
#include "lobby/007F1980.inc"
};
