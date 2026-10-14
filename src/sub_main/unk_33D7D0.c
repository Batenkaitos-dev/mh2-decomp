#include "common.h"
#include "chat_log.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "hud.h"
#include "syssave.h"

/* prototypes */
void Chat_ClearLog(void);
void * memset(void *p, s32 c, u32 n);
extern u8 gChatInputMode;
/* end of prototypes */

extern Em gEms[20];
s32 Quest_IsVersus(void);
void PlayerSys_Update(void);
void Em_CopyBoneMatrices(Em *em);
s32 GmEm_Task(Em *em);
void Em_UpdateMatrix(Em *em);
void Em_CarryRiders(Em *em);
s32 Em_RunByState(Em *em);
void Em_UpdateWorldMatrix(Em *em);
void Field_PushApart(void);
void Bgm_FieldUpdate(void);
void Em_CollideField(Em *em, u8 keep);
void Players_UpdateMatrices(void);
void Players_RunEquipEffects(void);
void CamCtl_Update(s32 a);
void LightSet_UpdateAll(void);
void StageObj_UpdateAll(void);
void ObjAttach_UpdateAll(void);
void Obj_UpdateAll(void);
void FlashObj_Tick(void);
void SightBlock_Tick(void);
void Area_UpdateTimers(void);
void ItemHud_Update(void);

/*
 * The field's per-frame update: the monsters (bones, then the game overlay's update or
 * Em_RunByState and the world matrix), Field_PushApart outside gScene.eventRunning, the attachments
 * of the players and the monsters (Em_CollideField), then the objects, areas and timers.
 */
void Field_UpdateAll(void)
{
    s32 i;
    Em *em;
    Player *pl;

    em = gEms;
    if (gScene.eventRunning == 0 || Quest_IsVersus() == 1) {
        PlayerSys_Update();
    }
    for (i = 0; i < 20; i++, em++) {
        if (em->active) {
            Em_CopyBoneMatrices(em);
            if (((u8 *)em)[0x1E] == 0) {
                if (GmEm_Task(em) == 0) {
                    Em_UpdateMatrix(em);
                    Em_CarryRiders(em);
                }
            } else if (Em_RunByState(em) == 0) {
                Em_UpdateWorldMatrix(em);
            }
        }
    }
    if (gScene.eventRunning == 0) {
        Field_PushApart();
    }
    Bgm_FieldUpdate();
    for (i = 0, pl = gPlayers; i < 10; i++, pl++) {
        if (pl->active && pl->unk72C[0x738 - 0x72C]) {
            Em_CollideField((Em *)pl, 1);
        }
    }
    for (i = 0, em = gEms; i < 20; i++, em++) {
        if (em->active && em->pushed) {
            Em_CollideField(em, 1);
        }
    }
    Players_UpdateMatrices();
    Players_RunEquipEffects();
    CamCtl_Update(0);
    LightSet_UpdateAll();
    StageObj_UpdateAll();
    ObjAttach_UpdateAll();
    Obj_UpdateAll();
    FlashObj_Tick();
    SightBlock_Tick();
    Area_UpdateTimers();
    ItemHud_Update();
}

void Hud_Reset(void)
{
    gHudState = &gHudStateBuf;
    memset((void *) gHudState, 0, 0x108U);
    memset(&gChat, 0, 0x1420U);
    gScene.menuOpen = 0;
    gHudState->pingHold = 0xA;
    Chat_ClearLog();
}

void DrawPool_ClearHud(void);
void Hud_ResetMenuState(void);
void Kbd_Close(void);
void Chat_SetColor(s16 color);
void Chat_InitMode(void);
void Chat_ShowWindows(void);
void GameMenu_Reset(void);
void BoxMenu_InitWork(void);
void Hud_DrawLayer0(void);
void Hud_DrawLayer1(void);
void Hud_DrawLayer2(void);
void Hud_DrawLayer3(void);
void Hud_DrawLayer1B(void);
void Hud_DrawLayer2B(void);
void Hud_DrawLayer3B(void);
extern void (*D_006A3A64)(void);
extern void (*D_006A3A84)(void);
extern void (*D_006A3AA4)(void);
extern void (*D_006A3AC4)(void);
extern s32 gChatSendWait;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;
extern u8 gChatWindowSel;
extern u8 gHelpShow;
extern u8 gHelpColumn;
extern s32 gNoticeWidth;
extern u8 gNoticeFlags;
extern u8 gPauseWinStep;
extern u8 gPauseMenuReq;
extern s16 gChatRecipients;
extern u8 gChatToAll;
extern u8 gChatMsgClass;
extern u8 gChatHideNames;
extern u8 gChatWinHold;
extern f32 gMapScales[][2];
s32 Game_IsOnline(void);
s32 Quest_GetTime(s32 a);

/* Hud_StartQuest with the chat, the scale of the display mode gScene.map and the
 * options (gSysSave.options.aimReverse..) copied into the state; returns the state. */
void *Hud_Init(void)
{
    DrawPool_ClearHud();
    D_006A3A64 = Hud_DrawLayer0;
    D_006A3A84 = Hud_DrawLayer1;
    D_006A3AA4 = Hud_DrawLayer2;
    D_006A3AC4 = Hud_DrawLayer3;
    gHudState->player = &gPlayers[gScene.localPlayer];
    *(s32 *)((u8 *)gHudState + 4) = 0;
    Hud_ResetMenuState();
    gHudState->menuPhase = 0;
    gScene.menuOpen = 0;
    gHudState->deliveryMenu = 0;
    gHudState->deliveryOpen = 0;
    Kbd_Close();
    Chat_ClearLog();
    gChatInputMode = 0;
    gChatSendWait = 0;
    gChatLog.win.open = gChatWinOpen = gChatWinTimer = 0;
    gChatLog.win.timer = 0;
    Chat_SetColor(-1);
    gChatRecipients = 0;
    gChatToAll = 1;
    gChatMsgClass = 3;
    if (Game_IsOnline() != 1) {
        gChatHideNames = 0;
    }
    gChatWindowSel = 1;
    Chat_InitMode();
    gChatWinHold = 0;
    Chat_ShowWindows();
    gHudState->invScaleX = 1.0f / gMapScales[gScene.map][0];
    gHudState->invScaleY = 1.0f / gMapScales[gScene.map][1];
    gHudState->pingTimer[3] = -1;
    gHudState->pingTimer[2] = -1;
    gHudState->pingTimer[1] = -1;
    gHudState->pingTimer[0] = -1;
    gHudState->mapZoomed = 0;
    gHudState->glowTimer = 0;
    gHudState->fieldMenuCursor = 0;
    gHelpShow = 0;
    gHelpColumn = 0;
    gHudState->fadeActive = 0;
    gHudState->dropItem = 0;
    gHudState->combineTaken = 0;
    gHudState->camEventFade = 4;
    gHudState->camEventBlink = 0;
    gHudState->camEventStart = 0;
    gHudState->pouchCursor = gHudState->player->pouchCursor;
    gHudState->ammoSlot = gHudState->player->ammoSlot;
    gHudState->cockpitOff = 0;
    gHudState->onlineHud = 0;
    gScene.aimReverse = gSysSave.options.aimReverse;
    gHudState->optAimReverse = gSysSave.options.aimReverse;
    gScene.camControl = gSysSave.options.camControl;
    gHudState->optCamControl = gSysSave.options.camControl;
    gScene.camFollow = gSysSave.options.camFollow;
    gHudState->optCamFollow = gSysSave.options.camFollow;
    gHudState->optVibration = gSysSave.options.vibration;
    gHudState->optChatSize = gSysSave.options.chatSize;
    gHudState->optChatHideNames = gChatHideNames;
    gHudState->optChatBeep = gSysSave.options.chatBeep;
    gChat.notice = 0;
    gNoticeWidth = -1;
    gNoticeFlags = 0;
    if (gScene.online == 1) {
        GameMenu_Reset();
        BoxMenu_InitWork();
        gHudState->onlineHud = 1;
        D_006A3A84 = Hud_DrawLayer1B;
        D_006A3AA4 = Hud_DrawLayer2B;
        D_006A3AC4 = Hud_DrawLayer3B;
    } else {
        gHudState->timer[0] = -1;
        gHudState->timer[1] = -1;
        gHudState->timer[2] = -1;
        gHudState->timer[3] = -1;
        gHudState->onlineHud = 0;
        gHudState->timeLeft = Quest_GetTime(0);
        gHudState->timeTotal = Quest_GetTime(1);
        gHudState->clockHand = 0;
        gHudState->unkB3 = 0;
        gHudState->popupOpen = 0;
    }
    gHudState->camReqStarted = 0;
    gHudState->camEvent = -1;
    gPauseWinStep = 0;
    gPauseMenuReq = 0;
    return (void *)gHudState;
}

void DrawPool_ClearHud(void);
void Hud_ResetMenuState(void);
void Kbd_Close(void);
void Chat_SetColor(s16 color);
void Chat_InitMode(void);
void Chat_ShowWindows(void);
void GameMenu_Reset(void);
void BoxMenu_InitWork(void);
void Hud_DrawLayer0(void);
void Hud_DrawLayer1(void);
void Hud_DrawLayer2(void);
void Hud_DrawLayer3(void);
void Hud_DrawLayer1B(void);
void Hud_DrawLayer2B(void);
void Hud_DrawLayer3B(void);
extern void (*D_006A3A64)(void);
extern void (*D_006A3A84)(void);
extern void (*D_006A3AA4)(void);
extern void (*D_006A3AC4)(void);
extern s32 gChatSendWait;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;
extern u8 gChatWindowSel;
extern u8 gHelpShow;
extern u8 gHelpColumn;
extern s32 gNoticeWidth;
extern u8 gNoticeFlags;
extern u8 gPauseWinStep;
extern u8 gPauseMenuReq;

/* Starts the quest's game screen state: the menu callbacks, the local player (the
 * online ones when gScene.online is 1) and the state at gHudState. */
void Hud_StartQuest(void)
{
    DrawPool_ClearHud();
    D_006A3A64 = Hud_DrawLayer0;
    D_006A3A84 = Hud_DrawLayer1;
    D_006A3AA4 = Hud_DrawLayer2;
    D_006A3AC4 = Hud_DrawLayer3;
    gHudState->player = &gPlayers[gScene.localPlayer];
    *(s32 *)((u8 *)gHudState + 4) = 0;
    Hud_ResetMenuState();
    gHudState->menuPhase = 0;
    gScene.menuOpen = 0;
    gHudState->deliveryMenu = 0;
    gHudState->deliveryOpen = 0;
    Kbd_Close();
    gChatInputMode = 0;
    gChatSendWait = 0;
    gChatLog.win.open = gChatWinOpen = gChatWinTimer = 0;
    gChatLog.win.timer = 0;
    Chat_SetColor(-1);
    gChatWindowSel = 1;
    Chat_InitMode();
    Chat_ShowWindows();
    gHudState->pingTimer[3] = -1;
    gHudState->pingTimer[2] = -1;
    gHudState->pingTimer[1] = -1;
    gHudState->pingTimer[0] = -1;
    gHudState->glowTimer = 0;
    gHudState->fieldMenuCursor = 0;
    gHelpShow = 0;
    gHelpColumn = 0;
    gHudState->fadeActive = 0;
    gHudState->dropItem = 0;
    gHudState->combineTaken = 0;
    gHudState->camEventFade = 4;
    gHudState->camEventBlink = 0;
    gHudState->camEventStart = 0;
    gHudState->pouchCursor = gHudState->player->pouchCursor;
    gHudState->ammoSlot = gHudState->player->ammoSlot;
    gChat.notice = 0;
    gNoticeWidth = -1;
    gNoticeFlags = 0;
    if (gScene.online == 1) {
        GameMenu_Reset();
        BoxMenu_InitWork();
        gHudState->onlineHud = 1;
        D_006A3A84 = Hud_DrawLayer1B;
        D_006A3AA4 = Hud_DrawLayer2B;
        D_006A3AC4 = Hud_DrawLayer3B;
    } else {
        gHudState->timer[0] = -1;
        gHudState->timer[1] = -1;
        gHudState->timer[2] = -1;
        gHudState->timer[3] = -1;
        gHudState->onlineHud = 0;
        gHudState->clockHand = 0;
        gHudState->unkB3 = 0;
        gHudState->popupOpen = 0;
    }
    gHudState->camReqStarted = 0;
    gHudState->camEvent = -1;
    gPauseWinStep = 0;
    gPauseMenuReq = 0;
}

s32 Hud_InputBlocked(void)
{
    if (gScene.menuOpen != 0) {
        return 1;
    }
    if (gChatInputMode != 0) {
        return 1;
    }
    if (gHudState->dropState != 0) {
        return 1;
    }
    if ((gScene.online == 0) && (gHudState->popupOpen != 0)) {
        return 1;
    }
    return 0;
}
