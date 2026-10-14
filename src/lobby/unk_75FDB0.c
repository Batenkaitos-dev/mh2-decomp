/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "item.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"

/* prototypes */
void Quest_SendPaid(void);
void QuestRoom_ResetSettings(void);
/* end of prototypes */

void LobbyPkt_6508Send(void);
void *memset(void *dst, s32 c, u32 n);
extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern M2C_UNK gRoomPassword;
extern s8 D_008B2B61;
extern M2C_UNK gRoomText;

void Quest_SendPaid(void) {
    u8 questPaid;

    questPaid = gLobby->questPaid;
    if (questPaid == 1) {
        LobbyPkt_6508Send();
    }
}

s32 QuestFlags_GetCategory(u32 *q)
{
    s32 cat;
    u32 flags;

    flags = *q;
    if (flags & 0x10) {
        return 4;
    }
    cat = 3;
    if (!((flags >> 3) & 1)) {
loop_3:
        cat = (s8)((cat - 1));
        if (cat < 0) {
            cat = 0;
        } else if (!((flags >> cat) & 1)) {
            goto loop_3;
        }
    }
    return (s8)(cat);
}

s32 Items_HaveEnough(u16 item, s32 n);
s32 Player_CheckRequirement(Player *pl, u16 kind, s32 value);
s32 Save_TestEventFlag(s32 flag);
s32 Box_HasEquip(EquipItem *e);

/* The conditions of the village (ids 0-0x5E): hunter ranks, items, event
 * flags, the weapon class of the local player, requirements. */
s32 Quest_CheckOrderTerms(u8 id, u8 flag)
{
    EquipItem a;
    EquipItem b;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 *hr = &gPlayerText.card.hr;

    switch (id) {
    case 0:
    default:
        return 1;
    case 0x1:
        if (*hr > 0x4 || flag == 0) {
            return 1;
        }
        break;
    case 0x2:
        if (*hr > 0xa || flag == 0) {
            return 1;
        }
        break;
    case 0x3:
        if (*hr > 0x10 || flag == 0) {
            return 1;
        }
        break;
    case 0x4:
        if (*hr > 0x15 || flag == 0) {
            return 1;
        }
        break;
    case 0x5:
        if (*hr > 0x28 || flag == 0) {
            return 1;
        }
        break;
    case 0x6:
        if (*hr > 0x32 || flag == 0) {
            return 1;
        }
        break;
    case 0x7:
        if (*hr > 0x63 || flag == 0) {
            return 1;
        }
        break;
    case 0x8:
        if (*hr > 0xc7 || flag == 0) {
            return 1;
        }
        break;
    case 0x9:
        if (*hr > 0x12b || flag == 0) {
            return 1;
        }
        break;
    case 0xA:
        if (*hr > 0x1f3 || flag == 0) {
            return 1;
        }
        break;
    case 0xB:
        if (*hr > 0x383 || flag == 0) {
            return 1;
        }
        break;
    case 0xC:
        if (*hr == 999 || flag == 0) {
            return 1;
        }
        break;
    case 0xD:
        if (*hr < 0x5) {
            return 1;
        }
        break;
    case 0xE:
        if (*hr < 0xb) {
            return 1;
        }
        break;
    case 0xF:
        if (*hr < 0x11) {
            return 1;
        }
        break;
    case 0x10:
        if (*hr < 0x16) {
            return 1;
        }
        break;
    case 0x11:
        if (*hr < 0x1f) {
            return 1;
        }
        break;
    case 0x12:
        if (*hr < 0x29) {
            return 1;
        }
        break;
    case 0x13:
        if (*hr < 0x33) {
            return 1;
        }
        break;
    case 0x14:
        if (*hr > 0 && *hr < 0x5) {
            return 1;
        }
        break;
    case 0x15:
        if (*hr > 0 && *hr < 0xb) {
            return 1;
        }
        break;
    case 0x16:
        if (*hr > 0 && *hr < 0x11) {
            return 1;
        }
        break;
    case 0x17:
        if (*hr > 0 && *hr < 0x16) {
            return 1;
        }
        break;
    case 0x18:
        if (*hr > 0 && *hr < 0x1f) {
            return 1;
        }
        break;
    case 0x19:
        if (*hr > 0 && *hr < 0x29) {
            return 1;
        }
        break;
    case 0x1A:
        if (*hr > 0 && *hr < 0x33) {
            return 1;
        }
        break;
    case 0x1B:
        if (*hr >= 0x1F && *hr < 0x29) {
            return 1;
        }
        break;
    case 0x1C:
        if (*hr >= 0x1F && *hr < 0x33) {
            return 1;
        }
        break;
    case 0x1D:
        if (Items_HaveEnough(0x2D3, 1) == 1) {
            return 1;
        }
        break;
    case 0x1E:
        if (Items_HaveEnough(0x2D7, 1) == 1 && gSave.hr >= 0x33) {
            return 1;
        }
        break;
    case 0x1F:
        if (Save_TestEventFlag(0x122) == 1) {
            return 1;
        }
        break;
    case 0x20:
        memset((M2C_UNK *)&a, 0, sizeof(EquipItem));
        memset((M2C_UNK *)&b, 0, sizeof(EquipItem));
        a.id = 0x17D;
        a.type = 6;
        b.id = 0x1E4;
        b.type = 6;
        if (Box_HasEquip(&a) == 1 || Box_HasEquip(&b) == 1) {
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
        if (gSave.styleBits != 0 && gSave.hr >= 0x1F && gPlayers[gScene.localPlayer].style == 1) {
            return 1;
        }
        break;
    case 0x24:
        if (gPlayers[gScene.localPlayer].style == 2) {
            return 1;
        }
        break;
    case 0x25:
        if (gSave.styleBits != 0 && gSave.hr >= 0x1F && gPlayers[gScene.localPlayer].style == 2) {
            return 1;
        }
        break;
    case 0x26:
        if (gPlayers[gScene.localPlayer].style == 0) {
            return 1;
        }
        break;
    case 0x27:
        if (gSave.styleBits != 0 && gSave.hr >= 0x1F && gPlayers[gScene.localPlayer].style == 0) {
            return 1;
        }
        break;
    case 0x28:
        if (((u8 *)hr)[0xA] == 0) {
            return 1;
        }
        break;
    case 0x29:
        if (((u8 *)hr)[0xA] == 1) {
            return 1;
        }
        break;
    case 0x2A:
        if (pl->weaponType == 0) {
            return 1;
        }
        break;
    case 0x2B:
        if (pl->weaponType == 3) {
            return 1;
        }
        break;
    case 0x2C:
        if (pl->weaponType == 2) {
            return 1;
        }
        break;
    case 0x2D:
        if (pl->weaponType == 4) {
            return 1;
        }
        break;
    case 0x2E:
        if (pl->weaponType == 6) {
            return 1;
        }
        break;
    case 0x2F:
        if (pl->weaponType == 5) {
            return 1;
        }
        break;
    case 0x30:
        if (pl->weaponType == 1) {
            return 1;
        }
        break;
    case 0x31:
        if (pl->weaponType == 7) {
            return 1;
        }
        break;
    case 0x32:
        if (pl->weaponType == 8) {
            return 1;
        }
        break;
    case 0x33:
        if (pl->weaponType == 10) {
            return 1;
        }
        break;
    case 0x34:
        if (pl->weaponType == 9) {
            return 1;
        }
        break;
    case 0x35:
        if (pl->weaponType == 6 || pl->weaponType == 4) {
            return 1;
        }
        break;
    case 0x36:
        if (pl->weaponType == 7 || pl->weaponType == 0) {
            return 1;
        }
        break;
    case 0x37:
        if (pl->weaponType == 9 || pl->weaponType == 3) {
            return 1;
        }
        break;
    case 0x38:
        if (pl->weaponType == 8 || pl->weaponType == 2) {
            return 1;
        }
        break;
    case 0x39:
        if (pl->weaponType == 1 || pl->weaponType == 5) {
            return 1;
        }
        break;
    case 0x3A:
        if (pl->weaponType == 1 || pl->weaponType == 5 || pl->weaponType == 10) {
            return 1;
        }
        break;
    case 0x3B:
        return Player_CheckRequirement(pl, 0x10, 0);
    case 0x3C:
        return Player_CheckRequirement(pl, 0xD, 0);
    case 0x3D:
        if (*hr >= 0xB) {
            return 1;
        }
        break;
    case 0x3E:
        return Player_CheckRequirement(pl, 0xC, 0);
    case 0x3F:
        if (*(u8 *)((u8 *)&gSave + 0x2091) == 6 && *(u16 *)((u8 *)&gSave + 0x2092) == 0x9C) {
            return 1;
        }
        break;
    case 0x40:
        if (gSave.money <= 100) {
            return 1;
        }
        break;
    case 0x41:
        return Player_CheckRequirement(pl, 0x2, 0xb4);
    case 0x42:
        return Player_CheckRequirement(pl, 0x3, 0x78);
    case 0x43:
        return Player_CheckRequirement(pl, 0x3, 0x4b);
    case 0x44:
        return Player_CheckRequirement(pl, 0x3, 0x23);
    case 0x45:
        return Player_CheckRequirement(pl, 0x4, 0xf);
    case 0x46:
        return Player_CheckRequirement(pl, 0x5, -0xf);
    case 0x47:
        return Player_CheckRequirement(pl, 0x6, 0xf);
    case 0x48:
        return Player_CheckRequirement(pl, 0x7, -0xf);
    case 0x49:
        return Player_CheckRequirement(pl, 0x8, 0xf);
    case 0x4A:
        return Player_CheckRequirement(pl, 0x9, -0xf);
    case 0x4B:
        return Player_CheckRequirement(pl, 0xe, 0xf);
    case 0x4C:
        return Player_CheckRequirement(pl, 0xf, -0xf);
    case 0x4D:
        return Player_CheckRequirement(pl, 0xa, 0xf);
    case 0x4E:
        return Player_CheckRequirement(pl, 0xb, -0xf);
    case 0x4F:
        if (gSave.money >= 50000 || flag == 1) {
            return 1;
        }
        break;
    case 0x50:
        if (gSave.money >= 100000 || flag == 1) {
            return 1;
        }
        break;
    case 0x51:
        if (gSave.money >= 200000 || flag == 1) {
            return 1;
        }
        break;
    case 0x52:
        if (gSave.hr > 0x7 || flag == 0) {
            return 1;
        }
        break;
    case 0x53:
        if (gSave.hr > 0xe || flag == 0) {
            return 1;
        }
        break;
    case 0x54:
        if (gSave.hr > 0x13 || flag == 0) {
            return 1;
        }
        break;
    case 0x55:
        if (gSave.hr > 0x1e || flag == 0) {
            return 1;
        }
        break;
    case 0x56:
        if (gSave.hr > 0x25 || flag == 0) {
            return 1;
        }
        break;
    case 0x57:
        if (gSave.hr > 0x2f || flag == 0) {
            return 1;
        }
        break;
    case 0x58:
        if (gSave.hr >= 0x11) {
            return 1;
        }
        break;
    case 0x59:
        if (gSave.hr >= 0x16) {
            return 1;
        }
        break;
    case 0x5A:
        if (gSave.hr >= 0x1f) {
            return 1;
        }
        break;
    case 0x5B:
        if (gSave.hr >= 0x29) {
            return 1;
        }
        break;
    case 0x5C:
        if (gSave.hr >= 0x33) {
            return 1;
        }
        break;
    case 0x5D:
        if (flag == 0) {
            if (gSave.hr >= 0x29) {
                return 1;
            }
        } else if (gSave.hr >= 0x26) {
            return 1;
        }
        break;
    case 0x5E:
        if (Player_CheckRequirement(pl, 0x11, 0) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

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

void QuestRoom_ResetSettings(void) {
    gRoomPassOn = 0;
    gQuestMinMembers = 3;
    memset(&gRoomPassword, 0, 9);
    D_008B2B61 = 0;
    memset(&gRoomText, 0, 0x3D);
}