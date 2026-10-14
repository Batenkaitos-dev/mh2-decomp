/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"

void QuestList_Rebuild(void);
void LobbySe_Play(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
void CamFocus_SetAside(s8 arg0);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
extern u8 gQuestListRebuildPending;

void QuestDesk_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_SetAside(0);
    CamFocus_End();
    gMenuInputLock = 0x14;
    if (gQuestListRebuildPending != 0) {
        QuestList_Rebuild();
        gQuestListRebuildPending = 0;
    }
    LobbySe_Play(4);
    gFieldControl = 1;
}
