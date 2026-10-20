/*
 * The server choice menu, where the player picks one of the lobby's
 * servers (gLobbyServers, eight to a page), the lookups of a server by
 * its id, and two small hooks of the lobby's menus.
 * - SrvMenu_Run runs one frame of the menu for the network screen
 *   (NetScreen_Step3). Its pages (enum SrvMenuPage, gSrvMenuPages) open
 *   the menu, slide its window in, show an introductory message when the
 *   screen asks the player to pick a server, run the list's cursor, and
 *   ask whether to quit. It returns the server chosen, SRV_MENU_NONE
 *   while the menu runs, or SRV_MENU_QUIT.
 * - SrvMenu_DrawInfo and SrvMenu_DrawWindow, the menu's draw hooks, draw
 *   the location panel and the list (SrvMenu_DrawList), both sliding in
 *   from the left while LobbyMenu.slideTimer runs.
 * - The LobbySrv_* lookups find a server in the list by its id (the
 *   current server's, gCurServerId, or one given): its name, its index,
 *   whether the list holds it, and its colour, the list's value
 *   (LbsRow.value), which the menus draw a server's icon in with an alpha
 *   over it.
 * - LobbyDialog_DrawPending, a menu's draw hook, draws the lobby's dialog
 *   when the menu asks for a redraw (LobbyMenu.redraw).
 * - LobbySrv_HasCurName and LobbySrv_CurName give the current server's
 *   name, which the server name request writes (gCurServerName).
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "browser.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "lobby_servers.h"
#include "menu_task.h"
#include "bgm.h"
#include "pad.h"
#include "lobby_labels.h"

extern char gCurServerId[];                 /* the current server's id (sub_main) */
s32 memcmp(void *a, void *b, s32 n);
u16 Pad_GetCursorButtons(s32 pad);
void Stream_FadeOut(s32, s32);
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDownPad(s32 cur, s32 n, u16 pad);
s32 Menu_WrapLeftRightPad(s32 cur, s32 n, u16 pad);
void LobbyText_Open(s32 msg);
extern s8 gSrvMenuChoice;

void LobbyTitle_Draw(void);
void LobbyHelp_Draw(void);
void Font_SetSpacing(u8 a, u8 b);
void Text_SetSize(s32);
void SrvMenu_DrawList(s16 x, s16 y);
extern LobbyMenu *gLobbyMenu;
void LobbySrv_DrawServerPanel(s32, s32); /* unit's own declaration: the definition is in the old (K&R) style, so its short parameters arrive as ints, as here; with (s16 x, s16 y) the call would narrow them first, which the game's call doesn't */
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void SrvMenu_DrawInfo(MenuTask *task);
void SrvMenu_DrawWindow(MenuTask *task);
s32 LobbyPad_Pressed(s32 button);
void Menu_SetYesNo(u8 choice); /* unit's own declaration: the answer is passed as the u8 field it is read from; with the definition's s8 parameter, SrvMenu_Quit would sign the byte before the call, which the game's code doesn't */
s32 LobbyMenu_MoveCursor(u8 *, u8, u8);
void Bgm_PlayWithVolume(s32, s32);
void LobbyDialog_DrawPending(MenuTask *task);
void Lobby_DrawBlackLine(void);
extern void (*gSrvMenuPages[])(void);      /* the server menu's pages */
s32 LobbySrv_Index(char *s);
s32 LoginWave_Update(void);
void LobbyMenu_QueueLayers(void);
void SrvMenu_Choose(void);
void SrvMenu_Intro(void);
void SrvMenu_Quit(void);
void SrvMenu_SlideIn(void);
void SrvMenu_Start(void);
s32 Menu_CountPages(s16, s16);
void LobbyTitle_Set(s32);
void LobbyHelp_Set(s32);
void LobbyHelp_SetBox(s16, s16, s16);
void NetMode_Init(void);
void LoginWave_Clear(void);
void LoginWave_Mark(s32);
void FadeA_Start(s32);

#define SRV_SLIDE_FRAMES 6          /* frames: the window's slide in */
#define SRV_CHOSEN_FRAMES 0x28      /* frames from a choice (or quitting) to the result */
#define SRV_INTRO_FRAMES 0x260      /* frames: the introductory message's time on screen */
#define SRV_INTRO_SKIP_AFTER 0x224  /* circle skips the message once the timer is below this (0x3C frames in) */
#define SRV_QUIT_WAIT_FRAMES 8      /* frames before and after the quit question's answer */
#define YES_NO_YES 0                /* LobbyMenu.quitYesNo */
#define YES_NO_NO 1
#define CURSOR_LEFT_RIGHT 3         /* LobbyMenu_MoveCursor's mode: left and right move the cursor */

/* The server choice: when a group was joined (LobbyState.groupJoin), the
 * index of its server (LobbySrv_Index of groupId); else runs the menu's
 * page (gSrvMenuPages[screen], SrvMenu_Start ... SrvMenu_Quit) and
 * returns its choice (gSrvMenuChoice, SRV_MENU_NONE while none). The
 * pages take no parameter: the address of groupId left in the first
 * argument register is the if-branch's argument, which the compiler
 * moves ahead of the branch. */
s32 SrvMenu_Run(void)
{
    LobbyState *lobby = gLobby;

    if (lobby->groupJoin != 0) {
        return LobbySrv_Index(lobby->groupId);
    }
    gSrvMenuChoice = SRV_MENU_NONE;
    gSrvMenuPages[gLobbyMenu->screen]();
    LoginWave_Update();
    LobbyMenu_QueueLayers();
    return gSrvMenuChoice;
}

/* The server choice menu's pages, by LobbyMenu.screen (enum SrvMenuPage). */
void (*gSrvMenuPages[8])(void) = {
    SrvMenu_Start, SrvMenu_SlideIn, SrvMenu_Choose, SrvMenu_Intro,
    SrvMenu_Quit, NULL, NULL, NULL,
};

/*
 * Opens the server choice menu (SRV_PAGE_START): resets the network mode
 * and the login wave, moves on to the slide-in with the cursor and page
 * at 0, counts the pages of the server list (gLobbyServerCount), sets the
 * draw hooks, the fade, the title and the help box, and plays the
 * window's sound.
 */
void SrvMenu_Start(void)
{
    NetMode_Init();
    LoginWave_Clear();
    LoginWave_Mark(0);
    gLobbyMenu->screen = (u8) (gLobbyMenu->screen + 1);
    gLobbyMenu->step = 0;
    gLobbyMenu->cursor = 0;
    gLobbyMenu->slideTimer = SRV_SLIDE_FRAMES;
    gLobbyMenu->page = 0;
    gLobbyMenu->pages = Menu_CountPages(gLobbyServerCount, SRV_MENU_PAGE_LEN);
    LobbyMenu_SetDraw(SrvMenu_DrawInfo, SrvMenu_DrawWindow, 0, LobbyDialog_DrawPending);
    FadeA_Start(2);
    LobbyTitle_Set(LOBBY_TITLE_SERVER_MENU);
    LobbyHelp_Set(0);
    LobbySe_Play(LOBBY_SE_WINDOW_IN);
    LobbyHelp_SetBox(0x24, 0x16A, 0x154);
}

/*
 * The window's slide in (SRV_PAGE_SLIDE_IN): counts slideTimer down
 * below 0, then goes on to the list (SRV_PAGE_CHOOSE) and starts the
 * menu's music. When the network screen set LobbyMenu.promptStep, it goes
 * to the introductory message (SRV_PAGE_INTRO) at once instead, with only
 * the black line and the dialog drawn.
 */
void SrvMenu_SlideIn(void)
{
    s16 timer;
    u8 step;

    if (gLobbyMenu->promptStep == 0) {
        step = gLobbyMenu->step;
        switch (step) {
        case 0:
            timer = gLobbyMenu->slideTimer - 1;
            gLobbyMenu->slideTimer = timer;
            if (timer < 0) {
                gLobbyMenu->step = (u8) (gLobbyMenu->step + 1);
            }
            return;
        case 1:
            gLobbyMenu->slideTimer = 0;
            gLobbyMenu->screen = (u8) (gLobbyMenu->screen + 1);
            gLobbyMenu->step = 0;
            Bgm_PlayWithVolume(BGM_SERVER_MENU, 0x1E);
            return;
        }
    } else {
        gLobbyMenu->screen = SRV_PAGE_INTRO;
        gLobbyMenu->step = 0;
        gLobbyMenu->promptStep = 0;
        LobbyMenu_SetDraw(0, &Lobby_DrawBlackLine, &LobbyDialog_DrawPending, 0);
    }
}

/*
 * The list (SRV_PAGE_CHOOSE). Step 0: circle takes the server under the
 * cursor when it has a capacity (else the refusal sound), shows the "to
 * the town" message and fades the music out; cross asks whether to quit
 * (SRV_PAGE_QUIT); left and right turn the page and up and down move the
 * cursor. Step 1 waits SRV_CHOSEN_FRAMES and gives the server's index as
 * the choice; step 2 (a yes to the quit question) waits as long and gives
 * SRV_MENU_QUIT.
 */
void SrvMenu_Choose(void)
{
    u16 pad;
    u8 *stepPtr;
    u8 step;
    s32 page;
    s32 n;

    pad = Pad_GetCursorButtons(0);
    stepPtr = &gLobbyMenu->step;
    step = *stepPtr;
    switch (step) {
    case 0:
        if (pad & PAD_CIRCLE) {
            if (gLobbyServers.row[(u8)(gLobbyMenu->cursor + gLobbyMenu->page * SRV_MENU_PAGE_LEN)].capacity != 0) {
                *stepPtr = step + 1;
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                gLobbyMenu->timer = SRV_CHOSEN_FRAMES;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                LobbyText_Open(LOBBY_TEXT_TO_TOWN);
                gLobbyMenu->redraw = 1;
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            gLobbyMenu->screen = SRV_PAGE_QUIT;
            gLobbyMenu->step = 0;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            page = gLobbyMenu->page;
            gLobbyMenu->page = Menu_WrapLeftRightPad(page, gLobbyMenu->pages, pad);
            if (page != gLobbyMenu->page) {
                gLobbyMenu->cursor = 0;
            }
        } else {
            if (gLobbyMenu->page < gLobbyMenu->pages - 1) {
                n = SRV_MENU_PAGE_LEN;
            } else {
                n = gLobbyServers.count % SRV_MENU_PAGE_LEN;
            }
            gLobbyMenu->cursor = Menu_WrapUpDownPad(gLobbyMenu->cursor, n, pad);
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        gLobbyMenu->timer--;
        if (gLobbyMenu->timer < 0) {
            gSrvMenuChoice = gLobbyMenu->cursor + gLobbyMenu->page * SRV_MENU_PAGE_LEN;
        }
        break;
    case 2:
        gLobbyMenu->timer--;
        if (gLobbyMenu->timer < 0) {
            gSrvMenuChoice = SRV_MENU_QUIT;
        }
        break;
    }
}

/*
 * The introductory message (SRV_PAGE_INTRO, LOBBY_TEXT_SERVER_INTRO):
 * shown for SRV_INTRO_FRAMES, which circle cuts short after its first
 * 0x3C frames; then the list (SRV_PAGE_CHOOSE) with its draw hooks and
 * the menu's music.
 */
void SrvMenu_Intro(void)
{
    u8 *stepPtr;
    u8 step;

    stepPtr = &gLobbyMenu->step;
    step = *stepPtr;
    switch (step) {
    case 0:
        *stepPtr = step + 1;
        gLobbyMenu->timer = SRV_INTRO_FRAMES;
        LobbyText_Open(LOBBY_TEXT_SERVER_INTRO);
        gLobbyMenu->redraw = 1;
        break;
    case 1:
        gLobbyMenu->timer--;
        gLobbyMenu->redraw = 1;
        if (gLobbyMenu->timer < SRV_INTRO_SKIP_AFTER && LobbyPad_Pressed(PAD_CIRCLE) != 0) {
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gLobbyMenu->screen = SRV_PAGE_CHOOSE;
            gLobbyMenu->step = 0;
            LobbyMenu_SetDraw(SrvMenu_DrawInfo, SrvMenu_DrawWindow, NULL, &LobbyDialog_DrawPending);
            Bgm_PlayWithVolume(BGM_SERVER_MENU, 0x3C);
        } else if (gLobbyMenu->timer < 0) {
            gLobbyMenu->screen = SRV_PAGE_CHOOSE;
            gLobbyMenu->step = 0;
            LobbyMenu_SetDraw(SrvMenu_DrawInfo, SrvMenu_DrawWindow, NULL, &LobbyDialog_DrawPending);
            Bgm_PlayWithVolume(BGM_SERVER_MENU, 0x3C);
        }
        break;
    }
}

/*
 * The quit question (SRV_PAGE_QUIT, LOBBY_TEXT_QUIT_ASK), with yes and no
 * in LobbyMenu.quitYesNo, "no" first. Circle answers; cross moves the
 * cursor to "no", or on "no" goes back to the list; left and right move
 * the cursor. After the answer and a wait, yes goes to the list's step 2,
 * which fades the music out and quits the menu; no goes back to the list.
 */
void SrvMenu_Quit(void)
{
    u8 *stepPtr;
    u8 step;
    u8 *answer;

    stepPtr = &gLobbyMenu->step;
    step = *stepPtr;
    switch (step) {
    case 0:
        *stepPtr = step + 1;
        gLobbyMenu->timer = SRV_QUIT_WAIT_FRAMES;
        LobbyText_Open(LOBBY_TEXT_QUIT_ASK);
        gLobbyMenu->quitYesNo = YES_NO_NO;
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
        if (LobbyPad_Pressed(PAD_CIRCLE) != 0) {
            if (gLobbyMenu->quitYesNo == YES_NO_YES) {
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            } else {
                LobbySe_Play(LOBBY_SE_CANCEL);
            }
            gLobbyMenu->step++;
            gLobbyMenu->timer = SRV_QUIT_WAIT_FRAMES;
        } else if (LobbyPad_Pressed(PAD_CROSS) != 0) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            answer = &gLobbyMenu->quitYesNo;
            if (*answer != YES_NO_NO) {
                *answer = YES_NO_NO;
            } else {
                gLobbyMenu->screen = SRV_PAGE_CHOOSE;
                gLobbyMenu->step = 0;
            }
        } else {
            LobbyMenu_MoveCursor(&gLobbyMenu->quitYesNo, 1, CURSOR_LEFT_RIGHT);
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
            if (gLobbyMenu->quitYesNo == YES_NO_YES) {
                gLobbyMenu->screen = SRV_PAGE_CHOOSE;
                gLobbyMenu->step = 2;
                gLobbyMenu->timer = SRV_CHOSEN_FRAMES;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
            } else {
                gLobbyMenu->screen = SRV_PAGE_CHOOSE;
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
s32 LobbySrv_ColorAt(s32 i);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);

#define SRV_TEXT_COLOR 0x80101010       /* the title and the servers' rows */
#define SRV_HEAD_COLOR 0xFF202060       /* the column heads */
#define SRV_RULE_COLOR 0xFF000000       /* the dashed line under the heads */
#define SRV_CURSOR_BAR_COLOR 0x99106060 /* the bar under the cursor's row */
#define SRV_CURSOR_COLOR 0xFF20C0C0     /* the cursor's arrow */
#define SRV_ICON_ALPHA 0xB0000000       /* over the server's colour (LbsRow.value) */
#define SPR_CURSOR_BAR 0x6F             /* Icon_Draw's sprite for the cursor's bar */

/*
 * Draws the server list at x, y: its frame, the title and the page
 * number, the icon in the colour of the server under the cursor
 * (LbsRow.value), the column heads (the name, the users), and the page's
 * rows (name, users/capacity), with the cursor's bar and blinking arrow.
 * x + 0x14 and its s16 + 0xB4 are written out at each use: kept in
 * locals, they were given other registers than the game's code uses.
 */
void SrvMenu_DrawList(s16 x, s16 y)
{
    s32 i;
    s32 idx;
    s16 lineY = y;
    s16 arrowShift;
    LbsRow *server;

    Icon_Draw(x, y, 0x10E, 0x130, -1, 0);
    lineY += 0xE;
    Font_SetColor(SRV_TEXT_COLOR);
    Font_SetPos(x + 0x14, lineY);
    Text_PrintfColored(gListTitles[LIST_TITLE_LAND_LIST]);
    Menu_DrawPageNum((s16)(x + 0x14) + 0xB4, lineY, gLobbyMenu->page, gLobbyMenu->pages, 1);
    LobbyUi_DrawIcon(x + 0x87, y + 0x98, 0x70, LobbySrv_ColorAt(gLobbyMenu->cursor + gLobbyMenu->page * SRV_MENU_PAGE_LEN) | SRV_ICON_ALPHA, 1);
    Font_SetColor(SRV_HEAD_COLOR);
    Font_SetPos(x + 0x14, lineY += 0x16);
    Text_PrintfColored(gListTitles[LIST_TITLE_LAND_NAME]);
    Font_SetPos((s16)(x + 0x14) + 0xB4, lineY);
    Text_PrintfColored(gCountLabels[COUNT_LABEL_PEOPLE]);
    Menu_DrawDashLine(x + 0x14, lineY + 0x10, 0xFA, SRV_RULE_COLOR);
    lineY += 4;
    Font_SetColor(SRV_TEXT_COLOR);
    x += 0x28;
    for (i = 0; i < SRV_MENU_PAGE_LEN; i++) {
        idx = i + gLobbyMenu->page * SRV_MENU_PAGE_LEN;
        if (idx >= SERVER_LIST_MAX || idx >= gLobbyServers.count) {
            break;
        }
        lineY += 0x16;
        if (i == gLobbyMenu->cursor) {
            arrowShift = (gFrameTick & 0x10) ? (s16)0 : (s16)6;     /* the arrow blinks every 16 frames */
            Icon_Draw(x - 0xA, lineY + 0xE, 0xE2, 8, SRV_CURSOR_BAR_COLOR, SPR_CURSOR_BAR);
            Menu_DrawCursorIcon(x - 0x1C - arrowShift, lineY - 2, 0x18, SRV_CURSOR_COLOR);
        }
        Font_SetPos(x, lineY);
        Font_SetColor(SRV_TEXT_COLOR);
        server = &gLobbyServers.row[idx];
        Text_PrintfColored("%s", server->text);
        Font_SetPos(x + 0x82, lineY);
        Text_PrintfColored("%4d/%4d", server->users, server->capacity);
    }
}

/* The menu's first draw hook: the location panel, sliding in from the
 * left while slideTimer runs. */
void SrvMenu_DrawInfo(MenuTask *task)
{
    s16 slide;
    s32 x;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    slide = gLobbyMenu->slideTimer;
    if (slide < 0) {
        x = 0x1A;
    } else {
        x = (s16)((0x14 - (slide * 0x30)));
    }
    LobbySrv_DrawServerPanel(x, 0x44);
}

/* The menu's second draw hook: the server list, sliding in with the
 * panel, then the title and the help line. */
void SrvMenu_DrawWindow(MenuTask *task)
{
    s16 slide;
    s16 x;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    slide = gLobbyMenu->slideTimer;
    if (slide < 0) {
        x = 0x2E;
    } else {
        x = 0x28 - (slide * 0x33);
    }
    SrvMenu_DrawList(x, 0x62);
    LobbyTitle_Draw();
    LobbyHelp_Draw();
}

/* The current server's name (the list's entry of gCurServerId), or NULL
 * when the list doesn't hold it. */
char *LobbySrv_MyName(void)
{
    s32 i;
    LbsRow *row;

    for (i = 0, row = gLobbyServers.row; i < SERVER_LIST_MAX; i++, row++) {
        if (memcmp(gCurServerId, row->key, SERVER_ID_LEN) == 0) {
            break;
        }
    }
    if (i <= SERVER_LIST_MAX - 1) {
        return gLobbyServers.row[i].text;
    }
    return NULL;
}

/* The name of the server whose id is id, or NULL when the list doesn't
 * hold it. */
char *LobbySrv_Name(char *id)
{
    s32 i;
    LbsRow *row;

    for (i = 0, row = gLobbyServers.row; i < SERVER_LIST_MAX; i++, row++) {
        if (memcmp(id, row->key, SERVER_ID_LEN) == 0) {
            break;
        }
    }
    if (i <= SERVER_LIST_MAX - 1) {
        return gLobbyServers.row[i].text;
    }
    return NULL;
}

/* The current server's index in the list (gCurServerId), or 0 when the
 * list doesn't hold it. */
s32 LobbySrv_CurIndex(void)
{
    s32 i;
    LbsRow *row;

    for (i = 0, row = gLobbyServers.row; i < SERVER_LIST_MAX; i++, row++) {
        if (memcmp(gCurServerId, row->key, SERVER_ID_LEN) == 0) {
            break;
        }
    }
    if (i <= SERVER_LIST_MAX - 1) {
        return i;
    }
    return 0;
}

s32 LobbySrv_IsCur(char *id);
s32 LobbySrv_HasCurName(void);
void LobbyDialog_Draw(void);
extern s8 gCurServerName;

/* 1 if id is the current server's, else 0. */
s32 LobbySrv_IsCur(char *id)
{
    return memcmp(gCurServerId, id, SERVER_ID_LEN) == 0;
}

/* The index of the server whose id is id, or 0 when the list doesn't
 * hold it. */
s32 LobbySrv_Index(char *id)
{
    s32 i;
    LbsRow *row;

    for (i = 0, row = gLobbyServers.row; i < SERVER_LIST_MAX; i++, row++) {
        if (memcmp(id, row->key, SERVER_ID_LEN) == 0) {
            break;
        }
    }
    if (i <= SERVER_LIST_MAX - 1) {
        return i;
    }
    return 0;
}

/* 1 if the list holds the server whose id is id, else 0. */
s32 LobbySrv_Exists(char *id)
{
    s32 i;
    LbsRow *row;

    for (i = 0, row = gLobbyServers.row; i < SERVER_LIST_MAX; i++, row++) {
        if (memcmp(id, row->key, SERVER_ID_LEN) == 0) {
            return 1;
        }
    }
    return 0;
}

/* The colour of the list's server i, or -1 past the list's end. */
s32 LobbySrv_ColorAt(s32 i)
{
    s32 ret;

    ret = -1;
    if (i < SERVER_LIST_MAX) {
        ret = gLobbyServers.row[i].value;
    }
    return ret;
}

/* The colour of the server whose id is id, or 0 when the list doesn't
 * hold it. */
s32 LobbySrv_Color(char *id)
{
    s32 i;
    LbsRow *row;

    for (i = 0, row = gLobbyServers.row; i < SERVER_LIST_MAX; i++, row++) {
        if (memcmp(id, row->key, SERVER_ID_LEN) == 0) {
            return gLobbyServers.row[i].value;
        }
    }
    return 0;
}

/* A menu's draw hook: draws the lobby's dialog when the menu asks for a
 * redraw, and clears the request. */
void LobbyDialog_DrawPending(MenuTask *task)
{
    u8 redraw;

    Text_SetSize(task->textSize);
    redraw = gLobbyMenu->redraw;
    if (redraw == 1) {
        LobbyDialog_Draw();
        gLobbyMenu->redraw = 0;
    }
}

/* 1 if the current server's name has arrived (its first byte is set). */
s32 LobbySrv_HasCurName(void)
{
    return gCurServerName != 0;
}

/* The current server's name. */
s8 *LobbySrv_CurName(void)
{
    return &gCurServerName;
}
