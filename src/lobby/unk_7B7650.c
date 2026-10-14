#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "browser.h"

extern char gCurServerId[];                 /* the current server id (sub_main) */
s32 memcmp(void *a, void *b, s32 n);
/* the number of servers (sub_main 0x5C34F8, after the list) */
u16 Pad_GetCursorButtons(s32 pad);
void Stream_FadeOut(s32, s32);
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDownPad(s32 cur, s32 n, u16 pad);
s32 Menu_WrapLeftRightPad(s32 cur, s32 n, u16 pad);
void LobbyText_Open(s32 msg);
extern s8 gSrvMenuChoice;

void LobbyTitle_Draw(void);
void LobbyMsg_Draw(void);
void Font_SetSpacing(u8 a, u8 b);
void Text_SetSize(s32);
void SrvMenu_DrawList(s16 x, s16 y);
extern LobbyMenu *gLobbyMenu;
void LobbyLoc_DrawPanel(s32, s32); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it); (s16 x, s16 y) here changes SrvMenu_DrawInfo by 9 of 27 words */
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void SrvMenu_DrawInfo(MenuTask *task);
void SrvMenu_DrawWindow(MenuTask *task);
s32 LobbyPad_Pressed(s32 button);
void Menu_SetYesNo(u8 choice); /* unit's own declaration: with the definition's void (s8 choice) (lobby/unk_7B0D80.c), SrvMenu_Quit 2 of 157 words differ */
s32 LobbyMenu_MoveCursor(u8 *, u8, u8);
void Bgm_PlayWithVolume(s32, s32);
extern M2C_UNK LobbyDialog_DrawPending;
extern M2C_UNK Lobby_DrawBlackLine;

void SrvMenu_SlideIn(void)
{
    s16 temp_v1;
    u8 temp_a0;

    if (gLobbyMenu->promptStep == 0) {
        temp_a0 = gLobbyMenu->step;
        switch (temp_a0) {                          /* irregular */
        case 0:
            temp_v1 = gLobbyMenu->slideTimer - 1;
            gLobbyMenu->slideTimer = temp_v1;
            if (((s16)(temp_v1)) < 0) {
                gLobbyMenu->step = (u8) (gLobbyMenu->step + 1);
            }
            return;
        case 1:
            gLobbyMenu->slideTimer = 0;
            gLobbyMenu->screen = (u8) (gLobbyMenu->screen + 1);
            gLobbyMenu->step = 0U;
            Bgm_PlayWithVolume(0x123, 0x1E);
            return;
        }
    } else {
        gLobbyMenu->screen = 3U;
        gLobbyMenu->step = 0U;
        gLobbyMenu->promptStep = 0U;
        LobbyMenu_SetDraw(0, &Lobby_DrawBlackLine, &LobbyDialog_DrawPending, 0);
    }
}

/* the lobby server page (page of pages, cursor): circle takes the server
 * under the cursor when its +0x108 word is set, cross goes back (screen 4),
 * left and right turn the page, up and down move the cursor; then a
 * countdown and the choice (gSrvMenuChoice = the server's index, -2 back) */
void SrvMenu_Choose(void)
{
    u16 pad;
    u8 *st;
    u8 s;
    s32 page;
    s32 n;

    pad = Pad_GetCursorButtons(0);
    st = &gLobbyMenu->step;
    s = *st;
    switch (s) {
    case 0:
        if (pad & 0x20) {
            if (gLobbyServers.row[(u8)(gLobbyMenu->cursor + gLobbyMenu->page * 8)].capacity != 0) {
                *st = s + 1;
                LobbySe_Play(0);
                gLobbyMenu->timer = 0x28;
                Stream_FadeOut(0, 0xF);
                LobbyText_Open(0);
                gLobbyMenu->redraw = 1;
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            gLobbyMenu->screen = 4;
            gLobbyMenu->step = 0;
            LobbySe_Play(4);
        } else if (pad & 0xC00) {
            page = gLobbyMenu->page;
            gLobbyMenu->page = Menu_WrapLeftRightPad(page, gLobbyMenu->pages, pad);
            if (page != gLobbyMenu->page) {
                gLobbyMenu->cursor = 0;
            }
        } else {
            if (gLobbyMenu->page < gLobbyMenu->pages - 1) {
                n = 8;
            } else {
                n = gLobbyServers.count % 8;
            }
            gLobbyMenu->cursor = Menu_WrapUpDownPad(gLobbyMenu->cursor, n, pad);
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        gLobbyMenu->timer--;
        if (gLobbyMenu->timer < 0) {
            gSrvMenuChoice = gLobbyMenu->cursor + gLobbyMenu->page * 8;
        }
        break;
    case 2:
        gLobbyMenu->timer--;
        if (gLobbyMenu->timer < 0) {
            gSrvMenuChoice = -2;
        }
        break;
    }
}

/* the server page.s intro: message 0x76 for 0x260 frames (circle
 * skips it after the first 0x3C), then the server page (screen 2) with its
 * panels and music */
void SrvMenu_Intro(void)
{
    u8 *st;
    u8 s;

    st = &gLobbyMenu->step;
    s = *st;
    switch (s) {
    case 0:
        *st = s + 1;
        gLobbyMenu->timer = 0x260;
        LobbyText_Open(0x76);
        gLobbyMenu->redraw = 1;
        break;
    case 1:
        gLobbyMenu->timer--;
        gLobbyMenu->redraw = 1;
        if (gLobbyMenu->timer < 0x224 && LobbyPad_Pressed(0x20) != 0) {
            LobbySe_Play(0);
            gLobbyMenu->screen = 2;
            gLobbyMenu->step = 0;
            LobbyMenu_SetDraw(SrvMenu_DrawInfo, SrvMenu_DrawWindow, NULL, &LobbyDialog_DrawPending);
            Bgm_PlayWithVolume(0x123, 0x3C);
        } else if (gLobbyMenu->timer < 0) {
            gLobbyMenu->screen = 2;
            gLobbyMenu->step = 0;
            LobbyMenu_SetDraw(SrvMenu_DrawInfo, SrvMenu_DrawWindow, NULL, &LobbyDialog_DrawPending);
            Bgm_PlayWithVolume(0x123, 0x3C);
        }
        break;
    }
}

/* the quit question (message 0x51, yes/no at unk38, "no" first): yes fades
 * out and goes on to screen 2's step 2, no or cross back to screen 2 */
void SrvMenu_Quit(void)
{
    u8 *st;
    u8 s;
    u8 *c;

    st = &gLobbyMenu->step;
    s = *st;
    switch (s) {
    case 0:
        *st = s + 1;
        gLobbyMenu->timer = 8;
        LobbyText_Open(0x51);
        gLobbyMenu->quitYesNo = 1;
        Menu_SetYesNo(gLobbyMenu->quitYesNo);
        break;
    case 1:
        gLobbyMenu->timer--;
        gLobbyMenu->redraw = 1;
        if (gLobbyMenu->timer < 0) {
            gLobbyMenu->step++;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (LobbyPad_Pressed(0x20) != 0) {
            if (gLobbyMenu->quitYesNo == 0) {
                LobbySe_Play(0);
            } else {
                LobbySe_Play(4);
            }
            gLobbyMenu->step++;
            gLobbyMenu->timer = 8;
        } else if (LobbyPad_Pressed(0x40) != 0) {
            LobbySe_Play(4);
            c = &gLobbyMenu->quitYesNo;
            if (*c != 1) {
                *c = 1;
            } else {
                gLobbyMenu->screen = 2;
                gLobbyMenu->step = 0;
            }
        } else {
            LobbyMenu_MoveCursor(&gLobbyMenu->quitYesNo, 1, 3);
            Menu_SetYesNo(gLobbyMenu->quitYesNo);
        }
        break;
    case 3:
        gLobbyMenu->timer--;
        if (gLobbyMenu->timer < 0) {
            gLobbyMenu->step++;
        }
        break;
    case 4:
        gLobbyMenu->timer--;
        if (gLobbyMenu->timer < 0) {
            if (gLobbyMenu->quitYesNo == 0) {
                gLobbyMenu->screen = 2;
                gLobbyMenu->step = 2;
                gLobbyMenu->timer = 0x28;
                Stream_FadeOut(0, 0xF);
            } else {
                gLobbyMenu->screen = 2;
                gLobbyMenu->step = 0;
            }
        }
        break;
    }
}

void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Text_PrintfColored(char *fmt, ...);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 color);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
s32 LobbySrv_ValueAt(s32 i);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
extern char *gHeadcountTexts[];
extern char *D_0080C164;
extern char *gLandNameTexts[];

/* the lobby server list: its frame, the page, the column heads and 8 rows
 * (name, users/capacity) with the cursor.
 * x + 0x14 and its s16 + 0xB4 are written at each use: as locals they took
 * other saved registers (the CSE values MWCC creates are numbered last). */
void SrvMenu_DrawList(s16 x, s16 y)
{
    s32 i;
    s32 idx;
    s16 yy = y;
    s16 d;
    LbsRow *s;

    Icon_Draw(x, y, 0x10E, 0x130, -1, 0);
    yy += 0xE;
    Font_SetColor(0x80101010);
    Font_SetPos(x + 0x14, yy);
    Text_PrintfColored(D_0080C164);
    Menu_DrawPageNum((s16)(x + 0x14) + 0xB4, yy, gLobbyMenu->page, gLobbyMenu->pages, 1);
    LobbyUi_DrawIcon(x + 0x87, y + 0x98, 0x70, LobbySrv_ValueAt(gLobbyMenu->cursor + gLobbyMenu->page * 8) | 0xB0000000, 1);
    Font_SetColor(0xFF202060);
    Font_SetPos(x + 0x14, yy += 0x16);
    Text_PrintfColored(gLandNameTexts[0]);
    Font_SetPos((s16)(x + 0x14) + 0xB4, yy);
    Text_PrintfColored(gHeadcountTexts[0]);
    Menu_DrawDashLine(x + 0x14, yy + 0x10, 0xFA, 0xFF000000);
    yy += 4;
    Font_SetColor(0x80101010);
    x += 0x28;
    for (i = 0; i < 8; i++) {
        idx = i + gLobbyMenu->page * 8;
        if (idx >= 0x28 || idx >= gLobbyServers.count) {
            break;
        }
        yy += 0x16;
        if (i == gLobbyMenu->cursor) {
            d = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(x - 0xA, yy + 0xE, 0xE2, 8, 0x99106060, 0x6F);
            Menu_DrawCursorIcon(x - 0x1C - d, yy - 2, 0x18, 0xFF20C0C0);
        }
        Font_SetPos(x, yy);
        Font_SetColor(0x80101010);
        s = &gLobbyServers.row[idx];
        Text_PrintfColored("%s", s->text);
        Font_SetPos(x + 0x82, yy);
        Text_PrintfColored("%4d/%4d", s->users, s->capacity);
    }
}

void SrvMenu_DrawInfo(MenuTask *task) {
    s16 temp_a0;
    s32 x;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    temp_a0 = gLobbyMenu->slideTimer;
    if (temp_a0 < 0) {
        x = 0x1A;
    } else {
        x = (s16)((0x14 - (temp_a0 * 0x30)));
    }
    LobbyLoc_DrawPanel(x, 0x44);
}

void SrvMenu_DrawWindow(MenuTask *task) {
    s16 temp_v1;
    s16 x;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    temp_v1 = gLobbyMenu->slideTimer;
    if (temp_v1 < 0) {
        x = 0x2E;
    } else {
        x = 0x28 - (temp_v1 * 0x33);
    }
    SrvMenu_DrawList(x, 0x62);
    LobbyTitle_Draw();
    LobbyMsg_Draw();
}

/* the player's own entry in the lobby server list: its name */
char *LobbySrv_MyName(void)
{
    s32 i;
    LbsRow *f;

    for (i = 0, f = gLobbyServers.row; i < 40; i++, f++) {
        if (memcmp(gCurServerId, f, 0xC) == 0) {
            break;
        }
    }
    if (i <= 39) {
        return gLobbyServers.row[i].text;
    }
    return NULL;
}

/* the name of server id */
char *LobbySrv_Name(char *id)
{
    s32 i;
    LbsRow *f;

    for (i = 0, f = gLobbyServers.row; i < 40; i++, f++) {
        if (memcmp(id, f, 0xC) == 0) {
            break;
        }
    }
    if (i <= 39) {
        return gLobbyServers.row[i].text;
    }
    return NULL;
}

/* the index of the player entry in the lobby server list, 0 if none */
s32 LobbySrv_CurIndex(void)
{
    s32 i;
    LbsRow *f;

    for (i = 0, f = gLobbyServers.row; i < 40; i++, f++) {
        if (memcmp(gCurServerId, f, 0xC) == 0) {
            break;
        }
    }
    if (i <= 39) {
        return i;
    }
    return 0;
}
