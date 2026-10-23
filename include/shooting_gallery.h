#ifndef SHOOTING_GALLERY_H
#define SHOOTING_GALLERY_H

#include "common.h"
#include "menu_task.h"
#include "stageobj.h"

/*
 * The shooting gallery (TOWN_MODE_GALLERY): a menu of courses, a round shot
 * at a board of monster targets, the score, and the prize exchange where
 * the points buy prizes for the pouch. Gallery_Open opens it, Gallery_Run
 * runs it and Gallery_DrawShooting draws it; the board, shots and score
 * are stage objects that register through Gallery_AddObj and ask where
 * the round is (Gallery_BeforeShooting, Gallery_InMenuOrClose,
 * Gallery_InResults).
 */

void Gallery_Open(void);
void Gallery_InitPrizes(void);
void Gallery_ResetWork(void);
void Gallery_ResetShots(void);
void Gallery_Aim(void);
void Gallery_Run(void);
s32 Gallery_BeforeShooting(void);
void Gallery_DrawShooting(MenuTask *task);
s32 Gallery_AddObj(StageObj *obj, u8 mode);
s32 Gallery_CountShots(s32 doCount);
void Gallery_ScoreTarget(void *board, u8 i);
void Gallery_ClearShots(void);
void Gallery_UpdateShots(void);
void Gallery_DrawShotMarks(s8 count, s8 lit);
void Gallery_DrawPrizeList(s16 x);
void Gallery_DrawLabelBar(s32 label);
void Gallery_DrawCourseCard(s16 x, s16 y, s16 time, s8 shots, s16 best, u8 level);
void Gallery_DrawPrizePage(s16 x);
void Gallery_DrawPrizeWindow(s16 x, u8 asking);
s32 Gallery_InMenuOrClose(void);
s32 Gallery_InResults(void);
void Gallery_MarkTooDear(void);
void Gallery_MarkFullPrizes(void);
void Gallery_CheckPouchFull(void);
s32 Gallery_IsPouchFull(void);
u8 Gallery_IsRepeatPrize(u8 idx);
void Gallery_ChoosePrize(void);
void Gallery_MarkTooDearCourses(void);
void Gallery_TakePrize(void);
void Gallery_MarkFullStacks(void);
void Gallery_MarkAllTaken(void);
void Gallery_PlayHitSe(u16 picture);

#endif
