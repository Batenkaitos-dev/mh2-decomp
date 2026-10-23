#ifndef MAIL_BLOCK_LIST_H
#define MAIL_BLOCK_LIST_H

#include "common.h"
#include "menu_task.h"

/*
 * The mail block list: the game menu's page that lists the users on the mail
 * block list and removes one of them. The mail page opens it
 * (BlockList_Open) and runs BlockList_Update once a frame; BlockList_Draw
 * draws it.
 */

void BlockList_Open(void);
s32 BlockList_Update(u16 pad);
void BlockList_Draw(MenuTask *task);

#endif
