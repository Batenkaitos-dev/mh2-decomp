/*
 * The quest room's steps, the quest desk's checks and the quest room's
 * settings:
 * - Money_PayPending settles gRewardMoney; Quest_PostStep posts the
 *   selected quest to the server step by step; LobbyMenu_KbdEntry types a
 *   text with the software keyboard (the room's password).
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
#include "lobby_menu.h"
#include "kbd.h"
#include "pad.h"
#include "lobby_labels.h"
#include "window.h"
#include "quest_order_terms.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_room.h"
#include "lobby_town.h"
#include "lobby_util.h"
#include "offer_menu.h"

void *memset(void *dst, s32 c, u32 n);
extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern s8 gRoomPassword[9];
extern s8 gRoomPasswordEnd;
extern char gRoomText[61];
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
extern u16 gLobbyQuestId;
extern u8 gMenuWork[];

/* Adds gRewardMoney to the save's money (Save_AddMoney) and clears it; the
 * money sound plays unless `silent` or the sum is 0. */
void Money_PayPending(u8 silent)
{
    Save_AddMoney(gRewardMoney);
    if (silent == 0 && gRewardMoney != 0) {
        LobbySe_Play(10);
    }
    gRewardMoney = 0;
}

extern LobbyMenu *gLobbyMenu;

void *memcpy(void *dst, const void *src, u32 n);
s32 LobbyStep_Download(s32 slot, s32 id);
s32 strcmp(const void *a, const void *b);
extern LobbyRecs gLobbyRecs;
extern s32 gTownMenuOpen;
extern s32 gTownMode;
extern s8 gTownModeStep;
extern u32 gQuestPostArg;

/*
 * Posts the selected quest (gLobbyQuestId) as a quest room, one step a frame
 * (QuestPostHead.postStep): a downloaded quest's file is loaded first, then
 * the server is asked for the room list, the entry, the records (the room's
 * password and the member count go in), the settings, the posting `req` and
 * the quest's acceptance. Returns 0 once posted (LobbyState.questPaid set),
 * 2 while running, 1 once an error shown has been closed with PAD_CIRCLE.
 * The hunter takes no pad input in the field meanwhile (gFieldControl).
 */
s32 Quest_PostStep(MsgQuestPost *req)
{
    QuestPostHead *post = (QuestPostHead *)gMenuWork;
    u16 pad;
    s32 i;
    s8 slot;
    QuestInfo *info;

    pad = LobbyPad_Get(0);
    switch (post->postStep) {
    case 0:
        if (gLobbyQuestId < QUEST_ID_DOWNLOADED) {
            post->postStep++;
            gFieldControl = 0;
        } else {
            slot = Quest_DownloadedSlot(gLobbyQuestId);
            switch (LobbyStep_Download(slot, gLobbyQuestId)) {
            case 0:
                info = Quest_CurrentInfo();
                if (info->id != gLobbyQuestId) {
                    LobbyText_Open(99);
                    post->postStep = 8;
                    gFieldControl = 0;
                } else {
                    post->postStep++;
                }
                break;
            case 1:
                LobbyText_Open(99);
                post->postStep = 8;
                gFieldControl = 0;
                break;
            }
        }
        break;
    case 1:
        switch (LobbyLevel2_FetchList()) {
        case 0:
            post->postStep++;
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 2:
        switch (LobbyLevel2_EnterFree(0)) {
        case 0:
            post->postStep++;
            break;
        case 1:
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 3:
        switch (LobbyLevel2_FetchRecords()) {
        case 0:
            memcpy(gLobbyRecs.pass, gRoomPassword, sizeof(gLobbyRecs.pass));
            for (i = 0; i < gLobbyRecs.rec[0].subCount; i++) {
                if (strcmp(gMemberCountTexts[gQuestMinMembers], gLobbyRecs.rec[0].subName[i]) == 0) {
                    gLobbyRecs.rec[0].cur = i;
                    break;
                }
            }
            post->postStep++;
            break;
        case 1:
            post->postStep = 8;
            gTownMenuOpen = 1;
            gTownMode = 24;
            gFieldControl = 0;
            gTownModeStep = 0;
            break;
        }
        break;
    case 4:
        switch (LobbyLevel2_SendSettings()) {
        case 0:
            post->postStep++;
            break;
        case 1:
            break;
        }
        break;
    case 5:
        switch (LobbyLevel2_SendQuestPost(req)) {
        case 0:
            post->postStep++;
            break;
        case 1:
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 6:
        post->postStep++;
        gQuestPostArg = (gQuestPostArg & 0xFFFF0000) | gLobbyQuestId;
        break;
    case 7:
        switch (LobbyLevel2_SendQuestAccept(gQuestPostArg)) {
        case 0:
            post->postStep = 0;
            gLobby->questPaid = 1;
            gFieldControl = 1;
            return 0;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return 1;
        }
        break;
    }
    return 2;
}

/* Types a text into `buf` with the software keyboard (the room's or the
 * records' password), one step a frame (LobbyMenu.screen): opens it, runs it
 * until a text is entered or PAD_SELECT closes it, then closes it. Returns 1
 * once closed, 0 before. */
s32 LobbyMenu_KbdEntry(void *buf)
{
    u16 pad = Pad_GetPressedRepeat(0);

    switch (gLobbyMenu->screen) {
    case 0:
        gLobbyMenu->screen++;
        Kbd_SetPos(58, 80.0f);
        Chat_OpenKbd(3, 6, 8, buf);
        gFieldControl = 0;
        break;
    case 1:
        if (Kbd_Run(buf, (s16)gPadState[0].held, (s16)gPadState[0].pressed) != 0) {
            gLobbyMenu->screen++;
        } else if (pad == PAD_SELECT) {
            gLobbyMenu->screen++;
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case 2:
        Kbd_Cancel();
        gLobbyMenu->screen = 0;
        gFieldControl = 1;
        return 1;
    }
    return 0;
}

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

/*
 * 1 when the local hunter meets order condition `id` (QuestInfo.terms,
 * 0-94; 0 and unknown ids always): the hunter card's rank above or below
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
    case 1:
        if (*cardHr > 4 || ordering == 0) {
            return 1;
        }
        break;
    case 2:
        if (*cardHr > 10 || ordering == 0) {
            return 1;
        }
        break;
    case 3:
        if (*cardHr > 16 || ordering == 0) {
            return 1;
        }
        break;
    case 4:
        if (*cardHr > 21 || ordering == 0) {
            return 1;
        }
        break;
    case 5:
        if (*cardHr > 40 || ordering == 0) {
            return 1;
        }
        break;
    case 6:
        if (*cardHr > 50 || ordering == 0) {
            return 1;
        }
        break;
    case 7:
        if (*cardHr > 99 || ordering == 0) {
            return 1;
        }
        break;
    case 8:
        if (*cardHr > 199 || ordering == 0) {
            return 1;
        }
        break;
    case 9:
        if (*cardHr > 299 || ordering == 0) {
            return 1;
        }
        break;
    case 10:
        if (*cardHr > 499 || ordering == 0) {
            return 1;
        }
        break;
    case 11:
        if (*cardHr > 899 || ordering == 0) {
            return 1;
        }
        break;
    case 12:
        if (*cardHr == 999 || ordering == 0) {
            return 1;
        }
        break;
    case 13:
        if (*cardHr < 5) {
            return 1;
        }
        break;
    case 14:
        if (*cardHr < 11) {
            return 1;
        }
        break;
    case 15:
        if (*cardHr < 17) {
            return 1;
        }
        break;
    case 16:
        if (*cardHr < 22) {
            return 1;
        }
        break;
    case 17:
        if (*cardHr < 31) {
            return 1;
        }
        break;
    case 18:
        if (*cardHr < 41) {
            return 1;
        }
        break;
    case 19:
        if (*cardHr < 51) {
            return 1;
        }
        break;
    case 20:
        if (*cardHr > 0 && *cardHr < 5) {
            return 1;
        }
        break;
    case 21:
        if (*cardHr > 0 && *cardHr < 11) {
            return 1;
        }
        break;
    case 22:
        if (*cardHr > 0 && *cardHr < 17) {
            return 1;
        }
        break;
    case 23:
        if (*cardHr > 0 && *cardHr < 22) {
            return 1;
        }
        break;
    case 24:
        if (*cardHr > 0 && *cardHr < 31) {
            return 1;
        }
        break;
    case 25:
        if (*cardHr > 0 && *cardHr < 41) {
            return 1;
        }
        break;
    case 26:
        if (*cardHr > 0 && *cardHr < 51) {
            return 1;
        }
        break;
    case 27:
        if (*cardHr >= 31 && *cardHr < 41) {
            return 1;
        }
        break;
    case 28:
        if (*cardHr >= 31 && *cardHr < 51) {
            return 1;
        }
        break;
    case 29:
        if (Items_HaveEnough(723, 1) == 1) {
            return 1;
        }
        break;
    case 30:
        if (Items_HaveEnough(727, 1) == 1 && gSave.hr >= 51) {
            return 1;
        }
        break;
    case 31:
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 1) {
            return 1;
        }
        break;
    case 32:
        memset(&bladeA, 0, sizeof(bladeA));
        memset(&bladeB, 0, sizeof(bladeB));
        bladeA.id = 381;
        bladeA.type = EQUIP_BLADE;
        bladeB.id = 484;
        bladeB.type = EQUIP_BLADE;
        if (Box_HasEquip(&bladeA) == 1 || Box_HasEquip(&bladeB) == 1) {
            return 1;
        }
        break;
    case 33:
        if (gSave.rateFlags & 0x800) {
            return 1;
        }
        break;
    case 34:
        if (gSave.styleBits != 0 && gPlayers[gScene.localPlayer].style == 1) {
            return 1;
        }
        break;
    case 35:
        if (gSave.styleBits != 0 && gSave.hr >= 31 && gPlayers[gScene.localPlayer].style == 1) {
            return 1;
        }
        break;
    case 36:
        if (gPlayers[gScene.localPlayer].style == 2) {
            return 1;
        }
        break;
    case 37:
        if (gSave.styleBits != 0 && gSave.hr >= 31 && gPlayers[gScene.localPlayer].style == 2) {
            return 1;
        }
        break;
    case 38:
        if (gPlayers[gScene.localPlayer].style == 0) {
            return 1;
        }
        break;
    case 39:
        if (gSave.styleBits != 0 && gSave.hr >= 31 && gPlayers[gScene.localPlayer].style == 0) {
            return 1;
        }
        break;
    case 40:
        if (((u8 *)cardHr)[10] == 0) {
            return 1;
        }
        break;
    case 41:
        if (((u8 *)cardHr)[10] == 1) {
            return 1;
        }
        break;
    case 42:
        if (pl->weaponType == WEAPON_GREAT_SWORD) {
            return 1;
        }
        break;
    case 43:
        if (pl->weaponType == WEAPON_LANCE) {
            return 1;
        }
        break;
    case 44:
        if (pl->weaponType == WEAPON_HAMMER) {
            return 1;
        }
        break;
    case 45:
        if (pl->weaponType == WEAPON_SWORD_SHIELD) {
            return 1;
        }
        break;
    case 46:
        if (pl->weaponType == WEAPON_DUAL_SWORDS) {
            return 1;
        }
        break;
    case 47:
        if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            return 1;
        }
        break;
    case 48:
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            return 1;
        }
        break;
    case 49:
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            return 1;
        }
        break;
    case 50:
        if (pl->weaponType == WEAPON_HUNTING_HORN) {
            return 1;
        }
        break;
    case 51:
        if (pl->weaponType == WEAPON_BOW) {
            return 1;
        }
        break;
    case 52:
        if (pl->weaponType == WEAPON_GUNLANCE) {
            return 1;
        }
        break;
    case 53:
        if (pl->weaponType == WEAPON_DUAL_SWORDS || pl->weaponType == WEAPON_SWORD_SHIELD) {
            return 1;
        }
        break;
    case 54:
        if (pl->weaponType == WEAPON_LONG_SWORD || pl->weaponType == WEAPON_GREAT_SWORD) {
            return 1;
        }
        break;
    case 55:
        if (pl->weaponType == WEAPON_GUNLANCE || pl->weaponType == WEAPON_LANCE) {
            return 1;
        }
        break;
    case 56:
        if (pl->weaponType == WEAPON_HUNTING_HORN || pl->weaponType == WEAPON_HAMMER) {
            return 1;
        }
        break;
    case 57:
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            return 1;
        }
        break;
    case 58:
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN || pl->weaponType == WEAPON_BOW) {
            return 1;
        }
        break;
    case 59:
        return Player_CheckRequirement(pl, QUEST_REQ_AMMO_ONLY, 0);
    case 60:
        return Player_CheckRequirement(pl, QUEST_REQ_EMPTY_POUCH, 0);
    case 61:
        if (*cardHr >= 11) {
            return 1;
        }
        break;
    case 62:
        return Player_CheckRequirement(pl, QUEST_REQ_NO_ARMOR, 0);
    case 63:
        if (gSave.equip[SAVE_EQUIP_WEAPON].type == EQUIP_BLADE && gSave.equip[SAVE_EQUIP_WEAPON].id == 156) {
            return 1;
        }
        break;
    case 64:
        if (gSave.money <= 100) {
            return 1;
        }
        break;
    case 65:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MIN, 180);
    case 66:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MAX, 120);
    case 67:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MAX, 75);
    case 68:
        return Player_CheckRequirement(pl, QUEST_REQ_DEFENSE_MAX, 35);
    case 69:
        return Player_CheckRequirement(pl, QUEST_REQ_FIRE_RES_MIN, 15);
    case 70:
        return Player_CheckRequirement(pl, QUEST_REQ_FIRE_RES_MAX, -15);
    case 71:
        return Player_CheckRequirement(pl, QUEST_REQ_WATER_RES_MIN, 15);
    case 72:
        return Player_CheckRequirement(pl, QUEST_REQ_WATER_RES_MAX, -15);
    case 73:
        return Player_CheckRequirement(pl, QUEST_REQ_THUNDER_RES_MIN, 15);
    case 74:
        return Player_CheckRequirement(pl, QUEST_REQ_THUNDER_RES_MAX, -15);
    case 75:
        return Player_CheckRequirement(pl, QUEST_REQ_DRAGON_RES_MIN, 15);
    case 76:
        return Player_CheckRequirement(pl, QUEST_REQ_DRAGON_RES_MAX, -15);
    case 77:
        return Player_CheckRequirement(pl, QUEST_REQ_ICE_RES_MIN, 15);
    case 78:
        return Player_CheckRequirement(pl, QUEST_REQ_ICE_RES_MAX, -15);
    case 79:
        if (gSave.money >= 50000 || ordering == 1) {
            return 1;
        }
        break;
    case 80:
        if (gSave.money >= 100000 || ordering == 1) {
            return 1;
        }
        break;
    case 81:
        if (gSave.money >= 200000 || ordering == 1) {
            return 1;
        }
        break;
    case 82:
        if (gSave.hr > 7 || ordering == 0) {
            return 1;
        }
        break;
    case 83:
        if (gSave.hr > 14 || ordering == 0) {
            return 1;
        }
        break;
    case 84:
        if (gSave.hr > 19 || ordering == 0) {
            return 1;
        }
        break;
    case 85:
        if (gSave.hr > 30 || ordering == 0) {
            return 1;
        }
        break;
    case 86:
        if (gSave.hr > 37 || ordering == 0) {
            return 1;
        }
        break;
    case 87:
        if (gSave.hr > 47 || ordering == 0) {
            return 1;
        }
        break;
    case 88:
        if (gSave.hr >= 17) {
            return 1;
        }
        break;
    case 89:
        if (gSave.hr >= 22) {
            return 1;
        }
        break;
    case 90:
        if (gSave.hr >= 31) {
            return 1;
        }
        break;
    case 91:
        if (gSave.hr >= 41) {
            return 1;
        }
        break;
    case 92:
        if (gSave.hr >= 51) {
            return 1;
        }
        break;
    case 93:
        if (ordering == 0) {
            if (gSave.hr >= 41) {
                return 1;
            }
        } else if (gSave.hr >= 38) {
            return 1;
        }
        break;
    case 94:
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
    case 39:
    case 9:
    case 3001:
    case 1035:
    case 3002:
    case 2028:
    case 2050:
    case 3003:
    case 10001:
    case 10006:
    case 10014:
    case 10022:
    case 10032:
    case 10041:
    case 10042:
    case 10046:
    case 10045:
    case 10043:
    case 10044:
    case 10048:
    case 3004:
    case 3005:
    case 3006:
    case 3007:
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
