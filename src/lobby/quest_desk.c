/*
 * The quest desk: the village menu where quests are taken (gTownMode
 * TOWN_MODE_QUEST_DESK; its work area is gMenuWork as lobby_menu.h's
 * QuestDesk). QuestDesk_Open opens it with the entries QuestDesk_HasEntry
 * offers (the field guide, the elder's forecast, the quest board, the
 * quests by map group and by type), QuestDesk_Update runs its steps each
 * frame through the input functions (QuestDesk_TopMenu ...
 * QuestDesk_GuideInput) and QuestDesk_Close closes it. QuestDesk_Accept
 * makes a quest the taken one and QuestDesk_Drop drops it; online the
 * quest is posted to the quest room first (QuestDesk_Post).
 * QuestDesk_BuildLists fills the lists the choices show. The desk's
 * draws are QuestDesk_Draw and QuestDesk_Draw2 (with QuestBoard_Draw, the quest sheet).
 * Lobby_EnterRollRare, at the end, is the lobby entry's random roll.
 */
#include "common.h"
#include "item.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "lobby_menu.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "quest.h"
#include "area.h"
#include "pad.h"
#include "lobby_text.h"
#include "lobby_npc.h"

extern u8 gMenuWork[];
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void *memset(void *dst, s32 c, u32 n);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void QuestDesk_Draw(MenuTask *task);
void QuestDesk_Draw2(MenuTask *task);
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
s32 Equip_GunHasNoAmmo(Player *pl, SaveData *save);
s32 Menu_PushNoticeOnce(s32);
s32 Game_IsOnline(void);
s32 Cond_Check(u16 id);
void ElderForecast_Load(u16 *out, s8 b); /* unit's own declaration: declared with an s8 second parameter as QuestDesk_HasEntry passes it; with the definition's s32 the call compiles differently */
s32 QuestDesk_HasEntry(s16 k);
void QuestList_Rebuild(void);
void LobbySe_Play(s32 se);
void CamFocus_End(void);
void CamFocus_SetAside(s8 aside);
extern s8 gMenuInputLock;
extern u8 gQuestListRebuildPending;
u16 Rand_Next(s32);
extern s16 gQuestMapGroups[];
extern u16 gLobbyQuestId;
extern LobbyMenu *gLobbyMenu;
extern NetUser gNetUsers[];
void *memcpy(void *dst, void *src, u32 n);
s32 MapGroup_IsOpen(u16 id);
void HunterCard_Build(void *out);
void Money_PayPending(u8 silent);
s32 Lobby_HasQuest(void);
void Talk_Say(s32 a, s8 b);
QuestInfo *Quest_GetInfoAny(u16 id);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 se);
void LobbyText_Open(s32 id);
s32 Quest_PostStep(MsgQuestPost *req);
s32 Talk_Update(void);
s32 LobbyStep_Level2Leave(void);
s32 Quest_CheckOrderTerms(u8 id, u8 flag);
void LobbyDialog_Grow(void);
extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern s8 gRoomPassword[9];
s32 LobbyMenu_KbdEntry(void *opts);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
extern u16 gTalkLine;
extern char *gTalkLineText;
extern s8 gTalkMode;
QuestInfo *Quest_GetDiscInfo(u16 id);
s32 QuestDesk_IsWanted(u8 *p);
s16 QuestDesk_GetWantedFee(void);
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *fmt, s32 n);
char *Text_FillDollar2(s32 arg0);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);

/*
 * Opens the quest desk for the townsperson npc (an Em; the town's talk
 * handlers pass the player's talk target): marks a town menu open,
 * sets gTownMode to TOWN_MODE_QUEST_DESK, clears the menu work area, sets
 * QuestDesk_Draw and QuestDesk_Draw2 as the draws, stores the season and
 * the time of day, and pushes the no-ammo notice once when the local
 * player's gun has no ammo (Equip_GunHasNoAmmo). The desk of townsperson
 * NPC_QUEST_DESK_2 (NpcWork.id) uses the second quest list set (questSet 1), every
 * other desk the first. Then fills the entry list with the entries
 * QuestDesk_HasEntry offers (of the five) and puts the cursor on the quest
 * board (QUEST_DESK_BOARD). Returns nothing.
 */
void QuestDesk_Open(u8 *npc)
{
    QuestDesk *desk;
    NpcWork *npcWork;
    s32 i;

    gTownMenuOpen = 1;
    desk = (QuestDesk *)gMenuWork;
    npcWork = (NpcWork *)(npc + 0x438);
    gTownMode = TOWN_MODE_QUEST_DESK;
    memset(desk, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(QuestDesk_Draw, QuestDesk_Draw2, 0, 0);
    desk->season = LobbyWorld_GetSeason();
    desk->pm = LobbyTime_IsPm();
    if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], &gSave) == 1) {
        Menu_PushNoticeOnce(LOBBY_NOTICE_GUN_NO_AMMO);
    }
    switch (npcWork->id) {
    case NPC_QUEST_DESK_2:
        desk->questSet = 1;
        break;
    default:
        desk->questSet = 0;
        break;
    }
    desk->entryCount = 0;
    for (i = 0; i < QUEST_DESK_ENTRY_COUNT; i++) {
        if (QuestDesk_HasEntry(i) == 1) {
            desk->entries[desk->entryCount] = i;
            if (i == QUEST_DESK_BOARD) {
                desk->entrySel = desk->entryCount;
            }
            desk->entryCount++;
        }
    }
}

/*
 * 1 when the desk offers entry (QUEST_DESK_*), else 0: QUEST_DESK_GUIDE and
 * QUEST_DESK_BOARD always; QUEST_DESK_FORECAST once the forecast has been
 * heard (gElderForecastHeard), and offline only under the village's
 * condition 0xD (Cond_Check); it also loads the season's forecast into the
 * work area (ElderForecast_Load). QUEST_DESK_BY_MAP online or under
 * condition 7; QUEST_DESK_BY_TYPE online or under condition 0xA.
 */
s32 QuestDesk_HasEntry(s16 entry)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;

    switch (entry) {
    case QUEST_DESK_GUIDE:
        return 1;
    case QUEST_DESK_FORECAST:
        if (Game_IsOnline() == 0 && Cond_Check(0xD) == 0) {
            return 0;
        }
        if (gElderForecastHeard != 0) {
            ElderForecast_Load(desk->forecast, desk->season);
            return 1;
        }
        break;
    case QUEST_DESK_BOARD:
        return 1;
    case QUEST_DESK_BY_MAP:
        if (Game_IsOnline() == 1) {
            return 1;
        }
        if (Cond_Check(7) == 1) {
            return 1;
        }
        break;
    case QUEST_DESK_BY_TYPE:
        if (Game_IsOnline() == 1) {
            return 1;
        }
        if (Cond_Check(0xA) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * Closes the desk: clears the town menu flag and gTownMode
 * (TOWN_MODE_FREE), resets the menu and its draws, ends the camera focus,
 * locks menu input for 20 frames (gMenuInputLock), rebuilds the quest
 * lists when a rebuild is pending (gQuestListRebuildPending), plays the
 * cancel sound and gives pad control back to the field (gFieldControl 1).
 */
void QuestDesk_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_SetAside(0);
    CamFocus_End();
    gMenuInputLock = 0x14;
    if (gQuestListRebuildPending != 0) {
        QuestList_Rebuild();
        gQuestListRebuildPending = 0;
    }
    LobbySe_Play(LOBBY_SE_CANCEL);
    gFieldControl = 1;
}

/*
 * The number of the 7 quest map groups (gQuestMapGroups) that are open
 * (MapGroup_IsOpen); group 2 is not counted in AREA_TAVERN and
 * AREA_HUB_0x89, and group 0xD is never counted. The parameter is not
 * used. The top menu stores the result as the field guide's row count.
 */
s16 QuestDesk_CountOpenGroups(void *desk)
{
    s16 i;
    s16 count = 0;
    s16 *group = gQuestMapGroups;

    for (i = 0; i < QUEST_MAP_GROUP_COUNT; i++, group++) {
        if (MapGroup_IsOpen(*group) == 1) {
            if (*group == 2 && (gScene.area == AREA_TAVERN || gScene.area == AREA_HUB_0x89)) {
                continue;
            }
            if (*group != 0xD) {
                count++;
            }
        }
    }
    return count;
}

s32 QuestDesk_Greet(void);
s32 QuestDesk_TopMenu(u16 pad);
s32 QuestDesk_ChooseList(u16 pad);
s32 QuestDesk_ChooseQuest(u16 pad);
s32 QuestDesk_CheckFee(u16 pad);
s32 QuestDesk_Post(u16 pad);
s32 QuestDesk_Cancel(QuestDeskCancel *cancel, u16 pad);
s32 QuestDesk_ForecastInput(u16 pad);
s32 QuestDesk_GuideInput(u16 pad);
u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Win_Shake(WinShake *s);
void QuestDesk_SayBye(void);
void QuestDesk_BuildLists(void);
void QuestRoom_ResetSettings(void);
void QuestDesk_Close(void);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
extern s16 gQuestStampRand;

/*
 * Runs the desk once a frame by its step (QuestDesk.step, QUEST_DESK_STEP_*):
 * - START: with no quest taken (QuestDesk_Greet returns 1) the window
 *   slides in (SLIDE_IN); with a quest taken, keeps it and the wanted fee
 *   paid for it (the money taken less the quest's fee), asks whether to drop
 *   it and goes to DROP.
 * - SLIDE_IN: the window slides in, then TOP_MENU.
 * - TOP_MENU: QUEST_DESK_BY_MAP and BY_TYPE build their lists and go to
 *   CHOOSE_LIST; GUIDE, FORECAST and BOARD (which builds its list) slide the
 *   window out (SLIDE_OUT); cancel says goodbye (WAIT_TALK).
 * - CHOOSE_LIST: chosen, the window slides out; cancel back to TOP_MENU;
 *   refused, goodbye.
 * - SLIDE_OUT: once the window is out, the lists and the board go to
 *   CHOOSE_QUEST, the guide to GUIDE, the forecast to FORECAST.
 * - CHOOSE_QUEST: chosen, a new stamp random (gQuestStampRand), CHECK_FEE;
 *   cancel, the window slides back (SLIDE_BACK).
 * - CHECK_FEE: passed offline, the wanted stamp shakes first for a wanted
 *   quest (WANTED_STAMP), else the stamp (STAMP); passed online, the row is
 *   kept (savedSel), the room settings are reset (QuestRoom_ResetSettings)
 *   and the posting runs (POST); cancel, SLIDE_BACK.
 * - POST: posted, the stamps as above and the row restored; failed, CLOSE.
 * - WANTED_STAMP: the wanted stamp shakes with field control off, then
 *   STAMP.
 * - STAMP: after the stamp, the accepted line (another online when a
 *   minimum of members is set) and QuestDesk_Accept with the wanted fee,
 *   paid; then WAIT_TALK.
 * - WAIT_TALK: waits for the talk to end, then CLOSE.
 * - CLOSE: QuestDesk_Close.
 * - DROP: the drop talk (QuestDesk_Cancel): dropped, CLOSE; kept, goodbye.
 * - GUIDE (QuestDesk_GuideInput) and FORECAST (QuestDesk_ForecastInput):
 *   on back, SLIDE_BACK.
 * - SLIDE_BACK: the window slides back in, then CHOOSE_LIST for the lists,
 *   else TOP_MENU.
 */
void QuestDesk_Update(void)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    u16 pad;
    s16 entry;

    pad = LobbyPad_Get(0);
    Chat_SetWindowMode(1, 2);
    switch (desk->step) {
    case QUEST_DESK_STEP_START:
        if (QuestDesk_Greet() == 1) {
            desk->step++;
            WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WIN_X);
            CamFocus_SetAside(1);
            LobbySe_Play(LOBBY_SE_WINDOW_IN);
            break;
        }
        desk->step = QUEST_DESK_STEP_DROP;
        desk->quest = Quest_GetInfoAny(gLobbyQuestId);
        desk->wantedFee = gRewardMoney - desk->quest->fee;
        Talk_Say(QUEST_DESK_TALK_ASK_DROP, 1);
        Talk_Update();
        CamFocus_SetAside(1);
        desk->hidePageNum = 1;
        LobbySe_Play(LOBBY_SE_WINDOW_OUT);
        break;
    case QUEST_DESK_STEP_SLIDE_IN:
        switch (WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WIN_X)) {
        case MENU_DONE:
            desk->step++;
            break;
        }
        break;
    case QUEST_DESK_STEP_TOP_MENU:
        switch (QuestDesk_TopMenu(pad)) {
        case MENU_DONE:
            entry = desk->entries[desk->entrySel];
            switch (entry) {
            case QUEST_DESK_BY_TYPE:
            case QUEST_DESK_BY_MAP:
                desk->step++;
                desk->list = 0;
                desk->sel = 0;
                QuestDesk_BuildLists();
                break;
            case QUEST_DESK_BOARD:
                QuestDesk_BuildLists();
            case QUEST_DESK_GUIDE:
            case QUEST_DESK_FORECAST:
                desk->step = QUEST_DESK_STEP_SLIDE_OUT;
                desk->sel = 0;
                WinSlide_Step(&desk->win, QUEST_DESK_WIN_X, QUEST_DESK_WIN_X_OUT);
                break;
            }
            break;
        case MENU_CANCEL:
            desk->step = QUEST_DESK_STEP_WAIT_TALK;
            QuestDesk_SayBye();
            break;
        }
        break;
    case QUEST_DESK_STEP_CHOOSE_LIST:
        switch (QuestDesk_ChooseList(pad)) {
        case MENU_DONE:
            desk->step++;
            desk->sel = 0;
            WinSlide_Step(&desk->win, QUEST_DESK_WIN_X, QUEST_DESK_WIN_X_OUT);
            break;
        case MENU_CANCEL:
            desk->step = QUEST_DESK_STEP_TOP_MENU;
            break;
        case MENU_UNDONE:
            desk->step = QUEST_DESK_STEP_WAIT_TALK;
            QuestDesk_SayBye();
            break;
        }
        break;
    case QUEST_DESK_STEP_SLIDE_OUT:
        switch (WinSlide_Step(&desk->win, QUEST_DESK_WIN_X, QUEST_DESK_WIN_X_OUT)) {
        case MENU_DONE:
            switch (desk->entries[desk->entrySel]) {
            case QUEST_DESK_BY_TYPE:
            case QUEST_DESK_BY_MAP:
                desk->step++;
                break;
            case QUEST_DESK_GUIDE:
                desk->step = QUEST_DESK_STEP_GUIDE;
                break;
            case QUEST_DESK_FORECAST:
                desk->step = QUEST_DESK_STEP_FORECAST;
                break;
            case QUEST_DESK_BOARD:
                desk->step++;
                break;
            }
            break;
        }
        break;
    case QUEST_DESK_STEP_CHOOSE_QUEST:
        switch (QuestDesk_ChooseQuest(pad)) {
        case MENU_DONE:
            desk->step++;
            gQuestStampRand = Rand_Next(1);
            break;
        case MENU_CANCEL:
            desk->step = QUEST_DESK_STEP_SLIDE_BACK;
            WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WIN_X);
            break;
        }
        break;
    case QUEST_DESK_STEP_CHECK_FEE:
        switch (QuestDesk_CheckFee(pad)) {
        case MENU_DONE:
            if (Game_IsOnline() == 0) {
                if (QuestDesk_IsWanted((u8 *) desk->quest) == 1) {
                    Win_Shake(&desk->wantedStamp);
                    desk->step = QUEST_DESK_STEP_WANTED_STAMP;
                } else {
                    Win_Shake(&desk->stamp);
                    desk->step = QUEST_DESK_STEP_STAMP;
                }
                break;
            }
            desk->step++;
            desk->savedSel = desk->sel;
            desk->sel = 0;
            QuestRoom_ResetSettings();
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            break;
        case MENU_CANCEL:
            desk->step = QUEST_DESK_STEP_SLIDE_BACK;
            WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WIN_X);
            break;
        }
        break;
    case QUEST_DESK_STEP_POST:
        switch (QuestDesk_Post(pad)) {
        case MENU_DONE:
            if (QuestDesk_IsWanted((u8 *) desk->quest) == 1) {
                desk->step++;
                Win_Shake(&desk->wantedStamp);
            } else {
                desk->step = QUEST_DESK_STEP_STAMP;
                Win_Shake(&desk->stamp);
            }
            desk->sel = desk->savedSel;
            break;
        case MENU_UNDONE:
            desk->step = QUEST_DESK_STEP_CLOSE;
            break;
        }
        break;
    case QUEST_DESK_STEP_WANTED_STAMP:
        gFieldControl = 0;
        switch (Win_Shake(&desk->wantedStamp)) {
        case MENU_DONE:
            desk->step++;
            Win_Shake(&desk->stamp);
            gFieldControl = 1;
            break;
        }
        break;
    case QUEST_DESK_STEP_STAMP:
        switch (Win_Shake(&desk->stamp)) {
        case MENU_DONE:
            desk->step++;
            if (Game_IsOnline() == 0) {
                Talk_Say(QUEST_DESK_TALK_ACCEPTED, 0);
            } else if (gQuestMinMembers == 0) {
                Talk_Say(QUEST_DESK_TALK_ACCEPTED, 0);
            } else {
                Talk_Say(QUEST_DESK_TALK_ACCEPTED_MIN_MEMBERS, 0);
            }
            QuestDesk_Accept(desk->quest->id, desk->wantedFee, 1);
            break;
        }
        break;
    case QUEST_DESK_STEP_WAIT_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
            desk->step = QUEST_DESK_STEP_CLOSE;
            break;
        }
        break;
    case QUEST_DESK_STEP_CLOSE:
        QuestDesk_Close();
        break;
    case QUEST_DESK_STEP_DROP:
        switch (QuestDesk_Cancel(&desk->cancel, pad)) {
        case MENU_DONE:
            desk->step = QUEST_DESK_STEP_CLOSE;
            CamFocus_SetAside(0);
            break;
        case MENU_CANCEL:
            QuestDesk_SayBye();
            desk->step = QUEST_DESK_STEP_WAIT_TALK;
            break;
        }
        break;
    case QUEST_DESK_STEP_GUIDE:
        switch (QuestDesk_GuideInput(pad)) {
        case MENU_DONE:
            desk->step = QUEST_DESK_STEP_SLIDE_BACK;
            WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WIN_X);
            break;
        }
        break;
    case QUEST_DESK_STEP_FORECAST:
        switch (QuestDesk_ForecastInput(pad)) {
        case MENU_DONE:
            desk->step = QUEST_DESK_STEP_SLIDE_BACK;
            WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WIN_X);
            break;
        }
        break;
    case QUEST_DESK_STEP_SLIDE_BACK:
        switch (WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WIN_X)) {
        case MENU_DONE:
            switch (desk->entries[desk->entrySel]) {
            case QUEST_DESK_BY_TYPE:
            case QUEST_DESK_BY_MAP:
                desk->step = QUEST_DESK_STEP_CHOOSE_LIST;
                break;
            default:
                desk->step = QUEST_DESK_STEP_TOP_MENU;
                break;
            }
            break;
        }
        break;
    }
}

extern u8 gQuestIsOfficial;
extern u8 gMemberNamesOff;
void Quest_ClearVersusDefeat(void);
void Quest_SetInfo(QuestInfo *q);
void NetQuest_SetStage(u16 id);
s32 QuestFlags_GetCategory(u32 *arg0);
void Save_AddMoney(s32 n);
s32 HunterCard_SendChanged(void);
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeasonDay(void);

/*
 * Makes quest id the taken quest: clears the versus defeat, sets the money
 * taken (gRewardMoney) to money, keeps the area to return to online,
 * clears the official flag, sets gLobbyQuestId, the lobby's taken flag,
 * the quest info, the network stage and the scene's quest id, and plays
 * the money sound. With pay set the quest's fee is added to the money
 * taken, the quest is marked paid (lobby and scene) and the local player's
 * quest status is the quest's category with 0x80; without pay the category
 * with 0x40. The money taken (when nonzero) leaves gSave.money; the hunter
 * cards (the lobby member's and the net user's) are rebuilt; gNextQuest
 * gets the quest's flags and map. Online it sends the changed card, shows
 * member names again and takes a nonzero ready argument from the lobby
 * work buffer (gQuestQNetReadyArg); offline it clears the quest offer and
 * records the season, the time of day and the season day of the
 * acceptance. Returns nothing.
 */
void QuestDesk_Accept(u16 id, s16 money, s32 pay)
{
    QuestInfo *quest = Quest_GetInfoAny(id);
    s32 flags;

    Quest_ClearVersusDefeat();
    gRewardMoney = money;
    if (Game_IsOnline() == 1) {
        gQuestReturnArea = gScene.area;
    }
    gQuestIsOfficial = 0;
    gLobbyQuestId = id;
    gLobby->questTaken = 1;
    Quest_SetInfo(quest);
    NetQuest_SetStage(id);
    gScene.questId = id;
    LobbySe_Play(LOBBY_SE_MONEY);
    flags = QuestFlags_GetCategory((u32 *) quest);
    if ((u8)pay) {
        gRewardMoney += quest->fee;
        gLobby->questPaid = 1;
        gScene.questPaid = gLobby->questPaid;
        gPlayers[gScene.localPlayer].questStatus = flags | QUEST_STATUS_PAID;
    } else {
        gPlayers[gScene.localPlayer].questStatus = flags | QUEST_STATUS_TAKEN;
    }
    if (gRewardMoney != 0) {
        Save_AddMoney(-gRewardMoney);
    }
    HunterCard_Build(gLobby->members[gScene.localPlayer].text);
    HunterCard_Build(gNetUsers[gScene.localPlayer].text);
    gNextQuest.flags = quest->flags;
    gNextQuest.map = quest->map;
    if (Game_IsOnline() == 1) {
        HunterCard_SendChanged();
        gMemberNamesOff = 0;
        if (*(u16 *)(gLobbyWorkBuf + LOBBY_WORK_QNET_READY_ARG) != 0) {
            gQuestQNetReadyArg = *(u16 *)(gLobbyWorkBuf + LOBBY_WORK_QNET_READY_ARG);
        }
    } else {
        gQuestOfferId = 0;
        gLobbyLastSeason = LobbyWorld_GetSeason();
        gLobbyLastPm = LobbyTime_IsPm();
        gQuestAcceptDay = LobbyWorld_GetSeasonDay();
    }
}

/*
 * Drops the taken quest: clears the lobby's taken flag and the local
 * player's quest status, rebuilds the member's hunter card and copies it
 * to the net user's, clears the quest offer, hands the money taken back
 * through Money_PayPending when it is above 0 (else plays the cancel
 * sound) and clears the paid flag. Returns nothing.
 */
void QuestDesk_Drop(void)
{
    gLobby->questTaken = 0;
    gPlayers[gScene.localPlayer].questStatus = 0;
    HunterCard_Build(gLobby->members[gScene.localPlayer].text);
    memcpy(gNetUsers[gScene.localPlayer].text, gLobby->members[gScene.localPlayer].text, sizeof(gNetUsers[0].text));
    gQuestOfferId = 0;
    if (gRewardMoney > 0) {
        Money_PayPending(0);
    } else {
        LobbySe_Play(LOBBY_SE_CANCEL);
    }
    gLobby->questPaid = 0;
}

/*
 * With no quest taken (Lobby_HasQuest 0) says
 * the greeting line and returns 1; else keeps the taken quest
 * (Quest_GetInfoAny of gLobbyQuestId), asks whether to drop it and returns
 * 0.
 */
s32 QuestDesk_Greet(void)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;

    if (Lobby_HasQuest() == 0) {
        Talk_Say(QUEST_DESK_TALK_GREETING, 0);
        return 1;
    }
    desk->quest = Quest_GetInfoAny(gLobbyQuestId);
    Talk_Say(QUEST_DESK_TALK_ASK_DROP, 1);
    return 0;
}

/*
 * The top menu's input. Circle on: the field guide, refused when no map
 * group is open (the open count kept as the guide's row count); the quest
 * board, refused when the hub's shown quest list is empty
 * (gQuestDeskShown[1] in AREA_ELDER_HALL, gQuestDeskShown[0] elsewhere); the
 * forecast, always taken; entries 3 and 4, taken with the confirm sound.
 * Taken returns MENU_DONE. Cross returns MENU_CANCEL with the cancel
 * sound; else the cursor moves up and down over the entries. MENU_RUNNING
 * otherwise.
 */
s32 QuestDesk_TopMenu(u16 pad)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;

    if (pad & PAD_CIRCLE) {
        switch (desk->entries[desk->entrySel]) {
        case QUEST_DESK_GUIDE:
            desk->groupCount = QuestDesk_CountOpenGroups(desk);
            if (desk->groupCount == 0) {
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else {
                LobbySe_Play(LOBBY_SE_WINDOW_OUT);
                return MENU_DONE;
            }
            break;
        case QUEST_DESK_BOARD:
            switch (gScene.area) {
            case AREA_ELDER_HALL:
                if (gQuestDeskShown[1][0] == 0) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                } else {
                    LobbySe_Play(LOBBY_SE_WINDOW_OUT);
                    return MENU_DONE;
                }
                break;
            default:
                if (gQuestDeskShown[0][0] == 0) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                } else {
                    LobbySe_Play(LOBBY_SE_WINDOW_OUT);
                    return MENU_DONE;
                }
                break;
            }
            break;
        case QUEST_DESK_FORECAST:
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            return MENU_DONE;
        default:
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else {
        desk->entrySel = Menu_WrapUpDown(desk->entrySel, desk->entryCount);
    }
    return MENU_RUNNING;
}

/*
 * The list choice (entries 3 and 4), by its sub step. 0: circle on an
 * empty list says the empty-list line (sub step 2), else the list-chosen
 * line (1); cross returns MENU_CANCEL; else the list cursor moves over
 * listCount lists. 1: when the talk ends, MENU_DONE. 2: when the talk ends
 * back to 0; answered no, returns 1 (the desk says goodbye). MENU_RUNNING
 * otherwise.
 */
s32 QuestDesk_ChooseList(u16 pad)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;

    switch (desk->subStep) {
    case 0:
        if (pad & PAD_CIRCLE) {
            if (desk->lists[desk->list][0] == 0) {
                desk->subStep = 2;
                Talk_Say(QUEST_DESK_TALK_LIST_EMPTY, 0);
            } else {
                desk->subStep++;
                Talk_Say(QUEST_DESK_TALK_LIST_CHOSEN, 0);
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            desk->list = Menu_WrapUpDown(desk->list, desk->listCount);
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case MENU_DONE:
            desk->subStep = 0;
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            return MENU_DONE;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case MENU_DONE:
            desk->subStep = 0;
            break;
        case MENU_CANCEL:
            desk->subStep = 0;
            return MENU_UNDONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The quest choice in the current list, by its sub step. 0: counts the
 * list's quests (at most 12) and shows the page number. Circle takes the
 * quest under the cursor (Quest_GetDiscInfo; gLobbyQuestId): a wanted
 * quest with a fee says the wanted fee line (the wanted fee,
 * QuestDesk_GetWantedFee, and the fee filled in) and slides the wanted
 * panel in (sub step 1); another quest with a fee says the fee line, one
 * without a fee the no-fee line (sub step 2); the page number is hidden.
 * Cross clears the quest and returns MENU_CANCEL. Else the cursor moves up
 * and down, the sheet's page left and right (3 pages). 1: the panel slides
 * in, then 2. 2: the talk's answer: yes on a wanted quest slides the panel
 * out (3); yes on another returns MENU_DONE; no on a wanted quest slides
 * the panel out (3), on another says the declined line (4). 3: once the
 * panel is out, the declined line (4) when no was chosen (gTalkMode 1: no chosen in the yes/no box),
 * else MENU_DONE. 4: when the talk ends the quest is cleared and the
 * choice starts again. MENU_RUNNING otherwise.
 */
s32 QuestDesk_ChooseQuest(u16 pad)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    char *line;
    u16 count;

    switch (desk->subStep) {
    case 0:
        count = 0;
        while (desk->lists[desk->list][count] != 0) {
            count++;
            if (count >= QUEST_LIST_LEN) {
                break;
            }
        }
        desk->hidePageNum = 0;
        if (pad & PAD_CIRCLE) {
            desk->wantedFee = 0;
            desk->quest = Quest_GetDiscInfo(desk->lists[desk->list][desk->sel]);
            gLobbyQuestId = desk->quest->id;
            if (desk->quest->fee != 0) {
                if (QuestDesk_IsWanted((u8 *) desk->quest) == 1) {
                    Talk_Say(QUEST_DESK_TALK_WANTED_FEE, 0);
                    line = Talk_LineText(gTalkLine);
                    desk->wantedFee = QuestDesk_GetWantedFee();
                    Text_FillDollar(line, desk->wantedFee);
                    gTalkLineText = Text_FillDollar2(desk->quest->fee);
                    WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WANTED_X);
                    desk->subStep++;
                    LobbySe_Play(LOBBY_SE_WINDOW_OUT);
                } else {
                    Talk_Say(QUEST_DESK_TALK_FEE, 0);
                    gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), desk->quest->fee);
                    desk->subStep = 2;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
            } else {
                Talk_Say(QUEST_DESK_TALK_NO_FEE, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), desk->quest->fee);
                desk->subStep = 2;
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
            desk->hidePageNum = 1;
        } else if (pad & PAD_CROSS) {
            desk->quest = NULL;
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            return MENU_CANCEL;
        } else {
            desk->sel = Menu_WrapUpDownSe(desk->sel, count, LOBBY_SE_WINDOW_OUT);
            desk->page = Menu_WrapLeftRight(desk->page, QUEST_SHEET_PAGES);
        }
        break;
    case 1:
        switch (WinSlide_Step(&desk->win, QUEST_DESK_WIN_X_OUT, QUEST_DESK_WANTED_X)) {
        case MENU_DONE:
            desk->subStep++;
            break;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case MENU_DONE:
            if (QuestDesk_IsWanted((u8 *) desk->quest) == 1) {
                desk->subStep++;
                WinSlide_Step(&desk->win, QUEST_DESK_WANTED_X, QUEST_DESK_WIN_X_OUT);
                LobbySe_Play(LOBBY_SE_LIST_ENTRY);
            } else {
                desk->subStep = 0;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
            break;
        case MENU_CANCEL:
            if (QuestDesk_IsWanted((u8 *) desk->quest) == 1) {
                desk->subStep++;
                WinSlide_Step(&desk->win, QUEST_DESK_WANTED_X, QUEST_DESK_WIN_X_OUT);
            } else {
                Talk_Say(QUEST_DESK_TALK_DECLINED, 0);
                desk->subStep = 4;
            }
            break;
        }
        break;
    case 3:
        switch (WinSlide_Step(&desk->win, QUEST_DESK_WANTED_X, QUEST_DESK_WIN_X_OUT)) {
        case MENU_DONE:
            if (gTalkMode == 1) {
                desk->subStep++;
                Talk_Say(QUEST_DESK_TALK_DECLINED, 0);
            } else {
                desk->subStep = 0;
                return MENU_DONE;
            }
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            desk->subStep = 0;
            desk->quest = NULL;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Checks the chosen quest, by its sub step. 0: when the quest's order
 * terms are not met (Quest_CheckOrderTerms) opens the terms message (sub
 * step 2); when the fee plus the wanted fee is more than gSave.money says
 * the not-enough-money line (1); else returns MENU_DONE. 1: when the talk
 * ends the quest is cleared and MENU_CANCEL returned. 2: keeps the message
 * drawn; circle clears the quest and returns MENU_CANCEL. MENU_RUNNING
 * otherwise.
 */
s32 QuestDesk_CheckFee(u16 pad)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;

    switch (desk->subStep) {
    case 0:
        if (Quest_CheckOrderTerms(desk->quest->terms, 0) == 0) {
            desk->subStep = 2;
            LobbyText_Open(LOBBY_TEXT_ORDER_TERMS);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
        } else if (desk->quest->fee + desk->wantedFee > gSave.money) {
            desk->subStep++;
            Talk_Say(QUEST_DESK_TALK_NO_MONEY, 0);
            Talk_Update();
        } else {
            return MENU_DONE;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case MENU_DONE:
            desk->subStep = 0;
            desk->quest = NULL;
            return MENU_CANCEL;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            desk->subStep = 0;
            desk->quest = NULL;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_CANCEL;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Online: the quest room's settings before posting, rows 0 to 3 by the
 * cursor. 0: circle on row 3 (post) returns 1, unless the password is on
 * and empty (the empty-password notice); circle on row 2 opens the
 * keyboard for the password (sub step 1, field control off). Left or right
 * on row 1 toggles the password (clearing it when turned off), on row 0
 * sets the minimum members (0..3). Up and down move the cursor; row 2 is
 * skipped while the password is off. 1: the keyboard runs; when it closes,
 * back to 0 with field control on. Returns 0 except when posting.
 */
s32 QuestDesk_RoomSettings(u16 pad)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    s32 oldSel;

    switch (desk->roomStep) {
    case 0:
        if (pad & PAD_CIRCLE) {
            if (desk->sel == ROOM_SETTING_DECIDE) {
                if (gRoomPassOn != 0 && gRoomPassword[0] == 0) {
                    Menu_PushNoticeOnce(LOBBY_NOTICE_PASSWORD_EMPTY);
                } else {
                    desk->roomStep = 0;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    return 1;
                }
            } else if (desk->sel == ROOM_SETTING_PASSWORD) {
                desk->roomStep++;
                gFieldControl = 0;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                LobbyMenu_Init();
                LobbyMenu_SetDraw(QuestDesk_Draw, QuestDesk_Draw2, 0, 0);
                LobbyMenu_KbdEntry(gRoomPassword);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (desk->sel == ROOM_SETTING_PASS_ON) {
                gRoomPassOn ^= 1;
                if (gRoomPassOn == 0) {
                    memset(gRoomPassword, 0, sizeof(gRoomPassword));
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            } else if (desk->sel == ROOM_SETTING_MEMBERS) {
                gQuestMinMembers = Menu_WrapLeftRight(gQuestMinMembers, ROOM_MIN_MEMBERS_CHOICES);
            }
        } else {
            oldSel = desk->sel;
            desk->sel = Menu_WrapUpDown(oldSel, ROOM_SETTINGS);
            if ((s8)oldSel != desk->sel && desk->sel == ROOM_SETTING_PASSWORD && gRoomPassOn == 0) {
                desk->sel += desk->sel - (s8)oldSel;
            }
        }
        break;
    case 1:
        if (LobbyMenu_KbdEntry(gRoomPassword) == 1) {
            desk->roomStep = 0;
            LobbyMenu_Init();
            gFieldControl = 1;
        }
        break;
    }
    return 0;
}

/*
 * Online: the posting, by its sub step. 0: the room settings
 * (QuestDesk_RoomSettings); when they finish opens the sending message and
 * fills the posting (the quest id, the wanted fee, QUEST_POST_DESK_QUEST, the area), then
 * 1. 1: keeps the message drawn and runs Quest_PostStep: posted returns 0
 * (MENU_DONE); failed clears the quest and returns MENU_UNDONE. Field
 * control stays off on every frame. MENU_RUNNING otherwise. gFieldControl
 * is stored through an s8 pointer here: the plain store of the u8 it is
 * declared as compiles differently.
 */
s32 QuestDesk_Post(u16 pad)
{
    QuestDesk *deskWork = (QuestDesk *)gMenuWork;

    switch (deskWork->subStep) {
    case 0:
        if (QuestDesk_RoomSettings(pad) == 1) {
            deskWork->subStep++;
            LobbyText_Open(LOBBY_TEXT_SENDING);
            deskWork->head.req.quest = deskWork->quest->id;
            deskWork->head.req.money = deskWork->wantedFee;
            deskWork->head.req.desk = QUEST_POST_DESK_QUEST;
            deskWork->head.req.area = gScene.area;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (Quest_PostStep(&deskWork->head.req)) {
        case 0:
            deskWork->subStep = 0;
            *(s8 *)&gFieldControl = 0;
            return MENU_DONE;
        case 1:
            deskWork->subStep = 0;
            deskWork->quest = NULL;
            *(s8 *)&gFieldControl = 0;
            return MENU_UNDONE;
        }
        break;
    }
    *(s8 *)&gFieldControl = 0;
    return MENU_RUNNING;
}

/*
 * The drop talk for a taken quest, by cancel->step; cancel->hideSheet set to
 * 1 hides the quest sheet. 0: on yes, a paid quest online asks again (1); offline
 * the dropped line is said and the sheet hidden (3); online unpaid opens
 * the room-leaving message (2). On no the sheet is hidden and MENU_CANCEL
 * returned. 1: yes opens the room-leaving message (2); no returns
 * MENU_CANCEL. 2: keeps the message drawn until the room is left
 * (LobbyStep_Level2Leave), then the dropped line, the sheet hidden (3). 3:
 * when the talk ends drops the quest (QuestDesk_Drop) and returns
 * MENU_DONE. MENU_RUNNING otherwise.
 */
s32 QuestDesk_Cancel(QuestDeskCancel *cancel, u16 pad)
{
    switch (cancel->step) {
    case 0:
        switch (Talk_Update()) {
        case MENU_DONE:
            if (gLobby->questPaid != 0 && Game_IsOnline() == 1) {
                cancel->step++;
                Talk_Say(QUEST_DESK_TALK_ASK_DROP_PAID, 1);
                break;
            }
            if (Game_IsOnline() == 0) {
                cancel->step = 3;
                Talk_Say(QUEST_DESK_TALK_DROPPED, 0);
                CamFocus_SetAside(0);
                cancel->hideSheet = 1;
            } else {
                cancel->step = 2;
                LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
            }
            break;
        case MENU_CANCEL:
            cancel->step = 0;
            CamFocus_SetAside(0);
            cancel->hideSheet = 1;
            return MENU_CANCEL;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case MENU_DONE:
            cancel->step++;
            LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
            break;
        case MENU_CANCEL:
            cancel->step = 0;
            return MENU_CANCEL;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_Level2Leave() == 1) {
            Talk_Say(QUEST_DESK_TALK_DROPPED, 0);
            cancel->hideSheet = 1;
            cancel->step++;
            CamFocus_SetAside(0);
        }
        break;
    case 3:
        switch (Talk_Update()) {
        case MENU_DONE:
            cancel->step = 0;
            QuestDesk_Drop();
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The forecast page's input: cross
 * returns MENU_DONE with the window-out sound, else MENU_RUNNING.
 */
s32 QuestDesk_ForecastInput(u16 pad)
{
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_WINDOW_OUT);
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * The field guide's input: cross returns
 * MENU_DONE with the window-out sound; else the cursor moves up and down
 * over the open map groups (groupCount) and MENU_RUNNING is returned.
 */
s32 QuestDesk_GuideInput(u16 pad)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;

    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_WINDOW_OUT);
        return MENU_DONE;
    }
    desk->sel = Menu_WrapUpDownSe(desk->sel, desk->groupCount, LOBBY_SE_WINDOW_OUT);
    return MENU_RUNNING;
}

u16 Map_Group(u16 map);
s32 QuestDesk_CountMapLists(void);
s32 Save_TestEventFlag(s32 flag);
s64 abs(s32);
extern u16 gLobbyEnterRand;
extern u16 gQuestLists[2][60];

/* Says the desk's goodbye line and turns the camera back from the side. */
void QuestDesk_SayBye(void)
{
    Talk_Say(QUEST_DESK_TALK_BYE, 0);
    CamFocus_SetAside(0);
}

/*
 * Fills the desk's quest lists (QuestDesk.lists) for the entry at the
 * cursor, from the desk's quest list set (gQuestLists[questSet]), at most
 * QUEST_LIST_LEN quests a list:
 * - QUEST_DESK_BY_TYPE: one list per quest type 1 to 6 (QuestInfo.type);
 *   listCount 5.
 * - QUEST_DESK_BY_MAP: one list per map group of gQuestMapGroups
 *   (Map_Group of the quest's map); listCount as the story allows
 *   (QuestDesk_CountMapLists).
 * - QUEST_DESK_BOARD: the hub's shown quests (gQuestDeskShown[1] in
 *   AREA_ELDER_HALL, [0] elsewhere) as list 0.
 */
void QuestDesk_BuildLists(void)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    s32 n;
    s32 i;
    u16 *id;
    s32 group;

    memset(desk->lists, 0, sizeof(desk->lists));
    switch (desk->entries[desk->entrySel]) {
    case QUEST_DESK_BY_TYPE:
        for (i = 1; i < 7; i++) {
            n = 0;
            for (id = gQuestLists[desk->questSet]; *id != 0; id++) {
                if (Quest_GetDiscInfo(*id)->type == i) {
                    desk->lists[i - 1][n++] = *id;
                    if (n >= QUEST_LIST_LEN) {
                        break;
                    }
                }
            }
        }
        desk->listCount = 5;
        break;
    case QUEST_DESK_BY_MAP:
        for (i = 0; i < QUEST_MAP_GROUP_COUNT; i++) {
            group = gQuestMapGroups[i];
            n = 0;
            for (id = gQuestLists[desk->questSet]; *id != 0; id++) {
                if (group == Map_Group(Quest_GetDiscInfo(*id)->map)) {
                    desk->lists[i][n++] = *id;
                    if (n >= QUEST_LIST_LEN) {
                        break;
                    }
                }
            }
        }
        desk->listCount = QuestDesk_CountMapLists();
        break;
    case QUEST_DESK_BOARD:
        desk->list = 0;
        switch (gScene.area) {
        case AREA_ELDER_HALL:
            memcpy(desk->lists[desk->list], gQuestDeskShown[1], sizeof(desk->lists[0]));
            break;
        default:
            memcpy(desk->lists[desk->list], gQuestDeskShown[0], sizeof(desk->lists[0]));
            break;
        }
        break;
    case QUEST_DESK_GUIDE:
        break;
    }
}

/*
 * The number of map-group lists the by-map entry offers (QuestDesk.listCount),
 * by the story's progress. Offline by the village's conditions (Cond_Check):
 * 1 at first, 2 to 6 as conditions 7, 0xA, 0x10, 0x15 and 0x1A hold, and
 * under condition 0x4F 6, or 7 in AREA_HUB_0x57. Online 5, 6 under
 * condition 0x60, 7 with EVENT_FLAG_SONG_TRACK_3.
 */
s32 QuestDesk_CountMapLists(void)
{
    if (Game_IsOnline() == 0) {
        if (Cond_Check(0x4F) == 1) {
            return gScene.area == AREA_HUB_0x57 ? 7 : 6;
        }
        if (Cond_Check(0x1A) == 1) {
            return 6;
        }
        if (Cond_Check(0x15) == 1) {
            return 5;
        }
        if (Cond_Check(0x10) == 1) {
            return 4;
        }
        if (Cond_Check(0xA) == 1) {
            return 3;
        }
        return Cond_Check(7) == 1 ? 2 : 1;
    } else {
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 1) {
            return 7;
        }
        if (Cond_Check(0x60) == 1) {
            return 6;
        }
        return 5;
    }
}

/*
 * 1 when the roll made on entering the lobby (gLobbyEnterRand, mod 100) is
 * below 11, else 0: an 11 in 100 chance, which the town's status message
 * sends.
 */
s32 Lobby_EnterRollRare(void)
{
    return (s16)abs((s32)gLobbyEnterRand % 100) < 11;
}
