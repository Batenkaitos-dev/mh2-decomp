#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "pad.h"
#include "player.h"
#include "lobby_menu.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void HouseAccess_Close(void)
{
    gTownMode = 0;
    gTownMenuOpen = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
}

typedef struct MemberPick {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ char pass[9];
    /* 0x11 */ s8 step;
    /* 0x12 */ u8 unk12[2];
    /* 0x14 */ s16 cursor;
} MemberPick;
extern u8 gMenuWork;
extern NetUser gNetUsers[];
extern char gMyUserId[];
extern s16 gHouseCloseTimer;
extern LobbyMenu *gLobbyMenu;
s8 MyHouse_LocalSlot(void);
u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
s32 memcmp(void *a, void *b, s32 n);
void LobbySe_Play(s32);
void TownMsg_Send14(void *u);
void AreaMenu_Open(u16 id);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
void HouseAccess_Close(void);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
void Kbd_Cancel(void);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void LobbyText_Open(s32);
void LobbyDialog_Grow(void);
void *memset(void *dst, s32 c, u32 n);

static inline NetUser *Member_Get(s8 i)
{
    return &gNetUsers[i];
}

/* Case 2 reads the member again into its own locals (text, k): the game
 * keeps them in s1/s0 apart from case 1's t and i. */
#pragma optimization_level 4
void HouseAccess_Update(void)
{
    HunterCard *text;
    MemberPick *m = (MemberPick *)&gMenuWork;
    s32 k;
    u16 pad;
    s8 i;
    HunterCard *t;
    NetUser *u;

    MyHouse_LocalSlot();
    pad = LobbyPad_Get(0);
    Chat_SetWindowMode(1, 2);
    switch (m->step) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x19A)) {
        case 0:
            m->step++;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (gNetUsers[m->cursor].id[0] == 0 ||
                memcmp(gNetUsers[m->cursor].id, gMyUserId, 6) == 0) {
                LobbySe_Play(9);
                break;
            }
            if (gPlayers[m->cursor].area == 0) {
                LobbySe_Play(9);
                break;
            }
            i = m->cursor;
            u = Member_Get(i);
            t = (HunterCard *)u->text;
            if (((HunterCard *)u->text)->houseAccess == 1) {
                TownMsg_Send14(u);
                gLobby->memberSlot[gScene.localPlayer] = i + 0x30;
                LobbyMenu_SetDraw(0, 0, 0, 0);
                AreaMenu_Open(t->houseLevel + 0xAD);
                gHouseCloseTimer = -1;
                LobbySe_Play(0);
            } else if (((HunterCard *)u->text)->houseAccess == 2) {
                m->step = 2;
                gFieldControl = 0;
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 6, 8, m->pass);
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            m->cursor = 0;
            m->step = 0;
            HouseAccess_Close();
            LobbySe_Play(4);
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, 10);
        }
        break;
    case 2:
        if (((HunterCard *)Member_Get(m->cursor)->text)->houseAccess != 2 || gNetUsers[m->cursor].id[0] == 0 ||
            memcmp(gNetUsers[m->cursor].id, gMyUserId, 6) == 0) {
            m->step = 1;
            LobbySe_Play(4);
            Kbd_Cancel();
            gFieldControl = 1;
            break;
        }
        if (gPlayers[m->cursor].area == 0) {
            m->step = 1;
            LobbySe_Play(4);
            Kbd_Cancel();
            gFieldControl = 1;
            break;
        }
        if (Kbd_Run(m->pass, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            Kbd_Cancel();
            gFieldControl = 1;
            if (m->pass[0] != 0) {
                m->step = 1;
                text = (HunterCard *)Member_Get(m->cursor)->text;
                k = (s8)m->cursor;
                if (memcmp(text->housePass, m->pass, 9) == 0) {
                    TownMsg_Send14(&gNetUsers[k]);
                    gLobby->memberSlot[gScene.localPlayer] = k + 0x30;
                    LobbyMenu_SetDraw(0, 0, 0, 0);
                    AreaMenu_Open(text->houseLevel + 0xAD);
                    gHouseCloseTimer = -1;
                } else {
                    m->step = 3;
                    LobbyText_Open(0x60);
                    LobbyDialog_Grow();
                    gLobbyMenu->redraw = 1;
                }
            } else {
                m->step = 3;
                LobbyText_Open(0x60);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
            }
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->step = 1;
            LobbySe_Play(8);
            memset(m->pass, 0, 9);
        }
        break;
    }
}
#pragma optimization_level reset

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), HouseAccess_DrawList 225 of 247 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
extern u8 gMenuWork;
extern NetUser gNetUsers[];
extern char *D_0080C1C4;
extern char *gCloseLabelTexts[];
extern char *D_0080C1CC;
extern char *gGoneOutTexts[];
extern char *D_0080C1D4;

/* The member list draw (the 10 NetUsers at gNetUsers and their state
 * icons); x + 10 is a local (lx) for the three lines. */
void HouseAccess_DrawList(s16 x, s16 y)
{
    HunterCard *t;
    NetUser *u;
    s16 ty = y;
    u8 *m = (u8 *)&gMenuWork;
    s32 lx;
    s8 i;

    Win_Draw(x, y, 0xD2, 0x104, 0xBDB59F6C, -1, 2);
    lx = x + 10;
    Gfx_DrawLine(lx, ty + 10, 0xBE, 0x14, 0x80B59F6C);
    ty += 10;
    Font_SetPos((s16)(x + 0x1E) - 0x14, ty);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(D_0080C1D4);
    if (*(s8 *)(m + 0x11) < 2) {
        Menu_DrawLine(lx, ty + 0x14 + *(s16 *)(m + 0x14) * 0x16, 0xBE, 0x16, 0);
    } else {
        Menu_DrawLine(lx, ty + 0x14 + *(s16 *)(m + 0x14) * 0x16, 0xBE, 0x16, 1);
    }
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < 10; i++) {
        ty += 0x16;
        u = &gNetUsers[i];
        t = (HunterCard *)u->text;
        Font_SetPos((s16)(x + 0x1E) + 0x5A, ty);
        if (u->id[0] != 0) {
            if (gPlayers[i].area == 0) {
                Font_SetColor(0x80404040);
                Text_PrintfColored(gGoneOutTexts[0]);
                Icon_Draw((s16)(x + 0x1E) - 0x14, ty, 0x14, 0x14, -1, 0x47);
            } else {
                switch (t->houseAccess) {
                case 0:
                    Font_SetColor(0x80404040);
                    Text_PrintfColored(gCloseLabelTexts[0]);
                    Icon_Draw((s16)(x + 0x1E) - 0x14, ty, 0x14, 0x14, -1, 0x47);
                    break;
                case 1:
                    Font_SetColor(0x807F7F7F);
                    Text_PrintfColored(D_0080C1C4);
                    break;
                case 2:
                    Font_SetColor(0x807F7F7F);
                    Text_PrintfColored(D_0080C1CC);
                    Icon_Draw((s16)(x + 0x1E) - 0x14, ty, 0x14, 0x14, -1, 0x59);
                    break;
                }
            }
        }
        Gfx_DrawLine(x + 10, ty - 2, 0xBE, 2, 0xBDB59F6C);
        if (u->id[0] != 0) {
            Font_SetPos(x + 0x1E, ty);
            if (u->id[0] != 0) {
                if (gChatHideNames == 0) {
                    Text_PrintfColored(u->name);
                } else {
                    Text_PrintfColored((char *)u);
                }
            } else {
                Text_PrintfColored("--------");
            }
        }
    }
}
