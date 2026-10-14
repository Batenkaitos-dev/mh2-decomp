#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "scene.h"
#include "player.h"
#include "lobby_menu.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4


typedef struct TradeNeed {
    u16 item;
    u8 count;
    u8 cond; /* a condition (Forge_CheckCondition) */
} TradeNeed;
typedef struct Trade {
    u16 product;
    TradeNeed need[4];
} Trade;
typedef struct ForgeRecipe {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 pad1;
    /* 0x02 */ u16 id;
    /* 0x04 */ ItemStack need[4]; /* the materials */
    /* 0x14 */ u8 unk14[4];
} ForgeRecipe;
/* the materials of a weapon upgrade */
typedef struct WeaponMats {
    /* 0x0 */ ItemStack need[3];
} WeaponMats;
/* the weapon workshop (task + 0xC; a copy of unk_6E01D0.c's): forging (the
   list at 0xD8) and upgrades (the upgrades of the chosen weapon at 0x2C..0xCC) */
typedef struct WeaponMenu {
    /* 0x000 */ EquipItem equip; /* the weapon being upgraded */
    /* 0x00C */ EquipItem orig;
    /* 0x018 */ u8 *table; /* the weapon's upgrade entry */
    /* 0x01C */ WinSlide win;
    /* 0x024 */ WinSlide win2;
    /* 0x02C */ s32 gunCost[4];
    /* 0x03C */ s32 msg[4];
    /* 0x04C */ EquipItem upgrades[4]; /* what the weapon can become */
    /* 0x07C */ u8 unk7C[0xC];
    /* 0x088 */ u32 cost[4];
    /* 0x098 */ u8 unk98[4];
    /* 0x09C */ WeaponMats mats[4];
    /* 0x0CC */ u8 unkCC[0xC];
    /* 0x0D8 */ s32 list[7]; /* a page of gWeaponRecipes indices */
    /* 0x0F4 */ s16 choice; /* 0 forging, 1 upgrades */
    /* 0x0F6 */ s16 unkF6;
    /* 0x0F8 */ s16 page; /* the box: page * 100 + row * 10 + col */
    /* 0x0FA */ s16 pages;
    /* 0x0FC */ s16 detailPage;
    /* 0x0FE */ s16 detailPages;
    /* 0x100 */ s16 col;
    /* 0x102 */ s16 row; /* the forge list: the tab */
    /* 0x104 */ s16 cursor;
    /* 0x106 */ s16 count;
    /* 0x108 */ s16 mode;
    /* 0x10A */ u8 unk10A[2];
    /* 0x10C */ s8 step; /* 0 the choice, 1 upgrades, 2 forging */
    /* 0x10D */ s8 unk10D;
    /* 0x10E */ s8 equipped; /* the weapon is worn */
    /* 0x10F */ s8 kind; /* the weapon's type: 6 blade, 7 gun */
    /* 0x110 */ s8 unk110;
    /* 0x111 */ s8 filter;
} WeaponMenu;

extern ForgeRecipe gArmorRecipes[];
extern ForgeRecipe gWeaponRecipes[];
s32 Forge_CheckRecipe(u16 which, s16 idx, s32 mode);
u16 Box_FindFreeSlot(void *unused0);
u32 Equip_HalfPrice(s32 type, s32 id);
s32 Forge_IsFixedPrice(s16 id);
extern Trade gForgeTrades[];
void Items_Take(s32 item, s32 n);
s8 Items_Give(s32 item, s32 n);
s32 Item_SellPrice(u16 item);
void LobbySe_Play(s32 se);
s32 Menu_PushNoticeOnce(s32);
u8 Notice_Push(u8, s16, s16);
void Save_AddMoney(s32 n);

s32 Deco_GetPrice(u16 deco);
s32 Equip_FreeSlots(EquipItem *e);
s32 Deco_FitsEquip(EquipItem *e, u16 deco);
struct DecoData *Item_GetDecoration(u16 item);
/* the calls cast to s32: this unit's code takes the value as s32 */
u32 Equip_GetPrice(u8, u16);
void func_006E01C0(WeaponMenu *m);
void func_006D8460(void *a);
void Chat_SetColor(s16 color);
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
u8 Weapon_GetClass(u8 type, u16 id);
s16 Box_GetPageCount(SaveData *save);
void *memset(void *dst, s32 c, u32 n);




/* the armour workshop (task + 0xC): production (0xF4..) and upgrades */
typedef struct ArmorMats {
    /* 0x00 */ ItemStack need[4];
    /* 0x10 */ u8 unk10[4];
} ArmorMats;
typedef struct ArmorMenu {
    /* 0x000 */ EquipItem shown; /* upgrades: the piece the model wears */
    /* 0x00C */ EquipItem worn; /* the piece of that type being worn */
    /* 0x018 */ EquipItem piece; /* upgrades: the box piece */
    /* 0x024 */ EquipItem sel; /* upgrades: the chosen upgrade */
    /* 0x030 */ EquipItem upgrades[4]; /* what the chosen piece can become */
    /* 0x060 */ ArmorMats mats[4];
    /* 0x0B0 */ s32 cost[4];
    /* 0x0C0 */ u8 unkC0[4];
    /* 0x0C4 */ WinSlide win;
    /* 0x0CC */ WinSlide win2;
    /* 0x0D4 */ u8 unkD4[0x20];
    /* 0x0F4 */ s32 list[7]; /* production: a page of gArmorRecipes indices */
    /* 0x110 */ s32 kind; /* upgrades: the box piece's type when it is a weapon */
    /* 0x114 */ s16 col; /* upgrades: the box, page * 100 + row * 10 + col */
    /* 0x116 */ s16 row; /* production: the armour type */
    /* 0x118 */ s16 cursor;
    /* 0x11A */ s16 count;
    /* 0x11C */ s16 detail; /* upgrades: the piece's details shown */
    /* 0x11E */ s16 page;
    /* 0x120 */ s16 pages;
    /* 0x122 */ s16 detailPage;
    /* 0x124 */ s16 detailPages;
    /* 0x126 */ s16 choice; /* 0 production, 1 upgrades */
    /* 0x128 */ u8 unk128[7];
    /* 0x12F */ s8 unk12F;
    /* 0x130 */ s8 state; /* upgrades */
    /* 0x131 */ s8 equipped; /* upgrades: the box piece is worn */
    /* 0x132 */ s8 unk132;
    /* 0x133 */ s8 filter; /* production: 0 recipes not in gRecipeIdPairs (Recipe_IsInList), 1 those in it */
} ArmorMenu;

/* the decorations counter (task + 0xC): make (choice 0), attach (1) or
   remove (2) decorations */
typedef struct DecoMenu {
    /* 0x00 */ EquipItem equip; /* the piece being edited */
    /* 0x0C */ EquipItem unkC;
    /* 0x18 */ WinSlide win;
    /* 0x20 */ WinSlide win2;
    /* 0x28 */ s32 recipes[7]; /* choice 0: a page of gForgeTrades indices */
    /* 0x44 */ u16 list[7];
    /* 0x52 */ s16 choice;
    /* 0x54 */ s16 row; /* the box: page * 100 + row * 10 + col */
    /* 0x56 */ s16 col;
    /* 0x58 */ s16 page;
    /* 0x5A */ s16 pages;
    /* 0x5C */ s16 unk5C;
    /* 0x5E */ s16 unk5E;
    /* 0x60 */ s16 cursor;
    /* 0x62 */ s16 count;
    /* 0x64 */ s16 decoPage;
    /* 0x66 */ s16 decoPages;
    /* 0x68 */ u8 unk68[4];
    /* 0x6C */ s32 quantity; /* choice 0: how many to make */
    /* 0x70 */ s8 mode;
    /* 0x71 */ s8 unk71;
    /* 0x72 */ s8 unk72;
    /* 0x73 */ s8 unk73;
    /* 0x74 */ s8 equipped; /* the piece is worn */
} DecoMenu;

void Deco_LoadRecipes(DecoMenu *m, s16 page);
s32 Deco_ConfirmInput(DecoMenu *m, u16 pad);
void Deco_DrawEquipAsk(DecoMenu *m, s16 k);

/* the online town's guide (in the menu work area gMenuWork): three groups
   of topics, each topic a list of entries shown when a condition holds */
typedef struct GuideEntry {
    /* 0x00 */ s32 cond; /* -1: none */
    /* 0x04 */ u16 cond2;
    /* 0x06 */ u8 unk6[2];
    /* 0x08 */ s32 msg; /* groups 0 and 2: a message; group 1: a page count */
    /* 0x0C */ s32 page0; /* group 1: the first page (gGuidePageTexts) */
    /* 0x10 */ char *title; /* NULL: end of the list */
} GuideEntry;
typedef struct GuideMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ char *topics[10];
    /* 0x30 */ GuideEntry *entries[10];
    /* 0x58 */ s16 group;
    /* 0x5A */ s16 topicPage;
    /* 0x5C */ s16 topicPages;
    /* 0x5E */ s16 topic;
    /* 0x60 */ s16 topicCount;
    /* 0x62 */ s16 entryPage;
    /* 0x64 */ s16 entryPages;
    /* 0x66 */ s16 entry;
    /* 0x68 */ s16 entryCount;
    /* 0x6A */ s16 page;
    /* 0x6C */ s16 pageCount;
    /* 0x6E */ s8 state;
    /* 0x6F */ u8 pad6F;
    /* 0x70 */ char *title; /* the chosen topic's (or entry's) title */
} GuideMenu;

extern u8 gMenuWork[];
extern char *gGuideBasicsTopics[];
extern char *gGuidePlaceTopics[];
extern char *gGuideDragonTopics[];
extern GuideEntry *gGuideBasicsEntries[];
extern GuideEntry *gGuidePlaceEntries[];
extern GuideEntry *gGuideDragonEntries[];
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_WrapLeftRight(s32 cursor, s32 count);
s32 Menu_CountPages(s16 n, s16 per);
s32 Cond_Check(u16 id);
s32 TownGuide_PageInput(GuideMenu *m, u16 pad);
s32 TownGuide_EntryInput(GuideMenu *m, u16 pad);
s32 TownGuide_TopicInput(GuideMenu *m, u16 pad);
void TownGuide_LoadEntries(GuideMenu *m, s16 page);
void TownGuide_LoadTopics(GuideMenu *m, s16 page);
s32 TownGuide_GroupInput(GuideMenu *m, u16 pad);

u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
void Talk_Say(s32 msg, s8);
s32 Talk_Update(void);
void TownGuide_Close(void);
/* unk_6D5330's .rodata (0x82CB98-0x82E900) */
#pragma rodata on
char D_0082CB98[0xC] = {
#include "lobby/0082CB98.inc"
};
char D_0082CBA8[0xC] = {
#include "lobby/0082CBA8.inc"
};
char D_0082CBC0[0x18] = {
#include "lobby/0082CBC0.inc"
};
char D_0082CBD8[13] = {
#include "lobby/0082CBD8.inc"
};
char D_0082CBE8[0xC] = {
#include "lobby/0082CBE8.inc"
};
char D_0082CBF8[0xC] = {
#include "lobby/0082CBF8.inc"
};
char D_0082CC08[15] = {
#include "lobby/0082CC08.inc"
};
char D_0082CC18[0xC] = {
#include "lobby/0082CC18.inc"
};
char D_0082CC30[0x20] = {
#include "lobby/0082CC30.inc"
};
char D_0082CC50[0x20] = {
#include "lobby/0082CC50.inc"
};
char D_0082CC70[0x18] = {
#include "lobby/0082CC70.inc"
};
char D_0082CC88[0xC] = {
#include "lobby/0082CC88.inc"
};
char D_0082CC98[0xC] = {
#include "lobby/0082CC98.inc"
};
char D_0082CCA8[0xC] = {
#include "lobby/0082CCA8.inc"
};
char D_0082CCB8[0xC] = {
#include "lobby/0082CCB8.inc"
};
char D_0082CCD0[0x20] = {
#include "lobby/0082CCD0.inc"
};
char D_0082CCF0[0x18] = {
#include "lobby/0082CCF0.inc"
};
char D_0082CD08[0xC] = {
#include "lobby/0082CD08.inc"
};
char D_0082CD18[0xC] = {
#include "lobby/0082CD18.inc"
};
char D_0082CD30[0x18] = {
#include "lobby/0082CD30.inc"
};
char D_0082CD48[0xC] = {
#include "lobby/0082CD48.inc"
};
char D_0082CD58[0xC] = {
#include "lobby/0082CD58.inc"
};
char D_0082CD68[0xC] = {
#include "lobby/0082CD68.inc"
};
char D_0082CD78[0xC] = {
#include "lobby/0082CD78.inc"
};
char D_0082CD88[0xC] = {
#include "lobby/0082CD88.inc"
};
char D_0082CD98[0xC] = {
#include "lobby/0082CD98.inc"
};
char D_0082CDB0[0x20] = {
#include "lobby/0082CDB0.inc"
};
char D_0082CDD0[0x18] = {
#include "lobby/0082CDD0.inc"
};
char D_0082CDE8[15] = {
#include "lobby/0082CDE8.inc"
};
char D_0082CDF8[0xC] = {
#include "lobby/0082CDF8.inc"
};
char D_0082CE10[0x18] = {
#include "lobby/0082CE10.inc"
};
char D_0082CE28[0xC] = {
#include "lobby/0082CE28.inc"
};
char D_0082CE38[0xC] = {
#include "lobby/0082CE38.inc"
};
char D_0082CE48[15] = {
#include "lobby/0082CE48.inc"
};
char D_0082CE60[0x20] = {
#include "lobby/0082CE60.inc"
};
char D_0082CE80[0x10] = {
#include "lobby/0082CE80.inc"
};
char D_0082CE90[0x10] = {
#include "lobby/0082CE90.inc"
};
char D_0082CEA0[0x10] = {
#include "lobby/0082CEA0.inc"
};
char D_0082CEB0[0x18] = {
#include "lobby/0082CEB0.inc"
};
char D_0082CEC8[0x8] = {
#include "lobby/0082CEC8.inc"
};
char D_0082CED0[0x10] = {
#include "lobby/0082CED0.inc"
};
char D_0082CEE0[0x10] = {
#include "lobby/0082CEE0.inc"
};
char D_0082CEF0[0x10] = {
#include "lobby/0082CEF0.inc"
};
char D_0082CF00[0x8] = {
#include "lobby/0082CF00.inc"
};
char D_0082CF08[0xC] = {
#include "lobby/0082CF08.inc"
};
char D_0082CF20[0xD0] = {
#include "lobby/0082CF20.inc"
};
char D_0082CFF0[0xA0] = {
#include "lobby/0082CFF0.inc"
};
char D_0082D090[0xB0] = {
#include "lobby/0082D090.inc"
};
char D_0082D140[0xD0] = {
#include "lobby/0082D140.inc"
};
char D_0082D210[0xB0] = {
#include "lobby/0082D210.inc"
};
char D_0082D2C0[0xD0] = {
#include "lobby/0082D2C0.inc"
};
char D_0082D390[0xE0] = {
#include "lobby/0082D390.inc"
};
char D_0082D470[0xF0] = {
#include "lobby/0082D470.inc"
};
char D_0082D560[0xB0] = {
#include "lobby/0082D560.inc"
};
char D_0082D610[0xC0] = {
#include "lobby/0082D610.inc"
};
char D_0082D6D0[0xA0] = {
#include "lobby/0082D6D0.inc"
};
char D_0082D770[0x90] = {
#include "lobby/0082D770.inc"
};
char D_0082D800[0xA0] = {
#include "lobby/0082D800.inc"
};
char D_0082D8A0[0xE0] = {
#include "lobby/0082D8A0.inc"
};
char D_0082D980[0xF0] = {
#include "lobby/0082D980.inc"
};
char D_0082DA70[0xE0] = {
#include "lobby/0082DA70.inc"
};
char D_0082DB50[0xD0] = {
#include "lobby/0082DB50.inc"
};
char D_0082DC20[0xA0] = {
#include "lobby/0082DC20.inc"
};
char D_0082DCC0[0xA0] = {
#include "lobby/0082DCC0.inc"
};
char D_0082DD60[0x90] = {
#include "lobby/0082DD60.inc"
};
char D_0082DDF0[0xA0] = {
#include "lobby/0082DDF0.inc"
};
char D_0082DE90[0x100] = {
#include "lobby/0082DE90.inc"
};
char D_0082DF90[0x90] = {
#include "lobby/0082DF90.inc"
};
char D_0082E020[0xC0] = {
#include "lobby/0082E020.inc"
};
char D_0082E0E0[0xF0] = {
#include "lobby/0082E0E0.inc"
};
char D_0082E1D0[0xC0] = {
#include "lobby/0082E1D0.inc"
};
char D_0082E290[0xB0] = {
#include "lobby/0082E290.inc"
};
char D_0082E340[0xC0] = {
#include "lobby/0082E340.inc"
};
char D_0082E400[0xB0] = {
#include "lobby/0082E400.inc"
};
char D_0082E4B0[0x100] = {
#include "lobby/0082E4B0.inc"
};
char D_0082E5B0[0xA0] = {
#include "lobby/0082E5B0.inc"
};
char D_0082E650[0xE0] = {
#include "lobby/0082E650.inc"
};
char D_0082E730[0xE0] = {
#include "lobby/0082E730.inc"
};
char D_0082E810[0x20] = {
#include "lobby/0082E810.inc"
};
char D_0082E830[0x20] = {
#include "lobby/0082E830.inc"
};
char D_0082E850[0x18] = {
#include "lobby/0082E850.inc"
};
char D_0082E868[0x8] = {
#include "lobby/0082E868.inc"
};
char D_0082E870[0x10] = {
#include "lobby/0082E870.inc"
};
char D_0082E880[0x10] = {
#include "lobby/0082E880.inc"
};
char D_0082E890[0x10] = {
#include "lobby/0082E890.inc"
};
char D_0082E8A0[0x10] = {
#include "lobby/0082E8A0.inc"
};
char D_0082E8B0[0x10] = {
#include "lobby/0082E8B0.inc"
};
char D_0082E8C0[0x18] = {
#include "lobby/0082E8C0.inc"
};
char D_0082E8D8[0xC] = {
#include "lobby/0082E8D8.inc"
};
char D_0082E8E8[0xC] = {
#include "lobby/0082E8E8.inc"
};
#pragma rodata off

void TownGuide_Update(void)
{
    GuideMenu *m = (GuideMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->state) {
    case 0:
        switch (TownGuide_GroupInput(m, pad)) {
        case 0:
            m->state++;
            WinSlide_Step(&m->win, 0x280, 0x140);
            m->topic = 0;
            TownGuide_LoadTopics(m, 0);
            break;
        case 3:
            m->state = 6;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x140)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 2:
        switch (TownGuide_TopicInput(m, pad)) {
        case 0:
            m->state++;
            m->entry = 0;
            TownGuide_LoadEntries(m, 0);
            break;
        case 3:
            m->state = 0;
            break;
        }
        break;
    case 3:
        switch (TownGuide_EntryInput(m, pad)) {
        case 0:
            switch (m->group) {
            case 0:
            case 2:
                m->state = 4;
                Talk_Say(m->entries[m->entry]->msg, 0);
                break;
            default:
                m->state = 5;
                m->page = 0;
                m->pageCount = m->entries[m->entry]->msg;
                break;
            }
            break;
        case 3:
            m->state = 2;
            break;
        }
        break;
    case 5:
        switch (TownGuide_PageInput(m, pad)) {
        case 0:
        case 3:
            m->state = 3;
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state = 3;
            break;
        }
        break;
    case 6:
        switch (Talk_Update()) {
        case 0:
        case 3:
            TownGuide_Close();
            break;
        }
        break;
    }
}

s32 TownGuide_PageInput(GuideMenu *m, u16 pad)
{
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 0;
    }
    m->page = Menu_WrapLeftRight(m->page, m->pageCount);
    return 2;
}

s32 TownGuide_EntryInput(GuideMenu *m, u16 pad)
{
    if (pad & 0x20) {
        LobbySe_Play(0x2E);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 3;
    }
    if (pad & 0xC00) {
        if (m->entryPages > 1) {
            m->entryPage = Menu_WrapLeftRight(m->entryPage, m->entryPages);
            TownGuide_LoadEntries(m, m->entryPage);
        }
    } else {
        m->entry = Menu_WrapUpDown(m->entry, m->entryCount);
    }
    return 2;
}

s32 TownGuide_TopicInput(GuideMenu *m, u16 pad)
{
    if (pad & 0x20) {
        LobbySe_Play(0x2E);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 3;
    }
    if (pad & 0xC00) {
        if (m->topicPages > 1) {
            m->topicPage = Menu_WrapLeftRight(m->topicPage, m->topicPages);
            TownGuide_LoadTopics(m, m->topicPage);
        }
    } else {
        m->topic = Menu_WrapUpDown(m->topic, m->topicCount);
    }
    return 2;
}

void TownGuide_LoadEntries(GuideMenu *m, s16 page)
{
    GuideEntry *e;
    s16 n;
    s16 lo;
    s16 hi;

    switch (m->group) {
    default:
        e = gGuideBasicsEntries[m->topic];
        break;
    case 1:
        e = gGuidePlaceEntries[m->topic];
        break;
    case 2:
        e = gGuideDragonEntries[m->topic];
        break;
    }
    m->entryCount = 0;
    n = 0;
    lo = page * 10;
    hi = lo + 9;
    for (; e->title != NULL; e++) {
        if ((e->cond != -1 && Cond_Check(e->cond) == 1) || Cond_Check(e->cond2) == 1) {
            if (n >= lo && n <= hi) {
                m->entries[m->entryCount] = e;
                m->entryCount++;
            }
            n++;
        }
    }
    m->entryPage = page;
    m->entryPages = Menu_CountPages(n, 10);
    if (m->entry >= m->entryCount) {
        m->entry = m->entryCount - 1;
    }
}

void TownGuide_LoadTopics(GuideMenu *m, s16 page)
{
    char **p;
    s16 n;
    s16 lo;
    s16 hi;

    switch (m->group) {
    default:
        p = gGuideBasicsTopics;
        break;
    case 1:
        p = gGuidePlaceTopics;
        break;
    case 2:
        p = gGuideDragonTopics;
        break;
    }
    m->topicCount = 0;
    n = 0;
    lo = page * 10;
    hi = lo + 9;
    for (; *p != NULL; p++) {
        if (n >= lo && n <= hi) {
            m->topics[m->topicCount] = *p;
            m->topicCount++;
        }
        n++;
    }
    m->topicPage = page;
    m->topicPages = Menu_CountPages(n, 10);
    if (m->topic >= m->topicCount) {
        m->topic = m->topicCount - 1;
    }
}

s32 TownGuide_GroupInput(GuideMenu *m, u16 pad)
{
    if (pad & 0x20) {
        LobbySe_Play(0x2E);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 3;
    }
    m->group = Menu_WrapUpDown(m->group, 3);
    return 2;
}

void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Text_Draw(char *s);
extern char *D_0080C28C;
extern char *gUpDownArrowTexts[];
s16 TownGuide_DrawHeading(char *title, s16 x, s16 y, s32 mode)
{
    s16 x0 = x;
    s16 tx;
    s32 i;

    Font_SetSpacing(20, 20);
    if (mode == 0) {
        tx = x0 + 28;
    } else {
        x += 150;
        tx = Text_CenterX(title, x, 20);
    }
    Font_SetPos(tx, y + 24);
    Text_PrintfColored(title);
    if (mode == 0) {
        y += 48;
    } else {
        y += 28;
    }
    Font_SetSpacing(14, 14);
    if (mode == 1) {
        x = x0;
    }
    x += 20;
    for (i = 0; i < 9; i++) {
        Font_SetPos(x, y + 22);
        if (mode == 0) {
            Text_Draw(D_0080C28C);
        } else {
            Text_Draw(gUpDownArrowTexts[0]);
        }
        x += 28;
    }
    return y;
}

void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Font_SetColor(s32 color);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
extern char *gTownBoardTexts[];
extern char *gTownBoardHelpTexts[];
void TownGuide_DrawTopics(GuideMenu *m, s16 x, s16 y)
{
    char *s;
    s16 tx;
    s16 ty;
    s16 i;
    s16 cx;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x140, 0x12, 0xF0, 0xFFB0B000, gTownBoardTexts[m->group], 2, 1);
    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 2);
    Font_SetColor(0x80101010);
    if (m->topicPages > 1) {
        Menu_DrawPageNum(x + 0xFA, y + 8, m->topicPage, m->topicPages, 1);
    }
    ty = TownGuide_DrawHeading(gTownBoardHelpTexts[m->group], x, y, 0);
    Font_SetSpacing(18, 18);
    tx = x + 150;
    ty += 20;
    for (i = 0; i < m->topicCount; i++) {
        s = m->topics[i];
        cx = Text_CenterX(s, tx, 18);
        ty += 22;
        Font_SetPos(cx, ty);
        Text_PrintfColored(s);
        if (m->topic == i) {
            Menu_DrawArrowsRev(cx, ty, 18, s, 0xFF106060);
            m->title = s;
        }
    }
}

void TownGuide_DrawEntries(GuideMenu *m, s16 x, s16 y)
{
    GuideEntry *e;
    s16 tx;
    s16 ty;
    s16 i;
    s16 cx;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x140, 0x12, 0xF0, 0xFFB0B000, gTownBoardTexts[m->group], 2, 1);
    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    if (m->entryPages > 1) {
        Menu_DrawPageNum(x + 0xFA, y + 8, m->entryPage, m->entryPages, 1);
    }
    ty = TownGuide_DrawHeading(m->title, x, y, 1);
    Font_SetSpacing(18, 18);
    tx = x + 150;
    ty += 20;
    for (i = 0; i < m->entryCount; i++) {
        e = m->entries[i];
        cx = Text_CenterX(e->title, tx, 18);
        ty += 22;
        Font_SetPos(cx, ty);
        Text_PrintfColored(e->title);
        if (m->entry == i) {
            Menu_DrawArrowsRev(cx, ty, 18, e->title, 0xFF106060);
        }
    }
}

void Text_PrintfStyled(s32 a, const char *fmt, ...);
extern char *gGuidePageTexts[];
void TownGuide_DrawPage(GuideMenu *m, s16 x, s16 y)
{
    GuideEntry *e;
    s32 ty;
    s32 tx;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x140, 0x12, 0xF0, 0xFFB0B000, gTownBoardTexts[m->group], 2, 1);
    Icon_Draw(x, y, 0x12C, 0x154, 0xFFE0D8C8, 1);
    Font_SetColor(0x80101010);
    if (m->pageCount > 1) {
        Menu_DrawPageNum(x + 0xDC, y + 8, m->page, m->pageCount, 1);
    }
    e = m->entries[m->entry];
    Font_SetSpacing(18, 18);
    ty = y + 40;
    tx = x;
    Font_SetPos(tx + 40, ty);
    Text_PrintfStyled(4, "%s", gGuidePageTexts[e->page0 + m->page]);
    switch (e->page0 + m->page) {
    case 10:
        Icon_Draw(tx + 0xAA, ty, 0x78, 0x78, 0xA0000000, 0x25);
        break;
    case 11:
        Icon_Draw(tx + 0xA0, ty, 0x87, 0x87, 0xA0000000, 0x22);
        break;
    case 12:
        Icon_Draw(tx + 0x78, ty, 0xAA, 0x7F, 0xA0000000, 0x21);
        break;
    case 17:
    case 18:
        Icon_Draw(tx + 0xAA, ty, 0x64, 0x96, 0xA0000000, 0x24);
        break;
    case 19:
    case 20:
        Icon_Draw(tx + 0xA0, ty, 0x78, 0x78, 0xA0000000, 0x26);
        break;
    }
}

void Text_SetSize(s32 size);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void TownGuide_DrawTopics(GuideMenu *m, s16 x, s16 y);
void TownGuide_DrawEntries(GuideMenu *m, s16 x, s16 y);
void TownGuide_DrawPage(GuideMenu *m, s16 x, s16 y);
void TownGuide_Draw(u8 *task)
{
    GuideMenu *m = (GuideMenu *)gMenuWork;

    Text_SetSize(*(s32 *)(task + 0x18));
    switch (m->state) {
    case 0:
        Menu_DrawLabelList(0x1C2, gTownBoardTexts, 3, m->group, 0xFFB0B000);
        break;
    case 1:
    case 2:
        TownGuide_DrawTopics(m, m->win.pos, 0x3A);
        break;
    case 3:
        TownGuide_DrawEntries(m, m->win.pos, 0x3A);
        break;
    case 5:
        TownGuide_DrawPage(m, m->win.pos, 0x3A);
        break;
    }
}

void ArmorForge_Open(s8 *task)
{
    s8 *m;

    task[0x13E] = 1;
    task[0x13F] = task[3];
    m = task + 0xC;
    if (task[0x13F] != 0) {
        m[0x132] = 1;
        *(s16 *)(m + 0x116) = 0;
        *(s16 *)(m + 0x126) = 0;
        m[0x12F] = 2;
        func_006D8460(m);
        Chat_SetColor(0);
    }
}

void ArmorForge_LoadBox(ArmorMenu *m);
void ArmorForge_AddUpgrade(ArmorMenu *m, s32 i);
s32 ArmorForge_UpgradeInput(ArmorMenu *m, u16 pad);
s32 ArmorForge_MakeInput(ArmorMenu *m, u16 pad);
void CamFocus_SetAside(s8 arg0);
void EquipPreview_Reset(void);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 se);
s32 ArmorForge_Input(u8 *task, u16 pad)
{
    ArmorMenu *m = (ArmorMenu *)(task + 0xC);

    switch (m->unk12F) {
    case 0:
        if (pad & 0x20) {
            m->unk132 = 1;
            m->row = 0;
            if (m->choice == 0) {
                m->unk12F = 2;
                func_006D8460(m);
            } else {
                m->unk12F = 1;
                ArmorForge_LoadBox(m);
                ArmorForge_AddUpgrade(m, 0);
                CamFocus_SetAside(1);
                EquipPreview_Reset();
            }
            Chat_SetColor(0);
            LobbySe_Play(0x1B);
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else {
            m->choice = Menu_WrapUpDownSe(m->choice, 2, 2);
        }
        break;
    case 1:
        switch (ArmorForge_UpgradeInput(m, pad)) {
        case 0:
            m->unk12F = 0;
            return 0;
        case 3:
            m->unk12F = 0;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case 2:
        switch (ArmorForge_MakeInput(m, pad)) {
        case 0:
            m->unk12F = 0;
            return 0;
        case 3:
            if (m->filter != 0) {
                Chat_SetColor(-1);
                return 3;
            }
            m->unk12F = 0;
            Chat_SetColor(-1);
            break;
        }
        break;
    }
    return 2;
}

void ArmorForge_LoadBox(ArmorMenu *m)
{
    s32 i;

    m->page = 0;
    m->pages = Box_GetPageCount(&gSave);
    m->col = 0;
    m->row = 0;
    m->detail = 0;
    for (i = 0; i < 500; i++) {
        if (gSave.box[i].used != 0) {
            switch (gSave.box[i].type) {
            case 6:
            case 7:
                break;
            default:
                ArmorForge_AddUpgrade(m, i);
                break;
            }
        }
    }
    WinSlide_Step(&m->win, 0x280, 0x12C);
    WinSlide_Step(&m->win2, -0x128, 10);
}

u8 Equip_LevelMark(EquipItem *a, u8 flags, EquipItem *b);
s32 ArmorForge_BoxInput(ArmorMenu *m, u16 pad)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    s32 now;

    if (pad & 0x20) {
        switch (gSave.box[idx].type) {
        case 6:
        case 7:
            LobbySe_Play(9);
            break;
        default:
            if (m->count != 0) {
                LobbySe_Play(0x1B);
                return 0;
            }
            LobbySe_Play(9);
            break;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        if (m->detail == 0) {
            return 3;
        }
        m->detail = 0;
    } else if (pad & 0x200) {
        if (gSave.box[idx].id != 0) {
            m->detail ^= 1;
            LobbySe_Play(0x39);
            m->detailPage = 0;
            m->detailPages = Equip_LevelMark(&gSave.box[idx], 0, NULL);
        }
    } else if (pad & 0x80) {
        if (m->detail != 0 && m->detailPages > 1) {
            if (++m->detailPage >= m->detailPages) {
                m->detailPage = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->detail != 0 && m->detailPages > 1) {
            if (--m->detailPage < 0) {
                m->detailPage = m->detailPages - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x10) {
        if (m->detail == 0 && m->pages > 1) {
            if (m->pages > 1) {
                if (++m->page >= m->pages) {
                    m->page = 0;
                }
                LobbySe_Play(1);
            }
        }
    } else if (pad & 0x4) {
        if (m->detail == 0 && m->pages > 1) {
            if (m->pages > 1) {
                if (--m->page < 0) {
                    m->page = m->pages - 1;
                }
                LobbySe_Play(1);
            }
        }
    } else if (pad & 0xC00) {
        if (m->detail == 0) {
            m->col = Menu_WrapLeftRight(m->col, 10);
        }
    } else {
        if (m->detail == 0) {
            m->row = Menu_WrapUpDown(m->row, 10);
        }
    }
    now = m->row * 10 + m->page * 100 + m->col;
    if (idx != now) {
        switch (gSave.box[now].type) {
        case 6:
        case 7:
            m->kind = gSave.box[now].type;
            break;
        default:
            ArmorForge_AddUpgrade(m, now);
            break;
        }
    }
    return 2;
}

s32 ArmorUpgrade_Unseen(EquipItem *e);
s32 ArmorUpgrade_Materials(EquipItem *e, ItemStack *out, s8 *cond);
s32 ArmorUpgrade_Unlocks(EquipItem *e, u16 *out);
s32 ArmorUpgrade_FirstMaterials(EquipItem *e, ItemStack *out, s8 *cond);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 Forge_CheckCondition(s16 cond);
s32 Armor_GetUpgradeCost(EquipItem *e);
void *memcpy(void *dst, const void *src, u32 n);
u8 Equip_GetWearMask(SaveData *save, EquipItem *item);
s32 Equip_TestFlag(EquipItem *e, s16 bit);
void Equip_SetFlag(EquipItem *e, s16 bit);
void Equip_CopyEquipped(EquipItem *out, s16 type);
void ArmorForge_AddUpgrade(ArmorMenu *m, s32 idx)
{
    s8 cond[4];
    EquipItem tmp;
    u16 list[4];
    ItemStack need[6];
    s32 bad;
    s32 i;
    s32 j;
    s32 n;
    s32 k;

    memset(&tmp, 0, sizeof(tmp));
    memset(m->upgrades, 0, sizeof(m->upgrades));
    memset(m->cost, 0, sizeof(m->cost));
    memset(m->unkC0, 0, sizeof(m->unkC0));
    m->count = 0;
    if (ArmorUpgrade_Unseen(&gSave.box[idx]) == 0) {
        ArmorUpgrade_Materials(&gSave.box[idx], need, cond);
        bad = 0;
        for (i = 0; i < 4; i++) {
            if (need[i].item != 0) {
                if (cond[i] == 1 || cond[i] == 2) {
                    if (Save_IsItemObtained(&gSave, need[i].item) == 0) {
                        bad = 1;
                    }
                }
                if (Forge_CheckCondition(cond[i]) == 0) {
                    bad = 1;
                }
            }
        }
        if (bad == 0) {
            m->upgrades[m->count] = gSave.box[idx];
            m->upgrades[m->count].used = 1;
            m->upgrades[m->count].level++;
            m->cost[m->count] = Armor_GetUpgradeCost(&m->upgrades[m->count]);
            memcpy(&m->mats[m->count], need, 0x10);
            m->count++;
        }
    }
    n = ArmorUpgrade_Unlocks(&gSave.box[idx], list);
    for (k = 0; k < n; k++) {
        memset(&tmp, 0, sizeof(tmp));
        tmp.type = gSave.box[idx].type;
        tmp.id = list[k];
        ArmorUpgrade_FirstMaterials(&tmp, need, cond);
        bad = 0;
        for (j = 0; j < 4; j++) {
            if (need[j].item != 0) {
                if (cond[j] == 1 || cond[j] == 2) {
                    if (Save_IsItemObtained(&gSave, need[j].item) == 0) {
                        bad = 1;
                    }
                }
                if (Forge_CheckCondition(cond[j]) == 0) {
                    bad = 1;
                }
                /* i, the first loop's counter, reused for the class bit */
                i = 1 << gPlayers[gScene.localPlayer].gender;
                if ((i & Equip_GetWearMask(&gSave, &tmp)) == 0) {
                    bad = 1;
                }
            }
        }
        if (bad == 0) {
            m->upgrades[m->count].used = 1;
            m->upgrades[m->count].type = gSave.box[idx].type;
            m->upgrades[m->count].id = list[k];
            m->cost[m->count] = Armor_GetUpgradeCost(&m->upgrades[m->count]);
            memcpy(&m->mats[m->count], need, 0x10);
            if (Equip_TestFlag(&m->upgrades[m->count], 1) == 0) {
                Equip_SetFlag(&m->upgrades[m->count], 2);
                Equip_SetFlag(&m->upgrades[m->count], 1);
            }
            m->count++;
        }
    }
    if (m->count != 0) {
        m->cursor = 0;
        m->piece = gSave.box[idx];
        m->kind = gSave.box[idx].type;
        Equip_CopyEquipped(&m->worn, gSave.box[idx].type);
        m->sel = m->upgrades[m->cursor];
    }
}

void EquipPreview_Show(u8 on);
extern s8 gMenuInputLock;
s32 ArmorForge_UpgradeListInput(ArmorMenu *m, u16 pad);
void ArmorForge_UpgradeDetail(ArmorMenu *m);
void Menu_SetYesNo(s8 arg0);
s32 Menu_YesNoInput(void);
s32 ArmorForge_UpgradeInput(ArmorMenu *m, u16 pad)
{
    switch (m->state) {
    case 0:
        WinSlide_Step(&m->win2, -0x128, 10);
        switch (WinSlide_Step(&m->win, 0x280, 0x12C)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 1:
        switch (ArmorForge_BoxInput(m, pad)) {
        case 0:
            m->state++;
            m->detailPage = 0;
            m->detail = 0;
            if (m->unk132 == 0) {
                EquipPreview_Show(0);
                m->detailPages = Equip_LevelMark(&m->piece, 0x10, &m->sel);
            }
            ArmorForge_UpgradeDetail(m);
            break;
        case 3:
            m->state = 0;
            return 3;
        }
        break;
    case 2:
        switch (ArmorForge_UpgradeListInput(m, pad)) {
        case 0:
            m->state++;
            m->detailPage = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            m->detail = 2;
            ArmorForge_UpgradeDetail(m);
            EquipPreview_Show(1);
            break;
        case 3:
            m->state = 1;
            m->detail = 0;
            m->detailPage = 0;
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
            switch (Menu_YesNoInput()) {
            case 0:
                m->state = 0;
                m->detailPage = 0;
                WinSlide_Step(&m->win2, -0x128, 10);
                WinSlide_Step(&m->win, 0x280, 0x12C);
                return 0;
            case 3:
                m->state--;
                m->unk132 = 1;
                EquipPreview_Show(1);
                break;
            }
        }
        break;
    }
    return 2;
}

s32 Equip_CanWear(SaveData *save, EquipItem *item);
s32 Armor_GetUpgradeCost(EquipItem *e);
s32 Items_HaveEnough(u16 item, s32 n);
s32 ArmorForge_CanUpgrade(ArmorMenu *m, s16 i)
{
    EquipItem tmp;
    EquipItem *e;
    s32 cost;
    s32 j;

    memset(&tmp, 0, sizeof(tmp));
    e = &m->upgrades[i];
    if (Equip_CanWear(&gSave, e) == 0) {
        return 0;
    }
    cost = Armor_GetUpgradeCost(e);
    if (gSave.money < cost) {
        return 0;
    }
    for (j = 0; j < 4; j++) {
        if (m->mats[i].need[j].item != 0 && Items_HaveEnough(m->mats[i].need[j].item, m->mats[i].need[j].count) == 0) {
            return 0;
        }
    }
    return 1;
}

s32 Equip_IsNoModelArmor(EquipItem *e);
void StageModels_Free(void);
/* unit's own declaration: with the definition's u16 (u16 kind, u16 id, u16 female) (sub_main/unk_33B080.c), ArmorForge_MakeDetail: 1 of 137 words differ, ArmorForge_MakeInput: 1 of 410 words differ ... */
s32 Equip_GetModelId(u8 type, u16 id, u8 x);
void StageModels_LoadEquip(u8 type, s32 n, u8 x); /* unit's own declaration: with the definition's void (u16 kind, u16 id, u8 female) (sub_main/unk_33B080.c), ArmorForge_UpgradeListInput 1 of 208 words, ArmorForge_UpgradeDetail 1 of 107 words, ArmorForge_MakeDetail 1 of 137 words differ */
s32 ArmorForge_UpgradeListInput(ArmorMenu *m, u16 pad)
{
    u16 old = m->cursor;
    u8 x;

    if (pad & 0x20) {
        if (ArmorForge_CanUpgrade(m, m->cursor) == 1) {
            m->detailPage = 0;
            LobbySe_Play(8);
            return 0;
        }
        if (Equip_CanWear(&gSave, &m->upgrades[m->cursor]) == 0) {
            Menu_PushNoticeOnce(0x25);
        }
        LobbySe_Play(9);
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        if (++m->detail >= 4) {
            m->detail = 0;
        }
        m->detailPage = 0;
        ArmorForge_UpgradeDetail(m);
        LobbySe_Play(0x39);
    } else if (pad & 0x80) {
        if (m->detail != 1 && m->detailPages > 1) {
            if (++m->detailPage >= m->detailPages) {
                m->detailPage = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->detail != 1 && m->detailPages > 1) {
            if (--m->detailPage < 0) {
                m->detailPage = m->detailPages - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x4000) {
        if (m->detail == 0 || m->detail == 1) {
            if (m->unk132 == 0) {
                m->unk132 = 1;
                EquipPreview_Show(1);
            } else {
                m->unk132 = 0;
                if (m->sel.type != m->shown.type || m->sel.id != m->shown.id) {
                    m->shown = m->sel;
                    if (Equip_IsNoModelArmor(&m->shown) == 0) {
                        StageModels_Free();
                        x = gSave.gender;
                        StageModels_LoadEquip(m->sel.type, Equip_GetModelId(m->sel.type, m->sel.id, x), x);
                    }
                }
                EquipPreview_Show(0);
            }
        }
    } else {
        m->cursor = Menu_WrapUpDownSe(m->cursor, m->count, 2);
    }
    if (old != m->cursor) {
        m->sel = m->upgrades[m->cursor];
        ArmorForge_UpgradeDetail(m);
    }
    return 2;
}

u8 Equip_PreviewSkills(Player *player, EquipItem *e);
void ArmorForge_UpgradeDetail(ArmorMenu *m)
{
    u8 x;

    switch (m->detail) {
    case 3:
        EquipPreview_Show(1);
        m->detailPage = 0;
        m->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &m->sel);
        break;
    default:
        EquipPreview_Show(1);
        m->detailPages = Equip_LevelMark(&m->sel, 0x10, NULL);
        break;
    case 2:
        m->detailPages = Equip_LevelMark(&m->piece, 0x10, &m->sel);
        EquipPreview_Show(1);
        break;
    case 0:
    case 1:
        if (m->unk132 == 0) {
            if (m->sel.type != m->shown.type || m->sel.id != m->shown.id) {
                m->shown = m->sel;
                if (Equip_IsNoModelArmor(&m->shown) == 0) {
                    StageModels_Free();
                    x = gSave.gender;
                    StageModels_LoadEquip(m->sel.type, Equip_GetModelId(m->sel.type, m->sel.id, x), x);
                }
            }
            EquipPreview_Show(0);
        }
        m->detailPages = Equip_LevelMark(&m->sel, 0, NULL);
        break;
    }
}

void ArmorForge_MakeDetail(ArmorMenu *m)
{
    ForgeRecipe *r = &gArmorRecipes[m->list[m->cursor]];
    EquipItem tmp;
    u16 *id;
    u8 *type;
    u8 x;

    memset(&tmp, 0, sizeof(tmp));
    type = &tmp.type;
    id = &tmp.id;
    *type = r->type;
    *id = r->id;
    switch (m->detail) {
    case 4:
        EquipPreview_Show(1);
        m->detailPage = 0;
        m->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &tmp);
        break;
    default:
        EquipPreview_Show(1);
        m->detailPages = Equip_LevelMark(&m->sel, 0x10, NULL);
        break;
    case 3:
        m->detailPages = Equip_LevelMark(&m->worn, 0x50, &tmp);
        EquipPreview_Show(1);
        break;
    case 1:
    case 2:
        if (m->unk132 == 0) {
            if (*type != m->shown.type || *id != m->shown.id) {
                m->shown = tmp;
                if (Equip_IsNoModelArmor(&m->shown) == 0) {
                    StageModels_Free();
                    x = gSave.gender;
                    StageModels_LoadEquip(*type, Equip_GetModelId(*type, *id, x), x);
                }
            }
            EquipPreview_Show(0);
        }
        if (m->detail == 2) {
            m->detailPages = 1;
        } else {
            m->detailPages = Equip_LevelMark(&tmp, 0, NULL);
        }
        break;
    }
}

s32 Forge_CanMakeArmor(ArmorMenu *m, s16 i)
{
    s32 idx = m->list[i];
    ForgeRecipe *r = &gArmorRecipes[idx];
    s32 cost;

    if (Forge_CheckRecipe(0, idx, 0) != 2) {
        return 0;
    }
    cost = Equip_HalfPrice(r->type, r->id);
    if (gSave.money < cost) {
        return 0;
    }
    if ((u16)Box_FindFreeSlot(&gSave) == 0xFFFF) {
        return 0;
    }
    return 1;
}

void Equip_CopyEquipped(EquipItem *out, s16 type)
{
    out->type = type;
    switch (type) {
    case 2:
        *out = gSave.equip[2];
        break;
    case 3:
        *out = gSave.equip[3];
        break;
    case 4:
        *out = gSave.equip[4];
        break;
    case 5:
        *out = gSave.equip[5];
        break;
    case 0:
        *out = gSave.equip[1];
        break;
    case 6:
    case 7:
        *out = gSave.equip[0];
        break;
    }
    out->used = 1;
}

extern s16 gEquipTabTypes[];
void Menu_PushNotice(s32 msg);
s32 Forge_CanMakeArmor(ArmorMenu *m, s16 i);
void Equip_CopyEquipped(EquipItem *out, s16 type);
s32 LobbyMenu_YesNoInput(s32 msg);
void ArmorForge_LoadMakePage(ArmorMenu *m, s32 page);
s32 ArmorForge_MakeInput(ArmorMenu *m, u16 pad)
{
    EquipItem tmp;
    u16 *id;
    u8 *type;
    s32 i;
    u8 x;

    switch (m->state) {
    case 0:
        if (pad & 0x20) {
            m->state++;
            m->page = 0;
            m->detail = 0;
            m->detailPage = 0;
            m->cursor = 0;
            WinSlide_Step(&m->win, 0x280, 0x118);
            ArmorForge_LoadMakePage(m, 0);
            ArmorForge_MakeDetail(m);
            Equip_CopyEquipped(&m->worn, gEquipTabTypes[m->row]);
            LobbySe_Play(0x17);
            CamFocus_SetAside(1);
            EquipPreview_Reset();
            if ((u16)Box_FindFreeSlot(&gSave) == 0xFFFF) {
                Menu_PushNotice(0xB);
            }
        } else if (pad & 0x40) {
            m->state = 0;
            LobbySe_Play(4);
            return 3;
        } else {
            m->row = Menu_WrapUpDownSe(m->row, 5, 2);
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 2:
        if (pad & 0x20) {
            if (m->count != 0 && Forge_CanMakeArmor(m, m->cursor) == 1) {
                m->state++;
                m->detailPage = 0;
                m->detail = 3;
                ArmorForge_MakeDetail(m);
                Menu_SetYesNo(0);
                gMenuInputLock = 9;
                EquipPreview_Show(1);
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            if (m->detail != 0) {
                m->detail = 0;
                m->detailPage = 0;
                ArmorForge_MakeDetail(m);
            } else {
                m->state = 0;
                CamFocus_SetAside(0);
                EquipPreview_Show(1);
            }
            LobbySe_Play(0x17);
        } else if (pad & 0x200) {
            if (m->count != 0) {
                if (++m->detail == 5) {
                    m->detail = 1;
                }
                m->detailPage = 0;
                ArmorForge_MakeDetail(m);
                LobbySe_Play(0x39);
            }
        } else if (pad & 0xC00) {
            if (m->pages > 1) {
                m->page = Menu_WrapLeftRight(m->page, m->pages);
                ArmorForge_LoadMakePage(m, m->page);
                ArmorForge_MakeDetail(m);
            }
        } else if (pad & 0x3000) {
            m->cursor = Menu_WrapUpDownSe(m->cursor, m->count, 2);
            ArmorForge_MakeDetail(m);
        } else if (pad & 0x80) {
            if (m->detail != 0 && m->detailPages > 1) {
                if (++m->detailPage >= m->detailPages) {
                    m->detailPage = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->detail != 0 && m->detailPages > 1) {
                if (--m->detailPage < 0) {
                    m->detailPage = m->detailPages - 1;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x4000) {
            if (m->detail == 1 || m->detail == 2) {
                i = m->cursor;
                memset(&tmp, 0, sizeof(tmp));
                if (m->unk132 == 0) {
                    m->unk132 = 1;
                    EquipPreview_Show(1);
                } else {
                    m->unk132 = 0;
                    type = &tmp.type;
                    id = &tmp.id;
                    *type = gArmorRecipes[m->list[i]].type;
                    *id = gArmorRecipes[m->list[i]].id;
                    if (*type != m->shown.type || *id != m->shown.id) {
                        m->shown = tmp;
                        if (Equip_IsNoModelArmor(&m->shown) == 0) {
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
                m->state = 0;
                m->detailPage = 0;
                WinSlide_Step(&m->win, 0x280, 0x118);
                return 0;
            case 3:
                m->state = 2;
                m->unk132 = 1;
                EquipPreview_Show(1);
                break;
            }
        }
        break;
    }
    return 2;
}

void func_006D8460(void *a) {

}

u8 Equip_GetWearMask(SaveData *save, EquipItem *item);
s32 Equip_TestFlag(EquipItem *e, s16 bit);
void Equip_SetFlag(EquipItem *e, s16 bit);
s32 ArmorForge_IsListed(s16 tab, ForgeRecipe *r, s32 idx)
{
    Player *p = &gPlayers[gScene.localPlayer];
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    if (r->type != gEquipTabTypes[tab]) {
        return 0;
    }
    tmp.type = r->type;
    tmp.id = r->id;
    if (!(Equip_GetWearMask(&gSave, &tmp) & (1 << p->gender))) {
        return 0;
    }
    if (Forge_CheckRecipe(0, idx, 0) == 0) {
        return 0;
    }
    if (Equip_TestFlag(&tmp, 1) == 0) {
        Equip_SetFlag(&tmp, 2);
        Equip_SetFlag(&tmp, 1);
    }
    return 1;
}

s32 Recipe_IsInList(s32 type, s32 id);
void ArmorForge_LoadMakePage(ArmorMenu *m, s32 page)
{
    s32 hi;
    ForgeRecipe *r;
    s32 k;
    s32 n;
    s32 lo;
    s32 i;

    r = gArmorRecipes;
    lo = page * 7;
    k = 0;
    hi = lo + 6;
    n = 0;
    i = 0;
    if (r->type != 0xFF) {
        do {
            if (ArmorForge_IsListed(m->row, r, i) == 1) {
                if ((m->filter == 0 && Recipe_IsInList(r->type, r->id) == 0)
                    || (m->filter == 1 && Recipe_IsInList(r->type, r->id) == 1)) {
                    if (n >= lo && n <= hi) {
                        m->list[k] = i;
                        k++;
                    }
                    n++;
                }
            }
            r++;
            i++;
        } while (r->type != 0xFF);
    }
    m->count = k;
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
    m->pages = Menu_CountPages(n, 7);
}

void Box_SetSlot(SaveData *save, u16 idx, EquipItem *item);
void Equip_Refresh(SaveData *save);
struct Player;
void Player_LoadFromSave(struct Player *player);
s32 Equip_EquipSlot(SaveData *save, u16 idx);
void Player_SetupFromSave(struct Player *player, struct SaveData *save);
void Player_CalcSkills(struct Player *player);
void Town_ReloadLocalPlayer(void);
void Player_SetColorWord(struct Player *pl, u8 idx);
extern u8 gLobbyTextDirty;
/* Raw addresses kept here: the named form changes the code. */
void ArmorForge_Upgrade(ArmorMenu *m)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;
    s32 cost;
    s32 i;

    tmp = gSave.box[idx];
    tmp = m->sel;
    cost = m->cost[m->cursor];
    for (i = 0; i < 4; i++) {
        Items_Take(m->mats[m->cursor].need[i].item, m->mats[m->cursor].need[i].count);
    }
    if (gSave.box[idx].id != tmp.id) {
        tmp.level = 0;
        memset(tmp.deco, 0, sizeof(tmp.deco));
    }
    Box_SetSlot(&gSave, idx, &tmp);
    Save_AddMoney(-cost);
    LobbySe_Play(0xA);
    if (m->equipped != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, idx);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
        Menu_PushNotice(0x17);
        LobbySe_Play(0x12);
    } else {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(0xF);
    }
    m->cursor = 0;
    ArmorForge_AddUpgrade(m, idx);
}

s32 Equip_IsSlotEquipped(SaveData *save, s32 idx);
s32 ArmorForge_UpgradeConfirm(ArmorMenu *m, u16 pad)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;

    switch (m->unk12F) {
    case 0:
        if (Equip_IsSlotEquipped(&gSave, idx) == 1) {
            m->equipped = 1;
            m->unk12F = 2;
        } else if (Equip_CanWear(&gSave, &m->sel) == 0) {
            m->equipped = 0;
            m->unk12F = 2;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            m->unk12F = 1;
        }
        m->detailPages = Equip_LevelMark(&m->worn, 0x10, &m->sel);
        break;
    case 1:
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
            switch (Menu_YesNoInput()) {
            case 0:
                m->unk12F = 2;
                m->equipped = 1;
                break;
            case 3:
                m->unk12F = 2;
                m->equipped = 0;
                break;
            }
        }
        break;
    case 2:
        m->unk12F = 1;
        m->state = 0;
        m->detail = 0;
        ArmorForge_Upgrade(m);
        m->cursor = 0;
        return 0;
    }
    return 2;
}

s32 ArmorForge_UpgradeConfirm(ArmorMenu *m, u16 pad);
s32 ArmorForge_MakeConfirm(ArmorMenu *m, u16 pad);
s32 ArmorForge_Confirm(u8 *task, u16 pad)
{
    ArmorMenu *m = (ArmorMenu *)(task + 0xC);

    if (m->choice == 0) {
        return ArmorForge_MakeConfirm(m, pad);
    }
    return ArmorForge_UpgradeConfirm(m, pad);
}

u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
/* Raw addresses kept here: the named form changes the code. */
void ArmorForge_Make(ArmorMenu *m)
{
    ForgeRecipe *r = &gArmorRecipes[m->list[m->cursor]];
    EquipItem tmp;
    s32 price;
    u16 *id;
    u8 *type;
    s32 i;
    s32 slot;

    memset(&tmp, 0, sizeof(tmp));
    type = &tmp.type;
    id = &tmp.id;
    *type = r->type;
    *id = r->id;
    price = Equip_HalfPrice(*type, *id);
    for (i = 0; i < 4; i++) {
        Items_Take(r->need[i].item, r->need[i].count);
    }
    slot = Box_AddItem(&gSave, *type, *id, 0);
    Save_AddMoney(-price);
    LobbySe_Play(0xA);
    if (m->equipped != 0) {
        LobbySe_Play(0x12);
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, slot);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
        Menu_PushNotice(0x17);
        LobbySe_Play(0x12);
    } else {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(0xF);
    }
}

void ArmorForge_Make(ArmorMenu *m);
s32 ArmorForge_MakeConfirm(ArmorMenu *m, u16 pad)
{
    ForgeRecipe *r = &gArmorRecipes[m->list[m->cursor]];
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    tmp.used = 1;
    tmp.type = r->type;
    tmp.id = r->id;
    switch (m->unk12F) {
    case 0:
        if (Equip_CanWear(&gSave, &tmp) == 0) {
            m->equipped = 0;
            m->unk12F = 2;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            m->unk12F = 1;
        }
        m->detailPages = Equip_LevelMark(&m->worn, 0x10, &tmp);
        break;
    case 1:
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
            switch (Menu_YesNoInput()) {
            case 0:
                m->unk12F = 2;
                m->equipped = 1;
                break;
            case 3:
                m->unk12F = 2;
                m->equipped = 0;
                break;
            }
        }
        break;
    case 2:
        m->unk12F = 2;
        m->state = 1;
        m->detail = 0;
        ArmorForge_Make(m);
        Equip_CopyEquipped(&m->worn, gEquipTabTypes[m->row]);
        WinSlide_Step(&m->win, 0x280, 0x118);
        return 0;
    }
    return 2;
}

/* The game has a second "%s" here (the one at 0x0082E920 is another unit's?):
   a literal would reuse the first. */
static const char D_0082E950[3] = {
#include "lobby/0082E950.inc"
};
extern char *gNoArmorToMakeTexts[];
extern char *gPriceMarks[];
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 color);
void EquipShop_DrawLine(s16 x, s16 y, EquipItem *e);
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, EquipItem *e) (lobby/unk_78F070.c), ArmorForge_DrawMakeListFull 6 of 274 words, ArmorForge_DrawMakeList 6 of 228 words differ */
char *Equip_GetName(u8 type, u16 id);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void ArmorForge_DrawMakeListFull(ArmorMenu *m, s16 x, s16 y)
{
    ForgeRecipe *r;
    s16 ty = y;
    EquipItem tmp;
    s16 i;
    s32 tx;
    EquipItem sel;
    s16 d;
    s32 ly;
    s32 lx;
    char *s;
    s32 cy;
    s16 cx;

    memset(&tmp, 0, sizeof(tmp));
    Icon_Draw(x, y, 0x154, 0x122, 0xFFFFDDDD, 0);
    ly = ty;
    lx = x;
    cy = ty;
    Icon_Draw(lx + 0x6E, ly + 0x2D, 0x78, 0xC8, 0x60FFFFFF, 0x24);
    Font_SetColor(0x80101010);
    if (m->count == 0) {
        s = gNoArmorToMakeTexts[0];
        cx = Text_CenterX(s, lx + 0xAA, 0x12);
        Font_SetColor(0x8000052C);
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(cx, cy + 0x5A);
        Text_PrintfColored(D_0082E950, s);
        Menu_DrawDashLine(lx + 0x14, ly + 0xBA, 0x12C, 0xFF010101);
        return;
    }
    Menu_DrawPageNum(lx + 0xFA, ly + 0xE, m->page, m->pages, 1);
    ty += 0x22;
    Font_SetSpacing(0x14, 0x14);
    i = 0;
    if (i < m->count) {
        tx = lx + 0x28;
        do {
            r = &gArmorRecipes[m->list[i]];
            tmp.type = r->type;
            tmp.id = r->id;
            if (Equip_TestFlag(&tmp, 2) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw((s16)tx - 0x16, ty, 0x10, 0x14, 0xFFC03000, 0x63);
            } else {
                Lobby_DrawEquipIcon((s16)tx - 0x1C, ty, 0x14, 0x14, &tmp);
            }
            if (i == m->cursor) {
                sel = tmp;
                if (gFrameTick & 0x10) {
                    d = 4;
                } else {
                    d = 10;
                }
                Icon_Draw(tx, ty + 0xE, 0x112, 8, 0x99106060, 0x6F);
                Menu_DrawCursorIcon((s16)tx - 0x32 - d, ty - 2, 0x18, 0xFF20C0C0);
            }
            if (Forge_CanMakeArmor(m, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(tx, ty);
            Text_PrintfColored("%-20s %6d%s", Equip_GetName(r->type, r->id), Equip_HalfPrice(r->type, r->id), gPriceMarks[0]);
            ty += 0x16;
            i++;
        } while (i < m->count);
    }
    Menu_DrawDashLine(lx + 0x14, ly + 0xBA, 0x12C, 0xFF010101);
    EquipShop_DrawLine(lx + 0x10, ly + 0xC9, &sel);
}

void ArmorForge_DrawMakeList(ArmorMenu *m, s16 x, s16 y)
{
    ForgeRecipe *r;
    s16 ty = y;
    EquipItem tmp;
    s16 i;
    s32 tx;
    EquipItem sel;
    s16 d;
    s32 ly;
    s32 lx;

    memset(&tmp, 0, sizeof(tmp));
    memset(&sel, 0, sizeof(sel));
    Icon_Draw(x, y, 0x122, 0xB4, 0xFFFFDDDD, 0);
    ly = ty;
    lx = x;
    Icon_Draw(lx + 0x64, ly + 0xF, 0x5A, 0x96, 0x60FFFFFF, 0x24);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(lx + 0xC8, ly + 0xC, m->page, m->pages, 1);
    ty += 0x22;
    Font_SetSpacing(0x11, 0x11);
    i = 0;
    if (i < m->count) {
        tx = lx + 0x28;
        do {
            r = &gArmorRecipes[m->list[i]];
            tmp.type = r->type;
            tmp.id = r->id;
            if (Equip_TestFlag(&tmp, 2) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw((s16)tx - 0x14, ty, 0x10, 0x14, 0xFFC03000, 0x63);
            } else {
                Lobby_DrawEquipIcon((s16)tx - 0x1A, ty, 0x14, 0x14, &tmp);
            }
            if (i == m->cursor) {
                sel = tmp;
                if (gFrameTick & 0x10) {
                    d = 4;
                } else {
                    d = 10;
                }
                Icon_Draw((s16)tx - 8, ty + 0xC, 0xF8, 8, 0x99106060, 0x6F);
                Menu_DrawCursorIcon((s16)tx - 0x2A - d, ty - 2, 0x16, 0xFF20C0C0);
            }
            if (Forge_CanMakeArmor(m, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(tx, ty);
            Text_PrintfColored("%-20s", Equip_GetName(r->type, r->id));
            Font_SetPos((s16)tx + 0xB2, ty);
            Text_PrintfColored("%6d%s", Equip_HalfPrice(r->type, r->id), gPriceMarks[0]);
            ty += 0x14;
            i++;
        } while (i < m->count);
    }
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), ArmorForge_DrawMaterials 63 of 76 words, ArmorForge_DrawUpgradeBox 30 of 91 words, Deco_DrawAttachList 221 of 227 words differ */
s32 Items_DrawNeedLine(s16 x, s16 y, u16 item, s32 n, u32 color);
extern char *D_0080C82C;
void ArmorForge_DrawMaterials(ArmorMenu *m, s16 x, s16 y)
{
    ForgeRecipe *r = &gArmorRecipes[m->list[m->cursor]];
    s16 i;

    Win_Draw(x, y, 0x132, 0x72, 0xBDB59F6C, 0xA0202020, 6);
    y += 10;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 14, y);
    Text_PrintfColored(D_0080C82C);
    for (i = 0; i < 4; i++) {
        y += 20;
        Items_DrawNeedLine(x + 0x22, y, r->need[i].item, r->need[i].count, 0x807F7F7F);
    }
}

void EquipWindow_DrawLocal(EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
extern char *D_0080C6F4[];
extern char *D_0080C8F4;
extern char *D_0080C8AC;
extern char *gNeedMaterialsTexts[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;
void Equip_DrawPreviewHelp(u32 color, s8 k);
void EquipCompare_Draw(EquipItem *a, EquipItem *b, s32 c, s16 y, u8 flags);
void EquipDetail_Draw(Player *a, s16 x, s16 y, u32 flags);
char *Text_FillDollar(char *fmt, M2C_UNK n);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void ArmorForge_DrawMakeListFull(ArmorMenu *m, s16 x, s16 y);
void ArmorForge_DrawMaterials(ArmorMenu *m, s16 x, s16 y);
void ArmorForge_DrawMake(ArmorMenu *m)
{
    s32 cur = m->cursor;
    EquipItem tmp;
    s16 x;
    s16 y;
    char *s;
    ForgeRecipe *r;

    memset(&tmp, 0, sizeof(tmp));
    if (m->state != 0) {
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(10, 0x12, 0xA0, 0xFFB0B000, D_0080C6F4[m->row], 2, 1);
        Menu_DrawMoney(0x1E0, 0x10, 0);
        r = &gArmorRecipes[m->list[cur]];
        tmp.used = 1;
        y = 0x34;
        tmp.type = r->type;
        tmp.id = r->id;
        x = m->win.pos;
    }
    switch (m->state) {
    case 0:
        Font_SetSpacing(0x14, 0x14);
        Menu_DrawLabelList(0x1CC, D_0080C6F4, 5, m->row, 0xFFB0B000);
        break;
    case 1:
    case 2:
        switch (m->detail) {
        case 0:
            ArmorForge_DrawMakeListFull(m, x + 0xC, y);
            break;
        case 1:
            if (m->unk132 == 0) {
                EquipPreview_Show(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, m->unk132);
            if (m->unk132 == 0 && Equip_IsNoModelArmor(&m->shown) == 1) {
                s = D_0080C8F4;
                EquipPreview_Show(1);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(s);
            }
            ArmorForge_DrawMakeList(m, 10, 0x32);
            EquipWindow_DrawLocal(&tmp, 0x136, 0xD8, m->detailPage | 0x40, NULL);
            break;
        case 2:
            if (m->unk132 == 0) {
                EquipPreview_Show(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, m->unk132);
            if (m->unk132 == 0 && Equip_IsNoModelArmor(&m->shown) == 1) {
                s = D_0080C8F4;
                EquipPreview_Show(1);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(s);
            }
            ArmorForge_DrawMakeList(m, 10, 0x32);
            ArmorForge_DrawMaterials(m, 0x136, 0xD8);
            break;
        case 3:
            ArmorForge_DrawMakeList(m, 10, 0x32);
            EquipCompare_Draw(&m->worn, &tmp, 0x136, 0x36, m->detailPage | 0x50);
            break;
        case 4:
            ArmorForge_DrawMakeList(m, 10, 0x32);
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, 0x3A, m->detailPage);
            break;
        }
        if (m->detail != 4) {
            Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, D_0080C8AC);
        }
        break;
    case 3:
        EquipCompare_Draw(&m->worn, &tmp, 0x136, 0x36, m->detailPage | 0x50);
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020,
                      Text_FillDollar(gNeedMaterialsTexts[0], Equip_HalfPrice(tmp.type, tmp.id)));
        Font_SetPos(0x1BC, 0x190);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

void BoxMenu_DrawItemList(s16 cur, u32 flags, f32 fx);
void ArmorUpgrade_DrawMaterials(s16 x, s16 y, ItemStack *need, s8 n);
extern char *gUpgradeKindTexts[];
extern char *D_0080C80C;
extern char *D_0080C81C;
void ArmorForge_DrawUpgrade(ArmorMenu *m, s16 x, s16 y)
{
    s32 idx = m->col + (m->row * 10 + m->page * 100);
    char *s;

    switch (m->state) {
    case 2:
        switch (m->detail) {
        case 0:
            if (m->unk132 == 0) {
                EquipPreview_Show(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, m->unk132);
            if (m->unk132 == 0 && Equip_IsNoModelArmor(&m->shown) == 1) {
                s = D_0080C8F4;
                EquipPreview_Show(1);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(s);
            }
            EquipWindow_DrawLocal(&m->sel, 0x136, 0xE0, m->detailPage | 0x40, NULL);
            break;
        case 2:
            EquipCompare_Draw(&m->piece, &m->sel, 0x136, y + 4, m->detailPage | 0x50);
            break;
        case 1:
            if (m->unk132 == 0) {
                EquipPreview_Show(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, m->unk132);
            if (m->unk132 == 0 && Equip_IsNoModelArmor(&m->shown) == 1) {
                s = D_0080C8F4;
                EquipPreview_Show(1);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(s);
            }
            ArmorUpgrade_DrawMaterials(0x136, 0xE0, m->mats[m->cursor].need, 4);
            break;
        case 3:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, 0x3A, m->detailPage);
            return;
        }
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, gUpgradeKindTexts[0]);
        break;
    case 3:
        EquipCompare_Draw(&m->piece, &m->sel, 0x136, y + 4, m->detailPage | 0x50);
        if (m->piece.id != m->sel.id) {
            s = Text_FillDollar(D_0080C81C, m->cost[m->cursor]);
        } else {
            s = Text_FillDollar(gNeedMaterialsTexts[0], m->cost[m->cursor]);
        }
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
        Font_SetPos(0x1BC, 0x190);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    default:
        if (m->detail == 0) {
            BoxMenu_DrawItemList(idx, 0x2800010, x);
        } else {
            BoxMenu_DrawItemList(idx, (m->detailPage & 7) | 0x2800078, x);
        }
        if (m->state == 1 || m->state == 0) {
            Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, D_0080C80C);
        } else {
            Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, gUpgradeKindTexts[0]);
        }
        break;
    }
}

void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
extern char *D_0080C834;
extern char *gCannotUpgradeTexts[];
extern char *D_0080C83C;
void ArmorForge_DrawUpgradeList(ArmorMenu *m, EquipItem *e, s16 x, s16 y)
{
    s16 tx = x;
    EquipItem tmp;
    u32 cost;
    s32 i;
    s16 ty = y;
    u16 *id;
    u8 *type;

    memset(&tmp, 0, sizeof(tmp));
    ty += 0xA;
    Font_SetPos(tx + 0xA, ty);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(D_0080C834);
    switch (e->type) {
    case 6:
    case 7:
        return;
    }
    if (e->id == 0) {
        return;
    }
    if (m->count == 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(Text_CenterX(gCannotUpgradeTexts[0], tx + 0x8A, 0x11), y + 0x33);
        Text_PrintfColored(gCannotUpgradeTexts[0]);
        return;
    }
    i = 0;
    if (i < m->count) {
        do {
            if (ArmorForge_CanUpgrade(m, i) == 1) {
                Font_SetColor(0x807F7F7F);
            } else {
                Font_SetColor(0x8042352E);
            }
            tmp = m->upgrades[i];
            cost = m->cost[i];
            Font_SetPos(tx + 0xE, ty += 0x14);
            id = &tmp.id;
            if (*id != m->piece.id) {
                type = &tmp.type;
                Text_PrintfColored(Equip_GetName(*type, *id));
            } else {
                Text_PrintfColored(D_0080C83C);
            }
            Font_SetPos(tx + 0xC8, ty);
            Text_PrintfColored("%7dz", cost);
            if (Equip_TestFlag(&tmp, 2) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw(x, ty - 4, 0x10, 0x14, 0xFFC03000, 0x63);
            }
            if (m->cursor == i && m->state == 2) {
                Menu_DrawGlowBar(tx + 0x8A, ty - 1, 0x114, 0x12, 0xB05050F0);
            }
            i++;
        } while (i < m->count);
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6D5330", D_0082E980);

extern char *D_0080C7AC;
void ArmorForge_DrawUpgradeList(ArmorMenu *m, EquipItem *e, s16 x, s16 y);
void ArmorForge_DrawUpgradeBox(ArmorMenu *m, s16 x, s16 y)
{
    u32 c1;
    u32 c2;
    s32 idx = m->row * 10 + m->page * 100 + m->col;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 0x12, 0xA0, 0xFFB0B000, D_0080C7AC, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->state) {
    case 2:
        c1 = 0xBDB59F6C;
        c2 = 0xA0202020;
        break;
    default:
        c1 = 0xFF202020;
        c2 = 0xA0000000;
        break;
    }
    Font_SetSpacing(17, 17);
    Win_Draw(x, y, 0x114, 0xAE, c1, c2, 6);
    ArmorForge_DrawUpgradeList(m, &gSave.box[idx], x, y);
}

extern char *gArmorMakeTexts[];
void ArmorForge_DrawMake(ArmorMenu *m);
void ArmorForge_DrawUpgrade(ArmorMenu *m, s16 x, s16 y);
void ArmorForge_DrawUpgradeBox(ArmorMenu *m, s16 x, s16 y);
void ArmorForge_Draw(u8 *task)
{
    ArmorMenu *m = (ArmorMenu *)(task + 0xC);

    switch (m->unk12F) {
    case 0:
        Font_SetSpacing(0x14, 0x14);
        Menu_DrawLabelList(0x1CC, gArmorMakeTexts, 2, m->choice, 0xFFB0B000);
        break;
    case 2:
        ArmorForge_DrawMake(m);
        break;
    case 1:
        ArmorForge_DrawUpgrade(m, m->win.pos + 12, 0x32);
        ArmorForge_DrawUpgradeBox(m, m->win2.pos + 6, 0x36);
        break;
    }
}

extern char *D_0080C814;
extern char *gEquipNowTexts[];
void ArmorForge_DrawEquipPrompt(u8 *task)
{
    ArmorMenu *m = (ArmorMenu *)(task + 0xC);
    EquipItem tmp;
    char *s;
    ForgeRecipe *r;

    if (m->unk12F == 0) {
        return;
    }
    memset(&tmp, 0, sizeof(tmp));
    if (m->choice == 0) {
        r = &gArmorRecipes[m->list[m->cursor]];
        tmp.used = 1;
        tmp.type = r->type;
        tmp.id = r->id;
    } else {
        tmp = m->sel;
    }
    if (Equip_CanWear(&gSave, &tmp)) {
        switch (m->unk12F) {
        case 1:
            s = D_0080C814;
            break;
        }
        EquipCompare_Draw(&m->worn, &tmp, 0x132, 0x38, m->detailPage | 0x50);
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
        Font_SetPos(0x144, 0x190);
        Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
    }
}

void func_006DA9D0(void) {

}

void Deco_OpenMake(DecoMenu *m);
s32 Deco_MenuInput(DecoMenu *m, u16 pad);
s32 Deco_MakeInput(DecoMenu *m, u16 pad);
s32 Deco_AttachInput(DecoMenu *m, u16 pad);
s32 Deco_RemoveInput(DecoMenu *m, u16 pad);
void Deco_OpenBox(DecoMenu *m);
void Deco_StartBox(DecoMenu *m);
s32 Deco_Input(u8 *task, u16 pad)
{
    DecoMenu *m = (DecoMenu *)(task + 0xC);

    switch (m->mode) {
    case 0:
        switch (Deco_MenuInput(m, pad)) {
        case 0:
            switch (m->choice) {
            case 0:
                m->mode = 1;
                Deco_OpenMake(m);
                break;
            case 1:
                m->mode = 2;
                Deco_OpenBox(m);
                break;
            case 2:
                m->mode = 3;
                Deco_StartBox(m);
                break;
            }
            CamFocus_SetAside(1);
            Chat_SetColor(0);
            break;
        case 3:
            LobbySe_Play(4);
            return 3;
        }
        break;
    case 1:
        switch (Deco_MakeInput(m, pad)) {
        case 0:
            m->mode = 0;
            return 0;
        case 3:
            m->mode = 0;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case 2:
        switch (Deco_AttachInput(m, pad)) {
        case 0:
            m->mode = 0;
            return 0;
        case 3:
            m->mode = 0;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case 3:
        switch (Deco_RemoveInput(m, pad)) {
        case 0:
            m->mode = 0;
            return 0;
        case 3:
            m->mode = 0;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    }
    return 2;
}

s32 Forge_CheckCondition(s16 cond);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 Deco_RecipeUnlocked(Trade *t)
{
    s16 i;

    for (i = 0; i < 4; i++) {
        if (t->need[i].cond != 0 && Forge_CheckCondition(t->need[i].cond) == 1 && Save_IsItemObtained(&gSave, t->need[i].item) == 0) {
            return 0;
        }
    }
    return 1;
}

void Deco_LoadRecipes(DecoMenu *m, s16 page)
{
    Trade *t;
    s32 i;
    s16 n;
    u16 product;
    s16 lo = page * 7;
    s16 hi = lo + 6;

    memset(m->recipes, 0, sizeof(m->recipes));
    m->count = 0;
    n = 0;
    i = 0;
    t = gForgeTrades;
    product = gForgeTrades[0].product;
    while (product != 0) {
        if (Deco_RecipeUnlocked(t) == 1) {
            if (n >= lo && n <= hi) {
                m->recipes[m->count] = i;
                m->count++;
            }
            n++;
        }
        t++;
        i++;
        product = t->product;
    }
    m->page = page;
    m->pages = Menu_CountPages(n, 7);
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
}

void Deco_OpenMake(DecoMenu *m)
{
    m->unk71 = 0;
    m->cursor = 0;
    m->unk73 = 0;
    m->page = 0;
    m->unk5E = 3;
    Deco_LoadRecipes(m, 0);
    WinSlide_Step(&m->win, 0x280, 0x128);
}

s32 Items_Room(s32 item);
s32 Deco_CanMake(s32 idx, s32 n)
{
    Trade *t = &gForgeTrades[idx];
    s32 i;
    s32 price;

    price = Item_SellPrice(t->product);
    if (gSave.money < price * n) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (t->need[i].item != 0 && Items_HaveEnough(t->need[i].item, t->need[i].count * n) == 0) {
            return 0;
        }
    }
    if (Items_Room(t->product) < n) {
        return 0;
    }
    if (n < 100) {
        return 1;
    }
    return 0;
}

s32 Deco_RecipeListInput(DecoMenu *m, u16 pad);
s32 Deco_QuantityInput(DecoMenu *m, u16 pad);
void DecoShop_Make(DecoMenu *m);
s32 Deco_MakeInput(DecoMenu *m, u16 pad)
{
    switch (m->unk71) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x128)) {
        case 0:
            m->unk71++;
            break;
        }
        break;
    case 1:
        switch (Deco_RecipeListInput(m, pad)) {
        case 0:
            m->unk71++;
            m->quantity = 1;
            m->unk73 = 1;
            break;
        case 3:
            m->unk71 = 0;
            return 3;
        }
        break;
    case 2:
        switch (Deco_QuantityInput(m, pad)) {
        case 0:
            m->unk71++;
            m->unk5C = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case 3:
            m->unk71 = 1;
            break;
        }
        break;
    case 3:
        if (pad & 0x200) {
            if (m->unk73 == 1) {
                m->unk73 = 2;
            } else {
                m->unk73 = 1;
            }
            LobbySe_Play(0x39);
        } else if (pad & 0x80) {
            if (m->unk73 == 2 && m->unk5E > 1) {
                if (++m->unk5C >= m->unk5E) {
                    m->unk5C = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->unk73 == 2 && m->unk5E > 1) {
                if (--m->unk5C < 0) {
                    m->unk5C = m->unk5E - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x1B)) {
            case 0:
                m->unk71 = 1;
                DecoShop_Make(m);
                break;
            case 3:
                m->unk71 = 1;
                break;
            }
        }
        break;
    }
    return 2;
}

void DecoShop_Make(DecoMenu *m)
{
    Trade *t = &gForgeTrades[m->recipes[m->cursor]];
    s32 price;
    s32 i;

    price = Item_SellPrice(t->product);
    Save_AddMoney(-(price * m->quantity));
    LobbySe_Play(10);
    for (i = 0; i < 4; i++) {
        Items_Take(t->need[i].item, t->need[i].count * m->quantity);
    }
    Notice_Push(1, 0x12, t->product);
    if (Items_Give(t->product, m->quantity) != 0) {
        Menu_PushNoticeOnce(0xF);
    }
}

s32 Deco_QuantityInput(DecoMenu *m, u16 pad)
{
    s32 idx = m->recipes[m->cursor];
    s32 n;
    s16 old;

    if (pad & 0x20) {
        LobbySe_Play(8);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x2000) {
        if (Deco_CanMake(idx, m->quantity + 1) == 1) {
            m->quantity++;
            LobbySe_Play(1);
        }
    } else if (pad & 0x1000) {
        if (m->quantity > 1) {
            m->quantity--;
            LobbySe_Play(1);
        }
    } else if (pad & 0x800) {
        if (m->quantity > 1) {
            m->quantity = 1;
            LobbySe_Play(1);
        }
    } else if (pad & 0x400) {
        n = m->quantity;
        old = n;
        for (; n <= 100; n++) {
            if (Deco_CanMake(idx, n) == 0) {
                m->quantity = n - 1;
                break;
            }
        }
        if (m->quantity != old) {
            LobbySe_Play(1);
        }
    } else if (pad & 0x200) {
        if (m->unk73 == 1) {
            m->unk73 = 2;
        } else {
            m->unk73 = 1;
        }
        LobbySe_Play(0x39);
    } else if (pad & 0x80) {
        if (m->unk73 == 2 && m->unk5E > 1) {
            if (++m->unk5C >= m->unk5E) {
                m->unk5C = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->unk73 == 2 && m->unk5E > 1) {
            if (--m->unk5C < 0) {
                m->unk5C = m->unk5E - 1;
            }
            LobbySe_Play(1);
        }
    }
    return 2;
}

s32 Deco_RecipeListInput(DecoMenu *m, u16 pad)
{
    if (pad & 0x20) {
        s32 idx = m->recipes[m->cursor];

        if (m->count != 0 && Deco_CanMake(idx, 1) == 1) {
            LobbySe_Play(8);
            return 0;
        }
        if (m->count != 0) {
            LobbySe_Play(9);
        }
    } else if (pad & 0x40) {
        LobbySe_Play(0x17);
        if (m->unk73 == 0) {
            return 3;
        }
        m->unk73 = 0;
    } else if (pad & 0x200) {
        if (m->count != 0) {
            if (++m->unk73 > 2) {
                m->unk73 = 1;
            }
            m->unk5C = 0;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0x80) {
        if (m->unk73 == 2 && m->unk5E > 1) {
            if (++m->unk5C >= m->unk5E) {
                m->unk5C = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->unk73 == 2 && m->unk5E > 1) {
            if (--m->unk5C < 0) {
                m->unk5C = m->unk5E - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0xC00) {
        if (m->pages > 1) {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
            Deco_LoadRecipes(m, m->page);
        }
    } else if (pad & 0x3000) {
        if (m->count > 1) {
            m->cursor = Menu_WrapUpDownSe(m->cursor, m->count, 2);
        }
    }
    return 2;
}

void Deco_StartBox(DecoMenu *m)
{
    m->unk71 = 0;
    m->row = 0;
    m->col = 0;
    m->page = 0;
    m->pages = Box_GetPageCount(&gSave);
    m->unk73 = 0;
    m->equip = gSave.box[m->row * 10 + m->page * 100 + m->col];
    WinSlide_Step(&m->win, 0x280, 0x12C);
}

void Deco_StartRemove(DecoMenu *m);
void Deco_UpdatePreview(DecoMenu *m);
s32 Deco_BoxInput(DecoMenu *m, u16 pad);
s32 Deco_ChooseRemove(DecoMenu *m, u16 pad);
void Equip_RemoveDecoration(EquipItem *e, s32 i);
s32 Deco_RemoveInput(DecoMenu *m, u16 pad)
{
    switch (m->unk71) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x12C)) {
        case 0:
            m->unk71++;
            break;
        }
        break;
    case 1:
        switch (Deco_BoxInput(m, pad)) {
        case 0:
            m->unk71++;
            Deco_StartRemove(m);
            break;
        case 3:
            m->unk71 = 0;
            return 3;
        }
        break;
    case 2:
        switch (Deco_ChooseRemove(m, pad)) {
        case 0:
            m->unk71++;
            m->unk73 = 2;
            Deco_UpdatePreview(m);
            m->unk5C = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case 3:
            m->unk71 = 1;
            m->unk5C = 0;
            break;
        }
        break;
    case 3:
        if (pad & 0x80) {
            if (m->unk5E > 1) {
                if (++m->unk5C >= m->unk5E) {
                    m->unk5C = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->unk5E > 1) {
                if (--m->unk5C < 0) {
                    m->unk5C = m->unk5E - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x1B)) {
            case 0:
                m->unk71 = 0;
                m->unkC = m->equip;
                Equip_RemoveDecoration(&m->unkC, m->cursor);
                return 0;
            case 3:
                m->unk71 = 2;
                m->unk73 = 0;
                break;
            }
        }
        break;
    }
    return 2;
}

void Deco_StartRemove(DecoMenu *m)
{
    s32 n;
    s32 i;

    memset(m->list, 0, sizeof(m->list));
    n = 0;
    for (i = 0; i < 3; i++) {
        m->list[i] = m->equip.deco[i];
        if (m->list[i] != 0) {
            n++;
        }
    }
    m->cursor = 0;
    m->count = n;
    m->unk5C = 0;
    WinSlide_Step(&m->win2, 0x280, 0x128);
    Deco_UpdatePreview(m);
}

s32 Deco_GetRemoveFee(u16 deco);
s32 Deco_ChooseRemove(DecoMenu *m, u16 pad)
{
    s8 changed = 0;

    switch (m->unk72) {
    case 0:
        switch (WinSlide_Step(&m->win2, 0x280, 0x128)) {
        case 0:
            m->unk72++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (Deco_GetRemoveFee(m->list[m->cursor]) <= gSave.money) {
                m->unk72 = 0;
                LobbySe_Play(8);
                return 0;
            }
            LobbySe_Play(9);
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            if (m->unk73 == 0) {
                m->unk72 = 0;
                return 3;
            }
            m->unk73 = 0;
        } else if (pad & 0x200) {
            if (Deco_GetRemoveFee(m->list[m->cursor]) <= gSave.money) {
                if (++m->unk73 >= 4) {
                    m->unk73 = 1;
                }
            } else {
                m->unk73 ^= 1;
            }
            Deco_UpdatePreview(m);
            LobbySe_Play(0x39);
        } else if (pad & 0x80) {
            if (m->unk73 != 0 && m->unk5E > 1) {
                if (++m->unk5C >= m->unk5E) {
                    m->unk5C = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->unk73 != 0 && m->unk5E > 1) {
                if (--m->unk5C < 0) {
                    m->unk5C = m->unk5E - 1;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x3000) {
            if (m->unk73 < 2) {
                m->cursor = Menu_WrapUpDownSe(m->cursor, m->count, 2);
                changed = 1;
            }
        }
        if (changed != 0) {
            Deco_UpdatePreview(m);
        }
        break;
    }
    return 2;
}

void Deco_LoadOwnedPage(DecoMenu *m, s16 page)
{
    s32 lo;
    s32 hi;
    s32 n;
    u16 item;

    memset(m->list, 0, sizeof(m->list));
    lo = page * 7;
    hi = lo + 6;
    m->count = 0;
    n = 0;
    for (item = 1; item < 0x399; item++) {
        if (gItemData[item].deco != 0 && Items_HaveEnough(item, 1) == 1) {
            if (lo <= n && n <= hi) {
                m->list[m->count] = item;
                m->count++;
            }
            n++;
        }
    }
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
    m->decoPage = page;
    m->decoPages = Menu_CountPages(n, 7);
}

void Deco_OpenBox(DecoMenu *m)
{
    m->unk71 = 0;
    m->row = 0;
    m->col = 0;
    m->page = 0;
    m->pages = Box_GetPageCount(&gSave);
    m->unk73 = 0;
    m->equip = gSave.box[m->row * 10 + m->page * 100 + m->col];
    WinSlide_Step(&m->win, 0x280, 0x12C);
}

void Deco_LoadOwnedPage(DecoMenu *m, s16 page);
s32 Deco_ChooseAttach(DecoMenu *m, u16 pad);
void Equip_AddDecoration(EquipItem *e, u16 deco);
s32 Deco_AttachInput(DecoMenu *m, u16 pad)
{
    switch (m->unk71) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x12C)) {
        case 0:
            m->unk71++;
            break;
        }
        break;
    case 1:
        switch (Deco_BoxInput(m, pad)) {
        case 0:
            m->unk71++;
            m->unk73 = 0;
            m->cursor = 0;
            m->unk5C = 0;
            m->unk5E = 3;
            Deco_LoadOwnedPage(m, 0);
            WinSlide_Step(&m->win2, 0x280, 0x128);
            m->unkC = m->equip;
            Equip_AddDecoration(&m->unkC, m->list[m->cursor]);
            break;
        case 3:
            m->unk71 = 0;
            return 3;
        }
        break;
    case 2:
        switch (WinSlide_Step(&m->win2, 0x280, 0x128)) {
        case 0:
            m->unk71++;
            break;
        }
        break;
    case 3:
        switch (Deco_ChooseAttach(m, pad)) {
        case 0:
            m->unk71++;
            m->unk5C = 0;
            m->unk73 = 2;
            Deco_UpdatePreview(m);
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case 3:
            m->unk71 = 1;
            m->unk73 = 0;
            m->unk5C = 0;
            break;
        }
        break;
    case 4:
        if (pad & 0x80) {
            if (m->unk5E > 1) {
                if (++m->unk5C >= m->unk5E) {
                    m->unk5C = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->unk5E > 1) {
                if (--m->unk5C < 0) {
                    m->unk5C = m->unk5E - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (LobbyMenu_YesNoInput(0x1B)) {
            case 0:
                m->unk71 = 0;
                return 0;
            case 3:
                m->unk71 = 3;
                m->unk73 = 0;
                break;
            }
        }
        break;
    }
    return 2;
}

s32 Item_SellPrice(u16 item)
{
    s32 price = gItemData[item].price >> 1;

    if (price == 0) {
        price = 1;
    }
    return price;
}

/* a decoration's price (its data at Item_GetDecoration, +4; 100 without data) */
s32 Deco_GetPrice(u16 deco)
{
    u8 *d = (u8 *)Item_GetDecoration(deco);

    if (d == NULL) {
        return 100;
    }
    return *(s32 *)(d + 4);
}

s32 Deco_GetRemoveFee(u16 deco)
{
    s32 v = Deco_GetPrice(deco) / 10;

    if (v == 0) {
        v = 1;
    }
    return v;
}

/* can the decoration be attached to the piece: 1 (Deco_FitsEquip), 2 not
   enough free slots, 4 not enough money */
s32 Deco_CheckAttach(EquipItem *e, u16 deco)
{
    s32 flags = 0;
    s32 cost;

    if (Deco_FitsEquip(e, deco) == 0) {
        flags |= 1;
    }
    if (Equip_FreeSlots(e) < *(u8 *)Item_GetDecoration(deco)) {
        flags |= 2;
    }
    cost = Deco_GetPrice(deco);
    if (gSave.money < cost) {
        flags |= 4;
    }
    return flags;
}

s32 Deco_ChooseAttach(DecoMenu *m, u16 pad)
{
    s8 changed = 0;

    if (pad & 0x20) {
        if (m->count != 0) {
            if (Deco_CheckAttach(&m->equip, m->list[m->cursor]) == 0) {
                LobbySe_Play(8);
                return 0;
            }
            LobbySe_Play(9);
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        if (m->unk73 == 0) {
            return 3;
        }
        m->unk73 = 0;
    } else if (pad & 0x200) {
        if (m->count != 0) {
            if (Deco_CheckAttach(&m->equip, m->list[m->cursor]) != 0) {
                m->unk73 ^= 1;
            } else if (++m->unk73 >= 4) {
                m->unk73 = 0;
            }
            m->unk5C = 0;
            Deco_UpdatePreview(m);
            LobbySe_Play(0x39);
        }
    } else if (pad & 0x80) {
        if (m->count != 0 && m->unk73 != 0 && m->unk5E > 1) {
            if (++m->unk5C >= m->unk5E) {
                m->unk5C = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->count != 0 && m->unk73 != 0 && m->unk5E > 1) {
            if (--m->unk5C < 0) {
                m->unk5C = m->unk5E - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0xC00) {
        if (m->count != 0 && m->unk73 < 2) {
            m->decoPage = Menu_WrapLeftRight(m->decoPage, m->decoPages);
            Deco_LoadOwnedPage(m, m->decoPage);
            changed = 1;
        }
    } else if (pad & 0x3000) {
        if (m->count != 0 && m->unk73 < 2) {
            m->cursor = Menu_WrapUpDownSe(m->cursor, m->count, 2);
            changed = 1;
        }
    }
    if (changed != 0) {
        Deco_UpdatePreview(m);
    }
    m->unkC = m->equip;
    Equip_AddDecoration(&m->unkC, m->list[m->cursor]);
    return 2;
}

s32 Equip_CountDecos(EquipItem *e);
void Deco_UpdateSkills(DecoMenu *m);

s32 Deco_BoxInput(DecoMenu *m, u16 pad)
{
    s16 idx = m->row * 10 + m->page * 100 + m->col;
    s32 n;
    s16 k;

    if (pad & 0x20) {
        if (gSave.box[idx].used != 0) {
            if (m->mode == 3) {
                n = Equip_CountDecos(&gSave.box[idx]);
            } else {
                n = Equip_FreeSlots(&gSave.box[idx]);
            }
            if (n > 0) {
                LobbySe_Play(8);
                return 0;
            }
            if (m->mode == 3) {
                Menu_PushNoticeOnce(0x1D);
            } else {
                Menu_PushNoticeOnce(0x1C);
            }
            LobbySe_Play(9);
        } else {
            LobbySe_Play(9);
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        if (m->unk73 == 0) {
            return 3;
        }
        m->unk73 = 0;
    } else if (pad & 0x200) {
        if (m->unk73 == 0) {
            if (gSave.box[idx].id != 0) {
                m->unk73++;
            }
            m->unk5C = 0;
            m->unk5E = Equip_LevelMark(&gSave.box[idx], 0, NULL);
        } else {
            if (++m->unk73 >= 3) {
                m->unk73 = 1;
            }
            Deco_UpdateSkills(m);
        }
        LobbySe_Play(0x39);
    } else if (pad & 0x80) {
        if (m->unk73 != 0 && m->unk5E > 1) {
            if (++m->unk5C >= m->unk5E) {
                m->unk5C = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x8) {
        if (m->unk73 != 0 && m->unk5E > 1) {
            if (--m->unk5C < 0) {
                m->unk5C = m->unk5E - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x10) {
        if (m->unk73 == 0 && m->pages > 1) {
            if (++m->page >= m->pages) {
                m->page = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x4) {
        if (m->unk73 == 0 && m->pages > 1) {
            if (--m->page < 0) {
                m->page = m->pages - 1;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0xC00) {
        if (m->unk73 == 0) {
            m->col = Menu_WrapLeftRight(m->col, 10);
        }
    } else {
        if (m->unk73 == 0) {
            m->row = Menu_WrapUpDown(m->row, 10);
        }
    }
    k = m->row * 10 + m->page * 100 + m->col;
    if (idx != k) {
        m->equip = gSave.box[k];
        Deco_UpdateSkills(m);
    }
    return 2;
}

void Deco_UpdateSkills(DecoMenu *m)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;

    switch (m->unk73) {
    case 2:
        m->unk5E = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &m->equip);
        m->unk5C = 0;
        break;
    case 1:
        m->unk5E = Equip_LevelMark(&gSave.box[idx], 0, NULL);
        break;
    }
}

void Deco_UpdatePreview(DecoMenu *m)
{
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    switch (m->unk73) {
    case 3:
        tmp = m->equip;
        if (m->mode == 3) {
            Equip_RemoveDecoration(&tmp, m->cursor);
            m->unk5E = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &tmp);
        } else {
            m->unk5E = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &m->unkC);
        }
        m->unk5C = 0;
        break;
    case 1:
        m->unk5C = 0;
        m->unk5E = 3;
        break;
    case 2:
        tmp = m->equip;
        if (m->mode == 3) {
            Equip_RemoveDecoration(&tmp, m->cursor);
        } else {
            Equip_AddDecoration(&tmp, m->list[m->cursor]);
        }
        m->unk5E = Equip_LevelMark(&m->equip, 0x10, &tmp);
        m->unk5C = 0;
        break;
    default:
        m->unk5E = 0;
        m->unk5C = 0;
        break;
    }
}

s32 Deco_MenuInput(DecoMenu *m, u16 pad)
{
    if (pad & 0x20) {
        switch (m->choice) {
        case 0:
            LobbySe_Play(0x17);
            break;
        default:
            LobbySe_Play(0x1B);
            break;
        }
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    m->choice = Menu_WrapUpDownSe(m->choice, 3, 2);
    return 2;
}

/* Raw addresses kept here: the named form changes the code. */
void Deco_Attach(DecoMenu *m)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    u16 deco = m->list[m->cursor];

    Items_Take(deco, 1);
    Equip_AddDecoration(&gSave.box[idx], deco);
    Save_AddMoney(-Deco_GetPrice(deco));
    LobbySe_Play(10);
    if (m->equipped != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, idx);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
    }
    LobbySe_Play(0x3A);
    Menu_PushNotice(0x1E);
    m->equip = m->unkC;
}

/* Raw addresses kept here: the named form changes the code. */
void Deco_Remove(DecoMenu *m)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    u16 deco = m->list[m->cursor];

    Equip_RemoveDecoration(&gSave.box[idx], m->cursor);
    Save_AddMoney(-Deco_GetRemoveFee(deco));
    LobbySe_Play(10);
    if (m->equipped != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, idx);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
    }
    LobbySe_Play(0x3B);
    Menu_PushNotice(0x1F);
    m->equip = m->unkC;
}

void Deco_Remove(DecoMenu *m);
s32 Deco_ConfirmInput(DecoMenu *m, u16 pad)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    switch (m->mode) {
    case 0:
        if (Equip_CanWear(&gSave, &m->unkC) == 0) {
            m->mode = 3;
            m->equipped = 0;
        } else if (Equip_IsSlotEquipped(&gSave, idx) == 1) {
            m->mode = 3;
            m->equipped = 1;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            if (m->unkC.type == 6 || m->unkC.type == 7) {
                if (m->unkC.type != gSave.equip[0].type) {
                    m->mode = 2;
                } else {
                    m->mode = 1;
                }
            } else {
                m->mode = 1;
            }
            m->unk5C = 0;
            if (gSave.box[idx].type == 7 || gSave.box[idx].type == 6) {
                tmp = gSave.equip[0];
            } else {
                Equip_CopyEquipped(&tmp, m->unkC.type);
            }
            m->unk5E = Equip_LevelMark(&tmp, 0x10, &m->unkC);
        }
        break;
    case 1:
    case 2:
        if (pad & 0x80) {
            if (m->unk5E > 1) {
                if (++m->unk5C >= m->unk5E) {
                    m->unk5C = 0;
                }
                LobbySe_Play(1);
            }
        } else if (pad & 0x8) {
            if (m->unk5E > 1) {
                if (--m->unk5C < 0) {
                    m->unk5C = m->unk5E - 1;
                }
                LobbySe_Play(1);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case 0:
                m->mode = 3;
                m->equipped = 1;
                break;
            case 3:
                m->mode = 3;
                m->equipped = 0;
                break;
            }
        }
        break;
    case 3:
        switch (m->choice) {
        case 1:
            m->mode = 2;
            m->unk71 = 0;
            Deco_Attach(m);
            break;
        case 2:
            m->mode = 3;
            m->unk71 = 0;
            Deco_Remove(m);
            break;
        }
        m->unk73 = 0;
        m->unk5C = 0;
        WinSlide_Step(&m->win, 0x280, 0x12C);
        return 0;
    }
    return 2;
}

s32 Deco_Confirm(u8 *task, u16 pad)
{
    DecoMenu *m = (DecoMenu *)(task + 0xC);

    switch (m->choice) {
    case 1:
        return Deco_ConfirmInput(m, pad);
    case 2:
        return Deco_ConfirmInput(m, pad);
    default:
        return 0;
    }
}

extern char *D_0080C7B4;
extern char *gDecoAttachPromptTexts[];
extern char *D_0080C8B4;
extern char *gNoFreeSlotTexts[];
void Deco_DrawAttachEquip(DecoMenu *m, s32 x, s16 y)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    s32 n;
    char *s;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(10, 0x12, 0xAA, 0xFFB0B000, D_0080C7B4, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->unk71) {
    case 0:
    case 1:
        switch (m->unk73) {
        case 2:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, y, m->unk5C);
            break;
        default:
            if (m->unk73 == 0) {
                BoxMenu_DrawItemList(idx, 0x10, x);
            } else {
                BoxMenu_DrawItemList(idx, (m->unk5C & 7) | 0x78, x);
            }
            n = Equip_FreeSlots(&m->equip);
            if (n != 0) {
                s = Text_FillDollar(D_0080C8B4, n);
            } else if (m->equip.used != 0) {
                s = gNoFreeSlotTexts[0];
            } else {
                s = gDecoAttachPromptTexts[0];
            }
            Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
            break;
        }
        break;
    }
}

void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 n, u32 color);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawItemPriceRow(s16 x, s16 y, u16 item, u32 color, s32 price);
void ItemInfo_Draw(s16 x, s16 y, s32 w, u8 a, u16 item, s32 b, s8 c, u32 color); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color) (lobby/unk_7A1120.c), Deco_DrawAttachList 11 of 227 words, Deco_DrawRemovePanel 1 of 179 words, Deco_DrawRecipeDetail 1 of 148 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
extern char *D_0080C8BC;
extern char *gSlotShortTexts[];
extern char *D_0080C8C4;
extern char *gDecoChoosePromptTexts[];
extern char *gNoDecoTexts[];






void Deco_DrawAttachList(DecoMenu *m, s16 x, s16 y)
{
    u32 color;
    s16 ty = y;
    s32 ly;
    s32 lx;
    u16 sel = 0;
    s16 i;
    u16 id;
    s32 n;
    u8 *d;
    char *s;
    s32 tx;
    s32 cx;

    Win_Draw(x, y, 0x14C, 0x122, 0xBDB59F6C, 0x80202020, 2);
    if (m->list[0] == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(Text_CenterX(gNoDecoTexts[0], x + 0xA6, 0x12), 0xAE);
        Text_Printf(gNoDecoTexts[0]);
    } else {
        Font_SetColor(0x807F7F7F);
        ly = ty;
        lx = x;
        Menu_DrawPageArrows(lx + 0xF0, ly + 0xA, m->decoPage, m->decoPages, 0xFF106060);
        ty += 0x1E;
        Font_SetSpacing(0x14, 0x14);
        tx = lx + 0x32;
        cx = lx + 0xA;
        for (i = 0; i < 7; i++) {
            id = m->list[i];
            Font_SetPos(tx, ty);
            if (i == m->cursor) {
                sel = id;
                Menu_DrawLine(cx, ty - 1, 0x138, 0x18, 0);
            }
            if (id != 0) {
                if (Deco_CheckAttach(&m->equip, id) == 0) {
                    color = 0x807F7F7F;
                } else {
                    color = 0x8042352E;
                }
                Menu_DrawItemPriceRow(tx, ty, id, color, Deco_GetPrice(id));
            }
            ty += 0x16;
        }
        Gfx_DrawLine(lx + 0x14, ly + 0xBA, 0x124, 2, 0xBDB59F6C);
        if (sel != 0) {
            Font_SetColor(0x807F7F7F);
            ItemInfo_Draw((s16)(lx + 0x32) - 0x38, ly + 0xBE, 0x14C, m->unk73, (s32)sel, 0, m->unk5C, 0x807F7F7F);
        }
    }
    if (sel != 0) {
        n = Equip_FreeSlots(&m->equip);
        d = (u8 *)Item_GetDecoration(sel);
        if (Deco_FitsEquip(&m->equip, sel) == 0) {
            s = D_0080C8BC;
        } else if (n < d[0]) {
            s = gSlotShortTexts[0];
        } else if (gSave.money < *(s32 *)(d + 4)) {
            s = D_0080C8C4;
        } else {
            s = gDecoChoosePromptTexts[0];
        }
    } else {
        s = gDecoChoosePromptTexts[0];
    }
    Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
}

void Deco_DrawAttachList(DecoMenu *m, s16 x, s16 y);
extern char *D_0080C8CC;
void Deco_DrawAttachConfirm(DecoMenu *m, s16 x, s16 y)
{
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(10, 0x12, 0xAA, 0xFFB0B000, D_0080C7B4, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->unk73) {
    case 0:
    case 1:
        Deco_DrawAttachList(m, x, y);
        break;
    case 2:
        tmp = m->equip;
        Equip_AddDecoration(&tmp, m->list[m->cursor]);
        EquipCompare_Draw(&m->equip, &tmp, 0x136, y + 4, m->unk5C | 0x50);
        break;
    case 3:
        EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, 0x36, m->unk5C);
        return;
    }
    switch (m->unk71) {
    case 4:
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020,
                      Text_FillDollar(D_0080C8CC, Deco_GetPrice(m->list[m->cursor])));
        Font_SetPos(0x1BC, 0x190);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

extern char *D_0080C7B8;
extern char *D_0080C8D4;
extern char *gNoDecoAttachedTexts[];
void Deco_DrawRemoveEquip(DecoMenu *m, s32 x, s16 y)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    char *s;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(10, 0x12, 0xAA, 0xFFB0B000, D_0080C7B8, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->unk71) {
    case 0:
    case 1:
        switch (m->unk73) {
        case 2:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, y, m->unk5C);
            break;
        default:
            if (m->unk73 == 0) {
                BoxMenu_DrawItemList(idx, 0x10, x);
            } else {
                BoxMenu_DrawItemList(idx, (m->unk5C & 7) | 0x78, x);
            }
            if (Equip_CountDecos(&m->equip) != 0) {
                s = D_0080C8D4;
            } else if (m->equip.used != 0) {
                s = gNoDecoAttachedTexts[0];
            } else {
                s = D_0080C8D4;
            }
            Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
            break;
        }
        break;
    }
}

u8 Equip_GetRarity(u8 type, u16 id);
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
extern char D_0082E980[];
/* Raw addresses kept here: the named form changes the code. */
void Deco_DrawRemovePanel(DecoMenu *m, s16 x, s16 y)
{
    s16 ty = y;
    s16 tx;
    s16 by;
    s16 i;
    u16 id;
    s32 last;
    s32 n;

    Win_Draw(x, y, 0x14C, 0x122, 0xBDB59F6C, 0x80202020, 2);
    tx = x + 12;
    by = y;
    y += 14;
    Lobby_DrawEquipIcon(tx, y, 0x3C, 0x3C, &m->equip);
    Item_DrawRarity(tx + 12, y + 0x3C, Equip_GetRarity(m->equip.type, m->equip.id));
    Font_SetPos(tx + 0x4E, y + 0x14);
    Text_PrintfColored(D_0082E980, Equip_GetName(m->equip.type, m->equip.id));
    Gfx_DrawLine(tx, by + 0x5E, 0x134, 2, 0xBDB59F6C);
    ty += 0x6A;
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < 3; i++) {
        id = m->list[i];
        Font_SetPos(tx + 0x14, ty);
        if (i == m->cursor) {
            last = id;
            Menu_DrawLine(x + 10, ty - 1, 0x138, 0x18, 0);
        }
        if (id != 0) {
            n = Deco_GetRemoveFee(id);
            Menu_DrawItemPriceRow(tx + 0x28, ty, id, (gSave.money < n) ? 0x8042352E : 0x807F7F7F, n);
        }
        ty += 0x16;
    }
    Gfx_DrawLine(tx, ty, 0x134, 2, 0xBDB59F6C);
    Font_SetColor(0x807F7F7F);
    ItemInfo_Draw(tx - 0x10, ty + 12, 0x14C, m->unk73, last, 0, m->unk5C, 0x807F7F7F);
}

void Deco_DrawRemovePanel(DecoMenu *m, s16 x, s16 y);
extern char *D_0080C8DC;
extern char *gDecoRemoveWarnTexts[];
void Deco_DrawRemove(DecoMenu *m, s16 x, s16 y)
{
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(10, 0x12, 0xAA, 0xFFB0B000, D_0080C7B8, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->unk73) {
    case 0:
    case 1:
        Deco_DrawRemovePanel(m, x, y);
        break;
    case 2:
        tmp = m->equip;
        Equip_RemoveDecoration(&tmp, m->cursor);
        EquipCompare_Draw(&m->equip, &tmp, 0x136, y + 4, m->unk5C | 0x50);
        break;
    case 3:
        EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, y, m->unk5C);
        return;
    }
    switch (m->unk71) {
    case 2:
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, D_0080C8DC);
        break;
    case 3:
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020,
                      Text_FillDollar(gDecoRemoveWarnTexts[0], Deco_GetRemoveFee(m->list[m->cursor])));
        Font_SetPos(0x1BC, 0x190);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2);
s32 Deco_CanMake(s32 idx, s32 n);
extern char *gNoDecoToMakeTexts[];

void Deco_DrawMakeList(DecoMenu *m, s16 x, s16 y)
{
    s16 lx;
    s32 ly;
    Trade *t;
    u32 color;
    s16 ty = y;
    s16 i;
    u16 sel;
    char *s;

    Icon_Draw(x, y, 0x154, 0x122, 0xFFFFDDDD, 0);
    ly = ty;
    lx = x;
    Icon_Draw(lx + 0x6E, ly + 0x2D, 0x78, 0xC8, 0x60FFFFFF, 0x24);
    Font_SetSpacing(0x14, 0x14);
    if (m->count == 0) {
        s = gNoDecoToMakeTexts[0];
        Font_SetPos(Text_CenterX(s, lx + 0xAA, 0x14), ly + 0x78);
        Font_SetColor(0x8000052C);
        Text_PrintfColored(D_0082E980, s);
        return;
    }
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(lx + 0xFA, ly + 0xE, m->page, m->pages, 1);
    Font_SetSpacing(0x14, 0x14);
    ty += 0x1E;
    for (i = 0; i < m->count; i++) {
        t = &gForgeTrades[(s16)m->recipes[i]];
        if (Deco_CanMake((s16)m->recipes[i], 1) == 1) {
            color = 0x80101010;
        } else {
            color = 0x8042352E;
        }
        if (i == m->cursor) {
            sel = t->product;
            Menu_DrawCursorRow(lx + 0x46, ty, 0xFA, 0x32, 0xC0106060, 0xFF106060);
        }
        Menu_DrawItemPriceRow(lx + 0x46, ty, t->product, color, Item_SellPrice(t->product));
        ty += 0x16;
    }
    Menu_DrawDashLine(lx + 0x14, ly + 0xBA, 0x12C, 0xFF000000);
    ItemInfo_Draw(lx + 0xA, ly + 0xC3, 0x14C, m->unk73, (s32)sel, 0, -1, 0x80101010);
}

void Deco_DrawRecipeList(DecoMenu *m, s16 x, s16 y)
{
    Trade *t;
    s32 color;
    s16 cy = y;
    s16 i;
    s16 idx;

    Icon_Draw(x, y, 0x122, 0xB4, 0xFFFFDDDD, 0);
    Icon_Draw(x + 0x64, y + 0xF, 0x5A, 0x96, 0x60FFFFFF, 0x24);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xC8, y + 0xC, m->page, m->pages, 1);
    Font_SetSpacing(0x11, 0x11);
    cy += 0x1E;
    for (i = 0; i < m->count; i++) {
        idx = m->recipes[i];
        t = &gForgeTrades[idx];
        if (Deco_CanMake(idx, 1) == 1) {
            color = 0x80101010;
        } else {
            color = 0x8042352E;
        }
        if (i == m->cursor) {
            Menu_DrawCursorRow((s16)(x + 0x3C) - 6, cy, 0xC8, 0x32, 0xC0106060, 0xFF106060);
        }
        Menu_DrawItemPriceRow(x + 0x3C, cy, t->product, color, Item_SellPrice(t->product));
        cy += 0x14;
    }
}

s32 Items_DrawNeedLineAfter(s16 x, s16 y, u16 item, s32 n, u32 color);
extern char *gSmithyLabelTexts[];
void Deco_DrawRecipeDetail(DecoMenu *m, s16 x, s16 y)
{
    Trade *t = &gForgeTrades[m->recipes[m->cursor]];
    s16 ty = y;
    s32 product = t->product;
    s16 i;

    Win_Draw(x, y, 0x14C, 0x122, 0xBDB59F6C, 0x80202020, 2);
    ItemInfo_Draw(x - 6, ty += 0x18, 0x14C, m->unk73 - 1, product, 0, m->unk5C, 0x807F7F7F);
    Gfx_DrawLine(x + 0x14, ty += 0x6E, 0x124, 2, 0xBDB59F6C);
    ty += 0xE;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0xE, ty);
    if (m->unk71 == 2 || m->unk71 == 3) {
        Text_PrintfColored(gSmithyLabelTexts[0]);
    } else {
        Text_PrintfColored(D_0080C82C);
    }
    Font_SetColor(0x807F7F7F);
    x += 0x22;
    for (i = 0; i < 4; i++) {
        if (t->need[i].item != 0) {
            if (m->unk71 == 2 || m->unk71 == 3) {
                Items_DrawNeedLineAfter(x, ty += 0x14, t->need[i].item, t->need[i].count * m->quantity, 0x807F7F7F);
            } else {
                Items_DrawNeedLine(x, ty += 0x14, t->need[i].item, t->need[i].count, 0x807F7F7F);
            }
        }
    }
}

extern char *gDecoMenuTexts[];
void Deco_DrawMakeList(DecoMenu *m, s16 x, s16 y);
void Sjis_Widen(char *src, char *dst);
s32 sprintf(char *buf, const char *fmt, ...);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
extern char *gPriceConfirmTexts;
extern char *D_0080C8EC;
extern char *gDecoMakePromptTexts[];
void Deco_DrawMake(DecoMenu *m, s16 x, s16 y)
{
    char buf[16];
    char wide[16];
    s32 idx;
    s32 fee;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(10, 0x12, 0xAA, 0xFFB0B000, gDecoMenuTexts[0], 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->unk71) {
    case 3:
    case 2:
        Deco_DrawRecipeList(m, 0, y);
        Deco_DrawRecipeDetail(m, x, y);
        break;
    default:
        switch (m->unk73) {
        case 0:
            Deco_DrawMakeList(m, x, y);
            break;
        case 1:
        case 2:
            Deco_DrawRecipeList(m, 0, y);
            Deco_DrawRecipeDetail(m, x, y);
            break;
        }
        break;
    }
    switch (m->unk71) {
    case 3:
        fee = Item_SellPrice(gForgeTrades[m->recipes[m->cursor]].product);
        fee *= m->quantity;
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, Text_FillDollar(gPriceConfirmTexts, fee));
        Font_SetPos(0x1B2, 0x192);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    case 2:
        idx = m->recipes[m->cursor];
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, D_0080C8EC);
        Font_SetPos(0x20B, 0x184);
        sprintf(buf, "%02d", m->quantity);
        Sjis_Widen(buf, wide);
        Text_PrintfColored(wide);
        if (Deco_CanMake(idx, m->quantity + 1) == 1 && m->quantity <= 98) {
            Icon_DrawMode(0x20C, 0x16E, 0x24, 0x16, 0xFF00FF00, 0x29, 2);
        }
        if (m->quantity > 1) {
            Icon_DrawMode(0x20D, 0x196, 0x24, 0x16, 0xFF00FF00, 0x28, 2);
        }
        break;
    default:
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, gDecoMakePromptTexts[0]);
        break;
    }
}

void Deco_DrawMake(DecoMenu *m, s16 x, s16 y);
void Deco_DrawRemove(DecoMenu *m, s16 x, s16 y);
void Deco_Draw(u8 *task)
{
    DecoMenu *m = (DecoMenu *)(task + 0xC);

    switch (m->mode) {
    case 0:
        Font_SetSpacing(0x14, 0x14);
        Menu_DrawLabelList(0x1CC, gDecoMenuTexts, 3, m->choice, 0xFFB0B000);
        break;
    case 1:
        Deco_DrawMake(m, m->win.pos, 0x34);
        break;
    case 2:
        switch (m->unk71) {
        case 2:
        case 3:
        case 4:
            Deco_DrawAttachConfirm(m, m->win2.pos, 0x34);
            break;
        default:
            Deco_DrawAttachEquip(m, (s16)(m->win.pos + 12), 0x34);
            break;
        }
        break;
    case 3:
        switch (m->unk71) {
        case 0:
        case 1:
            Deco_DrawRemoveEquip(m, (s16)(m->win.pos + 12), 0x34);
            break;
        case 2:
        case 3:
            Deco_DrawRemove(m, m->win2.pos, 0x34);
            break;
        }
        break;
    }
}

void Deco_Draw2(u8 *unused0) {

}

extern char *gWeaponChangeWarnTexts[];
void Deco_DrawEquipAsk(DecoMenu *m, s16 k)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;
    char *s;

    memset(&tmp, 0, sizeof(tmp));
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(10, 0x12, 0x8C, 0xFFB0B000, D_0080C7B4, 2, 1);
    switch (m->mode) {
    case 1:
        s = D_0080C814;
        break;
    case 2:
        s = gWeaponChangeWarnTexts[0];
        break;
    default:
        return;
    }
    if (gSave.box[idx].type == 7 || gSave.box[idx].type == 6) {
        tmp = gSave.equip[0];
    } else {
        Equip_CopyEquipped(&tmp, m->unkC.type);
    }
    EquipCompare_Draw(&tmp, &m->unkC, 0x136, 0x38, m->unk5C | 0x50);
    Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
    Font_SetPos(0x144, 0x190);
    Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
}

void Deco_DrawEquipPrompt(u8 *task)
{
    DecoMenu *m = (DecoMenu *)(task + 0xC);
    s16 k = m->choice;

    switch (k) {
    case 1:
    case 2:
        Deco_DrawEquipAsk(m, k);
        break;
    }
}

void WeaponForge_Open(s8 *task)
{
    s8 *m;

    task[0x11C] = 1;
    task[0x11D] = task[3];
    m = task + 0xC;
    if (task[0x11D] != 0) {
        m[0x110] = 1;
        *(s16 *)(m + 0x102) = 0;
        *(s16 *)(m + 0xF4) = 0;
        m[0x10C] = 2;
        func_006E01C0((WeaponMenu *) m);
        Chat_SetColor(0);
    }
}

s32 Forge_IsFixedPrice(s16 id) {
    return id >= 900;
}

void WeaponForge_OpenUpgrades(WeaponMenu *);
s32 WeaponForge_UpgradeInput(WeaponMenu *m, u16 pad);
s32 WeaponForge_MakeInput(WeaponMenu *m, u16 pad);
s32 WeaponForge_Input(u8 *task, u16 pad)
{
    WeaponMenu *m = (WeaponMenu *)(task + 0xC);

    switch (m->step) {
    case 0:
        if (pad & 0x20) {
            m->unk110 = 1;
            m->row = 0;
            if (m->choice == 0) {
                m->step = 2;
                func_006E01C0(m);
            } else {
                EquipPreview_Reset();
                m->step = 1;
                WeaponForge_OpenUpgrades(m);
                CamFocus_SetAside(1);
            }
            LobbySe_Play(0x1B);
            Chat_SetColor(0);
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else {
            m->choice = Menu_WrapUpDownSe(m->choice, 2, 2);
        }
        break;
    case 1:
        switch (WeaponForge_UpgradeInput(m, pad)) {
        case 0:
            m->step = 0;
            return 0;
        case 3:
            m->step = 0;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case 2:
        switch (WeaponForge_MakeInput(m, pad)) {
        case 0:
            m->step = 0;
            return 0;
        case 3:
            if (m->filter != 0) {
                Chat_SetColor(-1);
                return 3;
            }
            m->step = 0;
            Chat_SetColor(-1);
            break;
        }
        break;
    }
    return 2;
}

/* Raw addresses kept here: the named form changes the code. */
void WeaponForge_Make(WeaponMenu *m)
{
    ForgeRecipe *r = &gWeaponRecipes[m->list[m->cursor]];
    EquipItem tmp;
    s32 price;
    u16 *id;
    u8 *type;
    s32 i;
    s32 slot;

    memset(&tmp, 0, sizeof(tmp));
    if (Forge_IsFixedPrice(r->id)) {
        tmp = m->orig;
        price = 1000;
    } else {
        type = &tmp.type;
        id = &tmp.id;
        *type = r->type;
        *id = r->id;
        price = Equip_HalfPrice(*type, *id);
    }
    for (i = 0; i < 4; i++) {
        Items_Take(r->need[i].item, r->need[i].count);
    }
    slot = Box_AddItem(&gSave, tmp.type, tmp.id, 0);
    Save_AddMoney(-price);
    LobbySe_Play(0xA);
    if (m->equipped != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, slot);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
        Menu_PushNotice(0x17);
        LobbySe_Play(0x12);
    } else {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(0xF);
    }
}

s32 WeaponForge_MakeConfirm(WeaponMenu *m, u16 pad);
s32 WeaponForge_UpgradeConfirm(WeaponMenu *m, u16 pad);
void WeaponForge_Make(WeaponMenu *m);
s32 WeaponForge_MakeConfirm(WeaponMenu *m, u16 pad)
{
    ForgeRecipe *r = &gWeaponRecipes[m->list[m->cursor]];
    EquipItem tmp;
    u8 *type;

    memset(&tmp, 0, sizeof(tmp));
    type = &tmp.type;
    *type = r->type;
    tmp.id = r->id;
    switch (m->step) {
    case 0:
        m->detailPages = Equip_LevelMark(&gSave.equip[0], 0x10, &tmp);
        Menu_SetYesNo(0);
        gMenuInputLock = 9;
        if (*type != gSave.equip[0].type) {
            m->step = 2;
        } else {
            m->step = 1;
        }
        break;
    case 1:
    case 2:
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
            switch (Menu_YesNoInput()) {
            case 0:
                m->step = 3;
                m->equipped = 1;
                break;
            case 3:
                m->step = 3;
                m->equipped = 0;
                break;
            }
        }
        break;
    case 3:
        m->step = 2;
        m->unk10D = 1;
        m->mode = 0;
        WeaponForge_Make(m);
        WinSlide_Step(&m->win, 0x280, 0x118);
        return 0;
    }
    return 2;
}

void WeaponForge_Upgrade(WeaponMenu *m);
void WeaponForge_LoadUpgrades(WeaponMenu *m);
s32 WeaponForge_UpgradeConfirm(WeaponMenu *m, u16 pad)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;

    memset(&tmp, 0, sizeof(tmp));
    switch (m->step) {
    case 0:
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
        m->detailPages = Equip_LevelMark(&gSave.equip[0], 0x10, &tmp);
        if (Equip_IsSlotEquipped(&gSave, idx) == 1) {
            m->equipped = 1;
            m->step = 3;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            if (gSave.box[idx].type != gSave.equip[0].type) {
                m->step = 2;
            } else {
                m->step = 1;
            }
        }
        break;
    case 1:
    case 2:
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
            switch (Menu_YesNoInput()) {
            case 0:
                m->step = 3;
                m->equipped = 1;
                break;
            case 3:
                m->step = 3;
                m->equipped = 0;
                break;
            }
        }
        break;
    case 3:
        m->step = 1;
        m->unk10D = 0;
        m->mode = 0;
        WeaponForge_Upgrade(m);
        m->cursor = 0;
        WeaponForge_LoadUpgrades(m);
        return 0;
    }
    return 2;
}

s32 WeaponForge_Confirm(u8 *task, u16 pad)
{
    WeaponMenu *m = (WeaponMenu *)(task + 0xC);

    if (m->choice == 0) {
        return WeaponForge_MakeConfirm(m, pad);
    }
    return WeaponForge_UpgradeConfirm(m, pad);
}

void Box_GunLevelUp(SaveData *save, s16 idx);
void Box_SetGunOption20(SaveData *save, s16 idx, s32 off);
void Box_SetGunOption10(SaveData *save, s16 idx, s32 off);
void Box_SetGunOption40(SaveData *save, s16 idx, s32 off);
void Box_SetGunShield(SaveData *save, s16 idx, s32 off);
void WeaponForge_LoadGunMods(WeaponMenu *m, s32 idx);
/* Raw addresses kept here: the named form changes the code. */
void WeaponForge_Upgrade(WeaponMenu *m)
{
    s32 idx = m->row * 10 + m->page * 100 + m->col;
    EquipItem tmp;
    s32 cost;
    s32 i;

    tmp = gSave.box[idx];
    switch (m->kind) {
    case 6:
        tmp = m->upgrades[m->cursor];
        cost = m->cost[m->cursor];
        for (i = 0; i < 3; i++) {
            Items_Take(m->mats[m->cursor].need[i].item, m->mats[m->cursor].need[i].count);
        }
        tmp.level = 0;
        Box_SetSlot(&gSave, idx, &tmp);
        Save_AddMoney(-cost);
        LobbySe_Play(0xA);
        break;
    case 7:
        switch (m->msg[m->cursor]) {
        case 19:
            Box_GunLevelUp(&gSave, idx);
            break;
        case 20:
            Box_SetGunOption10(&gSave, idx, 0);
            break;
        case 21:
            Box_SetGunOption10(&gSave, idx, 1);
            break;
        case 22:
        case 28:
            Box_SetGunOption20(&gSave, idx, 0);
            break;
        case 23:
        case 29:
            Box_SetGunOption20(&gSave, idx, 1);
            break;
        case 24:
            Box_SetGunOption40(&gSave, idx, 0);
            break;
        case 25:
            Box_SetGunOption40(&gSave, idx, 1);
            break;
        case 26:
            Box_SetGunShield(&gSave, idx, 0);
            break;
        case 27:
            Box_SetGunShield(&gSave, idx, 1);
            break;
        }
        Save_AddMoney(-m->gunCost[m->cursor]);
        LobbySe_Play(0xA);
        break;
    }
    if (m->equipped != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, idx);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
        Menu_PushNotice(0x17);
        LobbySe_Play(0x12);
    } else {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(0xF);
    }
    if (m->kind == 7) {
        WeaponForge_LoadGunMods(m, idx);
    }
}

s32 EquipEnt_HalfPrice(u8 *e)
{
    return (s32)Equip_GetPrice(e[1], *(u16 *)(e + 2)) / 2;
}

s32 WeaponForge_IsClass10(EquipItem *e) {
    return (Weapon_GetClass(e->type, e->id) & 0xFF) == 0xA;
}

void func_006E01C0(WeaponMenu *m) {
    m->kind = 6;
}

/* unk_6D5330's .data (0x7E84D0-0x7E9360) */
GuideEntry gGuideBasicsEntries_0[7] = {
#include "lobby/007E84D0.inc"
};
GuideEntry gGuideBasicsEntries_1[4] = {
#include "lobby/007E8560.inc"
};
GuideEntry gGuideBasicsEntries_2[8] = {
#include "lobby/007E85B0.inc"
};
GuideEntry gGuidePlaceEntries_0[5] = {
#include "lobby/007E8650.inc"
};
GuideEntry gGuidePlaceEntries_1[8] = {
#include "lobby/007E86C0.inc"
};
GuideEntry gGuidePlaceEntries_2[2] = {
#include "lobby/007E8760.inc"
};
GuideEntry gGuidePlaceEntries_3[4] = {
#include "lobby/007E8790.inc"
};
GuideEntry gGuidePlaceEntries_4[4] = {
#include "lobby/007E87E0.inc"
};
GuideEntry gGuidePlaceEntries_5[2] = {
#include "lobby/007E8830.inc"
};
GuideEntry gGuidePlaceEntries_6[2] = {
#include "lobby/007E8860.inc"
};
GuideEntry gGuideDragonEntries_0[9] = {
#include "lobby/007E8890.inc"
};
GuideEntry gGuideDragonEntries_1[3] = {
#include "lobby/007E8950.inc"
};
char *gGuidePageTexts[36] = {
    D_0082CF20, D_0082CFF0, D_0082D090, D_0082D140,
    D_0082D210, D_0082D2C0, D_0082D390, D_0082D470,
    D_0082D560, D_0082D610, D_0082D6D0, D_0082D770,
    D_0082D800, D_0082D8A0, D_0082D980, D_0082DA70,
    D_0082DB50, D_0082DC20, D_0082DCC0, D_0082DD60,
    D_0082DDF0, D_0082DE90, D_0082DF90, D_0082E020,
    D_0082E0E0, D_0082E1D0, D_0082E290, D_0082E340,
    D_0082E400, D_0082E4B0, D_0082E5B0, D_0082E650,
    D_0082E730, NULL, NULL, NULL,
};
char *gGuideBasicsTopics[4] = {
    D_0082E810, D_0082E830, D_0082E850, NULL,
};
char *gGuidePlaceTopics[8] = {
    D_0082E868, D_0082E870, D_0082E880, D_0082E890,
    D_0082E8A0, D_0082E8B0, D_0082E8C0, NULL,
};
char *gGuideDragonTopics[4] = {
    D_0082E8D8, D_0082E8E8, NULL, NULL,
};
GuideEntry *gGuideBasicsEntries[4] = {
    gGuideBasicsEntries_0, gGuideBasicsEntries_1, gGuideBasicsEntries_2, NULL,
};
GuideEntry *gGuidePlaceEntries[8] = {
    gGuidePlaceEntries_0, gGuidePlaceEntries_1, gGuidePlaceEntries_2, gGuidePlaceEntries_3,
    gGuidePlaceEntries_4, gGuidePlaceEntries_5, gGuidePlaceEntries_6, NULL,
};
GuideEntry *gGuideDragonEntries[4] = {
    gGuideDragonEntries_0, gGuideDragonEntries_1, NULL, NULL,
};
s16 gEquipTabTypes[8] = {
#include "lobby/007E8AA0.inc"
};
Trade gForgeTrades[123] = {
#include "lobby/007E8AB0.inc"
};
