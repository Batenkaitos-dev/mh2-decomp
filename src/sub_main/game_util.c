/*
 * A run of small functions sub_main's code and the overlays share: the
 * options applied to the system settings (Options_Apply), the quest to
 * run (Quest_SetCurrent), the reset of the game's subsystems for a new
 * scene (Game_ResetSubsystems, with the pools' clear and the motion
 * table's frees), the reset that goes back to the title overlay, the pad
 * and USB keyboard
 * getters (the chat's hotkeys), the menu sound shortcuts (sound.h's
 * SE_MENU_* and SE_SYS_*), the quest and scene queries, the supply box's
 * set-up, and the loading screen's captions (Loading_SetMessage, with the
 * loading screen itself drawn by its own unit).
 */
#include "common.h"
#include "sound.h"
#include "quest.h"
#include "kbd.h"
#include "scene.h"
#include "pad.h"
#include "player.h"
#include "qsync.h"
#include "overlay.h"
#include "area_draw.h"
#include "bgm.h"
#include "obj.h"
#include "card.h"
#include "hud.h"
#include "model.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "syssettings.h"
#include "syssave.h"
#include "movie.h"
#include "stream.h"
#include "texture.h"
#include "game_util.h"
#include "area.h"
#include "camera.h"
#include "display_util.h"
#include "draw_slots.h"
#include "em.h"
#include "file_load.h"
#include "font.h"
#include "frame.h"
#include "loading_screen.h"
#include "main.h"
#include "mem_util.h"
#include "rand.h"
#include "stage_data.h"
#include "sys_main.h"
#include "town_frame.h"
#include "vibration.h"
#include "screen_fade.h"
#include "libc.h"


extern u8 gSoundLoading;
extern u8 gFieldControl;
extern u8 gOnlineFlag;
extern u32 gNextQuest;
extern s32 gLoadingState;
extern s32 gLoadingArg;
extern char *gLoadingMessage;

extern u8 gAreaSubModel[];
extern void *gBones;
extern void *gMatSets;
extern void *gMaterials;
extern void *gModelLoadBuf;
extern void *gModels;
extern void *gMotions[];
extern void *gObjBufBlocks;
extern void *gPrims;
extern u8 gScreenCaptureOk;
extern u8 gSysBusy;
extern void *gWorkArea;

/* Applies the options to the system settings. */
void Options_Apply(void)
{
    SysSettings *sys = &gSysSettings;

    gSysSettings.soundMode = gSysSave.options.soundMode;
    gSysSettings.musicVolume = gSysSave.options.musicVolume;
    gSysSettings.seVolume = gSysSave.options.seVolume;
    gSysSettings.brightness = gSysSave.options.brightness;
    if (gSysSettings.brightness < 56 || sys->brightness > 164) {
        sys->brightness = 128;
        gSysSave.options.brightness = 128;
    }
    Sound_ApplyOutputMode(sys->soundMode);
    Stream_UpdateMaxVol(1);
    Display_SetOffset(gSysSave.options.screenX, gSysSave.options.screenY);
}

/* Records the quest to run: its id, its data and whether it came from the
 * quest list. */
void Quest_SetCurrent(u16 questId, void *questData, s32 fromList)
{
    gSysSettings.questData = questData;
    gSysSettings.questId = questId;
    gSysSettings.questFromList = fromList;
}

/* Clears the model, material, bone and primitive pools, the work area,
 * the camera data and the model load buffer. */
void Gfx_ClearModelBuffers(void)
{
    memset(gObjBufBlocks, 0, 0x20000);
    memset(gMatSets, 0, 53760);
    memset(gMaterials, 0, 0x14000);
    memset(gBones, 0, 0xC8000);
    memset(gModels, 0, 16384);
    memset(gPrims, 0, 20480);
    memset(gSysSettings.extraBuf, 0, 0x14000);
    memset(gWorkArea, 0, 0x18000);
    memset(gSysSettings.camData, 0, 8192);
    memset(gModelLoadBuf, 0, 0x134000);
}

/*
 * Resets the game's subsystems for a new scene: the camera, render states,
 * motions, models, textures, objects, players, monsters, areas, draw lists,
 * sound and text, and reloads the two common texture files (slots 4 and 5).
 */
void Game_ResetSubsystems(void)
{
    s16 i;

    FileQueue_Cancel();
    Camera_Init();
    Camera_SetPreset(0);
    Gfx_ResetRenderStates(0);
    Motion_FreeAll();
    Model_FreeAll();
    UnreadFlags_ClearAll();
    Movie_Stop();
    ScreenTex_Release();
    Tex_ReleaseAll();
    Mem_Compact();
    Model_ResetAll();
    Obj_InitAll();
    Area_ResetStub();
    Light_Reset();
    Player_ClearAll();
    EmSlot_ClearAll();
    Area_ClearState();
    EquipPreview_Clear();
    memset(gAreaSubModels, 0, 16);
    memset(gAreaSubModel, 0, 8);
    memset(gCommonModels, 0, 20);
    Mem_Compact();
    Callbacks_Clear();
    Gs_SetTableColor(0);
    Tex_LoadFile(4, 328, 0);
    Tex_LoadFile(5, 341, 0);
    FadeA_Reset();
    FreeList_InitAll();
    DrawSlot_ResetAll();
    DrawSlotB_ResetAll();
    Sound_Silence();
    Stream_Reset();
    Vib_StopAll();
    Sound_ReverbOn();
    Sound_SetSpuMapPage(0);
    gScreenCaptureOk = 0;
    gFieldControl = 1;
    gSysSettings.clearCapture = 0;
    gSysBusy = 0;
    Text_Reset();
    gSoundLoading = 0;
    gSysSettings.cardMenuDone = 1;
    if (Overlay_IsLoaded(OVERLAY_SUB_MAIN) == 1) {
        Notice_ResetQueue();
    }
    for (i = 0; i < MOTIONS_EM_SLOTS; i++) {
        EmSlot_ForgetModel(i, 2);
    }
}

/* Frees every loaded motion (gMotions) and clears its entry. */
void Motion_FreeAll(void)
{
    s32 i;
    void **motion = gMotions;

    for (i = 0; i < MOTIONS_TOTAL; i++, motion++) {
        if (*motion != NULL && Mem_FreeRet1(*motion) == 1) {
            *motion = NULL;
        }
    }
}

/* Frees the common set's motions (those before the players'). */
void Motion_FreeCommon(void)
{
    s32 i;
    void **motion = gMotions;

    for (i = 0; i < MOTIONS_PLAYER_FIRST; i++, motion++) {
        if (*motion != NULL && Mem_FreeRet1(*motion) == 1) {
            *motion = NULL;
        }
    }
}

/* Clears monster slot `slot`'s model and frees its motions. */
void Motion_FreeEmSlot(s16 slot)
{
    s32 end;
    s16 i;

    EmSlot_ForgetModel(slot, 1);
    for (i = slot * MOTIONS_PER_EM + MOTIONS_EM_FIRST, end = i + MOTIONS_PER_EM; i < end; i++) {
        if (gMotions[i] != NULL && Mem_FreeRet1(gMotions[i]) == 1) {
            gMotions[i] = NULL;
        }
    }
}

void Area_FreeAll(void)
{
    Area_FreeModels();
    Area_ClearState();
}

void Scene_Clear(void)
{
    memset(&gScene, 0, sizeof(SceneState));
}

/* Clears gQuestSync, keeping its charIndex. */
void QuestSync_Clear(void)
{
    u8 saved = gQuestSync.charIndex;

    memset(&gQuestSync, 0, sizeof(QuestSync));
    gQuestSync.charIndex = saved;
}

/* Resets the game state and loads the title/menu overlay. */
void Game_ReturnToTitle(void)
{
    gSoundLoading = 0;
    Loading_Reset();
    Vib_StopAll();
    Game_ResetSubsystems();
    Scene_Clear();
    QuestSync_Clear();
    Player_ClearAll();
    EmSlot_ClearAll();
    Area_ClearState();
    FreeList_InitAll();
    DrawSlot_ResetAll();
    DrawSlotB_ResetAll();
    LoadOverlay(OVERLAY_SELECT);
    TaskSlot_ClearFirstNine();
    SysSettings_Reset();
    Task_StartBootToGmSub();
}

s32 SubMain_ResetRequested(void)
{
    return 0;
}

/*
 * 1 when the hunter takes pad input: gFieldControl is set, no keyboard is
 * open, and online the lobby doesn't hold the player (Town_IsPlayerHeld).
 */
s32 Field_ControlEnabled(void)
{
    if (gFieldControl == 0 || Kbd_IsOpen() != 0) {
        return 0;
    }
    if (gScene.inLobby != 0 && Town_IsPlayerHeld() == 1) {
        return 0;
    }
    return 1;
}

/* Buttons newly pressed on this port this frame, or auto-repeating. */
u16 Pad_GetPressedRepeat(s32 pad)
{
    return gPadState[pad].repeat | gPadState[pad].pressed;
}

/* Newly pressed buttons plus auto-repeat of the D-pad, for menu cursors;
 * none while Kbd_IsOpen() is set. */
u16 Pad_GetCursorButtons(s32 pad)
{
    u16 buttons = 0;

    if (!Kbd_IsOpen()) {
        buttons = (gPadState[pad].repeat & PAD_DPAD) | gPadState[pad].pressed;
    }
    return buttons;
}

u8 Kbd_UsbNewKey(void)
{
    return gKbd->usbNewKey;
}

/*
 * The chat's keyboard hotkeys, for the field and the lobby (HUD input):
 * Alt with F1 to F12 asks for gesture 1 to 12 when the chat is available,
 * and returns 0. Otherwise returns 1 when the key pressed should open the
 * chat's input: with Ctrl or Shift held, or any key but Escape, F6, F7,
 * the Enter keys and the arrows (0 for those, and for no key).
 */
s32 Kbd_ActHotkeys(void)
{
    KbdEntry *kbd = gKbd;
    s32 mods = kbd->usbMods;

    if ((mods & USB_MOD_ALT) && kbd->usbNewKey >= USB_KEY_F1 && kbd->usbNewKey < USB_KEY_F12 + 1) {
        if (Chat_IsAvailable() == 1) {
            Player_RequestGesture(gKbd->usbNewKey - (USB_KEY_F1 - 1));
            Hud_SetCancel();
        }
        return 0;
    }
    if (mods & (USB_MOD_CTRL | USB_MOD_SHIFT)) {
        return 1;
    }
    switch (kbd->usbNewKey) {
    case USB_KEY_NONE:
    case USB_KEY_ESCAPE:
    case USB_KEY_F6:
    case USB_KEY_F7:
    case USB_KEY_ENTER:
    case USB_KEY_KP_ENTER:
    case USB_KEY_RIGHT:
    case USB_KEY_LEFT:
    case USB_KEY_DOWN:
    case USB_KEY_UP:
        return 0;
    }
    return 1;
}

/* 1 when an arrow key was pressed with Alt held. */
s32 Kbd_IsModArrow(void)
{
    if (gKbd->usbMods & USB_MOD_ALT) {
        switch (gKbd->usbNewKey) {
        case USB_KEY_RIGHT:
        case USB_KEY_LEFT:
        case USB_KEY_DOWN:
        case USB_KEY_UP:
            return 1;
        }
    }
    return 0;
}

void Sound_PlayConfirm(void)
{
    Sound_PlaySeFlat(1, SE_MENU_CONFIRM, 0);
}

void Sound_PlayCursorMove(void)
{
    Sound_PlaySeFlat(1, (Rand_Next(1) & 3) + SE_MENU_CURSOR, 0);
}

void Sound_PlaySlotChange(void)
{
    Sound_PlaySeFlat(1, SE_MENU_SLOT_CHANGE, 0);
}

void Sound_PlayCancel(void)
{
    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
}

/* Plays jingle 12 for the player: the carried object was lost. */
void Jingle_CarryLost(Player *pl)
{
    Jingle_Play(pl, 12);
}

s32 Game_IsOnline(void)
{
    return gOnlineFlag != 0;
}

/*
 * 1 when the quest is a drill (QUEST_FLAG_DRILL): online, the flag of the
 * quest chosen to start next (gNextQuest), once the quest menu's info is
 * set; offline, the running quest's.
 */
s32 Quest_IsDrill(void)
{
    if (gScene.inLobby == 1) {
        if (QuestMenu_CheckInfo() == 1 && (gNextQuest & QUEST_FLAG_DRILL)) {
            return 1;
        }
        return 0;
    }
    return (gQuest.flags & QUEST_FLAG_DRILL) != 0;
}

/* 1 while the scene is the quest in the field or the quest's end. */
s32 Scene_InField(void)
{
    if (gScene.scene == SCENE_FIELD || gScene.scene == SCENE_QUEST_END) {
        return 1;
    }
    return 0;
}

/*
 * 1 when the quest's phase means it is over, for the player's actions.
 * The cleared, failed, aborted and quit phases always count; the script's
 * end (QUEST_PHASE_ENDED) counts unless mode is 2; with mode 0 the
 * goal-met phase (the return countdown) counts too. The player's code asks with mode 1 or 0.
 */
s32 Quest_PhaseIsOver(s32 mode)
{
    switch (gScene.questPhase) {
    case QUEST_PHASE_GOAL_MET:
        if (mode == 0) {
            return 1;
        }
        break;
    case QUEST_PHASE_ENDED:
        if (mode != 2) {
            return 1;
        }
        break;
    case QUEST_PHASE_CLEARED:
    case QUEST_PHASE_FAILED:
    case QUEST_PHASE_ABORTED:
    case QUEST_PHASE_QUIT:
        return 1;
    }
    return 0;
}

/*
 * Fills the supply box's first 40 slots (gScene.supply) from the stage's
 * 40 supply items, which keep their places (an empty one leaves its slot
 * empty), and clears the taken flags.
 */
void Scene_InitSupplyBox(void)
{
    SceneState *scene = &gScene;
    ItemStack *src;
    s16 i;

    for (i = 0; i < 40; i++) {
        scene->supply[i].item = 0;
        scene->supply[i].count = 0;
    }
    src = Stage_GetSupplyItems();
    for (i = 0; i < 40; i++, src++) {
        if (src->item != 0) {
            scene->supply[i].item = src->item;
            scene->supply[i].count = src->count;
        }
    }
    scene->supplyTaken[0] = 0;
    scene->supplyTaken[1] = 0;
}

/* Does nothing. The gallery scenes call it as they load the game or lobby overlay for a quest. */
void Load_EmptyHook(void)
{
}

s32 Str_Length(char *s);
extern s8 gGalleryTitle[];

extern s32 gLoadingScreenOn;
/*
 * Opens the loading screen if it isn't open and sets its caption by mode:
 * 1 and 4 a caption of the table at gLoadingCaptions, 5 the gallery's
 * title (or mode 4's caption when it is empty), 3 the first of
 * gLoadingCaptionsAlt; 2 draws that one centred at once. 0 calls an empty
 * function and 6 does nothing. The callers pass 0, 1, 4 and 5.
 */
void Loading_SetMessage(u32 mode)
{
    char *text;

    if (gLoadingScreenOn == 0) {
        Loading_Start();
    }
    switch (mode) {
    default:
    case 0:
        Loading_CaptionStub(0);
        break;
    case 1:
        gLoadingState = 2;
        gLoadingMessage = gLoadingCaptions[0];
        break;
    case 2:
        Font_SetSpacing(24, 24);
        text = gLoadingCaptionsAlt[0];
        Text_DrawShadow((640 - Str_Length(text) * 12) / 2, 200, 1, 0, text);
        break;
    case 3:
        gLoadingState = 2;
        gLoadingMessage = gLoadingCaptionsAlt[0];
        break;
    case 4:
        gLoadingState = 2;
        gLoadingMessage = gLoadingCaptions[1];
        break;
    case 5:
        gLoadingState = 2;
        if (gGalleryTitle[0] == 0) {
            gLoadingMessage = gLoadingCaptions[1];
        } else {
            gLoadingMessage = (char *) gGalleryTitle;
        }
        break;
    case 6:
        break;
    }
}

/* Empty: the loaders call it with the frames that passed while a load
 * waited (main's Load_Idle, the gallery's texture load). */
void Load_AdvanceFrames(s32 frames, s32 unused)
{
}

/* Closes the loading screen and clears its caption, state and argument. */
void Loading_Reset(void)
{
    gSoundLoading = 0;
    gLoadingMessage = NULL;
    gLoadingState = 0;
    gLoadingArg = 0;
    Loading_Close();
}

/* Opens the loading screen with gLoadingArg (Loading_Open doesn't read its
 * first argument). */
void Loading_Start(void)
{
    Loading_Open(0xAC6800, gLoadingArg);
}

/* Closes the loading screen and clears its caption, state and argument,
 * as Loading_Reset does but for gSoundLoading. */
void Loading_Stop(void)
{
    gLoadingMessage = NULL;
    gLoadingState = 0;
    gLoadingArg = 0;
    Loading_Close();
}
