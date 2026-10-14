/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "item.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "lobby_menu.h"
#pragma optimization_level 4
void CamFocus_SetAside(s8 arg0);

/* The item list menu of this file (work area gMenuWork, ids 0x48, 0x49). */
typedef struct ListMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u8 unk8[0x62];
    /* 0x6A */ u16 items[7];
    /* 0x78 */ u16 unk78;
    /* 0x7A */ u16 unk7A;
    /* 0x7C */ u8 unk7C[8];
    /* 0x84 */ s16 unk84;
    /* 0x86 */ s16 cursor;
    /* 0x88 */ s16 count;
    /* 0x8A */ s16 page;
    /* 0x8C */ s16 pages;
    /* 0x8E */ u8 unk8E[4];
    /* 0x92 */ s8 unk92;
    /* 0x93 */ s8 state;
} ListMenu;

extern u8 gMenuWork[];

void CamFocus_End(void);
void LobbySe_Play(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void CamReq_Cancel(void);
struct Em *Npc_Find(u8 id);
void Npc_SetKitchenRoute(struct Em *obj, s32 kind); /* unit's own declaration: with the definition's void (Em *obj, u8 kind) (lobby/unk_780580.c), KitchenGuest_OnRoute 1 of 16 words differ */
extern LobbyMenu *gLobbyMenu;
void KitchenGuest_Draw(void *task);

void IngredientList_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
}

u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Talk_Update(void);
void Talk_Say(s32 a, s8 b);
s32 WinSlide_Step(WinSlide *win, s16 from, s16 to);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void IngredientList_BuildPage(ListMenu *m, s16 page);
void IngredientList_Close(void);

void IngredientList_Update(void)
{
    ListMenu *m = (ListMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->state) {
    case 0:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state++;
            Talk_Say(10, 0);
            break;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            m->state++;
            WinSlide_Step(&m->win, 0x280, 0x118);
            IngredientList_BuildPage(m, 0);
            CamFocus_SetAside(1);
            break;
        case 3:
            m->state = 5;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 2:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 3:
        if (pad & 0x20) { /* compiled-out check */
        } else if (pad & 0x40) {
            m->state++;
            WinSlide_Step(&m->win, 0x118, 0x280);
            CamFocus_SetAside(0);
            LobbySe_Play(4);
        } else if (pad & 0x200) {
            if (m->count != 0) {
                m->unk92 ^= 1;
                LobbySe_Play(0x39);
            }
        } else if (pad & 0xC00) {
            if (m->pages > 1) {
                m->page = Menu_WrapLeftRight(m->page, m->pages);
                IngredientList_BuildPage(m, m->page);
            }
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, m->count);
        }
        break;
    case 4:
        switch (WinSlide_Step(&m->win, 0x118, 0x280)) {
        case 0:
            m->state++;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 5:
        switch (Talk_Update()) {
        case 0:
        case 3:
            IngredientList_Close();
            break;
        }
        break;
    }
}

typedef struct ItemRec {
    u16 item;
    u8 unk2[6];
} ItemRec;
extern ItemRec *gKitchenIngredients[];
s32 Items_HaveEnough(u16 item, s32 n);
s32 Menu_CountPages(s16 n, s16 per);

void IngredientList_BuildPage(ListMenu *m, s16 page)
{
    s16 first;
    s16 last;
    s32 i;
    ItemRec *p;
    s16 n = 0;
    s32 j;

    m->count = 0;
    first = page * 7;
    last = first + 6;
    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (p = gKitchenIngredients[i], j = 0; p[j].item != 0; j++) {
            if (Items_HaveEnough(p[j].item, 1) == 1) {
                if (n >= first && n <= last) {
                    m->items[m->count] = p[j].item;
                    m->count++;
                }
                n++;
            }
        }
    }
    m->page = page;
    m->pages = Menu_CountPages(n, 7);
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), Kitchen_DrawIngredients 6 of 232 words differ */
void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 n, u32 color);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Font_SetColor(s32 color);
extern char *gIngredientListTexts[];
extern char *D_0080C1F4;

void Kitchen_DrawIngredients(ListMenu *m, s16 x, s16 y)
{
    s16 cy = y;
    s16 i;
    u16 item;
    u16 sel;

    Win_Draw(x, y, 0x14A, 0x126, 0xBDB59F6C, 0xBB402010, 8);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xC8, 0xFFB0B000, gIngredientListTexts[0], 2, 1);
    if (m->count == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(Text_CenterX(D_0080C1F4, x + 0xA5, 0x12), cy + 0x64);
        Font_SetColor(0x802B7F7F);
        Text_PrintfColored(D_0080C1F4);
        Gfx_DrawLine(x + 0x14, (s16)(cy + 0xBE) + 2, 0x122, 2, 0xBDB59F6C);
        return;
    }
    Menu_DrawPageArrows(x + 0xF0, cy + 0xA, m->page, m->pages, 0xBDB59F6C);
    Font_SetColor(0x807F7F7F);
    cy += 0x1E;
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < m->count; i++) {
        item = m->items[i];
        if (gTownMode == 0x48) {
            switch (m->state) {
            case 5:
            case 6:
                if (m->unk78 == m->items[i] || m->unk7A == item) {
                    Menu_DrawLine(x + 0xA, cy - 1, 0x136, 0x18, 1);
                }
                break;
            }
        }
        if (i == m->cursor) {
            sel = item;
            Menu_DrawLine(x + 0xA, cy - 1, 0x136, 0x18, 0);
        }
        Menu_DrawItemCountRow(x + 0x32, cy, item, Items_HaveEnough(item, m->unk84) != 0 ? 0x807F7F7F : 0x80404040);
        cy += 0x16;
    }
    cy = y + 0xBE;
    Gfx_DrawLine(x + 0x14, cy + 2, 0x122, 2, 0xBDB59F6C);
    ItemInfo_Draw((s16)(x + 0x32) - 0x32, cy + 4, 0x14A, m->unk92, sel, 0, -1, 0x807F7F7F);
}

void Text_SetSize(s32 a);
void Kitchen_DrawIngredients(ListMenu *m, s16 x, s16 y);
void IngredientList_Draw(void *task)
{
    ListMenu *m = (ListMenu *)gMenuWork;

    switch (m->state) {
    case 1:
    case 5:
    case 0:
        return;
    default:
        Text_SetSize(*(s32 *)((u8 *)task + 0x18));
        Kitchen_DrawIngredients(m, m->win.pos, 0x3A);
        break;
    }
}

extern char *gHostChoosingFoodTexts[];
extern char *D_0080C20C;
extern WinSlide gKitchenGuestSlide; /* the guest confirm window: pos is its y */
extern u16 D_0084A326;
extern u16 D_0084A328;
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
void Font_SetPos(s16, s16);
void Text_Printf(const char *fmt, ...);
void Kitchen_DrawConfirm(s16 x, s16 y, u16 a, u16 b);

void KitchenGuest_Draw(void *task)
{
    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    switch (gLobbyMenu->subStep) {
    case 0:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gHostChoosingFoodTexts[0]);
        Font_SetPos(0x1E8, 0x190);
        Text_Printf(D_0080C20C, *(s16 *)((u8 *)gLobbyMenu + 0x24) / 30 + 1);
        break;
    case 1:
    case 2:
    case 3:
        Kitchen_DrawConfirm(0x122, gKitchenGuestSlide.pos, D_0084A326, D_0084A328);
        break;
    case 4:
        break;
    }
}

void KitchenGuest_Open(void) {
    gTownMode = 0x49;
    gTownMenuOpen = 1;
    LobbyMenu_SetDraw(&KitchenGuest_Draw, 0, 0, 0);
    LobbyMenu_Init();
    gLobbyMenu->timer = 0x708;
}

void KitchenGuest_Close(void) {
    gTownMode = 0;
    gTownMenuOpen = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
}

void CamReq_Kitchen(s32 a);
void Tutorial_Start(s8 a, s32 b, s32 c);
s32 Tutorial_IsDone(void);
void Actor_SetState(void *, s32, s32);
void Kitchen_EatCourse(u16 a, u16 b);
s16 Kitchen_EatDish(s16 a, u16 b, u16 c, u16 d);
extern s32 gKitchenGuestMeal;
extern s16 D_0084A324;
extern u16 D_0084A32A;
extern s8 D_0084A32C;

void KitchenGuest_Update(void)
{
    u8 *npc;

    switch (gLobbyMenu->subStep) {
    case 0:
        if (--gLobbyMenu->timer <= 0) {
            KitchenGuest_Close();
        }
        break;
    case 1:
        if (gLobbyMenu->timer != 0) {
            gLobbyMenu->timer--;
        }
        switch (WinSlide_Step(&gKitchenGuestSlide, -0x118, 0x3C)) {
        case 0:
            gLobbyMenu->subStep++;
            gLobbyMenu->slideTimer = 0x1E;
            CamReq_Kitchen(D_0084A32C);
            if (D_0084A326 == 0) {
                Tutorial_Start(2, gKitchenGuestMeal, 4);
            } else {
                Tutorial_Start(2, gKitchenGuestMeal, (gItemData[D_0084A326].rarity + gItemData[D_0084A328].rarity + 2) / 2 - 1);
            }
            break;
        }
        break;
    case 2:
        if (gLobbyMenu->timer != 0) {
            gLobbyMenu->timer--;
        }
        if (--gLobbyMenu->slideTimer <= 0) {
            gLobbyMenu->subStep++;
        }
        break;
    case 3:
        if (gLobbyMenu->timer != 0) {
            gLobbyMenu->timer--;
        }
        switch (WinSlide_Step(&gKitchenGuestSlide, 0x3C, -0x118)) {
        case 0:
            gLobbyMenu->subStep++;
            break;
        }
        break;
    case 4:
        if (Tutorial_IsDone() == 1) {
            gLobbyMenu->subStep++;
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x29);
        }
        break;
    case 5:
        if (gPlayers[gScene.localPlayer].motionId[0] == 0x261) {
            gLobbyMenu->subStep++;
            if (D_0084A326 == 0) {
                Kitchen_EatCourse(D_0084A328, D_0084A32A);
            } else {
                Kitchen_EatDish(D_0084A324, D_0084A326, D_0084A328, D_0084A32A);
            }
            npc = (u8 *)Npc_Find(0x71);
            if (npc != NULL) {
                Actor_SetState(npc, 0, 0x55);
            }
        }
        break;
    case 6:
        if (gPlayers[gScene.localPlayer].state == 1) {
            gLobbyMenu->timer = 0;
            gLobbyMenu->subStep = 0;
            KitchenGuest_Close();
        }
        break;
    }
}

void KitchenGuest_OnRoute(s32 kind) {
    struct Em *obj;

    obj = Npc_Find(0x71);
    if (obj != NULL) {
        Npc_SetKitchenRoute(obj, kind);
    }
}
