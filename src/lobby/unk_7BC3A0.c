/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "item.h"
#include "save.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "scene.h"
#include "player.h"

/* prototypes */
void Money_PayPending(u8 silent);
void EquipPreview_Show(u8 on);
void ServerList_ClearTried(void);
void ServerList_MarkTried(void);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 Lobby_SendHeld(void);
s32 LobbyLevel_SelId(s32 arg0);
void Lobby_ResetIdleTimer(s32 *arg0);
void Lobby_ClearHost(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Lobby_DrawBlackLine(void);
s32 Lobby_HasQuest(void);
void LobbyTime_Tick(void);
void LobbyTime_SetClock(u32 time);
f32 LobbyTime_GetDayFrac(void);
f32 LobbyTime_GetDayLen30(void);
s32 LobbyTime_IsAm(void);
s32 LobbyTime_IsPm(void);
void Lobby_LoadTextures(void);
void Lobby_UpdatePlayerText(void);
void LobbyLeave_Reset(void);
void LobbyExit_Draw(MenuTask *layer);
void LobbyPkt_6508Send(void);
void StageModels_Free(void);
void Loading_Reset(void);
extern u8 gLobbyTextDirty;
extern s8 gServerSkipTried;
/* end of prototypes */
s32 Lobby_ReceiveAll(s32 sock);
extern u8 gChatTargets[];
void LobbyText_Open(s32 a);
void LobbyDialog_OpenWait(void *text);
void FileQueue_Cancel(void);
void Lobby_SetPlace3928(s32 a);
extern ItemStack gPouchBackup[20];

s32 Lobby_GetPlace3928(void);
s32 Game_IsOnline(void);
void *memset(void *dst, s32 c, u32 n);
extern LobbyMenu *gLobbyMenu;
extern MenuTask gLobbyMenuLayer0;
extern MenuTask gLobbyMenuLayer1;
extern MenuTask gLobbyMenuLayer2;
extern MenuTask gLobbyMenuLayer3;
extern MenuTask gLobbyMenuLayer4;
extern MenuTask gLobbyMenuLayer5;
extern MenuTask gLobbyMenuLayer6;
extern MenuTask gLobbyMenuLayer7;
extern M2C_UNK VillageClock_Draw;
extern M2C_UNK LobbyMenu_DrawMemberNames;
extern M2C_UNK Lobby_DrawNameLayer;
extern M2C_UNK LobbyMenu_DrawDialog;
extern u8 gLobbyLeaveStep;
void Lobby_Disconnect(s32);
void Chat_ClearTargets(void);
extern u8 gMemberNamesOff;
void Gfx_DrawBlackLineFixed(void);
void LobbyWorld_Tick(void);
void LobbyWorld_SetClock(u32 time);
f32 LobbyWorld_GetDayFrac(void);
f32 LobbyWorld_GetDayLen30(void);
extern u32 gLobbyTime;
void Hud_LoadTextures(void);
void NetMode_LoadTextures(s32);
extern s8 gNetDisconnectStep;
void Text_SetSize(s32);
s32 LobbyPkt_6508(void);

void LobbyMenu_StartTimer(void) {
    gLobby->timer = 0xE10;
}

s32 LobbyMenu_TickTimer(void) {
    gLobby->timer = gLobby->timer - 1;
    if (gLobby->timer < 0) {
        Lobby_Disconnect(5);
        return 1;
    }
    return 0;
}

void LobbyMenu_Init(void)
{
    memset(gLobbyMenu, 0, 0x44);
    gLobbyMenu->layer[0] = &gLobbyMenuLayer0;
    gLobbyMenu->layer[1] = &gLobbyMenuLayer1;
    gLobbyMenu->layer[2] = &gLobbyMenuLayer2;
    gLobbyMenu->layer[3] = &gLobbyMenuLayer3;
    gLobbyMenu->layer[4] = &gLobbyMenuLayer4;
    gLobbyMenu->layer[4]->draw = &LobbyMenu_DrawDialog;
    gLobbyMenu->layer[5] = &gLobbyMenuLayer5;
    gLobbyMenu->layer[5]->draw = &Lobby_DrawNameLayer;
    gLobbyMenu->layer[6] = &gLobbyMenuLayer6;
    gLobbyMenu->layer[6]->draw = &LobbyMenu_DrawMemberNames;
    gLobbyMenu->layer[7] = &gLobbyMenuLayer7;
    gLobbyMenu->layer[7]->draw = &VillageClock_Draw;
    gLobbyMenu->layer[0]->textSize = 0;
    gLobbyMenu->layer[5]->textSize = 6;
    gLobbyMenu->layer[6]->textSize = 0;
    gLobbyMenu->layer[7]->textSize = 0;
    gLobbyMenu->layer[1]->textSize = 1;
    gLobbyMenu->layer[2]->textSize = 2;
    gLobbyMenu->layer[3]->textSize = 3;
    gLobbyMenu->layer[4]->textSize = 5;
    if (Lobby_GetPlace3928() == 0) {
        Game_IsOnline();
    }
}

void NetReq_ClearAll(void);
void NetTask_ClearAll(void);

/* clears the lobby's request flags, then every request and task */
void LobbyScreen_ClearRequests(void)
{
    memset(&gLobby->leave6307, 0, 3);
    gLobby->playersListed = 0;
    gLobby->unk3963 = 0;
    gLobby->awaitReply = 0;
    NetReq_ClearAll();
    NetTask_ClearAll();
}

s32 Lobby_SendHeld(void) {
    return gLobby->playersListed != 0;
}

s32 LobbyDialog_Clear(void);
void LobbyNet_Reset(void);
void Lobby_ResetTimers(void);
void Rate_ClearSnapshot(void);
extern s8 gConnCheckState;
extern LobbyMenu gLobbyMenuData;                 /* the menu state gLobbyMenu points to */
extern LobbyState gLobbyState;
extern u8 D_008BFB10[];
extern u16 gFriendCheckTimers[12];
/* the download records in the save's online area (unk_7B3140.c) */
typedef struct DlRec {
    u32 a;
    u32 b;
} DlRec;
#define QDL_BASE ((DlRec *)(*(s32 *)&gLobbyWorkBuf + 0x6EF4))
extern s8 gFriendStatus[50];
extern u8 gLobbyLocText[];

/* the lobby's start: the save's online area (*0x6AE020) cleared, the
 * lobby state and the menu state reset, the tables emptied */
void Lobby_ResetState(void)
{
    s32 i;

    memset(gLobbyWorkBuf, 0, 0x6EF4);
    memset(QDL_BASE, 0, 0x20);
    memset(QDL_BASE + 4, 0, 0x11C4);
    memset((u8 *)((s32)(QDL_BASE + 4) + 0x11C4), 0, 0xF00);
    gLobbyMenu = &gLobbyMenuData;
    gLobby = &gLobbyState;
    gLobby->phase = 0;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    gLobby->taskStep = 0;
    Lobby_ResetIdleTimer(&gLobby->idleTimer);
    Lobby_ClearHost();
    LobbyDialog_Clear();
    gLobby->recvTimer = 0xE10;
    gLobby->serverClose = 0;
    gLobby->linkState = 0;
    gLobby->unk3968 = 0;
    Lobby_SetPlace3928(9);
    Chat_ClearTargets();
    gLobby->areaLoaded = 0;
    gConnCheckState = 0;
    LobbyNet_Reset();
    memset(D_008BFB10, 0, 0x618);
    for (i = 0; i < 12; i++) {
        gFriendCheckTimers[i] = 0xFFFF;
    }
    for (i = 0; i < 50; i++) {
        gFriendStatus[i] = -1;
    }
    Chat_ClearTargets();
    Lobby_ResetTimers();
    Rate_ClearSnapshot();
    memset(gLobbyLocText, 0, 0x80);
}

extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
extern u8 gLobbyLevel2Sel;
s32 LobbyLevel_SelId(s32 arg0) {
    s32 temp_v1;

    temp_v1 = arg0 & 0xFF;
    switch (temp_v1) {                              /* irregular */
    case 0:
        return gLobbyLevel0Sel;
    case 1:
        return gLobbyLevel1Sel;
    case 2:
        return gLobbyLevel2Sel;
    default:
        return 0U;
    }
}

void Lobby_ResetIdleTimer(s32 *timer) {
    *timer = 0x1A5E0;
}

/* receives while the scene waits (up to 100 messages; each one resets the
 * minute timer recvTimer) */
void Lobby_ReceiveWhileWaiting(void)
{
    s32 n;

    if (Game_IsOnline() == 1) {
        n = 0;
        do {
            if (Lobby_ReceiveAll(gNetSock) != 1) {
                break;
            }
            n++;
            gLobby->recvTimer = 0xE10;
        } while (n < 100);
    }
}

void Lobby_ClearHost(void) {
    gLobby->hostId[0] = 0;
    Chat_ClearTargets();
    gMemberNamesOff = gMemberNamesOffSaved;
}

extern LobbyMenu *gLobbyMenu;
extern MenuTask gLobbyMenuLayer0;
extern MenuTask gLobbyMenuLayer1;
extern MenuTask gLobbyMenuLayer2;
extern MenuTask gLobbyMenuLayer3;
/* The four draw layers' callbacks (+0x14 of each layer). */
void LobbyMenu_SetDraw(void *draw0, void *draw1, void *draw2, void *draw3) {
    gLobbyMenu->layer[0] = &gLobbyMenuLayer0;
    gLobbyMenu->layer[0]->draw = draw0;
    gLobbyMenu->layer[1] = &gLobbyMenuLayer1;
    gLobbyMenu->layer[1]->draw = draw1;
    gLobbyMenu->layer[2] = &gLobbyMenuLayer2;
    gLobbyMenu->layer[2]->draw = draw2;
    gLobbyMenu->layer[3] = &gLobbyMenuLayer3;
    gLobbyMenu->layer[3]->draw = draw3;
}

void Lobby_DrawBlackLine(void) {
    Gfx_DrawBlackLineFixed();
}

void *LobbyLevel2_Entry(s32 idx)
{
    return &gLobbyLevel2Entries[(s16)idx];
}

s32 Lobby_HasQuest(void) {
    return gLobby->questTaken != 0;
}

void Lobby_SetPlace3928(s32 arg0)
{
    switch (arg0) {
    case 9:
        gLobby->unk396E = 0;
        break;
    case 0:
    case 1:
    case 2:
        gLobby->unk396E = 1;
        break;
    }
    gLobby->place = arg0;
}

s32 Lobby_GetPlace3928(void) {
    return gLobby->place;
}

void LobbyTime_Tick(void) {
    LobbyWorld_Tick();
}

void LobbyTime_SetClock(u32 time) {
    LobbyWorld_SetClock(time);
}

f32 LobbyTime_GetDayFrac(void) {
    return LobbyWorld_GetDayFrac();
}

f32 LobbyTime_GetDayLen30(void) {
    return LobbyWorld_GetDayLen30();
}

s32 LobbyTime_IsAm(void) {
    return (u32) gLobbyTime < 0xA8C0U;
}

s32 LobbyTime_IsPm(void) {
    return (u32) gLobbyTime >= 0xA8C0U;
}

void Chat_ClearTargets(void)
{
    s8 i;
    u8 *p;

    gLobby->unk3967 = 0;
    for (i = 0, p = gChatTargets; i < 7; i++, p += 8) {
        memset(p, 0, 8);
    }
}

void Lobby_LoadTextures(void) {
    Hud_LoadTextures();
    NetMode_LoadTextures(0);
}

void HunterCard_Build(void *out);
s32 memcmp(void *a, void *b, s32 n);
void *memcpy(void *dst, const void *src, u32 n);
extern NetUser gNetUsers[];

/* the player's text, if it changed, into every copy of it */
void Lobby_UpdatePlayerText(void)
{
    u8 text[0xE0];

    HunterCard_Build(text);
    if (memcmp(text, gPlayerText.raw, 0x100) != 0) {
        memcpy(gLobby->members[gScene.localPlayer].text, text, 0x100);
        memcpy(gLobby->myText, text, 0x100);
        memcpy(gPlayerText.raw, text, 0x100);
        memcpy(gNetUsers[gScene.localPlayer].text, text, 0x100);
    }
}

void Lobby_Disconnect(s32 reason)
{
    gLobby->phase = 5;
    gLobby->phaseStep = 0;
    gLobby->mode = reason;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->seqStep = 0;
    if (gLobbyTextDirty != 0) {
        Lobby_UpdatePlayerText();
        gLobbyTextDirty = 0;
    }
    gLobby->quitReason = reason;
    Money_PayPending(1U);
    EquipPreview_Show(1U);
    StageModels_Free();
    LobbyMenu_SetDraw(0, 0, NULL, 0);
    if ((reason & 0xFF) == 8) {
        gServerSkipTried = 1;
        ServerList_MarkTried();
    } else {
        ServerList_ClearTried();
    }
    Loading_Reset();
}

/* leaving the lobby server (a scene of gLobbyScreens): the step function of
 * mode (0-3 LobbyExit_Steps0, 4-8 LobbyExit_Steps4) */
extern void (*jtbl_00819650[])(void);
void LobbyExit_Run(void)
{
    if (gLobby->mode > 8) {
        gLobby->mode = 0;
    }
    jtbl_00819650[gLobby->mode]();
}


s32 LobbyLeave_Run(u8 kind);
void LobbyLeave_Reset(void);
void LobbyExit_Draw(MenuTask *layer);
void LobbyStream_FadeOut(s32 a);
void FadeA_Start(s32);
u8 FadeA_GetState(void);
void Stream_Stop(s32 ch);
void Kbd_Cancel(void);
void LobbyScreen_ShowResult(s32 k);
s32 NetTcp_Abort(s32 sock);
s32 NetTcp_Delete(s32 *sock);
extern s32 gNetSock;
void LobbySe_Play(s32 se);
void Game_ResetSubsystems(void);
void NetScene_ToState4b(void);
void NetScene_ToState6(void);
extern u8 gNetEntryMode;
u16 LobbyPad_Get(s32 a);
void LobbyStream_Stop(void);
void Hud_StartQuest(void);
void Area_FreeAll(void);
void Lobby_SetBootMode(s32 arg0);
void NetScene_ToNetFileCheck(void);

/* the lobby screen's closing for mode 0-3 (entries 0-3 of
 * jtbl_00819650; LobbyExit_Steps4 is 4-8), step step: the fade, the
 * streams and the scene's music, the leaving request (LobbyLeave_Run, then
 * the socket closed), and the next menu by quitReason */
void LobbyExit_Steps0(void)
{
    switch (gLobby->step) {
    case 0:
        LobbyMenu_Init();
        Lobby_SetPlace3928(9);
        if (gLobby->quitReason != 0) {
            gLobby->step++;
            LobbyMenu_Init();
            LobbyMenu_SetDraw(LobbyExit_Draw, 0, 0, 0);
            LobbyStream_FadeOut(0xF);
            FadeA_Start(0xA);
        } else {
            gLobby->step = 3;
        }
        break;
    case 1:
        if ((u8)FadeA_GetState() != 1) {
            if (gLobby->quitReason != 0) {
                Stream_Stop(0);
                Stream_Stop(1);
                if (gPlayers[gScene.localPlayer].state != 0xF) {
                    FadeA_Start(2);
                    LobbyStream_Stop();
                    Hud_StartQuest();
                    gLobby->step += 2;
                } else {
                    gLobby->step += 2;
                }
            } else {
                gLobby->step += 2;
            }
            Kbd_Cancel();
        }
        break;
    case 2:
        gLobby->step++;
        break;
    case 3:
        gLobby->step++;
        LobbyLeave_Reset();
        Kbd_Cancel();
        if (gLobby->quitReason == 4) {
            LobbyText_Open(0xB);
            FadeA_Start(2);
        } else if (gLobby->quitReason != 0) {
            FadeA_Start(2);
            LobbyText_Open(9);
        }
        Area_FreeAll();
        break;
    case 4:
        gLobby->step++;
    case 5:
        gLobbyMenu->redraw = 1;
        if (LobbyLeave_Run(gLobby->quitReason) != 0) {
            gLobby->step++;
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
        }
        break;
    case 6:
        if ((u8)FadeA_GetState() != 1) {
            gLobby->step++;
            if (gLobby->quitReason != 0) {
                gLobbyMenu->redraw = 1;
                FadeA_Start(1);
            }
        }
        break;
    case 7:
        if ((u8)FadeA_GetState() == 1) {
            if (gLobby->quitReason != 0) {
                gLobbyMenu->redraw = 1;
            }
            return;
        }
        {
            switch (gLobby->quitReason) {
            case 0:
                NetScene_ToState6();
                break;
            case 3:
                Lobby_SetBootMode(2);
                NetScene_ToNetFileCheck();
                break;
            case 2:
                gNetEntryMode = 0;
                gPortalPage = 3;
                NetScene_ToState6();
                break;
            case 1:
                Lobby_SetBootMode(1);
                NetScene_ToNetFileCheck();
                gPortalPage = 0;
                break;
            }
        }
        break;
    }
}

/* the lobby screen's closing (step): the fade and the streams, the
 * leaving request (LobbyLeave_Run), the result shown, then (kind 8) a key
 * press, and back to the network menu or the title */
void LobbyExit_Steps4(void)
{
    u16 pad = LobbyPad_Get(0);

    switch (gLobby->step) {
    case 0:
        LobbyMenu_Init();
        Lobby_SetPlace3928(9);
        gLobbyMenu->hideLayers = 1;
        gLobby->step++;
        LobbyMenu_SetDraw(LobbyExit_Draw, 0, 0, 0);
        if (gLobby->quitReason != 0) {
            LobbyStream_FadeOut(0xF);
        }
        gLobby->step = 4;
        FadeA_Start(0xA);
        break;
    case 1:
        gLobbyMenu->hideLayers = 1;
        if ((u8)FadeA_GetState() != 1) {
            gLobby->step++;
            if (gLobby->quitReason != 0) {
                Stream_Stop(0);
                Stream_Stop(1);
            }
            Kbd_Cancel();
        }
        break;
    case 2:
        gLobbyMenu->hideLayers = 1;
        gLobby->step++;
        gLobby->connected = 0;
        break;
    case 3:
        gLobbyMenu->hideLayers = 1;
        gLobby->connected = 1;
        gLobby->step++;
        break;
    case 4:
        gLobbyMenu->hideLayers = 1;
        if ((u8)FadeA_GetState() != 1) {
            gLobby->step++;
            LobbyLeave_Reset();
            FadeA_Start(2);
            LobbyText_Open(9);
            Kbd_Cancel();
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        gLobby->step++;
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        gLobby->step = 7;
    case 7:
        gLobbyMenu->redraw = 1;
        if (LobbyLeave_Run(gLobby->quitReason) != 0) {
            LobbyScreen_ShowResult((s8)gLobby->quitReason);
            if (gLobby->quitReason == 8) {
                gLobby->step = 9;
                gLobby->countdown = 0x5A;
            } else {
                gLobby->step++;
            }
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            gLobby->step = 0xA;
            LobbySe_Play(0);
            FadeA_Start(1);
        }
        break;
    case 9:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            gLobby->step++;
            FadeA_Start(1);
            LobbySe_Play(0);
        }
        break;
    case 10:
        if ((u8)FadeA_GetState() != 1) {
            Game_ResetSubsystems();
            gLobbyMenu->hideLayers = 1;
            switch (gLobby->quitReason) {
            case 4:
                NetScene_ToState4b();
                gPortalPage = 0;
                break;
            case 5:
            case 6:
            case 7:
            case 8:
                NetScene_ToState6();
                gNetEntryMode = 0;
                gPortalPage = 3;
                break;
            }
        }
        break;
    }
}

/* the message for a result k: 1, 4, 5/7 texts 0xC, 0xA, 0xD; 6/8 the
 * server's error text */
void LobbyScreen_ShowResult(s32 k)
{
    Loading_Reset();
    switch ((s8)k) {
    case 0:
    case 2:
    case 3:
        break;
    case 1:
        LobbyText_Open(0xC);
        break;
    case 4:
        LobbyText_Open(0xA);
        break;
    case 5:
    case 7:
        LobbyText_Open(0xD);
        break;
    case 6:
    case 8:
        LobbyDialog_OpenWait(gLobby->errText);
        break;
    }
}

void LobbyLeave_Reset(void) {
    gLobbyLeaveStep = 0;
    gNetDisconnectStep = 0;
}

void LobbyLeave_OnReply(void);
s32 LobbyReq_6002(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6006(void (*onReply)(NetMsg, NetMsg *));
s32 NetIf_GetError(void);
s32 LobbyNet_DisconnectWait(void);

/* leaving the lobby (gLobbyLeaveStep): 6002 (kinds 0, 2, 8) or 6006 (1, 3-6)
 * sent when still online (0x6ADCF1), a wait for the reply or the
 * connection's end, then a pause; 1 once done (kinds 1, 3, 4 also wait
 * for LobbyNet_DisconnectWait) */
s32 LobbyLeave_Run(u8 kind)
{
    s32 r = 0;

    switch (gLobbyLeaveStep) {
    case 0:
        gLobbyLeaveStep++;
        break;
    case 1:
        gLobby->countdown = 0x708;
        if (gScene.online != 0) {
            switch (kind) {
            case 0:
            case 2:
            case 8:
                gLobbyLeaveStep++;
                gLobby->awaitReply = 0x35;
                LobbyReq_6002((void (*)(NetMsg, NetMsg *))LobbyLeave_OnReply); /* defined (void): it ignores the reply */
                break;
            case 1:
            case 3:
            case 4:
            case 5:
            case 6:
                gLobbyLeaveStep++;
                gLobby->awaitReply = 0x35;
                LobbyReq_6006((void (*)(NetMsg, NetMsg *))LobbyLeave_OnReply);
                break;
            case 7:
                gLobbyLeaveStep = 3;
                break;
            }
        } else {
            gLobbyLeaveStep = 3;
        }
        break;
    case 2:
        gLobby->countdown--;
        if (NetIf_GetError() != 0 || gLobby->countdown < 0) {
            gLobbyLeaveStep++;
        }
        break;
    case 3:
        gLobbyLeaveStep++;
        if (gScene.online != 0) {
            gLobby->countdown = 0x3C;
        } else {
            gLobby->countdown = 1;
        }
        break;
    case 4:
        if (--gLobby->countdown < 0) {
            gNetDisconnectStep = 0;
            gLobbyLeaveStep++;
        }
        break;
    case 5:
        switch (kind) {
        case 0:
        case 2:
        case 5:
        case 6:
        case 7:
        case 8:
            r = 1;
            break;
        case 1:
        case 3:
        case 4:
            if (LobbyNet_DisconnectWait() != 0) {
                r = 1;
            }
            break;
        }
        break;
    }
    return r;
}

void LobbyLeave_OnReply(void)
{
    if (gLobby->awaitReply == 0x35) {
        gLobby->awaitReply = 0U;
        if (gLobbyLeaveStep == 2) {
            gLobbyLeaveStep += 1;
        }
    }
}

void LobbyExit_Draw(MenuTask *layer) {
    Text_SetSize(layer->textSize);
    Gfx_DrawBlackLineFixed();
}

/* closes the screen (mode 4): cancels the file queue, clears the requests,
 * keeps a copy of the pouch */
void LobbyScreen_Close(void)
{
    s32 i;

    gLobby->phase = 4;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    FileQueue_Cancel();
    LobbyScreen_ClearRequests();
    Lobby_SetPlace3928(9);
    for (i = 0; i < 20; i++) {
        gPouchBackup[i].item = gSave.pouch[i].item;
        gPouchBackup[i].count = gSave.pouch[i].count;
    }
}

void LobbyPkt_6508Send(void) {
    LobbyPkt_6508();
}

/* unk_7BC3A0's .data (0x819650-0x819678) */
void (*jtbl_00819650[10])(void) = {
    LobbyExit_Steps0, LobbyExit_Steps0, LobbyExit_Steps0, LobbyExit_Steps0,
    LobbyExit_Steps4, LobbyExit_Steps4, LobbyExit_Steps4, LobbyExit_Steps4,
    LobbyExit_Steps4, NULL,
};
