#ifndef EM_TRADER_H
#define EM_TRADER_H
#include "em.h"

/*
 * The trader's vocabulary (EM_KIND_TRADER, EmTrader_*): its motions, its
 * states (group 0 only), the kinds of its messages (EmTraderMsg.kind, which
 * say what an answer to the message does) and the outcomes of an offer.
 */

/* Motions: the table's row N plays motion N. */
enum EmTraderMotion {
    TRADER_MOT_STAND = 0x01,            /* the standing loop: TRADER_IDLE_STAND, Init, after the jump, REST's pause, the talk wait in REFUSE_* and AGREE */
    TRADER_MOT_HIDDEN = 0x02,           /* TRADER_IDLE_HIDDEN's pose (and Init's pose 0 case) */
    TRADER_MOT_JUMP_OUT = 0x03,         /* TRADER_IDLE_JUMP_OUT: the leap, airborne from frame 14 */
    TRADER_MOT_LAND = 0x04,             /* TRADER_IDLE_JUMP_OUT: the landing */
    TRADER_MOT_REST = 0x05,             /* TRADER_IDLE_REST's loop (pose 2), also TRADER_IDLE_STAND in pose 2 */
    TRADER_MOT_REST_FIDGET = 0x06,      /* TRADER_IDLE_REST's one-shot */
    TRADER_MOT_GREET = 0x07,            /* TRADER_IDLE_GREET: turning to the player as the talk opens */
    TRADER_MOT_TALK = 0x08,             /* TRADER_IDLE_TALK's loop */
    TRADER_MOT_REFUSE_B = 0x09,         /* TRADER_IDLE_REFUSE_B */
    TRADER_MOT_REFUSE_C = 0x0A,         /* TRADER_IDLE_REFUSE_C */
    TRADER_MOT_REFUSE_A = 0x0B,         /* TRADER_IDLE_REFUSE_A */
    TRADER_MOT_AGREE = 0x0C,            /* TRADER_IDLE_AGREE: the gesture on a yes or a gift */
    TRADER_MOT_WALK = 0x0D,             /* TRADER_IDLE_WANDER (and ToIdle in pose 1) */
    TRADER_MOT_FIDGET = 0x0E            /* TRADER_IDLE_FIDGET's one-shot (long, many sounds) */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound helpers read the position from). Every sound the
 * trader plays is placed at the skeleton's first bone.
 *
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeAtBoneOnFrame); Em_PlayCommonSeAtBoneOnFrame
 * plays from the common bank. None is named: no sound plays on the frame of
 * an event it goes with (the jump starts at frame 14 of TRADER_MOT_JUMP_OUT,
 * its sounds play at 6, 22 and 26).
 */
enum EmTraderBone {
    TRADER_BONE_ROOT = 0            /* the skeleton's first bone: where every sound plays */
};

/* Group 0, EM_GROUP_IDLE (EmTrader_RunIdleState). */
enum EmTraderIdleState {
    TRADER_IDLE_STAND = 0x00,           /* stands in place, waits to be talked to */
    TRADER_IDLE_HIDDEN = 0x01,          /* hidden at home until the player comes near (never reached) */
    TRADER_IDLE_JUMP_OUT = 0x02,        /* leaps out and lands */
    TRADER_IDLE_GREET = 0x03,           /* turns to the player, opens the talk */
    TRADER_IDLE_TALK = 0x04,            /* talking loop until the talk closes */
    TRADER_IDLE_REFUSE_A = 0x05,        /* refusal at 7 trades, gesture TRADER_MOT_REFUSE_A */
    TRADER_IDLE_REST = 0x06,            /* idle of pose 2 with random fidgets */
    TRADER_IDLE_REFUSE_B = 0x07,        /* refusal at 7 trades, gesture TRADER_MOT_REFUSE_B */
    TRADER_IDLE_REFUSE_C = 0x08,        /* refusal at 7 trades, gesture TRADER_MOT_REFUSE_C */
    TRADER_IDLE_AGREE = 0x09,           /* gesture on a yes or a gift, then talk */
    TRADER_IDLE_WANDER = 0x0A,          /* walks steering toward home (pose 1 idle) */
    TRADER_IDLE_FIDGET = 0x0B           /* one-shot fidget, then back to wandering */
};

/* EmTraderMsg.kind: what EmTrader_RunTalk does when the message is answered. */
enum EmTraderMsgKind {
    TRADER_MSG_CLOSE = 0,               /* an answer ends the talk (tradeMsgTimer = 0) */
    TRADER_MSG_CHOICE = 1,              /* two answers: first goes to next (with TRADER_IDLE_AGREE), second to nextNo, 255 closes; waits 30 frames; no row of gEmTraderMsgs uses it */
    TRADER_MSG_NEXT = 2,                /* an answer goes on to next (255 closes) */
    TRADER_MSG_OFFER = 3,               /* an answer works out the trade (the RESULT values below) and shows its message; rows 0/1, the opener of TRADER_IDLE_GREET */
    TRADER_MSG_CONFIRM_TRADE = 4,       /* yes: TRADER_IDLE_AGREE, Pouch_Add tradeGiveItem by tradeGiveCount and tradeGetItem by 1, clears gQuest.traderWantItem if it was the item given, notices; shows 16 (after 14/15) or 12 (after 8/9); no: shows 18 or 10; waits 30 frames */
    TRADER_MSG_ASK_GIFT = 5,            /* yes: EmTrader_ShowGiftOrChat */
    TRADER_MSG_SWAP_CARRIED = 6,        /* takes the carried item (Pouch_FindCarryItem */
    TRADER_MSG_GIFT = 7,                /* adds Player.tradeGetItem (1) and the got-item notice */
    TRADER_MSG_CHAT_A = 8,              /* shows a random line 38..66 (Rand % 29) */
    TRADER_MSG_CHAT_B = 9,              /* shows a random line 67..88 (Rand % 22) */
    TRADER_MSG_DRILL_CHARM = 10,        /* yes: shows 96 and Quest_DrillTakeCharm */
    TRADER_MSG_REDEEM_TOKENS = 11       /* takes 5 of item 696 and gives a weighted random item from EmTrader_TradeRates (roll % 32, default 94), notice 3 when Pouch_Add returns 3 else 13; rows 102/103 */
};

/* The outcome of an offer (TRADER_MSG_OFFER), which picks the message EmTrader_RunTalk shows. */
enum EmTraderOffer {
    TRADE_NONE = 0,                     /* nothing to trade: EmTrader_ShowGiftOrChat */
    TRADE_UNUSED_1 = 1,                 /* never set */
    TRADE_OFFER = 2,                    /* a swap is ready (one of the five fixed swaps checked in order, or 90% a random row of the map table the player holds the give item of, with room for the get item): row 14, TRADER_MSG_CONFIRM_TRADE */
    TRADE_UNUSED_3 = 3,                 /* never set */
    TRADE_CARRYING = 4,                 /* the player carries an item (Pouch_FindCarryItem != 65535), checked first: row 24 leading to TRADER_MSG_SWAP_CARRIED */
    TRADE_WANT_NEW = 5,                 /* 10% when nothing else: picks a wanted item from map table rows 7..14 into gQuest.traderWantItem (and offers its reward), caps tradeCount at 2: row 2 */
    TRADE_WANT_MISSING = 6,             /* a wanted item is set but the player has none (or it is not in the map table): row 6; tradeCount - 1 and cooldown cleared */
    TRADE_WANT_READY = 7,               /* the player has the wanted item and room for its reward: row 8, TRADER_MSG_CONFIRM_TRADE */
    TRADE_TOKENS = 8,                   /* online (Game_IsOnline) with 5 or more of item 696: row 100 leading to TRADER_MSG_REDEEM_TOKENS */
    TRADE_WANT_NO_ROOM = 9              /* has the wanted item but no room for the reward: row 104 */
};

extern void *EmTrader_Callbacks[8];
void EmTrader_MotionSounds(void *em, EmWork *work);
s32 EmTrader_MsgIdForVariant(Em *em, s32 item);
s32 EmTrader_IsPlayerNear(Em *em);
void EmTrader_ShowMessage(Em *em, u16 id);
s32 EmTrader_RunTalk(Em *em);
void EmTrader_Init(Em *em);
void EmTrader_ToIdle(Em *em, u8 mode, u16 blend, u16 startFrame);
void EmTrader_TurnToYaw(Em *em);
void EmTrader_StandIdle(Em *em);
void EmTrader_JumpOut(Em *em);
void EmTrader_GreetPlayer(Em *em);
void EmTrader_Talk(Em *em);
void EmTrader_RefuseTrade(Em *em, s32 variant);
void EmTrader_AgreeGesture(Em *em);
void EmTrader_WanderNearHome(Em *em);
void EmTrader_Update(Em *em);
void EmTrader_RunState(Em *em);

#endif
