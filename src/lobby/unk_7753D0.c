/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"

void QuitMenu_Open(void);
s32 GameMenu_IsBusy(void);
s32 Menu_PushNoticeOnce(s32);
s32 QuitMenu_Update(s32); /* unit's own declaration: with the definition's s32 (u16 pad) (lobby/unk_6F47C0.c), GameMenu_UpdateExit 1 of 30 words differ */
s32 Chat_InInputMode(void);

s32 GameMenu_OpenExit(void *unused0) {
    if (gLobby->questTaken == 1) {
        Menu_PushNoticeOnce(1);
        return 1;
    }
    if (GameMenu_IsBusy() == 1) {
        return 1;
    }
    QuitMenu_Open();
    gHelpShow = 0;
    return 0;
}

s32 GameMenu_UpdateExit(s32 pad) {
    s32 res;

    if (Chat_InInputMode() == 1) {
        return pad;
    }
    res = QuitMenu_Update(pad);
    switch (res) {                              /* irregular */
    case 3:
        return 0x40;
    case 4:
        return 0x8000;
    default:
        return 0;
    }
}
