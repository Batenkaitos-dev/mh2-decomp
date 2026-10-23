/*
 * The elder dragon forecast and the raid's town movies.
 *
 * The forecast is a town menu (TOWN_MODE_ELDER_FORECAST) of three rows,
 * gElderForecastTexts' first three labels:
 * - the habitats (ELDER_FORECAST_HABITAT): a panel with one line per elder
 *   dragon, the place and time of day of the season's quest that hunts it,
 *   from the town's quest lists (TownQuestLists.quests);
 * - the attack (ELDER_FORECAST_ATTACK): the townsperson's line about the
 *   elder dragon raid (gElderRaidEvent);
 * - the events (ELDER_FORECAST_EVENTS): the downloaded quests, up to three,
 *   each with three pages.
 * Its state is an ElderForecastMenu in gMenuWork. ElderForecast_Open and
 * ElderForecast_Close open and end it, ElderForecast_Update runs it and
 * ElderForecast_Draw draws it. ElderForecast_Roll fills the season's quest
 * lists offline, by chance, from gElderForecastQuests.
 *
 * The raid: ElderRaid_SetState turns it on when it begins and
 * ElderRaid_CheckChanged off when it is over, each asking for its town movie
 * (LobbyState.movieState) once per raid. The town's update starts that movie
 * when LobbyTown_WantsMovie says so (LobbyTown_StartMovie) and runs it until
 * LobbyTown_MovieDone.
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "save.h"
#include "quest.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "village_cond.h"
#include "player.h"
#include "scene.h"
#include "area.h"
#include "pad.h"
#include "menu_task.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "camera.h"
#include "elder_forecast.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "official_desk.h"
#include "quest_desk_draw.h"
#include "quest_offers.h"
#include "rand.h"
#include "talk.h"
#include "town_frame.h"
#include "town_player_states.h"
#include "town_players.h"

extern u16 gElderForecastQuests[ELDER_MONSTERS][WANTED_SEASONS][ELDER_QUEST_CHOICES];
extern ElderRaidEvent gElderRaidEvent;
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
extern s16 gElderMonstersOnline[];
extern s16 gElderMonstersOffline[];
extern u16 gTalkLine;
extern char *gTalkLineText;
extern u8 gMenuWork;
void Text_SetSize(s32);

extern s32 gTownMode;
extern s32 gTownMenuOpen;
/* Opens the elder dragon forecast (TOWN_MODE_ELDER_FORECAST) for the
 * season it is now. */
void ElderForecast_Open(void)
{
    ElderForecastMenu *menu = (ElderForecastMenu *)&gMenuWork;

    memset(menu, 0, MENU_WORK_SIZE);
    menu->season = LobbyWorld_GetSeason();
    menu->isPm = LobbyTime_IsPm();
    gTownMode = TOWN_MODE_ELDER_FORECAST;
    gTownMenuOpen = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ElderForecast_Draw, 0, 0, 0);
    LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
}

/* Ends the forecast: closes the town's menu and turns the camera back to
 * the player. */
void ElderForecast_Close(void)
{
    gTownMode = TOWN_MODE_FREE;
    gTownMenuOpen = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/* Lists the downloaded quests as the event quests, from the first, on
 * their first page. */
void ElderForecast_ListEvents(ElderForecastMenu *menu)
{
    s32 i;
    QuestInfo *info;

    memset(menu->events, 0, sizeof(menu->events));
    for (i = 0; i < gLobby->dlQuestCount; i++) {
        info = Quest_DownloadedInfo(i);
        if (info != NULL) {
            menu->events[i] = info->id;
        }
    }
    menu->eventCount = gLobby->dlQuestCount;
    menu->eventSel = 0;
    menu->page = ELDER_PAGE_OVERVIEW;
    menu->pages = ELDER_PAGES;
}

/*
 * Runs a frame of the forecast (TOWN_MODE_ELDER_FORECAST). In the rows,
 * circle opens the habitats' panel (marking the forecast as heard:
 * gElderForecastHeard), the raid's line or the event quests, and cross says
 * goodbye and closes the menu. The panel and the event quests go back to the
 * rows when left.
 */
void ElderForecast_Update(void)
{
    ElderForecastMenu *menu = (ElderForecastMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (menu->mode) {
    case ELDER_MODE_MENU:
        switch (ElderForecast_MenuInput(menu, pad)) {
        case MENU_DONE:
            switch (menu->cursor) {
            case ELDER_FORECAST_HABITAT:
                menu->mode = ELDER_MODE_HABITATS;
                ElderForecast_Load(menu->habitats, menu->season);
                WinSlide_Step(&menu->win, 640, 320);
                gElderForecastHeard = 1;
                CamFocus_SetAside(1);
                break;
            case ELDER_FORECAST_ATTACK:
                menu->mode = ELDER_MODE_ATTACK;
                ElderForecast_SayRaid();
                break;
            case ELDER_FORECAST_EVENTS:
                menu->mode = ELDER_MODE_EVENTS;
                WinSlide_Step(&menu->win, 640, 280);
                ElderForecast_ListEvents(menu);
                CamFocus_SetAside(1);
                break;
            }
            break;
        case MENU_CANCEL:
            menu->mode = ELDER_MODE_LEAVE;
            Talk_Say(1, 0);
            break;
        }
        break;
    case ELDER_MODE_EVENTS:
        switch (ElderForecast_ListInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = ELDER_MODE_MENU;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case ELDER_MODE_HABITATS:
        switch (ElderForecast_ViewInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = ELDER_MODE_MENU;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case ELDER_MODE_ATTACK:
        switch (Talk_Update()) {
        case 0:
            menu->mode = ELDER_MODE_MENU;
            break;
        }
        break;
    case ELDER_MODE_LEAVE:
        switch (Talk_Update()) {
        case 0:
            ElderForecast_Close();
            break;
        }
        break;
    }
}

/* The event quests' input: slides the window in, then up and down change
 * the quest and left and right its page; cross goes back (MENU_CANCEL).
 * MENU_RUNNING until then. */
s32 ElderForecast_ListInput(ElderForecastMenu *menu, u16 pad)
{
    switch (menu->step) {
    case ELDER_STEP_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->step++;
            break;
        }
        break;
    case ELDER_STEP_INPUT:
        if (pad & PAD_CROSS) {
            menu->step = ELDER_STEP_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        if (pad & (PAD_UP | PAD_DOWN)) {
            menu->eventSel = Menu_WrapUpDownSe(menu->eventSel, menu->eventCount, LOBBY_SE_WINDOW_SLIDE);
        } else {
            menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
        }
        break;
    }
    return MENU_RUNNING;
}

/* Fills out[i] with the quest of the season's list whose first goal is the
 * elder dragon gElderMonstersOnline[i] (offline, gElderMonstersOffline[i]),
 * or 0. Online from hunter rank 41 the lists come from the higher set
 * (TownQuestLists.quests[1]). */
void ElderForecast_Load(u16 *out, s32 season)
{
    TownQuestLists *lists = (TownQuestLists *)&gTownQuestLists;
    s32 i;
    s32 j;
    u16 id;
    s16 *monsters;
    QuestInfo *info;

    if (Game_IsOnline() == 1) {
        monsters = gElderMonstersOnline;
    } else {
        monsters = gElderMonstersOffline;
    }
    memset(out, 0, ELDER_MONSTERS * sizeof(u16));
    for (i = 0; i < ELDER_MONSTERS; i++) {
        for (j = 0; j < ELDER_LIST_QUESTS; j++) {
            if (Game_IsOnline() == 1 && gSave.hr >= 41) {
                id = lists->quests[1][season][j];
            } else {
                id = lists->quests[0][season][j];
            }
            if (id != 0) {
                info = Quest_GetDiscInfo(id);
                if (info->goals[0].target == monsters[i]) {
                    out[i] = id;
                }
            }
        }
    }
}

/* The habitats' panel's input: slides the window in, then waits for cross
 * (MENU_CANCEL). MENU_RUNNING until then. */
s32 ElderForecast_ViewInput(ElderForecastMenu *menu, u16 pad)
{
    switch (menu->step) {
    case ELDER_STEP_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 320)) {
        case MENU_DONE:
            menu->step++;
            break;
        }
        break;
    case ELDER_STEP_INPUT:
        if (pad & PAD_CROSS) {
            menu->step = ELDER_STEP_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Says the raid's line (talk line 2), with the two values the raid's event
 * gives it filled in (gElderRaidEvent.talkArg1, talkArg2). The event is
 * reached through a pointer cast through a byte pointer: the compiler then
 * keeps its address in a register across the calls, as the game's code
 * does; with the plain address it rebuilds the address at each use. */
void ElderForecast_SayRaid(void)
{
    ElderRaidEvent *raid = (ElderRaidEvent *)(u8 *)&gElderRaidEvent;

    Talk_Say(2, 0);
    Text_FillDollar(Talk_LineText(gTalkLine), raid->talkArg1);
    gTalkLineText = Text_FillDollar2(raid->talkArg2);
}

/* The rows' input: up and down move the cursor; circle takes the row
 * (MENU_DONE), unless the habitats aren't open yet (ElderForecast_IsUnlocked)
 * or no quest has been downloaded for the events; cross leaves (MENU_CANCEL).
 * MENU_RUNNING otherwise. */
s32 ElderForecast_MenuInput(ElderForecastMenu *menu, u16 pad)
{
    u8 refused;

    if (pad & PAD_CIRCLE) {
        refused = 0;
        switch (menu->cursor) {
        case ELDER_FORECAST_HABITAT:
            if (ElderForecast_IsUnlocked() == 0) {
                refused = 1;
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else {
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            }
            break;
        case ELDER_FORECAST_ATTACK:
            LobbySe_Play(LOBBY_SE_CONFIRM);
            break;
        case ELDER_FORECAST_EVENTS:
            if (gLobby->dlQuestsReady == 0) {
                refused = 1;
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else {
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            }
            break;
        }
        if (refused == 0) {
            return MENU_DONE;
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else {
        menu->cursor = Menu_WrapUpDown(menu->cursor, ELDER_FORECAST_ROWS);
    }
    return MENU_RUNNING;
}

extern char gElderSeasonFmt[];
extern char gElderMonsterFmt[];
extern char gElderPlaceFmt[];
void Text_PrintfColored(char *fmt, ...);
s32 sprintf(char *buf, char *fmt, ...);
u16 Map_Group(u16 a);

/*
 * Draws the habitats' panel at (x, y): its title with the season, then a
 * line per elder dragon with the place and time of day of its quest
 * (quests[i], from ElderForecast_Load), or that it has none. Online the
 * panel lists four elder dragons, five from HR gate 3 with hunter rank 41;
 * offline, as many of the table's first as have come to the village
 * (Monster_IsAvailable). The forecast (TOWN_MODE_ELDER_FORECAST) draws the
 * panel's frame and rules first; the quest desk draws it without them.
 * isPm isn't used.
 * The frame's x steps (lineX += 10, += 256, then lineX = x) keep lineX a
 * variable: the compiler then builds the x arguments as the game's code does.
 */
void ElderForecast_DrawPanel(u16 *quests, s16 x, s16 y, s32 season, s32 isPm)
{
    QuestInfo *info;
    s16 i;
    s16 lineY;
    char buf[64];
    s16 midY;
    s16 *monster;
    s16 lineX;
    s16 count;
    s32 stage;

    lineX = x;
    lineY = y;
    if (gTownMode == TOWN_MODE_ELDER_FORECAST) {
        Icon_Draw(x, y, 300, 340, 0xFFE0D8C8, ICON_PANEL);
        midY = lineY + 170;
        lineX += 10;
        Icon_Draw(lineX, midY - 120, 256, 32, 0x80202020, 27);
        Icon_Draw(lineX, midY + 120, 256, 32, 0x80202020, 27);
        lineX += 256;
        Icon_Draw(lineX, midY - 120, 46, 32, 0x80202020, 28);
        Icon_Draw(lineX, midY + 120, 46, 32, 0x80202020, 28);
        lineX = x;
    }
    Font_SetColor(0x80301010);
    Font_SetSpacing(20, 20);
    lineY += 16;
    Font_SetColor(0x80301010);
    sprintf(buf, gElderSeasonFmt, gSeasonNames[season]);
    Font_SetPos(Text_CenterX(buf, x + 160, 20) - 20, lineY);
    Text_PrintfColored(gPriceInfoTexts[PRICE_INFO_BRACKET_FMT], buf);
    Font_SetPos(Text_CenterX(gElderForecastTexts[ELDER_FORECAST_PANEL_TITLE], x + 150, 20), lineY += 22);
    Text_PrintfColored(gElderForecastTexts[ELDER_FORECAST_PANEL_TITLE]);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x80101010);
    if (Game_IsOnline() == 1) {
        if (Cond_Check(COND_HR_GATE_3_HR_41) == 0) {
            count = ELDER_MONSTERS - 1;
        } else {
            count = ELDER_MONSTERS;
        }
        monster = gElderMonstersOnline;
    } else {
        monster = gElderMonstersOffline;
        count = 0;
        for (i = 0; i < ELDER_MONSTERS; i++) {
            if (Monster_IsAvailable(monster[i]) == 1) {
                count++;
            }
        }
    }
    lineX += 40;
    for (i = 0; i < count; i++) {
        Font_SetPos(lineX, lineY + 30);
        Text_PrintfColored(gElderMonsterFmt, gMonsterNames[*monster], gLobbyWords[LOBBY_WORD_COLON]);
        Font_SetPos(lineX + 40, lineY += 52);
        if (*quests != 0) {
            info = Quest_GetDiscInfo(*quests);
            stage = Map_Group(info->map);
            if ((info->flags & QUEST_FLAG_DAY) && (info->flags & QUEST_FLAG_DAY_AND_NIGHT)) {
                Text_PrintfColored(gElderPlaceFmt, gStageNames[stage], gLobbyWords[LOBBY_WORD_MID_DOT], gElderForecastTexts[ELDER_FORECAST_DAY_NIGHT]);
            } else if (info->flags & QUEST_FLAG_DAY) {
                Text_PrintfColored(gElderPlaceFmt, gStageNames[stage], gLobbyWords[LOBBY_WORD_MID_DOT], gElderForecastTexts[ELDER_FORECAST_DAY]);
            } else {
                Text_PrintfColored(gElderPlaceFmt, gStageNames[stage], gLobbyWords[LOBBY_WORD_MID_DOT], gElderForecastTexts[ELDER_FORECAST_NIGHT]);
            }
        } else {
            Text_PrintfColored(gElderForecastTexts[ELDER_FORECAST_NO_TIME]);
        }
        monster++;
        quests++;
    }
}

extern s8 gMapGroupMapIds[][2];
/* Draws the event quest's page (ElderForecastMenu.page) at (x - 20, y),
 * and its cleared mark at (x, y) once the quest has been cleared. */
static inline void ElderForecast_DrawPage(ElderForecastMenu *menu, s16 x, s16 y, QuestInfo *info)
{
    switch (menu->page) {
    case ELDER_PAGE_OVERVIEW:
        Quest_DrawOverview(x - 20, y, info);
        break;
    case ELDER_PAGE_TERMS:
        Quest_DrawTerms(x - 20, y, info);
        break;
    case ELDER_PAGE_DETAILS:
        Quest_DrawDetails(x - 20, y, info);
        break;
    }
    if (Save_IsQuestCleared(info->id)) {
        Quest_DrawClearMark(x, y);
    }
}

/* Draws the event quest shown on a quest sheet at (x, y): a tab per event
 * quest with its stage (the one shown negated), the page's number and the
 * page. */
void ElderForecast_DrawQuest(ElderForecastMenu *menu, s16 x, s16 y)
{
    QuestInfo *info = Quest_GetDiscInfo(menu->events[menu->eventSel]);
    s16 tabs[4];
    s32 i;
    s8 mapId;

    memset(tabs, 0, sizeof(tabs));
    for (i = 0; i < ELDER_EVENTS; i++) {
        if (menu->events[i] == 0) {
            break;
        }
        tabs[i] = Map_Group(Quest_GetDiscInfo(menu->events[i])->map);
        if (i == menu->eventSel) {
            tabs[i] = -tabs[i];
        }
    }
    mapId = gMapGroupMapIds[Map_Group(info->map)][0];
    Menu_DrawQuestSheet(x - 20, y - 14, 320, 360, tabs, menu->eventSel, mapId, menu->season);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 210, y += 5, menu->page, ELDER_PAGES, 1);
    Font_SetSpacing(18, 18);
    ElderForecast_DrawPage(menu, x + 40, y, info);
}

/* Draws the forecast: the rows, the habitats' panel or the event quest, by
 * its mode. */
void ElderForecast_Draw(MenuTask *task)
{
    ElderForecastMenu *menu = (ElderForecastMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->mode) {
    case ELDER_MODE_MENU:
        Menu_DrawLabelList(450, gElderForecastTexts, ELDER_FORECAST_ROWS, menu->cursor, 0xFFB0B000);
        break;
    case ELDER_MODE_HABITATS:
        ElderForecast_DrawPanel(menu->habitats, menu->win.pos, 58, menu->season, menu->isPm);
        break;
    case ELDER_MODE_EVENTS:
        ElderForecast_DrawQuest(menu, menu->win.pos, 58);
        break;
    }
}

/* Rolls the season's list of elder dragon quests (out[i] for elder dragon
 * i): with one choice, it is posted 60 times in 100; with two, each 40 times
 * in 100; else none (0). */
void ElderForecast_RollList(s32 season, u16 *out)
{
    s32 i;
    u16 *choices;
    s32 roll;

    if (season < 0 || season > WANTED_SEASONS - 1) {
        return;
    }
    for (i = 0; i < ELDER_MONSTERS; i++) {
        choices = gElderForecastQuests[i][season];
        roll = Rand_Next(1) % 100;
        if (choices[1] == 0) {
            if (roll < 60) {
                out[i] = choices[0];
            } else {
                out[i] = 0;
            }
        } else if (roll < 40) {
            out[i] = choices[0];
        } else if (roll < 80) {
            out[i] = choices[1];
        } else {
            out[i] = 0;
        }
    }
}

/* 1 if quest id is one of gElderForecastQuests' quests, else 0. */
s32 ElderForecast_HasQuest(u16 id)
{
    s16 i;
    s16 j;
    s16 k;

    for (i = 0; i < ELDER_MONSTERS; i++) {
        for (j = 0; j < WANTED_SEASONS; j++) {
            for (k = 0; k < ELDER_QUEST_CHOICES; k++) {
                if (id == gElderForecastQuests[i][j][k]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Offline, once village condition 21 holds (the condition that also opens
 * the habitats offline, ElderForecast_IsUnlocked): rolls the three seasons'
 * lists of elder dragon quests (ElderForecast_RollList) into
 * gTownQuestLists, and keeps a copy in gTownQuestListsCopy. */
void ElderForecast_Roll(void)
{
    TownQuestLists *lists = (TownQuestLists *)&gTownQuestLists;
    TownQuestLists *copy = (TownQuestLists *)&gTownQuestListsCopy;
    s32 i;

    if (Cond_Check(21) != 0) {
        memset(lists->quests, 0, sizeof(lists->quests));
        for (i = 0; i < WANTED_SEASONS; i++) {
            ElderForecast_RollList(i, lists->quests[0][i]);
        }
        memcpy(copy->quests, lists->quests, sizeof(lists->quests));
    }
}

/* The raid has begun: turns it on, with its movies unless the event says
 * none (gElderRaidEvent.noMovie), and asks for its start movie if this raid's
 * hasn't been played yet. The pointer cast through a byte pointer keeps
 * the event's address in a register, as in ElderForecast_SayRaid. */
void ElderRaid_SetState(void)
{
    ElderRaidEvent *raid = (ElderRaidEvent *)(u8 *)&gElderRaidEvent;

    if (raid->noMovie != 1) {
        gLobby->elderRaidState = ELDER_RAID_ON;
    } else {
        gLobby->elderRaidState = ELDER_RAID_ON_NO_MOVIE;
    }
    if (raid->id != gSave.raidStartMovieId && gLobby->elderRaidState == ELDER_RAID_ON) {
        gLobby->movieState = RAID_MOVIE_START;
    }
}

/* The raid is over: if its end movie hasn't been played for this raid and
 * the raid was on with its movies, asks for the end movie; the raid is off. */
void ElderRaid_CheckChanged(void)
{
    if (gElderRaidEvent.id != gSave.raidEndMovieId) {
        if (gLobby->elderRaidState == ELDER_RAID_ON) {
            gLobby->movieState = RAID_MOVIE_END;
        }
        gLobby->elderRaidState = ELDER_RAID_OFF;
    }
}

extern LobbyMenu *gLobbyMenu;

/* The game built the town movie's functions at optimization level 4. */
#pragma optimization_level 4
/* Starts the raid's town movie that waits: stands the player up if seated,
 * resets the town's subsystems and hides the menu layers, and records in the
 * save that this raid's start (or end) movie has been played. In
 * AREA_TOWN_ARENA the town music is told to stop with it. */
void LobbyTown_StartMovie(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ElderRaidEvent *raid = &gElderRaidEvent;

    if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
        Actor_SetState(pl, 0, TOWN_PC_STAND_UP);
    }
    Game_ResetSubsystems();
    gLobbyMenu->hideLayers = 1;
    gLobby->seqStep = 0;
    if (gLobby->movieState == RAID_MOVIE_START) {
        LobbyMovie_Prepare(0);
        gSave.raidStartMovieId = raid->id;
    } else {
        LobbyMovie_Prepare(1);
        gSave.raidEndMovieId = raid->id;
    }
    if (gScene.area == AREA_TOWN_ARENA) {
        LobbyBgm_SetStageCam(0);
    }
}

/* Runs the movie; 1 once it has finished (LobbyMovie_Run's last step, 3). */
s32 LobbyTown_MovieDone(void)
{
    switch (LobbyMovie_Run()) {
    case 3:
        return 1;
    }
    return 0;
}

/* 1 if the raid's town movie should play now: the raid has its movies, the
 * player is connected, in no town menu, with a movie waiting and no quest
 * taken, and not in AREA_TOWN_ARENA while its town music plays. */
s32 LobbyTown_WantsMovie(void)
{
    if (gElderRaidEvent.noMovie == 1) {
        return 0;
    }
    if (gLobby->connected == 0) {
        return 0;
    }
    if (gTownMode != TOWN_MODE_FREE) {
        return 0;
    }
    if (gLobby->movieState == RAID_MOVIE_NONE) {
        return 0;
    }
    if (gLobby->questTaken != 0) {
        return 0;
    }
    if (0 < LobbyBgm_GetTrack() && gScene.area == AREA_TOWN_ARENA) {
        return 0;
    }
    switch (gScene.area) {
    case AREA_ONLINE_VILLAGE:
    case AREA_HUB_0x84:
        return 1;
    }
    return 1;
}
#pragma optimization_level reset

/* The elder dragons of the habitats' panel (monster kinds), online and
 * offline, and each one's quests by season for the offline lists. */
s16 gElderMonstersOnline[6] = {
#include "lobby/007EC178.inc"
};
s16 gElderMonstersOffline[6] = {
#include "lobby/007EC188.inc"
};
u16 gElderForecastQuests[ELDER_MONSTERS][WANTED_SEASONS][ELDER_QUEST_CHOICES] = {
#include "lobby/007EC1A0.inc"
};

/* The habitats' panel's formats: the title's season, a line's elder dragon,
 * a line's place and time of day. gElderPlaceFmt's 32 bytes also hold
 * Lobby_DrawTimeLeft's format (gTimeLeftFormat) from byte 8: on its own, a
 * 24-byte object would be aligned to 16 bytes and the data after it would
 * move. */
#pragma rodata on
char gElderSeasonFmt[8] = {
#include "lobby/00838460.inc"
};
char gElderMonsterFmt[8] = {
#include "lobby/00838468.inc"
};
char gElderPlaceFmt[32] = {
#include "lobby/00838470.inc"
};
#pragma rodata off
