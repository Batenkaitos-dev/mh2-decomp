#ifndef WORKSHOP_MENU_H
#define WORKSHOP_MENU_H

#include "common.h"
#include "menu_task.h"
#include "camera.h"

/*
 * The workshop's menu (TOWN_MODE_WORKSHOP), opened by a workshop
 * townsperson: a top list of the three counters (the weapon forge, the
 * armour forge and the decorations) and the townsperson's offers. The
 * counter chosen runs through its own functions for each step, and the
 * forging scene plays when its work is done. Workshop_Open and
 * Workshop_Update run it; Workshop_Draw and Workshop_Draw2 draw it.
 */

void Workshop_Open(struct Em *npc);
void Workshop_Update(void);
void Workshop_OpenCounter(void);
s32 Workshop_CounterInput(u16 pad);
s32 Workshop_ResultInput(u16 pad);
void Workshop_Draw(MenuTask *task);
void Workshop_Draw2(MenuTask *task);

#endif
