#include "common.h"
#include "em.h"
#include "lobby_state.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"

/* prototypes */
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMenuOpen;
/* end of prototypes */
extern s8 gTownNetMenuStep;
void Bgm_SetPaused(u8);

void TownSave_Close(void)
{
    gTownMenuOpen = 0;
    gFieldControl = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
}

typedef struct TextBox { u8 pad[0x18]; s32 size; } TextBox;
/* the village's network menu (gMenuWork) */
typedef struct NetMenuWork {
    /* 0x0 */ u8 kind;
    /* 0x1 */ u8 pad1;
    /* 0x2 */ s8 cur;
    /* 0x3 */ u8 pad3;
    /* 0x4 */ s16 timer;
    /* 0x6 */ u8 pad6[2];
    /* 0x8 */ s32 vals[3];             /* the prices */
} NetMenuWork;
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
s32 Game_IsOnline(void);
void Gfx_SetState63(s32 a);
s32 Gs_SetState(u8, u32);
void Text_SetSize(s32 size);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Menu_DrawLabelList(s16 x, char **items, s16 count, s16 cursor, u32 color);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
char *Text_FillDollar(char *fmt, s32 n);
void Text_Printf(char *fmt, ...);
extern char *gBedMenuTexts[];
extern char *D_0080D1FC[];
extern char *gLivingCostTexts[];
extern char *gYesNoTexts[];
extern s32 gTownMode;
extern s32 gTownDay;
u16 LobbyPad_Get(s32 a);
void LobbyWorld_HoldClock(void);
void Player_ClearFlag(void *obj, u32 flag);
void Actor_SetState(void *, s32, s32);
void LobbySe_Play(s32 se);
void Menu_SetYesNo(s8 choice);
s32 Menu_PushNoticeOnce(s32);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void LobbyText_Open(s32 msg);
void TownNetMenu_Begin(void);
s32 Menu_YesNoInput(void);
void ActSpot_PlaceLocal(void);
s32 TownNetMenu_Update(s32 kind);
void Save_AddMoney(s32 n);
s32 LobbyTime_IsPm(void);
void LobbyTime_SetClock(u32 time);
s32 LobbyWorld_GetSeason(void);

/* the village's network menu (gMenuWork.kind): 0 the choice (a price
 * paid first for the paid lines), 1-2 the confirmation, 3 the network menu
 * (TownNetMenu_Update), 4-5 paying, 6 closing; 3 when cancelled, 0 once the menu
 * is done, 2 while running */
s32 TownSave_Update(void)
{
    NetMenuWork *w = (NetMenuWork *)gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);

    if (Game_IsOnline() == 0 && w->kind != 7 && w->kind != 6) {
        LobbyWorld_HoldClock();
    }
    switch (w->kind) {
    case 6:
        if (--w->timer <= 0) {
            w->kind = 7;
            TownSave_Close();
            Player_ClearFlag(p, 0x20000);
            Actor_SetState(p, 0, 0x10);
        }
        break;
    case 0:
        if (Game_IsOnline() == 1) {
            w->kind++;
        } else if (pad & 0x20) {
            if (gSave.money >= w->vals[w->cur]) {
                if (w->cur == 0) {
                    w->kind++;
                    LobbySe_Play(8);
                } else {
                    w->kind = 4;
                    Menu_SetYesNo(1);
                    LobbySe_Play(8);
                }
            } else {
                LobbySe_Play(9);
                Menu_PushNoticeOnce(0x16);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            gTownMode = 0;
            TownSave_Close();
            return 3;
        } else {
            w->cur = Menu_WrapUpDown(w->cur, 3);
        }
        break;
    case 1:
        if (Game_IsOnline() == 0) {
            LobbyText_Open(0x5D);
            Menu_SetYesNo(1);
            w->kind++;
        } else {
            TownNetMenu_Begin();
            w->kind = 3;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            w->kind++;
            TownNetMenu_Begin();
            ActSpot_PlaceLocal();
            w->timer = 0xF0;
            break;
        case 3:
            w->kind = 0;
            break;
        }
        break;
    case 3:
        if (w->timer != 0) {
            w->timer--;
        }
        if (TownNetMenu_Update(0) == 1) {
            w->kind = 6;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            return 0;
        }
        break;
    case 4:
        switch (Menu_YesNoInput()) {
        case 0:
            w->kind++;
            w->timer = 0xF0;
            Save_AddMoney(-w->vals[w->cur]);
            LobbySe_Play(0xA);
            ActSpot_PlaceLocal();
            break;
        case 3:
            w->kind = 0;
            break;
        }
        break;
    case 5:
        if (--w->timer <= 0) {
            if (w->cur == 1) {
                switch (LobbyTime_IsPm()) {
                case 0:
                    LobbyTime_SetClock(0xA8BF);
                    break;
                case 1:
                    LobbyTime_SetClock(0x1517F);
                    break;
                }
            } else {
                switch (LobbyWorld_GetSeason()) {
                case 0:
                    gSave.day = 2;
                    break;
                case 1:
                    gSave.day = 2;
                    break;
                case 2:
                    gSave.day = 2;
                    break;
                }
                gTownDay = gSave.day;
                LobbyTime_SetClock(0x1517F);
            }
            w->kind = 6;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            return 0;
        }
        break;
    case 7:
        break;
    }
    return 2;
}



/* the village's network menu window (gMenuWork: kind 0 or 4, the
 * cursor at +2): its three lines and the chosen line's help, or (kind 4)
 * a count from +8 and a text of gYesNoTexts */
void TownSave_Draw(TextBox *t)
{
    NetMenuWork *w;

    w = (NetMenuWork *)gMenuWork;
    if (gPlayers[gScene.localPlayer].state == 0xE && Game_IsOnline() == 0) {
        return;
    }
    if (w->kind == 0 || w->kind == 4) {
        Gfx_SetState63(1);
        Gs_SetState(0x60, 0);
        Text_SetSize(t->size);
        Menu_DrawMoney(0x1E0, 0x10, 0);
        Font_SetSpacing(0x14, 0x14);
        Menu_DrawLabelList(0x1C2, gBedMenuTexts, 3, w->cur, 0xFFB0B000);
        switch (w->kind) {
        case 4:
            Win_DrawText(0x114, 0x160, 0x154, 0x50, 0xBDB59F6C, 0x80202020,
                          Text_FillDollar(gLivingCostTexts[0], w->vals[w->cur]));
            Font_SetPos(0x1A0, 0x190);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
            break;
        default:
            Win_DrawText(0x114, 0x160, 0x154, 0x50, 0xBDB59F6C, 0x80202020, D_0080D1FC[w->cur]);
            break;
        }
    }
}

void TownNetMenu_Begin(void)
{
    gTownNetMenuStep = 0;
    gLobby->connected = 0;
    gFieldControl = 0;
    gTownMenuOpen = 1;
    Bgm_SetPaused(1);
}

extern s8 gTownNetMenuFade;
void NetMenu_Open(u8 mode);
u8 NetMenu_Update(void);
void Stream_FadeOut(s32, s32);
void Stream_Stop(s32 ch);
void Stream_Pause(s32, s32);
void LobbySe_Play(s32 se);
void TownNetMenu_Resume(void);

/* the network menu (kind 5 offline, 6 online) opened over the village,
 * its music faded out; returns 1 when the menu ends (2: kind 1 goes back,
 * -1), 0 while it runs */
s32 TownNetMenu_Update(s32 kind)
{
    switch (gTownNetMenuStep) {
    case 0:
        gTownNetMenuStep++;
        if (Game_IsOnline() == 0) {
            NetMenu_Open(5);
        } else {
            NetMenu_Open(6);
        }
        gTownNetMenuFade = 0x41;
        Stream_FadeOut(0, *(volatile s8 *)&gTownNetMenuFade);
        break;
    case 1:
        if (--gTownNetMenuFade < 6) {
            gTownNetMenuStep++;
            Stream_Stop(1);
            Stream_Pause(0, 1);
            if (kind == 0) {
                LobbySe_Play(0xD);
            }
        }
        break;
    case 2:
        switch ((u8)NetMenu_Update()) {
        case 1:
            TownNetMenu_Resume();
            return 1;
        case 2:
            TownNetMenu_Resume();
            if (kind == 1) {
                TownSave_Close();
                return -1;
            }
            return 1;
        }
        break;
    }
    return 0;
}
