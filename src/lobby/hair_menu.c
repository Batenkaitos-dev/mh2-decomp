/*
 * The hair menu (TOWN_MODE_HAIR_MENU). At the hairdresser (HairMenu_Open's
 * withNpc), a haircut for HAIR_NPC_PRICE: the player walks to the chair and
 * sits, the hairdresser does the hair (NPC_STATE_HAIR_WAIT ... _HAIR_DO) and
 * the new hair is one of three styles at random. Elsewhere, the hair-style
 * change: a list of the styles (gHairStyles, a page of HAIR_MENU_PAGE_ROWS
 * at a time), then a colour editor (red, green, blue or a preset), with the
 * model previewed in the chosen colour, and a yes/no on the price.
 *
 * The menu's state is HairMenuWork in gMenuWork. HairMenu_Update runs the
 * steps (enum HairMenuStep), HairMenu_ListInput the list and the editor
 * (enum HairListState), and HairMenu_Draw draws them.
 */
#include "common.h"
#include "scene.h"
#include "player.h"
#include "lobby_globals.h"
#include "save.h"
#include "em.h"
#include "lobby_menu.h"
#include "lobby_state.h"
#include "lobby_npc.h"
#include "npc_motion.h"
#include "quest.h"
#include "bgm.h"
#include "pad.h"
#include "lobby_labels.h"
#include "menu_task.h"
#include "prim.h"
#include "equip_preview.h"
#include "area_draw.h"
#include "camera.h"
#include "hair_menu.h"
#include "equip_shop.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "rand.h"
#include "talk.h"
#include "town_players.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
/* The functions reach gMenuWork through a HairMenuWork pointer: reading
 * gMenuWork.hair's fields by name, the compiler addresses each field from the
 * symbol, where the game's code keeps the base in a register
 * (HairMenu_MovePlayer, HairMenu_Update). */
extern MenuWork gMenuWork;

/* Returns whether the hair menu is open. */
s32 HairMenu_IsOpen(void)
{
    return gTownMode == TOWN_MODE_HAIR_MENU;
}

void *memset(void *d, s32 c, u32 n);
extern u16 gTalkLine;
extern char *gTalkLineText;
extern s8 gTownJingleOn;

/*
 * Opens the hair menu: clears gMenuWork, sets the draw callback and the town
 * mode, and starts the talk. At the hairdresser (withNpc 1) the talk's line
 * gets the price, and the townspeople's tune starts unless one plays.
 */
void HairMenu_Open(Em *npc, s32 withNpc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;

    memset(menu, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(HairMenu_Draw, 0, 0, 0);
    gTownMode = TOWN_MODE_HAIR_MENU;
    gTownMenuOpen = 1;
    Talk_Say(HAIR_TALK_GREET, 1);
    if (withNpc != 0) {
        gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), HAIR_NPC_PRICE);
    }
    menu->withNpc = withNpc;
    menu->npc = npc;
    if (menu->withNpc == 1 && Bgm_IsNpcTunePlaying() == 0) {
        Jingle_Play(pl, BGM_TUNE_NPC);
        gTownJingleOn = 1;
    }
    HairMenu_ResetColor();
    menu->previewOff = 1;
    gFieldControl = 0;
}

/* Closes the hair menu: the town mode, the draw callback, the preview model
 * and the camera go back, and the player gets control again. */
void HairMenu_Close(void)
{
    Player *pl;

    gTownMenuOpen = 0;
    gTownMode = 0;
    pl = &gPlayers[gScene.localPlayer];
    LobbyMenu_SetDraw(0, 0, 0, 0);
    pl->target = NULL;
    EquipPreview_Free();
    CamFocus_End();
    CamReq_Cancel();
    gFieldControl = 1;
}

/* Sends the hairdresser and the player to the hair chair: the player walks
 * to the chair's spot (TOWN_PC_WALK_TO_HAIR_CHAIR). */
void HairMenu_MovePlayer(void)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];

    menu->timer = 20; /* frames */
    Npc_SetRouteToHairChair(menu->npc);
    pl->moveTo[0] = 2480.0f;
    pl->moveTo[1] = 0.0f;
    pl->moveTo[2] = 2876.0f;
    Actor_SetState(pl, 0, TOWN_PC_WALK_TO_HAIR_CHAIR);
}

/* Returns 1 while the hair menu is at its first talk or its last one, 0 in
 * between. */
s32 HairMenu_IsIdle(void)
{
    if ((gMenuWork.hair.step > HAIR_STEP_TALK) && (gMenuWork.hair.step != HAIR_STEP_LAST_TALK)) {
        return 0;
    }
    return 1;
}

/*
 * The hair menu's frame (enum HairMenuStep). After the offer's talk, a
 * player with a head piece on gets a refusal, and at the hairdresser one
 * without HAIR_NPC_PRICE gets another; otherwise the hairdresser's haircut
 * runs in steps that wait for her states and motions and the player's, or
 * the style list opens.
 */
void HairMenu_Update(void)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);

    switch (menu->step) {
    case HAIR_STEP_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
            if (gSave.equip[SAVE_EQUIP_HEAD].id != 0) {
                menu->step = HAIR_STEP_LAST_TALK;
                if (menu->withNpc == 1) {
                    Actor_SetState(menu->npc, 0, NPC_STATE_HAIR_WAIT);
                }
                Talk_Say(HAIR_TALK_HEAD_WORN, 0);
            } else {
                if (menu->withNpc == 1) {
                    if (gSave.money < HAIR_NPC_PRICE) {
                        menu->step = HAIR_STEP_LAST_TALK;
                        Talk_Say(HAIR_TALK_NO_MONEY, 0);
                        break;
                    }
                    menu->step = HAIR_STEP_WAIT_NPC;
                } else {
                    (menu->step)++;
                    HairMenu_ShowList();
                }
                HairMenu_MovePlayer();
            }
            CamFocus_End();
            break;
        case MENU_CANCEL:
            menu->step = HAIR_STEP_LAST_TALK;
            Talk_Say(HAIR_TALK_DECLINED, 0);
            if (menu->withNpc == 1 && Bgm_IsNpcTunePlaying() == 0) {
                Jingle_Play(pl, BGM_TUNE_NPC);
                gTownJingleOn = 1;
            }
            break;
        }
        break;
    case HAIR_STEP_LIST:
        switch (HairMenu_ListInput(pad)) {
        case MENU_DONE:
            (menu->step)++;
            break;
        case MENU_CANCEL:
            menu->step = HAIR_STEP_LAST_TALK;
            Talk_Say(HAIR_TALK_DECLINED, 0);
            break;
        }
        break;
    case HAIR_STEP_WAIT_NPC:
        if (menu->timer > 0) {
            (menu->timer)--;
        }
        if (menu->npc->state == NPC_STATE_HAIR_WAIT && menu->timer == 0 && pl->state == TOWN_PC_IDLE) {
            Actor_SetState(menu->npc, 0, NPC_STATE_HAIR_PREPARE);
            CamReq_HairMenu();
            (menu->step)++;
        }
        break;
    case HAIR_STEP_PREPARE:
        if (menu->npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_IDLE)) {
            (menu->step)++;
            Actor_SetState(pl, 0, TOWN_PC_HAIR_CHAIR);
        }
        break;
    case HAIR_STEP_SIT:
        if (pl->motionId[0] == PL_MOT_TOWN_SEATED) {
            (menu->step)++;
            Actor_SetState(menu->npc, 0, NPC_STATE_HAIR_DO);
        }
        break;
    case HAIR_STEP_CUT:
        if (menu->npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT_NEXT) && menu->applied == 0 &&
            Motion_ReachedFrame(menu->npc, 0, 118.0f)) {
            HairMenu_Apply();
            menu->applied = 1;
            Talk_Say(HAIR_TALK_DONE, 0);
            (menu->step)++;
        }
        break;
    case HAIR_STEP_FINISH:
        if (menu->npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT_NEXT)) {
            Talk_RunText();
        }
        if (menu->npc->state == NPC_STATE_HAIR_DONE) {
            (menu->step)++;
            Actor_SetState(pl, 0, TOWN_PC_STAND_UP);
        }
        break;
    case HAIR_STEP_CLOSE:
        HairMenu_Close();
        break;
    case HAIR_STEP_LAST_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            HairMenu_Close();
            break;
        }
        break;
    }
}

/* Returns 1 when style id is listed: not the player's current style, and
 * HAIR_STYLE_PSP_LINK only once EVENT_FLAG_PSP_LINK_4 is set. */
s32 HairMenu_IsUnlocked(s32 id)
{
    s32 style;

    style = (s16)(id);
    if ((style == HAIR_STYLE_PSP_LINK) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_4) == 0)) {
        return 0;
    }
    if (style == gSave.hairStyle) {
        return 0;
    }
    return 1;
}

extern HairStyle gHairStyles[];
void *memcpy(void *d, void *s, u32 n);

/*
 * Copies page `page` of the listed styles (HairMenu_IsUnlocked) into the
 * menu's list, and sets the count, the page, the page count and the cursor
 * within the count. n and count are declared (and zeroed) before first and
 * last: in the other order the compiler keeps page * HAIR_MENU_PAGE_ROWS in
 * other registers than the game's code does. base steps one entry at a time
 * from the work area's start, and each copy goes to its list: the game's
 * code adds the list's offset at each copy.
 */
void HairMenu_LoadPage(s16 page)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    HairStyle *src = gHairStyles;
    s16 n = 0;
    s16 count = 0;
    s16 first = page * HAIR_MENU_PAGE_ROWS;
    s16 last = first + HAIR_MENU_PAGE_ROWS - 1;
    s32 id = gHairStyles[0].id;
    u8 *base;

    if (id != HAIR_STYLE_LIST_END) {
        base = (u8 *)menu;
        do {
            if (HairMenu_IsUnlocked(id) == 1) {
                if (n >= first && n <= last) {
                    memcpy(((HairMenuWork *)base)->list, src, sizeof(HairStyle));
                    base += sizeof(HairStyle);
                    count++;
                }
                n++;
            }
            src++;
            id = src->id;
        } while (id != HAIR_STYLE_LIST_END);
    }
    menu->count = count;
    menu->page = page;
    menu->pages = Menu_CountPages(n, HAIR_MENU_PAGE_ROWS);
    if (menu->sel >= count) {
        menu->sel = count - 1;
    }
}

/* Slides the style list's window in, moves the camera aside and loads the
 * first page. */
void HairMenu_ShowList(void)
{
    WinSlide_Step(&gMenuWork.hair.win, -320, 10);
    CamFocus_SetAside(1);
    HairMenu_LoadPage(0);
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: declared with the s16 position and u32 colours the callers pass; with the definition's s32 parameters and s16 kind, HairMenu_DrawConfirm compiles differently */
void Text_Printf(char *fmt, ...);
void Text_SetSize(s32);

s32 Kbd_IsOpen(void); /* unit's own declaration: the definition returns s8;
 * with s8, HairMenu_ColorInput tests the result differently */
void EquipPreview_Load(u16 kind, u16 id, u8 female);

/* Shows the preview model in the colour being edited: the picked entry's
 * style, or the current one for HAIR_STYLE_COLOR_ONLY. */
static inline void HairMenu_ShowPreview(HairMenuWork *menu)
{
    EquipPreview_Free();
    gEquipPreview.color = (menu->r << 16) | 0xFF000000 | (menu->g << 8) | menu->b;
    if (menu->list[menu->sel].id != HAIR_STYLE_COLOR_ONLY) {
        EquipPreview_Load(EQUIP_PREVIEW_FIXED, menu->list[menu->sel].id, gSave.gender);
    } else {
        EquipPreview_Load(EQUIP_PREVIEW_FIXED, gSave.hairStyle, gSave.gender);
    }
}

/*
 * The style list and the colour editor (enum HairListState): PAD_SELECT
 * shows or hides the preview, the directions move the cursor and the page,
 * confirm on a style the player can pay opens the editor, and the yes/no
 * after it ends the menu. Returns MENU_DONE when bought, MENU_CANCEL when
 * left, else MENU_RUNNING.
 */
s32 HairMenu_ListInput(u16 pad)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;

    switch (menu->state) {
    case HAIR_LIST_OPEN:
        switch (WinSlide_Step(&menu->win, -320, 10)) {
        case MENU_DONE:
            menu->state++;
        }
        break;
    case HAIR_LIST_PICK:
        if (pad & PAD_SELECT) {
            menu->previewOff ^= 1;
            if (menu->previewOff == 0) {
                HairMenu_ShowPreview(menu);
            } else {
                EquipPreview_Free();
            }
        } else if (pad & PAD_CIRCLE) {
            if (menu->list[menu->sel].cost <= gSave.money) {
                menu->state++;
                HairMenu_ResetColor();
                WinSlide_Step(&menu->win2, -320, 10);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            menu->state = HAIR_LIST_OPEN;
            CamFocus_SetAside(0);
            LobbySe_Play(LOBBY_SE_CANCEL);
            EquipPreview_Free();
            return MENU_CANCEL;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
            HairMenu_LoadPage(menu->page);
            if (menu->previewOff == 0) {
                HairMenu_ShowPreview(menu);
            }
        } else if (pad & (PAD_UP | PAD_DOWN)) {
            menu->sel = Menu_WrapUpDown(menu->sel, menu->count);
            if (menu->previewOff == 0) {
                HairMenu_ShowPreview(menu);
            }
        }
        break;
    case HAIR_LIST_COLOR_OPEN:
        switch (WinSlide_Step(&menu->win2, -320, 10)) {
        case MENU_DONE:
            menu->state++;
        }
        break;
    case HAIR_LIST_COLOR:
        switch (HairMenu_ColorInput(pad)) {
        case MENU_DONE:
            menu->state++;
            Menu_SetYesNo(1);
            break;
        case MENU_CANCEL:
            menu->state = HAIR_LIST_OPEN;
            HairMenu_ResetColor();
            gEquipPreview.color = (menu->r << 16) | 0xFF000000 | (menu->g << 8) | menu->b;
            WinSlide_Step(&menu->win, -320, 10);
            break;
        }
        break;
    case HAIR_LIST_CONFIRM:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            menu->state = HAIR_LIST_OPEN;
            EquipPreview_Free();
            return MENU_DONE;
        case MENU_CANCEL:
            menu->state--;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Starts the colour editor on the player's hair colour, at its first row. */
void HairMenu_ResetColor(void)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];

    menu->repeat = 0;
    menu->row = HAIR_COLOR_ROW_R;
    menu->preset = 0;
    menu->r = pl->hairColor >> 16;
    menu->g = pl->hairColor >> 8;
    menu->b = pl->hairColor;
}

/*
 * The colour editor (enum HairColorRow): up and down pick a row, left and
 * right change red, green or blue by one or pick a preset, and a held
 * button repeats after 10 frames. Confirm on HAIR_COLOR_ROW_PRESET takes the
 * preset's colour; on HAIR_COLOR_ROW_OK it keeps the style and returns
 * MENU_DONE. Returns MENU_CANCEL when left, else MENU_RUNNING; the preview
 * takes the colour every frame.
 */
s32 HairMenu_ColorInput(u16 pad)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;

    if (Kbd_IsOpen() == 0) {
        if (gPadState[0].prevHeld == gPadState[0].held) {
            menu->repeat++;
            if (menu->repeat > 10) {
                pad = gPadState[0].held;
            }
        } else {
            menu->repeat = 0;
        }
    }
    if (pad & PAD_SELECT) {
        menu->previewOff ^= 1;
        if (menu->previewOff == 0) {
            EquipPreview_Free();
            gEquipPreview.color = (menu->r << 16) | 0xFF000000 | (menu->g << 8) | menu->b;
            if (menu->list[menu->sel].id != HAIR_STYLE_COLOR_ONLY) {
                EquipPreview_Load(EQUIP_PREVIEW_FIXED, menu->list[menu->sel].id, gSave.gender);
            } else {
                EquipPreview_Load(EQUIP_PREVIEW_FIXED, gSave.hairStyle, gSave.gender);
            }
        } else {
            EquipPreview_Free();
        }
    } else if (pad & PAD_CIRCLE) {
        if (menu->row == HAIR_COLOR_ROW_PRESET) {
            u32 rgb = gPresetColors[menu->preset];
            menu->r = rgb >> 16;
            menu->g = rgb >> 8;
            menu->b = rgb;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (menu->row == HAIR_COLOR_ROW_OK) {
            menu->style = menu->list[menu->sel].id;
            if (menu->style == HAIR_STYLE_COLOR_ONLY) {
                menu->style = gSave.hairStyle;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            return MENU_DONE;
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & (PAD_UP | PAD_DOWN)) {
        menu->row = Menu_WrapUpDown(menu->row, HAIR_COLOR_ROWS);
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        switch (menu->row) {
        case HAIR_COLOR_ROW_PRESET:
            menu->preset = Menu_WrapLeftRight(menu->preset, HAIR_PRESET_COLORS);
            break;
        case HAIR_COLOR_ROW_R:
            if (pad & PAD_LEFT) {
                if (menu->r > 0) {
                    menu->r--;
                }
            } else if (menu->r < 255) {
                menu->r++;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
            break;
        case HAIR_COLOR_ROW_G:
            if (pad & PAD_LEFT) {
                if (menu->g > 0) {
                    menu->g--;
                }
            } else if (menu->g < 255) {
                menu->g++;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
            break;
        case HAIR_COLOR_ROW_B:
            if (pad & PAD_LEFT) {
                if (menu->b > 0) {
                    menu->b--;
                }
            } else if (menu->b < 255) {
                menu->b++;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
            break;
        }
    }
    gEquipPreview.color = (menu->r << 16) | 0xFF000000 | (menu->g << 8) | menu->b;
    return MENU_RUNNING;
}

void Player_FreeChangedPartModels(Player *p);
void Player_LoadPartModels(Player *p);

/*
 * Puts the new hair on and takes its price: at the hairdresser one of three
 * styles at random (17, 18 or 20) in the current colour for HAIR_NPC_PRICE,
 * else the chosen style and colour for the entry's cost. The player's models
 * are reloaded when the style changes.
 */
void HairMenu_Apply(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    s32 cost;
    u8 style;
    s32 color;

    if (menu->withNpc == 1) {
        switch (Rand_Next(1) % 3) {
        case 0:
            style = 17;
            break;
        case 1:
            style = 18;
            break;
        case 2:
            style = 20;
            break;
        }
        color = gSave.hairColor;
        cost = HAIR_NPC_PRICE;
    } else {
        /* only the style's low byte is read, as in the game's code: a
         * truncation of the whole field compiles differently */
        style = *(u8 *)&menu->style;
        color = menu->b | ((menu->r << 16) | 0xFF000000 | (menu->g << 8));
        cost = menu->list[menu->sel].cost;
    }
    gEquipPreview.color = color;
    pl->hairColor = color;
    gSave.hairColor = color;
    if (style != gSave.hairStyle) {
        gSave.hairStyle = style;
        pl->hairStyle = style;
        Player_FreeChangedPartModels(pl);
        Player_LoadPartModels(pl);
    }
    Save_AddMoney(-cost);
}

void Icon_DrawEquip(s16 x, s16 y, s32 w, s32 h, s32 color, s32 i); /* unit's own declaration: declared with the s16 position the caller passes; with the definition's s32 x and y, HairMenu_DrawList compiles differently */
void Text_PrintfColored(char *fmt, ...);
extern char gHairListRowFmt[];

/*
 * Draws the style list's page at (x, y): the page number, each style's name
 * (by the player's gender) and cost, dimmed when the player can't pay it,
 * the equipment icon on a real style, the cursor, and the price's help.
 */
void HairMenu_DrawList(s16 x, s16 y)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    s16 rowY = y;
    s16 i;

    Icon_Draw(x, y, 300, 200, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 220, rowY + 12, menu->page, menu->pages, 1);
    Font_SetSpacing(20, 20);
    rowY += 42;
    i = 0;
    if (i < menu->count) {
        do {
            if (menu->list[i].cost > gSave.money) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(x + 46, rowY);
            if (gPlayers[gScene.localPlayer].gender == 1) {
                Text_PrintfColored(gHairListRowFmt, menu->list[i].name[0], gLobbyWords[LOBBY_WORD_COLON], menu->list[i].cost, gLobbyWords[LOBBY_WORD_ZENNY]);
            } else {
                Text_PrintfColored(gHairListRowFmt, menu->list[i].name[1], gLobbyWords[LOBBY_WORD_COLON], menu->list[i].cost, gLobbyWords[LOBBY_WORD_ZENNY]);
            }
            if (menu->list[i].id != HAIR_STYLE_COLOR_ONLY) {
                Icon_DrawEquip((s16)(x + 46) - 30, rowY, 20, 20, -1, 18);
            }
            if (menu->sel == i) {
                Menu_DrawCursorRow(x + 46, rowY, 220, 50, 0xC0106060, 0xFF106060);
            }
            rowY += 22;
            i++;
        } while (i < menu->count);
    }
    Font_SetSpacing(18, 18);
    Win_DrawText(330, 350, 290, 80, 0xBDB59F6C, 0xC0101010, gHairMenuTexts[HAIR_MENU_PRICE_HEAD]);
}

void ColorEdit_Draw(s32 a, s16 x, s16 y, u8 r, u8 g, u8 b, s32 c, s32 d); /* unit's own declaration: declared with the s32 mode, row and preset the caller passes; with the definition's u8 parameters, HairMenu_DrawConfirm compiles differently */
char *Text_FillDollar(char *fmt, s32 n);
extern LobbyMenu *gLobbyMenu;

/*
 * Draws the colour editor at (x, y) and its help: the question while it is
 * open, the price and the yes/no at HAIR_LIST_CONFIRM. The row and the
 * preset are passed by their low bytes, as in the game's code.
 */
void HairMenu_DrawConfirm(s16 x, s16 y)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    char *text;

    Win_Draw(x, y, 300, 200, 0xBDB59F6C, 0xFF000000, 2);
    ColorEdit_Draw(1, x - 10, y - 18, menu->r, menu->g, menu->b, *(u8 *)&menu->row, *(u8 *)&menu->preset);
    Font_SetSpacing(18, 18);
    switch (menu->state) {
    case HAIR_LIST_COLOR_OPEN:
    case HAIR_LIST_COLOR:
        text = gHairMenuTexts[HAIR_MENU_COLOR_ASK];
        break;
    case HAIR_LIST_CONFIRM:
        text = Text_FillDollar(gHairMenuTexts[HAIR_MENU_COLOR_HELP], menu->list[menu->sel].cost);
        Font_SetPos(430, 400);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
    Win_DrawText(330, 350, 290, 80, 0xBDB59F6C, 0xC0101010, text);
}

/* Draws the menu's title bar, the money and the list's backdrop, with the
 * hint while the preview is hidden. */
void HairMenu_DrawHeader(void)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;
    PrimGradRect rect;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(330, 18, 130, 0xFFB0B000, gHairMenuTexts[HAIR_MENU_TITLE], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    rect.x0 = 330;
    rect.y0 = 58;
    rect.x1 = rect.x0 + 290;
    rect.y1 = rect.y0 + 280;
    rect.color[0] = gHeaderBoxColors[0];
    rect.color[1] = gHeaderBoxColors[1];
    rect.color[2] = gHeaderBoxColors[2];
    rect.color[3] = gHeaderBoxColors[3];
    Prim_FillGradRect(&rect);
    Win_Draw(330, 58, 290, 280, 0xBDB59F6C, 0, 6);
    if (menu->previewOff == 1) {
        Font_SetPos(380, 154);
        Font_SetSpacing(18, 18);
        Text_Printf(gHairMenuTexts[HAIR_MENU_SHOW_HINT]);
    }
}

/* The menu's draw callback: at HAIR_STEP_LIST, the header and the list or
 * the colour editor, each at its window's slide position. */
void HairMenu_Draw(MenuTask *task)
{
    HairMenuWork *menu = (HairMenuWork *)&gMenuWork;

    if (menu->step == HAIR_STEP_LIST) {
        Text_SetSize(task->textSize);
        HairMenu_DrawHeader();
        switch (menu->state) {
        case HAIR_LIST_OPEN:
        case HAIR_LIST_PICK:
            HairMenu_DrawList(menu->win.pos, 58);
            break;
        case HAIR_LIST_COLOR_OPEN:
        case HAIR_LIST_COLOR:
        case HAIR_LIST_CONFIRM:
            HairMenu_DrawConfirm(menu->win2.pos, 58);
            break;
        }
    }
}

HairStyle gHairStyles[20] = {
#include "lobby/007EB8C0.inc"
};

#pragma rodata on
char gHairListRowFmt[13] = {
#include "lobby/00838278.inc"
};
#pragma rodata off
