/*
 * plsel: the player menu of the quest-preparation screen, with its state
 * pointer in .bss and its panels in .data.
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
#include "render_state.h"
#include "area_draw.h"
#include "sound.h"
#include "text.h"
#include "font.h"
#include "game_util.h"
#include "menu_util.h"
#include "libc.h"

struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
extern u16 gStagePageStageIds[];
extern char **gStagePageTexts[];

struct ListWin;

extern s16 gHelpLine;

/* The menu's state, the field menu's (Hud_Init). The next file's .bss
 * starts on a 16-byte boundary. */
HudState *gPlselMenu;

/* The menu's panels and lists. */
ListPanel gPlselStagePagePanel = {
#include "plsel/00D76060.inc"
};
s32 QuestMenu_Begin(void); /* sub_main's hud.c (hud.h doesn't declare it) */

/* Per menu entry, the function that opens its page (nonzero: refused). */
s32 (*gPlselMenuOpens[8])(void) = {
    (s32 (*)(void))PouchMenu_Open, PersonalStatus_Open, EquipStatus_Open, QuestMenu_Begin,
    Plsel_OpenStagePage, NULL, NULL, NULL,
};
u32 gPlselMenuInputs[6] = {
    (u32)PouchMenu_Update, (u32)PersonalStatus_Update, (u32)EquipStatus_Update, (u32)QuestMenu_Update,
    (u32)Plsel_StagePageInput, 0,
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
    Sound_PlaySeFlat(0, 17, 0);
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
            refused = gPlselMenuOpens[gPlselMenu->fieldMenuCursor]();
            gPlselMenu->player = &gPlayers[gQuestSync.pick];
            if (refused == 0) {
                gPlselMenu->menuPhase++;
                gPlselMenu->state = 0;
                Sound_PlaySeFlat(0, 19, 0);
            } else {
                Sound_PlaySeFlat(0, 21, 0);
            }
        } else if (pad & (PAD_START | PAD_CROSS)) {
            Plsel_ClosePlayerMenu();
            Sound_PlaySeFlat(0, 20, 0);
        }
        break;
    case 1:
        if (gPlselMenu->fieldMenuCursor == PLSEL_MENU_POUCH) {
            pad &= ~(PAD_SELECT | PAD_CIRCLE);
        }
        ret = ((s32 (*)(s32))gPlselMenuInputs[gPlselMenu->fieldMenuCursor])(pad);
        if (ret & PAD_START) {
            Plsel_ClosePlayerMenu();
            Sound_PlaySeFlat(0, 20, 0);
        } else if (ret & PAD_CROSS) {
            Plsel_PlayerMenuBack();
            Sound_PlaySeFlat(0, 20, 0);
        }
        break;
    }
}

/* Draws the menu's list, with the online entries when online. */
void Plsel_DrawPlayerMenuList(void)
{
    ListWin box;

    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    box = gPlselMenuListBox;
    if (Game_IsOnline() == 1) {
        box.items = gPlselMenuOnlineItems;
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
    gHelpLine = item + 24;
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

/*
 * The stage page: in its panel (titled for online or offline play), the
 * lines of gQuest.stage's entry in the game overlay's per-stage text table
 * (gStagePageTexts, by the stage ids in gStagePageStageIds, which end in 0).
 */
void Plsel_DrawStagePage(void)
{
    s32 y;
    char **list;
    s32 i;
    u16 *stage;
    u16 id;

    if (Game_IsOnline() == 0) {
        ListPanel_Draw(&gPlselStagePagePanel, 0, gPlselStageTitleOffline, -1);
    } else {
        ListPanel_Draw(&gPlselStagePagePanel, 0, gPlselStageTitleOnline, -1);
    }
    i = 0;
    stage = gStagePageStageIds;
    id = *stage;
    while (id != 0) {
        if (id == gQuest.stage) {
            break;
        }
        stage++;
        id = *stage;
        i++;
    }
    list = gStagePageTexts[i];
    Text_SetColor(0);
    y = 86;
    Font_SetSpacing(18, 18);
    while (*list != 0) {
        Font_SetPos(328, y);
        Text_Printf(*list);
        list++;
        y += 20;
    }
}
