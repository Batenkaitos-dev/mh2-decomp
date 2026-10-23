#ifndef QUIT_MENU_H
#define QUIT_MENU_H

#include "common.h"

/*
 * The quit menu: three ways to leave the lobby, each confirmed with a yes
 * or no before the session ends. QuitMenu_Open starts it and
 * QuitMenu_Update runs it a frame at a time.
 */

void QuitMenu_Open(void);
s32 QuitMenu_Update(u16 pad);

#endif
