/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "scene.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_state.h"
#include "player.h"
#include "lobby_menu.h"
#include "area.h"

u32 Equip_GetPrice(u8 type, u16 id);
void CamFocus_End(void);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gFriendListOpen;
extern u8 gMenuWork;

/* Menu 0x38's items (gSongMenuItems; a kind of -1 ends the list) */
typedef struct Menu38Item {
    s32 kind;
    u16 item;
    u8 value;
    u8 pad7;
} Menu38Item;
/* Menu 0x38's work area (at gMenuWork) */
typedef struct Menu38Work {
    WinSlide win;
    Menu38Item list[5];         /* 0x08: the items held */
    s16 count;                  /* 0x30 */
    s16 cursor;                 /* 0x32 */
    s16 unk34;
    s8 state;                   /* 0x36 */
    s8 toggle;                  /* 0x37: button 0x200 */
} Menu38Work;

typedef struct ForgeRecipe {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 pad1;
    /* 0x02 */ u16 id;
    /* 0x04 */ struct {
        u16 item;
        s16 count;
    } need[4];                  /* the materials */
    /* 0x14 */ u8 unk14[4];
} ForgeRecipe;
extern ForgeRecipe gWeaponRecipes[];
typedef struct RecipeMenu {
    EquipItem shown;            /* 0x00 */
    u8 unkC[0x10];
    s16 winX;                   /* 0x1C */
    u8 unk1E[0xBA];
    s32 list[8];                /* 0xD8: recipe indexes of the page */
    s16 page;                   /* 0xF8 */
    s16 pages;                  /* 0xFA */
    s16 detailPage;             /* 0xFC */
    u8 unkFE[4];
    s16 row;                    /* 0x102: weapon kind */
    s16 cursor;                 /* 0x104 */
    s16 count;                  /* 0x106: rows on the page */
    s16 detail;                 /* 0x108 */
    u8 unk10A[3];
    s8 state;                   /* 0x10D */
    u8 unk10E[2];
    s8 unk110;                  /* 0x110 */
} RecipeMenu;

u32 Equip_HalfPrice(s32 type, s32 id) {
    return Equip_GetPrice(type & 0xFF, id & 0xFFFF) >> 1;
}

void *memset(void *dst, s32 c, u32 n);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
extern char D_0082EA28[];
extern char *gPriceMarks[];
extern char *gUnknownNameTexts[];
extern char *D_0080C78C;
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
s32 Equip_TestFlag(EquipItem *e, s16 bit);
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, EquipItem *e) (lobby/unk_78F070.c), WeaponForge_DrawMakeListFull 6 of 292 words, WeaponForge_DrawMakeList 6 of 250 words differ */
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 color);
struct WeaponMenu;
s32 Forge_CanMakeWeapon(struct WeaponMenu *m, s16 i);
s32 Forge_IsFixedPrice(s16 id);
char *Equip_GetName(u8 type, u16 id);
void EquipShop_DrawLine(s16 x, s16 y, EquipItem *e);
void WeaponForge_DrawMakeListFull(RecipeMenu *m, s16 x, s16 y)
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
    memset(&sel, 0, sizeof(sel));
    Icon_Draw(x, y, 0x154, 0x122, 0xFFFFDDDD, 0);
    ly = ty;
    lx = x;
    cy = ty;
    Icon_Draw(lx + 0x6E, ly + 0x2D, 0x78, 0xC8, 0x60FFFFFF, 0x24);
    Font_SetColor(0x80101010);
    if (m->count == 0) {
        s = D_0080C78C;
        cx = Text_CenterX(s, lx + 0xAA, 0x12);
        Font_SetColor(0x8000052C);
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(cx, cy + 0x5A);
        Text_PrintfColored(D_0082EA28, s);
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
            r = &gWeaponRecipes[m->list[i]];
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
                Icon_Draw(tx, ty + 0xE, 0x112, 8, 0x99106060, 0x6F);
                Menu_DrawCursorIcon((s16)tx - 0x32 - d, ty - 2, 0x18, 0xFF20C0C0);
            }
            if (Forge_CanMakeWeapon((struct WeaponMenu *)m, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(tx, ty);
            if (Forge_IsFixedPrice(tmp.id) != 0) {
                Text_PrintfColored("%-20s %6d%s", gUnknownNameTexts[0], 1000, gPriceMarks[0]);
            } else {
                Text_PrintfColored("%-20s %6d%s", Equip_GetName(r->type, r->id), Equip_HalfPrice(r->type, r->id), gPriceMarks[0]);
            }
            ty += 0x16;
            i++;
        } while (i < m->count);
    }
    Menu_DrawDashLine(lx + 0x14, ly + 0xBA, 0x12C, 0xFF000000);
    EquipShop_DrawLine(lx + 0x10, ly + 0xC8, &sel);
}

void WeaponForge_DrawMakeList(RecipeMenu *m, s16 x, s16 y)
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
            r = &gWeaponRecipes[m->list[i]];
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
            if (Forge_CanMakeWeapon((struct WeaponMenu *)m, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(tx, ty);
            if (Forge_IsFixedPrice(tmp.id) != 0) {
                Text_PrintfColored("%-20s", gUnknownNameTexts[0]);
                Font_SetPos((s16)tx + 0xB2, ty);
                Text_PrintfColored("%6d%s", 1000, gPriceMarks[0]);
            } else {
                Text_PrintfColored("%-20s", Equip_GetName(r->type, r->id));
                Font_SetPos((s16)tx + 0xB2, ty);
                Text_PrintfColored("%6d%s", Equip_HalfPrice(r->type, r->id), gPriceMarks[0]);
            }
            ty += 0x14;
            i++;
        } while (i < m->count);
    }
}

extern LobbyMenu *gLobbyMenu;

extern char *gWeaponClassTexts[];
extern char *D_0080C8F4;
extern char *gWeaponMakePromptTexts[];
extern char *gNeedMaterialsTexts[];
extern char *gYesNoTexts[];
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void EquipPreview_Show(u8 on);
void Equip_DrawPreviewHelp(u32 color, s8 k);
void EquipWindow_DrawLocal(EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other);
void WeaponForge_DrawMakeMaterials(RecipeMenu *m, s16 x, s16 y);
void EquipCompare_Draw(EquipItem *a, EquipItem *b, s32 c, s16 y, u8 flags);
void EquipDetail_Draw(Player *a, s16 x, s16 y, u32 flags);
char *Text_FillDollar(char *arg0, s32 arg1);
void Text_Printf(const char *fmt, ...);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void WeaponForge_DrawMake(RecipeMenu *m)
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
        Menu_DrawLabelBar(10, 0x12, 0xBE, 0xFFB0B000, gWeaponClassTexts[m->row], 2, 1);
        Menu_DrawMoney(0x1E0, 0x10, 0);
        r = &gWeaponRecipes[m->list[cur]];
        tmp.used = 1;
        y = 0x34;
        tmp.type = r->type;
        tmp.id = r->id;
        x = m->winX;
    }
    switch (m->state) {
    case 0:
        Font_SetSpacing(0x14, 0x14);
        Menu_DrawLabelList(0x1A4, gWeaponClassTexts, 5, m->row, 0xFFB0B000);
        break;
    case 1:
    case 2:
        switch (m->detail) {
        case 0:
            WeaponForge_DrawMakeListFull(m, x + 0xC, y);
            break;
        case 1:
            if (m->unk110 == 0) {
                EquipPreview_Show(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, m->unk110);
            if (m->unk110 == 0 && Forge_IsFixedPrice(m->shown.id)) {
                s = D_0080C8F4;
                EquipPreview_Show(1);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(s);
            }
            WeaponForge_DrawMakeList(m, 10, 0x32);
            EquipWindow_DrawLocal(&tmp, 0x136, 0xD8, m->detailPage | 0x40, NULL);
            break;
        case 2:
            if (m->unk110 == 0) {
                EquipPreview_Show(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, m->unk110);
            if (m->unk110 == 0 && Forge_IsFixedPrice(m->shown.id)) {
                s = D_0080C8F4;
                EquipPreview_Show(1);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(Text_CenterX(s, 0x1CE, 0x12), 0x6E);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(s);
            }
            WeaponForge_DrawMakeList(m, 10, 0x32);
            WeaponForge_DrawMakeMaterials(m, 0x136, 0xD8);
            break;
        case 3:
            WeaponForge_DrawMakeList(m, 10, 0x32);
            EquipCompare_Draw(gSave.equip, &tmp, 0x136, 0x36, m->detailPage | 0x50);
            break;
        case 4:
            WeaponForge_DrawMakeList(m, 10, 0x32);
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 0x136, 0x3A, m->detailPage);
            break;
        }
        if (m->detail != 4) {
            Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, gWeaponMakePromptTexts[0]);
        }
        break;
    case 3:
        EquipCompare_Draw(gSave.equip, &tmp, 0x136, 0x36, m->detailPage | 0x50);
        if (Forge_IsFixedPrice(tmp.id)) {
            s = Text_FillDollar((char *) gNeedMaterialsTexts[0], 1000);
        } else {
            s = Text_FillDollar((char *) gNeedMaterialsTexts[0], Equip_HalfPrice(tmp.type, tmp.id));
        }
        Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
        Font_SetPos(0x1BC, 0x190);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

extern char *D_0080C82C;
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), WeaponForge_DrawMakeMaterials 63 of 76 words, SongMenu_DrawList 153 of 185 words differ */
s32 Items_DrawNeedLine(s16, s16, u16, s32, u32);
void WeaponForge_DrawMakeMaterials(RecipeMenu *m, s16 x, s16 y)
{
    ForgeRecipe *r = &gWeaponRecipes[m->list[m->cursor]];
    s16 i;

    Win_Draw(x, y, 0x132, 0x72, 0xBDB59F6C, 0xA0202020, 6);
    y += 6;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 10, y);
    Text_PrintfColored(D_0080C82C);
    for (i = 0; i < 4; i++) {
        y += 20;
        Items_DrawNeedLine(x + 0x1E, y, r->need[i].item, r->need[i].count, 0x807F7F7F);
    }
}

extern char *gWeaponForgeTexts[];
struct WeaponMenu;
void WeaponForge_DrawDetail(struct WeaponMenu *m, s16 y, s32 h); /* unit's own declaration: with the definition's void (WeaponMenu *m, s32 x, s16 y) (lobby/unk_6E01D0.c), WeaponForge_Draw 22 of 50 words differ */
void WeaponForge_DrawUpgradeBox(struct WeaponMenu *m, s16 x, s16 y);
void WeaponForge_DrawMake(RecipeMenu *m);
void WeaponForge_Draw(u8 *o)
{
    u8 *m = o + 0xC;

    switch ((s8)o[0x118]) {
    case 0:
        Font_SetSpacing(0x14, 0x14);
        Menu_DrawLabelList(0x1CC, gWeaponForgeTexts, 2, *(s16 *)(m + 0xF4), 0xFFB0B000);
        break;
    case 1:
        WeaponForge_DrawDetail((struct WeaponMenu *) m, *(s16 *)(m + 0x1C) + 10, 0x32);
        WeaponForge_DrawUpgradeBox((struct WeaponMenu *) m, *(s16 *)(m + 0x24) + 6, 0x36);
        break;
    case 2:
        WeaponForge_DrawMake((RecipeMenu *)m);
        break;
    }
}

typedef struct ForgeView {
    u8 unk0[0xC];
    EquipItem item;             /* 0x0C: the piece being made (fixed-price recipes) */
    u8 unk18[0x34];
    EquipItem box[11];          /* 0x4C: the box page's pieces (upgrades) */
    u8 unkD0[8];
    s32 list[7];                /* 0xD8 */
    s16 tab;                    /* 0xF4 */
    u8 unkF6[6];
    s16 unkFC;                  /* 0xFC */
    u8 unkFE[6];
    s16 cursor;                 /* 0x104 */
    u8 unk106[6];
    s8 prompt;                  /* 0x10C */
    u8 unk10D[2];
    s8 kind;                    /* 0x10F */
} ForgeView;
extern char *D_0080C814;
extern char *gWeaponChangeWarnTexts[];
extern char *gEquipNowTexts[];

void WeaponForge_DrawPreview(u8 *o)
{
    ForgeView *m = (ForgeView *)(o + 0xC);
    ForgeRecipe *r;
    char *s;
    s32 k;
    EquipItem e;

    if ((s8)o[0x118] == 0) {
        return;
    }
    memset(&e, 0, sizeof(EquipItem));
    if (m->tab == 0) {
        k = m->list[m->cursor];
        r = &gWeaponRecipes[k];
        if (Forge_IsFixedPrice(gWeaponRecipes[k].id)) {
            e = m->item;
        } else {
            e.used = 1;
            e.type = r->type;
            e.id = r->id;
        }
    } else if (m->kind == 6) {
        e = m->box[m->cursor];
        if (e.id != 0) {
            e.used = 1;
        } else {
            e.used = 0;
        }
    } else {
        e = m->item;
    }
    switch (m->prompt) {
    case 1:
        s = D_0080C814;
        break;
    case 2:
        s = gWeaponChangeWarnTexts[0];
        break;
    case 3:
        return;
    }
    EquipCompare_Draw(gSave.equip, &e, 0x134, 0x38, m->unkFC | 0x50);
    Win_DrawText(0x130, 0x160, 0x13C, 0x50, 0xBDB59F6C, 0xA0202020, s);
    Font_SetPos(0x144, 0x190);
    Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
}

s32 LobbyBgm_IsActive(void);
s32 SongMenu_IsPlaying(void)
{
    if (gScene.area != AREA_HUB_0x87) {
        return 0;
    }
    if (LobbyBgm_IsActive() != 0) {
        return 1;
    }
    return 0;
}

extern Menu38Item gSongMenuItems[];
void *memcpy(void *dst, const void *src, u32 n);
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
void LobbySe_Play(s32);
s32 Items_HaveEnough(u16 item, s32 n);
void SongMenu_Draw(u8 *o);
void SongMenu_Open(u8 *unused0)
{
    Menu38Work *w = (Menu38Work *)&gMenuWork;
    Menu38Item *p;
    s32 i;

    memset(w, 0, 0xF00);
    gTownMode = 0x38;
    gTownMenuOpen = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(SongMenu_Draw, 0, 0, 0);
    WinSlide_Step(&w->win, 0x280, 0x118);
    LobbySe_Play(8);
    w->count = 0;
    p = gSongMenuItems;
    for (i = 0; i < 5; i++) {
        if (p->kind == -1) {
            memcpy(&w->list[w->count], p, 8);
            return;
        }
        if (Items_HaveEnough(p->item, 1) == 1) {
            memcpy(&w->list[w->count], p, 8);
            w->count++;
        }
        p++;
    }
}

void SongMenu_Close(void) {
    gTownMode = 0;
    gTownMenuOpen = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
}

u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 LobbyBgm_EndsToday(s32 kind);
s32 LobbyBgm_GetTrack(void);
s32 LobbyTime_IsPm(void);
void TownMsg_SendSong(s32 kind);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_PushNoticeOnce(s32);
void SongMenu_Update(void)
{
    Menu38Work *w = (Menu38Work *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (w->state) {
    case 0:
        switch (WinSlide_Step(&w->win, 0x280, 0x118)) {
        case 0:
            w->state++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (LobbyBgm_EndsToday(w->list[w->cursor].kind) == 1 && LobbyBgm_GetTrack() == -1 && LobbyTime_IsPm() == 1) {
                w->state++;
                w->unk34 = 0xD2;
                TownMsg_SendSong(w->list[w->cursor].kind);
                LobbySe_Play(8);
            } else {
                Menu_PushNoticeOnce(0x28);
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            SongMenu_Close();
        } else if (pad & 0x200) {
            if (w->list[w->cursor].item != 0) {
                w->toggle ^= 1;
                LobbySe_Play(1);
            }
        } else {
            w->cursor = Menu_WrapUpDown(w->cursor, w->count);
        }
        break;
    case 2:
        if (--w->unk34 <= 0) {
            SongMenu_Close();
        }
        break;
    }
}

void SongMenu_OnBusy(void)
{
    s8 *w = (s8 *)&gMenuWork;

    if (gTownMode == 0x38 && w[0x36] == 2) {
        Menu_PushNoticeOnce(0x28);
        SongMenu_Close();
    }
}

void Items_Take(s32 item, s32 n);
s32 Save_TestEventFlag(s32 flag);
void Save_SetEventFlag(s32 flag);
void Menu_PushNotice(s32 msg);
void SongMenu_OnAccepted(void)
{
    Menu38Work *w = (Menu38Work *)&gMenuWork;
    u8 n;

    if (gTownMode == 0x38 && w->state == 2) {
        if (w->list[w->cursor].item != 0) {
            Items_Take(w->list[w->cursor].item, 1);
            n = gSave.unk36E9;
            if (n < 100) {
                gSave.unk36E9 = n + w->list[w->cursor].value;
            }
            if (w->list[w->cursor].kind == 3 && Save_TestEventFlag(0x122) == 0) {
                Save_SetEventFlag(0x122);
                Menu_PushNotice(0x21);
            }
        }
        SongMenu_Close();
    }
}

void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color);
extern char *gSongSelectTexts[];
extern char *D_0080C3F4;
extern char *D_0080C074;
extern char *gShortDashTexts[];
extern char *gSongHelpTexts[];
extern char *D_0080D1A4;

void SongMenu_DrawList(s16 x, s16 y)
{
    Menu38Work *w = (Menu38Work *)&gMenuWork;
    s16 ty = y;
    s16 i;
    u16 item;
    u16 sel;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x8C, 0xFFB0B000, gSongSelectTexts[0], 2, 1);
    Win_Draw(x, y, 0x154, 0x122, 0xBDB59F6C, 0x80202020, 2);
    ty += 0x1E;
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < 5; i++) {
        if (w->list[i].kind == -1) {
            break;
        }
        item = w->list[i].item;
        Font_SetPos(x + 0x32, ty);
        if (i == w->cursor) {
            sel = item;
            Menu_DrawLine(x + 10, ty - 1, 0x140, 0x18, 0);
        }
        if (item == 0) {
            Font_SetColor(0x807F7F7F);
            Text_PrintfColored("%-16s%s%s", D_0080C3F4, D_0080C074, gShortDashTexts[0]);
        } else {
            Menu_DrawItemCountRow(x + 0x32, ty, item, 0x807F7F7F);
        }
        ty += 0x16;
    }
    Gfx_DrawLine(x + 0x14, y + 0xBA, 0x12C, 2, 0xBDB59F6C);
    if (sel != 0) {
        ItemInfo_Draw((s16)(x + 0x32) - 0x28, y + 0xBE, 0x154, w->toggle, sel, 0, -1, 0x807F7F7F);
    } else {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(x + 0x5A, 0x100);
        Text_Printf(gSongHelpTexts[0]);
    }
    Win_DrawText(x, y + 0x12A, 0x154, 0x50, 0xBDB59F6C, 0x80202020, D_0080D1A4);
}

void Text_SetSize(s32);
void SongMenu_DrawList(s16 x, s16 y);
void SongMenu_Draw(u8 *o)
{
    s8 *w = (s8 *)&gMenuWork;
    s16 id;

    Text_SetSize(*(s32 *)(o + 0x18));
    id = *(s16 *)w;
    switch (w[0x36]) {
    case 0:
    case 1:
        SongMenu_DrawList(id, 0x3A);
        break;
    }
}

/* The list menu at 0x8BD610 (0x84 bytes; unk_6E56C0.c's ListMenu) */
typedef struct ListMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u8 unk8[0x14];
    /* 0x1C */ void (*draw)(u8 *);
    /* 0x20 */ s32 unk20;
    /* 0x24 */ u8 unk24[0x18];
    /* 0x3C */ void (*draw2)(u8 *);
    /* 0x40 */ s32 unk40;
    /* 0x44 */ u8 unk44[4];
    /* 0x48 */ s32 list[10];
    /* 0x70 */ s32 unk70;
    /* 0x74 */ s32 unk74;
    /* 0x78 */ s16 page;
    /* 0x7A */ s16 pageCount;
    /* 0x7C */ s16 cursor;
    /* 0x7E */ s16 count;
    /* 0x80 */ s16 unk80;
    /* 0x82 */ s8 state;
    /* 0x83 */ s8 step;
} ListMenu;
extern ListMenu gFriendListMenu;
void FriendList_BuildPage(ListMenu *m, s16 page);
void FriendList_Draw(u8 *o);
void FriendList_Draw2(u8 *o);
void FriendList_Open(s32 arg)
{
    memset(&gFriendListMenu, 0, sizeof(ListMenu));
    FriendList_BuildPage(&gFriendListMenu, 0);
    gFriendListMenu.cursor = 0;
    gFriendListOpen = 1;
    gFriendListMenu.draw = FriendList_Draw;
    gFriendListMenu.unk74 = arg;
    gFriendListMenu.draw2 = FriendList_Draw2;
    gFriendListMenu.unk20 = 0;
    gFriendListMenu.unk40 = 1;
    WinSlide_Step(&gFriendListMenu.win, 0x280, 0xE6);
}

/* unk_6E3BF0's .data (0x7E93E0-0x7E9410) */
Menu38Item gSongMenuItems[6] = {
#include "lobby/007E93E0.inc"
};
