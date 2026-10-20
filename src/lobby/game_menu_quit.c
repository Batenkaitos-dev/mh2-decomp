/*
 * The game menu's quit and pause entries (its offline list).
 * - The quit: two ways (QUIT_WAY_*: the next boot goes straight to gm_sub's
 *   task once, or at every boot), a yes/no (GameMenu_YesNoInput), the
 *   questions on saving first with the save menu (TownNetMenu_Update),
 *   then the quit itself (GameMenu_Quit: the boot mode, TOWN_MODE_QUIT and
 *   the fade). GameMenu_OpenQuit, GameMenu_UpdateQuit and GameMenu_DrawQuit
 *   are its entry's handlers; GameMenu.page holds its QUIT_PAGE_*.
 * - The pause: GameMenu_OpenPause holds the day clock and pauses the sound
 *   streams, GameMenu_UpdatePause lets them go; the pause screen is drawn
 *   elsewhere (gPauseMenuReq), so GameMenu_DrawPause is empty.
 * NetUser_Get, a member's record in gNetUsers, sits between the two draws
 * in the game's code.
 * The .data holds the quit's list window and its question's frame.
 */
#include "common.h"
#include "game_menu.h"
#include "hud.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "lobby_state.h"
#include "pad.h"
#include "scene.h"
#include "sound.h"
#include "task.h"
#include "window.h"

void Lobby_SetBootMode(s32);
s32 Lobby_CheckDay(s32);
s32 Lobby_GetDayState(void);
void FadeA_Start(s32);
void Chat_ShowWindows(void);
extern s32 gTownMode;
void Stream_Pause(s32, s32);
s32 Game_IsOnline(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
extern NetUser gNetUsers[];
s32 Card_IsSystemSaved(void);
void GameMenu_DrawList(s8 dim);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
struct Frame;
void WindowFrame_Draw(struct Frame *f, s16 *off);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
extern ListPanel gQuitPanel;
extern Frame gQuitFrame;
extern char *gYesNoTexts[];
extern char *gQuitSaveTexts[];
extern char *gQuitConfirmTexts[];
extern char *gQuitMenuTexts[];
extern char *gQuitMenuDimTexts[];

void Text_Printf(const char *, ...);
void Text_PrintfColored(const char *, ...);

/* The game menu's open handler for the quit entry: returns 1 (refused)
 * while the menu is busy (GameMenu.busy); else shows the entry's help,
 * puts the cursor on its first line and returns 0. */
s32 GameMenu_OpenQuit(void *unused0)
{
    GameMenu *menu = gSubMenuWork;

    if (menu->busy != 0) {
        return 1;
    }
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_LOBBY;
    gHelpLine = 0x24; /* the first way's line; 0x25 the second's */
    menu->subEntry = QUIT_WAY_ONCE;
    return 0;
}

/* Quits the game from the town: ends the day clock's check if one runs,
 * sets where the next boot goes (QUIT_WAY_ONCE: straight to gm_sub's task
 * once; QUIT_WAY_EVERY_BOOT: at every boot), puts the town in its quit
 * mode (TOWN_MODE_QUIT: the music fades out, then the town ends), starts
 * the fade to black, and gives the hunter the pad and the chat windows
 * back. */
void GameMenu_Quit(void)
{
    if (Lobby_GetDayState() != 0) {
        Lobby_CheckDay(0);
    }
    Lobby_SetBootMode((gSubMenuWork->subEntry == QUIT_WAY_ONCE) ? BOOT_MODE_GMSUB : BOOT_MODE_GMSUB_KEEP);
    gTownMode = TOWN_MODE_QUIT;
    FadeA_Start(FADE_OUT_BLACK);
    gFieldControl = 1;
    Chat_ShowWindows();
}

void GameMenu_Back(void);
void Chat_HideWindows(void);
void Chat_SetWindowMode(s8 a, s8 b);
s32 GameMenu_YesNoInput(u16 *pad);
void TownNetMenu_Begin(void);
s32 TownNetMenu_Update(s32);
u8 YesNo_Toggle(u16 buttons, u8 on, u16 mask);

/* The quit entry's input. subStep 0, the list of the two ways to quit
 * (subEntry, QUIT_WAY_*; the second needs the system data saved,
 * Card_IsSystemSaved): PAD_CIRCLE asks to confirm, PAD_CROSS returns to the
 * game menu's list. subStep 1, by page (GameMenu.page, QUIT_PAGE_*;
 * GameMenu.yesNo 0 yes, 1 no):
 * - CONFIRM (GameMenu_YesNoInput): no goes back to the list; yes goes to
 *   ASK_SAVE (the first way) or straight to the save menu, SAVE_DIRECT;
 * - ASK_SAVE and ASK_AGAIN, two questions on saving first: yes on either
 *   opens the save menu (SAVE); no on ASK_SAVE goes to ASK_AGAIN, no on
 *   ASK_AGAIN quits without saving; PAD_CROSS steps back a page;
 * - SAVE and SAVE_DIRECT, the save menu (TownNetMenu_Update): once saved,
 *   the quit (GameMenu_Quit) and the page after; left without saving,
 *   ASK_AGAIN or CONFIRM;
 * - from QUITTING on the quit runs: the hunter keeps the pad and the chat
 *   windows.
 * Returns PAD_CROSS to go back to the game menu's list, else 0. */
u16 GameMenu_UpdateQuit(u16 pad)
{
    u8 *cur;
    GameMenu *menu;
    s32 result;
    s32 buttons;

    switch (gSubMenuWork->subStep) {
    case 0:
        if (pad & PAD_START) {
            GameMenu_Back();
            return 0;
        }
        Menu_CursorUpDown(&gSubMenuWork->subEntry, pad, QUIT_WAYS);
        menu = gSubMenuWork;
        result = menu->subEntry + 0x24; /* the way's help line */
        buttons = pad;
        gHelpLine = result;
        cur = &menu->subEntry;
        if (buttons & PAD_CROSS) {
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            return PAD_CROSS;
        }
        if (buttons & PAD_CIRCLE) {
            if (*cur == QUIT_WAY_EVERY_BOOT && Card_IsSystemSaved() != 1) {
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
            } else {
                gHelpShow = 0;
                gSubMenuWork->yesNo = 1;
                gSubMenuWork->subStep++;
                gSubMenuWork->page = QUIT_PAGE_CONFIRM;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                gFieldControl = 0;
                Chat_HideWindows();
            }
        }
        break;
    case 1:
        Chat_SetWindowMode(3, 2);
        switch (gSubMenuWork->page) {
        case QUIT_PAGE_CONFIRM:
            if (GameMenu_YesNoInput(&pad) == 1) {
                menu = gSubMenuWork;
                if (menu->yesNo == 0) {
                    menu->page++;
                    gSubMenuWork->yesNo = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    if (gSubMenuWork->subEntry == QUIT_WAY_EVERY_BOOT) {
                        TownNetMenu_Begin();
                        gSubMenuWork->page = QUIT_PAGE_SAVE_DIRECT;
                    }
                } else {
                    menu->subStep = 0;
                    gHelpShow = 1;
                    gHelpLine = gSubMenuWork->subEntry + 0x24;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                    gFieldControl = 1;
                    Chat_ShowWindows();
                }
            }
            break;
        case QUIT_PAGE_ASK_SAVE:
            if (pad & PAD_CROSS) {
                gSubMenuWork->yesNo = 1;
                gSubMenuWork->page--;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else if (pad & PAD_CIRCLE) {
                if (gSubMenuWork->yesNo == 0) {
                    TownNetMenu_Begin();
                    gSubMenuWork->page = QUIT_PAGE_SAVE;
                } else {
                    gSubMenuWork->yesNo = 0;
                    gSubMenuWork->page++;
                }
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
            } else {
                gSubMenuWork->yesNo = YesNo_Toggle(pad, gSubMenuWork->yesNo, PAD_LEFT | PAD_RIGHT);
            }
            break;
        case QUIT_PAGE_ASK_AGAIN:
            if (pad & PAD_CROSS) {
                gSubMenuWork->yesNo = 0;
                gSubMenuWork->page--;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else if (pad & PAD_CIRCLE) {
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                if (gSubMenuWork->yesNo == 0) {
                    TownNetMenu_Begin();
                    if (gSubMenuWork->subEntry == QUIT_WAY_ONCE) {
                        gSubMenuWork->page = QUIT_PAGE_SAVE;
                    } else {
                        gSubMenuWork->page = QUIT_PAGE_SAVE_DIRECT;
                    }
                } else {
                    GameMenu_Quit();
                    gSubMenuWork->page = QUIT_PAGE_QUITTING;
                }
            } else {
                gSubMenuWork->yesNo = YesNo_Toggle(pad, gSubMenuWork->yesNo, PAD_LEFT | PAD_RIGHT);
            }
            break;
        case QUIT_PAGE_SAVE:
            result = TownNetMenu_Update(1);
            if (result != 0) {
                if (result < 0) {
                    gSubMenuWork->subStep = 1;
                    gSubMenuWork->page = QUIT_PAGE_ASK_AGAIN;
                    gSubMenuWork->yesNo = 0;
                } else {
                    GameMenu_Quit();
                    gSubMenuWork->page++;
                }
            }
            break;
        case QUIT_PAGE_SAVE_DIRECT:
            result = TownNetMenu_Update(1);
            if (result != 0) {
                if (result < 0) {
                    gSubMenuWork->page = QUIT_PAGE_CONFIRM;
                } else {
                    GameMenu_Quit();
                    gSubMenuWork->page++;
                }
            }
            break;
        default:
            gFieldControl = 1;
            Chat_ShowWindows();
            break;
        }
        break;
    }
    return 0;
}

/* One frame of the game menu's yes/no prompt, on the pad bits at pad (the
 * confirm, cancel and left/right bits are taken out of them). On yes,
 * PAD_CIRCLE answers it; PAD_CROSS or PAD_RIGHT moves to no. On no,
 * PAD_CIRCLE or PAD_CROSS answers it and PAD_LEFT moves back to yes. Returns 1
 * once answered (yesNo holds the answer), 0 until then. */
s32 GameMenu_YesNoInput(u16 *pad)
{
    s32 onYes;
    s32 onNo;
    u16 pressed;

    pressed = *pad;
    *pad = pressed & (u16)~(PAD_CIRCLE | PAD_CROSS | PAD_LEFT | PAD_RIGHT);
    if (gSubMenuWork->yesNo == 0) {
        onYes = pressed & 0xFFFF;
        if (onYes & PAD_CIRCLE) {
            return 1;
        }
        if (onYes & PAD_CROSS) {
            gSubMenuWork->yesNo = 1;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        }
        if (onYes & PAD_RIGHT) {
            gSubMenuWork->yesNo = 1;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
        }
        /* a jump to the one return 0 below: with a return here the compiler
         * gives this branch its own return, unlike the game's code */
        goto done;
    }
    onNo = pressed & 0xFFFF;
    if (onNo & (PAD_CIRCLE | PAD_CROSS)) {
        return 1;
    }
    if (onNo & PAD_LEFT) {
        gSubMenuWork->yesNo = 0;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
    }
done:
    return 0;
}

/* The game menu's open handler for the pause: offline, while the day clock's
 * check is idle (Lobby_GetDayState 0), it hides the help line and the chat
 * windows, asks for the pause screen (gPauseMenuReq), holds the clock and
 * pauses both streams, and returns 0. Online, or mid-check, it returns 1: the
 * entry doesn't open. */
s32 GameMenu_OpenPause(void *menu)
{
    if ((Game_IsOnline() != 1) && (Lobby_GetDayState() == 0)) {
        gHelpShow = 0;
        gPauseMenuReq = 1;
        Lobby_CheckDay(1);
        gChatWinOpen = gChatWinTimer = 0;
        Stream_Pause(1, 1);
        Stream_Pause(0, 1);
        return 0;
    }
    return 1;
}

/* The pause's update: PAD_CIRCLE, PAD_CROSS or PAD_START ends it, letting the
 * day clock run and resuming both streams, and returns PAD_START; 0 while it
 * stays. */
u16 GameMenu_UpdatePause(u16 pad)
{
    if (pad & (PAD_START | PAD_CIRCLE | PAD_CROSS)) {
        Lobby_CheckDay(0);
        Stream_Pause(1, 0);
        Stream_Pause(0, 0);
        return PAD_START;
    }
    return 0;
}

/* The pause entry's draw: nothing (the pause screen draws itself). */
void GameMenu_DrawPause(void)
{
}

/* The member record of town slot idx (gNetUsers). */
void *NetUser_Get(s32 idx)
{
    return &gNetUsers[idx & 0xFF];
}

/* The quit entry's draw. In its list, the game menu's list dimmed with the
 * two ways over it (the second dimmed while the system data isn't saved).
 * Then, on the confirmation and the two questions on saving, the yes/no
 * line (the second question's own labels) and the question in its frame:
 * the way's confirmation, then gQuitConfirmTexts[2] and [3]; nothing on
 * the save menu's pages and after. */
void GameMenu_DrawQuit(void)
{
    s8 question;

    switch (gSubMenuWork->subStep) {
    case 0:
        GameMenu_DrawList(1);
        gQuitPanel.items = (Card_IsSystemSaved() == 1) ? gQuitMenuTexts : gQuitMenuDimTexts;
        ListPanel_Draw(&gQuitPanel, 0, 0, gSubMenuWork->subEntry);
        break;
    case 1:
        Font_SetSpacing(0x12, 0x12);
        Font_SetColor(0x807F7F7F);
        switch (gSubMenuWork->page) {
        case QUIT_PAGE_CONFIRM:
            question = gSubMenuWork->subEntry;
            Font_SetPos(0xE6, 0x104);
            Text_Printf(gYesNoTexts[gSubMenuWork->yesNo]);
            break;
        case QUIT_PAGE_ASK_SAVE:
            question = 2;
            Font_SetPos(0xE6, 0x104);
            Text_Printf(gYesNoTexts[gSubMenuWork->yesNo]);
            break;
        case QUIT_PAGE_ASK_AGAIN:
            question = 3;
            Font_SetPos(0x9E, 0x104);
            Text_Printf(gQuitSaveTexts[gSubMenuWork->yesNo]);
            break;
        default:
            question = -1;
            break;
        }
        if (question >= 0) {
            WindowFrame_Draw(&gQuitFrame, NULL);
            Font_SetColor(0x807F7F7F);
            Font_SetPos(0x9A, 0xB2);
            Text_PrintfColored(gQuitConfirmTexts[question]);
        }
        break;
    }
}

ListPanel gQuitPanel = {
#include "lobby/007F2010.inc"
};
Frame gQuitFrame = {
#include "lobby/007F2030.inc"
};
