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

s32 LobbyWorld_GetSeason(void);
struct QuestGoal;
s32 Wanted_MatchesQuest(struct QuestGoal *a, TownEntry *b);
s32 QuestDesk_IsWanted(u8 *p)
{
    TownQuestLists *t = &gTownQuestLists;
    s32 i = LobbyWorld_GetSeason();

    if (gScene.area == AREA_HUB_0x88) {
        if (Wanted_MatchesQuest((struct QuestGoal *)(p + 0x2C), t->b[i]) == 1) {
            return 1;
        }
    } else if (gScene.area == AREA_HUB_0x86 || Game_IsOnline() == 0) {
        if (Wanted_MatchesQuest((struct QuestGoal *)(p + 0x2C), t->a[i]) == 1) {
            return 1;
        }
    }
    return 0;
}

s16 Wanted_GetValue(void)
{
    u16 area;

    if (Game_IsOnline() == 0) {
        return 0x3E8;
    }
    area = gScene.area;
    switch (area) {
    case AREA_HUB_0x86:
        return *(s16 *)((u8 *)(gLobbyWorkBuf) + 0x7AAA);
    case AREA_HUB_0x88:
        return *(s16 *)((u8 *)(gLobbyWorkBuf) + 0x7AAC);
    default:
        return 0;
    }
}

void Records_LoadEquipPage(RecordMenu *m, s16 page);
extern u8 gMenuWork[];
extern u8 gNetUsers[];
extern s32 gTownMode;
s32 memcmp(void *a, void *b, s32 n);
void Records_RecvPage(void *id, s32 idx, void *data)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    u16 k = (u8)gLobby->memberSlot[gScene.localPlayer] - 0x30;

    if (gTownMode == 0x46 && memcmp(gNetUsers + k * 0x11C, id, 6) == 0 && m->mode == 10 &&
        m->step == 1) {
        m->equipType = idx;
        memcpy(m->equipBits, data, 0x50);
        Records_LoadEquipPage(m, 0);
        m->step++;
    }
}

u8 Weapon_GetClass(u8 type, u16 id);
s32 Weapon_IsKind(EquipItem *e, s16 kind)
{
    switch ((u8)Weapon_GetClass(e->type, e->id)) {
    case 0:
    case 7:
        if (kind == 0) {
            return 1;
        }
        break;
    case 4:
    case 6:
        if (kind == 1) {
            return 1;
        }
        break;
    case 2:
    case 8:
        if (kind == 2) {
            return 1;
        }
        break;
    case 3:
    case 9:
        if (kind == 3) {
            return 1;
        }
        break;
    case 1:
    case 5:
    default:
        if (kind == 4) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Weapon_IsKind(EquipItem *e, s16 kind);
s32 Records_MatchesKind(RecordMenu *m, EquipItem *item)
{
    if (m->kind[m->menuSel] == 5) {
        return 1;
    }
    return Weapon_IsKind(item, m->genre);
}

s32 MyHouse_InOwnHouse(void);
s8 MyHouse_LocalSlot(void);
u8 Equip_GetWearMask(SaveData *save, EquipItem *item);
static inline s8 Village_PlayerSlot(void)
{
    return MyHouse_LocalSlot();
}
s32 Equip_FitsPlayer(EquipItem *e)
{
    Player *p;

    p = (MyHouse_InOwnHouse() == 1) ? &gPlayers[gScene.localPlayer] : &gPlayers[Village_PlayerSlot()];
    if (e->id == 0) {
        return 0;
    }
    if (Equip_GetWearMask(&gSave, e) & (1 << p->gender)) {
        return 1;
    }
    return 0;
}

s32 Equip_TestFlag(EquipItem *item, s16 bit);
void Records_TallyWeapons(s32 *out)
{
    EquipItem e;
    s16 total;
    s16 have;
    s16 i;
    s16 kind;
    u16 *id = &e.id;
    s32 *p;
    u8 *type = &e.type;

    kind = 0;
    *type = 6;
    p = out;
    do {
        total = 0;
        have = 0;
        for (i = 0; i < 0x1E6; i++) {
            *id = i;
            if (Weapon_IsKind(&e, kind) == 1 && Equip_FitsPlayer(&e) != 0) {
                if (Equip_TestFlag(&e, 0) == 1) {
                    have++;
                }
                total++;
            }
        }
        *p++ = have;
        *p++ = total;
        kind++;
    } while (kind < 4);
    total = 0;
    *type = 7;
    have = 0;
    for (i = 0; i < 0x80; i++) {
        *id = i;
        if (Weapon_IsKind(&e, 4) == 1 && Equip_FitsPlayer(&e) != 0) {
            if (Equip_TestFlag(&e, 0) == 1) {
                have++;
            }
            total++;
        }
    }
    out[8] = have;
    out[9] = total;
}

void Records_TallyArmor(s32 *arg)
{
    EquipItem e;
    s16 n;
    s16 total;
    u8 *tp = &e.type;
    s16 have;
    s16 i;
    s16 k;
    s32 *out;
    s16 type;

    for (out = arg, k = 0; k < 5; k++, out += 2) {
        switch (k) {
        case 0:
            type = 2;
            n = 0xFD;
            break;
        case 1:
            type = 3;
            n = 0xF3;
            break;
        case 2:
            type = 4;
            n = 0xEE;
            break;
        case 3:
            n = 0xED;
            type = 5;
            break;
        case 4:
            n = 0xEC;
            type = 0;
            break;
        }
        *tp = type;
        total = 0;
        have = 0;
        for (i = 0; i < n; i++) {
            e.id = i;
            if (Equip_FitsPlayer(&e) != 0) {
                if (Equip_TestFlag(&e, 0) == 1) {
                    have++;
                }
                total++;
            }
        }
        out[0] = have;
        out[1] = total;
    }
}

s32 Bits_Test(u8 *bits, u16 n);
s32 Menu_CountPages(s16 n, s16 per);
void Records_LoadEquipPage(RecordMenu *m, s16 page)
{
    s16 cnt;
    s16 lo;
    EquipItem e;
    s16 n;
    s16 hi;
    s16 i;

    switch (m->equipType) {
    case 6:
        n = 0x1E6;
        break;
    case 7:
        n = 0x80;
        break;
    case 2:
        n = 0xFD;
        break;
    case 3:
        n = 0xF3;
        break;
    case 4:
        n = 0xEE;
        break;
    case 5:
        n = 0xED;
        break;
    case 0:
        n = 0xEC;
        break;
    }
    memset(&e, 0, sizeof(e));
    e.type = m->equipType;
    m->rowCount = 0;
    cnt = 0;
    lo = page * 8;
    hi = lo + 7;
    for (i = 0; i < n; i++) {
        e.id = i;
        if (Equip_FitsPlayer(&e) && Bits_Test(m->equipBits, e.id) == 1 && Records_MatchesKind(m, &e) == 1) {
            if (cnt >= lo && cnt <= hi) {
                m->items[m->rowCount] = e;
                m->rowCount++;
            }
            cnt++;
        }
    }
    m->page = page;
    m->pages = Menu_CountPages(cnt, 8);
    if (m->sel >= m->rowCount) {
        m->sel = m->rowCount - 1;
    }
}

void Records_RecvMsg3A(void *id, void *data)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    u16 k = (u8)gLobby->memberSlot[gScene.localPlayer] - 0x30;

    if (gTownMode == 0x46 && memcmp(gNetUsers + k * 0x11C, id, 6) == 0 && m->mode == 1 &&
        m->step == 1) {
        memcpy(m->equipTally, data, 0x28);
        m->step = 3;
    }
}

void Records_RecvMsg2C(void *id, void *data)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    u16 k = (u8)gLobby->memberSlot[gScene.localPlayer] - 0x30;

    if (gTownMode == 0x46 && memcmp(gNetUsers + k * 0x11C, id, 6) == 0 && m->mode == 1 &&
        m->step == 1) {
        memcpy(m->itemBits, data, 0x80);
        m->step = 3;
    }
}

void Records_RecvMsg2D(void *id, void *data)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    u16 k = (u8)gLobby->memberSlot[gScene.localPlayer] - 0x30;

    if (gTownMode == 0x46 && memcmp(gNetUsers + k * 0x11C, id, 6) == 0 && m->mode == 1 &&
        m->step == 1) {
        memcpy(m->titleBits, data, 0x20);
        m->step = 3;
    }
}

void Records_RecvMsg2E(void *id, u8 *data, s8 kind)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    u16 k = (u8)gLobby->memberSlot[gScene.localPlayer] - 0x30;
    s32 i;

    if (gTownMode == 0x46 && memcmp(gNetUsers + k * 0x11C, id, 6) == 0 && m->mode == 1 &&
        m->step == 1) {
        switch (kind) {
        case 0:
            for (i = 0; i < 0x4A; i++) {
                m->sizeMax[i] = data[i];
            }
            break;
        case 1:
            for (i = 0; i < 0x4A; i++) {
                m->sizeMin[i] = data[i];
            }
            break;
        case 2:
            memcpy(m->hunts, data, 0x94);
            m->step = 3;
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
void Records_Draw2(MenuTask *);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 MyHouse_IsOwner(void);
void Records_Open(void)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    s32 i;

    gTownMenuOpen = 1;
    gTownMode = 0x46;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Records_Draw, Records_Draw2, 0, 0);
    gMenuInputLock = 6;
    WinSlide_Step(&m->win, 0x280, 0x118);
    LobbySe_Play(0x16);
    m->menuCount = 0;
    for (i = 0; i < 9; i++) {
        switch (i) {
        case 0:
            if (Game_IsOnline() == 1 && MyHouse_IsOwner()) {
                m->kind[m->menuCount] = i;
                m->menuCount++;
            }
            break;
        case 6:
            if (MyHouse_IsOwner()) {
                m->kind[m->menuCount] = i;
                m->menuCount++;
            }
            break;
        case 7:
            if (Save_IsItemObtained(&gSave, 0x2C1) && MyHouse_IsOwner()) {
                m->kind[m->menuCount] = i;
                m->menuCount++;
            }
            break;
        case 8:
            if (Save_IsItemObtained(&gSave, 0x2C2) && MyHouse_IsOwner()) {
                m->kind[m->menuCount] = i;
                m->menuCount++;
            }
            break;
        default:
            m->kind[m->menuCount] = i;
            m->menuCount++;
            break;
        }
    }
    gMenuOffsetX = -0x14;
    gMenuOffsetY = 0x28;
}
