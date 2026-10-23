/*
 * The move menu (TOWN_MODE_MOVE_MENU): from the town, the player chooses
 * between moving to another area of the lobby server and leaving the
 * lobby. Moving opens the areas' pages, grouped by kind as the area list
 * groups them (AreaList_Group), with three panels sliding in from the
 * right: the server's name, the player's server and the areas' page. The
 * chosen area is stamped, the screen fades out and LobbyLogin_EnterEntry
 * enters it; leaving asks again, then disconnects.
 *
 * The menu's work is a MoveMenu in gMenuWork (lobby_menu.h). The two
 * server panels are drawn here for the server menu and the area list too.
 */
#include "common.h"
#include "lobby_state.h"
#include "scene.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "lobby_text.h"
#include "menu_task.h"
#include "pad.h"
#include "chat_log.h"
#include "lobby_servers.h"
#include "stream.h"
#include "text.h"
#include "move_menu.h"
#include "area_list.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_disconnect.h"
#include "lobby_util.h"
#include "lobby_village.h"
#include "lobby_web_users.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "quit_menu.h"
#include "rand.h"
#include "screen_fade.h"
#include "libc.h"

#define SERVER_PANEL_W 270
#define SERVER_PANEL_H 304

extern u8 gMenuWork;
void LobbySrv_DrawServerPanel(s32, s32); /* unit's own declaration: the definition is in the old (K&R) style, so its short parameters arrive as ints, as here; a prototype with short parameters doesn't agree with such a definition */
void Text_PrintfColored(const char *, ...);
extern char gMoveMenuTextFmt[];
void LobbyHelp_Draw(void);
char *LobbySrv_CurName(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s16 gHouseCloseTimer;
extern s8 gMenuInputLock;
extern u8 gLobbyLevel0Sel;
void LobbyHelp_SetText(void *text);
void LobbyHelp_SetBox(s16 x, s16 y, s16 w);

void Menu_SetYesNo(u8 a); /* unit's own declaration: declared here as the callers use it; with the definition's void (s8 choice), MoveMenu_Update compiles differently */
void LobbyDialog_Grow(void);
void LobbyText_Open(s32);
extern LobbyMenu *gLobbyMenu;
#pragma rodata on
/* the plain string format ("%s") the panels print their texts with */
char gMoveMenuTextFmt[8] = {
#include "lobby/00838298.inc"
};
/* an area's line: its users and its limit */
char gMoveMenuUsersFmt[16] = {
#include "lobby/008382A0.inc"
};
#pragma rodata off

/* Opens the move menu from the town: its question (LOBBY_TEXT_MOVE_ASK)
 * with the cursor on MOVE_CHOICE_AREA. */
void MoveMenu_Open(void)
{
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_MOVE_MENU;
    memset(&gMenuWork, 0, sizeof(MenuWork));
    LobbyMenu_Init();
    LobbyMenu_SetDraw(MoveMenu_DrawBack, MoveMenu_DrawFront, 0, 0);
    LobbyText_Open(LOBBY_TEXT_MOVE_ASK);
    LobbyDialog_Grow();
    Menu_SetYesNo(MOVE_CHOICE_AREA);
    LobbySe_Play(LOBBY_SE_CONFIRM);
}

/* Closes the move menu: the town takes the pad again after 3 frames. */
void MoveMenu_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    gMenuInputLock = 3;
}

/* Built at optimization level 4 from here on: MoveMenu_DrawAreaList's
 * code is the game's only at that level; the rest compile the same at
 * both. */
#pragma optimization_level 4

/*
 * Runs the move menu each frame (TOWN_MODE_MOVE_MENU), by MoveMenu.step:
 * the question (MOVE_CHOICE_AREA opens the areas' pages, MOVE_CHOICE_LEAVE
 * asks whether to leave, cross closes the menu); the pages, whose chosen
 * area starts the town's entry (LobbyVillage_BeginStay), cancel coming back
 * to the question; and the leave question, whose yes disconnects with the
 * screen black and the streams stopped.
 */
void MoveMenu_Update(void)
{
    MoveMenu *menu = (MoveMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);
    s16 old;

    switch (menu->step) {
    case MOVE_MENU_ASK:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            switch (menu->choice) {
            case MOVE_CHOICE_LEAVE:
                LobbyText_Open(LOBBY_TEXT_LEAVE_ASK);
                LobbyDialog_Grow();
                Menu_SetYesNo(1);
                menu->step = MOVE_MENU_LEAVE_ASK;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case MOVE_CHOICE_AREA:
                menu->step = MOVE_MENU_PICK;
                MoveMenu_OpenPick();
                break;
            default:
                MoveMenu_Close();
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            MoveMenu_Close();
        } else {
            old = menu->choice;
            menu->choice = Menu_WrapUpDown(old, 2);
            if (old != menu->choice) {
                Menu_SetYesNo(menu->choice);
            }
        }
        break;
    case MOVE_MENU_PICK:
        switch (MoveMenu_PickInput(pad)) {
        case MENU_DONE:
            LobbyVillage_BeginStay();
            LobbyMenu_SetDraw(0, 0, 0, 0);
            break;
        case MENU_CANCEL:
            LobbyText_Open(LOBBY_TEXT_MOVE_ASK);
            LobbyDialog_Grow();
            Menu_SetYesNo(menu->choice);
            menu->step = MOVE_MENU_ASK;
            break;
        }
        break;
    case MOVE_MENU_LEAVE_ASK:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            Lobby_Disconnect(0);
            FadeA_Start(FADE_BLACK);
            Stream_Stop(0);
            Stream_Stop(1);
            break;
        case MENU_CANCEL:
            LobbyText_Open(LOBBY_TEXT_MOVE_ASK);
            LobbyDialog_Grow();
            Menu_SetYesNo(menu->choice);
            menu->step = MOVE_MENU_ASK;
            break;
        }
        break;
    case MOVE_MENU_QUIT:
        switch (QuitMenu_Update(pad)) {
        case MENU_CANCEL:
            LobbyText_Open(LOBBY_TEXT_MOVE_ASK);
            LobbyDialog_Grow();
            Menu_SetYesNo(menu->choice);
            menu->step = MOVE_MENU_ASK;
            break;
        }
        break;
    }
}

/* Opens the areas' pages: groups the fetched areas (AreaList_Group), starts
 * the panels' slide, puts the cursor on the area the player is in
 * (gLobbyLevel0Sel) and shows the help text of that area's kind. The town
 * ignores the pad for 10 frames. */
void MoveMenu_OpenPick(void)
{
    MoveMenu *menu = (MoveMenu *)&gMenuWork;
    s16 group;
    s16 i;

    gMenuInputLock = 10;
    AreaList_Group((AreaList *)menu);
    menu->slideStep = MOVE_MENU_SLIDE_STEP;
    menu->slide = AREA_LIST_SLIDE_FRAMES;
    LobbySe_Play(LOBBY_SE_WINDOW_IN);
    for (group = 0; group < menu->list.groupCount; group++) {
        for (i = 0; i < menu->list.groups[group].count; i++) {
            if (menu->list.groups[group].ids[i] == gLobbyLevel0Sel) {
                menu->list.group = group;
                menu->list.index = i;
            }
        }
    }
    LobbyHelp_SetText(QDL_AREA_KIND_HELP(menu->list.groups[menu->list.group].kind));
    LobbyHelp_SetBox(320, 362, 320);
}

void LobbyDialog_OpenWait(void *text);
/*
 * The areas' pages' input (MoveMenu.pick, enum MovePickStep): waits for the
 * slide; then left and right turn the page, up and down move the cursor,
 * and circle takes the area, unless the player's hunter rank doesn't allow
 * it (Lobby_CanEnterByRank's message) or the player is already there
 * (LOBBY_TEXT_AREA_CURRENT). A taken area's stamp shakes, the screen fades
 * out and LobbyLogin_EnterEntry enters it; a failure shows the server's
 * text and fades back in. The help text follows the kind of the cursor's
 * page. Returns MENU_DONE once the area is entered, MENU_CANCEL on cross,
 * MENU_RUNNING before.
 */
s32 MoveMenu_PickInput(u16 pad)
{
    MoveMenu *menu = (MoveMenu *)&gMenuWork;
    s32 id = menu->list.groups[menu->list.group].ids[menu->list.index];
    u8 kind = menu->list.groups[menu->list.group].kind;
    s16 old;

    switch (menu->pick) {
    case MOVE_PICK_SLIDE_IN:
        if (--menu->slide < 0) {
            menu->pick++;
        }
        /* an empty check of the step's range: the game's code was built
         * with it, and the step's code compiles differently without it */
        if (menu->pick < 0 || menu->pick > MOVE_PICK_MESSAGE) {}
        break;
    case MOVE_PICK_INPUT:
        menu->slide = 0;
        if (pad & PAD_CIRCLE) {
            if (Lobby_CanEnterByRank(id, 0) == 0) {
                menu->pick = MOVE_PICK_MESSAGE;
                LobbyDialog_Grow();
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else if (gLobbyLevel0Sel == id) {
                menu->pick = MOVE_PICK_MESSAGE;
                LobbyText_Open(LOBBY_TEXT_AREA_CURRENT);
                LobbyDialog_Grow();
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else {
                menu->pick++;
                gLobby->seqStep = 0;
                menu->list.shake = AREA_LIST_SHAKE_FRAMES;
                menu->list.stampSeed = Rand_Next(1);
                LobbySe_Play(LOBBY_SE_STAMP);
                LobbyText_Open(LOBBY_TEXT_TOWN_ENTERING);
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            menu->pick = MOVE_PICK_SLIDE_IN;
            return MENU_CANCEL;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            old = menu->list.group;
            menu->list.group = Menu_WrapLeftRight(old, menu->list.groupCount);
            if (old != menu->list.group) {
                menu->list.index = 0;
            }
        } else {
            menu->list.index = Menu_WrapUpDown(menu->list.index, menu->list.groups[menu->list.group].count);
        }
        if (kind != menu->list.groups[menu->list.group].kind) {
            LobbyHelp_SetText(QDL_AREA_KIND_HELP(menu->list.groups[menu->list.group].kind));
            LobbyHelp_SetBox(320, 362, 320);
        }
        break;
    case MOVE_PICK_STAMP:
        switch (menu->list.shake) {
        case 3:
            menu->list.shakeX = -2;
            menu->list.shakeY = 0;
            break;
        case 2:
            menu->list.shakeX = 0;
            menu->list.shakeY = 1;
            break;
        case 1:
            menu->list.shakeX = 0;
            menu->list.shakeY = 0;
            break;
        }
        if (--menu->list.shake <= 0) {
            menu->list.shakeX = 0;
            menu->list.shakeY = 0;
            menu->pick++;
            FadeB_Start(FADE_OUT_BLACK);
        }
        break;
    case MOVE_PICK_ENTER:
        gLobbyMenu->redraw = 1;
        if (FadeB_GetState() != FADE_STATE_RUNNING) {
            switch (LobbyLogin_EnterEntry(id, 0)) {
            case LOBBY_STEP_DONE:
                menu->pick = MOVE_PICK_SLIDE_IN;
                ChatLog_Clear();
                return MENU_DONE;
            case LOBBY_STEP_FAILED:
                menu->pick = MOVE_PICK_MESSAGE;
                LobbyDialog_OpenWait(gLobby->errText);
                LobbyDialog_Grow();
                FadeB_Start(FADE_IN_BLACK);
                break;
            }
        }
        break;
    case MOVE_PICK_MESSAGE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            menu->pick = MOVE_PICK_INPUT;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Draws ICON_SERVER_WAIT on the server panel at (x, y), at twice its size.
 * w is unused (the callers pass the panel's width). */
void LobbySrv_DrawWaitMark(s16 x, s16 y, s16 w)
{
    IconSprite *sprite = &gIconSprites[ICON_SERVER_WAIT];
    s16 width = sprite->uv1.u - sprite->uv0.u;
    s16 height = sprite->uv1.v - sprite->uv0.v;

    Icon_Draw(x + 12, y + 16, width * 2, height * 2, 0xA0987222, ICON_SERVER_WAIT);
}

/*
 * Draws the server panel at (x, y): the current server's name, as the
 * server name request gave it (LobbySrv_CurName); until it has arrived,
 * the panel is dimmed and marked (LobbySrv_DrawWaitMark).
 *
 * Its definition is in the old (K&R) style: the callers pass x and y as
 * ints, as its declaration above says, and the body uses them as shorts
 * without narrowing them, as the game's code does. A prototype with short
 * parameters changes the callers, and int parameters narrowed in the body
 * change this function.
 */
void LobbySrv_DrawServerPanel(x, y)
s16 x;
s16 y;
{
    if (LobbySrv_HasCurName() == 1) {
        Icon_Draw(x, y, SERVER_PANEL_W, SERVER_PANEL_H, -1, ICON_PANEL_SERVER);
    } else {
        Icon_Draw(x, y, SERVER_PANEL_W, SERVER_PANEL_H, 0xFF407287, ICON_PANEL_SERVER);
        LobbySrv_DrawWaitMark(x, y, SERVER_PANEL_W);
        return;
    }
    Font_SetColor(0x80101010);
    Font_SetPos(x + 20, y + 10);
    Text_PrintfColored(gMoveMenuTextFmt, LobbySrv_CurName());
}

/* Draws the panel of the current server's name as the server list gives
 * it (LobbySrv_MyName) at (x, y). */
void LobbySrv_DrawListNamePanel(s16 x, s16 y)
{
    Icon_Draw(x, y, SERVER_PANEL_W, SERVER_PANEL_H, -1, ICON_PANEL_SERVER);
    Font_SetColor(0x80101010);
    Font_SetPos(x + 20, y + 10);
    Text_PrintfColored(gMoveMenuTextFmt, LobbySrv_MyName());
}

extern LevelEntry gLobbyLevel0Entries[];

/*
 * Draws the areas' panel at (x, y): the page's kind mark in the kind's
 * colour, the chosen area's stamp while it shakes, the page number, the
 * heading and the page's areas, each with its users and limit. The cursor's
 * line has a bar and a blinking arrow; the area the player is in is grey.
 *
 * FAKEMATCH: the loop's bound reads the page through a second pointer to
 * the same work (`w`). Through `m`, the compiler keeps the page's offset
 * from the loop's previous test, where the game's code reads the page
 * again at each test.
 */
void MoveMenu_DrawAreaList(s16 x, s16 y)
{
    MoveMenu *menu = (MoveMenu *)&gMenuWork;
    MoveMenu *sameMenu = (MoveMenu *)&gMenuWork;
    LevelEntry *entry;
    s16 lineY = y;
    s16 blink;
    s16 i;
    u32 color;

    Icon_Draw(x, y, SERVER_PANEL_W, SERVER_PANEL_H, -1, ICON_PANEL);
    entry = &gLobbyLevel0Entries[menu->list.groups[menu->list.group].ids[0] - 1];
    color = AreaList_KindColor(entry->kind);
    LobbyUi_DrawLevelMarks(x + 135, y + 152, 112, color, 2);
    if (menu->step == MOVE_MENU_PICK && (menu->pick == MOVE_PICK_STAMP || menu->pick == MOVE_PICK_ENTER)) {
        Menu_DrawStamp(x, y, SERVER_PANEL_W, SERVER_PANEL_H, (u16)menu->list.stampSeed, menu->list.shake);
    }
    x += 20;
    Menu_DrawPageNum(x + 180, y + 10, menu->list.group, menu->list.groupCount, 1);
    Font_SetColor(0x80101010);
    Font_SetPos(x, y + 10);
    Text_PrintfColored(gMoveMenuTextFmt, gListTitles[LIST_TITLE_AREA_LIST]);
    Font_SetColor(0xFF202060);
    y += 30;
    Font_SetPos(x, y);
    Text_PrintfColored(gMoveMenuTextFmt, gListTitles[LIST_TITLE_AREA_NAME]);
    Font_SetPos(x + 180, y);
    Text_PrintfColored(gCountLabels[COUNT_LABEL_PEOPLE]);
    Menu_DrawDashLine(x, y + 16, 250, 0xFF000000);
    lineY += 34;
    for (i = 0; i < menu->list.groups[sameMenu->list.group].count; i++) {
        entry = &gLobbyLevel0Entries[menu->list.groups[menu->list.group].ids[i] - 1];
        lineY += 20;
        Font_SetPos(x, lineY);
        if (i == menu->list.index) {
            if (gFrameTick & 0x10) {
                blink = 0;
            } else {
                blink = 6;
            }
            Icon_Draw(x, lineY + 14, 226, 8, 0x99106060, ICON_CURSOR_BAR);
            Menu_DrawCursorIcon(x - 26 - blink, lineY - 2, 24, 0xFF20C0C0);
        }
        if (gLobbyLevel0Sel == menu->list.groups[menu->list.group].ids[i]) {
            Font_SetColor(0x80404040);
        } else {
            Font_SetColor(0x80101010);
        }
        Text_PrintfColored(gMoveMenuTextFmt, entry->name);
        Font_SetPos(x + 170, lineY);
        Text_PrintfColored(gMoveMenuUsersFmt, entry->users, entry->maxUsers);
    }
}

/* The move menu's layer 0: on the areas' pages, the server panel, sliding
 * in from the right (MoveMenu.slide frames of MoveMenu.slideStep pixels). */
void MoveMenu_DrawBack(MenuTask *task)
{
    MoveMenu *menu = (MoveMenu *)&gMenuWork;
    s32 slide;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    slide = menu->slide;
    switch (menu->step) {
    case MOVE_MENU_PICK:
        LobbySrv_DrawServerPanel(slide < 0 ? 314 : (s16)(menu->slideStep * slide + 320), 54);
        break;
    }
}

/* The move menu's layer 1: on the areas' pages, the server list's name
 * panel over the server panel, then the areas' panel
 * (MoveMenu_DrawAreaPage), each sliding in as MoveMenu_DrawBack's. */
void MoveMenu_DrawFront(MenuTask *task)
{
    MoveMenu *menu = (MoveMenu *)&gMenuWork;
    s16 x;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    if (menu->slide < 0) {
        x = 334;
    } else {
        x = menu->slideStep * menu->slide + 340;
    }
    switch (menu->step) {
    case MOVE_MENU_PICK:
        LobbySrv_DrawListNamePanel(x, 84);
        break;
    }
    MoveMenu_DrawAreaPage(task);
}

/* Draws the areas' panel at its slide's place, offset by the stamp's
 * shake, and the help text of the page's kind while the cursor moves. */
void MoveMenu_DrawAreaPage(MenuTask *task)
{
    MoveMenu *menu = (MoveMenu *)&gMenuWork;
    s16 x;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    if (menu->slide < 0) {
        x = 324;
    } else {
        x = menu->slideStep * menu->slide + 330;
    }
    switch (menu->step) {
    case MOVE_MENU_PICK:
        MoveMenu_DrawAreaList(x + menu->list.shakeX, menu->list.shakeY + 114);
        break;
    }
    if (menu->step == MOVE_MENU_PICK && menu->pick == MOVE_PICK_INPUT) {
        LobbyHelp_Draw();
    }
}
