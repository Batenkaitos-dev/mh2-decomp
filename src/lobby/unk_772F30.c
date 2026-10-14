#include "common.h"
#include "game_menu.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "window.h"

void Shake_Start(u8 id, u8 flags, f32 amp);
s32 ChatMenu_CanSetState(void);
void Lobby_SetBootMode(s32);
s32 Lobby_CheckDay(s32);
s32 Lobby_GetDayState(void);
void SeqA_Start(s32);
void Chat_ShowWindows(void);
extern s32 gTownMode;

s32 ChatPage_OpenState(void)
{
    if (ChatMenu_CanSetState() != 1) {
        return 1;
    }
    gSubMenuWork->windowSel = gChatWindowSel;
    gHelpPage = 0xB;
    gHelpShow = 0;
    if (gScene.area != 0x83) {
        gHelpLine = gSubMenuWork->windowSel + 5;
    } else {
        gHelpLine = 5;
    }
    Shake_Start(0x10U, 0U, 300.0f);
    return 0;
}

s32 Shake_Update(u8 id);
u8 Shake_Flags(u8 id);
s32 Menu_CursorUpDown(u8 *cur, s32 buttons, u8 count); /* unit's own declaration: n s32 (the definition has u16); with the definition's type ChatPage_UpdateState by 1 of 84 words differ */
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

s32 ChatPage_UpdateState(s32 btn)
{
    u8 *cur;
    u8 v;

    if (Shake_Flags(0x10) & 0x80) {
        return Shake_Update(0x10) == 0 ? 0x8000 : 0;
    }
    if ((u16)btn & 0x8000) {
        Shake_Start(0x10, 0x80, 333.0f);
        gHelpShow = 0;
        Sound_PlaySeFlat(0, 0x14, 0);
        return 0;
    }
    gHelpShow = 1;
    if (Shake_Update(0x10) == 1) {
        gHelpShow = 0;
        btn = (u16)(btn & 0xC3BF);
    }
    if (gScene.area != 0x83) {
        Menu_CursorUpDown(&gSubMenuWork->windowSel, btn, 2);
        cur = &gSubMenuWork->windowSel;
        gHelpLine = *cur + 5;
        if ((u16)btn & 0x20) {
            v = *cur;
            if (v != gChatWindowSel) {
                gChatWindowSel = v;
                gChatSendTo = v;
                Sound_PlaySeFlat(0, 0x13, 0);
            }
        }
    }
    return btn;
}

void Gfx_DrawGauge(s16 x, s16 y, s16 w, s16 h, s32 flags);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
struct Pair;
s32 Shake_CopyOffset(u8 id, struct Pair *out);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
void Text_PrintfColored(const char *, ...);
extern ListPanel gChatStatePanel;
extern ListPanel gChatStatePanelDim;
extern char *D_0080CFAC;
extern char *gChatStateTexts[];
extern char *D_0080CFEC;

/* The window of the yes/no choice (sub_main's 0x5BC49A; area 0x83: a single
 * answer). */
void Chat_DrawState(void)
{
    s16 x[2];
    s32 a;
    s32 b;

    Shake_CopyOffset(0x10, (struct Pair *) x);
    if (gScene.area != 0x83) {
        ListPanel_Draw(&gChatStatePanel, x, D_0080CFAC, gSubMenuWork->windowSel);
        Font_SetPos(x[0] + 0x1A9, 0x55);
        if (gChatWindowSel == 0) {
            Text_PrintfStyled(4, "~C00%s\n~C10%s", gChatStateTexts[0], D_0080CFEC);
            a = 0;
            b = 2;
        } else {
            Text_PrintfStyled(4, "~C10%s\n~C00%s", gChatStateTexts[0], D_0080CFEC);
            a = 2;
            b = 1;
        }
        Gfx_DrawGauge(x[0] + 0x18E, 0x55, 0x14, 0x12, a);
        Gfx_DrawGauge(x[0] + 0x18E, 0x6B, 0x14, 0x12, b);
    } else {
        ListPanel_Draw(&gChatStatePanelDim, x, D_0080CFAC, -1);
        Gfx_DrawGauge(x[0] + 0x18E, 0x55, 0x14, 0x12, 0);
        Font_SetColor(0x807F7F7F);
        Font_SetPos(x[0] + 0x1A9, 0x55);
        Text_PrintfColored(gChatStateTexts[0]);
    }
}

s32 GameMenu_OpenQuit(void *unused0)
{
    GameMenu *p = gSubMenuWork;

    if (p->busy != 0) {
        return 1;
    }
    gHelpShow = 1;
    gHelpPage = 4;
    gHelpLine = 0x24;
    p->subEntry = 0;
    return 0;
}

void GameMenu_Quit(void)
{
    if (Lobby_GetDayState() != 0) {
        Lobby_CheckDay(0);
    }
    Lobby_SetBootMode((gSubMenuWork->subEntry == 0) ? 1 : 2);
    gTownMode = 0x23;
    SeqA_Start(1);
    gFieldControl = 1;
    Chat_ShowWindows();
}

void GameMenu_Back(void);
s32 Card_IsFlagE055(void);
void Chat_HideWindows(void);
void Chat_SetWindowMode(s8 a, s8 b);
s32 GameMenu_YesNoInput(u16 *pad);
void TownNetMenu_Begin(void);
s32 TownNetMenu_Update(s32);
u8 YesNo_Toggle(u16 buttons, u8 on, u16 mask);

u16 GameMenu_UpdateQuit(u16 pad)
{
    u8 *cur;
    GameMenu *m;
    s32 r;
    s32 b;

    switch (gSubMenuWork->subStep) {
    case 0:
        if (pad & 0x8000) {
            GameMenu_Back();
            return 0;
        }
        Menu_CursorUpDown(&gSubMenuWork->subEntry, pad, 2);
        m = gSubMenuWork;
        r = m->subEntry + 0x24;
        b = pad;
        gHelpLine = r;
        cur = &m->subEntry;
        if (b & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            return 0x40;
        }
        if (b & 0x20) {
            if (*cur == 1 && Card_IsFlagE055() != 1) {
                Sound_PlaySeFlat(0, 0x15, 0);
            } else {
                gHelpShow = 0;
                gSubMenuWork->yesNo = 1;
                gSubMenuWork->subStep++;
                gSubMenuWork->page = 0;
                Sound_PlaySeFlat(0, 0x13, 0);
                gFieldControl = 0;
                Chat_HideWindows();
            }
        }
        break;
    case 1:
        Chat_SetWindowMode(3, 2);
        switch (gSubMenuWork->page) {
        case 0:
            if (GameMenu_YesNoInput(&pad) == 1) {
                m = gSubMenuWork;
                if (m->yesNo == 0) {
                    m->page++;
                    gSubMenuWork->yesNo = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                    if (gSubMenuWork->subEntry == 1) {
                        TownNetMenu_Begin();
                        gSubMenuWork->page = 5;
                    }
                } else {
                    m->subStep = 0;
                    gHelpShow = 1;
                    gHelpLine = gSubMenuWork->subEntry + 0x24;
                    Sound_PlaySeFlat(0, 0x14, 0);
                    gFieldControl = 1;
                    Chat_ShowWindows();
                }
            }
            break;
        case 1:
            if (pad & 0x40) {
                gSubMenuWork->yesNo = 1;
                gSubMenuWork->page--;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (pad & 0x20) {
                if (gSubMenuWork->yesNo == 0) {
                    TownNetMenu_Begin();
                    gSubMenuWork->page = 3;
                } else {
                    gSubMenuWork->yesNo = 0;
                    gSubMenuWork->page++;
                }
                Sound_PlaySeFlat(0, 0x13, 0);
            } else {
                gSubMenuWork->yesNo = YesNo_Toggle(pad, gSubMenuWork->yesNo, 0xC00);
            }
            break;
        case 2:
            if (pad & 0x40) {
                gSubMenuWork->yesNo = 0;
                gSubMenuWork->page--;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (pad & 0x20) {
                Sound_PlaySeFlat(0, 0x13, 0);
                if (gSubMenuWork->yesNo == 0) {
                    TownNetMenu_Begin();
                    if (gSubMenuWork->subEntry == 0) {
                        gSubMenuWork->page = 3;
                    } else {
                        gSubMenuWork->page = 5;
                    }
                } else {
                    GameMenu_Quit();
                    gSubMenuWork->page = 4;
                }
            } else {
                gSubMenuWork->yesNo = YesNo_Toggle(pad, gSubMenuWork->yesNo, 0xC00);
            }
            break;
        case 3:
            r = TownNetMenu_Update(1);
            if (r != 0) {
                if (r < 0) {
                    gSubMenuWork->subStep = 1;
                    gSubMenuWork->page = 2;
                    gSubMenuWork->yesNo = 0;
                } else {
                    GameMenu_Quit();
                    gSubMenuWork->page++;
                }
            }
            break;
        case 5:
            r = TownNetMenu_Update(1);
            if (r != 0) {
                if (r < 0) {
                    gSubMenuWork->page = 0;
                } else {
                    GameMenu_Quit();
                    gSubMenuWork->page++;
                }
            }
            break;
        default:
            gFieldControl = 1;
            Chat_ShowWindows();
            break;
        }
        break;
    }
    return 0;
}

/* unk_772F30's .data (0x7F1FD0-0x7F2010) */
ListPanel gChatStatePanel = {
#include "lobby/007F1FD0.inc"
};
ListPanel gChatStatePanelDim = {
#include "lobby/007F1FF0.inc"
};
