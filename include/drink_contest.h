#ifndef DRINK_CONTEST_H
#define DRINK_CONTEST_H

#include "common.h"
#include "em.h"
#include "menu_task.h"

/*
 * The drinking contest at the tavern (TOWN_MODE_DRINK_CONTEST), against a
 * townsperson: DrinkContest_Open and DrinkContest_Close start and end it,
 * DrinkContest_Update runs it, DrinkContest_Play its rounds of keys and
 * DrinkContest_Draw draws them. DrinkContest_IsOn tells whether one runs.
 */

void DrinkContest_Open(Em *npc);
void DrinkContest_Close(void);
void DrinkContest_Update(void);
void DrinkContest_DrawCounts(void);
s32 DrinkContest_Play(void);
void DrinkContest_Draw(MenuTask *task);
void DrinkContest_DrawKeyFlash(u32 red, u32 green, u32 blue, u32 frame, f32 x, f32 y);
s32 DrinkContest_FlyKey(f32 *x, f32 *y, s32 *vx, s32 *vy, u8 *frame, u8 animate);
s32 DrinkContest_ShakeKey(void *arg, s8 *vel, f32 *pos, u8 bounce, u8 *unused);
s32 DrinkContest_KeysFlown(void);
void DrinkContest_DrawButton(s16 id, s16 x, s16 y, s32 depth, u32 color, f32 scaleY, f32 scaleX);
s32 DrinkContest_IsOn(void);
u8 *DrinkContest_GetLevel(void);

#endif
