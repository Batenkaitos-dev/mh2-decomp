/*
 * The HUD's setup: Hud_Reset clears the HUD state (hud.h) and the chat state
 * (chat_log.h), and Hud_Init and Hud_StartQuest prepare them for a stage and
 * for a quest.
 */
#include "common.h"
#include "chat_log.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "hud.h"
#include "syssave.h"
#include "sprite.h"
#include "area_anim.h"
#include "bgm.h"
#include "chat_box.h"
#include "box_menu.h"
#include "camera.h"
#include "game_menu.h"
#include "kbd.h"
#include "light.h"
#include "quest.h"
#include "window.h"
#include "draw_slots.h"
#include "game_util.h"
#include "libc.h"

extern DrawSlot gHudDrawHooks[4];

/* prototypes */
extern u8 gChatInputMode;
/* end of prototypes */

/* Points gHudState at its buffer and clears it and the chat state; the map
 * ping needs the button held 10 frames. */
void Hud_Reset(void)
{
    gHudState = &gHudStateBuf;
    memset((void *) gHudState, 0, sizeof(HudState));
    memset(&gChat, 0, sizeof(ChatState));
    gScene.menuOpen = 0;
    gHudState->pingHold = 10;
    Chat_ClearLog();
}

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

/*
 * Sets up the HUD for a stage: what Hud_StartQuest does, and also clears the
 * chat log and its recipients, takes the map's scale (gMapScales) and the
 * clock's times, and copies the options (gSysSave.options) into the scene
 * and the option menu's copies. The four draw hooks get the HUD's draw
 * layers, layers 1 to 3 the town's (the B ones) in the town.
 * Returns the state.
 */
void *Hud_Init(void)
{
    DrawPool_ClearHud();
    gHudDrawHooks[0].draw = (DrawSlotFn)Hud_DrawLayer0;
    gHudDrawHooks[1].draw = (DrawSlotFn)Hud_DrawLayer1;
    gHudDrawHooks[2].draw = (DrawSlotFn)Hud_DrawLayer2;
    gHudDrawHooks[3].draw = (DrawSlotFn)Hud_DrawLayer3;
    gHudState->player = &gPlayers[gScene.localPlayer];
    gHudState->states = 0;
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
    Chat_PickSendTarget();
    gChatWinHold = 0;
    Chat_ShowWindows();
    gHudState->mapScaleX = 1.0f / gMapScales[gScene.map][0];
    gHudState->mapScaleZ = 1.0f / gMapScales[gScene.map][1];
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
    gHudState->aimFade = AIM_FADE_IDLE;
    gHudState->aimBlink = 0;
    gHudState->aimStart = 0;
    gHudState->pouchCursor = gHudState->player->pouchCursor;
    gHudState->ammoSlot = gHudState->player->ammoSlot;
    gHudState->cockpitOff = 0;
    gHudState->mapOff = 0;
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
    if (gScene.inLobby == 1) {
        GameMenu_Reset();
        BoxMenu_InitWork();
        gHudState->mapOff = 1;
        gHudDrawHooks[1].draw = (DrawSlotFn)Hud_DrawLayer1B;
        gHudDrawHooks[2].draw = (DrawSlotFn)Hud_DrawLayer2B;
        gHudDrawHooks[3].draw = (DrawSlotFn)Hud_DrawLayer3B;
    } else {
        gHudState->statusTimer[0] = -1;
        gHudState->statusTimer[1] = -1;
        gHudState->statusTimer[2] = -1;
        gHudState->statusTimer[3] = -1;
        gHudState->mapOff = 0;
        gHudState->timeLeft = Quest_GetTime(0);
        gHudState->timeTotal = Quest_GetTime(1);
        gHudState->clockHand = 0;
        gHudState->pouchBarState = 0;
        gHudState->popupOpen = 0;
    }
    gHudState->camReqStarted = 0;
    gHudState->aimKind = -1;
    gPauseWinStep = 0;
    gPauseMenuReq = 0;
    return (void *)gHudState;
}



/*
 * Resets the HUD for a quest's start: the draw hooks (as Hud_Init), the
 * local player, the menus, the chat input and windows, the map pings, the
 * item and camera event displays, and the notice window; outside the town, the
 * clock and the players' timers.
 */
void Hud_StartQuest(void)
{
    DrawPool_ClearHud();
    gHudDrawHooks[0].draw = (DrawSlotFn)Hud_DrawLayer0;
    gHudDrawHooks[1].draw = (DrawSlotFn)Hud_DrawLayer1;
    gHudDrawHooks[2].draw = (DrawSlotFn)Hud_DrawLayer2;
    gHudDrawHooks[3].draw = (DrawSlotFn)Hud_DrawLayer3;
    gHudState->player = &gPlayers[gScene.localPlayer];
    gHudState->states = 0;
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
    Chat_PickSendTarget();
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
    gHudState->aimFade = AIM_FADE_IDLE;
    gHudState->aimBlink = 0;
    gHudState->aimStart = 0;
    gHudState->pouchCursor = gHudState->player->pouchCursor;
    gHudState->ammoSlot = gHudState->player->ammoSlot;
    gChat.notice = 0;
    gNoticeWidth = -1;
    gNoticeFlags = 0;
    if (gScene.inLobby == 1) {
        GameMenu_Reset();
        BoxMenu_InitWork();
        gHudState->mapOff = 1;
        gHudDrawHooks[1].draw = (DrawSlotFn)Hud_DrawLayer1B;
        gHudDrawHooks[2].draw = (DrawSlotFn)Hud_DrawLayer2B;
        gHudDrawHooks[3].draw = (DrawSlotFn)Hud_DrawLayer3B;
    } else {
        gHudState->statusTimer[0] = -1;
        gHudState->statusTimer[1] = -1;
        gHudState->statusTimer[2] = -1;
        gHudState->statusTimer[3] = -1;
        gHudState->mapOff = 0;
        gHudState->clockHand = 0;
        gHudState->pouchBarState = 0;
        gHudState->popupOpen = 0;
    }
    gHudState->camReqStarted = 0;
    gHudState->aimKind = -1;
    gPauseWinStep = 0;
    gPauseMenuReq = 0;
}
