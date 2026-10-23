/*
 * The bed's menu in the hunter's house (TOWN_MODE_SAVE: TownSave_Open,
 * TownSave_Update, TownSave_Draw, TownSave_Close) and the save menu it
 * leads to (TownNetMenu_*).
 *
 * The bed offers three lines (enum BedLine): the save, a rest to the next
 * half of the day and a sleep to the next day. Online, the bed only saves.
 * The menu keeps its state in gMenuWork as a TownSaveWork (lobby_menu.h).
 *
 * The save menu is sub_main's card menu (NetMenu_Open, NetMenu_Update),
 * opened over the town: TownNetMenu_Begin stops the town's input and its
 * music, TownNetMenu_Update runs the card menu, and TownNetMenu_Resume
 * starts the town again. The game menu's save line runs it too.
 */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "lobby_net.h"
#include "lobby_text.h"
#include "menu_task.h"
#include "card.h"
#include "em.h"
#include "pad.h"
#include "render_state.h"
#include "player.h"
#include "scene.h"
#include "save.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "quest.h"
#include "stream.h"
#include "bed_menu.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "town_frame.h"
#include "town_input.h"
#include "town_players.h"

extern s8 gTownAreaMsgPending;
extern u8 gMenuWork[];
extern s32 gTownMenuOpen;
extern s32 gTownMode;
/* end of prototypes */
extern s8 gTownNetMenuStep;

/* Opens the bed's menu: the town's mode becomes TOWN_MODE_SAVE, the
 * hunter stops taking input, the menu's work is cleared and TownSave_Draw
 * draws it. The rest costs 400 and the sleep 800; the save is free.
 * Online, the hunter goes to the bed at once (the save is the only line);
 * offline, the menu's sound plays. */
void TownSave_Open(void)
{
    TownSaveWork *w;

    gLobby->connected = 0;
    gTownMode = TOWN_MODE_SAVE;
    gTownMenuOpen = 1;
    w = (TownSaveWork *)gMenuWork;
    memset(w, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(TownSave_Draw, 0, 0, 0);
    gFieldControl = 0;
    w->prices[BED_LINE_SAVE] = 0;
    w->prices[BED_LINE_REST] = 400;
    w->prices[BED_LINE_SLEEP] = 800;
    if (Game_IsOnline() == 1) {
        TownPc_WalkToBed();
    } else {
        LobbySe_Play(LOBBY_SE_MENU_OPEN);
    }
}

/* Starts the town again after the save menu: the hunter takes input
 * again, the area message is sent once more (gTownAreaMsgPending), and the
 * stream and the music, paused by TownNetMenu_Update, play again (the
 * music fades in over 30 frames). */
void TownNetMenu_Resume(void)
{
    gLobby->connected = 1;
    gTownAreaMsgPending = 1;
    gFieldControl = 1;
    Stream_Pause(0, 0);
    Bgm_FadeInFromZero(30);
    Bgm_SetPaused(0);
}

/* Closes the bed's menu: the hunter takes input again and the menu's
 * draw goes. The caller sets the town's mode. */
void TownSave_Close(void)
{
    gTownMenuOpen = 0;
    gFieldControl = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
}

extern LobbyMenu *gLobbyMenu;
void Gfx_SetTexFilter(s32 a);
void Text_SetSize(s32 size);
void Text_Printf(char *fmt, ...);
extern s32 gTownDay;

/*
 * Runs one frame of the bed's menu (enum TownSaveStep). Offline, the
 * town's clock holds while the menu is up.
 *
 * The save asks first offline (LOBBY_TEXT_SAVE_ASK), sends the hunter to
 * the bed and runs the save menu. The rest and the sleep ask whether to
 * pay their price, take it, send the hunter to the bed, and after 240
 * frames move the clock: the rest to the morning's or the afternoon's last
 * second, the sleep to the day's last second, with the world's day set to
 * 2. Either way the hunter gets up when the 240 frames are over.
 *
 * Returns MENU_RUNNING while the menu runs, MENU_DONE once the save or the
 * payment is done (the menu's draw has gone; the hunter still has to get
 * up), MENU_CANCEL when PAD_CROSS leaves the menu at its first step.
 */
s32 TownSave_Update(void)
{
    TownSaveWork *w = (TownSaveWork *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);

    if (Game_IsOnline() == 0 && w->step != TOWN_SAVE_DONE && w->step != TOWN_SAVE_GETTING_UP) {
        LobbyWorld_HoldClock();
    }
    switch (w->step) {
    case TOWN_SAVE_GETTING_UP:
        if (--w->timer <= 0) {
            w->step = TOWN_SAVE_DONE;
            TownSave_Close();
            Player_ClearFlag(pl, PLF_NO_ROOT_MOVE);
            Actor_SetState(pl, 0, TOWN_PC_BED_GET_UP);
        }
        break;
    case TOWN_SAVE_CHOOSE:
        if (Game_IsOnline() == 1) {
            w->step++;
        } else if (pad & PAD_CIRCLE) {
            if (gSave.money >= w->prices[w->line]) {
                if (w->line == BED_LINE_SAVE) {
                    w->step++;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                } else {
                    w->step = TOWN_SAVE_PAY_ANSWER;
                    Menu_SetYesNo(1);
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
                Menu_PushNoticeOnce(LOBBY_NOTICE_NO_MONEY);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            gTownMode = TOWN_MODE_FREE;
            TownSave_Close();
            return MENU_CANCEL;
        } else {
            w->line = Menu_WrapUpDown(w->line, BED_LINE_COUNT);
        }
        break;
    case TOWN_SAVE_ASK:
        if (Game_IsOnline() == 0) {
            LobbyText_Open(LOBBY_TEXT_SAVE_ASK);
            Menu_SetYesNo(1);
            w->step++;
        } else {
            TownNetMenu_Begin();
            w->step = TOWN_SAVE_SAVING;
        }
        break;
    case TOWN_SAVE_ASK_ANSWER:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            w->step++;
            TownNetMenu_Begin();
            TownPc_WalkToBed();
            w->timer = 240;
            break;
        case MENU_CANCEL:
            w->step = TOWN_SAVE_CHOOSE;
            break;
        }
        break;
    case TOWN_SAVE_SAVING:
        if (w->timer != 0) {
            w->timer--;
        }
        if (TownNetMenu_Update(0) == 1) {
            w->step = TOWN_SAVE_GETTING_UP;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            return MENU_DONE;
        }
        break;
    case TOWN_SAVE_PAY_ANSWER:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            w->step++;
            w->timer = 240;
            Save_AddMoney(-w->prices[w->line]);
            LobbySe_Play(LOBBY_SE_MONEY);
            TownPc_WalkToBed();
            break;
        case MENU_CANCEL:
            w->step = TOWN_SAVE_CHOOSE;
            break;
        }
        break;
    case TOWN_SAVE_LYING_DOWN:
        if (--w->timer <= 0) {
            if (w->line == BED_LINE_REST) {
                switch (LobbyTime_IsPm()) {
                case 0:
                    LobbyTime_SetClock(11 * 3600 + 59 * 60 + 59);
                    break;
                case 1:
                    LobbyTime_SetClock(23 * 3600 + 59 * 60 + 59);
                    break;
                }
            } else {
                /* every season sets the same day */
                switch (LobbyWorld_GetSeason()) {
                case 0:
                    gSave.day = 2;
                    break;
                case 1:
                    gSave.day = 2;
                    break;
                case 2:
                    gSave.day = 2;
                    break;
                }
                gTownDay = gSave.day;
                LobbyTime_SetClock(23 * 3600 + 59 * 60 + 59);
            }
            w->step = TOWN_SAVE_GETTING_UP;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            return MENU_DONE;
        }
        break;
    case TOWN_SAVE_DONE:
        break;
    }
    return MENU_RUNNING;
}

/*
 * Draws the bed's menu while the cursor is on its lines or a price is
 * asked: the money held, the three lines with the cursor, and a window
 * below with the chosen line's help, or the line's price and the yes/no.
 * Offline, nothing is drawn while the hunter walks to the bed (TOWN_PC_BED_LIE_DOWN).
 */
void TownSave_Draw(MenuTask *task)
{
    TownSaveWork *w;

    w = (TownSaveWork *)gMenuWork;
    if (gPlayers[gScene.localPlayer].state == TOWN_PC_BED_LIE_DOWN && Game_IsOnline() == 0) {
        return;
    }
    if (w->step == TOWN_SAVE_CHOOSE || w->step == TOWN_SAVE_PAY_ANSWER) {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Gs_SetState(RS_ALPHA_REF, 0);
        Text_SetSize(task->textSize);
        Menu_DrawMoney(480, 16, 0);
        Font_SetSpacing(20, 20);
        Menu_DrawLabelList(450, gBedMenuTexts, BED_LINE_COUNT, w->line, 0xFFB0B000);
        switch (w->step) {
        case TOWN_SAVE_PAY_ANSWER:
            Win_DrawText(276, 352, 340, 80, 0xBDB59F6C, 0x80202020,
                          Text_FillDollar(gBedMenuTexts[BED_MENU_TEXT_LIVING_COST], w->prices[w->line]));
            Font_SetPos(416, 400);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
            break;
        default:
            Win_DrawText(276, 352, 340, 80, 0xBDB59F6C, 0x80202020,
                         (gBedMenuTexts + BED_LINE_COUNT)[w->line]);
            break;
        }
    }
}

/* Stops the town for the save menu: the hunter takes no input, the
 * town's menu is marked open and the music pauses. TownNetMenu_Update
 * starts from its first step. */
void TownNetMenu_Begin(void)
{
    gTownNetMenuStep = 0;
    gLobby->connected = 0;
    gFieldControl = 0;
    gTownMenuOpen = 1;
    Bgm_SetPaused(1);
}

extern s8 gTownNetMenuFade;
u8 NetMenu_Update(void);

/*
 * Runs one frame of the save menu over the town (TownNetMenu_Begin first).
 * It opens the card menu (NETMENU_MODE_SAVE offline,
 * NETMENU_MODE_GAME_SAVE_TOWN online) and fades the stream out while a
 * count goes from 65 down to 6 (59 frames), then stops and pauses the streams, and runs the card menu until
 * it ends, when TownNetMenu_Resume starts the town again.
 *
 * fromGameMenu is 1 when the game menu's save line runs it, 0 from the
 * bed (which plays the save menu's sound). Returns 0 while it runs, 1 when
 * the card menu is done; when it is left without saving, 1 for the bed
 * and -1 for the game menu, which also closes the town's menu.
 */
s32 TownNetMenu_Update(s32 fromGameMenu)
{
    switch (gTownNetMenuStep) {
    case 0:
        gTownNetMenuStep++;
        if (Game_IsOnline() == 0) {
            NetMenu_Open(NETMENU_MODE_SAVE);
        } else {
            NetMenu_Open(NETMENU_MODE_GAME_SAVE_TOWN);
        }
        gTownNetMenuFade = 65;
        /* read back through a volatile view: the game's code stores the
         * count and loads it again for the call */
        Stream_FadeOut(0, *(volatile s8 *)&gTownNetMenuFade);
        break;
    case 1:
        if (--gTownNetMenuFade < 6) {
            gTownNetMenuStep++;
            Stream_Stop(1);
            Stream_Pause(0, 1);
            if (fromGameMenu == 0) {
                LobbySe_Play(LOBBY_SE_SAVE_MENU);
            }
        }
        break;
    case 2:
        switch ((u8)NetMenu_Update()) {
        case 1:
            TownNetMenu_Resume();
            return 1;
        case 2:
            TownNetMenu_Resume();
            if (fromGameMenu == 1) {
                TownSave_Close();
                return -1;
            }
            return 1;
        }
        break;
    }
    return 0;
}
