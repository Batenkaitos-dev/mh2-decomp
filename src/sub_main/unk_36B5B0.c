/*
 * A run of small functions: the game-state reset that reloads the title
 * overlay, the pad getters, sound-effect shortcuts and flag queries.
 */
#include "common.h"
#include "quest.h"
#include "kbd.h"
#include "scene.h"
#include "pad.h"
#include "player.h"
#include "qsync.h"


extern u8 gSoundLoading;
extern u8 gFieldControl;
extern u8 gOnlineFlag;
extern u32 gNextQuest;
extern s32 gLoadingState;
extern s32 gLoadingArg;
extern char *gLoadingMessage;

void *memset(void *dst, s32 c, u32 n);
void LoadOverlay(s32 id);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void SysSettings_Reset(void);
s8 Kbd_IsOpen(void);
void Area_FreeModels(void);
void TaskSlot_Clear9(void);
void Task_StartBootToGmSub(void);
void Hud_SetCancel(void);
s32 QuestMenu_CheckInfo(void);
s32 Chat_IsAvailable(void);
void Jingle_Play(void *obj, s32 id);
u16 Rand_Next(s32 a);
void FreeList_InitAll(void);
void Game_ResetSubsystems(void);
void DrawSlot_ResetAll(void);
void DrawSlotB_ResetAll(void);
void Vib_StopAll(void);
void EmSlot_ClearAll(void);
void Area_ClearState(void);
void Player_ClearAll(void);
void *Stage_GetSupplyItems(void);
void Player_RequestAct(u8 a);
void Loading_Open(u32 a, s32 b);
void Loading_Close(void);
s32 Town_IsPlayerHeld(void);
void Scene_Clear(void);
void QuestSync_Clear(void);
void Loading_Reset(void);

void Area_FreeAll(void)
{
    Area_FreeModels();
    Area_ClearState();
}

void Scene_Clear(void)
{
    memset(&gScene, 0, sizeof(SceneState));
}

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
    LoadOverlay(2);
    TaskSlot_Clear9();
    SysSettings_Reset();
    Task_StartBootToGmSub();
}

s32 SubMain_ResetRequested(void)
{
    return 0;
}

s32 Field_ControlEnabled(void)
{
    if (gFieldControl == 0 || Kbd_IsOpen() != 0) {
        return 0;
    }
    if (gScene.online != 0 && Town_IsPlayerHeld() == 1) {
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
        buttons = (gPadState[pad].repeat & 0x3C00) | gPadState[pad].pressed;
    }
    return buttons;
}

u8 Kbd_UsbNewKey(void)
{
    return gKbd->usbNewKey;
}

s32 Kbd_ActHotkeys(void)
{
    KbdEntry *s = gKbd;
    s32 flags = s->usbMods;

    if ((flags & 0x44) && s->usbNewKey >= 0x3A && s->usbNewKey < 0x46) {
        if (Chat_IsAvailable() == 1) {
            Player_RequestAct(gKbd->usbNewKey - 0x39);
            Hud_SetCancel();
        }
        return 0;
    }
    if (flags & 0x33) {
        return 1;
    }
    switch (s->usbNewKey) {
    case 0:
    case 0x29:
    case 0x3F:
    case 0x40:
    case 0x28:
    case 0x58:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
        return 0;
    }
    return 1;
}

s32 Kbd_IsModArrow(void)
{
    if (gKbd->usbMods & 0x44) {
        switch (gKbd->usbNewKey) {
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
            return 1;
        }
    }
    return 0;
}

void Sound_PlaySe73(void)
{
    Sound_PlaySeFlat(1, 0x73, 0);
}

void Sound_PlayCursorMove(void)
{
    Sound_PlaySeFlat(1, (Rand_Next(1) & 3) + 0x78, 0);
}

void Sound_PlaySe72(void)
{
    Sound_PlaySeFlat(1, 0x72, 0);
}

void Sound_PlayCancel(void)
{
    Sound_PlaySeFlat(0, 0x14, 0);
}

void Jingle_CarryLost(Player *a)
{
    Jingle_Play(a, 0xC);
}

s32 Game_IsOnline(void)
{
    return gOnlineFlag != 0;
}

s32 Quest_IsDrill(void)
{
    if (gScene.online == 1) {
        if (QuestMenu_CheckInfo() == 1 && (gNextQuest & 0x200000)) {
            return 1;
        }
        return 0;
    }
    return (gQuest.flags & 0x200000) != 0;
}

s32 Scene_InField(void)
{
    if (gScene.scene == 2 || gScene.scene == 3) {
        return 1;
    }
    return 0;
}

/* Uses a jump table (rodata): stays assembly until data is split per file. */
s32 Quest_PhaseIsOver(s32 a)
{
    switch (gScene.questPhase) {
    case 3:
        if (a == 0) {
            return 1;
        }
        break;
    case 5:
        if (a != 2) {
            return 1;
        }
        break;
    case 4:
    case 6:
    case 7:
    case 8:
        return 1;
    }
    return 0;
}

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

void func_0036BC60(void)
{
}

/* Uses a jump table (rodata): stays assembly until data is split per file. */
void Loading_Start(void);
void func_003D54B0(s32 unused);
void Font_SetSpacing(u8 a, u8 b);
s32 Str_Length(char *s);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
extern char *D_00446B18[];
extern char *D_00446B1C;
extern char *D_00446B20[];
extern s8 gGalleryTitle[];

extern s32 gLoadingScreenOn;
void Loading_SetMessage(u32 mode)
{
    char *s;

    if (gLoadingScreenOn == 0) {
        Loading_Start();
    }
    switch (mode) {
    default:
    case 0:
        func_003D54B0(0);
        break;
    case 1:
        gLoadingState = 2;
        gLoadingMessage = D_00446B18[0];
        break;
    case 2:
        Font_SetSpacing(0x18, 0x18);
        s = D_00446B20[0];
        Text_DrawShadow((0x280 - Str_Length(s) * 12) / 2, 0xC8, 1, 0, s);
        break;
    case 3:
        gLoadingState = 2;
        gLoadingMessage = D_00446B20[0];
        break;
    case 4:
        gLoadingState = 2;
        gLoadingMessage = D_00446B1C;
        break;
    case 5:
        gLoadingState = 2;
        if (gGalleryTitle[0] == 0) {
            gLoadingMessage = D_00446B1C;
        } else {
            gLoadingMessage = (char *) gGalleryTitle;
        }
        break;
    case 6:
        break;
    }
}

void func_0036BDF0(void)
{
}

void Loading_Reset(void)
{
    gSoundLoading = 0;
    gLoadingMessage = NULL;
    gLoadingState = 0;
    gLoadingArg = 0;
    Loading_Close();
}

void Loading_Start(void)
{
    Loading_Open(0xAC6800, gLoadingArg);
}

void Loading_Stop(void)
{
    gLoadingMessage = NULL;
    gLoadingState = 0;
    gLoadingArg = 0;
    Loading_Close();
}
