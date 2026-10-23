/*
 * The trader met in the field (EM_KIND_TRADER): the code from its first
 * function, EmTrader_MotionSounds, to its table's empty hook.
 *
 * The trader is a monster of one state group (include/em_trader.h names its
 * states, motions, message kinds and offers): it stands, rests or wanders
 * near its home, turns to the local player who comes near and talks
 * (EmTrader_GreetPlayer, EmTrader_Talk). EmTrader_RunTalk runs the talk:
 * each message is a row of gEmTraderMsgs (em.h's EmTraderMsg, copied into
 * the work area, EmTraderWork), whose kind says what an answer does: go on,
 * choose, make an offer, trade, give a gift or a chat line. An offer picks
 * what the trader takes and gives from the player's pouch, the map's trade
 * table (by variant), the item the quest wants (gQuest.traderWantItem) and
 * a few fixed swaps; EmTrader_ShowMessage and EmTrader_FormatMessage put
 * the item's name into the message's text.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_trader.h"
#include "sound.h"
#include "player.h"
#include "area.h"
#include "chat_log.h"
#include "shadow.h"
#include "area_draw.h"
#include "bgm.h"
#include "capsule.h"
#include "window.h"
#include "engine_math.h"
#include "game_util.h"
#include "gm_load.h"
#include "rand.h"
#include "libc.h"
#include "vec_math.h"
#include "save.h"
/* prototypes of callees */
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* unit's own declaration: declared here as the callers use it; with the definition's s16 (void *obj, u8 state, u8 sub), the calls in EmTrader_TurnToYaw and EmPlesioth_WaitForBait compile differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
extern char *gItemNames[];
extern char gEmTraderMsgBufs[];
extern u16 EmTrader_GiftItemsV0[];
extern u16 EmTrader_GiftItemsV1[];
/* unit's own declaration: declared here as the callers use it; with the definition's u8 (Player *p, u16 item, s16 n), the calls in EmTrader_RunTalk compile differently */
u16 Pouch_Add(Player *pl, u16 item, s16 n);
extern u16 EmTrader_TradesForestHillsV0[];
extern u16 EmTrader_TradesForestHillsV1[];
extern u16 EmTrader_TradesSwampV0[];
extern u16 EmTrader_TradesSwampV1[];
extern u16 EmTrader_TradesJungleV0[];
extern u16 EmTrader_TradesJungleV1[];
extern u16 EmTrader_TradesDesertV0[];
extern u16 EmTrader_TradesDesertV1[];
extern u16 EmTrader_TradesSnowyMountainsV0[];
extern u16 EmTrader_TradesSnowyMountainsV1[];
typedef struct EmTradeRate {
    u16 weight;
    u16 item;
} EmTradeRate;
extern EmTradeRate EmTrader_TradeRates[];
/* end of prototypes */

/* The trader's (EM_KIND_TRADER) own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmTraderWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk001[8 - 1];
    f32 home[3];                /* 0x008: Init: the position; State0_01 puts the trader back there */
    u16 homeYaw;                /* 0x014: Init: rotY; State0_01 restores rotY and targetYaw from it */
    u8 unk016[24 - 22];
    u8 greeted;                 /* 0x018: Init: 0; EmTrader_GreetPlayer: 1 (nothing reads it) */
    u8 unk019[26 - 25];
    u8 noAnswer;                /* 0x01A: 1: Trade takes no answer (State0_05 and State0_09 set it before calling Trade while their motion plays); Trade and the states clear it */
    u8 choice;                  /* 0x01B: the talk's cursor (Notice_OpenTalk's last argument): 0 on the first answer, 1 on the second (ShowMessage's start); Trade moves it */
    s16 msgWait;                /* 0x01C: ShowMessage: 30 frames for the kinds that wait (TRADER_MSG_CHOICE, _CONFIRM_TRADE, _ASK_GIFT, _DRILL_CHARM), else 0; Trade counts it down and takes no answer before 0 */
    u8 msgBuf;                  /* 0x01E: which of the two 256-byte text buffers of gEmTraderMsgBufs holds the message (SetMessage flips it) */
    u8 unk01F[32 - 31];
    EmTraderMsg msg;            /* 0x020: the message shown: ShowMessage copies its gEmTraderMsgs row here, then points text at the buffer SetMessage wrote */
    u8 unk028[256 - 40];
} EmTraderWork;
typedef char EmTraderWork_size_check[(sizeof(EmTraderWork) == sizeof(EmWork)) ? 1 : -1];

/*
 * Plays the sounds of the trader's motions on their frames, switched on
 * work->fxMotion (EM_MOTION_ID(0, N): TRADER_MOT_HIDDEN to
 * TRADER_MOT_FIDGET); the last case falls into default.
 */
void EmTrader_MotionSounds(void *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, TRADER_MOT_HIDDEN):
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 26, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_JUMP_OUT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 10, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 27, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 6, 67, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 4, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 6, 58, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REST):
        Em_PlaySeAtBoneOnFrame((Em *) em, 176, 7, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 26, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 212, 26, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REST_FIDGET):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 8, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 178, 9, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 92, 28, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 206, 28, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_GREET):
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 6, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 26, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_TALK):
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 11, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, Sound_RandomSe(34, 6, 35, 0), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, Sound_RandomSe(35, 6, 36, 0), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 182, Sound_RandomSe(36, 6, 9, 2), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 238, 13, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 350, 7, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 26, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 178, 26, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 286, 28, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 78, 25, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REFUSE_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 5, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 22, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 164, 23, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 24, 61, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 27, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 122, 28, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REFUSE_C):
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 3, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 50, 10, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 13, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 23, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, 23, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 164, 22, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 27, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 126, 28, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REFUSE_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 0, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 1, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 114, 2, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 176, 3, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 23, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 27, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 76, 27, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 160, 27, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 22, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 234, 22, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 60, 61, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 114, 74, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 136, 74, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 176, 74, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_AGREE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 12, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 126, 4, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 28, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 160, 27, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 80, 74, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_WALK):
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, Sound_RandomSe(14, 4, 15, 2), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 108, Sound_RandomSe(17, 2, 14, 4), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 20, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 21, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 76, 20, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 106, 21, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 26, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 80, 26, TRADER_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_FIDGET):
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, Sound_RandomSe(14, 4, 17, 4), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 180, Sound_RandomSe(16, 3, 17, 3), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 300, Sound_RandomSe(14, 3, 15, 3), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 372, Sound_RandomSe(17, 3, 16, 3), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 476, Sound_RandomSe(14, 3, 15, 3), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 600, Sound_RandomSe(14, 3, 16, 3), TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, 28, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 254, 26, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 446, 26, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 126, 76, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 176, 76, TRADER_BONE_ROOT);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 220, 76, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 420, 25, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 440, 25, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 20, TRADER_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 290, 21, TRADER_BONE_ROOT);
        
    default:
        break;
    }
}

/*
 * Returns the gEmTraderMsgs row for the trader's variant: the id itself for
 * variant 0 and for ids 34..99 (the shared chat and drill rows), else
 * (id + em->variant) & 0xFF (the variant rows come in pairs). Takes and
 * returns a message id, not an item.
 */
s32 EmTrader_MsgIdForVariant(Em *em, s32 item)
{
    s32 id;
    u8 variant;

    variant = em->variant;
    if (variant == 0) {
        return item;
    }
    id = item & 0xFF;
    if ((id >= 34) && (id < 89)) {
        return item;
    }
    if ((id >= 89) && (id < 100)) {
        return item;
    }
    return (id + variant) & 0xFF;
}

/*
 * Flips w->msgBuf and copies w->msg.text into that 256-byte half of
 * gEmTraderMsgBufs, replacing each "||" with the colored name of
 * em->msgItem (gItemNames). Its callers pass the local Player cast as Em,
 * so the item printed is Player.tradeGiveItem (offset 0x636).
 */
void EmTrader_FormatMessage(Em *em, EmTraderWork *w)
{
    char *src;
    char *dst;

    w->msgBuf ^= 1;
    src = w->msg.text;
    dst = gEmTraderMsgBufs + (w->msgBuf << 8);
    while (*src != 0) {
        if (src[0] == 124 && src[1] == 124) {
            src += 2;
            dst += (s8) sprintf(dst, "~C05%s~C00", gItemNames[em->msgItem]);
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

/*
 * Returns 1 when the local player is active, visible, in the trader's area
 * (and Obj_InCurrentArea) and within 500 units of it, else 0.
 */
s32 EmTrader_IsPlayerNear(Em *em)
{
    Player *player = &gPlayers[gScene.localPlayer];

    if (!player->active || !player->visible) {
        return 0;
    }
    if (em->area != player->area) {
        return 0;
    }
    if (!(u8) Obj_InCurrentArea(player)) {
        return 0;
    }
    if (Vec_Distance((f32 *) player->pos, (f32 *) &em->posX) <= 500.0f) {
        return 1;
    }
    return 0;
}

/*
 * Starts message id: sets em->msgId, clears tradeStep and msgTimer, puts the
 * cursor on the second answer, copies the gEmTraderMsgs row into w->msg,
 * formats its text, and gives the kinds TRADER_MSG_CHOICE, _CONFIRM_TRADE,
 * _ASK_GIFT and _DRILL_CHARM a 30-frame answer wait; keeps the talk open
 * (Player.tradeMsgTimer = 10).
 */
void EmTrader_ShowMessage(Em *em, u16 id)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    EmTraderMsg *shown = &w->msg;
    Player *player = &gPlayers[gScene.localPlayer];
    EmTraderMsg *row;

    player->tradeMsgTimer = 10;
    em->tradeStep = 0;
    em->msgId = id;
    em->msgTimer = 0;
    w->choice = 1;
    row = &gEmTraderMsgs[em->msgId];
    shown->kind = row->kind;
    shown->next = row->next;
    shown->nextNo = row->nextNo;
    shown->mode = row->mode;
    shown->text = row->text;
    EmTrader_FormatMessage((Em *) player, w);
    shown->text = gEmTraderMsgBufs + (w->msgBuf << 8);
    switch (shown->kind) {
    case TRADER_MSG_CHOICE:
    case TRADER_MSG_CONFIRM_TRADE:
    case TRADER_MSG_ASK_GIFT:
    case TRADER_MSG_DRILL_CHARM:
        w->msgWait = 30;
        break;
    default:
        w->msgWait = 0;
        break;
    }
}

/*
 * The outcome when nothing is traded: 40% picks a gift from
 * EmTrader_GiftItemsV0/V1 (weights against a roll of 0..100) into
 * Player.tradeGetItem and shows the gift message (row 20 by variant,
 * TRADER_MSG_GIFT) with the agree gesture (TRADER_IDLE_AGREE); else, or
 * when the player already has the gift or no free slot, a chat opener
 * (36, or 34 from 70%). Shows the message with an inlined copy of
 * ShowMessage; does not add the item itself.
 */
void EmTrader_ShowGiftOrChat(Em *em)
{
    s16 roll;
    u16 *gift;
    EmTraderWork *w;
    Player *player;
    EmTraderMsg *row;
    EmTraderMsg *shown;
    s16 weightSum;
    s16 pick;

    w = (EmTraderWork *)&em->work;
    player = &gPlayers[gScene.localPlayer];
    roll = Rand_Next(1) % 100;
    if (roll <= 39) {
        pick = Rand_Next(1) % 101;
        weightSum = 0;
        if (em->variant == 0) {
            gift = EmTrader_GiftItemsV0;
        } else {
            gift = EmTrader_GiftItemsV1;
        }
        for (; *gift != 0xFFFF; gift += 2) {
            weightSum += gift[0];
            if (weightSum >= pick) {
                if (Pouch_Count(player, gift[1]) > 0 || Pouch_HasFreeSlot(player) != 1) {
                    goto other;
                }
                player->tradeGetItem = gift[1];
                break;
            }
        }
        em->msgId = (u8) EmTrader_MsgIdForVariant(em, 20);
        Em_SetStateRand(em, 0, 9);
    } else {
    other:
        if (roll <= 69) {
            em->msgId = 36;
        } else {
            em->msgId = 34;
        }
    }
    shown = &w->msg;
    player->tradeMsgTimer = 10;
    em->tradeStep = 0;
    em->msgTimer = 0;
    w->choice = 1;
    row = &gEmTraderMsgs[em->msgId];
    shown->kind = row->kind;
    shown->next = row->next;
    shown->nextNo = row->nextNo;
    shown->mode = row->mode;
    shown->text = row->text;
    EmTrader_FormatMessage((Em *) player, w);
    shown->text = gEmTraderMsgBufs + (w->msgBuf << 8);
    switch (row->kind) {
    case TRADER_MSG_CHOICE:
    case TRADER_MSG_CONFIRM_TRADE:
    case TRADER_MSG_ASK_GIFT:
    case TRADER_MSG_DRILL_CHARM:
        w->msgWait = 30;
        break;
    default:
        w->msgWait = 0;
        break;
    }
}

/*
 * Runs the open talk each frame: Notice_OpenTalk shows w->msg; once the text
 * is out it moves the cursor (the kinds TRADER_MSG_CHOICE, _CONFIRM_TRADE,
 * _ASK_GIFT and _DRILL_CHARM) and, on an answer (unless w->noAnswer or
 * w->msgWait), acts on w->msg.kind: closes the talk, follows next/nextNo,
 * works out a trade offer (kind 3), swaps items, gives a gift, takes the
 * drill charm or redeems tokens. Picks the map's trade table
 * (EmTrader_Trades<Map>V0/V1) first. Always returns 0.
 */
s32 EmTrader_RunTalk(Em *em)
{
    u16 *trades;
    Player *player;
    EmTraderWork *w;
    EmTraderMsg *shown;
    u8 cursor;
    u8 answer;
    u8 result;
    u16 carryItem;
    s16 slot;
    ItemStack *unusedSlot;
    EmTradeRate *rate;
    s16 roll;
    s16 weightSum;
    u16 rateWeight;

    w = (EmTraderWork *)&em->work;
    player = &gPlayers[gScene.localPlayer];
    player->tradeMsgTimer = 5;
    shown = &w->msg;
    if (*(u16 *) &em->msgTimer <= 999) {
        (*(u16 *) &em->msgTimer)++;
    } else {
        *(u16 *) &em->msgTimer = 1000;
    }
    switch ((u16) em->tradeStep) {
    case 0:
    if (Notice_OpenTalk(shown->text, *(u16 *) &em->msgTimer, shown->mode, w->choice) == 0) {
        if (Quest_PhaseIsOver(1) == 1) {
            goto end;
        }
        if (w->msgWait != 0) {
            w->msgWait--;
        }
        cursor = w->choice;
        answer = 0;
        if (cursor == 0 && (player->input.pressed & 0x20)) {
            answer = 1;
        } else if (cursor == 1 && (player->input.pressed & 0x60)) {
            answer = 2;
        }
        if (answer != 0 && w->noAnswer == 0 && w->msgWait == 0) {
            switch ((u16) Map_DayId(gScene.map)) {
            case MAP_FOREST_HILLS:
            default:
                if (em->variant == 0) {
                    trades = EmTrader_TradesForestHillsV0;
                } else {
                    trades = EmTrader_TradesForestHillsV1;
                }
                break;
            case MAP_SWAMP:
                if (em->variant == 0) {
                    trades = EmTrader_TradesSwampV0;
                } else {
                    trades = EmTrader_TradesSwampV1;
                }
                break;
            case MAP_JUNGLE:
                if (em->variant == 0) {
                    trades = EmTrader_TradesJungleV0;
                } else {
                    trades = EmTrader_TradesJungleV1;
                }
                break;
            case MAP_DESERT:
                if (em->variant == 0) {
                    trades = EmTrader_TradesDesertV0;
                } else {
                    trades = EmTrader_TradesDesertV1;
                }
                break;
            case MAP_SNOWY_MOUNTAINS:
                if (em->variant == 0) {
                    trades = EmTrader_TradesSnowyMountainsV0;
                } else {
                    trades = EmTrader_TradesSnowyMountainsV1;
                }
                break;
            }
            switch (shown->kind) {
            case TRADER_MSG_CLOSE:
                player->tradeMsgTimer = 0;
                break;
            case TRADER_MSG_OFFER:
                result = TRADE_NONE;
                if ((u16) Pouch_FindCarryItem(player) != 0xFFFF) {
                    result = TRADE_CARRYING;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 736) >= 99 && Pouch_Space(player, 157) > 0) {
                    player->tradeGiveItem = 736;
                    player->tradeGiveCount = -99;
                    player->tradeGetItem = 157;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 550) > 0 && Pouch_Space(player, 728) > 0) {
                    player->tradeGiveItem = 550;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 728;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 676) > 0 && Pouch_Space(player, 677) > 0) {
                    player->tradeGiveItem = 676;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 677;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 757) > 0 && Pouch_Space(player, 98) > 0) {
                    player->tradeGiveItem = 757;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 98;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 754) >= 99 && Pouch_Space(player, 160) > 0) {
                    player->tradeGiveItem = 754;
                    player->tradeGiveCount = -99;
                    player->tradeGetItem = 160;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 696) >= 5 && Game_IsOnline() == 1) {
                    player->tradeGiveItem = 696;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 92;
                    result = TRADE_TOKENS;
                }
                if (result == TRADE_NONE && gQuest.traderWantItem != 0) {
                    if (Pouch_Count(player, gQuest.traderWantItem) > 0) {
                        player->tradeGiveItem = gQuest.traderWantItem;
                        for (; *trades != 0xFFFF; trades += 3) {
                            if (*trades == gQuest.traderWantItem) {
                                player->tradeGiveItem = *trades;
                                player->tradeGiveCount = -1;
                                player->tradeGetItem = trades[1];
                                if (Pouch_Space(player, player->tradeGetItem) > 0) {
                                    result = TRADE_WANT_READY;
                                } else {
                                    result = TRADE_WANT_NO_ROOM;
                                    if (player->tradeCount > 0) {
                                        player->tradeCount--;
                                        player->tradeCooldown = 0;
                                    }
                                }
                                goto chosen;
                            }
                        }
                        player->tradeGiveItem = gQuest.traderWantItem;
                        result = TRADE_WANT_MISSING;
                        if (player->tradeCount > 0) {
                            player->tradeCount--;
                            player->tradeCooldown = 0;
                        }
                    } else {
                        player->tradeGiveItem = gQuest.traderWantItem;
                        result = TRADE_WANT_MISSING;
                        if (player->tradeCount > 0) {
                            player->tradeCount--;
                            player->tradeCooldown = 0;
                        }
                    }
                }
            chosen:
                if (result == 0) {
                    if (Rand_Next(1) % 100 >= 10 && gQuest.traderWantItem == 0) {
                        for (; *trades != 0xFFFF; trades += 3) {
                            if (Pouch_Count(player, *trades) > 0) {
                                player->tradeGiveItem = *trades;
                                player->tradeGiveCount = -1;
                                if (Rand_Next(1) % 100 >= 20) {
                                    player->tradeGetItem = trades[1];
                                } else {
                                    player->tradeGetItem = trades[2];
                                }
                                if (Pouch_Space(player, player->tradeGetItem) > 0) {
                                    result = TRADE_OFFER;
                                    break;
                                }
                            }
                        }
                    } else {
                        trades += (Rand_Next(1) % 8 + 7) * 3;
                        gQuest.traderWantItem = *trades;
                        player->tradeGiveItem = *trades;
                        player->tradeGiveCount = -1;
                        if (Rand_Next(1) % 100 >= 20) {
                            player->tradeGetItem = trades[1];
                        } else {
                            player->tradeGetItem = trades[2];
                        }
                        result = TRADE_WANT_NEW;
                        if (player->tradeCount > 2) {
                            player->tradeCount = 2;
                            player->tradeCooldown = 0;
                        }
                    }
                }
                switch (result) {
                case TRADE_NONE:
                    EmTrader_ShowGiftOrChat(em);
                    break;
                case TRADE_UNUSED_1:
                case TRADE_OFFER:
                case TRADE_UNUSED_3:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 14));
                    break;
                case TRADE_CARRYING:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 24));
                    break;
                case TRADE_WANT_NEW:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 2));
                    break;
                case TRADE_WANT_MISSING:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 6));
                    break;
                case TRADE_WANT_READY:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 8));
                    break;
                case TRADE_TOKENS:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 100));
                    break;
                case TRADE_WANT_NO_ROOM:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 104));
                    break;
                }
                break;
            case TRADER_MSG_SWAP_CARRIED:
                carryItem = Pouch_FindCarryItem(player);
                if (carryItem == 0xFFFF) { /* compiled-out check */
                } else {
                    while (*trades != 0xFFFF) {
                        if (*trades == carryItem) {
                            if (Rand_Next(1) % 100 >= 20) {
                                player->tradeGetItem = trades[1];
                            } else {
                                player->tradeGetItem = trades[2];
                            }
                            break;
                        }
                        trades += 3;
                    }
                    for (slot = 0; slot < 20; slot++) {
                        if (player->pouch[slot].item == carryItem) {
                            Pouch_ClearSlot(player, slot);
                        }
                    }
                    Pouch_Add(player, player->tradeGetItem, 1);
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, (s16) player->tradeGetItem);
                }
                player->tradeMsgTimer = 0;
                break;
            case TRADER_MSG_GIFT:
                player->tradeMsgTimer = 0;
                Pouch_Add(player, player->tradeGetItem, 1);
                Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, (s16) player->tradeGetItem);
                break;
            case TRADER_MSG_REDEEM_TOKENS:
                player->tradeMsgTimer = 0;
                Pouch_Add(player, 696, -5);
                rate = EmTrader_TradeRates;
                roll = Rand_Next(1) % 32;
                player->tradeGetItem = 94;
                weightSum = 0;
                if ((rateWeight = EmTrader_TradeRates[0].weight) != 0xFFFF) {
                    do {
                        weightSum += rateWeight;
                        if (roll < weightSum) {
                            player->tradeGetItem = rate->item;
                            break;
                        }
                        rate++;
                    } while ((rateWeight = rate->weight) != 0xFFFF);
                }
                if (Pouch_Add(player, player->tradeGetItem, 1) == 3) {
                    Notice_Push(1, 3, (s16) player->tradeGetItem);
                } else {
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, (s16) player->tradeGetItem);
                }
                break;
            case TRADER_MSG_CONFIRM_TRADE:
                if (w->choice == 0) {
                    Em_SetStateRand(em, 0, 9);
                    Sound_PlaySeFlat(SE_SLOT_EM, 33, gSeBankByType[EM_KIND_TRADER]);
                    switch (em->msgId) {
                    case 15:
                    case 14:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 16));
                        break;
                    case 8:
                    case 9:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 12));
                        break;
                    }
                    Pouch_Add(player, player->tradeGiveItem, player->tradeGiveCount);
                    Pouch_Add(player, player->tradeGetItem, 1);
                    if (gQuest.traderWantItem == player->tradeGiveItem) {
                        gQuest.traderWantItem = 0;
                    }
                    if (player->tradeGiveCount == -1) {
                        Notice_Push(1, 16, (s16) player->tradeGiveItem);
                    } else {
                        Notice_Push(1, 17, (s16) player->tradeGiveItem);
                    }
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, (s16) player->tradeGetItem);
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 32, gSeBankByType[EM_KIND_TRADER]);
                    switch (em->msgId) {
                    case 15:
                    case 14:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 18));
                        break;
                    case 8:
                    case 9:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 10));
                        break;
                    }
                }
                break;
            case TRADER_MSG_ASK_GIFT:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(SE_SLOT_EM, 33, gSeBankByType[EM_KIND_TRADER]);
                    EmTrader_ShowGiftOrChat(em);
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 32, gSeBankByType[EM_KIND_TRADER]);
                    player->tradeMsgTimer = 0;
                }
                break;
            case TRADER_MSG_DRILL_CHARM:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(SE_SLOT_EM, 33, gSeBankByType[EM_KIND_TRADER]);
                    EmTrader_ShowMessage(em, 96);
                    Quest_DrillTakeCharm();
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 32, gSeBankByType[EM_KIND_TRADER]);
                    EmTrader_ShowMessage(em, 95);
                }
                break;
            case TRADER_MSG_CHOICE:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(SE_SLOT_EM, 33, gSeBankByType[EM_KIND_TRADER]);
                    if (shown->next != 255) {
                        Em_SetStateRand(em, 0, 9);
                        EmTrader_ShowMessage(em, shown->next);
                    } else {
                        player->tradeMsgTimer = 0;
                    }
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 32, gSeBankByType[EM_KIND_TRADER]);
                    if (shown->nextNo != 255) {
                        EmTrader_ShowMessage(em, shown->nextNo);
                    } else {
                        player->tradeMsgTimer = 0;
                    }
                }
                break;
            case TRADER_MSG_NEXT:
                if (shown->next != 255) {
                    EmTrader_ShowMessage(em, shown->next);
                } else {
                    player->tradeMsgTimer = 0;
                }
                break;
            case TRADER_MSG_CHAT_A:
                EmTrader_ShowMessage(em, Rand_Next(1) % 29 + 38);
                break;
            case TRADER_MSG_CHAT_B:
                EmTrader_ShowMessage(em, Rand_Next(1) % 22 + 67);
                break;
            }
        } else {
            switch (shown->kind) {
            case TRADER_MSG_CHOICE:
            case TRADER_MSG_CONFIRM_TRADE:
            case TRADER_MSG_ASK_GIFT:
            case TRADER_MSG_DRILL_CHARM:
                if ((player->input.pressed & 0x800) && cursor != 0) {
                    w->choice = 0;
                    Sound_PlaySeFlat(0, 22, 0);
                } else if ((player->input.pressed & 0x400) && cursor == 0) {
                    w->choice = 1;
                    Sound_PlaySeFlat(0, 22, 0);
                } else if ((player->input.pressed & 0x40) && cursor == 0) {
                    w->choice = 1;
                    Sound_PlaySeFlat(SE_SLOT_EM, 32, gSeBankByType[EM_KIND_TRADER]);
                }
                break;
            }
        }
    } else if ((player->input.pressed & 0x60) && *(u16 *) &em->msgTimer > 10) {
        *(u16 *) &em->msgTimer = 1000;
    }
        break;
    }
end:
    w->noAnswer = 0;
    return 0;
}

/*
 * Sets the trader up: no hit or push, hp 200, scale 1, blind, home position
 * and yaw (work and startPos), idle pose 1 (em->yawCenter) with motion 1 in
 * TRADER_IDLE_STAND; clears greeted, noAnswer, unk634 and msgItem. The jump
 * to TRADER_IDLE_HIDDEN tests Rand & 3 against 255 and never runs.
 */
void EmTrader_Init(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    u16 roll;

    em->noHitTimer = 99;
    em->noPushTimer = 99;
    em->yawCenter = 0;
    em->unk634 = 0;
    em->msgItem = 0;
    roll = Rand_Next(1) & 3;
    Em_SetStateRand(em, 0, 0);
    em->yawCenter = 1;
    switch (em->yawCenter) {
    case 0:
        Em_SetMotion(em, TRADER_MOT_HIDDEN, 0, 0);
        break;
    case 1:
    case 2:
        Em_SetMotion(em, TRADER_MOT_STAND, 0, 0);
        break;
    }
    em->startPos[0] = em->posX;
    em->startPos[1] = em->posY;
    em->startPos[2] = em->posZ;
    em->targetYaw = em->rotY;
    em->stance = EM_STANCE_GROUND;
    em->timer = 0;
    if (roll == 255) {
        Em_SetStateRand(em, 0, 1);
        Em_SetMotion(em, TRADER_MOT_HIDDEN, 0, 0);
    }
    em->hp = 200;
    em->hpMax = 200;
    em->scaleX = 1.0f;
    em->scaleY = 1.0f;
    em->scaleZ = 1.0f;
    em->scriptPending = 0;
    em->blind = 1;
    w->home[0] = em->posX;
    w->home[1] = em->posY;
    w->home[2] = em->posZ;
    w->homeYaw = em->rotY;
    w->greeted = 0;
    w->noAnswer = 0;
    Em_InitParts(em);
}

/*
 * Returns to the idle of the trader's pose (em->yawCenter): 0
 * TRADER_IDLE_STAND (TRADER_MOT_STAND), 1 TRADER_IDLE_WANDER
 * (TRADER_MOT_WALK), 2 TRADER_IDLE_REST (TRADER_MOT_REST), with the given
 * blend and start frame unless that motion already plays; only mode 0 does
 * anything; turns field collision back on.
 */
void EmTrader_ToIdle(Em *em, u8 mode, u16 blend, u16 startFrame)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;

    switch (mode) {
    case 0:
        w->noAnswer = 0;
        switch (em->yawCenter) {
        case 0:
            Em_SetStateRand(em, 0, 0);
            if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_STAND)) {
                Em_SetMotion(em, TRADER_MOT_STAND, blend, startFrame);
            }
            break;
        case 1:
            Em_SetStateRand(em, 0, 10);
            if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_WALK)) {
                Em_SetMotion(em, TRADER_MOT_WALK, blend, startFrame);
            }
            break;
        case 2:
            Em_SetStateRand(em, 0, 6);
            if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_REST)) {
                Em_SetMotion(em, TRADER_MOT_REST, blend, startFrame);
            }
            break;
        }
        break;
    }
    em->fieldCollide = 1;
}

/*
 * Every frame (from Update) steps rotY toward targetYaw, 1820 per frame
 * while the trader is in TRADER_IDLE_GREET (Player_IsState on the Em),
 * 4004 otherwise, snapping when within a step.
 */
void EmTrader_TurnToYaw(Em *em)
{
    s32 step;
    u32 diff;

    if ((s16) Player_IsState(em, 0, 3)) {
        step = 1820;
    } else {
        step = 4004;
    }
    diff = (em->targetYaw - (em->rotY & 0xFFFF)) & 0xFFFF;
    if ((u32) ((diff + step) & 0xFFFF) < step * 2) {
        em->rotY = em->targetYaw;
    } else if (diff < 0x8000U) {
        em->rotY = (em->rotY + step) & 0xFFFF;
    } else {
        em->rotY = (em->rotY - step) & 0xFFFF;
    }
}

/*
 * TRADER_IDLE_STAND: stands (motion 1, or motion 5 in pose 2) for a random
 * 0..0xBB frames; greets the player (TRADER_IDLE_GREET) when they are near
 * and have opened a talk (Player.tradeMsgTimer); when the time runs out in
 * pose 1 goes to TRADER_IDLE_WANDER (3 in 4) or TRADER_IDLE_FIDGET; pose 0
 * just keeps standing.
 */
void EmTrader_StandIdle(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    Player *player = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = Rand_Next(1) & 0xBB;
        if (em->yawCenter == 2) {
            if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_REST)) {
                Em_SetMotion(em, TRADER_MOT_REST, 2, 0);
            }
        } else {
            if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_STAND)) {
                Em_SetMotion(em, TRADER_MOT_STAND, 2, 0);
            }
        }
        w->noAnswer = 0;
        break;
    case 1:
        if (EmTrader_IsPlayerNear(em) == 1) {
            if (player->tradeMsgTimer != 0) {
                Em_SetStateRand(em, 0, 3);
            }
        } else {
            em->timer--;
            if (em->timer <= 0) {
                switch (em->yawCenter) {
                case 0:
                    break;
                case 1:
                    if (Rand_Next(1) & 3) {
                        Em_SetStateRand(em, 0, 10);
                    } else {
                        Em_SetStateRand(em, 0, 11);
                    }
                    break;
                }
            }
        }
        break;
    }
}

/*
 * TRADER_IDLE_HIDDEN: puts the trader back at its home position and yaw
 * with no field collision in the hidden motion 2, and waits for the player
 * to come near, then TRADER_IDLE_JUMP_OUT. Only Init's dead branch leads
 * here.
 */
void EmTrader_WaitHidden(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->fieldCollide = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_HIDDEN)) {
            em->posX = w->home[0];
            em->posY = w->home[1];
            em->posZ = w->home[2];
            em->rotY = (s32) w->homeYaw;
            em->targetYaw = w->homeYaw;
            Em_SetMotion(em, TRADER_MOT_HIDDEN, 2, 0);
        }
        w->noAnswer = 0;
        break;
    case 1:
        if (EmTrader_IsPlayerNear(em) == 1) {
            em->fieldCollide = 1;
            Em_SetStateRand(em, 0, 2);
        }
        break;
    }
}

/*
 * TRADER_IDLE_JUMP_OUT: motion 3; at frame 14 leaps forward (4 along its
 * facing, 6 up, airborne stance 2, Player_StartJump), falls to the floor,
 * lands with motion 4 and then stands (TRADER_IDLE_STAND).
 */
void EmTrader_JumpOut(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    f32 offset[3];
    f32 worldOffset[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, TRADER_MOT_JUMP_OUT, 0, 0);
        em->stance = EM_STANCE_AIR;
        w->noAnswer = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            em->step++;
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 4.0f;
            Vec_Rotate(worldOffset, offset, (f32 *) em->worldMtx);
            em->velX = worldOffset[0];
            em->velY = 6.0f;
            em->velZ = worldOffset[2];
            Player_ClearAccel((Player *) em);
            Player_StartJump(em, 12);
        }
        break;
    case 2:
        Player_MoveAccel((Player *) em);
        if (em->velY <= 0.0f && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, TRADER_MOT_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, TRADER_MOT_STAND, 6, 0);
            Em_SetStateRand(em, 0, 0);
        }
        break;
    }
}

/*
 * TRADER_IDLE_GREET: turns to the player (targetYaw = heading -
 * ANGLE_TURN / 4) with TRADER_MOT_GREET and the greeting sound, sets greeted,
 * and opens the talk: in a drill the charm talk (89) or a drill line (97..99)
 * unless carrying an item; with Player.tradeCount at 7 and nothing carried a
 * refusal (28/30/32 by stateRand % 3, going to TRADER_IDLE_REFUSE_A/B/C);
 * else the offer opener (row 0, TRADER_MSG_OFFER). Then runs the talk: idles
 * when it closes, goes to TRADER_IDLE_TALK when the motion ends.
 */
void EmTrader_GreetPlayer(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    Player *player = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, TRADER_MOT_GREET, 8, 0);
        w->greeted = 1;
        w->noAnswer = 0;
        em->targetYaw = Vec_Heading(&em->posX, player->pos) - ANGLE_TURN / 4;
        Sound_PlaySeFlat(SE_SLOT_EM, 30, gSeBankByType[EM_KIND_TRADER]);
        if (Quest_IsDrill() == 1) {
            if ((u16) Pouch_FindCarryItem(player) == 0xFFFF) {
                if (gQuest.drillCharm == 0) {
                    EmTrader_ShowMessage(em, 89);
                } else {
                    EmTrader_ShowMessage(em, Rand_Next(1) % 3 + 97);
                }
            } else {
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0));
            }
        } else if (player->tradeCount >= 7 && (u16) Pouch_FindCarryItem(player) == 0xFFFF) {
            switch (em->stateRand % 3) {
            case 0:
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 28));
                Em_SetStateRand(em, 0, 5);
                break;
            case 1:
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 30));
                Em_SetStateRand(em, 0, 7);
                break;
            case 2:
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 32));
                Em_SetStateRand(em, 0, 8);
                break;
            }
        } else {
            EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0));
        }
        break;
    case 1:
        w->noAnswer = 0;
        EmTrader_RunTalk(em);
        if (player->tradeMsgTimer == 0) {
            EmTrader_ToIdle(em, 0, 4, 0);
        } else if (em->motion[0].playing == 0) {
            Em_SetStateRand(em, 0, 4);
        }
        break;
    }
}

/*
 * TRADER_IDLE_TALK: the talking loop with motion 8; runs the talk (twice on
 * the first frame, case 0 falls into 1) until Player.tradeMsgTimer is 0,
 * then idles. Clears work + 0x1A (noAnswer) by a raw offset every frame.
 */
void EmTrader_Talk(Em *em)
{
    Player *player;
    u8 answer;

    player = &gPlayers[gScene.localPlayer];
    ((EmTraderWork *)&em->work)->noAnswer = 0;
    switch (em->step) {
    case 0:
        em->step += 1;
        em->stateVar0 = 0;
        em->tradeStep = 0;
        Em_SetMotion(em, TRADER_MOT_TALK, 4, 0);
        EmTrader_RunTalk(em);
        
    case 1:
        EmTrader_RunTalk(em);
        if (player->tradeMsgTimer == 0) {
            EmTrader_ToIdle(em, 0, 4, 0U);
        }
        break;
    }
}

/*
 * TRADER_IDLE_REFUSE_A/B/C (variant 0, 1, 2: TRADER_MOT_REFUSE_A, _B, _C):
 * plays the refusal gesture while the talk runs without answers (noAnswer),
 * then stands in TRADER_MOT_STAND taking answers until the talk closes, then
 * idles.
 */
void EmTrader_RefuseTrade(Em *em, s32 variant)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    Player *player = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        switch (variant) {
        case 0:
            Em_SetMotion(em, TRADER_MOT_REFUSE_A, 6, 0);
            break;
        case 1:
            Em_SetMotion(em, TRADER_MOT_REFUSE_B, 4, 0);
            break;
        case 2:
            Em_SetMotion(em, TRADER_MOT_REFUSE_C, 4, 0);
            break;
        }
        w->noAnswer = 1;
        EmTrader_RunTalk(em);
        break;
    case 1:
        w->noAnswer = 1;
        EmTrader_RunTalk(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, TRADER_MOT_STAND, 2, 0);
        }
        break;
    case 2:
        w->noAnswer = 0;
        EmTrader_RunTalk(em);
        if (player->tradeMsgTimer == 0) {
            EmTrader_ToIdle(em, 0, 2, 0);
        }
        break;
    }
}

/*
 * TRADER_IDLE_REST: the idle of pose 2: motion 5 for 0..0xBF frames, then
 * at random motion 1 for 0..0xBF frames or the one-shot motion 6, and
 * again; greets the player when they are near and talking.
 */
void EmTrader_RestIdle(Em *em)
{
    Player *player;
    u8 step;

    step = em->step;
    player = &gPlayers[gScene.localPlayer];
    switch (step) {
    case 0:
        em->step += 1;
        Em_SetMotion(em, TRADER_MOT_REST, 4, 0);
        em->timer = Rand_Next(1) & 0xFFFF & 0xBF;
        break;
    case 1:
        if (--em->timer <= 0) {
            if (Rand_Next(1) & 0xFFFF & 1) {
                em->step = 2;
                em->timer = Rand_Next(1) & 0xFFFF & 0xBF;
                Em_SetMotion(em, TRADER_MOT_STAND, 4, 0);
            } else {
                em->step = 3;
                Em_SetMotion(em, TRADER_MOT_REST_FIDGET, 4, 0);
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step = 0;
        }
        break;
    }
    if ((EmTrader_IsPlayerNear(em) == 1) && (player->tradeMsgTimer != 0)) {
        Em_SetStateRand(em, 0, 3);
    }
}

/*
 * TRADER_IDLE_AGREE: plays TRADER_MOT_AGREE while the talk runs without
 * answers, then stands in TRADER_MOT_STAND taking answers until the talk
 * closes, then idles. Entered on a yes (an accepted trade, a choice with a
 * next row) and on a gift.
 */
void EmTrader_AgreeGesture(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    Player *player = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, TRADER_MOT_AGREE, 6, 0);
        w->noAnswer = 1;
        EmTrader_RunTalk(em);
        break;
    case 1:
        w->noAnswer = 1;
        EmTrader_RunTalk(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, TRADER_MOT_STAND, 2, 0);
        }
        break;
    case 2:
        w->noAnswer = 0;
        EmTrader_RunTalk(em);
        if (player->tradeMsgTimer == 0) {
            EmTrader_ToIdle(em, 0, 2, 0);
        }
        break;
    }
}

/*
 * TRADER_IDLE_WANDER: the idle of pose 1: walks (TRADER_MOT_WALK) for 100..163
 * frames while turning 0x100 per frame toward its home position (work + 8),
 * then TRADER_IDLE_FIDGET; greets the player when they are near and
 * talking.
 */
void EmTrader_WanderNearHome(Em *em)
{
    Player *player;
    u8 step;
    u8 *traderWork;
    step = em->step;
    player = &gPlayers[gScene.localPlayer];
    traderWork = &em->work;
    switch (step) {
    case 0:
        em->step += 1;
        em->timer = (Rand_Next(1) & 0xFFFF & 0x3F) + 100;
        if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_WALK)) {
            Em_SetMotion(em, TRADER_MOT_WALK, 6, 0);
        }
        break;
    case 1:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            Em_SetStateRand(em, 0, 11);
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, (f32 *) (traderWork + 8)) & 0xFFFF;
            Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100);
            em->targetYaw = (u16) em->rotY;
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        }
        if ((EmTrader_IsPlayerNear(em) == 1) && (player->tradeMsgTimer != 0)) {
            Em_SetStateRand(em, 0, 3);
        }
        break;
    }
}

/*
 * TRADER_IDLE_FIDGET: plays the one-shot TRADER_MOT_FIDGET, then idles (back
 * to the wander in pose 1); greets the player when they are near and talking.
 */
void EmTrader_Fidget(Em *em)
{
    Player *player;
    u8 step;

    step = em->step;
    player = &gPlayers[gScene.localPlayer];
    switch (step) {
    case 0:
        em->step += 1;
        em->timer = 0;
        Em_SetMotion(em, TRADER_MOT_FIDGET, 4, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmTrader_ToIdle(em, 0, 4, 0U);
        }
        if ((EmTrader_IsPlayerNear(em) == 1) && (player->tradeMsgTimer != 0)) {
            Em_SetStateRand(em, 0, 3);
        }
        break;
    }
}

/*
 * The group 0 dispatcher: saves prevStateGroup/prevState and runs the
 * state's function (5, 7 and 8 share EmTrader_RefuseTrade with variant 0,
 * 1, 2).
 */
void EmTrader_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case TRADER_IDLE_STAND:
        EmTrader_StandIdle(em);
        break;
    case TRADER_IDLE_HIDDEN:
        EmTrader_WaitHidden(em);
        break;
    case TRADER_IDLE_JUMP_OUT:
        EmTrader_JumpOut(em);
        break;
    case TRADER_IDLE_GREET:
        EmTrader_GreetPlayer(em);
        break;
    case TRADER_IDLE_TALK:
        EmTrader_Talk(em);
        break;
    case TRADER_IDLE_REFUSE_A:
        EmTrader_RefuseTrade(em, 0);
        break;
    case TRADER_IDLE_REST:
        EmTrader_RestIdle(em);
        break;
    case TRADER_IDLE_REFUSE_B:
        EmTrader_RefuseTrade(em, 1);
        break;
    case TRADER_IDLE_REFUSE_C:
        EmTrader_RefuseTrade(em, 2);
        break;
    case TRADER_IDLE_AGREE:
        EmTrader_AgreeGesture(em);
        break;
    case TRADER_IDLE_WANDER:
        EmTrader_WanderNearHome(em);
        break;
    case TRADER_IDLE_FIDGET:
        EmTrader_Fidget(em);
        
    default:
        break;
    }
}

/*
 * Every frame: keeps hits and pushes off, runs the state (again on a
 * stateRequest) and turns to targetYaw.
 */
void EmTrader_Update(Em *em)
{
    em->noHitTimer = 99;
    em->noPushTimer = 99;
    EmTrader_RunState(em);
    if (em->stateRequest != 0) {
        EmTrader_RunState(em);
        em->stateRequest = 0;
    }
    EmTrader_TurnToYaw(em);
}

/*
 * Sends every group 0..7 to EmTrader_RunGroup0 (the trader has only group
 * 0's states).
 */
void EmTrader_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmTrader_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmTrader_RunIdleState(em);
        break;
    case EM_GROUP_FLY:
        EmTrader_RunIdleState(em);
        break;
    case EM_GROUP_ATTACK:
        EmTrader_RunIdleState(em);
        break;
    case EM_GROUP_REACT:
        EmTrader_RunIdleState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmTrader_RunIdleState(em);
        break;
    case EM_GROUP_EVENT:
        EmTrader_RunIdleState(em);
        break;
    case EM_GROUP_EVENT2:
        EmTrader_RunIdleState(em);
        
    default:
        break;
    }
}

/* Records motionId[0] in work->fxMotion and plays its sounds. */
void EmTrader_MotionEffects(Em *em, EmWork *work)
{
    u16 motionId;

    motionId = em->motionId[0];
    if (motionId != work->fxMotion) {
        work->fxMotion = (s16) motionId;
    }
    EmTrader_MotionSounds(em, work);
}

/*
 * Skips the first frame (fxStep goes from 0 to 1), then runs
 * EmTrader_MotionEffects each frame.
 */
void EmTrader_UpdateMotionEffects(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmTrader_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Spawns the trader's shadow (SHADOW_MODEL); in the first two slots of
 * EmTrader_Callbacks.
 */
void EmTrader_SpawnShadow(u8 *em)
{
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/* Does nothing; fills two slots of EmTrader_Callbacks. */
void EmTrader_TableNop(void)
{
}

/* The file's initialised data, in the game's order */
u16 EmTrader_TradesJungleV0[168] = {
#include "game/008C6540.inc"
};
u16 EmTrader_TradesJungleV1[168] = {
#include "game/008C6690.inc"
};
u16 EmTrader_TradesForestHillsV0[160] = {
#include "game/008C67E0.inc"
};
u16 EmTrader_TradesForestHillsV1[160] = {
#include "game/008C6920.inc"
};
u16 EmTrader_TradesSwampV0[152] = {
#include "game/008C6A60.inc"
};
u16 EmTrader_TradesSwampV1[152] = {
#include "game/008C6B90.inc"
};
u16 EmTrader_TradesDesertV0[152] = {
#include "game/008C6CC0.inc"
};
u16 EmTrader_TradesDesertV1[152] = {
#include "game/008C6DF0.inc"
};
u16 EmTrader_TradesSnowyMountainsV0[168] = {
#include "game/008C6F20.inc"
};
u16 EmTrader_TradesSnowyMountainsV1[168] = {
#include "game/008C7070.inc"
};
u16 EmTrader_GiftItemsV0[40] = {
#include "game/008C71C0.inc"
};
u16 EmTrader_GiftItemsV1[40] = {
#include "game/008C7210.inc"
};
EmTradeRate EmTrader_TradeRates[12] = {
#include "game/008C7260.inc"
};
void *EmTrader_Callbacks[8] = {
    EmTrader_SpawnShadow, EmTrader_SpawnShadow, EmTrader_TableNop, EmTrader_UpdateMotionEffects,
    GmEm_ReturnZero, EmTrader_TableNop, NULL, NULL,
};
