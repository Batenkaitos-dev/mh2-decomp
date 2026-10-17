/*
 * plsel: the player menu of the quest-preparation screen (0xD64560-0xD64C80,
 * with its .bss pointer at 0xD7BD80 and its tables in .data).
 *
 * plsel.c opens it (Plsel_OpenPlayerMenu) for one of the players and runs
 * it each frame (Plsel_PlayerMenuUpdate, Plsel_DrawPlayerMenu). It reuses
 * the field menu's state (HudState, which Hud_Init returns) and sub_main's
 * pages: the pouch, the personal status, the equipment status, the quest's
 * information, and a page of this file's own (Plsel_DrawStagePage).
 */
#include "common.h"
#include "pad.h"
#include "hud.h"
extern u8 gHelpShow;
extern u8 gHelpPage;
extern s8 gHelpColumn;
#include "scene.h"
#include "m2c_macros.h"
#include "plsel.h"
#include "player.h"
#include "save.h"
#include "qsync.h"


s32 Plsel_OpenStagePage(void);
s32 Plsel_StagePageInput(s32 arg0);
void Plsel_DrawPouchPage(void);

void ItemList_DrawPouch(s32, s32, s32);
s32 Game_IsOnline(void);
void HelpWindow_DrawCurrent(void);
void Plsel_DrawPlayerMenuList(void);
void Plsel_DrawStagePage(void);

void *Hud_Init(void);
void *memcpy(void *, const void *, u32);
void Player_RecomputeStats(Player *);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Text_SetColor(s16 color);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(const char *fmt, ...);
extern u16 D_896AF0[];
extern s32 *D_896A80[];

void Gfx_SetBlend(s32, s32);
struct ListWin;
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel);
extern char *D_446F90[];

s32 Plsel_ChatUpdate(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);

extern s16 gHelpLine;

/* .bss (0xD7BD80): the menu's state, the field menu's (Hud_Init). The
 * next unit's .bss starts 16-aligned. */
HudState *gPlselMenu;

/* .data (0xD76060-0xD76100) */
ListPanel gPlselStagePagePanel = {
#include "plsel/00D76060.inc"
};
void EquipStatus_Draw(void); /* sub_main */
s32 EquipStatus_Open(void); /* sub_main */
u16 EquipStatus_Update(u16 buttons); /* sub_main */
void PersonalStatus_Draw(void); /* sub_main */
s32 PersonalStatus_Open(void); /* sub_main */
u16 PersonalStatus_Update(u16 buttons); /* sub_main */
s32 PouchMenu_Open(void *m); /* sub_main */
u16 PouchMenu_Update(u16 buttons); /* sub_main */
void QuestMenu_DrawInfo(void); /* sub_main */
u16 QuestMenu_Update(u16 buttons); /* sub_main */
u32 gPlselMenuOpens[8] = {
#include "plsel/00D76080.inc"
};
u32 gPlselMenuInputs[6] = {
    (u32)PouchMenu_Update, (u32)PersonalStatus_Update, (u32)EquipStatus_Update, (u32)QuestMenu_Update,
    (u32)Plsel_StagePageInput, 0x00000000,
};
u8 gPlselMenuHelpOffline[8] = {
#include "plsel/00D760B8.inc"
};
u8 gPlselMenuHelpOnline[16] = {
#include "plsel/00D760C0.inc"
};
u32 gPlselMenuDraws[4] = {
    (u32)Plsel_DrawPouchPage, (u32)PersonalStatus_Draw, (u32)EquipStatus_Draw, (u32)QuestMenu_DrawInfo,
};
extern char *D_00446F70[];
ListWin gPlselMenuListBox = {
#include "plsel/00D760E0.inc"
};

/* Closes the player menu and its help line. */
void Plsel_ClosePlayerMenu(void)
{
    gPlselMenu->menuPhase = 0;
    gScene.menuOpen = 0;
    gHelpShow = 0;
}

/*
 * Opens the player menu for player slot `player`: takes the field menu's
 * state from Hud_Init, puts the cursor on the first entry with its help
 * line, gives the player the save's name, recomputes the player's stats
 * with no meal's bonus, and plays the open sound.
 */
void Plsel_OpenPlayerMenu(u16 player)
{
    Player *pl;

    gPlselMenu = Hud_Init();
    gPlselMenu->menuPhase = 0;
    gScene.menuOpen = 1;
    gPlselMenu->fieldMenuCursor = 0;
    gHelpShow = 1;
    gHelpPage = 0;
    if (Game_IsOnline() == 1) {
        gHelpLine = gPlselMenuHelpOnline[gPlselMenu->fieldMenuCursor];
    } else {
        gHelpLine = gPlselMenuHelpOffline[gPlselMenu->fieldMenuCursor];
    }
    gPlselMenu->player = &gPlayers[player];
    pl = gPlselMenu->player;
    memcpy(pl->name, gSave.name, sizeof(gSave.name));
    gScene.mealAttack = gScene.mealDefense = gScene.mealHealth = gScene.mealStamina = 0;
    Player_RecomputeStats(pl);
    Sound_PlaySeFlat(0, 0x11, 0);
}

/* Goes back from a page to the menu's list, with the entry's help line. */
void Plsel_PlayerMenuBack(void)
{
    gPlselMenu->menuPhase = 0;
    gHelpShow = 1;
    gHelpPage = 0;
    if (Game_IsOnline() == 1) {
        gHelpLine = gPlselMenuHelpOnline[gPlselMenu->fieldMenuCursor];
    } else {
        gHelpLine = gPlselMenuHelpOffline[gPlselMenu->fieldMenuCursor];
    }
}

/*
 * The player menu's input, each frame (the chat window first takes it when
 * open). On the list: the cursor, its help line (the online or offline
 * table), confirm opens the entry's page (gPlselMenuOpens; a refusal plays
 * the error sound), start or cancel closes the menu. On a page: the page's
 * input function (gPlselMenuInputs; the pouch's page gets neither select
 * nor confirm) returns start to close the menu or cancel to go back.
 */
void Plsel_PlayerMenuUpdate(u16 pad)
{
    s32 refused;
    u16 ret;

    if (Plsel_ChatUpdate() == 1) {
        return;
    }
    switch (gPlselMenu->menuPhase) {
    case 0:
        Menu_CursorUpDown(&gPlselMenu->fieldMenuCursor, pad, PLSEL_MENU_COUNT);
        if (Game_IsOnline() == 1) {
            gHelpLine = gPlselMenuHelpOnline[gPlselMenu->fieldMenuCursor];
        } else {
            gHelpLine = gPlselMenuHelpOffline[gPlselMenu->fieldMenuCursor];
        }
        if (pad & PAD_CIRCLE) {
            refused = ((s32 (*)(void))gPlselMenuOpens[gPlselMenu->fieldMenuCursor])();
            gPlselMenu->player = &gPlayers[gQuestSync.pick];
            if (refused == 0) {
                gPlselMenu->menuPhase++;
                gPlselMenu->state = 0;
                Sound_PlaySeFlat(0, 0x13, 0);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
        } else if (pad & (PAD_START | PAD_CROSS)) {
            Plsel_ClosePlayerMenu();
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        break;
    case 1:
        if (gPlselMenu->fieldMenuCursor == PLSEL_MENU_POUCH) {
            pad &= ~(PAD_SELECT | PAD_CIRCLE);
        }
        ret = ((s32 (*)(s32))gPlselMenuInputs[gPlselMenu->fieldMenuCursor])(pad);
        if (ret & PAD_START) {
            Plsel_ClosePlayerMenu();
            Sound_PlaySeFlat(0, 0x14, 0);
        } else if (ret & PAD_CROSS) {
            Plsel_PlayerMenuBack();
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        break;
    }
}

extern char *gFieldMenuTitle[];
/* Draws the menu's list, with the online entries when online. */
void Plsel_DrawPlayerMenuList(void)
{
    ListWin box;

    Gfx_SetBlend(4, 5);
    box = gPlselMenuListBox;
    if (Game_IsOnline() == 1) {
        box.items = D_446F90;
    }
    ListPanel_DrawDim(&box, gFieldMenuTitle[0], gPlselMenu->fieldMenuCursor);
}

/*
 * Draws the player menu: the list, or the open entry's page
 * (gPlselMenuDraws, or the stage page), then the help window when shown.
 */
void Plsel_DrawPlayerMenu(void)
{
    HudState *menu;

    menu = gPlselMenu;
    switch (menu->menuPhase) {
    case 0:
        Plsel_DrawPlayerMenuList();
        break;
    case 1:
        switch (menu->fieldMenuCursor) {
        case PLSEL_MENU_POUCH:
        case PLSEL_MENU_STATUS:
        case PLSEL_MENU_EQUIPMENT:
        case PLSEL_MENU_QUEST:
            ((void (*)(void))gPlselMenuDraws[menu->fieldMenuCursor])();
            break;
        case PLSEL_MENU_STAGE:
            Plsel_DrawStagePage();
            return;
        }
        break;
    }
    if (gHelpShow != 0) {
        HelpWindow_DrawCurrent();
    }
}

/*
 * The pouch page: the help line of the item in the selected slot, then the
 * pouch's item list.
 */
void Plsel_DrawPouchPage(void)
{
    HudState *menu;
    u16 item;

    gHelpShow = 1;
    gHelpPage = 1;
    menu = gPlselMenu;
    item = (s16)menu->player->pouch[menu->slot].item;
    gHelpColumn = 0;
    gHelpLine = item + 0x18;
    ItemList_DrawPouch(menu->slot, 0xA9182, 8);
}

s32 Plsel_OpenStagePage(void)
{
    return 0;
}

s32 Plsel_StagePageInput(s32 buttons)
{
    return buttons;
}

extern s32 D_00446F80;
extern s32 D_00446FA0;
/*
 * The stage page: in its panel (titled for online or offline play), the
 * lines of gQuest.stage's entry in the game overlay's per-stage text table
 * (D_896A80, by the stage ids in D_896AF0, which end in 0).
 */
void Plsel_DrawStagePage(void)
{
    s32 y;
    s32 *list;
    s32 i;
    u16 *stage;
    u16 id;

    if (Game_IsOnline() == 0) {
        ListPanel_Draw(&gPlselStagePagePanel, 0, (char *) D_00446F80, -1);
    } else {
        ListPanel_Draw(&gPlselStagePagePanel, 0, (char *) D_00446FA0, -1);
    }
    i = 0;
    stage = D_896AF0;
    id = *stage;
    while (id != 0) {
        if (id == gQuest.stage) {
            break;
        }
        stage++;
        id = *stage;
        i++;
    }
    list = D_896A80[i];
    Text_SetColor(0);
    y = 0x56;
    Font_SetSpacing(0x12, 0x12);
    while (*list != 0) {
        Font_SetPos(0x148, y);
        Text_Printf((const char *)*list);
        list++;
        y += 0x14;
    }
}
