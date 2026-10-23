#ifndef GM_MAIN_H
#define GM_MAIN_H

#include "common.h"
#include "select.h"
#include "task.h"

/*
 * The title screen and its menu, gm_sub's main task (GmSub_Task): the
 * menu's set-up and cursor helpers, the mode select (online or offline)
 * that follows the load entry, the start of the game (GmSub_StartGame) and
 * the way back to the boot screens. gGmWork is the title's work, which
 * select's title drawing reads too.
 */

void gm_sub_entry(void);
void GmSub_OpenModeSelect(void);
void GmSub_Task(TaskSlot *task); /* started when the boot ends */
s32 GmSub_Setup(void);
void GmSub_InitMenu(void);
void GmSub_MoveCursorV(u16 held, u16 pressed, u8 *sel, u8 count);
void GmSub_MoveCursorH(u16 held, u16 pressed, u8 *sel, u8 count);
s32 GmSub_UpdateMenu(u16 held, u16 pressed);
s32 GmSub_StartGame(TaskSlot *task, u16 held, u16 pressed);
s32 GmSub_ExitToBoot(TaskSlot *task, u16 held, u16 pressed);
s32 GmSub_UpdateModeSelect(u16 held, u16 pressed);
s32 GmSub_IsCharaSelectMenu(void);
extern GmWork *gGmWork;

#endif
