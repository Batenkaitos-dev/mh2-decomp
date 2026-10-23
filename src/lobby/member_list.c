/*
 * The town's member list, a page of the game menu online: the other
 * hunters in the town (gNetUsers and LobbyState.members, LOBBY_MEMBER_COUNT
 * slots), LobbyMember_OpenList, LobbyMember_ListInput and
 * LobbyMember_DrawList; a chosen member's card (MemberCard_Draw) and the
 * question of adding the member to the friend list. Also here:
 * LobbyMember_InMyParty (whether a member is in the local hunter's quest
 * party) and LobbyMenu_DrawMemberNames (the town's window of the members'
 * names with their marks, drawn over the town). A member's record is its
 * NetUser, whose text is the member's HunterCard: its quest status, online
 * mark, symbol colour and weapon class make the marks.
 * The .rodata holds the empty row's text, the .data its pointer and the
 * member list's window.
 */
#include "common.h"
#include "lobby_menu.h"
#include "pad.h"
#include "hud.h"
#include "game_menu.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "window.h"
#include "render_state.h"
#include "lobby_text.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "card.h"
#include "model.h"
#include "text.h"
#include "member_list.h"
#include "font.h"
#include "friend_add.h"
#include "game_util.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "member_card.h"
#include "name_tags.h"
#include "libc.h"

extern NetUser gNetUsers[];
extern LobbyMenu *gLobbyMenu;

/* The member list's empty row (a slot with no member). */
#pragma rodata on
char gMemberListEmptyText[13] = {
#include "lobby/0083B4A8.inc"
};
#pragma rodata off

struct Frame;
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);

/* The game menu's open handler for the member list: sets up the lobby
 * menu's work with the cursor off the local hunter's own slot, hides the
 * help line and returns 0. */
s32 LobbyMember_OpenList(void)
{
    u8 cursor;

    LobbyMenu_Init();
    cursor = gLobbyMenu->cursor;
    if (cursor == gScene.localPlayer) {
        gLobbyMenu->cursor = (u8) (cursor + 1);
    }
    gLobbyMenu->step = 0;
    gHelpShow = 0;
    return 0;
}

extern u8 gTownAreaMsgPending;

/* The member list's input, by screen (LobbyMenu.screen, enum
 * MemberListScreen):
 * - MEMBER_LIST_ROWS: up and down move over the LOBBY_MEMBER_COUNT slots,
 *   skipping the local hunter's; PAD_CIRCLE opens a present member's card
 *   (LobbyMember_LoadEquip); PAD_START closes the game menu; PAD_CROSS is
 *   returned (back to the game menu's list).
 * - MEMBER_LIST_CARD: left and right turn its page (LobbyMenu.step);
 *   PAD_SQUARE on the first page asks to add the member to the friend list
 *   (FriendList_AskAdd), or shows LOBBY_TEXT_FRIEND_RESTRICTED instead when
 *   the member's friend limit is set; PAD_CROSS, or the member leaving,
 *   goes back to the list.
 * - MEMBER_LIST_FRIEND_ASK (FriendList_AddInput): once answered, back to
 *   the card, or to the list when the slot's member changed meanwhile.
 * - MEMBER_LIST_NOTICE: PAD_CIRCLE goes back to the card.
 * Returns the buttons left while the lobby is connected, else 0. */
u16 LobbyMember_ListInput(u16 pad)
{
    s16 sel;
    u8 *selPtr;
    u8 prevSel;
    LobbyState *slotBase;   /* gLobby moved on by the chosen slot's NetUser: its members[0] is the slot's member (LOBBY_MEMBER_AT) */

    s32 buttons;

    switch (gLobbyMenu->screen) {
    case MEMBER_LIST_ROWS:
        if (pad & PAD_START) {
            GameMenu_Back();
            return 0;
        }
        if (pad & PAD_CIRCLE) {
            if (gPlayers[gLobbyMenu->cursor].active != 0 && NetUser_State(gLobbyMenu->cursor) != NET_USER_ABSENT) {
                LobbyMember_LoadEquip(gLobbyMenu->cursor);
                gLobbyMenu->screen++;
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
            break;
        }
        if (pad & PAD_CROSS) {
            return pad;
        }
        selPtr = &gLobbyMenu->cursor;
        prevSel = *selPtr;
        sel = prevSel;
        if (pad & PAD_UP) {
            sel--;
            if (sel < 0) {
                sel = LOBBY_MEMBER_COUNT - 1;
            }
            if (sel == gScene.localPlayer) {
                sel--;
            }
            if (sel < 0) {
                sel = LOBBY_MEMBER_COUNT - 1;
            }
        } else if (pad & PAD_DOWN) {
            sel++;
            if (sel >= LOBBY_MEMBER_COUNT) {
                sel = 0;
            }
            if (sel == gScene.localPlayer) {
                sel++;
            }
            if (sel >= LOBBY_MEMBER_COUNT) {
                sel = 0;
            }
        }
        if (sel != prevSel) {
            *selPtr = sel;
            LobbySe_Play(LOBBY_SE_SCROLL);
            gLobbyMenu->step = 0;
        }
        break;
    case MEMBER_LIST_CARD:
        buttons = pad;
        if (buttons & PAD_START) {
            GameMenu_Back();
            return 0;
        }
        if (NetUser_State(gLobbyMenu->cursor) == NET_USER_ABSENT) {
            gLobbyMenu->screen--;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (buttons & PAD_CROSS) {
            gLobbyMenu->screen--;
            LobbySe_Play(LOBBY_SE_CANCEL);
            pad = buttons & ~PAD_CROSS;
        } else if (gLobbyMenu->step == 0 && (buttons & PAD_SQUARE)) {
            if (((HunterCard *)gLobby->members[gLobbyMenu->cursor].text)->friendLimit != 0) {
                gLobbyMenu->screen = MEMBER_LIST_NOTICE;
                gFieldControl = 0;
                LobbyText_Open(LOBBY_TEXT_FRIEND_RESTRICTED);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                gLobbyMenu->screen++;
                gLobby->connected = 0;
                slotBase = (LobbyState *)((u8 *)gLobby + gLobbyMenu->cursor * sizeof(NetUser));
                FriendList_AskAdd(slotBase->members[0].id, slotBase->members[0].name);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else if (buttons & (PAD_LEFT | PAD_RIGHT)) {
            gLobbyMenu->step ^= 1;
            LobbySe_Play(LOBBY_SE_OPEN);
        }
        break;
    case MEMBER_LIST_FRIEND_ASK:
        pad &= ~PAD_START;
        switch (FriendList_AddInput(pad)) {
        case 0:
        case 1:
            gLobby->connected = 1;
            gTownAreaMsgPending = 1;
            if (memcmp(gNetUsers[gLobbyMenu->cursor].id, gLobby->members[gLobbyMenu->cursor].id, LOBBY_USER_ID_LEN) != 0) {
                gLobbyMenu->screen -= 2;
            } else {
                gLobbyMenu->screen--;
            }
            break;
        }
        break;
    case MEMBER_LIST_NOTICE:
        buttons = pad;
        gLobbyMenu->redraw = 1;
        if (buttons & PAD_CIRCLE) {
            gLobbyMenu->screen = MEMBER_LIST_CARD;
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        pad = buttons & ~PAD_CROSS;
        break;
    }
    if (gLobby->connected != 0) {
        return pad;
    }
    return 0;
}

struct ListWin;
/* unit's own declaration: y is declared s16 (the definition takes an s32): with s32 the compiler passes it unlike the game's code in both draws */
void Icon_DrawOnlineMark(s32 x, s16 y, s32 h, s32 color, s16 mark);
extern char *gMemberListEmptyRow;
extern ListWin gMemberListPanel;
extern char gMemberListTexts[10][20];   /* the member list texts */
extern char *gMemberListRows[];          /* the member list (text pointers) */

#define LM ((LobbyMenu *)gLobbyMenu)


/* The member list's draw. On the list (MEMBER_LIST_ROWS): the game menu's
 * list dimmed, then one row per other slot, the member's name in text
 * colour 6 when in the local hunter's party (LobbyMember_InMyParty), 10
 * when in no area, else 0 (with gChatHideNames online, the id instead of
 * the name), or the empty row; then each present member's marks: its
 * online mark (Icon_DrawOnlineMark) when its user state is 0, and while it is in
 * an area its quest's (its card's questStatus: LOBBY_SPRITE_QUEST_PAID or
 * _TAKEN in its category's colour, blinking once it has set out). On the
 * card and the friend question: MemberCard_Draw, with the friend line on
 * the card's first page. */
void LobbyMember_DrawList(void)
{
    u8 onlineMarks[LOBBY_MEMBER_COUNT];
    u8 *markRead;
    Player *pl;
    s32 memberOff;
    u8 *markOut;
    char *name;
    NetUser *user;
    char *buf;
    char **list;
    s32 i;
    s16 color;
    s32 cur;
    s16 y;
    u8 flags;
    u8 cardFlags;

    cardFlags = MEMBER_CARD_NO_FRAME;
    switch (LM->screen) {
    case MEMBER_LIST_ROWS:
        GameMenu_DrawList(1);
        i = 0;
        pl = gPlayers;
        memberOff = 0;
        user = gNetUsers;
        markRead = onlineMarks;
        buf = gMemberListTexts[0];
        markOut = markRead;
        list = gMemberListRows;
        do {
            if (i != gScene.localPlayer) {
                if (pl->active != 0 && NetUser_State(i) != NET_USER_ABSENT) {
                    name = LOBBY_MEMBER_AT(memberOff).name;
                    if (Game_IsOnline() == 1) {
                        if (gChatHideNames != 0) {
                            name = user->id;
                        }
                        *markOut = LOBBY_MEMBER_CARD_AT(memberOff)->onlineMark;
                    }
                    if (LobbyMember_InMyParty(i) == 1) {
                        color = 6;
                    } else if (pl->area == 0) {
                        color = 10;
                    } else {
                        color = 0;
                    }
                    sprintf(buf, "~C%02d~%%%s~%%", color, name);
                    *list = buf;
                } else {
                    *list = gMemberListEmptyRow;
                }
                buf += 20;
                list++;
            }
            i++;
            pl++;
            memberOff += 284;
            user++;
            markOut++;
        } while (i < LOBBY_MEMBER_COUNT);
        cur = LM->cursor;
        if (gScene.localPlayer < cur) {
            cur--;
        }
        gMemberListPanel.items = gMemberListRows;
        ListPanel_DrawDim(&gMemberListPanel, 0, cur);
      {
        Player *rowPlayer;
        s32 rowOff;
        s32 j;

        y = 62;
        j = 0;
        rowPlayer = gPlayers;
        rowOff = 0;
        do {
            if (j != gScene.localPlayer) {
                if (rowPlayer->active != 0) {
                    if (NetUser_State(j) == NET_USER_HERE) {
                        Icon_DrawOnlineMark(420, y, 18, -1, *markRead);
                    }
                    if (rowPlayer->area != 0) {
                        flags = LOBBY_MEMBER_CARD_AT(rowOff)->questStatus;
                        if (flags & QUEST_STATUS_PAID) {
                            if (!(flags & QUEST_STATUS_SET_OUT) || (gFrameTick & 0x10)) {
                                LobbySprite_Draw(398, y, 18, gQuestCategoryColors[flags & QUEST_STATUS_CATEGORY], LOBBY_SPRITE_QUEST_PAID);
                            }
                        } else if (flags & QUEST_STATUS_TAKEN) {
                            if (!(flags & QUEST_STATUS_SET_OUT) || (gFrameTick & 0x10)) {
                                LobbySprite_Draw(398, y, 18, gQuestCategoryColors[flags & QUEST_STATUS_CATEGORY], LOBBY_SPRITE_QUEST_TAKEN);
                            }
                        }
                    }
                }
                y += 21;
            }
            j++;
            rowPlayer++;
            markRead++;
            rowOff += 284;
        } while (j < LOBBY_MEMBER_COUNT);
      }
        break;
    case MEMBER_LIST_CARD:
        cardFlags = 0;
    case MEMBER_LIST_FRIEND_ASK:
        if (LM->redraw != 0) {
            cardFlags = MEMBER_CARD_NO_FRAME;
        }
        MemberCard_Draw(LM->cursor, LM->step | cardFlags);
        if (LM->step == 0) {
            MemberCard_DrawFriendReg(388, LM->screen - MEMBER_LIST_CARD);
        }
        break;
    }
}

/* Whether member slot idx is in the local hunter's quest party: 1 when a
 * quest is taken and its id is among the party's four players, else 0. */
s32 LobbyMember_InMyParty(s32 idx)
{
    s32 i;

    if (gLobby->questTaken == 0) {
        return 0;
    }
    for (i = 0; i < LOBBY_ROOM_USER_COUNT; i++) {
        if (memcmp(gLobby->members[idx].id, gLobby->players[i].id, LOBBY_USER_ID_LEN) == 0) {
            return 1;
        }
    }
    return 0;
}

/* unit's own declaration: x, y and h are declared s16 and the colours u32 (the definition takes s32s): with the definition's types the compiler builds the call unlike the game's code */
void Win_Draw(s16 x, s16 y, s32 w, s16 h, u32 c1, u32 c2, s32 k);
/* unit's own declaration: x and y are declared s16 and the icon a u8 (the definition takes s32s, an s16 and an s8): with the definition's types the compiler extends y before the call, unlike the game's code */
void Icon_DrawWeaponClass(s16, s16, s32, s32, u8, s32);
extern s32 gTownMode;
extern u8 gLobbyLevel1Sel;
extern u8 gMemberNamesOff;
extern char gMemberNameBufs[LOBBY_MEMBER_COUNT][20];
extern char *gMemberNameTexts[];
extern u32 gPlayerColorWords[];

/* The town's window of the members' names, drawn over the town after its
 * name tags (TownPc_DrawTargetBanner, whose text size it takes). Only online,
 * in TOWN_MODE_FREE, _SET_OUT and _SEAT_WAIT, with a level-1 entry chosen
 * and the window not turned off (gMemberNamesOff): a title line, then the
 * local hunter first and the others in slot order, each name coloured as in
 * the member list. A member in an area also gets, from its hunter card, its
 * quest's mark (as in the member list), its online mark (while the card's
 * chatToLobby is set, every other 16 frames a green sprite instead), its
 * symbol colour's square (SYMBOL_COLOR_CYCLE cycles through the colours) and its weapon
 * class's icon. */
void LobbyMenu_DrawMemberNames(MenuTask *task)
{
    s32 i;
    char *name;
    s16 y;
    s32 slot;
    NetUser *user;
    Player *pl;
    s8 count;
    HunterCard *list[LOBBY_MEMBER_COUNT];
    HunterCard *card;
    HunterCard **rowEntry;
    s32 rowIndex;
    u8 flags;
    u8 colorIdx;

    TownPc_DrawTargetBanner(task);
    Gs_SetState(RS_ALPHA_REF, 0);
    Text_SetSize(task->textSize);
    Tex_Prepare(1, 341);
    Tex_Bind(341);
    if (Game_IsOnline() == 0) {
        return;
    }
    if (gTownMode != TOWN_MODE_FREE && gTownMode != TOWN_MODE_SET_OUT && gTownMode != TOWN_MODE_SEAT_WAIT) {
        return;
    }
    if (gLobbyLevel1Sel == 0) {
        return;
    }
    if (gMemberNamesOff != 0) {
        return;
    }
    memset(list, 0, sizeof(list));
    count = 0;
    i = 0;
    slot = 1;
    user = gNetUsers;
    pl = gPlayers;
    for (; i < LOBBY_MEMBER_COUNT; i++) {
        if ((s8)user->id[0] != 0) {
            card = (HunterCard *)user->text;
            if (i == gScene.localPlayer) {
                rowIndex = 0;
                slot--;
            } else {
                rowIndex = slot;
            }
            rowEntry = &list[rowIndex];
            *rowEntry = card;
            if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                if (LobbyMember_InMyParty(i) == 1) {
                    name = gMemberNameBufs[rowIndex];
                    sprintf(name, "~C06~%%%s~%%", user->id);
                } else if (pl->area == 0) {
                    name = gMemberNameBufs[rowIndex];
                    sprintf(name, "~C10~%%%s~%%", user->id);
                    *rowEntry = NULL;
                } else {
                    name = gMemberNameBufs[rowIndex];
                    sprintf(name, "~C00~%%%s~%%", user->id);
                }
            } else {
                if (LobbyMember_InMyParty(i) == 1) {
                    name = gMemberNameBufs[rowIndex];
                    sprintf(name, "~C06~%%%s~%%", user->name);
                } else if (pl->area == 0) {
                    name = gMemberNameBufs[rowIndex];
                    sprintf(name, "~C10~%%%s~%%", user->name);
                    *rowEntry = NULL;
                } else {
                    name = gMemberNameBufs[rowIndex];
                    sprintf(name, "~C00~%%%s~%%", user->name);
                }
            }
            gMemberNameTexts[rowIndex] = name;
            slot++;
            count++;
        }
        user++;
        pl++;
    }
    Font_SetSpacing(18, 18);
    i = count;
    y = 420 - (i + 1) * 22;
    Win_Draw(460, y, 150, (i + 1) * 22 - 2, 0xBDB59F6C, 0x80202020, 6);
    Gfx_DrawLine(460, y, 148, 20, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(466, y + 2);
    Text_Printf("%s", gTownLabels[TOWN_LABEL_SHOW_ON]);
    y += 22;
    for (slot = 0; slot < i; slot++) {
        card = list[slot];
        Gfx_DrawLine(460, y - 2, 148, 2, 0x80FFFFFF);
        if (card != NULL) {
            flags = card->questStatus;
            if (flags & QUEST_STATUS_PAID) {
                LobbySprite_Draw(562, y, 18, gQuestCategoryColors[flags & QUEST_STATUS_CATEGORY], LOBBY_SPRITE_QUEST_PAID);
            } else if (flags & QUEST_STATUS_TAKEN) {
                if (!(flags & QUEST_STATUS_SET_OUT) || (gFrameTick & 0x10)) {
                    LobbySprite_Draw(562, y, 18, gQuestCategoryColors[flags & QUEST_STATUS_CATEGORY], LOBBY_SPRITE_QUEST_TAKEN);
                }
            }
            if (card->chatToLobby == 0 || (gFrameTick & 0x10)) {
                Icon_DrawOnlineMark(586, y, 18, -1, card->onlineMark);
            } else {
                LobbySprite_Draw(586, y, 20, 0xFF40FF40, 19);
            }
            colorIdx = card->symbolColor;
            if (colorIdx != 255) {
                if (colorIdx == SYMBOL_COLOR_CYCLE) {
                    Gfx_DrawLine(460, y, 22, 20, Color_Cycle8());
                } else {
                    Gfx_DrawLine(460, y, 22, 20, gPlayerColorWords[colorIdx]);
                }
            }
            Icon_DrawWeaponClass(460, y, 20, -1, card->weaponClass, 0);
        }
        Font_SetPos(484, y);
        Text_Printf("%s", gMemberNameTexts[slot]);
        y += 22;
    }
}

/* The empty row's pointer, and the member list's window. */
extern char gMemberListEmptyText[];
char *gMemberListEmptyRow = (char *)gMemberListEmptyText;
ListWin gMemberListPanel = {
#include "lobby/007F2050.inc"
};
