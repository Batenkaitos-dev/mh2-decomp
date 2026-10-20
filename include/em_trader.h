#ifndef EM_TRADER_H
#define EM_TRADER_H

/*
 * The trader's vocabulary (EM_KIND_TRADER, EmTrader_*): its motions, its
 * states (group 0 only), the kinds of its messages (EmTraderMsg.kind, which
 * say what an answer to the message does) and the outcomes of an offer.
 */

/* Motions: the table's row N plays motion N. */
enum EmTraderMotion {
    TRADER_MOT_STAND = 0x01,            /* the standing loop: TRADER_IDLE_STAND, Init, after the ... */
    TRADER_MOT_HIDDEN = 0x02,           /* TRADER_IDLE_HIDDEN's pose (and Init's pose 0 case) */
    TRADER_MOT_JUMP_OUT = 0x03,         /* TRADER_IDLE_JUMP_OUT: the leap, airborne from frame 14 */
    TRADER_MOT_LAND = 0x04,             /* TRADER_IDLE_JUMP_OUT: the landing */
    TRADER_MOT_REST = 0x05,             /* TRADER_IDLE_REST's loop (pose 2), also TRADER_IDLE_STAND ... */
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

/* Group 0, EM_GROUP_IDLE (EmTrader_RunIdleState). */
enum EmTraderIdleState {
    TRADER_IDLE_STAND = 0x00,           /* stands in place, waits to be talked to */
    TRADER_IDLE_HIDDEN = 0x01,          /* hidden at home until the player comes near (never reached) */
    TRADER_IDLE_JUMP_OUT = 0x02,        /* leaps out and lands */
    TRADER_IDLE_GREET = 0x03,           /* turns to the player, opens the talk */
    TRADER_IDLE_TALK = 0x04,            /* talking loop until the talk closes */
    TRADER_IDLE_REFUSE_A = 0x05,        /* refusal at 7 trades, gesture motion 0xB */
    TRADER_IDLE_REST = 0x06,            /* idle of pose 2 with random fidgets */
    TRADER_IDLE_REFUSE_B = 0x07,        /* refusal at 7 trades, gesture motion 9 */
    TRADER_IDLE_REFUSE_C = 0x08,        /* refusal at 7 trades, gesture motion 0xA */
    TRADER_IDLE_AGREE = 0x09,           /* gesture on a yes or a gift, then talk */
    TRADER_IDLE_WANDER = 0x0A,          /* walks steering toward home (pose 1 idle) */
    TRADER_IDLE_FIDGET = 0x0B           /* one-shot fidget, then back to wandering */
};

/* EmTraderMsg.kind: what EmTrader_RunTalk does when the message is answered. */
enum EmTraderMsgKind {
    TRADER_MSG_CLOSE = 0,               /* an answer ends the talk (tradeMsgTimer = 0) */
    TRADER_MSG_CHOICE = 1,              /* two answers: first goes to next (with TRADER_IDLE_AGREE), ... */
    TRADER_MSG_NEXT = 2,                /* an answer goes on to next (0xFF closes) */
    TRADER_MSG_OFFER = 3,               /* an answer works out the trade (the RESULT values below) ... */
    TRADER_MSG_CONFIRM_TRADE = 4,       /* yes: TRADER_IDLE_AGREE, Pouch_Add tradeGiveItem by ... */
    TRADER_MSG_ASK_GIFT = 5,            /* yes: EmTrader_ShowGiftOrChat */
    TRADER_MSG_SWAP_CARRIED = 6,        /* takes the carried item (Pouch_FindCarryItem */
    TRADER_MSG_GIFT = 7,                /* adds Player.tradeGetItem (1) and the got-item notice */
    TRADER_MSG_CHAT_A = 8,              /* shows a random line 0x26..0x42 (Rand % 29) */
    TRADER_MSG_CHAT_B = 9,              /* shows a random line 0x43..0x58 (Rand % 22) */
    TRADER_MSG_DRILL_CHARM = 10,        /* yes: shows 0x60 and Quest_DrillTakeCharm */
    TRADER_MSG_REDEEM_TOKENS = 11       /* takes 5 of item 0x2B8 and gives a weighted random item ... */
};

/* The outcome of an offer (TRADER_MSG_OFFER), which picks the message EmTrader_RunTalk shows. */
enum EmTraderOffer {
    TRADE_NONE = 0,                     /* nothing to trade: EmTrader_ShowGiftOrChat */
    TRADE_UNUSED_1 = 1,                 /* never set */
    TRADE_OFFER = 2,                    /* a swap is ready (one of the five fixed swaps checked in ... */
    TRADE_UNUSED_3 = 3,                 /* never set */
    TRADE_CARRYING = 4,                 /* the player carries an item (Pouch_FindCarryItem != ... */
    TRADE_WANT_NEW = 5,                 /* 10% when nothing else: picks a wanted item from map table ... */
    TRADE_WANT_MISSING = 6,             /* a wanted item is set but the player has none (or it is ... */
    TRADE_WANT_READY = 7,               /* the player has the wanted item and room for its reward: ... */
    TRADE_TOKENS = 8,                   /* online with 5 or more of item 0x2B8: row 0x64 leading to ... */
    TRADE_WANT_NO_ROOM = 9              /* has the wanted item but no room for the reward: row 0x68 */
};

#endif
