#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "scene.h"

s32 Lobby_HandleLeaveRequest(void);
extern void (*gLobbyRoomSteps[])(void);
extern LobbyMenu *gLobbyMenu;
void Loading_SetMessage(u32 mode);
void LobbyStream_FadeOut(s32 a);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
void Lobby_Disconnect(s32);
void LobbyScreen_ClearRequests(void);
s32 LobbyReq_6002(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6504(s32 arg0, void (*onReply)(NetMsg, NetMsg *));
void LobbyRoom_6002Done(NetMsg m, NetMsg *pm);
void LobbyStep_6504Done(NetMsg m, NetMsg *pm);
void Game_ResetSubsystems(void);
s32 LobbyNet_GetErrText(char *);
void LobbyText_Open(s32 a);
void LobbyDialog_OpenWait(void *text);
void FadeA_Start(s32 a);
u16 LobbyPad_Get(s32 a);
void LobbySe_Play(s32 a);
extern s32 D_008F8C98;
extern u8 gNetConnStep;
extern u8 D_008F8CB0;
extern u8 gNetEntryMode;
extern u8 gLoginState;
extern u8 gLbsConnectStep;
extern u8 gNetScreenStep;
extern u8 gLoginConnectStep;
extern u8 gNetDropped;
extern s32 gLobbyActive;
extern s32 D_008F8CA0;
extern u8 gNetPartySelf;
s32 NetTask9_Start(void (*onDone)(NetMsg, NetMsg *));
void LobbyRoom_PartyDone(NetMsg m, NetMsg *pm);
s32 LobbyNet_GetParty(NetParty *dst);
s32 ArenaCup_QuestTaken(void);
extern u8 gNetQuestSeeds[];
extern void (*jtbl_00819680[])(void);   /* the closing screen's steps (a data table) */
void LobbyMenu_Init(void);
void Lobby_DrawBlackLine(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Loading_Stop(void);
/* sub_main 0x6ADB1C: two 6-byte ids and two bytes (LobbyRoom_FetchParty swaps the ids) */
typedef struct Sub6ADB1C {
    u8 id0[6];
    u8 id1[6];
    u8 pad[6];
    u8 b12;
    u8 b13;
} Sub6ADB1C;
extern Sub6ADB1C D_6ADB1C;
extern u8 gPlayerIds[10][8];              /* the players' ids (sub_main) */
extern u8 gMemberNames[10][0x11];           /* their names */
extern u8 gPresetChars[10][0x100];          /* their texts */

/* the screen's step (gLobbyRoomSteps[mode]) while no message box is up */
void LobbyRoom_Run(void)
{
    if (Lobby_HandleLeaveRequest() == 0) {
        gLobbyRoomSteps[gLobby->mode]();
        gLobbyMenu->hideLayers = 0;
        if (gLobby->phase != 5) {
            Loading_SetMessage(4);
        }
    }
}

/* step 0: task 9 fetches the room (69xx); then its members become the
 * players (LobbyState and sub_main's tables) */
void LobbyRoom_FetchParty(void)
{
    u8 tmp[8];
    NetParty list;
    s32 i;
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8)(step + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x31;
        NetTask9_Start(LobbyRoom_PartyDone);
        return;
    case 1:
        LobbyMenu_TickTimer();
        return;
    case 2:
        gLobby->mode++;
        gLobby->step = 0;
        LobbyNet_GetParty(&list);
        gLobby->partyCount = list.count;
        gNetPartySelf = list.unk1;
        memcpy(gLobby->roomName, list.name, 0x10);
        memcpy(gNetQuestSeeds, gLobby->roomName, 0xF);
        if (ArenaCup_QuestTaken() == 1) {
            D_6ADB1C.b13 = list.text[0];
            D_6ADB1C.b12 = list.text[2];
            memset(tmp, 0, 7);
            memcpy(tmp, D_6ADB1C.id0, 6);
            memcpy(D_6ADB1C.id0, D_6ADB1C.id1, 6);
            memcpy(D_6ADB1C.id1, tmp, 6);
        }
        for (i = 0; i < gLobby->partyCount; i++) {
            memset(&gLobby->players[i], 0, 0x11C);
            memcpy(gLobby->players[i].id, list.members[i].id, 8);
            memcpy(gLobby->players[i].name, list.members[i].name, 0x10);
            memcpy(gLobby->players[i].text, list.members[i].text, 0x100);
            memset(gPlayerIds[i], 0, 8);
            memcpy(gPlayerIds[i], list.members[i].id, 8);
            memcpy(gMemberNames[i], list.members[i].name, 0x11);
            memcpy(gPresetChars[i], list.members[i].text, 0x100);
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

/* step 1: sends 6002, then resets the connection state */
void LobbyRoom_Send6002(void)
{
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8)(step + 1);
        LobbyStream_FadeOut(0xF);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x32;
        LobbyReq_6002(LobbyRoom_6002Done);
        return;
    case 1:
        LobbyMenu_TickTimer();
        return;
    case 2:
        D_008F8C98 = 0;
        gNetConnStep = 0;
        D_008F8CB0 = 1;
        gNetEntryMode = 3;
        gLoginState = 4;
        gLbsConnectStep = 0;
        gNetScreenStep = 0;
        gLoginConnectStep = 0;
        gNetDropped = 0;
        gLobbyActive = 0;
        D_008F8CA0 = gFrameTick;
        return;
    default:
        Lobby_Disconnect(5);
        return;
    }
}

void LobbyRoom_PartyDone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 0x31) {
            *s = 0;
            if (m.kind == 0) {
                Game_ResetSubsystems();
                gLobby->step++;
            } else {
                gLobby->unk3963 = 1;
                LobbyNet_GetErrText(gLobby->errText);
            }
        }
    }
}

/* 6002's reply */
void LobbyRoom_6002Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 0x32) {
            *s = 0;
            gLobby->step++;
        }
    }
}

/* sends 6504 (1); returns 2 while waiting, 0 when done, 1 on a failure */
s32 LobbyStep_6504(void)
{
    s16 *seq;
    LobbyState *p;
    s16 step;

    p = gLobby;
    seq = &p->seqStep;
    step = *seq;
    switch (step) {
    case 0:
        *seq = (s16)(step + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x2F;
        LobbyReq_6504(1, LobbyStep_6504Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gScene.questPaid = p->questPaid;
        *seq = 0;
        return 0;
    case 99:
        *seq = 0;
        return 1;
    }
    return 2;
}

/* 6504's reply */
void LobbyStep_6504Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 0x2F) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->seqStep++;
                gLobby->entered6504 = 1;
            } else {
                LobbyNet_GetErrText(gLobby->errText);
                gLobby->seqStep = 99;
            }
        }
    }
}

void LobbyRoom_BeginClose(u8 step)
{
    gLobby->phase = 6;
    gLobby->phaseStep = step;
    gLobby->mode = 0;
    gLobby->step = 0;
    LobbyScreen_ClearRequests();
}

/* the closing screen's step (jtbl_00819680[mode], a function table) */
void LobbyRoom_RunClose(void)
{
    jtbl_00819680[gLobby->mode]();
}

/* The first entry of the function table jtbl_00819680: splat had merged it into
 * LobbyRoom_RunClose, the dispatcher before it (lead, round 8). */
void LobbyRoom_Close0(void)
{
    gLobby->mode++;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Lobby_DrawBlackLine, 0, 0, 0);
    FadeA_Start(10);
    Loading_Stop();
}

void LobbyRoom_Close1(void)
{
    gLobby->mode++;
    gLobby->countdown = 0;
    gLobbyMenu->redraw = 1;
    if (gLobby->phaseStep == 0) {
        LobbyText_Open(0x6C);
    } else {
        LobbyDialog_OpenWait(gLobby->errText);
    }
    gLobby->countdown = 0x14;
    FadeA_Start(2);
}

void LobbyRoom_Close2(void)
{
    u16 pad;

    pad = LobbyPad_Get(0);
    gLobbyMenu->redraw = 1;
    if (gLobby->countdown++ >= 300 || (gLobby->countdown >= 60 && (pad & 0x60))) {
        gLobby->mode++;
        FadeA_Start(1);
        LobbySe_Play(8);
    }
}

/* unk_7BDB00's .data (0x819678-0x8196A0) */
void LobbyRoom_Close3(void);
void LobbyRoom_Close4(void);
void (*gLobbyRoomSteps[2])(void) = {
    LobbyRoom_FetchParty, LobbyRoom_Send6002,
};
void (*jtbl_00819680[8])(void) = {
    LobbyRoom_Close0, LobbyRoom_Close1, LobbyRoom_Close2, LobbyRoom_Close3,
    LobbyRoom_Close4, NULL, NULL, NULL,
};
