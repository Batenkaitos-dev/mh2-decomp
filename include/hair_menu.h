#ifndef HAIR_MENU_H
#define HAIR_MENU_H

#include "common.h"
#include "em.h"
#include "menu_task.h"

/*
 * The hair menu (TOWN_MODE_HAIR_MENU): a haircut at the hairdresser, or a
 * change of hair style and colour with the model previewed, paid for after
 * a yes or no. HairMenu_Open starts it, HairMenu_Update runs it and
 * HairMenu_Draw draws it.
 */

s32 HairMenu_IsOpen(void);
void HairMenu_Open(Em *npc, s32 withNpc);
s32 HairMenu_IsIdle(void);
void HairMenu_Update(void);
void HairMenu_LoadPage(s16 page);
void HairMenu_ShowList(void);
s32 HairMenu_ListInput(u16 pad);
void HairMenu_ResetColor(void);
s32 HairMenu_ColorInput(u16 pad);
void HairMenu_Apply(void);
void HairMenu_DrawList(s16 x, s16 y);
void HairMenu_DrawConfirm(s16 x, s16 y);
void HairMenu_DrawHeader(void);
void HairMenu_Draw(MenuTask *task);

#endif
