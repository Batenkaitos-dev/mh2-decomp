/*
 * The quest desk's checks and the quest room's settings:
 * - Quest_CheckOrderTerms: whether the local hunter meets a quest's order
 *   condition (QuestInfo.terms): hunter rank bounds, items, event flags,
 *   equipment, the weapon type, the style, the gender, money, or an entry
 *   requirement (QUEST_REQ_*).
 * - Quest_IsSpecialId: the quests the quest list marks (Quest_GetListFlags).
 * - QuestFlags_GetCategory: the category of a quest's flags that the
 *   player's Player.questStatus carries.
 * - Quest_SendPaid and QuestRoom_ResetSettings: the paid quest's packet and
 *   the quest room's settings (gRoomPassOn, gRoomPassword, gRoomText).
 */
#include "common.h"
#include "lobby_state.h"
#include "item.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "quest.h"

void Quest_SendPaid(void);
void QuestRoom_ResetSettings(void);

void LobbyPkt_QuestPaidSend(void);
void *memset(void *dst, s32 c, u32 n);
extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern s8 gRoomPassword[9];
extern s8 gRoomPasswordEnd;
extern char gRoomText[0x3D];

/* Sends the quest-paid packet (LobbyPkt_QuestPaidSend) when a quest was
 * accepted with its fee paid (LobbyState.questPaid). */
void Quest_SendPaid(void)
{
    u8 questPaid;

    questPaid = gLobby->questPaid;
    if (questPaid == 1) {
        LobbyPkt_QuestPaidSend();
    }
}

/* The category of a quest's flags (QuestInfo.flags, through `infoFlags`): 4
 * when bit 4 is set, else the highest of bits 0-3 set, 0 when none is.
 * QuestDesk_Accept puts it in Player.questStatus. */
s32 QuestFlags_GetCategory(u32 *infoFlags)
{
    s32 cat;
    u32 flags;

    flags = *infoFlags;
    if (flags & 0x10) {
        return 4;
    }
    cat = 3;
    if (!((flags >> 3) & 1)) {
        do {
            cat = (s8)(cat - 1);
            if (cat < 0) {
                cat = 0;
                break;
            }
        } while (!((flags >> cat) & 1));
    }
    return (s8)(cat);
}

s32 Items_HaveEnough(u16 item, s32 n);
s32 Player_CheckRequirement(Player *pl, u16 kind, s32 value);
s32 Save_TestEventFlag(s32 flag);
s32 Box_HasEquip(EquipItem *e);

/*
 * 1 when the local hunter meets order condition `id` (QuestInfo.terms,
 * 0-0x5E; 0 and unknown ids always): the hunter card's rank above or below
 * a bound (`cardHr`) or the save's (gSave.hr), an item held, an event flag,
 * a blade owned or worn, a rate flag, the style, the gender, the weapon type,
 * money, or an entry requirement (Player_CheckRequirement). `ordering` is 1
 * while the quest is ordered and 0 once it is paid: the minimum ranks are
 * checked only when ordering, the money conditions only when not.
 */
s32 Quest_CheckOrderTerms(u8 id, u8 ordering)
{
    EquipItem bladeA;
    EquipItem bladeB;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 *cardHr = &gPlayerText.card.hr;

    switch (id) {
    case 0:
    default:
        return 1;
    case 0x1:
        if (*cardHr > 0x4 || ordering == 0) {
            return 1;
        }
        break;
    case 0x2:
        if (*cardHr > 0xa || ordering == 0) {
            return 1;
        }
        break;
    case 0x3:
        if (*cardHr > 0x10 || ordering == 0) {
            return 1;
        }
        break;
    case 0x4:
        if (*cardHr > 0x15 || ordering == 0) {
            return 1;
        }
        break;
    case 0x5:
        if (*cardHr > 0x28 || ordering == 0) {
            return 1;
        }
        break;
    case 0x6:
        if (*cardHr > 0x32 || ordering == 0) {
            return 1;
        }
        break;
    case 0x7:
        if (*cardHr > 0x63 || ordering == 0) {
            return 1;
        }
        break;
    case 0x8:
        if (*cardHr > 0xc7 || ordering == 0) {
            return 1;
        }
        break;
    case 0x9:
        if (*cardHr > 0x12b || ordering == 0) {
            return 1;
        }
        break;
    case 0xA:
        if (*cardHr > 0x1f3 || ordering == 0) {
            return 1;
        }
        break;
    case 0xB:
        if (*cardHr > 0x383 || ordering == 0) {
            return 1;
        }
        break;
    case 0xC:
        if (*cardHr == 999 || ordering == 0) {
            return 1;
        }
        break;
    case 0xD:
        if (*cardHr < 0x5) {
            return 1;
        }
        break;
    case 0xE:
        if (*cardHr < 0xb) {
            return 1;
        }
        break;
    case 0xF:
        if (*cardHr < 0x11) {
            return 1;
        }
        break;
    case 0x10:
        if (*cardHr < 0x16) {
            return 1;
        }
        break;
    case 0x11:
        if (*cardHr < 0x1f) {
            return 1;
        }
        break;
    case 0x12:
        if (*cardHr < 0x29) {
            return 1;
        }
        break;
    case 0x13:
        if (*cardHr < 0x33) {
            return 1;
        }
        break;
    case 0x14:
        if (*cardHr > 0 && *cardHr < 0x5) {
            return 1;
        }
        break;
    case 0x15:
        if (*cardHr > 0 && *cardHr < 0xb) {
            return 1;
        }
        break;
    case 0x16:
        if (*cardHr > 0 && *cardHr < 0x11) {
            return 1;
        }
        break;
    case 0x17:
        if (*cardHr > 0 && *cardHr < 0x16) {
            return 1;
        }
        break;
    case 0x18:
        if (*cardHr > 0 && *cardHr < 0x1f) {
            return 1;
        }
        break;
    case 0x19:
        if (*cardHr > 0 && *cardHr < 0x29) {
            return 1;
        }
        break;
    case 0x1A:
        if (*cardHr > 0 && *cardHr < 0x33) {
            return 1;
        }
        break;
    case 0x1B:
        if (*cardHr >= 0x1F && *cardHr < 0x29) {
            return 1;
        }
        break;
    case 0x1C:
        if (*cardHr >= 0x1F && *cardHr < 0x33) {
            return 1;
        }
        break;
    case 0x1D:
        if (Items_HaveEnough(0x2D3, 1) == 1) {
            return 1;
        }
        break;
    case 0x1E:
        if (Items_HaveEnough(0x2D7, 1) == 1 && gSave.hr >= 51) {
            return 1;
        }
        break;
    case 0x1F:
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 1) {
            return 1;
        }
        break;
    case 0x20:
        memset(&bladeA, 0, sizeof(bladeA));
        memset(&bladeB, 0, sizeof(bladeB));
        bladeA.id = 0x17D;
        bladeA.type = EQUIP_BLADE;
        bladeB.id = 0x1E4;
        bladeB.type = EQUIP_BLADE;
        if (Box_HasEquip(&bladeA) == 1 || Box_HasEquip(&bladeB) == 1) {
            return 1;
        }
        break;
    case 0x21:
        if (gSave.rateFlags & 0x800) {
            return 1;
        }
        break;
    case 0x22:
        if (gSave.styleBits != 0 && gPlayers[gScene.localPlayer].style == 1) {
            return 1;
        }
        break;
    case 0x23:
        if (gSave.styleBits != 0 && gSave.hr >= 31 && gPlayers[gScene.localPlayer].style == 1) {
            return 1;
        }
        break;
    case 0x24:
        if (gPlayers[gScene.localPlayer].style == 2) {
            return 1;
        }
        break;
    case 0x25:
        if (gSave.styleBits != 0 && gSave.hr >= 31 && gPlayers[gScene.localPlayer].style == 2) {
            return 1;
        }
        break;
    case 0x26:
        if (gPlayers[gScene.localPlayer].style == 0) {
            return 1;
        }
        break;
    case 0x27:
        if (gSave.styleBits != 0 && gSave.hr >= 31 && gPlayers[gScene.localPlayer].style == 0) {
            return 1;
        }
        break;
    case 0x28:
        if (((u8 *)cardHr)[0xA] == 0) {
            return 1;
        }
        break;
    case 0x29:
        if (((u8 *)cardHr)[0xA] == 1) {
            return 1;
        }
        break;
    case 0x2A:
        if (pl->weaponType == WEAPON_GREAT_SWORD) {
            return 1;
        }
        break;
    case 0x2B:
        if (pl->weaponType == WEAPON_LANCE) {
            return 1;
        }
        break;
    case 0x2C:
        if (pl->weaponType == WEAPON_HAMMER) {
            return 1;
        }
        break;
    case 0x2D:
        if (pl->weaponType == WEAPON_SWORD_SHIELD) {
            return 1;
        }
        break;
    case 0x2E:
        if (pl->weaponType == WEAPON_DUAL_SWORDS) {
            return 1;
        }
        break;
    case 0x2F:
        if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            return 1;
        }
        break;
    case 0x30:
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            return 1;
        }
        break;
    case 0x31:
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            return 1;
        }
        break;
    case 0x32:
        if (pl->weaponType == WEAPON_HUNTING_HORN) {
            return 1;
        }
        break;
    case 0x33:
        if (pl->weaponType == WEAPON_BOW) {
            return 1;
        }
        break;
    case 0x34:
        if (pl->weaponType == WEAPON_GUNLANCE) {
            return 1;
        }
        break;
    case 0x35:
        if (pl->weaponType == WEAPON_DUAL_SWORDS || pl->weaponType == WEAPON_SWORD_SHIELD) {
            return 1;
        }
        break;
    case 0x36:
        if (pl->weaponType == WEAPON_LONG_SWORD || pl->weaponType == WEAPON_GREAT_SWORD) {
            return 1;
        }
        break;
    case 0x37:
        if (pl->weaponType == WEAPON_GUNLANCE || pl->weaponType == WEAPON_LANCE) {
            return 1;
        }
        break;
    case 0x38:
        if (pl->weaponType == WEAPON_HUNTING_HORN || pl->weaponType == WEAPON_HAMMER) {
            return 1;
        }
        break;
    case 0x39:
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            return 1;
        }
        break;
    case 0x3A:
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN || pl->weaponType == WEAPON_BOW) {
            return 1;
        }
        break;
    case 0x3B:
        return Player_CheckRequirement(pl, QUEST_REQ_AMMO_ONLY, 0);
    case 0x3C:
        return Player_CheckRequirement(pl, QUEST_REQ_EMPTY_POUCH, 0);
    case 0x3D:
        if (*cardHr >= 0xB) {
            return 1;
        }
        break;
    case 0x3E:
        return Player_CheckRequirement(pl, QUEST_REQ_NO_ARMOR, 0);
    case 0x3F:
        if (gSave.equip[SAVE_EQUIP_WEAPON].type == EQUIP_BLADE && gSave.equip[SAVE_EQUIP_WEAPON].id == 0x9C) {
            return 1;
        }
        break;
    case 0x40:
        if (gSave.money <= 100) {
            return 1;
        }
        break;
    case 0x41:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MIN, 180);
    case 0x42:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MAX, 120);
    case 0x43:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MAX, 75);
    case 0x44:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MAX, 35);
    case 0x45:
        return Player_CheckRequirement(pl, QUEST_REQ_FIRE_RES_MIN, 15);
    case 0x46:
        return Player_CheckRequirement(pl, QUEST_REQ_FIRE_RES_MAX, -15);
    case 0x47:
        return Player_CheckRequirement(pl, QUEST_REQ_WATER_RES_MIN, 15);
    case 0x48:
        return Player_CheckRequirement(pl, QUEST_REQ_WATER_RES_MAX, -15);
    case 0x49:
        return Player_CheckRequirement(pl, QUEST_REQ_THUNDER_RES_MIN, 15);
    case 0x4A:
        return Player_CheckRequirement(pl, QUEST_REQ_THUNDER_RES_MAX, -15);
    case 0x4B:
        return Player_CheckRequirement(pl, QUEST_REQ_DRAGON_RES_MIN, 15);
    case 0x4C:
        return Player_CheckRequirement(pl, QUEST_REQ_DRAGON_RES_MAX, -15);
    case 0x4D:
        return Player_CheckRequirement(pl, QUEST_REQ_ICE_RES_MIN, 15);
    case 0x4E:
        return Player_CheckRequirement(pl, QUEST_REQ_ICE_RES_MAX, -15);
    case 0x4F:
        if (gSave.money >= 50000 || ordering == 1) {
            return 1;
        }
        break;
    case 0x50:
        if (gSave.money >= 100000 || ordering == 1) {
            return 1;
        }
        break;
    case 0x51:
        if (gSave.money >= 200000 || ordering == 1) {
            return 1;
        }
        break;
    case 0x52:
        if (gSave.hr > 7 || ordering == 0) {
            return 1;
        }
        break;
    case 0x53:
        if (gSave.hr > 14 || ordering == 0) {
            return 1;
        }
        break;
    case 0x54:
        if (gSave.hr > 19 || ordering == 0) {
            return 1;
        }
        break;
    case 0x55:
        if (gSave.hr > 30 || ordering == 0) {
            return 1;
        }
        break;
    case 0x56:
        if (gSave.hr > 37 || ordering == 0) {
            return 1;
        }
        break;
    case 0x57:
        if (gSave.hr > 47 || ordering == 0) {
            return 1;
        }
        break;
    case 0x58:
        if (gSave.hr >= 17) {
            return 1;
        }
        break;
    case 0x59:
        if (gSave.hr >= 22) {
            return 1;
        }
        break;
    case 0x5A:
        if (gSave.hr >= 31) {
            return 1;
        }
        break;
    case 0x5B:
        if (gSave.hr >= 41) {
            return 1;
        }
        break;
    case 0x5C:
        if (gSave.hr >= 51) {
            return 1;
        }
        break;
    case 0x5D:
        if (ordering == 0) {
            if (gSave.hr >= 41) {
                return 1;
            }
        } else if (gSave.hr >= 38) {
            return 1;
        }
        break;
    case 0x5E:
        if (Player_CheckRequirement(pl, QUEST_REQ_NO_BARRED_ITEMS, 0) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 1 for the quests the quest list marks (Quest_GetListFlags' 8): a fixed
 * list of disc and event quest ids. */
s32 Quest_IsSpecialId(u16 id)
{
    switch (id & 0xFFFF) {
    case 0x27:
    case 9:
    case 0xBB9:
    case 0x40B:
    case 0xBBA:
    case 0x7EC:
    case 0x802:
    case 0xBBB:
    case 0x2711:
    case 0x2716:
    case 0x271E:
    case 0x2726:
    case 0x2730:
    case 0x2739:
    case 0x273A:
    case 0x273E:
    case 0x273D:
    case 0x273B:
    case 0x273C:
    case 0x2740:
    case 0xBBC:
    case 0xBBD:
    case 0xBBE:
    case 0xBBF:
        return 1;
    }
    return 0;
}

/* Resets the quest room's settings: no password, three members at least, no text. */
void QuestRoom_ResetSettings(void)
{
    gRoomPassOn = 0;
    gQuestMinMembers = 3;
    memset(gRoomPassword, 0, sizeof(gRoomPassword));
    gRoomPasswordEnd = 0;
    memset(gRoomText, 0, sizeof(gRoomText));
}
