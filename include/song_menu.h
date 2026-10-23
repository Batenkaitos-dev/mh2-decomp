#ifndef SONG_MENU_H
#define SONG_MENU_H

#include "common.h"
#include "menu_task.h"

/*
 * The song menu of the hub area (TOWN_MODE_SONG_MENU): a list of music
 * tracks a hunter asks the host to play. SongMenu_OnBusy and
 * SongMenu_OnAccepted take the host's reply, and SongMenu_IsPlaying tells
 * the hub's residents whether a requested track plays.
 */

s32 SongMenu_IsPlaying(void);
void SongMenu_Open(u8 *npc);
void SongMenu_Update(void);
void SongMenu_OnBusy(void);
void SongMenu_OnAccepted(void);
void SongMenu_Draw(MenuTask *task);

#endif
