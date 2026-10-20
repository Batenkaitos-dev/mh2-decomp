/*
 * The records menu's lists and replies (TOWN_MODE_RECORDS; the menu's
 * state is the RecordMenu view of gMenuWork).
 *
 * The records shown are those of the house's owner. When the local player
 * owns the house (or plays offline) they come from gSave; otherwise the
 * menu asks the owner by town message and the owner's answers arrive here:
 * Records_RecvEquipBits, Records_RecvEquipTally, Records_RecvItemBits,
 * Records_RecvTitleBits and Records_RecvMonsterRecords each take a reply
 * only while the menu waits for it (RECORDS_STEP_WAIT) and only from the
 * owner. The file also counts the equipment the owner can wear and has
 * owned (per weapon genre, enum RecordsGenre, and per armour type), builds
 * the equipment list's pages, and opens the menu with the entries the
 * player may see (enum RecordKind). Records_Update runs the menu and its
 * screens; Records_Draw and Records_DrawMenuLayer draw them.
 */
#include "common.h"
#include "lobby_state.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "area.h"
void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
void LobbySe_Play(s32);

s32 Game_IsOnline(void);

void Records_SetEquipPage(RecordMenu *m, s16 page);
extern u8 gMenuWork[];
extern NetUser gNetUsers[];
extern s32 gTownMode;
s32 memcmp(void *a, void *b, s32 n);

/* The house owner's slot in gNetUsers: LobbyState.memberSlot holds it as
 * the digit character '0' + slot. */
#define RECORDS_OWNER_SLOT() ((u8)gLobby->memberSlot[gScene.localPlayer] - '0')

/*
 * Town messages 0x32 to 0x38: the house owner's packed flags of one
 * equipment type (equipType, an EquipItem type). Taken only while the
 * equipment screen waits for them and only from the owner: fills
 * equipBits, builds the list's first page and moves on to the list.
 */
void Records_RecvEquipBits(void *from, s32 equipType, void *bits)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    u16 ownerSlot = RECORDS_OWNER_SLOT();

    if (gTownMode == TOWN_MODE_RECORDS && memcmp(gNetUsers[ownerSlot].id, from, NET_USER_ID_MATCH) == 0 &&
        menu->mode == RECORDS_MODE_EQUIP && menu->step == RECORDS_STEP_WAIT) {
        menu->equipType = equipType;
        memcpy(menu->equipBits, bits, RECORDS_EQUIP_BITS / 8);
        Records_SetEquipPage(menu, 0);
        menu->step++;
    }
}

u8 Weapon_GetClass(u8 type, u16 id);

/* 1 if the weapon's class (Weapon_GetClass, enum WeaponType) belongs to
 * the records genre (enum RecordsGenre); else 0. */
s32 Weapon_IsInGenre(EquipItem *item, s16 genre)
{
    switch ((u8)Weapon_GetClass(item->type, item->id)) {
    case WEAPON_GREAT_SWORD:
    case WEAPON_LONG_SWORD:
        if (genre == RECORDS_GENRE_GREAT_SWORDS) {
            return 1;
        }
        break;
    case WEAPON_SWORD_SHIELD:
    case WEAPON_DUAL_SWORDS:
        if (genre == RECORDS_GENRE_SMALL_SWORDS) {
            return 1;
        }
        break;
    case WEAPON_HAMMER:
    case WEAPON_HUNTING_HORN:
        if (genre == RECORDS_GENRE_HAMMERS) {
            return 1;
        }
        break;
    case WEAPON_LANCE:
    case WEAPON_GUNLANCE:
        if (genre == RECORDS_GENRE_LANCES) {
            return 1;
        }
        break;
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
    default:
        if (genre == RECORDS_GENRE_GUNS) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Weapon_IsInGenre(EquipItem *item, s16 genre);

/* 1 if a piece of equipment belongs in the list shown: every piece in the
 * armour list, else a weapon of the genre chosen (RecordMenu.genre). */
s32 Records_IsInGenre(RecordMenu *menu, EquipItem *item)
{
    if (menu->kind[menu->menuSel] == RECORD_KIND_ARMOR) {
        return 1;
    }
    return Weapon_IsInGenre(item, menu->genre);
}

s32 MyHouse_InOwnHouse(void);
s8 MyHouse_LocalSlot(void);
u8 Equip_GetWearMask(SaveData *save, EquipItem *item);

/* The house owner's player slot (MyHouse_LocalSlot). */
static inline s8 Records_OwnerPlayerSlot(void)
{
    return MyHouse_LocalSlot();
}

/*
 * 1 if the piece has an id and the records' owner can wear it (the owner's
 * gender against Equip_GetWearMask): the local player in their own house,
 * else the house's owner. 0 otherwise.
 */
s32 Records_OwnerCanWear(EquipItem *item)
{
    Player *pl;

    pl = (MyHouse_InOwnHouse() == 1) ? &gPlayers[gScene.localPlayer] : &gPlayers[Records_OwnerPlayerSlot()];
    if (item->id == 0) {
        return 0;
    }
    if (Equip_GetWearMask(&gSave, item) & (1 << pl->gender)) {
        return 1;
    }
    return 0;
}

s32 Equip_TestFlag(EquipItem *item, s16 bit);

/*
 * Counts, per weapon genre, the weapons the owner can wear: out holds
 * RECORDS_GENRE_COUNT pairs (owned, in all), the first four genres from
 * the blade table, the guns' genre from the gun table. Owned is
 * EQUIP_FLAG_OWNED.
 */
void Records_TallyWeapons(s32 *out)
{
    EquipItem item;
    s16 total;
    s16 owned;
    s16 i;
    s16 genre;
    u16 *id = &item.id;
    s32 *pair;
    u8 *type = &item.type;

    genre = RECORDS_GENRE_GREAT_SWORDS;
    *type = EQUIP_BLADE;
    pair = out;
    do {
        total = 0;
        owned = 0;
        for (i = 0; i < BLADE_ENTRY_COUNT; i++) {
            *id = i;
            if (Weapon_IsInGenre(&item, genre) == 1 && Records_OwnerCanWear(&item) != 0) {
                if (Equip_TestFlag(&item, EQUIP_FLAG_OWNED) == 1) {
                    owned++;
                }
                total++;
            }
        }
        *pair++ = owned;
        *pair++ = total;
        genre++;
    } while (genre < RECORDS_GENRE_GUNS);
    total = 0;
    *type = EQUIP_GUN;
    owned = 0;
    for (i = 0; i < GUN_ENTRY_COUNT; i++) {
        *id = i;
        if (Weapon_IsInGenre(&item, RECORDS_GENRE_GUNS) == 1 && Records_OwnerCanWear(&item) != 0) {
            if (Equip_TestFlag(&item, EQUIP_FLAG_OWNED) == 1) {
                owned++;
            }
            total++;
        }
    }
    out[RECORDS_GENRE_GUNS * 2] = owned;
    out[RECORDS_GENRE_GUNS * 2 + 1] = total;
}

/*
 * The same for the armour the owner can wear: five pairs (owned, in all),
 * for the head, body, arms, waist and legs, in the menu's order.
 */
void Records_TallyArmor(s32 *out)
{
    EquipItem item;
    s16 count;
    s16 total;
    u8 *typeField = &item.type;
    s16 owned;
    s16 i;
    s16 part;
    s32 *pair;
    s16 type;

    for (pair = out, part = 0; part < 5; part++, pair += 2) {
        switch (part) {
        case 0:
            type = EQUIP_HEAD;
            count = HEAD_ENTRY_COUNT;
            break;
        case 1:
            type = EQUIP_BODY;
            count = BODY_ENTRY_COUNT;
            break;
        case 2:
            type = EQUIP_ARMS;
            count = ARMS_ENTRY_COUNT;
            break;
        case 3:
            count = WAIST_ENTRY_COUNT;
            type = EQUIP_WAIST;
            break;
        case 4:
            count = LEGS_ENTRY_COUNT;
            type = EQUIP_LEGS;
            break;
        }
        *typeField = type;
        total = 0;
        owned = 0;
        for (i = 0; i < count; i++) {
            item.id = i;
            if (Records_OwnerCanWear(&item) != 0) {
                if (Equip_TestFlag(&item, EQUIP_FLAG_OWNED) == 1) {
                    owned++;
                }
                total++;
            }
        }
        pair[0] = owned;
        pair[1] = total;
    }
}

s32 Bits_Test(u8 *bits, u16 n);
s32 Menu_CountPages(s16 n, s16 per);

/*
 * Fills RecordMenu.items with page `page` (RECORDS_PAGE_ROWS rows) of the
 * pieces of equipType that the owner can wear, whose flag is set in
 * equipBits and that fit the genre; sets page and pages, and keeps sel on
 * a row.
 */
void Records_SetEquipPage(RecordMenu *menu, s16 page)
{
    s16 shown;
    s16 first;
    EquipItem item;
    s16 count;
    s16 last;
    s16 i;

    switch (menu->equipType) {
    case EQUIP_BLADE:
        count = BLADE_ENTRY_COUNT;
        break;
    case EQUIP_GUN:
        count = GUN_ENTRY_COUNT;
        break;
    case EQUIP_HEAD:
        count = HEAD_ENTRY_COUNT;
        break;
    case EQUIP_BODY:
        count = BODY_ENTRY_COUNT;
        break;
    case EQUIP_ARMS:
        count = ARMS_ENTRY_COUNT;
        break;
    case EQUIP_WAIST:
        count = WAIST_ENTRY_COUNT;
        break;
    case EQUIP_LEGS:
        count = LEGS_ENTRY_COUNT;
        break;
    }
    memset(&item, 0, sizeof(item));
    item.type = menu->equipType;
    menu->rowCount = 0;
    shown = 0;
    first = page * RECORDS_PAGE_ROWS;
    last = first + RECORDS_PAGE_ROWS - 1;
    for (i = 0; i < count; i++) {
        item.id = i;
        if (Records_OwnerCanWear(&item) && Bits_Test(menu->equipBits, item.id) == 1 && Records_IsInGenre(menu, &item) == 1) {
            if (shown >= first && shown <= last) {
                menu->items[menu->rowCount] = item;
                menu->rowCount++;
            }
            shown++;
        }
    }
    menu->page = page;
    menu->pages = Menu_CountPages(shown, RECORDS_PAGE_ROWS);
    if (menu->sel >= menu->rowCount) {
        menu->sel = menu->rowCount - 1;
    }
}

/* Town messages 0x3A and 0x3B: the owner's weapon or armour counts, taken
 * while the menu waits for them; the page then opens. */
void Records_RecvEquipTally(void *from, void *tally)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    u16 ownerSlot = RECORDS_OWNER_SLOT();

    if (gTownMode == TOWN_MODE_RECORDS && memcmp(gNetUsers[ownerSlot].id, from, NET_USER_ID_MATCH) == 0 &&
        menu->mode == RECORDS_MODE_MENU && menu->step == RECORDS_STEP_WAIT) {
        memcpy(menu->equipTally, tally, sizeof(menu->equipTally));
        menu->step = RECORDS_STEP_ARRIVED;
    }
}

/* Town message 0x2C: the owner's items-obtained bits, taken the same way. */
void Records_RecvItemBits(void *from, void *bits)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    u16 ownerSlot = RECORDS_OWNER_SLOT();

    if (gTownMode == TOWN_MODE_RECORDS && memcmp(gNetUsers[ownerSlot].id, from, NET_USER_ID_MATCH) == 0 &&
        menu->mode == RECORDS_MODE_MENU && menu->step == RECORDS_STEP_WAIT) {
        memcpy(menu->itemBits, bits, sizeof(menu->itemBits));
        menu->step = RECORDS_STEP_ARRIVED;
    }
}

/* Town message 0x2D: the owner's title bits, taken the same way. */
void Records_RecvTitleBits(void *from, void *bits)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    u16 ownerSlot = RECORDS_OWNER_SLOT();

    if (gTownMode == TOWN_MODE_RECORDS && memcmp(gNetUsers[ownerSlot].id, from, NET_USER_ID_MATCH) == 0 &&
        menu->mode == RECORDS_MODE_MENU && menu->step == RECORDS_STEP_WAIT) {
        memcpy(menu->titleBits, bits, sizeof(menu->titleBits));
        menu->step = RECORDS_STEP_ARRIVED;
    }
}

/*
 * Town messages 0x2E to 0x30, one part of the owner's monster records
 * each: part 0 the largest sizes, 1 the smallest (a byte per monster,
 * widened), 2 the hunt counts, which end the wait.
 */
void Records_RecvMonsterRecords(void *from, u8 *data, s8 part)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    u16 ownerSlot = RECORDS_OWNER_SLOT();
    s32 i;

    if (gTownMode == TOWN_MODE_RECORDS && memcmp(gNetUsers[ownerSlot].id, from, NET_USER_ID_MATCH) == 0 &&
        menu->mode == RECORDS_MODE_MENU && menu->step == RECORDS_STEP_WAIT) {
        switch (part) {
        case 0:
            for (i = 0; i < MONSTER_RECORD_COUNT; i++) {
                menu->sizeMax[i] = data[i];
            }
            break;
        case 1:
            for (i = 0; i < MONSTER_RECORD_COUNT; i++) {
                menu->sizeMin[i] = data[i];
            }
            break;
        case 2:
            memcpy(menu->hunts, data, MONSTER_RECORD_COUNT * sizeof(u16));
            menu->step = RECORDS_STEP_ARRIVED;
            break;
        }
    }
}

extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
extern s16 gMenuOffsetX;
extern s16 gMenuOffsetY;
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Records_Draw(MenuTask *);
void Records_DrawMenuLayer(MenuTask *);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 MyHouse_IsOwner(void);

/*
 * Opens the records menu (TOWN_MODE_RECORDS): clears gMenuWork, sets the
 * draw callbacks, ignores the pad for 6 frames, starts the window's slide
 * in and lists the entries the player may see: the house's access online
 * and for the owner only, the titles for the owner only, each score page
 * for the owner once its unlocking item (0x2C1, 0x2C2) is obtained, the
 * others always.
 */
void Records_Open(void)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    s32 i;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_RECORDS;
    memset(menu, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Records_Draw, Records_DrawMenuLayer, 0, 0);
    gMenuInputLock = 6;
    WinSlide_Step(&menu->win, 0x280, 0x118);
    LobbySe_Play(0x16); /* the window opens */
    menu->menuCount = 0;
    for (i = 0; i < RECORD_KIND_COUNT; i++) {
        switch (i) {
        case RECORD_KIND_HOUSE_ACCESS:
            if (Game_IsOnline() == 1 && MyHouse_IsOwner()) {
                menu->kind[menu->menuCount] = i;
                menu->menuCount++;
            }
            break;
        case RECORD_KIND_TITLES:
            if (MyHouse_IsOwner()) {
                menu->kind[menu->menuCount] = i;
                menu->menuCount++;
            }
            break;
        case RECORD_KIND_SCORES_A:
            if (Save_IsItemObtained(&gSave, 0x2C1) && MyHouse_IsOwner()) {
                menu->kind[menu->menuCount] = i;
                menu->menuCount++;
            }
            break;
        case RECORD_KIND_SCORES_B:
            if (Save_IsItemObtained(&gSave, 0x2C2) && MyHouse_IsOwner()) {
                menu->kind[menu->menuCount] = i;
                menu->menuCount++;
            }
            break;
        default:
            menu->kind[menu->menuCount] = i;
            menu->menuCount++;
            break;
        }
    }
    gMenuOffsetX = -0x14;
    gMenuOffsetY = 0x28;
}
