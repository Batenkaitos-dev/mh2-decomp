/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "menu_task.h"
#include "browser.h"
s32 memcmp(void *, void *, s32);

/* prototypes */
s32 LobbySrv_IsCur(char *id);
void LobbyDialog_DrawPending(MenuTask *task);
s32 LobbyLoc_Has(void);
/* end of prototypes */
extern char gCurServerId[];                 /* the current server id (sub_main) */
extern M2C_UNK D_5C0AD4;

void Text_SetSize(s32);
void LobbyDialog_Draw(void);
extern LobbyMenu *gLobbyMenu;
extern s8 gLobbyLocText;

/* 1 if id is the player's */
s32 LobbySrv_IsCur(char *id)
{
    return memcmp(gCurServerId, id, 0xC) == 0;
}

/* the index of server id, 0 if none */
s32 LobbySrv_Index(char *id)
{
    s32 i;
    LbsRow *f;

    for (i = 0, f = gLobbyServers.row; i < 40; i++, f++) {
        if (memcmp(id, f, 0xC) == 0) {
            break;
        }
    }
    if (i <= 39) {
        return i;
    }
    return 0;
}

/* 1 if id is in the lobby server list */
s32 LobbySrv_Exists(char *id)
{
    s32 i;
    LbsRow *f;

    for (i = 0, f = gLobbyServers.row; i < 40; i++, f++) {
        if (memcmp(id, f, 0xC) == 0) {
            return 1;
        }
    }
    return 0;
}

s32 LobbySrv_ValueAt(s32 i)
{
    s32 ret;

    ret = -1;
    if (i < 0x28) {
        ret = *(M2C_UNK *)((u8 *)&D_5C0AD4 + (i * 0x114));
    }
    return ret;
}

/* server id: its word at +0x104, 0 if none */
s32 LobbySrv_Value(char *id)
{
    s32 i;
    LbsRow *f;

    for (i = 0, f = gLobbyServers.row; i < 40; i++, f++) {
        if (memcmp(id, f, 0xC) == 0) {
            return *(s32 *)((u8 *)&D_5C0AD4 + i * 0x114);
        }
    }
    return 0;
}

void LobbyDialog_DrawPending(MenuTask *task) {
    u8 temp_a0;

    Text_SetSize(task->textSize);
    temp_a0 = gLobbyMenu->redraw;
    if (temp_a0 == 1) {
        LobbyDialog_Draw();
        gLobbyMenu->redraw = 0U;
    }
}

s32 LobbyLoc_Has(void) {
    return gLobbyLocText != 0;
}

s8 *LobbyLoc_Text(void)
{
    return &gLobbyLocText;
}
