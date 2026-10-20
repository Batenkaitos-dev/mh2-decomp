/*
 * The game menu's mail page (online): its list of four entries (the mail
 * box, a new short mail, the short mail from the last one kept, the mail
 * block list; MAIL_ENTRY_*). GameMenu_OpenMail opens it, GameMenu_UpdateMail
 * runs it and GameMenu_DrawMail draws it; each entry runs its own menu
 * (MailMenu_*, ShortMail_*, BlockList_*), which draws itself. The game
 * menu's tables pair the three handlers.
 * The .data holds the page's list window.
 */
#include "common.h"
#include "game_menu.h"
#include "hud.h"
#include "lobby_menu.h"
#include "pad.h"
#include "sound.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "window.h"

/* prototypes */
void GameMenu_DrawList(s8 dim);
struct ListPanel;
/* unit's own declaration: the cursor is declared s32 (the definition takes an s16): declared s16, GameMenu_DrawMail's call builds differently from the game's code */
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s32 sel);
extern ListPanel gMailPanel;
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void LobbySe_Play(s32);
extern LobbyMenu *gLobbyMenu;
/* end of prototypes */

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void GameMenu_Back(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
void MailMenu_Open(void);
void ShortMail_Open(void *id, void *name, u32 flags);
void BlockList_Open(void);
s32 MailMenu_Update(u16 pad);
s32 ShortMail_Step(u16 pad);
s32 BlockList_Update(u16 pad);

/* The game menu's open handler for the mail page: its own list at its first
 * entry, with its help line. Returns 0. */
s32 GameMenu_OpenMail(void *unused)
{
    gSubMenuWork->subStep = 0;
    gSubMenuWork->subEntry = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_LOBBY;
    gHelpLine = MAIL_HELP_LINE;
    return 0;
}

/* The mail page's input, by GameMenu.subStep:
 * - 0, its list: PAD_START closes the game menu; up and down move over the
 *   four entries, each with its help line; PAD_CIRCLE opens the entry's
 *   menu: the mail box (MailMenu_Open), a new short mail with the addressee's
 *   id to type (ShortMail_Open's flag 1), the saved short mail (flags 1 and
 *   8: the form starts from the mail kept last), or the mail block list.
 * - 1, the entry's menu runs: cancelled (MENU_CANCEL) it goes back to the
 *   list; ended or closed (MENU_UNDONE, MENU_CLOSE) the game menu closes,
 *   the mail box first giving the hunter back the field control.
 * The help window shows only on the list. Returns the buttons left for the
 * game menu: 0 once the page used them, else pad (PAD_CROSS on the list goes
 * back to the game menu's list). */
u16 GameMenu_UpdateMail(u16 pad)
{
    u8 *cur;
    GameMenu *menu;

    switch (gSubMenuWork->subStep) {
    case 0:
        if (pad & PAD_START) {
            GameMenu_Back();
            return 0;
        }
        Menu_CursorUpDown(&gSubMenuWork->subEntry, pad, MAIL_ENTRIES);
        menu = gSubMenuWork;
        gHelpLine = menu->subEntry + MAIL_HELP_LINE;
        cur = &menu->subEntry;
        if (pad & PAD_CIRCLE) {
            switch (*cur) {
            case MAIL_ENTRY_BOX:
                menu->subStep++;
                MailMenu_Open();
                break;
            case MAIL_ENTRY_NEW:
                menu->subStep++;
                ShortMail_Open(NULL, NULL, SHORT_MAIL_TYPE_ID);
                break;
            case MAIL_ENTRY_SAVED:
                menu->subStep++;
                ShortMail_Open(NULL, NULL, SHORT_MAIL_TYPE_ID | SHORT_MAIL_SAVED);
                break;
            case MAIL_ENTRY_BLOCK_LIST:
                menu->subStep++;
                BlockList_Open();
                break;
            }
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        }
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case MAIL_ENTRY_BOX:
            switch (MailMenu_Update(pad)) {
            case MENU_CANCEL:
                gSubMenuWork->subStep--;
                return 0;
            case MENU_UNDONE:
            case MENU_CLOSE:
                gFieldControl = 1;
                pad = PAD_START;
                break;
            case MENU_RUNNING:
                return 0;
            }
            break;
        case MAIL_ENTRY_NEW:
        case MAIL_ENTRY_SAVED:
            switch (ShortMail_Step(pad)) {
            case MENU_CANCEL:
                gSubMenuWork->subStep--;
                return 0;
            case MENU_UNDONE:
            case MENU_CLOSE:
                pad = PAD_START;
                break;
            case MENU_RUNNING:
                return 0;
            }
            break;
        case MAIL_ENTRY_BLOCK_LIST:
            switch (BlockList_Update(pad)) {
            case MENU_CANCEL:
                gSubMenuWork->subStep--;
                return 0;
            case MENU_UNDONE:
            case MENU_CLOSE:
                pad = PAD_START;
                break;
            case MENU_RUNNING:
                return 0;
            }
            break;
        }
        if (pad & PAD_START) {
            GameMenu_Back();
            pad = 0;
        }
        break;
    }
    if (gSubMenuWork->subStep == 0) {
        gHelpShow = 1;
    } else {
        gHelpShow = 0;
    }
    return pad;
}

/* Draws the mail page: on its list, the game menu's list dimmed and the
 * page's list window with its cursor; an open entry's menu draws itself. */
void GameMenu_DrawMail(void)
{
    u8 step;

    step = gSubMenuWork->subStep;
    switch (step) {
    case 0:
        GameMenu_DrawList(1);
        ListPanel_Draw(&gMailPanel, (s16 *)0, (char *)0, (s32) gSubMenuWork->subEntry);
        
    case 1: /* an entry's menu is open: it draws itself */
        break;
    }
}

/* The mail page's list window. */
extern char *gMailboxMenuTexts[];
ListPanel gMailPanel = {
#include "lobby/007F2070.inc"
};
