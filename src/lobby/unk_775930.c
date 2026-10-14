/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "lobby_state.h"
#include "lobby_npc.h"

/* prototypes */
void SaveOpt37F6_Open(void);
void func_00775980(void);
void TownMenu_CloseQuiet(void);
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

void SaveOpt37F6_Open(void) {
    LobbyMenu_Init();
    gLobbyMenu->cursor = (u8) *(u8 *)((u8 *)&gSave + 0x37F6);
    gHelpLine = gLobbyMenu->cursor + 0xE;
}

void func_00775980(void) {

}

/* Raw addresses kept here: the named form changes the code. */
s32 SaveOpt37F6_Input(s32 pad)
{
    s32 btn;
    u8 sel;

    btn = pad & 0xFFFF;
    if (btn & 0x20) {
        sel = gLobbyMenu->cursor;
        if (gSave.restriction != sel) {
            gSave.restriction = sel;
            LobbySe_Play(8);
        }
        goto block_11;
    }
    if (btn & 0x40) {
        func_00775980();
        return 0x40;
    }
    if (btn & 0x8000) {
        func_00775980();
        return 0x8000;
    }
    *(u8 *)((u8 *)gLobbyMenu + 0x35) = Menu_WrapUpDown((s32) *(u8 *)((u8 *)gLobbyMenu + 0x35), 2);
    gHelpLine = gLobbyMenu->cursor + 0xE;
block_11:
    return pad;
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
u32 Color_PulseAlpha(u32 color);
void Text_PrintfColored(const char *fmt, ...);
void Win_Draw(s32, s32, s32, s32, s32, s32, s16);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
extern char *D_0080C214;
extern char *gLimitOnTexts[];

/* The two-choice window (sub_main 0x5D8256 the choice, gLobbyMenu's +0x35
 * the cursor). The second step of y is made inside the call's arguments:
 * with every step a statement, MWCC folds the constant y into the calls. */
/* Raw addresses kept here: the named form changes the code. */
void SaveOpt37F6_Draw(void)
{
    s16 y = 0xC8;

    Font_SetSpacing(0x12, 0x12);
    Win_Draw(0x12C, y, 0xA0, 0x3C, 0xBDB59F6C, 0x80202020, 3);
    y += 10;
    Gfx_DrawLine(0x136, y - 1 + gLobbyMenu->cursor * 0x16, 0x8C, 0x16, Color_PulseAlpha(0x8080));
    if (gSave.restriction == 0) {
        Font_SetColor(0x807F7F7F);
    } else {
        Font_SetColor(0x80404040);
    }
    Font_SetPos(0x136, y);
    Text_PrintfColored(D_0080C214);
    Gfx_DrawLine(0x136, y += 0x14, 0x8C, 2, 0xBDB59F6C);
    if (gSave.restriction == 1) {
        Font_SetColor(0x807F7F7F);
    } else {
        Font_SetColor(0x80404040);
    }
    Font_SetPos(0x136, y + 4);
    Text_PrintfColored(gLimitOnTexts[0]);
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
extern u8 gHouseAccessName[];
extern u8 D_008BEB75;
extern u8 gChatTextColor;

void HunterCard_Build(void *out)
{
    Player *pl;
    HunterCard c;
    s32 i;

    pl = &gPlayers[gScene.localPlayer];
    memset(&c, 0, sizeof(HunterCard));
    c.hairColor = pl->hairColor;
    if (gSave.styleBits == 0) {
        c.symbolColor = 0xFF;
    } else {
        c.symbolColor = gSave.symbolColor;
    }
    for (i = 0; i < 6; i++) {
        c.armour[i] = pl->equip[i];
        c.armour[i].type = i;
    }
    c.gender = pl->gender;
    c.chatColor = gChatTextColor;
    c.questStatus = pl->questStatus;
    c.arenaRole = gPlayerText.card.arenaRole;
    c.title = gSave.title;
    c.hr = gSave.hr;
    c.weapon = gSave.equip[0];
    c.weaponClass = Weapon_GetClass(gSave.equip[0].type, gSave.equip[0].id);
    c.hairStyle = gSave.hairStyle;
    c.voice = gSave.voice;
    c.charType = gSave.charType;
    c.versusWins = gSave.versusWins;
    c.houseLevel = Village_FacilityLevel(4);
    c.boxLevel = Village_FacilityLevel(5);
    if ((s8)gSave.poogieName[0] == 0) {
        c.poogieOutfit = 0xFF;
    } else {
        c.poogieOutfit = gSave.poogieOutfit;
    }
    c.houseAccess = gHouseAccess;
    c.unkD7 = D_008BEB75;
    c.restriction = gSave.restriction;
    memcpy(c.housePass, gHouseAccessName, 9);
    memcpy(c.comment, gSave.comment, 0x62);
    memcpy(out, &c, sizeof(HunterCard));
}


void HunterCard_Apply(s8 idx, void *src)
{
    Player *pl;
    HunterCard c;
    EquipItem *a;
    u8 k;

    pl = &gPlayers[idx];
    memcpy(&c, src, sizeof(HunterCard));
    pl->hairColor = c.hairColor;
    pl->gender = c.gender;
    pl->hairStyle = c.hairStyle;
    pl->voice = c.voice;
    pl->questStatus = c.questStatus;
    pl->weaponKind = Equip_GetKind(&c.weapon);
    pl->equip[6] = c.weapon;
    pl->weaponType = gWeaponKindClass[pl->weaponKind];
    pl->isGunner = WeaponClass_IsGunner(pl->weaponType);
    pl->weaponFlags = WeaponType_Flags(pl->weaponType);
    pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[6]);
    pl->charType = c.charType;
    k = c.symbolColor;
    if (k != 0xFF) {
        Player_SetColorWord(pl, k);
    } else {
        pl->style = 0xFF;
    }
    a = c.armour;
    pl->equip[0] = a[0];
    pl->equip[1] = a[1];
    pl->equip[2] = a[2];
    pl->equip[3] = a[3];
    pl->equip[4] = a[4];
    pl->equip[5] = a[5];
}

void *memset(void *p, s32 c, u32 n);
void Chat_FilterWords(char *s);
extern u16 gPartIdMax[][7];
extern u16 D_44A1BC[][7];
extern u16 gArmourIdMax[];

/* Clamps the fields of a hunter card received from another player;
 * returns 1 when one of the checked fields was reset. */
s32 HunterCard_Sanitize(void *data)
{
    s16 i;
    s32 changed;
    HunterCard *p;
    HunterCard c;

    changed = 0;
    memcpy(&c, data, sizeof(HunterCard));
    p = &c;
    if (p->hr > 999) {
        p->hr = 999;
        changed = 1;
    }
    if (p->chatColor >= 9) {
        p->chatColor = 0;
        changed = 1;
    }
    if (p->weaponClass >= 11) {
        p->weaponClass = 0;
        changed = 1;
    }
    if ((p->questStatus & 0x1F) >= 5) {
        p->questStatus = 0;
        changed = 1;
    }
    if (p->hairStyle < 0 || p->hairStyle >= D_44A1BC[p->gender][0]) {
        p->hairStyle = 0;
        changed = 1;
    }
    if (p->gender != 0 && p->gender != 1) {
        changed = 1;
        p->gender = 0;
    }
    if (p->voice < 0 || p->voice >= 15) {
        p->voice = 0;
        changed = 1;
    }
    if (p->weapon.type != 6 && p->weapon.type != 7) {
        memset(&p->weapon, 0, sizeof(EquipItem));
        p->weapon.type = 6;
        p->weapon.id = 0x9C;
        p->weaponClass = 4;
        changed = 1;
    } else if (p->weapon.type == 6) {
        if (p->weapon.id > 0x1E6) {
            memset(&p->weapon, 0, sizeof(EquipItem));
            p->weapon.id = 0x9C;
            p->weaponClass = 4;
            changed = 1;
        }
    } else if (p->weapon.id > 0x80) {
        memset(&p->weapon, 0, sizeof(EquipItem));
        changed = 1;
        p->weapon.id = 1;
        p->weaponClass = 1;
    }
    for (i = 0; i < 6; i++) {
        switch (i) {
        case 1:
            if (p->armour[1].id <= 0 || p->armour[1].id > gPartIdMax[p->gender][1]) {
                memset(&p->armour[1], 0, sizeof(EquipItem));
                changed = 1;
                p->armour[1].used = 1;
                p->armour[1].type = 1;
                p->armour[1].id = 1;
            }
            break;
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            if (p->armour[i].id > gArmourIdMax[i]) {
                memset(&p->armour[i], 0, sizeof(EquipItem));
                changed = 1;
                p->armour[i].used = 1;
                p->armour[i].type = i;
                p->armour[i].id = 0;
            }
            break;
        }
    }
    if (p->charType > 3) {
        p->charType = 0;
        changed = 1;
    }
    if (p->symbolColor >= 12) {
        p->symbolColor = 0xFF;
    }
    if (p->title >= 0x94) {
        p->title = 0;
    }
    if (p->restriction > 1) {
        p->restriction = 0;
    }
    Chat_FilterWords((char *)p->comment);
    if (p->versusWins < 0) {
        p->versusWins = 0;
    }
    if (p->houseLevel > 2) {
        p->houseLevel = 0;
    }
    if (p->poogieOutfit != 0xFF && p->poogieOutfit > 4) {
        p->poogieOutfit = 0xFF;
    }
    if (p->houseAccess > 2) {
        p->houseAccess = 0;
    }
    if (p->boxLevel > 4) {
        p->boxLevel = 0;
    }
    if (p->arenaRole > 4) {
        p->arenaRole = 0;
    }
    if (p->unkD7 > 1) {
        p->unkD7 = 0;
    }
    memcpy(data, p, sizeof(HunterCard));
    return changed;
}

void *memset(void *p, s32 c, u32 n);
struct OfferTask;
s16 OfferMenu_BuildList(struct OfferTask *t, s32 b);
void CombShop_Draw(u8 *);
extern u8 gMenuWork;
extern s8 gMenuInputLock;

void CombShop_Open(u8 *t)
{
    u8 *w = (u8 *)&gMenuWork;
    NpcWork *st = (NpcWork *)(t + 0x438);

    gTownMenuOpen = 1;
    gTownMode = 10;
    memset(w, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(CombShop_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    *(s16 *)(w + 0x48) = 5;
    *(s16 *)(w + 0x4A) = 1;
    *(s16 *)(w + 0x4C) = 2;
    *(s16 *)(w + 0x52) = 3;
    if (OfferMenu_BuildList((struct OfferTask *) t, 0) != 0) {
        *(s16 *)(w + 0x4E) = 4;
        *(s16 *)(w + 0x52) += 1;
    }
    w[0x1CD] = st->id;
    LobbySe_Play(0x11);
}

void TownMenu_CloseQuiet(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
    Chat_SetColor(-1);
}
