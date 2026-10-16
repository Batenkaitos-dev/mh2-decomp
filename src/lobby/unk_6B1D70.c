#include "common.h"
#include "lobby_text.h"
#include "item.h"
#include "lobby_vobj.h"
#include "lobby_state.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "pad.h"
char *strcpy(char *, const char *);
void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);
s32 ItemBits_Test(u32 *bits, u16 n);

extern char *D_0080C8F4;
extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
void Equip_DrawPreviewHelp(u32 color, s8 k);
s32 Equip_IsNoModelArmor(EquipItem *e);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *s, ...);
void EquipWindow_DrawLocal(EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other);
extern char *D_0080C324;
void Text_Printf(char *s, ...);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Menu_DrawPageNum(s16 x, s16 y, s32 page, s32 pages, s32 a); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 page, s16 n, u8 alt) (lobby/unk_790EC0.c), Records_DrawMonsters 164 of 301 words differ */
void Text_PrintfStyled(s32, char *, ...);
extern char *D_007DB880[];
extern char *D_007DB8A0[];
extern char *D_0080C33C;
extern char *gAncientScoreTexts[];
extern u8 gMenuWork[];
void Text_SetSize(s32 size);
void Records_DrawMenu(RecordMenu *m, s16 x, s16 y);
u16 MonsterGroup_Sum(u16 *counts, u8 group, s32 unused);

void Records_TallyItems(u8 *m)
{
    s32 i;
    ItemData *p;
    u8 *t;

    *(s16 *)(m + 0x766) = 0;
    *(s16 *)(m + 0x75C) = 0;
    memset(m + 0x6DC, 0, 0x48);
    for (i = 0, p = gItemData; i < 0x399; i++, p++) {
        t = m + p->category * 8; /* +0x14: the item category */
        (*(s32 *)(t + 0x6E0))++;
        if (ItemBits_Test((u32 *)(m + 0x10), i) == 1) {
            (*(s32 *)(t + 0x6DC))++;
        }
    }
}

s32 Game_IsOnline(void);
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void LobbyText_Open(s32);
void TownMsg_Send2B(u8 c);
void Records_TallyWeapons(s32 *out);
void Records_TallyArmor(s32 *arg);
extern LobbyMenu *gLobbyMenu;

/* The records screen's load step: the player's own records (copied from the
 * save), or another member's, requested with TownMsg_Send2B and waited for
 * 0x1E0 frames. */
s32 Records_LoadInput(RecordMenu *m, u16 pad)
{
    s32 i;

    switch (m->step) {
    case 0:
        if (pad & 0x20) {
            if (Game_IsOnline() == 0 || (u8)gLobby->memberSlot[gScene.localPlayer] == gScene.localPlayer + 0x30) {
                switch (m->kind[m->menuSel]) {
                case 3:
                    memcpy(m->itemBits, gSave.itemBits, 0x80);
                    break;
                case 6:
                    memcpy(m->titleBits, gSave.unk3660, 0x20);
                    break;
                case 2:
                    memcpy(m->sizeMax, gSave.sizeMax, 0x100);
                    memcpy(m->sizeMin, gSave.sizeMin, 0x100);
                    for (i = 0; i < 0x80; i++) {
                        m->hunts[i] = gSave.hunted[i] + gSave.captured[i];
                    }
                    break;
                case 4:
                    Records_TallyWeapons(m->equipTally);
                    break;
                case 5:
                    Records_TallyArmor(m->equipTally);
                    break;
                case 0:
                case 1:
                    break;
                }
                LobbySe_Play(0x17);
                return 0;
            }
            m->step++;
            LobbyText_Open(0x8E);
            gLobbyMenu->redraw = 1;
            switch (m->kind[m->menuSel]) {
            case 3:
                memset(m->itemBits, 0, 0x80);
                break;
            case 6:
                memset(m->titleBits, 0, 0x20);
                break;
            case 2:
                memset(m->sizeMax, 0, 0x100);
                memset(m->sizeMin, 0, 0x100);
                memset(m->hunts, 0, 0x100);
                break;
            case 1:
                m->step = 0;
                gLobbyMenu->redraw = 0;
                LobbySe_Play(0x17);
                return 0;
            case 4:
            case 5:
                memset(m->equipTally, 0, 0x28);
                break;
            default:
                return 2;
            }
            TownMsg_Send2B(m->kind[m->menuSel]);
            m->reqKind = m->kind[m->menuSel];
            m->timer = 0x1E0;
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else {
            m->menuSel = Menu_WrapUpDown(m->menuSel, m->menuCount);
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (--m->timer <= 0) {
            m->step++;
            LobbyText_Open(0x79);
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->step = 0;
            LobbySe_Play(8);
        }
        break;
    case 3:
        m->step = 0;
        LobbySe_Play(0x17);
        return 0;
    }
    return 2;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6B1D70", D_00825C30);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6B1D70", D_00825C38);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6B1D70", D_00825C40);

struct Em *Npc_Find(u8 kind);
extern u8 gHouseAccess;
extern char gHouseAccessName[];
extern NetUser gNetUsers[];
void HunterCard_Build(void *out);
void Actor_SetState(void *, s32, s32);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
s32 LobbyStep_6190(void);
void LobbyDialog_Grow(void);

typedef struct NpcTalk {
    u8 unk0[0xC];
    u16 fC;
    u8 unkE[6];
} NpcTalk;

s32 Records_InviteInput(RecordMenu *m, u16 pad)
{
    u8 *npc = (u8 *)Npc_Find(0x14);

    switch (m->step) {
    case 0:
        if (pad & 0x20) {
            switch (m->sel) {
            case 3:
                m->step = 4;
                LobbyText_Open(0x78);
                gLobbyMenu->redraw = 1;
                gHouseAccess = m->prevSel;
                memset(gHouseAccessName, 0, 9);
                strcpy(gHouseAccessName, m->name);
                HunterCard_Build(gNetUsers[gScene.localPlayer].text);
                HunterCard_Build(gLobby->members[gScene.localPlayer].text);
                HunterCard_Build(&gPlayerText);
                if (gHouseAccess != 0) {
                    if (npc != NULL && npc[0x47C] == 0) {
                        Actor_SetState(npc, 0, 0xB6);
                    }
                } else if (npc != NULL) {
                    Actor_SetState(npc, 0, *(u16 *)(*(u8 **)(npc + 0x444) + *(s16 *)(npc + 0x474) * 20 + 0xC));
                }
                break;
            case 2:
                m->step++;
                gFieldControl = 0;
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 6, 8, m->name);
                break;
            default:
                m->prevSel = m->sel;
                m->sel = 3;
                memset(m->name, 0, 9);
                break;
            }
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
        if (Kbd_Run(m->name, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            Kbd_Cancel();
            gFieldControl = 1;
            if (m->name[0] != 0) {
                m->step = 0;
                m->prevSel = m->sel;
                m->sel = 3;
            } else {
                m->step++;
                LobbyText_Open(0x90);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
            }
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->step++;
            LobbySe_Play(8);
        }
        break;
    case 3:
        m->step = 1;
        Chat_OpenKbd(0, 6, 8, m->name);
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_6190()) {
        case 0:
            m->step = 0;
            LobbySe_Play(0x17);
            return 0;
        }
        break;
    }
    return 2;
}

extern char *gProfileMenuTexts[];
void Menu_DrawDecoFrame(s16 x, s16 y0, s16 w0, s16 h, s8 kind, s8 b);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
void Records_DrawMenu(RecordMenu *m, s16 x, s16 y)
{
    char *s;
    s32 i;
    s16 tx;
    s16 step;

    Menu_DrawDecoFrame(x, y, 0x154, 0x114, 5, 0);
    switch (m->menuCount) {
    case 9:
        step = 0x1C;
        y = y + 0x1E;
        break;
    case 8:
        step = 0x1E;
        y = y + 0x22;
        break;
    case 7:
        step = 0x1E;
        y = y + 0x36;
        break;
    case 6:
        step = 0x1E;
        y = y + 0x3C;
        break;
    case 5:
        step = 0x24;
        y = y + 0x46;
        break;
    default:
        step = 0x28;
        y = y + 0x46;
        break;
    }
    Font_SetColor(0x807F7F7F);
    x = x + 0xAA;
    Font_SetSpacing(0x16, 0x16);
    for (i = 0; i < m->menuCount; i++) {
        s = gProfileMenuTexts[m->kind[i]];
        tx = Text_CenterX(s, x, 0x18) + 10;
        if (i == m->menuSel) {
            Menu_DrawArrowsRev(tx, y, 0x16, s, 0xFFB57510);
            Text_DrawShadowRgb(tx, y, 0xFF101010, 0xFF106060, s);
        } else {
            Font_SetColor(0xFF101010);
            Font_SetPos(tx, y);
            Text_PrintfColored(s);
        }
        y += step;
    }
}

extern char D_00825C30[];
extern char *D_0080C32C;
extern char *D_0080C354;
extern char *D_0080C3B4[];
extern char *gEquipGroupTexts[];
s32 sprintf(char *buf, char *fmt, ...);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Records_DrawItemGenres(RecordMenu *m, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 ty = y;
    s16 tx;
    char **p;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, D_0080C32C, 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x12, 0x12);
    if (m->unk774 == 0) {
        ty += 0x14;
    }
    for (i = 1, p = D_0080C3B4; i < 9; i++, p++) {
        sprintf(buf, D_00825C30, *p);
        tx = Text_CenterX(buf, 0x1C2, 0x12);
        ty += 0x16;
        Font_SetPos(tx, ty);
        Text_PrintfColored(D_00825C30, buf);
        if (m->sel == i - 1) {
            Menu_DrawArrowsRev(tx, ty, 0x12, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C354);
}

extern char *gWeaponCollectTexts[];
extern char *D_0080C364;
void Records_DrawWeaponGenres(RecordMenu *m, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 ty = y;
    s16 tx;
    char **p;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, gWeaponCollectTexts[0], 2, 1);
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    ty += 0x1E;
    for (i = 0, p = gEquipGroupTexts; i < 5; i++, p++) {
        sprintf(buf, D_00825C30, *p);
        tx = Text_CenterX(buf, 0x1C2, 0x12);
        ty += 0x22;
        Font_SetPos(tx, ty);
        Text_PrintfColored(D_00825C30, buf);
        if (m->genre == i) {
            Menu_DrawArrowsRev(tx, ty, 0x14, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C364);
}

extern char *D_0080C334;
extern char *gArmorGenrePromptTexts[];
void Records_DrawArmorGenres(RecordMenu *m, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 ty = y;
    s16 tx;
    char **p;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, D_0080C334, 2, 1);
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    ty += 0x1E;
    for (i = 0, p = gEquipGroupTexts; i < 5; i++, p++) {
        sprintf(buf, D_00825C30, p[5]);
        tx = Text_CenterX(buf, 0x1C2, 0x12);
        ty += 0x22;
        Font_SetPos(tx, ty);
        Text_PrintfColored(D_00825C30, buf);
        if (m->genre == i) {
            Menu_DrawArrowsRev(tx, ty, 0x14, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gArmorGenrePromptTexts[0]);
}

extern char *gTitleLabelTexts[];
extern char *gTitleGenrePromptTexts[];
extern char *gHunterNotesTexts[];
void Records_DrawTitleGenres(RecordMenu *m, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 ty = y;
    s16 tx;
    char **p;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x10E, 0x12, 0x64, 0xFFB0B000, gTitleLabelTexts[0], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    ty += 0x14;
    if (m->unk774 == 0) {
        ty += 0x14;
    }
    for (i = 0, p = gHunterNotesTexts; i < 5; i++, p++) {
        sprintf(buf, D_00825C30, *p);
        tx = Text_CenterX(buf, 0x1C2, 0x12);
        ty += 0x18;
        Font_SetPos(tx, ty);
        Text_PrintfColored(D_00825C30, buf);
        if (m->sel == i) {
            Menu_DrawArrowsRev(tx, ty, 0x14, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gTitleGenrePromptTexts[0]);
}

extern char *gHunterTitleDescs[];
extern char *gYesNoTexts[];
s8 MyHouse_LocalSlot(void);
s32 Bits32_Test(u32 *bits, s32 n);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2);
void Win_Draw(s32 x, s32 y, s32 w, s32 h, s32 color, s32 color2, s16 kind);
extern char *D_0080C3AC;
extern char *D_0080C3A8;
extern char *gTitleConfirmTexts[];
extern char *D_0080C35C;
extern char D_00825C38[];

void Records_DrawTitleList(RecordMenu *m, s16 x, s16 y)
{
    HunterTitle *t;
    s32 i;
    s16 ty = y;
    s32 cx;
    u16 id;
    u16 cur;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x10E, 0x12, 0x64, 0xFFB0B000, gTitleLabelTexts[0], 2, 1);
    if (Game_IsOnline() == 0 || (u8)gLobby->memberSlot[gScene.localPlayer] == gScene.localPlayer + 0x30) {
        t = &gHunterTitles[gSave.title];
    } else {
        t = &gHunterTitles[((u8 *)&gNetUsers[MyHouse_LocalSlot()])[0x2A]];
    }
    Font_SetSpacing(0x14, 0x14);
    Menu_DrawLabelBar(0x178, 0x12, 0xE6, 0xFFB0B000, t->text, 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(D_00825C30, gHunterNotesTexts[m->sel]);
    if (m->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(Text_CenterX(D_0080C3AC, x + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(D_00825C30, D_0080C3AC);
        return;
    }
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xFA, y + 0xE, m->page, m->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    ty += 0x38;
    i = 0;
    if (i < m->rowCount) {
        do {
        Font_SetPos((s16)(x + 0x78) - 0x2E, ty);
        Text_PrintfColored(D_00825C38, i + m->page * 8 + 1);
        id = m->ids[i];
        t = &gHunterTitles[id];
        Font_SetPos(x + 0x78, ty);
        if (Bits32_Test(m->titleBits, id) == 1) {
            Text_PrintfColored(D_00825C30, t->text);
        } else {
            Text_PrintfColored(D_00825C30, D_0080C3A8);
        }
        if (m->cursor == i) {
            cur = id;
            Menu_DrawCursorRow(x + 0x32, ty, 0xF0, 0, 0xC0106060, 0xFF106060);
        }
        ty += 0x18;
        i++;
        } while (i < m->rowCount);
    }
    Win_Draw(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, 0);
    if (m->step == 2) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gTitleConfirmTexts[0]);
        Font_SetPos(0x1A2, 0x192);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    } else if (m->toggle == 0) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C35C);
    } else if (Bits32_Test(m->titleBits, (u16)m->ids[m->cursor])) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gHunterTitleDescs[cur]);
    } else {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C3A8);
    }
}

extern char *D_0080C3DC;
extern char *gUnknownItemTexts[];
extern char *gItemNames[];
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
void Deco_DrawDetails(u16 item, s16 x, s16 y, s8 c, u8 pal);

void Records_DrawItemList(RecordMenu *m, s16 x, s16 y)
{
    s32 i;
    s16 ty = y;
    u16 id;
    u16 cur;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, D_0080C32C, 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(D_00825C30, D_0080C3B4[m->sel]);
    if (m->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(Text_CenterX(D_0080C3DC, x + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(D_00825C30, D_0080C3DC);
        return;
    }
    Menu_DrawPageNum(x + 0xFA, y + 0xA, m->page, m->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    ty += 0x38;
    i = 0;
    if (i < m->rowCount) {
        do {
            id = m->ids[i];
            Font_SetPos((s16)(x + 0x8C) - 0x42, ty);
            Text_PrintfColored(D_00825C38, i + m->page * 8 + 1);
            if (ItemBits_Test((u32 *)m->itemBits, id) == 1) {
                Font_SetPos(x + 0x8C, ty);
                Text_PrintfColored(D_00825C30, gItemNames[id]);
                Item_DrawIcon((s16)(x + 0x8C) - 0x1C, ty, 0x14, id);
            } else {
                Font_SetPos((s16)(x + 0x8C) - 0x14, ty);
                Text_PrintfColored(D_00825C30, gUnknownItemTexts[0]);
            }
            if (m->cursor == i) {
                cur = id;
                Menu_DrawCursorRow(x + 0x32, ty, 0xF0, 0, 0xC0106060, 0xFF106060);
            }
            ty += 0x18;
            i++;
        } while (i < m->rowCount);
    }
    Win_Draw(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, 0);
    if (ItemBits_Test((u32 *)m->itemBits, cur) == 1) {
        Font_SetSpacing(0x12, 0x12);
        Item_DrawIcon(0x126, 0x166, 0x36, cur);
        Item_DrawRarity(0x12E, 0x19C, gItemData[cur].rarity);
        Deco_DrawDetails(cur, 0x16C, 0x166, m->subCursor | 0x10, 0);
    }
}

void Records_DrawItem(RecordMenu *m)
{
    char *s;

    Equip_DrawPreviewHelp(0xBDB59F6C, 0);
    if (Equip_IsNoModelArmor(&m->items[m->sel]) == 1) {
        s = D_0080C8F4;
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x17A, 0x9A);
        Font_SetColor(0x807F212E);
        Text_PrintfColored(s);
    }
    EquipWindow_DrawLocal(&m->items[m->sel], gMenuOffsetX + 0x136, gMenuOffsetY + 0xD8, m->subCursor | 0x40, NULL);
}

// x y x y x lx ly
extern char D_00825C40[];
extern char *D_0080C36C;
extern char *D_0080C0A4;
char *Equip_GetName(u8 type, u16 id);
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, EquipItem *e) (lobby/unk_78F070.c), Records_DrawWeaponList 6 of 242 words, Records_DrawArmorList 6 of 242 words differ */
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void EquipShop_DrawLine(s16 x, s16 y, EquipItem *e);

void Records_DrawWeaponList(RecordMenu *m, s16 x, s16 y)
{
    s32 i;
    s32 tx;
    s16 ty = y;
    char *s;
    EquipItem e;
    EquipItem sel;
    s32 lx;
    s32 ly;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, gWeaponCollectTexts[0], 2, 1);
    if (m->toggle != 0) {
        Records_DrawItem(m);
        return;
    }
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    lx = x;
    ly = (s32)y;
    tx = x;
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(D_00825C30, gEquipGroupTexts[m->genre]);
    if (m->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        s = D_0080C36C;
        Font_SetPos(Text_CenterX(s, tx + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(D_00825C30, s);
        return;
    }
    Menu_DrawPageNum(x + 0xFA, y + 0xE, m->page, m->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    ty += 0x38;
    i = 0;
    if (i < m->rowCount) {
        do {
            e = m->items[i];
            Font_SetPos((s16)(x + 0x78) - 0x42, ty);
            Text_PrintfColored(D_00825C38, i + m->page * 8 + 1);
            Font_SetPos(x + 0x78, ty);
            Text_PrintfColored(D_00825C40, Equip_GetName(e.type, e.id));
            Font_SetPos(x, ty);
            Lobby_DrawEquipIcon((s16)(x + 0x78) - 0x1C, ty, 0x14, 0x14, &e);
            if (m->sel == i) {
                sel = e;
                Menu_DrawCursorRow(x + 0x1E, ty, 0x10E, 0, 0xC0106060, 0xFF106060);
            }
            ty += 0x18;
            i++;
        } while (i < m->rowCount);
    }
    Menu_DrawDashLine(lx + 0x14, ly + 0x104, 0x12C, 0xFF000000);
    EquipShop_DrawLine(lx + 0x10, ly + 0x112, &sel);
    Font_SetColor(0x80101010);
    Font_SetPos(lx + 0x10E, ly + 0x13E);
    Text_Printf(D_0080C0A4);
}

// x y x y x lx ly
extern char *gNoArmorFoundTexts[];
extern char *D_0080C6F4[];

void Records_DrawArmorList(RecordMenu *m, s16 x, s16 y)
{
    s32 i;
    s32 tx;
    s16 ty = y;
    char *s;
    EquipItem e;
    EquipItem sel;
    s32 lx;
    s32 ly;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, D_0080C334, 2, 1);
    if (m->toggle != 0) {
        Records_DrawItem(m);
        return;
    }
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    lx = x;
    ly = (s32)y;
    tx = x;
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(D_00825C30, D_0080C6F4[m->genre]);
    if (m->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        s = gNoArmorFoundTexts[0];
        Font_SetPos(Text_CenterX(s, tx + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(D_00825C30, s);
        return;
    }
    Menu_DrawPageNum(x + 0xFA, y + 0xE, m->page, m->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    ty += 0x38;
    i = 0;
    if (i < m->rowCount) {
        do {
            e = m->items[i];
            Font_SetPos((s16)(x + 0x78) - 0x42, ty);
            Text_PrintfColored(D_00825C38, i + m->page * 8 + 1);
            Font_SetPos(x + 0x78, ty);
            Text_PrintfColored(D_00825C40, Equip_GetName(e.type, e.id));
            Font_SetPos(x, ty);
            Lobby_DrawEquipIcon((s16)(x + 0x78) - 0x1C, ty, 0x14, 0x14, &e);
            if (m->sel == i) {
                sel = e;
                Menu_DrawCursorRow(x + 0x1E, ty, 0x10E, 0, 0xC0106060, 0xFF106060);
            }
            ty += 0x18;
            i++;
        } while (i < m->rowCount);
    }
    Menu_DrawDashLine(lx + 0x14, ly + 0x104, 0x12C, 0xFF000000);
    EquipShop_DrawLine(lx + 0x10, ly + 0x112, &sel);
    Font_SetColor(0x80101010);
    Font_SetPos(lx + 0x10E, ly + 0x13E);
    Text_Printf(D_0080C0A4);
}

extern char *gHouseAccessTexts[];
extern char *D_0080C38C;

void Records_DrawInvite(RecordMenu *m, s16 x, s16 y)
{
    s16 tx;
    s16 ty = y;
    s16 i;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gProfileMenuTexts[0], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x16, 0x16);
    for (i = 0; i < 3; i++) {
        if (m->prevSel == i) {
            Font_SetColor(0x8000052C);
        } else {
            Font_SetColor(0x80101010);
        }
        tx = Text_CenterX(gHouseAccessTexts[i], x + 0xAA, 0x16);
        Font_SetPos(tx, ty += 0x28);
        Text_PrintfColored(gHouseAccessTexts[i]);
        if (m->sel == i) {
            Menu_DrawArrowsRev(tx, ty, 0x16, gHouseAccessTexts[i], 0xFF008080);
        }
    }
    Font_SetColor(0x80101010);
    ty += 0x28;
    if (m->name[0] != 0) {
        tx = Text_CenterX(m->name, x + 0xAA, 0x16);
        Font_SetPos(tx, ty);
        Text_PrintfColored(D_00825C30, m->name);
        Icon_Draw(tx - 0x1E, ty - 2, 0x18, 0x18, -1, 0x59);
    }
    tx = Text_CenterX(D_0080C38C, x + 0xAA, 0x16);
    Font_SetPos(tx, ty += 0x28);
    Text_PrintfColored(D_00825C30, D_0080C38C);
    if (m->sel == 3) {
        Menu_DrawArrowsRev(tx, ty, 0x16, gHouseAccessTexts[i], 0xFF008080);
    }
}

void Records_DrawCategories(RecordMenu *m, s16 x, s16 y)
{
    s16 ty = y;
    s16 i;
    RecordCat *c;
    s32 tx;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, D_0080C324, 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x14, 0x14);
    Font_SetColor(0x80101010);
    ty += 0x1E;
    tx = x + 0x32;
    for (i = 0, c = gRecordCats; i < 4; i++, c++) {
        Font_SetPos(tx, ty);
        Text_Printf(c->name);
        if (m->sel == i) {
            Menu_DrawArrowsRev(tx, ty + 0xB, 0x16, "                      ", 0xFF106060);
        }
        ty += 0x33;
    }
}

void Records_DrawRows(RecordMenu *m, s16 x, s16 y)
{
    char *s;
    s16 tx;
    s16 ty = y;
    s16 i;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, D_0080C324, 2, 1);
    Font_SetColor(0x80101010);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Menu_DrawPageNum(x + 0x104, ty + 6, m->page, m->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    ty += 0x1E;
    for (i = 0; i < m->rowCount; i++) {
        s = ((RecordEntry *)m->rows[i])->name;
        tx = Text_CenterX(s, x + 0xAA, 0x12);
        Font_SetPos(tx, ty);
        Text_Printf(s);
        if (m->cursor == i) {
            Menu_DrawArrowsRev(tx, ty, 0x12, s, 0xFF106060);
        }
        ty += 0x16;
    }
}

void Records_DrawPage(RecordMenu *m, s16 x, s16 y)
{
    char *s;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, D_0080C324, 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Menu_DrawPageNum(x + 0x104, y + 6, m->subCursor, ((RecordEntry *)m->rows[m->cursor])->pageCount, 1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80000000);
    s = ((RecordEntry *)m->rows[m->cursor])->pages[m->subCursor];
    Font_SetPos(x + 0x1E, y + 0x1E);
    Text_PrintfStyled(2, s);
}

void Records_DrawHornScores(RecordMenu *m, s16 x, s16 y)
{
    char *s;

    Font_SetSpacing(0x16, 0x16);
    if (m->kind[m->menuSel] == 7) {
        s = D_007DB880[m->page];
        Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, D_0080C33C, 2, 1);
    } else {
        s = D_007DB8A0[m->page];
        Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gAncientScoreTexts[0], 2, 1);
    }
    Icon_Draw(x, y, 0x154, 0x114, 0xFF706858, 0);
    Menu_DrawPageNum(x + 0x104, y + 6, m->page, m->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfStyled(2, s);
}

void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind);
void Records_DrawMonsters(RecordMenu *m, s16 x, s16 y);
void Records_DrawTitleList(RecordMenu *m, s16 x, s16 y);
void Records_DrawItemList(RecordMenu *m, s16 x, s16 y);
void Records_DrawWeaponList(RecordMenu *m, s16 x, s16 y);
void Records_DrawArmorList(RecordMenu *m, s16 x, s16 y);
extern char *gCheckItemPromptTexts[];

/* The records screen's draw: by the mode (+0x772), the category kind and the step. */
void Records_Draw(MenuTask *t)
{
    RecordMenu *m = (RecordMenu *)gMenuWork;
    s16 x;

    Text_SetSize(t->textSize);
    switch ((s8)m->mode) {
    case 0:
    case 1:
        x = *(s16 *)m;
        Menu_DrawDecoBox(x, 0x3A, 0x154, 0x114, 5);
        Records_DrawMenu(m, x, 0x3A);
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gCheckItemPromptTexts[0]);
        break;
    case 2:
    case 3:
        switch (m->kind[m->menuSel]) {
        case 3:
            Records_DrawItemGenres(m, 0x118, 0x3A);
            break;
        case 0:
            Records_DrawInvite(m, 0x118, 0x3A);
            break;
        case 6:
            Records_DrawTitleGenres(m, 0x118, 0x3A);
            break;
        case 1:
            Records_DrawCategories(m, 0x118, 0x3A);
            break;
        case 2:
            Records_DrawMonsters(m, 0x118, 0x3A);
            break;
        case 7:
        case 8:
            Records_DrawHornScores(m, 0x118, 0x3A);
            break;
        case 4:
            Records_DrawWeaponGenres(m, 0x118, 0x3A);
            break;
        case 5:
            Records_DrawArmorGenres(m, 0x118, 0x3A);
            break;
        }
        break;
    case 10:
        switch (m->step) {
        default:
            switch (m->kind[m->menuSel]) {
            case 4:
                Records_DrawWeaponGenres(m, 0x118, 0x3A);
                break;
            case 5:
                Records_DrawArmorGenres(m, 0x118, 0x3A);
                break;
            }
            break;
        case 2:
            switch (m->kind[m->menuSel]) {
            case 4:
                Records_DrawWeaponList(m, 0x118, 0x3A);
                break;
            case 5:
                Records_DrawArmorList(m, 0x118, 0x3A);
                break;
            }
            break;
        }
        break;
    case 9:
        Records_DrawHornScores(m, 0x118, 0x3A);
        break;
    case 8:
        Records_DrawMonsters(m, 0x118, 0x3A);
        break;
    case 6:
        if (m->step == 0) {
            Records_DrawCategories(m, 0x118, 0x3A);
        } else if (m->step == 1) {
            Records_DrawRows(m, 0x118, 0x3A);
        } else {
            Records_DrawPage(m, 0x118, 0x3A);
        }
        break;
    case 4:
        if (m->step == 0) {
            Records_DrawItemGenres(m, 0x118, 0x3A);
        } else {
            Records_DrawItemList(m, 0x118, 0x3A);
        }
        break;
    case 5:
        if (m->step == 0) {
            Records_DrawTitleGenres(m, 0x118, 0x3A);
        } else {
            Records_DrawTitleList(m, 0x118, 0x3A);
        }
        break;
    case 7:
        Records_DrawInvite(m, 0x118, 0x3A);
        break;
    }
}

void Records_Draw2(MenuTask *t)
{
    s8 *m = (s8 *)gMenuWork;

    Text_SetSize(t->textSize);
    switch (m[0x772]) {
    case 0:
    case 1:
    case 2:
    case 3:
        Records_DrawMenu((RecordMenu *)m, *(s16 *)m, 0x3A);
        break;
    }
}

extern char *gHuntRecordTexts[];
extern char *D_0080C3EC;
extern char *gHuntCountTexts[];
extern char *D_0080C3E4;
extern char *gMinSizeTexts[];
extern char *gMonsterNames[];
u16 MonsterGroup_Max(u16 *list, u8 group, u8 *out);
u16 MonsterGroup_MinNonzero(u16 *list, u8 group, u8 *out);
f32 Monster_SizeFromPercent(u8 kind, u16 x);
f64 fptodp(f32 x);
/* unit's own declaration: with the definition's s32 (u16 i, s16 x, s16 y, s32 v) (sub_main/unk_33E620.c), Records_DrawMonsters: 77 of 301 words differ */
s32 MonsterNote_DrawLargeCrown(u16 a, s16 x, s16 y, s16 n);
/* unit's own declaration: with the definition's s32 (u16 i, s16 x, s16 y, s32 v) (sub_main/unk_33E620.c), Records_DrawMonsters: 38 of 301 words differ */
s32 MonsterNote_DrawSmallCrown(u16 a, s16 x, s16 y, s16 n);

void Records_DrawMonsters(RecordMenu *m, s16 x, s16 y)
{
    s32 i;
    s16 ty = y;
    s16 page;
    s32 n;
    s32 j;
    char buf[0x80];
    MonsterNoteDef *e;
    s32 tx;
    s32 c;
    s32 c2;
    s32 sy;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, gHuntRecordTexts[0], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    n = 0;
    e = gMonsterNoteDefs;
    for (i = 0; i < 0x34; i++, e++) {
        if (MonsterGroup_Sum(m->hunts, e->emType, 0) > 0) {
            n++;
        }
    }
    if (n == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos((s16)(x + 0x14) + 0x32, (s16)(y + 0x16) + 0x64);
        Text_PrintfColored(D_0080C3EC);
        return;
    }
    page = m->page;
    i = 0;
    e = gMonsterNoteDefs;
    if (page > 0) {
        n = 0;
        do {
            if (MonsterGroup_Sum(m->hunts, e->emType, 0) > 0) {
                n++;
                if (n / 6 == page) {
                    i++;
                    e++;
                    break;
                }
            }
            i++;
            e++;
        } while (i < 0x34);
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x + 0x11E, y + 4);
    Menu_DrawPageNum(x + 0xF2, y + 2, page, m->pages, 0);
    ty += 0x16;
    j = 0;
    if (i < 0x34) {
        tx = x + 0x14;
        do {
            c = MonsterGroup_Sum(m->hunts, e->emType, 0);
            if (c > 0) {
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(tx, ty);
                Text_PrintfColored(D_00825C30, gMonsterNames[e->emType]);
                Font_SetPos((s16)tx + 0xB4, ty);
                sprintf(buf, gHuntCountTexts[0], c);
                Text_PrintfColored(buf);
                Font_SetSpacing(0x10, 0x10);
                c2 = MonsterGroup_Max(m->sizeMax, e->emType, 0);
                if (c2 > 0) {
                    f32 size = Monster_SizeFromPercent(e->emType, c2);
                    sy = ty + 0x14;
                    Font_SetPos((s16)tx - 2, sy);
                    sprintf(buf, D_0080C3E4, fptodp(size));
                    Text_PrintfColored(buf);
                    MonsterNote_DrawLargeCrown(e->note, (s16)tx - 1, (s16)sy - 2, c2);
                }
                c = MonsterGroup_MinNonzero(m->sizeMin, e->emType, 0);
                if (c > 0) {
                    f32 size = Monster_SizeFromPercent(e->emType, c);
                    sy = ty + 0x14;
                    Font_SetPos((s16)tx + 0x9A, ty + 0x14);
                    sprintf(buf, gMinSizeTexts[0], fptodp(size));
                    Text_PrintfColored(buf);
                    MonsterNote_DrawSmallCrown(e->note, (s16)tx + 0x9C, (s16)sy - 2, c);
                }
                ty += 0x28;
                j++;
            }
            if (j >= 6) {
                break;
            }
            i++;
            e++;
        } while (i < 0x34);
    }
}

s32 Records_CountMonsterPages(u8 *save)
{
    s32 i;
    MonsterNoteDef *p;
    s32 n;
    s32 q;

    n = 0;
    p = gMonsterNoteDefs;
    for (i = 0; i < 0x34; i++, p++) {
        if (MonsterGroup_Sum((u16 *) (save + 0x2B0), p->emType, 0) > 0) {
            n++;
        }
    }
    if (n == 0) {
        return 1;
    }
    q = n / 6;
    n -= q * 6;
    if (n != 0) {
        q++;
    }
    return q;
}

/* unk_6B1D70's .data (0x7DB880-0x7DBED0) */
extern u32 D_0081C600[];
extern u32 D_0081C710[];
extern u32 D_0081C7E0[];
extern u32 D_0081C960[];
extern u32 D_0081CA70[];
extern u32 D_0081CC40[];
extern u32 D_0081CD10[];
extern u32 D_0081CE00[];
extern u32 D_0081CF70[];
extern char gRecordCats_0_1_0[];
extern char gRecordCats_0_1_1[];
extern char gRecordCats_0_1_2[];
extern char gRecordCats_0_4_0[];
extern char gRecordCats_0_4_1[];
extern char gRecordCats_0_7_0[];
extern char gRecordCats_0_7_1[];
extern char gRecordCats_0_10_0[];
extern char gRecordCats_0_10_1[];
extern char gRecordCats_0_13_0[];
extern char gRecordCats_0_13_1[];
extern char gRecordCats_0_13_2[];
extern char gRecordCats_0_16_0[];
extern char gRecordCats_0_16_1[];
extern char gRecordCats_0_19_0[];
extern char gRecordCats_0_19_1[];
extern char gRecordCats_0_22_0[];
extern char gRecordCats_0_22_1[];
extern char gRecordCats_0_22_2[];
extern char gRecordCats_0_22_3[];
extern char gRecordCats_0_28_0[];
extern char gRecordCats_0_28_1[];
extern char gRecordCats_0_28_2[];
extern char gRecordCats_0_28_3[];
extern char gRecordCats_0_31_1[];
extern char gRecordCats_0_37_0[];
extern char gRecordCats_0_37_1[];
extern char gRecordCats_0_37_2[];
extern char gRecordCats_0_40_0[];
extern char gRecordCats_0_40_1[];
extern char gRecordCats_0_43_0[];
extern char gRecordCats_0_43_1[];
extern char gRecordCats_1_1_0[];
extern char gRecordCats_1_1_1[];
extern char gRecordCats_1_4_0[];
extern char gRecordCats_1_4_1[];
extern char gRecordCats_1_4_2[];
extern char gRecordCats_1_4_3[];
extern char gRecordCats_1_7_0[];
extern char gRecordCats_1_7_1[];
extern char gRecordCats_1_10_0[];
extern char gRecordCats_1_10_1[];
extern char gRecordCats_1_13_0[];
extern char gRecordCats_1_13_1[];
extern char gRecordCats_1_13_2[];
extern char gRecordCats_1_16_0[];
extern char gRecordCats_1_16_1[];
extern char gRecordCats_1_19_0[];
extern char gRecordCats_1_19_1[];
extern char gRecordCats_1_22_0[];
extern char gRecordCats_1_22_1[];
extern char gRecordCats_1_25_0[];
extern char gRecordCats_1_28_0[];
extern char gRecordCats_1_28_1[];
extern char gRecordCats_1_28_2[];
extern char gRecordCats_1_28_3[];
extern char gRecordCats_2_1_0[];
extern char gRecordCats_2_4_0[];
extern char gRecordCats_2_4_1[];
extern char gRecordCats_2_7_0[];
extern char gRecordCats_2_7_1[];
extern char gRecordCats_2_7_2[];
extern char gRecordCats_2_10_0[];
extern char gRecordCats_2_10_1[];
extern char gRecordCats_2_10_2[];
extern char gRecordCats_2_10_3[];
extern char gRecordCats_2_10_4[];
extern char gRecordCats_2_13_0[];
extern char gRecordCats_2_13_1[];
extern char gRecordCats_2_13_2[];
extern char gRecordCats_2_16_0[];
extern char gRecordCats_2_16_1[];
extern char gRecordCats_2_19_0[];
extern char gRecordCats_2_19_1[];
extern char gRecordCats_2_19_2[];
extern char gRecordCats_2_22_0[];
extern char gRecordCats_2_22_1[];
extern char gRecordCats_2_25_0[];
extern char gRecordCats_2_25_1[];
extern char gRecordCats_2_28_0[];
extern char gRecordCats_2_28_1[];
extern char gRecordCats_3_1_0[];
extern char gRecordCats_3_1_1[];
extern char gRecordCats_3_1_2[];
extern char gRecordCats_3_1_3[];
extern char gRecordCats_3_4_0[];
extern char gRecordCats_3_4_1[];
extern char gRecordCats_3_4_2[];
extern char gRecordCats_3_7_0[];
extern char gRecordCats_3_7_1[];
extern char gRecordCats_3_7_2[];
extern char gRecordCats_3_7_3[];
extern char gRecordCats_3_10_0[];
extern char gRecordCats_3_10_1[];
extern char gRecordCats_3_10_2[];
extern char gRecordCats_3_13_0[];
extern char gRecordCats_3_13_1[];
extern char gRecordCats_3_13_2[];
extern char gRecordCats_3_16_0[];
extern char gRecordCats_3_16_1[];
extern char gRecordCats_3_16_2[];
extern char gRecordCats_3_16_3[];
extern char gRecordCats_3_16_4[];
extern char gRecordCats_3_19_0[];
extern char gRecordCats_3_19_1[];
extern char gRecordCats_3_22_0[];
extern char gRecordCats_3_22_1[];
extern char gRecordCats_3_22_2[];
extern char gRecordCats_3_22_3[];
extern char gRecordCats_3_25_0[];
extern char gRecordCats_3_25_1[];
extern char gRecordCats_3_25_2[];
extern char gRecordCats_3_25_3[];
extern char gRecordCats_3_28_0[];
extern char gRecordCats_3_28_1[];
extern char gRecordCats_3_28_2[];
extern char gRecordCats_3_31_0[];
extern char gRecordCats_3_31_1[];
extern char gRecordCats_3_31_2[];
extern char gRecordCats_3_34_0[];
extern char gRecordCats_3_34_1[];
extern char gRecordCats_3_34_2[];
extern char gRecordCats_3_37_0[];
extern char gRecordCats_3_37_1[];
extern char gRecordCats_3_37_2[];
extern char gRecordCats_3_40_0[];
extern char gRecordCats_3_40_1[];
extern char gRecordCats_3_40_2[];
extern char gRecordCats_3_40_3[];
extern char gRecordCats_3_43_0[];
extern char gRecordCats_3_43_1[];
extern char gRecordCats_3_43_2[];
extern char gRecordCats_3_43_3[];
extern char gRecordCats_3_46_0[];
extern char gRecordCats_3_46_1[];
extern char gRecordCats_3_46_2[];
extern char gRecordCats_3_46_3[];
extern char gRecordCats_3_46_4[];
extern char gRecordCats_3_49_0[];
extern char gRecordCats_3_49_1[];
extern char gRecordCats_3_49_2[];
extern char gRecordCats_3_49_3[];
extern char gRecordCats_3_52_0[];
extern char gRecordCats_3_52_1[];
extern char gRecordCats_3_52_2[];
extern char gRecordCats_3_52_3[];
extern char gRecordCats_3_55_0[];
extern char gRecordCats_3_55_1[];
extern char gRecordCats_3_55_2[];
extern char gRecordCats_3_55_3[];
extern char gRecordCats_3_58_0[];
extern char gRecordCats_3_58_1[];
extern char gRecordCats_3_58_2[];
extern char gRecordCats_3_58_3[];
extern char gRecordCats_3_61_0[];
extern char gRecordCats_3_61_1[];
extern char gRecordCats_3_61_2[];
extern char gRecordCats_3_64_0[];
extern char gRecordCats_3_64_1[];
extern char gRecordCats_3_67_0[];
extern char gRecordCats_3_67_1[];
extern u32 gRecordCats_0_0[];
extern u32 gRecordCats_0_3[];
extern u32 gRecordCats_0_6[];
extern u32 gRecordCats_0_9[];
extern u32 gRecordCats_0_12[];
extern u32 gRecordCats_0_15[];
extern u32 gRecordCats_0_18[];
extern u32 gRecordCats_0_21[];
extern u32 gRecordCats_0_24[];
extern u32 gRecordCats_0_27[];
extern u32 gRecordCats_0_30[];
extern u32 gRecordCats_0_33[];
extern u32 gRecordCats_0_36[];
extern u32 gRecordCats_0_39[];
extern u32 gRecordCats_0_42[];
extern u32 gRecordCats_1_0[];
extern u32 gRecordCats_1_3[];
extern u32 gRecordCats_1_6[];
extern u32 gRecordCats_1_9[];
extern u32 gRecordCats_1_12[];
extern u32 gRecordCats_1_15[];
extern u32 gRecordCats_1_18[];
extern u32 gRecordCats_1_21[];
extern u32 gRecordCats_1_24[];
extern u32 gRecordCats_1_27[];
extern u32 gRecordCats_2_0[];
extern u32 gRecordCats_2_3[];
extern u32 gRecordCats_2_6[];
extern u32 gRecordCats_2_9[];
extern u32 gRecordCats_2_12[];
extern u32 gRecordCats_2_15[];
extern u32 gRecordCats_2_18[];
extern u32 gRecordCats_2_21[];
extern u32 gRecordCats_2_24[];
extern u32 gRecordCats_2_27[];
extern u32 gRecordCats_3_0[];
extern u32 gRecordCats_3_3[];
extern u32 gRecordCats_3_6[];
extern u32 gRecordCats_3_12[];
extern u32 gRecordCats_3_18[];
extern u32 gRecordCats_3_24[];
extern u32 gRecordCats_3_30[];
extern u32 gRecordCats_3_36[];
extern u32 gRecordCats_3_42[];
extern u32 gRecordCats_3_48[];
extern u32 gRecordCats_3_54[];
extern u32 gRecordCats_3_60[];
extern u32 gRecordCats_3_66[];
extern u32 gRecordCats_0_a[];
extern u32 gRecordCats_1_a[];
extern u32 gRecordCats_2_a[];
extern u32 gRecordCats_3_a[];
char *D_007DB880[8] = {
    (char *)D_0081C600, (char *)D_0081C710, (char *)D_0081C7E0, (char *)D_0081C960,
    (char *)D_0081CA70, NULL, NULL, NULL,
};
char *D_007DB8A0[4] = {
    (char *)D_0081CC40, (char *)D_0081CD10, (char *)D_0081CE00, (char *)D_0081CF70,
};
char *gRecordCats_0_1[4] = {
    gRecordCats_0_1_0, gRecordCats_0_1_1, gRecordCats_0_1_2, NULL,
};
char *gRecordCats_0_4[2] = {
    gRecordCats_0_4_0, gRecordCats_0_4_1,
};
char *gRecordCats_0_7[2] = {
    gRecordCats_0_7_0, gRecordCats_0_7_1,
};
char *gRecordCats_0_10[2] = {
    gRecordCats_0_10_0, gRecordCats_0_10_1,
};
char *gRecordCats_0_13[3] = {
    gRecordCats_0_13_0, gRecordCats_0_13_1, gRecordCats_0_13_2,
};
char *gRecordCats_0_16[2] = {
    gRecordCats_0_16_0, gRecordCats_0_16_1,
};
char *gRecordCats_0_19[4] = {
    gRecordCats_0_19_0, gRecordCats_0_19_1, NULL, NULL,
};
char *gRecordCats_0_22[4] = {
    gRecordCats_0_22_0, gRecordCats_0_22_1, gRecordCats_0_22_2, gRecordCats_0_22_3,
};
extern char gRecordCats_0_25_0[];
extern char gRecordCats_0_25_1[];
extern char gRecordCats_0_31_0[];
extern char gRecordCats_0_34_0[];
extern char gRecordCats_0_34_1[];
extern char gRecordCats_0_34_2[];
char *gRecordCats_0_25[4] = {
    gRecordCats_0_25_0, gRecordCats_0_25_1, NULL, NULL,
};
char *gRecordCats_0_28[4] = {
    gRecordCats_0_28_0, gRecordCats_0_28_1, gRecordCats_0_28_2, gRecordCats_0_28_3,
};
char *gRecordCats_0_31[2] = {
    gRecordCats_0_31_0, gRecordCats_0_31_1,
};
char *gRecordCats_0_34[3] = {
    gRecordCats_0_34_0, gRecordCats_0_34_1, gRecordCats_0_34_2,
};
char *gRecordCats_0_37[3] = {
    gRecordCats_0_37_0, gRecordCats_0_37_1, gRecordCats_0_37_2,
};
char *gRecordCats_0_40[2] = {
    gRecordCats_0_40_0, gRecordCats_0_40_1,
};
char *gRecordCats_0_43[2] = {
    gRecordCats_0_43_0, gRecordCats_0_43_1,
};
char *gRecordCats_1_1[2] = {
    gRecordCats_1_1_0, gRecordCats_1_1_1,
};
char *gRecordCats_1_4[4] = {
    gRecordCats_1_4_0, gRecordCats_1_4_1, gRecordCats_1_4_2, gRecordCats_1_4_3,
};
char *gRecordCats_1_7[2] = {
    gRecordCats_1_7_0, gRecordCats_1_7_1,
};
char *gRecordCats_1_10[2] = {
    gRecordCats_1_10_0, gRecordCats_1_10_1,
};
char *gRecordCats_1_13[4] = {
    gRecordCats_1_13_0, gRecordCats_1_13_1, gRecordCats_1_13_2, NULL,
};
char *gRecordCats_1_16[2] = {
    gRecordCats_1_16_0, gRecordCats_1_16_1,
};
char *gRecordCats_1_19[2] = {
    gRecordCats_1_19_0, gRecordCats_1_19_1,
};
char *gRecordCats_1_22[2] = {
    gRecordCats_1_22_0, gRecordCats_1_22_1,
};
char *gRecordCats_1_25[2] = {
    gRecordCats_1_25_0, NULL,
};
char *gRecordCats_1_28[4] = {
    gRecordCats_1_28_0, gRecordCats_1_28_1, gRecordCats_1_28_2, gRecordCats_1_28_3,
};
char *gRecordCats_2_1[2] = {
    gRecordCats_2_1_0, NULL,
};
char *gRecordCats_2_4[2] = {
    gRecordCats_2_4_0, gRecordCats_2_4_1,
};
char *gRecordCats_2_7[4] = {
    gRecordCats_2_7_0, gRecordCats_2_7_1, gRecordCats_2_7_2, NULL,
};
char *gRecordCats_2_10[6] = {
    gRecordCats_2_10_0, gRecordCats_2_10_1, gRecordCats_2_10_2, gRecordCats_2_10_3,
    gRecordCats_2_10_4, NULL,
};
char *gRecordCats_2_13[3] = {
    gRecordCats_2_13_0, gRecordCats_2_13_1, gRecordCats_2_13_2,
};
char *gRecordCats_2_16[2] = {
    gRecordCats_2_16_0, gRecordCats_2_16_1,
};
char *gRecordCats_2_19[4] = {
    gRecordCats_2_19_0, gRecordCats_2_19_1, gRecordCats_2_19_2, NULL,
};
char *gRecordCats_2_22[2] = {
    gRecordCats_2_22_0, gRecordCats_2_22_1,
};
char *gRecordCats_2_25[2] = {
    gRecordCats_2_25_0, gRecordCats_2_25_1,
};
char *gRecordCats_2_28[4] = {
    gRecordCats_2_28_0, gRecordCats_2_28_1, NULL, NULL,
};
char *gRecordCats_3_1[4] = {
    gRecordCats_3_1_0, gRecordCats_3_1_1, gRecordCats_3_1_2, gRecordCats_3_1_3,
};
char *gRecordCats_3_4[4] = {
    gRecordCats_3_4_0, gRecordCats_3_4_1, gRecordCats_3_4_2, NULL,
};
char *gRecordCats_3_7[4] = {
    gRecordCats_3_7_0, gRecordCats_3_7_1, gRecordCats_3_7_2, gRecordCats_3_7_3,
};
char *gRecordCats_3_10[4] = {
    gRecordCats_3_10_0, gRecordCats_3_10_1, gRecordCats_3_10_2, NULL,
};
char *gRecordCats_3_13[4] = {
    gRecordCats_3_13_0, gRecordCats_3_13_1, gRecordCats_3_13_2, NULL,
};
char *gRecordCats_3_16[6] = {
    gRecordCats_3_16_0, gRecordCats_3_16_1, gRecordCats_3_16_2, gRecordCats_3_16_3,
    gRecordCats_3_16_4, NULL,
};
char *gRecordCats_3_19[2] = {
    gRecordCats_3_19_0, gRecordCats_3_19_1,
};
char *gRecordCats_3_22[4] = {
    gRecordCats_3_22_0, gRecordCats_3_22_1, gRecordCats_3_22_2, gRecordCats_3_22_3,
};
char *gRecordCats_3_25[4] = {
    gRecordCats_3_25_0, gRecordCats_3_25_1, gRecordCats_3_25_2, gRecordCats_3_25_3,
};
char *gRecordCats_3_28[4] = {
    gRecordCats_3_28_0, gRecordCats_3_28_1, gRecordCats_3_28_2, NULL,
};
char *gRecordCats_3_31[4] = {
    gRecordCats_3_31_0, gRecordCats_3_31_1, gRecordCats_3_31_2, NULL,
};
char *gRecordCats_3_34[4] = {
    gRecordCats_3_34_0, gRecordCats_3_34_1, gRecordCats_3_34_2, NULL,
};
char *gRecordCats_3_37[4] = {
    gRecordCats_3_37_0, gRecordCats_3_37_1, gRecordCats_3_37_2, NULL,
};
char *gRecordCats_3_40[4] = {
    gRecordCats_3_40_0, gRecordCats_3_40_1, gRecordCats_3_40_2, gRecordCats_3_40_3,
};
char *gRecordCats_3_43[4] = {
    gRecordCats_3_43_0, gRecordCats_3_43_1, gRecordCats_3_43_2, gRecordCats_3_43_3,
};
char *gRecordCats_3_46[8] = {
    gRecordCats_3_46_0, gRecordCats_3_46_1, gRecordCats_3_46_2, gRecordCats_3_46_3,
    gRecordCats_3_46_4, NULL, NULL, NULL,
};
char *gRecordCats_3_49[4] = {
    gRecordCats_3_49_0, gRecordCats_3_49_1, gRecordCats_3_49_2, gRecordCats_3_49_3,
};
char *gRecordCats_3_52[4] = {
    gRecordCats_3_52_0, gRecordCats_3_52_1, gRecordCats_3_52_2, gRecordCats_3_52_3,
};
char *gRecordCats_3_55[4] = {
    gRecordCats_3_55_0, gRecordCats_3_55_1, gRecordCats_3_55_2, gRecordCats_3_55_3,
};
char *gRecordCats_3_58[4] = {
    gRecordCats_3_58_0, gRecordCats_3_58_1, gRecordCats_3_58_2, gRecordCats_3_58_3,
};
char *gRecordCats_3_61[4] = {
    gRecordCats_3_61_0, gRecordCats_3_61_1, gRecordCats_3_61_2, NULL,
};
char *gRecordCats_3_64[2] = {
    gRecordCats_3_64_0, gRecordCats_3_64_1,
};
char *gRecordCats_3_67[2] = {
    gRecordCats_3_67_0, gRecordCats_3_67_1,
};
u32 gRecordCats_0[48] = {
#include "lobby/007DBBC0.inc"
};
u32 gRecordCats_1[32] = {
#include "lobby/007DBC80.inc"
};
u32 gRecordCats_2[32] = {
#include "lobby/007DBD00.inc"
};
u32 gRecordCats_3[72] = {
#include "lobby/007DBD80.inc"
};
RecordCat gRecordCats[4] = {
#include "lobby/007DBEA0.inc"
};
