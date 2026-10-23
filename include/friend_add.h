#ifndef FRIEND_ADD_H
#define FRIEND_ADD_H

#include "common.h"

/*
 * Adding a user to the friend list or to the mail block list, and the
 * question that starts taking one off the block list. Each starts with an
 * Ask call given the user's id and name; its input function then runs each
 * frame until it stops returning MENU_RUNNING.
 */

void FriendList_AskAdd(void *userId, void *name);
s32 FriendList_AddInput(u16 pad);
void BlockList_AskAdd(void *userId, void *name);
s32 BlockList_AddInput(u16 pad);
void BlockList_AskRemove(void *userId, u8 *name);
s32 BlockList_RemoveInput(u16 pad);

#endif
