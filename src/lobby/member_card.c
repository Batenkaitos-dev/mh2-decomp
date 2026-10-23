/*
 * The lobby's member card.
 *
 * - The member card (MemberCard_Draw): a member's hunter card (HunterCard,
 *   the first bytes of its NetUser's text in LobbyState.members), page
 *   MEMBER_CARD_PAGE_PROFILE or MEMBER_CARD_PAGE_EQUIP, and the friend
 *   registration line under it (MemberCard_DrawFriendReg).
 *   MemberCard_DrawStatus and MemberCard_DrawFriendLimit draw two of its
 *   profile's lines, and Profile_DrawStatus and Profile_DrawFriendLimit the
 *   local hunter's own (the status with its online mark's icon).
 * - LobbyMember_LoadEquip puts a member's equipment, from its hunter card,
 *   on its player; Equip_GetSellPrice gives what a piece sells for.
 *
 * The chat's sending (Chat_SendToTargets) comes before this file.
 *
 * Its data: the card's two panels and windows, the indices of its tabs'
 * texts, and the friend registration window and icon.
 */
#include "common.h"
#include "window.h"
#include "chat_log.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "plsel.h"
#include "lobby_globals.h"
#include "syssave.h"
#include "area.h"
#include "lobby_labels.h"
#include "render_state.h"
#include "lobby_menu.h"
#include "texture.h"
#include "area_draw.h"
#include "text.h"
#include "member_card.h"
#include "draw_helpers.h"
#include "item_shop.h"
#include "menu_util.h"
#include "place_info.h"

/* end of prototypes */

extern ListPanel gMemberCardPanels[];
extern Panel gMemberCardWindows[];
extern u8 gMemberCardTabTexts[];
extern char *gHunterTitleTexts[][2];
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
s32 sprintf(char *buf, char *fmt, ...);
void Font_SetPos(s32 x, s32 y); /* unit's own declaration: int x and y, though the function takes shorts: the friend limit and status lines pass their int x and y on without narrowing them, as the game's code does */
void Text_Draw(char *s);
void Player_DrawSkillsPlain(Player *a, s32 b, s16 c);
void Sjis_Widen(char *src, char *dst);
struct V00793350_0;

/* Draws member pl's card (pl: its gPlayers slot), page
 * MEMBER_CARD_PAGE_PROFILE (name, id, title, weapon class, symbol colour,
 * hr (LOBBY_WORD_UNREGISTERED when 0), versus wins, status, friend limit,
 * area, comment) or MEMBER_CARD_PAGE_EQUIP (equipment and skills). With
 * MEMBER_CARD_NO_FRAME set in page, the card's frame (Menu_DrawFrame) is
 * left out. While the
 * member's player isn't active, only the empty panel is drawn and it
 * returns -1; else 0.
 * tab holds the player's active flag before it holds the page: one
 * variable for both keeps the registers of the game's code. */
s32 MemberCard_Draw(u8 pl, u8 page)
{
    char numText[16];
    char buf[64];
    Player *player;
    s32 noFrame;
    HunterCard *card;
    s32 tab;

    noFrame = page & MEMBER_CARD_NO_FRAME;
    player = &gPlayers[pl];
    if (noFrame) {
        page &= MEMBER_CARD_PAGE_MASK;
    }
    tab = player->active;
    if (tab == 0) {
        ListPanel_Draw(&gMemberCardPanels[page], 0, gStatusTitleTexts[0], -1);
        return -1;
    }
    sprintf(buf, gStatusTitleTexts[1], page + 1);
    tab = page;
    ListPanel_Draw(&gMemberCardPanels[tab], 0, buf, -1);
    if (!noFrame) {
        Menu_DrawFrame(gMemberCardPanels, 0);
    }
    Window_DrawText(&gMemberCardWindows[tab], (s16 *)0, (u8 *)gStatusBoxTitles[gMemberCardTabTexts[tab]]);
    card = LobbyMember_GetCard(player->index);
    switch (page) {
    case MEMBER_CARD_PAGE_PROFILE:
        Text_SetColor(0);
        Font_SetPos(378, 82);
        Text_Draw(gLobby->members[player->index].name);
        Font_SetPos(378, 102);
        Text_Draw(gLobby->members[player->index].id);
        Font_SetPos(378, 122);
        Text_PrintfColored(gHunterTitleTexts[card->title][0]);
        Font_SetPos(378, 142);
        StatusPage_DrawWeaponClass(player);
        Font_SetPos(306, 162);
        if (card->symbolColor != SYMBOL_COLOR_NONE) {
            Text_PrintfColored("~C05%s~C00%s", gNetLabels[NET_LABEL_SYMBOL_COLOR], gSymbolColorNames[card->symbolColor]);
        }
        Font_SetPos(378, 182);
        if (card->hr != 0) {
            Text_PrintfColored(gMemberCardNumFmt, card->hr);
        } else {
            Text_Draw(gLobbyWords[LOBBY_WORD_UNREGISTERED]);
        }
        Font_SetPos(414, 202);
        sprintf(numText, "%4d", card->versusWins);
        Sjis_Widen(numText, buf);
        Text_PrintfColored(buf);
        MemberCard_DrawStatus(card, 396, 222);
        MemberCard_DrawFriendLimit(card, 468, 242);
        Font_SetPos(414, 262);
        Text_PrintfColored(LobbyArea_GetName(player->area));
        Text_DrawWrapped33(306, 314, 20, (char *)card->comment);
        break;
    case MEMBER_CARD_PAGE_EQUIP:
        PersonalStatus_DrawEquipment(player);
        Player_DrawSkillsPlain(player, 306, 294);
        break;
    }
    return 0;
}

extern Panel gFriendRegWindow;   /* read as a Window (Window_DrawTextB2); 24 bytes */
extern Icon gFriendRegIcons[];

/* Draws friend registration text textIndex (gFriendRegTexts) in its
 * window at height y, under the member card; text 0 has its icon beside
 * it. */
void MemberCard_DrawFriendReg(s16 y, u8 textIndex)
{
    gFriendRegWindow.y = y;
    Window_DrawTextB2((Window *)&gFriendRegWindow, (u8 *) gFriendRegTexts[textIndex]);
    if (textIndex == 0) {
        gFriendRegIcons[0].y = y - 2;
        Draw_Icons(gFriendRegIcons, 1);
    }
}

/* Member member's hunter card: the text of its NetUser in LobbyState.members,
 * whose first bytes are a HunterCard. */
HunterCard *LobbyMember_GetCard(u8 member)
{
    return (HunterCard *)gLobby->members[member].text;
}

void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: int parameters, though Icon_Draw takes shorts: the two status functions pass their int x and y on without narrowing them, as the game's code does */
extern char *gMemberStatusTexts[];

/* Puts member pl's equipment, from its hunter card, on its player
 * (gPlayers[pl]): the weapon, then HunterCard.armour[i] in Player.equip[i]
 * (the armour pieces by their types, and slot 1, the face), and works out
 * the player's skills again. */
void LobbyMember_LoadEquip(u8 pl)
{
    HunterCard *card = LobbyMember_GetCard(pl);
    Player *player = &gPlayers[pl];

    player->equip[EQUIP_SLOT_WEAPON] = card->weapon;
    player->equip[0] = card->armour[0];
    player->equip[1] = card->armour[1];
    player->equip[2] = card->armour[2];
    player->equip[3] = card->armour[3];
    player->equip[4] = card->armour[4];
    player->equip[5] = card->armour[5];
    Player_CalcSkills(player);
}

/* What a piece of equipment sells for: an armour piece's price from
 * Armor_GetSellPrice, half a weapon's price (gBladeData, gGunData), 0 for
 * any other type. */
s32 Equip_GetSellPrice(EquipItem *item)
{
    s32 price;

    switch (item->type) {
    case EQUIP_LEGS:
    case EQUIP_HEAD:
    case EQUIP_BODY:
    case EQUIP_ARMS:
    case EQUIP_WAIST:
        return Armor_GetSellPrice(item);
    case EQUIP_BLADE:
        price = gBladeData[item->id].price;
        break;
    case EQUIP_GUN:
        price = gGunData[item->id].price;
        break;
    default:
        return 0;
    }
    return price / 2;
}

/* Prints a member card's friend limit line at (x, y): gFriendLimitTexts by
 * the card's friend limit. */
void MemberCard_DrawFriendLimit(HunterCard *card, s32 x, s32 y)
{
    Font_SetPos(x, y);
    Text_PrintfColored(gFriendLimitTexts[card->friendLimit]);
}

/* Prints the local hunter's friend limit line at (x, y), from its own card
 * (gPlayerText). */
void Profile_DrawFriendLimit(s32 x, s32 y)
{
    Font_SetPos(x, y);
    Text_PrintfColored(gFriendLimitTexts[gPlayerText.card.friendLimit]);
}

/* Prints a member card's status line at (x, y): the text of its online mark
 * (gMemberStatusTexts) and, for a mark other than ONLINE_MARK_NONE, the
 * mark's icon. */
void MemberCard_DrawStatus(HunterCard *card, s32 x, s32 y)
{
    u8 mark;

    Font_SetPos(x, y);
    Text_PrintfColored(gMemberStatusTexts[card->onlineMark]);
    mark = card->onlineMark;
    if (mark != ONLINE_MARK_NONE) {
        Icon_Draw(x, y, 25, 20, -1, mark + ICON_ONLINE_MARK);
    }
}

/* Prints the local hunter's status line at (x, y) as MemberCard_DrawStatus
 * does, from its own card (gPlayerText). */
void Profile_DrawStatus(s32 x, s32 y)
{
    HunterCard *card = (HunterCard *)&gPlayerText; /* the cast, not &gPlayerText.card: with it the compiler keeps the card's address in a register, as the game's code does */

    Font_SetPos(x, y);
    Text_PrintfColored(gMemberStatusTexts[card->onlineMark]);
    if (card->onlineMark != ONLINE_MARK_NONE) {
        Icon_Draw(x, y, 25, 20, -1, card->onlineMark + ICON_ONLINE_MARK);
    }
}

/* The file's data: the member card's two panels and windows, the indices
 * of its tabs' texts, and the friend registration window and icon. */
ListPanel gMemberCardPanels[2] = {
#include "lobby/007FF280.inc"
};
Panel gMemberCardWindows[2] = {
#include "lobby/007FF2C0.inc"
};
u8 gMemberCardTabTexts[16] = {
#include "lobby/007FF2F0.inc"
};
Panel gFriendRegWindow = {
#include "lobby/007FF300.inc"
};
Icon gFriendRegIcons[1] = {
#include "lobby/007FF318.inc"
};
