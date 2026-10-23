/*
 * The hunter info window: another hunter's details, opened from the name
 * search and the area board's member lists (HunterInfo_Open with the
 * HUNTER_INFO_* actions on offer). It fetches the hunter's profile and the
 * start of the hunter's card (LobbyUser_FetchDetails: the profile into the
 * window's state, the card into gHunterPanel), slides in, and shows three
 * pages turned with left and right: where the hunter is (the server, the
 * area and land, the group there), the hunter's status (the card's title,
 * weapon, colour, rank, wins and comment) and the equipment. Beside them a
 * list of actions: send a short mail, add the hunter to the friend list,
 * or join the hunter's group (Group_CanJoin, Group_Join: the yes or no,
 * the move to the hunter's server or level entry).
 *
 * The window's state is a HunterInfoMenu in gHunterInfoMenu, a
 * NetUserProfile followed by the window's own fields. HunterInfo_Update
 * runs it and returns a MENU_* result to the menu that opened it;
 * HunterInfo_DrawIfOpen draws it.
 */
#include "common.h"
#include "lobby_text.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_vobj.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "lobby_net.h"
#include "area_draw.h"
#include "chat_log.h"
#include "stream.h"
#include "text.h"
#include "hunter_info.h"
#include "area_list.h"
#include "draw_helpers.h"
#include "font.h"
#include "friend_add.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_queries.h"
#include "lobby_town.h"
#include "lobby_util.h"
#include "lobby_village.h"
#include "lobby_web_users.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "short_mail.h"
#include "town_frame.h"
#include "screen_fade.h"
#include "libc.h"

#pragma optimization_level 4


/* The hunter's card, as fetched (gFetchedCard's data). The draws take its
 * address through a cast to `u8 *`, kept for the match: with a plain
 * `&gHunterPanel` the compiler reaches the card's fields differently from
 * the game's code. */
extern HunterCard gHunterPanel;
extern HunterInfoMenu gHunterInfoMenu;
extern MailEntry gMailBox[8];
extern u8 gMenuWork;

extern LobbyMenu *gLobbyMenu;

/* Opens the hunter info window on the hunter whose user id is `id`,
 * offering the actions in `flags` (HUNTER_INFO_*): the window's state is
 * cleared, the search's message shown (LOBBY_TEXT_HUNTER_SEARCHING), and
 * the details' fetch starts with the next HunterInfo_Update. */
void HunterInfo_Open(void *id, s32 flags)
{
    HunterInfoMenu *menu = &gHunterInfoMenu;
    s8 act;
    s8 n;

    memset(menu, 0, sizeof(*menu));
    menu->flags = flags;
    memcpy(menu->userId, id, LOBBY_USER_ID_LEN);
    LobbyText_Open(LOBBY_TEXT_HUNTER_SEARCHING);
    gLobbyMenu->redraw = 1;
    WinSlide_Step(&menu->win, 640, 280);
    n = 0;
    for (act = 0; act < HUNTER_ACTS; act++) {
        if (flags & (1 << act)) {
            menu->acts[n++] = act;
        }
    }
    menu->count = n;
    gFieldControl = 0;
}

/* Empty: called as the window ends after a failed fetch. */
static void HunterInfo_End(void) {

}

/* Runs the hunter info window for one frame (enum HunterInfoState): the
 * details' fetch, the slide in, then the pages and the actions, and the
 * action chosen (the short mail form, the friend list's question, the
 * join). Returns MENU_RUNNING while it runs; MENU_UNDONE once the failed
 * fetch's message is confirmed; MENU_CANCEL when cross leaves the actions;
 * MENU_CLOSE on start, which closes the whole menu. */
s32 HunterInfo_Update(u16 pad)
{
    HunterInfoMenu *menu = &gHunterInfoMenu;

    switch (gHunterInfoMenu.state) {
    case HUNTER_INFO_FETCH:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_FetchDetails(menu, menu->userId)) {
        case LOBBY_STEP_DONE:
            menu->state++;
            gFieldControl = 1;
            break;
        case LOBBY_STEP_FAILED:
            menu->state = HUNTER_INFO_FETCH_FAILED;
            break;
        }
        break;
    case HUNTER_INFO_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case 0:
            menu->state++;
            break;
        }
        break;
    case HUNTER_INFO_ACTIONS:
        switch (HunterInfo_ActionInput(menu, pad)) {
        case MENU_DONE:
            switch (menu->acts[menu->sel]) {
            case HUNTER_ACT_MAIL:
                menu->state = HUNTER_INFO_MAIL_FORM;
                ShortMail_Open(menu->userId, menu, SHORT_MAIL_TO_HUNTER);
                gFieldControl = 0;
                break;
            case HUNTER_ACT_FRIEND:
                if (gHunterPanel.friendLimit != 0) {
                    menu->state = HUNTER_INFO_MESSAGE;
                    gFieldControl = 0;
                    LobbyText_Open(LOBBY_TEXT_FRIEND_RESTRICTED);
                    LobbyDialog_Grow();
                    gLobbyMenu->redraw = 1;
                } else {
                    menu->state = HUNTER_INFO_FRIEND_ADD;
                    FriendList_AskAdd(menu->userId, menu);
                }
                break;
            case HUNTER_ACT_JOIN:
                menu->state = HUNTER_INFO_JOINING;
                gFieldControl = 0;
                break;
            }
            break;
        case MENU_CANCEL:
            return MENU_CANCEL;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    case HUNTER_INFO_JOINING:
        switch (Group_Join(menu, pad)) {
        case MENU_DONE:
            gFieldControl = 1;
            break;
        case MENU_UNDONE:
            menu->state = HUNTER_INFO_MESSAGE;
            gFieldControl = 0;
            break;
        case MENU_CANCEL:
            menu->state = HUNTER_INFO_ACTIONS;
            gFieldControl = 1;
            break;
        case MENU_CLOSE:
            gFieldControl = 1;
            return MENU_CLOSE;
        }
        break;
    case HUNTER_INFO_FRIEND_ADD:
        switch (FriendList_AddInput(pad)) {
        case MENU_DONE:
        case MENU_UNDONE:
            menu->state = HUNTER_INFO_ACTIONS;
            break;
        case MENU_CLOSE:
            gFieldControl = 1;
            return MENU_CLOSE;
        }
        break;
    case HUNTER_INFO_MAIL_FORM:
        switch (ShortMail_Step(pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->state = HUNTER_INFO_ACTIONS;
            break;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    case HUNTER_INFO_FETCH_FAILED:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            HunterInfo_End();
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_UNDONE;
        }
        if (pad & PAD_START) {
            gFieldControl = 1;
            return MENU_CLOSE;
        }
        break;
    case HUNTER_INFO_MESSAGE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->state = HUNTER_INFO_ACTIONS;
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_START) {
            gFieldControl = 1;
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/hunter_info", gHunterInfoPairFmt);
INCLUDE_RODATA("asm/lobby/nonmatchings/hunter_info", gHunterInfoLabelFmt);

extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
s32 LobbySrv_Exists(s8 *id);
s32 LobbySrv_IsCur(s8 *id);
/* Whether the player may join the hunter `user` describes: the hunter must
 * be joinable (PROFILE_STATUS_JOINABLE) on a server the list holds, and not
 * where the player already is; in an area, the player's hunter rank must
 * be in the area kind's range (the room's data), and in a level-1 entry
 * the group's limit (GROUP_LIMIT_*) must admit it. Returns 1 if so; else
 * 0, with the reason's text opened. */
s32 Group_CanJoin(NetUserProfile *user)
{
    u16 *playerHr = &gPlayerText.card.hr;
    RoomData *room = QDL_ROOM;

    if (user->status != PROFILE_STATUS_JOINABLE || LobbySrv_Exists((s8 *)user->serverId) == 0) {
        LobbyText_Open(LOBBY_TEXT_HUNTER_UNAVAILABLE);
        return 0;
    }
    if (LobbySrv_IsCur((s8 *)user->serverId) == 1 && user->area == gLobbyLevel0Sel && user->land == gLobbyLevel1Sel) {
        LobbyText_Open(LOBBY_TEXT_HUNTER_SAME_PLACE);
        return 0;
    }
    if (user->area != 0) {
        if (*playerHr < room->areaKindHr[(s8)(user->areaKind - 1)][0]) {
            LobbyText_Open(LOBBY_TEXT_AREA_RANK);
            return 0;
        }
        if (room->areaKindHr[(s8)(user->areaKind - 1)][1] < *playerHr) {
            LobbyText_Open(LOBBY_TEXT_AREA_RANK);
            return 0;
        }
        if (user->land != 0) {
            switch (user->groupInfo[0]) {
            case GROUP_LIMIT_NONE:
                return 1;
            case GROUP_LIMIT_HR_BELOW_31:
                if (Lobby_HrAtLeast31() == 1) {
                    LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
                    return 0;
                }
                break;
            case GROUP_LIMIT_HR_31_UP:
                if (Lobby_HrAtLeast31() == 0) {
                    LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
                    return 0;
                }
                break;
            }
        }
    }
    return 1;
}

/* Joins the hunter of the window `menu` (enum GroupJoinStep): Group_CanJoin,
 * then the question (yes or no). On yes the join is kept in gLobby
 * (groupJoin, groupId, area, land). On another server the lobby
 * disconnects to connect again to it (LOBBY_QUIT_RECONNECT). On the
 * player's own server, from an area: with the hunter in no area, the area
 * is left; else the fade starts and the hunter's level entry is entered
 * (LobbyLogin_EnterEntry), then the village or the town begins. Returns
 * MENU_RUNNING while it runs; MENU_UNDONE when the join can't be made or
 * the entry failed (with its message); MENU_CANCEL on no; MENU_CLOSE on
 * start. */
s32 Group_Join(HunterInfoMenu *menu, u16 pad)
{
    s32 area;

    switch (menu->joinStep) {
    case GROUP_JOIN_CHECK:
        if (Group_CanJoin(&menu->user) == 1) {
            menu->joinStep++;
            if (LobbySrv_IsCur((s8 *)menu->user.serverId) == 1) {
                LobbyText_Open(LOBBY_TEXT_HUNTER_JOIN_ASK);
            } else {
                LobbyText_Open(LOBBY_TEXT_HUNTER_JOIN_MOVE_ASK);
            }
            Menu_SetYesNo(YES_NO_NO);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
        } else {
            gLobbyMenu->redraw = 1;
            LobbyDialog_Grow();
            return MENU_UNDONE;
        }
        break;
    case GROUP_JOIN_ASK:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_START) {
            return MENU_CLOSE;
        }
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            gLobby->groupJoin = 1;
            memcpy(gLobby->groupId, menu->user.serverId, sizeof(gLobby->groupId));
            gLobby->area = menu->user.area;
            gLobby->land = menu->user.land;
            menu->joinArea = gLobbyLevel0Sel;
            if (LobbySrv_IsCur((s8 *)menu->user.serverId) == 1) {
                area = gLobbyLevel0Sel;
                if (area != 0) {
                    if (menu->user.area == 0 && menu->user.land == 0) {
                        memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
                        menu->joinStep = GROUP_JOIN_LEAVE_AREA;
                    } else {
                        menu->joinStep++;
                        if (area != gLobby->land) {
                            ChatLog_Clear();
                        }
                        FadeB_Start(1);
                    }
                    LobbyText_Open(LOBBY_TEXT_MOVING);
                }
            } else {
                Lobby_Disconnect(LOBBY_QUIT_RECONNECT);
                FadeA_Start(10);
                Stream_Stop(0);
                Stream_Stop(1);
            }
            break;
        case MENU_CANCEL:
            menu->joinStep = GROUP_JOIN_CHECK;
            return MENU_CANCEL;
        }
        break;
    case GROUP_JOIN_ENTER:
        gLobbyMenu->redraw = 1;
        switch (LobbyLogin_EnterEntry(gLobby->area, gLobby->land)) {
        case LOBBY_STEP_DONE:
            if (gLobby->land == 0) {
                menu->joinStep = GROUP_JOIN_CHECK;
                if (gLobbyLevel1Sel == 0 && gLobbyLevel0Sel != 0) {
                    LobbyVillage_BeginStay();
                } else {
                    LobbyVillage_Begin();
                }
                FadeB_Start(10);
                memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            } else {
                menu->joinStep++;
                memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            }
            break;
        case LOBBY_STEP_FAILED:
            menu->joinStep = GROUP_JOIN_CHECK;
            gLobbyLevel1Sel = 0;
            memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            if (gLobby->errText[0] != 0) {
                LobbyDialog_OpenWait(gLobby->errText);
            }
            LobbyDialog_Grow();
            FadeB_Start(2);
            if (menu->joinArea != gLobbyLevel0Sel && gLobbyLevel0Sel != 0) {
                Town_PlaceLocalPlayer(gScene.area);
            }
            return MENU_UNDONE;
        }
        break;
    case GROUP_JOIN_TOWN:
        gLobbyMenu->redraw = 1;
        LobbyTown_Begin();
        break;
    case GROUP_JOIN_LEAVE_AREA:
        LobbyLevel0_Leave();
        break;
    }
    return MENU_RUNNING;
}

/* The actions' input: circle chooses the action under the cursor
 * (MENU_DONE), cross leaves (MENU_CANCEL), start closes the menu
 * (MENU_CLOSE); left and right turn the pages, up and down move the
 * cursor (MENU_RUNNING). */
s32 HunterInfo_ActionInput(HunterInfoMenu *menu, u16 pad)
{
    s32 btn;

    btn = pad & 0xFFFF;
    if (btn & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (btn & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (btn & PAD_START) {
        return MENU_CLOSE;
    }
    if (btn & (PAD_LEFT | PAD_RIGHT)) {
        menu->page = Menu_WrapLeftRight(menu->page, HUNTER_INFO_PAGES);
    } else {
        menu->sel = Menu_WrapUpDown(menu->sel, menu->count);
    }
    return MENU_RUNNING;
}

extern char gHunterInfoPairFmt[];
extern char gHunterInfoLabelFmt[];
extern char gHunterInfoLineFmt[];
extern char gHunterInfoCountFmt[];
/* HUNTER_PAGE_GROUP at (x, y): the hunter's name, id and online mark (the
 * name from its fifth byte, and the mark's icon); the location's four
 * lines as the online window draws the player's own (the server's
 * location; its name with one mark; the area's name with two marks and the
 * land's with three, in the area kind's colour); then, in a land, the
 * group's recruiting message, in a group area (AREA_KIND_GROUPS) its limit
 * and target quest, and its members out of its most. */
void HunterInfo_DrawGroup(HunterInfoMenu *menu, s16 x, s16 y)
{
    HunterCard *card = (HunterCard *)(u8 *)&gHunterPanel; /* the cast: see gHunterPanel */
    u32 color = 0;
    s16 textX = x;
    s16 rowY = y;

    rowY += 20;
    textX += 14;
    Font_SetPos(textX, rowY);
    Text_PrintfColored(gHunterInfoPairFmt, gNetLabels[NET_LABEL_NAME], gLobbyWords[LOBBY_WORD_COLON], menu->user.name);
    Font_SetPos(textX + 144, rowY);
    Text_PrintfColored(gHunterInfoPairFmt, gNetLabels[NET_LABEL_ID], gLobbyWords[LOBBY_WORD_COLON], menu->userId);
    Font_SetPos(textX, rowY += 20);
    Text_PrintfColored(gHunterInfoPairFmt, gNetLabels[NET_LABEL_MARK], gLobbyWords[LOBBY_WORD_COLON], gOnlineMarkNames[card->onlineMark] + 4);
    if (card->onlineMark != ONLINE_MARK_NONE) {
        Icon_Draw(textX + 72, rowY, 25, 20, -1, card->onlineMark + ICON_ONLINE_MARK);
    }
    Font_SetPos(textX, rowY += 20);
    Text_PrintfColored(gHunterInfoLabelFmt, gNetLabels[NET_LABEL_LOCATION]);
    Font_SetColor(0x807F7F7F);
    textX += Text_DisplayLength(gNetLabels[NET_LABEL_LOCATION]) * 9 + 22;
    Font_SetPos(textX, rowY);
    Text_PrintfColored("%s", menu->user.location);
    Font_SetPos(textX, rowY += 20);
    Text_PrintfColored("%s", menu->user.serverName);
    LobbyUi_DrawLevelMarks(textX - 14, rowY + 9, 20, menu->user.serverColor | 0xFF000000, 1);
    Font_SetPos(textX, rowY += 20);
    if (menu->user.area != 0) {
        Text_PrintfColored("%s", menu->user.areaName);
        color = AreaList_KindColor(menu->user.areaKind);
        color |= 0xFF000000;
        LobbyUi_DrawLevelMarks(textX - 14, rowY + 9, 20, color, 2);
    }
    Font_SetPos(textX, rowY += 20);
    if (menu->user.land == 0) {
        return;
    }
    Text_PrintfColored("%s", menu->user.landName);
    LobbyUi_DrawLevelMarks(textX - 14, rowY + 9, 20, color, 3);
    x = x + 14;
    Font_SetPos(x, rowY += 20);
    Text_Printf(gHunterInfoLineFmt, gNetLabels[NET_LABEL_RECRUIT_MSG_LABEL], menu->user.message);
    if (menu->user.areaKind == AREA_KIND_GROUPS) {
        Font_SetPos(x, rowY += 20);
        Text_Printf(gHunterInfoLineFmt, gNetLabels[NET_LABEL_LIMIT_COND], gGroupLimitTexts[menu->user.groupInfo[0]]);
        Font_SetPos(x, rowY += 20);
        Text_Printf(gHunterInfoLineFmt, gNetLabels[NET_LABEL_TARGET_COLON], QDL_ROOM->quests[menu->user.groupInfo[1]].name);
    }
    Font_SetPos(x, rowY + 20);
    Text_Printf(gHunterInfoCountFmt, gNetLabels[NET_LABEL_GROUP_MEMBERS], menu->user.members, menu->user.maxMembers);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/hunter_info", gHunterInfoLineFmt);
INCLUDE_RODATA("asm/lobby/nonmatchings/hunter_info", gHunterInfoCountFmt);

extern char *gHunterTitleTexts[];
/* HUNTER_PAGE_STATUS at (x, y), from the hunter's card (gHunterPanel): the
 * name and id, the title, the weapon class, the symbol colour (none when
 * SYMBOL_COLOR_NONE), the hunter rank and the arena cup's wins (as
 * full-width digits), and the comment. */
void HunterInfo_DrawStatus(HunterInfoMenu *menu, s16 x, s16 y)
{
    HunterCard *card = (HunterCard *)(u8 *)&gHunterPanel; /* the cast: see gHunterPanel */
    s16 rowY = y;
    s32 left = x;
    char num[16];
    char buf[32];

    Font_SetPos(left + 14, rowY += 20);
    Text_PrintfColored(gHunterInfoPairFmt, gNetLabels[NET_LABEL_NAME], gLobbyWords[LOBBY_WORD_COLON], menu->user.name);
    Font_SetPos((s16)(left + 14) + 144, rowY);
    Text_PrintfColored(gHunterInfoPairFmt, gNetLabels[NET_LABEL_ID], gLobbyWords[LOBBY_WORD_COLON], menu->userId);
    Font_SetPos(left + 14, rowY += 20);
    Text_PrintfColored(gHunterInfoLineFmt, gNetLabels[NET_LABEL_TITLE], gHunterTitleTexts[card->title * 2]);
    Font_SetPos(left + 14, rowY += 20);
    Text_PrintfColored(gHunterInfoLineFmt, gNetLabels[NET_LABEL_WEAPON_COLON], gWeaponTypeNames[card->weaponClass]);
    Font_SetPos(left + 14, rowY += 20);
    if (card->symbolColor != SYMBOL_COLOR_NONE) {
        Text_PrintfColored(gHunterInfoLineFmt, gNetLabels[NET_LABEL_SYMBOL_COLOR], gSymbolColorNames[card->symbolColor]);
    }
    Font_SetPos(left + 14, rowY += 20);
    sprintf(num, "%d", card->hr);
    Sjis_Widen(num, buf);
    Text_PrintfColored(gHunterInfoPairFmt, gNetLabels[NET_LABEL_HR], gLobbyWords[LOBBY_WORD_COLON], buf);
    Font_SetPos(left + 14, rowY += 20);
    sprintf(num, "%d", card->versusWins);
    Sjis_Widen(num, buf);
    Text_PrintfColored("~C05%s~C00%s~C05%s", gNetLabels[NET_LABEL_ARENA_CUP], buf, gNetLabels[NET_LABEL_WINS]);
    Font_SetPos(left + 14, rowY += 20);
    Text_PrintfColored(gHunterInfoLabelFmt, gNetLabels[NET_LABEL_COMMENT]);
    Font_SetColor(0x807F7F7F);
    Text_DrawWrapped33(left + 14, rowY + 20, 20, (char *)card->comment);
}

extern s8 gHunterEquipOrder[];
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: declared with s16 coordinates as the draws pass them; with the definition's s32 ones the calls compile differently */
char *Equip_GetName(u8 type, u16 id);
/* HUNTER_PAGE_EQUIPMENT at (x, y): the weapon, then the HUNTER_ARMOR_ROWS
 * armour pieces in gHunterEquipOrder's order, each with its icon and
 * name, from the hunter's card. */
void HunterInfo_DrawEquipment(HunterInfoMenu *menu, s16 x, s16 y)
{
    s16 piece;
    HunterCard *card = (HunterCard *)(u8 *)&gHunterPanel; /* the cast: see gHunterPanel */
    s16 rowY = y;
    s16 i;

    Font_SetPos(x + 60, rowY += 24);
    Lobby_DrawEquipIcon(x + 14, rowY - 10, 36, 36, &card->weapon);
    Text_PrintfColored(gHunterInfoLineFmt, gLobbyWords[LOBBY_WORD_COLON], Equip_GetName(card->weapon.type, card->weapon.id));
    for (i = 0; i < HUNTER_ARMOR_ROWS; i++) {
        piece = gHunterEquipOrder[i];
        rowY += 36;
        Font_SetPos(x + 60, rowY);
        Lobby_DrawEquipIcon(x + 14, rowY - 10, 36, 36, &card->armour[piece]);
        Text_PrintfColored(gHunterInfoLineFmt, gLobbyWords[LOBBY_WORD_COLON], Equip_GetName(card->armour[piece].type, card->armour[piece].id));
    }
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: declared as this file's callers use it, with s16 sizes; with the definition's s32 sizes and colours and s16 kind, the calls compile differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
/* The actions' window, to the left of x: one row per action offered, the
 * row under the cursor lit (dimmed while an action runs). */
void HunterInfo_DrawActions(HunterInfoMenu *menu, s16 x, s16 y)
{
    s16 i;
    s8 dim;

    x -= 130;
    Win_Draw(x, y, 130, menu->count * 22 + 14, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < menu->count; i++) {
        if (i > 0) {
            Gfx_DrawLine(x + 8, y - 2, 114, 2, 0xBDB59F6C);
        }
        if (menu->sel == i) {
            switch (menu->state) {
            case HUNTER_INFO_ACTIONS:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawRowHighlight(x + 8, y - 1, 114, 20, dim);
        }
        Font_SetPos(x + 8, y);
        Text_PrintfColored(gMemberActTexts[menu->acts[i]]);
        y += 22;
    }
}

/* Draws the window at (x, y): its frame and title with the page number,
 * the actions once it has slid in, and the page shown. */
void HunterInfo_Draw(HunterInfoMenu *menu, s16 x, s16 y)
{
    s16 titleY = y;
    s16 pageY;

    Win_Draw(x, y, 340, 270, 0xBDB59F6C, 0xBB402010, 8);
    titleY += 12;
    Gfx_DrawLine(x + 8, titleY, 324, 20, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 14, titleY);
    Text_PrintfColored(gGameMenuLabels[MENU_LABEL_STATUS]);
    Menu_DrawPageNum(x + 250, titleY, menu->page, HUNTER_INFO_PAGES, 1);
    if (menu->state != HUNTER_INFO_SLIDE_IN) {
        HunterInfo_DrawActions(menu, x, y);
    }
    pageY = titleY + 8;
    switch (menu->page) {
    case HUNTER_PAGE_GROUP:
        HunterInfo_DrawGroup(menu, x, pageY);
        break;
    case HUNTER_PAGE_STATUS:
        HunterInfo_DrawStatus(menu, x, pageY);
        break;
    case HUNTER_PAGE_EQUIPMENT:
        HunterInfo_DrawEquipment(menu, x, pageY);
        break;
    }
}

/* Draws the window where it has slid to, unless the details are still
 * being fetched or the short mail form is shown in its place. */
void HunterInfo_DrawIfOpen(void)
{
    HunterInfoMenu *menu = &gHunterInfoMenu;

    Font_SetSpacing(18, 18);
    if (gHunterInfoMenu.state == HUNTER_INFO_FETCH || menu->state == HUNTER_INFO_MAIL_FORM) {
        return;
    }
    HunterInfo_Draw(menu, menu->win.pos, 58);
}

/* The armour's order on the equipment page, by HunterCard.armour's index. */
s8 gHunterEquipOrder[8] = {
#include "lobby/007EC170.inc"
};
