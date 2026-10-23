#ifndef ONLINE_MARK_H
#define ONLINE_MARK_H

#include "common.h"
#include "menu_task.h"

/*
 * The online mark window, an entry of the field menu's status page: the
 * mark a hunter shows the others beside the name and on the hunter card.
 * OnlineMark_Open and OnlineMark_Close open and close the window, and
 * OnlineMark_Draw draws it.
 */

void OnlineMark_Open(void);
void OnlineMark_Close(void);
void OnlineMark_Draw(MenuTask *task);

#endif
