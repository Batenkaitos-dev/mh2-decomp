#ifndef FRAME_H
#define FRAME_H

#include "common.h"
#include "sprite.h"

/*
 * What runs and draws each frame: the per-frame callback list (Callbacks_*)
 * and the layer callback list (LayerCallbacks_*), the pads' input each frame
 * (Pad_UpdateInput), the order in which a frame is drawn (Frame_Draw, and
 * Frame_DrawOverCapture over a captured screen) with the draw slots' player
 * callbacks (DrawCb_*), and the screen captures of the quest's end and of
 * the pause.
 */

void Callbacks_Clear(void);
void Callbacks_Add(void (*fn)(void));
void Callbacks_Run(void);
void Scene_LoadOverlay(s32 id);
void Scene_EmptyFrameHook(void); /* Scene_EmptyFrameHook (sub_main), empty */
void EquipPreview_Clear(void);
void Pad_UpdateInput(void);
u16 Pad_ReadButtons(s32 port);
void Pad_CopyWords(s32 port);
void LayerCallbacks_Run(u8 layer);
void LayerCallbacks_Clear(void);
void LayerCallbacks_Add(void (*fn)(void), u8 layer);
void Frame_Draw(void);
void Frame_DrawOverCapture(void);
void DrawCb_LocalPlayer(DrawSlot *slot);
void DrawCb_Player(DrawSlot *slot); /* sub_main: takes its draw slot */
void DrawCb_CharMake(DrawSlot *slot);
void ScreenCapture_Draw(void);
void ScreenCapture_Take(void);
void Pause_CaptureScreen(void);
void Pause_DrawCapture(void);

#endif
