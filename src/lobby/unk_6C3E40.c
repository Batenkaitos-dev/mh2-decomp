#include "common.h"
#include "em.h"
#include "lobby_globals.h"
#include "scene.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_npc.h"

/* The talk scene's menu (work area gMenuWork, id 0x2C). */
typedef struct TalkMenu {
    /* 0x00 */ Em *npc;
    /* 0x04 */ WinSlide win;
    /* 0x0C */ u16 items[7];        /* the page shown (BaitMenu_LoadPage) */
    /* 0x1A */ s8 state;
    /* 0x1B */ s8 unk1B;
    /* 0x1C */ s8 unk1C;
    /* 0x1D */ u8 unk1D;
    /* 0x1E */ s16 page;
    /* 0x20 */ s16 pages;
    /* 0x22 */ s16 count;
    /* 0x24 */ s16 cursor;
    /* 0x26 */ u16 unk26;
    /* 0x28 */ s32 unk28;            /* an item */
    /* 0x2C */ s32 unk2C;
} TalkMenu;

/* prototypes */
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Win_Draw(s16, s16, s32, s32, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), BaitMenu_DrawTitle 27 of 30 words differ */
void Font_SetSpacing(u8 a, u8 b);
extern s32 D_0080C17C;
/* end of prototypes */

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gMenuWork[];
extern s8 gMenuInputLock;
extern void *gNpcKindSe[];
extern u8 gBaitGiveTable[];
extern u8 D_007E794C[];
s32 Game_IsOnline(void);
Em *Npc_Find(u8 kind);
void Player_SetFlag(void *obj, u32 flag);
void Npc_PlaySeDelayed(Em *npc, void *p);
void *memset(void *dst, s32 c, u32 n);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void BaitMenu_LoadPage(TalkMenu *m, s16 page);
void BaitMenu_Draw(void *task);
void Talk_Start(Em *npc, s8 quiet);
void Talk_Say(s32 a, s8 b);
void Npc_PickIdleState(Em *npc);
void Npc_HoldRoute(Em *npc);
void Npc_EnterRoutePoint(Em *npc);
void CamFocus_Start(Em *npc);
void CamFocus_End(void);
void LobbySe_Play(s32 se);
s32 Actor_HeadingDiff(void *obj, void *pos);
void Text_SetSize(s32 a);
void Font_SetColor(s32 color);
void TalkMenu_DrawItems(s16 x, s16 y);
void TalkMenu_DrawItemTotal(void);

void BaitMenu_Open(void)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;
    Em *npc;

    if (Game_IsOnline() == 1) {
        npc = Npc_Find(0x33);
        Player_SetFlag(npc, 0x20000);
    } else {
        npc = Npc_Find(0x19);
    }
    if (npc != NULL) {
        if (Game_IsOnline() == 0) {
            Npc_PlaySeDelayed(npc, gNpcKindSe[npc->work]);
        }
        gTownMenuOpen = 1;
        gTownMode = 0x2C;
        gMenuInputLock = 6;
        memset(gMenuWork, 0, 0xF00);
        LobbyMenu_Init();
        LobbyMenu_SetDraw(BaitMenu_Draw, 0, 0, 0);
        BaitMenu_LoadPage(m, 0);
        m->npc = npc;
        Talk_Start(m->npc, 0);
        Talk_Say(1, 0);
        Npc_PickIdleState(m->npc);
        Npc_HoldRoute(m->npc);
        CamFocus_Start(m->npc);
    }
}

s32 Item_IsBait(u16 item, s32 unused1);
s32 Items_HaveEnough(u16 item, s32 n);
s32 Menu_CountPages(s16 n, s16 per);
void BaitMenu_LoadPage(TalkMenu *m, s16 page)
{
    s16 first;
    s16 last;
    s16 n = 0;
    s16 i;

    m->count = 0;
    first = page * 7;
    last = first + 6;
    memset(m->items, 0, sizeof(m->items));
    m->count = 0;
    for (i = 0; i < 0x399; i++) {
        if (Item_IsBait(i, 0) == 1 && i != 0x91 && Items_HaveEnough(i, 1) == 1) {
            if (n >= first && n <= last) {
                m->items[m->count] = i;
                m->count++;
            }
            n++;
        }
    }
    m->page = page;
    m->pages = Menu_CountPages(n, 7);
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
}

void BaitMenu_Close(void)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;

    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    ((NpcWork *)&m->npc->work)->routeIdx = 0;
    Npc_EnterRoutePoint(m->npc);
    LobbySe_Play(4);
}

void Chat_SetWindowMode(s8 a, s8 b);
s32 Talk_Update(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void CamFocus_SetAside(s8 a);
s32 BaitMenu_ListInput(u16 pad);
s32 BaitMenu_GiveInput(u16 unused);
s32 Menu_PushNoticeOnce(s32);
void BaitMenu_StartGiving(void);
void BaitMenu_Update(void)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = pl->input.pressed;

    Chat_SetWindowMode(1, 2);
    switch (m->state) {
    case 0:
        switch (Talk_Update()) {
        case 0:
            if (m->count != 0) {
                m->state++;
                gMenuInputLock = 6;
                CamFocus_SetAside(1);
                WinSlide_Step(&m->win, 0x280, 0x118);
            } else {
                m->state = 4;
                Talk_Say(2, 0);
            }
            break;
        case 3:
            m->state = 4;
            Talk_Say(0, 0);
            break;
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
        switch (BaitMenu_ListInput(pad)) {
        case 0:
            m->state++;
            BaitMenu_StartGiving();
            Menu_PushNoticeOnce(0x18);
            m->unk26 = m->npc->rotY;
            break;
        case 3:
            CamFocus_SetAside(0);
            Talk_Say(0, 0);
            m->state = 4;
            break;
        }
        break;
    case 3:
        switch (BaitMenu_GiveInput(pad)) {
        case 0:
            m->state = 4;
            Talk_Say(3, 0);
            CamFocus_Start(m->npc);
            Npc_PickIdleState(m->npc);
            break;
        case 3:
            BaitMenu_LoadPage(m, 0);
            m->state = 1;
            gMenuInputLock = 6;
            WinSlide_Step(&m->win, 0x280, 0x118);
            CamFocus_Start(m->npc);
            CamFocus_SetAside(1);
            Npc_PickIdleState(m->npc);
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
        case 3:
            BaitMenu_Close();
            break;
        }
        break;
    }
}

void BaitMenu_StartGiving(void)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 *tbl;

    m->unk1B = 0;
    if (Game_IsOnline() == 1) {
        tbl = D_007E794C;
    } else {
        tbl = gBaitGiveTable;
    }
    pl->targetYaw += (u16)Actor_HeadingDiff(pl, tbl);
    pl->useItem = m->unk28;
    CamFocus_End();
}

s32 Items_Total(u16 item);
void Actor_SetStateEx(struct Player *obj, u8 state, u8 sub, u16 flags);
void Npc_StartTurnTo(Em *npc, s32 a, s32 b);
void Actor_SetState(void *, s32, s32);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
s32 BaitMenu_GiveInput(u16 unused)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = pl->input.pressed;

    switch (m->unk1B) {
    case 0:
        if (Items_HaveEnough(m->unk28, 1) == 0) {
            BaitMenu_LoadPage(m, 0);
            if (m->count == 0) {
                return 0;
            }
            Menu_PushNoticeOnce(0x19);
            return 3;
        }
        if (pad & 0x20) {
            LobbySe_Play(8);
            Actor_SetStateEx(pl, 0, 0x62, 0);
            m->unk1B++;
            if (Game_IsOnline() == 0) {
                Npc_PickIdleState(m->npc);
                Npc_StartTurnTo(m->npc, 10, 0x3BBC);
            } else {
                Npc_StartTurnTo(m->npc, 10, 0xF6C1);
                Actor_SetState(m->npc, 0, 0x9E);
                Player_SetFlag(m->npc, 0x20000);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        }
        break;
    case 1:
        if (pl->state == 1) {
            if (Items_HaveEnough(m->unk28, 1) == 1) {
                m->unk1B = 0;
                m->unk2C = Items_Total(m->unk28);
                Npc_PickIdleState(m->npc);
            } else {
                BaitMenu_LoadPage(m, 0);
                if (m->count == 0) {
                    return 0;
                }
                Menu_PushNoticeOnce(0x19);
                return 3;
            }
        }
        if (pl->motionId[0] == 0x328) {
            if (Game_IsOnline() == 0) {
                if (m->npc->state != 0x7C && Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                    Actor_SetState(m->npc, 0, 0x7C);
                    Npc_StartTurnTo(m->npc, 10, m->unk26);
                }
            } else {
                if (m->npc->state != 0x55 && Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                    Actor_SetState(m->npc, 0, 0x55);
                    Player_SetFlag(m->npc, 0x20000);
                }
            }
        }
        break;
    }
    return 2;
}

s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 BaitMenu_ListInput(u16 pad)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;

    if (pad & 0x20) {
        m->unk28 = m->items[m->cursor];
        m->unk2C = Items_Total(m->unk28);
        m->cursor = 0;
        m->page = 0;
        LobbySe_Play(8);
        return 0;
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        m->unk1C ^= 1;
        LobbySe_Play(0x39);
    } else if (pad & 0xC00) {
        if (m->pages > 1) {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
            BaitMenu_LoadPage(m, m->page);
            LobbySe_Play(1);
        }
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
    }
    return 2;
}

void BaitMenu_DrawTitle(s16 x, s16 y)
{
    Win_Draw(x, y, 0x154, 0x120, 0xBDB59F6C, 0xBB402010, 8);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xC8, -0x4F5000, (char *) D_0080C17C, 2, 1);
}

extern char *D_0080C1E4;
s32 Hud_ItemGetBusy(void);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
void TalkMenu_DrawItems(s16 x, s16 y)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;
    s32 px;
    s32 item;
    s32 sel;
    s16 i;
    TalkMenu *p;
    s32 bx;
    s32 cx;
    if (Hud_ItemGetBusy() == 1) {
        return;
    }
    BaitMenu_DrawTitle(x, y);
    Font_SetColor(0x807F7F7F);
    y += 0x1E;
    Font_SetSpacing(0x14, 0x14);
    px = x;
    bx = px + 10;
    cx = px + 0x3C;
    for (i = 0, p = m; i < 7; i++) {
        item = p->items[0];
        if (i == m->cursor) {
            sel = item;
            Menu_DrawLine(bx, y - 1, 0x140, 0x18, 0);
        }
        Menu_DrawItemCountRow(cx, y, item, 0x807F7F7F);
        y += 0x16;
        p = (TalkMenu *)((u8 *)p + 2);
    }
    Gfx_DrawLine(px + 0x14, y + 2, 0x12C, 2, 0xBDB59F6C);
    ItemInfo_Draw((s16)(px + 0x3C) - 0x3C, y + 4, 0x154, m->unk1C, sel, 0, -1, 0x807F7F7F);
    Win_DrawText(0x118, 0x15E, 0x154, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C1E4);
}

s32 sprintf(char *buf, const char *fmt, ...);
extern char *gItemNames[];
extern char *gFishingBaitTexts[];
extern char *D_0080C184;
extern char D_00826040[];
s32 Lobby_StrLength(char *);
void Gfx_DrawPulseGradient(s16 x, s16 y, s16 w, s16 h, u32 color, u8 dir);
s16 Lobby_DrawSprite155(s16, s16, s32);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);

void TalkMenu_DrawItemTotal(void)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;
    char buf[0x40];

    if (Hud_ItemGetBusy() == 1) {
        return;
    }
    Font_SetColor(0x807F7F7F);
    sprintf(buf, gFishingBaitTexts[0], gItemNames[m->unk28], Items_Total(m->unk28));
    Gfx_DrawPulseGradient(0, 0x6F, Lobby_StrLength(buf) * 9 + 0x50, 0x1E, 0x80006000, 0);
    Gfx_DrawPulseGradient(0, 0x8F, 0xC8, 0x1E, 0x80006000, 0);
    Lobby_DrawSprite155(0x14, 0x72, 0);
    Lobby_DrawSprite155(0x14, 0x92, 1);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(0x38, 0x73);
    Text_PrintfColored(buf);
    Font_SetPos(0x38, 0x93);
    Text_PrintfColored(D_00826040, D_0080C184);
}

void BaitMenu_Draw(void *task)
{
    TalkMenu *m = (TalkMenu *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    Font_SetColor(0x80101010);
    switch (m->state) {
    case 1:
        BaitMenu_DrawTitle(m->win.pos, 0x34);
        break;
    case 2:
        TalkMenu_DrawItems(m->win.pos, 0x34);
        break;
    case 3:
        if (m->unk1B == 0) {
            TalkMenu_DrawItemTotal();
        }
        break;
    }
}

/* Menu 9 (gMenuWork), opened at an NPC; its draws are in unk_6C4E40.c. */
typedef struct Menu9 {
    /* 0x0 */ u8 unk0[2];
    /* 0x2 */ s8 unk2;
    /* 0x3 */ s8 unk3;              /* 1 for NPC kinds 0x58 and 0xF */
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 count;
} Menu9;
struct OfferTask;
s16 OfferMenu_BuildList(struct OfferTask *t, s32 b);
void Workshop_Draw(void *);
void Workshop_Draw2(void *);
void Workshop_Open(u8 *npc)
{
    Menu9 *m = (Menu9 *)gMenuWork;
    NpcWorkshopWork *w = (NpcWorkshopWork *)(npc + 0x438);

    gTownMenuOpen = 1;
    gTownMode = 9;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Workshop_Draw, Workshop_Draw2, 0, 0);
    m->unk4 = w->menuSave;
    if (OfferMenu_BuildList((struct OfferTask *) npc, 0) != 0) {
        m->unk2 = 1;
        m->count = 4;
    } else {
        m->count = 3;
    }
    if (w->id == 0x58 || w->id == 0xF) {
        m->unk3 = 1;
    } else {
        m->unk3 = 0;
    }
    if (m->unk3 != 0) {
        m->count--;
    }
    gMenuInputLock = 6;
    LobbySe_Play(0x1B);
}

/* unk_6C3E40's .data (0x7E7940-0x7E7960) */
u8 gBaitGiveTable[32] = {
#include "lobby/007E7940.inc"
};

/* unk_6C3E40's .rodata (0x826040-0x826050) */
#pragma rodata on
char D_00826040[0x10] = {
#include "lobby/00826040.inc"
};
#pragma rodata off
