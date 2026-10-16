/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "game_menu.h"
#include "em.h"
#include "scene.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "window.h"

/* prototypes */
void Shake_Start(u8 id, u8 flags, f32 amp);
void GameMenu_Reset(void);
s32 GameMenu_Poll(void);
u16 GameMenu_GetPad(void);
s32 GameMenu_IsBusy(void);
void GameMenu_DrawList(s8 dim);
s32 GameMenu_OpenSearch(void);
s32 ChatMenu_CanListSpeakers(void);
s32 ChatMenu_CanSetState(void);
s32 GameMenu_OpenChat(void);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s32 sel); /* unit's own declaration: sel s32 (the definition has s16): with s16 GameMenu_DrawSearch changes by 10 of 27 words */
extern ListPanel gSearchPanel;
/* end of prototypes */

s32 Cond_Check(u16);
void GameMenu_Start(void);
void GameMenu_SetHelp(void);
void Hud_ResetMenuState(void);
s32 TownPc_IsSeatedState(u8, u8);
s32 Area_IsHouse(u16);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 Hud_ItemGetBusy(void);
s32 Game_IsOnline(void);
extern s32 gTownMode;
u16 CombineMenu_Input(u16 pad);
void Save_StorePouch(Player *p);
u16 PouchMenu_Update(u16 pad);
s32 CombineMenu_Open(void);
void Player_CalcSkills(Player *p);
u16 LobbyPad_Get(s32);
s32 Lobby_GetDayState(void);
void GameMenu_Clear(void);

void GameMenu_Reset(void) {
    gSubMenuWork = &gSubMenuWorkMem;
    gSubMenuWork->entry = 0;
    gSubMenuWork->open = 0;
}

extern u8 gGameMenuHelpLines0[];
extern u8 gGameMenuHelpLines1[];

/* gScene.menuOpen (0x6ADCBE) is stored through its raw address: named (also
 * through a cast or volatile), its store goes after gTownMode's, into the
 * jal's delay slot. */
void GameMenu_Open(void)
{
    s32 townMode;
    u8 localPlayer;
    Player *p;

    localPlayer = gScene.localPlayer;
    p = &gPlayers[localPlayer];
    if (Game_IsOnline() == 0) {
        if (Cond_Check(1) != 0) {
            goto block_3;
        }
    } else {
block_3:
        if (((Area_IsHouse(gScene.area) != 1) || (TownPc_IsSeatedState(p->stateGroup, p->state) != 1)) && (Hud_ItemGetBusy() != 1)) {
            townMode = gTownMode;
            if (townMode != 0xB) {
                gScene.menuOpen = 1;
                gTownMode = 0xB;
                LobbyMenu_Init();
                LobbyMenu_SetDraw(0, 0, 0, 0);
                GameMenu_Start();
                gSubMenuWork->open = 1;
                gSubMenuWork->pad = 0;
                Sound_PlaySeFlat(0, 0x11, 0);
            }
        }
    }
}

void GameMenu_Start(void)
{
    gSubMenuWork->steps = 0;
    gHelpShow = 1;
    gHelpPage = 4;
    if (Game_IsOnline() != 1) {
        gSubMenuWork->kind = 0;
        gSubMenuWork->count = 9;
    } else if (gScene.area == 0x83) {
        gSubMenuWork->kind = 1;
        gSubMenuWork->count = 9;
    } else {
        gSubMenuWork->kind = 2;
        gSubMenuWork->count = 10;
    }
    GameMenu_SetHelp();
    gChatHidden = 0;
    Shake_Start(0, 0, 180.0f);
    Hud_ResetMenuState();
}

void GameMenu_Clear(void) {
    gSubMenuWork->open = 0;
    gSubMenuWork->steps = 0;
    gHelpShow = 0;
}

s32 GameMenu_Poll(void) {
    if (gSubMenuWork->open == 0) {
        GameMenu_Clear();
        return 1;
    }
    if (Lobby_GetDayState() >= 2) {
        gSubMenuWork->pad = (u16) gLobby->buttons;
    } else {
        gSubMenuWork->pad = LobbyPad_Get(0);
    }
    return 0;
}

u16 GameMenu_GetPad(void) {
    if (Lobby_GetDayState() >= 2) {
        return gLobby->buttons;
    }
    if (gSubMenuWork->open == 0) {
        return LobbyPad_Get(0);
    }
    return gSubMenuWork->pad;
}

s32 Town_IsMode(s32 a);
s32 Lobby_HasQuest(void);
s32 Hud_ItemMenusAllowed(void);
s32 ItemGet_IsReady(struct Player *p);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Shake_Update(u8 id);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
void Shake_Stop(void);
void GameMenu_Back(void);
extern s32 (*gGameMenuOpen0[])(void *m);
extern s32 (*gGameMenuOpen1[])(void *m);
extern s32 (*gGameMenuOpen2[])(void *m);
extern u16 (*gGameMenuUpdate0[])(s32 pad);
extern u16 (*gGameMenuUpdate1[])(s32 pad);
extern u16 (*gGameMenuUpdate2[])(s32 pad);
extern u8 gTownMsg0DPending;

/* The game menu (gSubMenuWork): the list (kind, entry) while step is 0,
 * then the chosen page's own input from the tables at
 * 0x7F1D40.. Returns 1 when the menu is closed (or not open). */
s32 GameMenu_Update(u16 pad)
{
    GameMenu *m;
    s32 r;
    u16 k;

    if (gSubMenuWork->open == 0) {
        return 1;
    }
    if (Town_IsMode(0xB) == 0) {
        return 1;
    }
    gSubMenuWork->pad = 0;
    gSubMenuWork->busy = 1;
    if (Lobby_HasQuest() == 0 && Hud_ItemMenusAllowed() == 1 && ItemGet_IsReady(0) == 1) {
        gSubMenuWork->busy = 0;
    }
    Chat_SetWindowMode(1, 2);
    m = gSubMenuWork;
    switch (m->step) {
    case 0:
        if (Shake_Update(0) == 1) {
            pad &= 0x3C40;
        }
        Menu_CursorUpDown(&gSubMenuWork->entry, pad, gSubMenuWork->count);
        GameMenu_SetHelp();
        if (pad & 0x20) {
            Shake_Stop();
            m = gSubMenuWork;
            switch (m->kind) {
            case 0:
                r = gGameMenuOpen0[m->entry](m);
                break;
            case 1:
                r = gGameMenuOpen1[m->entry](m);
                break;
            case 2:
                r = gGameMenuOpen2[m->entry](m);
                break;
            }
            if (r == 0) {
                gSubMenuWork->step++;
                Sound_PlaySeFlat(0, 0x13, 0);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
        } else if (pad & 0x8040) {
            GameMenu_Back();
        }
        break;
    case 1:
        switch (m->kind) {
        case 0:
            k = gGameMenuUpdate0[m->entry](pad);
            break;
        case 1:
            k = gGameMenuUpdate1[m->entry](pad);
            break;
        case 2:
            k = gGameMenuUpdate2[m->entry](pad);
            break;
        }
        if (k & 0x8000) {
            gSubMenuWork->open = 0;
            if (gLobby->connected == 0) {
                gLobby->connected = 1;
                gTownMsg0DPending = 1;
            }
            return 1;
        }
        if (k & 0x40) {
            gSubMenuWork->steps = 0;
            gHelpShow = 1;
            gHelpPage = 4;
            GameMenu_SetHelp();
            Sound_PlaySeFlat(0, 0x14, 0);
            if (gLobby->connected == 0) {
                gLobby->connected = 1;
                gTownMsg0DPending = 1;
            }
        }
        break;
    case 2:
        if (Shake_Update(0) == 0) {
            gSubMenuWork->open = 0;
            return 1;
        }
        break;
    }
    return 0;
}

void GameMenu_Back(void)
{
    gSubMenuWork->step = 2;
    gHelpShow = 0;
    Shake_Start(0U, 0x80U, 180.0f);
    Sound_PlaySeFlat(0, 0x14, 0);
}

void GameMenu_SetHelp(void)
{
    u8 temp_a0;

    temp_a0 = gSubMenuWork->kind;
    switch (temp_a0) {
    case 0:
        gHelpLine = (s16) *(gGameMenuHelpLines0 + gSubMenuWork->entry);
        break;
    case 1:
        gHelpLine = (s16) *(gGameMenuHelpLines1 + gSubMenuWork->entry);
        break;
    case 2:
        gHelpLine = (s16) gSubMenuWork->entry;
        break;
    }
}

s32 GameMenu_IsBusy(void) {
    return gSubMenuWork->busy != 0;
}

typedef void (*VCondFunc)(void);
extern VCondFunc gGameMenuDraw0[];
extern VCondFunc gGameMenuDraw1[];
extern VCondFunc gGameMenuDraw2[];
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
void Gfx_SetState63(s32);
s32 Gs_SetState(u8, u32);
s32 Shake_Value(u8);
void HelpWindow_DrawCurrent(void);

/* The game menu's frame (by step): the list, the open page's draw function
 * by kind and entry (the tables are called without arguments), or the list
 * dimmed while closing. */
void GameMenu_Draw(void)
{
    if (gSubMenuWork->open == 0) {
        return;
    }
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    switch (gSubMenuWork->step) {
    case 0:
        GameMenu_DrawList(0);
        break;
    case 1:
        switch (gSubMenuWork->kind) {
        case 0:
            gGameMenuDraw0[gSubMenuWork->entry]();
            break;
        case 1:
            if (gSubMenuWork->entry != 8) {
                gGameMenuDraw1[gSubMenuWork->entry]();
            }
            break;
        case 2:
            if (gSubMenuWork->entry != 9) {
                gGameMenuDraw2[gSubMenuWork->entry]();
            }
            break;
        }
        break;
    case 2:
        GameMenu_DrawList(1);
        return;
    }
    if (Shake_Value(0) == 0 && gHelpShow != 0) {
        HelpWindow_DrawCurrent();
    }
}

void *Shake_GetOffset(u8 id);
void Gfx_SetBlend(s32 a, s32 b);

extern char *gGameMenuTexts0[];
extern char *gGameMenuTexts0Dim[];
extern char *gGameMenuTexts1[];
extern char *gGameMenuTexts2[];
extern char *gGameMenuTexts2Dim[];
extern ListPanel gGameMenuPanel;
extern s32 D_0080C0B4;
f32 Math_Sin(f32 a);

extern u16 gFrameTick; /* sub_main: the frame counter */

/* The frame counter is read as a named object (gFrameTick): the game loads
 * it above the store to gGameMenuListColor, which MWCC does only for a named one. */
void GameMenu_DrawList(s8 dim)
{
    s32 w;
    u16 t;

    Gfx_SetBlend(4, 5);
    w = (s32) Shake_GetOffset(0);
    if (dim == 0) {
        t = (gFrameTick & 0x3F) << 10;
        gGameMenuPanel.cursorColor = 0xA9182;
        Math_Sin(0.0000958738f * t);
    } else {
        gGameMenuPanel.cursorColor = 0x808080;
    }
    switch (gSubMenuWork->kind) {
    case 0:
        if (gSubMenuWork->busy == 0) {
            gGameMenuPanel.items = gGameMenuTexts0;
        } else {
            gGameMenuPanel.items = gGameMenuTexts0Dim;
        }
        break;
    case 1:
        gGameMenuPanel.items = gGameMenuTexts1;
        break;
    case 2:
        if (gSubMenuWork->busy == 0) {
            gGameMenuPanel.items = gGameMenuTexts2;
        } else {
            gGameMenuPanel.items = gGameMenuTexts2Dim;
        }
        break;
    }
    gGameMenuPanel.lines = gSubMenuWork->count;
    ListPanel_Draw(&gGameMenuPanel, (s16 *)w, (char *)D_0080C0B4, gSubMenuWork->entry);
}

s32 GameMenu_UpdateItems(u16 pad)
{
    s32 ret;

    ret = PouchMenu_Update(pad);
    Save_StorePouch(&gPlayers[gScene.localPlayer]);
    return ret;
}

void GameMenu_OpenCombine(void)
{
    Player_CalcSkills(&gPlayers[gScene.localPlayer]);
    CombineMenu_Open();
}

s32 GameMenu_UpdateCombine(u16 pad)
{
    s32 ret;

    ret = CombineMenu_Input(pad);
    Save_StorePouch(&gPlayers[gScene.localPlayer]);
    return ret;
}

s32 GameMenu_OpenSearch(void) {
    gSubMenuWork->subStep = 0;
    gSubMenuWork->subEntry = 0;
    gHelpShow = 1;
    gHelpPage = 4;
    gHelpLine = 0x1C;
    return 0;
}

void GameMenu_Back(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
void FriendList_Open(s32 a);
void AreaSearch_Open(void);
void NameSearch_Open(void);
void IdSearch_Open(void);
s32 FriendList_Update(u16);
s32 AreaSearch_Update(u16);
s32 NameSearch_Update(u16);
s32 IdSearch_Update(u16);

/* The search page: subStep 0 its list (subEntry, four searches), 1 the
 * chosen search; the page's own input while one is open. Returns the pad, 0 when
 * the menu takes it. */
u16 GameMenu_UpdateSearch(u16 pad)
{
    GameMenu *m;
    u8 *cur;

    switch (gSubMenuWork->subStep) {
    case 0:
        if (pad & 0x8000) {
            GameMenu_Back();
            return 0;
        }
        Menu_CursorUpDown(&gSubMenuWork->subEntry, pad, 4);
        m = gSubMenuWork;
        cur = &m->subEntry;
        gHelpLine = m->subEntry + 0x1C;
        if (pad & 0x20) {
            switch (*cur) {
            case 0:
                m->subStep++;
                FriendList_Open(0);
                break;
            case 1:
                m->subStep++;
                AreaSearch_Open();
                break;
            case 2:
                m->subStep++;
                NameSearch_Open();
                break;
            case 3:
                m->subStep++;
                IdSearch_Open();
                break;
            }
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case 0:
            switch (FriendList_Update(pad)) {
            case 3:
            case 1:
                gSubMenuWork->subStep--;
                return 0;
            case 2:
                return 0;
            case 4:
                pad = 0x8000;
                break;
            }
            break;
        case 1:
            switch (AreaSearch_Update(pad)) {
            case 3:
            case 1:
                gSubMenuWork->subStep--;
                return 0;
            case 4:
                pad = 0x8000;
                break;
            case 2:
                return 0;
            }
            break;
        case 2:
            switch (NameSearch_Update(pad)) {
            case 3:
            case 1:
                gSubMenuWork->subStep--;
                return 0;
            case 2:
                return 0;
            case 4:
                pad = 0x8000;
                break;
            }
            break;
        case 3:
            switch (IdSearch_Update(pad)) {
            case 3:
            case 1:
                gSubMenuWork->subStep--;
                return 0;
            case 2:
                return 0;
            case 4:
                pad = 0x8000;
                break;
            }
            break;
        }
        if (pad & 0x8000) {
            GameMenu_Back();
            pad = 0;
        }
        break;
    }
    if (gSubMenuWork->subStep == 0) {
        gHelpShow = 1;
    } else {
        gHelpShow = 0;
    }
    return pad;
}

void GameMenu_DrawSearch(void)
{
    u8 temp_v1;

    temp_v1 = gSubMenuWork->subStep;
    switch (temp_v1) {
    case 0:
        GameMenu_DrawList(1);
        ListPanel_Draw(&gSearchPanel, (s16 *)0, (char *)0, (s32) gSubMenuWork->subEntry);
        
    case 1:
        break;
    }
}

s32 ChatMenu_CanListSpeakers(void) {
    return gChatSendTo == 0;
}

s32 ChatMenu_CanSetState(void) {
    return 1;
}

s32 GameMenu_OpenChat(void) {
    gSubMenuWork->subStep = 0;
    gSubMenuWork->subEntry = 0;
    gHelpShow = 1;
    gHelpPage = 0xB;
    gHelpLine = 0;
    return 0;
}

/* unk_770970's .data (0x7F1D20-0x7F1F30) */
void CombineMenu_DrawInfo(void);
extern char *gSearchPanel_7[];
void InfoMenu_Draw(void);
s32 InfoMenu_Open(void *m);
u16 InfoMenu_Update(u16 pad);
void OptionMenu_Draw(void);
s32 OptionMenu_Open(void *m);
s32 OptionMenu_Update(u16 pad);
void PhraseMenu_Draw(void);
s32 PhraseMenu_Open(void *m);
u16 PhraseMenu_Update(u16 pad);
void PouchMenu_Draw(void);
s32 PouchMenu_Open(void *m);
void QuestMenu_DrawPage(void);
s32 QuestMenu_Open(void *m);
u16 QuestMenu_Update(u16 pad);
void StatusMenu_Draw(void);
s32 StatusMenu_Open(void *m);
u16 StatusMenu_Update(u16 pad);
void GameMenu_DrawChat(void);
void GameMenu_DrawMail(void);
void GameMenu_DrawPause(void);
void GameMenu_DrawQuit(void);
s32 GameMenu_OpenExit(void *unused0);
s32 GameMenu_OpenMail(void *m);
s32 GameMenu_OpenPause(void *unused0);
s32 GameMenu_OpenQuit(void *unused0);
u16 GameMenu_UpdateChat(u16 pad);
s32 GameMenu_UpdateExit(s32 pad);
u16 GameMenu_UpdateMail(u16 pad);
u16 GameMenu_UpdatePause(u16 pad);
u16 GameMenu_UpdateQuit(u16 pad);
u8 gGameMenuHelpLines0[16] = {
#include "lobby/007F1D20.inc"
};
u8 gGameMenuHelpLines1[16] = {
#include "lobby/007F1D30.inc"
};
s32 (*gGameMenuOpen0[12])(void *m) = {
    PouchMenu_Open, (s32 (*)(void *m))GameMenu_OpenCombine, StatusMenu_Open, QuestMenu_Open,
    InfoMenu_Open, PhraseMenu_Open, OptionMenu_Open, GameMenu_OpenPause,
    GameMenu_OpenQuit, NULL, NULL, NULL,
};
u16 (*gGameMenuUpdate0[12])(s32 pad) = {
    (u16 (*)(s32 pad))GameMenu_UpdateItems, (u16 (*)(s32 pad))GameMenu_UpdateCombine, (u16 (*)(s32 pad))StatusMenu_Update, (u16 (*)(s32 pad))QuestMenu_Update,
    (u16 (*)(s32 pad))InfoMenu_Update, (u16 (*)(s32 pad))PhraseMenu_Update, (u16 (*)(s32 pad))OptionMenu_Update, (u16 (*)(s32 pad))GameMenu_UpdatePause,
    (u16 (*)(s32 pad))GameMenu_UpdateQuit, NULL, NULL, NULL,
};
s32 (*gGameMenuOpen1[12])(void *m) = {
    PouchMenu_Open, (s32 (*)(void *m))GameMenu_OpenCombine, StatusMenu_Open, InfoMenu_Open,
    (s32 (*)(void *m))GameMenu_OpenChat, GameMenu_OpenMail, (s32 (*)(void *m))GameMenu_OpenSearch, OptionMenu_Open,
    GameMenu_OpenExit, NULL, NULL, NULL,
};
u16 (*gGameMenuUpdate1[12])(s32 pad) = {
    (u16 (*)(s32 pad))GameMenu_UpdateItems, (u16 (*)(s32 pad))GameMenu_UpdateCombine, (u16 (*)(s32 pad))StatusMenu_Update, (u16 (*)(s32 pad))InfoMenu_Update,
    (u16 (*)(s32 pad))GameMenu_UpdateChat, (u16 (*)(s32 pad))GameMenu_UpdateMail, (u16 (*)(s32 pad))GameMenu_UpdateSearch, (u16 (*)(s32 pad))OptionMenu_Update,
    (u16 (*)(s32 pad))GameMenu_UpdateExit, NULL, NULL, NULL,
};
s32 (*gGameMenuOpen2[12])(void *m) = {
    PouchMenu_Open, (s32 (*)(void *m))GameMenu_OpenCombine, StatusMenu_Open, QuestMenu_Open,
    InfoMenu_Open, (s32 (*)(void *m))GameMenu_OpenChat, GameMenu_OpenMail, (s32 (*)(void *m))GameMenu_OpenSearch,
    OptionMenu_Open, GameMenu_OpenExit, NULL, NULL,
};
u16 (*gGameMenuUpdate2[12])(s32 pad) = {
    (u16 (*)(s32 pad))GameMenu_UpdateItems, (u16 (*)(s32 pad))GameMenu_UpdateCombine, (u16 (*)(s32 pad))StatusMenu_Update, (u16 (*)(s32 pad))QuestMenu_Update,
    (u16 (*)(s32 pad))InfoMenu_Update, (u16 (*)(s32 pad))GameMenu_UpdateChat, (u16 (*)(s32 pad))GameMenu_UpdateMail, (u16 (*)(s32 pad))GameMenu_UpdateSearch,
    (u16 (*)(s32 pad))OptionMenu_Update, (u16 (*)(s32 pad))GameMenu_UpdateExit, NULL, NULL,
};
VCondFunc gGameMenuDraw0[12] = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, PhraseMenu_Draw, OptionMenu_Draw, GameMenu_DrawPause,
    GameMenu_DrawQuit, NULL, NULL, NULL,
};
VCondFunc gGameMenuDraw1[12] = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, InfoMenu_Draw,
    GameMenu_DrawChat, GameMenu_DrawMail, GameMenu_DrawSearch, OptionMenu_Draw,
    NULL, NULL, NULL, NULL,
};
VCondFunc gGameMenuDraw2[12] = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, GameMenu_DrawChat, GameMenu_DrawMail, GameMenu_DrawSearch,
    OptionMenu_Draw, NULL, NULL, NULL,
};
ListPanel gGameMenuPanel = {
#include "lobby/007F1EF0.inc"
};
ListPanel gSearchPanel = {
#include "lobby/007F1F10.inc"
};
