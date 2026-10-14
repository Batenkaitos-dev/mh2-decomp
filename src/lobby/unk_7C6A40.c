#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"

/* prototypes */
s32 Level1_CanEnter(s32);
void LobbyText_Open(s32 msg);
s32 LobbyStep_EnterLevel1(void);
s32 LobbyStep_DownloadQuests(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void FileQueue_Cancel(void);
s32 LobbyStep_Level1List(s32);
void LobbyMenu_Init(void);
void Lobby_Disconnect(s32);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 LobbyReq_6306(void (*onReply)(NetMsg, NetMsg *));
void Game_ResetSubsystems(void);
void Lobby_LoadTextures(void);
void LevelList_Begin(void);
void LobbyVillage_Begin1(void);
void LobbyLevel0_OnLeaveReply(NetMsg m, NetMsg *pm);
void *memset(void *dst, s32 c, u32 n);
extern u8 gMenuWork[];
extern u8 gLobbyLevel1Sel;
extern LobbyMenu *gLobbyMenu;
extern u16 gAreaSearchTimer;
u16 Pad_GetPressedRepeat(s32 pad);
void LobbyDialog_OpenWait(void *);
void Text_SetSize(s32 size);
s32 sprintf(char *, const char *, ...);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Lobby_DrawPressButton(void);
void LobbySe_Play(s32 se);
void LobbyLevel0_OnCountdownReply(NetMsg m, NetMsg *pm);
/* end of prototypes */


/* enters the entry chosen in the menu work (gMenuWork: step at +0): the
 * rank check (Level1_CanEnter on unk18; else message 0x88), the second
 * level, LobbyStep_DownloadQuests; returns 2 while running, 0 done, 1 failed */
s32 LobbyLevel1_Enter(void)
{
    s8 *st = (s8 *)gMenuWork;

    switch (*st) {
    case 0:
        if (Level1_CanEnter(gLobby->land) == 1) {
            (*st)++;
            break;
        }
        *st = 0;
        gLobbyLevel1Sel = 0;
        LobbyText_Open(0x88);
        return 1;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_EnterLevel1()) {
        case 0:
            (*st)++;
            break;
        case 1:
            gLobbyLevel1Sel = 0;
            return 1;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_DownloadQuests()) {
        case 0:
        case 1:
            LobbyMenu_SetDraw(0, 0, 0, 0);
            memset(gLobby->pcLoaded, 0, 10);
            FileQueue_Cancel();
            return 0;
        }
        break;
    }
    return 2;
}

/* leaves the second level (LobbyStep_Level1List(0)) and LobbyStep_DownloadQuests, then back
 * to the first level's screen (LobbyMenu_Init); 2 while running, 0 done, 1
 * failed */
s32 LobbyLevel1_Leave(void)
{
    gLobbyMenu->redraw = 1;
    switch (gLobby->step) {
    case 0:
        switch (LobbyStep_Level1List(0)) {
        case 0:
            gLobby->step++;
            break;
        case 1:
            return 1;
        }
        break;
    case 1:
        switch (LobbyStep_DownloadQuests()) {
        case 1:
        case 0:
            gLobby->connected = 1;
            LobbyMenu_Init();
            gLobby->step = 0;
            gAreaSearchTimer = 0;
            return 0;
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    return 2;
}

void LobbyLevel0_OnLeaveReply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 16) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* leaves the first level (6306); then back to the start (Lobby_LoadTextures,
 * LevelList_Begin) or, failed, LobbyVillage_Begin1 */
s32 LobbyLevel0_Leave(void)
{
    gLobbyMenu->redraw = 1;
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 16;
        LobbyReq_6306(LobbyLevel0_OnLeaveReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        Game_ResetSubsystems();
        gLobbyMenu->hideLayers = 1;
        Lobby_LoadTextures();
        LevelList_Begin();
        return 0;
    case 99:
        gLobby->seqStep = 0;
        LobbyVillage_Begin1();
        return 0;
    }
    return 2;
}

/* leaving the first level (step its step): the server's text shown with
 * a 300-frame countdown (countdown), 6306; once gLobbyMenu[0x33] is 2, back
 * to the start (or, failed, LobbyVillage_Begin1). gLobbyMenu[0x33]: 0 counting
 * down, 1 waiting for the button 0x20 (message 0x11) */
void LobbyLevel0_LeaveCountdown(void)
{
    char buf[0x10];
    u16 pad;

    pad = Pad_GetPressedRepeat(0);
    gLobbyMenu->redraw = 1;
    switch (gLobby->step) {
    case 0:
        gLobby->step++;
        LobbyMenu_Init();
        gLobby->countdown = 300;
        LobbyDialog_OpenWait(gLobby->errText);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 16;
        LobbyReq_6306(LobbyLevel0_OnCountdownReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        if (gLobbyMenu->promptStep == 2) {
            Game_ResetSubsystems();
            gLobbyMenu->hideLayers = 1;
            Lobby_LoadTextures();
            LevelList_Begin();
            return;
        }
        break;
    case 99:
        if (gLobbyMenu->promptStep == 2) {
            LobbyVillage_Begin1();
            return;
        }
        break;
    }
    switch (gLobbyMenu->promptStep) {
    case 0:
        if (--gLobby->countdown == 0) {
            gLobbyMenu->promptStep++;
        } else {
            Text_SetSize(5);
            sprintf(buf, "%3d", gLobby->countdown / 30);
            Text_DrawShadow(0x1D6, 0x54, 1, 5, buf);
        }
        break;
    case 1:
        Lobby_DrawPressButton();
        if (pad & 0x20) {
            gLobbyMenu->promptStep++;
            LobbyText_Open(0x11);
            LobbySe_Play(0);
        }
        break;
    }
}

void LobbyLevel0_OnCountdownReply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 16) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->step)++;
            } else {
                gLobby->step = 99;
            }
        }
    }
}
