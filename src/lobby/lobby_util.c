/*
 * The lobby's small helpers, which the town's menus and screens share:
 * - the network mode's textures released;
 * - a menu cursor moved by the D-pad, with its key repeat
 *   (LobbyMenu_MoveCursor), and the pad's tests (LobbyPad_*);
 * - the friend and mail block lists copied to and from the save
 *   (Lobby_BackupWork, Lobby_RestoreWork);
 * - the stream's stop and fade, and the menus' sounds (LobbySe_Play maps
 *   each enum LobbySe id to a bank's sound);
 * - the fade's state, text output;
 * - the user lists: the friend list (FriendEntry records) and the block
 *   lists (BlockedUser records: the mail and chat block lists), searched,
 *   counted and shortened (FriendList_*, IdList_*).
 */
#include "common.h"
#include "net_save.h"
#include "pad.h"
#include "em.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "scene.h"
#include "lobby_state.h"
#include "sound.h"
#include "bgm.h"
#include "camera.h"
#include "net_mode.h"
#include "stream.h"
#include "text.h"
#include "lobby_util.h"
#include "font.h"
#include "screen_fade.h"
#include "libc.h"

extern BlockedUser gMailBlockList[MAIL_BLOCK_LIST_LEN];
extern FriendEntry gFriendList[];
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* Releases the network mode's textures. */
void Lobby_ReleaseNetTextures(void)
{
    NetMode_ReleaseTextures(0);
}

extern u16 gCursorRepeatV; /* frames until the up/down cursor's key repeats */
extern u16 gCursorRepeatH; /* Moves the cursor *cur (0 to max) by the D-pad: up and down for the
 * MOVE_CURSOR_V modes, left and right for the MOVE_CURSOR_H modes. A
 * press moves it at once and waits 20 frames; a held key then moves it
 * every 10 frames. The _WRAP modes wrap round at either end; the others
 * stop there. Returns 1 when the cursor moved (and plays
 * LOBBY_SE_SCROLL), else 0.
 *
 * The four tests share the two moves at the end (the gotos). */
s32 LobbyMenu_MoveCursor(u8 *cur, u8 max, u8 mode)
{
    s32 kdec;
    s32 kinc;
    u16 *rep;

    if ((mode == MOVE_CURSOR_V_WRAP) | (mode == MOVE_CURSOR_V)) {
        kdec = PAD_UP;
        kinc = PAD_DOWN;
        rep = &gCursorRepeatV;
    } else {
        kdec = PAD_LEFT;
        kinc = PAD_RIGHT;
        rep = &gCursorRepeatH;
    }
    if (LobbyPad_Pressed(kdec)) {
        *rep = 20; /* frames: the first repeat */
        goto dec;
    }
    if (LobbyPad_Pressed(kinc)) {
        *rep = 20; /* frames: the first repeat */
        goto inc;
    }
    if (*rep) {
        *rep = *rep - 1;
        return 0;
    }
    if (LobbyPad_Held(kdec)) {
        *rep = 10; /* frames: the next repeats */
        goto dec;
    }
    if (LobbyPad_Held(kinc)) {
        *rep = 10; /* frames: the next repeats */
        goto inc;
    }
    return 0;
inc:
    if (mode < MOVE_CURSOR_V) { /* the _WRAP modes */
        if (*cur == max) {
            *cur = 0;
        } else {
            (*cur)++;
        }
    } else {
        if (*cur == max) {
            return 0;
        }
        (*cur)++;
    }
    LobbySe_Play(LOBBY_SE_SCROLL);
    return 1;
dec:
    if (mode < MOVE_CURSOR_V) { /* the _WRAP modes */
        if (*cur == 0) {
            *cur = max;
        } else {
            (*cur)--;
        }
    } else {
        if (*cur == 0) {
            return 0;
        }
        (*cur)--;
    }
    LobbySe_Play(LOBBY_SE_SCROLL);
    return 1;
}

/* 1 when a button of the mask was pressed this frame or repeats. */
s32 LobbyPad_Pressed(s32 button)
{
    return (button & (gPadState[0].pressed | gPadState[0].repeat)) != 0;
}

/* 1 when a button of the mask is held. */
s32 LobbyPad_Held(s32 mask)
{
    return (gPadState[0].held & mask) != 0;
}

/* Copies the save's friend list and mail block list into the lobby's
 * lists (gFriendList, gMailBlockList). */
void Lobby_BackupWork(void)
{
    memcpy(gFriendList, gNetSaveData.friends, sizeof(gNetSaveData.friends));
    memcpy(gMailBlockList, gNetSaveData.blocked, sizeof(gNetSaveData.blocked));
}

/* Copies the lobby's friend list and mail block list back into the
 * save. */
void Lobby_RestoreWork(void)
{
    memcpy(gNetSaveData.friends, gFriendList, sizeof(gNetSaveData.friends));
    memcpy(gNetSaveData.blocked, gMailBlockList, sizeof(gNetSaveData.blocked));
}

/* Stops the stream. */
void LobbyStream_Stop(void)
{
    Stream_Stop(0);
}

/* Fades streams 0 and 1 out over the given frames. */
void LobbyStream_FadeOut(s32 frames)
{
    s32 len;

    len = frames & 0xFFFF;
    Stream_FadeOut(0, len);
    Stream_FadeOut(1, len);
}

/* Plays the menu sound `sound` (enum LobbySe): the system bank's SE_SYS_*,
 * the shared bank's sounds, or bank 6's (the shooting gallery's and the
 * save menu's, among others). Ids without a case play nothing. */
void LobbySe_Play(s32 sound)
{
    switch (sound) {
    case LOBBY_SE_LIST_BACK:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 164, 0);
        break;
    case LOBBY_SE_LIST_ENTRY:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 163, 0);
        break;
    case LOBBY_SE_LIST_SELECT:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 162, 0);
        break;
    case LOBBY_SE_LIST_CURSOR:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 161, 0);
        break;
    case LOBBY_SE_LIST_OPEN:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 160, 0);
        break;
    case LOBBY_SE_DESCRIPTION:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
        break;
    case LOBBY_SE_SELECT:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 145, 0);
        break;
    case 28:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 146, 0);
        break;
    case LOBBY_SE_CONFIRM_ALT:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        break;
    case LOBBY_SE_CANCEL:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        break;
    case 5:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 147, 0);
        break;
    case LOBBY_SE_SCROLL:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
        break;
    case LOBBY_SE_CURSOR:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 144, 0);
        break;
    case LOBBY_SE_GALLERY_SHOT:
        Sound_PlaySeFlat(6, 104, 0);
        break;
    case LOBBY_SE_GALLERY_DOWN:
        Sound_PlaySeFlat(6, 106, 0);
        break;
    case LOBBY_SE_MAIL:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 128, 0);
        break;
    case LOBBY_SE_CONFIRM:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        break;
    case LOBBY_SE_MONEY:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 131, 0);
        break;
    case LOBBY_SE_OPEN:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
        break;
    case 12:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 135, 0);
        break;
    case LOBBY_SE_SAVE_MENU:
        Sound_PlaySeFlat(6, 64, 0);
        break;
    case LOBBY_SE_TALK:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 133, 0);
        break;
    case LOBBY_SE_OPEN_ALT:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
        break;
    case 15:
        Sound_PlaySeFlat(6, 71, 0);
        break;
    case LOBBY_SE_MENU_OPEN:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 129, 0);
        break;
    case LOBBY_SE_EQUIP:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 137, 0);
        break;
    case LOBBY_SE_DECO_ATTACH:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 150, 0);
        break;
    case LOBBY_SE_DECO_REMOVE:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 151, 0);
        break;
    case 19:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 130, 0);
        break;
    case LOBBY_SE_PARTY_READY:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 140, 0);
        break;
    case 21:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, 133, 0);
        break;
    case LOBBY_SE_WINDOW_IN:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 141, 0);
        break;
    case LOBBY_SE_WINDOW_SLIDE:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 142, 0);
        break;
    case LOBBY_SE_STAMP:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 143, 0);
        break;
    case 29:
        Sound_PlaySeFlat(6, 240, 0);
        break;
    case 30:
        Sound_PlaySeFlat(6, 241, 0);
        break;
    case 31:
        Sound_PlaySeFlat(6, 242, 0);
        break;
    case 32:
        Sound_PlaySeFlat(6, 243, 0);
        break;
    case 33:
        Sound_PlaySeFlat(6, 244, 0);
        break;
    case 34:
        Sound_PlaySeFlat(6, 245, 0);
        break;
    case 35:
        Sound_PlaySeFlat(6, 246, 0);
        break;
    case 36:
        Sound_PlaySeFlat(6, 247, 0);
        break;
    case 37:
        Sound_PlaySeFlat(6, 248, 0);
        break;
    case 38:
        Sound_PlaySeFlat(6, 249, 0);
        break;
    case 39:
        Sound_PlaySeFlat(6, 250, 0);
        break;
    case 40:
        Sound_PlaySeFlat(6, 251, 0);
        break;
    case 41:
        Sound_PlaySeFlat(6, 85, 0);
        break;
    case 42:
        Sound_PlaySeFlat(6, 86, 0);
        break;
    case 43:
        Sound_PlaySeFlat(6, 87, 0);
        break;
    case 49:
        Sound_PlaySeFlat(6, 216, 0);
        break;
    case 50:
        Sound_PlaySeFlat(6, 217, 0);
        break;
    case 51:
        Sound_PlaySeFlat(6, 218, 0);
        break;
    case 52:
        Sound_PlaySeFlat(6, 219, 0);
        break;
    case 53:
        Sound_PlaySeFlat(6, 220, 0);
        break;
    case 54:
        Sound_PlaySeFlat(6, 221, 0);
        break;
    case LOBBY_SE_GALLERY_WALL:
        Sound_PlaySeFlat(6, 107, 0);
        break;
    case LOBBY_SE_GALLERY_SWING:
        Sound_PlaySeFlat(6, 106, 0);
        break;
    case LOBBY_SE_REFUSE_ALT:
    case LOBBY_SE_REFUSE:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
        break;
    case LOBBY_SE_PAGE:
        Sound_PlaySeFlat(SE_SLOT_COMMON, 152, 0);
        break;
    case LOBBY_SE_NOTICE:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_NOTICE, 0);
        break;
    case 61:
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, 155, 0);
        break;
    }
}

/* 1 while the screen fade runs (FADE_STATE_RUNNING), else 0. */
s32 LobbyFade_IsRunning(void)
{
    s32 state;

    state = FadeA_GetState() & 0xFF;
    switch (state) {
    case FADE_STATE_IDLE:
        return 0;
    case FADE_STATE_RUNNING:
        return 1;
    case FADE_STATE_HELD:
        return 0;
    default:
        return 0;
    }
}

/* Prints the text at x, y. */
void Text_PrintAt(char *text, f32 x, f32 y)
{
    Font_SetPos(x, y);
    Text_Printf("%s", text);
}

/* Sets the font's colour by index. */
void Text_SetColorIndex(s16 i)
{
    Font_SetColorIndex(i);
}

/* The index of the first free entry of the friend list at list (max entries
 * of FriendEntry; free: the id's first byte is 0), or -1 when it is full. */
s32 FriendList_FindFree(s8 *list, s32 max)
{
    s32 n;
    s32 i;
    s8 *entry;

    entry = list;
    n = max & 0xFF;
    for (i = 0; i < n; i++, entry += sizeof(FriendEntry)) {
        if (*entry == 0) {
            return i;
        }
    }
    return -1;
}

/* The index of the entry of the friend list at list (max entries of FriendEntry)
 * whose user id is key, or -1. */
s32 FriendList_FindId(s8 *list, s32 max, void *key)
{
    s32 i;
    s8 *entry;
    s32 n;

    n = max & 0xFF;
    i = 0;
    if (0 < n) {
        entry = list;
        do {
            if ((*entry != 0) && (memcmp((void *) entry, key, LOBBY_USER_ID_LEN) == 0)) {
                return i;
            }
            i++;
            entry += sizeof(FriendEntry);
        } while (i < n);
    }
    return -1;
}


/* Removes entry idx of the friend list at list (n entries): the later ones
 * move down and the last is cleared. 0 when idx is past the end, else 1. */
s32 FriendList_DeleteAt(FriendEntryWords *list, u8 idx, u8 n)
{
    u8 i;

    if (idx >= n) {
        return 0;
    }
    for (i = idx + 1; i <= n; i++) {
        if (i < n) {
            list[i - 1] = list[i];
            list[i].idFirst = 0;
            list[i].nameFirst = 0;
        } else {
            list[i - 1].idFirst = 0;
            list[i - 1].nameFirst = 0;
        }
    }
    return 1;
}

/* The entries of the block list at list (n at most) before the first
 * free one. */
s32 IdList_Count(s8 *list, u8 n)
{
    s32 count = 0;
    s32 i;

    for (i = 0; i < n; i++, list += sizeof(BlockedUser)) {
        if (*list == 0) {
            break;
        }
        count++;
    }
    return count;
}

/* The index of the first free entry of the block list at list (max entries
 * of BlockedUser; free: the id's first byte is 0), or -1 when it is full. */
s32 IdList_FindFree(s8 *list, s32 max)
{
    s32 n;
    s32 i;
    s8 *entry;

    entry = list;
    n = max & 0xFF;
    for (i = 0; i < n; i++, entry += sizeof(BlockedUser)) {
        if (*entry == 0) {
            return i;
        }
    }
    return -1;
}

/* The index of the entry of the block list at list (max entries of BlockedUser)
 * whose user id is key, or -1. */
s32 IdList_Find(s8 *list, s32 max, void *key)
{
    s32 i;
    s8 *entry;
    s32 n;

    n = max & 0xFF;
    i = 0;
    if (0 < n) {
        entry = list;
        do {
            if ((*entry != 0) && (memcmp((void *) entry, key, LOBBY_USER_ID_LEN) == 0)) {
                return i;
            }
            i++;
            entry += sizeof(BlockedUser);
        } while (i < n);
    }
    return -1;
}

/* Removes entry idx of the block list at list (n entries): the later ones
 * move down and the last is cleared. 0 when idx is past the end, else 1. */
s32 IdList_Remove(BlockedUser *list, u8 idx, u8 n)
{
    u8 i;

    if (idx >= n) {
        return 0;
    }
    for (i = idx + 1; i <= n; i++) {
        if (i < n) {
            list[i - 1] = list[i];
            list[i].userId[0] = 0;
            list[i].name[0] = 0;
        } else {
            list[i - 1].userId[0] = 0;
            list[i - 1].name[0] = 0;
        }
    }
    return 1;
}

/* 1 while gSysBusy is set (a card save or a lobby network step runs). */
s32 Lobby_IsSysBusy(void)
{
    return gSysBusy != 0;
}

extern u8 gMenuWork[];
