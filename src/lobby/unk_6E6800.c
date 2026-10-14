/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_state.h"

void Menu_SetYesNo(s8);
void LobbyDialog_SetMember(void *, void *);
void LobbyText_Open(s32);
void *memcpy(void *dst, const void *src, u32 n);
extern M2C_UNK gAskUserName;
extern M2C_UNK gAskUserId;
extern LobbyMenu *gLobbyMenu;

void FriendList_AskAdd(void *id, void *name) {
    gLobbyMenu->redraw = 1;
    gLobbyMenu->subStep = 0;
    LobbyText_Open(0x25);
    Menu_SetYesNo(0);
    if ((id != 0) && (name != 0)) {
        memcpy(&gAskUserId, (void *) id, 8);
        memcpy(&gAskUserName, (void *) name, 0x11);
    }
    LobbyDialog_SetMember(&gAskUserId, &gAskUserName);
    gFieldControl = 0;
}

extern char gMyUserId[];
void LobbySe_Play(s32 se);
typedef struct Entry5C39D0 {
    u8 name[8];
    u8 text[0x28];
} Entry5C39D0;
extern Entry5C39D0 gFriendList[];
extern s8 gFriendStatus[];
s32 Menu_YesNoInput(void);
s32 memcmp(void *a, void *b, s32 n);
s32 FriendList_FindId(s8 *arg0, s32 arg1, void *key);
s32 FriendList_FindFree(s8 *arg0, s32 arg1);
void *memset(void *d, s32 c, u32 n);
s32 RateFlag_SetAndNotify(s32 a);
s32 Award_Grant(s32 a);
s32 FriendList_AddInput(u16 pad)
{
    s32 i;
    u8 *p;

    if (pad & 0x8000) {
        return 4;
    }
    gLobbyMenu->redraw = 1;
    switch (gLobbyMenu->subStep) {
    case 0:
        switch (Menu_YesNoInput()) {
        case 0:
            gLobbyMenu->subStep++;
            if (memcmp(&gAskUserId, gMyUserId, 8) == 0) {
                gLobbyMenu->subStep = 3;
                LobbyText_Open(0x26);
            } else if (FriendList_FindId((s8 *) gFriendList, 0x32, &gAskUserId) == -1) {
                if (FriendList_FindFree((s8 *) gFriendList, 0x32) == -1) {
                    gLobbyMenu->subStep = 3;
                    LobbyText_Open(0x2D);
                } else {
                    LobbyText_Open(0x2C);
                    Menu_SetYesNo(0);
                }
            } else {
                LobbyText_Open(0x2B);
                Menu_SetYesNo(0);
            }
            break;
        case 3:
            gLobbyMenu->subStep = 3;
            LobbyText_Open(0x28);
            break;
        }
        break;
    case 1:
        switch (Menu_YesNoInput()) {
        case 0:
            i = FriendList_FindId((s8 *) gFriendList, 0x32, &gAskUserId);
            if (i == -1) {
                i = FriendList_FindFree((s8 *) gFriendList, 0x32);
                if (i == -1) {
                    i = 0;
                }
                LobbyText_Open(0x29);
            } else {
                LobbyText_Open(0x2A);
            }
            p = (u8 *)&gFriendList[i];
            memset(p, 0, 8);
            memcpy((s32 *)p, &gAskUserId, 8);
            memset(p += 8, 0, 0x10);
            memcpy((s32 *)p, &gAskUserName, 0x10);
            gFriendStatus[i] = -1;
            gLobbyMenu->subStep++;
            if (i == 0x31) {
                if (!(gSave.rateFlags & 0x10)) {
                    RateFlag_SetAndNotify(4);
                }
                Award_Grant(0x23);
            }
            break;
        case 3:
            FriendList_AskAdd(0, 0);
            gLobbyMenu->subStep = 0;
            break;
        }
        break;
    case 2:
        if (pad & 0x20) {
            gLobbyMenu->subStep = 0;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(0);
            gFieldControl = 1;
            FriendList_FindId((s8 *) gFriendList, 0x32, &gAskUserId);
            return 0;
        }
        break;
    case 3:
        if (pad & 0x20) {
            gLobbyMenu->subStep = 0;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(0);
            gFieldControl = 1;
            return 1;
        }
        break;
    }
    return 2;
}

void BlockList_AskAdd(void *id, void *name) {
    gLobbyMenu->redraw = 1;
    gLobbyMenu->subStep = 0;
    LobbyText_Open(0x30);
    Menu_SetYesNo(0);
    if ((id != 0) && (name != 0)) {
        memcpy(&gAskUserId, (void *) id, 8);
        memcpy(&gAskUserName, (void *) name, 0x11);
    }
    LobbyDialog_SetMember(&gAskUserId, &gAskUserName);
    gFieldControl = 0;
}

extern char gMyUserId[];
extern u8 gMailBlockList[][0x18];
s32 memcmp(void *a, void *b, s32 n);
s32 Menu_YesNoInput(void);
s32 IdList_Find(s8 *arg0, s32 arg1, void *name);
s32 IdList_FindFree(s8 *arg0, s32 arg1);
void *memset(void *d, s32 c, u32 n);
void LobbySe_Play(s32 se);

s32 BlockList_AddInput(u16 pad)
{
    s32 i;
    u8 *p;

    if (pad & 0x8000) {
        return 4;
    }
    gLobbyMenu->redraw = 1;
    switch (gLobbyMenu->subStep) {
    case 0:
        switch (Menu_YesNoInput()) {
        case 0:
            gLobbyMenu->subStep++;
            if (memcmp(&gAskUserId, gMyUserId, 8) == 0) {
                gLobbyMenu->subStep = 3;
                LobbyText_Open(0x31);
            } else if (IdList_Find((s8 *) gMailBlockList, 8, &gAskUserId) == -1) {
                if (IdList_FindFree((s8 *) gMailBlockList, 8) == -1) {
                    gLobbyMenu->subStep = 3;
                    LobbyText_Open(0x36);
                } else {
                    LobbyText_Open(0x35);
                    Menu_SetYesNo(0);
                }
            } else {
                LobbyText_Open(0x34);
                Menu_SetYesNo(0);
            }
            break;
        case 3:
            gLobbyMenu->subStep = 3;
            LobbyText_Open(0x31);
            break;
        }
        break;
    case 1:
        switch (Menu_YesNoInput()) {
        case 0:
            i = IdList_Find((s8 *) gMailBlockList, 8, &gAskUserId);
            if (i == -1) {
                i = IdList_FindFree((s8 *) gMailBlockList, 8);
                if (i == -1) {
                    i = 0;
                }
                LobbyText_Open(0x32);
            } else {
                LobbyText_Open(0x33);
            }
            p = gMailBlockList[i];
            memset(p, 0, 8);
            memcpy((M2C_UNK *)p, &gAskUserId, 8);
            memset(p += 8, 0, 0x10);
            memcpy((M2C_UNK *)p, &gAskUserName, 0x10);
            gLobbyMenu->subStep++;
            break;
        case 3:
            BlockList_AskAdd(0, 0);
            gLobbyMenu->subStep = 0;
            break;
        }
        break;
    case 2:
        if (pad & 0x20) {
            gLobbyMenu->subStep = 0;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(0);
            gFieldControl = 1;
            return 0;
        }
        break;
    case 3:
        if (pad & 0x20) {
            gLobbyMenu->subStep = 0;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(0);
            gFieldControl = 1;
            return 1;
        }
        break;
    }
    return 2;
}

void BlockList_AskRemove(void *e, u8 *name) {
    gLobbyMenu->redraw = 1;
    gLobbyMenu->subStep = 0;
    LobbyText_Open(0x37);
    Menu_SetYesNo(0);
    if ((e != 0) && (name != 0)) {
        memcpy(&gAskUserId, (void *) e, 8);
        memcpy(&gAskUserName, (void *) name, 0x11);
    }
    LobbyDialog_SetMember(&gAskUserId, &gAskUserName);
    gFieldControl = 0;
}
