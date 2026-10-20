/*
 * The hunter card (HunterCard): what a hunter shows the others online.
 * - The town's card of another hunter (TOWN_MODE_PC_CARD): the card of the
 *   hunter the local player faces and talks to (Player.target). PcCard_Open
 *   opens it on that hunter's slot, PcCard_Update takes the buttons (the
 *   card's two pages, and the question of adding the hunter to the friend
 *   list, which the hunter's card may refuse: HunterCard.friendLimit) and
 *   PcCard_Draw draws it over the town. The game menu's member list shows
 *   the same card and question.
 * - The friend limit window, the field menu status page's last entry
 *   (STATUS_FRIEND_LIMIT): two choices, whether other hunters may add this
 *   one to their friend lists (SaveData.friendLimit, sent on the card):
 *   FriendLimitWin_Open, _Input, _Draw and its empty _Close.
 * - HunterCard_Build makes the local hunter's card from the save and the
 *   player, HunterCard_Apply puts a received card on a player, and
 *   HunterCard_Sanitize brings a received card's fields into their ranges.
 * The .rodata is the table of HunterCard_Sanitize's switch.
 */
#include "common.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "pad.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "hud.h"
#include "select.h"
#include "em.h"
#include "item.h"
#include "save.h"
#include "lobby_npc.h"
#include "lobby_labels.h"

void Text_SetSize(s32 size);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void PcCard_Draw(MenuTask *task);
void LobbyMember_LoadEquip(u8 pl);
void LobbySe_Play(s32 se);
void MyHouse_DrawLabel(void);
s32 MemberCard_Draw(u8, u8);
void MemberCard_DrawFriendReg(s16, u8);
/* unit's own declaration: declared s32 (the definition returns an s8): with the s8, PcCard_Update builds differently from the game's code */
s32 Kbd_IsOpen(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern LobbyMenu *gLobbyMenu;

/* Opens the card of the hunter the local player faces (Player.target): the
 * menu state cleared, the card's draw set, the cursor on the hunter's slot,
 * its equipment loaded for the card. With no hunter there (none, or its slot
 * no longer in use) the town goes back to TOWN_MODE_FREE. */
void PcCard_Open(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    Player *other;

    LobbyMenu_Init();
    LobbyMenu_SetDraw(NULL, PcCard_Draw, NULL, NULL);
    other = (Player *)pl->target;
    if (other == NULL || other->active == 0) {
        gTownMode = TOWN_MODE_FREE;
        gTownMenuOpen = 0;
        return;
    }
    gHelpShow = 0;
    gLobbyMenu->cursor = other->index;
    LobbyMember_LoadEquip(gLobbyMenu->cursor);
    LobbySe_Play(LOBBY_SE_CONFIRM);
}

extern NetUser gNetUsers[];
extern u8 gTownAreaMsgPending;
void LobbyText_Open(s32);
void LobbyDialog_Grow(void);
void FriendList_AskAdd(void *id, void *name);
s32 FriendList_AddInput(u16 pad);
s32 memcmp(void *a, void *b, s32 n);

/* The town card's input, by screen (LobbyMenu.screen):
 * - 0, the card: a slot left empty closes it; nothing while the keyboard is
 *   open; PAD_SQUARE on the first page asks to add the hunter to the friend
 *   list (screen 1, FriendList_AskAdd) or, when the hunter's card sets the
 *   friend limit, shows lobby text 0x93 instead (screen 2); PAD_CROSS closes
 *   the card; left and right turn its page (LobbyMenu.step).
 * - 1, the friend question (FriendList_AddInput, PAD_START held back): once
 *   answered, back to the card, or closed when the slot's user changed
 *   meanwhile. LobbyState.connected is 0 while the question is up.
 * - 2, the text: PAD_CIRCLE goes back to the card.
 * Returns 1 when the card closes (the town's update then clears the mode,
 * the draw and the hunter's target), else 0. */
s32 PcCard_Update(u16 pad)
{
    LobbyState *slotBase;   /* gLobby moved on by the chosen slot's NetUser: its members[0] is the slot's member (LOBBY_MEMBER_AT) */

    switch (gLobbyMenu->screen) {
    case 0:
        if (gNetUsers[gLobbyMenu->cursor].id[0] == 0) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return 1;
        }
        if (Kbd_IsOpen() != 0) {
            return 0;
        }
        if ((pad & PAD_SQUARE) && gLobbyMenu->step == 0) {
            /* the hunter's card (its member text's first bytes) sets the friend limit */
            if (((HunterCard *)gLobby->members[gLobbyMenu->cursor].text)->friendLimit != 0) {
                gLobbyMenu->screen = 2;
                gFieldControl = 0;
                LobbyText_Open(LOBBY_TEXT_FRIEND_RESTRICTED);
                LobbyDialog_Grow();
                LobbySe_Play(LOBBY_SE_CONFIRM);
                gLobbyMenu->redraw = 1;
            } else {
                gLobbyMenu->screen++;
                gLobby->connected = 0;
                slotBase = (LobbyState *)((u8 *)gLobby + gLobbyMenu->cursor * sizeof(NetUser));
                FriendList_AskAdd(slotBase->members[0].id, slotBase->members[0].name);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return 1;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            gLobbyMenu->step ^= 1;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        break;
    case 1:
        switch (FriendList_AddInput(pad & ~PAD_START)) {
        case MENU_DONE:
        case MENU_UNDONE:
            gLobbyMenu->screen--;
            gLobby->connected = 1;
            gTownAreaMsgPending = 1;
            /* the slot's user changed while the question was up (the id and the name's first
             * 8 bytes differ from the town's copy): the card closes */
            if (memcmp(&gNetUsers[gLobbyMenu->cursor], &gLobby->members[gLobbyMenu->cursor], 0x10) != 0) {
                LobbySe_Play(LOBBY_SE_CANCEL);
                return 1;
            }
            break;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->screen = 0;
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return 0;
}

/* Draws the town card after the house label (MyHouse_DrawLabel), unless the
 * keyboard is open: MemberCard_Draw on the page chosen, without its frame
 * (flag 0x80) while the lobby text is up or past the card's own screen; on
 * the first page and outside the text, the friend line under it
 * (MemberCard_DrawFriendReg: its line by the screen, card or question). */
void PcCard_Draw(MenuTask *task)
{
    u8 cardFlags;
    u8 curScreen;

    cardFlags = 0;
    Text_SetSize(task->textSize);
    MyHouse_DrawLabel();
    if (Kbd_IsOpen() == 0) {
        if (gLobbyMenu->redraw != 0 || gLobbyMenu->screen > 0) {
            cardFlags = 0x80;
        }
        MemberCard_Draw(gLobbyMenu->cursor, gLobbyMenu->step | cardFlags);
        curScreen = gLobbyMenu->screen;
        if ((curScreen != 2) && (gLobbyMenu->step == 0)) {
            MemberCard_DrawFriendReg(0x184, curScreen);
        }
    }
}

/* prototypes */
void FriendLimitWin_Open(void);
void FriendLimitWin_Close(void);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
/* end of prototypes */

void LobbyMenu_Init(void);
extern LobbyMenu *gLobbyMenu;
void CamFocus_End(void);
void Chat_SetColor(s16);
void LobbySe_Play(s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* Opens the friend limit window: the menu state cleared, the cursor on the
 * saved choice (SaveData.friendLimit), and that choice's help line. */
void FriendLimitWin_Open(void)
{
    LobbyMenu_Init();
    gLobbyMenu->cursor = gSave.friendLimit;
    gHelpLine = gLobbyMenu->cursor + 0xE; /* HELP_PAGE_STATUS: the choices' lines from 0xE */
}

/* The friend limit window's close: nothing to undo (the status page draws
 * the window only while its entry is open). */
void FriendLimitWin_Close(void)
{
}

/* The friend limit window's input: PAD_CIRCLE saves the choice under the
 * cursor (SaveData.friendLimit; the town sends it on the card) with the
 * confirm sound when it changed, and the window stays; PAD_CROSS and
 * PAD_START close it and are returned (the status page goes back to its list,
 * or the field menu closes); up and down move between the two choices, each
 * with its help line. Returns pad otherwise. */
s32 FriendLimitWin_Input(s32 pad)
{
    s32 btn;
    u8 sel;

    btn = pad & PAD_ALL; /* the buttons are the low 16 bits */
    if (btn & PAD_CIRCLE) {
        sel = gLobbyMenu->cursor;
        if (gSave.friendLimit != sel) {
            gSave.friendLimit = sel;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else if (btn & PAD_CROSS) {
        FriendLimitWin_Close();
        return PAD_CROSS;
    } else if (btn & PAD_START) {
        FriendLimitWin_Close();
        return PAD_START;
    } else {
        gLobbyMenu->cursor = Menu_WrapUpDown(gLobbyMenu->cursor, 2); /* the two choices */
        gHelpLine = gLobbyMenu->cursor + 0xE; /* HELP_PAGE_STATUS: the choices' lines from 0xE */
    }
    return pad;
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
u32 Color_PulseAlpha(u32 color);
void Text_PrintfColored(const char *fmt, ...);
void Win_Draw(s32, s32, s32, s32, s32, s32, s16);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);

/* Draws the friend limit window over the status page: the window, the
 * pulsing bar on the cursor's line, the first choice (no limit), a divider,
 * then the second (the limit); the saved choice is drawn bright, the other
 * dim. The second step of y is made inside the call's arguments: with every
 * step its own statement, the compiler folds the constant y into the calls,
 * unlike the game's code. */
void FriendLimitWin_Draw(void)
{
    s16 y = 0xC8;

    Font_SetSpacing(0x12, 0x12);
    Win_Draw(0x12C, y, 0xA0, 0x3C, 0xBDB59F6C, 0x80202020, 3);
    y += 10;
    Gfx_DrawLine(0x136, y - 1 + gLobbyMenu->cursor * 0x16, 0x8C, 0x16, Color_PulseAlpha(0x8080));
    if (gSave.friendLimit == 0) {
        Font_SetColor(STATUS_COLOR_VALUE);
    } else {
        Font_SetColor(0x80404040);
    }
    Font_SetPos(0x136, y);
    Text_PrintfColored(gTownLabels[TOWN_LABEL_NO_LIMIT]);
    Gfx_DrawLine(0x136, y += 0x14, 0x8C, 2, 0xBDB59F6C);
    if (gSave.friendLimit == 1) {
        Font_SetColor(STATUS_COLOR_VALUE);
    } else {
        Font_SetColor(0x80404040);
    }
    Font_SetPos(0x136, y + 4);
    Text_PrintfColored(gTownLabels[TOWN_LABEL_LIMIT]);
}

void *memcpy(void *dst, const void *src, u32 n);
u16 Equip_GetKind(EquipItem *e);
u8 WeaponClass_IsGunner(u8 cls);
u8 WeaponType_Flags(u8 cls);
u8 Equip_GetBladeSpecial(EquipItem *e);
void Player_SetColorWord(Player *pl, u8 idx);
extern u8 gWeaponKindClass[];

void *memset(void *p, s32 c, u32 n);
u8 Village_FacilityLevel(s32 k);
u8 Weapon_GetClass(u8 type, u16 id);
extern u8 gHouseAccess;
extern u8 gHousePassword[];
extern u8 gChatToLobby;
extern u8 gOnlineMark;

/* Builds the local hunter's card into out (sizeof(HunterCard) bytes): the
 * look and armour from the hunter's player (each armour piece's type set to
 * its slot), the weapon, rank, title, voice and comment from the save, the
 * style's colour (0xFF until a style is earned), the house's and the box's
 * levels, the Poogie's outfit (0xFF while it has no name), the house's
 * access and password, the online mark, the arena role, the friend limit
 * and the lobby chat flag (gChatToLobby). */
void HunterCard_Build(void *out)
{
    Player *pl;
    HunterCard myCard;
    s32 i;

    pl = &gPlayers[gScene.localPlayer];
    memset(&myCard, 0, sizeof(HunterCard));
    myCard.hairColor = pl->hairColor;
    if (gSave.styleBits == 0) {
        myCard.symbolColor = 0xFF;
    } else {
        myCard.symbolColor = gSave.symbolColor;
    }
    for (i = 0; i < 6; i++) {
        myCard.armour[i] = pl->equip[i];
        myCard.armour[i].type = i;
    }
    myCard.gender = pl->gender;
    myCard.onlineMark = gOnlineMark;
    myCard.questStatus = pl->questStatus;
    myCard.arenaRole = gPlayerText.card.arenaRole;
    myCard.title = gSave.title;
    myCard.hr = gSave.hr;
    myCard.weapon = gSave.equip[SAVE_EQUIP_WEAPON];
    myCard.weaponClass = Weapon_GetClass(gSave.equip[SAVE_EQUIP_WEAPON].type, gSave.equip[SAVE_EQUIP_WEAPON].id);
    myCard.hairStyle = gSave.hairStyle;
    myCard.voice = gSave.voice;
    myCard.charType = gSave.charType;
    myCard.versusWins = gSave.versusWins;
    myCard.houseLevel = Village_FacilityLevel(VILLAGE_FACILITY_HOUSE);
    myCard.boxLevel = Village_FacilityLevel(VILLAGE_FACILITY_BOX);
    if ((s8)gSave.poogieName[0] == 0) {
        myCard.poogieOutfit = 0xFF;
    } else {
        myCard.poogieOutfit = gSave.poogieOutfit;
    }
    myCard.houseAccess = gHouseAccess;
    myCard.chatToLobby = gChatToLobby;
    myCard.friendLimit = gSave.friendLimit;
    memcpy(myCard.housePass, gHousePassword, sizeof(myCard.housePass));
    memcpy(myCard.comment, gSave.comment, sizeof(myCard.comment));
    memcpy(out, &myCard, sizeof(HunterCard));
}


/* Puts a received card (src) on player idx: the look, the voice and the
 * quest status; the weapon with its kind, type, gunner flag, the type's
 * flags and the blade's special; the style's colour (none for 0xFF); and
 * the six armour pieces. */
void HunterCard_Apply(s8 idx, void *src)
{
    Player *pl;
    HunterCard card;
    EquipItem *pieces;
    u8 color;

    pl = &gPlayers[idx];
    memcpy(&card, src, sizeof(HunterCard));
    pl->hairColor = card.hairColor;
    pl->gender = card.gender;
    pl->hairStyle = card.hairStyle;
    pl->voice = card.voice;
    pl->questStatus = card.questStatus;
    pl->weaponKind = Equip_GetKind(&card.weapon);
    pl->equip[EQUIP_SLOT_WEAPON] = card.weapon;
    pl->weaponType = gWeaponKindClass[pl->weaponKind];
    pl->isGunner = WeaponClass_IsGunner(pl->weaponType);
    pl->weaponFlags = WeaponType_Flags(pl->weaponType);
    pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[EQUIP_SLOT_WEAPON]);
    pl->charType = card.charType;
    color = card.symbolColor;
    if (color != 0xFF) {
        Player_SetColorWord(pl, color);
    } else {
        pl->style = 0xFF;
    }
    pieces = card.armour;
    pl->equip[0] = pieces[0];
    pl->equip[1] = pieces[1];
    pl->equip[2] = pieces[2];
    pl->equip[3] = pieces[3];
    pl->equip[4] = pieces[4];
    pl->equip[5] = pieces[5];
}

void *memset(void *p, s32 c, u32 n);
void Chat_FilterWords(char *s);
extern u16 gPartIdMax[][7];
extern u16 gArmourIdMax[];

/* Brings the fields of a hunter card received from another player into
 * their ranges, in place. Returns 1 when one of the first fields was reset
 * (rank, online mark, weapon class, quest status, hair, gender, voice,
 * weapon, armour, character type); the later ones (style colour, title,
 * friend limit, wins, levels, Poogie outfit, house access, arena role, lobby
 * chat flag) are reset without counting, and the comment's words are
 * filtered (Chat_FilterWords). */
s32 HunterCard_Sanitize(void *data)
{
    s16 i;
    s32 changed;
    HunterCard *card;
    HunterCard copy;

    changed = 0;
    memcpy(&copy, data, sizeof(HunterCard));
    card = &copy;
    if (card->hr > HR_MAX) {
        card->hr = HR_MAX;
        changed = 1;
    }
    if (card->onlineMark >= ONLINE_MARK_COUNT) {
        card->onlineMark = 0;
        changed = 1;
    }
    if (card->weaponClass >= WEAPON_BOW + 1) { /* past the last weapon type */
        card->weaponClass = 0;
        changed = 1;
    }
    if ((card->questStatus & 0x1F) >= 5) { /* the bits below QUEST_STATUS_SET_OUT: the quest category is 0-4 (QuestFlags_GetCategory) */
        card->questStatus = 0;
        changed = 1;
    }
    if (card->hairStyle < 0 || card->hairStyle >= gPartIdMax[card->gender][6]) {
        card->hairStyle = 0;
        changed = 1;
    }
    if (card->gender != 0 && card->gender != 1) {
        changed = 1;
        card->gender = 0;
    }
    if (card->voice < 0 || card->voice >= CHARA_VOICE_COUNT) {
        card->voice = 0;
        changed = 1;
    }
    if (card->weapon.type != EQUIP_BLADE && card->weapon.type != EQUIP_GUN) { /* not a weapon: a sword and shield */
        memset(&card->weapon, 0, sizeof(EquipItem));
        card->weapon.type = EQUIP_BLADE;
        card->weapon.id = 0x9C; /* the default sword and shield */
        card->weaponClass = WEAPON_SWORD_SHIELD;
        changed = 1;
    } else if (card->weapon.type == EQUIP_BLADE) {
        if (card->weapon.id > BLADE_ENTRY_COUNT) {
            memset(&card->weapon, 0, sizeof(EquipItem));
            card->weapon.id = 0x9C; /* the default sword and shield */
            card->weaponClass = WEAPON_SWORD_SHIELD;
            changed = 1;
        }
    } else if (card->weapon.id > GUN_ENTRY_COUNT) { /* a gun past the table: bowgun 1 */
        memset(&card->weapon, 0, sizeof(EquipItem));
        changed = 1;
        card->weapon.id = 1;
        card->weaponClass = WEAPON_HEAVY_BOWGUN;
    }
    for (i = 0; i < 6; i++) {
        switch (i) {
        case EQUIP_SLOT_FACE: /* the face: 1 up to the gender's count, else face 1 */
            if (card->armour[1].id <= 0 || card->armour[1].id > gPartIdMax[card->gender][1]) {
                memset(&card->armour[1], 0, sizeof(EquipItem));
                changed = 1;
                card->armour[1].used = 1;
                card->armour[1].type = 1;
                card->armour[1].id = 1;
            }
            break;
        case EQUIP_LEGS:
        case EQUIP_HEAD:
        case EQUIP_BODY:
        case EQUIP_ARMS:
        case EQUIP_WAIST: /* a piece: up to its slot's limit, else none */
            if (card->armour[i].id > gArmourIdMax[i]) {
                memset(&card->armour[i], 0, sizeof(EquipItem));
                changed = 1;
                card->armour[i].used = 1;
                card->armour[i].type = i;
                card->armour[i].id = 0;
            }
            break;
        }
    }
    if (card->charType > 3) { /* the four character types */
        card->charType = 0;
        changed = 1;
    }
    if (card->symbolColor >= 12) { /* the twelve style colours (gPlayerColorWords); 0xFF none */
        card->symbolColor = 0xFF;
    }
    if (card->title >= HUNTER_TITLE_COUNT) {
        card->title = 0;
    }
    if (card->friendLimit > 1) {
        card->friendLimit = 0;
    }
    Chat_FilterWords((char *)card->comment);
    if (card->versusWins < 0) {
        card->versusWins = 0;
    }
    if (card->houseLevel > 2) {
        card->houseLevel = 0;
    }
    if (card->poogieOutfit != 0xFF && card->poogieOutfit > 4) {
        card->poogieOutfit = 0xFF;
    }
    if (card->houseAccess > HOUSE_ACCESS_PASSWORD) {
        card->houseAccess = 0;
    }
    if (card->boxLevel > 4) {
        card->boxLevel = 0;
    }
    if (card->arenaRole > 4) {
        card->arenaRole = 0;
    }
    if (card->chatToLobby > 1) {
        card->chatToLobby = 0;
    }
    memcpy(data, card, sizeof(HunterCard));
    return changed;
}
