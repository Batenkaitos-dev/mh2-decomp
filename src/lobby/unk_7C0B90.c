#include "common.h"
#include "lobby_net.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"

s32 LobbyNet_GetErrText(char *);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 LobbyNet_Get6891(struct Net6891 *dst);
extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
s32 LobbyReq_6891(void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_6891Done(NetMsg m, NetMsg *pm);

void Lobby_Apply6884(void);
s32 LobbyStep_6891(void);
void Player_LoadFromSave(Player *pl);
void LevelList_Begin(void);
s32 LobbyStep_Level0List(void);
void Lobby_Disconnect(s32);
s32 LobbyStep_Level1List(s32 a);
s32 LobbyStep_630A(void);
s32 LobbyStep_DownloadQuests(void);
void Town_Reset(void);
s32 LobbyScreen_EnterSeq(s32 a);
void LobbyTown_BeginAtStep3(void);
extern NetUser gNetUsers[10];
extern u8 gLobbyScreenMode;
extern LobbyMenu *gLobbyMenu;
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void TownMsg_SendArea(u16 id);
s32 LobbyText_Update(void);
void LobbySe_Play(s32 a);
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
extern char gPlayerIds[];                 /* sub_main's copies of the four players' ids (8 each), */
extern char gMemberNames[];                 /* names (0x11 each) */
extern char gPresetChars[];                 /* and texts (0x100 each) */
void Net_SetHandler(s32 i, void (*f)(NetMsg, NetMsg *));
void LobbyRoute_6884(NetMsg m, NetMsg *pm);
void LobbyRoute_Players(NetMsg m, NetMsg *pm);
void LobbyRoute_6003(NetMsg m, NetMsg *pm);
void LobbyRoute_6702(NetMsg m, NetMsg *pm);
void LobbyRoute_670C(NetMsg m, NetMsg *pm);
void LobbyRoute_6705(NetMsg m, NetMsg *pm);
void LobbyRoute_OnMsg05(NetMsg m, NetMsg *pm);
void LobbyRoute_6005(NetMsg m, NetMsg *pm);
void LobbyRoute_6007(NetMsg m, NetMsg *pm);
void LobbyRoute_6205(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6204(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6206(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6302(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6304(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_630D(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6303(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6308(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_631B(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_UserIn(NetMsg m, NetMsg *pm);
void LobbyRoute_UserOut(NetMsg m, NetMsg *pm);
void LobbyRoute_6402(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6404(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_650A(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6403(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_640B(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6405(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_640D(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_650C(NetMsg unused0, NetMsg *unused1);
void LobbyRoute_6307(NetMsg m, NetMsg *pm);
void LobbyRoute_640C(NetMsg m, NetMsg *pm);
void LobbyRoute_6505(NetMsg m, NetMsg *pm);
void LobbyRoute_In36(NetMsg m, NetMsg *pm);
void LobbyRoute_Out37(NetMsg m, NetMsg *pm);
void LobbyRoute_6412(NetMsg m, NetMsg *pm);
void LobbyRoute_Chat(NetMsg m, NetMsg *pm);
void LobbyRoute_6191(NetMsg m, NetMsg *pm);
void LobbyRoute_6193(NetMsg m, NetMsg *pm);

/* prototypes */
/* end of prototypes */

void LobbyEnter_Run(void)
{
    u8 *seq;
    LobbyState *p;

    seq = &gLobby->phaseStep;
    switch (*seq) {
    case 0:
        (*seq)++;
        gLobby->seqStep = 0;
    case 1:
        switch (LobbyStep_6891()) {
        case 0:
            p = gLobby;
            if (p->groupJoin != 0) {
                p->phaseStep++;
                gLobby->seqStep = 0;
                gLobby->unk3969 = 0;
                Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
                LobbyText_Update();
            } else if (gLobbyLevel0Sel == 0 || gLobbyLevel1Sel == 0) {
                LevelList_Begin();
                gLobby->unk3969 = 0;
            } else {
                p->phaseStep++;
                gLobby->seqStep = 0;
                gLobby->unk3969 = 1;
                Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
                LobbyText_Update();
            }
            break;
        case 1:
            LevelList_Begin();
            gLobby->unk3969 = 0;
            break;
        }
        break;
    case 2:
        switch (LobbyStep_Level0List()) {
        case 0:
            p = gLobby;
            if (p->groupJoin != 0) {
                LevelList_Begin();
            } else {
                p->phaseStep++;
                gLobby->seqStep = 0;
            }
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 3:
        switch (LobbyStep_Level1List(0)) {
        case 0:
            gLobby->phaseStep++;
            memset(gLobby->members, 0, sizeof(gLobby->members));
            memset(gNetUsers, 0, sizeof(gNetUsers));
            memset(gLobby->pcLoaded, 0, sizeof(gLobby->pcLoaded));
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 4:
        switch (LobbyStep_630A()) {
        case 0:
            gLobby->phaseStep++;
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 5:
        switch (LobbyStep_DownloadQuests()) {
        case 1:
        case 0:
            gLobby->phaseStep++;
            gLobby->seqStep = 0;
            gLobbyMenu->redraw = 1;
            Town_Reset();
            return;
        }
        break;
    case 6:
        gLobbyMenu->hideLayers = 1;
        switch (LobbyScreen_EnterSeq(5)) {
        case 0:
            gLobbyScreenMode = 2;
            gLobby->connected = 1;
            LobbyTown_BeginAtStep3();
            break;
        }
        break;
    }
    gLobbyMenu->redraw = 1;
}

/* sends 6891 */
s32 LobbyStep_6891(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = 3;
        LobbyMenu_StartTimer();
        LobbyReq_6891(LobbyStep_6891Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

/* 6891s reply (u16 x3): the first two into gLobbyLevel0Sel, gLobbyLevel1Sel */
void LobbyStep_6891Done(NetMsg m, NetMsg *pm)
{
    u16 v[3];
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 3) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->seqStep++;
                LobbyNet_Get6891((struct Net6891 *) v);
                gLobbyLevel0Sel = v[0];
                gLobbyLevel1Sel = v[1];
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* registers the lobby screen's route handlers (Net_SetHandler: route,
 * handler) */
void LobbyRoute_Register(void)
{
    Net_SetHandler(1, LobbyRoute_Players);
    Net_SetHandler(2, LobbyRoute_6003);
    Net_SetHandler(11, LobbyRoute_6702);
    Net_SetHandler(12, LobbyRoute_670C);
    Net_SetHandler(3, LobbyRoute_6705);
    Net_SetHandler(5, LobbyRoute_OnMsg05);
    Net_SetHandler(6, LobbyRoute_6005);
    Net_SetHandler(7, LobbyRoute_6007);
    Net_SetHandler(17, LobbyRoute_6205);
    Net_SetHandler(16, LobbyRoute_6204);
    Net_SetHandler(18, LobbyRoute_6206);
    Net_SetHandler(20, LobbyRoute_6302);
    Net_SetHandler(22, LobbyRoute_6304);
    Net_SetHandler(25, LobbyRoute_630D);
    Net_SetHandler(21, LobbyRoute_6303);
    Net_SetHandler(23, LobbyRoute_6308);
    Net_SetHandler(51, LobbyRoute_631B);
    Net_SetHandler(26, LobbyRoute_UserIn);
    Net_SetHandler(27, LobbyRoute_UserOut);
    Net_SetHandler(29, LobbyRoute_6402);
    Net_SetHandler(31, LobbyRoute_6404);
    Net_SetHandler(35, LobbyRoute_650A);
    Net_SetHandler(30, LobbyRoute_6403);
    Net_SetHandler(33, LobbyRoute_640B);
    Net_SetHandler(34, LobbyRoute_6405);
    Net_SetHandler(32, LobbyRoute_640D);
    Net_SetHandler(49, LobbyRoute_650C);
    Net_SetHandler(8, LobbyRoute_6307);
    Net_SetHandler(9, LobbyRoute_640C);
    Net_SetHandler(10, LobbyRoute_6505);
    Net_SetHandler(36, LobbyRoute_In36);
    Net_SetHandler(37, LobbyRoute_Out37);
    Net_SetHandler(38, LobbyRoute_6412);
    Net_SetHandler(13, LobbyRoute_Chat);
    Net_SetHandler(44, LobbyRoute_6191);
    Net_SetHandler(46, LobbyRoute_6193);
    Net_SetHandler(48, LobbyRoute_6884);
}

void LobbyRoute_6884(NetMsg m, NetMsg *pm)
{
    if (gLobby->phase != 5) {
        Lobby_Apply6884();
    }
}

void LobbyRoute_Players(NetMsg m, NetMsg *pm)
{
    s8 i;
    char *id;
    s32 k;
    char *name;
    char *text;

    gLobby->playersListed = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
    TownMsg_SendArea(0);
    gPlayers[gScene.localPlayer].questStatus = 0;
    gPlayerText.card.arenaRole = 0;
    LobbyText_Update();
    if (gLobby->questPaid == 0) {
        LobbySe_Play(0x14);
    }
    for (i = 0, id = gPlayerIds, k = 0, name = gMemberNames, text = gPresetChars; i < 4; i++) {
        memset(id, 0, 8);
        memcpy(id, (u8 *)gLobby + k + 0x5A6, 8);
        memcpy(name, (u8 *)gLobby + k + 0x5AE, 0x11);
        memcpy(text, (u8 *)gLobby + k + 0x5C2, 0x100);
        id += 8;
        k += sizeof(NetUser);
        name += 0x11;
        text += 0x100;
    }
}

void LobbyRoute_6003(NetMsg m, NetMsg *pm)
{
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        p->serverClose = 1;
        LobbyNet_GetErrText(gLobby->errText);
    }
}
