#ifndef GAME_UTIL_H
#define GAME_UTIL_H

#include "common.h"
#include "player.h"

/*
 * Small functions sub_main's code and the overlays share: the reset of the
 * game's subsystems for a new scene (Game_ResetSubsystems) and the return to
 * the title, the pad and USB keyboard getters, the menu sound shortcuts
 * (Sound_Play*), the quest and scene queries, and the supply box's set-up
 * (Scene_InitSup).
 */

void Options_Apply(void);
void Quest_SetCurrent(u16 questId, void *questData, s32 fromList);
void Gfx_ClearModelBuffers(void);
void Game_ResetSubsystems(void);
void Motion_FreeAll(void);
void Motion_FreeCommon(void);
void Motion_FreeEmSlot(s16 slot);
void Area_FreeAll(void);
void Scene_Clear(void);
void QuestSync_Clear(void);
void Game_ReturnToTitle(void);
s32 SubMain_ResetRequested(void);
s32 Field_ControlEnabled(void);
u16 Pad_GetPressedRepeat(s32 pad);
u16 Pad_GetCursorButtons(s32 pad);
u8 Kbd_UsbNewKey(void);
s32 Kbd_ActHotkeys(void);
s32 Kbd_IsModArrow(void);
void Sound_PlayConfirm(void);
void Sound_PlayCursorMove(void);
void Sound_PlaySlotChange(void);
void Sound_PlayCancel(void);
void Jingle_CarryLost(Player *pl);
s32 Game_IsOnline(void);
s32 Quest_IsDrill(void);
s32 Scene_InField(void);
s32 Quest_PhaseIsOver(s32 mode);
void Scene_InitSupplyBox(void);
void Load_EmptyHook(void);
void Loading_SetMessage(u32 mode);
void Load_AdvanceFrames(s32 frames, s32 unused);
void Loading_Reset(void);
void Loading_Start(void);
void Loading_Stop(void);

#endif
