#ifndef NET_SCENE_H
#define NET_SCENE_H

#include "common.h"
#include "task.h"

/*
 * The lobby overlay's scene task, NetScene_Run, which sub_main runs whenever
 * the lobby overlay is the scene. With it, the moves between its steps that
 * the other lobby code makes (NetScene_ToPortal, NetScene_ToNetFileCheck,
 * NetScene_ToDisconnect), the play time's count (Lobby_CountPlayTime) and
 * the lobby's copies of the offline pause (Lobby_SetPause,
 * Lobby_GetPauseState).
 */

void NetScene_DrawFrame(void);
void NetScene_LoadOverlays(void);
void Lobby_CountPlayTime(void);
void NetScene_Run(TaskSlot *task);
void NetScene_Quit(TaskSlot *task);
void Lobby_SetBootMode(s32 mode);
void NetScene_Finish(TaskSlot *task);
s32 NetScene_StepConnect(TaskSlot *task);
s32 NetScene_StateOnline(TaskSlot *task);
s32 NetScene_StateOffline(TaskSlot *task);
s32 NetScene_DropConnection(TaskSlot *task);
void NetScene_ToPortal(void);
void NetScene_ToNetFileCheck(void);
void NetScene_ToDisconnect(void);
s32 Lobby_SetPause(s32 pause);
s32 Lobby_GetPauseState(void);

#endif
