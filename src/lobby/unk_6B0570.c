/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#pragma optimization_level 4
void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);


void LobbySe_Play(s32);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
extern s32 gTownMode;
extern s32 gTownMenuOpen;
s32 Records_CountMonsterPages(u8 *save);

void Records_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gMenuOffsetY = 0;
    gMenuOffsetX = 0;
}

u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
s32 Records_LoadInput(RecordMenu *m, u16 pad);
void Records_TallyItems(u8 *m);
void Records_EquipInit(RecordMenu *m);
void Records_TallyTitles(RecordMenu *m);
void Records_CatsInit(RecordMenu *m);
void Records_HistoryInit(RecordMenu *m);
s32 Records_EquipInput(RecordMenu *m, u16 pad);
s32 Records_HornScoresInput(RecordMenu *m, u16 pad);
s32 Records_HistoryInput(RecordMenu *m, u16 pad);
s32 Records_CatsInput(RecordMenu *m, u16 pad);
s32 Records_TitlesInput(RecordMenu *m, u16 pad);
s32 Records_ItemsInput(RecordMenu *m, u16 pad);
s32 Records_InviteInput(RecordMenu *m, u16 pad);
extern u8 gMenuWork[];
extern u8 gHouseAccess;
extern char gHouseAccessName[];

void Records_Update(void)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->mode) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->mode++;
            break;
        }
        break;
    case 1:
        switch (Records_LoadInput(m, pad)) {
        case 0:
            m->mode++;
            m->sel = 0;
            m->page = 0;
            m->toggle = 0;
            WinSlide_Step(&m->win, 0x118, 0x280);
            m->unk774 = 0;
            switch (m->kind[m->menuSel]) {
            case 3:
                if (gSave.recordCounts[41] == 1) {
                    m->unk774 = 1;
                }
                Records_TallyItems((u8 *)m);
                break;
            case 4:
                if (gSave.recordCounts[42] == 1) {
                    m->unk774 = 1;
                }
                Records_EquipInit(m);
                break;
            case 5:
                if (gSave.recordCounts[43] == 1) {
                    m->unk774 = 1;
                }
                Records_EquipInit(m);
                break;
            case 6:
                if (gSave.recordCounts[40] == 1) {
                    m->unk774 = 1;
                }
                Records_TallyTitles(m);
                break;
            case 0:
                m->prevSel = gHouseAccess;
                memset(m->name, 0, 9);
                memcpy(m->name, gHouseAccessName, 9);
                break;
            case 1:
                Records_CatsInit(m);
                break;
            case 2:
                Records_HistoryInit(m);
                break;
            case 7:
                m->pages = 5;
                break;
            case 8:
                m->pages = 4;
                break;
            }
            break;
        case 3:
            Records_Close();
            break;
        }
        break;
    case 2:
        switch (WinSlide_Step(&m->win, 0x118, 0x280)) {
        case 0:
            switch (m->kind[m->menuSel]) {
            case 0:
                m->mode = 7;
                break;
            case 3:
                m->mode = 4;
                break;
            case 4:
            case 5:
                m->mode = 10;
                break;
            case 6:
                m->mode = 5;
                break;
            case 1:
                m->mode = 6;
                break;
            case 2:
                m->mode = 8;
                break;
            case 7:
            case 8:
                m->mode = 9;
                break;
            }
            break;
        }
        break;
    case 3:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->mode = 1;
            break;
        }
        break;
    case 10:
        Chat_SetWindowMode(2, 2);
        switch (Records_EquipInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 3;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 9:
        switch (Records_HornScoresInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 3;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 8:
        switch (Records_HistoryInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 3;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 6:
        switch (Records_CatsInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 3;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 5:
        switch (Records_TitlesInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 3;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 4:
        switch (Records_ItemsInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 3;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 7:
        switch (Records_InviteInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 3;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    }
}

s32 Game_IsOnline(void);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
void EquipPreview_Reset(void);
void Bits_Pack(u8 *dst, u8 *src, s32 n);
void LobbyText_Open(s32);
void TownMsg_Send2B(u8 c);
void Records_LoadEquipPage(RecordMenu *m, s16 page);
s32 Records_EquipListInput(RecordMenu *m, u16 pad);
extern LobbyMenu *gLobbyMenu;

s32 Records_EquipInput(RecordMenu *m, u16 pad)
{
    s32 k;

    switch (m->step) {
    case 0:
        if (pad & 0x20) {
            m->sel = 0;
            m->toggle = 0;
            EquipPreview_Reset();
            if (Game_IsOnline() == 0 || (u8)gLobby->memberSlot[gScene.localPlayer] == gScene.localPlayer + 0x30) {
                m->step = 2;
                if (m->kind[m->menuSel] == 4) {
                    if (m->genre != 4) {
                        m->equipType = 6;
                        Bits_Pack(m->equipBits, gSave.bladeFlags, 0x280);
                    } else {
                        m->equipType = 7;
                        Bits_Pack(m->equipBits, gSave.gunFlags, 0x280);
                    }
                } else {
                    switch (m->genre) {
                    case 0:
                        m->equipType = 2;
                        Bits_Pack(m->equipBits, gSave.armorFlags[0], 0x280);
                        break;
                    case 1:
                        m->equipType = 3;
                        Bits_Pack(m->equipBits, gSave.armorFlags[1], 0x280);
                        break;
                    case 2:
                        m->equipType = 4;
                        Bits_Pack(m->equipBits, gSave.armorFlags[2], 0x280);
                        break;
                    case 3:
                        m->equipType = 5;
                        Bits_Pack(m->equipBits, gSave.armorFlags[3], 0x280);
                        break;
                    case 4:
                        m->equipType = 0;
                        Bits_Pack(m->equipBits, gSave.armorFlags[4], 0x280);
                        break;
                    }
                }
                Records_LoadEquipPage(m, 0);
            } else {
                m->step++;
                LobbyText_Open(0x8E);
                gLobbyMenu->redraw = 1;
                memset(m->equipBits, 0, 0x280);
                if (m->kind[m->menuSel] == 4) {
                    if (m->genre != 4) {
                        k = 0x10;
                        m->equipType = 6;
                    } else {
                        k = 0x11;
                        m->equipType = 7;
                    }
                } else {
                    switch (m->genre) {
                    case 0:
                    default:
                        m->equipType = 2;
                        k = 0xC;
                        break;
                    case 1:
                        m->equipType = 3;
                        k = 0xD;
                        break;
                    case 2:
                        m->equipType = 4;
                        k = 0xE;
                        break;
                    case 3:
                        m->equipType = 5;
                        k = 0xF;
                        break;
                    case 4:
                        m->equipType = 0;
                        k = 0xA;
                        break;
                    }
                }
                TownMsg_Send2B(k);
                m->reqKind = k;
                m->timer = 0x1E0;
            }
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            LobbySe_Play(0x17);
            return 3;
        } else {
            m->genre = Menu_WrapUpDown(m->genre, m->genreCount);
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (--m->timer <= 0) {
            m->step = 3;
            LobbyText_Open(0x79);
        }
        break;
    case 2:
        switch (Records_EquipListInput(m, pad)) {
        case 3:
            m->step = 0;
            break;
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->step = 0;
            LobbySe_Play(8);
        }
        break;
    }
    return 2;
}

void StageModels_Free(void);
void EquipPreview_Show(u8 on);
s32 Game_IsOnline(void);
s8 MyHouse_LocalSlot(void);
s32 Equip_IsNoModelArmor(EquipItem *e);
s32 Equip_GetModelId(u8 type, u16 id, u8 x); /* unit's own declaration: with the definition's u16 (u16 kind, u16 id, u16 female) (sub_main/unk_33B080.c), Records_EquipListInput 2 of 196 words differ */
void StageModels_LoadEquip(u8 type, s32 n, s32 x); /* unit's own declaration: with the definition's void (u16 kind, u16 id, u8 female) (sub_main/unk_33B080.c), Records_EquipListInput 2 of 196 words differ */
u8 Equip_LevelMark(EquipItem *a, u8 flags, EquipItem *b);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
void Records_LoadEquipPage(RecordMenu *m, s16 page);
extern NetUser gNetUsers[];

static inline u8 Records_NetUserByte(void)
{
    return ((HunterCard *)((NetUser *)&gNetUsers[((Player *)&gPlayers[MyHouse_LocalSlot()])->index])->text)->gender;
}

s32 Records_EquipListInput(RecordMenu *m, u16 pad)
{
    EquipItem e;
    s32 x;

    if (pad & 0x40) {
        if (m->toggle == 0) {
            LobbySe_Play(4);
            StageModels_Free();
            return 3;
        }
        StageModels_Free();
        EquipPreview_Show(1);
        m->toggle = 0;
        LobbySe_Play(4);
    } else if (pad & 0x200) {
        if (m->rowCount != 0) {
            if (m->toggle == 0) {
                if (Game_IsOnline() == 0) {
                    x = gSave.gender;
                } else {
                    x = Records_NetUserByte();
                }
                e = m->items[m->sel];
                if (Equip_IsNoModelArmor(&e) == 0) {
                    StageModels_LoadEquip(e.type, Equip_GetModelId(e.type, e.id, x), x);
                    EquipPreview_Show(0);
                }
                m->subCursor = 0;
                m->levelCount = Equip_LevelMark(&e, 0x40, 0);
                m->toggle = 1;
            } else {
                StageModels_Free();
                EquipPreview_Show(1);
                m->toggle = 0;
            }
            LobbySe_Play(1);
        }
    } else if (pad & 0x80) {
        if (m->toggle != 0) {
            if (m->levelCount > 1) {
                if (++m->subCursor >= m->levelCount) {
                    m->subCursor = 0;
                }
                LobbySe_Play(1);
            }
        }
    } else if (pad & 8) {
        if (m->toggle != 0) {
            if (m->levelCount > 1) {
                if (--m->subCursor < 0) {
                    m->subCursor = m->levelCount - 1;
                }
                LobbySe_Play(1);
            }
        }
    } else if (pad & 0xC00) {
        if (m->toggle == 0) {
            if (m->pages > 1) {
                m->page = Menu_WrapLeftRight(m->page, m->pages);
                Records_LoadEquipPage(m, m->page);
            }
        }
    } else if (m->toggle == 0) {
        m->sel = Menu_WrapUpDown(m->sel, m->rowCount);
    }
    return 2;
}

s32 Records_HornScoresInput(RecordMenu *m, u16 pad)
{
    if (pad & 0x40) {
        LobbySe_Play(0x17);
        return 3;
    }
    m->page = Menu_WrapLeftRight(m->page, m->pages);
    return 2;
}

void Records_HistoryInit(RecordMenu *m) {
    m->page = 0;
    m->pages = Records_CountMonsterPages((u8 *)m);
}

s32 Records_HistoryInput(RecordMenu *m, u16 pad)
{
    if (pad & 0x40) {
        LobbySe_Play(0x17);
        return 3;
    }
    if (pad & 0xC00) {
        m->page = Menu_WrapLeftRight(m->page, m->pages);
    }
    return 2;
}

s32 Menu_CountPages(s16 n, s16 per);
void Records_SetCatPage(RecordMenu *m, s16 page)
{
    RecordCat *c = &gRecordCats[m->sel];
    s16 first = page * 10;
    s16 last = first + 9;
    u8 *p = c->list;
    s16 i;

    m->rowCount = 0;
    for (i = 0; i < c->count; i++, p += 12) {
        if (i >= first && i <= last) {
            m->rows[m->rowCount] = p;
            m->rowCount++;
        }
    }
    m->page = page;
    m->pages = Menu_CountPages(c->count, 10);
    if (m->cursor >= m->rowCount) {
        m->cursor = m->rowCount - 1;
    }
}

void Records_CatsInit(RecordMenu *m) {

}

s32 Records_CatsInput(RecordMenu *m, u16 pad)
{
    switch (m->step) {
    case 0:
        if (pad & 0x20) {
            m->step++;
            m->cursor = 0;
            Records_SetCatPage(m, 0);
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->step = 0;
            LobbySe_Play(0x17);
            return 3;
        } else {
            m->sel = Menu_WrapUpDown(m->sel, 4);
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->step++;
            m->subCursor = 0;
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->step--;
            LobbySe_Play(4);
        } else if (pad & 0xC00) {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
            Records_SetCatPage(m, m->page);
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, m->rowCount);
        }
        break;
    case 2:
        if (pad & 0x40) {
            m->step--;
            LobbySe_Play(4);
        } else if (pad & 0xC00) {
            m->subCursor = Menu_WrapLeftRight(m->subCursor, *(s32 *)((u8 *)m->rows[m->cursor] + 8));
        }
        break;
    }
    return 2;
}

void Records_SetTitlePage(RecordMenu *m, s16 page);
s32 Game_IsOnline(void);
s32 LobbyMenu_YesNoInput(s32 msg);
void Menu_SetYesNo(s8 a);
s32 Bits32_Test(u32 *bits, s32 n);
s32 Records_TitlesInput(RecordMenu *m, u16 pad)
{
    switch (m->step) {
    case 0:
        if (pad & 0x20) {
            m->step++;
            m->cursor = 0;
            Records_SetTitlePage(m, 0);
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->step = 0;
            LobbySe_Play(0x17);
            return 3;
        } else {
            m->sel = Menu_WrapUpDown(m->sel, 5);
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (Game_IsOnline() != 0 && (u8)gLobby->memberSlot[gScene.localPlayer] != gScene.localPlayer + 0x30) {
                break;
            }
            if (m->rowCount != 0) {
                if (Bits32_Test(m->titleBits, m->ids[m->cursor]) != 0) {
                    m->step++;
                    Menu_SetYesNo(0);
                    LobbySe_Play(8);
                } else {
                    LobbySe_Play(9);
                }
            }
        } else if (pad & 0x40) {
            m->step = 0;
            LobbySe_Play(4);
        } else if (pad & 0x200) {
            if (m->rowCount != 0) {
                m->toggle ^= 1;
                LobbySe_Play(0x39);
            }
        } else if (pad & 0xC00) {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
            Records_SetTitlePage(m, m->page);
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, m->rowCount);
        }
        break;
    case 2:
        switch (LobbyMenu_YesNoInput(0x3A)) {
        case 0:
            m->step--;
            gSave.title = m->ids[m->cursor];
            break;
        case 3:
            m->step--;
            break;
        }
        break;
    }
    return 2;
}

void Records_SetTitlePage(RecordMenu *m, s16 page)
{
    s16 i;
    s16 k;
    HunterTitle *p;
    s16 first = page * 8;
    s16 last = first + 7;

    m->rowCount = 0;
    for (i = 0, k = 0, p = gHunterTitles; i < 0x94; i++, p++) {
        if (p->group == m->sel && Bits32_Test(m->titleBits, i) == 1) {
            if (k >= first && k <= last) {
                m->ids[m->rowCount] = i;
                m->rowCount++;
            }
            k++;
        }
    }
    m->page = page;
    m->pages = Menu_CountPages(k, 8);
    if (m->cursor >= m->rowCount) {
        m->cursor = m->rowCount - 1;
    }
}

s32 ItemBits_Test(u32 *bits, u16 n);
/* 11 of 86 words differ: the saved registers of id, first and last are rotated */
void Records_SetItemPage(RecordMenu *m, s16 page)
{
    s16 i;
    s16 k;
    ItemData *p;
    s16 first = page * 8;
    s16 last = first + 7;
    s16 cat;
    u16 id;

    m->rowCount = 0;
    cat = m->sel + 1;
    for (i = 0, k = 0, p = gItemData; i < 0x399; i++, p++) {
        id = i;
        if (cat == p->category && ItemBits_Test(m->itemBits, i) == 1) {
            if (k >= first && k <= last) {
                m->ids[m->rowCount] = id;
                m->rowCount++;
            }
            k++;
        }
    }
    m->page = page;
    m->pages = Menu_CountPages(k, 8);
    if (m->cursor >= m->rowCount) {
        m->cursor = m->rowCount - 1;
    }
}

s32 Item_IsDeco(u16 item);
s32 Records_ItemsInput(RecordMenu *m, u16 pad)
{
    s16 n;

    switch (m->step) {
    case 0:
        if (pad & 0x20) {
            m->step++;
            m->cursor = 0;
            Records_SetItemPage(m, 0);
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->step = 0;
            LobbySe_Play(0x17);
            return 3;
        } else {
            m->sel = Menu_WrapUpDown(m->sel, 8);
        }
        break;
    case 1:
        if (!(pad & 0x20)) {
            if (pad & 0x40) {
                m->step = 0;
                LobbySe_Play(4);
            } else if (pad & 0xC00) {
                n = m->pages;
                if (n > 1) {
                    m->page = Menu_WrapLeftRight(m->page, n);
                    Records_SetItemPage(m, m->page);
                }
            } else if (pad & 0x80) {
                if (m->rowCount != 0 && Item_IsDeco(m->ids[m->cursor]) == 1) {
                    if (++m->subCursor >= 3) {
                        m->subCursor = 0;
                    }
                    LobbySe_Play(1);
                }
            } else {
                m->cursor = Menu_WrapUpDown(m->cursor, m->rowCount);
            }
        }
        break;
    }
    return 2;
}

void Records_TallyTitles(RecordMenu *m)
{
    s32 i;
    HunterTitle *p;
    u8 *t;

    m->sel = 0;
    m->subCursor = 0;
    memset(m->tally, 0, 0x48);
    for (i = 0, p = gHunterTitles; i < 0x94; i++, p++) {
        /* the game keeps m + group * 8 as the base of both counts */
        t = (u8 *)m + p->group * 8;
        (*(s32 *)(t + 0x6E0))++;
        if (Bits32_Test(m->titleBits, i) == 1) {
            (*(s32 *)(t + 0x6DC))++;
        }
    }
}

void Records_EquipInit(RecordMenu *m) {
    m->sel = 0;
    m->subCursor = 0;
    m->page = 0;
    m->genre = 0;
    m->genreCount = 5;
}
