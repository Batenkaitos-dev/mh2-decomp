/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_state.h"

void Font_SetSpacing(u8 a, u8 b);
void Text_SetSize(s32);
void HouseAccess_DrawList(s16 x, s16 y);
extern s16 gMenuWork;
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;

void HouseAccess_Draw(MenuTask *task) {
    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    HouseAccess_DrawList(gMenuWork, 0x3C);
}

void Menu_SetYesNo(u8); /* unit's own declaration: with the definition's void (s8 choice) (lobby/lobby_dialog.c), QuitMenu_Open 1 of 25 words, QuitMenu_Update 2 of 132 words differ */
void LobbyDialog_Grow(void);
void LobbyText_Open(s32);
void *memset(void *dst, s32 c, u32 n);
extern s16 gQuitMenu;
extern LobbyMenu *gLobbyMenu;
void QuitMenu_Open(void) {
    memset(&gQuitMenu, 0, 4);
    gQuitMenu = 0;
    LobbyText_Open(0x85);
    LobbyDialog_Grow();
    Menu_SetYesNo((u8) gQuitMenu);
    gLobbyMenu->redraw = 1;
    gFieldControl = 0;
}

extern s8 D_008BCD1A;
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_YesNoInput(void);
void Lobby_Disconnect(s32);
s32 QuitMenu_Update(u16 pad)
{
    s16 *m = &gQuitMenu;

    gLobbyMenu->redraw = 1;
    switch (D_008BCD1A) {
    case 0:
        if (pad & 0x20) {
            switch (*m) {
            case 0:
                LobbyText_Open(0x3C);
                break;
            case 1:
                LobbyText_Open(0x3D);
                break;
            case 2:
                LobbyText_Open(0x3E);
                break;
            }
            ((s8 *)m)[2]++;
            LobbyDialog_Grow();
            Menu_SetYesNo(1);
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            gFieldControl = 1;
            return 3;
        } else if (pad & 0x8000) {
            gFieldControl = 1;
            return 4;
        } else {
            *m = Menu_WrapUpDown(*m, 3);
            Menu_SetYesNo(*m);
        }
        break;
    case 1:
        if (pad & 0x8000) {
            gFieldControl = 1;
            return 4;
        }
        switch (Menu_YesNoInput()) {
        case 0:
            gFieldControl = 1;
            switch (*m) {
            case 0:
                Lobby_Disconnect(1);
                break;
            case 1:
                Lobby_Disconnect(3);
                break;
            case 2:
                Lobby_Disconnect(2);
                break;
            }
            break;
        case 3:
            ((s8 *)m)[2] = 0;
            LobbyText_Open(0x85);
            LobbyDialog_Grow();
            Menu_SetYesNo(*m);
            break;
        }
        break;
    }
    return 2;
}
