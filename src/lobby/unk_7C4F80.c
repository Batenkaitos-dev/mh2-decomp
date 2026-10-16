#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_globals.h"
s32 LobbyKddi_WaitPage(void);

/* prototypes */
s32 LobbyNet_GetErrText(char *);
void LobbyDialog_OpenWait(void *arg0);
s32 LobbyNet_Get6712(Net6193 *dst);       /* copies 6712's reply (gLobbyNet.x6712) */
s32 HunterCard_Sanitize(void *);
s32 LobbyNet_Get6711(NetFound6711 **out);      /* the 6711 list (gLobbyNet.found6711) */
extern u8 D_008BD6D0[];             /* a Net6193 copy (0x122 bytes) */
extern u8 gHunterPanel[];             /* its data */
extern NetFound6711 *gFound6711;
extern NetFound6709 *gFound6709;
s32 LobbyNet_Get6709(NetFound6709 **out);
s32 Browser_Update(void);
void LobbyKddi_CloseBrowser(void);
s32 memcmp(void *a, void *b, s32 n);  /* memcmp */
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
extern s8 gBrowserTopMode;
extern char gBrowserUrl[0x100];       /* the URL the browser opens */
void Browser_Init(u8 mode);
void Stream_Stop(s32 ch);
void FlushCache(s32);
void func_007A6460(s8 arg0);
s32 LobbyEntry_InStep9(void);
void Kbd_Cancel(void);
u8 FadeA_GetState(void);
void Game_ResetSubsystems(void);
void NetScene_ToPortalPage3(void);
void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
extern u8 gLobbyLevel0Sel;
extern u8 D_5C8086[];
void LobbyText_Open(s32 msg);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
void Lobby_Disconnect(s32);
s32 Lobby_Strlen(char *s); /* strlen */
void LobbyNet_Get6713(Net6713 *dst);
s32 LobbyReq_6711(NetBlob b, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6712(void *id, u16 a, u16 b, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6713(void *id, void (*onReply)(NetMsg, NetMsg *));
void LobbyUser_OnReply(NetMsg m, NetMsg *pm);
void LobbyUser_On6712Reply(NetMsg m, NetMsg *pm);
void LobbyUser_On6711Reply(NetMsg m, NetMsg *pm);
extern NetBlob gFindNameBlob;
extern char gMyUserId[];              /* the character's id (the save) */
s32 LobbyReq_6707(void *id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6709(NetSearch6709 q, void (*onReply)(NetMsg, NetMsg *));
void LobbyUser_On6709Reply(NetMsg m, NetMsg *pm);
void LobbyLogin_On6209Reply(NetMsg m, NetMsg *pm);
s32 LobbyReq_6209(u16 a, u16 b, u16 c, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyStep_EnterLevel1(void);
s32 LobbyStep_Level1List(s32);
s32 LobbyStep_DownloadQuests(void);
s32 Lobby_HrAtLeast31(void);
void Lobby_ClearHost(void);
extern u8 gLobbyLevel1Sel;
extern u8 D_5C41E7[];
/* end of prototypes */


/* the lobby's web page (unk3943 its step): stops the streams, opens the
 * HTML browser (Browser_Init(1)) at the URL gLobbyWebUrl, keeps the page's
 * data before (gLobbyPageData into gLobbyPageDataPrev: LobbyKddi_WaitPage compares
 * them), waits for it, then returns to the lobby (Lobby_Disconnect(2), 6 or
 * 7) or goes on (Kbd_Cancel, func_007A6460(1)) */
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
        func_007A6460(0);
        break;
    case 2:
        gLobby->webStep++;
        gBrowserTopMode = 0;
        FlushCache(0);
        Browser_Init(1);
        memset(gBrowserUrl, 0, 0x100);
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
            if (LobbyEntry_InStep9() == 1) {
                if (gLobby->serverClose == 1) {
                    Lobby_Disconnect(6);
                } else if (gConnCheckState != 0) {
                    Lobby_Disconnect(7);
                } else {
                    Lobby_Disconnect(2);
                }
            } else {
                gLobby->webStep++;
                Kbd_Cancel();
                func_007A6460(1);
            }
            break;
        case 1:
            if (LobbyEntry_InStep9() == 1) {
                Lobby_Disconnect(5);
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
            gLobby->linkState = 2;
            gConnCheckState = 0;
        }
        break;
    case 6:
        if (FadeA_GetState() != 1) {
            gLobby->webStep = 0;
            Game_ResetSubsystems();
            gLobby->connected = 1;
            gLobby->linkState = 2;
            gLobbyMenu->hideLayers = 0;
            NetScene_ToPortalPage3();
            gConnCheckState = 0;
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

/* waits for the HTML browser opened on gLobbyMenu's page: 1 (it ended well:
 * the page's result copied from gLobbyPageData to gLobbyPageDataPrev, unk3942 set if
 * memcmp found a difference) returns 0, -1 returns 0, -100 returns 1,
 * else 2 while it runs */
s32 LobbyKddi_WaitPage(void)
{
    gLobbyMenu->hideLayers = 1;
    switch (Browser_Update()) {
    case 0:
        break;
    case 1:
        LobbyKddi_CloseBrowser();
        if (memcmp(&gLobbyPageDataPrev, &gLobbyPageData, 0x1D0) != 0) {
            gLobby->unk3942 = 1;
        } else {
            gLobby->unk3942 = 0;
        }
        FadeA_Start(1);
        return 0;
    case -1:
        LobbyKddi_CloseBrowser();
        FadeA_Start(2);
        return 0;
    case -100:
        LobbyKddi_CloseBrowser();
        FadeA_Start(2);
        return 1;
    }
    return 2;
}

/* enters first-level entry `entry` (6209) and its sub-entry `sub`
 * (gLobbyLevel0Sel and gLobbyLevel1Sel keep them): the entry's condition by
 * D_5C41E7's byte for sub (1: Lobby_HrAtLeast31() must not be 1, 2: must be
 * nonzero; else message 0x88), then the second level (LobbyStep_EnterLevel1;
 * a full one gives message 0x1D) and LobbyStep_DownloadQuests; returns 2 while
 * running, 0 when entered, 1 when it failed */
s32 LobbyLogin_EnterEntry(s32 entry, s32 sub)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 14;
        LobbyReq_6209(entry, 0, 0, LobbyLogin_On6209Reply);
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
            return 0;
        } else {
            gLobby->seqStep = 4;
        }
        gLobbyLevel0Sel = entry;
        gLobbyLevel1Sel = sub;
        break;
    case 3:
        switch (LobbyStep_Level1List(sub)) {
        case 0:
            if (sub == 0) {
                gLobby->seqStep = 0;
                gLobbyLevel0Sel = entry;
                Lobby_ClearHost();
                return 0;
            }
            gLobby->seqStep++;
            break;
        case 1:
            gLobby->seqStep = 0;
            return 1;
        }
        break;
    case 4:
        switch (D_5C41E7[sub * 0x164]) {
        case 0:
            gLobby->seqStep++;
            gLobbyLevel1Sel = sub;
            break;
        case 1:
            if (Lobby_HrAtLeast31() == 1) {
                gLobby->seqStep = 0;
                LobbyText_Open(0x88);
                gLobbyLevel1Sel = 0;
                gLobby->errText[0] = 0;
                return 1;
            }
            gLobby->seqStep++;
            gLobbyLevel1Sel = sub;
            break;
        case 2:
            if (Lobby_HrAtLeast31() == 0) {
                gLobby->seqStep = 0;
                LobbyText_Open(0x88);
                gLobbyLevel1Sel = 0;
                gLobby->errText[0] = 0;
                return 1;
            }
            gLobby->seqStep++;
            gLobbyLevel1Sel = sub;
            break;
        default:
            gLobby->seqStep = 0;
            LobbyText_Open(0x88);
            gLobbyLevel1Sel = 0;
            return 1;
        }
        break;
    case 5:
        switch (LobbyStep_EnterLevel1()) {
        case 0:
            gLobby->seqStep++;
            break;
        case 1:
            gLobby->seqStep = 7;
            LobbyText_Open(0x1D);
            break;
        }
        break;
    case 6:
        switch (LobbyStep_DownloadQuests()) {
        case 0:
            gLobby->seqStep = 0;
            return 0;
        case 1:
            gLobby->seqStep = 0;
            return 0;
        }
        break;
    case 7:
        switch (LobbyStep_Level1List(0)) {
        case 0:
        case 1:
            gLobby->seqStep = 0;
            return 1;
        }
        break;
    default:
        Lobby_Disconnect(5);
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

void LobbyLogin_On6209Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 14) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText(gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = 99;
            }
        }
    }
}

void LobbyUser_On6712Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 17) {
            *s = 0;
            LobbyNet_Get6712((Net6193 *) D_008BD6D0);
            HunterCard_Sanitize(gHunterPanel);
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText(gLobby->errText);
            }
        }
    }
}

/* fetches user `id`'s details: 6713 (the user's record, into D_008BD6D0 by
 * LobbyUser_OnReply... the reply callbacks), then 6712 (0xD8, 0) with `out`
 * cleared first (0x3CC bytes, LobbyNet_Get6713); returns 2 while running, 0
 * when done, 1 when it failed (the server's error text shown) */
s32 LobbyUser_FetchDetails(void *out, void *id)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 17;
        LobbyReq_6713(id, LobbyUser_OnReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        memset(out, 0, 0x3CC);
        LobbyNet_Get6713(out);
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 17;
        LobbyReq_6712(id, 0xD8, 0, LobbyUser_On6712Reply);
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    case 4:
        gLobby->seqStep = 0;
        return 0;
    default:
        Lobby_Disconnect(5);
    case 99:
        gLobby->seqStep = 0;
        LobbyNet_GetErrText(gLobby->errText);
        LobbyDialog_OpenWait(gLobby->errText);
        return 1;
    }
    return 2;
}

/* looks up user `id` (at least 6 characters): 6707; returns 3 at once
 * for the character's own id (gMyUserId), 2 while running, 0 when done, 1
 * when it failed */
s32 LobbyUser_FindById(s8 *id)
{
    switch (gLobby->seqStep) {
    case 0:
        if (id[0] == 0 || Lobby_Strlen((char *) id) <= 5) {
            gLobby->seqStep = 99;
            break;
        }
        if (memcmp(gMyUserId, id, 8) == 0) {
            return 3;
        }
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 17;
        LobbyReq_6707(id, LobbyUser_OnReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    default:
        Lobby_Disconnect(5);
    case 99:
        gLobby->seqStep = 0;
        LobbyNet_GetErrText(gLobby->errText);
        return 1;
    }
    return 2;
}

void LobbyUser_OnReply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 17) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* searches users by `name` (1-16 characters): 6711 with the name as a blob
 * of kind 100 (gFindNameBlob); returns 2 while running, 0 when done, 1 when it
 * failed */
s32 LobbyUser_FindByName(s8 *name)
{
    switch (gLobby->seqStep) {
    case 0:
        if (name[0] == 0 || Lobby_Strlen((char *) name) > 16) {
            gLobby->seqStep = 99;
            break;
        }
        gLobby->seqStep++;
        gFindNameBlob.kind = 100;
        gFindNameBlob.data = name;
        gFindNameBlob.n = Lobby_Strlen((char *) name);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 28;
        LobbyReq_6711(gFindNameBlob, LobbyUser_On6711Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    default:
        Lobby_Disconnect(5);
    case 99:
        gLobby->seqStep = 0;
        LobbyNet_GetErrText(gLobby->errText);
        LobbyDialog_OpenWait(gLobby->errText);
        return 1;
    }
    return 2;
}

void LobbyUser_On6711Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 28) {
            *s = 0;
            LobbyNet_Get6711(&gFound6711);
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* searches users by n conditions (6709, NetSearch6709 with unk0 100);
 * returns 2 while running, 0 when done, 1 when it failed */
s32 LobbyUser_Search(NetCond6709 *cond, s8 n)
{
    NetSearch6709 q;
    s32 i;

    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        q.unk0 = 100;
        q.n = n;
        for (i = 0; i < n; i++) {
            q.cond[i] = *cond++;
        }
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 17;
        LobbyReq_6709(q, LobbyUser_On6709Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    default:
        Lobby_Disconnect(5);
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

void LobbyUser_On6709Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 17) {
            *s = 0;
            LobbyNet_Get6709(&gFound6709);
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText(gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/* whether the character may enter entry `i` (D_5C8086: 0x164-byte records,
 * their first byte a rank class): its HR (gPlayerText.card.hr) must be
 * within the class's range; the entry the player is in (gLobbyLevel0Sel) only
 * when `again` is set */
s32 Lobby_CanEnterByRank(s32 i, s32 again)
{
    u16 *hr = &gPlayerText.card.hr;
    s16 *r;

    if (i != 0 && i == gLobbyLevel0Sel && again == 0) {
        LobbyText_Open(0x18);
        return 0;
    }
    r = (s16 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20 + (s8)(D_5C8086[i * 0x164] - 1) * 4);
    if (*hr < r[0x26C / 2]) {
        LobbyText_Open(0xF);
        return 0;
    }
    if (r[0x26E / 2] < *hr) {
        LobbyText_Open(0xF);
        return 0;
    }
    return 1;
}
