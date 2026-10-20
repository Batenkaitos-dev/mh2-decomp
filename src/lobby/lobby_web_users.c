/*
 * The lobby's web pages, the move between the town's areas and groups,
 * and the lookups of other users.
 *
 * LobbyWeb_Run shows a web page the server names in the HTML browser
 * (LobbyKddi_WaitPage waits for it), then goes back to the lobby or, during
 * the login, quits it. LobbyLogin_EnterEntry moves the player into an area
 * of the town (level 0) and one of its groups (level 1), within the
 * group's limit on hunter ranks, and downloads the group's quests;
 * Lobby_CanEnterByRank checks an area's hunter rank range first. The
 * LobbyUser_* steps fetch another user's profile and hunter card, or look
 * users up by id, by name or by conditions.
 *
 * Each step runs one request at a time in LobbyState.seqStep: it sends the
 * request with its reply's tag in awaitReply (LOBBY_AWAIT_*), waits under
 * the menu's timer, and the reply handler moves seqStep on, or to
 * LOBBY_SEQ_FAILED. The steps return LOBBY_STEP_BUSY while they run, then
 * LOBBY_STEP_DONE or LOBBY_STEP_FAILED.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "lobby_text.h"
#include "browser.h"
s32 LobbyKddi_WaitPage(void);

/* prototypes */
s32 LobbyNet_GetErrText(char *);
void LobbyDialog_OpenWait(void *arg0);
s32 LobbyNet_GetUserCardFetch(NetUserCardPart *dst);       /* copies 6712's reply (gLobbyNet.userCardFetch) */
s32 HunterCard_Sanitize(void *);
s32 LobbyNet_GetUserFindByName(NetUserFindByNameFound **out);      /* the 6711 list (gLobbyNet.userFindByNameFound) */
extern NetUserCardPart gFetchedCard;   /* the last hunter card part fetched (LobbyUser_FetchDetails) */
extern NetUserFindByNameFound *gUserFindByNameFound;
extern NetUserSearchFound *gUserSearchFound;
s32 LobbyNet_GetUserSearch(NetUserSearchFound **out);
s32 Browser_Update(void);
void LobbyKddi_CloseBrowser(void);
s32 memcmp(void *a, void *b, s32 n);
void FadeA_Start(s32);
extern LobbyMenu *gLobbyMenu;
/* a page's data kept across the browser (0x1D0 bytes, 16-aligned) */
typedef struct LobbyPageData {
    u8 b[0x1D0];
} LobbyPageData __attribute__((aligned(16)));
extern LobbyPageData gLobbyPageDataPrev;   /* the copy before */
extern LobbyPageData gLobbyPageData;   /* the page's */
extern char gLobbyWebUrl[];          /* the page's URL */
extern u8 gConnCheckState;
extern char gBrowserUrl[0x100];       /* the URL the browser opens */
void Browser_Init(u8 mode);
void Stream_Stop(s32 ch);
void FlushCache(s32);
void Town_SetDrawSky(s8 arg0);
s32 LobbyEntry_AtWebPage(void);
void Kbd_Cancel(void);
u8 FadeA_GetState(void);
void Game_ResetSubsystems(void);
void NetScene_BackToPortal(void);
void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
extern u8 gLobbyLevel0Sel;
extern LevelEntry gLobbyLevel0Entries[];
void LobbyText_Open(s32 msg);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
void Lobby_Disconnect(s32);
s32 Lobby_Strlen(char *s);
void LobbyNet_GetUserProfile(NetUserProfile *dst);
s32 LobbyReq_UserFindByName(NetBlob b, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_UserCardFetch(void *id, u16 a, u16 b, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_UserProfile(void *id, void (*onReply)(NetMsg, NetMsg *));
void LobbyUser_OnReply(NetMsg m, NetMsg *pm);
void LobbyUser_OnUserCardFetchReply(NetMsg m, NetMsg *pm);
void LobbyUser_OnUserFindByNameReply(NetMsg m, NetMsg *pm);
extern NetBlob gFindNameBlob;
extern char gMyUserId[];              /* the character's id (the save) */
s32 LobbyReq_UserStatus(void *id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_UserSearch(NetUserSearchQuery q, void (*onReply)(NetMsg, NetMsg *));
void LobbyUser_OnUserSearchReply(NetMsg m, NetMsg *pm);
void LobbyLogin_OnEnterLevelsReply(NetMsg m, NetMsg *pm);
s32 LobbyReq_EnterLevels(u16 a, u16 b, u16 c, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyStep_EnterLevel1(void);
s32 LobbyStep_Level1List(s32);
s32 LobbyStep_DownloadQuests(void);
s32 Lobby_HrAtLeast31(void);
void Lobby_ClearHost(void);
extern u8 gLobbyLevel1Sel;
extern LevelEntry gLobbyLevel1Entries[];
/* end of prototypes */

#define LOBBY_USER_IS_SELF 3        /* LobbyUser_FindById: the id is the player's own */
#define USER_ID_LEN_MIN 6           /* the shortest user id looked up */
#define USER_NAME_LEN_MAX 16        /* the longest name looked up */
#define USER_QUERY_KIND 100         /* the kind sent with a user query's name or conditions (the only one seen) */
#define USER_CARD_FETCH_LEN 0xD8    /* the hunter card's bytes fetched with a user's details */

/*
 * The lobby's web page (LobbyState.webStep): stops the streams, opens the
 * HTML browser (Browser_Init(1)) at the URL gLobbyWebUrl, keeps a copy of
 * the page's data from before (LobbyKddi_WaitPage compares them), and waits
 * for the page. During the login the lobby is then quit: back to the
 * portal, or for a server close or a lost line, and a page that posted the
 * login fails it. Otherwise the lobby goes on (the keyboard closed, the sky
 * drawn again), or after a posted login it goes back to the portal's next
 * page.
 */
void LobbyWeb_Run(void)
{
    switch (gLobby->webStep) {
    case 0:
        gLobby->webStep++;
        gLobby->connected = 0;
        gConnCheckState = 0;
        Stream_Stop(0);
        Stream_Stop(1);
    case 1:
        gLobby->webStep++;
        Town_SetDrawSky(0);
        break;
    case 2:
        gLobby->webStep++;
        gBrowserTopMode = BROWSER_TOP_PORTAL;
        FlushCache(0);
        Browser_Init(1);
        memset(gBrowserUrl, 0, sizeof(gBrowserUrl));
        strcpy(gBrowserUrl, gLobbyWebUrl);
        break;
    case 3:
        gLobbyPageDataPrev = gLobbyPageData;
        gLobby->webStep++;
        FadeA_Start(2);
        break;
    case 4:
        switch (LobbyKddi_WaitPage()) {
        case 0:
            if (LobbyEntry_AtWebPage() == 1) {
                if (gLobby->serverClose == SERVER_CLOSE_CLOSED) {
                    Lobby_Disconnect(LOBBY_QUIT_SERVER_CLOSE);
                } else if (gConnCheckState != 0) {
                    Lobby_Disconnect(LOBBY_QUIT_LINK_LOST);
                } else {
                    Lobby_Disconnect(LOBBY_QUIT_TO_PORTAL);
                }
            } else {
                gLobby->webStep++;
                Kbd_Cancel();
                Town_SetDrawSky(1);
            }
            break;
        case 1:
            if (LobbyEntry_AtWebPage() == 1) {
                Lobby_Disconnect(LOBBY_QUIT_FAILED);
            } else {
                gLobby->webStep = 6;
            }
            break;
        }
        break;
    case 5:
        if (FadeA_GetState() != 1) {
            gLobby->webStep = 0;
            gLobby->connected = 1;
            gLobby->linkState = LINK_STATE_WEB_BACK;
            gConnCheckState = 0;
        }
        break;
    case 6:
        if (FadeA_GetState() != 1) {
            gLobby->webStep = 0;
            Game_ResetSubsystems();
            gLobby->connected = 1;
            gLobby->linkState = LINK_STATE_WEB_BACK;
            gLobbyMenu->hideLayers = 0;
            NetScene_BackToPortal();
            gConnCheckState = 0;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/* Waits for the HTML browser, the lobby menu's layers hidden. When it is
 * left (BROWSER_DONE), LobbyState.pageDataChanged says whether the page's
 * data changed, and it returns 0; on an error 0 too; after a posted login
 * (BROWSER_LOGIN) 1. 2 while the page runs. */
s32 LobbyKddi_WaitPage(void)
{
    gLobbyMenu->hideLayers = 1;
    switch (Browser_Update()) {
    case BROWSER_RUNNING:
        break;
    case BROWSER_DONE:
        LobbyKddi_CloseBrowser();
        if (memcmp(&gLobbyPageDataPrev, &gLobbyPageData, sizeof(LobbyPageData)) != 0) {
            gLobby->pageDataChanged = 1;
        } else {
            gLobby->pageDataChanged = 0;
        }
        FadeA_Start(1);
        return 0;
    case BROWSER_FAILED:
        LobbyKddi_CloseBrowser();
        FadeA_Start(2);
        return 0;
    case BROWSER_LOGIN:
        LobbyKddi_CloseBrowser();
        FadeA_Start(2);
        return 1;
    }
    return 2;
}

/*
 * Moves into area `entry` of the town (level 0) and its group `sub` (level
 * 1; 0 for none), which gLobbyLevel0Sel and gLobbyLevel1Sel then hold.
 * Steps: 0 sends the move (LobbyReq_EnterLevels), 1 waits for the reply;
 * 2 with a new area fetches its groups (3), else goes straight to the
 * group; 4 checks the group's limit on hunter ranks (GROUP_LIMIT_*;
 * LOBBY_TEXT_GROUP_RANK_REFUSED when it doesn't match); 5 enters the group (a
 * failure shows LOBBY_TEXT_MOVING and fetches the list again, 7); 6
 * downloads its quests. Returns LOBBY_STEP_BUSY while it runs, then
 * LOBBY_STEP_DONE or LOBBY_STEP_FAILED.
 */
s32 LobbyLogin_EnterEntry(s32 entry, s32 sub)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_ENTER_LEVELS;
        LobbyReq_EnterLevels(entry, 0, 0, LobbyLogin_OnEnterLevelsReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        if (gLobbyLevel0Sel != entry) {
            gLobby->seqStep++;
        } else if (sub == 0) {
            gLobby->seqStep = 0;
            gLobbyLevel0Sel = entry;
            Lobby_ClearHost();
            return LOBBY_STEP_DONE;
        } else {
            gLobby->seqStep = 4;
        }
        gLobbyLevel0Sel = entry;
        gLobbyLevel1Sel = sub;
        break;
    case 3:
        switch (LobbyStep_Level1List(sub)) {
        case LOBBY_STEP_DONE:
            if (sub == 0) {
                gLobby->seqStep = 0;
                gLobbyLevel0Sel = entry;
                Lobby_ClearHost();
                return LOBBY_STEP_DONE;
            }
            gLobby->seqStep++;
            break;
        case LOBBY_STEP_FAILED:
            gLobby->seqStep = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    case 4:
        /* the entry is indexed from the table's address one entry back
         * (ids count from 1): the compiler folds the field's offset into
         * that address, as the game's code does */
        switch ((gLobbyLevel1Entries - 1)[sub].groupInfo[0]) {
        case GROUP_LIMIT_NONE:
            gLobby->seqStep++;
            gLobbyLevel1Sel = sub;
            break;
        case GROUP_LIMIT_HR_BELOW_31:
            if (Lobby_HrAtLeast31() == 1) {
                gLobby->seqStep = 0;
                LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
                gLobbyLevel1Sel = 0;
                gLobby->errText[0] = 0;
                return LOBBY_STEP_FAILED;
            }
            gLobby->seqStep++;
            gLobbyLevel1Sel = sub;
            break;
        case GROUP_LIMIT_HR_31_UP:
            if (Lobby_HrAtLeast31() == 0) {
                gLobby->seqStep = 0;
                LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
                gLobbyLevel1Sel = 0;
                gLobby->errText[0] = 0;
                return LOBBY_STEP_FAILED;
            }
            gLobby->seqStep++;
            gLobbyLevel1Sel = sub;
            break;
        default:
            gLobby->seqStep = 0;
            LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
            gLobbyLevel1Sel = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    case 5:
        switch (LobbyStep_EnterLevel1()) {
        case LOBBY_STEP_DONE:
            gLobby->seqStep++;
            break;
        case LOBBY_STEP_FAILED:
            gLobby->seqStep = 7;
            LobbyText_Open(LOBBY_TEXT_MOVING);
            break;
        }
        break;
    case 6:
        switch (LobbyStep_DownloadQuests()) {
        case LOBBY_STEP_DONE:
            gLobby->seqStep = 0;
            return LOBBY_STEP_DONE;
        case LOBBY_STEP_FAILED:
            gLobby->seqStep = 0;
            return LOBBY_STEP_DONE;
        }
        break;
    case 7:
        switch (LobbyStep_Level1List(0)) {
        case LOBBY_STEP_DONE:
        case LOBBY_STEP_FAILED:
            gLobby->seqStep = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The move's reply: the next step, or the server's error text in a dialog
 * and LOBBY_SEQ_FAILED. Ignored while the lobby is being left. */
void LobbyLogin_OnEnterLevelsReply(NetMsg msg, NetMsg *msgRef)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_ENTER_LEVELS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText(gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* The hunter card's reply: the part into gFetchedCard, its text made safe
 * to show (HunterCard_Sanitize); the next step, or LOBBY_SEQ_FAILED with
 * the server's error text. */
void LobbyUser_OnUserCardFetchReply(NetMsg msg, NetMsg *msgRef)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_USERS) {
            *awaited = LOBBY_AWAIT_NONE;
            LobbyNet_GetUserCardFetch(&gFetchedCard);
            HunterCard_Sanitize(gFetchedCard.data);
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText(gLobby->errText);
            }
        }
    }
}

/* Fetches user `id`'s details: the profile (LobbyReq_UserProfile) into
 * `profile`, cleared first, then the first USER_CARD_FETCH_LEN bytes of
 * the hunter card (into gFetchedCard). Returns LOBBY_STEP_BUSY while it
 * runs, then LOBBY_STEP_DONE, or LOBBY_STEP_FAILED with the server's error
 * text shown. */
s32 LobbyUser_FetchDetails(void *profile, void *id)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_USERS;
        LobbyReq_UserProfile(id, LobbyUser_OnReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        memset(profile, 0, sizeof(NetUserProfile));
        LobbyNet_GetUserProfile(profile);
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_USERS;
        LobbyReq_UserCardFetch(id, USER_CARD_FETCH_LEN, 0, LobbyUser_OnUserCardFetchReply);
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    case 4:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        LobbyNet_GetErrText(gLobby->errText);
        LobbyDialog_OpenWait(gLobby->errText);
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* Looks up user `id` (LobbyReq_UserStatus): an id shorter than
 * USER_ID_LEN_MIN fails, and the player's own id returns
 * LOBBY_USER_IS_SELF at once. Returns LOBBY_STEP_BUSY while it runs, then
 * LOBBY_STEP_DONE or LOBBY_STEP_FAILED. */
s32 LobbyUser_FindById(s8 *id)
{
    switch (gLobby->seqStep) {
    case 0:
        if (id[0] == 0 || Lobby_Strlen((char *) id) < USER_ID_LEN_MIN) {
            gLobby->seqStep = LOBBY_SEQ_FAILED;
            break;
        }
        if (memcmp(gMyUserId, id, LOBBY_USER_ID_LEN) == 0) {
            return LOBBY_USER_IS_SELF;
        }
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_USERS;
        LobbyReq_UserStatus(id, LobbyUser_OnReply);
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
        LobbyNet_GetErrText(gLobby->errText);
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The reply to a user's profile or status: the next step, or
 * LOBBY_SEQ_FAILED. */
void LobbyUser_OnReply(NetMsg msg, NetMsg *msgRef)
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

/* Looks users up by `name` (1 to USER_NAME_LEN_MAX characters), sent as
 * a blob (gFindNameBlob). Returns LOBBY_STEP_BUSY while it runs, then
 * LOBBY_STEP_DONE, or LOBBY_STEP_FAILED with the server's error text
 * shown. */
s32 LobbyUser_FindByName(s8 *name)
{
    switch (gLobby->seqStep) {
    case 0:
        if (name[0] == 0 || Lobby_Strlen((char *) name) > USER_NAME_LEN_MAX) {
            gLobby->seqStep = LOBBY_SEQ_FAILED;
            break;
        }
        gLobby->seqStep++;
        gFindNameBlob.kind = USER_QUERY_KIND;
        gFindNameBlob.data = name;
        gFindNameBlob.n = Lobby_Strlen((char *) name);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_USER_FIND_BY_NAME;
        LobbyReq_UserFindByName(gFindNameBlob, LobbyUser_OnUserFindByNameReply);
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
        LobbyNet_GetErrText(gLobby->errText);
        LobbyDialog_OpenWait(gLobby->errText);
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The reply to the name lookup: the users found (gUserFindByNameFound);
 * the next step, or LOBBY_SEQ_FAILED. */
void LobbyUser_OnUserFindByNameReply(NetMsg msg, NetMsg *msgRef)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_USER_FIND_BY_NAME) {
            *awaited = LOBBY_AWAIT_NONE;
            LobbyNet_GetUserFindByName(&gUserFindByNameFound);
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Searches users by `n` conditions (LobbyReq_UserSearch). Returns
 * LOBBY_STEP_BUSY while it runs, then LOBBY_STEP_DONE or
 * LOBBY_STEP_FAILED. */
s32 LobbyUser_Search(NetUserSearchCond *cond, s8 n)
{
    NetUserSearchQuery query;
    s32 i;

    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        query.kind = USER_QUERY_KIND;
        query.n = n;
        for (i = 0; i < n; i++) {
            query.cond[i] = *cond++;
        }
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_USERS;
        LobbyReq_UserSearch(query, LobbyUser_OnUserSearchReply);
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

/* The search's reply: the users found (gUserSearchFound); the next step,
 * or LOBBY_SEQ_FAILED with the server's error text in a dialog. */
void LobbyUser_OnUserSearchReply(NetMsg msg, NetMsg *msgRef)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_USERS) {
            *awaited = LOBBY_AWAIT_NONE;
            LobbyNet_GetUserSearch(&gUserSearchFound);
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText(gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/*
 * 1 if the player may move into area `area`: not the area the player is
 * in (LOBBY_TEXT_AREA_CURRENT) unless `again` is set, and the hunter rank
 * within the range the quest list gives the area's kind
 * (RoomData.areaKindHr; LOBBY_TEXT_AREA_RANK). The area's entry is
 * indexed from one entry back (ids count from 1), and the range's address
 * is stepped by the kind first and the field read at its offset from
 * there, as the game's code computes them.
 */
s32 Lobby_CanEnterByRank(s32 area, s32 again)
{
    u16 *rank = &gPlayerText.card.hr;
    RoomData *list;

    if (area != 0 && area == gLobbyLevel0Sel && again == 0) {
        LobbyText_Open(LOBBY_TEXT_AREA_CURRENT);
        return 0;
    }
    list = (RoomData *)((s32)(gLobbyWorkBuf + QDL_OFFSET) + QDL_FILE_COUNT * sizeof(NetDlFile) + (s8)((gLobbyLevel0Entries - 1)[area].kind - 1) * sizeof(list->areaKindHr[0]));
    if (*rank < list->areaKindHr[0][0]) {
        LobbyText_Open(LOBBY_TEXT_AREA_RANK);
        return 0;
    }
    if (list->areaKindHr[0][1] < *rank) {
        LobbyText_Open(LOBBY_TEXT_AREA_RANK);
        return 0;
    }
    return 1;
}
