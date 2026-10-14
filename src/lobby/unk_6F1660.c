/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "menu_task.h"
#include "lobby_menu.h"

void Font_SetSpacing(u8 a, u8 b);
void Text_SetSize(s32);
void AreaBoard_DrawList(s16, s16);
extern MenuWork gMenuWork;

void AreaBoard_Draw(MenuTask *task) {
    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    AreaBoard_DrawList(0xE, gMenuWork.room.listWin.pos);
}
