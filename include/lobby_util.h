#ifndef LOBBY_UTIL_H
#define LOBBY_UTIL_H

#include "common.h"
#include "lobby_globals.h"

/*
 * The lobby's small helpers, which the town's menus and screens share: the
 * pad's tests (LobbyPad_Pressed, LobbyPad_Held), the friend and block lists
 * copied to and from the save (Lobby_BackupWork, Lobby_RestoreWork), the
 * stream's stop and fade, the menus' sounds (LobbySe_Play), the fade's
 * state, text output, and the user lists searched, counted and shortened
 * (FriendList_*, IdList_*).
 */

/* A FriendEntry seen as words: the records move down by whole-record
 * copies, which the compiler makes with word loads and stores only for a
 * word-aligned struct (FriendEntry's fields are all bytes). The callers
 * pass gFriendList as one. */
typedef struct FriendEntryWords {
    /* 0x00 */ u8 idFirst;          /* FriendEntry.userId[0]: 0 for a free entry */
    /* 0x01 */ u8 pad1[3];
    /* 0x04 */ f32 word4;
    /* 0x08 */ u8 nameFirst;        /* FriendEntry.name[0] */
    /* 0x09 */ u8 pad9[3];
    /* 0x0C */ f32 wordsC[9];
} FriendEntryWords;

void Lobby_ReleaseNetTextures(void);
s32 LobbyPad_Pressed(s32 button);
s32 LobbyPad_Held(s32 mask);
void Lobby_BackupWork(void);
void Lobby_RestoreWork(void);
void LobbyStream_Stop(void);
void LobbyStream_FadeOut(s32 frames);
void LobbySe_Play(s32 sound);
s32 LobbyFade_IsRunning(void);
void Text_PrintAt(char *text, f32 x, f32 y);
void Text_SetColorIndex(s16 i);
s32 FriendList_FindFree(s8 *list, s32 max);
s32 FriendList_FindId(s8 *list, s32 max, void *key);
s32 FriendList_DeleteAt(FriendEntryWords *list, u8 idx, u8 n);
s32 IdList_Count(s8 *list, u8 n);
s32 IdList_FindFree(s8 *list, s32 max);
s32 IdList_Find(s8 *list, s32 max, void *key);
s32 IdList_Remove(BlockedUser *list, u8 idx, u8 n);
s32 Lobby_IsSysBusy(void);

#endif
