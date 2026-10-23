/*
 * The group board (TOWN_MODE_GROUP_BOARD), in a group area: the board that
 * lists the groups (level-1 entries) of the player's area and searches the
 * groups of every land.
 *
 * Its first question offers the area's groups or the search. The area's
 * groups: a free entry is opened as a new group through a form (who may
 * join, the target quest, a recruiting text, against a time limit), and an
 * open one is joined. The search: a form (the limit, up to four target
 * quests) sends LOBBY_CMD_LEVEL1_SEARCH; the results are listed, a chosen
 * group's details (LOBBY_CMD_LEVEL1_SEARCH_INFO, gGroupSearchInfo) show its
 * members and location, and joining it moves to its land if needed.
 *
 * The board's work is a GroupBoardMenu in gMenuWork (lobby_group_board.h).
 * Up to its search request it has the area board's AreaBoardMenu layout, since
 * the area board's join (AreaBoard_Join), recruiting-text list
 * (AreaBoard_LoadPage), settings sender (GroupBoard_SendSettings) and member
 * panel (AreaBoard_DrawMembers) run on it too. The room's quests are
 * RoomData's (QDL_ROOM); the area's groups are gLobbyLevel1Entries, and the
 * search's results come from gGroupSearchResults.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_text.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "lobby_group_board.h"
#include "screen_fade.h"

#include "save.h"
#include "quest.h"
#include "pad.h"
#include "lobby_servers.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "scene.h"
#include "stream.h"
#include "text.h"
#include "village_cond.h"
#include "area_list.h"
#include "font.h"
#include "hunter_info.h"
#include "item_shop.h"
#include "lobby_disconnect.h"
#include "lobby_queries.h"
#include "lobby_town.h"
#include "lobby_util.h"
#include "lobby_web_users.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "name_tags.h"
#include "rand.h"
#include "libc.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
extern s8 gLobbyLevel1Sel;
extern u8 gMenuWork;

void LobbyText_Open(s32);
void LobbyDialog_Grow(void);
void Menu_SetYesNo(u8); /* unit's own declaration: the choice is passed as a u8 here; with the definition's s8, GroupBoard_Update compiles differently */
void GroupBoard_Draw(MenuTask *task);

/* Opens the group board (TOWN_MODE_GROUP_BOARD): its first question,
 * recruit or search (LOBBY_TEXT_GROUP_BOARD_ASK, a two-way choice), and the
 * count of the room's first GROUP_BOARD_SLOTS slots that hold a recruiting
 * text. */
void GroupBoard_Open(void)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 i;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_GROUP_BOARD;
    memset(board, 0, sizeof(MenuWork));
    LobbyMenu_Init();
    LobbyMenu_SetDraw(GroupBoard_Draw, GroupBoard_DrawDetails, 0, 0);
    LobbyText_Open(LOBBY_TEXT_GROUP_BOARD_ASK);
    LobbyDialog_Grow();
    Menu_SetYesNo(0);
    board->recruitCount = 0;
    for (i = 0; i < GROUP_BOARD_SLOTS; i++) {
        if (QDL_ROOM->slots[i].text[0] != 0) {
            board->recruitCount++;
        }
    }
    LobbySe_Play(LOBBY_SE_CONFIRM);
}

/*
 * Whether quest questIndex of the room's quest list is open to the player:
 * it must be in use, and its unlock (RoomQuest.unlockKind, enum
 * RoomQuestUnlock) met: a hunter rank, a monster hunted with a rank, the
 * arena cup open, the elder raid running, EVENT_FLAG_SONG_TRACK_3 alone or
 * with a monster hunted. Returns 1 if open, else 0.
 */
s32 GroupBoard_QuestAllowed(s16 questIndex)
{
    RoomData *room;

    if ((room = QDL_ROOM)->quests[questIndex].used == 0) {
        return 0;
    }
    switch (room->quests[questIndex].unlockKind) {
    case ROOM_QUEST_UNLOCK_NONE:
        break;
    case ROOM_QUEST_UNLOCK_HR:
        if (gSave.hr < room->quests[questIndex].unlockArg1) {
            return 0;
        }
        break;
    case ROOM_QUEST_UNLOCK_HUNTED_HR:
        if (gSave.hunted[room->quests[questIndex].unlockArg1] == 0) {
            return 0;
        }
        if (gSave.hr < room->quests[questIndex].unlockArg2) {
            return 0;
        }
        break;
    case ROOM_QUEST_UNLOCK_ARENA_CUP:
        if (!Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN)) {
            return 0;
        }
        break;
    case ROOM_QUEST_UNLOCK_ELDER_RAID:
        if (!ElderRaid_IsOn()) {
            return 0;
        }
        break;
    case ROOM_QUEST_UNLOCK_SONG_TRACK_3:
        if (!Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3)) {
            return 0;
        }
        break;
    case ROOM_QUEST_UNLOCK_SONG_TRACK_3_HUNTED:
        if (!Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3)) {
            return 0;
        }
        if (gSave.hunted[QDL_ROOM->quests[questIndex].unlockArg1] == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* Closes the group board: the town takes the pad again after 3 frames,
 * and the chosen room is forgotten (gLobbyLevel1Sel). */
void GroupBoard_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    gMenuInputLock = 3;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gLobbyLevel1Sel = 0;
}

/* The rest of the group board was built at optimization level 4. */
#pragma optimization_level 4

extern s16 gLobbyLevel1Count;
/*
 * Builds the pages of target quests (GROUP_TARGETS_PER_PAGE a page, at
 * most GROUP_TARGET_PAGES pages) from the room's quests open to the player
 * (GroupBoard_QuestAllowed), and resets the board's cursors and pages.
 * mode is the first question's answer (enum GroupBoardChoice): for the
 * area's groups it also counts the pages of the area's group list and
 * draws the stamp's seed; for the search it puts GROUP_TARGET_ANY first
 * and stores each quest as its index + 1, so that a marked quest can be
 * negated.
 */
void GroupBoard_LoadTargets(s16 mode)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 count = 0;
    s16 slot = 0;
    s16 pageNo = 0;
    s16 i;

    if (mode == 1) {
        slot = count = 1;
        board->targets[0][0] = GROUP_TARGET_ANY;
    } else {
        board->pageCount = Menu_CountPages(gLobbyLevel1Count, GROUP_ROWS_PER_PAGE);
        board->stampSeed = Rand_Next(1);
    }
    for (i = 0; i < ROOM_QUESTS; i++) {
        if (GroupBoard_QuestAllowed(i) == 1) {
            count++;
            if (mode == 1) {
                board->targets[pageNo][slot] = i + 1;
            } else {
                board->targets[pageNo][slot] = i;
            }
            slot++;
            if (slot >= GROUP_TARGETS_PER_PAGE) {
                board->targetsOnPage[pageNo] = slot;
                slot = 0;
                pageNo++;
            }
        }
    }
    board->targetsOnPage[pageNo] = slot;
    board->targetPages = Menu_CountPages(count, GROUP_TARGETS_PER_PAGE);
    board->targetCount = count;
    board->targetsChosen = 0;
    board->infoPages = 2;
    board->searchLimit = 0;
    board->targetCursor = 0;
    board->row = 0;
    board->rowCursor = 0;
    board->targetPage = 0;
    board->infoPage = 0;
    board->page = 0;
}

extern LobbyMenu *gLobbyMenu;
extern LobbyState *gLobby;
void LobbyDialog_OpenWait(void *text);
s32 LobbyStep_DownloadQuests(void);
/*
 * The group board's frame (TOWN_MODE_GROUP_BOARD): the first question (the
 * area's groups or the group search; cross closes the board), then the
 * chosen flow. A group joined or opened is stamped (the panel shakes), the
 * quest files are downloaded and the town is entered; a failed search
 * shows the server's error text and closes the board on circle.
 */
void GroupBoard_Update(void)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (board->state) {
    case GROUP_BOARD_ASK:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            GroupBoard_LoadTargets(board->choice);
            switch (board->choice) {
            case GROUP_BOARD_CHOICE_AREA:
                board->state++;
                WinSlide_Step(&board->listWin, -260, 54);
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case GROUP_BOARD_CHOICE_SEARCH:
                board->state = GROUP_BOARD_SEARCH;
                WinSlide_Step(&board->panelWin, 640, 320);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
                break;
            }
        } else if (pad & PAD_CROSS) {
            GroupBoard_Close();
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else {
            board->choice = Menu_WrapUpDown(board->choice, 2);
            Menu_SetYesNo(board->choice);
        }
        break;
    case GROUP_BOARD_AREA:
        switch (GroupBoard_AreaGroups(pad)) {
        case MENU_DONE:
            board->state = GROUP_BOARD_STAMP;
            board->stamped = 1;
            Win_Shake(&board->stampShake);
            break;
        case LOBBY_STEP_FAILED:
        case MENU_CANCEL:
            gLobbyMenu->redraw = 1;
            board->state = GROUP_BOARD_ASK;
            LobbyText_Open(LOBBY_TEXT_GROUP_BOARD_ASK);
            Menu_SetYesNo(board->choice);
            break;
        }
        break;
    case GROUP_BOARD_SEARCH:
        switch (GroupBoard_Search(pad)) {
        case MENU_DONE:
            board->state++;
            board->stamped = 1;
            Win_Shake(&board->stampShake);
            break;
        case MENU_CANCEL:
            board->state = GROUP_BOARD_ASK;
            gLobbyMenu->redraw = 1;
            LobbyText_Open(LOBBY_TEXT_GROUP_BOARD_ASK);
            Menu_SetYesNo(board->choice);
            break;
        case LOBBY_STEP_FAILED:
            board->state = GROUP_BOARD_ERROR;
            gLobbyMenu->redraw = 1;
            LobbyDialog_OpenWait(gLobby->errText);
            break;
        }
        break;
    case GROUP_BOARD_STAMP:
        switch (Win_Shake(&board->stampShake)) {
        case MENU_DONE:
            board->state++;
            break;
        }
        break;
    case GROUP_BOARD_ENTER_TOWN:
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
    case GROUP_BOARD_ERROR:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            GroupBoard_Close();
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
}

extern NetLevel1SearchReply *gGroupSearchResults;
struct NetLevel1SearchQuery;
/*
 * The group search: slides the search form in, runs it, sends the search
 * and either lists the results or shows that none was found. Returns
 * MENU_DONE once a group is entered, LOBBY_STEP_FAILED on a server error
 * (its text shown), MENU_CANCEL when left, MENU_RUNNING meanwhile.
 */
s32 GroupBoard_Search(u16 pad)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;

    switch (board->step) {
    case GROUP_SEARCH_SLIDE_IN:
        switch (WinSlide_Step(&board->panelWin, 640, 320)) {
        case MENU_DONE:
            board->step++;
            break;
        }
        break;
    case GROUP_SEARCH_FORM:
        switch (GroupBoard_SearchInput(pad)) {
        case MENU_DONE:
            board->step++;
            LobbyText_Open(LOBBY_TEXT_GROUP_SEARCHING);
            gLobbyMenu->redraw = 1;
            break;
        case MENU_CANCEL:
            board->step = GROUP_SEARCH_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case GROUP_SEARCH_SEND:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel1_Search(&board->query)) {
        case LOBBY_STEP_DONE:
            if (gGroupSearchResults->found.total == 0) {
                board->step = GROUP_SEARCH_NONE_FOUND;
                LobbyText_Open(LOBBY_TEXT_GROUP_NONE_FOUND);
                gLobbyMenu->redraw = 1;
            } else {
                board->step++;
                board->pageCount = Menu_CountPages(gGroupSearchResults->found.total, GROUP_ROWS_PER_PAGE);
                WinSlide_Step(&board->listWin, -260, 54);
            }
            break;
        case LOBBY_STEP_FAILED:
            board->step = GROUP_SEARCH_SLIDE_IN;
            return LOBBY_STEP_FAILED;
        }
        break;
    case GROUP_SEARCH_RESULTS:
        switch (GroupBoard_Results(pad)) {
        case MENU_DONE:
            board->step = GROUP_SEARCH_SLIDE_IN;
            return MENU_DONE;
        case MENU_CANCEL:
            board->step = GROUP_SEARCH_SLIDE_IN;
            return MENU_CANCEL;
        case LOBBY_STEP_FAILED:
            board->step = GROUP_SEARCH_SLIDE_IN;
            return LOBBY_STEP_FAILED;
        }
        break;
    case GROUP_SEARCH_NONE_FOUND:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            board->step = GROUP_SEARCH_FORM;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return MENU_RUNNING;
}

extern u8 gCurServerId[];
extern NetLevel1SearchInfo gGroupSearchInfo;
/*
 * The search results: the cursor moves over pages of GROUP_ROWS_PER_PAGE
 * groups, and circle fetches the group's details and slides them in. On
 * the details, square moves a cursor over the members (circle opens a
 * member's hunter card), left and right turn between the members and the
 * location page, and circle asks whether to join (refused when the group's
 * limit doesn't admit the player's hunter rank). A yes on the player's own
 * land enters the group at once; on another land it sets gLobby's group
 * join and disconnects, to connect again to the group's land. Returns as
 * GroupBoard_Search.
 */
s32 GroupBoard_Results(u16 pad)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    NetLevel1SearchInfo *info = &gGroupSearchInfo;
    s32 i;

    switch (board->subStep) {
    case GROUP_RESULTS_SLIDE_IN:
        switch (WinSlide_Step(&board->listWin, -260, 54)) {
        case MENU_DONE:
            board->subStep++;
            break;
        }
        break;
    case GROUP_RESULTS_LIST:
        if (pad & PAD_CIRCLE) {
            board->subStep = GROUP_RESULTS_FETCH;
            gLobbyMenu->redraw = 1;
            LobbyText_Open(LOBBY_TEXT_GROUP_FETCHING);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            board->subStep = GROUP_RESULTS_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            Menu_GridMove((u8 *) &board->rowCursor, &board->page, GROUP_ROWS_PER_PAGE, gGroupSearchResults->found.total, pad);
        }
        break;
    case GROUP_RESULTS_FETCH:
        gLobbyMenu->redraw = 1;
        /* The chosen record's index, its address added up as the results'
         * base plus its offset (8: the head and found.total before
         * found.list): indexing found.list adds them the other way round. */
        switch (LobbyLevel1_FetchInfo(((NetLevel1SearchRec *)((s32)gGroupSearchResults + (board->rowCursor + board->page * GROUP_ROWS_PER_PAGE) * (s32)sizeof(NetLevel1SearchRec) + 8))->index)) {
        case LOBBY_STEP_DONE:
            board->subStep++;
            board->infoPage = 0;
            WinSlide_Step(&board->panelWin, 640, 320);
            board->memberCursor = 0;
            board->memberCount = 0;
            for (i = 0; i < GROUP_MEMBERS_SHOWN; i++) {
                if (info->users[i].id[0]) {
                    board->memberCount++;
                }
            }
            break;
        case LOBBY_STEP_FAILED:
            board->subStep = GROUP_RESULTS_SLIDE_IN;
            return LOBBY_STEP_FAILED;
        }
        break;
    case GROUP_RESULTS_SLIDE_PANEL:
        switch (WinSlide_Step(&board->panelWin, 640, 320)) {
        case MENU_DONE:
            board->subStep++;
            break;
        }
        break;
    case GROUP_RESULTS_DETAILS:
        board->joinStep = 0;
        if (pad & PAD_CIRCLE) {
            switch (info->groupInfo[0]) {
            case GROUP_LIMIT_HR_BELOW_31:
                if (Lobby_HrAtLeast31() == 1) {
                    board->subStep = GROUP_RESULTS_RANK_NOTICE;
                    gLobbyMenu->redraw = 1;
                    LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
                    LobbySe_Play(LOBBY_SE_REFUSE);
                    return MENU_RUNNING;
                }
                break;
            case GROUP_LIMIT_HR_31_UP:
                if (Lobby_HrAtLeast31() == 0) {
                    board->subStep = GROUP_RESULTS_RANK_NOTICE;
                    gLobbyMenu->redraw = 1;
                    LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
                    LobbySe_Play(LOBBY_SE_REFUSE);
                    return MENU_RUNNING;
                }
                break;
            }
            board->subStep++;
            board->joinStep = 1;
            if (memcmp(gCurServerId, info->serverId, SERVER_ID_LEN) == 0) {
                LobbyText_Open(LOBBY_TEXT_GROUP_JOIN_ASK);
            } else {
                LobbyText_Open(LOBBY_TEXT_GROUP_JOIN_MOVE_ASK);
            }
            Menu_SetYesNo(1);
            LobbyDialog_Grow();
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            board->subStep = GROUP_RESULTS_LIST;
            board->memberView = MEMBER_VIEW_HINTS;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_SQUARE) {
            if (board->infoPage == GROUP_INFO_MEMBERS && board->memberCount != 0) {
                board->subStep = GROUP_RESULTS_MEMBERS;
                board->memberCursor = 0;
                board->memberView = MEMBER_VIEW_CURSOR;
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            board->infoPage = Menu_WrapLeftRight(board->infoPage, board->infoPages);
            if (board->infoPage == GROUP_INFO_MEMBERS) {
                board->memberView = MEMBER_VIEW_HINTS;
            } else {
                board->memberView = MEMBER_VIEW_CURSOR;
            }
        }
        break;
    case GROUP_RESULTS_MEMBERS:
        if (pad & PAD_CIRCLE) {
            board->subStep = GROUP_RESULTS_HUNTER_CARD;
            board->memberView = MEMBER_VIEW_HUNTER_CARD;
            HunterInfo_Open(&info->users[board->memberCursor], HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND | HUNTER_INFO_JOIN);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            board->subStep = GROUP_RESULTS_DETAILS;
            board->memberView = MEMBER_VIEW_HINTS;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & (PAD_UP | PAD_DOWN)) {
            board->memberCursor = Menu_WrapUpDown(board->memberCursor, board->memberCount);
        }
        break;
    case GROUP_RESULTS_HUNTER_CARD:
        switch (HunterInfo_Update(pad)) {
        case MENU_DONE:
        case 1:
        case MENU_CANCEL:
            board->subStep = GROUP_RESULTS_MEMBERS;
            board->memberView = MEMBER_VIEW_CURSOR;
            break;
        }
        break;
    case GROUP_RESULTS_JOIN_ASK:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            if (memcmp(gCurServerId, info->serverId, SERVER_ID_LEN) == 0) {
                board->subStep++;
                LobbyText_Open(LOBBY_TEXT_SENDING);
            } else {
                gLobby->groupJoin = 1;
                memcpy(gLobby->groupId, info->serverId, sizeof(info->serverId));
                gLobby->area = info->level0Id;
                gLobby->land = info->level1Id;
                Lobby_Disconnect(0);
                FadeA_Start(10);
                Stream_Stop(0);
                Stream_Stop(1);
            }
            break;
        case MENU_CANCEL:
            board->subStep = GROUP_RESULTS_DETAILS;
            break;
        }
        break;
    case GROUP_RESULTS_ENTER:
        gLobbyMenu->redraw = 1;
        switch (LobbyLogin_EnterEntry(info->level0Id, info->level1Id)) {
        case LOBBY_STEP_DONE:
            board->subStep = GROUP_RESULTS_SLIDE_IN;
            board->viaSearch = 1;
            return MENU_DONE;
        case LOBBY_STEP_FAILED:
            board->subStep = GROUP_RESULTS_SLIDE_IN;
            return LOBBY_STEP_FAILED;
        }
        break;
    case GROUP_RESULTS_RANK_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            board->subStep = GROUP_RESULTS_DETAILS;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Fills the search's request (board->query) from the form: up to
 * LEVEL1_SEARCH_MAX_FOUND results, a limit condition from the limit choice
 * (enum GroupSearchLimit; the player's own band is GROUP_LIMIT_HR_31_UP or
 * GROUP_LIMIT_HR_BELOW_31 by his hunter rank) and a target condition (any,
 * or the marked quests).
 */
void GroupBoard_BuildSearch(void)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s32 condIdx = 0;
    s32 i;
    s32 j;
    s32 valueIdx;
    s32 target;

    memset(&board->query, 0, sizeof(board->query));
    board->query.maxFound = LEVEL1_SEARCH_MAX_FOUND;
    switch (board->searchLimit) {
    case GROUP_SEARCH_LIMIT_ANY:
        board->query.n++;
        condIdx++;
        board->query.cond[0].type = LEVEL1_SEARCH_COND_LIMIT;
        board->query.cond[0].n = 1;
        board->query.cond[0].values[0] = LEVEL1_SEARCH_ANY;
        break;
    case GROUP_SEARCH_LIMIT_NONE:
        board->query.n++;
        condIdx++;
        board->query.cond[0].type = LEVEL1_SEARCH_COND_LIMIT;
        board->query.cond[0].n = 1;
        board->query.cond[0].values[0] = GROUP_LIMIT_NONE;
        break;
    case GROUP_SEARCH_LIMIT_OWN:
        board->query.n++;
        board->query.cond[0].type = LEVEL1_SEARCH_COND_LIMIT;
        board->query.cond[0].n = 1;
        if (Lobby_HrAtLeast31() == 1) {
            board->query.cond[0].values[0] = GROUP_LIMIT_HR_31_UP;
        } else {
            board->query.cond[0].values[0] = GROUP_LIMIT_HR_BELOW_31;
        }
        condIdx++;
        break;
    case GROUP_SEARCH_LIMIT_NONE_OWN:
        board->query.n++;
        board->query.cond[0].type = LEVEL1_SEARCH_COND_LIMIT;
        board->query.cond[0].n = 2;
        board->query.cond[0].values[0] = GROUP_LIMIT_NONE;
        if (Lobby_HrAtLeast31() == 1) {
            board->query.cond[0].values[1] = GROUP_LIMIT_HR_31_UP;
        } else {
            board->query.cond[0].values[1] = GROUP_LIMIT_HR_BELOW_31;
        }
        condIdx++;
        break;
    }
    if (board->targetsChosen == 0) {
        board->query.n++;
        board->query.cond[condIdx].type = LEVEL1_SEARCH_COND_TARGET;
        board->query.cond[condIdx].n++;
        board->query.cond[condIdx].values[0] = LEVEL1_SEARCH_ANY;
    } else {
        board->query.n++;
        board->query.cond[condIdx].type = LEVEL1_SEARCH_COND_TARGET;
        valueIdx = 0;
        for (i = 0; i < GROUP_TARGET_PAGES; i++) {
            for (j = 0; j < GROUP_TARGETS_PER_PAGE; j++) {
                target = board->targets[i][j];
                if (target < 0) {
                    board->query.cond[condIdx].n++;
                    board->query.cond[condIdx].values[valueIdx++] = -1 - target;
                }
            }
        }
    }
}

/*
 * The search form's input: up and down move over the rows, left and right
 * change the limit, circle on the targets opens the target list and circle
 * on the start builds the request. In the target list circle marks or
 * unmarks a quest (at most GROUP_SEARCH_TARGET_MAX), and the first entry
 * clears every mark. Returns MENU_DONE to search, MENU_CANCEL when left,
 * MENU_RUNNING meanwhile.
 */
s32 GroupBoard_SearchInput(u16 pad)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 i;
    s16 j;
    s16 *target;

    switch (board->subStep) {
    case GROUP_FORM_ROWS:
        if (pad & PAD_CIRCLE) {
            if (board->row == GROUP_SEARCH_ROW_TARGETS) {
                board->subStep++;
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else if (board->row == GROUP_SEARCH_ROW_START) {
                GroupBoard_BuildSearch();
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (board->row == GROUP_SEARCH_ROW_LIMIT) {
                board->searchLimit = Menu_WrapLeftRight(board->searchLimit, GROUP_SEARCH_LIMITS);
            }
        } else {
            board->row = Menu_WrapUpDown(board->row, GROUP_SEARCH_ROWS);
        }
        break;
    case GROUP_FORM_TARGETS:
        if (pad & PAD_CROSS) {
            board->subStep = GROUP_FORM_ROWS;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_CIRCLE) {
            target = (s16 *)board->targets + board->targetPage * GROUP_TARGETS_PER_PAGE + board->targetCursor;
            if (*target != GROUP_TARGET_ANY) {
                if (*target >= 0 && board->targetsChosen >= GROUP_SEARCH_TARGET_MAX) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                } else {
                    *target *= -1;
                    if (board->targets[board->targetPage][board->targetCursor] < 0) {
                        board->targetsChosen++;
                    } else {
                        board->targetsChosen--;
                    }
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
            } else {
                for (i = 0; i < GROUP_TARGET_PAGES; i++) {
                    for (j = 0; j < GROUP_TARGETS_PER_PAGE; j++) {
                        board->targets[i][j] = abs(board->targets[i][j]);
                    }
                }
                board->targetsChosen = 0;
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (board->targetPages > 1) {
                board->targetPage = Menu_WrapLeftRight(board->targetPage, board->targetPages);
                board->targetCursor = 0;
            }
        } else {
            board->targetCursor = Menu_WrapUpDown(board->targetCursor, board->targetsOnPage[board->targetPage]);
        }
        break;
    }
    return MENU_RUNNING;
}

s32 AreaBoard_LeaveEntry(u16 pad);
struct AreaBoardMenu;
s32 AreaBoard_Join(u16 pad, struct AreaBoardMenu *w, s32 slot);
/*
 * The area's groups: slides the list in and runs it. A free entry chosen
 * leads to the new group's set-up, an open one to the area board's join
 * (AreaBoard_Join); a new group cancelled or timed out is given up
 * (AreaBoard_LeaveEntry) and the list comes back. Returns MENU_DONE once a
 * group is opened or joined, MENU_CANCEL when left, MENU_RUNNING
 * meanwhile; failures show a notice and return to the list.
 */
s32 GroupBoard_AreaGroups(u16 pad)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;

    switch (board->step) {
    case GROUP_AREA_SLIDE_IN:
        switch (WinSlide_Step(&board->listWin, -260, 54)) {
        case MENU_DONE:
            board->step++;
            break;
        }
        break;
    case GROUP_AREA_LIST:
        switch (GroupBoard_ListInput(pad)) {
        case MENU_DONE:
            switch (board->entryChoice) {
            case GROUP_ENTRY_NEW:
                board->step = GROUP_AREA_NEW_GROUP;
                board->row = 0;
                board->showForm = 1;
                break;
            case GROUP_ENTRY_JOIN:
            case 4: /* GroupBoard_ListInput never sets it */
                board->step = GROUP_AREA_JOIN;
                board->showForm = 0;
                LobbyText_Open(LOBBY_TEXT_GROUP_FETCHING);
                WinSlide_Step(&board->joinWin, 640, 320);
                break;
            }
            break;
        case MENU_CANCEL:
            board->step = GROUP_AREA_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case GROUP_AREA_NEW_GROUP:
        switch (GroupBoard_NewGroup(pad)) {
        case MENU_DONE:
            board->step = GROUP_AREA_SLIDE_IN;
            return MENU_DONE;
        case MENU_CANCEL:
            board->step = GROUP_AREA_LEAVE;
            LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
            break;
        case LOBBY_STEP_FAILED:
            board->step = GROUP_AREA_NOTICE;
            break;
        }
        break;
    case GROUP_AREA_LEAVE:
        gLobbyMenu->redraw = 1;
        switch (AreaBoard_LeaveEntry(pad)) {
        case LOBBY_STEP_DONE:
            board->step = GROUP_AREA_LIST;
            gLobbyLevel1Sel = 0;
            break;
        case LOBBY_STEP_FAILED:
            board->step = GROUP_AREA_NOTICE;
            break;
        }
        break;
    case GROUP_AREA_JOIN:
        switch (AreaBoard_Join(pad, (struct AreaBoardMenu *)board, board->entryNo)) {
        case MENU_DONE:
            board->step = GROUP_AREA_SLIDE_IN;
            return MENU_DONE;
        case MENU_CANCEL:
            board->step = GROUP_AREA_LIST;
            break;
        case LOBBY_STEP_FAILED:
            board->step = GROUP_AREA_NOTICE;
            break;
        }
        break;
    case GROUP_AREA_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            board->step = GROUP_AREA_LIST;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return MENU_RUNNING;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/group_board", gGroupBoardNumFmt);
INCLUDE_RODATA("asm/lobby/nonmatchings/group_board", gGroupBoardStrFmt);

extern u16 gRecruitFrames;
s32 LobbyStep_Level1Open(u16 id); /* unit's own declaration: the entry is passed as a u16 here; with the definition's s32, GroupBoard_NewGroup compiles differently */
void AreaBoard_LoadPage(s16 page);
struct RoomJoin;
s16 GroupBoard_SendSettings(struct RoomJoin *p);
/*
 * Opens the chosen free entry as the player's group
 * (LobbyStep_Level1Open), slides in the new group's form and runs it
 * against the time limit (gRecruitFrames), then sends the group's colour,
 * recruiting text and settings (GroupBoard_SendSettings). Returns
 * MENU_DONE when sent, LOBBY_STEP_FAILED on a server error, MENU_CANCEL
 * when the form is left or runs out of time.
 */
s32 GroupBoard_NewGroup(u16 pad)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;

    switch (board->subStep) {
    case GROUP_NEW_OPEN:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level1Open(board->entryNo)) {
        case LOBBY_STEP_DONE:
            board->subStep++;
            board->formTimer = gRecruitFrames;
            board->row = 0;
            board->targetCursor = 0;
            board->join.textIdx = 0;
            AreaBoard_LoadPage(0);
            memset(board->join.groupInfo, 0, sizeof(board->join.groupInfo));
            WinSlide_Step(&board->panelWin, 640, 320);
            break;
        case LOBBY_STEP_FAILED:
            board->subStep = GROUP_NEW_OPEN;
            LobbyDialog_OpenWait(gLobby->errText);
            return LOBBY_STEP_FAILED;
        }
        break;
    case GROUP_NEW_SLIDE_IN:
        switch (WinSlide_Step(&board->panelWin, 640, 320)) {
        case MENU_DONE:
            board->subStep++;
            break;
        }
        break;
    case GROUP_NEW_FORM:
        switch (GroupBoard_NewGroupInput(pad)) {
        case MENU_DONE:
            board->subStep++;
            LobbyText_Open(LOBBY_TEXT_SENDING);
            break;
        case MENU_CANCEL:
            board->subStep = GROUP_NEW_OPEN;
            return MENU_CANCEL;
        }
        break;
    case GROUP_NEW_SEND:
        switch (GroupBoard_SendSettings(&board->join)) {
        case LOBBY_STEP_DONE:
            board->subStep = GROUP_NEW_OPEN;
            return MENU_DONE;
        case LOBBY_STEP_FAILED:
            board->subStep = GROUP_NEW_OPEN;
            return LOBBY_STEP_FAILED;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The new group's form: counts the time limit down (at 0 the form is left
 * as cancelled) and moves over its rows. Left and right on the limit
 * toggle between no limit and the player's own rank band; circle on the
 * target or the text opens its picker, circle on the last row confirms.
 * Returns MENU_DONE on confirm, MENU_CANCEL when left or out of time,
 * MENU_RUNNING meanwhile.
 */
s32 GroupBoard_NewGroupInput(u16 pad)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;

    if (--board->formTimer <= 0) {
        board->picker = GROUP_PICK_NONE;
        LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    switch (board->picker) {
    case GROUP_PICK_NONE:
        if (pad & PAD_CIRCLE) {
            switch (board->row) {
            case GROUP_NEW_ROW_TARGET:
                board->picker = GROUP_PICK_TARGET;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case GROUP_NEW_ROW_TEXT:
                board->picker = GROUP_PICK_TEXT;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case GROUP_NEW_ROW_CONFIRM:
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            board->join.groupInfo[0] = 0;
            board->join.groupInfo[1] = 0;
            board->targetPage = 0;
            board->targetCursor = 0;
            board->textCursor = 0;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (board->row == GROUP_NEW_ROW_LIMIT) {
                if (Lobby_HrAtLeast31() == 1) {
                    board->join.groupInfo[0] ^= GROUP_LIMIT_HR_31_UP;
                } else {
                    board->join.groupInfo[0] ^= GROUP_LIMIT_HR_BELOW_31;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            board->row = Menu_WrapUpDown(board->row, GROUP_NEW_ROWS);
        }
        break;
    case GROUP_PICK_TARGET:
        if (pad & PAD_CIRCLE) {
            board->picker = GROUP_PICK_NONE;
            board->join.groupInfo[1] = board->targets[board->targetPage][board->targetCursor];
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            board->picker = GROUP_PICK_NONE;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (board->targetPages > 1) {
                board->targetPage = Menu_WrapLeftRight(board->targetPage, board->targetPages);
                board->targetCursor = 0;
            }
        } else {
            board->targetCursor = Menu_WrapUpDown(board->targetCursor, board->targetsOnPage[board->targetPage]);
        }
        break;
    case GROUP_PICK_TEXT:
        if (pad & PAD_CIRCLE) {
            board->picker = GROUP_PICK_NONE;
            board->join.textIdx = board->textCursor;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            board->picker = GROUP_PICK_NONE;
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
    }
    return MENU_RUNNING;
}

extern LevelEntry gLobbyLevel1Entries[];
/*
 * The area's group list: the cursor moves over pages of
 * GROUP_ROWS_PER_PAGE entries. Circle on a free entry chooses to open a
 * new group there, on an open one to join it, and is refused on the
 * others. Returns MENU_DONE with the choice and the entry (board->entryNo,
 * from 1) kept, MENU_CANCEL when left, MENU_RUNNING meanwhile.
 */
s32 GroupBoard_ListInput(u16 pad)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s32 entryIdx;
    u8 entryState;

    if (pad & PAD_CIRCLE) {
        entryIdx = board->rowCursor + board->page * GROUP_ROWS_PER_PAGE;
        entryState = gLobbyLevel1Entries[entryIdx].state;
        if (entryState == LEVEL_ENTRY_FREE) {
            board->entryChoice = GROUP_ENTRY_NEW;
            board->entryNo = entryIdx + 1;
            LobbyText_Open(LOBBY_TEXT_GROUP_OPENING);
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_DONE;
        } else if (entryState == LEVEL_ENTRY_OPEN || entryState == LEVEL_ENTRY_OPEN_ALT) {
            board->entryChoice = GROUP_ENTRY_JOIN;
            board->entryNo = entryIdx + 1;
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_DONE;
        } else {
            LobbySe_Play(LOBBY_SE_REFUSE);
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        board->page = Menu_WrapLeftRight(board->page, board->pageCount);
    } else {
        board->rowCursor = Menu_WrapUpDown(board->rowCursor, GROUP_ROWS_PER_PAGE);
    }
    return MENU_RUNNING;
}

extern char gGroupBoardNumFmt[];
extern char gGroupBoardStrFmt[];
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: s16 coordinates and an s32 kind here; with the definition's s32 coordinates and s16 kind, the list and picker draws compile differently */
void Font_SetColor(s32 color);
/*
 * Draws the area's group list: the title and page number while the list
 * has the cursor, the column headers, then for each of the page's rows the
 * number, the limit, the target quest, the recruiting text in the entry's
 * colour, the users and the users in town (grey when the entry can't be
 * joined), a dash line past the list's end, the cursor bar and the help
 * box.
 */
void GroupBoard_DrawAreaGroups(s16 x, s16 y)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    LevelEntry *entry;
    u32 color;
    s32 i;
    s32 entryIdx;
    s16 rowY = y;
    s8 dim;

    if (board->step == GROUP_AREA_LIST) {
        Font_SetSpacing(22, 22);
        Menu_DrawLabelBar(20, 18, 240, 0xFFB0B000, gNetLabels[NET_LABEL_GROUP_BOARD_TITLE], 2, 1);
        Menu_DrawPageNum(x + 512, 22, board->page, board->pageCount, 1);
    }
    Win_Draw(x, y, 612, 250, 0xBDB59F6C, 0xB02040F0, 7);
    Font_SetColor(0x807F7F28);
    Font_SetSpacing(18, 18);
    Font_SetPos(x + 20, rowY += 2);
    Text_PrintfColored(gNetLabels[NET_LABEL_NUMBER]);
    Font_SetPos(x + 60, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_LIMITED]);
    Font_SetPos(x + 120, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_TARGET]);
    Font_SetPos(x + 314, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_RECRUIT_MSG]);
    Font_SetPos(x + 510, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_HEADCOUNT]);
    Font_SetPos(x + 570, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_IN_TOWN]);
    for (i = 0; i < GROUP_ROWS_PER_PAGE; i++) {
        rowY += 22;
        entryIdx = i + board->page * GROUP_ROWS_PER_PAGE;
        if ((u16)gLobbyLevel1Count > entryIdx) {
            entry = &gLobbyLevel1Entries[entryIdx];
            if (entry->users == 0 || (entry->state != LEVEL_ENTRY_OPEN && entry->state != LEVEL_ENTRY_OPEN_ALT)) {
                color = 0x80404040;
            } else {
                color = 0x807F7F7F;
            }
            Font_SetColor(color);
            Font_SetPos(x + 20, rowY);
            Text_PrintfColored(gGroupBoardNumFmt, entryIdx + 1);
            Font_SetPos(x + 60, rowY);
            Text_PrintfColored(gGroupLimitTexts[entry->groupInfo[0] + GROUP_LIMIT_SHORT_TEXT]);
            Font_SetPos(x + 120, rowY);
            Text_PrintfColored(QDL_ROOM->quests[entry->groupInfo[1]].name);
            if (color == 0x807F7F7F) {
                Text_SetColor(entry->color);
            }
            Font_SetPos(x + 314, rowY);
            Text_PrintfColored(gGroupBoardStrFmt, entry->text);
            Font_SetColor(color);
            Font_SetPos(x + 504, rowY);
            Text_PrintfColored("%2d/%2d", entry->users, entry->maxUsers);
            Font_SetPos(x + 570, rowY);
            Text_PrintfColored(gGroupBoardNumFmt, entry->users - entry->questUsers);
        } else {
            Font_SetColor(0x80404040);
            Font_SetPos(x + 10, rowY);
            Text_PrintfColored(gNetLabels[NET_LABEL_DASHES]);
        }
        if (i == board->rowCursor && board->state == GROUP_BOARD_AREA) {
            switch (board->step) {
            case GROUP_AREA_SLIDE_IN:
            case GROUP_AREA_LIST:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawRowHighlight(x, rowY - 1, 612, 20, dim);
        }
    }
    Win_DrawText(300, 340, 320, 80, 0xBDB59F6C, 0xC0101020, gNetLabels[NET_LABEL_JOIN_PROMPT]);
}

/*
 * Draws the search results as GroupBoard_DrawAreaGroups draws the area's
 * list, with a land column and the recruiting text cut short; the cursor
 * bar is dimmed once a group's details are open.
 */
void GroupBoard_DrawResults(s16 x, s16 y)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    NetLevel1SearchRec *rec;
    s32 color;
    s32 i;
    s32 recIdx;
    s16 rowY = y;
    s8 dim;
    char shortText[32];

    if (board->subStep != GROUP_RESULTS_SLIDE_IN) {
        Font_SetSpacing(22, 22);
        Menu_DrawLabelBar(20, 18, 240, 0xFFB0B000, gNetLabels[NET_LABEL_SEARCH_RESULTS], 2, 1);
        Menu_DrawPageNum(x + 512, 22, board->page, board->pageCount, 1);
    }
    Win_Draw(x, y, 612, 250, 0xBDB59F6C, 0xB02040F0, 7);
    Font_SetColor(0x807F7F28);
    Font_SetSpacing(18, 18);
    Font_SetPos(x + 20, rowY += 2);
    Text_PrintfColored(gNetLabels[NET_LABEL_NUMBER]);
    Font_SetPos(x + 50, rowY);
    Text_PrintfColored(gLobbyWords[LOBBY_WORD_LAND]);
    Font_SetPos(x + 156, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_LIMITED]);
    Font_SetPos(x + 210, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_TARGET]);
    Font_SetPos(x + 404, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_RECRUIT_MSG]);
    Font_SetPos(x + 516, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_HEADCOUNT]);
    Font_SetPos(x + 570, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_IN_TOWN]);
    for (i = 0; i < GROUP_ROWS_PER_PAGE; i++) {
        rowY += 22;
        recIdx = i + board->page * GROUP_ROWS_PER_PAGE;
        rec = &gGroupSearchResults->found.list[recIdx];
        if (gGroupSearchResults->found.total > recIdx) {
            color = rec->users == 0 ? 0x80404040 : 0x807F7F7F;
            Font_SetColor(color);
            Font_SetPos(x + 20, rowY);
            Text_PrintfColored(gGroupBoardNumFmt, recIdx + 1);
            Font_SetPos(x + 50, rowY);
            Text_PrintfColored(gGroupBoardStrFmt, LobbySrv_Name(rec->serverId));
            Font_SetPos(x + 156, rowY);
            Text_PrintfColored(gGroupLimitTexts[rec->groupInfo[0] + GROUP_LIMIT_SHORT_TEXT]);
            Font_SetPos(x + 210, rowY);
            Text_PrintfColored(QDL_ROOM->quests[rec->groupInfo[1]].name);
            Font_SetPos(x + 404, rowY);
            memset(shortText, 0, sizeof(shortText));
            memcpy(shortText, rec->text, 8);
            Text_PrintfColored("%s%s", shortText, gLobbyWords[LOBBY_WORD_ELLIPSIS]);
            Font_SetColor(color);
            Font_SetPos(x + 520, rowY);
            Text_PrintfColored("%d/%d", rec->users, rec->maxUsers);
            Font_SetPos(x + 570, rowY);
            Text_PrintfColored(gGroupBoardNumFmt, rec->usersInTown);
        }
        if (i == board->rowCursor) {
            if (board->step == GROUP_SEARCH_RESULTS) {
                switch (board->subStep) {
                case GROUP_RESULTS_SLIDE_IN:
                case GROUP_RESULTS_LIST:
                    dim = 0;
                    break;
                default:
                    dim = 1;
                    break;
                }
            } else {
                dim = 1;
            }
            Menu_DrawRowHighlight(x, rowY - 1, 612, 20, dim);
        }
    }
    Win_DrawText(300, 340, 320, 80, 0xBDB59F6C, 0xC0101020, gNetLabels[NET_LABEL_JOIN_PROMPT]);
}

extern u8 gLobbyLevel0Sel;
extern LevelEntry gLobbyLevel0Entries[];
/*
 * Draws the new group's form: the entry's name over the area's emblem, the
 * limit (with arrows while it has the cursor), the target quest and the
 * recruiting text in their frames, the confirm row, the seconds left, and
 * the stamp once the group is opened.
 */
void GroupBoard_DrawNewGroupForm(s16 x, s16 y)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 rowY = y;
    s16 centerX;
    char *label;
    s32 frameX;
    s16 labelX;
    u32 areaColor;
    s32 topY;
    LevelEntry *entry;
    s32 leftX;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(320, 18, 200, 0xFFB0B000, gNetLabels[NET_LABEL_RECRUIT_MEMBERS], 2, 1);
    Font_SetSpacing(18, 18);
    Icon_Draw(x, y, 300, 290, 0xFFE0D8C8, 1);
    areaColor = AreaList_KindColor(gLobbyLevel0Entries[gLobbyLevel0Sel - 1].kind);
    topY = rowY;
    LobbyUi_DrawLevelMarks(x + 150, topY + 145, 112, areaColor, 3);
    Font_SetColor(0x80101010);
    centerX = x + 150;
    entry = &gLobbyLevel1Entries[board->entryNo - 1];
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(entry->name, centerX, 18), rowY += 20);
    Text_PrintfColored(entry->name);
    Font_SetPos(Text_CenterX(gNetLabels[NET_LABEL_RECRUIT_MEMBERS], centerX, 18), rowY += 22);
    leftX = x + 30;
    Font_SetPos(leftX, rowY += 40);
    Text_PrintfColored(gNetLabels[NET_LABEL_LIMIT_COND]);
    Font_SetColor(0x80101010);
    Font_SetPos(x + 156, rowY);
    label = gGroupLimitTexts[board->join.groupInfo[0]];
    Text_PrintfColored(label);
    if (board->row == GROUP_NEW_ROW_LIMIT && board->picker == GROUP_PICK_NONE) {
        Menu_DrawArrows(x + 156, rowY, 18, label, 0xFF106060);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(leftX, rowY += 30);
    Text_PrintfColored(gNetLabels[NET_LABEL_TARGET_COLON]);
    Font_SetColor(0x80101010);
    label = QDL_ROOM->quests[board->join.groupInfo[1]].name;
    Font_SetPos(Text_CenterX(label, centerX, 18), rowY + 24);
    Text_PrintfColored(label);
    if (board->row == GROUP_NEW_ROW_TARGET) {
        Gfx_DrawRectFrame(frameX = x + 50, rowY + 22, 216, 22, 0xFF106060);
        if (board->picker == GROUP_PICK_NONE) {
            Menu_DrawSideArrows(frameX, rowY + 22, 216, 22, 0, 0xFF106060);
        }
    } else {
        Gfx_DrawRectFrame(frameX = x + 50, rowY + 22, 216, 22, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(leftX, rowY += 50);
    Text_PrintfColored(gNetLabels[NET_LABEL_RECRUIT_MSG_LABEL]);
    Font_SetColor(0x80101010);
    label = QDL_ROOM->slots[board->textSlots[board->join.textIdx]].text;
    Font_SetPos(Text_CenterX(label, centerX, 18), rowY + 24);
    Text_PrintfColored(label);
    if (board->row == GROUP_NEW_ROW_TEXT) {
        Gfx_DrawRectFrame(frameX, rowY + 22, 216, 22, 0xFF106060);
        if (board->picker == GROUP_PICK_NONE) {
            Menu_DrawSideArrows(frameX, rowY + 22, 216, 22, 0, 0xFF106060);
        }
    } else {
        Gfx_DrawRectFrame(frameX, rowY + 22, 216, 22, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    labelX = Text_CenterX(gNetLabels[NET_LABEL_DECIDE], centerX, 18);
    Font_SetPos(labelX, rowY += 60);
    Text_PrintfColored(gNetLabels[NET_LABEL_DECIDE]);
    if (board->row == GROUP_NEW_ROW_CONFIRM && board->picker == GROUP_PICK_NONE && board->subStep == GROUP_NEW_FORM) {
        Menu_DrawArrowsRev(labelX, rowY - 2, 18, gNetLabels[NET_LABEL_DECIDE], 0xFF106060);
    }
    Font_SetPos(x + 156, topY + 256);
    Text_Printf(gArenaCupTexts[ARENA_TEXT_TIME_LEFT], board->formTimer / 30);
    if (board->stamped) {
        Menu_DrawStamp(x, y, 300, 290, (u16)board->stampSeed, board->stampShake.timer);
    }
}

/*
 * Draws the new group form's target picker: the page of the room's quests,
 * with the page number and the cursor bar.
 */
void GroupBoard_DrawTargetPicker(s16 x, s16 y)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 rowY = y;
    s16 i;
    s16 quest;

    Win_Draw(x, y, 280, 290, 0xBDB59F6C, 0xC0101010, 2);
    rowY += 10;
    Menu_DrawPageNum((s16)(x + 40) + 160, rowY, board->targetPage, board->targetPages, 1);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 40, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_TARGET]);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < board->targetsOnPage[board->targetPage]; i++) {
        quest = board->targets[board->targetPage][i];
        if (QDL_ROOM->quests[quest].used) {
            Font_SetPos(x + 40, rowY += 22);
            Text_PrintfColored(gGroupBoardStrFmt, QDL_ROOM->quests[quest].name);
            if (board->targetCursor == i) {
                Menu_DrawRowHighlight(x + 10, rowY - 1, 260, 20, 0);
            }
        }
    }
}


/*
 * Draws the search form: its title, the limit choice (the texts for the
 * player's rank band, with arrows while it has the cursor), the frame of
 * the marked target quests with their names, the start row and the help
 * box.
 */
void GroupBoard_DrawSearchForm(s16 x, s16 y)
{
    char *label;
    s16 rowY = y;
    s16 centerX;
    s16 labelX;
    s16 i;
    s16 j;
    s16 quest;
    s32 nameY;
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 *target;
    char *name;

    Font_SetSpacing(18, 18);
    Icon_Draw(x, y, 300, 290, 0xFFE0D8C8, 1);
    Font_SetColor(0x80101010);
    centerX = x + 150;
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gNetLabels[NET_LABEL_GROUP_SEARCH], centerX, 18), rowY += 22);
    Text_PrintfColored(gNetLabels[NET_LABEL_GROUP_SEARCH]);
    Font_SetPos(x + 30, rowY += 40);
    Text_PrintfColored(gNetLabels[NET_LABEL_LIMIT_COND]);
    if (Lobby_HrAtLeast31() == 1) {
        label = gGroupLimitExpTexts[board->searchLimit];
    } else {
        label = gGroupLimitNewTexts[board->searchLimit];
    }
    Font_SetColor(0x80101010);
    labelX = Text_CenterX(label, centerX, 18);
    Font_SetPos(labelX, rowY += 26);
    Text_PrintfColored(label);
    if (board->row == GROUP_SEARCH_ROW_LIMIT && board->subStep == GROUP_FORM_ROWS) {
        Menu_DrawArrows(labelX, rowY, 18, label, 0xFF106060);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(x + 30, rowY += 30);
    Text_PrintfColored(gNetLabels[NET_LABEL_TARGET_COLON]);
    Font_SetColor(0x80101010);
    if (board->row == GROUP_SEARCH_ROW_TARGETS) {
        Gfx_DrawRectFrame(x + 50, rowY + 22, 216, 87, 0xFF106060);
        if (board->subStep == GROUP_FORM_ROWS) {
            Menu_DrawSideArrows(x + 50, rowY + 50, 216, 22, 0, 0xFF106060);
        }
    } else {
        Gfx_DrawRectFrame(x + 50, rowY + 22, 216, 87, 0xFF606060);
    }
    rowY += 3;
    nameY = rowY;
    /* The marked targets. The pointer steps from the board's start, one row
     * of targets per page and one entry per target, and reads each entry at
     * targets' offset: indexing board->targets[i][j] compiles to a different
     * use of registers. */
    for (i = 0; i < GROUP_TARGET_PAGES; i++) {
        for (j = 0, target = (s16 *)((u8 *)board + i * GROUP_TARGETS_PER_PAGE * 2); j < GROUP_TARGETS_PER_PAGE; j++, target++) {
            if (target[3250 / 2] < 0) {
                quest = -target[3250 / 2];
                name = QDL_ROOM->quests[(s16)(quest - 1)].name;
                nameY += 22;
                Font_SetPos(Text_CenterX(name, centerX, 18), nameY);
                Text_PrintfColored(name);
            }
        }
    }
    Font_SetColor(0x80301010);
    labelX = Text_CenterX(gNetLabels[NET_LABEL_SEARCH_START], centerX, 18);
    Font_SetPos(labelX, rowY += 120);
    Text_PrintfColored(gNetLabels[NET_LABEL_SEARCH_START]);
    if (board->row == GROUP_SEARCH_ROW_START) {
        Menu_DrawArrowsRev(labelX, rowY - 2, 18, gNetLabels[NET_LABEL_SEARCH_START], 0xFF106060);
    }
    Win_DrawText(300, 340, 320, 80, 0xBDB59F6C, 0xC0101020, gGroupSearchHelpTexts[0]);
}


/*
 * Draws the search's target list: the page of quests with the page number
 * and the cursor bar, the first entry as any quest, and a mark beside each
 * marked quest.
 */
void GroupBoard_DrawSearchTargets(s16 x, s16 y)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 rowY = y;
    s16 i;
    s16 quest;

    Win_Draw(x, y, 280, 290, 0xBDB59F6C, 0xC0101010, 2);
    rowY += 10;
    Font_SetColor(0x807F7F7F);
    Menu_DrawPageNum((s16)(x + 40) + 160, rowY, board->targetPage, board->targetPages, 1);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 40, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_TARGET]);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < board->targetsOnPage[board->targetPage]; i++) {
        quest = board->targets[board->targetPage][i];
        Font_SetPos(x + 40, rowY += 22);
        if (board->targetCursor == i) {
            Menu_DrawRowHighlight(x + 10, rowY - 1, 260, 20, 0);
        }
        if (quest == GROUP_TARGET_ANY) {
            Text_PrintfColored(gGroupBoardStrFmt, gGroupLimitNewTexts[0]);
        } else {
            if (quest < 0) {
                Icon_Draw(x + 18, rowY, 20, 20, 0xFFFF0000, 128);
                quest *= -1;
            }
            Text_PrintfColored(gGroupBoardStrFmt, QDL_ROOM->quests[(s16)(quest - 1)].name);
        }
    }
}

void Icon_DrawWeaponClass(s16, s16, s32, s32, u8, s32); /* unit's own declaration: s16 coordinates and a u8 kind here; with the definition's types, GroupBoard_DrawGroupInfo compiles differently */
s32 sprintf(char *buf, const char *fmt, ...);
/*
 * Draws a searched group's details: its recruiting text and page number,
 * then either the members (name, hunter rank, weapon icon; the member
 * cursor while it is up) or the location (the land, its level-0 entry and
 * the group's text, with their icons); the stamp once joined, and the
 * button hints.
 */
void GroupBoard_DrawGroupInfo(s16 x, s16 y)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    u32 areaColor;
    char hrText[32];
    NetLevel1SearchInfo *info = &gGroupSearchInfo;
    NetMember10 *member;
    s16 rowY = y;
    char hrWide[32];
    s16 i;

    Font_SetSpacing(18, 18);
    Icon_Draw(x, y, 300, 290, 0xFFE0D8C8, 1);
    areaColor = AreaList_KindColor(gLobbyLevel0Entries[gLobbyLevel0Sel - 1].kind);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 210, y + 20, board->infoPage, board->infoPages, 1);
    Font_SetColor(0x80301010);
    Font_SetPos(x + 40, y + 20);
    Text_PrintfColored(gGroupSearchInfo.level1Text);
    rowY += 42;
    if (board->infoPage == GROUP_INFO_MEMBERS) {
        Font_SetColor(0x80301010);
        Font_SetPos(x + 40, rowY);
        Text_PrintfColored(gNetLabels[NET_LABEL_NAME]);
        Font_SetPos(x + 190, rowY);
        Text_PrintfColored(gNetLabels[NET_LABEL_HR]);
        Font_SetPos(x + 240, rowY);
        Text_PrintfColored(gNetLabels[NET_LABEL_WEAPON]);
        Font_SetColor(0x80101010);
        for (i = 0; i < GROUP_MEMBERS_SHOWN; i++) {
            rowY += 22;
            member = &info->users[i];
            if (member->id[0] != 0) {
                if (board->memberView == MEMBER_VIEW_CURSOR && i == board->memberCursor) {
                    Menu_DrawCursorRow(x + 40, rowY, 190, 20, 0xC0106060, 0xFF106060);
                }
                Font_SetPos(x + 40, rowY);
                Text_PrintfColored(gGroupBoardStrFmt, member->name);
                Font_SetPos(x + 172, rowY);
                sprintf(hrText, "%3d", member->hr);
                Sjis_Widen(hrText, hrWide);
                Text_PrintfColored(hrWide);
                Icon_DrawWeaponClass(x + 240, rowY, 24, -1, member->icon, 0);
            }
        }
    } else {
        Font_SetColor(0x80301010);
        Font_SetPos(x + 40, rowY);
        Text_PrintfColored(gNetLabels[NET_LABEL_LOCATION]);
        Font_SetColor(0x80101010);
        Font_SetPos(x + 100, rowY += 22);
        Text_PrintfColored(gGroupBoardStrFmt, LobbySrv_Name(info->serverId));
        Icon_Draw(x + 70, rowY - 2, 22, 22, LobbySrv_Color(info->serverId) | 0xB0000000, 117);
        Font_SetPos(x + 100, rowY += 22);
        Text_PrintfColored(gGroupBoardStrFmt, info->level0Name);
        Icon_Draw(x + 70, rowY - 2, 22, 22, areaColor, 118);
        Font_SetPos(x + 100, rowY += 22);
        Text_PrintfColored(gGroupBoardStrFmt, info->level1Text);
        Icon_Draw(x + 70, rowY - 2, 22, 22, areaColor, 119);
        Icon_Draw(x + 90, rowY + 15, strlen(info->level1Text) * 9 + 20, 8, 0xA0FF0000, 111);
    }
    if (board->stamped) {
        Menu_DrawStamp(x, y, 300, 290, board->stampSeed, board->stampShake.timer);
    }
    GroupBoard_DrawButtonHints(board, x + 40, y + 220, 0x80101010);
}

/*
 * Draws the three button hints under a group's details: square for the
 * member details (only at MEMBER_VIEW_HINTS), circle to confirm and cross
 * to cancel. A held button's icon is drawn a pixel lower, except while a
 * question is up or the dialog redraws. menu is the group board's work or
 * the area board's AreaBoardMenu, which share the fields read here.
 */
void GroupBoard_DrawButtonHints(void *menu, s16 x, s16 y, u32 color)
{
    GroupBoardMenu *board = (GroupBoardMenu *)menu;
    u16 pad = LobbyPad_Get(0);
    s16 pressed;

    Font_SetSpacing(18, 18);
    Font_SetColor(color);
    pressed = (pad & PAD_SQUARE) != 0;
    if (board->memberView == MEMBER_VIEW_HINTS) {
        Lobby_DrawButtonIcon(x, y + pressed, 3);
        Font_SetPos(x + 26, y + 2);
        Text_PrintfColored(gNetLabels[NET_LABEL_MEMBER_DETAILS]);
    }
    pressed = (pad & PAD_CIRCLE) != 0;
    if (board->joinStep != 0 || gLobbyMenu->redraw != 0) {
        pressed = 0;
    }
    y += 26;
    Lobby_DrawButtonIcon(x, y + pressed, 0);
    Font_SetPos(x + 26, y + 2);
    Text_PrintfColored(gNetLabels[NET_LABEL_DECIDE]);
    pressed = (pad & PAD_CROSS) != 0;
    if (board->joinStep != 0 || gLobbyMenu->redraw != 0) {
        pressed = 0;
    }
    x += 120;
    Lobby_DrawButtonIcon(x, y + pressed, 1);
    Font_SetPos(x + 26, y + 2);
    Text_PrintfColored(gNetLabels[NET_LABEL_CANCEL]);
}

/*
 * The board's first draw callback, the group list: the area's list (or the
 * search results, when the group came from the search) in the area's flow
 * and after joining; in the search, the results, or else the search form
 * with the target list while it is open.
 */
void GroupBoard_Draw(MenuTask *task)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 listY;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (board->state) {
    case GROUP_BOARD_AREA:
    case GROUP_BOARD_STAMP:
    case GROUP_BOARD_ENTER_TOWN:
        listY = board->listWin.pos;
        if (board->viaSearch == 0) {
            GroupBoard_DrawAreaGroups(14, listY);
        } else {
            GroupBoard_DrawResults(14, listY);
        }
        break;
    case GROUP_BOARD_SEARCH:
        if (board->step == GROUP_SEARCH_RESULTS) {
            GroupBoard_DrawResults(14, board->listWin.pos);
            break;
        }
        GroupBoard_DrawSearchForm(board->panelWin.pos, 50);
        if (board->step == GROUP_SEARCH_FORM && board->subStep == GROUP_FORM_TARGETS) {
            GroupBoard_DrawSearchTargets(30, 54);
        }
        break;
    }
}

void AreaBoard_DrawTextPicker(s16 x, s16 y, s16 cur, s16 n);
void AreaBoard_DrawMembers(s16 x, s32 y, u32 entryNo, WinShake *shake, u16 stampSeed, s8 stamped); /* unit's own declaration: an s32 y and a u32 entry here; with the definition's s16 y and s32 entry, GroupBoard_DrawDetails compiles differently */
/*
 * The board's second draw callback, the right-hand panel, shaken with the
 * stamp: a search result's details or a member's hunter card; the new
 * group's form with its target or text picker; the members of the group
 * being joined (AreaBoard_DrawMembers) or a hunter card; after joining,
 * the panel that was up.
 */
void GroupBoard_DrawDetails(MenuTask *task)
{
    GroupBoardMenu *board = (GroupBoardMenu *)&gMenuWork;
    s16 panelX;
    s16 joinX;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    panelX = board->panelWin.pos + board->stampShake.dx;
    switch (board->state) {
    case GROUP_BOARD_SEARCH:
        if (board->step == GROUP_SEARCH_RESULTS) {
            switch (board->subStep) {
            case GROUP_RESULTS_SLIDE_PANEL:
            case GROUP_RESULTS_DETAILS:
            case GROUP_RESULTS_JOIN_ASK:
            case GROUP_RESULTS_ENTER:
            case GROUP_RESULTS_MEMBERS:
                GroupBoard_DrawGroupInfo(panelX, 50);
                break;
            case GROUP_RESULTS_HUNTER_CARD:
                HunterInfo_DrawIfOpen();
                break;
            }
        }
        break;
    case GROUP_BOARD_AREA:
        if (board->step == GROUP_AREA_NEW_GROUP) {
            switch (board->subStep) {
            case GROUP_NEW_SEND:
                GroupBoard_DrawNewGroupForm(panelX, 50);
                break;
            case GROUP_NEW_SLIDE_IN:
            case GROUP_NEW_FORM:
                GroupBoard_DrawNewGroupForm(panelX, 50);
                switch (board->picker) {
                case GROUP_PICK_TARGET:
                    GroupBoard_DrawTargetPicker(20, 50);
                    break;
                case GROUP_PICK_TEXT:
                    AreaBoard_DrawTextPicker(20, 50, board->textCursor, board->textCount);
                    break;
                }
                break;
            }
        } else if (board->step == GROUP_AREA_JOIN) {
            joinX = board->joinWin.pos + board->stampShake.dx;
            if (board->showForm == 0) {
                if (board->memberView == MEMBER_VIEW_HUNTER_CARD) {
                    HunterInfo_DrawIfOpen();
                } else {
                    Font_SetSpacing(22, 22);
                    Menu_DrawLabelBar(320, 18, 200, 0xFFB0B000, gNetLabels[NET_LABEL_JOIN_GROUP], 2, 1);
                    AreaBoard_DrawMembers(joinX, 50, board->entryNo, &board->stampShake, board->stampSeed, board->stamped);
                }
            }
        }
        break;
    case GROUP_BOARD_STAMP:
    case GROUP_BOARD_ENTER_TOWN:
        if (board->viaSearch) {
            GroupBoard_DrawGroupInfo(panelX, 50);
        } else if (board->showForm) {
            GroupBoard_DrawNewGroupForm(panelX, 50);
        } else {
            AreaBoard_DrawMembers(board->joinWin.pos + board->stampShake.dx, 50, board->entryNo, &board->stampShake, board->stampSeed, board->stamped);
        }
        break;
    }
}
