/*
 * The arena cup's desk (TOWN_MODE_ARENA_CUP; its work area is gMenuWork as
 * lobby_menu.h's ArenaCupMenu): the online desk where the arena cup's quest
 * (QUEST_ID_ARENA_CUP) is entered with a monster kept from the arena
 * (gSave.arenaEms), the kept monsters are checked, renamed or released, and
 * the cup's record is shown.
 *
 * ArenaCup_Open opens it and ArenaCup_Close closes it; ArenaCup_Update runs
 * its steps (enum ArenaCupStep) each frame: the first list
 * (ArenaCup_ChooseQuest), the entry (ArenaCup_Post: the level-2 server's
 * quest room, the entry form ArenaCup_PostInput, the posting), the
 * monsters (ArenaCup_MonsterInput) and the record. ArenaCup_Draw and
 * ArenaCup_Draw2 are its draws: the entry form, the record, the monster
 * check with a monster's menu and details, and the held quest's sheet
 * during the drop talk. Arena_DrawMonsterList and Arena_DrawMonsterInfo are
 * shared with the arena menu.
 *
 * From ArenaCup_Update on the file is built at optimization level 4
 * (Arena_DrawMonsterInfo matches only there), and it has its own copy of
 * the plain string format (gArenaStrFmt).
 */
#include "common.h"
#include "lobby_text.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "quest.h"
#include "lobby_menu.h"
#include "lobby_net.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "kbd.h"
#include "text.h"
#include "window.h"
#include "arena_cup.h"
#include "draw_helpers.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_leave_menus.h"
#include "lobby_town.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "quest_counter.h"
#include "quest_desk.h"
#include "quest_desk_draw.h"
#include "quest_order_terms.h"
#include "rand.h"
#include "talk.h"
#include "word_filter.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u16 gLobbyQuestId;
extern u8 gMenuWork[];

/* 1 when the quest taken is the arena cup's (QUEST_ID_ARENA_CUP), else 0. */
s32 ArenaCup_QuestTaken(void)
{
    if (gLobby->questTaken == 0) {
        return 0;
    }
    if (gLobbyQuestId == QUEST_ID_ARENA_CUP) {
        return 1;
    }
    return 0;
}

extern s8 gMenuInputLock;

/*
 * Opens the arena cup's desk (npc, the clerk, is unused): TOWN_MODE_ARENA_CUP,
 * the work area cleared, the draws set, the season stored and the kept
 * monsters counted; the no-ammo notice when the hunter's gun has none.
 */
void ArenaCup_Open(u8 *npc)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_ARENA_CUP;
    memset(cup, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ArenaCup_Draw, ArenaCup_Draw2, 0, 0);
    cup->season = LobbyWorld_GetSeason();
    if (gSave.arenaEms[0].emKind != 0) {
        cup->monsterCount++;
    }
    if (gSave.arenaEms[1].emKind != 0) {
        cup->monsterCount++;
    }
    if (gSave.arenaEms[2].emKind != 0) {
        cup->monsterCount++;
    }
    if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], &gSave) == 1) {
        Menu_PushNoticeOnce(LOBBY_NOTICE_GUN_NO_AMMO);
    }
    gMenuInputLock = 6;
}

/*
 * Closes the arena cup's desk: clears the town menu flag and gTownMode
 * (TOWN_MODE_FREE), resets the menu and its draws, ends the camera focus,
 * plays the cancel sound and gives pad control back to the field. It
 * comes before the level-4 part of the file: it is built at the default
 * level.
 */
void ArenaCup_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_SetAside(0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
    gFieldControl = 1;
}

/* Built at optimization level 4 (Arena_DrawMonsterInfo matches only there). */
#pragma optimization_level 4

extern u16 gQuestStampRand;
#include "lobby_state.h"
#include "lobby_labels.h"
extern LobbyRecs gLobbyRecs;
extern LobbyMenu *gLobbyMenu;

/*
 * The arena cup desk's update, each frame while TOWN_MODE_ARENA_CUP runs,
 * by its step (enum ArenaCupStep). A quest held already is offered to be
 * dropped first (QuestDesk_Cancel); before the cup is open
 * (EVENT_FLAG_ARENA_CUP_OPEN) the clerk only talks. The first list's entries lead to the entry,
 * the monster check and the record, each in a window slid in from the
 * right; each comes back to the first list, and leaving ends with the
 * clerk's goodbye and ArenaCup_Close.
 */
void ArenaCup_Update(void)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (cup->step) {
    case ARENA_CUP_STEP_START:
        if (Lobby_HasQuest() == 1) {
            cup->step = ARENA_CUP_STEP_DROP;
            Talk_Say(ARENA_CUP_TALK_ASK_DROP, 1);
            Talk_Update();
            CamFocus_SetAside(1);
            cup->dropAsked = 1;
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        } else if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 1) {
            cup->step++;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else {
            cup->step = ARENA_CUP_STEP_NOT_OPEN;
            Talk_Say(ARENA_CUP_TALK_NOT_OPEN, 0);
            Talk_Update();
        }
        break;
    case ARENA_CUP_STEP_CHOOSE:
        switch (ArenaCup_ChooseQuest(pad)) {
        case MENU_DONE:
            cup->monster = 0;
            cup->cursor = 0;
            switch (cup->choice) {
            case ARENA_CUP_ENTER:
                cup->step++;
                LobbyText_Open(LOBBY_TEXT_SENDING);
                CamFocus_SetAside(1);
                WinSlide_Step(&cup->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X);
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case ARENA_CUP_MONSTERS:
                cup->step = ARENA_CUP_STEP_MONSTERS_IN;
                WinSlide_Step(&cup->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
                CamFocus_SetAside(1);
                break;
            case ARENA_CUP_RECORD:
                cup->step = ARENA_CUP_STEP_RECORD_IN;
                WinSlide_Step(&cup->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_RECORD_X);
                CamFocus_SetAside(1);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
                break;
            }
            break;
        case MENU_CANCEL:
            cup->step = ARENA_CUP_STEP_BYE;
            Talk_Say(ARENA_CUP_TALK_BYE, 0);
            break;
        case MENU_UNDONE:
            break;
        }
        break;
    case ARENA_CUP_STEP_POST:
        switch (ArenaCup_Post(pad)) {
        case MENU_DONE:
            cup->step = ARENA_CUP_STEP_BYE;
            Talk_Say(ARENA_CUP_TALK_ENTERED, 0);
            gQuestStampRand = Rand_Next(1);
            break;
        case MENU_CANCEL:
        case MENU_UNDONE:
            cup->step = ARENA_CUP_STEP_BYE;
            Talk_Say(ARENA_CUP_TALK_NOT_ENTERED, 0);
            break;
        }
        break;
    case ARENA_CUP_STEP_DROP:
        switch (QuestDesk_Cancel(&cup->cancel, pad)) {
        case MENU_DONE:
            ArenaCup_Close();
            break;
        case MENU_CANCEL:
            if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 1) {
                cup->step = ARENA_CUP_STEP_CHOOSE;
                CamFocus_SetAside(0);
            } else {
                cup->step = ARENA_CUP_STEP_BYE;
                Talk_Say(ARENA_CUP_TALK_BYE, 0);
                CamFocus_SetAside(0);
            }
            break;
        }
        break;
    case ARENA_CUP_STEP_BYE:
        switch (Talk_Update()) {
        case 0:
            ArenaCup_Close();
            break;
        }
        break;
    case ARENA_CUP_STEP_RECORD_IN:
        switch (WinSlide_Step(&cup->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_RECORD_X)) {
        case 0:
            cup->step++;
            break;
        }
        break;
    case ARENA_CUP_STEP_RECORD:
        if (pad & PAD_CROSS) {
            cup->step = ARENA_CUP_STEP_CHOOSE;
            CamFocus_SetAside(0);
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case ARENA_CUP_STEP_MONSTERS_IN:
        switch (WinSlide_Step(&cup->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X)) {
        case 0:
            cup->step++;
            break;
        }
        break;
    case ARENA_CUP_STEP_MONSTERS:
        switch (ArenaCup_MonsterInput(pad)) {
        case MENU_DONE:
            cup->step = ARENA_CUP_STEP_CHOOSE;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case ARENA_CUP_STEP_REFUSED:
        switch (Talk_Update()) {
        case 0:
            cup->step = ARENA_CUP_STEP_CHOOSE;
            break;
        }
        break;
    case ARENA_CUP_STEP_NOT_OPEN:
        switch (Talk_Update()) {
        case 0:
            cup->step = ARENA_CUP_STEP_BYE;
            Talk_Say(ARENA_CUP_TALK_BYE, 0);
            break;
        }
        break;
    case ARENA_CUP_STEP_TERMS_UNMET:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            cup->step = ARENA_CUP_STEP_BYE;
            Talk_Say(ARENA_CUP_TALK_BYE, 0);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
}

extern u32 gQuestPostArg;

/*
 * The entry, by its sub-step (enum ArenaPostStep): the level-2 server's
 * quest list, a free quest room entered and the records fetched; the entry
 * form slides in (ArenaCup_PostInput); then the room's settings, the
 * posting of QUEST_ID_ARENA_CUP at QUEST_POST_DESK_ARENA_CUP and its acceptance are sent, and
 * the quest is taken and paid (QuestDesk_Accept) with the hunter as the
 * arena's entrant. A server error waits for PAD_CIRCLE under the error's
 * dialog; leaving the form leaves the quest room first. Returns MENU_DONE
 * once the quest is taken, MENU_UNDONE after an error, MENU_CANCEL when
 * the form is left, else MENU_RUNNING. gFieldControl is written through an
 * s8 pointer: through its u8 declaration the function compiles to
 * different code.
 */
s32 ArenaCup_Post(u16 pad)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    switch (cup->subStep) {
    case ARENA_POST_FETCH_LIST:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchList()) {
        case 0:
            cup->subStep++;
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            cup->subStep = ARENA_POST_ERROR;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        break;
    case ARENA_POST_ENTER_ROOM:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_EnterFree(1)) {
        case 0:
            cup->subStep++;
            break;
        case 1:
            cup->subStep = ARENA_POST_ERROR;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        break;
    case ARENA_POST_FETCH_RECORDS:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchRecords()) {
        case 0:
            cup->subStep++;
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            break;
        case 1:
            cup->subStep = ARENA_POST_ERROR;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        break;
    case ARENA_POST_SLIDE_IN:
        switch (WinSlide_Step(&cup->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X)) {
        case 0:
            cup->subStep++;
            break;
        }
        break;
    case ARENA_POST_FORM:
        switch (ArenaCup_PostInput(pad)) {
        case MENU_DONE:
            cup->subStep++;
            LobbyText_Open(LOBBY_TEXT_SENDING);
            cup->req.quest = QUEST_ID_ARENA_CUP;
            cup->req.money = 0;
            cup->req.desk = QUEST_POST_DESK_ARENA_CUP;
            cup->req.area = gScene.area;
            break;
        case MENU_CANCEL:
            cup->subStep = ARENA_POST_LEAVE_ROOM;
            LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
            LobbySe_Play(LOBBY_SE_CANCEL);
            break;
        }
        break;
    case ARENA_POST_SEND_SETTINGS:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendSettings()) {
        case 0:
            cup->subStep++;
        case 1:
            break;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case ARENA_POST_SEND_POST:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendQuestPost(&cup->req)) {
        case 0:
            cup->subStep++;
            gQuestPostArg = (gQuestPostArg & 0xFFFF0000) | gLobbyQuestId;
            break;
        case 1:
            cup->subStep = ARENA_POST_ERROR;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case ARENA_POST_SEND_ACCEPT:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendQuestAccept(gQuestPostArg)) {
        case 0:
            cup->subStep = ARENA_POST_FETCH_LIST;
            gLobby->questPaid = 1;
            gPlayerText.card.arenaRole = 1;
            QuestDesk_Accept(QUEST_ID_ARENA_CUP, 0, 1);
            memset(gNextQuest.teams, 0, 92); /* the versus fields, teams to timeLimit */
            *(s8 *)&gFieldControl = 1;
            return MENU_DONE;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            cup->subStep = ARENA_POST_ERROR;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case ARENA_POST_LEAVE_ROOM:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_Level2Leave() == 1) {
            if (gLobby->questPaid == 1) {
                gLobby->questPaid = 0;
            }
            cup->subStep = ARENA_POST_FETCH_LIST;
            return MENU_CANCEL;
        }
        break;
    case ARENA_POST_ERROR:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            cup->subStep = ARENA_POST_FETCH_LIST;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_UNDONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The first list's input (enum ArenaCupChoice at cup->choice). PAD_CIRCLE
 * on the entry refuses it with a talk in the morning (the cup runs in the
 * afternoon) or with no monster kept (ARENA_CUP_STEP_REFUSED), with a
 * notice while a quest is taken, and with a dialog when the quest's order
 * terms aren't met (ARENA_CUP_STEP_TERMS_UNMET); these return MENU_UNDONE,
 * the notice MENU_RUNNING. Otherwise, and on the other entries, it returns
 * MENU_DONE; PAD_CROSS returns MENU_CANCEL, and up and down move the
 * cursor (MENU_RUNNING).
 */
s32 ArenaCup_ChooseQuest(u16 pad)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;
    QuestInfo *quest = Quest_GetInfoAny(QUEST_ID_ARENA_CUP);

    if (pad & PAD_CIRCLE) {
        switch (cup->choice) {
        case ARENA_CUP_ENTER:
            if (LobbyTime_IsPm() == 0) {
                Talk_Say(ARENA_CUP_TALK_MORNING, 0);
                cup->step = ARENA_CUP_STEP_REFUSED;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_UNDONE;
            }
            if (gLobby->questTaken != 0) {
                Menu_PushNoticeOnce(LOBBY_NOTICE_QUEST_TAKEN);
                LobbySe_Play(LOBBY_SE_REFUSE);
                break;
            }
            if (cup->monsterCount == 0) {
                Talk_Say(ARENA_CUP_TALK_NO_MONSTER, 0);
                cup->step = ARENA_CUP_STEP_REFUSED;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_UNDONE;
            }
            if (Quest_CheckOrderTerms(quest->terms, 0) == 0) {
                cup->step = ARENA_CUP_STEP_TERMS_UNMET;
                LobbyText_Open(LOBBY_TEXT_ARENA_TERMS_UNMET);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
                return MENU_UNDONE;
            }
            return MENU_DONE;
        default:
            return MENU_DONE;
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else {
        cup->choice = Menu_WrapUpDown(cup->choice, ARENA_CUP_CHOICE_COUNT);
    }
    return MENU_RUNNING;
}

/*
 * The monster check's input (cup->subStep, enum ArenaMonsterStep): the
 * list of the kept monsters (cup->monster); PAD_CIRCLE on one slides in
 * its menu (enum ArenaMonsterAction at cup->cursor): its details in a
 * window of their own, its rename (ArenaCup_RenameMonster) or its release
 * after a yes/no (ArenaCup_ReleaseMonster; refused with a notice while the
 * arena cup's quest is taken). PAD_CROSS steps back, and from the list
 * returns MENU_DONE; otherwise MENU_RUNNING.
 */
s32 ArenaCup_MonsterInput(u16 pad)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;
    s8 state = cup->subStep;

    switch (state) {
    case ARENA_MON_LIST:
        if (cup->monsterCount == 0) {
            if (pad & PAD_CROSS) {
                LobbySe_Play(LOBBY_SE_CANCEL);
                return MENU_DONE;
            }
            break;
        }
        if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_DONE;
        }
        if (pad & PAD_CIRCLE) {
            if (gSave.arenaEms[cup->monster].emKind != 0) {
                cup->subStep = state + 1;
                cup->cursor = 0;
                WinSlide_Step(&cup->entryWin, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            }
        } else {
            cup->monster = Menu_WrapUpDown(cup->monster, cup->monsterCount);
        }
        break;
    case ARENA_MON_MENU_IN:
        switch (WinSlide_Step(&cup->entryWin, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X)) {
        case 0:
            cup->subStep = ARENA_MON_MENU;
            break;
        }
        break;
    case ARENA_MON_MENU_OUT:
        switch (WinSlide_Step(&cup->entryWin, ARENA_CUP_WIN_X, ARENA_CUP_WIN_X_OUT)) {
        case 0:
            cup->subStep = ARENA_MON_LIST;
            break;
        }
        break;
    case ARENA_MON_MENU:
        if (pad & PAD_CROSS) {
            WinSlide_Step(&cup->entryWin, ARENA_CUP_WIN_X, ARENA_CUP_WIN_X_OUT);
            cup->subStep = ARENA_MON_MENU_OUT;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_CIRCLE) {
            switch (cup->cursor) {
            case ARENA_MON_SHOW_DETAILS:
                cup->subStep = ARENA_MON_DETAILS_IN;
                WinSlide_Step(&cup->monsterWin, ARENA_CUP_WIN_X_OUT, ARENA_CUP_MONSTER_X);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
                break;
            case ARENA_MON_RENAME:
                cup->subStep = ARENA_MON_RENAMING;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case ARENA_MON_RELEASE:
                if (ArenaCup_QuestTaken() == 1) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                    Menu_PushNoticeOnce(LOBBY_NOTICE_ARENA_RELEASE_REFUSED);
                } else {
                    cup->subStep = ARENA_MON_RELEASE_ASK;
                    LobbyText_Open(LOBBY_TEXT_ARENA_RELEASE_ASK);
                    LobbyDialog_Grow();
                    Menu_SetYesNo(1);
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    gFieldControl = 0;
                }
                break;
            }
        } else {
            cup->cursor = Menu_WrapUpDown(cup->cursor, ARENA_MON_ACTION_COUNT);
        }
        break;
    case ARENA_MON_DETAILS_IN:
        switch (WinSlide_Step(&cup->monsterWin, ARENA_CUP_WIN_X_OUT, ARENA_CUP_MONSTER_X)) {
        case 0:
            cup->subStep = ARENA_MON_DETAILS;
            break;
        }
        break;
    case ARENA_MON_DETAILS_OUT:
        switch (WinSlide_Step(&cup->monsterWin, ARENA_CUP_MONSTER_X, ARENA_CUP_WIN_X_OUT)) {
        case 0:
            cup->subStep = ARENA_MON_MENU;
            break;
        }
        break;
    case ARENA_MON_DETAILS:
        if (pad & PAD_CROSS) {
            WinSlide_Step(&cup->monsterWin, ARENA_CUP_MONSTER_X, ARENA_CUP_WIN_X_OUT);
            cup->subStep = ARENA_MON_DETAILS_OUT;
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case ARENA_MON_RENAMING:
        switch (ArenaCup_RenameMonster(pad, (s32)gSave.arenaEms[cup->monster].name)) {
        case MENU_DONE:
            cup->subStep = ARENA_MON_MENU;
            break;
        }
        break;
    case ARENA_MON_RELEASE_ASK:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            ArenaCup_ReleaseMonster();
            cup->subStep = ARENA_MON_LIST;
            if (cup->monster >= cup->monsterCount) {
                if (--cup->monster < 0) {
                    cup->monster = 0;
                }
            }
            gFieldControl = 1;
            break;
        case MENU_CANCEL:
            cup->subStep = ARENA_MON_MENU;
            gFieldControl = 1;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Renames a kept monster with the chat keyboard (name, the monster's name
 * buffer, passed as an address): the keyboard opens on the name
 * (Chat_OpenKbd), runs until it ends (Kbd_RunFiltered) and closes. Returns MENU_DONE once
 * closed, else MENU_RUNNING. The pad words are read through s16 pointers,
 * as the game's code does.
 */
s32 ArenaCup_RenameMonster(u16 pad, s32 name)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    Kbd_UsbNewKey();
    switch (cup->renameStep) {
    case 0:
        (cup->renameStep)++;
        Kbd_SetPos(58, 80.0f);
        Chat_OpenKbd(0, 6, sizeof(gSave.arenaEms[0].name) - 1, (void *)name);
        gFieldControl = 0;
        break;
    case 1:
        if (Kbd_RunFiltered((char *)name, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            (cup->renameStep)++;
        }
        break;
    case 2:
        Kbd_Cancel();
        cup->renameStep = 0;
        gFieldControl = 1;
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * Releases the kept monster at the cursor (cup->monster): its slot of
 * gSave.arenaEms is cleared and the ones after it move up.
 */
void ArenaCup_ReleaseMonster(void)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    memset(&gSave.arenaEms[cup->monster], 0, sizeof(ArenaEm));
    switch (cup->monster) {
    case 0:
        memcpy(&gSave.arenaEms[0], &gSave.arenaEms[1], sizeof(ArenaEm));
    case 1:
        memcpy(&gSave.arenaEms[1], &gSave.arenaEms[2], sizeof(ArenaEm));
        memset(&gSave.arenaEms[2], 0, sizeof(ArenaEm));
        break;
    }
    if (cup->monsterCount > 0) {
        (cup->monsterCount)--;
    }
}

/*
 * 1 when row `row` of the entry form (enum ArenaFormRow) is shown and can be
 * chosen, else 0: the password's switch and the decide row always, the
 * password while it is on (gLobbyRecs.usePass), and the three settings by
 * bits 1 to 3 of the downloaded quest list's arenaFormRows.
 */
s32 ArenaCup_IsFormRowOpen(s8 row)
{
    u16 rowBits = ((RoomData *)QDL_LIST_FILE)->arenaFormRows;

    switch (row) {
    case ARENA_FORM_PASS_ON:
    case ARENA_FORM_DECIDE:
        return 1;
    case ARENA_FORM_PASSWORD:
        if (gLobbyRecs.usePass != 0) {
            return 1;
        }
        break;
    case ARENA_FORM_SETTING_1:
        if (rowBits & 2) {
            return 1;
        }
        break;
    case ARENA_FORM_SETTING_2:
        if (rowBits & 4) {
            return 1;
        }
        break;
    case ARENA_FORM_SETTING_3:
        if (rowBits & 8) {
            return 1;
        }
        break;
    }
    return 0;
}

extern s8 gQuestMinMembers;
extern u8 gLobbyLevel2Sel;

/*
 * The entry form's input (enum ArenaFormRow), by ArenaCupMenu.postStep: 0
 * the form, 1 the password's keyboard. Circle on the decide row returns
 * MENU_DONE, unless the password is on and empty (the empty-password
 * notice): the least members (gQuestMinMembers, records 0's value and the
 * level-2 entry's maxUsers) become 1, or 3 when the second setting is
 * shown and its choice isn't the first. Circle on the password's row opens
 * the keyboard; cross returns MENU_CANCEL. Left or right toggles the
 * password (clearing it when turned off) or wraps a setting's value. Up and
 * down move to the next row shown, trying each row once. Else MENU_RUNNING.
 */
s32 ArenaCup_PostInput(u16 pad)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;
    u8 *setting = NULL;   /* the setting's LobbyRec; the cursor loops count their tries in it too, as the game's code keeps both in one register */

    switch (cup->postStep) {
    case 0:
        if (pad & PAD_CIRCLE) {
            switch (cup->cursor) {
            case ARENA_FORM_DECIDE:
                if (gLobbyRecs.usePass != 0 && gLobbyRecs.pass[0] == 0) {
                    Menu_PushNoticeOnce(LOBBY_NOTICE_PASSWORD_EMPTY);
                    break;
                }
                if (ArenaCup_IsFormRowOpen(ARENA_FORM_SETTING_2) != 0) {
                    if (gLobbyRecs.rec[2].cur == 0) {
                        gLobbyRecs.rec[0].cur = 1;
                    } else {
                        gLobbyRecs.rec[0].cur = 3;
                    }
                } else {
                    gLobbyRecs.rec[0].cur = 1;
                }
                gQuestMinMembers = gLobbyRecs.rec[0].cur;
                gLobbyLevel2Entries[gLobbyLevel2Sel - 1].maxUsers = gQuestMinMembers;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            case ARENA_FORM_PASSWORD:
                cup->postStep++;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            }
        } else if (pad & PAD_CROSS) {
            return MENU_CANCEL;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            switch (cup->cursor) {
            case ARENA_FORM_PASS_ON:
                gLobbyRecs.usePass ^= 1;
                if (gLobbyRecs.usePass == 0) {
                    memset(gLobbyRecs.pass, 0, sizeof(gLobbyRecs.pass));
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
                break;
            case ARENA_FORM_SETTING_1:
                setting = (u8 *)&gLobbyRecs.rec[1];
                break;
            case ARENA_FORM_SETTING_2:
                setting = (u8 *)&gLobbyRecs.rec[2];
                break;
            case ARENA_FORM_SETTING_3:
                setting = (u8 *)&gLobbyRecs.rec[3];
                break;
            }
            if (setting != NULL) {
                /* the setting's LobbyRec, read as bytes: cur (byte 69) wraps over
                 * subCount (byte 66). Through the struct the compiler keeps
                 * cur's address in a register, which the game's code doesn't */
                setting[69] = Menu_WrapLeftRight(setting[69], setting[66]);
            }
        } else if (pad & PAD_DOWN) {
            do {
                if (++cup->cursor >= ARENA_FORM_ROWS) {
                    cup->cursor = 0;
                }
                if (ArenaCup_IsFormRowOpen(cup->cursor) != 0) {
                    break;
                }
                setting++;
            } while ((s32)setting < ARENA_FORM_ROWS);
            LobbySe_Play(LOBBY_SE_SCROLL);
        } else if (pad & PAD_UP) {
            do {
                if (--cup->cursor < 0) {
                    cup->cursor = ARENA_FORM_ROWS - 1;
                }
                if (ArenaCup_IsFormRowOpen(cup->cursor) != 0) {
                    break;
                }
                setting++;
            } while ((s32)setting < ARENA_FORM_ROWS);
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        break;
    case 1:
        if (LobbyMenu_KbdEntry(gLobbyRecs.pass) == 1) {
            cup->postStep = 0;
        }
        break;
    }
    return MENU_RUNNING;
}

extern char gArenaStrFmt[];
extern char gArenaPassArrowsText[];

/*
 * The arena cup's entry form at (x, y), under its label bar: the heading,
 * the password's switch and the password in its box, the settings whose
 * rows are shown (ArenaCup_IsFormRowOpen), each its record's name and the
 * value chosen, and the decide row. The row at the cursor has arrows, and
 * the password's box takes the cursor's colour when it is chosen. The value
 * column's x is an s16 declared before centerX and labelX, and the heading's
 * y is set in one chained assignment (headY = top = y + 24): written
 * otherwise, the compiler gives the values other registers.
 */
void ArenaCup_DrawForm(s16 x, s16 y)
{
    s16 headY;
    s32 left;
    s32 top;
    s16 rowY;
    LobbyRec *setting;
    s16 decideX;
    s32 boxX;
    ArenaCupMenu *cup;
    s16 valueX;
    s32 centerX;
    s32 labelX;

    cup = (ArenaCupMenu *)gMenuWork;
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(260, 18, 230, 0xFFB0B000, (char *)*(s32 *)&gArenaCupTexts[ARENA_TEXT_APPLY], 2, 1);
    Icon_Draw(x, y, 340, 270, 0xFFE0D8C8, 0);
    Font_SetSpacing(18, 18);
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    headY = top = y + 24;
    left = x;
    centerX = left + 170;
    Font_SetPos(Text_CenterX(gArenaCupTexts[ARENA_TEXT_FORM_TITLE], centerX, 18), headY);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_FORM_TITLE]);
    rowY = top;
    valueX = left + 174;
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    labelX = left + 36;
    Font_SetPos(labelX, rowY += 30);
    Text_PrintfColored("%-10s%s", gArenaCupTexts[ARENA_TEXT_PASSWORD], gLobbyWords[LOBBY_WORD_COLON]);
    Font_SetColor(QUEST_SHEET_INK);
    Font_SetPos(valueX, rowY);
    Text_PrintfColored(gNoneSetTexts[gLobbyRecs.usePass]);
    if (cup->cursor == 0) {
        Menu_DrawArrows(valueX, rowY, 18, gNoneSetTexts[gLobbyRecs.usePass], QUEST_FORM_CURSOR_COLOR);
    }
    Font_SetPos(Text_CenterX(gLobbyRecs.pass, centerX, 18), rowY += 30);
    Text_PrintfColored(gArenaStrFmt, gLobbyRecs.pass);
    if (cup->cursor == 1) {
        boxX = left + 100;
        Menu_DrawArrowsRev(boxX, rowY, 18, gArenaPassArrowsText, QUEST_FORM_CURSOR_COLOR);
        Gfx_DrawRectFrame(boxX, (s16)rowY - 1, 140, 20, QUEST_FORM_CURSOR_COLOR);
        Gfx_DrawRectFrame(left + 99, (s16)rowY - 2, 142, 22, QUEST_FORM_CURSOR_COLOR);
    } else {
        Gfx_DrawRectFrame(left + 100, (s16)rowY - 1, 140, 20, QUEST_FORM_BOX_COLOR);
        Gfx_DrawRectFrame(left + 99, (s16)rowY - 2, 142, 22, QUEST_FORM_BOX_COLOR);
    }
    if (ArenaCup_IsFormRowOpen(2) != 0) {
        setting = &gLobbyRecs.rec[1];
        Font_SetColor(QUEST_SHEET_LABEL_INK);
        Font_SetPos(labelX, rowY += 40);
        Text_PrintfColored("%-10s%s", setting->name, gLobbyWords[LOBBY_WORD_COLON]);
        Font_SetColor(QUEST_SHEET_INK);
        Font_SetPos(valueX, rowY);
        Text_PrintfColored(setting->subName[setting->cur]);
        if (cup->cursor == 2) {
            Menu_DrawArrows(valueX, rowY, 18, setting->subName[setting->cur], QUEST_FORM_CURSOR_COLOR);
        }
    }
    if (ArenaCup_IsFormRowOpen(3) != 0) {
        setting = &gLobbyRecs.rec[2];
        Font_SetColor(QUEST_SHEET_LABEL_INK);
        Font_SetPos(labelX, rowY += 30);
        Text_PrintfColored("%-10s%s", setting->name, gLobbyWords[LOBBY_WORD_COLON]);
        Font_SetColor(QUEST_SHEET_INK);
        Font_SetPos(valueX, rowY);
        Text_PrintfColored(setting->subName[setting->cur]);
        if (cup->cursor == 3) {
            Menu_DrawArrows(valueX, rowY, 18, setting->subName[setting->cur], QUEST_FORM_CURSOR_COLOR);
        }
    }
    if (ArenaCup_IsFormRowOpen(4) != 0) {
        setting = &gLobbyRecs.rec[3];
        Font_SetColor(QUEST_SHEET_LABEL_INK);
        Font_SetPos(labelX, rowY += 30);
        Text_PrintfColored("%-10s%s", setting->name, gLobbyWords[LOBBY_WORD_COLON]);
        Font_SetColor(QUEST_SHEET_INK);
        Font_SetPos(valueX, rowY);
        Text_PrintfColored(setting->subName[setting->cur]);
        if (cup->cursor == 4) {
            Menu_DrawArrows(valueX, rowY, 18, setting->subName[setting->cur], QUEST_FORM_CURSOR_COLOR);
        }
    }
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    decideX = Text_CenterX(gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], centerX, 18);
    Font_SetPos(decideX, rowY += 50);
    Text_PrintfColored(gArenaStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE]);
    if (cup->cursor == 5) {
        Menu_DrawArrowsRev(decideX, rowY, 18, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], QUEST_FORM_CURSOR_COLOR);
    }
}

#pragma rodata on
char gArenaStrFmt[3] = {
#include "lobby/00838C60.inc"
};
char gArenaPassArrowsText[16] = {
#include "lobby/00838C70.inc"
};
char gArenaIntFmt[3] = {
#include "lobby/00838C80.inc"
};
#pragma rodata off

extern char gMyName[];
s32 sprintf(char *buf, char *fmt, ...);

/*
 * The cup's total record at (x, y), under its label bar: a heading with the
 * hunter's name, then the versus fights, wins, losses and time
 * (gSave.versusWins, versusLosses, versusMinutes) beside their labels. The
 * parameter y, no longer read once rowY holds it, is reused for the labels'
 * x, as the game's code keeps both in one register. The label bar's text is
 * read as a word: read as a char *, the compiler schedules the constant 2
 * before the colour.
 */
void ArenaCup_DrawRecord(s16 x, s16 y)
{
    s16 headY;
    char buf[48];
    s32 left;
    s16 rowY = y;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(320, 18, 230, 0xFFB0B000, (char *)*(s32 *)&gArenaCupTexts[ARENA_TEXT_TOTAL_RECORD], 2, 1);
    Icon_Draw(x, y, 300, 180, 0xFFE0D8C8, 0);
    Font_SetSpacing(20, 20);
    Font_SetColor(QUEST_SHEET_INK);
    left = x;
    Font_SetPos(left + 150, rowY += 64);
    Text_PrintfColored(gArenaIntFmt, gSave.versusWins + gSave.versusLosses);
    Font_SetPos(left + 150, rowY += 22);
    Text_PrintfColored(gArenaIntFmt, gSave.versusWins);
    Font_SetPos(left + 150, rowY += 22);
    Text_PrintfColored(gArenaIntFmt, gSave.versusLosses);
    Font_SetPos(left + 150, rowY + 22);
    Text_PrintfColored(gArenaStrFmt, Time_FormatMinSec(gSave.versusMinutes));
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    rowY = y;
    headY = rowY + 24;
    y = left + 40;
    sprintf(buf, gArenaCupTexts[ARENA_TEXT_RECORD_FMT], gMyName);
    Font_SetPos(Text_CenterX(buf, left + 150, 20), headY);
    Text_PrintfColored(buf);
    Font_SetPos(y, rowY += 64);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_BATTLES]);
    Font_SetPos(y, rowY += 22);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_WINS]);
    Font_SetPos(y, rowY += 22);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_LOSSES]);
    Font_SetPos(y, rowY + 22);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_BATTLE_TIME]);
}

/*
 * The kept monsters' list at (x, y) (the arena cup's monster check and the
 * arena menu), by `mode` (enum ArenaListMode). With none kept the panel
 * says so; else, under a heading with the hunter's name, each monster kept
 * (gSave.arenaEms) with its condition, its name (or the unnamed label) and
 * its kind, and with ARENA_LIST_CURSOR the cursor's bar on row `cursor`.
 */
void Arena_DrawMonsterList(s16 x, s16 y, s8 cursor, s8 mode)
{
    ArenaEm *em;
    s16 left;
    s16 rowY;
    s16 i;

    Icon_Draw(x, y, 360, 160, 0xFFE0D8C8, 0);
    Font_SetSpacing(18, 18);
    if (mode == ARENA_LIST_EMPTY) {
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(gArenaCupTexts[ARENA_TEXT_LIST_HELP], (s16)x + 180, 18), (s16)y + 62);
        Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_LIST_HELP]);
        return;
    }
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    rowY = (s16)y + 20;
    left = x + 0;
    Font_SetPos(Text_CenterX(gArenaCupTexts[ARENA_TEXT_MONSTER_LIST], left + 180, 18), rowY);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_MONSTER_LIST], gMyName);
    rowY += 30;
    Font_SetColor(0x80101050);
    Font_SetPos(left + 30, rowY);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_CONDITION]);
    Font_SetPos(left + 90, rowY);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_NAME]);
    Font_SetPos(left + 170, rowY);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_KIND]);
    Font_SetColor(QUEST_SHEET_INK);
    for (i = 0; i < 3; i++) {
        rowY += 22;
        em = &gSave.arenaEms[i];
        if (em->emKind != 0) {
            Font_SetPos(left + 22, rowY);
            Text_PrintfColored(gArenaEmCondNames[(s8)em->condition]);
            Font_SetPos(left + 90, rowY);
            if ((s8)em->name[0] == 0) {
                Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_UNSET]);
            } else {
                Text_PrintfColored(gArenaStrFmt, em->name);
            }
            Font_SetPos(left + 170, rowY);
            Text_PrintfColored(gArenaStrFmt, gMonsterNames[em->emKind]);
        }
        if (cursor == i && mode != ARENA_LIST_NO_CURSOR) {
            Menu_DrawCursorRow(left + 10, rowY - 2, 300, 10, 0xC0106060, QUEST_FORM_CURSOR_COLOR);
        }
    }
}

/*
 * The monster check at (x, y): the label bar and the kept monsters' list,
 * its cursor shown only in the list itself (ARENA_MON_LIST), and below the
 * help for the list, or for the monster's menu while it is open or
 * sliding in.
 */
void ArenaCup_DrawMonsterCheck(s16 x, s16 y)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(260, 18, 230, 0xFFB0B000, gArenaCupTexts[ARENA_TEXT_MONSTER_CHECK], 2, 1);
    if (cup->monsterCount == 0) {
        Arena_DrawMonsterList(x, y, (s8)cup->monster, ARENA_LIST_EMPTY);
        return;
    }
    if (cup->subStep != ARENA_MON_LIST) {
        Arena_DrawMonsterList(x, y, (s8)cup->monster, ARENA_LIST_NO_CURSOR);
    } else {
        Arena_DrawMonsterList(x, y, (s8)cup->monster, ARENA_LIST_CURSOR);
    }
    switch (cup->subStep) {
    case ARENA_MON_LIST:
    case ARENA_MON_MENU_OUT:
        Win_DrawText(260, 338, 360, 80, 0xBDB59F6C, 0x80202020, gArenaCupTexts[ARENA_TEXT_CHECK_HELP]);
        break;
    case ARENA_MON_MENU_IN:
    case ARENA_MON_MENU:
        Win_DrawText(260, 338, 360, 80, 0xBDB59F6C, 0x80202020, gArenaCupTexts[ARENA_TEXT_MENU_HELP]);
        break;
    }
}


/*
 * A kept monster's menu at (x, y) (enum ArenaMonsterAction), with the
 * cursor's bar on its row. Its three labels are gArenaCupTexts[ARENA_TEXT_CONDITION]' entries
 * 7 to 9, which the game's code reads as gArenaCupTexts' entries 23 to 25
 * (the same words).
 */
void ArenaCup_DrawMonsterMenu(s16 x, s16 y)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;
    s16 rowY = y;
    s16 centerX;
    s16 i;
    s32 labelX;

    Icon_Draw(x, y, 360, 100, 0xFFE0D8C8, 2);
    Font_SetSpacing(20, 20);
    Font_SetColor(QUEST_SHEET_INK);
    rowY += 20;
    centerX = x + 180;
    for (i = 0; i < ARENA_MON_ACTION_COUNT; i++) {
        labelX = Text_CenterX(gArenaCupTexts[ARENA_TEXT_MONSTER_MENU + i], centerX, 20);
        Font_SetPos(labelX, rowY);
        Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_MONSTER_MENU + i]);
        if (cup->cursor == i) {
            Menu_DrawCursorRow(x + 100, rowY - 2, 170, 10, 0xC0106060, QUEST_FORM_CURSOR_COLOR);
        }
        rowY += 22;
    }
}


/*
 * A kept monster's details at (x, y) (the arena cup's monster check and
 * the arena menu): its name (or the unnamed label), kind, size in
 * centimetres, tameness, condition, rank, time, wins and losses, beside the
 * nine labels of gArenaCupTexts[ARENA_TEXT_INFO_LABELS] under the heading. The labels are read
 * as gArenaCupTexts' entries 28 to 36, which are the same words: the
 * game's code indexes them from that table.
 */
void Arena_DrawMonsterInfo(s16 x, s16 y, ArenaEm *em)
{
    s16 rowY = y;
    s16 labelX;
    s16 i;

    Font_SetSpacing(22, 22);
    Icon_Draw(x, y, 320, 270, 0xFFE0D8C8, 1);
    Font_SetSpacing(18, 18);
    Font_SetColor(QUEST_SHEET_INK);
    Font_SetPos(x + 130, rowY += 60);
    if ((s8)em->name[0] == 0) {
        Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_UNSET]);
    } else {
        Text_PrintfColored(gArenaStrFmt, em->name);
    }
    Font_SetPos(x + 130, rowY += 20);
    Text_PrintfColored(gArenaStrFmt, gMonsterNames[em->emKind]);
    Font_SetPos(x + 130, rowY += 20);
    Text_PrintfColored("%6.1fcm", Monster_SizeFromPercent(em->emKind, em->sizePct));
    Font_SetPos(x + 130, rowY += 20);
    Text_PrintfColored(gArenaEmTameNames[em->gaugeLevel]);
    Font_SetPos(x + 130, rowY += 20);
    Text_PrintfColored(gArenaStrFmt, gArenaEmCondNames[(s8)em->condition]);
    Font_SetPos(x + 130, rowY += 20);
    Text_PrintfColored(gArenaStrFmt, gArenaEmRankNames[SaveRecord_KindIndex_Unk((u8 *)em)]);
    Font_SetPos(x + 130, rowY += 20);
    Text_PrintfColored(gArenaStrFmt, Time_FormatMinSec(em->minutes));
    Font_SetPos(x + 130, rowY += 20);
    Text_PrintfColored(gArenaIntFmt, em->wins);
    Font_SetPos(x + 130, rowY + 20);
    Text_PrintfColored(gArenaIntFmt, em->losses);
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    labelX = x + 36;
    Font_SetPos(Text_CenterX(gArenaCupTexts[ARENA_TEXT_MONSTER_INFO], x + 160, 20), y + 24);
    Text_PrintfColored(gArenaCupTexts[ARENA_TEXT_MONSTER_INFO]);
    rowY = y + 60;
    for (i = ARENA_TEXT_INFO_LABELS; i <= ARENA_TEXT_INFO_LABELS + 8; i++) {
        Font_SetPos(labelX, rowY);
        Text_PrintfColored("%-8s%s", gArenaCupTexts[i], gLobbyWords[LOBBY_WORD_COLON]);
        rowY += 20;
    }
}

/* The chosen kept monster's details (ArenaCupMenu.monster). */
void ArenaCup_DrawMonster(s16 x, s16 y)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    Arena_DrawMonsterInfo(x, y, &gSave.arenaEms[cup->monster]);
}

u16 Map_Group(u16 id);
extern s8 gMapGroupMapIds[];

/*
 * The quest held (gLobbyQuestId) on its sheet at (x, y), while the drop
 * talk asks (ARENA_CUP_STEP_DROP), unless the talk hides the sheet: a
 * versus quest (flag 0x10) shows its teams, any other its overview on its
 * map's sheet; then the clear mark if the quest was cleared, and the stamp.
 */
void ArenaCup_DrawHeldQuest(ArenaCupMenu *cup, s16 x, s16 y)
{
    QuestInfo *quest = Quest_GetInfoAny(gLobbyQuestId);
    s16 offs[3];
    s8 mapId;

    if (cup->cancel.hideSheet != 1 && quest != NULL) {
    Font_SetColor(0x80101010);
    Font_SetSpacing(18, 18);
    x += 40;
    y += 5;
    if (quest->flags & 0x10) {
        Menu_DrawQuestSheet(x - 20, y, 320, 360, NULL, 0, -1, -1);
        Quest_DrawArenaTeams(x - 20, y + 6, gLobby->players, gLobbyLevel2Sel - 1);
    } else {
        memset(offs, 0, sizeof(offs));
        mapId = gMapGroupMapIds[Map_Group(quest->map) * 2];
        Menu_DrawQuestSheet(x - 40, y - 10, 320, 360, offs, 0, mapId, -1);
        Quest_DrawOverview(x, y, quest);
    }
    if (Save_IsQuestCleared(quest->id) != 0) {
        Quest_DrawClearMark(x, y);
    }
    Menu_DrawStamp(x, y, 320, 360, gQuestStampRand, 0);
    }
}

void Menu_DrawLabelList(s16 x, s8 **text, s16 n, s16 cur, u32 color);

/*
 * The arena cup's desk's draw (LobbyMenu_SetDraw), by ArenaCupMenu.step:
 * the first list, the entry form while it is shown, the record, the monster
 * check with the monster's menu, and the held quest during the drop talk.
 */
void ArenaCup_Draw(MenuTask *task)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (cup->step) {
    case ARENA_CUP_STEP_START:
        break;
    case ARENA_CUP_STEP_CHOOSE:
        Menu_DrawLabelList(400, (s8 **)&gArenaCupTexts[ARENA_TEXT_APPLY], ARENA_CUP_CHOICE_COUNT, cup->choice, 0xFFB0B000);
        break;
    case ARENA_CUP_STEP_POST:
        switch (cup->subStep) {
        case ARENA_POST_SLIDE_IN:
        case ARENA_POST_FORM:
            ArenaCup_DrawForm(cup->win.pos, 58);
            break;
        }
        break;
    case ARENA_CUP_STEP_RECORD_IN:
    case ARENA_CUP_STEP_RECORD:
        ArenaCup_DrawRecord(cup->win.pos, 58);
        break;
    case ARENA_CUP_STEP_MONSTERS_IN:
    case ARENA_CUP_STEP_MONSTERS:
        ArenaCup_DrawMonsterCheck(cup->win.pos, 58);
        switch (cup->subStep) {
        case ARENA_MON_LIST:
            break;
        case ARENA_MON_MENU_IN:
        case ARENA_MON_MENU_OUT:
        case ARENA_MON_MENU:
        case ARENA_MON_DETAILS_IN:
        case ARENA_MON_DETAILS_OUT:
        case ARENA_MON_DETAILS:
        case ARENA_MON_RENAMING:
        case ARENA_MON_RELEASE_ASK:
            ArenaCup_DrawMonsterMenu(cup->entryWin.pos, 224);
            break;
        }
        break;
    case ARENA_CUP_STEP_DROP:
        ArenaCup_DrawHeldQuest(cup, 260, 48);
        break;
    }
}

/* The desk's second draw: a kept monster's details, over the monster check. */
void ArenaCup_Draw2(MenuTask *task)
{
    ArenaCupMenu *cup = (ArenaCupMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (cup->step) {
    case ARENA_CUP_STEP_MONSTERS:
        switch (cup->subStep) {
        case ARENA_MON_DETAILS_IN:
        case ARENA_MON_DETAILS_OUT:
        case ARENA_MON_DETAILS:
            ArenaCup_DrawMonster(cup->monsterWin.pos, 78);
            break;
        }
        break;
    }
}
