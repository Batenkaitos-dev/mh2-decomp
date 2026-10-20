/*
 * The town's quest counter (TOWN_MODE_QUEST_COUNTER), where a player joins a quest
 * another player posted in one of level 2's five rooms. QuestCounter_Open clears the
 * menu work area (QuestCounterMenu in gMenuWork) and starts the counter's talk;
 * QuestCounter_Step runs each frame: it fetches the rooms (gLobbyLevel2Entries, their
 * quests in gPostedQuests), slides in the board, lets the player pick a room, fetches
 * the room's settings (gLobbyRecs) and members, shows the quest window's pages, asks
 * whether to join, checks the quest (hours, monster, download, order terms, fee), takes
 * the room's password or the arena cup's role, enters the room and accepts the quest
 * (QuestDesk_Accept).
 * The rest draws what the counter's draw layers (QuestCounter_Draw) show: the board's
 * rows, and the quest window's pages: overview, terms, details and members for a
 * quest, the two teams and the room's settings for the arena cup.
 */
#include "common.h"
#include "lobby_text.h"
#include "scene.h"
#include "save.h"
#include "item.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "pad.h"
#include "quest.h"
#include "player.h"
#include "area.h"
#include "lobby_labels.h"

void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gTownActionDelay;
extern u8 gMenuWork;
void Text_SetSize(s32 size);
void QuestCounter_DrawTeamChoice(s32 x, s32 y0);
QuestInfo *Quest_GetInfoAny(u16);
void Quest_DrawArenaTeams(s16 x, s16 y, NetUser *list, s8 room);
void QuestCounter_DrawRoomSettings(s16 x, s32 y0, s32 k);
/* unit's own declaration: a u32 colour; with the definition's s32, QuestCounter_DrawBoardRow compiles differently */
void Font_SetColor(u32);
void *memset(void *p, s32 c, u32 n);
void Mem_Fill(void *dst, u8 c, s32 n);
u8 *Talk_FindScript(u16 id);
void LobbyMenu_Init(void);
void QuestCounter_Draw(MenuTask *task);
void QuestCounter_Draw2(MenuTask *task);
void LobbySe_Play(s32 se);
extern u8 gTalkNpcName;
extern u8 *gTalkScriptRec;
extern s16 gTalkScript;
extern s32 gTalkLine;
extern s8 gLobbyLevel2Index;
s32 Save_HasRate(void);
s32 Game_IsOnline(void);
void QuestCounter_DrawBoard(WinSlide *, s16);
void QuestCounter_DrawQuestWindow(s16 x, s16 y);

/* Opens the quest counter (TOWN_MODE_QUEST_COUNTER): clears the menu work area and
 * the talk's state, starts the counter's talk script, sets the counter's draw
 * layers, clears LobbyState.unk1542 while no quest is taken, and plays the confirm
 * sound. QuestCounter_Step then runs each frame. */
void QuestCounter_Open(void)
{
    memset(&gMenuWork, 0, MENU_WORK_SIZE);
    gLobbyLevel2Index = 0;
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_QUEST_COUNTER;
    gTalkLine = 0;
    gFieldControl = 0;
    Mem_Fill(&gTalkNpcName, 0, 0x34); /* the talk's state: gTalkNpcName up to gTalkMode */
    gTalkScriptRec = Talk_FindScript(0x63);
    gTalkScript = 0x63;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(QuestCounter_Draw, QuestCounter_Draw2, NULL, NULL);
    if (gLobby->questTaken == 0) {
        memset(gLobby->unk1542, 0, sizeof(gLobby->unk1542));
    }
    LobbySe_Play(LOBBY_SE_CONFIRM);
}

/* Closes the counter: the town's free mode, the hunter's control back, a short
 * delay before the player can act again (gTownActionDelay), no draw layers. */
void QuestCounter_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    gFieldControl = 1;
    gTownActionDelay = 20; /* frames before the player can act again */
    LobbyMenu_SetDraw(NULL, NULL, NULL, NULL);
}

void Talk_Say(s32 arg0, s8 arg1);
void LobbyText_Open(s32);
s32 Menu_PushNoticeOnce(s32 arg0);
s32 QuestCounter_OpenWindows(s32 x, s32 y);
/* unit's own declaration: s32 ends; with the definition's s16 from and to, QuestCounter_OpenWindows and QuestCounter_Step compile differently */
s32 WinSlide_Step(WinSlide *, s32, s32);
s32 QuestCounter_CanJoin(s16 idx);
s32 Talk_Update(void);
void QuestCounter_Close(void);
u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
s32 LobbyLevel2_FetchList(void);
struct Player;
s32 Equip_GunHasNoAmmo(struct Player *pl, struct SaveData *save);
s32 QuestCounter_ChooseQuest(u16 arg0);
s32 LobbyStep_Level2Entry(u16 id, NetUser *users);
s32 QuestCounter_ViewQuest(u16 arg0);
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *fmt, s32 n);
s32 QuestCounter_CheckQuest(void);
s32 Talk_YesNoBox(s8 flags);
s32 QuestCounter_InputPassword(void *task);
s32 Quest_IsDownloadId(s32);
s8 Quest_DownloadedSlot(u16 id);
/* unit's own declaration: an s8 slot and a u16 id; with the definition's s32 slot and id, QuestCounter_Step compiles differently */
s32 LobbyStep_Download(s8 slot, u16 id);
QuestInfo *Quest_CurrentInfo(void);
s32 QuestCounter_ChooseArenaRole(u16 arg0);
s32 LobbyLevel2_EnterWithPass(s32 v);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
s32 LobbyStep_Level2Leave(void);
extern LobbyMenu *gLobbyMenu;
extern LobbyRecs gLobbyRecs;
extern u16 gLobbyQuestId;
extern s8 gQuestMinMembers;
extern char *gTalkLineText;


/* The quest counter's update, once a frame on QuestCounterMenu.step (enum
 * QuestCounterStep): fetch the five rooms, slide in the board, choose a room
 * (QuestCounter_ChooseQuest), fetch its settings and members (LobbyStep_Level2Entry),
 * page through the quest (QuestCounter_ViewQuest), answer the talk's question on
 * joining, check the quest (QuestCounter_CheckQuest), then the password
 * (QuestCounter_InputPassword) for a locked room, the downloaded quest's file
 * (LobbyStep_Download) or the arena cup's role (QuestCounter_ChooseArenaRole), and enter
 * the room (LobbyLevel2_EnterWithPass) and accept the quest (QuestDesk_Accept).
 * If the room's quest changed meanwhile, it leaves the room again. Each failure
 * shows a message (QUEST_COUNTER_MESSAGE) that waits for PAD_CIRCLE and goes on
 * at returnStep. */
void QuestCounter_Step(void)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    PostedQuest *post = &gPostedQuests[w->quest];
    LevelEntry *room = &gLobbyLevel2Entries[w->quest];
    u16 pad = LobbyPad_Get(0);
    u16 slot;
    s8 minMembers;
    QuestInfo *info;

    Chat_SetWindowMode(1, 2);
    switch (w->step) {
    case QUEST_COUNTER_START:
        (w->step)++;
        LobbyText_Open(0x78);
        gFieldControl = 0;
        break;
    case QUEST_COUNTER_FETCH_LIST:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchList()) {
        case LOBBY_STEP_DONE:
            (w->step)++;
            Talk_Say(0, 0);
            if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave) == 1) {
                Menu_PushNoticeOnce(0x13);
            }
            gFieldControl = 1;
            QuestCounter_OpenWindows(0x280, 0xF0);
            LobbySe_Play(LOBBY_SE_LIST_OPEN);
            break;
        case LOBBY_STEP_FAILED:
            w->step = QUEST_COUNTER_MESSAGE;
            w->returnStep = QUEST_COUNTER_CLOSE;
            gFieldControl = 0;
            break;
        }
        break;
    case QUEST_COUNTER_OPEN_BOARD:
        switch (QuestCounter_OpenWindows(0x280, 0xF0)) {
        case MENU_DONE:
            (w->step)++;
            break;
        }
        break;
    case QUEST_COUNTER_CHOOSE_QUEST:
        switch (QuestCounter_ChooseQuest(pad)) {
        case MENU_DONE:
            (w->step)++;
            w->page = 0;
            gFieldControl = 0;
            LobbyText_Open(0x78);
            break;
        case MENU_CANCEL:
            w->step = QUEST_COUNTER_CLOSE;
            break;
        case MENU_UNDONE:
            w->step = QUEST_COUNTER_MESSAGE;
            w->returnStep = QUEST_COUNTER_CLOSE;
            gFieldControl = 0;
            LobbyText_Open(0x62);
            break;
        }
        break;
    case QUEST_COUNTER_FETCH_ROOM:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level2Entry(w->quest, w->members)) {
        case LOBBY_STEP_DONE:
            (w->step)++;
            w->questId = post->quest;
            gFieldControl = 1;
            WinSlide_Step(&w->questWin, 0x280, 0x118);
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            break;
        }
        break;
    case QUEST_COUNTER_OPEN_QUEST:
        switch (WinSlide_Step(&w->questWin, 0x280, 0x118)) {
        case MENU_DONE:
            (w->step)++;
            break;
        }
        break;
    case QUEST_COUNTER_VIEW_QUEST:
        switch (QuestCounter_ViewQuest(pad)) {
        case MENU_DONE:
            (w->step)++;
            if (post->money > 0) {
                Talk_Say(9, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), post->money);
            } else {
                Talk_Say(1, 0);
            }
            gFieldControl = 0;
            break;
        case MENU_CANCEL:
            w->step = QUEST_COUNTER_CHOOSE_QUEST;
            break;
        }
        break;
    case QUEST_COUNTER_ASK_JOIN:
        if (QuestCounter_CanJoin(w->quest) == 0) {
            switch (gLobbyLevel2Entries[w->quest].state) {
            case 4:
                Menu_PushNoticeOnce(0xA);
                break;
            default:
                Menu_PushNoticeOnce(9);
                break;
            }
            w->step = QUEST_COUNTER_CHOOSE_QUEST;
            Talk_Say(0, 0);
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CANCEL);
            break;
        }
        switch (Talk_Update()) {
        case MENU_DONE:
            if (gLobby->questTaken != 0) {
                Talk_Say(7, 0);
                w->step = QUEST_COUNTER_TALK_END;
                gFieldControl = 1;
                break;
            }
            (w->step)++;
            gFieldControl = 0;
            break;
        case MENU_CANCEL:
            w->step = QUEST_COUNTER_CHOOSE_QUEST;
            gFieldControl = 1;
            break;
        }
        break;
    case QUEST_COUNTER_CHECK:
        if (QuestCounter_CheckQuest() == 0) {
            if (gLobbyLevel2Entries[w->quest].locked != 0) {
                (w->step)++;
                Talk_Say(2, 0);
                memset(gLobbyRecs.pass, 0, 0x10); /* the password and the 7 bytes after it */
                gFieldControl = 0;
                break;
            }
            w->step = QUEST_COUNTER_PREPARE;
            Talk_Say(3, 0);
            LobbyText_Open(0x78);
            gLobbyQuestId = w->questId;
            minMembers = room->maxUsers - 1;
            gFieldControl = 0;
            gQuestMinMembers = minMembers;
            break;
        }
        gLobbyMenu->redraw = 1;
        w->step = QUEST_COUNTER_MESSAGE;
        w->returnStep = QUEST_COUNTER_CHOOSE_QUEST;
        gFieldControl = 0;
        break;
    case QUEST_COUNTER_PASSWORD:
        Talk_YesNoBox(1);
        switch (QuestCounter_InputPassword(gLobbyRecs.pass)) {
        case MENU_DONE:
            (w->step)++;
            Talk_Say(3, 0);
            LobbyText_Open(0x78);
            gLobbyQuestId = w->questId;
            minMembers = room->maxUsers - 1;
            gFieldControl = 0;
            gQuestMinMembers = minMembers;
            break;
        case MENU_CANCEL:
            w->step = QUEST_COUNTER_CHOOSE_QUEST;
            Talk_Say(0, 0);
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CANCEL);
            break;
        }
        break;
    case QUEST_COUNTER_PREPARE:
        if (Quest_IsDownloadId((u16)w->questId) == 0) {
            if (post->desk != 3) {
                w->joinRole = 0;
                w->step = QUEST_COUNTER_ENTER;
                break;
            }
            if (room->maxUsers == 2) {
                if (gSave.arenaEms[0].emKind == 0) {
                    LobbyText_Open(0x67);
                    w->step = QUEST_COUNTER_MESSAGE;
                    w->returnStep = QUEST_COUNTER_CLOSE;
                    gFieldControl = 0;
                    break;
                }
                w->arenaRole = 3;
                w->joinRole = 3;
                w->step = QUEST_COUNTER_ENTER;
                break;
            }
            (w->step)++;
            break;
        }
        gLobbyMenu->redraw = 1;
        slot = Quest_DownloadedSlot((u16)w->questId);
        switch (LobbyStep_Download(slot, (u16)w->questId)) {
        case LOBBY_STEP_DONE:
            info = Quest_CurrentInfo();
            if (info->id != w->questId) {
                LobbyText_Open(0x62);
                w->step = QUEST_COUNTER_MESSAGE;
                w->returnStep = QUEST_COUNTER_CHOOSE_QUEST;
                gFieldControl = 0;
                break;
            }
            w->joinRole = 0;
            w->step = QUEST_COUNTER_ENTER;
            gFieldControl = 0;
            break;
        case LOBBY_STEP_FAILED:
            w->step = QUEST_COUNTER_MESSAGE;
            w->returnStep = QUEST_COUNTER_CHOOSE_QUEST;
            gFieldControl = 0;
            break;
        }
        break;
    case QUEST_COUNTER_CHOOSE_ROLE:
        if (QuestCounter_CanJoin(w->quest) == 0) {
            switch (gLobbyLevel2Entries[w->quest].state) {
            case 4:
                Menu_PushNoticeOnce(0xA);
                break;
            default:
                Menu_PushNoticeOnce(9);
                break;
            }
            w->step = QUEST_COUNTER_CHOOSE_QUEST;
            w->subStep = 0;
            Talk_Say(0, 0);
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CANCEL);
            break;
        }
        switch (QuestCounter_ChooseArenaRole(pad)) {
        case MENU_CANCEL:
            w->step = QUEST_COUNTER_VIEW_QUEST;
            break;
        case MENU_DONE:
            (w->step)++;
            gFieldControl = 0;
            break;
        case MENU_UNDONE:
            w->step = QUEST_COUNTER_MESSAGE;
            w->returnStep = QUEST_COUNTER_CHOOSE_ROLE;
            gFieldControl = 0;
            break;
        }
        break;
    case QUEST_COUNTER_ENTER:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_EnterWithPass(w->joinRole)) {
        case LOBBY_STEP_DONE:
            if (w->questId != post->quest) {
                w->step = QUEST_COUNTER_LEAVE_ROOM;
                gFieldControl = 0;
                break;
            }
            (w->step)++;
            gFieldControl = 0;
            LobbyText_Open(0x61);
            gPlayerText.card.arenaRole = w->arenaRole;
            QuestDesk_Accept((u16)w->questId, (s16)post->money, 0);
            break;
        case LOBBY_STEP_FAILED:
            w->step = QUEST_COUNTER_MESSAGE;
            w->returnStep = QUEST_COUNTER_CHOOSE_QUEST;
            gFieldControl = 0;
            break;
        }
        break;
    case QUEST_COUNTER_LEAVE_ROOM:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_Level2Leave() == 1) /* this step returns 1 once left */ {
            w->step = QUEST_COUNTER_MESSAGE;
            w->returnStep = QUEST_COUNTER_CLOSE;
            LobbyText_Open(0x96);
        }
        break;
    case QUEST_COUNTER_ACCEPTED:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            (w->step)++;
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        }
        break;
    case QUEST_COUNTER_DONE:
        QuestCounter_Close();
        break;
    case QUEST_COUNTER_TALK_END:
        if (Talk_Update() != MENU_DONE) {
            break;
        }
        /* falls through once the talk is over */
    case QUEST_COUNTER_CLOSE:
        QuestCounter_Close();
        break;
    case QUEST_COUNTER_MESSAGE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            w->step = w->returnStep;
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
}

s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
void LobbyText_Open(s32);

/* The arena cup's role choice, for a room of more than two: its window (kindWin)
 * slides in, then up and down move the cursor over roles 2 to 4. Circle takes the
 * role (arenaRole and joinRole) and returns MENU_DONE, but role 3, the second team's
 * leader, needs a monster kept for the arena (gSave.arenaEms[0]): without one a
 * message and MENU_UNDONE. Cross gives MENU_CANCEL; MENU_RUNNING meanwhile. */
s32 QuestCounter_ChooseArenaRole(u16 pad)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    s32 btn;
    s32 role;

    switch (w->subStep) {
    case 0:
        w->arenaRole = 0;
        (w->subStep)++;
        WinSlide_Step(&w->kindWin, -0xE6, 0x72);
        break;
    case 1:
        switch (WinSlide_Step(&w->kindWin, -0xE6, 0x72)) {
        case MENU_DONE:
            (w->subStep)++;
            break;
        }
        break;
    case 2:
        btn = pad & PAD_ALL;
        if (btn & PAD_CIRCLE) {
            w->subStep = 0;
            role = w->cursor + 2;
            if (role == 3 && gSave.arenaEms[0].emKind == 0) {
                LobbyText_Open(0x67);
                return MENU_UNDONE;
            }
            w->arenaRole = role;
            w->joinRole = w->cursor + 2;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        if (btn & PAD_CROSS) {
            w->subStep = 0;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        w->cursor = Menu_WrapUpDown(w->cursor, 3);
        break;
    }
    return MENU_RUNNING;
}

QuestInfo *Quest_FindDownloaded(u16 id);
s32 QuestCounter_CanJoin(s16 idx);
s32 Quest_IsDownloadId(s32);
extern u16 gLobbyLevel2Count;

/* The board's cursor: up and down over the gLobbyLevel2Count rooms (QuestCounterMenu.quest).
 * Circle on a room that can be joined takes its quest (questId; the quest window's
 * pages: 2 for the arena cup, else 4 with joinRole 0) and returns MENU_DONE; a
 * downloaded quest the player can't use gives MENU_UNDONE, a room that can't be
 * joined only the refuse sound. Cross gives MENU_CANCEL; MENU_RUNNING meanwhile. */
s32 QuestCounter_ChooseQuest(u16 pad)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    s32 btn = pad & PAD_ALL;
    PostedQuest *post = &gPostedQuests[w->quest];

    if (btn & PAD_CIRCLE) {
        if (QuestCounter_CanJoin(w->quest) == 0) {
            LobbySe_Play(LOBBY_SE_REFUSE);
            return MENU_RUNNING;
        }
        if (Quest_IsDownloadId(post->quest) == 1 && (gLobby->dlQuestsReady == 0 || Quest_FindDownloaded(post->quest) == NULL)) {
            LobbySe_Play(LOBBY_SE_REFUSE);
            return MENU_UNDONE;
        }
        gLobbyLevel2Index = w->quest;
        w->questId = post->quest;
        switch (post->desk) {
        case 3:
            w->pages = 2;
            break;
        default:
            w->pages = 4;
            w->joinRole = 0;
            break;
        }
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        return MENU_DONE;
    }
    if (btn & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    w->quest = Menu_WrapUpDown(w->quest, gLobbyLevel2Count);
    return MENU_RUNNING;
}

s32 QuestCounter_CanJoin(s16 idx);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
s32 Menu_PushNoticeOnce(s32 arg0);

/* The quest window: left and right turn its pages. Circle gives MENU_DONE; cross,
 * or a room that can no longer be joined (with a notice), MENU_CANCEL; MENU_RUNNING
 * meanwhile. The step it sets on cross is set again by its caller. */
s32 QuestCounter_ViewQuest(u16 pad)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    LevelEntry *room = &gLobbyLevel2Entries[w->quest];
    s32 btn;

    if (QuestCounter_CanJoin(w->quest) == 0) {
        switch (room->state) {
        case 4:
            Menu_PushNoticeOnce(10);
            break;
        default:
            Menu_PushNoticeOnce(9);
            break;
        }
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    btn = pad & PAD_ALL;
    if (btn & PAD_CIRCLE) {
        gLobbyLevel2Index = w->quest;
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        return MENU_DONE;
    }
    if (btn & PAD_CROSS) {
        w->step = QUEST_COUNTER_CLOSE;
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    w->page = Menu_WrapLeftRight(w->page, w->pages);
    return MENU_RUNNING;
}

s32 Quest_CheckOrderTerms(u8 id, u8 flag);
void Talk_Say(s32 arg0, s8 arg1);
s32 LobbyTime_IsPm(void);

/* Whether the chosen quest can't be taken: 1, with a message, for an arena cup
 * quest while it isn't afternoon in the town (LobbyTime_IsPm) or when the room's
 * record 2 is at its first value and gSave.arenaEms[0] holds no monster; for a
 * downloaded quest that isn't there; when the order terms aren't met
 * (Quest_CheckOrderTerms); when the fee is more than gSave.money. 0 when it can. */
s32 QuestCounter_CheckQuest(void)
{
    PostedQuest *post = &gPostedQuests[((QuestCounterMenu *)&gMenuWork)->quest];
    QuestInfo *info;

    if (post->desk == 3) {
        if (LobbyTime_IsPm() == 0) {
            LobbyText_Open(0x91);
            return 1;
        }
        if (gLobbyRecs.rec[2].cur == 0 && gSave.arenaEms[0].emKind == 0) {
            LobbyText_Open(0x66);
            return 1;
        }
    }
    info = Quest_GetInfoAny(post->quest);
    if (Quest_IsDownloadId(post->quest) == 1 && (gLobby->dlQuestsReady == 0 || info->id != post->quest)) {
        Talk_Say(4, 0);
        return 1;
    }
    if (Quest_CheckOrderTerms(info->terms, 1) == 0) {
        if (post->desk == 3) {
            LobbyText_Open(0x92);
        } else {
            LobbyText_Open(0x65);
        }
        return 1;
    }
    if (post->money > gSave.money) {
        LobbyText_Open(0x68);
        return 1;
    }
    return 0;
}

u16 Pad_GetPressedRepeat(s32 pad);
void * LobbyLevel2_Entry(s32 arg0);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
u8 Kbd_UsbNewKey(void);

/* The room's password: opens the chat keyboard for 8 characters into pass, runs it
 * and returns MENU_DONE once it is closed; MENU_CANCEL (with a notice) when the
 * room can no longer be joined; MENU_RUNNING meanwhile. */
s32 QuestCounter_InputPassword(void *pass)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    LevelEntry *room;
    s32 step;

    Pad_GetPressedRepeat(0);
    Kbd_UsbNewKey();
    room = LobbyLevel2_Entry(w->quest);
    switch (step = w->subStep) {
    case 0:
        w->subStep = step + 1;
        Kbd_SetPos(0x3A, 80.0f);
        Chat_OpenKbd(0, 6, 8, pass);
        gFieldControl = 0;
        break;
    case 1:
        if ((s8)Kbd_Run(pass, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            (w->subStep)++;
            break;
        }
        if (QuestCounter_CanJoin(w->quest) == 0) {
            switch (room->state) {
            case 4:
                Menu_PushNoticeOnce(0xA);
                break;
            default:
                Menu_PushNoticeOnce(9);
                break;
            }
            Kbd_Cancel();
            w->subStep = 0;
            gFieldControl = 1;
            return MENU_CANCEL;
        }
        break;
    case 2:
        Kbd_Cancel();
        w->subStep = 0;
        gFieldControl = 1;
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/* Whether room idx can be joined: open (state 3), not full, and its quest posted
 * at this hall (PostedQuest.area is gScene.area). */
s32 QuestCounter_CanJoin(s16 idx)
{
    LevelEntry *room = &gLobbyLevel2Entries[idx];
    PostedQuest *post = &gPostedQuests[idx];

    if (room->state != 3 || room->users >= room->maxUsers) {
        return 0;
    }
    return post->area == gScene.area;
}


/* Slides in the board's five windows (QuestCounterMenu.board) from x fromX to
 * toX, one a frame after the other (subStep counts them, delay the frames
 * between), and moves those already started; those not started yet wait at
 * fromX. Returns MENU_RUNNING while any moves, MENU_DONE once all have settled. */
s32 QuestCounter_OpenWindows(s32 fromX, s32 toX)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    s16 busy = 0;
    s16 i;

    for (i = 0; i < 5; i++) {
        if (w->board[i].state != 0) {
            WinSlide_Step(&w->board[i], fromX, toX);
            busy = 1;
        }
    }
    switch (w->subStep) {
    case 0:
        w->delay = 0;
        WinSlide_Step(w->board, fromX, toX);
        w->board[1].pos = w->board[2].pos = w->board[3].pos = w->board[4].pos = fromX;
        busy = 1;
        (w->subStep)++;
        break;
    case 1:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[1], fromX, toX);
            busy = 1;
        }
        break;
    case 2:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[2], fromX, toX);
            busy = 1;
        }
        break;
    case 3:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[3], fromX, toX);
            busy = 1;
        }
        break;
    case 4:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[4], fromX, toX);
            busy = 1;
        }
        break;
    }
    if (busy == 0) {
        w->subStep = 0;
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/quest_counter", gQuestCounterLabelFmt);

void Text_PrintfColored(const char *, ...);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
s32 sprintf(char *, const char *, ...);
void Sjis_Widen(char *, char *);
extern char gQuestCounterLabelFmt[];
char *strcpy(char *, const char *);
void Text_Printf(const char *fmt, ...);

#pragma optimization_level 4
/* Draws the text of board row `row`: the room's label, then for an open room with
 * a quest the hall and desk it was posted at, the poster (its user id while
 * gChatHideNames, else its name), the members out of the room's size and the fee
 * if any (this half from x + 0x40); for any other room its state alone. Returns the
 * row panel's colour: grey for a room not open or a quest from another hall.
 * Built at optimization level 4, which gives the game's code for this function
 * (the file's other functions build the same at levels 3 and 4). */
s32 QuestCounter_DrawBoardRow(s16 x, s16 y, s16 row)
{
    PostedQuest *post;
    LevelEntry *room;
    u32 feeColor;
    u32 panelColor;
    u32 textColor;
    char *label;
    s32 lineY;
    char line[0x40];
    char wide[0x40];

    post = &gPostedQuests[row];
    room = &gLobbyLevel2Entries[row];
    Font_SetSpacing(0x12, 0x12);
    if (post->area == gScene.area) {
        textColor = 0x80101010;
        feeColor = 0x80301010;
        panelColor = 0xFFE0D8C8;
    } else {
        textColor = 0x80101010;
        feeColor = 0x80401017;
        panelColor = 0xFF808080;
    }
    Font_SetColor(textColor);
    lineY = y + 0x1B;
    Font_SetPos(x + 0x24, lineY);
    Text_PrintfColored(gArenaFeedTexts[ARENA_FEED_DIGITS + 1 + row]); /* the row number, from 1 */
    textColor = 0x80101010;
    if (room->state != 3 || post->quest == 0) {
        switch (room->state) {
        case 0:
        case 1:
        case 7:
        case 8:
            label = gQuestCounterTexts[QUEST_COUNTER_TEXT_NOT_TAKEN];
            break;
        case 2:
            label = gQuestCounterTexts[QUEST_COUNTER_TEXT_PREPARING];
            break;
        case 4:
            label = gQuestCounterTexts[QUEST_COUNTER_TEXT_FULL];
            textColor = 0x807F5230;
            break;
        case 5:
            label = gQuestCounterTexts[QUEST_COUNTER_TEXT_DEPARTED];
            break;
        case 6:
            label = gQuestCounterTexts[QUEST_COUNTER_TEXT_DISBANDING];
            break;
        default:
            label = NULL;
            break;
        }
        if (label != NULL) {
            Font_SetColor(textColor);
            Font_SetPos(Text_CenterX(label, x + 0xBE, 0x12), lineY);
            Text_PrintfColored(label);
        }
        return 0xFF808080;
    }
    if (post->money != 0) {
        y += 8;
    } else {
        y += 0x11;
    }
    x += 0x40;
    Font_SetPos(x, y);
    switch (post->area) {
    case AREA_TAVERN:
        label = gQuestBoardRows[BOARD_ROW_TAVERN];
        break;
    case AREA_TOWN_ARENA:
        label = gQuestBoardRows[BOARD_ROW_ARENA];
        break;
    case AREA_ELDER_HALL:
    default:
        label = gQuestBoardRows[BOARD_ROW_ELDER_HALL];
        break;
    }
    Text_Printf(gQuestCounterLabelFmt, label, gQuestBoardRows[BOARD_ROW_FREE + post->desk]); /* the desks' names follow the three halls' */
    Font_SetColor(textColor);
    memset(line, 0, 0x11);
    if (gChatHideNames) {
        strcpy(line, post->userId);
    } else {
        strcpy(line, post->name);
    }
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(gQuestCounterLabelFmt, gQuestBoardRows[BOARD_ROW_TAKER], line);
    Font_SetPos(x + 0xA2, y);
    sprintf(line, gQuestBoardRows[BOARD_ROW_MEMBERS_FMT], room->users, room->maxUsers);
    Sjis_Widen(line, wide);
    Text_PrintfColored(wide);
    if (post->money != 0) {
        Font_SetColor(feeColor);
        Font_SetPos(x, y + 0x14);
        Text_PrintfColored(gQuestBoardRows[BOARD_ROW_WANTED], post->money);
    }
    return panelColor;
}
#pragma optimization_level reset

void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
s32 QuestCounter_DrawBoardRow(s16 x, s16 y, s16 i);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 colour, s32 spr, u8 a);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
extern u16 gFrameTick;

/* The quest board: its title bar and the money held, then a panel per room at its
 * window's x (win, one per row) from y down, with its text and colour from
 * QuestCounter_DrawBoardRow, a lock mark on rooms with a password
 * (LevelEntry.locked), and blinking arrows beside the row under the cursor
 * (QuestCounterMenu.quest). */
void QuestCounter_DrawBoard(WinSlide *win, s16 y)
{
    QuestCounterMenu *menu = (QuestCounterMenu *)&gMenuWork;
    s16 i;
    s32 panelColor;
    s16 x;
    s16 blink;

    Font_SetSpacing(0x14, 0x14);
    Menu_DrawLabelBar(0xF0, 0x12, 0xA0, 0xFFB0B000, gListTitles[LIST_TITLE_HUNTER_RECRUIT], 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    for (i = 0; i < gLobbyLevel2Count; i++, win++) {
        x = win->pos;
        panelColor = QuestCounter_DrawBoardRow(x, y, i);
        Icon_DrawMode(x, y, 0x17C, 0x48, panelColor, 0, i % 4);
        if (gLobbyLevel2Entries[i].locked != 0) {
            Icon_Draw(x + 0x14, y + 4, 0x1A, 0x16, -1, 0x47);
        }
        if (i == menu->quest) {
            if (gFrameTick & 0x10) {
                blink = 10;
            } else {
                blink = 0;
            }
            Icon_Draw(x - 10 + blink, y + 0x14, 0x20, 0x20, 0xFFA0A040, 0x29);
            Icon_Draw(x + 0x166 - blink, y + 0x14, 0x20, 0x20, 0xFFA0A040, 0x28);
        }
        y += 0x4E;
    }
}

/* unit's own declaration: an s32 result; with the definition's u8, Quest_DrawArenaTeams and QuestCounter_DrawMembers compile differently */
s32 Equip_GetClass(EquipItem *e);
char *Quest_GetText(u16 id, s32 k);
void Text_PrintfColored(const char *, ...);
/* unit's own declaration: s16 x and y, a u8 fifth and an s32 sixth argument; with the definition's s32, s32, s32, s32, s16, s8, QuestCounter_DrawMembers and Quest_DrawArenaTeams compile differently */
void Icon_DrawWeaponClass(s16, s16, s32, s32, u8, s32);
s32 sprintf(char *, const char *, ...);
void Font_SetPos(s16, s16);
void Sjis_Widen(char *, char *);
extern char gQuestCounterLabelFmt[];

/* The quest window's members page: the quest's title, then the room's four
 * members (QuestCounterMenu.members) with their weapon class icon, name and user
 * id. y steps once inside a call's arguments, as the game's code does. */
void QuestCounter_DrawMembers(s16 x, s16 y, QuestInfo *quest)
{
    char line[0x40];
    char wide[0x60];
    NetUser *member;
    s32 i;

    member = ((QuestCounterMenu *)&gMenuWork)->members;
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored("%s", Quest_GetText(quest->id, 0));
    y += 0x32;
    Font_SetColor(0x80101010);
    x += 0x32;
    for (i = 0; i < LOBBY_ROOM_USER_COUNT; i++) {
        if (member->id[0] != 0) {
            Icon_DrawWeaponClass(x - 0x36, y + 2, 0x28, -1, Equip_GetClass(&((HunterCard *)member->text)->weapon), 1);
            sprintf(line, gQuestCounterLabelFmt, gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL], member->name);
            Sjis_Widen(line, wide);
            Font_SetPos(x, y);
            Text_PrintfColored(wide);
            y += 0x14;
            sprintf(line, gQuestCounterLabelFmt, gLobbyLabelTexts[LOBBY_LABEL_ID], member->id);
            Sjis_Widen(line, wide);
            Font_SetPos(x, y);
            Text_PrintfColored(wide);
        }
        y += 0x1A;
        member++;
    }
}

void Quest_DrawOverview(s16, s16, QuestInfo *);
void Quest_DrawTerms(s16, s16, QuestInfo *);
void Quest_DrawDetails(s16, s16, QuestInfo *);
void QuestCounter_DrawMembers(s16, s16, QuestInfo *);

/* The quest window's page (QuestCounterMenu.page) for a quest other than the arena
 * cup's: the overview, the terms, the details or the members, from x + 0x28. */
void QuestCounter_DrawQuestPage(s16 x, s16 y)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    QuestInfo *info;

    info = Quest_GetInfoAny(gPostedQuests[w->quest].quest);
    Font_SetColor(0x80101010);
    switch (w->page) {
    case 0:
        Quest_DrawOverview(x + 0x28, y, info);
        break;
    case 1:
        Quest_DrawTerms(x + 0x28, y, info);
        break;
    case 2:
        Quest_DrawDetails(x + 0x28, y, info);
        break;
    case 3:
        QuestCounter_DrawMembers(x + 0x28, y, info);
        break;
    }
}

u8 Lobby_FindMember(u8 *id);
extern NetUser gNetUsers[];

/* The arena cup's teams page: under the cup's title, the members in list (four
 * NetUsers) by the arenaRole of their hunter card in gNetUsers, roles 1 and 2
 * under the first team's heading and 3 and 4 under the second's (QUEST_COUNTER_TEXT_TEAM_A, _TEAM_B),
 * each with its weapon class icon, name and user id. A missing role 2, 3 or 4
 * shows the empty mark (roles 2 and 4 only in a room of 3 or more: a room of two
 * holds only the two leaders). Each loop keeps its own pointer, and the second
 * half clears shown before the colour call rather than in the loop's head: both
 * as the game's code does. */
void Quest_DrawArenaTeams(s16 x, s16 y, NetUser *list, s8 room)
{
    s16 rowY;
    char line[0x40];
    s16 nameX;
    NetUser *role2User;
    NetUser *netUser;
    NetUser *role4User;
    char *label;
    s16 i;
    NetUser *role3User;
    char wide[0x60];
    s8 netIndex;
    NetUser *role1User;
    s16 shown;
    HunterCard *card;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gQuestBoardRows[BOARD_ROW_ARENA_CUP], x + 0xA0, 0x12), y += 0x14);
    Text_PrintfColored(gQuestBoardRows[BOARD_ROW_ARENA_CUP]);
    Font_SetPos(Text_CenterX(gQuestCounterTexts[QUEST_COUNTER_TEXT_TEAM_A], x + 0xA0, 0x12), y += 0x28);
    Text_PrintfColored(gQuestCounterTexts[QUEST_COUNTER_TEXT_TEAM_A]);
    y += 0x1E;
    nameX = x + 0x5A;
    Font_SetColor(0x80101010);
    for (shown = 0, i = 0, role1User = list; i < LOBBY_ROOM_USER_COUNT; i++, role1User++) {
        if (role1User->id[0] != 0) {
            netIndex = Lobby_FindMember((u8 *)role1User->id);
            netUser = &gNetUsers[netIndex];
            card = (HunterCard *)netUser->text;
            if (((HunterCard *)netUser->text)->arenaRole == 1) {
                rowY = y;
                Icon_DrawWeaponClass(nameX - 0x36, rowY + 2, 0x28, -1, Equip_GetClass(&card->weapon), 1);
                sprintf(line, gQuestCounterLabelFmt, gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL], role1User->name);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, y);
                Text_PrintfColored(wide);
                sprintf(line, gQuestCounterLabelFmt, gLobbyLabelTexts[LOBBY_LABEL_ID], role1User->id);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, rowY + 0x14);
                Text_PrintfColored(wide);
                y += 0x2E;
                shown++;
            }
        }
    }
    for (shown = 0, i = 0, role2User = list; i < LOBBY_ROOM_USER_COUNT; i++, role2User++) {
        if (role2User->id[0] != 0) {
            netIndex = Lobby_FindMember((u8 *)role2User->id);
            netUser = &gNetUsers[netIndex];
            card = (HunterCard *)netUser->text;
            if (((HunterCard *)netUser->text)->arenaRole == 2) {
                rowY = y;
                Icon_DrawWeaponClass(nameX - 0x36, rowY + 2, 0x28, -1, Equip_GetClass(&card->weapon), 1);
                sprintf(line, gQuestCounterLabelFmt, gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL], role2User->name);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, y);
                Text_PrintfColored(wide);
                sprintf(line, gQuestCounterLabelFmt, gLobbyLabelTexts[LOBBY_LABEL_ID], role2User->id);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, rowY + 0x14);
                Text_PrintfColored(wide);
                y += 0x2E;
                shown++;
            }
        }
    }
    if (shown == 0 && gLobbyLevel2Entries[room].maxUsers >= 3) {
        label = gQuestCounterTexts[QUEST_COUNTER_TEXT_OPEN];
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(label, x + 0xA0, 0x12), y);
        Text_PrintfColored(label);
    }
    y = 0xF4;
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gQuestCounterTexts[QUEST_COUNTER_TEXT_TEAM_B], x + 0xA0, 0x12), y += 0x1E);
    Text_PrintfColored(gQuestCounterTexts[QUEST_COUNTER_TEXT_TEAM_B]);
    y += 0x1E;
    nameX = x + 0x5A;
    shown = 0;
    Font_SetColor(0x80101010);
    for (i = 0, role3User = list; i < LOBBY_ROOM_USER_COUNT; i++, role3User++) {
        if (role3User->id[0] != 0) {
            netIndex = Lobby_FindMember((u8 *)role3User->id);
            netUser = &gNetUsers[netIndex];
            card = (HunterCard *)netUser->text;
            if (((HunterCard *)netUser->text)->arenaRole == 3) {
                rowY = y;
                Icon_DrawWeaponClass(nameX - 0x36, rowY + 2, 0x28, -1, Equip_GetClass(&card->weapon), 1);
                sprintf(line, gQuestCounterLabelFmt, gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL], role3User->name);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, y);
                Text_PrintfColored(wide);
                sprintf(line, gQuestCounterLabelFmt, gLobbyLabelTexts[LOBBY_LABEL_ID], role3User->id);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, rowY + 0x14);
                Text_PrintfColored(wide);
                y += 0x2E;
                shown++;
            }
        }
    }
    if (shown == 0) {
        label = gQuestCounterTexts[QUEST_COUNTER_TEXT_OPEN];
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(label, x + 0xA0, 0x12), y);
        Text_PrintfColored(label);
        y += 0x2E;
    }
    shown = 0;
    Font_SetColor(0x80101010);
    for (i = 0, role4User = list; i < LOBBY_ROOM_USER_COUNT; i++, role4User++) {
        if (role4User->id[0] != 0) {
            netIndex = Lobby_FindMember((u8 *)role4User->id);
            netUser = &gNetUsers[netIndex];
            card = (HunterCard *)netUser->text;
            if (((HunterCard *)netUser->text)->arenaRole == 4) {
                rowY = y;
                Icon_DrawWeaponClass(nameX - 0x36, rowY + 2, 0x28, -1, Equip_GetClass(&card->weapon), 1);
                sprintf(line, gQuestCounterLabelFmt, gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL], role4User->name);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, y);
                Text_PrintfColored(wide);
                sprintf(line, gQuestCounterLabelFmt, gLobbyLabelTexts[LOBBY_LABEL_ID], role4User->id);
                Sjis_Widen(line, wide);
                Font_SetPos(nameX, rowY + 0x14);
                Text_PrintfColored(wide);
                y += 0x2E;
                shown++;
            }
        }
    }
    if (shown == 0 && gLobbyLevel2Entries[room].maxUsers >= 3) {
        label = gQuestCounterTexts[QUEST_COUNTER_TEXT_OPEN];
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(label, x + 0xA0, 0x12), y);
        Text_PrintfColored(label);
    }
}

s32 ArenaCup_IsFormRowOpen(s8);

/* gLobbyRecs seen as rows of sizeof(LobbyRec) bytes from its start: row i starts
 * 0x55 bytes (the head of LobbyRecs) before rec[i], so from +0x56 on its fields are
 * rec[i]'s. QuestCounter_DrawRoomSettings reads the records through it: the game's code
 * indexes them from gLobbyRecs' start, and &gLobbyRecs.rec[i] costs one more
 * register. */
typedef struct LobbyRecRow {
    /* 0x000 */ u8 head[0x56];          /* LobbyRecs' head and rec[i].flags */
    /* 0x056 */ char name[0x41];
    /* 0x097 */ u8 subCount;
    /* 0x098 */ u8 editable;
    /* 0x099 */ u8 defaultChoice;
    /* 0x09A */ u8 cur;                 /* the value chosen */
    /* 0x09B */ u8 pad9B[0xBD - 0x9B];
    /* 0x0BD */ char subName[0x20][0x41];
    /* 0x8DD */ u8 unk8DD[0x14A8 - 0x8DD];
} LobbyRecRow;
typedef char LobbyRecRow_size_check[(sizeof(LobbyRecRow) == 0x14A8) ? 1 : -1];

/* The arena cup's settings page: under the cup's title, each of the room's
 * gLobbyRecs.n records with the value chosen; the fourth only while
 * ArenaCup_IsFormRowOpen(4). room (the board row) is not read. */
void QuestCounter_DrawRoomSettings(s16 x, s32 startY, s32 room)
{
    s16 y = startY;
    s16 i;
    LobbyRecRow *row;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gQuestBoardRows[BOARD_ROW_ARENA_CUP], x + 0xA0, 0x12), y += 0x14);
    Text_PrintfColored(gQuestBoardRows[BOARD_ROW_ARENA_CUP]);
    y += 0x14;
    for (i = 0; i < gLobbyRecs.n; i++) {
        row = &((LobbyRecRow *)&gLobbyRecs)[i];
        if (i == 3) {
            if (ArenaCup_IsFormRowOpen(4) == 0) {
                continue;
            }
        } else if (i == 3) { /* never true: the game's test */
            if (ArenaCup_IsFormRowOpen(3) == 0) {
                continue;
            }
        }
        Font_SetColor(0x80301010);
        Font_SetPos(x + 0x28, y += 0x14);
        Text_PrintfColored("%10s%s", row->name, gLobbyWords[LOBBY_WORD_COLON]);
        Font_SetColor(0x80101010);
        Font_SetPos((s16)(x + 0x28) + 0x6C, y);
        Text_PrintfColored(row->subName[row->cur]);
    }
}


/* The zeros that end the file's .rodata (no reader). */
const u8 gQuestCounterRodataEnd[8] = { 0 };
