/*
 * The area list: the lobby screen between the server choice and the
 * village (LOBBY_PHASE_AREA_LIST). A server's first level lists its areas
 * (level 0, gLobbyLevel0Entries); the screen fetches the list
 * (LobbyStep_Level0List), groups the areas by kind into pages
 * (AreaList_Group), and lets the player choose one to enter, open the
 * friend list or ask to quit. A group joined from the group board or the
 * friend list (LobbyState.groupJoin) chooses its area by itself.
 *
 * The screen's work is the AreaList in gMenuWork; AreaList_Run's steps are
 * enum AreaListStep (lobby_menu.h). Entering the chosen area
 * (LobbyLogin_EnterEntry) starts the village (LobbyVillage_Begin). The
 * draws are the menu layers 0 and 1 (AreaList_DrawFrame, AreaList_DrawPanels):
 * the login screen's frame, the two server panels and the list.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "bgm.h"
#include "lobby_labels.h"
#include "chat_log.h"
#include "net_mode.h"
#include "scene.h"
#include "stream.h"
#include "area_list.h"
#include "font.h"
#include "friend_list.h"
#include "game_util.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "lobby_village.h"
#include "lobby_web_users.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "move_menu.h"
#include "name_tags.h"
#include "quit_menu.h"
#include "rand.h"
#include "screen_fade.h"
#include "libc.h"

extern MenuWork gMenuWork;
extern LobbyMenu *gLobbyMenu;

/*
 * Opens the area list: the lobby goes to LOBBY_PHASE_AREA_LIST at its
 * first step, the requests are dropped, the fetch's message shows, and the
 * menu work is cleared with the slide-in counter set.
 */
void AreaList_Begin(void)
{
    gLobby->phase = LOBBY_PHASE_AREA_LIST;
    gLobby->phaseStep = AREA_LIST_SETUP;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    Lobby_ClearRequests();
    LobbyText_Open(LOBBY_TEXT_AREA_LIST_FETCH);
    gLobbyMenu->redraw = 1;
    memset(&gMenuWork, 0, MENU_WORK_SIZE);
    gMenuWork.areaList.wait = AREA_LIST_SLIDE_FRAMES;
    LobbyHelp_SetBox(36, 362, 340);
}

/* The game built the functions below at optimization level 4, and
 * AreaList_Begin above at the default level. */
#pragma optimization_level 4

s32 LobbyNet_GetErrText(char *);
extern u16 gLobbyLevel0Count;

u32 LobbyLogin_LitColor(u8 slot); /* unit's own declaration: a u8 slot, as this file passes it; with the definition's s32 the call compiles differently */

void Text_SetSize(s32 size);
void LobbySrv_DrawServerPanel(s16 x, s16 y);
void Text_SetColor(s16 color);
void Text_PrintfColored(char *s, ...);
extern u8 gLobbyLevel0Sel;
extern s8 gLobbyLevel1Sel;
extern s8 gLobbyLevel2Sel;

extern LevelEntry gLobbyLevel0Entries[LEVEL0_ENTRY_MAX];   /* the areas, as sub_main keeps them */

/*
 * Runs the area list each frame, by the steps of enum AreaListStep: it
 * fetches and groups the areas, slides the panels in, takes the input,
 * shakes the chosen area's stamp, then enters it; a refusal or the
 * server's error text waits for PAD_CIRCLE. A failed fetch or an unknown
 * step leaves the lobby (LOBBY_QUIT_FAILED). The list is redrawn on every
 * step but the input's, which redraws it as the cursor moves.
 *
 * The list's work is reached here and in the other functions through a
 * cast of gMenuWork's address: with the member `&gMenuWork.areaList` three
 * of them compile differently.
 */
void AreaList_Run(void)
{
    AreaList *list = (AreaList *)&gMenuWork;

    switch (gLobby->phaseStep) {
    case AREA_LIST_SETUP:
        gLobby->phaseStep++;
        gLobby->seqStep = 0;
        AreaList_Init();
        break;
    case AREA_LIST_FETCH:
        switch (LobbyStep_Level0List()) {
        case LOBBY_STEP_DONE:
            gLobby->phaseStep++;
            gLobby->mode = AREA_LIST_MODE_LIST;
            LobbyHelp_SetText(QDL_AREA_KIND_HELP(0));
            LobbyHelp_SetBox(36, 362, 340);
            AreaList_Group(list);
            LobbySe_Play(LOBBY_SE_WINDOW_IN);
            list->wait = AREA_LIST_SLIDE_FRAMES;
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case AREA_LIST_SLIDE_IN:
        switch (AreaList_WaitSlide()) {
        case MENU_DONE:
            gLobby->phaseStep++;
            list->wait = 0;
            break;
        }
        break;
    case AREA_LIST_INPUT:
        switch (AreaList_Input()) {
        case MENU_DONE:
            gLobby->phaseStep++;
            list->shake = AREA_LIST_SHAKE_FRAMES;
            list->stampSeed = Rand_Next(1);
            LobbySe_Play(LOBBY_SE_STAMP);
            LobbyText_Open(LOBBY_TEXT_AREA_ENTERING);
            break;
        case MENU_UNDONE:
            gLobbyMenu->redraw = 1;
            gLobby->phaseStep = AREA_LIST_MESSAGE;
            break;
        case MENU_CANCEL:
            gLobby->phaseStep = AREA_LIST_QUIT_ASK;
            QuitMenu_Open();
            break;
        }
        break;
    case AREA_LIST_STAMP:
        switch (list->shake) {
        case 3:
            list->shakeX = -2;
            list->shakeY = 0;
            break;
        case 2:
            list->shakeX = 0;
            list->shakeY = 1;
            break;
        case 1:
            list->shakeX = 0;
            list->shakeY = 0;
            break;
        }
        if (--list->shake <= 0) {
            list->shakeX = 0;
            list->shakeY = 0;
            gLobby->phaseStep++;
        }
        break;
    case AREA_LIST_ENTER:
        switch (LobbyLogin_EnterEntry(LobbyLevel_SelId(0), 0)) {
        case LOBBY_STEP_DONE:
            LobbyVillage_Begin();
            LobbyMenu_SetDraw(0, 0, 0, 0);
            Stream_FadeOut(0, 15);  /* frames */
            break;
        case LOBBY_STEP_FAILED:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->phaseStep = AREA_LIST_ENTER_FAILED;
            gLobbyMenu->redraw = 1;
            break;
        }
        break;
    case AREA_LIST_QUIT_ASK:
        /* PAD_START is masked out: it would close the whole game menu */
        switch (QuitMenu_Update(Pad_GetCursorButtons(0) & ~PAD_START)) {
        case MENU_CANCEL:
            gLobby->phaseStep = AREA_LIST_INPUT;
            break;
        }
        break;
    case AREA_LIST_MESSAGE:
        gLobbyMenu->redraw = 1;
        if (gPadState[0].pressed & PAD_CIRCLE) {
            gLobby->phaseStep = AREA_LIST_INPUT;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            if (gLobby->groupJoin != 0) {
                memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            }
        }
        break;
    case AREA_LIST_ENTER_FAILED:
        gLobbyMenu->redraw = 1;
        if (gPadState[0].pressed & PAD_CIRCLE) {
            gLobby->phaseStep = AREA_LIST_SETUP;
            LobbyText_Open(LOBBY_TEXT_AREA_LIST_FETCH);
            LobbySe_Play(LOBBY_SE_CONFIRM);
            if (gLobby->groupJoin != 0) {
                memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            }
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    if (gLobby->phaseStep != AREA_LIST_INPUT) {
        gLobbyMenu->redraw = 1;
    }
    LoginWave_Update();
}

/*
 * Sets the screen up: a running fade is ended with fade 2, the chat log
 * and the three levels' choices are cleared, the areas' records are
 * emptied, the list's music starts, and the menu layers, the title and
 * the login screen's wave (side 1) are set.
 */
void AreaList_Init(void)
{
    s32 i;

    if (FadeA_GetState() & 0xFF) {
        FadeA_Start(2);
    }
    ChatLog_Clear();
    gLobbyLevel0Sel = 0;
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(&gLobbyLevel0Entries, 0, LEVEL0_ENTRY_MAX * sizeof(LevelEntry));
    Bgm_PlayWithVolume(BGM_AREA_LIST, 30);
    for (i = 0; i < LEVEL0_ENTRY_MAX; i++) {
        gLobbyLevel0Entries[i].state = 0;
    }
    LobbyMenu_Init();
    gLobby->connected = 1;
    gLobbyMenu->promptStep = 0;
    LobbyMenu_SetDraw(AreaList_DrawFrame, AreaList_DrawPanels, 0, 0);
    LobbyTitle_Set(LOBBY_TITLE_AREA_LIST);
    gLobby->unk393F = 0;
    LoginWave_Clear();
    LoginWave_Mark(1);
}

/*
 * Groups the fetched areas by kind, in the list's order: each kind gets a
 * page, and a second page once it has AREA_GROUP_LEN areas. With a group
 * join pending, the cursor goes to the joined area's page and place; a
 * join without an area is dropped.
 */
void AreaList_Group(AreaList *list)
{
    s32 kind;
    u16 areaCount;
    s32 i;
    s32 *kindCount;
    s32 group;
    s32 index;

    list->groupCount = 0;
    memset(list->kindCounts, 0, sizeof(list->kindCounts));
    memset(list->groups, 0, sizeof(list->groups));
    areaCount = gLobbyLevel0Count;
    for (i = 0; i < areaCount; i++) {
        kind = gLobbyLevel0Entries[i].kind - 1;
        kindCount = &list->kindCounts[kind];
        if (*kindCount == 0 || *kindCount == AREA_GROUP_LEN) {
            list->groups[list->groupCount].kind = kind;
            list->groupCount++;
        }
        (*kindCount)++;
        list->groups[list->groupCount - 1].ids[list->groups[list->groupCount - 1].count] = gLobbyLevel0Entries[i].id;
        list->groups[list->groupCount - 1].count++;
    }
    if (gLobby->groupJoin != 0) {
        if (gLobby->area == 0) {
            gLobby->groupJoin = 0;
            memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            return;
        }
        for (group = 0; group < list->groupCount; group++) {
            for (index = 0; index < list->groups[group].count; index++) {
                if (list->groups[group].ids[index] == gLobby->area) {
                    list->group = group;
                    list->index = index;
                }
            }
        }
    }
}

/* Counts the slide-in counter (AreaList.wait) down below 0, then returns
 * MENU_DONE a frame later with it at 0; MENU_RUNNING before. */
s32 AreaList_WaitSlide(void)
{
    AreaList *list = (AreaList *)&gMenuWork;

    switch (gLobby->mode) {
    case 0:
        if (--list->wait < 0) {
            gLobby->mode++;
        }
        break;
    case 1:
        list->wait = 0;
        gLobby->mode = 0;
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * The list's input. PAD_CIRCLE, or a pending group join (its area chosen
 * first; a join without a land is dropped), takes the area under the
 * cursor: refused for the hunter rank or when it isn't open (a message,
 * MENU_UNDONE), else it becomes gLobbyLevel0Sel (MENU_DONE). PAD_CROSS
 * asks to quit (MENU_CANCEL) when the player has another server to go
 * to; PAD_SQUARE opens the friend list; left and right turn the page, up
 * and down move on it, and the help line follows the page's kind. While
 * the friend list is open, a group joined from it chooses its area.
 * Returns MENU_RUNNING otherwise.
 */
s32 AreaList_Input(void)
{
    AreaList *list = (AreaList *)&gMenuWork;
    s16 group;
    u16 pad;
    u8 kind;
    s32 j;
    s32 i;
    s32 idx;

    pad = Pad_GetCursorButtons(0);
    group = list->group;
    kind = list->groups[group].kind;
    switch (gLobby->mode) {
    case AREA_LIST_MODE_LIST:
        if ((pad & PAD_CIRCLE) || gLobby->groupJoin != 0) {
            if (gLobby->groupJoin != 0) {
                for (i = 0; i < list->groupCount; i++) {
                    for (j = 0; j < list->groups[i].count; j++) {
                        if (list->groups[i].ids[j] == gLobby->area) {
                            list->group = i;
                            list->index = j;
                        }
                    }
                }
                if (gLobby->land == 0) {
                    gLobby->groupJoin = 0;
                    memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
                }
            }
            if (!Lobby_CanEnterByRank(list->groups[list->group].ids[list->index], 0)) {
                LobbyText_Open(LOBBY_TEXT_AREA_RANK);
                LobbySe_Play(LOBBY_SE_REFUSE);
                return MENU_UNDONE;
            }
            idx = list->groups[list->group].ids[list->index] - 1;
            if (gLobbyLevel0Entries[idx].state == LEVEL_ENTRY_OPEN) {
                gLobbyLevel0Sel = idx + 1;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
            LobbyText_Open(LOBBY_TEXT_AREA_FULL);
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_UNDONE;
        }
        if (pad & PAD_CROSS) {
            if (gLobbyServerCount > 1) {
                LobbySe_Play(LOBBY_SE_CANCEL);
                return MENU_CANCEL;
            }
        } else if (pad & PAD_SQUARE) {
            gLobby->mode++;
            FriendList_Open(0);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            list->group = Pad_StepWrap(group, list->groupCount, pad);
            if (group != list->group) {
                LobbySe_Play(LOBBY_SE_PAGE);
                list->index = 0;
            }
        } else {
            list->index = Menu_WrapUpDownPad(list->index, list->groups[group].count, pad);
        }
        if (kind != list->groups[list->group].kind) {
            LobbyHelp_SetText(QDL_AREA_KIND_HELP(list->groups[list->group].kind));
            LobbyHelp_SetBox(36, 362, 340);
        }
        break;
    case AREA_LIST_MODE_FRIENDS:
        switch (FriendList_Update(pad)) {
        case MENU_CLOSE:
            LobbySe_Play(LOBBY_SE_CANCEL);
        case MENU_CANCEL:
            gLobby->mode = AREA_LIST_MODE_LIST;
        }
        if (gLobby->groupJoin != 0) {
            gLobby->mode = AREA_LIST_MODE_LIST;
            for (i = 0; i < list->groupCount; i++) {
                for (j = 0; j < list->groups[i].count; j++) {
                    if (list->groups[i].ids[j] == gLobby->area) {
                        list->group = i;
                        list->index = j;
                    }
                }
            }
            LobbyHelp_SetText(QDL_AREA_KIND_HELP(list->groups[list->group].kind));
        }
        break;
    }
    return MENU_RUNNING;
}

/* An area kind's colour (kind from 1): the login screen's colour of its
 * page, at half alpha. */
u32 AreaList_KindColor(u8 kind)
{
    return (LobbyLogin_LitColor(kind - 1) & 0xFFFFFF) | 0x40000000;
}

/*
 * Draws the list's window at x, y: its frame, the icon of the area under
 * the cursor in its kind's colour (with the stamp while it is chosen and
 * entered), the page number, the title and the headings, then a line per
 * area of the page, its name and users of the maximum, with the cursor's
 * bar and blinking mark on the cursor's line. The text's x + 20 and
 * y + 20 stay ints, narrowed at each call, as the game's code has them.
 */
void AreaList_DrawEntries(s16 x, s16 y)
{
    AreaList *list = (AreaList *)&gMenuWork;
    LevelEntry *area;
    s32 textY;
    s32 numX;
    u32 color;
    s16 blink;
    s32 i;
    s16 lineY = y;

    Icon_Draw(x, y, 270, 304, -1, 0);
    area = &gLobbyLevel0Entries[list->groups[list->group].ids[list->index] - 1];
    color = AreaList_KindColor(area->kind);
    LobbyUi_DrawLevelMarks((s16)(x + 135), (s16)(y + 152), 112, color, 2);
    switch (gLobby->phaseStep) {
    case AREA_LIST_STAMP:
    case AREA_LIST_ENTER:
        Menu_DrawStamp(x, y, 270, 304, (u16)list->stampSeed, list->shake);
        break;
    }
    textY = y + 20;
    numX = (s16)(x + 20) + 180;
    Menu_DrawPageNum(numX, textY, list->group, list->groupCount, 1);
    Font_SetPos(x + 20, textY);
    Text_PrintfColored("%s", gListTitles[LIST_TITLE_AREA_LIST]);
    Font_SetColor(0xFF202060);
    textY = y + 40;
    Font_SetPos(x + 20, textY);
    Text_PrintfColored("%s", gListTitles[LIST_TITLE_AREA_NAME]);
    Font_SetPos(numX, textY);
    Text_PrintfColored(gCountLabels[COUNT_LABEL_PEOPLE]);
    Menu_DrawDashLine(x + 20, (s16)textY + 16, 250, 0xFF000000);
    lineY += 44;
    Font_SetColor(0x80101010);
    for (i = 0; i < list->groups[list->group].count; i++) {
        area = &gLobbyLevel0Entries[list->groups[list->group].ids[i] - 1];
        lineY += 20;
        Font_SetPos(x + 20, lineY);
        if (i == list->index) {
            if (gFrameTick & 0x10) {
                blink = 0;
            } else {
                blink = 6;
            }
            Icon_Draw(x + 20, lineY + 14, 226, 8, 0x99106060, 111);
            Menu_DrawCursorIcon((s16)(x + 20) - 24 - blink, lineY - 2, 24, 0xFF20C0C0);
        }
        Text_PrintfColored("%s", area->name);
        Font_SetPos((s16)(x + 20) + 170, lineY);
        Text_PrintfColored("%3d/%3d", area->users, area->maxUsers);
    }
}

/*
 * Menu layer 0: the login screen's frame (LobbyLogin_DrawMenu) and the
 * panel of the server under the server menu's cursor (LobbySrv_DrawServerPanel),
 * which slides in from the left with the counter AreaList.wait (x = 20 -
 * 48 * it, 26 once it is out). The panel isn't drawn while the list
 * is fetched or the friend list is open.
 */
void AreaList_DrawFrame(MenuTask *layer)
{
    s16 x;
    u8 step;

    Text_SetSize(layer->textSize);
    Font_SetSpacing(18, 18);
    LobbyLogin_DrawMenu();
    if (((AreaList *)&gMenuWork)->wait < 0) {
        x = 26;
    } else {
        x = 20 - ((AreaList *)&gMenuWork)->wait * 48;
    }
    step = gLobby->phaseStep;
    if (step == AREA_LIST_INPUT && gLobby->mode == AREA_LIST_MODE_FRIENDS) {
        return;
    }
    if (step == AREA_LIST_SETUP || step == AREA_LIST_FETCH) {
        return;
    }
    LobbySrv_DrawServerPanel(x, 68);
}

/*
 * Menu layer 1, at the same steps: the current server's name panel
 * (LobbySrv_DrawListNamePanel), sliding in with the counter (x = 40 - 51 *
 * it, 46 once it is out), then the list (AreaList_DrawList).
 */
void AreaList_DrawPanels(MenuTask *layer)
{
    AreaList *list = (AreaList *)&gMenuWork;
    s16 x;
    u8 step;

    step = gLobby->phaseStep;
    if (step == AREA_LIST_INPUT && gLobby->mode == AREA_LIST_MODE_FRIENDS) {
        return;
    }
    if (step == AREA_LIST_SETUP || step == AREA_LIST_FETCH) {
        return;
    }
    Text_SetSize(layer->textSize);
    Font_SetSpacing(18, 18);
    if (list->wait < 0) {
        x = 46;
    } else {
        x = 40 - list->wait * 51;
    }
    LobbySrv_DrawListNamePanel(x, 98);
    AreaList_DrawList();
}

/*
 * The button guide at the foot of the screen: a bar from the text's left
 * edge (568 less 9 per character, Lobby_StrLength) to the screen's
 * right, a mark (Lobby_DrawButtonIcon) and the guide's text.
 */
void AreaList_DrawGuide(void)
{
    s16 x;
    f32 leftEdge;

    x = 568 - Lobby_StrLength(gGameMenuLabels[MENU_LABEL_FRIEND_LIST]) * 9;
    leftEdge = x;
    Gfx_DrawPulseGradient(leftEdge, 398, 640.0f - leftEdge, 22, 0x80006000, 1);
    Lobby_DrawButtonIcon(x + 4, 398, 3);
    Font_SetPos(x + 38, 400);
    Font_SetSpacing(18, 18);
    Text_SetColor(0);
    Text_PrintfColored(gGameMenuLabels[MENU_LABEL_FRIEND_LIST]);
}

/*
 * The list's window (AreaList_DrawEntries), sliding in with the counter
 * (x = 30 - 50 * it, 36 once it is out) and moved by the stamp's
 * shake; the button guide while no area is chosen and no join is pending;
 * the help line from the input on.
 */
void AreaList_DrawList(void)
{
    AreaList *list = (AreaList *)&gMenuWork;
    s16 x;

    Font_SetSpacing(18, 18);
    if (list->wait < 0) {
        x = 36;
    } else {
        x = 30 - list->wait * 50;
    }
    AreaList_DrawEntries(x + list->shakeX, list->shakeY + 128);
    if (gLobby->groupJoin == 0 && gLobbyLevel0Sel == 0) {
        AreaList_DrawGuide();
    }
    if (gLobby->phaseStep >= AREA_LIST_INPUT) {
        LobbyHelp_Draw();
    }
}

/*
 * The request step that fetches the areas: it starts the level-0 list task
 * (NET_TASK_LEVEL0_LIST) for every request of an area (NET_LEVEL0_REQS)
 * and waits for LobbyStep_Level0ListDone. Returns LOBBY_STEP_DONE,
 * LOBBY_STEP_FAILED or LOBBY_STEP_BUSY.
 */
s32 LobbyStep_Level0List(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL0_LIST;
        NetLevel0ListTask_Start(0, NET_LEVEL0_REQS, LobbyStep_Level0ListDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * The level-0 list task's end: done, the areas are copied from the
 * network's list into gLobbyLevel0Entries, ids from 1; failed, the
 * server's error text is kept and the step fails.
 */
void LobbyStep_Level0ListDone(NetMsg msg, NetMsg *msgPtr)
{
    u16 i;
    LevelEntry *area;
    s32 id;
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL0_LIST && msg.kind != NET_MSG_PROGRESS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->seqStep++;
                NetList0_GetCount(&gLobbyLevel0Count);
                i = 0;
                if (i < gLobbyLevel0Count) {
                    area = gLobbyLevel0Entries;
                    do {
                        id = i + 1;
                        area->id = id;
                        NetList0_GetByte(id, &area->state);
                        NetList0_GetUsers(id, &area->users, &area->questUsers);
                        NetList0_GetName(id, area->name);
                        NetList0_GetText(id, area->text);
                        NetList0_GetMaxUsers(id, &area->maxUsers);
                        NetList0_GetValue10(id, &area->unk14);
                        NetList0_GetKind(id, &area->kind);
                        i++;
                        area++;
                    } while (i < gLobbyLevel0Count);
                }
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText(gLobby->errText);
            }
        }
    }
}
