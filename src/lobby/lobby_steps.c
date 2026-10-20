/*
 * The lobby's request steps (LobbyStep_*): each sends one request or runs one task,
 * with its reply handler (LobbyStep_*Done), and the sending of the player's hunter
 * card.
 * - A step is called once a frame by a screen. It runs on one of LobbyState's step
 *   fields (seqStep, subStep, subStep2 or taskStep): it sends the request, sets
 *   LobbyState.awaitReply to the request's tag and starts the reply timer
 *   (LobbyMenu_StartTimer), then ticks it until the reply handler moves the step on,
 *   or sets LOBBY_SEQ_FAILED. It returns LOBBY_STEP_BUSY while busy, then
 *   LOBBY_STEP_DONE or LOBBY_STEP_FAILED. A handler does nothing while the lobby
 *   is being left (LOBBY_PHASE_EXIT) or when another request's reply is awaited.
 * - The login sends the hunter card, the chat colour mode and the last quest's
 *   record, and fetches the login information, the character select screen's text,
 *   the town's clock and the town data (6884), which Lobby_ApplyTownData turns into
 *   the town event, the monster rate values and the elder dragon raid's two world
 *   events.
 * - The second level (level 1: the town and its rooms): its list into
 *   gLobbyLevel1Entries, entering an entry and making its users the members
 *   (LobbyState.members, gNetUsers), opening one for recruiting, its settings and
 *   colour, leaving it. The area and group boards call these.
 * - The quest downloads: the files the server offers, kept against the saved
 *   records in the lobby work buffer (LobbyStep_DownloadQuests), and one quest file
 *   into the stage file buffer (LobbyStep_Download).
 * - The third level (level 2): an entry's records into gLobbyRecs and its users,
 *   for the quest counter (LobbyStep_Level2Entry).
 * - The hunter card's updates (HunterCard_*): the town sends the card, a part of
 *   it, or the chat colour when they change.
 */
#include "common.h"
#include "lobby_menu.h"
#include "lobby_net.h"
#include "lobby_state.h"
#include "quest.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "lobby_labels.h"

s32 LobbyNet_GetErrText(s8 *);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 HunterCard_Sanitize(void *);
s32 LobbyReq_QuestRecord(void *src, s32 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_LoginInfo(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_CharaSelectText(char *dst, s32 arg1, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_HunterCard(void *src, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_ChatColorMode(u8 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetLoginInfo(struct NetLoginInfo *dst);
s32 LobbyNet_GetCharaSelectText(s32 *dst);
void LobbyStep_HunterCardDone(NetMsg m, NetMsg *pm);
void LobbyStep_LoginInfoDone(NetMsg m, NetMsg *pm);
void LobbyStep_CharaSelectTextDone(NetMsg m, NetMsg *pm);
void LobbyStep_ChatColorModeDone(NetMsg m, NetMsg *pm);
void LobbyStep_QuestRecordDone(NetMsg m, NetMsg *pm);
extern QuestRecord gQuestRecord;
s32 LobbyReq_TownData(u8 i, u8 *dst, s32 max, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_TownClock(void (*onReply)(NetMsg, NetMsg *));
void Lobby_ApplyTownData(void);
void LobbyStep_TownDataDone(NetMsg m, NetMsg *pm);
void LobbyStep_TownClockDone(NetMsg m, NetMsg *pm);
void LobbyStep_Level1ListDone(NetMsg m, NetMsg *pm);
void LobbyStep_Level1AllowedDone(NetMsg m, NetMsg *pm);
s32 LobbyNet_GetTownData(u8 *);
s32 LobbyNet_GetTownClock(NetTownClock *dst);
void Season_Set(s32 a);
u32 LobbyWorld_TicksToSeconds(u32 t);
void LobbyTime_SetClock(u32 time);
extern u32 gTownYear;
extern u32 gTownDay;
extern u32 gTownSeason;
extern u32 gTownSeasonDays[3];
extern u32 gTownDayTicks;
extern s32 gLobbyTime;
s32 LobbyNet_GetListReplies(u16 *);
s32 LobbyLevel_SelId(s32 level);
s32 LobbyReq_Level1Enter(u16 id, char *pass, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level1Users(u16 id, void (*onReply)(NetMsg, NetMsg *));
void Lobby_SetPlace(s32 a);
void LobbyDialog_OpenWait(void *text);
extern u8 gLobbyLevel1Sel;
void LobbyStep_Level1EnterDone(NetMsg m, NetMsg *pm);
void LobbyStep_Level1UsersDone(NetMsg m, NetMsg *pm);
void LobbyStep_FetchLevel1UsersDone(NetMsg m, NetMsg *pm);
s32 LobbyNet_GetUser(u16, char *, char *, u8 *);
s32 memcmp(void *a, void *b, s32 n);
extern NetUser gNetUsers[10];
extern u8 gMyUserId[];                   /* the player's id (sub_main) */
void HunterCard_Apply(s8 idx, void *src);
void TownPc_Spawn(u8 pl, struct NetUser *id, char *name);
void Lobby_Disconnect(s32);
extern u8 gLobbyScreenStep;
s32 LobbyReq_Level1Open(s32 id, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: an s32 id; with the definition's u16 id, LobbyStep_Level1Open compiles differently */
void LobbyStep_Level1OpenDone(NetMsg m, NetMsg *pm);
void LobbyStep_Level1SettingsDone(NetMsg m, NetMsg *pm);
s32 NetLevel1SettingsTask_Start(NetEntrySettings *p, void (*onDone)(NetMsg, NetMsg *));
char *strcpy(char *dst, const char *src);
s32 Lobby_HrAtLeast31(void);
void Lobby_ClearHost(void);
s32 LobbyStep_Level1Enter(void);
s32 LobbyStep_Level1Users(void);
s32 LobbyReq_Level1SetColor(s8 v, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: an s8 colour; with the definition's s32, LobbyStep_Level1SetColor compiles differently */
void LobbyStep_Level1SetColorDone(NetMsg m, NetMsg *pm);
QuestInfo *Quest_CurrentInfo(void);
s32 NetDlFileTask_Start(u8 file, u8 *buf, s32 size, void (*onDone)(NetMsg, NetMsg *));
void LobbyStep_DownloadDone(NetMsg m, NetMsg *pm);
s32 LobbyReq_Level1Leave(void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_Level1LeaveDone(NetMsg m, NetMsg *pm);
extern s32 gTownMenuOpen;
extern s32 gTownMode;
extern u8 gTownModeStep;
s32 Game_IsOnline(void);
void HunterCard_Build(void *out);
s32 LobbyReq_HunterCardPart(void *src, u16 n, u16 x, void (*onReply)(NetMsg, NetMsg *));
void Lobby_IgnoreReply(NetMsg m, NetMsg *pm);
extern u8 gLobbyTextDirty;
extern u8 gOnlineMark;

/*
 * A login step on LobbyState.seqStep: clamps the player's hunter card
 * (gPlayerText, HunterCard_Sanitize) and sends it whole with
 * LobbyReq_HunterCard (LOBBY_CMD_HUNTER_CARD, 6190), then runs the reply
 * timer until the reply comes. Returns LOBBY_STEP_BUSY while waiting,
 * LOBBY_STEP_DONE once the server took the card, LOBBY_STEP_FAILED when it
 * refused it (its text is in LobbyState.errText).
 */
s32 LobbyStep_HunterCard(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_HUNTER_CARD;
        HunterCard_Sanitize(gPlayerText.raw);
        LobbyReq_HunterCard(gPlayerText.raw, sizeof(UserText), LobbyStep_HunterCardDone);
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
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_HunterCard's reply handler. Unless the lobby is being left
 * (LOBBY_PHASE_EXIT), and only if this request's reply is the one awaited
 * (awaitReply LOBBY_AWAIT_HUNTER_CARD), it clears awaitReply;
 * on success (NET_MSG_DONE) it copies gPlayerText into LobbyState.myText and
 * moves seqStep on, else it sets LOBBY_SEQ_FAILED and fetches the error
 * text into errText. msgPtr points at the same message and is not used.
 */
void LobbyStep_HunterCardDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_HUNTER_CARD) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                memcpy(gLobby->myText, gPlayerText.raw, sizeof(gLobby->myText));
                gLobby->seqStep++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/*
 * A login step on LobbyState.seqStep: asks for the login information with
 * LobbyReq_LoginInfo (LOBBY_CMD_LOGIN_INFO, 614C) and waits for the reply.
 * Returns LOBBY_STEP_BUSY while waiting, LOBBY_STEP_DONE done,
 * LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_LoginInfo(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LOGIN_INFO;
        LobbyReq_LoginInfo(LobbyStep_LoginInfoDone);
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
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_LoginInfo's reply handler (LOBBY_AWAIT_LOGIN_INFO;
 * skipped while the lobby is being left):
 * on success it moves seqStep on and copies the reply (a byte and a text)
 * into LobbyState.loginInfo and loginInfoText (LobbyNet_GetLoginInfo);
 * else LOBBY_SEQ_FAILED.
 */
void LobbyStep_LoginInfoDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LOGIN_INFO) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->seqStep++;
                LobbyNet_GetLoginInfo((struct NetLoginInfo *)gLobby->loginInfo);
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/*
 * A step of the character select screen, on LobbyState.seqStep: asks for
 * the screen's text with LobbyReq_CharaSelectText (614D), which the reply
 * writes into LobbyState.errText (cleared first; at most its 0x300 bytes).
 * Returns LOBBY_STEP_BUSY while waiting, LOBBY_STEP_DONE done,
 * LOBBY_STEP_FAILED failed (errText is cleared again).
 */
s32 LobbyStep_CharaSelectText(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_CHARA_SELECT_TEXT;
        memset(gLobby->errText, 0, sizeof(gLobby->errText));
        LobbyReq_CharaSelectText(gLobby->errText, sizeof(gLobby->errText), LobbyStep_CharaSelectTextDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        memset(gLobby->errText, 0, sizeof(gLobby->errText));
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_CharaSelectText's reply handler (awaitReply
 * LOBBY_AWAIT_CHARA_SELECT_TEXT): on success seqStep goes on and
 * LobbyNet_GetCharaSelectText reads the reply's number into replyValue,
 * which is dropped; else LOBBY_SEQ_FAILED.
 */
void LobbyStep_CharaSelectTextDone(NetMsg msg, NetMsg *msgPtr)
{
    s32 replyValue;
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_CHARA_SELECT_TEXT) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->seqStep++;
                LobbyNet_GetCharaSelectText(&replyValue);
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/*
 * A login step on LobbyState.seqStep: sends the chat colour mode with
 * LobbyReq_ChatColorMode (LOBBY_CMD_CHAT_COLOR_MODE, 670F): colorMode 3
 * when the player card's onlineMark is 8, else 0. Returns LOBBY_STEP_BUSY
 * while waiting, LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_ChatColorMode(void)
{
    HunterCard *playerCard = &gPlayerText.card;
    u8 colorMode;
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = LOBBY_AWAIT_CHAT_COLOR_MODE;
        LobbyMenu_StartTimer();
        switch (playerCard->onlineMark) {
        case 8:
            colorMode = 3;
            break;
        default:
            colorMode = 0;
            break;
        }
        LobbyReq_ChatColorMode(colorMode, LobbyStep_ChatColorModeDone);
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
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_ChatColorMode's reply handler (awaitReply
 * LOBBY_AWAIT_CHAT_COLOR_MODE): seqStep goes on, or
 * LOBBY_SEQ_FAILED with the error text fetched into errText.
 */
void LobbyStep_ChatColorModeDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_CHAT_COLOR_MODE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/*
 * A login step on LobbyState.seqStep: sends the record of the last quest
 * (gQuestRecord, a QuestRecord) with LobbyReq_QuestRecord
 * (LOBBY_CMD_QUEST_RECORD, 6139). Returns LOBBY_STEP_BUSY while waiting,
 * LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_QuestRecord(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = LOBBY_AWAIT_QUEST_RECORD;
        LobbyMenu_StartTimer();
        LobbyReq_QuestRecord(&gQuestRecord, sizeof(QuestRecord), LobbyStep_QuestRecordDone);
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
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_QuestRecord's reply handler (awaitReply
 * LOBBY_AWAIT_QUEST_RECORD): seqStep goes on, or
 * LOBBY_SEQ_FAILED.
 */
void LobbyStep_QuestRecordDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_QUEST_RECORD) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

extern u16 gLobbyRateValues[];                /* the 6884 slot 1 */
extern ElderRaidEvent gElderRaidEvent;
extern s32 gRateSnapshot[LOBBY_RATE_COUNT];              /* sub_main */
void ElderRaid_SetState(void);
void ElderRaid_CheckChanged(void);
void LobbyWorld_ScheduleEvents(LobbyWorldEvent *ev, s32 *flags);

/*
 * Applies the town data the server sent with 6884 (its slots sit in
 * gTownQuestLists, TOWN_DATA_SLOT_SIZE bytes each): slot 0's first byte becomes
 * LobbyState.townEvent; slot 1's LOBBY_RATE_COUNT u16 rate values (gLobbyRateValues) are widened into gRateSnapshot; slot 3's
 * two times (gElderRaidEvent) become two world events, the first calling
 * ElderRaid_SetState and the second ElderRaid_CheckChanged.
 * LobbyWorld_ScheduleEvents sets scheduled[i] to 1 for each event it took
 * and to 0 for one already past; scheduled is not read here.
 * LobbyStep_TownData calls it once its slots are in, LobbyRoute_TownData
 * when the server sends the data unasked.
 */
void Lobby_ApplyTownData(void)
{
    LobbyWorldEvent events[2];
    s32 scheduled[2];
    ElderRaidEvent *raid;
    u16 *src;
    s16 i;

    gLobby->townEvent = gTownQuestLists.townEvent;
    src = gLobbyRateValues;
    for (i = 0; i < LOBBY_RATE_COUNT; i++) {
        gRateSnapshot[i] = *src++;
    }
    raid = (ElderRaidEvent *)&gElderRaidEvent; /* the cast keeps the compiler reading the times through raid, as the game's code does */
    events[0].year = raid->when[0].year;
    events[0].season = raid->when[0].season;
    events[0].day = raid->when[0].day;
    events[0].tick = raid->when[0].tick;
    events[0].func = ElderRaid_SetState;
    events[1].year = raid->when[1].year;
    events[1].season = raid->when[1].season;
    events[1].day = raid->when[1].day;
    events[1].tick = raid->when[1].tick;
    events[1].func = ElderRaid_CheckChanged;
    LobbyWorld_ScheduleEvents(events, scheduled);
}

/*
 * A login step on LobbyState.seqStep: fetches the 6884 slots first to last
 * (LobbyReq_TownData, LOBBY_CMD_TOWN_DATA), one request at a time, each
 * into its own 0x100 bytes of gTownQuestLists (cleared first);
 * LobbyState.questList holds the slot being fetched. Once the last is in it
 * applies them (Lobby_ApplyTownData). Returns LOBBY_STEP_BUSY while busy,
 * LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed (questList is reset either
 * way). The login asks for 0 to 3.
 */
s32 LobbyStep_TownData(u8 first, u8 last)
{
    s16 *seqStepPtr;
    LobbyState *lobby;
    s32 slot;
    s16 step;

    lobby = gLobby;
    seqStepPtr = &lobby->seqStep;
    step = *seqStepPtr;
    switch (step) {
    case 0:
        lobby->questList = first;
        gLobby->seqStep++;
    case 1:
        gLobby->seqStep++;
        gLobby->awaitReply = LOBBY_AWAIT_TOWN_DATA;
        LobbyMenu_StartTimer();
        memset((u8 *)&gTownQuestLists + (gLobby->questList << 8), 0, TOWN_DATA_SLOT_SIZE);
        slot = gLobby->questList;
        LobbyReq_TownData(slot, (u8 *)&gTownQuestLists + (slot << 8), TOWN_DATA_SLOT_SIZE, LobbyStep_TownDataDone);
        break;
    case 2:
        LobbyMenu_TickTimer();
        break;
    case 3:
        slot = lobby->questList + 1;
        lobby->questList = slot;
        if (slot <= last) {
            gLobby->seqStep = 1;
            break;
        }
        gLobby->seqStep = 0;
        gLobby->questList = 0;
        Lobby_ApplyTownData();
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        *seqStepPtr = 0;
        gLobby->questList = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_TownData's reply handler (awaitReply
 * LOBBY_AWAIT_TOWN_DATA): on success LobbyNet_GetTownData gives
 * the reply's slot number (into slot, unused) and seqStep goes on; else
 * LOBBY_SEQ_FAILED with the error text in errText. slot is declared first
 * and set to 0 so that it takes the last bytes of msg's 8-byte stack home
 * (NetMsg is 6 bytes), as in the game's code.
 */
void LobbyStep_TownDataDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 slot = 0;
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_TOWN_DATA) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                LobbyNet_GetTownData(&slot);
                gLobby->seqStep++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/*
 * A login step on LobbyState.seqStep: asks for the town's clock and
 * calendar with LobbyReq_TownClock (LOBBY_CMD_TOWN_CLOCK, 6892). Returns
 * LOBBY_STEP_BUSY while waiting, LOBBY_STEP_DONE done, LOBBY_STEP_FAILED
 * failed.
 */
s32 LobbyStep_TownClock(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = LOBBY_AWAIT_TOWN_CLOCK;
        LobbyMenu_StartTimer();
        LobbyReq_TownClock(LobbyStep_TownClockDone);
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
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_TownClock's reply handler (awaitReply
 * LOBBY_AWAIT_TOWN_CLOCK): on success it copies the reply
 * (LobbyNet_GetTownClock into clock): the three seasons' lengths in days
 * (gTownSeasonDays), the year (gTownYear), the season, the
 * day and the ticks of a day; it sets the season (Season_Set, from the
 * low byte of gTownSeason) and the lobby clock (gLobbyTime, the reply's
 * ticks in seconds, LobbyTime_SetClock), and moves seqStep on. Else
 * LOBBY_SEQ_FAILED with the error text in errText.
 */
void LobbyStep_TownClockDone(NetMsg msg, NetMsg *msgPtr)
{
    NetTownClock clock;
    u32 *seasonDays;
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_TOWN_CLOCK) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                LobbyNet_GetTownClock(&clock);
                seasonDays = clock.list;
                gTownSeasonDays[0] = seasonDays[0];
                gTownSeasonDays[1] = seasonDays[1];
                gTownSeasonDays[2] = seasonDays[2];
                gTownYear = clock.year;
                gTownSeason = clock.season;
                gTownDay = clock.day;
                gTownDayTicks = clock.dayTicks;
                Season_Set(*(u8 *)&gTownSeason);
                gLobbyTime = LobbyWorld_TicksToSeconds(clock.time);
                LobbyTime_SetClock(gLobbyTime);
                gLobby->seqStep++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* a level-1 entry as sub_main keeps them (gLobbyLevel1Entries, 0x164 bytes each) */
extern LevelEntry gLobbyLevel1Entries[];          /* level 1's entries (sub_main copies) */
extern u16 gLobbyLevel1Count;                  /* the number of level-1 entries */
s32 NetLevel1ListTask_Start(u16 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *)); /* unit's own declaration: a u16 first argument; with the definition's s32, LobbyStep_Level1List compiles differently */
s32 NetLevel1AllowedTask_Start(u16 n, void (*onDone)(NetMsg, NetMsg *));
s32 NetList1_GetCount(u16 *n);
s32 NetList1_GetByte(u16 id, u8 *out);
s32 NetList1_GetColor(u16 id, s32 *out);
s32 NetList1_GetUsers(u16 id, u16 *a, u16 *b);
s32 NetList1_GetName(u16 id, char *dst);
s32 NetList1_GetMaxUsers(u16 id, u16 *out);
s32 NetList1_GetText(u16 id, char *dst);
s32 NetList1_GetGroupInfo(u16 id, u8 *out);
s32 LobbyNet_GetEntryVals(u16 id, NetEntryVals *out);

/*
 * Fetches the second level's list (task 3, NetLevel1ListTask_Start: each
 * entry's values, state, name, text, colour and group info) and copies
 * it into gLobbyLevel1Entries: every entry when id is 0 (the count goes
 * to gLobbyLevel1Count), else entry id alone; task 3 itself ignores id and
 * fetches the whole list. Then task 5 (NetLevel1AllowedTask_Start) fetches
 * which settings may be set, and each entry copied gets its 6314 value
 * (allowed.v[2]) in textAllowed. Steps on LobbyState.taskStep. Returns
 * LOBBY_STEP_BUSY while busy, LOBBY_STEP_DONE done, LOBBY_STEP_FAILED
 * failed; a step it does not know leaves the lobby
 * (Lobby_Disconnect, LOBBY_QUIT_FAILED).
 */
s32 LobbyStep_Level1List(s32 id)
{
    LevelEntry *entry;
    u32 i;
    NetEntryVals allowed;
    s8 *taskStepPtr = &gLobby->taskStep;

    switch (*taskStepPtr) {
    case 0:
        (*taskStepPtr)++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_LIST;
        NetLevel1ListTask_Start(id, NET_ENTRY_VALUES | NET_ENTRY_STATE | NET_ENTRY_NAME | NET_ENTRY_TEXT | NET_ENTRY_COLOR | NET_ENTRY_GROUP_INFO,
                                LobbyStep_Level1ListDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        if (id == 0) {
            NetList1_GetCount(&gLobbyLevel1Count);
            for (i = 0; i < gLobbyLevel1Count; i++) {
                gLobbyLevel1Entries[i].id = i + 1;
                NetList1_GetByte(i + 1, &gLobbyLevel1Entries[i].state);
                NetList1_GetColor(i + 1, &gLobbyLevel1Entries[i].color);
                NetList1_GetUsers(i + 1, &gLobbyLevel1Entries[i].users, &gLobbyLevel1Entries[i].questUsers);
                NetList1_GetName(i + 1, gLobbyLevel1Entries[i].name);
                NetList1_GetMaxUsers(i + 1, &gLobbyLevel1Entries[i].maxUsers);
                NetList1_GetText(i + 1, gLobbyLevel1Entries[i].text);
                NetList1_GetGroupInfo(i + 1, gLobbyLevel1Entries[i].groupInfo);
            }
        } else {
            i = id - 1;
            gLobbyLevel1Entries[i].id = (u16)id;
            entry = &gLobbyLevel1Entries[i];
            NetList1_GetByte(id, &entry->state);
            NetList1_GetColor(id, &entry->color);
            NetList1_GetUsers(id, &entry->users, &entry->questUsers);
            NetList1_GetName(id, entry->name);
            NetList1_GetMaxUsers(id, &entry->maxUsers);
            NetList1_GetText(id, entry->text);
            NetList1_GetGroupInfo(id, entry->groupInfo);
        }
        gLobby->taskStep++;
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_ALLOWED;
        NetLevel1AllowedTask_Start(id, LobbyStep_Level1AllowedDone);
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    case 4:
        if (id == 0) {
            for (i = 0; i < gLobbyLevel1Count; i++) {
                LobbyNet_GetEntryVals(i + 1, &allowed);
                gLobbyLevel1Entries[i].textAllowed = allowed.v[2];
            }
        } else {
            LobbyNet_GetEntryVals(id, &allowed);
            gLobbyLevel1Entries[id - 1].textAllowed = allowed.v[2];
        }
    gLobby->taskStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        *taskStepPtr = 0;
        return LOBBY_STEP_FAILED;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * Task 5's end callback (LOBBY_AWAIT_LEVEL1_ALLOWED).
 * A progress message
 * (NET_MSG_PROGRESS) is ignored; NET_MSG_DONE clears awaitReply and moves
 * taskStep on; any other kind sets taskStep to LOBBY_SEQ_FAILED, leaving
 * awaitReply set.
 */
void LobbyStep_Level1AllowedDone(NetMsg msg, NetMsg *msgPtr)
{
    LobbyState *lobby = gLobby;
    s8 flag = msg.flag;
    u8 *awaited;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_ALLOWED) {
            if (msg.kind == NET_MSG_PROGRESS) {
                if (flag == NET_PROGRESS_LIST_REPLY) {
                    return;
                }
                return;
            }
            if (msg.kind == NET_MSG_DONE) {
                *awaited = LOBBY_AWAIT_NONE;
                gLobby->taskStep++;
                return;
            }
            lobby->taskStep = LOBBY_SEQ_FAILED;
        }
    }
}

/*
 * Task 3's end callback (LOBBY_AWAIT_LEVEL1_LIST).
 * A progress message (NET_MSG_PROGRESS)
 * with flag 11 (one entry's replies came) reads the replies still expected
 * (LobbyNet_GetListReplies into replyCount, unused); NET_MSG_DONE clears
 * awaitReply and moves taskStep on; any other kind sets taskStep to
 * LOBBY_SEQ_FAILED.
 */
void LobbyStep_Level1ListDone(NetMsg msg, NetMsg *msgPtr)
{
    u32 replyCount;
    LobbyState *lobby = gLobby;
    s8 flag = msg.flag;
    u8 *awaited;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_LIST) {
            if (msg.kind == NET_MSG_PROGRESS) {
                if (flag == NET_PROGRESS_LIST_REPLY) {
                    LobbyNet_GetListReplies((u16 *)&replyCount);
                    return;
                }
                return;
            }
            if (msg.kind == NET_MSG_DONE) {
                *awaited = LOBBY_AWAIT_NONE;
                gLobby->taskStep++;
                return;
            }
            lobby->taskStep = LOBBY_SEQ_FAILED;
        }
    }
}

/*
 * Enters the selected level-1 entry (LobbyLevel_SelId(1)) with an empty
 * password (LobbyReq_Level1Enter, LOBBY_CMD_LEVEL1_ENTER, 6305). Steps on
 * LobbyState.subStep. Returns LOBBY_STEP_BUSY while waiting,
 * LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed or on a step it does not
 * know.
 */
s32 LobbyStep_Level1Enter(void)
{
    char *pass = "";
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_ENTER;
        LobbyReq_Level1Enter(LobbyLevel_SelId(1), pass, LobbyStep_Level1EnterDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
    default:
        gLobby->subStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_Level1Enter's reply handler (awaitReply
 * LOBBY_AWAIT_LEVEL1_ENTER): on success subStep becomes 2 and the
 * lobby's place becomes 1 (Lobby_SetPlace: in a level-1 entry); on a
 * failure subStep is LOBBY_SEQ_FAILED, the error text is shown in a wait
 * dialog (LobbyDialog_OpenWait) and gLobbyLevel1Sel is cleared.
 */
void LobbyStep_Level1EnterDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_ENTER) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->subStep = 2;
                Lobby_SetPlace(LOBBY_PLACE_LEVEL1);
            } else {
                gLobby->subStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobbyLevel1Sel = 0;
            }
        }
    }
}

/*
 * Asks for the users of the selected level-1 entry (LobbyReq_Level1Users,
 * LOBBY_CMD_LEVEL1_USERS, 630A); the reply makes them the town's members
 * (LobbyStep_Level1UsersDone). Once done it clears LobbyState.memberSlot.
 * Steps on LobbyState.subStep. Returns LOBBY_STEP_BUSY while waiting,
 * LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_Level1Users(void)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_USERS;
        LobbyReq_Level1Users(LobbyLevel_SelId(1), LobbyStep_Level1UsersDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        memset(gLobby->memberSlot, 0, sizeof(gLobby->memberSlot));
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
    default:
        gLobby->subStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* 630A's reply: the entry's users become the members (LobbyState and
 * gNetUsers), each in the slot it had or the first free one */
/* Level 4 (the file stays at 3: HunterCard_SendOnlineMark doesn't match at 4);
 * at level 3, k * 0x11C is computed again after the host block. */
#pragma optimization_level 4
/*
 * LobbyStep_Level1Users's reply handler (awaitReply
 * LOBBY_AWAIT_USERS). On success each of the reply's 10
 * users (LobbyNet_GetUser into user, its card clamped) becomes a member:
 * it takes the slot of the member with the same id, else the first free
 * slot; its id, name (16 bytes) and card go into LobbyState.members[slot]
 * and gNetUsers[slot]. The slot with the player's id (gMyUserId) becomes
 * gScene.localPlayer; the member whose id compares greatest is kept as
 * the host (LobbyState.hostId); the card is applied (HunterCard_Apply),
 * the character spawned (TownPc_Spawn) and gScene.playerCount counted.
 * Then subStep goes on and gLobbyScreenStep is cleared. A failed reply
 * leaves the lobby (Lobby_Disconnect, LOBBY_QUIT_FAILED).
 * Compiled at optimization level 4, the rest of the file at 3: at level
 * 3 the compiler computes slot * sizeof(NetUser) a second time after the
 * host check, which the game's code does not.
 */
void LobbyStep_Level1UsersDone(NetMsg msg, NetMsg *msgPtr)
{
    NetUser user;
    s32 i;
    s32 slot;
    u8 *userText;
    char *userName;
    s32 j;
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_USERS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                i = 0;
                gScene.playerCount = 0;
                userText = user.text;
                userName = user.name;
                do {
                    memset(&user, 0, sizeof(NetUser));
                    LobbyNet_GetUser(i, user.id, userName, userText);
                    HunterCard_Sanitize(userText);
                    if (user.id[0] != 0) {
                        for (slot = 0; slot < LOBBY_MEMBER_COUNT; slot++) {
                            if (gNetUsers[slot].id[0] == 0) {
                                break;
                            }
                        }
                        for (j = 0; j < LOBBY_MEMBER_COUNT; j++) {
                            if (memcmp(user.id, gNetUsers[j].id, sizeof(user.id)) == 0) {
                                slot = j;
                            }
                        }
                        memset(&gLobby->members[slot], 0, sizeof(NetUser));
                        memset(&gNetUsers[slot], 0, sizeof(NetUser));
                        memcpy(gLobby->members[slot].id, user.id, sizeof(user.id));
                        memcpy(gLobby->members[slot].name, userName, 0x10);
                        memcpy(gLobby->members[slot].text, userText, sizeof(user.text));
                        memcpy(gNetUsers[slot].id, gLobby->members[slot].id, sizeof(user.id));
                        memcpy(gNetUsers[slot].name, gLobby->members[slot].name, 0x10);
                        memcpy(gNetUsers[slot].text, gLobby->members[slot].text, sizeof(user.text));
                        if (memcmp(user.id, gMyUserId, sizeof(user.id)) == 0) {
                            gScene.localPlayer = slot;
                        }
                        if (gScene.playerCount == 0 || 0 > memcmp(gLobby->hostId, gLobby->members[slot].id, sizeof(gLobby->hostId))) {
                            memset(gLobby->hostId, 0, sizeof(gLobby->hostId));
                            memcpy(gLobby->hostId, gLobby->members[slot].id, sizeof(gLobby->hostId));
                        }
                        HunterCard_Apply(slot, gLobby->members[slot].text);
                        (gScene.playerCount)++;
                        TownPc_Spawn(slot, &gLobby->members[slot], gLobby->members[slot].name);
                    }
                    i++;
                } while (i < LOBBY_MEMBER_COUNT);
                gLobby->subStep++;
                gLobbyScreenStep = 0;
            } else {
                Lobby_Disconnect(LOBBY_QUIT_FAILED);
            }
        }
    }
}
#pragma optimization_level reset

/*
 * LobbyStep_Level1Open's reply handler (awaitReply
 * LOBBY_AWAIT_LEVEL1_OPEN): subStep goes on, or
 * LOBBY_SEQ_FAILED with the error text in errText.
 */
void LobbyStep_Level1OpenDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_OPEN) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/*
 * Opens level-1 entry id for recruiting (LobbyReq_Level1Open,
 * LOBBY_CMD_LEVEL1_OPEN, 6310); the recruit screens call it. Steps on
 * LobbyState.subStep. Returns LOBBY_STEP_BUSY while waiting,
 * LOBBY_STEP_DONE done (gLobbyLevel1Sel becomes id), LOBBY_STEP_FAILED
 * failed (gLobbyLevel1Sel cleared).
 */
s32 LobbyStep_Level1Open(s32 id)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_OPEN;
        LobbyReq_Level1Open(id, LobbyStep_Level1OpenDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        gLobbyLevel1Sel = id;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->subStep = 0;
        gLobbyLevel1Sel = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * Task 8's end callback (LOBBY_AWAIT_LEVEL1_SETTINGS):
 * subStep goes on, or
 * LOBBY_SEQ_FAILED with the error text in errText.
 */
void LobbyStep_Level1SettingsDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_SETTINGS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/*
 * Sends the settings of the level-1 entry the player opened (task 8,
 * NetLevel1SettingsTask_Start: name, password, text and group info): text
 * as the entry's text, or the default text (gLobbyLabelTexts[LOBBY_LABEL_TOWN_UNNAMED])
 * when text is NULL; the four group-info bytes from groupBytes, or zeros
 * when it is NULL. Steps on LobbyState.subStep. Returns LOBBY_STEP_BUSY
 * while busy, LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_Level1Settings(char *text, u8 *groupBytes)
{
    NetEntrySettings settings;
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        memset(&settings, 0, sizeof(settings));
        if (text == NULL) {
            strcpy(settings.text, gLobbyLabelTexts[LOBBY_LABEL_TOWN_UNNAMED]);
        } else {
            strcpy(settings.text, text);
        }
        if (groupBytes != NULL) {
            memcpy(settings.groupInfo, groupBytes, sizeof(settings.groupInfo));
        }
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_SETTINGS;
        NetLevel1SettingsTask_Start(&settings, LobbyStep_Level1SettingsDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->subStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * 1 when the player may enter level-1 entry id (1-based), 0 when the
 * entry's group limit (groupInfo[0], GROUP_LIMIT_*) shuts out the player's
 * hunter rank (Lobby_HrAtLeast31); any other limit lets everyone in.
 */
s32 LobbyLevel1_CanEnter(s32 id)
{
    switch (LEVEL1_ENTRY(id)->groupInfo[0]) {
    case GROUP_LIMIT_NONE:
        return 1;
    case GROUP_LIMIT_HR_BELOW_31:
        if (Lobby_HrAtLeast31() == 1) {
            return 0;
        }
        break;
    case GROUP_LIMIT_HR_31_UP:
        if (Lobby_HrAtLeast31() == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/*
 * Enters the selected level-1 entry: selects it (LobbyLevel_SelId), clears
 * the host (Lobby_ClearHost), enters it (LobbyStep_Level1Enter), then makes
 * its users the members (LobbyStep_Level1Users). Steps on
 * LobbyState.subStep2. Returns LOBBY_STEP_BUSY while busy, LOBBY_STEP_DONE
 * done, LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_EnterLevel1(void)
{
    switch (gLobby->subStep2) {
    case 0:
        LobbyLevel_SelId(1);
        gLobby->subStep2++;
        Lobby_ClearHost();
    case 1:
        switch (LobbyStep_Level1Enter()) {
        case LOBBY_STEP_DONE:
            gLobby->subStep2++;
            break;
        case LOBBY_STEP_FAILED:
            gLobby->subStep2 = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    case 2:
        switch (LobbyStep_Level1Users()) {
        case LOBBY_STEP_DONE:
            gLobby->subStep2 = 0;
            return LOBBY_STEP_DONE;
        case LOBBY_STEP_FAILED:
            gLobby->subStep2 = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_Level1SetColor's reply handler (awaitReply
 * LOBBY_AWAIT_LEVEL1_SET_COLOR): subStep goes on, or
 * LOBBY_SEQ_FAILED with the error text in errText.
 */
void LobbyStep_Level1SetColorDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_SET_COLOR) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/*
 * Sends the colour of the opened level-1 entry's name
 * (LobbyReq_Level1SetColor, LOBBY_CMD_LEVEL1_SET_COLOR, 630C). Steps on
 * LobbyState.subStep. Returns LOBBY_STEP_BUSY while waiting,
 * LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_Level1SetColor(s32 color)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_SET_COLOR;
        LobbyReq_Level1SetColor(color, LobbyStep_Level1SetColorDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->subStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * Clears the host (Lobby_ClearHost) and makes the selected level-1 entry's
 * users the members again (LobbyStep_Level1Users). Steps on
 * LobbyState.subStep2. Returns LOBBY_STEP_BUSY while busy, LOBBY_STEP_DONE
 * done, LOBBY_STEP_FAILED failed.
 */
s32 LobbyStep_RefreshLevel1Users(void)
{
    u8 subStep2;

    subStep2 = gLobby->subStep2;
    switch (subStep2) {
    case 0:
        gLobby->subStep2 = (u8)(subStep2 + 1);
        Lobby_ClearHost();
        break;
    case 1:
        switch (LobbyStep_Level1Users()) {
        case LOBBY_STEP_DONE:
            gLobby->subStep2 = 0;
            return LOBBY_STEP_DONE;
        case LOBBY_STEP_FAILED:
            gLobby->subStep2 = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * Fetches the users of level-1 entry id (LobbyReq_Level1Users, 630A) into
 * users[10] (id, name and card each) without making them members. It clamps
 * the cards of LobbyState.members, not of users, as the game's code does,
 * and starts no reply timer. Steps on LobbyState.seqStep. Returns
 * LOBBY_STEP_BUSY while waiting, LOBBY_STEP_DONE done; a failed reply
 * leaves the lobby (Lobby_Disconnect, LOBBY_QUIT_FAILED) and returns LOBBY_STEP_FAILED.
 */
s32 LobbyStep_FetchLevel1Users(NetUser *users, s32 id)
{
    s16 *seqStepPtr;
    LobbyState *lobby;
    s32 i;

    lobby = gLobby;
    seqStepPtr = &lobby->seqStep;
    switch (*seqStepPtr) {
    case 0:
        lobby->awaitReply = LOBBY_AWAIT_USERS;
        gLobby->seqStep++;
        memset(users, 0, sizeof(NetUser) * LOBBY_MEMBER_COUNT);
        LobbyReq_Level1Users(id, LobbyStep_FetchLevel1UsersDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
            LobbyNet_GetUser(i, users[i].id, users[i].name, users[i].text);
            HunterCard_Sanitize(gLobby->members[i].text);
        }
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        *seqStepPtr = 0;
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_FetchLevel1Users's reply handler (awaitReply
 * LOBBY_AWAIT_USERS): seqStep goes on, or
 * LOBBY_SEQ_FAILED.
 */
void LobbyStep_FetchLevel1UsersDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_USERS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

extern u8 gQuestFiles[];
s32 NetDlListTask_Start(void (*onDone)(NetMsg, NetMsg *));
s32 LobbyNet_GetDlFiles(u8 *count, NetDlFile **files);
void Lobby_ApplyServerTimers(void);
void LobbyStep_DlListDone(NetMsg m, NetMsg *pm);

/*
 * Downloads the quest files the server offers. Task 13
 * (NetDlListTask_Start) gives fileCount files, each with a size and a stamp
 * (LobbyNet_GetDlFiles); the saved records are a NetDlFile per file at
 * +0x6EF4 of the lobby work buffer (gLobbyWorkBuf). The saved records and
 * quest files past the ones offered are cleared (when only file 0, the
 * list, is offered, LobbyState.dlQuestsReady is cleared too). Then each
 * file whose record differs from the saved one is fetched (task 14,
 * NetDlFileTask_Start): file 0 into the work buffer right after the four
 * records, files 1-3 into gQuestFiles, and its record saved. After file 0
 * the server timers are applied (Lobby_ApplyServerTimers); a quest file of
 * nonzero size sets dlQuestsReady and counts in dlQuestCount. subStep2
 * counts the files left. At the end the first event bit set in the list
 * file's event bits (bits 0-3) sets the record kind's event mark
 * (RECORD_EVENT_TITLES, _ITEMS, _WEAPONS, _ARMOR in that order). Steps
 * on LobbyState.subStep. Returns LOBBY_STEP_BUSY while busy,
 * LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed; on a failure with no file
 * offered and no list saved before, it leaves the lobby (Lobby_Disconnect,
 * LOBBY_QUIT_FAILED). The game's code leaves every branch delay slot of
 * this function empty. MWCC does the same when a switch whose default case
 * comes first holds a non-empty default; the last switch (on file) is
 * written that way, with an empty check that compiles to nothing.
 */
s32 LobbyStep_DownloadQuests(void)
{
    NetDlFile *list;
    u8 fileCount = 0;
    u8 *subStepPtr = &gLobby->subStep;
    LobbyState *lobby;
    u8 file;
    u16 eventBits;
    NetDlFile *saved;
    u8 checkFile;

    switch (*subStepPtr) {
    case 0:
        *subStepPtr = *subStepPtr + 1;
        gLobby->dlQuestCount = 0;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_DL_LIST;
        NetDlListTask_Start(LobbyStep_DlListDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        LobbyNet_GetDlFiles(&fileCount, &list);
        switch (fileCount) {
        case 0:
            break;
        case 1:
            gLobby->dlQuestsReady = 0;
        default:
            for (file = fileCount; file < QDL_FILE_COUNT; file++) {
                memset(QDL_SAVED + file, 0, sizeof(NetDlFile));
                memset(&gQuestFiles[(file - 1) * QUEST_FILE_SIZE], 0, QUEST_FILE_SIZE);
            }
            break;
        }
        gLobby->subStep2 = fileCount;
        gLobby->subStep++;
    case 3:
        if (gLobby->subStep2 != 0) {
            LobbyNet_GetDlFiles(&fileCount, &list);
            lobby = gLobby;
            checkFile = fileCount - lobby->subStep2;
            list += checkFile;
            saved = QDL_SAVED + checkFile;
            if (list->size != saved->size || list->stamp != saved->stamp) {
                lobby->subStep++;
                LobbyMenu_StartTimer();
                gLobby->awaitReply = LOBBY_AWAIT_DL_FILE;
                switch (checkFile) {
                case 0:
                    NetDlFileTask_Start(checkFile, QDL_LIST_FILE, list->size, LobbyStep_DownloadDone);
                    memcpy(QDL_SAVED + checkFile, list, sizeof(NetDlFile));
                    break;
                default:
                    NetDlFileTask_Start(checkFile, &gQuestFiles[(checkFile - 1) * QUEST_FILE_SIZE], QUEST_FILE_SIZE,
                                        LobbyStep_DownloadDone);
                    memcpy(QDL_SAVED + checkFile, list, sizeof(NetDlFile));
                    break;
                }
            } else {
                lobby->subStep = 5;
            }
            break;
        }
        gLobby->subStep = 0;
        gLobby->subStep2 = 0;
        eventBits = QDL_LIST_EVENT_BITS;
        if (eventBits & 1) {
            gSave.recordCounts[RECORD_EVENT_TITLES] = 1;
        } else if (eventBits & 2) {
            gSave.recordCounts[RECORD_EVENT_ITEMS] = 1;
        } else if (eventBits & 4) {
            gSave.recordCounts[RECORD_EVENT_WEAPONS] = 1;
        } else if (eventBits & 8) {
            gSave.recordCounts[RECORD_EVENT_ARMOR] = 1;
        }
        return LOBBY_STEP_DONE;
    case 4:
        LobbyMenu_TickTimer();
        break;
    case 5:
        LobbyNet_GetDlFiles(&fileCount, &list);
        lobby = gLobby;
        file = fileCount - lobby->subStep2;
        list += file;
        if (file != 0) {
            if (list->size != 0) {
                lobby->dlQuestsReady = 1;
                gLobby->dlQuestCount++;
            }
        } else {
            Lobby_ApplyServerTimers();
        }
        gLobby->subStep = 3;
        gLobby->subStep2--;
        break;
    case LOBBY_SEQ_FAILED:
        *subStepPtr = 0;
        gLobby->subStep2 = 0;
        LobbyNet_GetDlFiles(&fileCount, &list);
        file = fileCount - gLobby->subStep2;
        list += file;
        switch (file) {
        default:
            /* an empty check: with it the compiler leaves every branch delay slot of the function empty */
            if (file > QDL_FILE_COUNT || fileCount > QDL_FILE_COUNT) {}
            break;
        case 0:
            if (QDL_SAVED->size == 0) {
                Lobby_Disconnect(LOBBY_QUIT_FAILED);
            }
            break;
        }
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

extern u8 *gStageFile;
/*
 * Downloads quest file fileSlot + 1 (slot read as an s8) into the stage
 * file buffer (gStageFile) with task 14 (NetDlFileTask_Start), its size
 * taken from the saved download records, unless the current quest
 * (Quest_CurrentInfo) already has id: then it returns LOBBY_STEP_DONE at once. Steps on
 * LobbyState.subStep. Returns LOBBY_STEP_BUSY while busy, LOBBY_STEP_DONE
 * done, LOBBY_STEP_FAILED failed (the error text is shown in a wait
 * dialog).
 */
s32 LobbyStep_Download(s32 slot, s32 id)
{
    u8 *subStepPtr;
    LobbyState *lobby;
    QuestInfo *quest;
    s32 fileSlot;

    lobby = gLobby;
    subStepPtr = &lobby->subStep;
    switch (*subStepPtr) {
    case 0:
        quest = Quest_CurrentInfo();
        if (quest != NULL && quest->id == (u16)id) {
            return LOBBY_STEP_DONE;
        }
        gLobby->subStep++;
        LobbyMenu_StartTimer();
        fileSlot = (s8)slot;
        gLobby->awaitReply = LOBBY_AWAIT_DL_FILE;
        NetDlFileTask_Start(fileSlot + 1, gStageFile, (QDL_SAVED + 1)[fileSlot].size, LobbyStep_DownloadDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        *subStepPtr = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        LobbyNet_GetErrText((s8 *)lobby->errText);
        LobbyDialog_OpenWait(gLobby->errText);
        gLobby->subStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * Task 13's end callback (awaitReply LOBBY_AWAIT_DL_LIST):
 * subStep goes on, or LOBBY_SEQ_FAILED.
 */
void LobbyStep_DlListDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_DL_LIST) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/*
 * Task 14's end callback (awaitReply LOBBY_AWAIT_DL_FILE),
 * for LobbyStep_DownloadQuests and LobbyStep_Download: subStep goes on, or
 * LOBBY_SEQ_FAILED.
 */
void LobbyStep_DownloadDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_DL_FILE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/*
 * Leaves the level-1 entry (LobbyReq_Level1Leave, LOBBY_CMD_LEVEL1_LEAVE,
 * 6408). Steps on LobbyState.seqStep. Returns LOBBY_STEP_BUSY while
 * waiting, LOBBY_STEP_DONE done, LOBBY_STEP_FAILED failed; a step it does
 * not know first leaves the lobby (Lobby_Disconnect, LOBBY_QUIT_FAILED).
 */
s32 LobbyStep_Level1Leave(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_LEAVE;
        LobbyReq_Level1Leave(LobbyStep_Level1LeaveDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_Level1Leave's reply handler (awaitReply
 * LOBBY_AWAIT_LEVEL1_LEAVE): seqStep goes on, or the error text
 * is fetched into errText and seqStep is LOBBY_SEQ_FAILED.
 */
void LobbyStep_Level1LeaveDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_LEAVE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* the third-level entry the player looks at (gLobbyRecs): what may be sent,
 * and task 6's records as the level-2 screens show them (0x14A8 bytes each) */
extern LobbyRecs gLobbyRecs;
s32 NetLevel2RecsTask_Start(u16 id, void (*onDone)(NetMsg, NetMsg *));
s32 NetLevel2RecsTask_GetRecs(u16 id, NetRecs2 *dst); /* unit's own declaration: a u16 id; with the definition's s32, LobbyStep_Level2Entry compiles differently */
s32 LobbyReq_Level2Users(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetUser(u16 i, char *id, char *name, u8 *text);
void LobbyStep_Level2RecsDone(NetMsg m, NetMsg *pm);
void LobbyStep_Level2UsersDone(NetMsg m, NetMsg *pm);

/*
 * Fetches level-2 entry id (0-based; the server's id is entryId, id + 1)
 * for the quest counter: clears gLobbyRecs and sets its usePass from the
 * entry's locked (gLobbyLevel2Entries), fetches the entry's records (task
 * 6, NetLevel2RecsTask_Start, NetLevel2RecsTask_GetRecs into recs) and
 * copies them into gLobbyRecs: what may be sent, and for each record its
 * name, editable flag, choice count, default choice (also as the current
 * one) and the choices' names. Then it asks for the entry's users
 * (LobbyReq_Level2Users, LOBBY_CMD_LEVEL2_USERS, 640A) and copies four of
 * them into users. Steps on LobbyState.seqStep. Returns LOBBY_STEP_DONE done, else
 * LOBBY_STEP_BUSY: a failure is handled by the reply handlers, which set seqStep to 0.
 */
s32 LobbyStep_Level2Entry(u16 id, NetUser *users)
{
    NetRecs2 recs;
    s32 i;
    s32 j;
    s32 entryId;
    s16 *seqStepPtr = &gLobby->seqStep;

    switch (*seqStepPtr) {
    case 0:
        *seqStepPtr = *seqStepPtr + 1;
        memset(&gLobbyRecs, 0, sizeof(gLobbyRecs));
        gLobbyRecs.usePass = gLobbyLevel2Entries[id].locked;
        break;
    case 1:
        *seqStepPtr = *seqStepPtr + 1;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_ENTRY_RECS;
        NetLevel2RecsTask_Start(id + 1, LobbyStep_Level2RecsDone);
        break;
    case 2:
        LobbyMenu_TickTimer();
        break;
    case 3:
        *seqStepPtr = *seqStepPtr + 1;
        entryId = id + 1;
        NetLevel2RecsTask_GetRecs(entryId, &recs);
        gLobbyRecs.sendName = recs.sendName;
        gLobbyRecs.sendPass = recs.sendPass;
        gLobbyRecs.n = recs.n;
        for (i = 0; i < gLobbyRecs.n; i++) {
            strcpy(gLobbyRecs.rec[i].name, recs.rec[i].name);
            gLobbyRecs.rec[i].editable = recs.rec[i].editable;
            gLobbyRecs.rec[i].subCount = recs.rec[i].subCount;
            gLobbyRecs.rec[i].defaultChoice = gLobbyRecs.rec[i].cur = recs.rec[i].defaultChoice;
            for (j = 0; j < gLobbyRecs.rec[i].subCount; j++) {
                strcpy(gLobbyRecs.rec[i].subName[j], recs.rec[i].subName[j]);
            }
        }
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_ENTRY_USERS;
        LobbyReq_Level2Users(entryId, LobbyStep_Level2UsersDone);
        break;
    case 4:
        LobbyMenu_TickTimer();
        break;
    case 5:
        *seqStepPtr = 0;
        memset(users, 0, sizeof(NetUser) * LOBBY_ROOM_USER_COUNT);
        for (i = 0; i < LOBBY_ROOM_USER_COUNT; i++) {
            NetList2_GetUser(i, users[i].id, users[i].name, users[i].text);
        }
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        *seqStepPtr = 0;
        break;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * Task 6's end callback (LOBBY_AWAIT_LEVEL2_ENTRY_RECS).
 * Progress messages (NET_MSG_PROGRESS)
 * are ignored; NET_MSG_DONE moves seqStep on; NET_MSG_FAILED shows the
 * error text in a
 * wait dialog, takes the field controls away (gFieldControl 0), opens the
 * town menu in TOWN_MODE_MESSAGE_WAIT and sets
 * seqStep to 0.
 */
void LobbyStep_Level2RecsDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_ENTRY_RECS && msg.kind != NET_MSG_PROGRESS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->seqStep++;
            } else if (msg.kind == NET_MSG_FAILED) {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gFieldControl = 0;
                gTownMenuOpen = 1;
                gTownMode = TOWN_MODE_MESSAGE_WAIT;
                gLobby->seqStep = 0;
            }
        }
    }
}

/*
 * LobbyStep_Level2Entry's users reply handler (awaitReply
 * LOBBY_AWAIT_LEVEL2_ENTRY_USERS): seqStep goes on; on a failure the
 * same as LobbyStep_Level2RecsDone's, and gTownModeStep is cleared too.
 */
void LobbyStep_Level2UsersDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_ENTRY_USERS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->seqStep++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gFieldControl = 0;
                gTownMenuOpen = 1;
                gTownMode = TOWN_MODE_MESSAGE_WAIT;
                gLobby->seqStep = 0;
                gTownModeStep = 0;
            }
        }
    }
}

/* sends the player's text (6190) when it changed, and 670F first when its
 * byte 2 changed */
/*
 * When online, builds the player's hunter card afresh (HunterCard_Build)
 * and, if it differs from gPlayerText, sends it: first the chat colour
 * mode (670F: 3 for colour 8, else 0) when the colour changed, then the
 * card goes into LobbyState.members[localPlayer], myText, gPlayerText and
 * gNetUsers[localPlayer] and is sent whole (LobbyReq_HunterCard, 6190).
 * The replies are ignored (Lobby_IgnoreReply). Clears gLobbyTextDirty.
 * Returns 0. text holds 0xE0 bytes, but sizeof(UserText) are compared and copied
 * from it, as in the game's code.
 */
s32 HunterCard_SendChanged(void)
{
    u8 text[0xE0];
    u8 colorMode;
    u8 newColor;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    HunterCard_Build(text);
    if (memcmp(text, gPlayerText.raw, sizeof(UserText)) == 0) {
        return 0;
    }
    newColor = ((HunterCard *)text)->onlineMark;
    if (newColor != gPlayerText.card.onlineMark) {
        switch (newColor) {
        case 8:
            colorMode = 3;
            break;
        default:
            colorMode = 0;
            break;
        }
        LobbyReq_ChatColorMode(colorMode, Lobby_IgnoreReply);
    }
    memcpy(gLobby->members[gScene.localPlayer].text, text, sizeof(UserText));
    memcpy(gLobby->myText, text, sizeof(UserText));
    memcpy(gPlayerText.raw, text, sizeof(UserText));
    memcpy(gNetUsers[gScene.localPlayer].text, text, sizeof(UserText));
    LobbyReq_HunterCard(text, sizeof(UserText), Lobby_IgnoreReply);
    gLobbyTextDirty = 0;
    return 0;
}

/*
 * Sends the player's online mark (gOnlineMark): the chat colour mode (670F)
 * when its mode changes (3 for mark 8, else 0), then the mark goes
 * into the player's card in gNetUsers, LobbyState.members and gPlayerText,
 * and that byte alone is sent (LobbyReq_HunterCardPart,
 * LOBBY_CMD_HUNTER_CARD_PART, 6192, at offset 2: HunterCard.onlineMark).
 * The replies are ignored. oldMode is used again for the mark's byte, as
 * the game's code reuses that register. gOnlineMark is read through a
 * cast of its address: read by name, the compiler loads it differently.
 * playerCard is made by a cast of gPlayerText.raw for the same reason.
 */
void HunterCard_SendOnlineMark(void)
{
    HunterCard *userCard;
    HunterCard *playerCard;
    u8 oldMode;
    u8 newMode;

    userCard = (HunterCard *)gNetUsers[gScene.localPlayer].text;
    switch (userCard->onlineMark) {
    case 8:
        oldMode = 3;
        break;
    default:
        oldMode = 0;
        break;
    }
    switch (*(u8 *)&gOnlineMark) {
    case 8:
        newMode = 3;
        break;
    default:
        newMode = 0;
        break;
    }
    if (oldMode != newMode) {
        LobbyReq_ChatColorMode(newMode, Lobby_IgnoreReply);
    }
    oldMode = *(u8 *)&gOnlineMark;
    playerCard = (HunterCard *)gPlayerText.raw;
    userCard->onlineMark = oldMode;
    userCard = (HunterCard *)gLobby->members[gScene.localPlayer].text;
    userCard->onlineMark = *(u8 *)&gOnlineMark;
    playerCard->onlineMark = *(u8 *)&gOnlineMark;
    LobbyReq_HunterCardPart(&playerCard->onlineMark, 1, 2, Lobby_IgnoreReply);
}

/*
 * When online, builds and clamps the player's hunter card afresh and, if
 * its count bytes at offset differ from gNetUsers[localPlayer]'s, copies
 * the whole card into LobbyState.members[localPlayer], myText,
 * gPlayerText and gNetUsers[localPlayer] and sends those bytes
 * (LobbyReq_HunterCardPart, 6192; the reply is ignored). The callers pass
 * offsets of HunterCard fields. text holds 0xE0 bytes, but 0x100 are
 * copied from it, as in the game's code.
 */
void HunterCard_SendPart(u16 offset, u16 count)
{
    char text[0xE0];
    u8 *newText;
    u32 pos;

    if (Game_IsOnline() != 0) {
        HunterCard_Build((u8 *)text);
        HunterCard_Sanitize((u8 *)text);
        pos = offset;
        newText = (u8 *)text;
        if (memcmp(&gNetUsers[gScene.localPlayer].text[pos], newText + pos, count) != 0) {
            memcpy(gLobby->members[gScene.localPlayer].text, newText, sizeof(UserText));
            memcpy(gLobby->myText, newText, sizeof(UserText));
            memcpy(gPlayerText.raw, newText, sizeof(UserText));
            memcpy(gNetUsers[gScene.localPlayer].text, newText, sizeof(UserText));
            newText += pos;
            LobbyReq_HunterCardPart(newText, count, offset, Lobby_IgnoreReply);
        }
    }
}
