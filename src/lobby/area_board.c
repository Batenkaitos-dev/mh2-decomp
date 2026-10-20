/*
 * The area board (TOWN_MODE_AREA_BOARD): the board at a town's board spot
 * when the player's area holds rooms (LobbyLevel0_HasGroups picks between
 * it and the group board). It lists the area's level-1 entries
 * (gLobbyLevel1Entries) AREA_BOARD_ROWS_PER_PAGE to a page. An open entry
 * is joined after its members are shown (AreaBoard_Join); a free one is
 * opened as the player's own with a recruiting text picked from the room
 * data's slots (RoomData), against a time limit (gRecruitFrames). Either
 * way the board is stamped, the quests are downloaded and the town is
 * entered.
 *
 * Its work is an AreaBoardMenu (lobby_menu.h) in gMenuWork. The group
 * board's work starts with the same layout, and the group board runs this
 * file's join, text picker (AreaBoard_LoadPage, AreaBoard_DrawTextPicker),
 * entry release (AreaBoard_LeaveEntry) and members panel on it. The group
 * board's settings sender (GroupBoard_SendSettings) sits here between the
 * area board's functions.
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "pad.h"
void *memset(void *, s32, u32);
void LobbyText_Open(s32 msg);

u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Win_Shake(WinShake *s);
s32 LobbyStep_DownloadQuests(void);
void LobbyTown_Begin(void);
void Stream_Stop(s32 ch);
void AreaBoard_Close(void);
s32 AreaBoard_ListInput(u16 pad);
s32 AreaBoard_RecruitInput(u16 pad);
s32 AreaBoard_LeaveEntry(u16 pad);
s32 AreaBoard_Join(u16 pad, AreaBoardMenu *m, s32 slot);
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
extern s8 gLobbyLevel1Sel;
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
void LobbySe_Play(s32);

/* LobbyLevel0_HasGroups and AreaBoard_Open were built at optimization level 4. */
#pragma optimization_level 4

extern u8 gLobbyLevel0Sel;
extern LevelEntry gLobbyLevel0Entries[];

/*
 * Returns 1 if the area the player is in (gLobbyLevel0Sel) is of kind
 * AREA_KIND_GROUPS, whose board is the group board, and 0 if its board is
 * the area board.
 */
s32 LobbyLevel0_HasGroups(void)
{
    /* the first entry's kind stepped by whole entries: indexed as an array,
     * the compiler forms the address in another order */
    return *(&gLobbyLevel0Entries[0].kind + (gLobbyLevel0Sel - 1) * sizeof(LevelEntry)) == AREA_KIND_GROUPS;
}

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u16 gLobbyLevel1Count;
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
u16 Rand_Next(s32 stream);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 Menu_CountPages(s16 n, s16 per);
void AreaBoard_Draw(MenuTask *task);
void AreaBoard_DrawPanel(MenuTask *task);
/*
 * Opens the area board (TOWN_MODE_AREA_BOARD): clears the work, sets the
 * two draw callbacks, counts the pages of the area's entries, starts the
 * list's slide in from the left, draws the stamp's random seed and plays
 * the board's sound.
 */
void AreaBoard_Open(void)
{
    AreaBoardMenu *board = (AreaBoardMenu *)gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_AREA_BOARD;
    memset(board, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(AreaBoard_Draw, AreaBoard_DrawPanel, 0, 0);
    board->listPages = Menu_CountPages(gLobbyLevel1Count, AREA_BOARD_ROWS_PER_PAGE);
    WinSlide_Step(&board->listWin, -0x104, 0x36);
    board->stampSeed = Rand_Next(1);
    Sound_PlaySeFlat(1, 0x95, 0);
}

#pragma optimization_level reset

extern s8 gMenuInputLock;
/*
 * Closes the board: back to TOWN_MODE_FREE, the menu input held for 3
 * frames, no draw callbacks, no entry chosen.
 */
void AreaBoard_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    gMenuInputLock = 3;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gLobbyLevel1Sel = 0;
}

/*
 * The board's update, once a frame (enum AreaBoardState). Runs the entry
 * list; an open entry chosen leads to the join, a free one to the recruit
 * form. A join or an opening ends in the stamp and the town's entry; a
 * recruit cancelled or timed out gives the entry up
 * (AreaBoard_LeaveEntry); a failure shows its notice until circle.
 */
void AreaBoard_Update(void)
{
    AreaBoardMenu *board = (AreaBoardMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (board->state) {
    case AREA_BOARD_SLIDE_IN:
        switch (WinSlide_Step(&board->listWin, -0x104, 0x36)) {
        case MENU_DONE:
            board->state = AREA_BOARD_LIST;
            break;
        }
        break;
    case AREA_BOARD_SLIDE_OUT:
        switch (WinSlide_Step(&board->listWin, 0x36, -0x104)) {
        case MENU_DONE:
            AreaBoard_Close();
            break;
        }
        break;
    case AREA_BOARD_LIST:
        switch (AreaBoard_ListInput(pad)) {
        case MENU_DONE:
            board->state = AREA_BOARD_JOIN;
            board->showForm = 0;
            WinSlide_Step(&board->joinWin, 0x280, 0x140);
            break;
        case MENU_CANCEL:
            board->state = AREA_BOARD_SLIDE_OUT;
            WinSlide_Step(&board->listWin, 0x36, -0x104);
            break;
        case AREA_BOARD_PICK_FREE:
            board->state = AREA_BOARD_RECRUIT;
            board->showForm = 1;
            gLobbyLevel1Sel = board->entryNo;
            break;
        }
        break;
    case AREA_BOARD_RECRUIT:
        switch (AreaBoard_RecruitInput(pad)) {
        case MENU_DONE:
            board->state = AREA_BOARD_STAMP;
            board->showForm = 1;
            Win_Shake(&board->stampShake);
            board->stamped = 1;
            break;
        case MENU_CANCEL:
            board->state = AREA_BOARD_LEAVE;
            break;
        case LOBBY_STEP_FAILED:
            board->state = AREA_BOARD_NOTICE;
            board->noticeNext = AREA_BOARD_LIST;
            break;
        }
        break;
    case AREA_BOARD_JOIN:
        switch (AreaBoard_Join(pad, board, board->entryNo)) {
        case MENU_DONE:
            board->state = AREA_BOARD_STAMP;
            board->panelWin.pos = board->joinWin.pos;
            Win_Shake(&board->stampShake);
            board->stamped = 1;
            break;
        case MENU_CANCEL:
            board->state = AREA_BOARD_LIST;
            break;
        case LOBBY_STEP_FAILED:
            board->state = AREA_BOARD_NOTICE;
            board->noticeNext = AREA_BOARD_LIST;
            break;
        }
        break;
    case AREA_BOARD_STAMP:
        switch (Win_Shake(&board->stampShake)) {
        case MENU_DONE:
            (board->state)++;
            break;
        }
        break;
    case AREA_BOARD_ENTER_TOWN:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_DownloadQuests()) {
        case LOBBY_STEP_DONE:
        case LOBBY_STEP_FAILED:
            LobbyTown_Begin();
            Stream_Stop(0);
            Stream_Stop(1);
            break;
        }
        break;
    case AREA_BOARD_LEAVE:
        gLobbyMenu->redraw = 1;
        switch (AreaBoard_LeaveEntry(pad)) {
        case LOBBY_STEP_DONE:
            board->state = AREA_BOARD_LIST;
            break;
        case LOBBY_STEP_FAILED:
            board->state = AREA_BOARD_NOTICE;
            board->noticeNext = AREA_BOARD_LIST;
            break;
        }
        break;
    case AREA_BOARD_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            board->state = board->noticeNext;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
}

s32 AreaBoard_JoinInput(u16 pad, AreaBoardMenu *m, s32 slot);
s32 LobbyLevel1_CanEnter(s32 slot);
s32 LobbyStep_EnterLevel1(void);
s32 LobbyStep_FetchLevel1Users(NetUser *users, s16 entryNo); /* unit's own declaration: an s16 entry here; with the definition's s32, AreaBoard_Join compiles differently */
/*
 * Joins the level-1 entry entryNo (from 1): fetches its members, slides
 * the members panel in and runs it (AreaBoard_JoinInput), then enters the
 * entry. Returns MENU_DONE once entered, MENU_CANCEL when backed out,
 * LOBBY_STEP_FAILED when the entry can't be entered (its rank limit or a
 * server error), MENU_RUNNING meanwhile. menu is the area board's work or
 * the group board's, which share these fields.
 */
s32 AreaBoard_Join(u16 pad, AreaBoardMenu *menu, s32 entryNo)
{
    switch (menu->step) {
    case AREA_JOIN_FETCH:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_FetchLevel1Users((NetUser *) menu->members, entryNo)) {
        case LOBBY_STEP_DONE:
            menu->step++;
            menu->memberPage = 0;
            if (menu->members[AREA_BOARD_MEMBERS_PER_PAGE].id[0] == 0) {
                menu->memberPages = 1;
            } else {
                menu->memberPages = 2;
            }
            WinSlide_Step(&menu->joinWin, 0x280, 0x140);
            break;
        }
        break;
    case AREA_JOIN_SLIDE_IN:
        switch (WinSlide_Step(&menu->joinWin, 0x280, 0x140)) {
        case MENU_DONE:
            menu->step = AREA_JOIN_MEMBERS;
            break;
        }
        break;
    case AREA_JOIN_SLIDE_OUT:
        switch (WinSlide_Step(&menu->joinWin, 0x140, 0x280)) {
        case MENU_DONE:
            menu->step = AREA_JOIN_FETCH;
            return MENU_CANCEL;
        }
        break;
    case AREA_JOIN_MEMBERS:
        switch (AreaBoard_JoinInput(pad, menu, entryNo)) {
        case MENU_DONE:
            if (LobbyLevel1_CanEnter(entryNo) == 1) {
                menu->step++;
                gLobbyLevel1Sel = entryNo;
                break;
            }
            menu->step = AREA_JOIN_FETCH;
            gLobbyLevel1Sel = 0;
            LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
            return LOBBY_STEP_FAILED;
        case MENU_CANCEL:
            menu->step = AREA_JOIN_SLIDE_OUT;
            WinSlide_Step(&menu->joinWin, 0x140, 0x280);
            break;
        case LOBBY_STEP_FAILED:
            menu->step = AREA_JOIN_FETCH;
            return LOBBY_STEP_FAILED;
        }
        break;
    case AREA_JOIN_ENTER:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_EnterLevel1()) {
        case LOBBY_STEP_DONE:
            menu->step = AREA_JOIN_FETCH;
            return MENU_DONE;
        case LOBBY_STEP_FAILED:
            menu->step = AREA_JOIN_FETCH;
            gLobbyLevel1Sel = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    }
    return MENU_RUNNING;
}

extern LevelEntry gLobbyLevel1Entries[];
void LobbyText_Open(s32 msg);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
/*
 * The entry list's input: circle on an open entry returns MENU_DONE (join
 * it), on a free one AREA_BOARD_PICK_FREE (open it), on any other a
 * refusal sound; cross returns MENU_CANCEL; left and right turn the page,
 * up and down move the cursor. Returns MENU_RUNNING otherwise.
 */
s32 AreaBoard_ListInput(u16 pad)
{
    AreaBoardMenu *board = (AreaBoardMenu *)&gMenuWork;
    s32 chosenNo;
    u8 entryState;

    if (pad & PAD_CIRCLE) {
        chosenNo = board->rowCursor + board->listPage * AREA_BOARD_ROWS_PER_PAGE + 1;
        entryState = gLobbyLevel1Entries[chosenNo - 1].state;
        if (entryState == LEVEL_ENTRY_OPEN || entryState == LEVEL_ENTRY_OPEN_ALT) {
            board->entryNo = chosenNo;
            LobbyText_Open(LOBBY_TEXT_SENDING);
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        } else if (entryState == LEVEL_ENTRY_FREE) {
            board->entryNo = chosenNo;
            LobbyText_Open(LOBBY_TEXT_GROUP_OPENING);
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return AREA_BOARD_PICK_FREE;
        } else {
            LobbySe_Play(LOBBY_SE_REFUSE);
        }
    } else if (pad & PAD_CROSS) {
        board->rowCursor = 0;
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        board->listPage = Menu_WrapLeftRight(board->listPage, board->listPages);
    } else {
        board->rowCursor = Menu_WrapUpDown(board->rowCursor, AREA_BOARD_ROWS_PER_PAGE);
    }
    return MENU_RUNNING;
}

void LobbyDialog_Grow(void);
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
void HunterInfo_Open(void *src, s32 flags);
s32 HunterInfo_Update(u16 pad);
/*
 * The members panel of the entry being joined (enum AreaJoinInputStep):
 * left and right turn its member pages, square puts a cursor on the
 * members and circle there opens a member's hunter card, circle asks
 * whether to join. Returns MENU_DONE on yes, MENU_CANCEL on cross,
 * LOBBY_STEP_FAILED with a notice once the entry is no longer open.
 */
s32 AreaBoard_JoinInput(u16 pad, AreaBoardMenu *menu, s32 entryNo)
{
    LevelEntry *entry = &gLobbyLevel1Entries[entryNo - 1];
    s32 i;

    switch (menu->joinStep) {
    case AREA_JOIN_INPUT_PAGE:
        if (entry->state != LEVEL_ENTRY_OPEN && entry->state != LEVEL_ENTRY_OPEN_ALT) {
            LobbyText_Open(LOBBY_TEXT_GROUP_DISBANDED);
            LobbySe_Play(LOBBY_SE_CANCEL);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
            return LOBBY_STEP_FAILED;
        }
        if (pad & PAD_CIRCLE) {
            menu->joinStep = AREA_JOIN_INPUT_ASK;
            LobbyText_Open(LOBBY_TEXT_ENTRY_JOIN_ASK);
            Menu_SetYesNo(1);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_SQUARE) {
            menu->memberCount = 0;
            for (i = 0; i < AREA_BOARD_MEMBERS_PER_PAGE; i++) {
                if (menu->members[i + menu->memberPage * AREA_BOARD_MEMBERS_PER_PAGE].id[0] != 0) {
                    menu->memberCount++;
                }
            }
            if (menu->memberCount != 0) {
                menu->joinStep++;
                menu->memberView = MEMBER_VIEW_CURSOR;
                menu->memberCursor = 0;
                LobbySe_Play(LOBBY_SE_SCROLL);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            menu->memberPage = Menu_WrapLeftRight(menu->memberPage, menu->memberPages);
        }
        break;
    case AREA_JOIN_INPUT_MEMBERS:
        if (pad & PAD_CIRCLE) {
            menu->joinStep++;
            menu->memberView = MEMBER_VIEW_HUNTER_CARD;
            HunterInfo_Open(&menu->members[menu->memberCursor + menu->memberPage * AREA_BOARD_MEMBERS_PER_PAGE], HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND | HUNTER_INFO_JOIN);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            menu->joinStep--;
            menu->memberView = MEMBER_VIEW_HINTS;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & (PAD_UP | PAD_DOWN)) {
            menu->memberCursor = Menu_WrapUpDown(menu->memberCursor, menu->memberCount);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            menu->memberPage = Menu_WrapLeftRight(menu->memberPage, menu->memberPages);
            menu->memberCount = 0;
            for (i = 0; i < AREA_BOARD_MEMBERS_PER_PAGE; i++) {
                if (menu->members[i + menu->memberPage * AREA_BOARD_MEMBERS_PER_PAGE].id[0] != 0) {
                    menu->memberCount++;
                }
            }
            if (menu->memberCursor >= menu->memberCount) {
                menu->memberCursor = menu->memberCount - 1;
            }
        }
        break;
    case AREA_JOIN_INPUT_HUNTER_CARD:
        switch (HunterInfo_Update(pad)) {
        case MENU_DONE:
        case 1:
        case MENU_CANCEL:
            menu->joinStep--;
            menu->memberView = MEMBER_VIEW_CURSOR;
            break;
        }
        break;
    case AREA_JOIN_INPUT_ASK:
        if (entry->state != LEVEL_ENTRY_OPEN && entry->state != LEVEL_ENTRY_OPEN_ALT) {
            menu->joinStep = AREA_JOIN_INPUT_PAGE;
            LobbyText_Open(LOBBY_TEXT_GROUP_DISBANDED);
            LobbySe_Play(LOBBY_SE_CANCEL);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
            return LOBBY_STEP_FAILED;
        }
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            menu->joinStep = AREA_JOIN_INPUT_PAGE;
            LobbyText_Open(LOBBY_TEXT_SENDING);
            gLobbyLevel1Sel = entryNo;
            return MENU_DONE;
        case MENU_CANCEL:
            menu->joinStep = AREA_JOIN_INPUT_PAGE;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

extern LevelEntry gLobbyLevel0Entries[];
extern u8 gLobbyLevel0Sel;
s32 Menu_CountPages(s16 n, s16 per);
/*
 * Fills the text picker's page `page`: the room data's slots holding a
 * recruiting text offered in this kind of area, RECRUIT_TEXTS_PER_PAGE to
 * a page; counts the pages and keeps the cursor on the page.
 */
void AreaBoard_LoadPage(s16 page)
{
    RoomData *room = QDL_ROOM;
    AreaBoardMenu *menu = (AreaBoardMenu *)&gMenuWork;
    s16 firstShown = page * RECRUIT_TEXTS_PER_PAGE;
    s16 i;
    u8 areaNo;
    s16 listed;

    menu->textCount = 0;
    areaNo = gLobbyLevel0Sel;
    i = 0;
    listed = 0;
    for (; i < ROOM_SLOTS; i++) {
        if (room->slots[i].text[0] != 0 && room->slotKinds[(gLobbyLevel0Entries + areaNo - 1)->kind - 1][i] != 0) {
            if (listed >= firstShown && listed <= (s16)(firstShown + RECRUIT_TEXTS_PER_PAGE - 1)) {
                menu->textSlots[menu->textCount] = i;
                (menu->textCount)++;
            }
            listed++;
        }
    }
    menu->textPages = Menu_CountPages(listed, RECRUIT_TEXTS_PER_PAGE);
    if (menu->textCursor >= menu->textCount) {
        menu->textCursor = menu->textCount - 1;
    }
    menu->textPage = page;
}

s32 LobbyStep_Level1SetColor(s32 v);
s32 LobbyStep_Level1Settings(char *text, u8 *vals);
s32 LobbyStep_RefreshLevel1Users(void);
void LobbyDialog_OpenWait(void *);
char *strcpy(char *dst, const char *src);
s32 LobbyStep_Level1Open(s32 id);
extern u16 gRecruitFrames;
/*
 * Opens the free entry chosen as the player's own and runs the recruit
 * form against the time limit (enum AreaRecruitStep): the text row opens
 * the text picker, the confirm row sends the text's colour and the text,
 * then refetches the entry's users. Returns MENU_DONE when sent,
 * LOBBY_STEP_FAILED when the entry can't be opened, MENU_CANCEL when the
 * form is left, runs out of time or a send fails (the entry is then given
 * up).
 */
s32 AreaBoard_RecruitInput(u16 pad)
{
    AreaBoardMenu *board = (AreaBoardMenu *)&gMenuWork;
    RoomData *room = QDL_ROOM;

    switch (board->recruitStep) {
    case AREA_RECRUIT_SLIDE_IN:
    case AREA_RECRUIT_FORM:
    case AREA_RECRUIT_PICK_TEXT:
    case AREA_RECRUIT_NO_TEXT:
        if (--board->formTimer <= 0) {
            board->recruitStep = AREA_RECRUIT_OPEN;
            board->row = AREA_RECRUIT_ROW_TEXT;
            LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
            LobbySe_Play(LOBBY_SE_CANCEL);
            memset(board->text, 0, sizeof(board->text));
            return MENU_CANCEL;
        }
        break;
    }
    switch (board->recruitStep) {
    case AREA_RECRUIT_OPEN:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level1Open(*(u16 *)&board->entryNo)) {
        case LOBBY_STEP_DONE:
            board->recruitStep++;
            board->formTimer = gRecruitFrames;
            AreaBoard_LoadPage(0);
            WinSlide_Step(&board->panelWin, 0x280, 0x140);
            break;
        case LOBBY_STEP_FAILED:
            board->recruitStep = AREA_RECRUIT_OPEN;
            LobbyDialog_OpenWait(gLobby->errText);
            return LOBBY_STEP_FAILED;
        }
        break;
    case AREA_RECRUIT_SLIDE_IN:
        switch (WinSlide_Step(&board->panelWin, 0x280, 0x140)) {
        case MENU_DONE:
            memset(board->text, 0, sizeof(board->text));
            board->recruitStep++;
            break;
        }
        break;
    case AREA_RECRUIT_FORM:
        if (pad & PAD_CIRCLE) {
            if (board->row == AREA_RECRUIT_ROW_TEXT) {
                board->recruitStep = AREA_RECRUIT_PICK_TEXT;
                board->textCursor = 0;
            } else if (board->text[0] == 0) {
                board->recruitStep = AREA_RECRUIT_NO_TEXT;
                LobbyText_Open(LOBBY_TEXT_RECRUIT_TEXT_NONE);
                LobbyDialog_Grow();
            } else {
                board->recruitStep = AREA_RECRUIT_SEND_COLOR;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            board->recruitStep = AREA_RECRUIT_OPEN;
            board->row = AREA_RECRUIT_ROW_TEXT;
            LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
            LobbySe_Play(LOBBY_SE_CANCEL);
            memset(board->text, 0, sizeof(board->text));
            return MENU_CANCEL;
        } else {
            board->row = Menu_WrapUpDown(board->row, AREA_RECRUIT_ROWS);
        }
        break;
    case AREA_RECRUIT_PICK_TEXT:
        if (pad & PAD_CIRCLE) {
            if (room->slotLocked[(s8)board->textSlots[board->textCursor]] == 0) {
                board->recruitStep = AREA_RECRUIT_FORM;
                memset(board->text, 0, sizeof(board->text));
                strcpy(board->text, room->slots[(s8)board->textSlots[board->textCursor]].text);
                LobbyText_Open(LOBBY_TEXT_MOVING);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else if (pad & PAD_CROSS) {
            board->recruitStep = AREA_RECRUIT_FORM;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (board->textPages > 1) {
                board->textPage = Menu_WrapLeftRight(board->textPage, board->textPages);
                AreaBoard_LoadPage(board->textPage);
            }
        } else {
            board->textCursor = Menu_WrapUpDown(board->textCursor, board->textCount);
        }
        break;
    case AREA_RECRUIT_SEND_COLOR:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level1SetColor(room->slotColor[(s8)board->textSlots[board->textCursor]])) {
        case LOBBY_STEP_DONE:
            board->recruitStep++;
            break;
        case LOBBY_STEP_FAILED:
            board->recruitStep = AREA_RECRUIT_OPEN;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return MENU_CANCEL;
        }
        break;
    case AREA_RECRUIT_SEND_TEXT:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level1Settings(board->text, 0)) {
        case LOBBY_STEP_DONE:
            board->recruitStep++;
            break;
        case LOBBY_STEP_FAILED:
            board->recruitStep = AREA_RECRUIT_OPEN;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return MENU_CANCEL;
        }
        break;
    case AREA_RECRUIT_REFRESH:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_RefreshLevel1Users()) {
        case LOBBY_STEP_DONE:
            board->recruitStep = AREA_RECRUIT_OPEN;
            return MENU_DONE;
        case LOBBY_STEP_FAILED:
            gLobbyMenu->step = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return MENU_CANCEL;
        }
        break;
    case AREA_RECRUIT_NO_TEXT:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            board->recruitStep = AREA_RECRUIT_FORM;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The group board's new group: sends the chosen recruiting text's colour,
 * then the text with the group's settings (RoomJoin.groupInfo), then
 * refetches the entry's users (enum RoomJoinStep). Returns LOBBY_STEP_DONE
 * when done, LOBBY_STEP_FAILED after showing the server's error,
 * LOBBY_STEP_BUSY meanwhile.
 */
s16 GroupBoard_SendSettings(RoomJoin *join)
{
    AreaBoardMenu *menu = (AreaBoardMenu *)&gMenuWork;
    RoomData *room = QDL_ROOM;

    switch (join->step) {
    case ROOM_JOIN_SEND_COLOR:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level1SetColor(room->slotColor[menu->textSlots[join->textIdx]])) {
        case LOBBY_STEP_DONE:
            join->step++;
            break;
        case LOBBY_STEP_FAILED:
            join->step = ROOM_JOIN_SEND_COLOR;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return LOBBY_STEP_FAILED;
        }
        break;
    case ROOM_JOIN_SEND_SETTINGS:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level1Settings(room->slots[menu->textSlots[join->textIdx]].text, (u8 *)join->groupInfo)) {
        case LOBBY_STEP_DONE:
            join->step++;
            break;
        case LOBBY_STEP_FAILED:
            join->step = ROOM_JOIN_SEND_COLOR;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return LOBBY_STEP_FAILED;
        }
        break;
    case ROOM_JOIN_REFRESH:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_RefreshLevel1Users()) {
        case LOBBY_STEP_DONE:
            join->step = ROOM_JOIN_SEND_COLOR;
            return LOBBY_STEP_DONE;
        case LOBBY_STEP_FAILED:
            gLobbyMenu->screen = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return LOBBY_STEP_FAILED;
        }
        break;
    }
    return LOBBY_STEP_BUSY;
}

s32 LobbyStep_Level1Leave(void);
s32 LobbyStep_Level1List(s32);
/*
 * Gives up the level-1 entry the player opened and fetches the area's
 * entry list again (enum AreaLeaveStep). Returns LOBBY_STEP_DONE when
 * listed again, LOBBY_STEP_FAILED after showing the server's error,
 * LOBBY_STEP_BUSY meanwhile.
 */
s32 AreaBoard_LeaveEntry(u16 pad)
{
    switch (gLobby->subStep2) {
    case AREA_LEAVE_RELEASE:
        switch (LobbyStep_Level1Leave()) {
        case LOBBY_STEP_DONE:
            gLobby->subStep2++;
            gLobby->seqStep = 0;
            gLobbyLevel1Sel = 0;
            break;
        case LOBBY_STEP_FAILED:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->subStep2 = AREA_LEAVE_RELEASE;
            return LOBBY_STEP_FAILED;
        }
        break;
    case AREA_LEAVE_RELIST:
        switch (LobbyStep_Level1List(0)) {
        case LOBBY_STEP_DONE:
            gLobby->subStep2 = AREA_LEAVE_RELEASE;
            return LOBBY_STEP_DONE;
        case LOBBY_STEP_FAILED:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->subStep2 = AREA_LEAVE_RELEASE;
            return LOBBY_STEP_FAILED;
        }
        break;
    }
    return LOBBY_STEP_BUSY;
}

extern u16 gLobbyLevel1Count;
extern char gAreaBoardTextFmt[];
extern char gAreaBoardUsersFmt[];
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Text_SetColor(s16 color);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: s16 coordinates and an s32 kind here; with the definition's s32 coordinates and s16 kind, the entry list and the text picker compile differently */
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
/*
 * Draws the entry list at x, y: its label bar and page number while the
 * list has the input (the join's or the recruit's label otherwise), the
 * column headings, a page of entries (number, text in its colour, users
 * and maximum, users in town; a full or closed entry greyed, a missing one
 * as dashes), the cursor line and the help box.
 */
void AreaBoard_DrawList(s16 x, s16 y)
{
    AreaBoardMenu *board = (AreaBoardMenu *)&gMenuWork;
    LevelEntry *entry;
    u32 color;
    s16 leftX;
    s32 i;
    s32 entryIdx;
    s16 rowY = y;
    s8 dim;

    Font_SetSpacing(0x16, 0x16);
    switch (board->state) {
    case AREA_BOARD_SLIDE_OUT:
    case AREA_BOARD_SLIDE_IN:
        break;
    case AREA_BOARD_LIST:
        Menu_DrawLabelBar(0x14, 0x12, 0xC8, 0xFFB0B000, gNetLabels[NET_LABEL_GROUP_BOARD], 2, 1);
        Menu_DrawPageNum(x + 0x200, 0x16, board->listPage, board->listPages, 1);
        break;
    default:
        if (board->showForm == 1) {
            Menu_DrawLabelBar(0x140, 0x12, 0xC8, 0xFFB0B000, gNetLabels[NET_LABEL_RECRUIT_MEMBERS], 2, 1);
        } else {
            Menu_DrawLabelBar(0x140, 0x12, 0xC8, 0xFFB0B000, gNetLabels[NET_LABEL_JOIN_GROUP], 2, 1);
        }
        break;
    }
    Win_Draw(x, y, 0x264, 0xFA, 0xBDB59F6C, 0xB0F04020, 7);
    rowY += 2;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F28);
    leftX = x;
    Font_SetPos(leftX + 0xA, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_NUMBER]);
    Font_SetPos(leftX + 0x3C, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_RECRUIT_MSG]);
    Font_SetPos(leftX + 0x1CC, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_HEADCOUNT]);
    Font_SetPos(leftX + 0x21C, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_IN_TOWN]);
    for (i = 0; i < AREA_BOARD_ROWS_PER_PAGE; i++) {
        rowY += 0x16;
        entryIdx = i + board->listPage * AREA_BOARD_ROWS_PER_PAGE;
        if (entryIdx < gLobbyLevel1Count) {
            entry = &gLobbyLevel1Entries[entryIdx];
            if (entry->users == 0 || (entry->state != LEVEL_ENTRY_OPEN && entry->state != LEVEL_ENTRY_OPEN_ALT)) {
                color = 0x80404040;
            } else {
                color = 0x807F7F7F;
            }
            Font_SetColor(color);
            Font_SetPos(leftX + 0xA, rowY);
            Text_PrintfColored("%2d", entryIdx + 1);
            if (color == 0x807F7F7F) {
                Text_SetColor(entry->color);
            }
            Font_SetPos(leftX + 0x3C, rowY);
            Text_PrintfColored(gAreaBoardTextFmt, entry->text);
            Font_SetColor(color);
            Font_SetPos(leftX + 0x1C6, rowY);
            Text_PrintfColored(gAreaBoardUsersFmt, entry->users, entry->maxUsers);
            Font_SetPos(leftX + 0x21C, rowY);
            Text_PrintfColored("%2d", entry->users - entry->questUsers);
        } else {
            Font_SetColor(0x80404040);
            Font_SetPos(leftX + 0xA, rowY);
            Text_PrintfColored(gNetLabels[NET_LABEL_DASHES]);
        }
        if (i == board->rowCursor) {
            switch (board->state) {
            case AREA_BOARD_SLIDE_IN:
            case AREA_BOARD_SLIDE_OUT:
            case AREA_BOARD_LIST:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawLine(x, rowY - 1, 0x264, 0x14, dim);
        }
    }
    Win_DrawText(0x12C, 0x154, 0x140, 0x50, 0xBDB59F6C, 0xC0201010, gNetLabels[NET_LABEL_AREA_BOARD_HELP]);
    switch (board->state) {
    case AREA_BOARD_SLIDE_IN:
    case AREA_BOARD_SLIDE_OUT:
    case AREA_BOARD_LIST:
        break;
    default:
        return;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/area_board", gAreaBoardTextFmt);
INCLUDE_RODATA("asm/lobby/nonmatchings/area_board", gAreaBoardUsersFmt);

u32 AreaList_KindColor(u8 slot);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 a, s16 b);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2);
s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
struct EquipItem;
/* AreaBoard_DrawMembers passes the weapon's class as an s32 to Icon_DrawWeaponClass's u8 parameter: that way the compiler masks it to a byte as the game's code does */
u8 Equip_GetClass(struct EquipItem *e);
void Icon_DrawWeaponClass(s16, s16, s32, s32, u8, s32); /* unit's own declaration: s16 coordinates and a u8 class here; with the definition's types, the members panel compiles differently */
void GroupBoard_DrawButtonHints(void *w, s16 x, s16 y, u32 color);
/*
 * Draws the members panel of entry entryNo at x, y: the area kind's
 * emblem, the entry's name, a page of members (name, hunter rank, weapon
 * class) with the cursor at MEMBER_VIEW_CURSOR, the button hints and, when
 * stamped, the stamp (shake's timer and stampSeed). The area board and the
 * group board both draw their join panel with it.
 */
void AreaBoard_DrawMembers(s16 x, s16 y, s32 entryNo, WinShake *shake, u16 stampSeed, s8 stamped)
{
    AreaBoardMenu *menu = (AreaBoardMenu *)gMenuWork;
    NetUser *member;
    HunterCard *card;
    s16 rowY = y;
    s16 i;
    char hrText[16];
    char hrWide[16];

    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 1);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xDC, y + 6, menu->memberPage, menu->memberPages, 1);
    LobbyUi_DrawIcon(x + 0x96, y + 0x87, 0x70, AreaList_KindColor(gLobbyLevel0Entries[gLobbyLevel0Sel - 1].kind), 3);
    Font_SetColor(0x80301010);
    Font_SetPos(x + 0x28, y + 0x14);
    Text_PrintfColored(gAreaBoardTextFmt, gLobbyLevel1Entries[entryNo - 1].name);
    rowY += 0x2A;
    Font_SetPos(x + 0x28, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_NAME]);
    Font_SetPos(x + 0xBE, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_HR]);
    Font_SetPos(x + 0xF0, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_WEAPON]);
    Font_SetColor(0x80101010);
    for (i = 0; i < AREA_BOARD_MEMBERS_PER_PAGE; i++) {
        rowY += 0x16;
        member = &menu->members[i + menu->memberPage * AREA_BOARD_MEMBERS_PER_PAGE];
        card = (HunterCard *)member->text;
        if (member->id[0] != 0) {
            if (menu->memberView == MEMBER_VIEW_CURSOR && i == menu->memberCursor) {
                Menu_DrawCursorRow(x + 0x28, rowY, 0xBE, 0x14, 0xC0106060, 0xFF106060);
            }
            Font_SetPos(x + 0x28, rowY);
            Text_PrintfColored(gAreaBoardTextFmt, member->name);
            Font_SetPos(x + 0xAC, rowY);
            sprintf(hrText, "%3d", card->hr);
            Sjis_Widen(hrText, hrWide);
            Text_PrintfColored(hrWide);
            Icon_DrawWeaponClass(x + 0xF6, rowY, 0x18, -1, (s32)Equip_GetClass(&card->weapon), 0);
        }
    }
    GroupBoard_DrawButtonHints(menu, x + 0x28, rowY + 0x24, 0x80101010);
    if (stamped) {
        Menu_DrawStamp(x, y, 0x12C, 0x10E, stampSeed, shake->timer);
    }
}

/*
 * Draws the two blinking marks either side of the recruit form's chosen
 * row while the form has the input.
 */
void AreaBoard_DrawRowMarks(s16 x, s16 y)
{
    AreaBoardMenu *board = (AreaBoardMenu *)&gMenuWork;
    s16 bounce;

    if (board->recruitStep == AREA_RECRUIT_FORM) {
        bounce = (gFrameTick & 0x10) ? 0 : 6;
        switch (board->row) {
        case AREA_RECRUIT_ROW_TEXT:
            Icon_Draw(x + 10 - bounce, y + 0x7C, 0x18, 0x18, 0xFF106060, 0x29);
            Icon_Draw(x + 0x10A + bounce, y + 0x7C, 0x18, 0x18, 0xFF106060, 0x28);
            break;
        case AREA_RECRUIT_ROW_CONFIRM:
            Icon_Draw(x + 100 - bounce, y + 0xBE, 0x18, 0x18, 0xFF106060, 0x29);
            Icon_Draw(x + 0xB0 + bounce, y + 0xBE, 0x18, 0x18, 0xFF106060, 0x28);
            break;
        }
    }
}

void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
void Text_Printf(const char *fmt, ...);
/*
 * Draws the recruit form at x, y: the area kind's emblem, the entry's
 * name, the recruiting text in its frame (highlighted on the text row),
 * the confirm row and its marks, the time left in seconds (not while the
 * entry is being given up) and the stamp once stamped.
 */
void AreaBoard_DrawRecruitForm(s16 x, s16 y)
{
    AreaBoardMenu *board = (AreaBoardMenu *)gMenuWork;
    LevelEntry *entry = &gLobbyLevel1Entries[board->entryNo - 1];
    s16 rowY = y;
    u32 frameColor = board->row == AREA_RECRUIT_ROW_TEXT ? 0xFF106060 : 0xFF606060;
    s32 topY;
    s32 leftX;
    u32 kindColor;
    s32 centerX;
    s32 textX;

    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 1);
    kindColor = AreaList_KindColor(gLobbyLevel0Entries[gLobbyLevel0Sel - 1].kind);
    topY = rowY;
    leftX = x;
    centerX = leftX + 0x96;
    LobbyUi_DrawIcon(centerX, topY + 0x87, 0x70, kindColor, 3);
    Font_SetColor(0x80301010);
    Font_SetPos(leftX + 0x28, topY + 0x14);
    Text_PrintfColored(gAreaBoardTextFmt, entry->name);
    rowY += 0x2A;
    textX = leftX + 0x28;
    Font_SetPos(textX, rowY += 0x32);
    Text_PrintfColored(gAreaBoardTextFmt, gNetLabels[NET_LABEL_RECRUIT_MSG_COLON]);
    Gfx_DrawRectFrame(textX, rowY += 0x20, 0xDC, 0x18, frameColor);
    Font_SetColor(0x80101010);
    Font_SetPos(Text_CenterX(board->text, centerX, 0x12), rowY + 3);
    Text_PrintfColored(gAreaBoardTextFmt, board->text);
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gNetLabels[NET_LABEL_DECIDE], centerX, 0x12), rowY + 0x46);
    Text_PrintfColored(gAreaBoardTextFmt, gNetLabels[NET_LABEL_DECIDE]);
    AreaBoard_DrawRowMarks(x, y);
    if (board->state != AREA_BOARD_LEAVE) {
        Font_SetPos(leftX + 0x9C, topY + 0xEC);
        Text_Printf(gArenaCupTexts[ARENA_TEXT_TIME_LEFT], board->formTimer / 30); /* frames to seconds */
    }
    if (board->stamped != 0) {
        Menu_DrawStamp(x, y, 0x12C, 0x10E, board->stampSeed, board->stampShake.timer);
    }
}


/*
 * Draws the recruiting text picker at x, y: count texts of the current
 * page in their colours, the page number and the cursor line at row
 * cursor. Both boards draw their text picker with it.
 */
void AreaBoard_DrawTextPicker(s16 x, s16 y, s16 cursor, s16 count)
{
    AreaBoardMenu *menu = (AreaBoardMenu *)gMenuWork;
    RoomData *room = QDL_ROOM;
    s32 i;
    s16 rowY = y;
    s8 *slotNo;

    Win_Draw(x, y, 0x118, 0x122, 0xBDB59F6C, 0xC0101010, 2);
    rowY += 10;
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x28, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_RECRUIT_MSG]);
    Font_SetColor(0x807F7F7F);
    Menu_DrawPageNum((s16)(x + 0x28) + 0xA0, rowY, menu->textPage, menu->textPages, 1);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < count; i++) {
        if (room->slots[menu->textSlots[i]].text[0] != 0) {
            slotNo = &menu->textSlots[i];
            rowY += 0x16;
            Font_SetPos(x + 0x28, rowY);
            Text_SetColor(room->slotColor[*slotNo]);
            Text_PrintfColored(gAreaBoardTextFmt, room->slots[*slotNo].text);
            if (cursor == i) {
                Menu_DrawLine(x + 10, rowY - 1, 0x104, 0x14, 0);
            }
        }
    }
}

void Font_SetSpacing(u8 a, u8 b);
void Text_SetSize(s32);
void AreaBoard_DrawList(s16, s16);

/* The board's first draw callback: the entry list at its sliding x. */
void AreaBoard_Draw(MenuTask *task)
{
    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    AreaBoard_DrawList(0xE, ((MenuWork *)gMenuWork)->areaBoard.listWin.pos);
}

/* AreaBoard_DrawPanel was built at optimization level 4. */
#pragma optimization_level 4

void AreaBoard_DrawMembers(s16 x, s16 y, s32 n, WinShake *p, u16 a, s8 show);
void AreaBoard_DrawRecruitForm(s16 x, s16 y);
void AreaBoard_DrawTextPicker(s16 x, s16 y, s16 cur, s16 n);
void HunterInfo_DrawIfOpen(void);
/*
 * The board's second draw callback, the right-hand panel shaken with the
 * stamp: the members panel of the entry being joined or a member's hunter
 * card, or the recruit form with the text picker over it while the picker
 * is open.
 */
void AreaBoard_DrawPanel(MenuTask *task)
{
    AreaBoardMenu *board = (AreaBoardMenu *)gMenuWork;
    s16 x;
    s16 y;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    x = board->panelWin.pos + board->stampShake.dx;
    y = board->stampShake.dy + 0x36;
    switch (board->state) {
    case AREA_BOARD_JOIN:
        x = board->joinWin.pos + board->stampShake.dx;
    case AREA_BOARD_STAMP:
    case AREA_BOARD_ENTER_TOWN:
    case AREA_BOARD_LEAVE:
        if (board->showForm == 0) {
            if (board->memberView == MEMBER_VIEW_HUNTER_CARD) {
                HunterInfo_DrawIfOpen();
            } else {
                AreaBoard_DrawMembers(x, y, board->entryNo, &board->stampShake, board->stampSeed, board->stamped);
            }
        } else {
            AreaBoard_DrawRecruitForm(x, y);
        }
        break;
    case AREA_BOARD_RECRUIT:
        switch (board->recruitStep) {
        case AREA_RECRUIT_OPEN:
            break;
        case AREA_RECRUIT_PICK_TEXT:
            AreaBoard_DrawTextPicker(0x1E, 0x36, board->textCursor, board->textCount);
        default:
            AreaBoard_DrawRecruitForm(x, y);
            break;
        }
        break;
    }
}

#pragma optimization_level reset
