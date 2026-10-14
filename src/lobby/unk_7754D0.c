#include "common.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"

void Text_SetSize(s32 size);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void PcCard_Draw(void *);
void LobbyMember_LoadEquip(u8 pl);
void LobbySe_Play(s32 se);
void MyHouse_DrawLabel(void);
s32 MemberCard_Draw(u8, u8);
void MemberCard_DrawFriendReg(s16, u8);
s32 Kbd_IsOpen(void); /* unit's own declaration: the definition (sub_main) returns s8;
 * PcCard_Update is 151 words off with s8 */
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern LobbyMenu *gLobbyMenu;

void PcCard_Open(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 *p;

    LobbyMenu_Init();
    LobbyMenu_SetDraw(NULL, PcCard_Draw, NULL, NULL);
    p = pl->target;
    if (p == NULL || p[0] == 0) {
        gTownMode = 0;
        gTownMenuOpen = 0;
        return;
    }
    gHelpShow = 0;
    gLobbyMenu->cursor = *(u16 *)(p + 0xC);
    LobbyMember_LoadEquip(gLobbyMenu->cursor);
    LobbySe_Play(8);
}

extern NetUser gNetUsers[];
extern u8 gTownMsg0DPending;
void LobbyText_Open(s32);
void LobbyDialog_Grow(void);
void FriendList_AskAdd(void *id, void *name);
s32 FriendList_AddInput(u16 pad);
s32 memcmp(void *a, void *b, s32 n);

s32 PcCard_Update(u16 pad)
{
    u8 *b;

    switch (gLobbyMenu->screen) {
    case 0:
        if (gNetUsers[gLobbyMenu->cursor].id[0] == 0) {
            LobbySe_Play(4);
            return 1;
        }
        if (Kbd_IsOpen() != 0) {
            return 0;
        }
        if ((pad & 0x200) && gLobbyMenu->step == 0) {
                        if (((u8 *)gLobby + gLobbyMenu->cursor * 0x11C)[0xA41] != 0) {
                gLobbyMenu->screen = 2;
                gFieldControl = 0;
                LobbyText_Open(0x93);
                LobbyDialog_Grow();
                LobbySe_Play(8);
                gLobbyMenu->redraw = 1;
            } else {
                gLobbyMenu->screen++;
                gLobby->connected = 0;
                b = (u8 *)gLobby + gLobbyMenu->cursor * 0x11C;
                FriendList_AskAdd(b + 0xA16, b + 0xA1E);
                LobbySe_Play(8);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 1;
        } else if (pad & 0xC00) {
            gLobbyMenu->step ^= 1;
            LobbySe_Play(1);
        }
        break;
    case 1:
        switch (FriendList_AddInput(pad & ~0x8000)) {
        case 0:
        case 1:
            gLobbyMenu->screen--;
            gLobby->connected = 1;
            gTownMsg0DPending = 1;
            if (memcmp(&gNetUsers[gLobbyMenu->cursor], &gLobby->members[gLobbyMenu->cursor], 0x10) != 0) {
                LobbySe_Play(4);
                return 1;
            }
            break;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            gLobbyMenu->screen = 0;
            gFieldControl = 1;
            LobbySe_Play(8);
        }
        break;
    }
    return 0;
}

void PcCard_Draw(void *t)
{
    u8 var_s0;
    u8 i;

    var_s0 = 0;
    Text_SetSize(*(s32 *)((u8 *)t + 0x18));
    MyHouse_DrawLabel();
    if (Kbd_IsOpen() == 0) {
        if (gLobbyMenu->redraw != 0 || gLobbyMenu->screen > 0) {
            var_s0 = 0x80;
        }
        MemberCard_Draw(gLobbyMenu->cursor, gLobbyMenu->step | var_s0);
        i = gLobbyMenu->screen;
        if ((i != 2) && (gLobbyMenu->step == 0)) {
            MemberCard_DrawFriendReg(0x184, i);
        }
    }
}
