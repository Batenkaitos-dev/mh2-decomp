#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "em.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "player.h"
#include "lobby_menu.h"

/* prototypes */
void ArmorUpgrade_DrawMaterials(s32, s32, ItemStack *need, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y, ItemStack *need, s8 n) (lobby/unk_6BF430.c), WeaponForge_DrawMaterials 17 of 28 words differ */
s32 WeaponForge_IsClass10(EquipItem *e);
/* end of prototypes */

typedef struct ForgeRecipe {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 pad1;
    /* 0x02 */ u16 id;
    /* 0x04 */ u16 key; /* WeaponForge_RollRandom (the random pick) */
    /* 0x06 */ u8 unk6[0x12];
} ForgeRecipe;
typedef struct ArmorMenu {
    /* 0x00 */ u8 unk0[0xF4];
    /* 0xF4 */ s32 list[1];
} ArmorMenu;
/* the materials of a weapon upgrade */
typedef struct WeaponMats {
    /* 0x0 */ ItemStack need[3];
} WeaponMats;
/* a weapon's upgrade entry (gBladeUpgrades blades, gGunUpgrades guns, by id) */
typedef struct WeaponUp {
    /* 0x00 */ WeaponMats mats; /* what an upgrade into this weapon takes */
    /* 0x0C */ u16 next[4]; /* what it can become */
    /* 0x14 */ u8 unk14[4];
} WeaponUp;
/* the weapon workshop (task + 0xC): forging (the list at 0xD8) and upgrades
   (the upgrades of the chosen weapon at 0x2C..0xCC) */
typedef struct WeaponMenu {
    /* 0x000 */ EquipItem equip; /* the weapon being upgraded */
    /* 0x00C */ EquipItem orig;
    /* 0x018 */ WeaponUp *table; /* the weapon's upgrade entry (gBladeUpgrades blades, gGunUpgrades guns) */
    /* 0x01C */ WinSlide win;
    /* 0x024 */ WinSlide win2;
    /* 0x02C */ s32 gunCost[4]; /* a gun upgrade's price */
    /* 0x03C */ s32 msg[4]; /* message numbers (gDecoMakeTexts, from 11) */
    /* 0x04C */ EquipItem upgrades[4]; /* what the weapon can become */
    /* 0x07C */ u8 unk7C[0xC];
    /* 0x088 */ u32 cost[4];
    /* 0x098 */ u8 unk98[4];
    /* 0x09C */ WeaponMats mats[4];
    /* 0x0CC */ u8 unkCC[0xC];
    /* 0x0D8 */ s32 list[7]; /* a page of gWeaponRecipes indices */
    /* 0x0F4 */ s16 choice;
    /* 0x0F6 */ s16 unkF6;
    /* 0x0F8 */ s16 page; /* the box: page * 100 + row * 10 + col */
    /* 0x0FA */ s16 pages;
    /* 0x0FC */ s16 detailPage;
    /* 0x0FE */ s16 detailPages;
    /* 0x100 */ s16 col;
    /* 0x102 */ s16 row; /* the forge list: the tab (0-3 blade weapon classes, 4 guns: WeaponForge_MakeEquip) */
    /* 0x104 */ s16 cursor;
    /* 0x106 */ s16 count;
    /* 0x108 */ s16 mode;
    /* 0x10A */ u16 unk10A; /* WeaponForge_GunModBits flags */
    /* 0x10C */ s8 step; /* 0 the choice, 1 upgrades, 2 forging, 3 confirm */
    /* 0x10D */ s8 unk10D;
    /* 0x10E */ s8 equipped; /* the weapon is worn */
    /* 0x10F */ s8 kind; /* the weapon's type: 6 blade, 7 gun */
    /* 0x110 */ s8 unk110;
    /* 0x111 */ s8 filter; /* 0: recipes not in gRecipeIdPairs (Recipe_IsInList), 1: those in it */
} WeaponMenu;

extern ForgeRecipe gArmorRecipes[];
extern ForgeRecipe gWeaponRecipes[];
s32 Forge_CheckRecipe(u16 which, s16 idx, s32 mode);
u16 Box_FindFreeSlot(void *unused0);
u32 Equip_HalfPrice(s32 type, s32 id);
s32 Forge_IsFixedPrice(s16 id);

void *memset(void *dst, s32 c, u32 n);
u8 Weapon_GetClass(u8 type, u16 id);
s32 Equip_TestFlag(EquipItem *e, s16 bit);
void Equip_SetFlag(EquipItem *e, s16 bit);
s32 WeaponForge_MakeEquip(s16 tab, ForgeRecipe *r, s32 idx)
{
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    if (r->type == 6) {
        if (tab == 4) {
            return 0;
        }
    } else {
        if (tab != 4) {
            return 0;
        }
    }
    switch (Weapon_GetClass(r->type, r->id)) {
    case 0:
    case 7:
        if (tab != 0) {
            return 0;
        }
        break;
    case 4:
    case 6:
        if (tab != 1) {
            return 0;
        }
        break;
    case 2:
    case 8:
        if (tab != 2) {
            return 0;
        }
        break;
    case 3:
    case 9:
        if (tab != 3) {
            return 0;
        }
        break;
    case 1:
    case 5:
    default:
        if (tab != 4) {
            return 0;
        }
        break;
    }
    tmp.type = r->type;
    tmp.id = r->id;
    if (Forge_CheckRecipe(1, idx, 0) == 0) {
        return 0;
    }
    if (Equip_TestFlag(&tmp, 1) == 0) {
        Equip_SetFlag(&tmp, 2);
        Equip_SetFlag(&tmp, 1);
    }
    return 1;
}

s32 Forge_CanMakeWeapon(WeaponMenu *m, s16 i)
{
    s32 idx = m->list[i];
    ForgeRecipe *r = &gWeaponRecipes[idx];
    s32 cost;

    if (Forge_CheckRecipe(1, idx, 0) != 2) {
        return 0;
    }
    if (Forge_IsFixedPrice(r->id) != 0) {
        cost = 1000;
    } else {
        cost = Equip_HalfPrice(r->type, r->id);
    }
    if (gSave.money < cost) {
        return 0;
    }
    return (u16)Box_FindFreeSlot(&gSave) != 0xFFFF;
}

s32 Recipe_IsInList(s32 type, s32 id);
s32 Menu_CountPages(s16 n, s16 per);
void WeaponForge_LoadMakePage(WeaponMenu *m, s32 page)
{
    s32 last;
    ForgeRecipe *r;
    s32 count;
    s32 n;
    s32 first;
    s32 i;
    u8 type;

    r = gWeaponRecipes;
    first = page * 7;
    count = 0;
    type = gWeaponRecipes[0].type;
    last = first + 6;
    n = 0;
    i = 0;
    while (type != 0xFF) {
        if (WeaponForge_MakeEquip(m->row, r, i) == 1) {
            if ((m->filter == 0 && Recipe_IsInList(r->type, r->id) == 0)
                || (m->filter == 1 && Recipe_IsInList(r->type, r->id) == 1)) {
                if (n >= first && n <= last) {
                    m->list[count] = i;
                    count++;
                }
                n++;
            }
        }
        r++;
        i++;
        type = r->type;
    }
    m->count = count;
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
    m->pages = Menu_CountPages(n, 7);
}

extern u8 gMenuInputLock;
void Menu_SetYesNo(s8 arg0);
void EquipPreview_Show(u8 on);
s32 LobbyMenu_YesNoInput(s32 msg);
void WeaponForge_RollRandom(EquipItem *out, u16 key);
void LobbySe_Play(s32 se);
s32 WinSlide_Step(WinSlide *win, s16 from, s16 to);
void WeaponForge_MakeDetail(WeaponMenu *m);
void CamFocus_SetAside(s8 arg0);
void EquipPreview_Reset(void);
void Menu_PushNotice(s32 msg);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 se);
s32 Menu_WrapLeftRight(s32 cursor, s32 count);
void StageModels_Free(void);
/* unit's own declaration: with the definition's u16 (u16 kind, u16 id, u16 female) (sub_main/unk_33B080.c), WeaponForge_BoxInput: 1 of 337 words differ, WeaponForge_MakeDetail: 1 of 152 words differ ... */
s32 Equip_GetModelId(u8 type, u16 id, u8 x);
void StageModels_LoadEquip(u8 type, s32 n, u8 x); /* unit's own declaration: with the definition's void (u16 kind, u16 id, u8 female) (sub_main/unk_33B080.c), WeaponForge_MakeInput 1 of 434 words, WeaponForge_BoxInput 1 of 337 words, WeaponForge_UpgradeListInput 2 of 249 words differ */
s32 WeaponForge_MakeInput(WeaponMenu *m, u16 pad)
{
    s32 cur = m->cursor;
    EquipItem tmp;
    u16 *id;
    u8 *type;
    u8 x;

    memset(&tmp, 0, sizeof(tmp));
    switch (m->unk10D) {
    case 0:
        if (pad & 0x20) {
            m->unk10D++;
            m->page = 0;
            m->detailPage = 0;
            m->cursor = 0;
            m->mode = 0;
            WinSlide_Step(&m->win, 0x280, 0x118);
            WeaponForge_LoadMakePage(m, 0);
            WeaponForge_MakeDetail(m);
            LobbySe_Play(0x17);
            CamFocus_SetAside(1);
            EquipPreview_Reset();
            if ((u16)Box_FindFreeSlot(&gSave) == 0xFFFF) {
                Menu_PushNotice(0xB);
            }
        } else if (pad & 0x40) {
            m->unk10D = 0;
            LobbySe_Play(4);
            return 3;
        } else {
            m->row = Menu_WrapUpDownSe(m->row, 5, 2);
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->unk10D++;
            break;
        }
        break;
    case 2:
        if (pad & 0x20) {
            if (m->count != 0 && Forge_CanMakeWeapon(m, m->cursor) == 1) {
                m->unk10D++;
                m->detailPage = 0;
                Menu_SetYesNo(0);
                gMenuInputLock = 9;
                EquipPreview_Show(1);
                LobbySe_Play(0x1B);
                m->mode = 3;
                WeaponForge_MakeDetail(m);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            if (m->mode != 0) {
                m->mode = 0;
                WeaponForge_MakeDetail(m);
            } else {
                m->unk10D = 0;
                EquipPreview_Show(1);
                CamFocus_SetAside(0);
            }
            LobbySe_Play(0x17);
        } else if (pad & 0x200) {
            if (m->count != 0) {
                if (++m->mode == 5) {
                    m->mode = 1;
                }
                m->detailPage = 0;
                WeaponForge_MakeDetail(m);
                LobbySe_Play(0x39);
            }
        } else if (pad & 0xC00) {
            if (m->pages > 1) {
                m->page = Menu_WrapLeftRight(m->page, m->pages);
                WeaponForge_LoadMakePage(m, m->page);
                WeaponForge_MakeDetail(m);
            }
        } else if (pad & 0x3000) {
            m->cursor = Menu_WrapUpDownSe(m->cursor, m->count, 2);
            WeaponForge_MakeDetail(m);
        } else if (pad & 0x80) {
            if (m->mode != 0 && m->detailPages > 1) {
                if (++m->detailPage >= m->detailPages) {
                    m->detailPage = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->mode != 0 && m->detailPages > 1) {
                if (--m->detailPage < 0) {
                    m->detailPage = m->detailPages - 1;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x4000) {
            if (m->kind != 7 && (m->mode == 1 || m->mode == 2)) {
                if (m->unk110 == 0) {
                    m->unk110 = 1;
                    EquipPreview_Show(1);
                } else {
                    m->unk110 = 0;
                    memset(&tmp, 0, sizeof(tmp));
                    tmp.used = 1;
                    type = &tmp.type;
                    id = &tmp.id;
                    /* (u32): one index unsigned, as the registers of the original show */
                    *type = gWeaponRecipes[m->list[(u32)cur]].type;
                    *id = gWeaponRecipes[m->list[cur]].id;
                    if (*type != m->equip.type || *id != m->equip.id) {
                        m->equip = tmp;
                        if (Forge_IsFixedPrice(*id) == 0) {
                            StageModels_Free();
                            x = gSave.gender;
                            StageModels_LoadEquip(*type, Equip_GetModelId(*type, *id, x), x);
                        }
                    }
                    EquipPreview_Show(0);
                }
            }
        }
        break;
    case 3:
        if (pad & 0x80) {
            if (m->detailPages > 1) {
                if (++m->detailPage >= m->detailPages) {
                    m->detailPage = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->detailPages > 1) {
                if (--m->detailPage < 0) {
                    m->detailPage = m->detailPages - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x1B)) {
            case 0:
                m->unk10D = 0;
                m->detailPage = 0;
                WinSlide_Step(&m->win, 0x280, 0x118);
                if (Forge_IsFixedPrice(gWeaponRecipes[m->list[cur]].id)) {
                    WeaponForge_RollRandom(&m->orig, gWeaponRecipes[m->list[cur]].key);
                }
                return 0;
            case 3:
                m->unk10D = 2;
                m->unk110 = 1;
                EquipPreview_Show(1);
                break;
            }
        }
        break;
    }
    return 2;
}

void Rand_StepSaved(void);
typedef struct RandEquip {
    /* 0x0 */ u16 key;
    /* 0x2 */ s8 type;
    /* 0x3 */ u8 pad3;
    /* 0x4 */ s16 id[3]; /* chances 5%, 20%, 75% */
} RandEquip;
extern RandEquip gRandomEquips[];
void WeaponForge_RollRandom(EquipItem *out, u16 key)
{
    RandEquip *e = gRandomEquips;
    u16 r = gSave.randSaved[5] % 100;
    u16 k = gRandomEquips[0].key;

    while (k != 0) {
        if (k == key) {
            memset(out, 0, sizeof(EquipItem));
            out->used = 1;
            out->type = e->type;
            if (r < 5) {
                out->id = e->id[0];
            } else if (r < 25) {
                out->id = e->id[1];
            } else {
                out->id = e->id[2];
            }
        }
        e++;
        k = e->key;
    }
    Rand_StepSaved();
}

s16 Box_GetPageCount(SaveData *save);
s32 WeaponForge_CanUpgrade(WeaponMenu *m, s16 i);
void WeaponForge_LoadGunMods(WeaponMenu *m, s32 idx);
void WeaponForge_LoadUpgrades(WeaponMenu *m);
extern WeaponUp gBladeUpgrades[];
extern WeaponUp gGunUpgrades[];
void WeaponForge_OpenUpgrades(WeaponMenu *m)
{
    s32 i;
    s32 j;

    m->cursor = 0;
    m->page = 0;
    m->pages = Box_GetPageCount(&gSave);
    m->col = 0;
    m->row = 0;
    m->mode = 0;
    WinSlide_Step(&m->win, 0x280, 0x12C);
    WinSlide_Step(&m->win2, -0x128, 10);
    for (i = 0; i < 500; i++) {
        if ((&gSave.box[i])->used != 0) {
            switch ((&gSave.box[i])->type) {
            case 6:
                m->kind = 6;
                m->table = &gBladeUpgrades[(&gSave.box[i])->id];
                break;
            case 7:
                if (WeaponForge_IsClass10(&gSave.box[i]) != 1) {
                    continue;
                }
                m->kind = 6;
                m->table = &gGunUpgrades[(&gSave.box[i])->id];
                break;
            default:
                continue;
            }
            for (j = 0; j < 4; j++) {
                if (m->table->next[j] != 0) {
                    WeaponForge_CanUpgrade(m, j);
                }
            }
        }
    }
    /* the loop counter reused for the box index (as the registers show) */
    i = m->row * 10 + m->page * 100 + m->col;
    switch (gSave.box[i].type) {
    case 6:
        m->kind = 6;
        break;
    case 7:
        if (WeaponForge_IsClass10(&gSave.box[i]) == 1) {
            m->kind = 6;
        } else {
            m->kind = 7;
            WeaponForge_LoadGunMods(m, i);
        }
        break;
    }
    if (m->kind == 6) {
        if (WeaponForge_IsClass10(&gSave.box[i]) == 1) {
            m->table = &gGunUpgrades[gSave.box[i].id];
        } else {
            m->table = &gBladeUpgrades[gSave.box[i].id];
        }
    }
    WeaponForge_LoadUpgrades(m);
}

s32 WeaponForge_BoxInput(WeaponMenu *m, u16 pad);
s32 WeaponForge_UpgradeListInput(WeaponMenu *m, u16 pad);
void WeaponForge_UpgradeDetail(WeaponMenu *m);
s32 WeaponForge_UpgradeInput(WeaponMenu *m, u16 pad)
{
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    switch (m->unk10D) {
    case 0:
        WinSlide_Step(&m->win2, -0x128, 10);
        switch (WinSlide_Step(&m->win, 0x280, 0x12C)) {
        case 0:
            m->unk10D++;
            break;
        }
        break;
    case 1:
        switch (WeaponForge_BoxInput(m, pad)) {
        case 0:
            m->unk10D++;
            m->detailPage = 0;
            m->cursor = 0;
            m->mode = 0;
            WeaponForge_UpgradeDetail(m);
            break;
        case 3:
            m->unk10D = 0;
            return 3;
        }
        break;
    case 2:
        switch (WeaponForge_UpgradeListInput(m, pad)) {
        case 0:
            m->unk10D++;
            m->detailPage = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            EquipPreview_Show(1);
            m->mode = 2;
            WeaponForge_UpgradeDetail(m);
            break;
        case 3:
            m->unk10D = 1;
            m->mode = 0;
            m->detailPage = 0;
            m->cursor = 0;
            EquipPreview_Show(1);
            break;
        }
        break;
    case 3:
        if (pad & 0x80) {
            if (m->detailPages > 1) {
                if (++m->detailPage >= m->detailPages) {
                    m->detailPage = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->detailPages > 1) {
                if (--m->detailPage < 0) {
                    m->detailPage = m->detailPages - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x1B)) {
            case 0:
                m->unk10D = 0;
                m->detailPage = 0;
                WinSlide_Step(&m->win2, -0x128, 10);
                WinSlide_Step(&m->win, 0x280, 0x12C);
                return 0;
            case 3:
                m->unk10D--;
                m->unk110 = 1;
                EquipPreview_Show(1);
                break;
            }
        }
        break;
    }
    return 2;
}

u16 WeaponForge_GunModBits(u16 flags, u16 ev);
s32 Box_GetGunLevel(SaveData *save, s16 idx);
s32 Box_TestGunOption(SaveData *save, s16 idx, s32 mask);
u32 Equip_GetPrice(u8 type, u16 id);
extern s32 gGunModCostRates[];





void WeaponForge_LoadGunMods(WeaponMenu *m, s32 idx)
{
    s32 price;
    s16 n;
    u8 cls;
    s32 lv;
    s32 i;

    m->orig = gSave.box[idx];
    m->orig.used = 1;
    m->unk10A = m->orig.level;
    m->orig.level = WeaponForge_GunModBits(m->unk10A, m->msg[m->cursor]);
    if (Box_GetGunLevel(&gSave, idx) >= 4) {
        m->count = 3;
    } else {
        m->count = 4;
    }
    cls = Weapon_GetClass(m->orig.type, m->orig.id);
    n = 0;
    price = Equip_GetPrice(7, gSave.box[idx].id);
    m->msg[0] = -1;
    m->msg[1] = -1;
    m->msg[2] = -1;
    m->msg[3] = -1;
    lv = Box_GetGunLevel(&gSave, idx);
    if (lv < 4) {
        m->gunCost[n] = price / 10 * gGunModCostRates[lv];
        m->msg[n] = 0x13;
        n++;
    }
    if (cls == 1) {
        if (Box_TestGunOption(&gSave, idx, 0x80) == 1) {
            i = n;
            m->gunCost[i] = 10;
            m->msg[i] = 0x1B;
        } else {
            i = n;
            m->gunCost[i] = price / 10 * 3;
            m->msg[i] = 0x1A;
        }
    } else {
        if (Box_TestGunOption(&gSave, idx, 0x10) == 1) {
            i = n;
            m->gunCost[i] = 10;
            m->msg[i] = 0x15;
        } else {
            i = n;
            m->gunCost[i] = price / 10 * 2;
            m->msg[i] = 0x14;
        }
    }
    if (Box_TestGunOption(&gSave, idx, 0x20) == 1) {
        m->gunCost[(s16)(i + 1)] = 10;
        if (cls == 1) {
            m->msg[(s16)(i + 1)] = 0x1D;
        } else {
            m->msg[(s16)(i + 1)] = 0x17;
        }
    } else {
        m->gunCost[(s16)(i + 1)] = price / 10 * 3;
        if (cls == 1) {
            m->msg[(s16)(i + 1)] = 0x1C;
        } else {
            m->msg[(s16)(i + 1)] = 0x16;
        }
    }
    if (Box_TestGunOption(&gSave, idx, 0x40) == 1) {
        m->gunCost[(s16)(i + 2)] = 10;
        m->msg[(s16)(i + 2)] = 0x19;
    } else {
        m->gunCost[(s16)(i + 2)] = price / 10 * 3;
        m->msg[(s16)(i + 2)] = 0x18;
    }
}

u16 WeaponForge_GunModBits(u16 flags, u16 ev)
{
    u16 n;

    switch (ev) {
    case 0x13:
        n = flags & 0xF;
        if (n >= 4) {
            n = 4;
        } else {
            n++;
        }
        flags = (flags & 0xF0) | n;
        break;
    case 0x14:
        flags |= 0x10;
        flags &= ~0x20;
        break;
    case 0x15:
        flags &= ~0x10;
        break;
    case 0x16:
    case 0x1C:
        flags |= 0x20;
        flags &= ~0x90;
        break;
    case 0x17:
    case 0x1D:
        flags &= ~0x20;
        break;
    case 0x18:
        flags |= 0x40;
        break;
    case 0x19:
        flags &= ~0x40;
        break;
    case 0x1A:
        flags |= 0x80;
        flags &= ~0x20;
        break;
    case 0x1B:
        flags &= ~0x80;
        break;
    }
    return flags;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6E01D0", D_0082EA20);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6E01D0", D_0082EA28);

s32 WeaponForge_CheckWeapon(s32 kind, s32 id);
s32 EquipEnt_HalfPrice(u8 *e);
void *memcpy(void *dst, const void *src, u32 n);
void WeaponForge_LoadUpgrades(WeaponMenu *m)
{
    s32 idx;
    s32 i;
    u8 type;

    idx = m->row * 10 + m->page * 100 + m->col;
    type = gSave.box[idx].type;

    switch (type) {
    case 6:
        m->kind = 6;
        m->table = &gBladeUpgrades[gSave.box[idx].id];
        m->count = 0;
        for (i = 0; i < 4; i++) {
            if (m->table->next[i] != 0 && WeaponForge_CheckWeapon(type, m->table->next[i]) == 1) {
                memset(&m->upgrades[m->count], 0, sizeof(EquipItem));
                m->upgrades[m->count].type = type;
                m->upgrades[m->count].id = m->table->next[i];
                m->cost[m->count] = EquipEnt_HalfPrice((u8 *)&m->upgrades[m->count]);
                memcpy(&m->mats[m->count], &gBladeUpgrades[m->table->next[i]].mats, sizeof(WeaponMats));
                m->count++;
            }
        }
        m->orig = m->upgrades[m->cursor];
        break;
    case 7:
        if (WeaponForge_IsClass10(&gSave.box[idx]) == 1) {
            m->kind = 6;
            m->table = &gGunUpgrades[gSave.box[idx].id];
            m->count = 0;
            for (i = 0; i < 4; i++) {
                if (m->table->next[i] != 0 && WeaponForge_CheckWeapon(type, m->table->next[i]) == 1) {
                    memset(&m->upgrades[m->count], 0, sizeof(EquipItem));
                    m->upgrades[m->count].type = type;
                    m->upgrades[m->count].id = m->table->next[i];
                    m->cost[m->count] = EquipEnt_HalfPrice((u8 *)&m->upgrades[m->count]);
                    memcpy(&m->mats[m->count], &gGunUpgrades[m->table->next[i]].mats, sizeof(WeaponMats));
                    m->count++;
                }
            }
        } else {
            m->kind = 7;
            WeaponForge_LoadGunMods(m, idx);
        }
        break;
    default:
        m->kind = type;
        break;
    }
}

u8 Equip_LevelMark(EquipItem *a, u8 flags, EquipItem *b);
void WeaponForge_LoadUpgrades(WeaponMenu *m);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 WeaponForge_BoxInput(WeaponMenu *m, u16 pad)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;
    u16 *id;
    u8 *type;
    u8 x;

    memset(&tmp, 0, sizeof(tmp));
    if (pad & 0x20) {
        switch (m->kind) {
        case 6:
            if (m->count == 0) {
                LobbySe_Play(9);
                return 2;
            }
            m->cursor = 0;
            tmp = m->upgrades[m->cursor];
            m->orig = tmp;
            if (m->unk110 == 0) {
                type = &tmp.type;
                if (*type != m->equip.type || tmp.id != m->equip.id) {
                    m->equip = tmp;
                    id = &tmp.id;
                    if (Forge_IsFixedPrice(*id) == 0) {
                        StageModels_Free();
                        x = gSave.gender;
                        StageModels_LoadEquip(*type, Equip_GetModelId(*type, *id, x), x);
                    }
                }
                EquipPreview_Show(0);
            }
            m->detailPages = Equip_LevelMark(&gSave.box[idx], 0, &tmp);
            LobbySe_Play(0x1B);
            return 0;
        case 7:
            m->cursor = 0;
            WeaponForge_LoadGunMods(m, idx);
            LobbySe_Play(8);
            return 0;
        default:
            LobbySe_Play(9);
            break;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        if (m->mode == 0) {
            return 3;
        }
        m->mode = 0;
    } else if (pad & 0x200) {
        if (gSave.box[idx].id != 0) {
            m->mode ^= 1;
            m->detailPages = Equip_LevelMark(&gSave.box[idx], 0, NULL);
            m->detailPage = 0;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0x80) {
        if (m->mode != 0 && m->detailPages > 1) {
            if (++m->detailPage >= m->detailPages) {
                m->detailPage = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->mode != 0 && m->detailPages > 1) {
            if (--m->detailPage < 0) {
                m->detailPage = m->detailPages - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x10) {
        if (m->mode == 0 && m->pages > 1) {
            if (m->pages > 1) {
                if (++m->page >= m->pages) {
                    m->page = 0;
                }
                LobbySe_Play(1);
            }
        }
    } else if (pad & 0x4) {
        if (m->mode == 0 && m->pages > 1) {
            if (m->pages > 1) {
                if (--m->page < 0) {
                    m->page = m->pages - 1;
                }
                LobbySe_Play(1);
            }
        }
    } else if (pad & 0xC00) {
        if (m->mode == 0) {
            m->col = Menu_WrapLeftRight(m->col, 10);
        }
    } else {
        if (m->mode == 0) {
            m->row = Menu_WrapUpDown(m->row, 10);
        }
    }
    if (idx != m->row * 10 + m->page * 100 + m->col) {
        WeaponForge_LoadUpgrades(m);
    }
    return 2;
}

/* FAKEMATCH: the u32 k is only copied into t later (`t = k;`): `k = *type;` before the compare
 * puts the type's load where the game has it (from the permuter's second u32 copy). */
s32 WeaponForge_UpgradeListInput(WeaponMenu *m, u16 pad)
{
    u16 cur = m->cursor;
    EquipItem tmp;
    u8 *type;
    u8 x;
    u16 id;
    u8 t;
    u32 k;

    if (pad & 0x20) {
        if (WeaponForge_CanUpgrade(m, m->cursor) == 1) {
            LobbySe_Play(0x1B);
            return 0;
        }
        LobbySe_Play(9);
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        if (m->kind == 6) {
            if (++m->mode >= 4) {
                m->mode = 0;
            }
        } else if (m->mode == 0) {
            m->mode = 2;
        } else {
            m->mode = 0;
        }
        m->detailPage = 0;
        WeaponForge_UpgradeDetail(m);
        LobbySe_Play(0x39);
    } else if (pad & 0x80) {
        if (m->mode != 1 && m->detailPages > 1) {
            if (++m->detailPage >= m->detailPages) {
                m->detailPage = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->mode != 1 && m->detailPages > 1) {
            if (--m->detailPage < 0) {
                m->detailPage = m->detailPages - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x4000) {
        if (m->kind != 7 && (m->mode == 0 || m->mode == 1)) {
            if (m->unk110 == 0) {
                m->unk110 = 1;
                EquipPreview_Show(1);
            } else {
                m->unk110 = 0;
                tmp = m->upgrades[m->cursor];
                type = &tmp.type;
                k = *type;
                if (*type != m->equip.type || tmp.id != m->equip.id) {
                    m->equip = tmp;
                    id = tmp.id;
                    if (Forge_IsFixedPrice(id) == 0) {
                        StageModels_Free();
                        x = gSave.gender;
                        t = k;
                        StageModels_LoadEquip(t, Equip_GetModelId(t, id, x), x);
                    }
                }
                EquipPreview_Show(0);
            }
        }
    } else {
        m->cursor = Menu_WrapUpDownSe(m->cursor, m->count, 2);
    }
    if (cur != m->cursor) {
        if (m->kind == 7) {
            m->orig.level = WeaponForge_GunModBits(m->unk10A, m->msg[m->cursor]);
        } else {
            m->orig = m->upgrades[m->cursor];
        }
        WeaponForge_UpgradeDetail(m);
    }
    return 2;
}

s32 Items_HaveEnough(u16 item, s32 n);
s32 WeaponForge_CanUpgrade(WeaponMenu *m, s16 i)
{
    EquipItem tmp;
    s32 j;

    memset(&tmp, 0, sizeof(tmp));
    if (m->kind == 7) {
        if (m->gunCost[i] > gSave.money) {
            return 0;
        }
    } else {
        tmp = m->upgrades[i];
        if (Equip_TestFlag(&tmp, 1) == 0) {
            Equip_SetFlag(&tmp, 2);
            Equip_SetFlag(&tmp, 1);
        }
        if (m->cost[i] > (u32)gSave.money) {
            return 0;
        }
        for (j = 0; j < 3; j++) {
            if (Items_HaveEnough(m->mats[i].need[j].item, m->mats[i].need[j].count) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

u8 Equip_PreviewSkills(Player *player, EquipItem *e);
void WeaponForge_UpgradeDetail(WeaponMenu *m)
{
    EquipItem tmp;
    u8 *type;
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    u8 x;

    tmp = m->orig;
    switch (m->mode) {
    case 3:
        EquipPreview_Show(1);
        m->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &tmp);
        m->detailPage = 0;
        break;
    default:
        m->detailPages = Equip_LevelMark(&gSave.box[idx], 0x10, &tmp);
        EquipPreview_Show(1);
        break;
    case 2:
        m->detailPages = Equip_LevelMark(&gSave.box[idx], 0x10, &tmp);
        EquipPreview_Show(1);
        break;
    case 0:
    case 1:
        m->detailPages = Equip_LevelMark(&tmp, 0, NULL);
        if (m->kind == 6) {
            if (m->unk110 == 0) {
                type = &tmp.type;
                if (tmp.type != m->equip.type || tmp.id != m->equip.id) {
                    m->equip = tmp;
                    if (Forge_IsFixedPrice(tmp.id) == 0) {
                        StageModels_Free();
                        x = gSave.gender;
                        StageModels_LoadEquip(*type, Equip_GetModelId(*type, tmp.id, x), x);
                    }
                }
                EquipPreview_Show(0);
            }
        } else {
            EquipPreview_Show(1);
        }
        break;
    }
}

void WeaponForge_MakeDetail(WeaponMenu *m)
{
    s32 cur = m->cursor;
    EquipItem tmp;
    ForgeRecipe *r;
    u16 *id;
    u8 *type;
    u8 x;

    memset(&tmp, 0, sizeof(tmp));
    type = &tmp.type;
    id = &tmp.id;
    r = &gWeaponRecipes[m->list[cur]];
    *type = r->type;
    *id = r->id;
    switch (m->mode) {
    case 4:
        EquipPreview_Show(1);
        m->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &tmp);
        m->detailPage = 0;
        break;
    default:
        m->detailPages = Equip_LevelMark(gSave.equip, 0x10, &tmp);
        EquipPreview_Show(1);
        break;
    case 3:
        m->detailPages = Equip_LevelMark(gSave.equip, 0x50, &tmp);
        EquipPreview_Show(1);
        break;
    case 1:
    case 2:
        if (m->mode == 2) {
            m->detailPages = 0;
        } else {
            m->detailPages = Equip_LevelMark(&tmp, 0x40, NULL);
        }
        if (m->unk110 == 0) {
            if (*type != m->equip.type || *id != m->equip.id) {
                m->equip = tmp;
                if (Forge_IsFixedPrice(*id) == 0) {
                    StageModels_Free();
                    x = gSave.gender;
                    StageModels_LoadEquip(*type, Equip_GetModelId(*type, *id, x), x);
                }
            }
            EquipPreview_Show(0);
        }
        break;
    }
    if (m->detailPages > 0) {
        if (m->detailPage >= m->detailPages) {
            m->detailPage = m->detailPages - 1;
        }
    } else {
        m->detailPage = 0;
    }
}

void WeaponForge_DrawMaterials(WeaponMenu *m, EquipItem *e, s32 x, s32 y)
{
    WeaponForge_IsClass10(e);
    ArmorUpgrade_DrawMaterials(x, y, m->mats[m->cursor].need, 3);
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), WeaponForge_DrawHint 43 of 51 words, WeaponForge_DrawUpgradeBox 262 of 265 words differ */
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(const char *fmt, ...);
extern char *gDecoMakeTexts[];
void WeaponForge_DrawHint(WeaponMenu *m, s16 x, s16 y)
{
    s32 k;

    Win_Draw(x, y, 0x132, 0x72, 0xBDB59F6C, 0xA0202020, 6);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x + 8, y + 4);
    k = m->msg[m->cursor] + 11;
    Text_Printf(gDecoMakeTexts[k]);
}

void EquipWindow_DrawLocal(EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other);
void Equip_DrawPreviewHelp(u32 color, s8 k);
void EquipCompare_Draw(EquipItem *a, EquipItem *b, s32 c, s16 y, u8 flags);
void EquipDetail_Draw(Player *a, s16 x, s16 y, u32 flags);
void BoxMenu_DrawItemList(s16 cur, u32 flags, f32 fx);
char *Text_FillDollar(char *fmt, s32 n);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
void Font_SetColor(s32 color);
void Text_PrintfColored(const char *fmt, ...);
extern char *D_0080C8F4;
extern char *gUpgradeKindTexts[];
extern char *D_0080C804;
extern char *D_0080C81C;
extern char *D_0080C894;
extern char *gModFeeSilencerTexts[];
extern char *D_0080C89C;
extern char *gModFeeShieldTexts[];
extern char *D_0080C8A4;
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;
void WeaponForge_DrawDetail(WeaponMenu *m, s32 x, s16 y)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;
    char *s;

    tmp = gSave.box[idx];
    if (m->kind == 6) {
        tmp = m->upgrades[m->cursor];
        if (tmp.id != 0) {
            tmp.used = 1;
        } else {
            tmp.used = 0;
        }
    } else {
        tmp = m->orig;
    }
    switch (m->unk10D) {
    case 2:
        switch (m->mode) {
        case 0:
            if (m->kind != 6) {
                EquipWindow_DrawLocal(&tmp, 0x136, y + 4, m->detailPage | 0x40, NULL);
                WeaponForge_DrawHint(m, 0x136, 0xE0);
            } else {
                if (m->unk110 == 0) {
                    EquipPreview_Show(0);
                }
                Equip_DrawPreviewHelp(0xBDB59F6C, m->unk110);
                if (m->unk110 == 0 && Forge_IsFixedPrice(m->equip.id)) {
                    s = D_0080C8F4;
                    EquipPreview_Show(1);
                    Font_SetSpacing(0x12, 0x12);
                    Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                    Font_SetColor(0x807F212E);
                    Text_PrintfColored(s);
                }
                EquipWindow_DrawLocal(&tmp, 0x136, 0xE0, m->detailPage | 0x40, NULL);
            }
            break;
        case 2:
            EquipCompare_Draw(&gSave.box[idx], &tmp, 0x136, y + 4, m->detailPage | 0x50);
            break;
        case 1:
            if (m->unk110 == 0) {
                EquipPreview_Show(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, m->unk110);
            if (m->unk110 == 0 && Forge_IsFixedPrice(m->equip.id)) {
                s = D_0080C8F4;
                EquipPreview_Show(1);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(s);
            }
            WeaponForge_DrawMaterials(m, &tmp, 0x136, 0xE0);
            break;
        case 3:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, 0x3A, m->detailPage);
            return;
        }
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, gUpgradeKindTexts[0]);
        break;
    case 3:
        EquipCompare_Draw(&gSave.box[idx], &tmp, 0x136, y + 4, m->detailPage | 0x50);
        if (m->kind == 6) {
            s = Text_FillDollar(D_0080C81C, m->cost[m->cursor]);
        } else {
            switch (m->msg[m->cursor]) {
            case 21:
                s = Text_FillDollar(gModFeeSilencerTexts[0], m->gunCost[m->cursor]);
                break;
            case 23:
                s = Text_FillDollar(D_0080C89C, m->gunCost[m->cursor]);
                break;
            case 27:
                s = Text_FillDollar(gModFeeShieldTexts[0], m->gunCost[m->cursor]);
                break;
            case 29:
                s = Text_FillDollar(D_0080C8A4, m->gunCost[m->cursor]);
                break;
            default:
                s = Text_FillDollar(D_0080C894, m->gunCost[m->cursor]);
                break;
            }
        }
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
        Font_SetPos(0x1BC, 0x190);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    default:
        if (m->mode == 0) {
            BoxMenu_DrawItemList(idx, 0x410, x);
        } else {
            BoxMenu_DrawItemList(idx, (m->detailPage & 7) | 0x478, x);
        }
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, D_0080C804);
        break;
    }
}

void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
extern char D_0082EA20[];
char *Equip_GetName(u8 type, u16 id);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void WeaponForge_DrawUpgrades(WeaponMenu *m, s16 x, s16 y)
{
    EquipItem *e;
    u32 cost;
    s16 i;
    s16 ty = y;

    if (m->table->next[0] == 0 || m->count == 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(Text_CenterX(gDecoMakeTexts[3], x + 0x8A, 0x11), y + 0x33);
        Text_PrintfColored(gDecoMakeTexts[3]);
        return;
    }
    ty += 10;
    Font_SetPos(x + 10, ty);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gDecoMakeTexts[12]);
    for (i = 0; i < 4; i++) {
        if (i < m->count) {
            e = &m->upgrades[i];
            cost = m->cost[i];
            if (WeaponForge_CanUpgrade(m, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x807F7F7F);
            }
            ty += 20;
            Font_SetPos(x + 14, ty);
            Text_PrintfColored(Equip_GetName(e->type, e->id));
            Font_SetPos(x + 200, ty);
            Text_PrintfColored(D_0082EA20, cost);
            if (Equip_TestFlag(e, 2) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw(x, ty - 4, 0x10, 0x14, 0xFFC03000, 0x63);
            }
        } else {
            Font_SetColor(0x8042352E);
            ty += 20;
            Font_SetPos(x + 14, ty);
            Text_PrintfColored(gDecoMakeTexts[14]);
        }
        if (m->cursor == i && m->unk10D == 2) {
            Menu_DrawGlowBar(x + 138, ty - 1, 0x114, 0x12, 0xB05050F0);
        }
    }
}

void WeaponForge_DrawGunMods(WeaponMenu *m, s32 idx, s16 x, s16 y)
{
    s16 ty = y;
    s16 i;

    ty += 10;
    Font_SetPos(x + 10, ty);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gDecoMakeTexts[13]);
    Equip_GetPrice(7, gSave.box[idx].id);
    for (i = 0; i < 4; i++) {
        if (m->msg[i] != -1) {
            if (m->gunCost[i] > gSave.money) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x807F7F7F);
            }
            ty += 20;
            Font_SetPos(x + 14, ty);
            Text_PrintfColored(gDecoMakeTexts[m->msg[i]]);
            Font_SetPos(x + 200, ty);
            Text_PrintfColored(D_0082EA20, m->gunCost[i]);
            if (m->cursor == i && m->unk10D == 2) {
                Menu_DrawGlowBar(x + 138, ty - 1, 0x114, 0x12, 0xB05050F0);
            }
        }
    }
}

extern char D_0082EA28[];
extern char *gWeaponForgeTexts[];
extern char *D_0080C09C;
extern char *gCrossMarkTexts;
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, EquipItem *e) (lobby/unk_78F070.c), WeaponForge_DrawUpgradeBox 61 of 262 words differ */
s32 Box_CountEquip(u8 type, u16 id);
void WeaponForge_DrawUpgradeBox(WeaponMenu *m, s16 x, s16 y)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;
    u32 c1;
    u32 c2;
    u8 *type;
    s32 ly;
    s32 lx;
    u16 *id;

    memset(&tmp, 0, sizeof(tmp));
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(10, 0x12, 0xA0, 0xFFB0B000, gWeaponForgeTexts[m->choice], 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->unk10D) {
    case 2:
        c1 = 0xBDB59F6C;
        c2 = 0xA0202020;
        break;
    default:
        c1 = 0xFF202020;
        c2 = 0xA0000000;
        break;
    }
    switch (gSave.box[idx].type) {
    default:
        return;
    case 6:
    case 7:
        Font_SetSpacing(0x11, 0x11);
        Win_Draw(x, y, 0x114, 0xAE, c1, c2, 6);
        switch (m->kind) {
        case 6:
            if (WeaponForge_IsClass10(&gSave.box[idx]) == 1) {
                m->table = &gGunUpgrades[gSave.box[idx].id];
            } else {
                m->table = &gBladeUpgrades[gSave.box[idx].id];
            }
            WeaponForge_DrawUpgrades(m, x, y);
            tmp = m->upgrades[m->cursor];
            if (m->table->next[0] == 0 || m->count == 0) {
                return;
            }
            break;
        case 7:
            tmp = m->orig;
            WeaponForge_DrawGunMods(m, idx, x, y);
            break;
        default:
            return;
        }
        ly = y;
        lx = x;
        Gfx_DrawLine(lx + 8, ly + 0x74, 0x104, 2, c1);
        id = &tmp.id;
        if (*id != 0) {
            Lobby_DrawEquipIcon(lx + 10, ly + 0x76, 0x30, 0x30, &tmp);
            Font_SetSpacing(0x12, 0x12);
            Font_SetColor(0x807F7F7F);
            Font_SetPos(lx += 0x46, ly + 0x7C);
            type = &tmp.type;
            Text_PrintfColored(D_0082EA28, Equip_GetName(*type, *id));
            Font_SetPos(lx, ly + 0x90);
            Text_PrintfColored("%s%s%3d", D_0080C09C, gCrossMarkTexts, Box_CountEquip(*type, *id));
        }
        break;
    }
}

/* unk_6E01D0's .data (0x7E9360-0x7E93E0) */
s32 gGunModCostRates[4] = {
#include "lobby/007E9360.inc"
};
RandEquip gRandomEquips[11] = {
#include "lobby/007E9370.inc"
};
