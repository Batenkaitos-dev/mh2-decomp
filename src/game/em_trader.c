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
/* prototypes of callees */
void Em_PlayCommonSeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
u16 Rand_Next(s32 stream);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void EmTrader_MotionSounds(void *arg0, EmWork *work);
s32 EmTrader_IsPlayerNear(Em *);
void EmTrader_ToIdle(Em *em, u8 flag, u16 a, u16 b);
void EmTrader_TurnToYaw(Em *);
void EmTrader_StandIdle(Em *);
void EmTrader_JumpOut(Em *);
void EmTrader_GreetPlayer(Em *);
void EmTrader_Talk(Em *em);
void EmTrader_RefuseTrade(Em *, s32);
void EmTrader_AgreeGesture(Em *);
void EmTrader_WanderNearHome(Em *);
void EmTrader_RunState(Em *em);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
void Shadow_Spawn(void *obj, s16 type);
s32 EmTrader_RunTalk(Em *em);
s32 Game_IsOnline(void);
f32 Vec_Distance(f32 *a, f32 *b);
/* unit's own declaration: declared here as the callers use it; with the definition's s16 (void *obj, u8 state, u8 sub), the calls in EmTrader_TurnToYaw and EmPlesioth_WaitForBait compile differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
s32 sprintf(char *, const char *, ...);
extern char *gItemNames[];
extern char gEmTraderMsgBufs[];
s32 Obj_InCurrentArea(void *obj);
void Em_InitParts(Em *em);
void Player_MoveAccel(Player *p);
void Player_ClearAccel(Player *p);
s32 Player_StartJump(void *obj, s16 frames);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
u16 Vec_Heading(f32 *a, f32 *b);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 Quest_IsDrill(void);
u16 Pouch_FindCarryItem(Player *p);
s32 EmTrader_MsgIdForVariant(Em *em, s32 item);
void EmTrader_ShowMessage(Em *em, u16 id);
s16 Pouch_Count(Player *p, u16 item);
s16 Pouch_HasFreeSlot(Player *p);
extern u16 EmTrader_GiftItemsV0[];
extern u16 EmTrader_GiftItemsV1[];
s32 Notice_OpenTalk(char *text, u32 frame, s32 a, s32 b);
s32 Quest_PhaseIsOver(s32);
u16 Map_DayId(u16);
s16 Pouch_Space(Player *p, u16 item);
/* unit's own declaration: declared here as the callers use it; with the definition's u8 (Player *p, u16 item, s16 n), the calls in EmTrader_RunTalk compile differently */
u16 Pouch_Add(Player *pl, u16 item, s16 n);
s16 Pouch_ClearSlot(Player *p, s16 slot);
u8 Notice_Push(u8 kind, s16 a, s16 b);
s32 Quest_DrillTakeCharm(void);
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
    u8 unk001[0x008 - 0x001];
    f32 home[3];                /* 0x008: Init: the position; State0_01 puts the trader back there */
    u16 homeYaw;                /* 0x014: Init: rotY; State0_01 restores rotY and targetYaw from it */
    u8 unk016[0x018 - 0x016];
    u8 greeted;                 /* 0x018: Init: 0; EmTrader_GreetPlayer: 1 (nothing reads it) */
    u8 unk019[0x01A - 0x019];
    u8 noAnswer;                /* 0x01A: 1: Trade takes no answer (State0_05 and State0_09 set it before calling Trade while their motion plays); Trade and the states clear it */
    u8 choice;                  /* 0x01B: the talk's cursor (Notice_OpenTalk's last argument): 0 on the first answer, 1 on the second (ShowMessage's start); Trade moves it */
    s16 msgWait;                /* 0x01C: ShowMessage: 30 frames for the kinds that wait (1, 4, 5, 0xA), else 0; Trade counts it down and takes no answer before 0 */
    u8 msgBuf;                  /* 0x01E: which of the two 256-byte text buffers of gEmTraderMsgBufs holds the message (SetMessage flips it) */
    u8 unk01F[0x020 - 0x01F];
    EmTraderMsg msg;            /* 0x020: the message shown: ShowMessage copies its gEmTraderMsgs row here, then points text at the buffer SetMessage wrote */
    u8 unk028[0x100 - 0x028];
} EmTraderWork;
typedef char EmTraderWork_size_check[(sizeof(EmTraderWork) == sizeof(EmWork)) ? 1 : -1];

/*
 * Plays the sounds of the trader's motions on their frames, switched on
 * work->fxMotion (motion N shows as 0x3E8 + N: 0x3EA motion 2 .. 0x3F6
 * motion 0xE); the last case falls into default.
 */
void EmTrader_MotionSounds(void *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, TRADER_MOT_HIDDEN):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0x1A, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_JUMP_OUT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x1B, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 6, 0x43, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 6, 0x3A, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REST):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB0, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD4, 0x1A, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REST_FIDGET):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB2, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xCE, 0x1C, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_GREET):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 6, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x1A, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_TALK):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xB, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(0x22, 6, 0x23, 0), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, Sound_RandomSe(0x23, 6, 0x24, 0), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB6, Sound_RandomSe(0x24, 6, 9, 2), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEE, 0xD, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15E, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB2, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x11E, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 0x19, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REFUSE_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA4, 0x17, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0x18, 0x3D, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0x1C, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REFUSE_C):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0xD, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 0x1C, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_REFUSE_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x72, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB0, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEA, 0x16, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0x3C, 0x3D, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0x72, 0x4A, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0x88, 0x4A, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0xB0, 0x4A, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_AGREE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xC, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x1B, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0x50, 0x4A, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_WALK):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(0xE, 4, 0xF, 2), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, Sound_RandomSe(0x11, 2, 0xE, 4), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x14, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x14, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0x15, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0x1A, 0);
        break;
    case EM_MOTION_ID(0, TRADER_MOT_FIDGET):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, Sound_RandomSe(0xE, 4, 0x11, 4), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, Sound_RandomSe(0x10, 3, 0x11, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12C, Sound_RandomSe(0xE, 3, 0xF, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x174, Sound_RandomSe(0x11, 3, 0x10, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1DC, Sound_RandomSe(0xE, 3, 0xF, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x258, Sound_RandomSe(0xE, 3, 0x10, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xFE, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1BE, 0x1A, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0x7E, 0x4C, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0xB0, 0x4C, 0);
        Em_PlayCommonSeAtBoneOnFrame((Em *) em, 0xDC, 0x4C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A4, 0x19, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1B8, 0x19, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0x14, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x122, 0x15, 0);
        
    default:
        break;
    }
}

/*
 * Returns the gEmTraderMsgs row for the trader's variant: the id itself for
 * variant 0 and for ids 0x22..0x63 (the shared chat and drill rows), else
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
    if ((id >= 0x22) && (id < 0x59)) {
        return item;
    }
    if ((id >= 0x59) && (id < 0x64)) {
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
        if (src[0] == 0x7C && src[1] == 0x7C) {
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
 * Starts message id: sets em->msgId, clears tradeStep and msgTimer, puts
 * the cursor on the second answer, copies the gEmTraderMsgs row into
 * w->msg, formats its text, and gives kinds 1, 4, 5 and 0xA a 30-frame
 * answer wait; keeps the talk open (Player.tradeMsgTimer = 10).
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
 * Player.tradeGetItem and shows the gift message (row 0x14 by variant,
 * TRADER_MSG_GIFT) with the agree gesture (TRADER_IDLE_AGREE); else, or
 * when the player already has the gift or no free slot, a chat opener
 * (0x24, or 0x22 from 70%). Shows the message with an inlined copy of
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
        em->msgId = (u8) EmTrader_MsgIdForVariant(em, 0x14);
        Em_SetStateRand(em, 0, 9);
    } else {
    other:
        if (roll <= 69) {
            em->msgId = 0x24;
        } else {
            em->msgId = 0x22;
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

extern s8 gSeBankByType[];
/*
 * Runs the open talk each frame: Notice_OpenTalk shows w->msg; once the
 * text is out it moves the cursor (kinds 1, 4, 5, 0xA) and, on an answer
 * (unless w->noAnswer or w->msgWait), acts on w->msg.kind: closes the talk,
 * follows next/nextNo, works out a trade offer (kind 3), swaps items, gives
 * a gift, takes the drill charm or redeems tokens. Picks the map's trade
 * table (EmTrader_Trades<Map>V0/V1) first. Always returns 0.
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
                if (result == TRADE_NONE && Pouch_Count(player, 0x2E0) >= 99 && Pouch_Space(player, 0x9D) > 0) {
                    player->tradeGiveItem = 0x2E0;
                    player->tradeGiveCount = -99;
                    player->tradeGetItem = 0x9D;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 0x226) > 0 && Pouch_Space(player, 0x2D8) > 0) {
                    player->tradeGiveItem = 0x226;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 0x2D8;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 0x2A4) > 0 && Pouch_Space(player, 0x2A5) > 0) {
                    player->tradeGiveItem = 0x2A4;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 0x2A5;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 0x2F5) > 0 && Pouch_Space(player, 0x62) > 0) {
                    player->tradeGiveItem = 0x2F5;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 0x62;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 0x2F2) >= 99 && Pouch_Space(player, 0xA0) > 0) {
                    player->tradeGiveItem = 0x2F2;
                    player->tradeGiveCount = -99;
                    player->tradeGetItem = 0xA0;
                    result = TRADE_OFFER;
                }
                if (result == TRADE_NONE && Pouch_Count(player, 0x2B8) >= 5 && Game_IsOnline() == 1) {
                    player->tradeGiveItem = 0x2B8;
                    player->tradeGiveCount = -1;
                    player->tradeGetItem = 0x5C;
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
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0xE));
                    break;
                case TRADE_CARRYING:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x18));
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
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x64));
                    break;
                case TRADE_WANT_NO_ROOM:
                    EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x68));
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
                Pouch_Add(player, 0x2B8, -5);
                rate = EmTrader_TradeRates;
                roll = Rand_Next(1) % 32;
                player->tradeGetItem = 0x5E;
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
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x21, gSeBankByType[EM_KIND_TRADER]);
                    switch (em->msgId) {
                    case 0xF:
                    case 0xE:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x10));
                        break;
                    case 8:
                    case 9:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0xC));
                        break;
                    }
                    Pouch_Add(player, player->tradeGiveItem, player->tradeGiveCount);
                    Pouch_Add(player, player->tradeGetItem, 1);
                    if (gQuest.traderWantItem == player->tradeGiveItem) {
                        gQuest.traderWantItem = 0;
                    }
                    if (player->tradeGiveCount == -1) {
                        Notice_Push(1, 0x10, (s16) player->tradeGiveItem);
                    } else {
                        Notice_Push(1, 0x11, (s16) player->tradeGiveItem);
                    }
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, (s16) player->tradeGetItem);
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x20, gSeBankByType[EM_KIND_TRADER]);
                    switch (em->msgId) {
                    case 0xF:
                    case 0xE:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x12));
                        break;
                    case 8:
                    case 9:
                        EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0xA));
                        break;
                    }
                }
                break;
            case TRADER_MSG_ASK_GIFT:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x21, gSeBankByType[EM_KIND_TRADER]);
                    EmTrader_ShowGiftOrChat(em);
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x20, gSeBankByType[EM_KIND_TRADER]);
                    player->tradeMsgTimer = 0;
                }
                break;
            case TRADER_MSG_DRILL_CHARM:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x21, gSeBankByType[EM_KIND_TRADER]);
                    EmTrader_ShowMessage(em, 0x60);
                    Quest_DrillTakeCharm();
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x20, gSeBankByType[EM_KIND_TRADER]);
                    EmTrader_ShowMessage(em, 0x5F);
                }
                break;
            case TRADER_MSG_CHOICE:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x21, gSeBankByType[EM_KIND_TRADER]);
                    if (shown->next != 0xFF) {
                        Em_SetStateRand(em, 0, 9);
                        EmTrader_ShowMessage(em, shown->next);
                    } else {
                        player->tradeMsgTimer = 0;
                    }
                } else {
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x20, gSeBankByType[EM_KIND_TRADER]);
                    if (shown->nextNo != 0xFF) {
                        EmTrader_ShowMessage(em, shown->nextNo);
                    } else {
                        player->tradeMsgTimer = 0;
                    }
                }
                break;
            case TRADER_MSG_NEXT:
                if (shown->next != 0xFF) {
                    EmTrader_ShowMessage(em, shown->next);
                } else {
                    player->tradeMsgTimer = 0;
                }
                break;
            case TRADER_MSG_CHAT_A:
                EmTrader_ShowMessage(em, Rand_Next(1) % 29 + 0x26);
                break;
            case TRADER_MSG_CHAT_B:
                EmTrader_ShowMessage(em, Rand_Next(1) % 22 + 0x43);
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
                    Sound_PlaySeFlat(0, 0x16, 0);
                } else if ((player->input.pressed & 0x400) && cursor == 0) {
                    w->choice = 1;
                    Sound_PlaySeFlat(0, 0x16, 0);
                } else if ((player->input.pressed & 0x40) && cursor == 0) {
                    w->choice = 1;
                    Sound_PlaySeFlat(SE_SLOT_EM, 0x20, gSeBankByType[EM_KIND_TRADER]);
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
 * to TRADER_IDLE_HIDDEN tests Rand & 3 against 0xFF and never runs.
 */
void EmTrader_Init(Em *em)
{
    EmTraderWork *w = (EmTraderWork *)&em->work;
    u16 roll;

    em->noHitTimer = 0x63;
    em->noPushTimer = 0x63;
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
    if (roll == 0xFF) {
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
 * TRADER_IDLE_STAND (motion 1), 1 TRADER_IDLE_WANDER (motion 0xD), 2
 * TRADER_IDLE_REST (motion 5), with the given blend and start frame unless
 * that motion already plays; only mode 0 does anything; turns field
 * collision back on.
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
            Em_SetStateRand(em, 0, 0xA);
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
 * Every frame (from Update) steps rotY toward targetYaw, 0x71C per frame
 * while the trader is in TRADER_IDLE_GREET (Player_IsState on the Em),
 * 0xFA4 otherwise, snapping when within a step.
 */
void EmTrader_TurnToYaw(Em *em)
{
    s32 step;
    u32 diff;

    if ((s16) Player_IsState(em, 0, 3)) {
        step = 0x71C;
    } else {
        step = 0xFA4;
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
                        Em_SetStateRand(em, 0, 0xA);
                    } else {
                        Em_SetStateRand(em, 0, 0xB);
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
            Player_StartJump(em, 0xC);
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
 * TRADER_IDLE_GREET: turns to the player (targetYaw = heading - 0x4000)
 * with motion 7 and the greeting sound, sets greeted, and opens the talk:
 * in a drill the charm talk (0x59) or a drill line (0x61..0x63) unless
 * carrying an item; with Player.tradeCount at 7 and nothing carried a
 * refusal (0x1C/0x1E/0x20 by stateRand % 3, going to
 * TRADER_IDLE_REFUSE_A/B/C); else the offer opener (row 0,
 * TRADER_MSG_OFFER). Then runs the talk: idles when it closes, goes to
 * TRADER_IDLE_TALK when the motion ends.
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
        em->targetYaw = Vec_Heading(&em->posX, player->pos) - 0x4000;
        Sound_PlaySeFlat(SE_SLOT_EM, 0x1E, gSeBankByType[EM_KIND_TRADER]);
        if (Quest_IsDrill() == 1) {
            if ((u16) Pouch_FindCarryItem(player) == 0xFFFF) {
                if (gQuest.drillCharm == 0) {
                    EmTrader_ShowMessage(em, 0x59);
                } else {
                    EmTrader_ShowMessage(em, Rand_Next(1) % 3 + 0x61);
                }
            } else {
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0));
            }
        } else if (player->tradeCount >= 7 && (u16) Pouch_FindCarryItem(player) == 0xFFFF) {
            switch (em->stateRand % 3) {
            case 0:
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x1C));
                Em_SetStateRand(em, 0, 5);
                break;
            case 1:
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x1E));
                Em_SetStateRand(em, 0, 7);
                break;
            case 2:
                EmTrader_ShowMessage(em, (u8) EmTrader_MsgIdForVariant(em, 0x20));
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
 * TRADER_IDLE_REFUSE_A/B/C (variant 0, 1, 2: motion 0xB, 9, 0xA): plays the
 * refusal gesture while the talk runs without answers (noAnswer), then
 * stands in motion 1 taking answers until the talk closes, then idles.
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
 * TRADER_IDLE_AGREE: plays motion 0xC while the talk runs without answers,
 * then stands in motion 1 taking answers until the talk closes, then idles.
 * Entered on a yes (an accepted trade, a choice with a next row) and on a
 * gift.
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
 * TRADER_IDLE_WANDER: the idle of pose 1: walks (motion 0xD) for 100..163
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
        em->timer = (Rand_Next(1) & 0xFFFF & 0x3F) + 0x64;
        if (em->motionId[0] != EM_MOTION_ID(0, TRADER_MOT_WALK)) {
            Em_SetMotion(em, TRADER_MOT_WALK, 6, 0);
        }
        break;
    case 1:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            Em_SetStateRand(em, 0, 0xB);
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
 * TRADER_IDLE_FIDGET: plays the one-shot motion 0xE, then idles (back to
 * the wander in pose 1); greets the player when they are near and talking.
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
    em->noHitTimer = 0x63;
    em->noPushTimer = 0x63;
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
 * Spawns effect 01 of type 0 on the trader (Shadow_Spawn); in the first
 * two slots of EmTrader_Callbacks.
 */
void EmTrader_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0);
}

/* Does nothing; fills two slots of EmTrader_Callbacks. */
void EmTrader_TableNop(void)
{
}

/* The file's initialised data, in the game's order */
s32 GmEm_ReturnZero(void); /* gm_sub */
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
    EmTrader_SpawnEffect01, EmTrader_SpawnEffect01, EmTrader_TableNop, EmTrader_UpdateMotionEffects,
    GmEm_ReturnZero, EmTrader_TableNop, NULL, NULL,
};
