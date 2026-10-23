/*
 * The field HUD and the in-game menus (sub_main), on gHudState (HudState,
 * hud.h; Hud_Init sets it up for a quest or the town).
 *
 * - The update: ItemHud_Update runs offline and in quests, Hud_Update in
 *   the online town (where the lobby's game menu replaces the field
 *   menu). They pass the pad to a message popup, the chat key, the
 *   pouch-full drop dialog (PouchDrop_Update), the field menu or what the
 *   player has open, and put the HUD's draw hooks into the draw lists.
 * - The field menu (FieldMenu_*): its entries are tables of open, input
 *   and draw functions by cursor (hud.h's FIELD_MENU_* comment): the
 *   pouch (PouchMenu_*), combining (CombineMenu_*), the status page
 *   (StatusMenu_*, PersonalStatus_*, EquipStatus_*, CommentEdit_*), the
 *   quest pages (QuestMenu_*), the info page (InfoMenu_*: the combine
 *   list, the monster notes MonsterNote_*), the phrase or chat menu, the
 *   options (OptionMenu_*) and offline the pause menu. Each menu window
 *   slides in and out (Shake_*, by the SHAKE_* ids).
 * - The item get lock (ItemGet_*) and the drop dialog (PouchDrop_*).
 * - The cockpit's drawing (Hud_Draw*): the map (Map_*: the cockpit's
 *   small map, the area map, and the screen shown while the quest moves
 *   to another area), the item box and the pouch bar (POUCH_BAR_*), the
 *   pouch-full dialog (DROP_*), the party's health and status, the clock
 *   and compass, the weapon box, the horn's notes, the aim view, the
 *   gunners' ammo and prompts, the supply box and the tent menu;
 *   Hud_DrawLayer0..3 (1B..3B in the online town) are the draw hooks
 *   that call them.
 * - The menus' fades (Hud_FadeStart, Fade_*), keyed by FadeDef tables.
 * - At the end, the file's data: the menus' function tables and panels,
 *   the HUD's layouts, texture rectangles and fade tables.
 *
 * The menus' cursor helpers, the help window, the equipment icons and the
 * slides have code of their own, and so do the small queries that other
 * code asks of the HUD.
 *
 * One file: the menus and the cockpit's drawing share the data, in an order of their own. The
 * first tables (the players' map colours, the status icons, then the quest and option menus'
 * rows) mix the two, the item colours serve both, and the jump tables and literals interleave
 * as well; the only clean cuts fall among the last four functions (a quest page's helper and
 * the status page's comment editor), which are menus too. So the file stays whole.
 */
#include "common.h"
#include "window.h"
#include "chat_log.h"
#include "em.h"
#include "player.h"
#include "quest.h"
#include "sprite.h"
#include "scene.h"
#include "pad.h"
#include "item.h"
#include "prim.h"
#include "save.h"
#include "hud.h"
#include "lobby_state.h"
#include "syssave.h"
#include "sub_main/hud.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "sound.h"
#include "area.h"
#include "texture.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "box_menu.h"
#include "camera.h"
#include "game_menu.h"
#include "kbd.h"
#include "qsync.h"
#include "shake.h"
#include "arena_cup.h"
#include "cam_helpers.h"
#include "chat_phrases.h"
#include "draw_helpers.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "equip_parts.h"
#include "font.h"
#include "game_util.h"
#include "hunter_card.h"
#include "lobby_queries.h"
#include "member_list.h"
#include "menu_util.h"
#include "online_mark.h"
#include "place_info.h"
#include "place_info_draw.h"
#include "vibration.h"
#include "supply_box.h"
#include "libgcc.h"
#include "msg_event.h"
#include "em_blangonga.h"

/* Built at optimization level 4: MonsterNote_DrawCrown, Map_DrawAreaChange, Hud_DrawPouchItem,
 * Hud_DrawCompass and Hud_DrawCompassQuad match only at 4, Map_DrawZoomed only at 3 (a pair below),
 * the rest at both. The format strings its functions share show that the file is one of the
 * game's compilation units. */
#pragma optimization_level 4

/* prototypes */
void Text_Draw(char *s);
void Text_SetSize(s32 size);
struct Recipe;
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
/* unit's own declaration: declared here as MonsterNote_Update calls it; with the definition's s8 (s8 cur, s8 dir), the call compiles differently */
s8 MonsterNote_NextKnown(s32 cur, s32 dir);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetTexFilter(s32 a);
void Tex_Bind(s32 a);
s32 func_770310(void);
s32 FriendLimitWin_Input(void); /* unit's own declaration: FriendLimitWin_Input reads a0 = buttons (StatusMenu_Update's, left in a0 through FriendLimit_Update) */
s32 Lobby_LacksQuest(void);
extern u8 gQuestMenuCursorMax[];
extern u8 gFieldMenuHelpLines[];
extern ListPanel gInfoMenuPanel;
extern char **gInfoMenuLabelSets[];
extern ListPanel gChatMenuPanel;
extern s32 gItemNames[];
extern s16 gItemGetWait;
extern u16 gItemGetSerial;
extern u8 gPhraseMenuCursor;
extern u8 gHelpShow;
extern u8 gHelpPage;
extern u16 gHelpLine;
extern u8 gPauseMenuReq;
extern u8 gPauseWinStep;
extern u8 gPauseWinFade;
extern u8 gChatInputMode;
s32 sprintf(char *dst, char *fmt, ...);
void Text_PrintfColored(char *fmt, ...);
extern u16 gFrameTick;
extern QuestInfo *gQuestInfo;
extern u8 gChatHideNames;
extern s8 gHelpColumn;
struct ListWin;
struct Window;
void Text_Printf(char *fmt, ...);
void *memcpy(void *d, void *s, u32 n);
/* end of prototypes */




char *NetUser_Get(u8 id); /* unit's own declaration: the definition (lobby) is void *NetUser_Get(s32 arg0); the calls here match only with this one */
extern char gPlayerIds[][8];

/* The HUD's constants, read-only. */
#pragma rodata on
char gEmptyNameStr[12] = {
#include "sub_main/0046CFB8.inc"
};
#pragma rodata off

/* Prints a player's name, padded to 8 columns. Online with the hide-names
 * option (gChatHideNames), it prints the player's user id instead: from
 * gPlayerIds in a quest, from the lobby's user list (NetUser_Get) in town. */
void Hud_PrintPlayerName(Player *pl)
{
    char *name = (char *)pl->name;

    if (Game_IsOnline() == 1 && gChatHideNames != 0) {
        if (gScene.inLobby == 0) {
            name = gPlayerIds[pl->index];
        } else {
            name = NetUser_Get(pl->index);
        }
    }
    Text_PrintfColored("%-8.8s", name);
}

/* Returns 1 when the menu button (PAD_START) is in btn and the field menu may
 * open: not while the player's trade message shows, nor while a versus
 * quest's input lock runs. */
s32 FieldMenu_Requested(Player *pl, u16 buttons)
{
    if (pl->tradeMsgTimer != 0) {
        return 0;
    }
    if ((Quest_IsVersus() == 1) && (gQuest.inputLock > 0)) {
        return 0;
    }
    return (buttons & 0xFFFF & PAD_START) != 0;
}

extern u8 gChatLines;
extern s32 gChatSendWait;
extern DrawSlot gHudDrawHooks[4];   /* the HUD draw hooks */
/* unit's own declaration: as the file's last declaration of it (u8 in pouch.c) */
s32 Pouch_Add(Player *p, u16 item, s16 n);
u8 MsgEvent_PopupInput(u16 buttons); /* unit's own declaration: the definition (game) is u8 MsgEvent_PopupInput(s32 arg0); the calls here match only with this one */
s32 Field_ControlEnabled(void);

/* The field HUD's update, once a frame in a quest or offline. When the
 * quest is over it puts back the combine menu's ingredients and closes every
 * menu. Otherwise it eases the health shown toward the player's, starts the
 * pouch and ammo slots' blink when they change, steps the aim view's fade
 * and the timers, and passes the pad on: to a message popup first, else to
 * the chat key, the drop dialog, the field menu or what the player has open
 * (the supply box, the delivery menus, the map zoom). Last it puts the HUD's
 * draw hooks into their draw lists. */
void ItemHud_Update(void)
{
    Player *pl;
    u32 i;
    u16 btn;
    u16 pad;
    s32 hotkey;

    if (gScene.eventRunning != 0) {
        return;
    }
    pl = gHudState->player;
    if (Quest_PhaseIsOver(1) == 1) {
        if (gHudState->combineTaken != 0) {
            for (i = 0; i < 3; i++) {
                if (gHudState->combineItems[i] != 0xFFFF) {
                    Pouch_Add(pl, gHudState->combineItems[i], 1);
                }
            }
            gHudState->combineTaken = 0;
        }
        gHudState->mode = 0;
        gHudState->supply = 0;
        gHudState->dropState = DROP_NONE;
        gHudState->menuPhase = 0;
        gScene.menuOpen = 0;
        gChatInputMode = 0;
        return;
    }
    /* the new presses, with the D-pad's auto-repeat */
    btn = gPadState[0].pressed | (gPadState[0].repeat & PAD_DPAD);
    pad = gPadState[0].held;
    Menu_ReadDpad(0);
    if (gHudState->camReqStarted != 0) {
        gHudState->dpadRepeat = 0;
        btn &= (0xFFFF & ~PAD_SELECT);
        pad &= (0xFFFF & ~PAD_SELECT);
    }
    switch (gHudState->phase) {
    case 0:
        gHudState->phase++;
        gHudState->started = 1;
        gHudState->hp = pl->hp;
        gHudState->hpRecoverable = pl->hpRecoverable;
    case 1:
        if (gHudState->hp != pl->hp) {
            if (gHudState->hp < pl->hp) {
                gHudState->hp++;
            } else {
                gHudState->hp--;
            }
        }
        gHudState->hpRecoverable = pl->hpRecoverable;
        gHudState->pouchBarShown = 0;
        if (gHudState->pouchCursor != pl->pouchCursor) {
            gHudState->pouchCursor = pl->pouchCursor;
            if (Pouch_IsSlotUsable(pl, pl->pouchCursor) == 1) {
                gHudState->blink60 = 2;
                if (pl->pouchHudFlags & 4) {
                    gHudState->dir61 = -1;
                } else {
                    gHudState->dir61 = 1;
                }
            }
        }
        if (pl->isGunner == 1) {
            if (gHudState->ammoSlot != pl->ammoSlot) {
                gHudState->ammoSlot = pl->ammoSlot;
                if (pl->ammoSlot != 255) {
                    gHudState->blink64 = 2;
                    if (pl->pouchHudFlags & 0x10) {
                        gHudState->dir65 = -1;
                    } else {
                        gHudState->dir65 = 1;
                    }
                }
            }
        }
        gHudState->aimStart = 0;
        if (Player_IsAiming(pl) == 1) {
            if (gHudState->aimFade >= 4) {
                gHudState->aimStart = 1;
            }
            if (gHudState->aimFade != 0) {
                gHudState->aimFade--;
            }
        } else {
            gHudState->aimFade = 4;
        }
        gHudState->hidden = 0;
        if (pl->itemSelect == 0) {
            gHudState->hidden = gHudState->cockpitOff;
        }
        if (gHudState->glowTimer != 0) {
            gHudState->glowTimer--;
        }
        Hud_UpdateMapPing(pad);
        if (gHudState->supplyMsgTimer > 0) {
            gHudState->supplyMsgTimer--;
        }
        if (gHudState->supplyMoveTimer > 0) {
            gHudState->supplyMoveTimer--;
        }
        if (Game_IsOnline() == 1) {
            Players_UpdatePresent();
            gChatLines = 0;
            if (gChatSendWait <= 0) {
                gChatSendWait = 0;
            } else {
                gChatSendWait--;
            }
        }
        if ((gHudState->popupOpen = MsgEvent_PopupInput(btn)) == 0) {
            switch (gChatInputMode) {
            case 0:
                if (Game_PauseState() == 0 && pl->itemSelect == 0 && pl->tradeMsgTimer == 0 && Field_ControlEnabled() == 1) {
                    hotkey = Kbd_ActHotkeys();
                    if ((btn & PAD_TRIANGLE) || hotkey == 1) {
                        if (Player_Status7Active(pl) != 0) {
                            Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                        } else {
                            Chat_OpenInput(hotkey != 1);
                            btn = pad = gHudState->dpadRepeat = 0;
                        }
                    }
                }
                if (PouchDrop_Update(btn | gHudState->dpadRepeat) == 0) {
                    switch (gHudState->mode) {
                    case 0:
                        if (gChatInputMode == 0) {
                            if (FieldMenu_Requested(pl, btn) == 1) {
                                FieldMenu_Open();
                                Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
                            } else {
                                if (gHudState->supply != 0) {
                                    SupplyBox_Update(pl);
                                } else if (gHudState->deliveryMenu != 0) {
                                    Player_RunDeliveryMenu(pl);
                                } else if (gHudState->deliveryOpen != 0) {
                                    Quest_DeliveryMenu(pl);
                                } else {
                                    Hud_MapZoomInput(btn, pad);
                                }
                                gHudState->supply = pl->supplyBoxOpen;
                                gHudState->deliveryMenu = pl->deliveryMenu;
                                gHudState->deliveryOpen = pl->deliveryOpen;
                            }
                        }
                        break;
                    case 1:
                        FieldMenu_Update(btn | gHudState->dpadRepeat);
                        break;
                    }
                }
                Fade_UpdateAll();
                break;
            case 1:
                Chat_SendInput(btn);
                Hud_MapZoomInput(btn & (0xFFFF & ~PAD_SELECT), pad & (0xFFFF & ~PAD_SELECT));
                if (PouchDrop_Update(0) == 0) {
                    switch (gHudState->mode) {
                    case 1:
                        FieldMenu_Update(0);
                        break;
                    }
                }
                break;
            }
        }
        break;
    }
    Chat_UpdateWindow(btn);
    Pause_Update();
    OT_AddAt(gDrawListText6, &gHudDrawHooks[0], 0, 1);
    OT_AddAt(gDrawListText0b, &gHudDrawHooks[1], 0, 1);
    OT_AddAt(gDrawListText1, &gHudDrawHooks[2], 0, 1);
    OT_AddAt(gDrawListText2, &gHudDrawHooks[3], 0, 1);
    gHudState->camReqStarted = CamReq_Started();
}

extern s8 gNoticeFlags;

/* The HUD's update in the online town, where the game menu (GameMenu_Update)
 * takes the field menu's place: the chat key, the drop dialog, the menus and
 * the chat windows get the pad (none while a notice shows), then the draw
 * hooks go into their draw lists. */
void Hud_Update(void)
{
    u16 buttons;
    s32 hotkey;

    Chat_PickSendTarget();
    buttons = GameMenu_GetPad();
    Menu_ReadDpad(0);
    switch (gHudState->phase) {
    case 0:
        (gHudState->phase)++;
        gHudState->started = 1;
    case 1:
        if (Game_IsOnline() == 1) {
            Players_UpdatePresent();
            gChatLog.win.flags = 0;
            gChatLines = 0;
            if (gChatSendWait <= 0) {
                gChatSendWait = 0;
            } else {
                gChatSendWait--;
            }
        }
        switch (gChatInputMode) {
        case 0:
            if (gNoticeFlags == 0) {
                if (Hud_ChatKeyAllowed() == 1) {
                    hotkey = Kbd_ActHotkeys();
                    if ((buttons & PAD_TRIANGLE) || hotkey == 1) {
                        Chat_OpenInput(hotkey != 1);
                        buttons = gHudState->dpadRepeat = 0;
                    }
                }
                if (PouchDrop_UpdateSave((u16)(buttons | gHudState->dpadRepeat)) == 0) {
                    GameMenu_Update(buttons | gHudState->dpadRepeat);
                }
                Fade_UpdateAll();
            }
            break;
        case 1:
            Chat_SendInput(buttons);
            if (PouchDrop_UpdateSave(0) == 0) {
                GameMenu_Update(0);
            }
            break;
        }
    }
    Chat_UpdateWindowsOnline(buttons);
    Pause_Update();
    OT_AddAt(gDrawListText0b, &gHudDrawHooks[1], 0, 1);
    OT_AddAt(gDrawListText1, &gHudDrawHooks[2], 0, 1);
    OT_AddAt(gDrawListText2, &gHudDrawHooks[3], 0, 1);
}

/* Opens the field menu: its window slides in, the help line follows the
 * cursor, and it has FIELD_MENU_ENTRIES entries offline (one more when the
 * quest has advice), FIELD_MENU_ENTRIES_ONLINE online. */
void FieldMenu_Open(void)
{
    gScene.menuOpen = 1;
    gHudState->mode = 1;
    gHudState->menuPhase = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_FIELD_MENU;
    gHelpLine = (u16) gHudState->fieldMenuCursor;
    if (Game_IsOnline() != 1) {
        gHudState->fieldMenuCount = FIELD_MENU_ENTRIES;
        if (Quest_HasAdvice() == 1) {
            gHudState->fieldMenuCount = (u8) (gHudState->fieldMenuCount + 1);
        }
    } else {
        gHudState->fieldMenuCount = FIELD_MENU_ENTRIES_ONLINE;
    }
    Shake_Start(SHAKE_FIELD_MENU, SHAKE_AXIS_X, 180.0f);
    gHudState->fadeActive = 0;
    Hud_ResetMenuState();
}

extern s32 (*gFieldMenuDecideOnline[])(void);
extern s32 (*gFieldMenuDecide[])(void);
extern u16 (*gFieldMenuInputOnline[])(s32);
extern u16 (*gFieldMenuInput[])(s32);

/* The field menu's input. In phase 0 the cursor picks an entry (PAD_SQUARE
 * toggles the cockpit) and confirming starts it (gFieldMenuDecide or
 * gFieldMenuDecideOnline returns 0 to open it, else the error sound plays);
 * in phase 1 the entry's own input runs (gFieldMenuInput...), whose result
 * PAD_START closes the menu and PAD_CROSS goes back to the list; phase 2 waits for
 * the window to slide out. */
void FieldMenu_Update(u16 buttons)
{
    s32 refused;
    u16 result;
    switch (gHudState->menuPhase) {
    case 0:
        if (Shake_Update(SHAKE_FIELD_MENU) == 1) {
            buttons &= (PAD_DPAD | PAD_CROSS);
        }
        Menu_CursorUpDown(&gHudState->fieldMenuCursor, buttons, gHudState->fieldMenuCount);
        FieldMenu_SetHelpLine();
        if (buttons & PAD_SQUARE) {
            FieldMenu_ToggleCockpit();
        }
        if (buttons & PAD_CIRCLE) {
            Shake_Stop();
            if (Game_IsOnline() == 1) {
                refused = gFieldMenuDecideOnline[gHudState->fieldMenuCursor]();
            } else {
                refused = gFieldMenuDecide[gHudState->fieldMenuCursor]();
            }
            if (refused == 0) {
                (gHudState->menuPhase)++;
                gHudState->state = 0;
                Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
            } else {
                Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
            }
        } else if (buttons & (PAD_START | PAD_CROSS)) {
            FieldMenu_Close();
        }
        break;
    case 1:
        if (Game_IsOnline() == 1) {
            result = gFieldMenuInputOnline[gHudState->fieldMenuCursor](buttons);
        } else {
            result = gFieldMenuInput[gHudState->fieldMenuCursor](buttons);
        }
        if (result & PAD_START) {
            FieldMenu_Exit();
        } else if (result & PAD_CROSS) {
            gHudState->menuPhase = 0;
            gHelpShow = 1;
            gHelpPage = HELP_PAGE_FIELD_MENU;
            FieldMenu_SetHelpLine();
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        break;
    case 2:
        if (Shake_Update(SHAKE_FIELD_MENU) == 0) {
            FieldMenu_Exit();
        }
        break;
    }
}

/* Closes the field menu: its window slides out (in the town, the game menu goes
 * back instead). */
void FieldMenu_Close(void)
{
    if (gScene.inLobby != 0) {
        GameMenu_Back();
        return;
    }
    gHudState->menuPhase = 2;
    gHelpShow = 0;
    Shake_Start(SHAKE_FIELD_MENU, SHAKE_SLIDE, 180.0f);
    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
}

/* Sets the help line to the field menu entry's (online through
 * gFieldMenuHelpLines). */
void FieldMenu_SetHelpLine(void)
{
    if (Game_IsOnline() == 1) {
        gHelpLine = (u16) gFieldMenuHelpLines[gHudState->fieldMenuCursor];
        return;
    }
    gHelpLine = (u16) gHudState->fieldMenuCursor;
}

/* Shows or hides the cockpit (the HUD) from the field menu. */
void FieldMenu_ToggleCockpit(void)
{
    gHudState->cockpitOff ^= 1;
    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
}

extern ListPanel gFieldMenuPanel;
extern char *gFieldMenuItemsOffline[][9];
extern char *gFieldMenuItemsOnline[][7];

/* Draws the field menu's list (in the town, the game menu's). The entry texts
 * come in sets: 1 when the menu is refused or an item get is pending, plus
 * 2 once the quest is over, and set 4 while paused. dimmed is 0 for the
 * live list, with its cursor colour and the cockpit toggle below it, 1 for
 * the list behind an open entry or sliding out (a grey cursor). */
void FieldMenu_DrawList(s8 dimmed)
{
    s16 *pos;
    s16 textSet;

    if (gScene.inLobby != 0) {
        GameMenu_DrawList(dimmed);
        return;
    }
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    pos = Shake_GetOffset(SHAKE_FIELD_MENU);
    textSet = 0;
    if (Hud_ItemMenusAllowed() != 1 || ItemGet_IsReady(NULL) != 1) {
        textSet = 1;
    }
    if (Game_IsOnline() != 1) {
        if (Game_PauseState() != 0) {
            textSet = 4;
        } else if (Quest_PhaseIsOver(0) == 1) {
            textSet += 2;
        }
        gFieldMenuPanel.items = gFieldMenuItemsOffline[textSet];
    } else {
        gFieldMenuPanel.items = gFieldMenuItemsOnline[textSet];
    }
    if (dimmed == 0) {
        gFieldMenuPanel.cursorColor = 0xA9182;
        FieldMenu_DrawCockpit(gHudState->fieldMenuCount, pos);
    } else {
        gFieldMenuPanel.cursorColor = 0x808080; /* grey */
    }
    gFieldMenuPanel.lines = gHudState->fieldMenuCount;
    ListPanel_Draw(&gFieldMenuPanel, pos, gFieldMenuTitle[0], gHudState->fieldMenuCursor);
}

extern void (*gFieldMenuDrawOnline[])(void);
extern void (*gFieldMenuDraw[])(void);

/* Draws the field menu: the list, or the open entry's page (with the help
 * window unless the window is sliding). */
void FieldMenu_Draw(void)
{
    switch (gHudState->menuPhase) {
    case 0:
        FieldMenu_DrawList(0);
        break;
    case 1:
        if (Game_IsOnline() == 1) {
            gFieldMenuDrawOnline[gHudState->fieldMenuCursor]();
        } else {
            gFieldMenuDraw[gHudState->fieldMenuCursor]();
        }
        break;
    case 2:
        FieldMenu_DrawList(1);
        break;
    }
    if (Shake_Value(SHAKE_FIELD_MENU) == 0 && gHelpShow != 0) {
        HelpWindow_DrawCurrent();
    }
}

extern Sprite gFieldMenuCockpitBox;
extern Icon gFieldMenuCockpitIcon;

/* Draws the cockpit toggle's box and button icon below the list's last
 * entry (rows entries), at the window's slide offset pos. */
void FieldMenu_DrawCockpit(s16 rows, s16 *offset)
{
    s16 boxY = (rows + 1) * 22 + 80;
    gFieldMenuCockpitBox.y = boxY;
    Window_DrawText(&gFieldMenuCockpitBox, (s16 *)offset, (u8 *)gCockpitToggleTexts[gHudState->cockpitOff]);
    gFieldMenuCockpitIcon.x = offset[0] + 467;
    gFieldMenuCockpitIcon.y = boxY - 1;
    Draw_Icons(&gFieldMenuCockpitIcon, 1);
}

/* Opens the status page from the field menu: two entries without the lobby
 * overlay (gScene.inLobby 0); with it, five when Game_IsOnline, else three.
 * Returns 0 (opened). */
s32 StatusMenu_Open(void *menu)
{
    gHudState->menuStep = 0;
    gHudState->listCursor = 0;
    if (gScene.inLobby == 0) {
        gHudState->cursorMax = 2;
    } else {
        gHudState->cursorMax = Game_IsOnline() == 1 ? 5 : 3;
    }
    gHelpLine = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_STATUS;
    return 0;
}

u16 OnlineMark_Input(u16 buttons); /* unit's own declaration: the definition (lobby) is s32 OnlineMark_Input(s32 arg0); the calls here match only with this one */
void FriendLimit_Open(void);

/* The status page's input: in step 0 the cursor picks one of its entries
 * (STATUS_*) and confirming opens it; in step 1 the entry's own input runs,
 * and its result PAD_START closes the field menu, PAD_CROSS goes back to the list.
 * Returns the buttons left for the field menu (0 once used). */
u16 StatusMenu_Update(u16 buttons)
{
    switch (gHudState->menuStep) {
    case 0:
        if (buttons & PAD_START) {
            FieldMenu_Close();
            return 0;
        }
        Menu_CursorUpDown(&gHudState->listCursor, buttons, gHudState->cursorMax);
        gHelpLine = gHudState->listCursor;
        if (buttons & PAD_CIRCLE) {
            switch (gHudState->listCursor) {
            case STATUS_PERSONAL:
                PersonalStatus_Open();
                break;
            case STATUS_EQUIPMENT:
                EquipStatus_Open();
                break;
            case STATUS_COMMENT:
                CommentEdit_Open();
                break;
            case STATUS_ONLINE_MARK:
                OnlineMark_Open();
                break;
            case STATUS_FRIEND_LIMIT:
                FriendLimit_Open();
                break;
            }
            (gHudState->menuStep)++;
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        }
        break;
    case 1:
        switch (gHudState->listCursor) {
        case STATUS_PERSONAL:
            buttons = PersonalStatus_Update(buttons);
            break;
        case STATUS_EQUIPMENT:
            buttons = EquipStatus_Update(buttons);
            break;
        case STATUS_COMMENT:
            buttons = Chat_InputLine(buttons);
            break;
        case STATUS_ONLINE_MARK:
            buttons = OnlineMark_Input(buttons);
            break;
        case STATUS_FRIEND_LIMIT:
            buttons = FriendLimit_Update();
            break;
        }
        if (buttons & PAD_START) {
            FieldMenu_Close();
        } else if (buttons & PAD_CROSS) {
            gHudState->menuStep = 0;
            gHelpShow = 1;
            gHelpPage = HELP_PAGE_STATUS;
            gHelpLine = gHudState->listCursor;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        buttons = 0;
        break;
    }
    return buttons;
}

/* The status page's last entry, the lobby's setting: its open and input. */
void FriendLimit_Open(void)
{
    FriendLimitWin_Open();
}

/* Runs the lobby setting's input and returns its result. */
s32 FriendLimit_Update(void)
{
    return FriendLimitWin_Input();
}

extern ListPanel gStatusMenuPanel;

/* Draws the status page: its list over the dimmed field menu, or the open
 * entry's page (the online mark entry draws nothing here). */
void StatusMenu_Draw(void)
{
    u8 listCursor;
    u8 menuStep;

    menuStep = gHudState->menuStep;
    switch (menuStep) {
    case 0:
        FieldMenu_DrawList(1);
        gStatusMenuPanel.lines = gHudState->cursorMax;
        gStatusMenuPanel.cursorColor = 0xFF0A9182;
        ListPanel_Draw(&gStatusMenuPanel, NULL, NULL, gHudState->listCursor);
        break;
    case 1:
        listCursor = gHudState->listCursor;
        switch (listCursor) {
        case STATUS_PERSONAL:
            PersonalStatus_Draw();
            break;
        case STATUS_EQUIPMENT:
            EquipStatus_Draw();
            break;
        case STATUS_COMMENT:
            CommentEdit_Draw();
            break;
        case STATUS_ONLINE_MARK:
            break;
        case STATUS_FRIEND_LIMIT:
            FieldMenu_DrawList(1);
            gStatusMenuPanel.cursorColor = 0x808080;
            ListPanel_Draw(&gStatusMenuPanel, NULL, NULL, gHudState->listCursor);
            FriendLimitWin_Draw();
            break;
        }
        break;
    }
}

extern u8 gInfoMenuCounts[];
extern u8 gInfoMenuEntries[];

/* Sets the info page back to its list: picks the set of entries (0
 * without the lobby overlay, 1 with it while not online, 2 online, or 3
 * in AREA_ONLINE_VILLAGE) and the help line of the entry under the cursor. */
void InfoMenu_Reset(void)
{
    gHudState->menuStep = 0;
    if (gScene.inLobby == 0) {
        gHudState->infoSet = 0;
    } else if (Game_IsOnline() != 1) {
        gHudState->infoSet = 1;
    } else {
        gHudState->infoSet = gScene.area != AREA_ONLINE_VILLAGE ? 2 : 3;
    }
    gHudState->cursorMax = gInfoMenuCounts[gHudState->infoSet];
    gHudState->infoEntry = (gInfoMenuEntries + gHudState->infoSet * INFO_SET_ENTRIES)[gHudState->listCursor];
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_INFO;
    gHelpLine = gHudState->infoEntry;
}

/* Opens the info page from the field menu, its cursor on the first entry
 * and no recipe or monster picked. Returns 0 (opened). */
s32 InfoMenu_Open(void *menu)
{
    gHudState->listCursor = 0;
    InfoMenu_Reset();
    gHudState->unk94 = 0;
    gHudState->combineRecipe = NULL;
    gHudState->noteMonster = 0;
    return 0;
}

s32 QuestMenu_Open(); /* unit's own declaration: QuestMenu_Open ignores a0 (m), which holds &gHudState->infoEntry at InfoMenu_Update's call */
void FieldMenu_Close(void);

/* The info page's input: in step 0 the cursor picks an entry of the set
 * (INFO_*) and confirming opens it; in step 1 the entry's own input runs,
 * and its result PAD_START is passed up (the field menu closes), PAD_CROSS goes
 * back to the list. Returns the buttons left for the field menu. */
u16 InfoMenu_Update(u16 buttons)
{
    switch (gHudState->menuStep) {
    case 0:
        if (buttons & PAD_START) {
            FieldMenu_Close();
            return 0;
        }
        Menu_CursorUpDown(&gHudState->listCursor, buttons, gHudState->cursorMax);
        gHudState->infoEntry = (gInfoMenuEntries + gHudState->infoSet * INFO_SET_ENTRIES)[gHudState->listCursor];
        gHelpLine = gHudState->infoEntry;
        if (buttons & PAD_CIRCLE) {
            switch (gHudState->infoEntry) {
            case INFO_LOCATION:
                PlaceInfo_Open();
                break;
            case INFO_COMBINE_LIST:
                CombineList_Open();
                break;
            case INFO_MONSTER_NOTES:
                MonsterNote_Open();
                break;
            case INFO_QUEST:
                QuestMenu_Open();
                break;
            case INFO_LOBBY_MEMBERS:
                LobbyMember_OpenList();
                break;
            }
            (gHudState->menuStep)++;
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        }
        break;
    case 1:
        switch (gHudState->infoEntry) {
        case INFO_LOCATION:
            buttons = LocationInfo_Update(buttons);
            break;
        case INFO_COMBINE_LIST:
            buttons = CombineList_Update(buttons);
            break;
        case INFO_MONSTER_NOTES:
            buttons = MonsterNote_Update(buttons);
            break;
        case INFO_QUEST:
            buttons = QuestMenu_Update(buttons);
            break;
        case INFO_LOBBY_MEMBERS:
            buttons = LobbyMember_ListInput(buttons);
            break;
        }
        if (buttons & PAD_START) {
            return PAD_START;
        }
        if (buttons & PAD_CROSS) {
            InfoMenu_Reset();
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        buttons = 0;
        break;
    }
    return buttons;
}

/* Draws the info page: its list over the dimmed field menu, or the open
 * entry's page. */
void InfoMenu_Draw(void)
{
    u8 infoEntry;
    u8 menuStep;

    menuStep = gHudState->menuStep;
    switch (menuStep) {
    case 0:
        FieldMenu_DrawList(1);
        gInfoMenuPanel.lines = gHudState->cursorMax;
        gInfoMenuPanel.items = gInfoMenuLabelSets[gHudState->infoSet];
        ListPanel_Draw(&gInfoMenuPanel, NULL, NULL, gHudState->listCursor);
        break;
    case 1:
        infoEntry = gHudState->infoEntry;
        switch (infoEntry) {
        case INFO_LOCATION:
            LocationInfo_Draw();
            break;
        case INFO_COMBINE_LIST:
            ItemList_DrawPage();
            break;
        case INFO_MONSTER_NOTES:
            MonsterNote_DrawPage();
            break;
        case INFO_QUEST:
            QuestMenu_DrawPage();
            break;
        case INFO_LOBBY_MEMBERS:
            LobbyMember_DrawList();
        }
        break;
    }
}

u16 PlaceInfo_Input(void); /* unit's own declaration: PlaceInfo_Input (s32 arg) reads a0 = buttons (LocationInfo_Update's u16 argument, still in a0; passing it adds a mask) */

/* The location entry's input and draw: the lobby's place info, in the town only
 * (in a quest the buttons go back unused). */
u16 LocationInfo_Update(u16 buttons)
{
    if (gScene.inLobby == 0) {
        return buttons;
    }
    return PlaceInfo_Input();
}

/* Draws the place info, in the town only. */
void LocationInfo_Draw(void)
{
    if (gScene.inLobby != 0) {
        PlaceInfo_Draw();
    }
}

struct Grid;

/* Opens the combine list (the info page's entry): five rows a page from
 * the first, no recipe picked, its window sliding in. Returns 0. */
s32 CombineList_Open(void)
{
    gHudState->grid.index = 0;
    gHudState->grid.rows = 5;
    CombineList_Setup(&gHudState->grid, (s16 *)gHudState->listItems, 5);
    gHudState->set = 0;
    gHudState->sets[0].recipe = NULL;
    gHelpShow = 0;
    Shake_Start(SHAKE_COMBINE_LIST, SHAKE_AXIS_X, 333.0f);
    return 0;
}

struct RecipeHit;
u8 Combine_ListRecipesFor(struct RecipeHit *out, s16 item, s32 flags);

/* The combine list's input: moves the cursor over the items that have
 * recipes (Recipe_IsListedFor; the others show as unknown), lists the
 * recipes of the item under it (Combine_ListRecipesFor) and steps through
 * them with PAD_SQUARE. The menu button slides the window out, and the result
 * PAD_START once it is out closes the field menu. */
u16 CombineList_Update(u16 buttons)
{
    s32 i;
    u16 prev;
    u16 item;

    if (Shake_Flags(SHAKE_COMBINE_LIST) & 0x80) {
        if (Shake_Update(SHAKE_COMBINE_LIST) == 0) {
            return PAD_START;
        }
        return 0;
    }
    if ((u16)buttons & PAD_START) {
        Shake_Start(SHAKE_COMBINE_LIST, SHAKE_SLIDE, 333.0f);
        gHelpShow = 0;
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        return 0;
    }
    gHelpShow = 1;
    if (Shake_Update(SHAKE_COMBINE_LIST) == 1) {
        buttons &= (0xFFFF & ~(PAD_DPAD | PAD_CROSS));  /* no D-pad or cancel while it slides */
        gHelpShow = 0;
    }
    prev = gHudState->grid.index;
    CombineList_MoveCursor(&gHudState->grid, (s16 *)gHudState->listItems, buttons);
    for (i = 0; i < gHudState->grid.rows; i++) {
        if (Recipe_IsListedFor((s16)gHudState->listItems[i], 4) != 1) {
            gHudState->listItems[i] = 0;
        }
    }
    item = gHudState->listItems[gHudState->grid.row];
    if (item != 0) {
        gHelpPage = HELP_PAGE_ITEMS;
        gHelpLine = HELP_ITEM_LINE(item);
        if (prev != gHudState->grid.index) {
            gHudState->sets[0].recipe = NULL;
            buttons = 0;
            gHudState->set = 0;
        }
        if (gHudState->sets[0].recipe == NULL) {
            gHudState->setCount = Combine_ListRecipesFor(gHudState->sets, item, gHudState->grid.filter);
        }
        if (gHudState->setCount > 1 && ((u16)buttons & PAD_SQUARE)) {
            (gHudState->set)++;
            if (gHudState->set >= gHudState->setCount) {
                gHudState->set = 0;
            }
            Sound_PlaySeFlat(0, 9, 0);
        }
    } else {
        gHelpPage = HELP_PAGE_FIELD_MENU;
        gHelpLine = HELP_LINE_NONE;
        gHudState->sets[0].recipe = NULL;
        gHudState->set = 0;
    }
    return buttons;
}

extern ListPanel gItemListPanel;
extern Frame gItemListFrame[];
extern char *gCombineListNoText;
extern char *gUnknownNameText;
extern char *gRecipeUnknownFmt;
extern char *gRecipeRateLines[];
struct Frame;
struct ShakeOffset;
void Text_DrawLinesColored(s16 x, s16 y, char **names, s16 h);
u32 strlen(const void *s);
extern s32 gItemColors[];

/* Draws the combine list's page: each item with its number, name and icon
 * (unknown ones as a question mark), then the recipe picked for the item
 * under the cursor: its ingredients, success rate and yield if the hunter
 * knows it, and the hint to switch recipes when there are several. The
 * recipe rows use number for the row's y and y for the row's index (the
 * game's registers and steps follow that reuse). */
void ItemList_DrawPage(void)
{
    ItemData *itemData;
    Sprite iconSprite;
    char buf[48];
    s16 offset[2];
    s32 i;
    u16 *items;
    s16 number;
    s16 y;
    char *name;
    u8 count;
    RecipeHit *set;
    Recipe *shown;
    s32 rows;
    s16 halfWidth;

    Shake_CopyOffset(SHAKE_COMBINE_LIST, (struct ShakeOffset *)offset);
    sprintf(buf, gCombineListFmt[0], gHudState->grid.col + 1, gHudState->grid.cols);
    ListPanel_Draw(&gItemListPanel, offset, buf, gHudState->grid.row + 1);
    Menu_DrawFrame(&gItemListPanel, offset);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    Font_SetPos(offset[0] + 315, 82);
    Text_PrintfColored(gCombineListNoText);
    y = 102;
    items = gHudState->listItems;
    number = gHudState->grid.col * gHudState->grid.rows + 1;
    if (gHudState->grid.col < gHudState->grid.cols - 1) {
        count = gHudState->grid.rows;
    } else {
        count = gHudState->grid.lastRows;
    }
    iconSprite.x = offset[0] + 370;
    iconSprite.w = 23;
    iconSprite.h = 18;
    for (i = count; i > 0; i--) {
        itemData = &gItemData[*items];
        if (*items != 0) {
            name = (char *)gItemNames[*items];
            iconSprite.u0 = (((itemData->icon + 1) & 7) << 5) + 1;
            iconSprite.v0 = (((itemData->icon + 1) >> 3) << 5) + 1;
            iconSprite.u1 = iconSprite.u0 + 30;
            iconSprite.v1 = iconSprite.v0 + 30;
        } else {
            name = gUnknownNameText;
            *(s32 *)&iconSprite.u0 = TEX_UV(65, 129);   /* the question mark */
            *(s32 *)&iconSprite.u1 = TEX_UV(95, 159);
        }
        Font_SetPos(offset[0] + 315, y);
        Text_PrintfColored(gItemListLineTexts[0], number, name);
        iconSprite.y = y;
        iconSprite.color = gItemColors[itemData->color];
        Sprite_Draw(&iconSprite);
        items++;
        number++;
        y += 20;
    }
    WindowFrame_Draw(&gItemListFrame[0], offset);
    WindowFrame_Draw(&gItemListFrame[1], offset);
    Text_DrawLinesColored((s16)(offset[0] + 534), 228, gRecipeRateLines, 50);
    set = &gHudState->sets[gHudState->set];
    shown = set->recipe;
    if (shown != NULL) {
        Font_SetPos(offset[0] + 315, 218);
        if (Recipe_IsKnown(shown) == 1) {
            Text_PrintfColored(gRecipeMaterialsFmt[0], gHudState->set + 1);
            number = 248;
            rows = !(shown->flags & RECIPE_THREE_ITEMS) ? 2 : 3;
            for (y = 0; rows > 0; rows--, y++, number += 20) {
                Font_SetPos(offset[0] + 315, number);
                Text_PrintfColored(gRecipeMaterialFmts[y], gItemNames[set->info[y]]);
            }
            Font_SetColor(0x802B7F2B);
            Font_SetPos(offset[0] + 552, 248);
            Text_Draw(gRecipeRateLabels[shown->rateClass]);
            halfWidth = strlen(gRecipeYieldLabels[shown->yieldClass]) * 9 / 2;
            Font_SetPos(570 - halfWidth + offset[0], 298);
            Text_Draw(gRecipeYieldLabels[shown->yieldClass]);
        } else {
            Text_PrintfColored(gRecipeUnknownFmt, gHudState->set + 1);
        }
        if (gHudState->setCount > 1) {
            Font_SetPos(offset[0] + 315, 318);
            Font_SetColor(0x807F7F7F);
            Text_Printf(gRecipeSwitchHint[0]);
        }
    }
}

/* Opens the monster notes (the info page's entry) on the first monster the
 * hunter knows, on the record page, its window sliding in. Returns 0. */
s32 MonsterNote_Open(void)
{
    gHudState->noteMonster = MonsterNote_NextKnown(0, 0);
    gHudState->notePage = (s8)MONSTER_NOTE_RECORD;
    gHelpPage = HELP_PAGE_INFO;
    gHelpLine = 5;
    gHelpShow = 0;
    Shake_Start(SHAKE_MONSTER_LIST, SHAKE_AXIS_X, 333.0f);
    return 0;
}

/* The monster notes' input: left and right step through the monsters the
 * hunter knows, and once EVENT_FLAG_ARENA_CUP_OPEN is set PAD_SQUARE switches between the
 * record page and the materials page (which shows the help line). The menu
 * button slides the window out; the result PAD_START then closes the menu. */
u16 MonsterNote_Update(u16 buttons)
{
    s8 next;
    s8 *shown;

    gHelpShow = 0;
    if (Shake_Flags(SHAKE_MONSTER_LIST) & 0x80) {
        if (Shake_Update(SHAKE_MONSTER_LIST) == 0) {
            return PAD_START;
        }
        return 0;
    }
    if (buttons & PAD_START) {
        Shake_Start(SHAKE_MONSTER_LIST, SHAKE_SLIDE, 333.0f);
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        return 0;
    }
    if (Shake_Update(SHAKE_MONSTER_LIST) == 1) {
        buttons = 0;
    }
    if (MonsterNote_AnyKnown() == 1) {
        if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 1 && (buttons & PAD_SQUARE)) {
            if (gHudState->notePage & MONSTER_NOTE_RECORD) {
                gHudState->notePage &= ~MONSTER_NOTE_RECORD;
            } else {
                gHudState->notePage |= MONSTER_NOTE_RECORD;
            }
            Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
            buttons = 0;
        }
        if (!(gHudState->notePage & MONSTER_NOTE_RECORD)) {
            gHelpShow = 1;
        }
        if ((u16)buttons & (PAD_LEFT | PAD_RIGHT)) {         /* left or right */
            next = MonsterNote_NextKnown(gHudState->noteMonster, (buttons & PAD_LEFT) ? -1 : 1);
            shown = &gHudState->noteMonster;
            if (*shown != next) {
                *shown = next;
                Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            }
        }
    } else {
        gHudState->noteMonster = -1;
    }
    return buttons;
}

s32 Str_Length(char *s);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
extern ListPanel gMonsterNotePanels[];
extern Panel gMonsterNoteHelpBoxes[];
extern char *gMonsterCountFmt;
extern char *gMonsterSizeFmt;
extern char *gArenaBaitTitle;
extern char *gArenaCannotRaiseFmt;
extern char *gMonsterRecordLabels;
extern char *gMonsterListHint;
extern char *gMonsterNoteNumFmt;
extern u16 *gMonsterNoteMarks[]; /* per material set: 8-byte entries, the item first */

/* Draws the monster notes' page for the monster picked (or a line saying
 * the notes are empty): the note's number, then on the record page its
 * name, class, the count hunted and captured, its description, the size
 * records with their crowns (for kinds whose size varies) and its picture;
 * on the materials page the five materials with their icons, each named
 * once the hunter has it (MonsterNote_GetListFlags), or a line for a
 * monster without materials. The help box shows once event flag 0xC4 is
 * set. */
void MonsterNote_DrawPage(void)
{
    char largestText[24];
    char smallestText[16];
    Sprite iconSprite;
    char line[64];
    s16 offset[2];
    u8 recordKind;
    char *title;
    u8 page;
    u32 count;
    MonsterNoteDef *def;
    char *helpText;
    s32 noteIndex;
    u16 *material;
    s32 i;
    s16 y;
    u8 known;
    s32 sizePercent;
    f32 size;
    s32 halfWidth;

    Shake_CopyOffset(SHAKE_MONSTER_LIST, (struct ShakeOffset *)offset);
    if (gHudState->notePage & MONSTER_NOTE_RECORD) {
        title = gMonsterListTitle[0];
        page = 0;
    } else {
        title = gArenaBaitTitle;
        page = 1;
    }
    ListPanel_Draw(&gMonsterNotePanels[page], offset, title, -1);
    Font_SetSpacing(18, 18);
    count = MonsterNote_CountKnown();
    if (count != 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(offset[0] + 539, 62);
        sprintf(largestText, gMonsterNoteNumFmt, MonsterNote_Number((u8)gHudState->noteMonster), count);
        Text_PrintfColored(gMonsterNoteValueFmt[0], largestText);
    }
    if (count > 1) {
        Menu_DrawFrame(&gMonsterNotePanels[page], offset);
    }
    if (gHudState->noteMonster >= 0) {
        def = &gMonsterNoteDefs[gHudState->noteMonster];
        if (gHudState->notePage & MONSTER_NOTE_RECORD) {
            Font_SetColor(0x807F7F28);
            Font_SetPos(offset[0] + 287, 82);
            Text_PrintfStyled(2, gMonsterNoteLabels[0]);
            Font_SetColor(0x807F7F7F);
            Font_SetPos(offset[0] + 341, 82);
            Text_PrintfColored(gMonsterNames[def->emType]);
            Font_SetPos(offset[0] + 341, 102);
            Text_PrintfColored(gEmClassNames[def->emClass]);
            Font_SetPos(offset[0] + 413, 122);
            count = Record_GroupHunted(&gSave, def->emType);
            count += Record_GroupCaptured(&gSave, def->emType);
            if (count > 9999) {
                count = 9999;
            }
            Text_PrintfColored(gMonsterCountFmt, count);
            Font_SetPos(offset[0] + 287, 191);
            Text_PrintfStyled(1, gEmDescTexts[def->note]);
            if (Em_HasRandomSize(def->emType) == 1) {
                Font_SetColor(0x807F7F28);
                Font_SetPos(offset[0] + 287, 142);
                Text_PrintfStyled(2, gMonsterRecordLabels);
                if (count != 0) {
                    sizePercent = (s16)Record_GroupLargest(&gSave, def->emType, &recordKind);
                    size = Monster_SizeFromPercent(recordKind, (u16)sizePercent);
                    SizeRecord_DrawLarge(def, offset[0] + 377, 142, sizePercent);
                    sprintf(largestText, gMonsterSizeFmt, fptodp(size));
                    sizePercent = (s16)Record_GroupSmallest(&gSave, def->emType, &recordKind);
                    size = Monster_SizeFromPercent(recordKind, (u16)sizePercent);
                    SizeRecord_DrawSmall(def, offset[0] + 377, 162, sizePercent);
                    sprintf(smallestText, gMonsterSizeFmt, fptodp(size));
                } else {
                    sprintf(largestText, gMonsterNoSizeText[0]);
                    sprintf(smallestText, gMonsterNoSizeText[0]);
                }
                Font_SetColor(0x807F7F7F);
                Font_SetPos(offset[0] + 413, 142);
                Text_PrintfColored(largestText);
                Font_SetPos(offset[0] + 413, 162);
                Text_PrintfColored(smallestText);
            }
            Menu_DrawEntryPicture(def, offset[0] + 495, 102);
            helpText = gArenaBaitHint[0];
        } else {
            noteIndex = MonsterNote_Index(def->emType);
            if (noteIndex != MONSTER_NOTE_NONE) {
                Font_SetColor(0x807F7F28);
                Font_SetPos(offset[0] + 287, 82);
                Text_PrintfStyled(2, gArenaBaitLabels[0]);
                Font_SetColor(0x807F7F7F);
                Font_SetPos(offset[0] + 341, 82);
                Text_PrintfColored(gMonsterNames[def->emType]);
                material = gMonsterNoteMarks[noteIndex];
                known = MonsterNote_GetListFlags(def->emType);
                y = 122;
                Font_SetColor(0x807F7F7F);
                Tex_Prepare(1, 324);
                Tex_Bind(324);
                iconSprite.w = 23;
                iconSprite.x = offset[0] + 359;
                iconSprite.h = 18;
                for (i = 0; i < 5;) {
                    Font_SetPos(offset[0] + 395, y);
                    if (known & 1) {
                        ItemData *itemData;

                        Text_PrintfColored((char *)gItemNames[*material]);
                        itemData = &gItemData[*material];
                        iconSprite.u0 = (((itemData->icon + 1) & 7) << 5) + 1;
                        iconSprite.v0 = (((itemData->icon + 1) >> 3) << 5) + 1;
                        iconSprite.u1 = iconSprite.u0 + 30;
                        iconSprite.v1 = iconSprite.v0 + 30;
                        iconSprite.color = gItemColors[itemData->color];
                    } else {
                        Text_PrintfColored(gUnknownNameText);
                        *(s32 *)&iconSprite.u0 = TEX_UV(65, 129); /* the question mark */
                        *(s32 *)&iconSprite.u1 = TEX_UV(95, 159);
                        iconSprite.color = gItemColors[0];
                    }
                    iconSprite.y = y;
                    Sprite_Draw(&iconSprite);
                    i++;
                    known >>= 1;
                    material += 4;
                    y += 20;
                }
            } else {
                Font_SetColor(0x802B7F7F);
                sprintf(line, gArenaCannotRaiseFmt, gMonsterNames[def->emType]);
                halfWidth = (s16)Str_Length(line) * 9 / 2;
                Font_SetPos(offset[0] + 449 - halfWidth, 122);
                Text_Printf(line);
            }
            helpText = gMonsterListHint;
        }
        if (Save_TestEventFlag(196) == 1) {
            Window_DrawText(&gMonsterNoteHelpBoxes[page], (s16 *)offset, (u8 *)helpText);
        }
    } else {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(offset[0] + 327, 162);
        Text_Printf(gArenaBaitNoBookText[0]);
    }
}

/* Draws a monster note's picture (EntryPicture_GetUV) at x, y in its frame
 * and shadow; nothing for a note without one. */
void Menu_DrawEntryPicture(MonsterNoteDef *note, s16 x, s16 y)
{
    SpriteWords sprite;

    if (note->picture == 255) {
        return;
    }
    Gfx_DrawLine(x + 3, y + 3, 94, 74, 0xFF493120);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    sprite.x = x;
    sprite.y = y;
    sprite.w = 94;
    sprite.h = 6;
    sprite.color = 0xFF493120;
    sprite.uv0 = TEX_UV(65, 129);
    sprite.uv1 = TEX_UV(126, 135);
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x = x + 6;
    sprite.y = y + 74;
    sprite.uv0 = TEX_UV(125, 134);
    sprite.uv1 = TEX_UV(64, 128);
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x = x;
    sprite.y = y + 6;
    sprite.w = 6;
    sprite.h = 74;
    sprite.uv0 = TEX_UV(129, 131);
    sprite.uv1 = TEX_UV(136, 190);
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x = x + 94;
    sprite.y = y;
    sprite.uv0 = TEX_UV(135, 189);
    sprite.uv1 = TEX_UV(128, 130);
    Sprite_Draw((struct Sprite *)&sprite);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, TEX_SLOT_PICTURES);
    Tex_Bind(TEX_SLOT_PICTURES);
    sprite.x = x + 18;
    sprite.w = 64;
    sprite.y = y + 8;
    sprite.h = 64;
    EntryPicture_GetUV(&sprite.uv0, &sprite.uv1, note->picture);
    sprite.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&sprite);
}


/* The size records' crowns for monster note `note` (the lobby and plsel
 * call these): see SizeRecord_DrawLarge and SizeRecord_DrawSmall. */
s32 MonsterNote_DrawLargeCrown(u16 note, s16 x, s16 y, s32 size)
{
    return SizeRecord_DrawLarge(&gMonsterNoteDefs[note], x, y, size);
}

/* The same for the small crown. */
s32 MonsterNote_DrawSmallCrown(u16 note, s16 x, s16 y, s32 size)
{
    return SizeRecord_DrawSmall(&gMonsterNoteDefs[note], x, y, size);
}

/* Draws a size record's crown at x, y, size wide (25 when 0): CROWN_GOLD
 * picks the gold one, CROWN_SMALL the small crown's mark inside it. */
void MonsterNote_DrawCrown(s16 x, s16 y, s16 size, u8 flags)
{
    SpriteWords sprite;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    if (size == 0) {
        size = 25;
    }
    sprite.x = x;
    sprite.y = y;
    sprite.w = size;
    sprite.h = size * 18 / 25;
    sprite.color = 0xFFFFFFFF;
    if (flags & CROWN_GOLD) {
        sprite.uv0 = TEX_UV(100, 202);
        sprite.uv1 = TEX_UV(120, 215);
    } else {
        sprite.uv0 = TEX_UV(100, 221);
        sprite.uv1 = TEX_UV(120, 234);
    }
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x = x + size * 6 / 25;
    sprite.y = y + size * 7 / 25;
    sprite.w = size * 10 / 25;
    sprite.h = size * 7 / 25;
    if (flags & CROWN_SMALL) {
        sprite.uv0 = TEX_UV(126, 200);
        sprite.uv1 = TEX_UV(134, 205);
    } else {
        sprite.uv0 = TEX_UV(126, 208);
        sprite.uv1 = TEX_UV(134, 213);
    }
    Sprite_Draw((struct Sprite *)&sprite);
}

/* Draws the big crown for a largest size `value` (percent) at x, y if the
 * note's bigCrown is reached, gold from goldCrown. Returns 0 none, 1 the
 * silver crown, 2 the gold one. */
s32 SizeRecord_DrawLarge(MonsterNoteDef *note, s16 x, s16 y, s32 value)
{
    s16 size = value;
    u8 gold;

    if (size < note->bigCrown) {
        return 0;
    }
    gold = size >= note->goldCrown;
    MonsterNote_DrawCrown(x, y, 0, gold);
    return (s16)(gold + 1);
}

/* Draws the small crown for a smallest size `value` at x, y if it is at
 * most the note's smallCrown. Returns 1 if drawn, else 0. */
s32 SizeRecord_DrawSmall(MonsterNoteDef *note, s16 x, s16 y, s32 value)
{
    s16 size = value;

    if (size > note->smallCrown) {
        return 0;
    }
    MonsterNote_DrawCrown(x, y, 0, CROWN_GOLD | CROWN_SMALL);
    return 1;
}

/* Returns the monster note of monster kind `kind`, or NULL if it has none.
 * (The loop's goto form is the game's code.) */
MonsterNoteDef *MonsterNote_FindKind(s32 kind)
{
    MonsterNoteDef *def;
    s32 i;

    def = gMonsterNoteDefs;
    i = 0;
loop_1:
    if (def->emType == (kind & 0xFF)) {
        return def;
    }
    i += 1;
    def++;
    if (i >= MONSTER_NOTE_COUNT) {
        return 0;
    }
    goto loop_1;
}

/* Called when the hunter gets an item: every monster note whose knownItem
 * it is becomes known. */
void MonsterNote_OnItemGet(u16 item)
{
    MonsterNoteDef *def;
    s32 left;

    def = gMonsterNoteDefs;
    left = MONSTER_NOTE_COUNT;
    do {
        if (def->knownItem == (item & 0xFFFF)) {
            MonsterNote_SetKnown((s32) def->note);
        }
        left -= 1;
        def++;
    } while (left > 0);
}

/* Sets and returns hasQuestInfo: 0 in the lobby while no quest is taken,
 * else 1. */
s32 QuestMenu_CheckInfo(void)
{
    if ((gScene.inLobby == 1) && (Lobby_HasQuest() == 0)) {
        gHudState->hasQuestInfo = 0;
        return 0;
    }
    gHudState->hasQuestInfo = 1;
    return 1;
}

/* Opens the quest pages (the field menu's or the info page's entry) on the
 * first page, with the page count of the lobby or of a quest, the window
 * sliding in. Returns 0. */
s32 QuestMenu_Open(void *menu)
{
    QuestMenu_CheckInfo();
    gHudState->pageCursor = 0;
    gHudState->questMenuStep = 0;
    gHudState->questMenuInField = (u8) (gScene.inLobby == 0);
    gHudState->cursorMax = (u8) gQuestMenuCursorMax[gHudState->questMenuInField];
    gHelpShow = 0;
    Shake_Start(SHAKE_QUEST_MENU, SHAKE_AXIS_X, 280.0f);
    return 0;
}

/* The quest pages' input: left and right turn the pages (when there is a
 * quest), the menu button slides the window out, and once it is out the
 * result PAD_START closes the menu. */
u16 QuestMenu_Update(u16 buttons)
{
    gHelpShow = 0;
    switch (gHudState->questMenuStep) {
    case 0:
        Shake_Update(SHAKE_QUEST_MENU);
        if (QuestMenu_CheckInfo() == 1) {
            Menu_CursorLeftRight(&gHudState->pageCursor, buttons & (PAD_LEFT | PAD_RIGHT), gHudState->cursorMax);
        }
        if (buttons & PAD_START) {
            (gHudState->questMenuStep)++;
            Shake_Start(SHAKE_QUEST_MENU, SHAKE_SLIDE, 280.0f);
            buttons = 0;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        break;
    case 1:
        if (Shake_Update(SHAKE_QUEST_MENU) == 0) {
            return PAD_START;
        }
        buttons = 0;
        break;
    }
    return buttons;
}

extern ListPanel gQuestMenuPanel;
extern u8 gQuestMenuKinds[][4];
extern char *gNoQuestText;
extern char *gQuestMenuItemLists[][17];
s32 func_793C10(void);
void func_793BE0(void);

/* Draws the quest page under pageCursor (or a line saying no quest is
 * taken): the page's kind is gQuestMenuKinds[questMenuInField][pageCursor]
 * (QUEST_PAGE_*), and the labels it lists depend on the quest (a versus
 * quest in the field, the arena cup in the town, a must-order quest, a drill). Under
 * the quest's name: the targets, the summary (client, stars, terms), the
 * time limit, fee and rewards, or the place; a drill shows its par time
 * and best record instead. */
void QuestMenu_DrawPage(void)
{
    s16 *offset = Shake_GetOffset(SHAKE_QUEST_MENU);
    s32 mustOrder;
    s32 kind;
    s8 versus;
    s16 itemList;
    char buf[64];

    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    if (gHudState->hasQuestInfo == 0) {
        gQuestMenuPanel.items = NULL;
        ListPanel_Draw(&gQuestMenuPanel, offset, gQuestStatusTitle[0], -1);
        Font_SetPos(*offset + 359, 182);
        Text_Draw(gNoQuestText);
        return;
    }
    versus = 0;
    if (gScene.inLobby == 0) {
        if (Quest_IsVersus() == 1) {
            versus = 1;
        }
        mustOrder = Quest_IsMustOrder();
    } else {
        if (ArenaCup_QuestTaken() == 1) {
            versus = 2;
        }
        mustOrder = func_793C10();
    }
    kind = (s16)gQuestMenuKinds[gHudState->questMenuInField][gHudState->pageCursor];
    switch (kind) {
    case QUEST_PAGE_TARGETS:
        sprintf(buf, gQuestMenuRecordTitles[0]);
        break;
    default:
        sprintf(buf, gQuestMenuTitles[0], gHudState->pageCursor + 1, gHudState->cursorMax);
        break;
    }
    if (Quest_IsDrill() == 1) {
        gQuestMenuPanel.items = gQuestMenuItemLists[QuestMenu_DrillItemList(kind)];
    } else {
        itemList = kind;
        switch (kind) {
        case QUEST_PAGE_SUMMARY:
            if (mustOrder == 1) {
                itemList = 8;
            }
            break;
        case QUEST_PAGE_TERMS:
            if (versus != 0) {
                itemList = 7;
            } else if (mustOrder == 1) {
                itemList = 9;
            }
            break;
        }
        gQuestMenuPanel.items = gQuestMenuItemLists[itemList];
    }
    ListPanel_Draw(&gQuestMenuPanel, offset, buf, -1);
    Menu_DrawFrame(&gQuestMenuPanel, offset);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(*offset + 359, 102);
    Text_Draw(QuestInfo_Get(0));
    switch (kind) {
    case QUEST_PAGE_TARGETS:
        QuestMenu_DrawTargets(*offset);
        break;
    case QUEST_PAGE_SUMMARY:
        if (mustOrder != 1) {
            Font_SetPos(*offset + 359, 182);
            Text_Draw(QuestInfo_Get(1));
            Font_SetPos(*offset + 449, 202);
            Text_DrawStars(gQuestInfo->stars);
            Font_SetPos(*offset + 359, 262);
            Text_Draw(QuestInfo_Get(2));
            Font_SetPos(*offset + 359, 322);
            Text_Draw(QuestInfo_Get(3));
            Font_SetPos(*offset + 359, 382);
            Text_Draw(gQuestKindNames[gQuestInfo->terms]);
        } else {
            Font_SetPos(*offset + 359, 182);
            Text_Draw(QuestInfo_Get(1));
            Font_SetPos(*offset + 359, 202);
            Text_Draw(QuestInfo_Get(2));
            Font_SetPos(*offset + 359, 222);
            Text_Draw(QuestInfo_Get(3));
            Font_SetPos(*offset + 449, 262);
            Text_DrawStars(gQuestInfo->stars);
            Font_SetPos(*offset + 359, 322);
            Text_Draw(gQuestKindNames[gQuestInfo->terms]);
        }
        break;
    case QUEST_PAGE_TERMS:
        if (Quest_IsDrill() == 1) {
            QuestMenu_DrawParTime(*offset);
            break;
        }
        Font_SetPos(*offset + 449, 162);
        switch (versus) {
        case 0:
        case 1:
            QuestMenu_DrawTimeLimit(1);
            break;
        case 2:
            func_793BE0();
            break;
        }
        if (versus == 0) {
            Font_SetPos(*offset + 449, 182);
            QuestMenu_DrawNumber(gQuestInfo->fee);
            if (mustOrder != 1) {
                Font_SetPos(*offset + 485, 202);
                QuestMenu_DrawNumber(gQuestInfo->reward);
                Font_SetPos(*offset + 485, 222);
                QuestMenu_DrawNumber(gQuestInfo->rewardSubA);
                Font_SetPos(*offset + 485, 242);
                QuestMenu_DrawNumber(gQuestInfo->rewardSubB);
            } else {
                Font_SetPos(*offset + 449, 202);
                QuestMenu_DrawNumber(gQuestInfo->reward);
            }
        } else {
            Font_SetPos(*offset + 449, 182);
            QuestMenu_DrawNumber(0);
        }
        Font_SetPos(*offset + 359, 302);
        Text_Draw(QuestInfo_Get(4));
        Font_SetPos(*offset + 359, 382);
        Text_Draw(QuestInfo_Get(5));
        break;
    case QUEST_PAGE_PLACE:
        if (Quest_IsDrill() == 1) {
            QuestMenu_DrawBestRecord(*offset);
            break;
        }
        Font_SetPos(*offset + 449, 162);
        Text_Draw(QuestInfo_Get(6));
        Font_SetPos(*offset + 449, 182);
        Text_Draw(gStageNames[gQuestInfo->map]);
        Font_SetPos(*offset + 359, 242);
        Text_Draw(QuestInfo_Get(7));
        break;
    }
}

extern char gHudFmtInt[];

/* Prints the quest's time limit in minutes (gQuestMenuStatusTexts[0]),
 * the number in full-width digits. */
void QuestMenu_DrawTimeLimit(s32 unused)
{
    char buf[4];
    u16 out[6];
    u16 *dst;
    char *src;
    char ascii;

    sprintf(buf, gHudFmtInt, gQuestInfo->timeLimit / 1800);   /* frames to minutes */
    dst = out;
    src = buf;
    ascii = *src;
    while (ascii != 0) {
        *dst = SJIS_WIDE_DIGIT(ascii);
        src++;
        ascii = *src;
        dst++;
    }
    *dst = 0;
    Text_PrintfColored(gQuestMenuStatusTexts[0], out);
}

#pragma rodata on
char gHudFmtInt[3] = {
#include "sub_main/0046CFD0.inc"
};
#pragma rodata off

/* Draws a quest time (Quest_GetTime(which): 0 the time left, 1 the time
 * taken) as minutes and seconds in full-width characters. */
void QuestMenu_DrawTime(u8 which)
{
    char buf[8];
    u16 out[12];
    u16 *dst;
    char *src;
    char ascii;
    s32 seconds;
    s32 minutes;

    seconds = Quest_GetTime(which) / 30;  /* frames to seconds */
    minutes = seconds / 60;
    seconds -= minutes * 60;
    sprintf(buf, "%02d:%02d", minutes, seconds);
    dst = out;
    src = buf;
    ascii = *src;
    while (ascii != 0) {
        if (ascii != ':') {
            *dst = SJIS_WIDE_DIGIT(ascii);
        } else {
            *dst = SJIS_WIDE_COLON;
        }
        src++;
        ascii = *src;
        dst++;
    }
    *dst = 0;
    Text_Draw((char *)out);
}

/* Draws value in full-width digits followed by the money unit's character. */
void QuestMenu_DrawNumber(s32 value)
{
    u16 out[32];
    char buf[16];
    u16 *dst;
    char *src;
    char ascii;

    sprintf(buf, gHudFmtInt, value);
    dst = out;
    src = buf;
    ascii = *src;
    while (ascii != 0) {
        *dst = SJIS_WIDE_DIGIT(ascii);
        src++;
        ascii = *src;
        dst++;
    }
    dst[0] = SJIS_MONEY_UNIT;
    dst[1] = 0;
    Text_Draw((char *)out);
}

/* Draws n stars (a quest's difficulty). */
void Text_DrawStars(s32 count)
{
    u16 buf[16];
    u16 *dst = buf;

    while (count != 0) {
        *dst = SJIS_STAR;
        count--;
        dst++;
    }
    *dst = 0;
    Text_Draw((char *)buf);
}

extern char gHudFmtInt2[];

/* Draws a count in full-width characters: "count/max" with withMax, else
 * the count alone (two columns). */
void QuestMenu_DrawCount(u16 count, u16 max, u8 withMax)
{
    char buf[8];
    u16 out[12];
    u16 *dst;
    char *src;
    char ascii;

    if (withMax != 0) {
        sprintf(buf, "%2d/%d", count, max);
    } else {
        sprintf(buf, gHudFmtInt2, count);
    }
    dst = out;
    src = buf;
    ascii = *src;
    while (ascii != 0) {
        if (ascii == '/') {
            *dst = SJIS_WIDE_SLASH;
        } else if (ascii == ' ') {
            *dst = SJIS_WIDE_SPACE;
        } else {
            *dst = SJIS_WIDE_DIGIT(ascii);
        }
        src++;
        ascii = *src;
        dst++;
    }
    *dst = 0;
    Text_Draw((char *)out);
}

/* Prints the fort's remaining strength in percent (10 % for each stage
 * gScene.fortStage has not reached), in full-width digits. */
void QuestMenu_DrawRate(void)
{
    char buf[8];
    u16 out[12];
    u16 *dst;
    char *src;
    char ascii;

    sprintf(buf, gHudFmtInt, (10 - gScene.fortStage) * 10);
    dst = out;
    src = buf;
    ascii = *src;
    while (ascii != 0) {
        *dst = SJIS_WIDE_DIGIT(ascii);
        src++;
        ascii = *src;
        dst++;
    }
    *dst = 0;
    Text_PrintfColored(gQuestInfoFormats[3], out);
}

extern char gHudFmtFraction[];

/* Draws the faints so far: the party's total, or with a quest penalty
 * "faints/allowed", where the allowed count is how many penalties the
 * reward covers. */
void QuestMenu_DrawFaints(void)
{
    char out[24];
    char buf[8];
    Player *pl;
    s32 i;
    u8 total;
    u8 allowed;
    s32 left;

    pl = gPlayers;
    total = 0;
    for (i = 0; i < 4; i++, pl++) {
        if (pl->active != 0 && gScene.inQuest[i] == 1) {
            total += pl->faints;
        }
    }
    if (gQuestInfo->penalty == 0) {
        sprintf(buf, gHudFmtInt, total);
        Sjis_Widen(buf, out);
        Text_Draw(out);
    } else {
        left = gQuestInfo->reward;
        allowed = 1;
        while (left > gQuestInfo->penalty) {
            left -= gQuestInfo->penalty;
            allowed++;
        }
        if (allowed < total) {
            total = allowed;
        }
        sprintf(buf, gHudFmtFraction, total, allowed);
        Sjis_Widen(buf, out);
        Text_Draw(out);
    }
}




/* Draws the targets page at x: its title, the quest's goals and, outside
 * versus quests, the fort's strength (quest flag 0x10000000), the faints,
 * then the time left; each goal shown (goal flag 0x10) gets the count
 * still wanted while it runs (flags 3) and a done mark (goal flag 0x20). */
void QuestMenu_DrawTargets(s16 x)
{
    s16 left = x;
    s32 wanted;
    s32 i;
    s16 y;
    s32 textX;

    i = Quest_IsMustOrder();    /* i holds this until the loop below (the game's register use) */
    textX = left + 359;
    Font_SetPos(textX, 162);
    Text_PrintfStyled(42, gQuestTargetLabels[Quest_IsVersus() != 1 ? i == 1 : 2]);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(textX, 182);
    Text_Draw(QuestInfo_Get(1));
    if (Quest_IsVersus() != 1) {
        if (i != 1 || (gQuest.goals[1].flags & 0x10)) {
            Font_SetPos(textX, 242);
            Text_Draw(QuestInfo_Get(2));
        }
        if (i != 1 || (gQuest.goals[2].flags & 0x10)) {
            Font_SetPos(textX, 302);
            Text_Draw(QuestInfo_Get(3));
        }
        if (gQuest.flags & 0x10000000) {
            Font_SetPos(textX, 362);
            QuestMenu_DrawRate();
        }
        Font_SetPos(textX, 382);
        Text_Printf(gQuestInfoFormats[5]);
        Font_SetPos(left + 449, 382);
        QuestMenu_DrawFaints();
    }
    Font_SetPos(left + 449, 402);
    QuestMenu_DrawTime(0);
    if (Quest_IsVersus() != 1) {
        for (i = 0, y = 202; i < 3; i++, y += 60) {
            if (gQuest.goals[i].flags & 0x10) {
                if ((gQuest.goals[i].flags & 3) && !(gQuest.goals[i].flags & 0x20) && Quest_PhaseIsOver(0) != 1) {
                    Font_SetColor(0x807F5230);
                    Font_SetPos(left + 359, y);
                    Text_Draw(gQuestMenuStatusTexts[1]);
                    wanted = gQuest.goalsSet[i].count - gQuest.goals[i].count;
                    Font_SetColor(0x807F7F7F);
                    Font_SetPos(left + 485, y);
                    QuestMenu_DrawCount(wanted, 0, 0);
                }
                if (gQuest.goals[i].flags & 0x20) {
                    Font_SetColor(0x807F212E);
                    Font_SetPos(left + 359, y);
                    Text_Draw(gQuestMenuTargetTexts[0]);
                }
            }
        }
    }
}

/* Opens the chat menu (online, the field menu's phrase entry): its list of
 * CHAT_MENU_* entries. Returns 0. */
s32 ChatMenu_Open(void)
{
    gHudState->menuStep = 0;
    gHudState->listCursor = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_CHAT;
    gHelpLine = 0;
    return 0;
}

s32 PhraseMenu_Open(); /* unit's own declaration: PhraseMenu_Open ignores a0 (m), which holds &gHudState->listCursor at ChatMenu_Update's call */
u16 PhraseMenu_Update(u16 buttons);

/* The chat menu's input: in step 0 the cursor picks the chat log or the
 * phrases and confirming opens it; in step 1 its own input runs, and its
 * result PAD_START is passed up, PAD_CROSS goes back to the list. */
u16 ChatMenu_Update(u16 buttons)
{
    switch (gHudState->menuStep) {
    case 0:
        if (buttons & PAD_START) {
            FieldMenu_Close();
            return 0;
        }
        Menu_CursorUpDown(&gHudState->listCursor, buttons, CHAT_MENU_ENTRIES);
        gHelpLine = gHudState->listCursor;
        if (buttons & PAD_CIRCLE) {
            switch (gHudState->listCursor) {
            case CHAT_MENU_LOG:
                Chat_OpenWindow();
                break;
            case CHAT_MENU_PHRASES:
                PhraseMenu_Open();
                break;
            }
            (gHudState->menuStep)++;
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        }
        break;
    case 1:
        switch (gHudState->listCursor) {
        case CHAT_MENU_LOG:
            buttons = Chat_LogScroll(buttons);
            break;
        case CHAT_MENU_PHRASES:
            buttons = PhraseMenu_Update(buttons);
            break;
        }
        if (buttons & PAD_START) {
            return PAD_START;
        }
        if (buttons & PAD_CROSS) {
            gHudState->menuStep = 0;
            gHelpShow = 1;
            gHelpPage = HELP_PAGE_CHAT;
            gHelpLine = gHudState->listCursor;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        buttons = 0;
        break;
    }
    return buttons;
}

/* Draws the chat menu: its list over the dimmed field menu, or the chat
 * log or the phrase menu. */
void ChatMenu_Draw(void)
{
    u8 menuStep;
    u8 listCursor;

    menuStep = gHudState->menuStep;
    switch (menuStep) {
    case 0:
        FieldMenu_DrawList(1);
        ListPanel_Draw(&gChatMenuPanel, NULL, NULL, gHudState->listCursor);
        break;
    case 1:
        listCursor = gHudState->listCursor;
        switch (listCursor) {
        case CHAT_MENU_LOG:
            Chat_DrawLog();
            break;
        case CHAT_MENU_PHRASES:
            PhraseMenu_Draw();
        }
        break;
    }
}

/* Opens the phrase menu (the hunter's stored chat phrases) on the first
 * phrase, its window sliding in. Returns 0. */
s32 PhraseMenu_Open(void *menu)
{
    gHudState->phraseStep = 0;
    gHelpPage = HELP_PAGE_PHRASES;
    gHelpShow = 0;
    gHelpLine = 0;
    gPhraseMenuCursor = 0;
    Shake_Start(SHAKE_PHRASE_MENU, SHAKE_AXIS_X, 300.0f);
    return 0;
}

/* The phrase menu's input: in step 0 the cursor moves over the phrases
 * and confirming opens the keyboard to edit the one under it; in step 1
 * the keyboard runs until it closes. The menu button slides the window
 * out, and once it is out the result PAD_START closes the menu. */
u16 PhraseMenu_Update(u16 buttons)
{
    if (Shake_Flags(SHAKE_PHRASE_MENU) & 0x80) {
        if (Shake_Update(SHAKE_PHRASE_MENU) == 0) {
            return PAD_START;
        }
        return 0;
    }
    switch (gHudState->phraseStep) {
    case 0:
        if (buttons & PAD_START) {
            Shake_Start(SHAKE_PHRASE_MENU, SHAKE_SLIDE, 300.0f);
            gHelpShow = 0;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            return 0;
        }
        gHelpShow = 1;
        if (Shake_Update(SHAKE_PHRASE_MENU) == 1) {
            buttons &= (0xFFFF & ~PAD_CROSS);  /* no cancel while it slides */
            gHelpShow = 0;
        }
        if (!(buttons & PAD_CROSS)) {
            gHelpLine = 0;
            gPhraseMenuCursor = PhraseMenu_MoveCursor(buttons, gPhraseMenuCursor);
            if ((buttons & PAD_CIRCLE) && PhraseMenu_OpenKbd(gPhraseMenuCursor) == 1) {
                (gHudState->phraseStep)++;
            }
        }
        break;
    case 1:
        gHelpLine = 1;
        buttons &= (0xFFFF & ~(PAD_START | PAD_CROSS));      /* the keyboard takes cancel and the menu button */
        if (PhraseMenu_KbdUpdate(gPhraseMenuCursor) != 0) {
            gHudState->phraseStep = 0;
        }
        break;
    }
    return buttons;
}

extern ListPanel gPhraseMenuPanel;

/* Draws the phrase menu: the page of PHRASE_MENU_ROWS phrases the cursor
 * is on, each cut to 20 characters, with the cursor grey while the
 * keyboard is open. */
void PhraseMenu_Draw(void)
{
    s16 offset[2];
    char buf[44];
    s32 row;
    s32 i;
    s16 y;
    PhraseEntry *phrase;

    Shake_CopyOffset(SHAKE_PHRASE_MENU, (struct ShakeOffset *)offset);
    row = (s8)gPhraseMenuCursor % PHRASE_MENU_ROWS;
    sprintf(buf, gPhraseEditFmt[0], (s8)gPhraseMenuCursor / PHRASE_MENU_ROWS + 1);
    if (gHudState->phraseStep == 0) {
        gPhraseMenuPanel.cursorColor = 0xFF0A9182;
    } else {
        gPhraseMenuPanel.cursorColor = 0xFF707070;
    }
    ListPanel_Draw(&gPhraseMenuPanel, offset, buf, row);
    Menu_DrawFrame(&gPhraseMenuPanel, offset);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x807F7F7F);
    phrase = &gPhraseEntries[(s8)gPhraseMenuCursor / PHRASE_MENU_ROWS * PHRASE_MENU_ROWS];
    y = 84;
    i = PHRASE_MENU_ROWS;
    do {
        Font_SetPos(offset[0] + 431, y);
        Text_DrawTruncated(20, phrase->text);
        y += 22;
        phrase++;
    } while (--i != 0);
}

/* The field menu's pause entry (offline): pauses the game while the quest
 * runs and asks for the pause window. Returns 0, or 1 (refused) online,
 * after the quest or when already paused. */
s32 PauseMenu_Open(void)
{
    if ((Game_IsOnline() != 1) && (Quest_PhaseIsOver(0) != 1) && (Game_PauseState() == 0)) {
        gHelpShow = 0;
        gPauseMenuReq = 1;
        Game_Pause(1U);
        return 0;
    }
    return 1;
}

/* The pause entry's input: confirm, cancel or the menu button resume the
 * game and close the menu (PAD_START). */
u16 PauseMenu_Update(u16 buttons)
{
    if (buttons & (PAD_START | PAD_CROSS | PAD_CIRCLE)) {
        Game_Resume(1);
        return PAD_START;
    }
    return 0;
}

/* Steps the pause window's fade (gPauseWinFade, 4 hidden to 0 shown): step
 * 0 waits for a requested pause to take hold, step 1 fades in and waits
 * for the game to resume, step 2 fades out and falls through to 0. */
void Pause_Update(void)
{
    switch (gPauseWinStep) {
    case 2:
        gPauseWinFade += 1;
        if ((s32) gPauseWinFade > 4) {
            gPauseWinStep = 0;
            gPauseMenuReq = 0;
        }
        
    case 0:
        if ((gPauseMenuReq != 0) && (Game_PauseState() >= 2)) {
            gPauseWinStep = 1;
            gPauseWinFade = 4;
        }
        break;
    case 1:
        if (Game_PauseState() == 0) {
            gPauseWinFade = 0;
            gPauseWinStep += 1;
            break;
        }
        if (gPauseWinFade != 0) {
            gPauseWinFade -= 1;
        }
        break;
    }
}

/* The pause entry draws nothing here (Pause_DrawWindow draws the window). */
void PauseMenu_Draw(void)
{
}

/* The pouch menu steps back one state, with the slot's item help line. */
void PouchMenu_Back(void)
{
    gHudState->state = (u8) (gHudState->state - 1);
    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
    gHelpLine = HELP_ITEM_LINE(gHudState->player->pouch[gHudState->slot].item);
}

/* Adjusts the count to give or drop (HudState.count, from 1 to the slot's
 * count; atMax is set at the top): left and right by 1, up and down by 10. */
void Menu_AdjustCount(u16 buttons)
{
    Player *pl = gHudState->player;
    s16 newCount = 0;
    s16 max;

    if (buttons & PAD_LEFT) {
        newCount--;
    }
    if (buttons & PAD_RIGHT) {
        newCount++;
    }
    if (buttons & PAD_UP) {
        newCount += 10;
    }
    if (buttons & PAD_DOWN) {
        newCount -= 10;
    }
    if (newCount != 0) {
        gHudState->atMax = 0;
        newCount += gHudState->count;
        if (newCount <= 0) {
            newCount = 1;
        }
        max = pl->pouch[gHudState->slot].count;
        if (newCount >= max) {
            newCount = max;
            gHudState->atMax = 1;
        }
        if (gHudState->count != newCount) {
            gHudState->count = newCount;
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
        }
    }
}

s32 Area_HasTownPcs(u16 a);

/* Returns 1 if the selected pouch slot's item may be given to another
 * hunter: online only, not items of rarity 3 or more or without a count
 * limit (max ITEM_MAX_UNLIMITED), and in town only where other hunters are shown
 * (Area_HasTownPcs).
 * pl is unused: it reads gHudState's player. */
s32 PouchMenu_CanTransfer(Player *pl)
{
    u16 item;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    item = (gHudState->player)->pouch[gHudState->slot].item;
    if (gItemData[item].rarity >= 3) {
        return 0;
    }
    if (gItemData[item].max == 255) {
        return 0;
    }
    if (gScene.inLobby != 0) {
        return Area_HasTownPcs(gScene.area);
    }
    return 1;
}

/* Opens the pouch menu (the field menu's entry) on the first slot, the
 * give target on the first other player, its window sliding in. Returns 0. */
s32 PouchMenu_Open(void *menu)
{
    gHudState->state = 0;
    gHudState->slot = 0;
    *(s16 *)&gHudState->action = 0;     /* action and step, with one store */
    gHudState->target = 0;
    if (gScene.localPlayer == 0) {
        gHudState->target = 1;
    }
    gHudState->sorting = 0;
    gHelpShow = 0;
    gHelpPage = HELP_PAGE_ITEMS;
    gHelpLine = HELP_ITEM_LINE((gHudState->player)->pouch[gHudState->slot].item);
    gHelpColumn = 0;
    Shake_Start(SHAKE_POUCH_MENU, SHAKE_AXIS_X, 180.0f);
    return 0;
}

/* The pouch menu, driven by buttons: pick a slot (or sort the pouch), pick drop, give or move,
 * then do it; giving offers the item to another player (Player.offerItem) and waits in state 3.
 * Returns the buttons left unused, PAD_START once the menu has closed. */
u16 PouchMenu_Update(u16 buttons)
{
    Player *pl = gHudState->player;
    s32 ok;
    s32 last;
    ItemStack stack;

    if (gHudState->state != 4 && (buttons & PAD_START)) {
        Shake_Start(SHAKE_POUCH_MENU, SHAKE_SLIDE, 180.0f);
        gHudState->state = 4;
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
    }
    gHelpShow = 1;
    switch (gHudState->state) {
    case 0:
        if (Shake_Update(SHAKE_POUCH_MENU) == 1) {
            gHelpShow = 0;
            buttons &= (PAD_DPAD | PAD_CROSS);
        }
        gHelpLine = 6;
        if (gHudState->sorting != 0) {
            if (gHudState->yesNo == 0) {
                if (buttons & PAD_CIRCLE) {
                    Pouch_Compact(0);
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    gHudState->slot = gHudState->slot % 10;
                    gHudState->sorting = 0;
                } else {
                    if (buttons & PAD_CROSS) {
                        gHudState->yesNo = 1;
                        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                    } else if (buttons & PAD_RIGHT) {
                        gHudState->yesNo = 1;
                        Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    }
                    buttons = 0;
                    break;
                }
            } else {
                if (buttons & (PAD_CROSS | PAD_CIRCLE)) {
                    gHudState->sorting = 0;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else {
                    if (buttons & PAD_LEFT) {
                        gHudState->yesNo = 0;
                        Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    }
                    buttons = 0;
                    break;
                }
            }
            buttons = 0;
        } else {
            if (buttons & PAD_CROSS) {
                break;
            }
            if (buttons & PAD_SELECT) {
                gHudState->sorting = 1;
                buttons = 0;
                gHudState->yesNo = 0;
                Sound_PlaySeFlat(0, 15, 0);
                break;
            }
        }
        gHelpColumn &= 3;
        PouchMenu_MoveCursor(&gHudState->slot, buttons);
        gHelpColumn |= 0x10;
        if (buttons & PAD_CIRCLE) {
            if (pl->pouch[gHudState->slot].item != 0) {
                gHudState->state++;
                Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
            } else {
                Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
            }
        }
        break;
    case 1:
        if (pl->pouch[gHudState->slot].item == 0) {
            gHudState->state = 0;
            buttons &= ~PAD_CROSS;
            break;
        }
        Menu_CursorUpDown(&gHudState->action, buttons, 3);
        gHelpColumn &= 3;
        PouchMenu_DecoCursor(pl->pouch[gHudState->slot].item, buttons & PAD_R1);
        gHelpColumn |= 0x10;
        if (buttons & PAD_CROSS) {
            buttons &= ~PAD_CROSS;
            gHudState->state--;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        } else if (buttons & PAD_CIRCLE) {
            if (Player_IsFree(pl) == 0) {
                Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
            } else {
                gHudState->cancel = 0;
                switch (gHudState->action) {
                case 0:
                    gHudState->yesNo = 1;
                    gHudState->state++;
                    gHudState->step = 0;
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    break;
                case 1:
                    if (PouchMenu_CanTransfer(pl) == 1) {
                        gHudState->count = 1;
                        gHudState->state++;
                        if (pl->pouch[gHudState->slot].count != 1) {
                            gHudState->atMax = 0;
                            gHudState->step = 0;
                        } else {
                            gHudState->step = 1;
                        }
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                    break;
                case 2:
                    gHudState->state++;
                    gHudState->dest = gHudState->slot;
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    break;
                }
            }
        }
        break;
    case 2:
        if (pl->pouch[gHudState->slot].item == 0) {
            gHudState->state = 0;
            buttons = 0;
            gHelpLine = HELP_ITEM_LINE(gHudState->player->pouch[gHudState->slot].item);
        } else if (gHudState->cancel != 0) {
            gHudState->state = 1;
            buttons = 0;
            gHelpLine = HELP_ITEM_LINE(gHudState->player->pouch[gHudState->slot].item);
        } else if (Player_IsFree(pl) == 0) {
            buttons = 0;
            gHudState->state = 1;
            gHelpLine = HELP_ITEM_LINE(gHudState->player->pouch[gHudState->slot].item);
        } else {
            switch (gHudState->action) {
            case 0:
                if (buttons & PAD_CROSS) {
                    buttons &= ~PAD_CROSS;
                    PouchMenu_Back();
                } else {
                    gHelpLine = 0;
                    gHudState->yesNo = YesNo_Toggle(buttons, gHudState->yesNo, 12288);
                    if (buttons & PAD_CIRCLE) {
                        if (gHudState->yesNo == 0) {
                            Pouch_ClearSlot(pl, gHudState->slot);
                            gHudState->state = 0;
                            gHudState->action = 0;
                            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        } else {
                            PouchMenu_Back();
                        }
                    }
                }
                break;
            case 1:
                switch (gHudState->step) {
                case 0:
                    if (buttons & PAD_CROSS) {
                        buttons &= ~PAD_CROSS;
                        PouchMenu_Back();
                    } else {
                        gHelpLine = 3;
                        Menu_AdjustCount(buttons);
                        if (buttons & PAD_CIRCLE) {
                            gHelpLine = 1;
                            gHudState->step++;
                            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        }
                    }
                    break;
                case 1:
                    if (buttons & PAD_CROSS) {
                        buttons &= ~PAD_CROSS;
                        if (pl->pouch[gHudState->slot].count != 1 &&
                            gItemData[pl->pouch[gHudState->slot].item].max != 255) {
                            gHudState->step = 0;
                            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                        } else {
                            PouchMenu_Back();
                        }
                    } else {
                        gHelpLine = 1;
                        last = gScene.inLobby == 0 ? 3 : 9;
                        if (buttons & PAD_UP) {
                            gHudState->target--;
                            if (gHudState->target < 0) {
                                gHudState->target = last;
                            }
                            if (gHudState->target == gScene.localPlayer) {
                                gHudState->target--;
                            }
                            if (gHudState->target < 0) {
                                gHudState->target = last;
                            }
                            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                        } else if (buttons & PAD_DOWN) {
                            gHudState->target++;
                            if (gHudState->target > last) {
                                gHudState->target = 0;
                            }
                            if (gHudState->target == gScene.localPlayer) {
                                gHudState->target++;
                            }
                            if (gHudState->target > last) {
                                gHudState->target = 0;
                            }
                            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                        }
                        if (buttons & PAD_CIRCLE) {
                            if (gScene.inLobby == 0) {
                                ok = gScene.inQuest[gHudState->target] == 1;
                            } else {
                                switch (NetUser_State(gHudState->target)) {
                                case 0:
                                    ok = 1;
                                    break;
                                case 1:
                                case 2:
                                    ok = 0;
                                    break;
                                }
                            }
                            if (ok == 1) {
                                buttons = 0;
                                pl->offerTo = gHudState->target;
                                pl->offerItem = pl->pouch[gHudState->slot].item;
                                pl->offerCount = gHudState->count;
                                pl->offerRequest = 1;
                                gHudState->state = 3;
                            } else {
                                Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                            }
                        }
                    }
                    break;
                }
                break;
            case 2:
                if (buttons & PAD_CROSS) {
                    buttons &= ~PAD_CROSS;
                    PouchMenu_Back();
                } else {
                    gHelpLine = 7;
                    Pouch_MoveCursor(&gHudState->dest, buttons);
                    if (buttons & PAD_CIRCLE) {
                        if (gHudState->slot != gHudState->dest) {
                            stack = pl->pouch[gHudState->slot];
                            pl->pouch[gHudState->slot] = pl->pouch[gHudState->dest];
                            pl->pouch[gHudState->dest] = stack;
                            if (Scene_InField() == 1) {
                                if (pl->ammoSlot == gHudState->slot) {
                                    pl->ammoSlot = gHudState->dest;
                                } else if (pl->ammoSlot == gHudState->dest) {
                                    pl->ammoSlot = gHudState->slot;
                                }
                                if (pl->coatingSlot == gHudState->slot) {
                                    pl->coatingSlot = gHudState->dest;
                                } else if (pl->coatingSlot == gHudState->dest) {
                                    pl->coatingSlot = gHudState->slot;
                                }
                            }
                            gHudState->slot = gHudState->dest;
                            gHudState->state = 0;
                            gHelpLine = HELP_ITEM_LINE(gHudState->player->pouch[gHudState->slot].item);
                            Sound_PlaySeFlat(1, 139, 0);
                        } else {
                            Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                        }
                    }
                }
                break;
            default:
                gHudState->state = 0;
                break;
            }
        }
        break;
    case 3:
        if (pl->offerStarted != 0) {
            if (gScene.inLobby == 0) {
                FieldMenu_Exit();
            } else {
                GameMenu_Clear();
            }
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        } else {
            gHudState->state = 2;
            gHudState->action = 1;
            gHudState->step = 1;
            Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
        }
        buttons = 0;
        break;
    case 4:
        if (Shake_Update(SHAKE_POUCH_MENU) == 0) {
            return PAD_START;
        }
        gHelpShow = 0;
        buttons = 0;
        break;
    }
    return buttons;
}

/* Returns 1 if pouch slot `slot` is not the one under the pouch menu's
 * cursor. HudState.slot is read at its offset 0x51: the field access
 * allocates the registers the other way. */
s32 Menu_IsNotCursorSlot(s32 slot)
{
    return (slot & 0xFF) != *(u8 *)((u8 *)gHudState + 81);
}

/* Returns 1 if pouch slot `slot` still holds `item` and the item can start
 * a combine: one with a recipe, or for a three-item combine one usable in
 * any (Combine_ItemUsable). */
s32 CombineMenu_CheckFirst(u8 slot, u16 item)
{
    if (slot == 255) {
        return 0;
    }
    if ((gHudState->player)->pouch[slot].item != item) {
        return 0;
    }
    if (gHudState->combineThree == 0) {
        if (Combine_ItemHasRecipe(item) == 0) {
            return 0;
        }
    } else if (Combine_ItemUsable(item) == 0) {
        return 0;
    }
    return 1;
}

/* Returns 1 if the items of pouch slots slotA and slotB (two different
 * slots) combine: for a two-item combine it sets combineRecipe and the
 * result shown (combineResult, -1 while the recipe is unknown); for a
 * three-item one it only checks the pair can be part of one. */
s32 CombineMenu_CheckPair(u8 slotA, u8 slotB)
{
    Player *pl = gHudState->player;

    if (slotA == 255) {
        return 0;
    }
    if (slotB == slotA) {
        return 0;
    }
    if (gHudState->combineThree == 0) {
        gHudState->combineRecipe = Combine_FindRecipe(pl->pouch[slotB].item, pl->pouch[slotA].item);
        if (gHudState->combineRecipe == NULL) {
            return 0;
        }
        if (Recipe_IsKnown(gHudState->combineRecipe) != 0) {
            gHudState->combineResult = gHudState->combineRecipe->result;
        } else {
            gHudState->combineResult = -1;
        }
    } else if (Combine_CanUsePair(pl->pouch[slotB].item, pl->pouch[slotA].item) == 0) {
        return 0;
    }
    return 1;
}

Recipe *Combine_FindRecipe3(s16 a, s16 b, s16 c);

/* Returns 1 if the items of three different pouch slots make a three-item
 * recipe, and sets combineRecipe and combineResult as CombineMenu_CheckPair. */
s32 CombineMenu_CheckTriple(u8 slotA, u8 slotB, u8 slotC)
{
    Player *pl = gHudState->player;

    if (slotB == 255) {
        return 0;
    }
    if (slotA == 255) {
        return 0;
    }
    if (slotC == slotB) {
        return 0;
    }
    if (slotC == slotA) {
        return 0;
    }
    if (slotB == slotA) {
        return 0;
    }
    gHudState->combineRecipe = Combine_FindRecipe3(pl->pouch[slotC].item, pl->pouch[slotB].item, pl->pouch[slotA].item);
    if (gHudState->combineRecipe == NULL) {
        return 0;
    }
    if (Recipe_IsKnown(gHudState->combineRecipe) != 0) {
        gHudState->combineResult = gHudState->combineRecipe->result;
    } else {
        gHudState->combineResult = -1;
    }
    return 1;
}

/* Clears the combine menu's choice: no ingredients, recipe or result. */
void CombineMenu_Reset(void)
{
    gHudState->combineSteps = 0;
    gHudState->slot = 0;
    gHudState->combineThree = 0;
    gHudState->combineConfirm = 0;
    gHudState->combineSlots[2] = 255;
    gHudState->combineSlots[1] = 255;
    gHudState->combineSlots[0] = 255;
    gHudState->combineItems[2] = 65535;
    gHudState->combineItems[1] = 65535;
    gHudState->combineItems[0] = 65535;
    gHudState->combineResult = -1;
    gHudState->combineRecipe = NULL;
    gHudState->fadeActive = 0;
}

/* Returns 1 if the item menus may open: in a quest while the player is free
 * (Player_IsFree), in the town while no quest is taken. The goto is the
 * game's code. */
s32 Hud_ItemMenusAllowed(void)
{
    Player *pl;

    pl = gHudState->player;
    if (gScene.inLobby == 0) {
        if (Player_IsFree(pl) != 1) {
            return 0;
        }
        goto block_6;
    }
    if (Lobby_LacksQuest() == 0) {
        return 0;
    }
block_6:
    return 1;
}

/* Returns 1 if pouch slot `slot` is one of the ingredients picked. */
s32 CombineMenu_IsPicked(s32 slotArg)
{
    s32 slot = slotArg & 0xFFFF;
    HudState *hud = gHudState;

    if (slot == hud->combineSlots[0] || slot == hud->combineSlots[1]) {
        return 1;
    }
    if (hud->combineThree != 0 && slot == hud->combineSlots[2]) {
        return 1;
    }
    return 0;
}

/* For a pouch stack of the known recipe's result: returns 0 if the stack
 * is full, 1 if it has room for fewer than the recipe makes, 2 if for all.
 * -1 for another item or an unknown recipe. */
s32 CombineMenu_ResultRoom(ItemStack *stack)
{
    s32 room;
    s32 ret;
    Recipe *recipe;

    recipe = gHudState->combineRecipe;
    if (recipe != NULL) {
        if (stack->item != recipe->result) {
            return -1;
        }
        if (Recipe_IsKnown(recipe) == 0) {
            return -1;
        }
        room = gItemData[stack->item].max - stack->count;
        if (room <= 0) {
            return 0;
        }
        if (room < (Recipe_YieldBase(gHudState->combineRecipe) & 0xFF)) {
            ret = 1;
        } else {
            ret = 2;
        }
        return ret;
    }
    return -1;
}

/* Returns 1 once every ingredient the combine needs is picked. */
s32 CombineMenu_IsReady(void)
{
    HudState *hud = gHudState;

    if (hud->combineItems[0] == 0xFFFF || hud->combineItems[1] == 0xFFFF) {
        return 0;
    }
    if (hud->combineThree != 0 && hud->combineItems[2] == 0xFFFF) {
        return 0;
    }
    return 1;
}

/* Returns 1 if the result and one of the ingredients are special items
 * (Item_IsOnePerPouch). The goto is the game's code. */
s32 CombineMenu_HasSpecial(void)
{
    if (Item_IsOnePerPouch((u16)gHudState->combineResult) == 1) {
        if (Item_IsOnePerPouch(gHudState->combineItems[0]) != 0) {
            return 1;
        }
        if (Item_IsOnePerPouch(gHudState->combineItems[1]) != 0) {
            return 1;
        }
        if ((gHudState->combineThree != 0) && (Item_IsOnePerPouch(gHudState->combineItems[2]) != 0)) {
            return 1;
        }
        goto block_9;
    }
block_9:
    return 0;
}

/* Returns whether the hunter has made the chosen ingredients' recipe. */
s32 CombineMenu_IsKnown(void)
{
    if (gHudState->combineThree == 0) {
        return Combine_IsKnown((s16)gHudState->combineItems[0], (s16)gHudState->combineItems[1]);
    }
    return Combine_IsKnown3((s32) (s16)gHudState->combineItems[0], (s32) (s16)gHudState->combineItems[1], (s32) (s16)gHudState->combineItems[2]);
}

void Lobby_ClearConnected(void);

/* Opens the combine menu (the field menu's entry): refused (1) when the
 * item menus are not allowed or an item get is in progress; else it takes
 * an item-get ticket, clears the choice and slides the window in (0). */
s32 CombineMenu_Open(void)
{
    if (Hud_ItemMenusAllowed() != 1) {
        return 1;
    }
    gHudState->combineTicket = -1;
    if ((s16)ItemGet_Reserve(0, (u16 *)&gHudState->combineTicket) < 0) {
        return 1;
    }
    if (gScene.inLobby != 0) {
        Lobby_ClearConnected();
    }
    CombineMenu_Reset();
    gHudState->sorting = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_ITEMS;
    gHelpLine = gHudState->combineThree + 14;
    gHelpColumn = 0;
    Shake_Start(SHAKE_POUCH_MENU, SHAKE_AXIS_X, 180.0f);
    return 0;
}

#pragma rodata on
char gHudFmtInt2[4] = {
#include "sub_main/0046CFF0.inc"
};
char gHudFmtFraction[6] = {
#include "sub_main/0046CFF8.inc"
};
#pragma rodata off

/* The combine menu's input step: the ingredient picks (state 1), the confirmation (2), the
   result (3-5) and the checks after it (6); returns the buttons left for the caller. */
u16 CombineMenu_Input(u16 btn)
{
    Player *pl = gHudState->player;
    u16 ret;
    u16 pressed;

    if ((gHudState->combineState < 3 || gHudState->combineState == 5) && (btn & PAD_START)) {
        Shake_Start(SHAKE_POUCH_MENU, SHAKE_SLIDE, 180.0f);
        gHudState->combineState = 7;
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
    }
    pressed = btn;
    ret = pressed & ~PAD_START;
    if (gHudState->slot >= 20) {
        gHudState->slot = 0;
    }
    gHelpShow = 1;
    switch (gHudState->combineState) {
    case 0:
        if (Shake_Update(SHAKE_POUCH_MENU) == 1) {
            gHelpShow = 0;
            btn &= (PAD_DPAD | PAD_CROSS);
        }
        gHelpLine = 6;
        if (gHudState->sorting != 0) {
            ret &= ~PAD_CROSS;
            if (gHudState->yesNo == 0) {
                if (btn & PAD_CIRCLE) {
                    Pouch_Compact(0);
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    gHudState->slot = gHudState->slot % 10;
                    gHudState->sorting = 0;
                } else {
                    if (btn & PAD_CROSS) {
                        gHudState->yesNo = 1;
                        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                    } else if (btn & PAD_RIGHT) {
                        gHudState->yesNo = 1;
                        Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    }
                    break;
                }
            } else {
                if (btn & (PAD_CROSS | PAD_CIRCLE)) {
                    gHudState->sorting = 0;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else {
                    if (btn & PAD_LEFT) {
                        gHudState->yesNo = 0;
                        Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    }
                    break;
                }
            }
            btn = 0;
        } else if (btn & PAD_CROSS) {
            break;
        } else if (btn & PAD_SELECT) {
            gHudState->sorting = 1;
            gHudState->yesNo = 0;
            Sound_PlaySeFlat(0, 15, 0);
            break;
        }
        Menu_CursorUpDown(&gHudState->combineThree, btn, 2);
        pressed = btn;
        gHelpLine = gHudState->combineThree + 14;
        Pouch_MoveCursor(&gHudState->slot, pressed & (PAD_LEFT | PAD_RIGHT));
        if (pressed & PAD_CIRCLE) {
            gHudState->combineState++;
            gHudState->combinePick = 0;
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        }
        break;
    case 1:
        ret &= ~PAD_CROSS;
        switch (gHudState->combinePick) {
        case 0:
            Pouch_MoveCursor(&gHudState->slot, btn);
            if (pressed & PAD_CROSS) {
                gHudState->combineState = 0;
                gHelpLine = gHudState->combineThree + 14;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            } else if (pressed & PAD_CIRCLE) {
                if (CombineMenu_CheckFirst(gHudState->slot, pl->pouch[gHudState->slot].item) == 1) {
                    gHudState->combineSlots[0] = gHudState->slot;
                    gHudState->combineItems[0] = pl->pouch[gHudState->combineSlots[0]].item;
                    gHudState->combineItems[1] = 65535;
                    gHudState->combinePick++;
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                } else {
                    Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                }
            }
            break;
        case 1:
            if (CombineMenu_CheckFirst(gHudState->combineSlots[0], gHudState->combineItems[0]) == 1) {
                Pouch_MoveCursor(&gHudState->slot, btn);
                if (pressed & PAD_CROSS) {
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else {
                    if (pressed & PAD_CIRCLE) {
                        if (CombineMenu_CheckPair(gHudState->slot, gHudState->combineSlots[0]) == 1) {
                            gHudState->combineSlots[1] = gHudState->slot;
                            gHudState->combineItems[1] = pl->pouch[gHudState->combineSlots[1]].item;
                            gHudState->combineConfirm = 0;
                            if (gHudState->combineThree == 0) {
                                gHudState->combineState++;
                            } else {
                                gHudState->combineItems[2] = 65535;
                            }
                            gHudState->combinePick++;
                            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        } else {
                            Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                        }
                    }
                    break;
                }
            }
            gHudState->combineSlots[0] = 255;
            gHudState->combineItems[0] = 65535;
            gHudState->combinePick = 0;
            break;
        case 2:
            if (CombineMenu_CheckFirst(gHudState->combineSlots[0], gHudState->combineItems[0]) != 1) {
                gHudState->combineSlots[0] = 255;
                gHudState->combineItems[0] = 65535;
                gHudState->combinePick = 0;
                break;
            }
            if (CombineMenu_CheckFirst(gHudState->combineSlots[1], gHudState->combineItems[1]) == 1) {
                Pouch_MoveCursor(&gHudState->slot, btn);
                if (pressed & PAD_CROSS) {
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else {
                    if (pressed & PAD_CIRCLE) {
                        if (CombineMenu_CheckTriple(gHudState->slot, gHudState->combineSlots[0], gHudState->combineSlots[1]) == 1) {
                            gHudState->combineSlots[2] = gHudState->slot;
                            gHudState->combineItems[2] = pl->pouch[gHudState->combineSlots[2]].item;
                            gHudState->combineState++;
                            gHudState->combinePick++;
                            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        } else {
                            Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                        }
                    }
                    break;
                }
            }
            gHudState->combineSlots[1] = 255;
            gHudState->combineItems[1] = 65535;
            gHudState->combinePick = 1;
            break;
        }
        break;
    case 2:
        gHudState->combineError = 0;
        gHudState->combineKnown = 0;
        if (CombineMenu_CheckFirst(gHudState->combineSlots[0], gHudState->combineItems[0]) == 0) {
            gHudState->combineSlots[0] = Pouch_FindSlot(pl, gHudState->combineItems[0]);
            if (gHudState->combineSlots[0] == 255) {
                gHudState->combineError |= 4;
            }
        }
        if (CombineMenu_CheckFirst(gHudState->combineSlots[1], gHudState->combineItems[1]) == 0) {
            gHudState->combineSlots[1] = Pouch_FindSlot(pl, gHudState->combineItems[1]);
            if (gHudState->combineSlots[1] == 255) {
                gHudState->combineError |= 4;
            }
        }
        if (gHudState->combineThree != 0 && CombineMenu_CheckFirst(gHudState->combineSlots[2], gHudState->combineItems[2]) == 0) {
            gHudState->combineSlots[2] = Pouch_FindSlot(pl, gHudState->combineItems[2]);
            if (gHudState->combineSlots[2] == 255) {
                gHudState->combineError |= 4;
            }
        }
        if (gHudState->combineError == 0) {
            if (CombineMenu_IsKnown() != 0) {
                s16 n = Pouch_Space(pl, gHudState->combineResult);

                if (n == 0) {
                    if (CombineMenu_HasSpecial() == 0) {
                        gHudState->combineError |= 1;
                        gHelpLine = 12;
                    }
                } else if (n < 0) {
                    if (pl->pouch[gHudState->combineSlots[0]].count > 1 && pl->pouch[gHudState->combineSlots[1]].count > 1) {
                        gHudState->combineError |= 2;
                        gHelpLine = 13;
                    }
                }
                gHudState->combineKnown = 1;
            }
        } else {
            gHelpLine = 11;
        }
        if (gHudState->combineError == 0) {
            if (gHudState->combineResult == -1) {
                gHelpLine = 10;
            } else {
                gHelpLine = HELP_ITEM_LINE(gHudState->combineResult);
            }
        } else {
            gHudState->combineConfirm = 1;
            btn &= (0xFFFF & ~(PAD_UP | PAD_DOWN));
        }
        pressed = btn;
        if (pressed & PAD_CIRCLE) {
            ret &= (0xFFFF & ~(PAD_START | PAD_CROSS));
            switch (gHudState->combineConfirm) {
            case 0:
                if (gHudState->combineError == 0) {
                    if (gHudState->combineKnown != 0) {
                        gHudState->combineTimer = 1;
                    } else {
                        gHudState->combineTimer = 30;
                        CombineMenu_StartFade(0);
                    }
                    gHudState->combineState++;
                    if (gHudState->combineThree == 0) {
                        gHudState->combineMadeItem = Combine_Do(&gHudState->combineMade, pl, pl->pouch[gHudState->combineSlots[0]].item, pl->pouch[gHudState->combineSlots[1]].item, 0);
                    } else {
                        gHudState->combineMadeItem = Combine_Do3(&gHudState->combineMade, pl, (s16)pl->pouch[gHudState->combineSlots[0]].item, (s16)pl->pouch[gHudState->combineSlots[1]].item, (s16)pl->pouch[gHudState->combineSlots[2]].item, 0);
                    }
                    gHudState->combineTaken = 1;
                } else {
                    Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                }
                break;
            case 1:
                CombineMenu_Reset();
                return 64;
            }
        } else if (pressed & PAD_START) {
            CombineMenu_Reset();
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            return PAD_START;
        } else if (pressed & PAD_CROSS) {
            ret &= ~PAD_CROSS;
            if (gHudState->combineSlots[1] < 20) {
                gHudState->slot = gHudState->combineSlots[gHudState->combineThree + 1];
            } else {
                gHudState->slot = 0;
            }
            gHudState->combineResult = -1;
            gHudState->combineState = 1;
            if (gHudState->combineThree == 0) {
                gHudState->combineSlots[1] = 255;
                gHudState->combineItems[1] = 65535;
                gHudState->combinePick = 1;
            } else {
                gHudState->combineSlots[2] = 255;
                gHudState->combineItems[2] = 65535;
                gHudState->combinePick = 2;
            }
            gHelpLine = gHudState->combineThree + 14;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        } else {
            Menu_CursorUpDown(&gHudState->combineConfirm, btn, 2);
            Pouch_MoveCursor(&gHudState->slot, pressed & (PAD_LEFT | PAD_RIGHT));
        }
        break;
    case 3:
        ret &= (0xFFFF & ~(PAD_START | PAD_CROSS));
        gHudState->combineTimer--;
        if (gHudState->combineTimer > 0) {
            break;
        }
        if (gHudState->combineMadeItem > 0) {
            Recipe_SetKnown(gHudState->combineRecipe, &gHudState->combineMadeItem);
            gHudState->combineResult = gHudState->combineMadeItem;
            CombineMenu_StartFade(1);
            Recipe_CheckAllKnown();
        } else {
            gHudState->combineMadeItem = 174;
            gHudState->combineMade = 1;
            CombineMenu_StartFade(2);
        }
        if (pl->pouch[gHudState->combineSlots[0]].item == 0) {
            gHudState->combineSlots[0] = 255;
        }
        if (pl->pouch[gHudState->combineSlots[1]].item == 0) {
            gHudState->combineSlots[1] = 255;
        }
        if (gHudState->combineThree != 0 && pl->pouch[gHudState->combineSlots[2]].item == 0) {
            gHudState->combineSlots[2] = 255;
        }
        gHelpLine = HELP_ITEM_LINE(gHudState->combineMadeItem);
        gHudState->combineState++;
    case 4:
        ret &= (0xFFFF & ~(PAD_START | PAD_CROSS));
        switch (Pouch_GiveItems(pl, gHudState->combineMadeItem, gHudState->combineTicket, 0, gHudState->combineMade)) {
        case -1:
            break;
        case 0:
        case 1:
            if (gHudState->combineKnown == 0) {
                if (gHudState->combineMadeItem != 174) {
                    Item_PlayUseSound(pl, gHudState->combineMadeItem);
                } else {
                    Jingle_Play(pl, 10);
                }
            }
            if (gHudState->combineKnown != 0) {
                gHudState->combineTimer = 1;
            } else {
                gHudState->combineTimer = 30;
            }
            gHudState->combineState++;
            gHudState->combineTaken = 0;
            break;
        case 2:
        case 3:
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_POUCH_FULL, gHudState->combineMadeItem);
            if (gHudState->combineKnown != 0) {
                gHudState->combineTimer = 1;
            } else {
                gHudState->combineTimer = 30;
            }
            gHudState->combineState++;
            gHudState->combineTaken = 0;
            break;
        case 5:
            gHudState->slot = 0;
            gHudState->combineState = 6;
            gHudState->combineTaken = 0;
            break;
        }
        break;
    case 5:
        ret &= ~PAD_CROSS;
        if (gHudState->combineTimer != 0) {
            gHudState->combineTimer--;
        } else if (pressed & (0xFFFF & ~(PAD_START | PAD_SELECT))) {
            gHudState->combineState = 2;
            Sound_PlaySeFlat(0, 9, 0);
        }
        break;
    case 6:
        ret &= ~PAD_CROSS;
        if (gHudState->dropState != DROP_NONE) {
            break;
        }
        if (gHudState->combineItems[0] != pl->pouch[gHudState->combineSlots[0]].item) {
            gHudState->combineSlots[0] = 255;
        } else if (gHudState->combineItems[1] != pl->pouch[gHudState->combineSlots[1]].item) {
            gHudState->combineSlots[1] = 255;
        } else if (gHudState->combineThree != 0 && gHudState->combineItems[2] != pl->pouch[gHudState->combineSlots[2]].item) {
            gHudState->combineSlots[2] = 255;
        }
        gHudState->combineState = 2;
        if (gHudState->combineThree == 0) {
            if (gHudState->combineSlots[0] != 255 && gHudState->combineSlots[1] != 255 && Combine_FindRecipe(gHudState->combineItems[0], gHudState->combineItems[1]) == NULL) {
                CombineMenu_Reset();
                gHudState->combineState++;
            }
        } else if (gHudState->combineSlots[0] != 255 && gHudState->combineSlots[1] != 255 && gHudState->combineSlots[2] != 255
                   && Combine_FindRecipe3(gHudState->combineItems[0], gHudState->combineItems[1], gHudState->combineItems[2]) == NULL) {
            CombineMenu_Reset();
            gHudState->combineState++;
            gHudState->combineThree = 1;
        }
        break;
    case 7:
        if (Shake_Update(SHAKE_POUCH_MENU) == 0) {
            return PAD_START;
        }
        gHelpShow = 0;
        ret = 0;
        break;
    }
    return ret;
}

typedef struct RecipeView {
    s16 item;
    s16 result;         /* 0x02 */
} RecipeView;

extern ListPanel gCombineKindPanel;
extern ListPanel gCombineMaterialPanel;
extern Panel gCombineResultWindow;
extern ListPanel gCombineDoPanel;
extern u32 D_004321C4[]; /* gCombineDoPanel's text colour as one RGBA word (textColor, unk16, rgbText) */
extern char *gCombineUnknownText;
extern char *gCombineRateUnknownText;
extern char *gCombineSuccessLabels;

/* Draws the combine menu's info panel for the state at +0x88: the ingredient list, the chosen
   recipe's result, success rate and yield, or (states 5 and 6) the result item's counts. */
void CombineMenu_DrawInfo(void)
{
    Player *pl = gHudState->player;
    s32 help;
    s32 flags;
    u16 n;
    u16 cnt;
    u16 max;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    help = 0;
    flags = 133;
    if (gHudState->combineState == 0) {
        if (gHudState->sorting != 0) {
            flags = 17;
            Font_SetSpacing(18, 18);
            Font_SetPos(432, 398);
            Text_Printf(gYesNoTexts[gHudState->yesNo]);
        } else {
            if (Shake_Value(SHAKE_POUCH_MENU) == 0) {
                ListPanel_Draw(&gCombineKindPanel, NULL, NULL, gHudState->combineThree);
            }
            flags = ITEM_LIST_SLIDE | ITEM_LIST_SORT_HINT | ITEM_LIST_FRAME | ITEM_LIST_COMBINE | ITEM_LIST_TITLE2;
        }
    } else if (gHudState->combineState < 5) {
        gCombineMaterialPanel.lines = gHudState->combineThree + 2;
        ListPanel_Draw(&gCombineMaterialPanel, NULL, NULL, -1);
        Font_SetSpacing(18, 18);
        Font_SetColor(0x807F7F28);
        Text_DrawLinesColored(270, 62, gHudState->combineThree == 0 ? gCombine2MaterialLabels : gCombine3MaterialLabels, 39);
        Font_SetPos(270, 80);
        if (gHudState->combinePick == 0) {
            Text_Printf(gCombineSelectingText[0]);
        } else {
            CombineMenu_DrawItemName(gHudState->combineSlots[0], gHudState->combineItems[0]);
            Font_SetPos(270, 119);
            if (gHudState->combinePick == 1) {
                Text_Printf(gCombineSelectingText[0]);
            } else {
                CombineMenu_DrawItemName(gHudState->combineSlots[1], gHudState->combineItems[1]);
                if (gHudState->combineThree > 0) {
                    Font_SetPos(270, 158);
                    if (gHudState->combinePick == 2) {
                        Text_Printf(gCombineSelectingText[0]);
                    } else {
                        CombineMenu_DrawItemName(gHudState->combineSlots[2], gHudState->combineItems[2]);
                    }
                }
            }
        }
        if (gHudState->combineState < 2) {
            help = 0xA9182;
            Font_SetColor(0x807F7F7F);
            Font_SetPos(306, 358);
            Text_PrintfColored(gCombineSelectFmt[0], gCombine3MaterialLabels[gHudState->combinePick]);
        }
        if (gHudState->combineState < 3) {
            flags = ITEM_LIST_FRAME | ITEM_LIST_COMBINE | ITEM_LIST_TITLE2;
        }
    }
    ItemList_DrawPouch(gHudState->slot, help, flags);
    if (gHudState->combineState >= 2 && gHudState->combineState < 5) {
        gCombineResultWindow.y = gHudState->combineThree == 0 ? 157 : 196;
        Window_DrawText(&gCombineResultWindow, (s16 *)NULL, (u8 *)gCombineInfoLabels[0]);
        Font_SetColor(0x807F7F7F);
        Font_SetPos(270, gCombineResultWindow.y + 20);
        if (gHudState->combineResult > 0) {
            Text_Draw((char *)gItemNames[((RecipeView *)gHudState->combineRecipe)->result]);
        } else {
            Text_Draw(gCombineUnknownText);
        }
        Font_SetPos(360, gCombineResultWindow.y + 40);
        if (gHudState->combineResult > 0) {
            Text_PrintfColored(gCombineRateFmt[0], (s8)Combine_RecipeRate(gHudState->combineRecipe, 0));
        } else {
            Text_Draw(gCombineRateUnknownText);
        }
        Font_SetPos(360, gCombineResultWindow.y + 60);
        Text_Draw(Recipe_YieldLabel(gHudState->combineRecipe));
        if (gHudState->combineState == 2) {
            gCombineDoPanel.y = gHudState->combineThree == 0 ? 255 : 293;
            D_004321C4[0] = gHudState->combineError == 0 ? 0x807F7F7F : 0x80404040;
            ListPanel_Draw(&gCombineDoPanel, NULL, NULL, gHudState->combineConfirm);
        }
    }
    if (gHudState->combineState == 5 || gHudState->combineState == 6) {
        if (gHudState->combineMadeItem != 174) {
            Window_DrawText(&gCombineResultWindow, (s16 *)NULL, (u8 *)gCombineSuccessLabels);
            n = gHudState->combineMade;
            cnt = Pouch_Count(pl, gHudState->combineMadeItem);
            max = gItemData[gHudState->combineMadeItem].max;
        } else {
            Window_DrawText(&gCombineResultWindow, (s16 *)NULL, (u8 *)gCombineFailLabels[0]);
            n = 1;
            cnt = Pouch_Count(pl, 174);
            max = gItemData[174].max;
        }
        Font_SetColor(0x807F7F7F);
        Font_SetPos(270, gCombineResultWindow.y + 20);
        Text_Draw((char *)gItemNames[gHudState->combineMadeItem]);
        Font_SetPos(360, gCombineResultWindow.y + 40);
        Text_PrintfColored(gHudFmtInt, n);
        Font_SetPos(360, gCombineResultWindow.y + 60);
        Text_PrintfColored(gHudFmtFraction, cnt, max);
    }
}

/* Draws an ingredient's name, grey when no pouch slot holds it. */
void CombineMenu_DrawItemName(u8 slot, u16 item)
{
    u32 color;

    if ((slot & 0xFF) != 255) {
        color = 0x807F7F7F;
    } else {
        color = 0x80404040;
    }
    Font_SetColor(color);
    Text_Draw((char *) ((s8 *) gItemNames[item & 0xFFFF]));
}

/* Opens the hunter's status pages (in the town the stats are recomputed first)
 * and builds the skill list. Returns 0. */
s32 PersonalStatus_Open(void)
{
    if (gScene.inLobby != 0) {
        Player_RecomputeStats(&gPlayers[gScene.localPlayer]);
    }
    SkillList_Build(gHudState->player, NULL);
    gHudState->pageCursor = 0;
    gHelpShow = 0;
    return 0;
}

/* The status pages' input: left and right turn the pages (the first and
 * the skill list's), wrapping around. Returns cancel and the menu button. */
u16 PersonalStatus_Update(u16 buttons)
{
    u8 n = SkillList_PageCount() + 1;

    if (buttons & (PAD_LEFT | PAD_RIGHT)) {
        if (buttons & PAD_RIGHT) {
            (gHudState->pageCursor)++;
            if (gHudState->pageCursor > n) {
                gHudState->pageCursor = 0;
            }
        } else if (gHudState->pageCursor == 0) {
            gHudState->pageCursor = n;
        } else {
            gHudState->pageCursor -= 1;
        }
        Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    }
    return buttons & (PAD_START | PAD_CROSS);
}

/* Draws the status page under the cursor. */
void PersonalStatus_Draw(void)
{
    PersonalStatus_DrawPage(gHudState->player, gHudState->pageCursor);
}

/* The equipment in menu slot `slot` (0 the weapon, 1-5 the armour of types 2-5 and 0) copied to
 * `out`; NULL when the slot is empty. */
static EquipItem *Player_GetEquipBySlot(EquipItem *out, u8 slot, Player *pl)
{
    EquipItem *worn;

    out->used = out->level = 0;
    switch (slot) {
    case 0:
        *out = pl->equip[6];
        out->used = 1;
        return out;
    case 1:
        out->type = 2;
        break;
    case 2:
        out->type = 3;
        break;
    case 3:
        out->type = 4;
        break;
    case 4:
        out->type = 5;
        break;
    case 5:
        out->type = 0;
        break;
    default:
        return NULL;
    }
    worn = &pl->equip[out->type];
    if (worn->id == 0) {
        return NULL;
    }
    out->used = 1;
    out->id = worn->id;
    out->level = worn->level;
    out->deco[0] = worn->deco[0];
    out->deco[1] = worn->deco[1];
    out->deco[2] = worn->deco[2];
    return out;
}

/* Opens the equipment page on its first slot. Returns 0. */
s32 EquipStatus_Open(void)
{
    gHudState->pageCursor = 0;
    gHudState->equipCursor = 0;
    gHelpShow = 0;
    return 0;
}

/* The equipment page's input: up and down pick one of the six equipment
 * slots, left and right the pages of the item's levels (Equip_LevelMark). */
u16 EquipStatus_Update(u16 buttons)
{
    u8 cursor;
    EquipItem item;
    EquipItem *equip;
    Player *pl;

    pl = gHudState->player;
    cursor = gHudState->pageCursor;
    Menu_CursorUpDown(&cursor, buttons, 6);
    if (cursor != gHudState->pageCursor) {
        gHudState->pageCursor = cursor;
        gHudState->equipCursor = 0;
    }
    equip = Player_GetEquipBySlot(&item, gHudState->pageCursor, pl);
    if (equip != NULL) {
        gHudState->equipCursorMax = Equip_LevelMark(&item, 0, 0);
        Menu_CursorLeftRight(&gHudState->equipCursor, buttons & (PAD_LEFT | PAD_RIGHT), gHudState->equipCursorMax);
    }
    return buttons;
}

extern u8 gOptionMenuCounts[];

/* Opens the options menu from the field menu: picks the set of options it
 * lists (HudState.optionKind, an index into gOptionMenuCounts and
 * gOptionMenuRows: 0 in the town's AREA_ONLINE_VILLAGE, 1 and 2 elsewhere in the town, 3 and 4
 * outside it, by Game_IsOnline) and slides the window in. Returns 0 (opened). */
s32 OptionMenu_Open(void *menu)
{
    gHudState->pageCursor = 0;
    gHelpPage = HELP_PAGE_OPTIONS;
    gHelpShow = 0;
    if (gScene.inLobby != 0) {
        if (gScene.area == AREA_ONLINE_VILLAGE) {
            gHudState->optionKind = 0;
        } else {
            gHudState->optionKind = Game_IsOnline() == 1 ? 1 : 2;
        }
    } else {
        gHudState->optionKind = Game_IsOnline() == 1 ? 3 : 4;
    }
    gHudState->optionCount = gOptionMenuCounts[gHudState->optionKind];
    Shake_Start(SHAKE_OPTION_MENU, SHAKE_AXIS_X, 280.0f);
    return 0;
}

extern u8 *gOptionMenuRows[];


/* The options menu (vibration and the other settings): the cursor, the changed option, then the
 * settings copied to the game; PAD_START and PAD_CROSS of the buttons returned (PAD_START while shaking out). */
s32 OptionMenu_Update(u16 buttons)
{
    u8 k;

    gHelpShow = 0;
    if (Shake_Flags(SHAKE_OPTION_MENU) & 0x80) {
        return Shake_Update(SHAKE_OPTION_MENU) == 0 ? PAD_START : 0;
    }
    if (buttons & PAD_START) {
        Shake_Start(SHAKE_OPTION_MENU, SHAKE_SLIDE, 333.0f);
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        return 0;
    }
    gHelpShow = 1;
    if (Shake_Update(SHAKE_OPTION_MENU) == 1) {
        buttons &= (0xFFFF & ~(PAD_DPAD | PAD_CROSS));
        gHelpShow = 0;
    }
    Menu_CursorUpDown(&gHudState->pageCursor, buttons, gHudState->optionCount);
    k = gOptionMenuRows[gHudState->optionKind][gHudState->pageCursor];
    gHelpLine = k;
    if (buttons & (PAD_LEFT | PAD_RIGHT)) {
        switch (k) {
        case 1:
            if (gHudState->options[OPTION_COCKPIT_OFF] != 0) {
                Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                break;
            }
        case 0:
        case 3:
        case 5:
        case 7:
        case 8:
            gHudState->options[(u8)k] ^= 1;
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            break;
        case 2:
        case 4:
        case 6:
            if (buttons & PAD_LEFT) {
                /* &gHudState->options[k] (options is at 0x9B), with k added first as the game does */
                u8 *option = (u8 *)(k + (s32)gHudState + 155);

                if (*option == 0) {
                    *option = 2;
                } else {
                    *option = *option - 1;
                }
            } else {
                (*(k + gHudState->options))++;
                if (gHudState->options[k] > 2) {
                    gHudState->options[k] = 0;
                }
            }
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            break;
        }
    }
    gScene.aimReverse = gSysSave.options.aimReverse = gHudState->options[OPTION_AIM_REVERSE];
    gScene.camControl = gSysSave.options.camControl = gHudState->options[OPTION_CAM_CONTROL];
    gScene.camFollow = gSysSave.options.camFollow = gHudState->options[OPTION_CAM_FOLLOW];
    gSysSave.options.chatSize = gHudState->options[OPTION_CHAT_SIZE];
    if (gHudState->options[OPTION_VIBRATION] != gSysSave.options.vibration) {
        gSysSave.options.vibration = gHudState->options[OPTION_VIBRATION];
        if (gSysSave.options.vibration) {
            Vib_Start(0, 1);
        }
    }
    gChatHideNames = gHudState->options[OPTION_CHAT_HIDE_NAMES];
    gSysSave.options.chatBeep = gHudState->options[OPTION_CHAT_BEEP];
    return (u16)(buttons & (PAD_START | PAD_CROSS));
}


extern char *gFieldOptionLabels[][9];
extern char *gFieldOptionValues[][3];
extern ListPanel gOptionMenuPanel;

/* Draws the options menu: the list of the page options (gOptionMenuPanel), each option value text
 * centred at the right (option 1's own text while option 0 is set), and the cursor arrows
 * (grey on that option, pulsing otherwise). */
void OptionMenu_Draw(void)
{
    s16 pos[2];
    SpriteWords sprite;
    char *list[12];
    s16 w;
    s16 y;
    u8 *rows;
    s16 k;
    s16 i;
    char **names;

    Shake_CopyOffset(SHAKE_OPTION_MENU, (struct ShakeOffset *)pos);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    rows = gOptionMenuRows[gHudState->optionKind];
    names = gFieldOptionLabels[gHudState->options[OPTION_COCKPIT_OFF]];
    for (i = 0; i < gHudState->optionCount; i++) {
        list[i] = names[rows[i]];
    }
    list[i] = NULL;
    gOptionMenuPanel.items = list;
    gOptionMenuPanel.lines = gHudState->optionCount;
    ListPanel_Draw(&gOptionMenuPanel, pos, gOptionMenuTitle[0], -1);
    Font_SetColor(0x807F7F28);
    y = 85;
    for (i = 0; i < gHudState->optionCount; i++) {
        k = rows[i];
        if (gHudState->options[OPTION_COCKPIT_OFF] == 0 || k != 1) {
            w = Str_Length(gFieldOptionValues[k][gHudState->options[k]]);
            Font_SetPos(pos[0] + 553 - w * 9 / 2, y);
            Text_Draw(gFieldOptionValues[k][gHudState->options[k]]);
        } else {
            Font_SetPos(pos[0] + 526, y);
            Text_Printf(D_00448198);
        }
        y += 22;
    }
    sprite.w = 18;
    sprite.y = gHudState->pageCursor * 22 + 85;
    sprite.h = 18;
    if (gScene.inLobby != 0 || gHudState->options[OPTION_COCKPIT_OFF] == 0 || rows[gHudState->pageCursor] != 1) {
        sprite.color = (((s8)(48.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10))) + 175) << 8) | 0xF0200020;
    } else {
        sprite.color = 0xF0707070;
    }
    sprite.x = pos[0] + 494;
    sprite.uv0 = TEX_UV(166, 26);
    sprite.uv1 = TEX_UV(148, 46);
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x = pos[0] + 594;
    *(s16 *)&sprite.uv0 = TEX_UV(148, 0);
    *(s16 *)&sprite.uv1 = TEX_UV(166, 0);
    Sprite_Draw((struct Sprite *)&sprite);
}

/* Returns 1 if an item get may start now: for the local player (or none),
 * with no drop dialog up and no item get in its wait. */
s32 ItemGet_IsReady(Player *pl)
{
    if (pl != NULL && Player_IsLocal(pl) == 0) {
        return 0;
    }
    if (gHudState->dropState != DROP_NONE) {
        return 0;
    }
    return gItemGetWait <= 0;
}

/* Reserves the item get for the local player (or none): one runs at a time,
 * for ITEM_GET_WAIT frames. A free one gives *ticket a new serial and
 * returns 0; a caller whose ticket is the current serial renews it (0);
 * else -1. */
s32 ItemGet_Reserve(s32 pl, u16 *ticket)
{
    u16 serial;

    if ((pl != 0) && (Player_IsLocal((Player *) pl) == 0)) {
        return -1;
    }
    if (gHudState->dropState != DROP_NONE) {
        return -1;
    }
    if (gItemGetWait <= 0) {
        serial = gItemGetSerial + 1;
        gItemGetSerial = serial;
        *ticket = serial;
        gItemGetWait = ITEM_GET_WAIT;
        return 0;
    }
    if (*ticket == gItemGetSerial) {
        gItemGetWait = ITEM_GET_WAIT;
        return 0;
    }
    return -1;
}

/* Adds one item to p's pouch (Pouch_GiveItems, whose result it returns), in a quest also
 * through QuestRecord_Count. */
s16 Pouch_GiveItem(Player *pl, u16 item, s16 key, u8 flags)
{
    if (gScene.inLobby == 0) {
        QuestRecord_Count(item);
    }
    return Pouch_GiveItems(pl, item, key, flags, 1);
}

/* unit's own declaration: as the file's last declaration of it (u8 in pouch.c) */

/* Adds n of item to pl's pouch (the result of Pouch_Add, or -1 while the state is busy or `key` is
 * not the current request); flags 1: messages, 2: the use sound. */
s16 Pouch_GiveItems(Player *pl, u16 item, s16 key, u8 flags, s16 n)
{
    u16 result;

    if (gHudState->dropState != DROP_NONE) {
        return -1;
    }
    if (gItemGetWait > 0 && key != gItemGetSerial) {
        return -1;
    }
    result = Pouch_Add(pl, item, n);
    gHudState->dropItem = 0;
    switch (result) {
    case 0:
    case 1:
        if (flags & 2) {
            Item_PlayUseSound(pl, item);
        }
        if (flags & 1) {
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_GOT, item);
        }
        if (gScene.inLobby != 0) {
            Save_StorePouch(pl);
        }
        break;
    case 2:
    case 3:
        if (flags & 1) {
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_POUCH_FULL, item);
        }
        break;
    case 5:
        if (Item_CarryKind(item) != 0) {
            if (flags & 1) {
                Notice_Push(NOTICE_KIND_MESSAGE, 12, 0);
            }
            return 5;
        }
        gHudState->dropItem = item;
        gHudState->dropState = DROP_NOTICE;
        gHudState->dropHelpLine = item;
        gHudState->dropCount = n;
        if (flags & 2) {
            Item_PlayUseSound(pl, item);
        }
        gHudState->dropNotice = 255;
        break;
    }
    return result;
}

/* Returns the item the drop dialog holds. The test for no dialog can
 * never be true (dropState 0 and 6 at once), so it always does. */
u16 ItemGet_Result(void)
{
    u8 dropState;

    dropState = gHudState->dropState;
    if ((dropState == DROP_NONE) && (dropState == DROP_CLOSING)) {
        return 0U;
    }
    return gHudState->dropItem;
}

/* unit's own declaration: declared here as Pouch_GiveItems calls it; with the definition's u8 (Player *p, u16 item, s16 n), the call compiles differently */

/* Runs the pouch-full dialog by HudState.dropState (DROP_*): the notice
 * of the item got (dropItem), then the choice to swap it for a pouch
 * item or leave it, the slot to give up (HudState.slot) and the yes/no
 * that swaps them. Returns 0 when the dialog is closed (it takes no more
 * input), 1 while it runs. */
s32 PouchDrop_Update(u16 buttons)
{
    u8 *state = &gHudState->dropState;

    switch (*state) {
    case DROP_NONE:
        if (gItemGetWait != 0) {
            gItemGetWait--;
        }
        return 0;
    case DROP_NOTICE:
        if (gHudState->dropNotice == 255) {
            gHudState->dropNotice = Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_GOT, (s16)gHudState->dropItem);
        } else if (gHudState->dropNotice == gScene.bannerNotice) {
            (*state)++;
            gHudState->dropBlink = 6;
            return 1;
        } else if (Notice_IsQueued(gHudState->dropNotice) == 0) {
            (gHudState->dropState)++;
            return 1;
        }
        return 0;
    case DROP_WAIT_BANNER:
        if (gHudState->dropBlink != 0) {
            (gHudState->dropBlink)--;
        }
        if (gHudState->dropNotice == gScene.bannerNotice) {
            return 1;
        }
        gHudState->dropBlink = 0;
        gHudState->dropChoice = 0;
        (gHudState->dropState)++;
        Shake_Start(SHAKE_POUCH_MENU, SHAKE_AXIS_X, 200.0f);
        /* fall through */
    case DROP_CHOOSE:
        if (Shake_Update(SHAKE_POUCH_MENU) == 1) {
            buttons &= PAD_DPAD;
        }
        gHudState->dropBlink = (gHudState->dropBlink + 1) & 0x7F;
        if (gHudState->dropBlink < 64) {
            gHudState->dropHelpLine = 20;
        } else {
            gHudState->dropHelpLine = HELP_ITEM_LINE(gHudState->dropItem);
        }
        Pouch_MoveCursor(&gHudState->slot, buttons & (PAD_LEFT | PAD_RIGHT));
        if (buttons & (PAD_UP | PAD_DOWN)) {
            gHudState->dropChoice = (gHudState->dropChoice + 1) & 1;
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
        }
        if (buttons & PAD_CIRCLE) {
            switch (gHudState->dropChoice) {
            case 0:
                gHudState->dropHelpLine = 21;
                gHudState->dropShowSlot = 0;
                (gHudState->dropState)++;
                break;
            case 1:
                gHudState->dropState = DROP_CLOSING;
                Shake_Start(SHAKE_POUCH_MENU, SHAKE_SLIDE, 180.0f);
                gHudState->dropItem = 0;
                break;
            }
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        }
        break;
    case DROP_PICK_SLOT:
        if (gHudState->dropShowSlot != 0) {
            if (buttons & (PAD_SQUARE | PAD_CROSS)) {
                gHudState->dropShowSlot = 0;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            }
            buttons &= ~PAD_CROSS;
        } else if (buttons & PAD_SQUARE) {
            gHudState->dropShowSlot = 1;
            Sound_PlaySeFlat(0, 9, 0);
        }
        Pouch_MoveCursor(&gHudState->slot, buttons);
        if (buttons & PAD_CIRCLE) {
            gHudState->dropHelpLine = 22;
            gHudState->dropShowSlot = 0;
            gHudState->yesNo = 1;
            (gHudState->dropState)++;
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        } else if (buttons & PAD_CROSS) {
            gHudState->dropHelpLine = 20;
            gHudState->dropBlink = 0;
            gHudState->dropState = DROP_CHOOSE;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        } else if (gHudState->dropShowSlot == 0) {
            gHudState->dropHelpLine = 21;
        } else {
            gHudState->dropHelpLine = HELP_ITEM_LINE((gHudState->player)->pouch[gHudState->slot].item);
        }
        break;
    case DROP_CONFIRM:
        gHudState->dropHelpLine = 22;
        gHudState->yesNo = YesNo_Toggle(buttons, gHudState->yesNo, 12288);
        if (buttons & PAD_CIRCLE) {
            if (gHudState->yesNo == 0) {
                Pouch_ClearSlot(gHudState->player, gHudState->slot);
                Pouch_Add(gHudState->player, gHudState->dropItem, gHudState->dropCount);
                gHudState->dropItem = 0;
                if (gScene.inLobby != 0) {
                    Save_StorePouch(gHudState->player);
                }
                gHudState->dropState = DROP_CLOSING;
                Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                Shake_Start(SHAKE_POUCH_MENU, SHAKE_SLIDE, 180.0f);
            } else {
                gHudState->dropState = DROP_PICK_SLOT;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            }
        } else if (buttons & PAD_CROSS) {
            u8 *sel = &gHudState->yesNo;

            if (*sel == 0) {
                *sel = 1;
                Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            } else {
                gHudState->dropState = DROP_PICK_SLOT;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            }
        }
        break;
    case DROP_CLOSING:
        if (Shake_Update(SHAKE_POUCH_MENU) == 0) {
            gHudState->dropState = DROP_NONE;
            return 0;
        }
        break;
    }
    return 1;
}

/* PouchDrop_Update, and when it acts the pouch is stored to the save. */
s32 PouchDrop_UpdateSave(u16 buttons)
{
    s32 ret;

    ret = PouchDrop_Update(buttons);
    if (ret != 0) {
        Save_StorePouch(gHudState->player);
    }
    return ret;
}

/* PAD_SELECT zooms the map in or out while the map shows. */
void Hud_MapZoomInput(u16 btn, u16 unused)
{
    if ((gHudState->mapOff == 0) && (gHudState->hidden == 0) && (btn & 0xFFFF & PAD_SELECT)) {
        gHudState->mapZoomed = (u8) (gHudState->mapZoomed ^ 1);
    }
}

/*
 * Runs the four players' map pings: a ping plays sounds 27 to 31, one every
 * 20 frames, and ends after 105 frames. Holding button bit 2 for 10 frames
 * starts the local player's ping and sends it to the others
 * (QSync_SendState 9).
 */
void Hud_UpdateMapPing(u16 buttons)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (gHudState->pingTimer[i] >= 0) {
            if (gHudState->pingTimer[i] <= 99 && gHudState->pingTimer[i] % 20 == 0) {
                Sound_PlaySeFlat(0, gHudState->pingTimer[i] / 20 + 27, 0);
            }
            gHudState->pingTimer[i]++;
            if (gHudState->pingTimer[i] > 105) {
                gHudState->pingTimer[i] = -1;
            }
        }
    }
    if (gHudState->pingTimer[gScene.localPlayer] < 0 && (buttons & 2)) {
        (gHudState->pingHold)--;
        if (gHudState->pingHold <= 0) {
            gHudState->pingTimer[gScene.localPlayer] = 0;
            QSync_SendState(9, gScene.localPlayer);
        }
        return;
    }
    gHudState->pingHold = 10;
}

/* Starts player i's ping on the map. */
void Hud_StartMapPing(s32 i)
{
    gHudState->pingTimer[i] = 0;
}

extern char gHudFmtSideName[];
extern u32 gPlayerNameColors[];
extern char *gSideNames[];

/* The other hunters' names above their heads (in their team colour, with the team name, in
 * versus quests). */
void Hud_DrawPlayerNames(void)
{
    char buf[32];
    f32 scr[4];
    f32 mtx[16];
    f32 pos[3];
    Player *pl;
    char *name;
    s16 i;

    Font_SetSpacing(16, 16);
    Font_SetColor(0x807F7F7F);
    for (pl = gPlayers, i = 0; i < 4; i++, pl++) {
        if (pl->index == gScene.localPlayer) {
            continue;
        }
        if (gScene.inQuest[i] != 1) {
            continue;
        }
        if (pl->active == 0 || pl->visible == 0) {
            continue;
        }
        if (!(u8)Obj_InCurrentArea(pl)) {
            continue;
        }
        Mtx_Identity(mtx);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        pos[0] = pl->pos[0];
        pos[1] = 192.0f + pl->pos[1] + Player_NameTagRaise(pl);
        pos[2] = pl->pos[2];
        Camera_ProjectPoint(scr, pos);
        if (scr[0] < 700.0f && !(scr[0] <= -60.0f) && scr[1] < 500.0f && !(scr[1] <= -20.0f) && !(scr[3] <= 0.0f)) {
            name = (char *)pl->name;
            if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                name = gPlayerIds[pl->index];
            }
            if (Quest_IsVersus() == 1) {
                Font_SetColor(gPlayerNameColors[pl->side]);
                sprintf(buf, gHudFmtSideName, gSideNames[pl->side], name);
                name = buf;
            }
            scr[0] -= ((s16)Str_Length(name) / 2) * 8;
            Font_SetPos(1.25f * scr[0], scr[1]);
            Text_Draw(name);
        }
    }
}

extern NextQuest gNextQuest;

/* In versus quests, draws each side's monster's health bar above it (green,
 * yellow, then red, blinking under a fifth), with the side's name from
 * gNextQuest.teams. */
void Hud_DrawVersusMonsterBars(void)
{
    f32 scr[4];
    f32 mtx[16];
    f32 pos[3];
    Em *em;
    s32 i;
    s16 x;
    s16 y;
    u8 *side; /* gNextQuest moved on one team per side, so that its teams[0] is this side's (the game's own walk) */
    u32 color;
    f32 hpRatio;

    if (Quest_IsVersus() != 1) {
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    for (i = 0, side = (u8 *)&gNextQuest; i < 2; i++, side += sizeof(NextQuestTeam)) {
        if ((em = Em_FindOnSide(i + 1)) == NULL) {
            continue;
        }
        if (Em_IsHiddenFromHud(em) == 1) {
            continue;
        }
        Mtx_Identity(mtx);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        pos[0] = em->posX;
        pos[1] = 450.0f + em->posY;
        pos[2] = em->posZ;
        Camera_ProjectPoint(scr, pos);
        if (scr[0] < 552.0f && !(scr[0] <= -40.0f) && scr[1] < 448.0f && !(scr[1] <= -30.0f) && !(scr[3] <= 0.0f)) {
            x = 640.0f * scr[0] / 512.0f - 52.0f;
            y = scr[1];
            hpRatio = (f32)em->hp / (f32)em->hpMax;
            if (!(hpRatio <= 0.6666667f)) {
                color = 0xFF00FF20;
            } else if (!(hpRatio <= 0.33333334f)) {
                color = 0xFFFFFF00;
            } else {
                color = 0xFFFF0000;
                if (hpRatio < 0.2f) {
                    color = ((s8)(112.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10))) + 143) << 16 | 0xFF000000;
                }
            }
            Hud_DrawVersusMonsterBar(em, x, y, color, ((NextQuest *)side)->teams[0].name);
        }
    }
}


extern TexRect gBlockGaugeRects[];

/* Draws a gauge of `value` (up to 90) at x, y: a whole block
 * (gBlockGaugeRects[i]) per 30, then a third of the next block per 10
 * more. `full` is an s16 for the match: as an s32 it takes s3 and the
 * field pointers move down. */
void Hud_DrawBlockGauge(s16 x, s16 y, u16 value, u32 color)
{
    TexRect rect;
    s32 i;
    s16 rest;
    s16 full;
    s16 k;

    if (value > 90) {
        value = 90;
    }
    full = (s16)(value / 30);
    rect.x = x + 10;
    rect.y = y - 5;
    rest = value % 30;
    for (i = 0; i < full; i++) {
        rect.w = gBlockGaugeRects[i].w;
        rect.h = gBlockGaugeRects[i].h;
        rect.color = color;
        rect.uv0 = gBlockGaugeRects[i].uv0;
        rect.uv1 = gBlockGaugeRects[i].uv1;
        rect.y -= rect.h;
        Sprite_Draw((struct Sprite *)&rect);
        rect.x += rect.w;
        rect.y += rect.h;
    }
    k = rest / 10;
    if (k != 0) {
        rect.w = gBlockGaugeRects[full].w;
        rect.h = gBlockGaugeRects[full].h;
        rect.color = color;
        rect.uv0 = gBlockGaugeRects[full].uv0;
        rect.uv1 = gBlockGaugeRects[full].uv1;
        rect.w = rect.w * k / 3;
        rect.uv1.u = rect.uv0.u + k * (rect.uv1.u - rect.uv0.u) / 3;
        rect.y -= rect.h;
        Sprite_Draw((struct Sprite *)&rect);
    }
}

/* The health bar box of versus monster e at (x, y): the frame, the bar in `color`, the anger
 * gauge (Hud_DrawBlockGauge: em->versusGauge, or blinking from em->rageTimer once em->rageStarted is set) and the side's
 * name with `name`. */
void Hud_DrawVersusMonsterBar(Em *em, s16 x, s16 y, u32 color, u8 *name)
{
    Sprite sprite;
    char buf[32];
    s16 n;
    f32 wave;

    sprite.x = x - 10;
    sprite.y = y - 2;
    sprite.w = 122;
    sprite.h = 20;
    *(s32 *)&sprite.u0 = TEX_UV(56, 112);
    *(s32 *)&sprite.u1 = TEX_UV(86, 141);
    sprite.color = 0xC0000000;
    Sprite_DrawStretch9(&sprite, 8, 8, 8, 8);
    sprite.color = color;
    sprite.h = 22;
    sprite.x = x - 22;
    sprite.w = 25;
    *(s32 *)&sprite.u0 = TEX_UV(0, 119);
    *(s32 *)&sprite.u1 = TEX_UV(25, 141);
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x += sprite.w;
    sprite.w = 100;
    sprite.u0 = 25;
    sprite.u1 = 35;
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x += sprite.w;
    sprite.w = 11;
    sprite.u0 = 35;
    sprite.u1 = 54;
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x = x - 20;
    sprite.y = y - 13;
    sprite.w = 26;
    sprite.h = 10;
    *(s32 *)&sprite.u0 = TEX_UV(22, 143);
    *(s32 *)&sprite.u1 = TEX_UV(48, 153);
    Sprite_Draw((struct Sprite *)&sprite);
    if (em->rageStarted == 0) {
        if (em->versusGauge != 255 && em->versusGauge != 0) {
            Hud_DrawBlockGauge(x, y, em->versusGauge, 0xFFFFA040);
        }
    } else {
        n = (em->rageTimer - 450) / 30;
        if (n > 0) {
            wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x1F) << 11));
            Hud_DrawBlockGauge(x, y, n, (((s8)(80.0f * wave) + 80) << 8) | 0xFFFF0000 | ((s8)(32.0f * wave) + 32));
        }
    }
    Font_SetSpacing(16, 16);
    Font_SetPos(x, y);
    Font_SetColor((u8)em->side == 1 ? 0x802B7F7F : 0x807F212E);
    sprintf(buf, gHudFmtSideName, gSideNames[(u8)em->side], name);
    Text_Draw(buf);
}

extern char *gItemActionLabels[][3];
extern ListPanel gPouchMenuPanel;
extern ListPanel gPouchMenuPanel2;

/* Draws the pouch menu by HudState.state: 0 the list (while sorting, the
 * yes/no question instead of the cursor), 1 the item's actions
 * (gItemActionLabels: the set depends on whether the player is free to
 * hand items over), 2 and 3 the action's next step (the yes/no, the
 * player name list, or the destination slot while moving a stack, with
 * the source slot marked when it is on the same page), 4 the list
 * alone. */
void PouchMenu_Draw(void)
{
    Player *pl;
    u32 color;
    s32 flags;
    s32 actionSet;
    u8 listCursor;
    s32 cur;
    u8 *sel;

    pl = gHudState->player;
    listCursor = gHudState->slot;
    switch (gHudState->state) {
    case 0:
        if (gHudState->sorting != 0) {
            flags = ITEM_LIST_SORTED;
            color = 0;
            Font_SetSpacing(18, 18);
            Font_SetPos(432, 398);
            Text_Printf(gYesNoTexts[gHudState->yesNo]);
        } else {
            if (gHudState->camReqStarted != 0) {
                flags = ITEM_LIST_SLIDE | ITEM_LIST_FRAME;
            } else {
                flags = ITEM_LIST_SLIDE | ITEM_LIST_SORT_HINT | ITEM_LIST_FRAME;
            }
            color = 0xA9182;
        }
        break;
    case 1:
        flags = 0;
        actionSet = 2;
        if (Player_IsFree(pl) == 1) {
            actionSet = PouchMenu_CanTransfer(pl) != 1;
        }
        gPouchMenuPanel.items = gItemActionLabels[actionSet];
        gPouchMenuPanel.cursorColor = 0xA9182;
        ListPanel_Draw(&gPouchMenuPanel, NULL, NULL, gHudState->action);
        color = 0x808080;
        break;
    case 2:
    case 3:
        flags = 0;
        color = 0x808080;
        sel = &gHudState->action;
        switch (*sel) {
        case 0:
            gPouchMenuPanel.cursorColor = color;
            ListPanel_Draw(&gPouchMenuPanel, NULL, NULL, *sel);
            ListPanel_Draw(&gPouchMenuPanel2, NULL, NULL, gHudState->yesNo);
            break;
        case 1:
            Menu_PlayerNameList();
            break;
        case 2:
            gPouchMenuPanel.cursorColor = color;
            ListPanel_Draw(&gPouchMenuPanel, NULL, NULL, *sel);
            color = 0xA9182;
            listCursor = gHudState->dest;
            flags = ITEM_LIST_MOVING | ITEM_LIST_FRAME;
            break;
        }
        break;
    case 4:
        flags = ITEM_LIST_SLIDE;
        color = 0;
        break;
    }
    ItemList_DrawPouch(listCursor, color, flags);
    if (flags & ITEM_LIST_MOVING) {
        cur = gHudState->slot;
        if ((u8)(cur / 10) == gHudState->dest / 10) {
            Menu_DrawListCursor(NULL, cur - (u8)(cur / 10) * 10, 0x400000);
        }
    }
}

extern char gHudFmtCount[];
extern Panel gPlayerNameCountBox;

/* Draws the count to give in its box, with the arrows that can still
 * change it (up unless at the maximum, down unless at 1). */
void PlayerNameList_DrawCount(void)
{
    char buf[32];

    sprintf(buf, gHudFmtCount, gHudState->count);
    Window_DrawText(&gPlayerNameCountBox, (s16 *)NULL, (u8 *)buf);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    if (gHudState->atMax == 0) {
        Menu_DrawArrow(394, 62, 24, 18, 0xFF20FF28, 0);
    }
    if (gHudState->count != 1) {
        Menu_DrawArrow(332, 62, 24, 18, 0xFF20FF28, 1);
    }
}

extern Panel gDeliveryCountBox;

/* Draws the count to deliver in its box, with the arrows that can still
 * change it (up below the maximum, which the delivery keeps in
 * HudState.atMax, and down unless at 1). */
void Delivery_DrawCount(void)
{
    char buf[32];

    sprintf(buf, gHudFmtCount, gHudState->count);
    Window_DrawText(&gDeliveryCountBox, (s16 *)NULL, (u8 *)buf);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    if (gHudState->atMax != gHudState->count) {
        Menu_DrawArrow(324, 80, 24, 18, 0xFF20FF28, 0);
    }
    if (gHudState->count != 1) {
        Menu_DrawArrow(262, 80, 24, 18, 0xFF20FF28, 1);
    }
}

extern char *gEmptyNameText;
extern ListPanel gPlayerNameListPanels[];
extern NetUser gNetUsers[];

/* Draws the list of the other players' names (HudState.step 1; step 0
 * draws the count to give instead, PlayerNameList_DrawCount): gPlayers'
 * names in a quest, the lobby members' (gNetUsers) in the town, the ids instead
 * when the chat option hides names. The cursor (HudState.target) is a
 * player slot, and the list skips the local player. */
void Menu_PlayerNameList(void)
{
    char *names[12];
    s32 i;
    s32 n;
    NetUser *user;
    char **namePtr;
    s32 j;
    s32 count;
    s32 k;

    switch (gHudState->step) {
    case 1:
        if (gScene.inLobby == 0) {
            for (i = 0, n = 0; i < 4; i++) {
                if (gScene.localPlayer == i) {
                    continue;
                }
                if (gScene.inQuest[i] == 1) {
                    names[n] = (char *)gPlayers[i].name;
                    if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                        names[n] = gPlayerIds[i];
                    }
                } else {
                    names[n] = gEmptyNameText;
                }
                n++;
            }
            names[n] = NULL;
        } else {
            for (j = 0, count = 0, user = gNetUsers, namePtr = names; j < 10; j++, user++) {
                if (gScene.localPlayer == j) {
                    continue;
                }
                switch (NetUser_State(j)) {
                case 0:
                    *namePtr = user->name;
                    if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                        *namePtr = user->id;
                    }
                    break;
                case 1:
                case 2:
                    *namePtr = gEmptyNameText;
                    break;
                }
                namePtr++;
                count++;
            }
            names[count] = NULL;
        }
        k = gHudState->target;
        if (gScene.localPlayer < k) {
            k--;
        }
        ListPanel_Draw(&gPlayerNameListPanels[gScene.inLobby], NULL, NULL, k);
        Font_SetColor(0x807F7F7F);
        Font_SetSpacing(18, 18);
        Text_DrawLinesColored(340, 62, names, 21);
        break;
    case 0:
        PlayerNameList_DrawCount();
        break;
    }
}

struct SortPair;
void Sort_PairsDesc(struct SortPair *list, u32 n);

/* Draws a pouch as an item list (ItemList_Draw): the player's, or with
 * flag 2 the saved one. With flag 0x10 the items are packed and sorted
 * first, and the list starts at the cursor's row (cursor % 10). */
void ItemList_DrawPouch(s32 cursor, s32 cursorColor, s32 flags)
{
    ItemStack packed[20];
    ItemStack *src;
    ItemStack *dst;
    s32 i;
    s32 n;

    if (!(flags & ITEM_LIST_SAVED)) {
        src = (gHudState->player)->pouch;
    } else {
        src = gSave.pouch;
    }
    if (flags & ITEM_LIST_SORTED) {
        dst = packed;
        n = 0;
        i = 20;
        do {
            if (src->item != 0) {
                n++;
                *dst = *src;
                dst++;
            }
            src++;
        } while (--i != 0);
        for (i = 20 - n; i != 0; i--) {
            dst->count = 0;
            dst->item = 0;
            dst++;
        }
        Sort_PairsDesc((struct SortPair *)packed, n);
        src = packed;
        cursor %= 10;
        cursorColor = 0;
        flags &= ITEM_LIST_SORTED | ITEM_LIST_TITLE2;
    }
    ItemList_Draw(src, cursor, cursorColor, flags);
}

extern ListPanel gItemListBox;
extern Panel gItemListTextBox;
extern char *gItemListEmptyText;

/* Draws the page of 10 item stacks of `list` that holds entry `cursor` on
 * the item list box, with the cursor on that entry in colour `highlight` (none
 * when 0), by ITEM_LIST_* flags: each name, coloured by the combine menu's
 * state or dimmed, and its count (red at the item's maximum). `list` is
 * copied from `list_in` for the registers' order. */
void ItemList_Draw(ItemStack *list_in, s32 cursor, s32 highlight, s32 flags)
{
    s16 y;
    s16 cur;
    s16 zero[2];
    s16 *pos;
    s16 x;
    s16 base;
    u32 color;
    ItemStack *list = list_in;
    s32 usable;
    char buf[44];
    s32 mark;
    s16 k;
    s16 page;
    s32 countX;
    s16 i;

    page = cursor / 10;
    base = page * 10;
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    if (highlight != 0) {
        gItemListBox.cursorColor = highlight;
        cur = cursor - base;
    } else {
        cur = -1;
    }
    pos = NULL;
    if (flags & ITEM_LIST_SLIDE) {
        if (Shake_Value(SHAKE_POUCH_MENU) == 1) {
            pos = Shake_GetOffset(SHAKE_POUCH_MENU);
            flags &= ~ITEM_LIST_SORT_HINT;
        }
    }
    if (pos == NULL) {
        zero[1] = 0;
        zero[0] = 0;
        pos = zero;
    }
    sprintf(buf, gItemListTitleFmt[0], gItemListTitles[flags & ITEM_LIST_TITLE2], page + 1);
    ListPanel_Draw(&gItemListBox, pos, buf, cur);
    if (flags & ITEM_LIST_FRAME) {
        Menu_DrawFrame(&gItemListBox, pos);
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Font_SetSpacing(18, 18);
    i = 10;
    x = pos[0] + 431;
    list += base;
    y = pos[1] + 62;
    countX = x + 158;
    usable = flags & ITEM_LIST_COMBINE;
    mark = flags & ITEM_LIST_MOVING;
    for (; i > 0; i--, list++, base++) {
        y += 21;
        Font_SetPos(x, y);
        Font_SetColor(0x807F7F7F);
        color = 0x807F212E;
        if (usable) {
            if ((u8)CombineMenu_IsReady() == 0) {
                if (Combine_ItemHasRecipeIn((s16)list->item, gHudState->combineThree) == 0) {
                    Font_SetColor(0x80404040);
                    color = 0x80401017;
                } else if (CombineMenu_IsPicked((u16)base) == 1) {
                    Font_SetColor(0x807F7F28);
                }
            } else {
                k = CombineMenu_ResultRoom(list);
                if (k >= 0) {
                    switch (k) {
                    case 0:
                        Font_SetColor(0x807F212E);
                        break;
                    case 1:
                        Font_SetColor(0x807F5230);
                        break;
                    }
                } else if (CombineMenu_IsPicked((u16)base) == 1) {
                    Font_SetColor(0x807F7F28);
                } else {
                    Font_SetColor(0x80404040);
                    color = 0x80401017;
                }
            }
        } else if (mark) {
            if (Menu_IsNotCursorSlot((u8)base) != 1) {
                Font_SetColor(0x80404040);
                color = 0x80401017;
            }
        }
        if (list->item == 0) {
            Text_Draw(gItemListEmptyText);
        } else {
            Text_Draw((char *)gItemNames[list->item]);
            if (gItemData[list->item].max > 1) {
                Font_SetPos(countX, y);
                if (gItemData[list->item].max == 255) {
                    Text_Draw(gInfinityText[0]);
                } else {
                    if (list->count >= gItemData[list->item].max) {
                        Font_SetColor(color);
                    }
                    Text_PrintfColored(gHudFmtInt2, list->count);
                }
            }
        }
    }
    if (flags & ITEM_LIST_SORT_HINT) {
        Window_DrawText(&gItemListTextBox, (s16 *)NULL, (u8 *)NULL);
        ButtonHint_DrawSort(1);
    }
}


/* The blinking cursor frame around row `row` of the list box gItemListBox (moved by off). */
void Menu_DrawListCursor(s16 *off, s32 row, u32 color)
{
    ListPanel *panel = (ListPanel *)&gItemListBox; /* the cast keeps the address in a register (not %lo per read) */
    PrimLine line;
    s16 x;
    s16 y;
    s16 rowH;

    x = panel->x;
    y = panel->y;
    if (off != NULL) {
        x += off[0];
        y += off[1];
    }
    rowH = panel->charH + 4;
    line.x0 = x - 2;
    line.x1 = x + panel->charW * panel->lineChars + 1;
    line.y0 = y + rowH * (row + 1) - 1;
    line.y1 = line.y0 + rowH + 2;
    line.color = color | ((((s8)(48.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10)))) + 191) << 24);
    Prim_FillRect((struct PrimLine *)&line);
}

s32 func_786E50(Em *em);

/* Returns the map colour of monster `em`, pulsing with gFrameTick: grey
 * when it is defeated (EM_GROUP_DEFEAT) or the game overlay's
 * func_786E50 holds; orange without the detect skill (detectLevel below
 * 2); red, pulsing faster, while it is in combat; else purple. */
u32 Map_MonsterColor(Em *em)
{
    f32 wave;
    u8 level;

    if (em->stateGroup == EM_GROUP_DEFEAT || func_786E50(em) == 1) {
        level = (s8)(32.0f * Math_Sin(0.0000958738f * (u16)(*(u8 *)&gFrameTick << 8))) + 128;
        return level | ((level << 16) | 0xC0000000 | (level << 8));
    }
    if (gHudState->detectLevel <= 1) {
        wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10));
        return ((((s8)(54.0f * wave) + 83) << 8) | 0xFFFA0000) | ((s8)(6.0f * wave) + 221);
    }
    if (em->combat == 1) {
        wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0xF) << 12));
        return (((s8)(64.0f * wave) + 72) << 8) | 0xFFF00000;
    }
    wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10));
    return (((s8)(48.0f * wave) + 128) << 8) | 0xFF1000E0;
}

/* Draws a round dot of `radius` in `color` centred at x, y on the map. */
void Map_DrawDot(u32 color, f32 x, f32 y, f32 radius)
{
    SpriteWords sprite;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    sprite.x = x - radius;
    sprite.y = y - radius;
    sprite.w = radius + radius;
    sprite.h = radius + radius;
    sprite.color = color;
    sprite.uv0 = TEX_UV(16, 240);
    sprite.uv1 = TEX_UV(32, 256);
    Sprite_Draw((struct Sprite *)&sprite);
}

/* Draws the map's 16-pixel marker centred at x, y. */
void Map_DrawMarker(f32 x, f32 y)
{
    SpriteWords sprite;
    f32 radius = 8.0f;

    Tex_Prepare(1, 325);
    Tex_Bind(325);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    sprite.x = x - radius;
    sprite.y = y - radius;
    sprite.w = radius + radius;
    sprite.h = radius + radius;
    sprite.color = 0xFFFFFFFF;
    sprite.uv0 = TEX_UV(32, 240);
    sprite.uv1 = TEX_UV(47, 255);
    Sprite_Draw((struct Sprite *)&sprite);
}

/* Returns 1 while a monster is hidden from the HUD: its armour state, the
 * Monoblos and Diablos kinds while under the sand, the Blangonga kinds in
 * their armour state. */
s32 Em_IsHiddenFromHud(Em *em)
{
    u8 kind;

    if (em->stateArmor != 0) {
        return 1;
    }
    kind = em->type;
    switch (kind) {
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
        return em->stance == EM_STANCE_UNDER;
    case EM_KIND_BLANGONGA:
    case EM_KIND_BLANGO:
        return EmBlangonga_InStateArmor(em);
    default:
        return 0;
    }
}

/* Returns 1 while the map must not show a monster: Chameleos always; else,
 * unless the detect level is DETECT_SKILL_HIGH or more, as
 * Em_IsHiddenFromHud, Khezu on the ceiling and Basarios while the game
 * overlay's func_770310 says so. */
s32 Map_IsMonsterHidden(Em *em)
{
    u8 kind;

    kind = em->type;
    if (kind == EM_KIND_CHAMELEOS) {
        return 1;
    }
    if ((s32) gHudState->detectLevel >= DETECT_SKILL_HIGH) {
        return 0;
    }
    if (em->stateArmor != 0) {
        return 1;
    }
    switch (kind) {
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        return (s8)em->ceilingTimer != 0;
    case EM_KIND_BASARIOS:
        return func_770310();
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
        return em->stance == EM_STANCE_UNDER;
    case EM_KIND_BLANGONGA:
    case EM_KIND_BLANGO:
        return EmBlangonga_InStateArmor(em);
    default:
        return 0;
    }
}

extern u32 gMapMonsterIconColors[];
extern u8 gMapMonsterIconU[];

/* Draws the map icon of monster `em` at (x, y), sized by `scale`: a dot
 * (Map_DrawDot) for the kinds gMapIconKinds marks MAP_ICON_DOT, else its
 * icon as two textured triangles, coloured by Map_MonsterColor. With the
 * detect skill the icon turns with the monster's heading and shows
 * whether it flies; without, it is the plain icon 7, smaller. */
void Map_DrawMonsterIcon(Em *em, f32 x, f32 y, f32 scale)
{
    PrimTexTri tri;
    f32 mtx[16];
    f32 corner[4][3];
    f32 pos[3];
    u32 tint;
    f32 half;
    f32 radius;
    s16 i;
    u16 heading;
    s16 uLeft;
    s16 uRight;
    u8 icon;

    icon = gMapIconKinds[em->type];
    if (icon == MAP_ICON_NONE) {
        return;
    }
    if (icon & MAP_ICON_DOT) {
        if (icon & MAP_ICON_DOT_LARGE) {
            radius = 5.0f * scale;
            tint = Map_MonsterColor(em);
        } else {
            radius = 3.0f;
            tint = gMapMonsterIconColors[icon & MAP_ICON_DOT_COLOR];
        }
        Map_DrawDot(tint, x, y, radius);
        return;
    }
    if (Map_IsMonsterHidden(em) == 1) {
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    if (gHudState->detectLevel <= 1) {
        icon = 7;
        heading = 0;
        scale *= 0.8f;
    } else {
        if (em->stance != EM_STANCE_AIR) {
            icon += 8;
        }
        heading = 0x18000 - em->rotY;
    }
    uLeft = gMapMonsterIconU[icon];
    uRight = uLeft + 16;
    Mtx_Identity(mtx);
    Mtx_SetRotZYX(mtx, 0.0f, 0.0f, 6.2831855f * heading / 65536.0f);
    pos[2] = 0.0f;
    half = 12.8f * scale;
    for (i = 0; i < 4; i++) {
        pos[0] = -half + 2.0f * half * (i / 2);
        pos[1] = -half + 2.0f * half * (i & 1);
        Vec_Rotate(corner[i], pos, mtx);
        corner[i][0] += x;
        corner[i][1] += y;
    }
    tri.x0 = corner[0][0];
    tri.y0 = corner[0][1];
    tri.x1 = corner[1][0];
    tri.y1 = corner[1][1];
    tri.x2 = corner[2][0];
    tri.y2 = corner[2][1];
    tri.u0 = uLeft;
    tri.v0 = 240;
    tri.v1 = 256;
    tri.v2 = 240;
    tri.u1 = uLeft;
    tri.u2 = uRight;
    tri.color = Map_MonsterColor(em);
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    tri.x0 = corner[3][0];
    tri.y0 = corner[3][1];
    tri.u0 = uRight;
    tri.v0 = 256;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
}

extern u32 gPlayerMapColors[];

/* The map icon of player p at (x, y) (scale s), turned by its heading (the local player's own
 * icon, the others' in their colour gPlayerMapColors), flashing on its counter gHudState+0x3C (shown
 * by Map_DrawPingGlow; a quarter of it when p is hidden, +0x4B8 bit 11). */
void Map_DrawPlayerIcon(Player *pl, f32 x, f32 y, f32 scale)
{
    PrimTexTri tri;
    f32 mtx[16];
    f32 corner[4][3];
    f32 pos[3];
    f32 half;
    s32 hidden;
    s32 ping;
    s16 i;
    s16 uLeft;
    s16 uRight;

    hidden = (pl->statusIcons & 0x800) != 0; /* status 7 (status7Timer): hidden from the others' maps */
    if (pl->index != gScene.localPlayer && hidden == 1) {
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 325);
    Tex_Bind(325);
    if (pl->index == gScene.localPlayer) {
        uLeft = 0;
    } else {
        uLeft = 80;
    }
    uRight = uLeft + 15;
    Mtx_Identity(mtx);
    Mtx_SetRotZYX(mtx, 0.0f, 0.0f, 6.2831855f * (u16)(0x18000 - pl->rot[1]) / 65536.0f);
    pos[2] = 0.0f;
    half = 9.0f;
    for (i = 0; i < 4; i++) {
        pos[0] = -half + 2.0f * half * (i / 2);
        pos[1] = -half + 2.0f * half * (i & 1);
        Vec_Rotate(corner[i], pos, mtx);
        corner[i][0] = x + scale * corner[i][0];
        corner[i][1] = y + scale * corner[i][1];
    }
    tri.color = gPlayerMapColors[pl->index & 3];
    ping = gHudState->pingTimer[pl->index];
    if (ping >= 0 && ping <= 99 && ping % 20 == 0) {
        tri.color = 0xFFFFFFFF;
    }
    tri.x0 = corner[0][0];
    tri.y0 = corner[0][1];
    tri.x1 = corner[1][0];
    tri.y1 = corner[1][1];
    tri.x2 = corner[2][0];
    tri.y2 = corner[2][1];
    tri.u0 = uLeft;
    tri.v0 = 240;
    tri.v1 = 255;
    tri.v2 = 240;
    tri.u1 = uLeft;
    tri.u2 = uRight;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    tri.x0 = corner[3][0];
    tri.y0 = corner[3][1];
    tri.u0 = uRight;
    tri.v0 = 255;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    if (gHudState->pingTimer[pl->index] >= 0) {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        if (hidden != 1) {
            Map_DrawPingGlow(x, y, gHudState->pingTimer[pl->index], gPlayerMapColors[pl->index & 3]);
        } else {
            Map_DrawPingGlow(x, y, gHudState->pingTimer[pl->index] / 4, gPlayerMapColors[pl->index & 3]);
        }
    }
}

/* Draws a map ping's glow at x, y in color, frame frames after it started:
 * one shrinking ring, then rings repeating every 20 frames, then the last. */
void Map_DrawPingGlow(s16 x, s16 y, s16 frame, s32 color)
{
    s16 phase;
    s32 lastRing;

    if (frame <= 19) {
        Hud_DrawGlow(x, y, color, 40.0f, 25 - frame);
    } else if (frame <= 84) {
        phase = frame % 20;
        Hud_DrawGlow(x, y, color, 40.0f, 25 - phase);
        lastRing = 5 - phase;
        if (lastRing >= 0) {
            Hud_DrawGlow(x, y, color, 40.0f, lastRing);
        }
    } else {
        Hud_DrawGlow(x, y, color, 40.0f, 105 - frame);
    }
}



extern f32 gMapDrawScales[][2];
s32 Map_MonsterVisibility(Em *em); /* unit's own declaration: the definition returns u8; declared u8 here, two callers would drop their masks */
void Map_DrawPlayerIcon(Player *p, f32 x, f32 y, f32 s);

static void Map_GetPoint(f32 *x, f32 *y, f32 *z, s32 id);
AreaRecord *Stage_GetAreaRecord(s32 id); /* unit's own declaration: declared here as Map_GetPosition calls it; with the definition's AreaRecord *(u16 x), the call compiles differently */
s32 Player_HasMap(Player *p);

/* Draws the area map at x, y, scaled by `scale`: a dot on each area a
 * shown monster is in, the map texture (the whole map when the player has
 * one, Player_HasMap, else a window of 64 texels around the player), the
 * camp's mark (not in versus quests), the monsters' icons, the other
 * players on the same map half and the local player. */
void Map_DrawArea(f32 x, s16 y, f32 scale)
{
    f32 posX;
    f32 posY;
    f32 radius;
    Sprite sprite;
    Player *pl;
    Player *other;
    Em *emDot;
    Em *emIcon;
    AreaRecord *areaRec;
    f32 scaleX;
    f32 scaleZ;
    f32 viewLeft;
    f32 viewRight;
    f32 viewTop;
    f32 viewBottom;
    s16 i;

    pl = gHudState->player;
    gHudState->detectLevel = Player_DetectLevel(pl);
    gHudState->mapHalf = Player_GetMapHalf(pl);
    Tex_Prepare(1, 325);
    Tex_Bind(325);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    scaleX = 320.0f * gHudState->mapScaleX * scale;
    scaleZ = 320.0f * gHudState->mapScaleZ * scale;
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
    for (i = 20, emDot = (Em *)gEms; i > 0; i--, emDot++) {
        if ((u8)Map_MonsterVisibility(emDot) && emDot->showOnMap != 0) {
            Map_GetPoint(&posX, &posY, &radius, emDot->area);
            posX *= scale;
            posY *= scale;
            if (radius == 0.0f) {
                areaRec = Stage_GetAreaRecord(emDot->area);
                posX = scaleX * (areaRec->originX + 0.5f * areaRec->sizeX);
                posY = scaleZ * (areaRec->originZ + 0.5f * areaRec->sizeZ);
                radius = 32.0f;
            }
            posX += x;
            posY += y;
            Map_DrawDot(0x70C0C010, posX, posY, radius * scale);
        }
    }
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    if (Player_HasMap(pl) == 1) {
        sprite.y = y;
        sprite.x = x;
        sprite.h = sprite.w = 320.0f * scale;
        *(s32 *)&sprite.u0 = 0;
        *(s32 *)&sprite.u1 = TEX_UV(240, 240);
        switch (gHudState->mapHalf) {
        case MAP_HALF_WHOLE:
            break;
        case MAP_HALF_LEFT:
            sprite.u1 = 120; /* the texture holds two maps side by side, 120 texels each */
            break;
        case MAP_HALF_RIGHT:
            sprite.u0 = 120;
            break;
        }
    } else {
        posX = 240.0f * (pl->mapPos[0] * gHudState->mapScaleX);
        posY = 240.0f * (pl->mapPos[2] * gHudState->mapScaleZ);
        viewLeft = posX - 32.0f;
        viewRight = 32.0f + posX;
        if (viewLeft < 0.0f) {
            viewLeft = 0.0f;
        }
        if (viewRight > 240.0f) {
            viewRight = 240.0f;
        }
        viewTop = posY - 32.0f;
        viewBottom = 32.0f + posY;
        if (viewTop < 0.0f) {
            viewTop = 0.0f;
        }
        if (viewBottom > 240.0f) {
            viewBottom = 240.0f;
        }
        sprite.u0 = viewLeft;
        sprite.v0 = viewTop;
        sprite.u1 = viewRight;
        sprite.v1 = viewBottom;
        if (gHudState->mapHalf != MAP_HALF_WHOLE) {
            sprite.u0 = viewLeft / 2.0f;
            sprite.u1 = viewRight / 2.0f;
            if (gHudState->mapHalf == MAP_HALF_RIGHT) {
                sprite.u0 += 120;
                sprite.u1 += 120;
            }
        }
        sprite.x = x + 1.3333334f * viewLeft * scale;
        sprite.y = y + (s16)(320.0f * viewTop / 240.0f * scale);
        sprite.w = scale * (1.3333334f * (viewRight - viewLeft));
        sprite.h = scale * (320.0f * (viewBottom - viewTop) / 240.0f);
    }
    sprite.color = 0xDEFFFFFF;
    Sprite_Draw((struct Sprite *)&sprite);
    if (Quest_IsVersus() != 1) {
        if (gHudState->mapHalf == (u8)Map_GetHalf(gScene.map, gScene.campArea)) {
            posX = scaleX * gMapDrawScales[gScene.map][0];
            posY = scaleZ * gMapDrawScales[gScene.map][1];
            Map_DrawMarker(x + posX, y + posY);
        }
    }
    for (emIcon = (Em *)gEms, i = 20; i > 0; i--, emIcon++) {
        if ((u8)Map_MonsterVisibility(emIcon)) {
            posY = emIcon->mapPos[2] * scaleZ;
            if (posY >= 0.0f) {
                posX = emIcon->mapPos[0] * scaleX;
                Map_DrawMonsterIcon(emIcon, x + posX, y + posY, scale);
            }
        }
    }
    for (other = gPlayers, i = 0; i < gScene.playerCount; i++, other++) {
        if (other->active != 0 && i != gScene.localPlayer) {
            if (gHudState->mapHalf == (u8)Map_GetHalf(gScene.map, other->area)) {
                posX = other->mapPos[0] * scaleX;
                posY = other->mapPos[2] * scaleZ;
                Map_DrawPlayerIcon(other, x + posX, y + posY, scale);
            }
        }
    }
    other = gHudState->player;
    posX = other->mapPos[0] * scaleX;
    posY = other->mapPos[2] * scaleZ;
    Map_DrawPlayerIcon(other, x + posX, y + posY, scale);
}

extern MapPoint gMapPoints[];
extern MapPoint gMapPoints80[];

/* Reads map point id (gMapPoints; on stage MAP_TOWER_C the points 128 and
 * 129 come from gMapPoints80). */
static void Map_GetPoint(f32 *x, f32 *y, f32 *z, s32 id)
{
    MapPoint *point = &gMapPoints[id];

    if (gScene.map == MAP_TOWER_C && (id == 128 || id == 129)) {
        point = &gMapPoints80[id - 128];
    }
    *x = point->x;
    *y = point->y;
    *z = point->z;
}

/* The stages' map scale factors, x and y per stage (splat split the table in two). */
extern f32 gMapScales[];
extern f32 D_00430EA4[];

/* The map position of id (Map_GetPoint) scaled by the stage's factors; when its z is 0, the centre
 * of area id's rectangle instead (returns 0 then, else 1). */
/* Raw addresses kept here: the named form changes the code (gScene.map too). */
extern u8 D_006ADCE0;
/* Gives the map position of point or area id: a point with a z scaled by
 * the stage map's scales (returns 1), else the centre of area id's record
 * (returns 0). gScene.map and the second column of gMapScales are read
 * through names of their own: through gScene the compiler loads the byte
 * twice. */
s32 Map_GetPosition(f32 *x, f32 *y, s32 id)
{
    f32 z;
    AreaRecord *area;

    Map_GetPoint(x, y, &z, id);
    if (z != 0.0f) {
        *x = 0.003125f * *x * gMapScales[D_006ADCE0 + D_006ADCE0];
        *y = 0.003125f * *y * D_00430EA4[D_006ADCE0 + D_006ADCE0];
        return 1;
    }
    area = Stage_GetAreaRecord(id);
    *x = area->originX + 0.5f * area->sizeX;
    *y = area->originZ + 0.5f * area->sizeZ;
    return 0;
}

/* Draws the screen shown while the quest moves to another area
 * (Scene_UpdateQuest's step 2): the full-screen background, a band at the
 * top and one at the bottom in the season's colour, and the area map. */
void Map_DrawAreaChange(void)
{
    Sprite sprite;
    u32 tint;
    s16 season;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 340);
    Tex_Bind(340);
    sprite.x = 0;
    sprite.y = 0;
    sprite.w = 640;
    sprite.h = 448;
    *(s32 *)&sprite.u0 = 0;
    *(s32 *)&sprite.u1 = TEX_UV(255, 191);
    sprite.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&sprite);
    season = (u8)Season_Get();
    switch (season) {
    case SEASON_WARM:
        tint = 0xA0C6AD20;
        break;
    case SEASON_COLD:
        tint = 0xA053ACCC;
        break;
    case SEASON_BREEDING:
        tint = 0xA0398C34;
        break;
    }
    sprite.color = tint;
    sprite.y = 16;
    sprite.h = 32;
    sprite.v0 = season * 21 + 193;
    sprite.v1 = sprite.v0 + 20;
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.y = 400;
    Sprite_Draw((struct Sprite *)&sprite);
    Map_DrawArea(160.0f, 64, 1.0f);
}

/* The zoomed map around the local player at x, y: the camp mark, the monsters and the other
 * players inside the view, then the local player. */
/* Level 3: at level 4 the products for the map position are scheduled otherwise (49 words
 * off with `0.75f * (5.0f * dz) + y`, 193 as written). The rest of the unit is at level 4. */
#pragma optimization_level 3
/* Draws the cockpit's small map at x, y: a window of 44.8 map texels
 * around the local player (kept inside the 240-texel map), drawn 168
 * pixels wide, with the camp's mark, the monsters, the other players on
 * the same map half and the local player, each only when inside the
 * window. */
void Map_DrawZoomed(f32 x, s16 y)
{
    Sprite sprite;
    Player *pl;
    Player *other;
    Em *em;
    f32 playerU;
    f32 playerV;
    f32 viewU;
    f32 viewV;
    f32 relU;
    f32 relV;
    f32 iconX;
    f32 iconY;
    u32 i;
    u8 *half;

    pl = gHudState->player;
    gHudState->detectLevel = Player_DetectLevel(pl);
    gHudState->mapHalf = Player_GetMapHalf(pl);
    Tex_Prepare(1, 325);
    Tex_Bind(325);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Gs_SetState(RS_ALPHA_REF, 0);
    playerU = 240.0f * (pl->mapPos[0] * gHudState->mapScaleX);
    playerV = 240.0f * (pl->mapPos[2] * gHudState->mapScaleZ);
    if (playerU < 22.4f) {
        viewU = 0.0f;
    } else if (playerU > 217.6f) {
        viewU = 195.2f;
    } else {
        viewU = playerU - 22.4f;
    }
    if (playerV < 22.4f) {
        viewV = 0.0f;
    } else if (playerV > 217.6f) {
        viewV = 195.2f;
    } else {
        viewV = playerV - 22.4f;
    }
    Tex_Prepare(1, 325);
    Tex_Bind(325);
    sprite.x = x;
    sprite.y = y;
    sprite.w = 168;
    sprite.h = 168;
    sprite.u0 = viewU;
    sprite.v0 = viewV;
    sprite.u1 = 44.8f + viewU;
    sprite.color = 0xC8FFFFFF;
    sprite.v1 = 44.8f + viewV;
    half = &gHudState->mapHalf;
    if (*half != MAP_HALF_WHOLE) {
        sprite.u0 /= 2;
        sprite.u1 /= 2;
        if (*half == MAP_HALF_RIGHT) {
            sprite.u0 += 120;
            sprite.u1 += 120;
        }
    }
    Sprite_Draw((struct Sprite *)&sprite);
    if (Quest_IsVersus() != 1) {
        if (gHudState->mapHalf == (u8)Map_GetHalf(gScene.map, gScene.campArea)) {
            relU = 240.0f * (gHudState->mapScaleX * gMapDrawScales[gScene.map][0]) - viewU;
            if (relU >= 0.0f && relU <= 44.8f) {
                relV = 240.0f * (gHudState->mapScaleZ * gMapDrawScales[gScene.map][1]) - viewV;
                if (relV >= 0.0f && relV <= 44.8f) {
                    iconX = x + 0.75f * (5.0f * relU);
                    iconY = y + 0.75f * (5.0f * relV);
                    Map_DrawMarker(iconX, iconY);
                }
            }
        }
    }
    for (em = (Em *)gEms, i = 0; i < 20; i++, em++) {
        if ((u8)Map_MonsterVisibility(em)) {
            relU = 240.0f * (em->mapPos[0] * gHudState->mapScaleX) - viewU;
            if (relU >= 0.0f && relU <= 44.8f) {
                relV = 240.0f * (em->mapPos[2] * gHudState->mapScaleZ) - viewV;
                if (relV >= 0.0f && relV <= 44.8f) {
                    iconX = x + 0.75f * (5.0f * relU);
                    iconY = y + 0.75f * (5.0f * relV);
                    Map_DrawMonsterIcon(em, iconX, iconY, 1.4f);
                }
            }
        }
    }
    for (other = gPlayers, i = 0; i < gScene.playerCount; i++, other++) {
        if (other->active != 0 && i != gScene.localPlayer) {
            if (gHudState->mapHalf == (u8)Map_GetHalf(gScene.map, other->area)) {
                relU = 240.0f * (other->mapPos[0] * gHudState->mapScaleX) - viewU;
                if (relU >= 0.0f && relU <= 44.8f) {
                    relV = 240.0f * (other->mapPos[2] * gHudState->mapScaleZ) - viewV;
                    if (relV >= 0.0f && relV <= 44.8f) {
                        iconX = x + 0.75f * (5.0f * relU);
                        iconY = y + 0.75f * (5.0f * relV);
                        Map_DrawPlayerIcon(other, iconX, iconY, 1.0f);
                    }
                }
            }
        }
    }
    iconX = x + 0.75f * (5.0f * (playerU - viewU));
    iconY = y + 0.75f * (5.0f * (playerV - viewV));
    Map_DrawPlayerIcon(gHudState->player, iconX, iconY, 1.0f);
}
#pragma optimization_level reset

extern f32 gMapAreaOffsetX[];
extern s16 gMapAreaOffsetY[];

/* The map in its HUD corner: the area map, or the zoomed map when +0x46 is set. */
void Hud_DrawMap(void)
{
    s16 y;

    if (gHudState->mapOff != 0) {
        return;
    }
    if (gHudState->hidden != 0) {
        return;
    }
    y = gChatInputMode == 0 ? 60 : 182;
    if (gHudState->mapZoomed == 0) {
        Map_DrawArea(400.0f + gMapAreaOffsetX[gScene.map], y + gMapAreaOffsetY[gScene.map], 0.7f);
        Hud_DrawGlowEffect();
    } else {
        Map_DrawZoomed(456.0f, y);
    }
}

extern u32 gHudSeasonColors[];
extern Sprite gPouchBoxSprites[];
extern char gHudFmtString[];

/* Draws the cockpit's item box: the box in the season's colour
 * (gHudSeasonColors), then the selector or the pouch bar by
 * HudState.pouchBarState (POUCH_BAR_*, stepped here), and the selected
 * item's count and name. Counts the two blink timers down. */
void Hud_DrawPouchItem(void)
{
    Player *pl = gHudState->player;
    u16 item;
    s32 stack;

    if (pl->active == 0 || pl->visible == 0) {
        return;
    }
    if (gHudState->hidden != 0) {
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    gPouchBoxSprites[0].color = gHudSeasonColors[(u8)Season_Get()];
    Sprite_DrawStretch3(&gPouchBoxSprites[0], 8, 8);
    Sprite_DrawList(&gPouchBoxSprites[1], 2);
    switch (gHudState->pouchBarState) {
    case POUCH_BAR_CLOSED:
        if (pl->itemSelect == 0) {
            Hud_DrawPouchSelector();
            break;
        }
        gHudState->pouchBarState++;
        gHudState->pouchBarTimer = 6;
    case POUCH_BAR_OPENING:
        Hud_DrawPouchBar();
        if (pl->itemSelect == 0) {
            gHudState->pouchBarState = POUCH_BAR_CLOSING;
            break;
        }
        gHudState->pouchBarTimer -= 3;
        if (gHudState->pouchBarTimer <= 0) {
            gHudState->pouchBarTimer = 0;
            gHudState->pouchBarState++;
        }
        break;
    case POUCH_BAR_OPEN:
        gHudState->pouchBarTimer = 0;
        if (pl->itemSelect == 0) {
            gHudState->pouchBarState = POUCH_BAR_CLOSING;
            gHudState->pouchBarTimer = 3;
        }
        Hud_DrawPouchBar();
        break;
    case POUCH_BAR_CLOSING:
        if (pl->itemSelect == 0) {
            gHudState->pouchBarTimer += 3;
            if (gHudState->pouchBarTimer >= 9) {
                gHudState->pouchBarTimer = 9;
                gHudState->pouchBarState = POUCH_BAR_CLOSED;
                Hud_DrawPouchSelector();
                break;
            }
        } else {
            gHudState->pouchBarState = POUCH_BAR_OPENING;
        }
        Hud_DrawPouchBar();
        break;
    }
    if (Pouch_IsSlotUsable(pl, pl->pouchCursor) == 1) {
        stack = (u8)pl->pouch[pl->pouchCursor].count;
        item = pl->pouch[pl->pouchCursor].item;
        if (stack > 0 && gItemData[item].max > 1) {
            Font_SetSpacing(18, 18);
            Font_SetPos(stack > 9 ? 530 : 525, 368);
            Font_SetColor(stack >= gItemData[item].max ? 0x807F212E : 0x807F7F7F);
            Text_PrintfColored(gHudFmtInt2, stack);
        }
        Font_SetSpacing(20, 19);
        Font_SetPos(419.0f + (186.0f - Str_Length((char *)gItemNames[pl->pouch[pl->pouchCursor].item]) * 10) / 2.0f, 396);
        Font_SetColor(0x807F7F7F);
        Text_PrintfColored(gHudFmtString, gItemNames[item]);
    }
    if (gHudState->blink60 > 0) {
        gHudState->blink60--;
    }
    if (gHudState->blink64 > 0) {
        gHudState->blink64--;
    }
}

#undef HUD

extern Sprite gPouchBarSprites[];
extern Sprite gPouchSelectorSprites[];
extern Sprite gPouchSelectorSpritesB[];
extern Sprite gPouchSelectorSpritesC[];
extern Icon gPouchSelIconsA[];
extern Icon gPouchSelIconsB[];

/* Draws the item box's selector while the pouch bar is closed: the
 * frame, the selected pouch item's icon, and for gunners the loaded
 * ammo's name and icon in the item's colour (for the bow, the coating's
 * mark when the loaded slot is the coating). The sprites' positions and
 * colours are written into the tables before each draw. */
void Hud_DrawPouchSelector(void)
{
    Player *pl;
    Icon *icons;
    s32 notGunner;
    s32 ammoColor;
    s32 melee;
    u16 ammoSlot;

    pl = gHudState->player;
    notGunner = pl->isGunner != 1;
    Sprite_DrawList(gPouchBarSprites, 6);
    melee = notGunner & 0xFF;
    if (melee != 0) {
        Sprite_Draw((struct Sprite *)&gPouchBarSprites[6]);
        icons = gPouchSelIconsA;
        gPouchSelIconsA[1].y = 373;
    } else {
        Sprite_Draw((struct Sprite *)&gPouchBarSprites[7]);
        icons = gPouchSelIconsB;
        gPouchSelIconsB[1].y = 371;
    }
    Sprite_DrawList(&gPouchBoxSprites[3], 2);
    Sprite_DrawList(gPouchSelectorSprites, 3);
    if (Pouch_IsSlotUsable((Player *) pl, pl->pouchCursor) == 1) {
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        Hud_DrawPouchSlotIcon((u8) pl->pouchCursor, 512, 1.0f, 1.0f);
    }
    if (melee == 0) {
        ammoColor = -0x7F7F80;
        if (pl->ammoCount != 0) {
            Font_SetSpacing(20, 19);
            if (Hud_DrawPouchSlotName(*(u8 *)&pl->ammoSlot, 304) == 1) {
                ammoColor = gItemColors[*(&gItemData->color + (pl->pouch[pl->ammoSlot].item * 24))];
            }
        }
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        Sprite_DrawStretch3(&gPouchSelectorSprites[3], 8, -8);
        Sprite_DrawList(gPouchSelectorSpritesB, 6);
        if (pl->weaponType != WEAPON_BOW) {
            gPouchSelectorSpritesB[6].color = ammoColor;
            Sprite_Draw((struct Sprite *)&gPouchSelectorSpritesB[6]);
        } else {
            ammoSlot = pl->ammoSlot;
            if (pl->pouch[ammoSlot].item != 135) {
                if (ammoSlot == pl->coatingSlot) {
                    Sprite_DrawList(gPouchSelectorSpritesC, 2);
                }
                gPouchSelectorSpritesB[7].color = ammoColor;
                Sprite_Draw((struct Sprite *)&gPouchSelectorSpritesB[7]);
            }
        }
    }
    if (pl->pouchHudFlags & 1) {
        icons[1].y = (s16)(icons[1].y + 2);
    }
    Draw_Icons((void *)icons, 2U);
}

#pragma rodata on
#pragma explicit_zero_data on
char gSideNameNone[1] = {
#include "sub_main/0046D0B0.inc"
};
#pragma explicit_zero_data reset
char gSideNameA[6] = {
#include "sub_main/0046D0B8.inc"
};
char gSideNameB[6] = {
#include "sub_main/0046D0C0.inc"
};
char gHudFmtSideName[5] = {
#include "sub_main/0046D0C8.inc"
};
char gHudFmtCount[8] = {
#include "sub_main/0046D0D0.inc"
};
char gHudFmtString[3] = {
#include "sub_main/0046D0D8.inc"
};
#pragma rodata off

extern Icon gPouchBarIcons[];
extern s16 gPouchBarXA[];
extern s16 gPouchBarXB[];
extern s16 D_00432438[];
extern s16 D_00432440[];
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
extern Sprite gPouchBarFrame[];


/* The pouch item bar (state 1-3 of Hud_DrawPouchItem): the selected item and its
 * neighbours sliding in by the timer (+0xB4), the frame, and for gunners the ammo list. */
void Hud_DrawPouchBar(void)
{
    Sprite box;
    Sprite shade;
    u16 slot[6];
    u8 ammo[4];
    Player *pl;
    u8 melee;
    s32 n;
    s32 dir;
    s32 slide;
    s16 slotX0;
    s16 slotX1;
    s16 slotX2;
    s16 slotX3;
    s16 slotX4;
    s16 slotX5;
    s32 i;
    f32 shadeW;

    pl = gHudState->player;
    melee = pl->isGunner != 1;
    Sprite_Draw((struct Sprite *)&gPouchBarSprites[0]);
    shade.color = 0xA0000000;
    *(s32 *)&shade.u0 = TEX_UV(126, 105);
    *(s32 *)&shade.u1 = TEX_UV(161, 140);
    box.color = 0xFF808080;
    *(s32 *)&box.u0 = TEX_UV(139, 22);
    *(s32 *)&box.u1 = TEX_UV(189, 72);
    if (gHudState->blink60 != 0) {
        n = Pouch_CountSelectable(pl);
        slot[0] = slot[1] = slot[2] = slot[3] = slot[4] = slot[5] = 255;
        dir = gHudState->dir61 >= 0;
        if (Pouch_IsSlotUsable(pl, pl->pouchCursor) == 0) {
            n++;
        } else {
            slot[dir] = pl->pouchCursor;
        }
        if (n >= 2) {
            slot[1 - dir] = Pouch_NextSelectable(pl, pl->pouchCursor, dir & 0xFF);
        }
        switch (n) {
        default:
            slot[2] = Pouch_NextSelectable(pl, slot[0], 1);
            slot[3] = Pouch_NextSelectable(pl, slot[1], 0);
            slot[4] = Pouch_NextSelectable(pl, slot[2], 1);
            slot[5] = Pouch_NextSelectable(pl, slot[3], 0);
            break;
        case 5:
            slot[2] = Pouch_NextSelectable(pl, slot[0], 1);
            slot[3] = Pouch_NextSelectable(pl, slot[1], 0);
            slot[4] = Pouch_NextSelectable(pl, slot[2], 1);
            break;
        case 4:
            slot[3] = Pouch_NextSelectable(pl, slot[1], 0);
        case 3:
            slot[2] = Pouch_NextSelectable(pl, slot[0], 1);
            break;
        case 2:
        case 1:
        case 0:
            break;
        }
        slide = gHudState->pouchBarTimer;
        slotX0 = gPouchBarXB[0] + (502 - gPouchBarXB[0]) * slide / 9;
        slotX1 = gPouchBarXB[1] + (522 - gPouchBarXB[1]) * slide / 9;
        slotX2 = gPouchBarXB[2] + (490 - gPouchBarXB[2]) * slide / 9;
        slotX3 = gPouchBarXB[3] + (534 - gPouchBarXB[3]) * slide / 9;
        slotX4 = gPouchBarXB[4] + (478 - gPouchBarXB[4]) * slide / 9;
        slotX5 = gPouchBarXB[5] + (546 - gPouchBarXB[5]) * slide / 9;
        shadeW = 40 * 0.32;
        shade.w = shadeW;
        shade.y = 350;
        shade.h = 25;
        shade.x = slotX4 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shade.x = slotX5 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shadeW = 58 * 0.32;
        box.w = shadeW;
        box.y = 344;
        box.h = 37;
        box.x = slotX4 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        box.x = slotX5 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        if (slot[4] != 255) {
            Hud_DrawPouchSlotIcon(slot[4], slotX4, 0.32f, 0.64f);
        }
        if (slot[5] != 255) {
            Hud_DrawPouchSlotIcon(slot[5], slotX5, 0.32f, 0.64f);
        }
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        shadeW = 40 * 0.76;
        shade.w = shadeW;
        shade.y = 347;
        shade.h = 30;
        shade.x = slotX2 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shade.x = slotX3 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shadeW = 58 * 0.76;
        box.w = shadeW;
        box.y = 340;
        box.h = 44;
        box.x = slotX2 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        box.x = slotX3 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        if (slot[2] != 255) {
            Hud_DrawPouchSlotIcon(slot[2], slotX2, 0.76f, 0.76f);
        }
        if (slot[3] != 255) {
            Hud_DrawPouchSlotIcon(slot[3], slotX3, 0.76f, 0.76f);
        }
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        shadeW = 40 * 0.88;
        shade.w = shadeW;
        shade.y = 345;
        shade.h = 35;
        shade.x = slotX0 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shade.x = slotX1 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shadeW = 58 * 0.88;
        box.w = shadeW;
        box.y = 337;
        box.h = 51;
        box.x = slotX0 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        box.x = slotX1 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        Sprite_Draw((struct Sprite *)&gPouchSelectorSprites[2]);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        if (slot[0] != 255) {
            Hud_DrawPouchSlotIcon(slot[0], slotX0, 0.88f, 0.88f);
        }
        if (slot[1] != 255) {
            Hud_DrawPouchSlotIcon(slot[1], slotX1, 0.88f, 0.88f);
        }
    } else {
        slot[0] = pl->pouchCursor;
        slot[1] = slot[2] = slot[3] = slot[4] = 255;
        n = Pouch_CountSelectable(pl);
        if (Pouch_IsSlotUsable(pl, pl->pouchCursor) == 0) {
            n++;
            slot[0] = 255;
        }
        switch (n) {
        default:
            slot[1] = Pouch_NextSelectable(pl, pl->pouchCursor, 1);
            slot[2] = Pouch_NextSelectable(pl, pl->pouchCursor, 0);
            slot[3] = Pouch_NextSelectable(pl, slot[1], 1);
            slot[4] = Pouch_NextSelectable(pl, slot[2], 0);
            break;
        case 4:
            slot[1] = Pouch_NextSelectable(pl, pl->pouchCursor, 1);
            slot[2] = Pouch_NextSelectable(pl, pl->pouchCursor, 0);
            slot[3] = Pouch_NextSelectable(pl, slot[1], 1);
            break;
        case 3:
            slot[2] = Pouch_NextSelectable(pl, pl->pouchCursor, 0);
        case 2:
            slot[1] = Pouch_NextSelectable(pl, pl->pouchCursor, 1);
            break;
        case 1:
        case 0:
            break;
        }
        slide = gHudState->pouchBarTimer;
        slotX0 = gPouchBarXA[1] + (488 - gPouchBarXA[1]) * slide / 9;
        slotX1 = gPouchBarXA[2] + (536 - gPouchBarXA[2]) * slide / 9;
        slotX2 = gPouchBarXA[3] + (478 - gPouchBarXA[3]) * slide / 9;
        slotX3 = gPouchBarXA[4] + (546 - gPouchBarXA[4]) * slide / 9;
        shadeW = 40 * 0.68;
        shade.w = shadeW;
        shade.y = 349;
        shade.h = 27;
        shade.x = slotX3 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shade.x = slotX2 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shadeW = 58 * 0.68;
        box.w = shadeW;
        box.y = 343;
        box.h = 39;
        box.x = slotX3 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        box.x = slotX2 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        if (slot[4] != 255) {
            Hud_DrawPouchSlotIcon(slot[4], slotX3, 0.68f, 0.68f);
        }
        if (slot[3] != 255) {
            Hud_DrawPouchSlotIcon(slot[3], slotX2, 0.68f, 0.68f);
        }
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        shadeW = 40 * 0.8;
        shade.w = shadeW;
        shade.y = 346;
        shade.h = 32;
        shade.x = slotX1 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shade.x = slotX0 - (shade.w >> 1);
        Sprite_Draw((struct Sprite *)&shade);
        shadeW = 58 * 0.8;
        box.w = shadeW;
        box.y = 339;
        box.h = 46;
        box.x = slotX1 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        box.x = slotX0 - (box.w >> 1);
        Sprite_Draw((struct Sprite *)&box);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        if (slot[2] != 255) {
            Hud_DrawPouchSlotIcon(slot[2], slotX1, 0.8f, 0.8f);
        }
        if (slot[1] != 255) {
            Hud_DrawPouchSlotIcon(slot[1], slotX0, 0.8f, 0.8f);
        }
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        Sprite_DrawList(&gPouchBoxSprites[3], 2);
        Sprite_Draw((struct Sprite *)&gPouchSelectorSprites[2]);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        if (slot[0] != 255) {
            Hud_DrawPouchSlotIcon(slot[0], 512, 1.0f, 1.0f);
        }
    }
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    box.color = -1;
    if (gHudState->pouchBarState == 3) {
        box.w = 20;
        box.y = 352;
        box.h = 17;
        *(s32 *)&box.u0 = TEX_UV(223, 156);
        *(s32 *)&box.u1 = TEX_UV(203, 173);
        box.x = gHudState->pouchBarTimer * -43 / 9 + 598;
        Sprite_Draw((struct Sprite *)&box);
        box.w = 36;
        box.y = 367;
        box.h = 29;
        *(s32 *)&box.u0 = TEX_UV(184, 207);
        *(s32 *)&box.u1 = TEX_UV(220, 236);
        box.x = gHudState->pouchBarTimer * -43 / 9 + 573;
    } else {
        box.w = 24;
        box.y = 363;
        box.h = 25;
        *(s32 *)&box.u0 = TEX_UV(205, 174);
        *(s32 *)&box.u1 = TEX_UV(229, 199);
        box.x = gHudState->pouchBarTimer * -43 / 9 + 592;
        Sprite_Draw((struct Sprite *)&box);
        box.w = 36;
        box.y = 377;
        box.h = 19;
        *(s32 *)&box.u0 = TEX_UV(184, 217);
        *(s32 *)&box.u1 = TEX_UV(220, 236);
        box.x = gHudState->pouchBarTimer * -43 / 9 + 573;
    }
    Sprite_Draw((struct Sprite *)&box);
    box.x -= 63;
    box.w = 63;
    box.y = 388;
    box.h = 7;
    *(s32 *)&box.u0 = TEX_UV(193, 34);
    *(s32 *)&box.u1 = TEX_UV(256, 41);
    Sprite_Draw((struct Sprite *)&box);
    while (box.x > 517) {
        box.x -= 63;
        Sprite_Draw((struct Sprite *)&box);
    }
    box.w = box.x - 454;
    box.u0 = 258 - box.w;
    box.x = 454;
    Sprite_Draw((struct Sprite *)&box);
    if (melee) {
        Sprite_Draw((struct Sprite *)&gPouchBarSprites[5]);
        box.w = 24;
        box.y = 367;
        box.h = 24;
        *(s32 *)&box.u0 = TEX_UV(86, 112);
        *(s32 *)&box.u1 = TEX_UV(110, 136);
        box.x = (gPouchBarSprites[6].x - 414) * gHudState->pouchBarTimer / 9 + 414;
    } else {
        box.w = 26;
        box.y = 363;
        box.h = 27;
        *(s32 *)&box.u0 = TEX_UV(230, 173);
        *(s32 *)&box.u1 = TEX_UV(256, 200);
        box.x = (gPouchBarSprites[7].x - 407) * gHudState->pouchBarTimer / 9 + 407;
    }
    Sprite_Draw((struct Sprite *)&box);
    i = melee;
    if (!i) {
        box.w = 36;
        box.y = 324;
        box.h = 36;
        *(s32 *)&box.u0 = TEX_UV(255, 235);
        *(s32 *)&box.u1 = TEX_UV(219, 199);
        box.x = gHudState->pouchBarTimer * 43 / 9 + 415;
        Sprite_Draw((struct Sprite *)&box);
        box.x += 36;
        box.w = 63;
        box.y = 324;
        box.h = 7;
        *(s32 *)&box.u0 = TEX_UV(193, 34);
        *(s32 *)&box.u1 = TEX_UV(256, 41);
        while (box.x < 507) {
            Sprite_Draw((struct Sprite *)&box);
            box.x += 63;
        }
        box.w = 570 - box.x;
        box.u1 = box.w + 195;
        Sprite_Draw((struct Sprite *)&box);
    }
    n = 1;
    if (melee) {
        gPouchBarIcons[0].x = (gPouchSelIconsA[1].x - 414) * gHudState->pouchBarTimer / 9 + 414;
        gPouchBarIcons[0].y = 373;
    } else {
        gPouchBarIcons[0].x = (gPouchSelIconsB[1].x - 398) * gHudState->pouchBarTimer / 9 + 398;
        gPouchBarIcons[0].y = 369;
    }
    if (pl->pouchHudFlags & 4) {
        gPouchBarIcons[0].y += 2;
    }
    if (gHudState->pouchBarState < 3) {
        n = 2;
        gPouchBarIcons[1].x = gHudState->pouchBarTimer * -43 / 9 + 607;
        gPouchBarIcons[1].y = 367;
        if (pl->pouchHudFlags & 2) {
            gPouchBarIcons[1].y += 2;
        }
    }
    Draw_Icons(gPouchBarIcons, n);
    if (!i) {
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        Sprite_DrawStretch9(&gPouchBarFrame[0], 8, 8, -8, -8);
        Sprite_DrawList(&gPouchBarFrame[1], 8);
        box.h = 24;
        box.v0 = 26;
        box.v1 = 48;
        if (gHudState->blink64 != 0) {
            if (gHudState->dir65 < 0) {
                ammo[1] = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 1);
                ammo[0] = ammo[2] = ammo[3] = 255;
                if (ammo[1] != 255) {
                    if (pl->pouch[ammo[1]].item == pl->pouch[pl->ammoSlot].item) {
                        ammo[1] = 255;
                    } else {
                        Font_SetSpacing(20, 20);
                        if (Hud_DrawPouchSlotName(ammo[1], 293) == 0) {
                            ammo[1] = 255;
                        } else {
                            u8 k = Bowgun_NextAmmoSlot(pl, ammo[1], 1);
                            if (pl->pouch[k].item != pl->pouch[ammo[1]].item && pl->pouch[k].item != pl->pouch[pl->ammoSlot].item) {
                                Font_SetSpacing(20, 10);
                                if (Hud_DrawPouchSlotName(k, 316) == 1) {
                                    ammo[0] = k;
                                    k = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 0);
                                    if (pl->pouch[k].item != pl->pouch[ammo[0]].item && Hud_DrawPouchSlotName(k, 256) == 1) {
                                        ammo[3] = k;
                                    }
                                }
                            }
                        }
                    }
                    Font_SetSpacing(20, 20);
                    if (Hud_DrawPouchSlotName(*(u8 *)&pl->ammoSlot, 269) == 1) {
                        ammo[2] = pl->ammoSlot;
                    }
                }
            } else {
                ammo[0] = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 1);
                ammo[1] = ammo[2] = ammo[3] = 255;
                if (ammo[0] != 255) {
                    if (pl->pouch[ammo[0]].item == pl->pouch[pl->ammoSlot].item) {
                        ammo[0] = 255;
                    } else {
                        Font_SetSpacing(20, 10);
                        if (Hud_DrawPouchSlotName(ammo[0], 316) == 0) {
                            ammo[0] = 255;
                        } else {
                            u8 k = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 0);
                            if (pl->pouch[k].item != pl->pouch[ammo[0]].item) {
                                Font_SetSpacing(20, 20);
                                if (Hud_DrawPouchSlotName(k, 269) == 1) {
                                    ammo[2] = k;
                                    k = Bowgun_NextAmmoSlot(pl, ammo[2], 0);
                                    if (pl->pouch[k].item != pl->pouch[ammo[0]].item) {
                                        Font_SetSpacing(20, 10);
                                        if (Hud_DrawPouchSlotName(k, 256) == 1) {
                                            ammo[3] = k;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Font_SetSpacing(20, 20);
                    if (Hud_DrawPouchSlotName(*(u8 *)&pl->ammoSlot, 293) == 1) {
                        ammo[1] = pl->ammoSlot;
                    }
                }
            }
            Hud_AmmoIconSprite((u8 *)pl, &box);
            for (i = 3; i >= 0; i--) {
                if (pl->weaponType == WEAPON_BOW && pl->pouch[ammo[i]].item == 135) {
                    box.y += D_00432438[i];
                } else {
                    if (ammo[i] != 255) {
                        box.color = gItemColors[gItemData[pl->pouch[ammo[i]].item].color];
                    } else {
                        box.color = 0xFF707070;
                    }
                    box.h = D_00432440[i];
                    Sprite_Draw((struct Sprite *)&box);
                    box.y += D_00432438[i];
                }
            }
        } else {
            ammo[0] = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 1);
            ammo[1] = ammo[2] = 255;
            if (ammo[0] != 255) {
                Font_SetSpacing(20, 20);
                melee = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 0);
                if (pl->pouch[melee].item != pl->pouch[ammo[0]].item && Hud_DrawPouchSlotName(melee, 255) == 1) {
                    ammo[2] = melee;
                }
                if (pl->pouch[ammo[0]].item == pl->pouch[pl->ammoSlot].item) {
                    ammo[0] = 255;
                } else if (Hud_DrawPouchSlotName(ammo[0], 303) == 0) {
                    ammo[0] = 255;
                }
                if (Hud_DrawPouchSlotName(*(u8 *)&pl->ammoSlot, 279) == 1) {
                    ammo[1] = pl->ammoSlot;
                }
            }
            Hud_AmmoIconSprite((u8 *)pl, &box);
            for (i = 2; i >= 0; i--) {
                if (pl->weaponType == WEAPON_BOW) {
                    if (pl->pouch[ammo[i]].item != 135) {
                        if (ammo[i] != 255 && ammo[i] == pl->coatingSlot) {
                            Hud_DrawPouchBarEnds(427, box.y, box.h + 1);
                        }
                        goto draw;
                    }
                } else {
                draw:
                    if (ammo[i] != 255) {
                        box.color = gItemColors[gItemData[pl->pouch[ammo[i]].item].color];
                    } else {
                        box.color = 0xFF707070;
                    }
                    Sprite_Draw((struct Sprite *)&box);
                }
                box.y += 24;
            }
        }
        gHudState->pouchBarShown = 1;
    }
}

#undef HUD

extern Icon gHudButtonIcons[2];
extern Sprite gHudButtonSprites[];

/* The two icons gHudButtonIcons (pressed a bit when Player +0x84A bits 0x10 and 8 are set) and the three sprites of gHudButtonSprites[0], when +0x5F of the state is set. */
void Hud_DrawButtonIcons(void)
{
    Player *pl = gHudState->player;

    if (gHudState->pouchBarShown != 0) {
        gHudButtonIcons[0].y = 234;
        if (pl->pouchHudFlags & 0x10) {
            gHudButtonIcons[0].y += 2;
        }
        gHudButtonIcons[1].y = 320;
        if (pl->pouchHudFlags & 8) {
            gHudButtonIcons[1].y += 2;
        }
        Draw_Icons(gHudButtonIcons, 2);
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        Sprite_DrawList(gHudButtonSprites, 3);
    }
}


/* The name and count of pouch slot `slot` (the loaded ammo highlighted) at height y; 0 when the
 * slot is empty or not usable. */
s32 Hud_DrawPouchSlotName(u8 slot, s16 y)
{
    Player *pl = gHudState->player;
    ItemData *itemData;
    u16 item;
    u8 n;
    u32 color;
    char buf[32];

    if (slot == 255) {
        return 0;
    }
    item = pl->pouch[slot].item;
    itemData = &gItemData[item];
    if (!(itemData->flags & 6)) {
        return 0;
    }
    n = pl->pouch[slot].count;
    if (n == 0) {
        return 0;
    }
    if (pl->weaponType != WEAPON_BOW) {
        color = (u8)itemData->ammoKind == pl->savedAmmoType ? 0x807F7F28 : 0x807F7F7F;
    } else {
        color = slot == pl->coatingSlot ? 0x807F7F28 : 0x807F7F7F;
    }
    Font_SetColor(color);
    if (item == 135) {
        sprintf(buf, (char *)gItemNames[item]);
    } else if (itemData->max == 255) {
        sprintf(buf, gPouchInfiniteFmt[0], gItemNames[item]);
    } else {
        sprintf(buf, "%s %d", gItemNames[item], n);
    }
    Font_SetPos(419.0f + (186.0f - Str_Length(buf) * 10) / 2.0f, y);
    Text_Draw(buf);
    return 1;
}

/* The icon of pouch slot `slot` at x (scaled by sw, sh), marked when the item can't be used (or is
 * flagged in its data). */
/* icon is a u8: as an s32 it took s2 and the item*24 offset s1. */
void Hud_DrawPouchSlotIcon(u8 slot, s16 x, f32 scaleW, f32 scaleH)
{
    Sprite sprite;
    u8 color;
    u16 item;
    u8 icon;
    s8 mark;

    item = (gHudState->player)->pouch[slot].item;
    icon = gItemData[item].icon;
    if (icon == 255) {
        return;
    }
    color = gItemData[item].color;
    mark = 0;
    if (Item_IsUsable(item) == 0) {
        color = 10;
        mark = -1;
    } else if (gItemData[item].flags2 & 1) {
        mark = 1;
    }
    sprite.w = 45.0f * scaleW;
    sprite.x = x - (sprite.w >> 1);
    sprite.h = 45.0f * scaleH;
    sprite.y = 362 - (sprite.h >> 1);
    sprite.u0 = ((icon + 1) & 7) * 32 + 1;
    sprite.v0 = ((icon + 1) >> 3) * 32 + 1;
    sprite.u1 = sprite.u0 + 30;
    sprite.v1 = sprite.v0 + 30;
    sprite.color = gItemColors[color];
    Sprite_Draw((struct Sprite *)&sprite);
    if (mark != 0) {
        if (mark > 0) {
            *(s32 *)&sprite.u0 = TEX_UV(128, 224);
            *(s32 *)&sprite.u1 = TEX_UV(160, 256);
            sprite.color = 0xFFFFFFFF;
        } else {
            *(s32 *)&sprite.u0 = TEX_UV(160, 224);
            *(s32 *)&sprite.u1 = TEX_UV(192, 256);
            sprite.color = 0xC8FF0000;
        }
        Sprite_Draw((struct Sprite *)&sprite);
    }
}

extern Panel gItemWindowTextBox;
extern ListPanel gItemWindowDropPanel;
extern ListPanel gItemWindowPanel;
extern Panel gItemWindowNameBox[2];
extern char *gItemGotFmt;
extern u32 gItemDropPanelColors[];
extern s32 gItemDropPouchColors[];

/* Draws the pouch-full dialog (PouchDrop_Update) by its state: the
 * pouch-full text (blinking), then the window that slides in with the
 * swap-or-leave choice or the yes/no, the pouch's list, the item got
 * (with its count when more than one) and the help line. */

void Hud_DrawItemWindow(void)
{
    char buf[64];
    s32 flags;
    s16 *offset;
    u8 state;

    if (gHudState->dropState <= DROP_NOTICE) {
        return;
    }
    if (gHudState->dropState == DROP_WAIT_BANNER) {
        if (gHudState->dropBlink == 0) {
            Window_DrawText(&gItemWindowTextBox, (s16 *)NULL, (u8 *)gItemFullText[0]);
        }
        return;
    }
    flags = ITEM_LIST_SLIDE | ITEM_LIST_FRAME;
    offset = Shake_GetOffset(SHAKE_POUCH_MENU);
    state = gHudState->dropState;
    switch (state) {
    case DROP_PICK_SLOT:
    case DROP_CHOOSE:
        gItemWindowDropPanel.cursorColor = gItemDropPanelColors[state];
        ListPanel_Draw(&gItemWindowDropPanel, offset, NULL, gHudState->dropChoice);
        break;
    case DROP_CONFIRM:
        ListPanel_Draw(&gItemWindowPanel, NULL, NULL, gHudState->yesNo);
        break;
    case DROP_CLOSING:
        flags = ITEM_LIST_SLIDE;
        break;
    }
    ItemList_DrawPouch(gHudState->slot, gItemDropPouchColors[gHudState->dropState], flags);
    if (Shake_Value(SHAKE_POUCH_MENU) != 0) {
        return;
    }
    if (gHudState->dropCount <= 1) {
        Window_DrawText(&gItemWindowNameBox[0], (s16 *)NULL, (u8 *)gItemNames[gHudState->dropItem]);
    } else {
        sprintf(buf, gItemGotFmt, gItemNames[gHudState->dropItem], gHudState->dropCount);
        Window_DrawText(&gItemWindowNameBox[1], (s16 *)NULL, (u8 *)buf);
    }
    HelpWindow_DrawLine(1, (u16)gHudState->dropHelpLine);
    if (gHudState->dropState == DROP_PICK_SLOT && gHudState->dropShowSlot == 0) {
        ButtonHint_DrawDetails();
    }
}

/* Draws the local player's status, then the other hunters' in the quest
 * below it. */
void Hud_DrawPartyStatus(void)
{
    Player *pl;
    u32 i;
    s16 y;

    Hud_DrawPlayerStatus(gHudState->player, 99, 15.0f);
    y = 119;
    pl = gPlayers;
    for (i = 0; i < 4; i++, pl++) {
        if (pl->active != 0 && i != gScene.localPlayer && gScene.inQuest[i] == 1) {
            Hud_DrawPlayerStatus(pl, y, 15.0f);
            y += 22;
        }
    }
}

struct Pt;

/* A player's HUD panel at x, y: for another player the name plate and health bar, then the
 * colour icon and name, the target mark when a monster in the player's area is on them (or in
 * versus), the status icon and the flashing counter ring. */
void Hud_DrawPlayerStatus(Player *pl, s16 y, f32 x)
{
    Sprite sprite;
    PrimLine bar;
    Em *em;
    f32 pingX;
    f32 hpRatio;
    f32 barX;
    s32 shown;
    s32 hidden;
    s32 i;
    s16 nameY;
    s32 rowY;
    s16 glowX;
    s16 glowY;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    if (pl->index != gScene.localPlayer) {
        sprite.x = 12.0f + x;
        sprite.y = y;
        sprite.w = 91;
        sprite.h = 20;
        *(s32 *)&sprite.u0 = TEX_UV(56, 112);
        *(s32 *)&sprite.u1 = TEX_UV(86, 141);
        sprite.color = gHudSeasonColors[(u8)Season_Get()];
        Sprite_DrawStretch9(&sprite, 8, 8, 8, 8);
        sprite.color = 0xFF000000;
        sprite.y = y;
        sprite.h = 22;
        sprite.x = x;
        sprite.w = 25;
        *(s32 *)&sprite.u0 = TEX_UV(0, 119);
        *(s32 *)&sprite.u1 = TEX_UV(25, 141);
        Sprite_Draw((struct Sprite *)&sprite);
        sprite.x += sprite.w;
        sprite.w = 65;
        sprite.u0 = 25;
        sprite.u1 = 35;
        Sprite_Draw((struct Sprite *)&sprite);
        sprite.x += sprite.w;
        sprite.w = 19;
        sprite.u0 = 35;
        sprite.u1 = 54;
        Sprite_Draw((struct Sprite *)&sprite);
        hpRatio = (f32)pl->hp / (f32)pl->hpMax;
        barX = 22.0f + x;
        rowY = y;
        bar.x0 = barX;
        bar.x1 = barX + 72.0f * hpRatio;
        bar.y0 = bar.y1 = rowY + 18;
        bar.color = 0xFF00FF00;
        Prim_DrawLine(&bar);
        nameY = y;
    } else {
        rowY = y;
        nameY = rowY - 2;
    }
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    pingX = 11.0f + x;
    sprite.x = pingX;
    sprite.y = rowY + 2;
    sprite.w = 20;
    sprite.h = 18;
    sprite.color = gPlayerMapColors[pl->index & 3];
    Hud_WeaponIconUV((u8 *)pl, (struct Pt *)&sprite.u0, (struct Pt *)&sprite.u1);
    Sprite_Draw((struct Sprite *)&sprite);
    Font_SetSpacing(16, 16);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(33.0f + x, rowY + 1);
    Hud_PrintPlayerName(pl);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    shown = 0;
    if (Quest_IsVersus() == 1) {
        shown = 1;
    } else if (pl->inBattle != 0) {
        shown = 1;
    } else {
        for (em = (Em *)gEms, i = 0; i < 20; i++, em++) {
            if (em->active != 0 && em->area == pl->area && em->type != 15 && em->type != 45) {
                if (em->showOnMap != 0 && em->curGoalKind == 1 && em->curGoalArg == 0 && pl->index == em->curGoalNo
                    && em->combat == 1) {
                    shown = 1;
                    break;
                }
                if (em->combatPlayers & (1 << pl->index)) {
                    shown = 1;
                    break;
                }
            }
        }
    }
    if (shown > 0) {
        sprite.x = 109.0f + x;
        sprite.w = 25;
        sprite.y = nameY + 2;
        sprite.h = 20;
        sprite.color = 0xFFFFFFFF;
        *(s32 *)&sprite.u0 = TEX_UV(120, 237);
        *(s32 *)&sprite.u1 = TEX_UV(140, 255);
        Sprite_Draw((struct Sprite *)&sprite);
    }
    Hud_DrawStatusIcon(pl->statusIcons, (u8)pl->index, x, nameY);
    if (gHudState->pingTimer[pl->index] >= 0) {
        hidden = (pl->statusIcons & 0x800) != 0;
        if (pl->index == gScene.localPlayer || hidden != 1) {
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
            Tex_Prepare(1, 324);
            Tex_Bind(324);
            glowX = (s16)pingX + 10;
            glowY = rowY + 11;
            if (hidden != 1) {
                Map_DrawPingGlow(glowX, glowY, gHudState->pingTimer[pl->index], gPlayerMapColors[pl->index & 3]);
            } else {
                Map_DrawPingGlow(glowX, glowY, gHudState->pingTimer[pl->index] / 4, gPlayerMapColors[pl->index & 3]);
            }
        }
    }
}

extern IconUV gStatusIconUVs[];

/* Draws the status icon of party slot `slot` at x, y: it cycles through
 * the set bits of `mask` (15 statuses, the icons of gStatusIconUVs), 20
 * frames each, and resets the slot's timer when no bit is set. */
void Hud_DrawStatusIcon(u16 mask, s32 slot, u32 x, u16 y)
{
    TexRect rect;
    IconUV *iconUV;
    s16 *timerPtr;
    s32 cur;

    if (mask & 0x7FFF) {
        u8 k = slot;

        /* The cast (to the field's own type) is for the match: the add takes the index first. */
        timerPtr = &((s16 *)gHudState->statusTimer)[k];
        cur = *timerPtr;
        if (cur < 0) {
            for (cur = 0; cur < 15U; cur++) {
                if (!(mask & (1 << cur)) == 0) {
                    gHudState->statusIcon[k] = cur;
                    gHudState->statusTimer[k] = 20;
                    break;
                }
            }
        } else if ((mask & (1 << gHudState->statusIcon[k])) && cur > 0) {
            *timerPtr = cur - 1;
        } else {
            for (cur = 15; cur != 0; cur--) {
                (gHudState->statusIcon[k])++;
                if (gHudState->statusIcon[k] >= 15) {
                    gHudState->statusIcon[k] = 0;
                }
                if (mask & (1 << gHudState->statusIcon[k])) {
                    gHudState->statusTimer[k] = 20;
                    break;
                }
            }
        }
        rect.x = 134.0f + x;
        rect.y = y + 2;
        rect.w = 25;
        rect.h = 20;
        rect.color = 0xFFFFFFFF;
        iconUV = &gStatusIconUVs[gHudState->statusIcon[k]];
        rect.uv0 = iconUV->uv0;
        rect.uv1 = iconUV->uv1;
        Sprite_Draw((struct Sprite *)&rect);
    } else {
        gHudState->statusTimer[(u8)slot] = -1;
    }
}


extern s16 D_005B3858;
extern s16 D_005B3850;
extern u8 D_005B3860;
extern u32 gClockColors[];
extern Sprite gClockRimSprites[];
extern u32 D_004308C8[];
extern u32 D_004308DC[];
/* the calls cast to u8: this unit's code takes the value as u8 */

/* The quest clock: the compass markers, the clock face and its frame (colour by kind
 * Season_Get), its rim pulsing faster as the time left (+0x24) runs out, and the hands for the
 * time gone (+0x28 - +0x24), the hand at +0xAC catching up 364 a frame. */
void Hud_DrawClock(void)
{
    s32 total;
    Sprite sprite;
    s32 left;
    u32 color;
    u8 level;
    f32 wave;
    u16 hand;
    u8 kind;
    s32 gone;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    if (gScene.questPhase == 2) {
        gHudState->timeLeft = Quest_GetTime(0);
        gHudState->timeTotal = Quest_GetTime(1);
    } else if (gScene.questPhase == 3 || gScene.questPhase == 4) {
        if (gQuest.flags & 0x2000) {
            gHudState->timeLeft = 0;
        }
    }
    left = gHudState->timeLeft;
    total = gHudState->timeTotal;
    kind = (u8)Season_Get();
    Compass_DrawMarker(0, D_005B3858, kind);
    Compass_DrawMarker(0, D_005B3858 + 16384, kind);
    Compass_DrawMarker(0, D_005B3858 + ANGLE_TURN / 2, kind);
    Compass_DrawMarker(0, D_005B3858 + 49152, kind);
    Compass_DrawMarker(D_005B3850, 8192, kind);
    Compass_DrawMarker(D_005B3850, 24576, kind);
    Compass_DrawMarker(D_005B3850, -24576, kind);
    Compass_DrawMarker(D_005B3850, -8192, kind);
    Game_PauseState();
    sprite.x = 27;
    sprite.y = 98;
    sprite.w = 91;
    sprite.h = 20;
    *(s32 *)&sprite.u0 = TEX_UV(56, 112);
    *(s32 *)&sprite.u1 = TEX_UV(86, 141);
    sprite.color = gHudSeasonColors[kind];
    Sprite_DrawStretch9(&sprite, 8, 8, 8, 8);
    sprite.x = 15;
    sprite.y = 8;
    sprite.w = 109;
    sprite.h = 110;
    *(s32 *)&sprite.u0 = 0;
    *(s32 *)&sprite.u1 = TEX_UV(109, 110);
    sprite.color = gClockColors[kind];
    Sprite_Draw((struct Sprite *)&sprite);
    Hud_DrawHealthStamina();
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    sprite.x = 30;
    sprite.y = 71;
    sprite.w = sprite.h = 14;
    sprite.u0 = 112;
    sprite.v0 = kind * 16 + 92;
    sprite.u1 = 126;
    sprite.v1 = sprite.v0 + 14;
    sprite.color = gClockColors[kind];
    Sprite_Draw((struct Sprite *)&sprite);
    if (left > 9000) {
        color = 0xC07F7F7F;
    } else {
        if (left > 1800) {
            wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x1F) << 11));
            level = (s8)(60.0f * wave) + 192;
        } else {
            wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0xF) << 12));
            level = (s8)(76.0f * wave) + 172;
        }
        color = level << 8 | 0xC0FF0000 | level;
    }
    D_004308DC[0] = color;
    D_004308C8[0] = color;
    Sprite_DrawList(gClockRimSprites, 2);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    gone = total - left;
    if (gone < 0) {
        gone = 0;
    }
    hand = Clock_Angle(gone);
    if (hand < gHudState->clockHand) {
        gHudState->clockHand = hand;
    }
    Hud_DrawCompass((s16)(0x10000 - gHudState->clockHand), kind);
    if (D_005B3860 != 0) {
        Hud_DrawCompassQuad((s16)gHudState->clockHand, 0);
    } else {
        Hud_DrawCompassQuad((s16)Clock_Angle(gone), 0);
    }
    Hud_DrawCompassQuad((s16)Clock_Angle(total), 1);
    if (Game_PauseState() == 0 && gHudState->clockHand < hand) {
        gHudState->clockHand += 364;
    }
}

/* A textured triangle as Prim_DrawTexTri draws it. */
extern u32 gCompassColors[];
extern f32 gCompassPoints[][2];
extern u8 gCompassUVs[][4];

/* The compass: eight triangles around (66, 64), the points of gCompassPoints turned by `angle`
 * (ANGLE_TURN to a turn); kind selects the colour. */
void Hud_DrawCompass(s32 angle, u8 kind)
{
    PrimTexTri tri;
    f32 sinA;
    f32 cosA;
    f32 (*p)[2];
    s32 i;
    u8 (*uv)[4];

    Math_GetSinCos(&sinA, &cosA, 6.2831855f * angle / 65536.0f);
    tri.x0 = 66;
    tri.y0 = 64;
    tri.color = gCompassColors[kind];
    tri.u0 = 138;
    tri.v0 = 28;
    p = gCompassPoints;
    uv = gCompassUVs;
    tri.x2 = (s16)(0.5f + ((*p)[0] * cosA - (*p)[1] * sinA)) + 66;
    tri.y2 = (s16)(0.5f + ((*p)[0] * sinA + (*p)[1] * cosA)) + 64;
    for (i = 8; i != 0; i--) {
        tri.x1 = tri.x2;
        tri.y1 = tri.y2;
        p++;
        tri.x2 = (s16)(0.5f + ((*p)[0] * cosA - (*p)[1] * sinA)) + 66;
        tri.y2 = (s16)(0.5f + ((*p)[0] * sinA + (*p)[1] * cosA)) + 64;
        tri.u1 = (*uv)[0];
        tri.v1 = (*uv)[1];
        tri.u2 = (*uv)[2];
        tri.v2 = (*uv)[3];
        uv++;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
    }
}

extern u32 gCompassMarkerColors[];

/* Draws a marker on the compass ring (radius 28 around (66, 64)) at the
 * angle `heading - target` (16-bit angles; the difference is kept in
 * `target`): a 12x12 textured square as two triangles, in
 * gCompassMarkerColors[kind]. */
void Compass_DrawMarker(s16 heading, s16 target, u8 kind)
{
    PrimTexTri tri;
    f32 sinA;
    f32 cosA;
    f32 centerX;
    f32 centerY;

    tri.color = gCompassMarkerColors[kind];
    Math_GetSinCos(&sinA, &cosA, 6.2831855f * target / 65536.0f);
    centerX = 66.0f - 28.0f * sinA;
    centerY = 64.0f - 28.0f * cosA;
    target = (u16)(heading - target);
    Math_GetSinCos(&sinA, &cosA, 6.2831855f * target / 65536.0f);
    tri.x0 = (centerX + -6.0f * cosA) - -6.0f * sinA;
    tri.y0 = -6.0f * cosA + (centerY + -6.0f * sinA);
    tri.x1 = (centerX + 6.0f * cosA) - -6.0f * sinA;
    tri.y1 = -6.0f * cosA + (centerY + 6.0f * sinA);
    tri.x2 = (centerX + -6.0f * cosA) - 6.0f * sinA;
    tri.y2 = 6.0f * cosA + (centerY + -6.0f * sinA);
    tri.u0 = 155;
    tri.v0 = 1;
    tri.u1 = 167;
    tri.v1 = 1;
    tri.u2 = 155;
    tri.v2 = 13;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    tri.x0 = centerX + 6.0f * cosA - 6.0f * sinA;
    tri.y0 = centerY + 6.0f * sinA + 6.0f * cosA;
    tri.u0 = 167;
    tri.v0 = 13;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
}

/* The clock hand's angle (ANGLE_TURN to a turn) for `frames` of quest time: a
 * twelfth of a turn for each full 9000 frames (5 minutes at 30 a second). */
u16 Clock_Angle(s32 frames)
{
    return (frames / 9000) * 5461;
}

/* A textured quad (two triangles) around (66, 64): four points and their UVs. */
typedef struct QuadDef {
    f32 p[4][2];        /* 0x00 */
    u8 uv[4];           /* 0x20: u0, v0, u1, v1 */
    u32 color;          /* 0x24 */
} QuadDef;

extern QuadDef gCompassQuads[];

/* Draws quad k of gCompassQuads turned by `angle` (ANGLE_TURN to a turn). */
void Hud_DrawCompassQuad(s32 angle, s32 k)
{
    QuadDef *quad = &gCompassQuads[k];
    PrimTexTri tri;
    f32 sinA;
    f32 cosA;

    Math_GetSinCos(&sinA, &cosA, 0.0000958738f * angle);
    tri.color = quad->color;
    tri.u0 = quad->uv[0];
    tri.v0 = quad->uv[1];
    tri.u1 = quad->uv[2];
    tri.v1 = quad->uv[1];
    tri.u2 = quad->uv[0];
    tri.v2 = quad->uv[3];
    tri.x0 = (s16)(0.5f + (quad->p[0][0] * cosA - quad->p[0][1] * sinA)) + 66;
    tri.y0 = (s16)(0.5f + (quad->p[0][0] * sinA + quad->p[0][1] * cosA)) + 64;
    tri.x1 = (s16)(0.5f + (quad->p[1][0] * cosA - quad->p[1][1] * sinA)) + 66;
    tri.y1 = (s16)(0.5f + (quad->p[1][0] * sinA + quad->p[1][1] * cosA)) + 64;
    tri.x2 = (s16)(0.5f + (quad->p[2][0] * cosA - quad->p[2][1] * sinA)) + 66;
    tri.y2 = (s16)(0.5f + (quad->p[2][0] * sinA + quad->p[2][1] * cosA)) + 64;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    tri.u0 = quad->uv[2];
    tri.v0 = quad->uv[3];
    tri.x0 = (s16)(0.5f + (quad->p[3][0] * cosA - quad->p[3][1] * sinA)) + 66;
    tri.y0 = (s16)(0.5f + (quad->p[3][0] * sinA + quad->p[3][1] * cosA)) + 64;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
}

/* Draws the local player health and stamina gauges: the frames, the recoverable (red) health,
 * the health in its colour (by the area kind), the stamina blinking red below 76 or pulsing
 * (Player_StaminaLocked) or in its colour. */
void Hud_DrawHealthStamina(void)
{
    Player *pl = gHudState->player;
    u32 hpColor;
    u32 stColor;
    Gauge gauge;
    s16 kind;
    u8 pulse;
    f32 wave;
    s32 grey;
    s32 tint;

    Gs_SetState(RS_ZWRITE, 1);
    Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    hpColor = 0xFF10C010;
    stColor = 0xFFF0F000;
    kind = Area_Temperature(pl->area);
    switch (kind) {
    default:
        gHudState->staminaBlink = 0;
        gHudState->hpBlink = 0;
        break;
    case 0:
    case 1:
    case 3:
        hpColor = Hud_HealthColor(kind, pl);
        break;
    case 2:
    case 4:
        stColor = Hud_StaminaColor(kind, pl);
        hpColor = Hud_HealthBarColor(pl);
        break;
    }
    gauge.x = 119.0f;
    gauge.y = 21;
    gauge.len = 288.0f;
    gauge.max = 100;
    gauge.cur = pl->hpMax;
    gauge.color = 0xFF000000;
    Hud_DrawGaugeFrame(&gauge, 0);
    if (pl->hp > 0) {
        if (gHudState->hp < pl->hpRecoverable) {
            gauge.cur = pl->hpRecoverable;
            gauge.color = 0xFFC01010;
            Hud_DrawGaugeFill(&gauge, 0);
        }
        gauge.cur = gHudState->hp;
        gauge.color = hpColor;
        Hud_DrawGaugeFill(&gauge, 0);
    }
    gauge.y = 36;
    gauge.max = 300;
    gauge.cur = pl->staminaMax;
    gauge.color = 0xFF000000;
    Hud_DrawGaugeFrame(&gauge, 1);
    if (pl->stamina < 76) {
        pulse = (s16)(64.0f * Math_Sin(6.2831855f * ((gFrameTick % 45) * 8) / 360.0f)) + 64;
        gauge.color = pulse << 8 | 0xFFFF0000 | pulse;
    } else if (Player_StaminaLocked(pl) == 0) {
        gauge.color = stColor;
    } else {
        wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10));
        grey = (u8)((s8)(18.0f * wave) + 231);
        tint = grey << 16 | 0xFF000000 | grey << 8;
        gauge.color = (u8)((s8)(40.0f * wave) + 159) | tint;
    }
    gauge.cur = pl->stamina;
    Hud_DrawGaugeFill(&gauge, 1);
}

/* Returns 1 while the health bar pulses: the player carries special items,
 * has status icon 1, or Player_InPoisonArea says so. */
s32 Hud_HealthFlashes(Player *pl)
{
    if (Player_CountSpecialItems(pl) > 0) {
        return 1;
    }
    if (pl->statusIcons & 1) {
        return 1;
    }
    if (Player_InPoisonArea(pl) == 1) {
        return 1;
    }
    return 0;
}

extern s16 gHudGaugeFlashSpeeds[][5];
u8 Player_GetSkill(Player *p, u8 group);

/* The colour of gauge `kind` (0, 1 or 3) of p: pulsing at a speed from the skill of group 30
 * while p->unk870 is clear (or when Hud_HealthFlashes holds), else 0xFF10C010. */
u32 Hud_HealthColor(s16 kind, Player *pl)
{
    u32 color = 0xFF10C010;
    s16 speed = 0;
    u8 col;
    u8 lvl;
    u8 skill;
    f32 wave;

    if (pl->coolDrinkTimer == 0) {
        skill = Player_GetSkill(pl, 30);
        if (skill != 134) {
            switch (skill) {
            case 133:
                lvl = 0;
                break;
            case 132:
                lvl = 1;
                break;
            case 135:
                lvl = 3;
                break;
            case 136:
                lvl = 4;
                break;
            default:
                lvl = 2;
                break;
            }
            switch (kind) {
            case 0:
                lvl = col = 0;
                break;
            case 1:
                col = 0;
                break;
            case 3:
                col = 1;
                break;
            }
            speed = gHudGaugeFlashSpeeds[col][lvl];
        }
    }
    if (speed == 0 && Hud_HealthFlashes(pl) == 1) {
        speed = 256;
    }
    if (speed != 0) {
        gHudState->hpBlink += speed;
        wave = Math_Cos(0.0000958738f * gHudState->hpBlink);
        color = (((s8)(-28.0f * wave) + 45) << 16) | 0xFF000000 | (((s8)(15.0f * wave) + 177) << 8) | ((s8)(-72.0f * wave) + 88);
    } else {
        gHudState->hpBlink = 0;
    }
    return color;
}


/* The health bar's colour: green, or while Hud_HealthFlashes a colour that
 * pulses with hpBlink. */
u32 Hud_HealthBarColor(Player *pl)
{
    u32 color = 0xFF10C010;
    f32 wave;

    if (Hud_HealthFlashes(pl) == 1) {
        gHudState->hpBlink += 256;
        wave = Math_Cos(0.0000958738f * gHudState->hpBlink);
        color = (((s8)(-28.0f * wave) + 45) << 16) | 0xFF000000 | (((s8)(15.0f * wave) + 177) << 8) | ((s8)(-72.0f * wave) + 88);
    } else {
        gHudState->hpBlink = 0;
    }
    return color;
}

/* The colour of gauge `kind` (2 or 4) of p: pulsing at a speed from the skill of group 29 while
 * p->unk872 is clear, else 0xFFF0F000. */
u32 Hud_StaminaColor(s16 kind, Player *pl)
{
    u32 color = 0xFFF0F000;
    s16 speed = 0;
    u8 col;
    u8 lvl;
    u8 skill;
    f32 wave;

    if (pl->hotDrinkTimer == 0) {
        skill = Player_GetSkill(pl, 29);
        if (skill != 139) {
            switch (skill) {
            case 138:
                lvl = 0;
                break;
            case 137:
                lvl = 1;
                break;
            case 140:
                lvl = 3;
                break;
            case 141:
                lvl = 4;
                break;
            default:
                lvl = 2;
                break;
            }
            switch (kind) {
            case 2:
                col = 0;
                break;
            case 4:
                col = 1;
                break;
            }
            speed = gHudGaugeFlashSpeeds[col][lvl];
        }
    }
    if (speed != 0) {
        gHudState->staminaBlink += speed;
        wave = Math_Cos(0.0000958738f * gHudState->staminaBlink);
        color = (((s8)(58.0f * wave) + 182) << 16) | 0xFF000000 | (((s8)(102.0f * wave) + 138) << 8) | ((s8)(-21.0f * wave) + 21);
    } else {
        gHudState->staminaBlink = 0;
    }
    return color;
}

/* A gauge frame's texture piece: the st rectangle of its body (u0..v0) and end cap (u1, v1),
 * the body's height, cap's width and the frame's y offset. */
typedef struct GaugeFrame {
    f32 u0;             /* 0x00 */
    f32 v0;             /* 0x04 */
    f32 u1;             /* 0x08 */
    f32 v1;             /* 0x0C */
    u16 h;              /* 0x10 */
    u16 capw;           /* 0x12 */
    u16 dy;             /* 0x14 */
    u16 pad;
} GaugeFrame;

extern GaugeFrame gGaugeFrames[];

/* Draws the frame of gauge g: body pieces 45 pixels wide along its length, then the end cap. */
void Hud_DrawGaugeFrame(Gauge *gauge, s32 thin)
{
    GaugeFrame *frame = &gGaugeFrames[thin];
    u32 rest;
    u32 i;
    PrimSpriteF front;
    PrimSpriteF back;
    u32 n;
    f32 x;

    back.v[0].z = back.v[1].z = front.v[0].z = front.v[1].z = -0.9f;
    back.v[0].q = back.v[1].q = front.v[0].q = front.v[1].q = 1.0f;
    front.color = back.color = gauge->color;
    front.v[1].y = gauge->y + frame->h + frame->dy;
    front.v[0].y = front.v[1].y + frame->h;
    back.v[0].y = gauge->y;
    back.v[1].y = back.v[0].y + frame->h;
    front.st[0][1] = back.st[0][1] = frame->v0;
    front.st[1][1] = back.st[1][1] = frame->v0 + 0.00390625f * frame->h;
    n = 1.1295f * (gauge->len * ((f32)gauge->cur / (f32)gauge->max));
    rest = n % 45;
    front.st[0][0] = back.st[0][0] = frame->u0;
    front.st[1][0] = back.st[1][0] = 0.140625f + frame->u0;
    x = gauge->x;
    for (i = n / 45; i != 0; i--) {
        front.v[0].x = x;
        x += 45.0f;
        front.v[1].x = x;
        Prim_DrawSpriteF(&front);
        back.v[0].x = front.v[0].x;
        back.v[1].x = front.v[1].x;
        Prim_DrawSpriteF(&back);
    }
    front.v[0].x = x;
    front.v[1].x = x + rest;
    front.st[1][0] = frame->u0 + 0.003125f * rest;
    Prim_DrawSpriteF(&front);
    back.v[0].x = front.v[0].x;
    back.v[1].x = front.v[1].x;
    back.st[1][0] = front.st[1][0];
    Prim_DrawSpriteF(&back);
    back.v[0].x = 15.0f + back.v[1].x;
    back.v[1].y = back.v[0].y + frame->capw;
    back.st[0][0] = 0.046875f + frame->u1;
    back.st[1][0] = frame->u1;
    back.st[0][1] = frame->v1;
    back.st[1][1] = frame->v1 + 0.00390625f * frame->capw;
    Prim_DrawSpriteF(&back);
}

/* Draws the filled part of gauge g (3 or 5 pixels high). */
void Hud_DrawGaugeFill(Gauge *gauge, s32 thin)
{
    SpriteWords sprite;
    f32 w;

    if (gauge->cur <= 0 || gauge->max <= 0) {
        return;
    }
    w = 1.1295f * (gauge->len * ((f32)gauge->cur / (f32)gauge->max));
    if (thin == 0) {
        sprite.y = gauge->y + 6;
        sprite.h = 5;
    } else {
        sprite.y = gauge->y + 5;
        sprite.h = 3;
    }
    sprite.x = gauge->x;
    sprite.w = w;
    sprite.color = gauge->color;
    sprite.uv0 = TEX_UV(196, 4);
    sprite.uv1 = TEX_UV(198, 5);
    Sprite_Draw((struct Sprite *)&sprite);
}

extern u8 gSharpnessBarV[];
extern WeaponBoxHud gSharpnessGlowMasks;
extern s16 D_00430958[];
extern s16 D_0043095A[];
extern u8 gLongSwordGaugeSprites[];
extern u8 gWeaponBoxSprite[];

/* The weapon box: the sharpness bar of the local player's level (glowing when gSharpnessGlowMasks has a
 * mask for it), then the kind's extras (7: a gauge of +0x776, 8: Hud_DrawHornNotes). */
void Hud_DrawWeaponBox(void)
{
    Player *pl = gHudState->player;
    Sprite sprite;
    u8 sharpness;
    u32 mask;
    f32 wave;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    sharpness = Player_GetSharpnessLevel(pl);
    sprite.x = 112;
    sprite.y = 49;
    sprite.w = 80;
    sprite.h = 14;
    sprite.color = 0xFFFFFFFF;
    sprite.u0 = 192;
    sprite.u1 = 255;
    sprite.v0 = gSharpnessBarV[sharpness];
    sprite.v1 = sprite.v0 + 14;
    Sprite_Draw((struct Sprite *)&sprite);
    mask = gSharpnessGlowMasks.glowMasks[sharpness];
    if (mask != 0) {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
        wave = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x7F) << 9));
        sprite.color = ((((s8)(127.0f * wave) + 127) << 24) | 0xFFFFFF) & mask;
        sprite.x = 141;
        sprite.w = 51;
        sprite.u0 = 215;
        sprite.v0 -= 42;
        sprite.v1 -= 42;
        Sprite_Draw((struct Sprite *)&sprite);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    }
    switch (pl->weaponType) {
    case 7:
        if (pl->swordGauge > 0) {
            sprite.x = D_00430958[0];
            sprite.y = D_0043095A[0] + 4;
            sprite.w = pl->swordGauge * 117 / 100;
            sprite.h = 2;
            sprite.color = 0xFFFF2020;
            *(s32 *)&sprite.u0 = TEX_UV(196, 4);
            *(s32 *)&sprite.u1 = TEX_UV(198, 5);
            Sprite_Draw((struct Sprite *)&sprite);
        }
        Sprite_DrawList((Sprite *)gLongSwordGaugeSprites, 8);
        break;
    case 8:
        Hud_DrawHornNotes(pl);
    default:
        Sprite_DrawList((Sprite *)gWeaponBoxSprite, 1);
        break;
    }
}



extern u8 gHornNoteSprites[];
extern s16 gHornNoteYs[];

/* y0 caches the current row's height for the difference (the sum reads the table again: a local
 * that caches a field for some of its uses), and the interpolation reads the table as u16, as the
 * function's own x = (u16)(...) does. Without y0 the anim and difference registers swap.
 * FAKEMATCH: the history index is cast to u32, which changes only the order of the address sum's
 * operands, to the game's. */
void Hud_DrawHornNotes(Player *pl)
{
    SpriteWords sprite;
    u16 noteY;
    s32 i;
    s32 x;
    f32 scale;
    u8 note;

    Sprite_DrawList((Sprite *)gHornNoteSprites, 2);
    note = pl->hornNoteSlot;
    if (pl->hornNotes[note] == 0) {
        gHudState->hornLast = note;
        gHudState->hornAnim = 0;
        gHudState->hornHist[4] = 0;
        gHudState->hornHist[3] = 0;
        gHudState->hornHist[2] = 0;
        gHudState->hornHist[1] = 0;
        gHudState->hornHist[0] = 0;
        return;
    }
    if (gHudState->hornLast != note) {
        gHudState->hornLast = note;
        gHudState->hornAnim = 6;
        gHudState->hornHist[4] = gHudState->hornHist[3];
        gHudState->hornHist[3] = gHudState->hornHist[2];
        gHudState->hornHist[2] = gHudState->hornHist[1];
        gHudState->hornHist[1] = gHudState->hornHist[0];
        gHudState->hornHist[0] = pl->hornNotes[pl->hornNoteSlot];
    }
    sprite.uv0 = TEX_UV(21, 157);
    sprite.uv1 = TEX_UV(37, 172);
    if (gHudState->hornAnim > 0) {
        sprite.w = 20;
        sprite.h = 15;
        x = (u16)(gHudState->hornAnim * -24 / 6) + 240;
        for (i = 1; i < 5; i++) {
            if (gHudState->hornHist[(u32)i] == 0) {
                break;
            }
            x += 24;
            noteY = (u16)gHornNoteYs[i];
            sprite.x = x;
            sprite.y = (u16)gHornNoteYs[i] + gHudState->hornAnim * ((u16)gHornNoteYs[i - 1] - noteY) / 6;
            sprite.color = gHornNoteColors[gHudState->hornHist[(u32)i]];
            if (i == 4) {
                sprite.color &= 0xFFFFFF;
                sprite.color |= gHudState->hornAnim * 255 / 6 << 24;
            }
            Sprite_Draw((struct Sprite *)&sprite);
        }
        if (gHudState->hornAnim > 3) {
            scale = 1.0f + 0.41666666f * (gHudState->hornAnim - 3);
        } else {
            scale = 1.0f;
        }
        sprite.w = 20.0f * scale;
        sprite.h = 15.0f * scale;
        sprite.x = 250 - sprite.w / 2;
        sprite.y = gHornNoteYs[0] + 7 - sprite.h / 2;
        sprite.color = gHornNoteColors[gHudState->hornHist[0]];
        Sprite_Draw((struct Sprite *)&sprite);
        gHudState->hornAnim--;
    } else {
        sprite.x = 240;
        sprite.w = 20;
        sprite.h = 15;
        for (i = 0; i < 4; i++) {
            if (gHudState->hornHist[(u32)i] == 0) {
                break;
            }
            sprite.y = gHornNoteYs[i];
            sprite.color = gHornNoteColors[gHudState->hornHist[(u32)i]];
            Sprite_Draw((struct Sprite *)&sprite);
            sprite.x += 24;
        }
    }
}

#undef NH

/* A textured triangle with its texel pairs as words (Prim_DrawTexTri). */
typedef struct TriW {
    s16 x0, y0;         /* 0x00 */
    s16 x1, y1;         /* 0x04 */
    s16 x2, y2;         /* 0x08 */
    u32 color;          /* 0x0C */
    s32 uv0;            /* 0x10 */
    s32 uv1;            /* 0x14 */
    s32 uv2;            /* 0x18 */
} TriW;

struct TexRect;
extern u8 D_0043189C[];
extern s16 D_0043189E;
extern s16 gAimViewSizes[];
extern s16 D_004318A2[];
extern s16 gHudStepPulse[][2];
extern s16 D_00431876[];
extern s16 D_0043187A[];
extern s16 D_0043188A[];
extern s16 D_0043188E[];
extern AimViewSprites gAimViewSprites;
extern s16 gAimViewTris[];
extern s16 D_004318D4[];
extern s16 D_004318EC[];
extern s16 D_00431908[];
extern s16 D_00431924[];

/* The aim view (while Player_IsAiming): the zoom bar of five sprites with a marker at the
 * zoom's position, and the scope's frame, two sprites and two quads placed between their two
 * positions of gHudStepPulse by aimFade (0..3; they pulse at 0), a sound when aimStart is set. */
void Hud_DrawAimView(void)
{
    SpriteWords sprite;
    TriW tri;
    f32 pos;
    f32 wave;
    u8 show;
    u8 bar;
    s32 plWord;
    s32 i;
    s16 (*q)[2];
    s32 k;
    s16 restK;

    plWord = (s32)gHudState->player;
    if (CamCtl_IsAiming((Player *)plWord) == 0) {
        gHudState->aimKind = -1;
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    gHudState->aimKind = CamAim_GetState(plWord, &show, &bar, &pos);
    if (bar != 0) {
        Sprite_DrawList((Sprite *)D_0043189C, 2);
        sprite.x = 263;
        sprite.y = D_0043189E;
        sprite.w = gAimViewSizes[0];
        sprite.h = D_004318A2[0];
        sprite.color = -1;
        sprite.uv0 = TEX_UV(0, 196);
        sprite.uv1 = TEX_UV(20, 216);
        i = 5;
        do {
            Sprite_Draw((struct Sprite *)&sprite);
            sprite.x += 24;
        } while (--i != 0);        sprite.x = 246.0f + 142.0f * pos;
        sprite.y = 349;
        sprite.w = 12;
        sprite.h = 12;
        sprite.color = 0xFFFF0000;
        sprite.uv0 = TEX_UV(97, 143);
        sprite.uv1 = TEX_UV(106, 154);
        TexRect_Draw((struct TexRect *)&sprite, 2);
    }
    if (show != 0) {
        if (gHudState->aimFade < 4) {
            k = (s16)gHudState->aimFade;
            restK = 4 - k;
            if (gHudState->aimFade == 0) {
                wave = Math_Cos(0.0000958738f * (u16)((gHudState->aimBlink & 0x1F) << 11));
                sprite.color = ((s8)(94.0f * wave) + 161) << 24 | 0xFF0000;
                (gHudState->aimBlink)++;
            } else {
                sprite.color = 0xFFFF0000;
                gHudState->aimBlink = 0;
            }
            q = gHudStepPulse;
            sprite.x = (q[0][0] * k + q[0][1] * restK) >> 2;
            sprite.y = D_00431876[0];
            sprite.w = 25;
            sprite.h = D_0043187A[0];
            sprite.uv0 = TEX_UV(86, 143);
            sprite.uv1 = TEX_UV(106, 154);
            Sprite_Draw((struct Sprite *)&sprite);
            sprite.x = (q[1][0] * k + q[1][1] * restK) >> 2;
            sprite.y = D_0043188A[0];
            sprite.w = 25;
            sprite.h = D_0043188E[0];
            sprite.uv0 = TEX_UV(105, 143);
            sprite.uv1 = TEX_UV(85, 154);
            Sprite_Draw((struct Sprite *)&sprite);
            q++;
            q++; /* two steps: MWCC folds `q += 2` into the offsets */
            tri.color = sprite.color;
            tri.x0 = tri.x2 = gAimViewTris[0];
            tri.x1 = D_004318D4[0];
            tri.y0 = tri.y1 = (q[0][0] * k + q[0][1] * restK) >> 2;
            tri.y2 = tri.y0 + 12;
            tri.uv0 = TEX_UV(97, 154);
            tri.uv1 = TEX_UV(97, 143);
            tri.uv2 = 0x9A006A;
            Prim_DrawTexTri((struct PrimTexTri *)&tri);
            tri.x0 = tri.x1;
            tri.y0 = tri.y2;
            tri.uv0 = TEX_UV(106, 143);
            Prim_DrawTexTri((struct PrimTexTri *)&tri);
            tri.x0 = D_00431908[0];
            tri.y2 = (q[1][0] * k + q[1][1] * restK) >> 2;
            tri.y0 = tri.y1 = tri.y2 + 12;
            tri.uv0 = TEX_UV(97, 154);
            Prim_DrawTexTri((struct PrimTexTri *)&tri);
            tri.x0 = tri.x1;
            tri.y0 = tri.y2;
            tri.uv0 = TEX_UV(106, 143);
            Prim_DrawTexTri((struct PrimTexTri *)&tri);
            if ((u8)gHudState->aimStart != 0) {
                Sound_PlaySeFlat(0, 32, 0);
            }
        }
        Sprite_DrawList(gAimViewSprites.views, 3);
        Prim_DrawTexTri((struct PrimTexTri *)gAimViewTris);
        Prim_DrawTexTri((struct PrimTexTri *)D_004318EC);
        Prim_DrawTexTri((struct PrimTexTri *)D_00431908);
        Prim_DrawTexTri((struct PrimTexTri *)D_00431924);
    }
}

s16 Player_IsState(void *obj, u8 a, u8 b);

/* Draws the gunners' HUD (the two bowguns and the gunlance): the clip
 * (Hud_DrawBowgunClip) and the blinking prompts (BLINK_ICON_*) while the
 * weapon is out. */
void Hud_DrawBowgun(void)
{
    Player *pl = gHudState->player;

    if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN || pl->weaponType == WEAPON_GUNLANCE) {
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
        Tex_Prepare(1, 326);
        Tex_Bind(326);
        if (pl->weaponType == WEAPON_GUNLANCE) {
            Hud_DrawBowgunClip(pl, 2);
            if (pl->weaponOut != 0) {
                if (pl->roundsLoaded <= 0) {
                    Hud_DrawBlinkIcon(BLINK_ICON_RELOAD);
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, GLANCE_RELOAD) != 0) {
                    Hud_DrawBlinkIcon(BLINK_ICON_RELOADING);
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, GLANCE_HOP_RELOAD) != 0) {
                    Hud_DrawBlinkIcon(BLINK_ICON_RELOADING);
                }
            }
            return;
        }
        if (pl->ammoCount == 0) {
            if (pl->weaponOut != 0 && Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 1) != 255) {
                Hud_DrawBlinkIcon(BLINK_ICON_NEXT_AMMO);
            }
            return;
        }
        Hud_DrawBowgunClip(pl, gHudState->aimKind == 0);
        if (pl->roundsLoaded <= 0 && pl->weaponOut != 0) {
            Hud_DrawBlinkIcon(BLINK_ICON_RELOAD);
        }
        if (pl->motionId[0] == 1404) /* the bowguns' reload motion */ {
            Hud_DrawBlinkIcon(BLINK_ICON_RELOADING);
        }
    }
}

extern IconDef gBlinkIconDefs[];

/* Draws prompt `icon` (BLINK_ICON_*, gBlinkIconDefs) pulsing every 32
 * frames: in the weapon box, or near the centre while the aim view is
 * up (further right for the gunlance). */
void Hud_DrawBlinkIcon(s32 icon)
{
    SpriteWords sprite;
    IconDef *def = &gBlinkIconDefs[icon];
    u8 alpha;

    alpha = (s8)(96.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x1F) << 11))) + 159;
    if (gHudState->aimKind != 0) {
        sprite.x = 120;
        sprite.y = 58;
        sprite.color = (alpha << 24) | 0xFFFFFF;
        if ((gHudState->player)->weaponType == WEAPON_GUNLANCE) {
            sprite.x += 100;
        }
    } else {
        sprite.x = def->x;
        sprite.y = 213;
        sprite.color = 0xFF000000 | (alpha << 16) | (alpha << 8) | alpha;
    }
    sprite.w = def->w;
    sprite.h = def->h;
    sprite.uv0 = def->uv0;
    sprite.uv1 = def->uv1;
    Sprite_Draw((struct Sprite *)&sprite);
}

/* The bowgun's clip: p->clipSize slots, the first p->roundsLoaded of them loaded (kind 0-2: the
 * position). */
void Hud_DrawBowgunClip(Player *pl, u8 kind)
{
    SpriteWords sprite;
    u32 i;

    sprite.color = 0xFFFFFFFF;
    switch (kind) {
    case 0:
        sprite.x = 120;
        sprite.y = 50;
        break;
    case 1:
        sprite.x = 318 - pl->clipSize * 15 / 2;
        sprite.y = 291;
        break;
    case 2:
        if (Player_GetSharpnessLevel(pl) == 0) {
            sprite.color = 0xFFFF0000;
        }
        sprite.x = 220;
        sprite.y = 50;
        break;
    }
    sprite.w = 15;
    sprite.h = 20;
    for (i = 0; i < pl->clipSize; i++) {
        if (i < pl->roundsLoaded) {
            sprite.uv0 = TEX_UV(216, 237);
            sprite.uv1 = TEX_UV(228, 255);
        } else {
            sprite.uv0 = TEX_UV(228, 237);
            sprite.uv1 = TEX_UV(240, 255);
        }
        Sprite_Draw((struct Sprite *)&sprite);
        sprite.x += sprite.w;
    }
}

/* Returns 1 if the hunter knows where monster em is: it is painted, or the
 * local player has a detect effect running or the higher detect skill. */
s32 Map_MonsterLocated(Em *em)
{
    Player *pl;

    if (em->painted != 0) {
        return 1;
    }
    pl = gHudState->player;
    if ((*(u16 *)&pl->detectTimer | *(u16 *)&pl->actDetectTimer) != 0) {
        return 1;
    }
    return Player_HasSkill((Player *) pl, 162U);
}

/* Starts the map's glow (glowTimer) when monster em is located and the map
 * shows. */
void Hud_TriggerGlow(Em *em)
{
    if ((Map_MonsterLocated(em) != 0) && (gHudState->mode == 0) && (gHudState->mapOff == 0) && (gHudState->hidden == 0)) {
        gHudState->glowTimer = 26;
    }
}

/* Draws the map's glow while glowTimer runs. */
void Hud_DrawGlowEffect(void)
{
    f32 frames;
    u8 glowTimer;

    if (gHudState->glowTimer != 0) {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Tex_Prepare(1, 324);
        Tex_Bind(324);
        glowTimer = gHudState->glowTimer;
        frames = (f32) (u32) glowTimer;
        Hud_DrawGlow(512, 172, 0xFFFFFF, 120.0f, frames);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    }
}

/* A glow of size `size` fading with t (0-25) around x, y: four quarter sprites. */
void Hud_DrawGlow(s16 x, s16 y, s32 color, f32 size, f32 frames)
{
    Sprite sprite;
    u8 alpha;
    s16 w;

    alpha = 51.0f * Math_Sqrtf(frames);
    w = size * (0.291667014f + 0.1180555f * Math_Sqrtf(25.0f - frames));
    sprite.color = (color & 0xFFFFFF) | (alpha << 24);
    sprite.w = w;
    sprite.h = w;
    sprite.x = x;
    sprite.y = y;
    *(s32 *)&sprite.u0 = TEX_UV(225, 193);
    *(s32 *)&sprite.u1 = TEX_UV(255, 223);
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.y = y - sprite.h;
    sprite.v0 = 223;
    sprite.v1 = 193;
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.x = x - sprite.w;
    sprite.u0 = 255;
    sprite.u1 = 225;
    Sprite_Draw((struct Sprite *)&sprite);
    sprite.y = y;
    sprite.v0 = 193;
    sprite.v1 = 223;
    Sprite_Draw((struct Sprite *)&sprite);
}

extern Frame gSupplyBoxFrame;
extern char *gSupplyPouchFullText;
extern char *gSupplyInfiniteFmt;

/* The x of supply box slot i's column. */
#define SUPPLY_X(i) (313.0f + 36.0f * ((i) & 7))

/* Kept for the match: the moving item's alpha reuses the u16 local item; with a u32 local of its
 * own, the compiler keeps the Sprite fields' addresses in other registers than the game's. */
/* The supply box window, while it is open (Player.supplyBoxOpen): the slots' frames and items (a slot
 * taken is hidden, and so are the quest goals' slots until they are done), the item being taken
 * moving to the pouch, the cursor, then the selected item's name and count, and why it can't be
 * taken (the pouch is full, or has room for fewer). */
void SupplyBox_Draw(void)
{
    Player *pl = gHudState->player;
    SceneState *scene = &gScene;
    PrimLine rect;
    u32 taken[2];
    Sprite sprite;
    char buf[32];
    char *msg;
    s16 i;
    s32 k;
    f32 progress;
    f32 frames;
    f32 size;
    u8 alpha;
    u16 item;
    s16 n;
    s16 x;
    u8 cur;

    if (pl->supplyBoxOpen == 0) {
        return;
    }
    if (gQuest.timedEvent == 0) {
        taken[0] = gScene.supplyTaken[0];
        taken[1] = gScene.supplyTaken[1];
    } else {
        taken[0] = gScene.supplyTaken[0] | 0xFFFFFF;
        taken[1] = gScene.supplyTaken[1] | 0xFFFFFF00;
    }
    if (QuestGoal_IsDone(1) != 1) {
        taken[0] |= 0xFF000000;
    }
    if (QuestGoal_IsDone(2) != 1) {
        taken[1] |= 0xFF;
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    WindowFrame_Draw(&gSupplyBoxFrame, 0);
    rect.color = 0xFF200000;
    for (i = 0; i < 40; i++) {
        rect.x0 = (s16)SUPPLY_X(i) + 4;
        rect.x1 = rect.x0 + 32;
        rect.y0 = (i >> 3) * 32 + 96;
        rect.y1 = rect.y0 + 28;
        Prim_FillRect((struct PrimLine *)&rect);
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    sprite.w = 40;
    sprite.h = 32;
    for (i = 0; i < 40; i++) {
        if (!(taken[i >> 5] & (1 << (i & 31)))) {
            if (scene->supply[i].item != 0 && scene->supply[i].count != 0) {
                ItemData *itemData = &gItemData[scene->supply[i].item];

                if (itemData->icon != 255) {
                    k = itemData->icon + 1;
                    sprite.x = SUPPLY_X(i);
                    sprite.y = (i >> 3) * 32 + 94;
                    sprite.u0 = (k & 7) * 32 + 1;
                    sprite.v0 = (k >> 3) * 32 + 1;
                    sprite.u1 = sprite.u0 + 30;
                    sprite.v1 = sprite.v0 + 30;
                    sprite.color = gItemColors[itemData->color];
                    Sprite_Draw((struct Sprite *)&sprite);
                }
            }
        }
    }
    if (gHudState->supplyMoveTimer > 0) {
        progress = (f32)gHudState->supplyMoveTimer / (f32)gHudState->supplyMoveFrames;
        sprite.x = SUPPLY_X(gHudState->supplyMoveSlot) * progress + 315.0f * (1.0f - progress);
        sprite.y = progress * ((gHudState->supplyMoveSlot >> 3) * 32 + 94) + 284.0f * (1.0f - progress);
        k = gItemData[gHudState->supplyMoveItem].icon + 1;
        sprite.u0 = (k & 7) * 32 + 1;
        sprite.v0 = (k >> 3) * 32 + 1;
        sprite.u1 = sprite.u0 + 30;
        sprite.v1 = sprite.v0 + 30;
        sprite.color = gItemColors[gItemData[gHudState->supplyMoveItem].color] & 0xFFFFFF;
        item = 45.0f * Math_Sqrtf(gHudState->supplyMoveTimer);
        sprite.color |= (item & 0xFF) << 24;
        Sprite_Draw((struct Sprite *)&sprite);
    }
    sprite.x = SUPPLY_X(pl->menuCursor);
    sprite.y = (pl->menuCursor >> 3) * 32 + 94;
    *(s32 *)&sprite.u0 = 0;
    *(s32 *)&sprite.u1 = TEX_UV(32, 32);
    sprite.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&sprite);
    if (gHudState->supplyMsgTimer > 0) {
        frames = gHudState->supplyMsgTimer & 0x1F;
        alpha = 45.0f * Math_Sqrtf(frames);
        size = 32.0f * (1.0f + 0.17960416f * Math_Sqrtf(31.0f - frames));
        sprite.w = 1.25f * size;
        sprite.h = size;
        sprite.x = (s16)SUPPLY_X(gHudState->supplyFlashSlot) - ((sprite.w - 40) >> 1);
        sprite.y = (gHudState->supplyFlashSlot >> 3) * 32 + 94 - ((sprite.h - 32) >> 1);
        sprite.color = ((alpha & 0xFF) << 24) | 0xFFFFFF;
        Sprite_Draw((struct Sprite *)&sprite);
    }
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    sprite.x = 315;
    sprite.y = 284;
    sprite.w = 50;
    sprite.h = 50;
    *(s32 *)&sprite.u0 = TEX_UV(120, 236);
    *(s32 *)&sprite.u1 = TEX_UV(140, 256);
    sprite.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&sprite);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(324, 70);
    Text_Draw(gSupplySelectText[0]);
    cur = pl->menuCursor;
    item = (taken[cur >> 5] & (1 << (cur & 31))) ? 0 : scene->supply[cur].item;
    n = scene->supply[cur].count;
    if (item == 0 || n == 0) {
        sprintf(buf, gHudFmtString, gItemNames[0]);
        Font_SetPos(459 - Str_Length(buf) * 9 / 2, 266);
        Text_Draw(buf);
        item = 0;
    } else {
        switch (gItemData[item].max) {
        case 1:
            sprintf(buf, (char *)gItemNames[item]);
            break;
        case 255:
            sprintf(buf, gSupplyInfiniteFmt, gItemNames[item]);
            break;
        default:
            sprintf(buf, gSupplyCountFmt[0], gItemNames[item], n);
            break;
        }
        Font_SetPos(459 - Str_Length(buf) * 9 / 2, 266);
        Text_Draw(buf);
        x = -1;
        k = Pouch_Space(pl, item);
        if (k < 0) {
            x = 396;
            msg = gSupplyPouchFullText;
        } else if (k < n) {
            x = 378;
            msg = gSupplyCannotCarryText[0];
        }
        if (x > 0) {
            Font_SetPos(x, 296);
            Font_SetColor(0x807F212E);
            Text_Draw(msg);
        }
    }
    HelpWindow_DrawLine(HELP_PAGE_ITEMS, HELP_ITEM_LINE(item));
}

/* Starts the supply box's flash on the player's supply box cursor (the
 * byte at 0x81B of the player it is given). */
void Hud_StartSupplyMsg(u8 *player)
{
    gHudState->supplyMsgTimer = 31;
    gHudState->supplyFlashSlot = player[2075];
}

/* Starts the icon of `item`, taken from supply box slot `slot`, moving to
 * the pouch: its frames are a tenth of the distance in pixels. */
void SupplyBox_StartMove(u16 item, u8 slot)
{
    f32 dx;
    f32 dy;
    u8 frames;

    gHudState->supplyMoveItem = item;
    gHudState->supplyMoveSlot = slot;
    dx = 313.0f + 36.0f * (gHudState->supplyMoveSlot & 7) - 315.0f;
    dy = (gHudState->supplyMoveSlot >> 3) * 32 - 190;
    frames = 0.1f * Math_Sqrtf(dx * dx + dy * dy);
    gHudState->supplyMoveTimer = frames;
    gHudState->supplyMoveFrames = frames;
}

extern ListWin gTentMenuPanel;
extern Panel gSupplyBoxTextBox;

/* Draws the camp's tent menu while the player has it open
 * (Player.deliveryMenu): page 0 the list (gTentMenuLabels: a set of
 * three by online or offline, then by whether the quest is over or its
 * sub-target is done), dimmed while the menu waits; page 1 the yes/no
 * question for entry 1 (the sub-target) or 2 (the quest menu's). */
void TentMenu_Draw(void)
{
    Player *pl = gHudState->player;
    s32 labelSet;
    s32 helpLine;
    s32 shownCursor;
    s32 offline;
    char *text;

    if (pl->deliveryMenu == 0) {
        return;
    }
    switch (pl->menuPage) {
    case 0:
        helpLine = pl->menuCursor;
        labelSet = 0;
        if (Game_IsOnline() != 1) {
            labelSet += 3;
            helpLine += 3;
        }
        if (pl->menuDelay != 0) {
            gTentMenuPanel.color = 10;
            shownCursor = -1;
        } else {
            shownCursor = pl->menuCursor;
            gTentMenuPanel.color = 0;
            if (Quest_PhaseIsOver(0) == 1) {
                labelSet += 12;
            } else if (Quest_CheckSubClearMode() != 1) {
                labelSet += 6;
            }
        }
        gTentMenuPanel.items = &gTentMenuLabels[labelSet];
        ListPanel_DrawDim(&gTentMenuPanel, 0, shownCursor);
        HelpWindow_DrawLine(8, helpLine);
        break;
    case 1:
        offline = Game_IsOnline() != 1;
        switch (pl->menuCursor) {
        default:
            return;
        case 1:
            text = gSubTargetHelpText[offline];
            break;
        case 2:
            text = gQuestMenuHelpTexts[offline];
            break;
        }
        Window_DrawTextB2((struct Window *)&gSupplyBoxTextBox, (u8 *)((s32)text));
        Font_SetPos(399, 322);
        Text_Printf(gYesNoTexts[pl->yesNoCursor]);
        break;
    }
}

extern char *gNotDeliveryText;
extern ListPanel gDeliveryPanel;
extern ListPanel gDeliveryPanel2;
extern Panel gDeliveryTextBox;

/* Draws the delivery page of the quest menu (page 2 first shows the goal list, page 1 the
 * Delivery_DrawCount part): the title and, for each of the three targets, its label and the item
 * with the count wanted, coloured by how many the pouch holds. */
void QuestMenu_DrawDelivery(void)
{
    Player *pl = gHudState->player;
    ItemStack stack;
    char buf[76];
    s16 i;
    char *cursor;
    s8 n;
    s16 have;

    if (pl->deliveryOpen == 0) {
        return;
    }
    switch (pl->menuPage) {
    case 2:
        ListPanel_Draw(&gDeliveryPanel, NULL, NULL, gHudState->yesNo);
        Quest_GetDeliveryTarget(pl, pl->menuCursor, &stack);
        if (pl->menuCursor == 0 && stack.count == gHudState->count && QuestGoal_AllSubDone() == 0) {
            Window_DrawTextB2((struct Window *)&gDeliveryTextBox, (u8 *)((s32)gDeliveryHelpText[0]));
        }
    case 1:
        Delivery_DrawCount();
    case 0:
        ListPanel_Draw(&gDeliveryPanel2, NULL, NULL, pl->menuCursor + 1);
        Font_SetSpacing(18, 18);
        Font_SetPos(358, 80);
        Font_SetColor(0x807F7F28);
        Text_Printf(gHudFmtString, gDeliveryMenuTexts[0]);
        for (i = 0; i < 3; i++) {
            Font_SetPos(358, (i + 1) * 22 + 80);
            Font_SetColor(0x807F7F7F);
            cursor = buf;
            n = 0;
            n += (s8)sprintf(cursor, gHudFmtString, gDeliveryMenuTexts[i + 1]);
            switch (Quest_GetDeliveryTarget(&gPlayers[gScene.localPlayer], i, &stack)) {
            case 0:
                sprintf(cursor + n, gHudFmtString, gNotDeliveryText);
                Font_SetColor(0x80505050);
                break;
            case 1:
                sprintf(cursor + n, gHudFmtString, gDeliveryDoneText[0]);
                Font_SetColor(0x802B7F7F);
                break;
            case 2:
                n += (s8)sprintf(cursor + n, gHudFmtString, gItemNames[stack.item]);
                if (stack.count > 1) {
                    sprintf(cursor + n, TXT(0x0046D100) /* a full-width multiplication sign, then the count */, stack.count);
                }
                have = Pouch_Count(pl, stack.item);
                if (have <= 0) {
                    Font_SetColor(0x80505050);
                } else if (have >= stack.count) {
                    Font_SetColor(0x807F7F7F);
                } else {
                    Font_SetColor(0x807F5230);
                }
                break;
            }
            Text_Printf(gHudFmtString, buf);
        }
        break;
    }
}

extern ListWin gEquipStatusPanel;

/* Draws the status page's equipment window: the six slots
 * (PersonalStatus_DrawEquipment), the cursor's bar pulsing over the
 * slot picked, and the item in that slot (EquipWindow_DrawLocal) at the
 * level page HudState.equipCursor. */
void EquipStatus_Draw(void)
{
    EquipItem item;
    PrimLine line;
    Player *pl;
    EquipItem *equip;

    pl = gHudState->player;
    ListPanel_DrawDim(&gEquipStatusPanel, gEquipStatusTitle[0], -1);
    PersonalStatus_DrawEquipment(pl);
    line.x0 = 376;
    line.x1 = 614;
    line.y0 = gHudState->pageCursor * 32 + 85;
    line.y1 = line.y0 + 32;
    line.color = (((s8)(48.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10))) + 191) << 24) | 0xA9182;
    Prim_FillRect((struct PrimLine *)&line);
    equip = Player_GetEquipBySlot(&item, gHudState->pageCursor, pl);
    EquipWindow_DrawLocal(equip, 306, 298, gHudState->equipCursor, 0);
}

extern Sprite gBowgunAmmoSprite;
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);

/* The bowgun's loaded ammo: the tens as a number, the ones as rounds. */
void Hud_DrawBowgunAmmo(void)
{
    SpriteWords sprite;
    char buf[8];
    Player *pl = gHudState->player;
    u8 n;
    u16 item;
    u8 tens;
    s32 ones;
    u32 color;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    if (pl->coatingSlot != 255) {
        n = pl->pouch[pl->coatingSlot].count;
        item = pl->pouch[pl->coatingSlot].item;
        color = gItemColors[gItemData[item].color];
        ones = n % 10;
        sprite.x = 120;
        sprite.y = 52;
        sprite.w = 31;
        sprite.h = 21;
        sprite.uv0 = TEX_UV(232, 42);
        sprite.uv1 = TEX_UV(256, 63);
        tens = n / 10;
        sprite.color = tens != 0 ? color : 0xFF808080;
        Sprite_Draw((struct Sprite *)&sprite);
        Font_SetSpacing(18, 18);
        sprintf(buf, gAmmoTensFmt[0], tens);
        Text_DrawShadow(152, 55, 1, 0, buf);
        sprite.x = 180;
        sprite.y = 52;
        sprite.w = 20;
        sprite.h = 21;
        sprite.uv0 = TEX_UV(4, 173);
        sprite.uv1 = TEX_UV(20, 194);
        sprite.color = color;
        while (ones != 0) {
            Sprite_Draw((struct Sprite *)&sprite);
            sprite.x += 20;
            ones--;
        }
    } else {
        Sprite_Draw((struct Sprite *)&gBowgunAmmoSprite);
    }
}

extern Panel gPauseTextBox;

/* Draws the pause window, sliding with gPauseWinFade. */
void Pause_DrawWindow(void)
{
    s16 pos[2];

    if (gPauseWinStep != 0) {
        pos[0] = 0;
        pos[1] = gPauseWinFade * -56;
        Window_DrawText(&gPauseTextBox, (s16 *)pos, (u8 *)gPausedText[0]);
    }
}

/* Saves (and the next restores) the render states RS_ZWRITE and RS_ZTEST around the
 * HUD's drawing. */
void Hud_SaveGsState(void)
{
    gHudState->gsSaveZWrite = Gs_GetState(RS_ZWRITE);
    gHudState->gsSaveZTest = Gs_GetState(RS_ZTEST);
}

/* Puts back the render states RS_ZWRITE and RS_ZTEST that Hud_SaveGsState kept
 * (the depth test and the alpha test's mode, render_state.h). */
void Hud_RestoreGsState(void)
{
    Gs_SetState(RS_ZWRITE, gHudState->gsSaveZWrite);
    Gs_SetState(RS_ZTEST, gHudState->gsSaveZTest);
}

/* The HUD's first draw layer: the versus monsters' bars (not while
 * paused). */
void Hud_DrawLayer0(void)
{
    Hud_SaveGsState();
    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Text_SetSize(6);
    Font_SetPal(0);
    if (Game_PauseState() == 0) {
        Hud_DrawVersusMonsterBars();
    }
    Hud_RestoreGsState();
}

/* The HUD's first draw hook (quests and offline): unless the game is
 * paused, the aim view, then with the cockpit shown the clock, the
 * weapon's box (the gunners' ammo and clip), the party's status and the
 * players' names; then the pause window. The draw hooks keep the render
 * states they change (Hud_SaveGsState). */
void Hud_DrawLayer1(void)
{
    u8 weapon;

    Hud_SaveGsState();
    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Text_SetSize(0);
    Font_SetPal(0);
    if (Game_PauseState() == 0) {
        Hud_DrawAimView();
        if (gHudState->hidden == 0) {
            Hud_DrawClock();
            weapon = gHudState->player->weaponType;
            switch (weapon) {
            case WEAPON_BOW:
                Hud_DrawBowgunAmmo();
                break;
            case WEAPON_HEAVY_BOWGUN:
            case WEAPON_LIGHT_BOWGUN:
                Hud_DrawBowgun();
                break;
            case WEAPON_GUNLANCE:
                Hud_DrawBowgun();
                
            default:
                Hud_DrawWeaponBox();
                break;
            }
            Hud_DrawPartyStatus();
            Hud_DrawPlayerNames();
        } else if (gHudState->aimKind == 0) {
            Hud_DrawBowgun();
        }
    }
    Pause_DrawWindow();
    Hud_RestoreGsState();
}

/* The second draw hook: a message popup, else while chatting the chat
 * log and the map, else the pouch-full dialog, the field menu, or the
 * map (unless a supply box, the delivery or the tent menu is open) and
 * the item box. */
void Hud_DrawLayer2(void)
{
    Player *pl;

    Hud_SaveGsState();
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Text_SetSize(1);
    Font_SetPal(0);
    if (gHudState->popupOpen != 0) {
        MsgEvent_DrawPopup();
    } else if (gChatInputMode != 0) {
        Chat_DrawHudLog();
        Hud_DrawMap();
    } else if (gHudState->dropState != DROP_NONE) {
        Hud_DrawItemWindow();
    } else if (gHudState->mode != 0) {
        FieldMenu_Draw();
    } else {
        pl = gHudState->player;
        if ((pl->deliveryOpen | (pl->supplyBoxOpen | pl->deliveryMenu)) == 0) {
            Hud_DrawMap();
            Hud_DrawPouchItem();
        }
    }
    Hud_RestoreGsState();
}

/* The third draw hook: with no popup, the button icons, the supply box,
 * the tent menu and the delivery (when no menu is open), a notice or the
 * chat lines, the menus' fades and the software keyboard's box. */
void Hud_DrawLayer3(void)
{
    Hud_SaveGsState();
    Text_SetSize(2);
    Font_SetPal(0);
    if (gHudState->popupOpen == 0) {
        Hud_DrawButtonIcons();
        if (gChatInputMode == 0 && gHudState->dropState == DROP_NONE && gHudState->mode == 0) {
            SupplyBox_Draw();
            TentMenu_Draw();
            QuestMenu_DrawDelivery();
        }
        if (gNoticeFlags != 0) {
            Notice_Draw();
            gNoticeFlags = 0;
        } else {
            Chat_DrawHud();
        }
        if (gChatInputMode == 0) {
            Hud_FadeDraw();
        }
        if (Kbd_IsOpen() != 0) {
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
            Chat_DrawKbdBox(gChatInputMode != 0 ? 0xBDB59F6C : 0xBD6C6CC5);
        }
    }
    Hud_RestoreGsState();
}

/* The online town's first draw hook (Hud_Update): the pause window. */
void Hud_DrawLayer1B(void)
{
    Hud_SaveGsState();
    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Text_SetSize(0);
    Font_SetPal(0);
    Pause_DrawWindow();
    Hud_RestoreGsState();
}

/* The online town's second draw hook: unless chatting, the pouch-full
 * dialog or the lobby's game menu, and the item box menu. */
void Hud_DrawLayer2B(void)
{
    Hud_SaveGsState();
    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Text_SetSize(1);
    Font_SetPal(0);
    if (gChatInputMode == 0) {
        if (gHudState->dropState != DROP_NONE) {
            Hud_DrawItemWindow();
        } else if (gScene.menuOpen != 0) {
            GameMenu_Draw();
        }
        BoxMenu_Draw();
    }
    Hud_RestoreGsState();
}

extern u8 gChatSendTo;

/* The online town's third draw hook: a notice, the chat (the full log or
 * the HUD's lines while typing, by gChatSendTo), the fades and the
 * keyboard's box in the colour of the chat's target. */
void Hud_DrawLayer3B(void)
{
    u32 boxColor;
    Hud_SaveGsState();
    Text_SetSize(2);
    Font_SetPal(0);
    if (gNoticeFlags != 0) {
        Notice_Draw();
        gNoticeFlags = 0;
    }
    if (gChatInputMode != 0) {
        if (gChatSendTo == 0) {
            ChatLog_DrawFull();
        } else {
            Chat_DrawHudLog();
        }
    } else if (gChatSendTo != 0) {
        Chat_DrawHud();
    } else {
        LobbyChat_DrawHud();
    }
    if (gChatInputMode == 0) {
        Hud_FadeDraw();
    }
    if (Kbd_IsOpen() != 0) {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        boxColor = 0xBD6C6CC5;
        if (gChatInputMode != 0) {
            boxColor = gChatSendTo == 0 ? 0xBD6CC56C : 0xBDB59F6C;
        }
        Chat_DrawKbdBox(boxColor);
    }
    Hud_RestoreGsState();
}

/* Starts the combine menu's fade of kind `mode` at its result's place. */
void CombineMenu_StartFade(s32 mode)
{
    Hud_FadeStart(mode, 270, gHudState->combineThree == 0 ? 195 : 234);
}

extern Fade gHudFades[6];
extern FadeDef *gHudFadeSetsA[4];
extern FadeDef gHudFadeDefsB[2];
extern FadeDef *gHudFadeSetsC[4];
extern s16 gFadeOffsets[][3][2];

/* Starts the six fades of gHudFades (gHudState's fadeActive to fadeFrame) in one of
 * three patterns at (x, y): 0 a staggered row (with a jingle), 1 three pairs jittered by the
 * gFadeOffsets table picked by the frame tick, 2 four at once. */
void Hud_FadeStart(s32 mode, s16 x, s16 y)
{
    Fade *fade;
    f32 baseX;
    u32 i;
    s32 j;
    s32 k;

    gHudState->fadeActive = 1;
    gHudState->fadeFrame = 1;
    gHudState->fadeMode = mode;
    switch (mode) {
    case 0:
        fade = gHudFades;
        baseX = x + 50;
        for (i = 0; i < 6; i++) {
            fade->active = 1; fade->visible = 0; fade->start = i * 5; fade->bx = baseX; fade->by = y; fade->def = gHudFadeSetsA[i % 4];
            fade++;
        }
        Jingle_Play(gHudState->player, 28);
        break;
    case 1:
        fade = gHudFades;
        k = (u8)gFrameTick * 3 >> 8;
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 2; j++) {
                fade->active = 1; fade->visible = 0; fade->start = i * 3;
                fade->bx = x + gFadeOffsets[k][i][0]; fade->by = y + gFadeOffsets[k][i][1]; fade->def = &gHudFadeDefsB[j];
                fade++;
            }
        }
        Sound_PlaySeFlat(0, 14, 0);
        break;
    case 2:
        fade = gHudFades;
        baseX = x + 50;
        for (i = 0; i < 4; i++) {
            fade->active = 1; fade->visible = 0; fade->start = 0; fade->bx = baseX; fade->by = y; fade->def = gHudFadeSetsC[i];
            fade++;
        }
        gHudFades[5].active = 0;
        gHudFades[4].active = 0;
        Sound_PlaySeFlat(0, 13, 0);
        break;
    }
}

/* Steps the menus' fades (the six of gHudFades) one frame: each active
 * fade that has started (Fade.start, in HudState.fadeFrame's frames) is
 * shown and takes its position and alpha from its keys; the frame count
 * runs while any is active, and HudState.fadeActive clears when none is. */
void Fade_UpdateAll(void)
{
    s16 elapsed;
    Fade *fade;
    s32 i;
    s32 running;

    fade = gHudFades;
    running = 0;
    i = 6;
    do {
        if (fade->active != 0) {
            fade->visible = 0;
            running++;
            if (fade->start < gHudState->fadeFrame) {
                elapsed = gHudState->fadeFrame - fade->start;
                fade->visible = 1;
                Fade_ApplyPos(fade, fade->def->pos, elapsed);
                Fade_ApplyAlpha(fade, fade->def->alpha, elapsed);
            }
        }
        fade++;
    } while (--i != 0);
    if (running != 0) {
        (gHudState->fadeFrame)++;
    } else {
        gHudState->fadeActive = 0;
    }
}

/* Draws the active fades of gHudFades: each a textured quad (two triangles) of its
 * definition, turned by the definition angle (ANGLE_TURN to a turn) and scaled to its rectangle. */
void Hud_FadeDraw(void)
{
    Fade *fade;
    s32 i;
    PrimTexTri tri;
    f32 sinA;
    f32 cosA;

    if (gHudState->fadeActive == 0) {
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    fade = gHudFades;
    i = 6;
    do {
        if (fade->active != 0 && fade->visible != 0) {
            Gfx_SetBlendAlphaAdd(fade->def->tex);
            tri.color = fade->def->color | (fade->alpha << 24);
            tri.u0 = tri.u2 = fade->def->u + 1;
            tri.v0 = tri.v1 = fade->def->v + 1;
            tri.u1 = fade->def->u + fade->def->x1 - 1;
            tri.v2 = fade->def->v + fade->def->y1 - 1;
            Math_GetSinCos(&sinA, &cosA, 6.2831855f * fade->def->angle / 65536.0f);
            tri.x0 = fade->bx + fade->x0 * cosA - fade->y0 * sinA;
            tri.y0 = fade->by + (s16)(fade->x0 * sinA + fade->y0 * cosA);
            tri.x1 = fade->bx + fade->x1 * cosA - fade->y0 * sinA;
            tri.y1 = fade->by + (s16)(fade->x1 * sinA + fade->y0 * cosA);
            tri.x2 = fade->bx + fade->x0 * cosA - fade->y1 * sinA;
            tri.y2 = fade->by + (s16)(fade->x0 * sinA + fade->y1 * cosA);
            Prim_DrawTexTri((struct PrimTexTri *)&tri);
            tri.u0 = fade->def->u + fade->def->x1 - 1;
            tri.v0 = fade->def->v + fade->def->y1 - 1;
            tri.x0 = fade->bx + fade->x1 * cosA - fade->y1 * sinA;
            tri.y0 = fade->by + (s16)(fade->x1 * sinA + fade->y1 * cosA);
            Prim_DrawTexTri((struct PrimTexTri *)&tri);
        }
        fade++;
    } while (--i != 0);
}

/* Interpolates the position keys of e's definition at time t into e's rectangle (x0-y1); 1 before
 * the first key, -1 (and e cleared) after the last one. */
s32 Fade_ApplyPos(Fade *fade, PosKey *key, s16 frame)
{
    PosKey *next;
    f32 x;
    f32 y;
    s32 sinceKey;
    s32 keySpan;

    frame = (s16)frame;
    if (frame < key->time) {
        return 1;
    }
    for (next = key + 1; next->time > 0; key = next, next++) {
        if (frame <= next->time) {
            sinceKey = frame - key->time;
            keySpan = next->time - key->time;
            x = next->x - key->x;
            x = key->x + x * sinceKey / keySpan;
            y = next->y - key->y;
            y = key->y + y * sinceKey / keySpan;
            fade->x0 = -fade->def->x0 * x;
            fade->x1 = x * (fade->def->x1 - fade->def->x0);
            fade->y0 = -fade->def->y0 * y;
            fade->y1 = y * (fade->def->y1 - fade->def->y0);
            return 0;
        }
    }
    fade->active = 0;
    return -1;
}

/* Sets the fade's alpha at `frame` from its alpha keys (AlphaKey, a list
 * ending with a time of 0), interpolated between the two keys around it.
 * Returns 1 before the first key, 0 inside the keys, and -1 after the
 * last, which ends the fade. */
s32 Fade_ApplyAlpha(Fade *fade, AlphaKey *key, s16 frame)
{
    AlphaKey *next;
    f32 level;

    if (frame < key->time) {
        return 1;
    }
    for (next = key + 1; next->time > 0; key = next, next++) {
        if (frame <= next->time) {
            level = key->value;
            level += (next->value - level) * (f32)(frame - key->time) / (f32)(next->time - key->time);
            fade->alpha = 255.0f * level;
            return 0;
        }
    }
    fade->active = 0;
    return -1;
}

/* Opens the quest pages (as QuestMenu_Open, with its own page counts): three offline,
 * four online, the window sliding in. Returns 0. */
s32 QuestMenu_Begin(void)
{
    QuestMenu_CheckInfo();
    gHudState->pageCursor = 0;
    gHudState->questMenuStep = 0;
    if (Game_IsOnline() == 0) {
        gHudState->cursorMax = 3;
    } else {
        gHudState->cursorMax = 4;
    }
    gHelpShow = 0;
    Shake_Start(SHAKE_QUEST_MENU, SHAKE_AXIS_X, 280.0f);
    return 0;
}

/* Draws `value` in full-width digits. */
void QuestMenu_DrawWideNumber(s32 value)
{
    char out[64];
    char buf[64];

    sprintf(buf, gQuestInfoFormats[0], value);
    Sjis_Widen(buf, out);
    Text_Draw(out);
}

extern u16 gNextQuestId;
extern char *D_004458F4;

/* Draws a training quest's terms page at x: the time limit, the par
 * time in minutes (Dojo_GetParMinutes), the reward and the two sub
 * rewards in full-width digits, and two of the quest's text lines. The
 * quest is gNextQuestId in the town, gQuest.stage outside it. */
void QuestMenu_DrawParTime(s16 x)
{
    char buf[64];
    char out[64];
    u16 quest;

    if (gScene.inLobby == 1) {
        quest = gNextQuestId;
    } else {
        quest = gQuest.stage;
    }
    Font_SetPos(x + 503, 162);
    QuestMenu_DrawTimeLimit(1);
    Font_SetPos(x + 503, 182);
    sprintf(buf, D_004458F4, Dojo_GetParMinutes(quest));
    Sjis_Widen(buf, out);
    Text_Draw(out);
    Font_SetPos(x + 467, 202);
    QuestMenu_DrawWideNumber(gQuestInfo->reward);
    Font_SetPos(x + 467, 222);
    QuestMenu_DrawWideNumber(gQuestInfo->rewardSubA);
    Font_SetPos(x + 467, 242);
    QuestMenu_DrawWideNumber(gQuestInfo->rewardSubB);
    Font_SetPos(x + 359, 302);
    Text_Draw(QuestInfo_Get(4));
    Font_SetPos(x + 359, 382);
    Text_Draw(QuestInfo_Get(5));
}

extern char *gDojoBestTimeFmt;
char *strcpy(char *dst, char *src);

/* Draws a training quest's record page at x: the quest's text, the
 * stage's name, and the best score and the best time (in minutes and
 * seconds; the save keeps frames, 30 a second), or the no-record text
 * until the quest's list bit is set. */
void QuestMenu_DrawBestRecord(s16 x)
{
    char buf[64];
    char out[64];
    u16 quest;
    s32 seconds;
    s32 minutes;

    if (gScene.inLobby == 1) {
        quest = gNextQuestId;
    } else {
        quest = gQuest.stage;
    }
    Font_SetPos(x + 449, 142);
    Text_Draw(QuestInfo_Get(6));
    Font_SetPos(x + 449, 162);
    Text_Draw(gStageNames[gQuestInfo->map]);
    Font_SetPos(x + 485, 182);
    if (Save_IsQuestCleared(quest) == 0) {
        Text_Draw(gDojoNoRecordText[0]);
    } else {
        QuestMenu_DrawWideNumber(Dojo_GetBestScore(quest));
    }
    Font_SetPos(x + 485, 202);
    if (Save_IsQuestCleared(quest) == 0) {
        strcpy(buf, gDojoNoRecordText[0]);
    } else {
        seconds = Dojo_GetBestTime(quest) / 30;
        minutes = seconds / 60;
        seconds -= minutes * 60;
        sprintf(buf, gDojoBestTimeFmt, minutes, seconds);
    }
    Sjis_Widen(buf, out);
    Text_Draw(out);
    Font_SetPos(x + 359, 242);
    Text_Draw(QuestInfo_Get(7));
}

extern char *D_00445904;
extern char *D_00445908;
extern char gMemberNames[][17];
extern char gMyName[];
extern char gMyUserId[];
s32 func_D64530(s32 i);
void Text_SetColor(s16 color);

/* draws the quest info window: page 0 the quest, 1 and 2 the sub-pages, 3 the
 * players (two strings each, the local one from gMyName/gMyUserId) */
void QuestMenu_DrawInfo(void)
{
    char buf[64];
    char out[64];
    s16 *pos;
    s32 i;
    s32 lineY;
    s32 listY;
    s16 k;
    s16 page;
    u8 pageNo;
    u8 *scene;

    pos = Shake_GetOffset(SHAKE_QUEST_MENU);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    if (gHudState->hasQuestInfo == 0) {
        gQuestMenuPanel.items = NULL;
        ListPanel_Draw(&gQuestMenuPanel, pos, gQuestStatusTitle[0], -1);
        Font_SetPos(pos[0] + 359, 182);
        Text_Draw(gNoQuestText);
        return;
    }
    pageNo = gHudState->pageCursor;
    sprintf(buf, gQuestMenuTitles[0], pageNo + 1, gHudState->cursorMax);
    page = pageNo;
    switch (page) {
    case 0:
        k = 1;
        break;
    case 1:
        k = 4;
        break;
    case 2:
        k = 5;
        break;
    case 3:
        k = 6;
        break;
    }
    gQuestMenuPanel.items = gQuestMenuItemLists[k];
    ListPanel_Draw(&gQuestMenuPanel, pos, buf, -1);
    Menu_DrawFrame(&gQuestMenuPanel, pos);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(pos[0] + 359, 102);
    Text_Draw(QuestInfo_Get(0));
    switch (page) {
    case 0:
        Font_SetPos(pos[0] + 359, 182);
        Text_Draw(QuestInfo_Get(1));
        Font_SetPos(pos[0] + 449, 202);
        Text_DrawStars(gQuestInfo->stars);
        Font_SetPos(pos[0] + 359, 262);
        Text_Draw(QuestInfo_Get(2));
        Font_SetPos(pos[0] + 359, 322);
        Text_Draw(QuestInfo_Get(3));
        Font_SetPos(pos[0] + 359, 382);
        Text_Draw(gQuestKindNames[gQuestInfo->terms]);
        break;
    case 1:
        QuestMenu_DrawParTime(pos[0]);
        break;
    case 2:
        QuestMenu_DrawBestRecord(pos[0]);
        break;
    case 3:
        i = 0;
        if (i < gScene.playerCount) {
            scene = (u8 *)&gScene;
            lineY = 162;
            listY = 100;
            do {
            if (((SceneState *)scene)->inQuest[0] == 1) {
                Font_SetPos(pos[0] + 359, lineY);
                Text_SetColor((u8)func_D64530(i));
                sprintf(buf, "%dP", i + 1);
                Sjis_Widen(buf, out);
                Text_Draw(out);
                Font_SetColor(0x807F7F7F);
                Font_SetPos(pos[0] + 359, listY + 82);
                if (i == gScene.localPlayer) {
                    sprintf(buf, D_00445904, gMyName);
                } else {
                    sprintf(buf, D_00445904, gMemberNames[i]);
                }
                Sjis_Widen(buf, out);
                Text_Draw(out);
                Font_SetPos(pos[0] + 359, listY + 102);
                if (i == gScene.localPlayer) {
                    sprintf(buf, D_00445908, gMyUserId);
                } else {
                    sprintf(buf, D_00445908, gPlayerIds[i]);
                }
                Sjis_Widen(buf, out);
                Text_Draw(out);
                lineY += 60;
                listY += 60;
            }
            scene++;
            } while (++i < gScene.playerCount);
        }
        break;
    }
}

/* A drill's item list for page kind `kind` (gQuestMenuItemLists): the
 * terms and place pages have their own lists (4 and 5). */
s32 QuestMenu_DrillItemList(s32 kind)
{
    if (kind == 2) {
        return 4;
    }
    if (kind == 3) {
        kind = 5;
    }
    return kind;
}

/* Opens the comment line's page, keyboard closed. */
void CommentEdit_Open(void)
{
    gHelpShow = 0;
    gHudState->kbdOpen = 0;
}

extern u8 *gLoadBuf;
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
s32 HunterCard_SendChanged(void);

/* The chat input line: confirm opens the keyboard (the line saved), then the keyboard's result
 * (1: send, -1: cancelled, the line restored). */
u16 Chat_InputLine(u16 buttons)
{
    if ((buttons & PAD_CIRCLE) && gHudState->kbdOpen == 0) {
        memcpy(gLoadBuf, gSave.comment, 98);
        gHudState->kbdOpen = 1;
        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        Kbd_SetPos(75, 80.0f);
        Chat_OpenKbd(2, 14, 96, gSave.comment);
    } else if (gHudState->kbdOpen != 0) {
        if (gPadState[0].pressed & PAD_SELECT) {
            Kbd_Cancel();
            gHudState->kbdOpen = 0;
            memcpy(gSave.comment, gLoadBuf, 98);
        } else {
            switch (Kbd_Run((char *)gSave.comment, gPadState[0].held, gPadState[0].pressed)) {
            case 0:
                break;
            case 1:
                Chat_FilterWords((char *)gSave.comment);
            close:
                gHudState->kbdOpen = 0;
                Kbd_Cancel();
                if (Game_IsOnline() == 1) {
                    HunterCard_SendChanged();
                }
                break;
            case -1:
                memcpy(gSave.comment, gLoadBuf, 98);
                goto close;
            }
        }
    }
    return buttons;
}

extern ListPanel gCommentEditMenu __attribute__((aligned(16)));
extern ListWin gCommentEditPanel;

/* Draws the comment line's page: its window, the menu (grey while the
 * keyboard is open) and the comment. */
void CommentEdit_Draw(void)
{
    ListPanel panel = gCommentEditMenu;

    if (gHudState->kbdOpen != 0) {
        panel.cursorColor = 0xFF707070;
    }
    ListPanel_DrawDim(&gCommentEditPanel, gCommentEditTitle[0], -1);
    ListPanel_Draw(&panel, 0, 0, 0);
    Text_DrawWrapped33(306, 82, 18, (char *)gSave.comment);
}

/* The HUD's and the menus' initialised data. */
extern char gEmptyNameStr[];
extern u32 gHeapBlocks[];
s32 PhraseMenu_Open(void *m);
u32 gPlayerMapColors[4] = {
#include "sub_main/004306C0.inc"
};
char *gEmptyNameText = gEmptyNameStr;
IconUV gStatusIconUVs[15] = {
#include "sub_main/004306E0.inc"
};
u8 gQuestMenuCursorMax[8] = {
#include "sub_main/00430758.inc"
};
u8 gQuestMenuKinds[2][4] = {
#include "sub_main/00430760.inc"
};
u8 gOptionMenuCounts[8] = {
#include "sub_main/00430768.inc"
};
u8 gOptionMenuRows_0[8] = {
#include "sub_main/00430770.inc"
};
u8 gOptionMenuRows_1[8] = {
#include "sub_main/00430778.inc"
};
u8 gOptionMenuRows_2[8] = {
#include "sub_main/00430780.inc"
};
u8 gOptionMenuRows_3[8] = {
#include "sub_main/00430788.inc"
};
u8 gOptionMenuRows_4[16] = {
#include "sub_main/00430790.inc"
};
u8 *gOptionMenuRows[8] = {
    gOptionMenuRows_0, gOptionMenuRows_1, gOptionMenuRows_2, gOptionMenuRows_3,
    gOptionMenuRows_4, NULL, NULL, NULL,
};
QuadDef gCompassQuads[2] = {
#include "sub_main/004307C0.inc"
};
u32 gClockColors[4] = {
#include "sub_main/00430810.inc"
};
u32 gHudSeasonColors[4] = {
#include "sub_main/00430820.inc"
};
u32 gCompassMarkerColors[4] = {
#include "sub_main/00430830.inc"
};
u32 gCompassColors[4] = {
#include "sub_main/00430840.inc"
};
f32 gCompassPoints[10][2] = {
#include "sub_main/00430850.inc"
};
u8 gCompassUVs[8][4] = {
#include "sub_main/004308A0.inc"
};
Sprite gClockRimSprites[2] = {
#include "sub_main/004308C0.inc"
};
s16 gHudGaugeFlashSpeeds[2][5] = {
#include "sub_main/004308F0.inc"
};
u8 gSharpnessBarV[8] = {
#include "sub_main/00430908.inc"
};
WeaponBoxHud gSharpnessGlowMasks = {
#include "sub_main/00430910.inc"
};
s16 gHornNoteYs[8] = {
#include "sub_main/00430A20.inc"
};
TexRect gBlockGaugeRects[3] = {
#include "sub_main/00430A30.inc"
};
s32 gItemColors[12] = {
#include "sub_main/00430A70.inc"
};
u32 gItemFontColors[12] = {
#include "sub_main/00430AA0.inc"
};
s32 gItemDropPouchColors[8] = {
#include "sub_main/00430AD0.inc"
};
u32 gItemDropPanelColors[8] = {
#include "sub_main/00430AF0.inc"
};
Sprite gPouchBoxSprites[5] = {
#include "sub_main/00430B10.inc"
};
Sprite gPouchBarSprites[8] = {
#include "sub_main/00430B80.inc"
};
Sprite gPouchSelectorSprites[4] = {
#include "sub_main/00430C20.inc"
};
Sprite gPouchSelectorSpritesB[8] = {
#include "sub_main/00430C70.inc"
};
Sprite gPouchSelectorSpritesC[2] = {
#include "sub_main/00430D10.inc"
};
Sprite gHudButtonSprites[3] = {
#include "sub_main/00430D40.inc"
};
Sprite gPouchBarFrame[9] = {
#include "sub_main/00430D80.inc"
};
Icon gPouchSelIconsA[2] = {
#include "sub_main/00430E40.inc"
};
Icon gPouchSelIconsB[2] = {
#include "sub_main/00430E50.inc"
};
Icon gPouchBarIcons[2] = {
#include "sub_main/00430E60.inc"
};
Icon gHudButtonIcons[2] = {
#include "sub_main/00430E70.inc"
};
s16 gPouchBarXA[8] = {
#include "sub_main/00430E80.inc"
};
s16 gPouchBarXB[8] = {
#include "sub_main/00430E90.inc"
};
f32 gMapScales[52] = {
#include "sub_main/00430EA0.inc"
};
f32 gMapDrawScales[26][2] = {
#include "sub_main/00430F70.inc"
};
u8 gMapIconKinds[80] = {
#include "sub_main/00431040.inc"
};
u8 gMapMonsterIconU[16] = {
#include "sub_main/00431090.inc"
};
u32 gMapMonsterIconColors[4] = {
#include "sub_main/004310A0.inc"
};
MapPoint gMapPoints[200] = {
#include "sub_main/004310B0.inc"
};
MapPoint gMapPoints80[2] = {
#include "sub_main/00431560.inc"
};
MonsterNoteDef gMonsterNoteDefs[MONSTER_NOTE_COUNT] = {
#include "sub_main/00431570.inc"
};
s16 gHudStepPulse[4][2] = {
#include "sub_main/00431850.inc"
};
AimViewSprites gAimViewSprites = {
#include "sub_main/00431860.inc"
};
s16 gAimViewSizes[24] = {
#include "sub_main/004318A0.inc"
};
s16 gAimViewTris[56] = {
#include "sub_main/004318D0.inc"
};
IconDef gBlinkIconDefs[3] = {
#include "sub_main/00431940.inc"
};
Sprite gBowgunAmmoSprite = {
#include "sub_main/00431970.inc"
};
PosKey gHudFadeSetsA_0_9[4] = {
#include "sub_main/00431990.inc"
};
PosKey gHudFadeSetsA_1_9[4] = {
#include "sub_main/004319C0.inc"
};
PosKey gHudFadeSetsA_2_5[4] = {
#include "sub_main/004319F0.inc"
};
PosKey gHudFadeSetsA_3_5[4] = {
#include "sub_main/00431A20.inc"
};
AlphaKey gHudFadeSetsA_0_10[6] = {
#include "sub_main/00431A50.inc"
};
AlphaKey gHudFadeSetsA_1_10[6] = {
#include "sub_main/00431A80.inc"
};
AlphaKey gHudFadeSetsA_2_6[6] = {
#include "sub_main/00431AB0.inc"
};
AlphaKey gHudFadeSetsA_3_6[6] = {
#include "sub_main/00431AE0.inc"
};
FadeDef gHudFadeSetsA_0 = {
#include "sub_main/00431B10.inc"
};
FadeDef gHudFadeSetsA_1 = {
#include "sub_main/00431B30.inc"
};
FadeDef gHudFadeSetsA_2 = {
#include "sub_main/00431B50.inc"
};
FadeDef gHudFadeSetsA_3 = {
#include "sub_main/00431B70.inc"
};
FadeDef *gHudFadeSetsA[4] = {
    &gHudFadeSetsA_0, &gHudFadeSetsA_1, &gHudFadeSetsA_2, &gHudFadeSetsA_3,
};
PosKey gHudFadeDefsB_0[5] = {
#include "sub_main/00431BA0.inc"
};
PosKey gHudFadeDefsB_1[4] = {
#include "sub_main/00431BE0.inc"
};
AlphaKey gHudFadeDefsB_0_a[4] = {
#include "sub_main/00431C10.inc"
};
AlphaKey gHudFadeDefsB_1_a[4] = {
#include "sub_main/00431C30.inc"
};
FadeDef gHudFadeDefsB[2] = {
#include "sub_main/00431C50.inc"
};
PosKey gHudFadeSetsC_0_9[5] = {
#include "sub_main/00431C90.inc"
};
PosKey gHudFadeSetsC_1_9[5] = {
#include "sub_main/00431CD0.inc"
};
PosKey gHudFadeSetsC_2_5[4] = {
#include "sub_main/00431D10.inc"
};
PosKey gHudFadeSetsC_3_5[4] = {
#include "sub_main/00431D40.inc"
};
AlphaKey gHudFadeSetsC_0_10[4] = {
#include "sub_main/00431D70.inc"
};
AlphaKey gHudFadeSetsC_1_10[4] = {
#include "sub_main/00431D90.inc"
};
AlphaKey gHudFadeSetsC_2_6[4] = {
#include "sub_main/00431DB0.inc"
};
AlphaKey gHudFadeSetsC_3_6[4] = {
#include "sub_main/00431DD0.inc"
};
FadeDef gHudFadeSetsC_0 = {
#include "sub_main/00431DF0.inc"
};
FadeDef gHudFadeSetsC_1 = {
#include "sub_main/00431E10.inc"
};
FadeDef gHudFadeSetsC_2 = {
#include "sub_main/00431E30.inc"
};
FadeDef gHudFadeSetsC_3 = {
#include "sub_main/00431E50.inc"
};
FadeDef *gHudFadeSetsC[4] = {
    &gHudFadeSetsC_0, &gHudFadeSetsC_1, &gHudFadeSetsC_2, &gHudFadeSetsC_3,
};
/* The message-event list, the game overlay's (sub_main links game's names
 * link-only: tools/configure.py, LOADED_WITH). */
s32 (*gFieldMenuDecide[12])(void) = {
    (s32 (*)(void))PouchMenu_Open, CombineMenu_Open, (s32 (*)(void))StatusMenu_Open, (s32 (*)(void))QuestMenu_Open,
    (s32 (*)(void))InfoMenu_Open, (s32 (*)(void))PhraseMenu_Open, (s32 (*)(void))OptionMenu_Open, PauseMenu_Open,
    MsgEvent_OpenList, NULL, NULL, NULL,
};
s32 (*gFieldMenuDecideOnline[8])(void) = {
    (s32 (*)(void))PouchMenu_Open, CombineMenu_Open, (s32 (*)(void))StatusMenu_Open, (s32 (*)(void))QuestMenu_Open,
    (s32 (*)(void))InfoMenu_Open, ChatMenu_Open, (s32 (*)(void))OptionMenu_Open, NULL,
};
u16 (*gFieldMenuInput[12])(s32) = {
    (u16 (*)(s32))PouchMenu_Update, (u16 (*)(s32))CombineMenu_Input, (u16 (*)(s32))StatusMenu_Update, (u16 (*)(s32))QuestMenu_Update,
    (u16 (*)(s32))InfoMenu_Update, (u16 (*)(s32))PhraseMenu_Update, (u16 (*)(s32))OptionMenu_Update, (u16 (*)(s32))PauseMenu_Update,
    (u16 (*)(s32))MsgEvent_ListInput, NULL, NULL, NULL,
};
u16 (*gFieldMenuInputOnline[8])(s32) = {
    (u16 (*)(s32))PouchMenu_Update, (u16 (*)(s32))CombineMenu_Input, (u16 (*)(s32))StatusMenu_Update, (u16 (*)(s32))QuestMenu_Update,
    (u16 (*)(s32))InfoMenu_Update, (u16 (*)(s32))ChatMenu_Update, (u16 (*)(s32))OptionMenu_Update, NULL,
};
u8 gFieldMenuHelpLines[16] = {
#include "sub_main/00431F20.inc"
};
ListPanel gFieldMenuPanel = {
#include "sub_main/00431F30.inc"
};
void (*gFieldMenuDraw[12])(void) = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, PhraseMenu_Draw, OptionMenu_Draw, PauseMenu_Draw,
    MsgEvent_Draw, NULL, NULL, NULL,
};
void (*gFieldMenuDrawOnline[8])(void) = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, ChatMenu_Draw, OptionMenu_Draw, NULL,
};
Sprite gFieldMenuCockpitBox = {
#include "sub_main/00431FA0.inc"
};
Icon gFieldMenuCockpitIcon = {
#include "sub_main/00431FB8.inc"
};
ListPanel gStatusMenuPanel = {
#include "sub_main/00431FC0.inc"
};
u8 gInfoMenuCounts[16] = {
#include "sub_main/00431FE0.inc"
};
u8 gInfoMenuEntries[16] = {
#include "sub_main/00431FF0.inc"
};
ListPanel gInfoMenuPanel = {
#include "sub_main/00432000.inc"
};
char **gInfoMenuLabelSets[4] = {
    gInfoMenuLabelsOffline, gInfoMenuLabels, gInfoMenuLabels, gInfoMenuLabels,
};
ListPanel gItemListPanel = {
#include "sub_main/00432030.inc"
};
Frame gItemListFrame[2] = {
#include "sub_main/00432050.inc"
};
ListPanel gMonsterNotePanels[2] = {
#include "sub_main/00432080.inc"
};
Panel gMonsterNoteHelpBoxes[2] = {
#include "sub_main/004320C0.inc"
};
ListPanel gQuestMenuPanel = {
#include "sub_main/004320F0.inc"
};
ListPanel gChatMenuPanel = {
#include "sub_main/00432110.inc"
};
ListPanel gPhraseMenuPanel = {
#include "sub_main/00432130.inc"
};
ListPanel gCombineKindPanel = {
#include "sub_main/00432150.inc"
};
ListPanel gCombineMaterialPanel = {
#include "sub_main/00432170.inc"
};
Panel gCombineResultWindow = {
#include "sub_main/00432190.inc"
};
ListPanel gCombineDoPanel = {
#include "sub_main/004321B0.inc"
};
ListPanel gOptionMenuPanel = {
#include "sub_main/004321D0.inc"
};
u32 gPlayerNameColors[8] = {
#include "sub_main/004321F0.inc"
};
char *gSideNames[8] = {
    (char *)gSideNameNone, (char *)gSideNameA, (char *)gSideNameB, (char *)gSideNameA,
    (char *)gSideNameB, NULL, NULL, NULL,
};
ListPanel gPouchMenuPanel = {
#include "sub_main/00432230.inc"
};
ListPanel gPouchMenuPanel2 = {
#include "sub_main/00432250.inc"
};
ListPanel D_00432270[2] = {
#include "sub_main/00432270.inc"
};
ListPanel gDeliveryPanel = {
#include "sub_main/004322B0.inc"
};
ListPanel gPlayerNameListPanels[2] = {
#include "sub_main/004322D0.inc"
};
Panel gPlayerNameCountBox = {
#include "sub_main/00432310.inc"
};
Panel gDeliveryCountBox = {
#include "sub_main/00432330.inc"
};
ListPanel gItemListBox = {
#include "sub_main/00432350.inc"
};
Panel gItemListTextBox = {
#include "sub_main/00432370.inc"
};
f32 gMapAreaOffsetX[28] = {
#include "sub_main/00432390.inc"
};
s16 gMapAreaOffsetY[40] = {
#include "sub_main/00432400.inc"
};
Panel gItemWindowNameBox[2] = {
#include "sub_main/00432450.inc"
};
ListPanel gItemWindowDropPanel = {
#include "sub_main/00432480.inc"
};
ListPanel gItemWindowPanel = {
#include "sub_main/004324A0.inc"
};
Panel gItemWindowTextBox = {
#include "sub_main/004324C0.inc"
};
GaugeFrame gGaugeFrames[2] = {
#include "sub_main/004324E0.inc"
};
Frame gSupplyBoxFrame = {
#include "sub_main/00432510.inc"
};
ListWin gTentMenuPanel = {
#include "sub_main/00432530.inc"
};
Panel gSupplyBoxTextBox = {
#include "sub_main/00432550.inc"
};
ListPanel gDeliveryPanel2 = {
#include "sub_main/00432570.inc"
};
Panel gDeliveryTextBox = {
#include "sub_main/00432590.inc"
};
ListWin gEquipStatusPanel = {
#include "sub_main/004325B0.inc"
};
Panel gPauseTextBox = {
#include "sub_main/004325D0.inc"
};
s16 gFadeOffsets[4][3][2] = {
#include "sub_main/004325F0.inc"
};
ListWin gCommentEditPanel = {
#include "sub_main/00432620.inc"
};
ListPanel gCommentEditMenu __attribute__((aligned(16))) = {
#include "sub_main/00432640.inc"
};
WeaponIconUV gWeaponIconUVs[11] = {
#include "sub_main/00432660.inc"
};
