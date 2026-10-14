/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#pragma optimization_level 4

void Text_SetSize(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void *memset(void *dst, s32 c, u32 n);
/* The ID search menu (gMenuWork while IdSearch_* run). */
typedef struct IdSearchMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u8 unk08[0x28];
    /* 0x30 */ s8 id[8];            /* the hunter ID typed (6 characters: Kbd_Run, memcmp with gMyUserId) */
    /* 0x38 */ s16 sel;             /* the form row (Menu_WrapUpDown of 2) */
    /* 0x3A */ u8 pad3A[6];
    /* 0x40 */ s8 step;             /* IdSearch_Update */
    /* 0x41 */ s8 inputStep;        /* IdSearch_InputId */
} IdSearchMenu;

extern MenuWork gMenuWork;
extern M2C_UNK NameSearch_Draw;
extern M2C_UNK NameSearch_Draw2;

void IdSearch_Close(void) {
    gFieldControl = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
}

extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLevel1Sel;
s32 IdSearch_InputId(void *w, u16 pad);
s32 HunterInfo_Update(u16 pad);
void HunterInfo_Open(void *, s32);
void LobbyText_Open(s32);
void LobbySe_Play(s32);
s32 IdSearch_Update(u16 pad)
{
    IdSearchMenu *w = (IdSearchMenu *)&gMenuWork;

    switch (w->step) {
    case 0:
        switch (IdSearch_InputId(w, pad)) {
        case 0:
            w->step++;
            LobbyText_Open(0x20);
            if (gLobbyLevel1Sel != 0) {
                HunterInfo_Open(w->id, 3);
            } else {
                HunterInfo_Open(w->id, 7);
            }
            break;
        case 3:
            IdSearch_Close();
            return 3;
        case 4:
            IdSearch_Close();
            return 4;
        }
        break;
    case 1:
        switch (HunterInfo_Update(pad)) {
        case 0:
        case 1:
        case 3:
            gFieldControl = 0;
            w->step = 0;
            w->inputStep = 1;
            break;
        case 4:
            IdSearch_Close();
            return 4;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            IdSearch_Close();
            LobbySe_Play(8);
            return 1;
        }
        if (pad & 0x8000) {
            IdSearch_Close();
            return 4;
        }
        break;
    }
    return 2;
}

extern char gMyUserId[];
u32 strlen(const char *s);
s32 memcmp(const char *a, const char *b, s32 n);
void Kbd_SetPos(s32 a, f32 b);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
/* unit's own declaration: with the definition's s8 (char *dst, s16 held, s16 pressed) (sub_main/unk_314E00.c), IdSearch_InputId: 72 of 182 words differ */
s32 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
void LobbyDialog_Grow(void);
s32 Menu_WrapUpDown(s32 cur, s32 n);

s32 IdSearch_InputId(void *arg, u16 pad)
{
    IdSearchMenu *w = arg;

    switch (w->inputStep) {
    case 0:
        switch (WinSlide_Step(&w->win, 0x280, 0x190)) {
        case 0:
            w->inputStep++;
            w->sel = 0;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (w->sel == 0) {
                w->inputStep++;
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 6, 6, w->id);
                LobbySe_Play(8);
            } else if (strlen((char *)w->id) < 6) {
                w->inputStep = 3;
                LobbyText_Open(0x23);
                LobbyDialog_Grow();
                LobbySe_Play(9);
            } else if (memcmp((char *)w->id, gMyUserId, 6) == 0) {
                w->inputStep = 3;
                LobbyText_Open(0x50);
                LobbyDialog_Grow();
                LobbySe_Play(9);
            } else {
                w->inputStep = 0;
                LobbySe_Play(8);
                return 0;
            }
        } else if (pad & 0x40) {
            w->inputStep = 0;
            LobbySe_Play(4);
            return 3;
        } else if (pad & 0x8000) {
            return 4;
        } else {
            w->sel = Menu_WrapUpDown(w->sel, 2);
        }
        break;
    case 2:
        if ((s8)Kbd_Run((char *) w->id, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed)) {
            w->inputStep = 1;
            Kbd_Cancel();
            if (strlen((char *)w->id) < 6) {
                w->inputStep = 3;
                LobbyText_Open(0x23);
                LobbyDialog_Grow();
            } else if (memcmp((char *)w->id, gMyUserId, 6) == 0) {
                w->inputStep = 3;
                LobbyText_Open(0x50);
                LobbyDialog_Grow();
            } else {
                (w->sel)++;
            }
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            w->inputStep = 1;
            LobbySe_Play(8);
        } else if (pad & 0x8000) {
            return 4;
        }
        break;
    }
    return 2;
}

extern char *D_0080CF5C;
extern char *gSearchStartTexts[];
extern char D_00838190[];
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), IdSearch_DrawForm 10 of 152 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
u32 Color_PulseAlpha(u32 color);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);

void IdSearch_DrawForm(IdSearchMenu *w, s16 x, s16 y)
{
    s16 tx;
    u32 c1;
    u32 c2;
    s16 ty = y;

    Win_Draw(x, y, 0xC8, 0x82, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, ty + 0xC, 0xB8, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x10, ty + 0xE);
    Text_PrintfColored(D_0080CF5C);
    ty += 0x36;
    if (w->sel == 0) {
        c1 = Color_PulseAlpha(0x8080);
        c2 = 0x80404040;
        Menu_DrawSideArrows(x + 0xA, ty, 0xB4, 0x16, 0, 0xFF008080);
    } else {
        c1 = 0x80808080;
        c2 = 0x802B7F7F;
    }
    Gfx_DrawRectFrame(x + 0x14, ty, 0xA0, 0x18, c1);
    Gfx_DrawRectFrame(x + 0x13, ty - 1, 0xA2, 0x1A, c1);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(Text_CenterX((char *)w->id, x + 0x64, 0x12), ty + 4);
    Text_PrintfColored(D_00838190, w->id);
    Font_SetColor(c2);
    tx = Text_CenterX(gSearchStartTexts[0], x + 0x64, 0x12);
    Font_SetPos(tx, ty += 0x28);
    Text_PrintfColored(gSearchStartTexts[0]);
    if (w->sel == 1) {
        Menu_DrawArrowsRev(tx, ty, 0x12, gSearchStartTexts[0], 0xFF008080);
    }
}

void Font_SetSpacing(u8 a, u8 b);
void IdSearch_DrawForm(IdSearchMenu *w, s16 id, s16 y);
void HunterInfo_DrawIfOpen(void);
void IdSearch_Draw(u8 *o)
{
    IdSearchMenu *w = (IdSearchMenu *)&gMenuWork;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    switch (w->step) {
    case 0:
        IdSearch_DrawForm(w, w->win.pos, 0x3A);
        break;
    case 1:
        HunterInfo_DrawIfOpen();
        break;
    }
}

void IdSearch_Draw2(u8 *a0)
{
    Text_SetSize(*(s32 *)(a0 + 0x18));
}

void NameSearch_Open(void) {
    memset(&gMenuWork, 0, 0xF00);
    gFieldControl = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, &NameSearch_Draw, &NameSearch_Draw2, 0);
    WinSlide_Step(&gMenuWork.nameSearch.win2, 0x280, 0x190);
}

void NameSearch_Close(void) {
    gFieldControl = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
}

/* unk_6ED140's .rodata (0x838190-0x838198) */
#pragma rodata on
char D_00838190[0x8] = {
#include "lobby/00838190.inc"
};
#pragma rodata off
