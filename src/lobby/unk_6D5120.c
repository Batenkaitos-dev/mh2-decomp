/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "save.h"

void CamFocus_End(void);
void LobbyMenu_Init(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
void LobbySe_Play(s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void *memset(void *d, s32 c, u32 n);
extern M2C_UNK gMenuWork;
extern M2C_UNK TownGuide_Draw;

void Menu4E_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    gMenuInputLock = 6;
    CamFocus_End();
}

/* The item gift menu (gMenuWork): an item of the save's list at 0x376C. */
typedef struct GiftMenu {
    /* 0x0 */ u16 item;
    /* 0x2 */ s16 count;
    /* 0x4 */ s16 idx;              /* in gSave.npcRecsA */
    /* 0x6 */ s8 step;
} GiftMenu;
u16 LobbyPad_Get(s32 a);
s32 Talk_Update(void);
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
u8 Notice_Push(u8, s16, s16);
void Menu_PushNotice(s32 msg);
void Menu4E_Update(void)
{
    s16 n;
    GiftMenu *m = (GiftMenu *)&gMenuWork;

    LobbyPad_Get(0);
    switch (m->step) {
    case 0:
        switch (Talk_Update()) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 1:
        n = m->count;
        if (SavePouch_Space(m->item) >= n) {
            SavePouch_Add(m->item, n);
            Notice_Push(1, 0xD, m->item);
            memset(&gSave.npcRecsA[m->idx], 0, 4);
        } else {
            Menu_PushNotice(0x15);
        }
        Menu4E_Close();
        break;
    }
}

void TownGuide_Open(void) {
    gTownMenuOpen = 1;
    gTownMode = 0x4B;
    memset(&gMenuWork, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(&TownGuide_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    LobbySe_Play(0);
}

void TownGuide_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
}
