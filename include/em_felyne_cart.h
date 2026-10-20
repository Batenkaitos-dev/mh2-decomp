#ifndef EM_FELYNE_CART_H
#define EM_FELYNE_CART_H

/*
 * The Felyne cart's vocabulary (EM_KIND_FELYNE_CART): the cart the Felynes
 * pull to carry a fainted player back to the camp. The quest's set-up adds
 * the kind to the camp's area; Player_StateCart makes one at the fainted
 * player and follows its bone, and the cart's script picks its states.
 *
 * Motions: row N of the cart's motion table plays motion N, kept in
 * Em.motionId as EM_MOTION_ID(layer, N).
 */
enum EmFelyneCartMotion {
    CART_MOT_IDLE = 0x01,   /* standing */
    CART_MOT_RUN = 0x02,    /* running: dust at the feet every 12 frames */
    CART_MOT_DROP = 0x03    /* dropping the player: Player_StateCart moves the player to PL_EVENT_CART_DROP when it starts */
};

/* Group 0, EM_GROUP_IDLE (EmFelyneCart_RunIdleState). */
enum EmFelyneCartIdleState {
    CART_IDLE_NONE = 0x00,
    CART_IDLE_WAIT = 0x01,          /* standing for 200 frames */
    CART_IDLE_DROP_PLAYER = 0x02    /* CART_MOT_DROP once */
};

/* Group 1, EM_GROUP_MOVE (EmFelyneCart_RunMoveState): running in turns and lines. */
enum EmFelyneCartMoveState {
    CART_MOVE_CIRCLE = 0x00,        /* a full circle over 600 frames */
    CART_MOVE_CARRY = 0x01,         /* 90 frames, while the player it carries is in PL_GROUP_EVENT */
    CART_MOVE_TURN_AROUND = 0x02,   /* a turn of a little over half a circle in 53 frames */
    CART_MOVE_RUN_OFF = 0x03        /* 100 frames, fading out over the last 5, then gone */
};

#endif
