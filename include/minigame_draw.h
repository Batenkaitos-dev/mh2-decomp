#ifndef MINIGAME_DRAW_H
#define MINIGAME_DRAW_H

#include "common.h"
#include "window.h"

/*
 * The minigames' drawing: the arm-wrestling contest's time left, power gauge
 * and buttons to mash (Lobby_DrawTimeLeft, Contest_DrawGauge,
 * Contest_DrawButtons), and the result banner of the arm-wrestling and
 * drinking contests (Minigame_DrawResult). Also the gallery's gun-aim debug
 * view (Gallery_DrawGunDebug), the drinking contest's two questions about
 * the persistent monster saved last (PersistentEm_LastKind,
 * DrinkContest_HpStage), and the gallery's frames (gGalleryFrame,
 * gGalleryPrizeFrame).
 */

void Lobby_DrawTimeLeft(s16 frames);
void Contest_DrawGauge(s32 mine, s32 theirs);
void Minigame_DrawResult(s8 result, s16 frames, s8 isContest);
void Contest_DrawButtons(f32 *circleScale, f32 *crossScale, s16 *bumpFrames);
void Gallery_DrawGunDebug(f32 *base, f32 *out);
void GunDebug_DrawMarker(f32 *pos, f32 *scale, u32 color);
void Contest_DrawGaugeFrame(u32 color, f32 x, f32 y);
void Contest_DrawGaugeMarker(u32 color, f32 x, f32 y);
u8 PersistentEm_LastKind(void);
s32 DrinkContest_HpStage(void);
extern Frame gGalleryPrizeFrame;
extern Frame gGalleryFrame;

#endif
