/*
 * The records menu's input on its first screens and every draw
 * (TOWN_MODE_RECORDS; the state is the RecordMenu view of gMenuWork):
 * - the kinds menu's input (Records_MenuInput: the records copied from
 *   the save, or asked of the house's owner and waited for) and the house
 *   access page's (Records_HouseAccessInput);
 * - the draws: the kinds menu, each kind's first page (the item
 *   categories, the weapon genres, the armour types, the title tabs), the
 *   lists (titles, items, weapons, armour, with the equipment's details),
 *   the house access page, the guides (gRecordGuideCats' categories, entries
 *   and text pages, defined at the end of the file), the horn score pages
 *   and the hunts with their size crowns;
 * - Records_Draw and Records_DrawMenuLayer, the menu's two draw callbacks.
 * Records_Update runs the menu.
 */
#include "common.h"
#include "lobby_text.h"
#include "item.h"
#include "lobby_vobj.h"
#include "lobby_state.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "pad.h"
#include "em.h"
#include "lobby_npc.h"
#include "lobby_labels.h"
char *strcpy(char *, const char *);
void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);
s32 ItemBits_Test(u32 *bits, u16 n);

extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
void Equip_DrawPreviewHelp(u32 color, s8 k);
s32 Equip_IsNoModelArmor(EquipItem *e);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *s, ...);
void EquipWindow_DrawLocal(EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other);
void Text_Printf(char *s, ...);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
/* unit's own declaration: as the callers use it; with the definition's void (s16 x, s16 y, s16 page, s16 n, u8 alt), the calls in Records_DrawHunts compile differently */
void Menu_DrawPageNum(s16 x, s16 y, s32 page, s32 pages, s32 a);
void Text_PrintfStyled(s32, char *, ...);
extern char *gHornScorePages[];
extern char *gOldHornScorePages[];
extern u8 gMenuWork[];
void Text_SetSize(s32 size);
void Records_DrawMenu(RecordMenu *menu, s16 x, s16 y);
u16 MonsterGroup_Sum(u16 *counts, u8 group, s32 unused);

/* Counts per item category the items that exist and those the records
 * show obtained (tally[category][1] and [0]), and resets the category
 * cursor and the sub cursor. */
void Records_TallyItems(RecordMenu *menu)
{
    s32 i;
    ItemData *item;
    u8 *tallyBase;

    menu->sel = 0;
    menu->subCursor = 0;
    memset(menu->tally, 0, sizeof(menu->tally));
    for (i = 0, item = gItemData; i < ITEM_ID_COUNT; i++, item++) {
        /* tallyBase is the menu plus the category's 8 bytes of tally: the
         * game's code keeps that one base for both counts, which the array
         * form doesn't give */
        tallyBase = (u8 *)menu + item->category * 8;
        (*(s32 *)(tallyBase + 0x6E0))++; /* tally[category][1] */
        if (ItemBits_Test(menu->itemBits, i) == 1) {
            (*(s32 *)(tallyBase + 0x6DC))++; /* tally[category][0] */
        }
    }
}

s32 Game_IsOnline(void);
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void LobbyText_Open(s32);
void TownMsg_SendRecordsAsk(u8 c);
void Records_TallyWeapons(s32 *out);
void Records_TallyArmor(s32 *arg);
extern LobbyMenu *gLobbyMenu;

/*
 * The kinds menu's input: moves the cursor; on confirm, offline or in the
 * player's own house, copies the kind's records from the save (or tallies
 * the weapons or armour) and returns MENU_DONE; in another member's house,
 * clears them, asks the house's owner for them and waits up to 480 frames
 * with a waiting message, then shows that the owner didn't answer.
 * Returns MENU_DONE to open the kind, MENU_CANCEL to close the menu, else
 * MENU_RUNNING.
 */
s32 Records_MenuInput(RecordMenu *menu, u16 pad)
{
    s32 i;

    switch (menu->step) {
    case 0:
        if (pad & PAD_CIRCLE) {
            if (Game_IsOnline() == 0 || (u8)gLobby->memberSlot[gScene.localPlayer] == gScene.localPlayer + '0') {
                switch (menu->kind[menu->menuSel]) {
                case RECORD_KIND_ITEMS:
                    memcpy(menu->itemBits, gSave.itemBits, sizeof(menu->itemBits));
                    break;
                case RECORD_KIND_TITLES:
                    memcpy(menu->titleBits, gSave.titles, sizeof(menu->titleBits));
                    break;
                case RECORD_KIND_MONSTERS:
                    memcpy(menu->sizeMax, gSave.sizeMax, sizeof(menu->sizeMax));
                    memcpy(menu->sizeMin, gSave.sizeMin, sizeof(menu->sizeMin));
                    for (i = 0; i < MONSTER_RECORD_SLOTS; i++) {
                        menu->hunts[i] = gSave.hunted[i] + gSave.captured[i];
                    }
                    break;
                case RECORD_KIND_WEAPONS:
                    Records_TallyWeapons(menu->equipTally);
                    break;
                case RECORD_KIND_ARMOR:
                    Records_TallyArmor(menu->equipTally);
                    break;
                case RECORD_KIND_HOUSE_ACCESS:
                case RECORD_KIND_GUIDES:
                    break;
                }
                LobbySe_Play(LOBBY_SE_WINDOW_OUT);
                return MENU_DONE;
            }
            menu->step++;
            LobbyText_Open(LOBBY_TEXT_RECORDS_WAIT);
            gLobbyMenu->redraw = 1;
            switch (menu->kind[menu->menuSel]) {
            case RECORD_KIND_ITEMS:
                memset(menu->itemBits, 0, sizeof(menu->itemBits));
                break;
            case RECORD_KIND_TITLES:
                memset(menu->titleBits, 0, sizeof(menu->titleBits));
                break;
            case RECORD_KIND_MONSTERS:
                memset(menu->sizeMax, 0, sizeof(menu->sizeMax));
                memset(menu->sizeMin, 0, sizeof(menu->sizeMin));
                memset(menu->hunts, 0, sizeof(menu->hunts));
                break;
            case RECORD_KIND_GUIDES:
                menu->step = 0;
                gLobbyMenu->redraw = 0;
                LobbySe_Play(LOBBY_SE_WINDOW_OUT);
                return MENU_DONE;
            case RECORD_KIND_WEAPONS:
            case RECORD_KIND_ARMOR:
                memset(menu->equipTally, 0, sizeof(menu->equipTally));
                break;
            default:
                return MENU_RUNNING;
            }
            TownMsg_SendRecordsAsk(menu->kind[menu->menuSel]);
            menu->reqKind = menu->kind[menu->menuSel];
            menu->timer = 480; /* frames: 8 s */
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            menu->menuSel = Menu_WrapUpDown(menu->menuSel, menu->menuCount);
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (--menu->timer <= 0) {
            menu->step++;
            LobbyText_Open(LOBBY_TEXT_RECORDS_NO_REPLY);
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->step = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    case 3:
        menu->step = 0;
        LobbySe_Play(LOBBY_SE_WINDOW_OUT);
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/records_draw", gRecordsFmtString);
INCLUDE_RODATA("asm/lobby/nonmatchings/records_draw", gRecordsFmtRowNumber);
INCLUDE_RODATA("asm/lobby/nonmatchings/records_draw", gRecordsFmtEquipName);

struct Em *Npc_Find(u8 kind);
extern u8 gHouseAccess;
extern char gHousePassword[];
extern NetUser gNetUsers[];
void HunterCard_Build(void *out);
void Actor_SetState(void *, s32, s32);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
s32 LobbyStep_HunterCard(void);
void LobbyDialog_Grow(void);

/*
 * The house access page (RECORD_KIND_HOUSE_ACCESS): its rows are closed,
 * open, by password and done. Closed or open marks the choice; the
 * password row opens the keyboard (an empty password warns and reopens
 * it); done stores gHouseAccess and the password, rebuilds the player's
 * hunter card, moves the house's Poogie out of the way when the house is
 * not closed (or back to its route), and waits for the change to be sent.
 * Returns MENU_DONE once sent, MENU_CANCEL when left, else MENU_RUNNING.
 */
s32 Records_HouseAccessInput(RecordMenu *menu, u16 pad)
{
    Em *npc = Npc_Find(NPC_POOGIE);

    switch (menu->step) {
    case HOUSE_ACCESS_STEP_CHOOSE:
        if (pad & PAD_CIRCLE) {
            switch (menu->sel) {
            case HOUSE_ACCESS_ROW_DONE:
                menu->step = HOUSE_ACCESS_STEP_SENDING;
                LobbyText_Open(LOBBY_TEXT_SENDING);
                gLobbyMenu->redraw = 1;
                gHouseAccess = menu->houseAccess;
                memset(gHousePassword, 0, HOUSE_PASSWORD_SIZE);
                strcpy(gHousePassword, menu->password);
                HunterCard_Build(gNetUsers[gScene.localPlayer].text);
                HunterCard_Build(gLobby->members[gScene.localPlayer].text);
                HunterCard_Build(&gPlayerText);
                if (gHouseAccess != HOUSE_ACCESS_CLOSED) {
                    if (npc != NULL && ((NpcWork *)&npc->work)->petAct == 0) {
                        Actor_SetState(npc, 0, NPC_STATE_HOUSE_OPEN);
                    }
                } else if (npc != NULL) {
                    /* back to the state of its current route point; the
                     * game reads the state's low half, and a pointer kept
                     * in a local costs the code a register it doesn't use */
                    Actor_SetState(npc, 0,
                        *(u16 *)&((NpcWork *)&npc->work)->route[((NpcWork *)&npc->work)->routeIdx].state);
                }
                break;
            case HOUSE_ACCESS_PASSWORD:
                menu->step++;
                gFieldControl = 0;
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 6, 8, menu->password);
                break;
            default:
                menu->houseAccess = menu->sel;
                menu->sel = HOUSE_ACCESS_ROW_DONE;
                memset(menu->password, 0, HOUSE_PASSWORD_SIZE);
                break;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            menu->step = HOUSE_ACCESS_STEP_CHOOSE;
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            return MENU_CANCEL;
        } else {
            menu->sel = Menu_WrapUpDown(menu->sel, HOUSE_ACCESS_ROWS);
        }
        break;
    case HOUSE_ACCESS_STEP_TYPING:
        if (Kbd_Run(menu->password, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            Kbd_Cancel();
            gFieldControl = 1;
            if (menu->password[0] != 0) {
                menu->step = HOUSE_ACCESS_STEP_CHOOSE;
                menu->houseAccess = menu->sel;
                menu->sel = HOUSE_ACCESS_ROW_DONE;
            } else {
                menu->step++;
                LobbyText_Open(LOBBY_TEXT_PASSWORD_EMPTY);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
            }
        }
        break;
    case HOUSE_ACCESS_STEP_EMPTY_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->step++;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    case HOUSE_ACCESS_STEP_REOPEN:
        menu->step = HOUSE_ACCESS_STEP_TYPING;
        Chat_OpenKbd(0, 6, 8, menu->password);
        break;
    case HOUSE_ACCESS_STEP_SENDING:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_HunterCard()) {
        case 0:
            menu->step = HOUSE_ACCESS_STEP_CHOOSE;
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

void Menu_DrawDecoFrame(s16 x, s16 y0, s16 w0, s16 h, s8 kind, s8 b);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
/* Draws the kinds menu framed at (x, y): one label per entry of kind,
 * spaced by their count, the one under the cursor with arrows. */
void Records_DrawMenu(RecordMenu *menu, s16 x, s16 y)
{
    char *label;
    s32 i;
    s16 labelX;
    s16 rowStep;

    Menu_DrawDecoFrame(x, y, 0x154, 0x114, 5, 0);
    switch (menu->menuCount) {
    case 9:
        rowStep = 0x1C;
        y = y + 0x1E;
        break;
    case 8:
        rowStep = 0x1E;
        y = y + 0x22;
        break;
    case 7:
        rowStep = 0x1E;
        y = y + 0x36;
        break;
    case 6:
        rowStep = 0x1E;
        y = y + 0x3C;
        break;
    case 5:
        rowStep = 0x24;
        y = y + 0x46;
        break;
    default:
        rowStep = 0x28;
        y = y + 0x46;
        break;
    }
    Font_SetColor(0x807F7F7F);
    x = x + 0xAA;
    Font_SetSpacing(0x16, 0x16);
    for (i = 0; i < menu->menuCount; i++) {
        label = gRecordsMenuTexts[menu->kind[i]];
        labelX = Text_CenterX(label, x, 0x18) + 10;
        if (i == menu->menuSel) {
            Menu_DrawArrowsRev(labelX, y, 0x16, label, 0xFFB57510);
            Text_DrawShadowRgb(labelX, y, 0xFF101010, 0xFF106060, label);
        } else {
            Font_SetColor(0xFF101010);
            Font_SetPos(labelX, y);
            Text_PrintfColored(label);
        }
        y += rowStep;
    }
}

extern char gRecordsFmtString[];
s32 sprintf(char *buf, char *fmt, ...);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
/* Draws the items' first page: the kind's title bar, the item categories
 * with arrows on sel, and the prompt; the list starts a line lower while
 * eventMark is 0. */
void Records_DrawItemCategories(RecordMenu *menu, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 rowY = y;
    s16 textX;
    char **categoryName;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_ITEMS], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x12, 0x12);
    if (menu->eventMark == 0) {
        rowY += 0x14;
    }
    for (i = 1, categoryName = &gRecordsListTexts[RECORDS_LIST_ITEM_KINDS]; i < 9; i++, categoryName++) {
        sprintf(buf, gRecordsFmtString, *categoryName);
        textX = Text_CenterX(buf, 0x1C2, 0x12);
        rowY += 0x16;
        Font_SetPos(textX, rowY);
        Text_PrintfColored(gRecordsFmtString, buf);
        if (menu->sel == i - 1) {
            Menu_DrawArrowsRev(textX, rowY, 0x12, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsMenuTexts[RECORDS_MENU_ITEM_PROMPT]);
}

/* Draws the weapons' first page: the title bar, the five genres
 * (gEquipGroupTexts 0 to 4) with arrows on genre, and the prompt. */
void Records_DrawWeaponGenres(RecordMenu *menu, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 rowY = y;
    s16 textX;
    char **groupName;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_WEAPONS], 2, 1);
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    rowY += 0x1E;
    for (i = 0, groupName = gEquipGroupTexts; i < RECORDS_GENRE_COUNT; i++, groupName++) {
        sprintf(buf, gRecordsFmtString, *groupName);
        textX = Text_CenterX(buf, 0x1C2, 0x12);
        rowY += 0x22;
        Font_SetPos(textX, rowY);
        Text_PrintfColored(gRecordsFmtString, buf);
        if (menu->genre == i) {
            Menu_DrawArrowsRev(textX, rowY, 0x14, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsMenuTexts[RECORDS_MENU_WEAPON_GENRE_PROMPT]);
}

/* The same for the armour: the five types, whose labels follow the
 * weapon genres' in gEquipGroupTexts, and its own prompt. */
void Records_DrawArmorTypes(RecordMenu *menu, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 rowY = y;
    s16 textX;
    char **groupName;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_ARMOR], 2, 1);
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    rowY += 0x1E;
    for (i = 0, groupName = gEquipGroupTexts; i < RECORDS_GENRE_COUNT; i++, groupName++) {
        sprintf(buf, gRecordsFmtString, groupName[RECORDS_GENRE_COUNT]);
        textX = Text_CenterX(buf, 0x1C2, 0x12);
        rowY += 0x22;
        Font_SetPos(textX, rowY);
        Text_PrintfColored(gRecordsFmtString, buf);
        if (menu->genre == i) {
            Menu_DrawArrowsRev(textX, rowY, 0x14, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsMenuTexts[RECORDS_MENU_ARMOR_GENRE_PROMPT]);
}

/* Draws the titles' first page: the title bar, the title tabs
 * (HunterTitle.group) with arrows on sel, and the prompt; the list starts
 * a line lower while eventMark is 0. */
void Records_DrawTitleTabs(RecordMenu *menu, s16 x, s16 y)
{
    char buf[0x80];
    s32 i;
    s16 rowY = y;
    s16 textX;
    char **groupName;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x10E, 0x12, 0x64, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_TITLES], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    rowY += 0x14;
    if (menu->eventMark == 0) {
        rowY += 0x14;
    }
    for (i = 0, groupName = gRecordsListTexts; i < 5; i++, groupName++) {
        sprintf(buf, gRecordsFmtString, *groupName);
        textX = Text_CenterX(buf, 0x1C2, 0x12);
        rowY += 0x18;
        Font_SetPos(textX, rowY);
        Text_PrintfColored(gRecordsFmtString, buf);
        if (menu->sel == i) {
            Menu_DrawArrowsRev(textX, rowY, 0x14, buf, 0xFF106060);
        }
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsMenuTexts[RECORDS_MENU_TITLE_GENRE_PROMPT]);
}

extern char *gHunterTitleDescs[];
extern char *gYesNoTexts[];
s8 MyHouse_LocalSlot(void);
s32 Bits32_Test(u32 *bits, s32 n);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2);
void Win_Draw(s32 x, s32 y, s32 w, s32 h, s32 color, s32 color2, s16 kind);
extern char gRecordsFmtRowNumber[];

/*
 * Draws a title tab's page: the title worn (the player's own, or the
 * owner's from their hunter card), the tab's name, the numbered rows (an
 * earned title by name, others as unknown), the cursor, and below: the
 * take-this-title question on step 2, else the prompt or the cursor
 * title's description (toggle). A line says when the tab is empty.
 */
void Records_DrawTitleList(RecordMenu *menu, s16 x, s16 y)
{
    HunterTitle *titleRec;
    s32 i;
    s16 rowY = y;
    s32 columnX;
    u16 id;
    u16 cursorId;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x10E, 0x12, 0x64, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_TITLES], 2, 1);
    if (Game_IsOnline() == 0 || (u8)gLobby->memberSlot[gScene.localPlayer] == gScene.localPlayer + '0') {
        titleRec = &gHunterTitles[gSave.title];
    } else {
        titleRec = &gHunterTitles[((u8 *)&gNetUsers[MyHouse_LocalSlot()])[0x2A]];
    }
    Font_SetSpacing(0x14, 0x14);
    Menu_DrawLabelBar(0x178, 0x12, 0xE6, 0xFFB0B000, titleRec->text, 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(gRecordsFmtString, gRecordsListTexts[menu->sel]);
    if (menu->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(Text_CenterX(gRecordsListTexts[RECORDS_LIST_NO_TITLE], x + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(gRecordsFmtString, gRecordsListTexts[RECORDS_LIST_NO_TITLE]);
        return;
    }
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xFA, y + 0xE, menu->page, menu->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    rowY += 0x38;
    i = 0;
    if (i < menu->rowCount) {
        do {
        Font_SetPos((s16)(x + 0x78) - 0x2E, rowY);
        Text_PrintfColored(gRecordsFmtRowNumber, i + menu->page * 8 + 1);
        id = menu->ids[i];
        titleRec = &gHunterTitles[id];
        Font_SetPos(x + 0x78, rowY);
        if (Bits32_Test(menu->titleBits, id) == 1) {
            Text_PrintfColored(gRecordsFmtString, titleRec->text);
        } else {
            Text_PrintfColored(gRecordsFmtString, gRecordsListTexts[RECORDS_LIST_UNKNOWN_TITLE]);
        }
        if (menu->cursor == i) {
            cursorId = id;
            Menu_DrawCursorRow(x + 0x32, rowY, 0xF0, 0, 0xC0106060, 0xFF106060);
        }
        rowY += 0x18;
        i++;
        } while (i < menu->rowCount);
    }
    Win_Draw(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, 0);
    if (menu->step == 2) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsMenuTexts[RECORDS_MENU_TITLE_CONFIRM]);
        Font_SetPos(0x1A2, 0x192);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    } else if (menu->toggle == 0) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsMenuTexts[RECORDS_MENU_TITLE_PROMPT]);
    } else if (Bits32_Test(menu->titleBits, (u16)menu->ids[menu->cursor])) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gHunterTitleDescs[cursorId]);
    } else {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsListTexts[RECORDS_LIST_UNKNOWN_TITLE]);
    }
}

extern char *gItemNames[];
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
void Deco_DrawDetails(u16 item, s16 x, s16 y, s8 c, u8 pal);

/*
 * Draws an item category's page: the numbered rows (an obtained item
 * with its icon and name, others as unknown), the cursor, and for an
 * obtained item its large icon, rarity and details (a decoration's by
 * page subCursor). A line says when the category is empty.
 */
void Records_DrawItemList(RecordMenu *menu, s16 x, s16 y)
{
    s32 i;
    s16 rowY = y;
    u16 id;
    u16 cursorId;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_ITEMS], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(gRecordsFmtString, gRecordsListTexts[menu->sel + 9]);
    if (menu->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(Text_CenterX(gRecordsListTexts[RECORDS_LIST_NO_ITEM], x + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(gRecordsFmtString, gRecordsListTexts[RECORDS_LIST_NO_ITEM]);
        return;
    }
    Menu_DrawPageNum(x + 0xFA, y + 0xA, menu->page, menu->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    rowY += 0x38;
    i = 0;
    if (i < menu->rowCount) {
        do {
            id = menu->ids[i];
            Font_SetPos((s16)(x + 0x8C) - 0x42, rowY);
            Text_PrintfColored(gRecordsFmtRowNumber, i + menu->page * 8 + 1);
            if (ItemBits_Test((u32 *)menu->itemBits, id) == 1) {
                Font_SetPos(x + 0x8C, rowY);
                Text_PrintfColored(gRecordsFmtString, gItemNames[id]);
                Item_DrawIcon((s16)(x + 0x8C) - 0x1C, rowY, 0x14, id);
            } else {
                Font_SetPos((s16)(x + 0x8C) - 0x14, rowY);
                Text_PrintfColored(gRecordsFmtString, gRecordsListTexts[RECORDS_LIST_UNKNOWN_ITEM]);
            }
            if (menu->cursor == i) {
                cursorId = id;
                Menu_DrawCursorRow(x + 0x32, rowY, 0xF0, 0, 0xC0106060, 0xFF106060);
            }
            rowY += 0x18;
            i++;
        } while (i < menu->rowCount);
    }
    Win_Draw(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, 0);
    if (ItemBits_Test((u32 *)menu->itemBits, cursorId) == 1) {
        Font_SetSpacing(0x12, 0x12);
        Item_DrawIcon(0x126, 0x166, 0x36, cursorId);
        Item_DrawRarity(0x12E, 0x19C, gItemData[cursorId].rarity);
        Deco_DrawDetails(cursorId, 0x16C, 0x166, menu->subCursor | 0x10, 0);
    }
}

/* Draws the details PAD_SQUARE shows in the equipment lists: the
 * preview's help, a note for armour with no model, and the equipment
 * window of the row chosen at level subCursor. */
void Records_DrawEquipDetail(RecordMenu *menu)
{
    char *note;

    Equip_DrawPreviewHelp(0xBDB59F6C, 0);
    if (Equip_IsNoModelArmor(&menu->items[menu->sel]) == 1) {
        note = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x17A, 0x9A);
        Font_SetColor(0x807F212E);
        Text_PrintfColored(note);
    }
    EquipWindow_DrawLocal(&menu->items[menu->sel], gMenuOffsetX + 0x136, gMenuOffsetY + 0xD8, menu->subCursor | 0x40, NULL);
}

extern char gRecordsFmtEquipName[];
char *Equip_GetName(u8 type, u16 id);
/* unit's own declaration: as the callers use it; with the definition's void (s32 x, s32 y, s32 w, s32 h, EquipItem *e), the calls in Records_DrawWeaponList and Records_DrawArmorList compile differently */
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void EquipShop_DrawLine(s16 x, s16 y, EquipItem *e);

/*
 * Draws a weapon genre's page: the numbered rows (name and icon), the
 * cursor, then the cursor weapon's stats line and the details button's
 * guide; the details instead while toggle is set; a line when the genre
 * is empty.
 */
void Records_DrawWeaponList(RecordMenu *menu, s16 x, s16 y)
{
    s32 i;
    s32 centerX;
    s16 rowY = y;
    char *emptyText;
    EquipItem item;
    EquipItem selItem;
    s32 baseX;
    s32 baseY;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_WEAPONS], 2, 1);
    if (menu->toggle != 0) {
        Records_DrawEquipDetail(menu);
        return;
    }
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    baseX = x;
    baseY = (s32)y;
    centerX = x;
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(gRecordsFmtString, gEquipGroupTexts[menu->genre]);
    if (menu->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        emptyText = gRecordsMenuTexts[RECORDS_MENU_NO_WEAPON];
        Font_SetPos(Text_CenterX(emptyText, centerX + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(gRecordsFmtString, emptyText);
        return;
    }
    Menu_DrawPageNum(x + 0xFA, y + 0xE, menu->page, menu->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    rowY += 0x38;
    i = 0;
    if (i < menu->rowCount) {
        do {
            item = menu->items[i];
            Font_SetPos((s16)(x + 0x78) - 0x42, rowY);
            Text_PrintfColored(gRecordsFmtRowNumber, i + menu->page * 8 + 1);
            Font_SetPos(x + 0x78, rowY);
            Text_PrintfColored(gRecordsFmtEquipName, Equip_GetName(item.type, item.id));
            Font_SetPos(x, rowY);
            Lobby_DrawEquipIcon((s16)(x + 0x78) - 0x1C, rowY, 0x14, 0x14, &item);
            if (menu->sel == i) {
                selItem = item;
                Menu_DrawCursorRow(x + 0x1E, rowY, 0x10E, 0, 0xC0106060, 0xFF106060);
            }
            rowY += 0x18;
            i++;
        } while (i < menu->rowCount);
    }
    Menu_DrawDashLine(baseX + 0x14, baseY + 0x104, 0x12C, 0xFF000000);
    EquipShop_DrawLine(baseX + 0x10, baseY + 0x112, &selItem);
    Font_SetColor(0x80101010);
    Font_SetPos(baseX + 0x10E, baseY + 0x13E);
    Text_Printf(gGameMenuLabels[MENU_LABEL_DETAILS_GUIDE]);
}


/* The armour's version of Records_DrawWeaponList (the type's name from
 * the armour half of gEquipGroupTexts, its own empty line). */
void Records_DrawArmorList(RecordMenu *menu, s16 x, s16 y)
{
    s32 i;
    s32 centerX;
    s16 rowY = y;
    char *emptyText;
    EquipItem item;
    EquipItem selItem;
    s32 baseX;
    s32 baseY;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x78, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_ARMOR], 2, 1);
    if (menu->toggle != 0) {
        Records_DrawEquipDetail(menu);
        return;
    }
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    baseX = x;
    baseY = (s32)y;
    centerX = x;
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfColored(gRecordsFmtString, gEquipGroupTexts[menu->genre + RECORDS_GENRE_COUNT]);
    if (menu->rowCount == 0) {
        Font_SetSpacing(0x12, 0x12);
        emptyText = gRecordsMenuTexts[RECORDS_MENU_NO_ARMOR];
        Font_SetPos(Text_CenterX(emptyText, centerX + 0xAA, 0x12), y + 0x78);
        Font_SetColor(0x80301010);
        Text_PrintfColored(gRecordsFmtString, emptyText);
        return;
    }
    Menu_DrawPageNum(x + 0xFA, y + 0xE, menu->page, menu->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    rowY += 0x38;
    i = 0;
    if (i < menu->rowCount) {
        do {
            item = menu->items[i];
            Font_SetPos((s16)(x + 0x78) - 0x42, rowY);
            Text_PrintfColored(gRecordsFmtRowNumber, i + menu->page * 8 + 1);
            Font_SetPos(x + 0x78, rowY);
            Text_PrintfColored(gRecordsFmtEquipName, Equip_GetName(item.type, item.id));
            Font_SetPos(x, rowY);
            Lobby_DrawEquipIcon((s16)(x + 0x78) - 0x1C, rowY, 0x14, 0x14, &item);
            if (menu->sel == i) {
                selItem = item;
                Menu_DrawCursorRow(x + 0x1E, rowY, 0x10E, 0, 0xC0106060, 0xFF106060);
            }
            rowY += 0x18;
            i++;
        } while (i < menu->rowCount);
    }
    Menu_DrawDashLine(baseX + 0x14, baseY + 0x104, 0x12C, 0xFF000000);
    EquipShop_DrawLine(baseX + 0x10, baseY + 0x112, &selItem);
    Font_SetColor(0x80101010);
    Font_SetPos(baseX + 0x10E, baseY + 0x13E);
    Text_Printf(gGameMenuLabels[MENU_LABEL_DETAILS_GUIDE]);
}


/* Draws the house access page: the three settings (the current one,
 * houseAccess, in another colour), the password with a key icon when one is
 * set, and the done row; arrows on sel. */
void Records_DrawHouseAccess(RecordMenu *menu, s16 x, s16 y)
{
    s16 textX;
    s16 rowY = y;
    s16 i;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_HOUSE_ACCESS], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x16, 0x16);
    for (i = 0; i < HOUSE_ACCESS_ROW_DONE; i++) {
        if (menu->houseAccess == i) {
            Font_SetColor(0x8000052C);
        } else {
            Font_SetColor(0x80101010);
        }
        textX = Text_CenterX(gHouseAccessTexts[i], x + 0xAA, 0x16);
        Font_SetPos(textX, rowY += 0x28);
        Text_PrintfColored(gHouseAccessTexts[i]);
        if (menu->sel == i) {
            Menu_DrawArrowsRev(textX, rowY, 0x16, gHouseAccessTexts[i], 0xFF008080);
        }
    }
    Font_SetColor(0x80101010);
    rowY += 0x28;
    if (menu->password[0] != 0) {
        textX = Text_CenterX(menu->password, x + 0xAA, 0x16);
        Font_SetPos(textX, rowY);
        Text_PrintfColored(gRecordsFmtString, menu->password);
        Icon_Draw(textX - 0x1E, rowY - 2, 0x18, 0x18, -1, 0x59);
    }
    textX = Text_CenterX(gHouseAccessTexts[HOUSE_ACCESS_ROW_DONE], x + 0xAA, 0x16);
    Font_SetPos(textX, rowY += 0x28);
    Text_PrintfColored(gRecordsFmtString, gHouseAccessTexts[HOUSE_ACCESS_ROW_DONE]);
    if (menu->sel == HOUSE_ACCESS_ROW_DONE) {
        Menu_DrawArrowsRev(textX, rowY, 0x16, gHouseAccessTexts[i], 0xFF008080);
    }
}

/* Draws the guides' first page: the categories of gRecordGuideCats, arrows on
 * sel. */
void Records_DrawGuideCategories(RecordMenu *menu, s16 x, s16 y)
{
    s16 rowY = y;
    s16 i;
    RecordCat *cat;
    s32 textX;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_HUNTER_GUIDE], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x14, 0x14);
    Font_SetColor(0x80101010);
    rowY += 0x1E;
    textX = x + 0x32;
    for (i = 0, cat = gRecordGuideCats; i < 4; i++, cat++) {
        Font_SetPos(textX, rowY);
        Text_Printf(cat->name);
        if (menu->sel == i) {
            Menu_DrawArrowsRev(textX, rowY + 0xB, 0x16, "                      ", 0xFF106060);
        }
        rowY += 0x33;
    }
}

/* Draws a guide category's page: the page number and up to
 * RECORDS_GUIDE_ROWS entry names, arrows on cursor. */
void Records_DrawGuideEntries(RecordMenu *menu, s16 x, s16 y)
{
    char *entryName;
    s16 textX;
    s16 rowY = y;
    s16 i;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_HUNTER_GUIDE], 2, 1);
    Font_SetColor(0x80101010);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Menu_DrawPageNum(x + 0x104, rowY + 6, menu->page, menu->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    rowY += 0x1E;
    for (i = 0; i < menu->rowCount; i++) {
        entryName = ((RecordEntry *)menu->rows[i])->name;
        textX = Text_CenterX(entryName, x + 0xAA, 0x12);
        Font_SetPos(textX, rowY);
        Text_Printf(entryName);
        if (menu->cursor == i) {
            Menu_DrawArrowsRev(textX, rowY, 0x12, entryName, 0xFF106060);
        }
        rowY += 0x16;
    }
}

/* Draws page subCursor of the cursor entry's text, with its number out
 * of the entry's pageCount. */
void Records_DrawGuidePage(RecordMenu *menu, s16 x, s16 y)
{
    char *pageText;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_HUNTER_GUIDE], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Menu_DrawPageNum(x + 0x104, y + 6, menu->subCursor, ((RecordEntry *)menu->rows[menu->cursor])->pageCount, 1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80000000);
    pageText = ((RecordEntry *)menu->rows[menu->cursor])->pages[menu->subCursor];
    Font_SetPos(x + 0x1E, y + 0x1E);
    Text_PrintfStyled(2, pageText);
}

/* Draws page `page` of the score pages (RECORD_KIND_SCORES_A or _B) on a
 * dark panel, with the kind's title bar and the page number. */
void Records_DrawHornScores(RecordMenu *menu, s16 x, s16 y)
{
    char *pageText;

    Font_SetSpacing(0x16, 0x16);
    if (menu->kind[menu->menuSel] == RECORD_KIND_SCORES_A) {
        pageText = gHornScorePages[menu->page];
        Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_HORN_SCORES], 2, 1);
    } else {
        pageText = gOldHornScorePages[menu->page];
        Menu_DrawLabelBar(0x118, 0x12, 0xF0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_ANCIENT_SCORES], 2, 1);
    }
    Icon_Draw(x, y, 0x154, 0x114, 0xFF706858, 0);
    Menu_DrawPageNum(x + 0x104, y + 6, menu->page, menu->pages, 1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(x + 0x28, y + 0x1E);
    Text_PrintfStyled(2, pageText);
}

void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind);
void Records_DrawHunts(RecordMenu *menu, s16 x, s16 y);
void Records_DrawTitleList(RecordMenu *menu, s16 x, s16 y);
void Records_DrawItemList(RecordMenu *menu, s16 x, s16 y);
void Records_DrawWeaponList(RecordMenu *menu, s16 x, s16 y);
void Records_DrawArmorList(RecordMenu *menu, s16 x, s16 y);

/*
 * The records menu's draw callback (layer 0): by RecordMenu.mode, the
 * kinds menu and its prompt (opening and on the menu), the chosen kind's
 * first page while the window slides (to a page and back), or the page
 * the kind's screen is on, by its step.
 */
void Records_Draw(MenuTask *task)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    s16 x;

    Text_SetSize(task->textSize);
    switch (menu->mode) {
    case RECORDS_MODE_OPEN:
    case RECORDS_MODE_MENU:
        x = menu->win.pos;
        Menu_DrawDecoBox(x, 0x3A, 0x154, 0x114, 5);
        Records_DrawMenu(menu, x, 0x3A);
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecordsMenuTexts[RECORDS_MENU_MAIN_PROMPT]);
        break;
    case RECORDS_MODE_TO_PAGE:
    case RECORDS_MODE_BACK:
        switch (menu->kind[menu->menuSel]) {
        case RECORD_KIND_ITEMS:
            Records_DrawItemCategories(menu, 0x118, 0x3A);
            break;
        case RECORD_KIND_HOUSE_ACCESS:
            Records_DrawHouseAccess(menu, 0x118, 0x3A);
            break;
        case RECORD_KIND_TITLES:
            Records_DrawTitleTabs(menu, 0x118, 0x3A);
            break;
        case RECORD_KIND_GUIDES:
            Records_DrawGuideCategories(menu, 0x118, 0x3A);
            break;
        case RECORD_KIND_MONSTERS:
            Records_DrawHunts(menu, 0x118, 0x3A);
            break;
        case RECORD_KIND_SCORES_A:
        case RECORD_KIND_SCORES_B:
            Records_DrawHornScores(menu, 0x118, 0x3A);
            break;
        case RECORD_KIND_WEAPONS:
            Records_DrawWeaponGenres(menu, 0x118, 0x3A);
            break;
        case RECORD_KIND_ARMOR:
            Records_DrawArmorTypes(menu, 0x118, 0x3A);
            break;
        }
        break;
    case RECORDS_MODE_EQUIP:
        switch (menu->step) {
        default:
            switch (menu->kind[menu->menuSel]) {
            case RECORD_KIND_WEAPONS:
                Records_DrawWeaponGenres(menu, 0x118, 0x3A);
                break;
            case RECORD_KIND_ARMOR:
                Records_DrawArmorTypes(menu, 0x118, 0x3A);
                break;
            }
            break;
        case 2:
            switch (menu->kind[menu->menuSel]) {
            case RECORD_KIND_WEAPONS:
                Records_DrawWeaponList(menu, 0x118, 0x3A);
                break;
            case RECORD_KIND_ARMOR:
                Records_DrawArmorList(menu, 0x118, 0x3A);
                break;
            }
            break;
        }
        break;
    case RECORDS_MODE_SCORES:
        Records_DrawHornScores(menu, 0x118, 0x3A);
        break;
    case RECORDS_MODE_MONSTERS:
        Records_DrawHunts(menu, 0x118, 0x3A);
        break;
    case RECORDS_MODE_GUIDES:
        if (menu->step == 0) {
            Records_DrawGuideCategories(menu, 0x118, 0x3A);
        } else if (menu->step == 1) {
            Records_DrawGuideEntries(menu, 0x118, 0x3A);
        } else {
            Records_DrawGuidePage(menu, 0x118, 0x3A);
        }
        break;
    case RECORDS_MODE_ITEMS:
        if (menu->step == 0) {
            Records_DrawItemCategories(menu, 0x118, 0x3A);
        } else {
            Records_DrawItemList(menu, 0x118, 0x3A);
        }
        break;
    case RECORDS_MODE_TITLES:
        if (menu->step == 0) {
            Records_DrawTitleTabs(menu, 0x118, 0x3A);
        } else {
            Records_DrawTitleList(menu, 0x118, 0x3A);
        }
        break;
    case RECORDS_MODE_HOUSE_ACCESS:
        Records_DrawHouseAccess(menu, 0x118, 0x3A);
        break;
    }
}

/* The menu's second draw callback: the kinds menu at its window's x
 * while it slides (opening, to a page and back). */
void Records_DrawMenuLayer(MenuTask *task)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->mode) {
    case RECORDS_MODE_OPEN:
    case RECORDS_MODE_MENU:
    case RECORDS_MODE_TO_PAGE:
    case RECORDS_MODE_BACK:
        Records_DrawMenu(menu, menu->win.pos, 0x3A);
        break;
    }
}

extern char *gMonsterNames[];
u16 MonsterGroup_Max(u16 *list, u8 group, u8 *out);
u16 MonsterGroup_MinNonzero(u16 *list, u8 group, u8 *out);
f32 Monster_SizeFromPercent(u8 kind, u16 x);
f64 fptodp(f32 x);
/* unit's own declaration: as the caller uses it; with the definition's s32 (u16 note, s16 x, s16 y, s32 size), the calls in Records_DrawHunts compile differently */
s32 MonsterNote_DrawLargeCrown(u16 a, s16 x, s16 y, s16 n);
/* unit's own declaration: as the caller uses it; with the definition's s32 (u16 note, s16 x, s16 y, s32 size), the calls in Records_DrawHunts compile differently */
s32 MonsterNote_DrawSmallCrown(u16 a, s16 x, s16 y, s16 n);

/*
 * Draws the hunts' page `page`: six monster groups with hunts per page
 * (MonsterGroup_Sum of hunts), each with its name, its hunt count, its
 * largest size with its crown and its smallest with its crown. A line
 * says when nothing was hunted.
 */
void Records_DrawHunts(RecordMenu *menu, s16 x, s16 y)
{
    s32 i;
    s16 rowY = y;
    s16 page;
    s32 huntedKinds;
    s32 rowsDrawn;
    char buf[0x80];
    MonsterNoteDef *def;
    s32 colX;
    s32 count;
    s32 maxSize;
    s32 sizeY;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, gRecordsMenuTexts[RECORDS_MENU_HUNTS], 2, 1);
    Icon_Draw(x, y, 0x154, 0x114, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    huntedKinds = 0;
    def = gMonsterNoteDefs;
    for (i = 0; i < 0x34; i++, def++) {
        if (MonsterGroup_Sum(menu->hunts, def->emType, 0) > 0) {
            huntedKinds++;
        }
    }
    if (huntedKinds == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos((s16)(x + 0x14) + 0x32, (s16)(y + 0x16) + 0x64);
        Text_PrintfColored(gRecordsListTexts[RECORDS_LIST_NO_INFO]);
        return;
    }
    page = menu->page;
    i = 0;
    def = gMonsterNoteDefs;
    if (page > 0) {
        huntedKinds = 0;
        do {
            if (MonsterGroup_Sum(menu->hunts, def->emType, 0) > 0) {
                huntedKinds++;
                if (huntedKinds / 6 == page) {
                    i++;
                    def++;
                    break;
                }
            }
            i++;
            def++;
        } while (i < 0x34);
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x + 0x11E, y + 4);
    Menu_DrawPageNum(x + 0xF2, y + 2, page, menu->pages, 0);
    rowY += 0x16;
    rowsDrawn = 0;
    if (i < 0x34) {
        colX = x + 0x14;
        do {
            count = MonsterGroup_Sum(menu->hunts, def->emType, 0);
            if (count > 0) {
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(colX, rowY);
                Text_PrintfColored(gRecordsFmtString, gMonsterNames[def->emType]);
                Font_SetPos((s16)colX + 0xB4, rowY);
                sprintf(buf, gRecordsListTexts[RECORDS_LIST_HUNT_COUNT_FMT], count);
                Text_PrintfColored(buf);
                Font_SetSpacing(0x10, 0x10);
                maxSize = MonsterGroup_Max(menu->sizeMax, def->emType, 0);
                if (maxSize > 0) {
                    f32 size = Monster_SizeFromPercent(def->emType, maxSize);
                    sizeY = rowY + 0x14;
                    Font_SetPos((s16)colX - 2, sizeY);
                    sprintf(buf, gRecordsListTexts[RECORDS_LIST_MAX_SIZE_FMT], fptodp(size));
                    Text_PrintfColored(buf);
                    MonsterNote_DrawLargeCrown(def->note, (s16)colX - 1, (s16)sizeY - 2, maxSize);
                }
                count = MonsterGroup_MinNonzero(menu->sizeMin, def->emType, 0);
                if (count > 0) {
                    f32 size = Monster_SizeFromPercent(def->emType, count);
                    sizeY = rowY + 0x14;
                    Font_SetPos((s16)colX + 0x9A, rowY + 0x14);
                    sprintf(buf, gRecordsListTexts[RECORDS_LIST_MIN_SIZE_FMT], fptodp(size));
                    Text_PrintfColored(buf);
                    MonsterNote_DrawSmallCrown(def->note, (s16)colX + 0x9C, (s16)sizeY - 2, count);
                }
                rowY += 0x28;
                rowsDrawn++;
            }
            if (rowsDrawn >= 6) {
                break;
            }
            i++;
            def++;
        } while (i < 0x34);
    }
}

/* The hunts' page count: the monster groups with hunts, six per page, at
 * least 1. save is the RecordMenu, whose hunts it reads. */
s32 Records_CountHuntPages(u8 *save)
{
    s32 i;
    MonsterNoteDef *def;
    s32 huntedKinds;
    s32 pages;

    huntedKinds = 0;
    def = gMonsterNoteDefs;
    for (i = 0; i < 0x34; i++, def++) {
        if (MonsterGroup_Sum((u16 *) (save + 0x2B0), def->emType, 0) > 0) {
            huntedKinds++;
        }
    }
    if (huntedKinds == 0) {
        return 1;
    }
    pages = huntedKinds / 6;
    huntedKinds -= pages * 6;
    if (huntedKinds != 0) {
        pages++;
    }
    return pages;
}

/* The file's data: the horn score pages and the guides (gRecordGuideCats, their
 * entries and the entries' text pages). */
extern u32 gHornScorePage0[];
extern u32 gHornScorePage1[];
extern u32 gHornScorePage2[];
extern u32 gHornScorePage3[];
extern u32 gHornScorePage4[];
extern u32 gOldHornScorePage0[];
extern u32 gOldHornScorePage1[];
extern u32 gOldHornScorePage2[];
extern u32 gOldHornScorePage3[];
extern char gRecordGuidePage0_0_0[];
extern char gRecordGuidePage0_0_1[];
extern char gRecordGuidePage0_0_2[];
extern char gRecordGuidePage0_1_0[];
extern char gRecordGuidePage0_1_1[];
extern char gRecordGuidePage0_2_0[];
extern char gRecordGuidePage0_2_1[];
extern char gRecordGuidePage0_3_0[];
extern char gRecordGuidePage0_3_1[];
extern char gRecordGuidePage0_4_0[];
extern char gRecordGuidePage0_4_1[];
extern char gRecordGuidePage0_4_2[];
extern char gRecordGuidePage0_5_0[];
extern char gRecordGuidePage0_5_1[];
extern char gRecordGuidePage0_6_0[];
extern char gRecordGuidePage0_6_1[];
extern char gRecordGuidePage0_7_0[];
extern char gRecordGuidePage0_7_1[];
extern char gRecordGuidePage0_7_2[];
extern char gRecordGuidePage0_7_3[];
extern char gRecordGuidePage0_9_0[];
extern char gRecordGuidePage0_9_1[];
extern char gRecordGuidePage0_9_2[];
extern char gRecordGuidePage0_9_3[];
extern char gRecordGuidePage0_10_1[];
extern char gRecordGuidePage0_12_0[];
extern char gRecordGuidePage0_12_1[];
extern char gRecordGuidePage0_12_2[];
extern char gRecordGuidePage0_13_0[];
extern char gRecordGuidePage0_13_1[];
extern char gRecordGuidePage0_14_0[];
extern char gRecordGuidePage0_14_1[];
extern char gRecordGuidePage1_0_0[];
extern char gRecordGuidePage1_0_1[];
extern char gRecordGuidePage1_1_0[];
extern char gRecordGuidePage1_1_1[];
extern char gRecordGuidePage1_1_2[];
extern char gRecordGuidePage1_1_3[];
extern char gRecordGuidePage1_2_0[];
extern char gRecordGuidePage1_2_1[];
extern char gRecordGuidePage1_3_0[];
extern char gRecordGuidePage1_3_1[];
extern char gRecordGuidePage1_4_0[];
extern char gRecordGuidePage1_4_1[];
extern char gRecordGuidePage1_4_2[];
extern char gRecordGuidePage1_5_0[];
extern char gRecordGuidePage1_5_1[];
extern char gRecordGuidePage1_6_0[];
extern char gRecordGuidePage1_6_1[];
extern char gRecordGuidePage1_7_0[];
extern char gRecordGuidePage1_7_1[];
extern char gRecordGuidePage1_8_0[];
extern char gRecordGuidePage1_9_0[];
extern char gRecordGuidePage1_9_1[];
extern char gRecordGuidePage1_9_2[];
extern char gRecordGuidePage1_9_3[];
extern char gRecordGuidePage2_0_0[];
extern char gRecordGuidePage2_1_0[];
extern char gRecordGuidePage2_1_1[];
extern char gRecordGuidePage2_2_0[];
extern char gRecordGuidePage2_2_1[];
extern char gRecordGuidePage2_2_2[];
extern char gRecordGuidePage2_3_0[];
extern char gRecordGuidePage2_3_1[];
extern char gRecordGuidePage2_3_2[];
extern char gRecordGuidePage2_3_3[];
extern char gRecordGuidePage2_3_4[];
extern char gRecordGuidePage2_4_0[];
extern char gRecordGuidePage2_4_1[];
extern char gRecordGuidePage2_4_2[];
extern char gRecordGuidePage2_5_0[];
extern char gRecordGuidePage2_5_1[];
extern char gRecordGuidePage2_6_0[];
extern char gRecordGuidePage2_6_1[];
extern char gRecordGuidePage2_6_2[];
extern char gRecordGuidePage2_7_0[];
extern char gRecordGuidePage2_7_1[];
extern char gRecordGuidePage2_8_0[];
extern char gRecordGuidePage2_8_1[];
extern char gRecordGuidePage2_9_0[];
extern char gRecordGuidePage2_9_1[];
extern char gRecordGuidePage3_0_0[];
extern char gRecordGuidePage3_0_1[];
extern char gRecordGuidePage3_0_2[];
extern char gRecordGuidePage3_0_3[];
extern char gRecordGuidePage3_1_0[];
extern char gRecordGuidePage3_1_1[];
extern char gRecordGuidePage3_1_2[];
extern char gRecordGuidePage3_2_0[];
extern char gRecordGuidePage3_2_1[];
extern char gRecordGuidePage3_2_2[];
extern char gRecordGuidePage3_2_3[];
extern char gRecordGuidePage3_3_0[];
extern char gRecordGuidePage3_3_1[];
extern char gRecordGuidePage3_3_2[];
extern char gRecordGuidePage3_4_0[];
extern char gRecordGuidePage3_4_1[];
extern char gRecordGuidePage3_4_2[];
extern char gRecordGuidePage3_5_0[];
extern char gRecordGuidePage3_5_1[];
extern char gRecordGuidePage3_5_2[];
extern char gRecordGuidePage3_5_3[];
extern char gRecordGuidePage3_5_4[];
extern char gRecordGuidePage3_6_0[];
extern char gRecordGuidePage3_6_1[];
extern char gRecordGuidePage3_7_0[];
extern char gRecordGuidePage3_7_1[];
extern char gRecordGuidePage3_7_2[];
extern char gRecordGuidePage3_7_3[];
extern char gRecordGuidePage3_8_0[];
extern char gRecordGuidePage3_8_1[];
extern char gRecordGuidePage3_8_2[];
extern char gRecordGuidePage3_8_3[];
extern char gRecordGuidePage3_9_0[];
extern char gRecordGuidePage3_9_1[];
extern char gRecordGuidePage3_9_2[];
extern char gRecordGuidePage3_10_0[];
extern char gRecordGuidePage3_10_1[];
extern char gRecordGuidePage3_10_2[];
extern char gRecordGuidePage3_11_0[];
extern char gRecordGuidePage3_11_1[];
extern char gRecordGuidePage3_11_2[];
extern char gRecordGuidePage3_12_0[];
extern char gRecordGuidePage3_12_1[];
extern char gRecordGuidePage3_12_2[];
extern char gRecordGuidePage3_13_0[];
extern char gRecordGuidePage3_13_1[];
extern char gRecordGuidePage3_13_2[];
extern char gRecordGuidePage3_13_3[];
extern char gRecordGuidePage3_14_0[];
extern char gRecordGuidePage3_14_1[];
extern char gRecordGuidePage3_14_2[];
extern char gRecordGuidePage3_14_3[];
extern char gRecordGuidePage3_15_0[];
extern char gRecordGuidePage3_15_1[];
extern char gRecordGuidePage3_15_2[];
extern char gRecordGuidePage3_15_3[];
extern char gRecordGuidePage3_15_4[];
extern char gRecordGuidePage3_16_0[];
extern char gRecordGuidePage3_16_1[];
extern char gRecordGuidePage3_16_2[];
extern char gRecordGuidePage3_16_3[];
extern char gRecordGuidePage3_17_0[];
extern char gRecordGuidePage3_17_1[];
extern char gRecordGuidePage3_17_2[];
extern char gRecordGuidePage3_17_3[];
extern char gRecordGuidePage3_18_0[];
extern char gRecordGuidePage3_18_1[];
extern char gRecordGuidePage3_18_2[];
extern char gRecordGuidePage3_18_3[];
extern char gRecordGuidePage3_19_0[];
extern char gRecordGuidePage3_19_1[];
extern char gRecordGuidePage3_19_2[];
extern char gRecordGuidePage3_19_3[];
extern char gRecordGuidePage3_20_0[];
extern char gRecordGuidePage3_20_1[];
extern char gRecordGuidePage3_20_2[];
extern char gRecordGuidePage3_21_0[];
extern char gRecordGuidePage3_21_1[];
extern char gRecordGuidePage3_22_0[];
extern char gRecordGuidePage3_22_1[];
extern u32 gRecordGuideEntryName0_0[];
extern u32 gRecordGuideEntryName0_1[];
extern u32 gRecordGuideEntryName0_2[];
extern u32 gRecordGuideEntryName0_3[];
extern u32 gRecordGuideEntryName0_4[];
extern u32 gRecordGuideEntryName0_5[];
extern u32 gRecordGuideEntryName0_6[];
extern u32 gRecordGuideEntryName0_7[];
extern u32 gRecordGuideEntryName0_8[];
extern u32 gRecordGuideEntryName0_9[];
extern u32 gRecordGuideEntryName0_10[];
extern u32 gRecordGuideEntryName0_11[];
extern u32 gRecordGuideEntryName0_12[];
extern u32 gRecordGuideEntryName0_13[];
extern u32 gRecordGuideEntryName0_14[];
extern u32 gRecordGuideEntryName1_0[];
extern u32 gRecordGuideEntryName1_1[];
extern u32 gRecordGuideEntryName1_2[];
extern u32 gRecordGuideEntryName1_3[];
extern u32 gRecordGuideEntryName1_4[];
extern u32 gRecordGuideEntryName1_5[];
extern u32 gRecordGuideEntryName1_6[];
extern u32 gRecordGuideEntryName1_7[];
extern u32 gRecordGuideEntryName1_8[];
extern u32 gRecordGuideEntryName1_9[];
extern u32 gRecordGuideEntryName2_0[];
extern u32 gRecordGuideEntryName2_1[];
extern u32 gRecordGuideEntryName2_2[];
extern u32 gRecordGuideEntryName2_3[];
extern u32 gRecordGuideEntryName2_4[];
extern u32 gRecordGuideEntryName2_5[];
extern u32 gRecordGuideEntryName2_6[];
extern u32 gRecordGuideEntryName2_7[];
extern u32 gRecordGuideEntryName2_8[];
extern u32 gRecordGuideEntryName2_9[];
extern u32 gRecordGuideEntryName3_0[];
extern u32 gRecordGuideEntryName3_1[];
extern u32 gRecordGuideEntryName3_2[];
extern u32 gRecordGuideEntryName3_4[];
extern u32 gRecordGuideEntryName3_6[];
extern u32 gRecordGuideEntryName3_8[];
extern u32 gRecordGuideEntryName3_10[];
extern u32 gRecordGuideEntryName3_12[];
extern u32 gRecordGuideEntryName3_14[];
extern u32 gRecordGuideEntryName3_16[];
extern u32 gRecordGuideEntryName3_18[];
extern u32 gRecordGuideEntryName3_20[];
extern u32 gRecordGuideEntryName3_22[];
extern u32 gRecordGuideCatName0[];
extern u32 gRecordGuideCatName1[];
extern u32 gRecordGuideCatName2[];
extern u32 gRecordGuideCatName3[];
char *gHornScorePages[8] = {
    (char *)gHornScorePage0, (char *)gHornScorePage1, (char *)gHornScorePage2, (char *)gHornScorePage3,
    (char *)gHornScorePage4, NULL, NULL, NULL,
};
char *gOldHornScorePages[4] = {
    (char *)gOldHornScorePage0, (char *)gOldHornScorePage1, (char *)gOldHornScorePage2, (char *)gOldHornScorePage3,
};
char *gRecordGuidePages0_0[4] = {
    gRecordGuidePage0_0_0, gRecordGuidePage0_0_1, gRecordGuidePage0_0_2, NULL,
};
char *gRecordGuidePages0_1[2] = {
    gRecordGuidePage0_1_0, gRecordGuidePage0_1_1,
};
char *gRecordGuidePages0_2[2] = {
    gRecordGuidePage0_2_0, gRecordGuidePage0_2_1,
};
char *gRecordGuidePages0_3[2] = {
    gRecordGuidePage0_3_0, gRecordGuidePage0_3_1,
};
char *gRecordGuidePages0_4[3] = {
    gRecordGuidePage0_4_0, gRecordGuidePage0_4_1, gRecordGuidePage0_4_2,
};
char *gRecordGuidePages0_5[2] = {
    gRecordGuidePage0_5_0, gRecordGuidePage0_5_1,
};
char *gRecordGuidePages0_6[4] = {
    gRecordGuidePage0_6_0, gRecordGuidePage0_6_1, NULL, NULL,
};
char *gRecordGuidePages0_7[4] = {
    gRecordGuidePage0_7_0, gRecordGuidePage0_7_1, gRecordGuidePage0_7_2, gRecordGuidePage0_7_3,
};
extern char gRecordGuidePage0_8_0[];
extern char gRecordGuidePage0_8_1[];
extern char gRecordGuidePage0_10_0[];
extern char gRecordGuidePage0_11_0[];
extern char gRecordGuidePage0_11_1[];
extern char gRecordGuidePage0_11_2[];
char *gRecordGuidePages0_8[4] = {
    gRecordGuidePage0_8_0, gRecordGuidePage0_8_1, NULL, NULL,
};
char *gRecordGuidePages0_9[4] = {
    gRecordGuidePage0_9_0, gRecordGuidePage0_9_1, gRecordGuidePage0_9_2, gRecordGuidePage0_9_3,
};
char *gRecordGuidePages0_10[2] = {
    gRecordGuidePage0_10_0, gRecordGuidePage0_10_1,
};
char *gRecordGuidePages0_11[3] = {
    gRecordGuidePage0_11_0, gRecordGuidePage0_11_1, gRecordGuidePage0_11_2,
};
char *gRecordGuidePages0_12[3] = {
    gRecordGuidePage0_12_0, gRecordGuidePage0_12_1, gRecordGuidePage0_12_2,
};
char *gRecordGuidePages0_13[2] = {
    gRecordGuidePage0_13_0, gRecordGuidePage0_13_1,
};
char *gRecordGuidePages0_14[2] = {
    gRecordGuidePage0_14_0, gRecordGuidePage0_14_1,
};
char *gRecordGuidePages1_0[2] = {
    gRecordGuidePage1_0_0, gRecordGuidePage1_0_1,
};
char *gRecordGuidePages1_1[4] = {
    gRecordGuidePage1_1_0, gRecordGuidePage1_1_1, gRecordGuidePage1_1_2, gRecordGuidePage1_1_3,
};
char *gRecordGuidePages1_2[2] = {
    gRecordGuidePage1_2_0, gRecordGuidePage1_2_1,
};
char *gRecordGuidePages1_3[2] = {
    gRecordGuidePage1_3_0, gRecordGuidePage1_3_1,
};
char *gRecordGuidePages1_4[4] = {
    gRecordGuidePage1_4_0, gRecordGuidePage1_4_1, gRecordGuidePage1_4_2, NULL,
};
char *gRecordGuidePages1_5[2] = {
    gRecordGuidePage1_5_0, gRecordGuidePage1_5_1,
};
char *gRecordGuidePages1_6[2] = {
    gRecordGuidePage1_6_0, gRecordGuidePage1_6_1,
};
char *gRecordGuidePages1_7[2] = {
    gRecordGuidePage1_7_0, gRecordGuidePage1_7_1,
};
char *gRecordGuidePages1_8[2] = {
    gRecordGuidePage1_8_0, NULL,
};
char *gRecordGuidePages1_9[4] = {
    gRecordGuidePage1_9_0, gRecordGuidePage1_9_1, gRecordGuidePage1_9_2, gRecordGuidePage1_9_3,
};
char *gRecordGuidePages2_0[2] = {
    gRecordGuidePage2_0_0, NULL,
};
char *gRecordGuidePages2_1[2] = {
    gRecordGuidePage2_1_0, gRecordGuidePage2_1_1,
};
char *gRecordGuidePages2_2[4] = {
    gRecordGuidePage2_2_0, gRecordGuidePage2_2_1, gRecordGuidePage2_2_2, NULL,
};
char *gRecordGuidePages2_3[6] = {
    gRecordGuidePage2_3_0, gRecordGuidePage2_3_1, gRecordGuidePage2_3_2, gRecordGuidePage2_3_3,
    gRecordGuidePage2_3_4, NULL,
};
char *gRecordGuidePages2_4[3] = {
    gRecordGuidePage2_4_0, gRecordGuidePage2_4_1, gRecordGuidePage2_4_2,
};
char *gRecordGuidePages2_5[2] = {
    gRecordGuidePage2_5_0, gRecordGuidePage2_5_1,
};
char *gRecordGuidePages2_6[4] = {
    gRecordGuidePage2_6_0, gRecordGuidePage2_6_1, gRecordGuidePage2_6_2, NULL,
};
char *gRecordGuidePages2_7[2] = {
    gRecordGuidePage2_7_0, gRecordGuidePage2_7_1,
};
char *gRecordGuidePages2_8[2] = {
    gRecordGuidePage2_8_0, gRecordGuidePage2_8_1,
};
char *gRecordGuidePages2_9[4] = {
    gRecordGuidePage2_9_0, gRecordGuidePage2_9_1, NULL, NULL,
};
char *gRecordGuidePages3_0[4] = {
    gRecordGuidePage3_0_0, gRecordGuidePage3_0_1, gRecordGuidePage3_0_2, gRecordGuidePage3_0_3,
};
char *gRecordGuidePages3_1[4] = {
    gRecordGuidePage3_1_0, gRecordGuidePage3_1_1, gRecordGuidePage3_1_2, NULL,
};
char *gRecordGuidePages3_2[4] = {
    gRecordGuidePage3_2_0, gRecordGuidePage3_2_1, gRecordGuidePage3_2_2, gRecordGuidePage3_2_3,
};
char *gRecordGuidePages3_3[4] = {
    gRecordGuidePage3_3_0, gRecordGuidePage3_3_1, gRecordGuidePage3_3_2, NULL,
};
char *gRecordGuidePages3_4[4] = {
    gRecordGuidePage3_4_0, gRecordGuidePage3_4_1, gRecordGuidePage3_4_2, NULL,
};
char *gRecordGuidePages3_5[6] = {
    gRecordGuidePage3_5_0, gRecordGuidePage3_5_1, gRecordGuidePage3_5_2, gRecordGuidePage3_5_3,
    gRecordGuidePage3_5_4, NULL,
};
char *gRecordGuidePages3_6[2] = {
    gRecordGuidePage3_6_0, gRecordGuidePage3_6_1,
};
char *gRecordGuidePages3_7[4] = {
    gRecordGuidePage3_7_0, gRecordGuidePage3_7_1, gRecordGuidePage3_7_2, gRecordGuidePage3_7_3,
};
char *gRecordGuidePages3_8[4] = {
    gRecordGuidePage3_8_0, gRecordGuidePage3_8_1, gRecordGuidePage3_8_2, gRecordGuidePage3_8_3,
};
char *gRecordGuidePages3_9[4] = {
    gRecordGuidePage3_9_0, gRecordGuidePage3_9_1, gRecordGuidePage3_9_2, NULL,
};
char *gRecordGuidePages3_10[4] = {
    gRecordGuidePage3_10_0, gRecordGuidePage3_10_1, gRecordGuidePage3_10_2, NULL,
};
char *gRecordGuidePages3_11[4] = {
    gRecordGuidePage3_11_0, gRecordGuidePage3_11_1, gRecordGuidePage3_11_2, NULL,
};
char *gRecordGuidePages3_12[4] = {
    gRecordGuidePage3_12_0, gRecordGuidePage3_12_1, gRecordGuidePage3_12_2, NULL,
};
char *gRecordGuidePages3_13[4] = {
    gRecordGuidePage3_13_0, gRecordGuidePage3_13_1, gRecordGuidePage3_13_2, gRecordGuidePage3_13_3,
};
char *gRecordGuidePages3_14[4] = {
    gRecordGuidePage3_14_0, gRecordGuidePage3_14_1, gRecordGuidePage3_14_2, gRecordGuidePage3_14_3,
};
char *gRecordGuidePages3_15[8] = {
    gRecordGuidePage3_15_0, gRecordGuidePage3_15_1, gRecordGuidePage3_15_2, gRecordGuidePage3_15_3,
    gRecordGuidePage3_15_4, NULL, NULL, NULL,
};
char *gRecordGuidePages3_16[4] = {
    gRecordGuidePage3_16_0, gRecordGuidePage3_16_1, gRecordGuidePage3_16_2, gRecordGuidePage3_16_3,
};
char *gRecordGuidePages3_17[4] = {
    gRecordGuidePage3_17_0, gRecordGuidePage3_17_1, gRecordGuidePage3_17_2, gRecordGuidePage3_17_3,
};
char *gRecordGuidePages3_18[4] = {
    gRecordGuidePage3_18_0, gRecordGuidePage3_18_1, gRecordGuidePage3_18_2, gRecordGuidePage3_18_3,
};
char *gRecordGuidePages3_19[4] = {
    gRecordGuidePage3_19_0, gRecordGuidePage3_19_1, gRecordGuidePage3_19_2, gRecordGuidePage3_19_3,
};
char *gRecordGuidePages3_20[4] = {
    gRecordGuidePage3_20_0, gRecordGuidePage3_20_1, gRecordGuidePage3_20_2, NULL,
};
char *gRecordGuidePages3_21[2] = {
    gRecordGuidePage3_21_0, gRecordGuidePage3_21_1,
};
char *gRecordGuidePages3_22[2] = {
    gRecordGuidePage3_22_0, gRecordGuidePage3_22_1,
};
u32 gRecordGuideEntries0[48] = {
#include "lobby/007DBBC0.inc"
};
u32 gRecordGuideEntries1[32] = {
#include "lobby/007DBC80.inc"
};
u32 gRecordGuideEntries2[32] = {
#include "lobby/007DBD00.inc"
};
u32 gRecordGuideEntries3[72] = {
#include "lobby/007DBD80.inc"
};
RecordCat gRecordGuideCats[4] = {
#include "lobby/007DBEA0.inc"
};
