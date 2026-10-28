#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include "item.h"
#include "motion.h"
#include "stageobj.h"
#include "em.h"

/* The weapon types (Player.weaponType): Player_RunWeaponState runs each
 * one's own states (state group PL_GROUP_WEAPON); the two bowguns share
 * theirs. */
enum WeaponType {
    WEAPON_GREAT_SWORD = 0,
    WEAPON_HEAVY_BOWGUN = 1,
    WEAPON_HAMMER = 2,
    WEAPON_LANCE = 3,
    WEAPON_SWORD_SHIELD = 4,
    WEAPON_LIGHT_BOWGUN = 5,
    WEAPON_DUAL_SWORDS = 6,
    WEAPON_LONG_SWORD = 7,
    WEAPON_HUNTING_HORN = 8,
    WEAPON_GUNLANCE = 9,
    WEAPON_BOW = 10
};

/*
 * The player's state is a group (Player.stateGroup) and a state within it
 * (Player.state). Player_SetState, Player_RequestState and Player_IsState
 * take the pair; Player_RunState runs the group's dispatcher, which runs the
 * state's handler (Player_State*). The names below come from what the
 * handlers do and what starts them.
 */
enum PlayerStateGroup {
    PL_GROUP_ACTION = 0,  /* the field actions: idle, moving, climbing, items (enum PlayerActionState) */
    PL_GROUP_WEAPON = 1,  /* the drawn weapon's own states, per weapon type (Player_RunWeaponState) */
    PL_GROUP_REACT = 2,   /* hits, the guard, the statuses (enum PlayerReactState) */
    PL_GROUP_FAINT = 3,   /* fainted (enum PlayerFaintState) */
    PL_GROUP_EVENT = 4,   /* the quest's events: the cart, the start and the end (enum PlayerEventState) */
    PL_GROUP_CARRY = 5,   /* carrying an item in both arms (enum PlayerCarryState) */
    PL_GROUP_GESTURE = 6  /* the gestures (enum PlayerGestureState) */
};

/* The flags of Player_SetState and Player_RequestState. The local player
 * sends each state change to the others (QSync_SendPlayer); bits 4 and 8 go
 * with it and tell the receivers how to follow (qnet_sync.c). */
#define PL_SET_KEEP_PARTS 0x01  /* keep the part motion overrides and aimWeight */
#define PL_SET_NO_SYNC 0x02     /* send nothing */
#define PL_SET_SNAP_POS 0x04    /* the receivers take the position at once, not over 10 frames */
#define PL_SET_SNAP_YAW 0x08    /* the receivers take the heading at once, not by turning */
#define PL_SET_SYNC_OFFER 0x10  /* send it as message 6, with the item offer (offerTo, offerItem) */
#define PL_SET_KEEP_RAND 0x20   /* keep the low byte of stateRand (Player_RequestStateSynced) */
#define PL_SET_SYNC_ALT 0x40    /* send it as message 9 (seen: state PL_ACT_MOUNTED_GUN_FIRE) */

/* Player.input's bits are the game's pad bits (PAD_*, include/pad.h; the
 * left stick's directions in stickDir are the D-pad's). These two masks are
 * the player code's own. */
#define PL_RSTICK_ANY 0x3C   /* stickDir: the right stick pushed any way (the attack stick) */
#define PL_STICK_HELD 0x28   /* input.stickTilt: the left stick counts as held from this tilt */
#define PL_ANY_BUTTON 0x3FF  /* pressed: any of the ten buttons (PAD_R3 to PAD_SQUARE): mashing out of a hold */

/* The slots of gPlayers (the players, then the lobby's other characters). */
#define PLAYER_SLOTS 10

/* Player.action: the body's posture, which the states set and the hit, item
 * and motion code test. */
#define PL_POSTURE_STANDING 0  /* on its feet */
#define PL_POSTURE_CROUCHED 1  /* crouched (the crouch, the crouched walk and gathering) */
#define PL_POSTURE_AIRBORNE 2  /* in the air: a leap, a fall, a jump */
#define PL_POSTURE_DOWN 3      /* on the ground after a blow (blown back or forward, inhaled) */
/* Or'ed into Player_SetPosture's posture: leaves PLF_CAM_FOLLOW clear (the
 * crouch, a hard landing, the wall holds, the battle alert). */
#define PL_POSTURE_NO_CAM_FOLLOW 0x8000

/* Player.songEffect and songEffectTimer, by kind: the effects of the hunting
 * horn's songs, which HuntingHorn_PlaySong alone sets. A kind is on while its
 * timer runs (Player_HasSongEffect), and songEffect[kind] holds its level,
 * 1 to 4; the attack and defense kinds count only in a quest (outside the
 * lobby: gScene.inLobby 0). */
#define PL_SONG_SPEED 0        /* level 1: the horn's playing walk moves faster; 2: attacks don't bounce */
#define PL_SONG_ATTACK 1       /* attack x1.1, x1.15 or x1.25 by level (Player_CalcAttack) */
#define PL_SONG_DEFENSE 2      /* defense up by level */
#define PL_SONG_HEALTH 3       /* more health: the level is the amount, 10, 20 or 30 */
#define PL_SONG_ROAR_GUARD 4   /* level 1 blocks the weak roars, 2 the roars, 3 the strong ones too (Player_ReactToHit) */
#define PL_SONG_STAMINA 5      /* the stamina doesn't drain, as with staminaLockTimer */
#define PL_SONG_RES_FIRE 6     /* 6 to 10: a resistance up by level (gResistSongEffects maps Player.resist to them) */
#define PL_SONG_RES_WATER 7
#define PL_SONG_RES_THUNDER 8
#define PL_SONG_RES_DRAGON 9
#define PL_SONG_RES_ICE 10
#define PL_SONG_KINDS 12       /* the arrays' length; kind 11 is never set */

/* The "none" values of Player.lookPartId (no look model for the part;
 * Player_InitPartTable) and Player.weaponModelKind (no weapon model loaded). */
#define PL_LOOK_PART_NONE 0xFF
#define PL_WEAPON_MODEL_NONE 0xFFFF

/* Player.attack's cap (Player_CalcAttack). */
#define PL_ATTACK_MAX 800

/* The length of Player.skills: the skills the equipment gives. */
#define PL_SKILL_SLOTS 5

/* Player.carry: the high nibble is the carried item's kind less one
 * (Item_CarryKind - 1); the low nibble is nonzero while the item is held.
 * The carry states set it to PL_CARRY_HOLD every frame
 * (Player_UpdateCarryKind) and the timers count it down, so it runs out a
 * few frames after the carry ends; a drop clears it at once. */
#define PL_CARRY_KIND_MASK 0xF0
#define PL_CARRY_KIND_SHIFT 4
#define PL_CARRY_HOLD_MASK 0x0F
#define PL_CARRY_HOLD 4        /* set by the carry states each frame */
#define PL_CARRY_HOLD_ENTER 5  /* set by Player_Reset when the hunter enters another area holding the item */

/* Player.areaMoveState: how Player_Reset places the hunter when the area
 * it moves to is set up. */
#define PL_AREA_MOVE_NONE 0     /* no move: the quest's start, at its start point */
#define PL_AREA_MOVE_ENTER 1    /* enters another area (an exit spot, the return to camp): at startPos */
#define PL_AREA_MOVE_CART 2     /* carted after a faint: at the map's reset point, into PL_EVENT_CART */
#define PL_AREA_MOVE_RESPAWN 3  /* back on its feet after a faint without the cart (the tower's areas): at startPos, idle */

/* Player.offerState: an item handed between two players. The asker
 * (PL_ACT_TAKE_ITEM_WAIT) waits for it to leave PL_OFFER_WAITING; the giver
 * (PL_ACT_OFFER_ITEM) holds the asker's index for a moment when the ask
 * arrives, then PL_OFFER_GIVEN. Any nonzero value ends the giver's offer. */
#define PL_OFFER_WAITING 0x00   /* no answer yet */
#define PL_OFFER_RECEIVED 0x01  /* the asker: the item arrived */
#define PL_OFFER_REFUSED 0x02   /* the asker: the giver refused */
#define PL_OFFER_GIVEN 0xFF     /* the giver: the item went to the asker */

/* Player motion ids (Player_SetMotionPair's id) named so far, by use.
 * Item_Use ends each item's use with one of them. */
#define PL_MOT_EAT          0x192  /* eating: the foods */
#define PL_MOT_ITEM_POSE    0x197  /* the pose after an item's good effect */
#define PL_MOT_ITEM_NO_GAIN 0x198  /* no good effect on the player: a cure that failed, the poison gamble, a prize draw */
#define PL_MOT_PET_END      0x26B  /* the town: the player's motion as a petting of the pet ends, whatever its result (Poogie_UpdatePetting) */

/* The player's motions (Player_SetMotionPair's motion), named by the states
 * that play them. */
#define PL_MOT_IDLE                  0x001  /* Player_StateIdle (motion 1 is the plain idle; back from the tired idle once staminaMax > 75), Player_StateBedMenu, Player_StateBoxMenu, Player_StateTakeItemWait, Player_StateTrade: standing still while a menu/trade/answer is pending */
#define PL_MOT_WALK                  0x002  /* Player_StateMove variant 1 PL_ACT_WALK; after 5 passes of frame 108 it plays PL_MOT_WALK_VARIATION, then comes back to 2 from frame 36 */
#define PL_MOT_RUN                   0x003  /* Player_StateMove variant 0 PL_ACT_RUN and 6 PL_ACT_RUN_AFTER_DASH; Player_StateDash plays it too with motion step 2.5 (the sprint is the run sped up) */
#define PL_MOT_EXHAUSTED_PANT        0x005  /* Player_StateOutOfBreath (the 150-frame panting loop), Player_StateIdle (follows PL_MOT_EXHAUSTED_START from frame 144; an effect every 32 frames), Player_StateQuestAbort (after PL_MOT_EXHAUSTED_START) */
#define PL_MOT_EXHAUSTED_RECOVER     0x006  /* Player_StateOutOfBreath: after the pant; at its end stamina is refilled to staminaMax and idle */
#define PL_MOT_CROUCH_DOWN           0x007  /* Player_StateCrouch variant 0, Player_StateKneelToPlace, Player_StateBinoculars variant 0: going down into the crouch (Player_UpdateLean(pl, 7) while it plays) */
#define PL_MOT_CROUCH                0x008  /* the held crouch: Player_StateCrouch, Player_StateCrouchStop, Player_StateKneelToPlace(Crouched), Player_StatePlaceItem variant 0, Player_StateBinoculars (Player_UpdateLean(pl, 8)) */
#define PL_MOT_CROUCH_STAND_UP       0x009  /* Player_StateCrouchStandUp; Player_StatePlaceItem variant 0 after 14 frames (stands up after setting the tool down) */
#define PL_MOT_CROUCH_WALK           0x00A  /* Player_StateCrouchWalk; Player_StateMove variant 5 PL_ACT_CROUCH_WALK_ALT and variant 3 PL_ACT_RIDE_WALK (seen: the walk on a ridden monster uses the crouched walk) */
#define PL_MOT_JUMP_UP_CROUCH        0x00B  /* Player_StateClimbAirborne variant 0 PL_ACT_JUMP_TO_LEDGE: the crouch before the jump up toward a high ledge */
#define PL_MOT_CLIMB_AIRBORNE        0x00C  /* Player_StateClimbAirborne: the jump up (after PL_MOT_JUMP_UP_CROUCH), PL_ACT_WALL_LET_GO and PL_ACT_LEDGE_DROP (from frame 22): in the air off a wall or ledge */
#define PL_MOT_CLIMB_LAND            0x00D  /* Player_StateClimbAirborne: the landing after PL_MOT_CLIMB_AIRBORNE (short fall), then idle */
#define PL_MOT_LEAP_AIRBORNE         0x00F  /* Player_StateLeapOrFall: variant 0 the leap off a ledge, variant 1 the fall (from frame 32) */
#define PL_MOT_LAND_INTO_DASH        0x010  /* Player_StateLeapOrFall: the landing with R2 held and the stick pushed (sheathed); then PL_ACT_DASH_RESUME or PL_ACT_FLEE_DASH */
#define PL_MOT_LEDGE_HANG            0x011  /* Player_StateLedgeHang: hangs from a ledge */
#define PL_MOT_LEDGE_CLIMB_UP        0x012  /* Player_StateLedgePullUp and Player_StateStepUp (from frame 88): climbs up onto the ledge/step */
#define PL_MOT_LAND_HARD             0x013  /* Player_StateHardLanding: the landing after a long fall (pad and camera shake) */
#define PL_MOT_EXHAUSTED_START       0x015  /* Player_StateOutOfBreath variant 1 and Player_StateQuestAbort: doubles over, then PL_MOT_EXHAUSTED_PANT (Player_StateIdle also chains it into PL_MOT_EXHAUSTED_PANT) */
#define PL_MOT_ROLL_FROM_CROUCH      0x016  /* Player_StateRoll variant 1 PL_ACT_ROLL_FROM_CROUCH: rises out of the crouch, then PL_MOT_ROLL from frame 6 */
#define PL_MOT_STAGGER               0x01A  /* Player_StateRideStagger (standing on a fast-moving monster), Player_StateStun (dazed, stun stars), Player_StateQuake (shaken by a tremor): staggering in place, unable to act */
#define PL_MOT_ROLL                  0x01C  /* Player_StateRoll: the roll (sheathed, or drawn with any weapon but hammer/hunting horn) */
#define PL_MOT_WALK_VARIATION        0x01D  /* Player_StateMove: every fifth walk cycle (motion 2 passing frame 108) plays this once, then back to PL_MOT_WALK (likely a look-around while walking; the code shows only the timing) */
#define PL_MOT_RUN_STOP              0x01E  /* Player_StateRunStop: the skid at the end of a run or sprint */
#define PL_MOT_WALL_CLIMB_UP         0x022  /* Player_StateWallClimbUp, Player_StateWallGrab (grabs, PL_ACT_WALL_HOLD at frame 90), Player_StateWallHold (held at frame 46 or 90) */
#define PL_MOT_WALL_CLIMB_DOWN       0x023  /* Player_StateWallClimbDown; Player_StateWallHold (held at frame 0 or 44) */
#define PL_MOT_WALL_DISMOUNT         0x024  /* Player_StateWallDismount: steps off the foot of the climbing wall */
#define PL_MOT_CROUCH_WALK_FAST      0x025  /* Player_StateCrouchWalkFast: the crouched walk with the stick fully pushed */
#define PL_MOT_FLEE_DASH             0x026  /* Player_StateFleeDash variants 0 and 2: the sprint away from the monster in battle */
#define PL_MOT_DASH_TIRED            0x027  /* Player_StateFleeDash variant 1 PL_ACT_DASH_TIRED (stamina under 76), Player_StateTiredStop */
#define PL_MOT_BATTLE_ALERT          0x029  /* Player_StateBattleAlert: the start of a battle with the weapon sheathed */
#define PL_MOT_TURN_AROUND           0x02A  /* Player_StateTurnAround: the quick half turn when the stick is pulled back against a run */
#define PL_MOT_WALL_PRESS            0x02B  /* Player_StateWallPress: puts its back to a wall (variant 1 holds it from frame 30) */
#define PL_MOT_WALL_SIDLE_NEG        0x02C  /* Player_StateWallSidle variant 0 PL_ACT_WALL_SIDLE_NEG */
#define PL_MOT_WALL_SIDLE_POS        0x02D  /* Player_StateWallSidle variant 1 PL_ACT_WALL_SIDLE_POS */
#define PL_MOT_DIVE_START            0x02E  /* Player_StateDive: the take-off of the forward dive (75 stamina, hit-free) */
#define PL_MOT_DIVE_AIR              0x02F  /* Player_StateDive: in the air, 16 forward */
#define PL_MOT_DIVE_LAND             0x030  /* Player_StateDive: the belly landing (pad and camera shake) */
#define PL_MOT_WALK_SLOW             0x031  /* Player_StateMove variant 7 PL_ACT_WALK_SLOW */
#define PL_MOT_SHARED_ATTACK         0x032  /* Player_StateSharedAttack: the attack every weapon shares (PlayerAttack 5) */
#define PL_MOT_CARRY_IDLE            0x033  /* Player_StateCarryIdle, Player_StateCarryHandOver (waiting), Player_StateMashIdle: standing holding a carried item; the mash status reuses the carry motions */
#define PL_MOT_CARRY_WALK            0x034  /* Player_StateCarryWalk, Player_StateMashWalk */
#define PL_MOT_CARRY_RUN             0x035  /* Player_StateCarryRun, Player_StateMashRun: the run for gender 0 (seen in Player_StateCarryRun's gender test) */
#define PL_MOT_CARRY_PUT_DOWN        0x036  /* Player_StateCarryPutDown variants 0 and 2 (dropped or delivered, released at frame 114), Player_StateCarryHandOver (to the trader, frame 114) */
#define PL_MOT_CARRY_DROP_TIRED      0x037  /* Player_StateCarryPutDown variant 1 PL_CARRY_DROP_TIRED: let go at once (frame 4) when stamina runs out */
#define PL_MOT_CARRY_AIRBORNE        0x038  /* Player_StateCarryAirborne, Player_StateMashAirborne: hop off a ledge (frame 0) or fall (from frame 32) while carrying */
#define PL_MOT_CARRY_LAND_HARD       0x039  /* Player_StateCarryLand variant 0, Player_StateMashLand variant 0 */
#define PL_MOT_CARRY_LAND            0x03A  /* Player_StateCarryLand variant 1, Player_StateMashLand variant 1 */
#define PL_MOT_CARRY_RUN_ALT         0x03B  /* Player_StateCarryRun, Player_StateMashRun: the carry run for gender 1 */
#define PL_MOT_OFFER_ITEM            0x03C  /* Player_StateOfferItem: holds an item out to another player */
#define PL_MOT_OFFER_ITEM_END        0x03D  /* Player_StateOfferItemEnd: the giver's closing motion */
#define PL_MOT_LAND                  0x03E  /* Player_StateLeapOrFall: the ordinary landing (under 30 frames aloft) */
#define PL_MOT_IDLE_ALT              0x03F  /* Player_StateIdle, Player_StateBedMenu: replaces PL_MOT_IDLE after 240 frames for gender 1 */
#define PL_MOT_FLINCH                0x0C9  /* Player_StateFlinch, Player_StateFlinchCarrying with the weapon sheathed: the light hit */
#define PL_MOT_KNOCKDOWN_FACE_DOWN   0x0CA  /* Player_StateKnockdownFront: falls (then PL_MOT_GET_UP_FACE_DOWN) */
#define PL_MOT_KNOCKDOWN_FACE_UP     0x0CC  /* Player_StateKnockdownBack: falls (then PL_MOT_GET_UP_FACE_UP) */
#define PL_MOT_BLOWN_BACK_LAUNCH     0x0CD  /* Player_StateBlownBack: launched up and backward */
#define PL_MOT_BLOWN_BACK_LAND       0x0CE  /* Player_StateBlownBack: lands on the back (impact sound, armour rattle) */
#define PL_MOT_GET_UP_FACE_UP        0x0CF  /* getting up from lying on the back: Player_StateBlownBack, Player_StateKnockdownBack, Player_StateStatus2/4, Player_StateRevive (not after PL_MOT_LIE_FACE_DOWN), Player_StateGestureGetUp, Player_StateGesture03End (from frame 60) */
#define PL_MOT_BLOWN_FORWARD_LAUNCH  0x0D0  /* Player_StateBlownForward: flung forward */
#define PL_MOT_LIE_FACE_DOWN         0x0D1  /* lands/lies on the face: Player_StateBlownForward (landing), Player_StateFaint variant 2 PL_FAINT_FACE_DOWN (from frame 140), Player_StateCartDrop (from frame 70); Player_StateRevive tests it to pick PL_MOT_GET_UP_FACE_DOWN */
#define PL_MOT_LIE_FACE_UP           0x0D2  /* collapses/lies on the back: Player_StateFaint variants 0 and 1 (from frame 140), Player_StateStatus2/4 (lying loop), Player_StateGestureLieDown, Player_StateLeave */
#define PL_MOT_BRACE                 0x0D3  /* Player_StateWind (looped 4 or 1 times against wind pressure), Player_StateCannonFire (the cannon's recoil): bracing against a blast */
#define PL_MOT_BRACE_END             0x0D4  /* Player_StateWind, Player_StateCannonFire: the end of the brace, then idle */
#define PL_MOT_TOSSED_BACK_LAUNCH    0x0D5  /* Player_StateTossed variant 0, Player_StateInhaled: thrown up into the air backward */
#define PL_MOT_FLUNG_BACK_AIR        0x0D6  /* in the air after PL_MOT_BLOWN_BACK_LAUNCH or PL_MOT_TOSSED_BACK_LAUNCH: Player_StateBlownBack, Player_StateTossed variant 0, Player_StateInhaled */
#define PL_MOT_TOSSED_LAND           0x0D7  /* Player_StateTossed, Player_StateInhaled: the landing; then the player turns half a turn and plays PL_MOT_GET_UP_FACE_DOWN */
#define PL_MOT_GET_UP_FACE_DOWN      0x0D8  /* getting up from lying on the face: Player_StateKnockdownFront, Player_StateBlownForward, Player_StateTossed, Player_StateInhaled, Player_StateCartDrop, Player_StateRevive (after PL_MOT_LIE_FACE_DOWN), Player_StateFishReel (after PL_MOT_FISH_PULLED_DOWN) */
#define PL_MOT_ROAR_COVER            0x0DA  /* Player_StateRoar: covers up against the roar, pushed back until frame 40 */
#define PL_MOT_TOSSED_FORWARD_LAUNCH 0x0DB  /* Player_StateTossed variant 1: thrown up into the air forward */
#define PL_MOT_FLUNG_FORWARD_AIR     0x0DC  /* in the air after PL_MOT_BLOWN_FORWARD_LAUNCH or PL_MOT_TOSSED_FORWARD_LAUNCH: Player_StateBlownForward, Player_StateTossed variant 1, Player_StateInhaled */
#define PL_MOT_CART_RIDE             0x0DD  /* Player_StateCart: carried away by the cart (follows the cart's bone 29) */
#define PL_MOT_CART_DROPPED          0x0DE  /* Player_StateCartDrop: dropped off the cart at camp, sinks to the floor */
#define PL_MOT_LIE_FACE_UP_STIR      0x0DF  /* Player_StateFaint, Player_StateStatus4: a 1-in-64 random stir while lying on the back (after frame 164/170), then back to PL_MOT_LIE_FACE_UP */
#define PL_MOT_STATUS4_COLLAPSE      0x0E0  /* Player_StateStatus4 variant 0: falls into status 4 (unhittable, effect at bone 5), then PL_MOT_LIE_FACE_UP */
#define PL_MOT_FALL_ASLEEP           0x0E1  /* Player_StateStatus2: falls into status 2, then lies 240 frames regaining stamina (likely sleep: the same puff effect as Player_StateBedSleep) */
#define PL_MOT_ROAR_RECOVER          0x0E2  /* Player_StateRoar: lowers the cover after the roar, then idle */
#define PL_MOT_ROAR_STRONG_BRACE     0x0E3  /* Player_StateRoarStrong: braces against the strongest roar (unhittable to frame 64) */
#define PL_MOT_THROW_ITEM            0x191  /* Player_StateThrowItem variant 0 PL_ACT_THROW_ITEM (release at frame 24) */
#define PL_MOT_IDLE_TIRED            0x193  /* Player_StateIdle: replaces PL_MOT_IDLE or PL_MOT_IDLE_ALT while staminaMax is under 76 */
#define PL_MOT_KNEEL_GATHER          0x195  /* kneels, works, stands: Player_StateGather (gather, rewards, special, drop), Player_StateGatherCrouched (from frame 36), Player_StateSetTrap (kneel, to frame 130), Player_StateSetTrapDone (the stand-up from frame 206) */
#define PL_MOT_DRINK                 0x196  /* Player_StateConsume variant 0 PL_ACT_DRINK */
#define PL_MOT_PLACE_ITEM_CROUCHED   0x199  /* Player_StatePlaceItem variant 1 (from frame 20), Player_StateKneelToPlaceCrouched: sets a tool down from the crouch */
#define PL_MOT_SET_TRAP              0x19A  /* Player_StateSetTrap (after PL_MOT_KNEEL_GATHER), Player_StateSetTrapDone (from frame 60): sets the trap or device */
#define PL_MOT_GATHER_FROM_CROUCH    0x19B  /* Player_StateGatherCrouched variants 0, 2, 3: from the crouch into the gather (then PL_MOT_KNEEL_GATHER) */
#define PL_MOT_GATHER_TO_CROUCH      0x19C  /* Player_StateGatherCrouched: back to the crouch at frame 200 (Player_StateCrouch then blends 8 instead of 2) */
#define PL_MOT_SHARPEN               0x19D  /* Player_StateSharpen: whetstone; SKILL_SHARPEN_FAST jumps from frame 98 to 238 */
#define PL_MOT_MOUNTED_GUN           0x19E  /* Player_StateMountedGunAim (Player_UpdateLean(pl, PL_MOT_MOUNTED_GUN)), Player_StateMountedGunFire (from frame 66) */
#define PL_MOT_PICKAXE_SWING         0x19F  /* Player_StateGather variant 4 PL_ACT_MINE, Player_StateSpotStrike (strikes the countdown spot or the spot item) */
#define PL_MOT_EATING                0x1A0  /* Player_StateConsume variant 1 PL_ACT_EAT (SKILL_EAT_FAST speeds frames 38-180, SKILL_EAT_SLOW replays from frame 52); PL_MOT_EAT is the after-motion Item_Use plays */
#define PL_MOT_THROW_ITEM_ALT        0x1A1  /* Player_StateThrowItem variants 1 and 2 (release at frame 20); ItemAttack_SetupThrown uses ItemAttack_ThrowSpeedsAlt while this motion plays */
#define PL_MOT_NET_SWING             0x1A2  /* Player_StateGather variant 5 PL_ACT_BUG_NET */
#define PL_MOT_CARVE                 0x1A3  /* Player_StateGather variant 3 PL_ACT_CARVE (SKILL_GATHER_FAST skips to frame 224), Player_StateGatherCrouched variant 1 (from frame 68) */
#define PL_MOT_HORN_PLAY             0x1A4  /* Player_StateInstrumentPlay variant 0, Player_StateHornItemPlay: blows the horn item (repeated per round) */
#define PL_MOT_HORN_END              0x1A5  /* Player_StateInstrumentEnd variant 0, Player_StateHornItemPlay: lowers the horn */
#define PL_MOT_CARVE_FROM_CROUCH     0x1A6  /* Player_StateGatherCrouched variant 1 PL_ACT_CARVE_CROUCHED: into the carve (then PL_MOT_CARVE) */
#define PL_MOT_CARVE_TO_CROUCH       0x1A7  /* Player_StateGatherCrouched variant 1: back to the crouch at frame 294 */
#define PL_MOT_TEAM_ITEM_USE         0x1A8  /* Player_StateInstrumentPlay variant 1 PL_ACT_TEAM_ITEM_USE (played twice) */
#define PL_MOT_TEAM_ITEM_END         0x1A9  /* Player_StateInstrumentEnd variant 1 (Item_UseTeamPowder applied here) */
#define PL_MOT_BED_LIE_DOWN          0x1AA  /* Player_StateBedLieDown */
#define PL_MOT_BED_SLEEP             0x1AB  /* Player_StateBedSleep: the sleep loop (1 health a frame) */
#define PL_MOT_BED_SLEEP_STIR        0x1AC  /* Player_StateBedSleep: a 1-in-128 random stir, then back to PL_MOT_BED_SLEEP */
#define PL_MOT_BED_GET_UP            0x1AD  /* Player_StateBedGetUp: turns half a turn and gets out of bed */
#define PL_MOT_PLACE_LARGE_ITEM      0x1B0  /* Player_StatePlaceLargeItem (sets down at frame 78), Player_StatePlaceLargeItemSet (from frame 78) */
#define PL_MOT_TOWN_SIT              0x260  /* the town: sits down at a table (TownPc_State14) */
#define PL_MOT_TOWN_SEATED           0x261  /* the town: seated at a table, held (TownPc_State14 and the seat's gestures; the served meal: TOWN_PC_MEAL_SERVED plays 635, then holds it) */
#define PL_MOT_GESTURE_SCOUT         0x26C  /* Player_StateGestureScout: looks out; spotting the area's flyer within about 33 degrees of the facing sets scoutSpotted */
#define PL_MOT_GESTURE_SIT           0x26F  /* Player_StateGesture03 (looped, leave from frame 90) and Player_StateQuestEnd (sheathed, at the quest's end); likely sitting down: its end PL_MOT_GESTURE_SIT_END is followed by PL_MOT_GET_UP_FACE_UP */
#define PL_MOT_GESTURE_SIT_END       0x270  /* Player_StateGesture03End: then PL_MOT_GET_UP_FACE_UP from frame 60 */
#define PL_MOT_TAKE_ITEM             0x280  /* Player_StateTakeItem: the taker's motion once the offered item has arrived */
#define PL_MOT_SPIT_ROAST_START      0x320  /* Player_StateSpitRoastStart: sets up the spit */
#define PL_MOT_SPIT_ROAST_SUCCESS    0x321  /* Player_StateSpitRoastEnd variant 0 (done or gourmet) */
#define PL_MOT_SPIT_ROAST_FAIL       0x322  /* Player_StateSpitRoastEnd variant 1 (raw, rare or burnt) */
#define PL_MOT_SPIT_ROAST_TURN       0x323  /* Player_StateSpitRoast: turns the meat (starts at a random frame 0-63) */
#define PL_MOT_FISH_READY            0x324  /* Player_StateFishWait variant 0 PL_ACT_FISH_READY */
#define PL_MOT_FISH_CAST             0x325  /* Player_StateFishCast: casts; the lure appears at frame 104 */
#define PL_MOT_FISH_WAIT             0x326  /* Player_StateFishWait variant 1 PL_ACT_FISH_WAIT (the line out) */
#define PL_MOT_FISH_REEL             0x327  /* Player_StateFishReel: reels in (to frame 118, or twice with a monster on the line) */
#define PL_MOT_FISH_CATCH            0x328  /* Player_StateFishReel: lands the catch; the item is given at frame 120 */
#define PL_MOT_FISH_REEL_EMPTY       0x329  /* Player_StateFishReelEmpty: reels in nothing */
#define PL_MOT_FISH_PULLED_DOWN      0x32A  /* Player_StateFishReel: a monster took the bait (bait lost); likely pulled over, since PL_MOT_GET_UP_FACE_DOWN follows */
#define PL_MOT_WPN_IDLE              0x3E9  /* not set here; compared in Player_StateIdle and Player_BowgunAim: the drawn idle (gunners blend the aim layers 1005/1006 on it) */
#define PL_MOT_WPN_WALK              0x3EC  /* Player_StateMove variants 2 and 9: the drawn walk (blend by weapon type); ending it returns to gDrawnWalkStopIdle's idle */
#define PL_MOT_WPN_GUARD_START       0x3ED  /* Player_StateGuard variant 0 (sword and shield, lance, gunlance), the gunlance's alt hold (from frame 4); for gunners the aim layer of a positive aimWeight (Player_SetAimLayers) */
#define PL_MOT_WPN_GUARD_HOLD        0x3EE  /* Player_StateGuard hold (sword and shield, lance, gunlance); Player_StateSideEvade variant 1 for the long sword; the aim layer of a negative aimWeight (weapon-set slot: the meaning follows the weapon) */
#define PL_MOT_WPN_GUARD_END         0x3EF  /* Player_StateGuardEnd (sword and shield, lance, gunlance); Player_StateSideEvade variant 0 for the long sword (weapon-set slot) */
#define PL_MOT_WPN_GUARD_WALK        0x3F3  /* Player_StateMove variant 8 PL_ACT_GUARD_WALK; Player_StateRoll uses it as the drawn roll of the hammer and hunting horn; Player_GuardInput tests it (weapon-set slot) */
#define PL_MOT_WPN_EVADE_LEFT        0x3F4  /* Player_StateSideEvade variant 1 PL_ACT_EVADE_LEFT (not the long sword) */
#define PL_MOT_WPN_EVADE_RIGHT       0x3F5  /* Player_StateSideEvade variant 0 PL_ACT_EVADE_RIGHT (not the long sword) */
#define PL_MOT_LANCE_HOP_SIDE_A      0x3F6  /* Player_StateLanceHop variant 0 */
#define PL_MOT_LANCE_HOP_SIDE_B      0x3F7  /* Player_StateLanceHop variant 1 */
#define PL_MOT_LANCE_HOP_BACK        0x3F8  /* Player_StateLanceHop variants 2 and 3 (back, back long) */
#define PL_MOT_WPN_GUARD_DRAW        0x3FA  /* Player_StateGuardDraw: draws straight into the guard (lance, sword and shield, gunlance) */
#define PL_MOT_FLINCH_DRAWN          0x4BB  /* Player_StateFlinch, Player_StateFlinchCarrying with the weapon drawn */
#define PL_MOT_WPN_GUARD             0x584  /* Player_StateGuard for the great sword, long sword and default: guard start (blend 3) and hold (from frame 40) */
#define PL_MOT_WPN_GUARD_PUSH        0x585  /* Player_StateGuardPush (not the heavy bowgun): a light guarded hit */
#define PL_MOT_WPN_GUARD_STAGGER     0x586  /* Player_StateGuardRecoil variant 0 (short slide); Player_StateGuardEnd for the great sword, long sword and default */
#define PL_MOT_WPN_GUARD_KNOCKBACK   0x587  /* Player_StateGuardRecoil variant 1 (long slide), not the heavy bowgun */
#define PL_MOT_HBG_GUARD             0x58A  /* Player_StateGuard hold and Player_StateGuardPush for the heavy bowgun */
#define PL_MOT_HBG_GUARD_KNOCKBACK   0x58B  /* Player_StateGuardRecoil variant 1 for the heavy bowgun */
#define PL_MOT_HBG_GUARD_END         0x58D  /* Player_StateGuardEnd for the heavy bowgun */

/* The hammer's and the bow's charge motions. These ids are weapon-set slots:
 * with another weapon drawn the same id plays that weapon's motion (the
 * guard's, an evade's ...), so these names hold only in the hammer's and the
 * bow's own code. The charge's glow (effect type 6) ends once the player's
 * motion is none of these (the bow's list also holds 1419 and 1420). */
#define PL_MOT_HAMMER_CHARGE_WALK    0x3ED  /* Hammer_StateChargeWalk */
#define PL_MOT_HAMMER_CHARGE_START   0x3EE  /* Hammer_StateCharge: raises the hammer; the charge starts */
#define PL_MOT_HAMMER_CHARGE_HOLD    0x3EF  /* Hammer_StateCharge: holds the charge */
#define PL_MOT_HAMMER_CHARGE_ENTRY   0x582  /* Hammer_StateCharge variant 2, HAMMER_CHARGE_AFTER_ATTACK: the lead-in, then PL_MOT_HAMMER_CHARGE_START */
#define PL_MOT_BOW_CHARGE            0x57D  /* Bow_StateCharge: draws and holds the charge */
#define PL_MOT_BOW_CHARGE_WALK       0x3F1  /* Bow_StateChargeWalk */
#define PL_MOT_BOW_CHARGE_FROM_DRAW  0x3F4  /* Bow_StateCharge variant 5, BOW_DRAW_CHARGE: the lead-in, then PL_MOT_BOW_CHARGE */
#define PL_MOT_BOW_CHARGE_FROM_STAB  0x3F5  /* Bow_StateCharge variant 4, BOW_CHARGE_AFTER_STAB */
#define PL_MOT_BOW_CHARGE_FROM_SHOT  0x58A  /* Bow_StateCharge variant 3, BOW_CHARGE_AFTER_SHOT */
/* The dual swords' demon mode, by the same rule: these ids are the dual
 * swords' own motions only where the dual swords are known to be drawn. */
#define PL_MOT_DSWORDS_DEMON_ENTER   0x3FA  /* DualSwords_StateDemonToggle variants 0 and 2: enters the demon mode */
#define PL_MOT_DSWORDS_DEMON_LEAVE   0x3FB  /* DualSwords_StateDemonToggle variant 1: leaves it */

/* The player's hand bones: where a drawn weapon and the props are held. */
#define PL_BONE_HAND 0xE        /* the hand that holds the drawn weapon */
#define PL_BONE_OTHER_HAND 0x12 /* the other hand: a weapon in the other hand, most props */

/* Bits of Player.flags (Player_SetFlag, Player_ClearFlag, Player_TestFlag;
 * the monsters' code sets the same bits on an Em through these). */
#define PLF_HIT_WINDOW 0x1        /* the attack's hit window may run: cleared, Player_UpdateHitTimers closes it (Player_SetPosture's third argument) */
/* 0x2: set by Player_SetPosture's fourth argument, a hit and a guard's
 * recoil, cleared by a faint and the leave and abort states; no reader seen. */
#define PLF_CAM_FOLLOW 0x8        /* with L1 held, the camera swings behind the player (Player_SetPosture sets it unless PL_POSTURE_NO_CAM_FOLLOW) */
#define PLF_GUARD 0x8000          /* guarding: a hit inside the guard's arc is blocked (Player_InGuardArc) */
#define PLF_NO_ROOT_MOVE 0x20000  /* the motion's root movement is not applied (Player_MoveLocal skips it) */
#define PLF_FISHING 0x80000      /* fishing: set by the cast, wait and reel states and cleared when they end (Player_IsFishing) */

/* A wall a player or a monster touches: Em_CollideField's sweep (which
 * Field_Update also runs for the players) fills gEmWallContacts[index] with
 * one record per wall triangle it met, the special wall first, ended by
 * flags 0. The players read it to turn along a wall (Player_TurnAlongWall)
 * and to pick a voice (Motion_VoiceAtFrame). */
#define WALL_CONTACTS 11            /* records per object: gEmWallContacts[index][WALL_CONTACTS] */
#define WALL_CONTACT_HIT 0x1        /* WallContact.flags: a contact (every record before the end) */
#define WALL_CONTACT_SPECIAL 0x2    /* WallContact.flags: the triangle's kind has b[0] set in gAreaTriKindFlags; sorted to the front */
/* wallHitDirs (Player's and Em's): bit n is set for a contact whose
 * heading (plus a quarter turn) falls in the n-th 32nd of a turn. The
 * players' code tests two masks: the wall ahead (Player_TurnAlongWall,
 * Motion_VoiceAtFrame) and, for the other side, the wall behind
 * (Player_TurnAlongWallSide). */
#define WALL_DIRS_AHEAD 0xE0000007
#define WALL_DIRS_BEHIND 0x3E000
typedef struct WallContact {
    u16 flags;                  /* 0x0: WALL_CONTACT_*; 0 ends the list */
    u16 heading;                /* 0x2: to the contact point, relative to the object's rotY */
    f32 *normal;                /* 0x4: the triangle's normal */
    u8 seArg;                   /* 0x8: the wall's sound argument (the triangle's), passed with the voice (as Player.floorArg) */
    u8 pad9[3];
} WallContact;
typedef char WallContact_size_check[(sizeof(WallContact) == 0xC) ? 1 : -1];

/* Bits of Player.statusIcons: the HUD's status icons of a player, set each
 * frame by Player_UpdateStatusIcons (sub_main player_stats.c) for the local
 * player and sent to the others with its state. Hud_DrawStatusIcon draws
 * them; the map hides a player with PL_STATUS_ICON_STATUS7. */
#define PL_STATUS_ICON_POISON 0x0001       /* Player.poison */
#define PL_STATUS_ICON_STATUS2 0x0002      /* in PL_REACT_STATUS2 */
#define PL_STATUS_ICON_STATUS4 0x0004      /* in PL_REACT_STATUS4 or PL_REACT_STATUS4_HIT */
#define PL_STATUS_ICON_STUN 0x0008         /* in PL_REACT_STUN */
#define PL_STATUS_ICON_ATTACK_UP 0x0010    /* attackDrug + attackBuff, or song effect 1 */
#define PL_STATUS_ICON_DEFENSE_UP 0x0020   /* defenseDrug + defenseBuff, or song effect 2 */
#define PL_STATUS_ICON_MASH 0x0040         /* in the mash status (Player_InMashStatus) */
#define PL_STATUS_ICON_COOL_DRINK 0x0080   /* coolDrinkTimer */
#define PL_STATUS_ICON_HOT_DRINK 0x0100    /* hotDrinkTimer */
#define PL_STATUS_ICON_DRILL_CHARM 0x0200  /* Quest_DrillHasCharm */
#define PL_STATUS_ICON_STATUS6 0x0400      /* status6Timer */
#define PL_STATUS_ICON_STATUS7 0x0800      /* status7Timer */
#define PL_STATUS_ICON_REGEN 0x1000        /* regenTimer */
#define PL_STATUS_ICON_SONG 0x2000         /* any songEffectTimer */
#define PL_STATUS_ICON_VERSUS_FAINT 0x4000 /* fainted (PL_GROUP_FAINT) in a versus quest */
#define PL_STATUS_ICON_DEMON 0x8000        /* the dual swords' demon mode */
#define PL_SWORD_GAUGE_MAX 100              /* Player.swordGauge when the long sword's spirit gauge is full (LongSword_AddGauge) */

/* The kinds of Player_GetWeaponStat (sub_main equip_save.c): the equipped
 * weapon's element and status values (0 when the weapon has another), and its
 * affinity. The statuses are the blade's status types 1-3; a PlayerAttack
 * turns them into the hit's elementFlags 2, 4 and 1 (one hit in three). */
enum WeaponStatKind {
    WSTAT_FIRE = 0,
    WSTAT_WATER = 1,
    WSTAT_THUNDER = 2,
    WSTAT_ICE = 3,
    WSTAT_POISON = 4,        /* status type 1: elementFlags ELEMF_POISON */
    WSTAT_STATUS_TYPE2 = 5,  /* status type 2: elementFlags ELEMF_EM_STATUS4 */
    WSTAT_STATUS_TYPE3 = 6,  /* status type 3: elementFlags ELEMF_EM_STATUS2 */
    WSTAT_DRAGON = 7,
    WSTAT_AFFINITY = 8       /* percent: a critical hit's chance, or a weak hit's when negative */
};

/* The weapon group's one shared state: an attack every weapon has (button
 * bit 1 from idle; the attack object PlayerAttack kind 5; likely the kick). The
 * other weapon states are each weapon's own. */
#define PL_WPN_SHARED_ATTACK 0x63

/* The great sword's states (GreatSword_RunState, weapon_states.c). A flick of
 * the attack stick (input.stickDirPressed PL_RSTICK_ANY, its direction from
 * input.rightStickAngle: GreatSword_StickAttackDir) picks the attack: left or
 * right a sweep, up the overhead slash, down the charge. In an attack's
 * window a flick another way chains (GreatSword_CheckInput); "_TURN" variants
 * first turn 45 degrees toward the left stick. input.pressed PAD_R3 strikes. */
enum GreatSwordState {
    GSWORD_SWEEP_LEFT = 0x00,                       /* the left sweep (hit object 2) */
    GSWORD_SWEEP_RIGHT = 0x01,                      /* the right sweep (hit object 1); a ground impact when it hits nothing */
    GSWORD_OVERHEAD = 0x02,                         /* the overhead slash, after a wind-up (hit object 3) */
    GSWORD_BOUNCE = 0x03,                           /* the attack bounced off (hitKind 1); a flick queues GSWORD_BOUNCE_OVERHEAD */
    GSWORD_STRIKE = 0x04,                           /* input.pressed PAD_R3: one strike (hit object 4) */
    GSWORD_SWEEP_LEFT_AFTER_OVERHEAD = 0x05,        /* after the overhead or the charged slash */
    GSWORD_SWEEP_LEFT_AFTER_RIGHT = 0x06,           /* after the right sweep */
    GSWORD_SWEEP_LEFT_AFTER_OVERHEAD_TURN = 0x07,
    GSWORD_SWEEP_LEFT_AFTER_RIGHT_TURN = 0x08,
    GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD = 0x09,
    GSWORD_SWEEP_RIGHT_AFTER_LEFT = 0x0A,
    GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD_TURN = 0x0B,
    GSWORD_SWEEP_RIGHT_AFTER_LEFT_TURN = 0x0C,
    GSWORD_DRAW_OVERHEAD = 0x0D,                    /* the attack that draws on the move (GreatSword_DrawAttack) */
    GSWORD_OVERHEAD_AFTER_RIGHT = 0x0E,
    GSWORD_OVERHEAD_AFTER_LEFT = 0x0F,
    GSWORD_BOUNCE_OVERHEAD = 0x10,                  /* the overhead from the bounce */
    GSWORD_OVERHEAD_AFTER_LEFT_TURN = 0x11,
    GSWORD_OVERHEAD_AFTER_RIGHT_TURN = 0x12,
    GSWORD_CHARGE = 0x13,                           /* charges while the stick is held down (90 frames at most; weaponTimer), then the charged slash */
    GSWORD_DRAW_CHARGE = 0x14,                      /* draws on the move into the charge (the stick held down) */
    GSWORD_CHARGE_AFTER_RIGHT = 0x15,
    GSWORD_CHARGE_AFTER_LEFT = 0x16,
    GSWORD_CHARGE_NO_LEAD_IN = 0x17,                /* no motion set at its start (no entry seen) */
    GSWORD_CHARGE_AFTER_LEFT_TURN = 0x18,
    GSWORD_CHARGE_AFTER_RIGHT_TURN = 0x19,
    GSWORD_CHARGE_SLASH = 0x1A,                     /* the charged slash: chargeLevel 0-3 (GreatSword_SetChargeAttack) */
    GSWORD_CHARGE_SLASH_AFTER_LEFT = 0x1B,          /* the release of GSWORD_CHARGE_AFTER_LEFT */
    GSWORD_CHARGE_SLASH_AFTER_LEFT_TURN = 0x1C      /* the release of GSWORD_CHARGE_AFTER_LEFT_TURN */
};

/* The bowguns' states, heavy and light (Bowgun_RunState, weapon_states.c).
 * input.pressed PAD_R3 fires, a flick of the attack stick down
 * (input.stickDirPressed PAD_RSTICK_DOWN) reloads and up (PAD_RSTICK_UP) is the melee attack. */
enum BowgunState {
    BOWGUN_FIRE_RECOIL_0 = 0x00,       /* fires with the lightest recoil (Bowgun_RecoilLevel 0) */
    BOWGUN_RELOAD = 0x01,              /* reloads the clip (roundsLoaded) at a speed by Bowgun_ReloadLevel */
    BOWGUN_DRY_FIRE = 0x02,            /* the fire button with nothing loaded */
    BOWGUN_MELEE = 0x03,               /* a melee blow with the gun (hit object 29), outside the aim mode */
    BOWGUN_FIRE_RECOIL_1 = 0x04,       /* fires with recoil level 1 */
    BOWGUN_FIRE_RECOIL_2 = 0x05,       /* fires with recoil level 2 */
    BOWGUN_DRAW_RELOAD = 0x06,         /* draws, then reloads (the stick held down at the draw: Bowgun_DrawAttack) */
    BOWGUN_DRAW_RELOAD_MOVING = 0x07,  /* the same on the move (light bowgun) */
    BOWGUN_RAPID_FIRE = 0x08           /* the light bowgun's rapid fire (Bowgun_CanRapidFire): several shots at half power */
};

/* The hammer's states (Hammer_RunState, weapon_states.c). A flick of the attack
 * stick attacks (down HAMMER_DOWN_ATTACK, any other way the combo);
 * input.held PAD_R1 charges (Player_HoldSpecialInput: Hammer_StartCharge) and
 * letting go releases by Hammer_ChargeLevel, standing or moving. */
enum HammerState {
    HAMMER_DOWN_ATTACK = 0x00,              /* the down flick's swing (hit object 6) */
    HAMMER_COMBO_1 = 0x01,                  /* the combo's first swing (hit object 7) */
    HAMMER_COMBO_2 = 0x02,                  /* the combo's second swing (hit object 33) */
    HAMMER_CHARGE = 0x03,                   /* charges, standing (weaponTimer counts, stamina drains) */
    HAMMER_CHARGE_WALK = 0x04,              /* charges while walking */
    HAMMER_CHARGE_SPIN = 0x05,              /* the moving release at level 2: spins chargeLevel + 2 turns */
    HAMMER_CHARGE_RELEASE_1 = 0x06,         /* the standing release at level 1: two hits */
    HAMMER_CHARGE_RELEASE_2 = 0x07,         /* the standing release at level 2 (hit objects 10, 11) */
    HAMMER_DRAW_ATTACK = 0x08,              /* the attack that draws on the move (Hammer_DrawAttack) */
    HAMMER_SPIN_FINISH = 0x09,              /* ends the spin with a flick up (hit object 32) */
    HAMMER_BOUNCE = 0x0A,                   /* the attack bounced off; a flick queues HAMMER_BOUNCE_SWING */
    HAMMER_COMBO_1_AFTER_DOWN = 0x0B,       /* the combo's first swing chained from HAMMER_DOWN_ATTACK */
    HAMMER_CHARGE_HOLD = 0x0C,              /* charges again standing still after HAMMER_CHARGE_WALK (keeps the charge time) */
    HAMMER_CHARGE_AFTER_ATTACK = 0x0D,      /* charges from an attack's window */
    HAMMER_CHARGE_RESUME = 0x0E,            /* charges again after a release */
    HAMMER_CHARGE_RELEASE_0 = 0x0F,         /* the standing release at level 0 */
    HAMMER_CHARGE_RELEASE_0_MOVING = 0x10,  /* the moving release at level 0 */
    HAMMER_CHARGE_RELEASE_1_MOVING = 0x11,  /* the moving release at level 1 (also a flick up in the spin's first turns) */
    HAMMER_BOUNCE_SWING = 0x12,             /* the swing from the bounce */
    HAMMER_COMBO_3 = 0x13                   /* the combo's third swing (hit object 32, as HAMMER_SPIN_FINISH) */
};

/* The lance's states (Lance_RunState, weapon_states.c). A flick of the attack
 * stick thrusts (down an upper thrust, any other way a mid thrust; they chain
 * three times); input.pressed PAD_R3 charges; from the guard (Player_GuardInput:
 * Lance_GuardAttack) the flick thrusts and pressed PAD_R3 charges. */
enum LanceState {
    LANCE_THRUST = 0x00,           /* the first thrust (Lance_AttackInput; a hop's end, Player_StateLanceHop) */
    LANCE_UPPER_THRUST = 0x01,     /* likely: the first thrust of the down flick */
    LANCE_GUARD_THRUST = 0x02,     /* thrusts from the guard, back to PL_REACT_GUARD_HOLD */
    LANCE_CHARGE = 0x03,           /* charges forward (stamina drains; the left stick steers) */
    LANCE_CHARGE_STOP = 0x04,      /* stops the charge (pressed PAD_R3 again, the stick down, or no stamina) */
    LANCE_CHARGE_HIT = 0x05,       /* the charge hit something at sharpness level 2; it goes on */
    LANCE_CHARGE_FINISH = 0x06,    /* the charge's finishing thrust (the stick up) */
    LANCE_BOUNCE = 0x07,           /* the attack bounced off; a flick queues LANCE_BOUNCE_THRUST */
    LANCE_DRAW_THRUST = 0x08,      /* the attack that draws on the move (Lance_DrawAttack) */
    LANCE_THRUST_2 = 0x09,
    LANCE_THRUST_3 = 0x0A,         /* ends the chain */
    LANCE_BOUNCE_THRUST = 0x0B,    /* the thrust from the bounce */
    LANCE_UPPER_THRUST_2 = 0x0C,
    LANCE_UPPER_THRUST_3 = 0x0D,   /* ends the chain */
    LANCE_CHARGE_RESUME = 0x0E,    /* the charge going on after LANCE_CHARGE_HIT */
    LANCE_CHARGE_HIT_STOP = 0x0F,  /* the charge hit something at sharpness level 1: it stops */
    LANCE_CHARGE_WALL = 0x10       /* the charge ran into a wall (wallHitDirs): it stops */
};

/* The sword and shield's states (SwordShield_RunState, weapon_states.c). A flick
 * of the attack stick attacks: up the combo, left or right the side slash,
 * down the jump slash; input.pressed PAD_R3 the shield bash. */
enum SwordShieldState {
    SNS_COMBO_1 = 0x00,             /* the combo's first slash (the flick up) */
    SNS_JUMP_SLASH = 0x01,          /* likely: the flick down (the draw attack's motion) */
    SNS_SIDE_SLASH = 0x02,          /* the flick left or right; no attack chains from it */
    SNS_GUARD_SLASH = 0x03,         /* slashes from the guard (SwordShield_GuardSlash), back to the guard */
    SNS_RISING_SLASH = 0x04,        /* likely: the flick up after the jump slash */
    SNS_BOUNCE = 0x05,              /* the attack bounced off; a flick queues SNS_BOUNCE_JUMP_SLASH */
    SNS_COMBO_2 = 0x06,
    SNS_COMBO_3 = 0x07,             /* ends the combo */
    SNS_COMBO_1_LINKED = 0x08,      /* the combo's first slash chained from a rising slash or the bash */
    SNS_DRAW_JUMP_SLASH = 0x09,     /* the attack that draws on the move (SwordShield_DrawAttack) */
    SNS_BOUNCE_JUMP_SLASH = 0x0A,   /* the jump slash from the bounce */
    SNS_EVADE_RISING_SLASH = 0x0B,  /* the rising slash from a roll or an evade (SwordShield_EvadeAttack) */
    SNS_SHIELD_BASH = 0x0C          /* likely: input.pressed PAD_R3 (hit object 100), then the rising slash */
};

/* The dual swords' states (DualSwords_RunState, weapon_states.c). A flick of the
 * attack stick attacks: up the combo, left or right the side slashes, down
 * the jump slash (DualSwords_ComboInput); input.held PAD_R1 toggles the demon
 * mode (Player.demonMode: DualSwords_DemonToggle), in which the side slashes
 * and the down flick become the demon attacks. */
enum DualSwordsState {
    DSWORDS_COMBO_1 = 0x00,                  /* the combo's first slash (the flick up) */
    DSWORDS_SIDE_SLASH_RIGHT = 0x01,         /* three hits */
    DSWORDS_JUMP_SLASH = 0x02,               /* likely: the flick down */
    DSWORDS_RISING_SLASH = 0x03,             /* likely: the flick up after the jump slash */
    DSWORDS_DEMON_START = 0x04,              /* enters the demon mode */
    DSWORDS_DEMON_SIDE_RIGHT = 0x05,         /* the demon mode's side slashes, two parts, six hits */
    DSWORDS_DEMON_DANCE = 0x06,              /* likely: the demon mode's down flick, ten hits */
    DSWORDS_DANCE_STOP = 0x07,               /* the dance cut short (sharpness below level 2) */
    DSWORDS_BOUNCE = 0x08,                   /* the attack bounced off; a flick queues DSWORDS_BOUNCE_JUMP_SLASH */
    DSWORDS_COMBO_2 = 0x09,
    DSWORDS_COMBO_3 = 0x0A,                  /* ends the combo's flicks up */
    DSWORDS_COMBO_1_LINKED = 0x0B,           /* the combo's first slash chained from a rising slash */
    DSWORDS_SIDE_SLASH_LEFT = 0x0C,
    DSWORDS_SIDE_SLASH_RIGHT_LINKED = 0x0D,  /* chained from the combo */
    DSWORDS_SIDE_SLASH_LEFT_LINKED = 0x0E,   /* chained from the combo */
    DSWORDS_DRAW_JUMP_SLASH = 0x0F,          /* the attack that draws on the move (DualSwords_DrawAttack) */
    DSWORDS_BOUNCE_JUMP_SLASH = 0x10,        /* the jump slash from the bounce */
    DSWORDS_EVADE_RISING_SLASH = 0x11,       /* the rising slash from a roll or an evade (DualSwords_EvadeAttack) */
    DSWORDS_DEMON_END = 0x12,                /* leaves the demon mode */
    DSWORDS_DEMON_START_LINKED = 0x13,       /* enters the demon mode from an attack's window (input.pressed PAD_R1) */
    DSWORDS_DEMON_SIDE_LEFT = 0x14
};

/* The long sword's states (LongSword_RunState, weapon_states.c). The attacks
 * chain by a combo row and a key (LongSword_ComboInput,
 * LongSword_ReadAttackKey): the attack stick flicked to its upper half
 * slashes, to its lower half the fade slash; input.pressed PAD_R3 thrusts;
 * input.pressed PAD_R1 is the spirit combo, which spends the spirit gauge
 * (Player.swordGauge) and has an "_EMPTY" form when the gauge is short. */
enum LongSwordState {
    LSWORD_SLASH = 0x00,                        /* the first slash */
    LSWORD_SLASH_2 = 0x01,                      /* the second slash */
    LSWORD_FADE_SLASH = 0x02,                   /* likely a backward slash (the lower-half flick); only an evade follows */
    LSWORD_BOUNCE = 0x03,                       /* the attack bounced off; a flick queues LSWORD_BOUNCE_SLASH */
    LSWORD_THRUST = 0x04,                       /* input.pressed PAD_R3 */
    LSWORD_FADE_AFTER_SLASH = 0x05,             /* after the slash or the third spirit slash */
    LSWORD_FADE_AFTER_SLASH_2 = 0x06,
    LSWORD_SPIRIT_1 = 0x07,                     /* the first spirit slash (gauge 15) */
    LSWORD_SPIRIT_THRUST = 0x08,                /* the thrust after the first spirit slash */
    LSWORD_SPIRIT_2 = 0x09,                     /* the second spirit slash (gauge 20) */
    LSWORD_SPIRIT_RISING = 0x0A,                /* the rising slash after the second spirit slash */
    LSWORD_SLASH_ALT = 0x0B,                    /* the slash after a wind-up motion (no entry seen) */
    LSWORD_STEP_SLASH = 0x0C,                   /* the slash with the left stick held: steps forward */
    LSWORD_STEP_SLASH_ALT = 0x0D,               /* (no entry seen) */
    LSWORD_RISING_SLASH = 0x0E,                 /* likely a rising slash (no entry seen) */
    LSWORD_SPIRIT_3 = 0x0F,                     /* the third spirit slash (gauge 25), three hits */
    LSWORD_EXTRA_SLASH = 0x10,                  /* a slash whose only follow-up is the fade slash (no entry seen) */
    LSWORD_BOUNCE_SLASH = 0x11,                 /* the slash from the bounce */
    LSWORD_SPIRIT_1_EMPTY = 0x12,               /* the first spirit slash with the gauge below 15: no cost, can bounce */
    LSWORD_DRAW_SLASH = 0x13,                   /* the attack that draws on the move (LongSword_DrawAttack) */
    LSWORD_RISING_AFTER_THRUST = 0x14,
    LSWORD_FADE_AFTER_THRUST = 0x15,            /* after the thrust or the spirit thrust */
    LSWORD_FADE_AFTER_RISING = 0x16,            /* after a rising slash */
    LSWORD_SPIRIT_3_AFTER_RISING = 0x17,
    LSWORD_SLASH_2_AFTER_RISING = 0x18,
    LSWORD_FADE_AFTER_SPIRIT_1 = 0x19,
    LSWORD_FADE_AFTER_SPIRIT_2 = 0x1A,
    LSWORD_SPIRIT_2_AFTER_THRUST = 0x1B,        /* after the spirit thrust */
    LSWORD_SPIRIT_1_AFTER_RISING = 0x1C,        /* gauge 20 */
    LSWORD_SPIRIT_1_AFTER_RISING_EMPTY = 0x1D,
    LSWORD_DRAW_SPIRIT_1 = 0x1E,                /* input.held PAD_R1 while sheathed: draws into the first spirit slash (LongSword_SpiritDraw; gauge 20) */
    LSWORD_DRAW_SPIRIT_1_EMPTY = 0x1F
};

/* The hunting horn's states (HuntingHorn_RunState). A flick of the attack stick swings (left, right, down; up
 * the slam). The horn is played in four stances, named for the flick that
 * leads into each: a stance loops one motion and adds its note (up note 1,
 * right and left note 0, down note 2) every 60 frames (HuntingHorn_AddNote);
 * the last four notes make the songs (HuntingHorn_CheckSongs). A flick in a
 * stance is a stance attack, which ends in the stance of its direction;
 * input.pressed PAD_R1 leaves the stance (HORN_PLAY_END) or enters it from a
 * swing's window. 8 has no case. */
enum HuntingHornState {
    HORN_STANCE_UP_BLEND = 0x00,          /* the stance entered with a blend (no entry seen) */
    HORN_STANCE_RIGHT_BLEND = 0x01,       /* (no entry seen) */
    HORN_STANCE_LEFT_BLEND = 0x02,        /* (no entry seen) */
    HORN_STANCE_DOWN_BLEND = 0x03,        /* (no entry seen) */
    HORN_STANCE_UP = 0x04,                /* note 1 */
    HORN_STANCE_RIGHT = 0x05,             /* note 0; also after HORN_PLAY_START */
    HORN_STANCE_LEFT = 0x06,              /* note 0 */
    HORN_STANCE_DOWN = 0x07,              /* note 2 */
    HORN_SWING_LEFT = 0x09,
    HORN_SWING_RIGHT = 0x0A,
    HORN_SWING_DOWN = 0x0B,
    HORN_SWING_DOWN_2 = 0x0C,
    HORN_SWING_LEFT_LATE = 0x0D,          /* starts late in the motion (no entry seen) */
    HORN_PLAY_START = 0x0E,               /* input.held PAD_R1 with the horn out (HuntingHorn_StartPlaying): raises it into HORN_STANCE_RIGHT */
    HORN_NOTE0_ATTACK_UP = 0x0F,          /* a stance attack from a note-0 stance (right or left) */
    HORN_NOTE0_ATTACK_RIGHT = 0x10,
    HORN_NOTE0_ATTACK_LEFT = 0x11,
    HORN_NOTE0_ATTACK_DOWN = 0x12,
    HORN_PLAY_END = 0x13,                 /* leaves a note-0 stance with a swing */
    HORN_SLAM = 0x14,                     /* the flick up: a two-hit slam that shakes the camera */
    HORN_SWING_LEFT_SLOW = 0x15,          /* at 0.77 speed (no entry seen) */
    HORN_STANCE_DOWN_AFTER_LEFT = 0x16,   /* enters HORN_STANCE_DOWN from the left swing */
    HORN_NOTE1_ATTACK_UP = 0x17,          /* a stance attack from HORN_STANCE_UP */
    HORN_BOUNCE = 0x18,                   /* the attack bounced off; a flick queues HORN_BOUNCE_SWING */
    HORN_DRAW_ATTACK = 0x19,              /* the attack that draws on the move (HuntingHorn_DrawAttack) */
    HORN_PLAY_END_NOTE1 = 0x1A,           /* leaves HORN_STANCE_UP with a swing */
    HORN_PLAY_END_NOTE2 = 0x1B,           /* leaves HORN_STANCE_DOWN with a swing */
    HORN_NOTE1_ATTACK_RIGHT = 0x1C,
    HORN_NOTE1_ATTACK_LEFT = 0x1D,
    HORN_NOTE1_ATTACK_DOWN = 0x1E,
    HORN_NOTE2_ATTACK_UP = 0x1F,          /* a stance attack from HORN_STANCE_DOWN */
    HORN_NOTE2_ATTACK_RIGHT = 0x20,
    HORN_NOTE2_ATTACK_LEFT = 0x21,
    HORN_NOTE2_ATTACK_DOWN = 0x22,
    HORN_SWING_DOWN_3 = 0x23,             /* ends the down swings; only an evade follows */
    HORN_STANCE_DOWN_AFTER_RIGHT = 0x24,  /* enters HORN_STANCE_DOWN from the right swing */
    HORN_BOUNCE_SWING = 0x25,             /* the left swing from the bounce */
    HORN_DRAW_PLAY_START = 0x26,          /* input.held PAD_R1 while sheathed: draws, then HORN_PLAY_START */
    HORN_STANCE_DOWN_AFTER_DOWN = 0x27    /* enters HORN_STANCE_DOWN from a down swing */
};
/* The gunlance's states (Gunlance_RunState). A flick of the
 * attack stick (input.stickDirPressed PL_RSTICK_ANY) thrusts: down an upper thrust,
 * any other way a thrust, with the left stick pushed far a lunge; the
 * thrusts chain in their windows (Gunlance_CheckInput). input.pressed PAD_R3
 * shells. From the guard (Player_GuardInput: Gunlance_GuardInput) the flick
 * up thrusts, down reloads, and pressed PAD_R3 is the wyvern fire. */
enum GunlanceState {
    GLANCE_THRUST = 0x00,                  /* the first thrust (Gunlance_AttackInput; a forward hop) */
    GLANCE_SHELL_AFTER_THRUST = 0x01,      /* shells in a thrust's window */
    GLANCE_GUARD_THRUST = 0x02,            /* thrusts from the guard (Gunlance_GuardInput), back to the guard */
    GLANCE_WYVERN_FIRE = 0x03,             /* likely: a 50-frame charge, then a blast with a 1350-frame cooldown (weaponTimer) */
    GLANCE_UPPER_THRUST = 0x04,            /* likely: a flick down from idle, or a back hop with a full clip */
    GLANCE_RELOAD = 0x05,                  /* reloads from the guard */
    GLANCE_LUNGE = 0x06,                   /* the left stick pushed far: moves forward while thrusting */
    GLANCE_BOUNCE = 0x07,                  /* the attack bounced off (Gunlance_CheckBounce) */
    GLANCE_DRAW_THRUST = 0x08,             /* draws, then thrusts (no entry seen) */
    GLANCE_THRUST_2 = 0x09,                /* the second thrust (a forward chain) */
    GLANCE_THRUST_3 = 0x0A,                /* the third thrust, its own attack */
    GLANCE_BOUNCE_THRUST = 0x0B,           /* a thrust from the bounce */
    GLANCE_SHELL_AFTER_GUARD_THRUST = 0x0C, /* shells after a guard thrust or a lunge */
    GLANCE_SHELL_AFTER_UPPER = 0x0D,       /* shells after an upper thrust */
    GLANCE_SHELL_EMPTY = 0x0E,             /* the shell button with no shell loaded or no sharpness */
    GLANCE_GUARD_THRUST_2 = 0x0F,          /* the second guard thrust */
    GLANCE_GUARD_THRUST_3 = 0x10,          /* the third guard thrust */
    GLANCE_UPPER_THRUST_2 = 0x11,          /* an upper thrust chained from a thrust */
    GLANCE_UPPER_THRUST_3 = 0x12,          /* an upper thrust chained from a second thrust */
    GLANCE_WYVERN_FIRE_COOLING = 0x13,     /* likely: the wyvern fire input during its cooldown: nothing fires */
    GLANCE_DRAW_LUNGE = 0x14,              /* the attack that draws on the move (Gunlance_DrawAttack) */
    GLANCE_UPPER_TO_GUARD_THRUST = 0x15,   /* a guard thrust chained from an upper thrust */
    GLANCE_UPPER_TO_GUARD_THRUST_2 = 0x16, /* a guard thrust chained from a second upper thrust */
    GLANCE_SHELL = 0x17,                   /* shells from idle */
    GLANCE_SHELL_REPEAT = 0x18,            /* shells again after a shell */
    GLANCE_SHELL_REPEAT_GUARD = 0x19,      /* shells again after GLANCE_SHELL_AFTER_GUARD_THRUST */
    GLANCE_SHELL_REPEAT_UPPER = 0x1A,      /* shells again after GLANCE_SHELL_AFTER_UPPER */
    GLANCE_HOP_RELOAD = 0x1B               /* reloads after a back hop (Player_StateLanceHop, the clip not full) */
};

/* The bow's states (Bow_RunState). Its inputs: input.held
 * PAD_R1 aims (releasing it ends the aim), the attack stick held down
 * (input.stickDir PAD_RSTICK_DOWN) charges
 * and releasing it shoots, a flick up (input.stickDirPressed PAD_RSTICK_UP) stabs, input.pressed
 * PAD_CROSS evades and input.pressed PAD_R3 loads a coating. */
enum BowState {
    BOW_FIRE = 0x00,               /* shoots a charge of level 1 or more (Bow_ChargeLevel), then BOW_AIM_RESUME */
    BOW_CHARGE = 0x01,             /* charges from the aim (weaponTimer counts the charge) */
    BOW_TO_IDLE = 0x02,            /* one motion, then idle (no entry seen) */
    BOW_STAB = 0x03,               /* the melee stab with an arrow */
    BOW_STAB_2 = 0x04,             /* the second stab */
    BOW_AIM = 0x05,                /* the aim stance, entered by holding the aim button */
    BOW_CHARGE_WALK = 0x06,        /* charges while walking */
    BOW_AIM_RESUME = 0x07,         /* back to the aim stance after a shot, or after PL_ACT_BOW_WALK */
    BOW_EVADE = 0x08,              /* the evade, with no stick (stamina 75) */
    BOW_CHARGE_RESUME = 0x09,      /* the charge again from BOW_CHARGE_WALK, keeping its time */
    BOW_EVADE_2 = 0x0A,            /* an evade chained from an evade */
    BOW_CHARGE_FROM_IDLE = 0x0B,   /* charges from the drawn idle */
    BOW_DRAW_AIM = 0x0C,           /* draws into the aim (the aim button while sheathed: Bow_DrawToAim) */
    BOW_DRAW_STAB = 0x0D,          /* draws on the move into a stab (Bow_DrawAttack) */
    BOW_FIRE_QUICK = 0x0E,         /* shoots a charge of level 0 */
    BOW_CHARGE_AFTER_SHOT = 0x0F,  /* charges again in a shot's window */
    BOW_BOUNCE = 0x10,             /* the stab bounced off (Bow_CheckBounce) */
    BOW_LOAD_COATING = 0x11,       /* loads the coating of the ammo slot (Player.coatingSlot) */
    BOW_STAB_EVADE = 0x12,         /* an evade in a stab's window */
    BOW_CHARGE_AFTER_STAB = 0x13,  /* charges in a stab's window */
    BOW_DRAW_CHARGE = 0x14         /* draws on the move into a charge (Bow_DrawAttack, the charge button held) */
};

/* Group PL_GROUP_ACTION: the field actions, run by Player_RunActionState.
 * "Drawn": with the weapon out (Player.weaponOut). Several ids share a
 * handler, which the id's variant steers. */
enum PlayerActionState {
    PL_ACT_IDLE = 0x00,               /* standing, the weapon sheathed or drawn */
    PL_ACT_RUN = 0x01,                /* runs, sheathed */
    PL_ACT_WALK = 0x02,               /* walks, sheathed (seen: no entry; PL_ACT_WALK_SLOW is the walk used) */
    PL_ACT_WALK_DRAWN = 0x03,         /* walks with the weapon drawn */
    PL_ACT_DRAW = 0x04,               /* draws the weapon */
    PL_ACT_SHEATHE = 0x05,            /* sheathes it, standing */
    PL_ACT_LEAP_OFF_LEDGE = 0x06,     /* leaps off a ledge while dashing */
    PL_ACT_GUARD_WALK = 0x07,         /* walks while guarding (lance, gunlance) */
    PL_ACT_CROUCH = 0x08,             /* crouches down */
    PL_ACT_FALL = 0x09,               /* falls (Player_Update: high above the floor) */
    PL_ACT_SHEATHE_MOVING = 0x0A,     /* sheathes on the move */
    PL_ACT_THROW_ITEM_ALT = 0x0B,     /* throws an item, the second motion (items ITEM_ID_THROWN_GROUND_MARK_B to ITEM_ID_THROWN_GROUND_BURST_C standing) */
    PL_ACT_BED_MENU = 0x0C,           /* the menu of an action spot of kind 5 (the bed: sleep, or the quest's start and abort) */
    PL_ACT_TO_IDLE = 0x0D,            /* returns to idle at once (no entry seen) */
    PL_ACT_THROW_ITEM = 0x0E,         /* throws a pouch item */
    PL_ACT_SHEATHE_TO_CROUCH = 0x0F,  /* sheathes, then crouches */
    PL_ACT_SHEATHE_TO_USE_ITEM = 0x10, /* sheathes, then uses the item */
    PL_ACT_LEDGE_HANG = 0x11,         /* hangs from a ledge */
    PL_ACT_RIDE_WALK = 0x12,          /* walks on a monster's back (Player.rideState) */
    PL_ACT_MOVE_KEEP_MOTION = 0x13,   /* moves without changing the motion (no entry seen) */
    PL_ACT_PLACE_ITEM_CROUCHED = 0x14, /* places an item while crouched */
    PL_ACT_JUMP_TO_LEDGE = 0x15,      /* jumps up toward a high ledge (Player_CheckClimb) */
    PL_ACT_RIDE_STAGGER = 0x16,       /* staggers on a moving monster (Player_CheckRideSlip) */
    PL_ACT_DRAW_MOVING = 0x17,        /* draws on the move (the light bowgun) */
    PL_ACT_PLACE_ITEM = 0x18,         /* sets an item down (a barrel bomb), then stands */
    PL_ACT_KNEEL_TO_PLACE = 0x19,     /* kneels before placing it */
    PL_ACT_CROUCH_WALK_ALT = 0x1A,    /* the crouched walk's motion (no entry seen) */
    PL_ACT_LEDGE_GRAB = 0x1B,         /* grabs the ledge ahead and hangs */
    PL_ACT_ROLL = 0x1C,               /* the forward roll: evades, costs stamina */
    PL_ACT_CROUCH_HOLD = 0x1D,        /* stays crouched */
    PL_ACT_STEP_UP = 0x1E,            /* climbs a low step */
    PL_ACT_DASH = 0x1F,               /* sprints (the dash button, sheathed): costs stamina */
    PL_ACT_FLEE_DASH = 0x20,          /* sprints away from the monster it fights */
    PL_ACT_OUT_OF_BREATH = 0x21,      /* stamina used up: catches its breath */
    PL_ACT_DASH_TIRED = 0x22,         /* the tired sprint (stamina below 76) */
    PL_ACT_RUN_AFTER_DASH = 0x24,     /* runs on after a dash (dashDelay) */
    PL_ACT_TIRED_STOP = 0x25,         /* seen: the tired sprint's motion once, then idle (no entry seen) */
    PL_ACT_LAND_HARD = 0x26,          /* lands from a long fall */
    PL_ACT_WALL_HOLD = 0x27,          /* holds still on a climbable wall */
    PL_ACT_WALL_CLIMB_UP = 0x28,
    PL_ACT_WALL_CLIMB_DOWN = 0x29,
    PL_ACT_WALL_DISMOUNT = 0x2A,      /* steps off the wall at its foot */
    PL_ACT_WALL_GRAB = 0x2B,          /* starts climbing a wall */
    PL_ACT_RUN_STOP = 0x2C,           /* skids to a stop */
    PL_ACT_ROLL_FROM_CROUCH = 0x2D,
    PL_ACT_SET_TRAP = 0x2E,           /* kneels and sets a trap or a device */
    PL_ACT_SET_TRAP_DONE = 0x2F,      /* finishes setting it and stands */
    PL_ACT_WALL_LET_GO = 0x30,        /* drops off the wall (the roll button, or no stamina) */
    PL_ACT_EVADE_RIGHT = 0x31,        /* the sidestep evasion with the weapon drawn, to the right */
    PL_ACT_EVADE_LEFT = 0x32,         /* to the left */
    PL_ACT_BED_LIE_DOWN = 0x33,       /* lies down on the bed (the bed's menu) */
    PL_ACT_BED_SLEEP = 0x34,          /* sleeps: health recovers */
    PL_ACT_BED_GET_UP = 0x35,
    PL_ACT_MOUNTED_GUN_AIM = 0x36,    /* mans the mounted gun at an action spot of kind 6 (item ITEM_ID_MOUNTED_GUN_AMMO its ammo) */
    PL_ACT_CANNON_FIRE = 0x37,        /* fires the carried item ITEM_ID_CANNON_AMMO at an action spot of kind 8 (likely a cannon) */
    PL_ACT_LEDGE_DROP = 0x38,         /* lets go of the ledge */
    PL_ACT_CROUCH_STAND_UP = 0x39,    /* stands up from the crouch */
    PL_ACT_LEDGE_PULL_UP = 0x3A,      /* climbs up from the ledge hang */
    PL_ACT_CROUCH_WALK = 0x3B,        /* walks crouched (unk668 forces it) */
    PL_ACT_CROUCH_STOP = 0x3C,        /* stops walking crouched */
    PL_ACT_CROUCH_WALK_FAST = 0x3D,
    PL_ACT_BATTLE_ALERT = 0x3E,       /* reacts as a battle starts, sheathed (Player_EnteredBattle) */
    PL_ACT_TURN_AROUND = 0x3F,        /* the quick half turn at a run */
    PL_ACT_WALL_PRESS = 0x40,         /* back to a wall (Player_UpdateWallSidle) */
    PL_ACT_WALL_SIDLE_NEG = 0x41,     /* sidles along it, the stick a quarter turn one way */
    PL_ACT_WALL_SIDLE_POS = 0x42,     /* the other way */
    PL_ACT_WALL_PRESS_HOLD = 0x43,    /* back to the wall again, input at once */
    PL_ACT_DIVE = 0x44,               /* dives forward from the flee sprint: evades, costs stamina */
    PL_ACT_FLEE_DASH_RESUME = 0x45,   /* the flee sprint after a dive */
    PL_ACT_DASH_RESUME = 0x46,        /* the sprint picked up mid-stride */
    PL_ACT_GATHER = 0x47,             /* gathers at a gather point, standing */
    PL_ACT_MOUNTED_GUN_FIRE = 0x48,   /* fires the mounted gun (uses an ITEM_ID_MOUNTED_GUN_AMMO) */
    PL_ACT_WALK_SLOW = 0x49,          /* the slowest walk (the walk the stick gives) */
    PL_ACT_CARVE = 0x4A,              /* carves a monster (a mine point of type 2), standing */
    PL_ACT_SPIT_ROAST_FAIL = 0x4B,    /* the roast ends raw, rare or burnt */
    PL_ACT_SPIT_ROAST_START = 0x4C,   /* sets up the spit roast */
    PL_ACT_SPIT_ROAST_SUCCESS = 0x4D, /* the roast ends done or gourmet */
    PL_ACT_FISH_READY = 0x4E,         /* the fishing stance (no entry seen) */
    PL_ACT_FISH_CAST = 0x4F,          /* casts the line at an action spot of kind 2 */
    PL_ACT_FISH_WAIT = 0x50,          /* waits with the line out */
    PL_ACT_MINE = 0x51,               /* mines with a pickaxe item (gather point kind 3) */
    PL_ACT_BUG_NET = 0x52,            /* catches bugs with a net item (gather point kind 4) */
    PL_ACT_FISH_REEL = 0x53,          /* reels in after a bite */
    PL_ACT_FISH_REEL_EMPTY = 0x54,    /* reels in nothing, the bait lost */
    PL_ACT_SPIT_ROAST = 0x55,         /* turns the meat (cookTimer, cookResult) */
    PL_ACT_START_COUNTDOWN = 0x56,    /* strikes the countdown spot (an action spot of kind 9) */
    PL_ACT_SUPPLY_BOX = 0x57,         /* the supply box's menu is open */
    PL_ACT_DRINK = 0x58,              /* consumes an item, the short motion (likely drinking) */
    PL_ACT_KNEEL_TO_PLACE_CROUCHED = 0x59, /* as PL_ACT_KNEEL_TO_PLACE, from the crouch */
    PL_ACT_EAT = 0x5A,                /* consumes an item, the long motion the eating skills speed up */
    PL_ACT_TRADE = 0x5B,              /* trades with the trader (EM_KIND_TRADER) */
    PL_ACT_TAKE_REWARDS = 0x5C,       /* takes the quest's pending reward items (an action spot of kind 4) */
    PL_ACT_GATHER_CROUCHED = 0x5D,    /* gathers at a gather point from the crouch */
    PL_ACT_HORN_PLAY = 0x5E,          /* plays item ITEM_ID_HORN_PLAY (a horn: a hunting horn user breaks it less) */
    PL_ACT_HORN_END = 0x5F,           /* finishes it; the item may break */
    PL_ACT_CARVE_CROUCHED = 0x60,
    PL_ACT_SHARPEN = 0x61,            /* sharpens the weapon with a whetstone item */
    PL_ACT_TEAM_ITEM_USE = 0x62,      /* uses a team item (ITEM_ID_TEAM_HEAL to ITEM_ID_TEAM_DEFENSE) */
    PL_ACT_TEAM_ITEM_END = 0x63,      /* finishes it: its effect on the team (Item_UseTeamPowder) */
    PL_ACT_FISH_REEL_BAITED = 0x64,   /* reels in nothing, the bait kept */
    PL_ACT_BINOCULARS = 0x65,         /* looks through item ITEM_ID_SCOPE_VIEW (the aim view), from standing */
    PL_ACT_BINOCULARS_CROUCHED = 0x66,
    PL_ACT_OFFER_ITEM = 0x67,         /* holds out an item to another player (offerRequest) */
    PL_ACT_TAKE_ITEM_WAIT = 0x68,     /* waits for the giver to hand it over */
    PL_ACT_OFFER_ITEM_END = 0x69,     /* the giver's hand-over or cancel */
    PL_ACT_TAKE_ITEM = 0x6A,          /* takes the offered item */
    PL_ACT_LANCE_HOP_SIDE_A = 0x6B,   /* the side hop of the lance and the gunlance */
    PL_ACT_LANCE_HOP_SIDE_B = 0x6C,   /* the other side */
    PL_ACT_LANCE_HOP_BACK = 0x6D,     /* the back hop without the stick */
    PL_ACT_LANCE_HOP_BACK_LONG = 0x6E, /* the back hop with the stick held back */
    PL_ACT_PLACE_LARGE_ITEM = 0x6F,   /* crouches to place a large item (a large barrel bomb) */
    PL_ACT_PLACE_LARGE_ITEM_SET = 0x70, /* puts it down */
    PL_ACT_APPLY_ITEM = 0x71,         /* applies the item consumed (Item_Use) */
    PL_ACT_RETURN_TO_CAMP = 0x72,     /* uses item 0x4E: back to the camp area */
    PL_ACT_HORN_ITEM_PLAY = 0x73,     /* plays items ITEM_ID_HORN_ITEM_A to ITEM_ID_HORN_ITEM_LONG_B (as PL_ACT_HORN_PLAY) */
    PL_ACT_BOW_WALK = 0x74,           /* walks with the bow drawn; the stop goes to the bow's state 7 */
    PL_ACT_GATHER_SPECIAL = 0x75,     /* gathers at the special mine point (type 5), standing */
    PL_ACT_GATHER_SPECIAL_CROUCHED = 0x76,
    PL_ACT_WALL_CLIMB_UP_FAST = 0x77, /* climbs up with the dash button: costs stamina */
    PL_ACT_WALL_CLIMB_DOWN_FAST = 0x78,
    PL_ACT_SPOT_ITEM_STRIKE = 0x79,   /* uses item ITEM_ID_SPOT_STRIKE_ITEM at an action spot of kind 0xA: strikes each hostile monster */
    PL_ACT_MOUNTED_GUN_REAIM = 0x7A,  /* aims the mounted gun again after a shot */
    PL_ACT_DELIVERY_BOX = 0x7B,       /* the delivery box's menu is open (an action spot of kind 7) */
    PL_ACT_GATHER_DROP = 0x7C,        /* gathers at a dropped mine point (type 0: EmMine_Drop), standing */
    PL_ACT_GATHER_DROP_CROUCHED = 0x7D
};

/* The town's fishing states, in group PL_GROUP_ACTION. The lobby overlay
 * numbers its states apart from the field's PlayerActionState:
 * TownPc_FishInput picks one of these three when the action button is
 * pressed with the line out, and the town's state switch runs them. */
#define PL_TOWN_FISH_REEL 0x63         /* reels in after a bite (Player.fishTimer running) */
#define PL_TOWN_FISH_REEL_EMPTY 0x64   /* reels in nothing, the bait lost */
#define PL_TOWN_FISH_REEL_BAITED 0x65  /* reels in nothing, the bait kept (Player.baited) */

/* Group PL_GROUP_REACT: the reactions to hits, the guard, the status ailments
 * (Player_ReactToHit and Player_PickStatusState pick them from the hit and the
 * buildups) and the mash status. "Front": the player faced the attacker. */
enum PlayerReactState {
    PL_REACT_KNOCKDOWN_FRONT = 0x00,  /* a heavy hit from the front: falls, gets up */
    PL_REACT_GUARD_STAGGER = 0x01,    /* a guarded hit staggers the guard (great and long sword) */
    PL_REACT_BLOWN_BACK = 0x02,       /* flung backward onto the back (attack kind 2 from the front) */
    PL_REACT_GUARD_START = 0x03,      /* raises the guard (the guard button with the weapon) */
    PL_REACT_ROAR_STRONG = 0x04,      /* ATTACK_ROAR_STRONG (likely the strongest roar: SKILL_ROAR_GUARD_ALL blocks it) */
    PL_REACT_BLOWN_FORWARD = 0x05,    /* flung forward onto the face (attack kind 2 from behind) */
    PL_REACT_KNOCKDOWN_BACK = 0x06,   /* a heavy hit from behind */
    PL_REACT_TOSSED_BACK = 0x07,      /* thrown up and backward (attack kinds 3 and 23 from the front) */
    PL_REACT_GUARD_END = 0x08,        /* lowers the guard */
    PL_REACT_FLINCH_FRONT = 0x09,     /* a light hit from the front: pushed back */
    PL_REACT_GUARD_KNOCKBACK = 0x0A,  /* a heavy guarded hit: a long slide, some damage gets through */
    PL_REACT_GUARD_PUSH = 0x0B,       /* a light guarded hit */
    PL_REACT_GUARD_HOLD = 0x0C,       /* back into the held guard */
    PL_REACT_FLINCH_BACK = 0x0D,      /* a light hit from behind: pushed forward */
    PL_REACT_TOSSED_FORWARD = 0x0E,   /* thrown up and forward (from behind) */
    PL_REACT_ROAR = 0x0F,             /* ATTACK_ROAR (likely a roar: SKILL_ROAR_GUARD_MOST and SKILL_ROAR_GUARD_ALL block it) */
    PL_REACT_WIND = 0x10,             /* attack kind 5 (likely wind pressure: SKILL_WIND_GUARD_ALL blocks it) */
    PL_REACT_FLINCH_CARRYING = 0x11,  /* a flinch that keeps the carried item (no entry seen) */
    PL_REACT_FLINCH_CARRYING2 = 0x12, /* the same */
    PL_REACT_STUN = 0x13,             /* stunned (status 5 full; StunStars_CreatePlayer) */
    PL_REACT_14 = 0x14,               /* seen: motion PL_MOT_ITEM_POSE with the weapon sheathed; no entry seen */
    PL_REACT_STATUS2 = 0x15,          /* status 2: lying down 240 frames; presses shorten it */
    PL_REACT_STATUS4 = 0x16,          /* status 4: down for recoverTimer frames */
    PL_REACT_GUARD_DRAW = 0x17,       /* draws the weapon straight into the guard (lance, sword and shield, gunlance) */
    PL_REACT_STATUS4_HIT = 0x18,      /* hit while in status 4: keeps recoverTimer */
    PL_REACT_WIND_SHORT = 0x19,       /* ATTACK_WIND_WEAK (weaker wind: SKILL_WIND_GUARD_WEAK or SKILL_WIND_GUARD_ALL blocks it) */
    PL_REACT_ROAR_SHORT = 0x1A,       /* ATTACK_ROAR_WEAK (a weaker roar: SKILL_ROAR_GUARD_WEAK blocks it) */
    PL_REACT_MASH_START = 0x1B,       /* enters the mash status (mashLeft 900) */
    PL_REACT_MASH_FREE = 0x1C,        /* breaks free of the mash status */
    PL_REACT_QUAKE = 0x1D,            /* ATTACK_QUAKE (likely a tremor: SKILL_QUAKE_GUARD blocks it), 40 frames */
    PL_REACT_MASH_IDLE = 0x1E,        /* stands in the mash status */
    PL_REACT_MASH_WALK = 0x1F,        /* walks in the mash status */
    PL_REACT_MASH_RUN = 0x20,         /* runs in the mash status (the dash button) */
    PL_REACT_MASH_JUMP_OFF = 0x21,    /* hops off a ledge in the mash status: a long fall breaks free */
    PL_REACT_MASH_LAND_HARD = 0x22,
    PL_REACT_MASH_LAND = 0x23,
    PL_REACT_MASH_FALL = 0x24,        /* falls in the mash status (Player_Update) */
    PL_REACT_QUAKE_LONG = 0x25,       /* attack kind 0x12: as PL_REACT_QUAKE, 80 frames */
    PL_REACT_GUARD_HOLD_ALT = 0x26,   /* as PL_REACT_GUARD_HOLD; differs for the gunlance only (Gunlance_ReturnToGuardAlt) */
    PL_REACT_INHALED = 0x27           /* attack kind 22: pulled toward a monster's bone (em_yamatsukami.c) */
};

/* Group PL_GROUP_FAINT: fainted (Player_Die). */
enum PlayerFaintState {
    PL_FAINT = 0x00,           /* from Player_Die: the faint counted, the move to camp or the versus revive */
    PL_FAINT_FACE_UP = 0x01,   /* after PL_REACT_BLOWN_BACK */
    PL_FAINT_FACE_DOWN = 0x02  /* after PL_REACT_BLOWN_FORWARD */
};

/* Group PL_GROUP_EVENT: the quest's events. The only group a player moving to
 * another area may enter (Player_SetState). */
enum PlayerEventState {
    PL_EVENT_CART = 0x00,        /* the Felyne cart carries the fainted player away */
    PL_EVENT_CART_DROP = 0x01,   /* dropped off at the camp */
    PL_EVENT_QUEST_START = 0x02, /* once per quest, when the quest starts (phase 4) */
    PL_EVENT_QUEST_END = 0x03,   /* the quest is cleared or failed (phases 5, 6) */
    PL_EVENT_REVIVE = 0x04,      /* gets up from a faint (versus) */
    PL_EVENT_LEAVE = 0x05,       /* collapses and vanishes (the quest's end, a player who left) */
    PL_EVENT_ABORT = 0x06        /* the quest is abandoned (phase 7) */
};

/* Group PL_GROUP_CARRY: carrying an item in both arms (Player.carry; an egg). */
enum PlayerCarryState {
    PL_CARRY_IDLE = 0x00,
    PL_CARRY_WALK = 0x01,
    PL_CARRY_RUN = 0x02,          /* the dash button: uses stamina, none left drops the item */
    PL_CARRY_DROP = 0x03,         /* puts the item down: it is lost (the item button) */
    PL_CARRY_DROP_TIRED = 0x04,   /* drops it, out of stamina */
    PL_CARRY_JUMP_OFF = 0x05,     /* hops off a ledge: a long fall drops the item */
    PL_CARRY_FALL = 0x06,         /* falls (Player_Update) */
    PL_CARRY_LAND_HARD = 0x07,
    PL_CARRY_LAND = 0x08,
    PL_CARRY_PICKED_UP = 0x09,    /* stands after picking it up: no input for 30 frames */
    PL_CARRY_DELIVER = 0x0A,      /* hands it in at the delivery spot */
    PL_CARRY_HAND_OVER = 0x0B     /* gives it to EM_KIND_TRADER */
};

/* Group PL_GROUP_GESTURE: the gestures (Player.actRequest picks one from
 * gGestureStates, Player_StartGesture). The code shows only their motions. */
enum PlayerGestureState {
    PL_GESTURE_LIE_DOWN = 0x00,   /* lies down (the fainted pose); the stick gets up */
    PL_GESTURE_GET_UP = 0x01,
    PL_GESTURE_SCOUT = 0x02,      /* faced to an area's point, it sets actDetectTimer (300 frames) */
    PL_GESTURE_03 = 0x03,         /* seen: motion PL_MOT_GESTURE_SIT looped; the stick ends it (PL_GESTURE_03_END) */
    PL_GESTURE_03_END = 0x04,
    PL_GESTURE_05 = 0x05,         /* seen: motion 628 with effects */
    PL_GESTURE_06 = 0x06,         /* seen: motion 632 */
    PL_GESTURE_07 = 0x07,         /* seen: motion 622 */
    PL_GESTURE_08 = 0x08,         /* seen: motion 627 */
    PL_GESTURE_ONESHOT0 = 0x09,   /* the one-shot gestures: a motion, then idle (gGestureOneShots[0]) */
    PL_GESTURE_0A = 0x0A,         /* seen: motion 625 */
    PL_GESTURE_0B = 0x0B,         /* seen: motion 630 */
    PL_GESTURE_0C = 0x0C,         /* seen: motion 626 */
    PL_GESTURE_ONESHOT1 = 0x0D,
    PL_GESTURE_06_ALT = 0x0E,     /* as PL_GESTURE_06 */
    PL_GESTURE_ONESHOT3 = 0x0F,
    PL_GESTURE_10 = 0x10          /* seen: motion 633 */
};

/* A one-shot gesture's row: the field's gGestureOneShots
 * (Player_StateGestureOneShot) and the town's gTownGestureOneShots
 * (TownPc_GestureOneShot), one row per variant. */
typedef struct GestureOneShot {
    s16 motion;         /* 0x0: the motion it plays */
    s16 blend;          /* 0x2: the blend of the return to idle once the motion ends */
} GestureOneShot;

/* The weapons' table rows, which the weapon states read. */

/* A hunting horn stance's entry (HuntingHorn_StanceEntryMotions): the motion
 * and its blend, then a second motion, its blend and its start frame. */
typedef struct HornMotion {
    s16 mot;
    s16 blend;
    s16 mot2;
    s16 blend2;
    s16 start2;
} HornMotion;

/* A span of motion frames (HuntingHorn_Note0WalkFrames and the like). */
typedef struct FrameRange {
    f32 from;
    f32 to;
} FrameRange;

/* A motion's blend frames and start frame (GreatSword_OverheadBlends). */
typedef struct MotionBlend {
    s16 blend;
    s16 start;
} MotionBlend;

/* A bowgun's fire motion and its two aim limits (Bowgun_FireMotions). */
typedef struct GunMotion {
    s16 mot;
    u16 aimA;
    u16 aimB;
} GunMotion;

/* A gun's id and its ammunition bits (Bowgun_RapidFireMask). */
typedef struct GunAmmoMask {
    u16 id;
    u32 mask;
} GunAmmoMask;

/* The hunting horn's attacks (HuntingHorn_StatePlayAttack's variants): the note, the motions and the attack id. */
typedef struct HornAttack {
    s16 note;       /* 0x0: the note played (0-3) */
    s16 unk2;       /* 0x2: 0: no wind-up motion */
    s16 motion;     /* 0x4 */
    s16 unk6;       /* 0x6 */
    s16 motion2;    /* 0x8: after the wind-up */
    s16 unkA;       /* 0xA */
    s16 unkC;       /* 0xC */
    u8 attack;      /* 0xE: PlayerAttack_Create */
    u8 unkF;
} HornAttack;

/* A player's copy of its pad input (36 bytes at Player + 0x36C), filled
 * every frame by Player_CopyPadInput. */
typedef struct PlayerInput {
    u16 held;               /* 0x00 */
    u16 prevHeld;           /* 0x02 */
    u16 pressed;            /* 0x04: held and not held last frame */
    u16 prevPressed;        /* 0x06 */
    u16 held2;              /* 0x08: copies of 0x00-0x06 */
    u16 prevHeld2;          /* 0x0A */
    u16 pressed2;           /* 0x0C */
    u16 prevPressed2;       /* 0x0E */
    u16 changed;            /* 0x10 */
    u16 unk12;
    u16 stickDir;           /* 0x14: the sticks' directions as pad bits (gPadState's dir): the left
                             * stick PAD_DPAD (PAD_UP up, PAD_DOWN down, PAD_LEFT/PAD_RIGHT sideways), the
                             * right stick PL_RSTICK_ANY (PAD_RSTICK_UP up, PAD_RSTICK_DOWN down): a flick of it is the attack input */
    u16 prevStickDir;       /* 0x16 */
    u16 stickDirPressed;    /* 0x18: stickDir and not prevStickDir */
    u16 prevStickDirPressed; /* 0x1A */
    u16 stickAngle;         /* 0x1C: the left stick's angle, ANGLE_TURN a turn (gPadState's degL; the
                             * hunting horn, qnet_sync adds the camera's yaw) */
    u16 rightStickAngle;    /* 0x1E: the right stick's angle (gPadState's degR, in gPadInput's
                             * stickAngle[].right), 0 right, a quarter turn up: the attack stick */
    u16 stickTilt;          /* 0x20: the left stick's tilt (gPadState's magL; PL_STICK_HELD and over counts as held) */
    u16 rightStickTilt;     /* 0x22: the right stick's tilt (gPadState's magR) */
} PlayerInput;

/* A player's position received over the network, applied when the timer
 * runs out (Player.pend, qnet_sync). */
typedef struct PlayerPend {
    s16 timer;
    s16 unk2;
    f32 pos[3];
} PlayerPend;

struct Player; /* declared before PlayerVt, so that its handlers take the same struct (C scopes a tag first seen in a parameter list to it) */

/* The player's handlers (Player.handlers; the layout of EmVt's first
 * entries). The hunters' table (gPlayerHandlerTable) holds Player_NoHandler
 * in the first three and Player_MotionFxStep in the last. */
typedef struct PlayerVt {
    void (*init)(struct Player *p);         /* 0x0: the set-up handler (Player_Init calls it) */
    void (*unk4)(struct Player *p);         /* 0x4: no call seen */
    void (*unk8)(struct Player *p);         /* 0x8: no call seen */
    void (*motionFxStep)(struct Player *p); /* 0xC: the motion's effects step (Player_Init, the town's update) */
} PlayerVt;

/* A player part's record (176 bytes): Player.partRecs[i] points at the 32 of each player in
 * gPlayerRecs (Player_InitPartTable). The weapon and camera code reads m1 as the part's
 * matrix (its translation at m1[12..14]: CamScript_AtPoint). */
typedef struct PlRec {
    f32 m0[16];                 /* 0x00 */
    f32 m1[16];                 /* 0x40 */
    f32 unk80[6];               /* 0x80 */
    f32 scale[3];               /* 0x98 */
    f32 unkA4[3];               /* 0xA4 */
} PlRec;
typedef char PlRec_size_check[(sizeof(PlRec) == 0xB0) ? 1 : -1];

/* The state of a swinging equipment part's bone chain (32 bytes; four of them, one per chain
 * of the effect list: Player.equipFx, the equipment preview's at gEquipPreview + 0x28).
 * EquipFx_ChainUpdate moves the chain's bones with it; FxBlock_Clear clears the four. */
typedef struct EquipFxSwing {
    u8 calls;                   /* 0x00: the updates so far, counted to 2: the chain moves from the third */
    u8 unk01[3];                /* 0x01: cleared with it (FxBlock_Clear) */
    f32 vel[2][3];              /* 0x04: per bone of the chain, its swing velocity (damped by gEquipFxDamps) */
    u8 pad1C[4];
} EquipFxSwing;

/* An entry of the swing chains' lists (gEquipFxListsHead[kind] and
 * gEquipFxListsAltHead[kind], ended by key -1; Effect_RunList): the head
 * armour's kind it is for, and its rows of gEquipFxChains (up to four,
 * -1 for none). */
typedef struct EquipFxList {
    s32 key;
    s16 *ids;
} EquipFxList;
extern EquipFxList *gEquipFxListsAltHead[2];
extern EquipFxList *gEquipFxListsHead[2];
typedef char EquipFxSwing_size_check[(sizeof(EquipFxSwing) == 0x20) ? 1 : -1];

/* The head of the player's work area (Player.work at +0x438, where Em has its EmWork): the state
 * of Player_MotionFxStep, the last of the player's handlers (gPlayerHandlers), which plays the
 * sounds and effects of the hunter's motions (player_motion_fx.c). The lobby's TownPcWork extends it
 * past its 20 bytes. */
typedef struct PlayerWork {
    u8 unk00;
    u8 state;                   /* 0x01: 0 on the first step (which clears teamFxTimer), then 1 */
    u8 unk02[4];
    s16 motion;                 /* 0x06: the layer 0 motion last seen (Player.motionId[0]) */
    u8 unk08;                   /* 0x08: cleared when the motion changes; nothing reads it */
    u8 unk09[9];
    u16 teamFxTimer;            /* 0x12: frames before another team item glow may show (60 after one) */
} PlayerWork;
typedef char PlayerWork_size_check[(sizeof(PlayerWork) == 0x14) ? 1 : -1];

/*
 * A player (2416 bytes); gPlayers holds one per player slot, right after
 * the monsters (gEms), whose first fields have the same layout as Em's. The
 * struct is generated from include/player.spec (tools/gen_struct.py): add
 * fields there.
 */
typedef struct Player {
    u8 active;                  /* 0x000: nonzero while the slot is in use */
    u8 visible;                 /* 0x001: nonzero while drawn: checked with active before the draw calls (DrawCb_CharMake, Hud_DrawPlayerNames); Player_Reset sets 1 (as Em.visible) */
    u8 weaponType;              /* 0x002: the weapon type: 0 great sword, 1 heavy bowgun, 2 hammer, 3 lance, 4 sword and shield, 5 light bowgun, 6 dual swords, 7 long sword, 8 hunting horn, 9 gunlance, 10 bow (21 coating slots) */
    u8 isGunner;                /* 0x003: 1: a gunner (bowgun: ammo, no sharpness) */
    u8 unk004;                  /* 0x004: with step, stateVar0, stateVar1: cleared together (Player_ClearActionState) */
    u8 step;                    /* 0x005: as Em.step: the step within the state (counted up by the states), cleared on every state change (Player_ClearActionState) */
    u8 stateVar0;               /* 0x006: as Em.stateVar0: per-state scratch byte (a counter to 5 in PL_ACT_WALL_HOLD), cleared on every state change */
    u8 stateVar1;               /* 0x007: as Em.stateVar1: per-state scratch byte (a "hit once" flag with hitPending; a countdown in the lobby states) */
    s32 timer;                  /* 0x008: as Em.timer: per-state counter (a "hit once" flag in weapon_states.c, a countdown from 10 in the lobby states) */
    u16 index;                  /* 0x00C: the slot in gPlayers (== gScene.localPlayer, gNetUsers[index]; as Em.index) */
    u16 targetYaw;              /* 0x00E: heading to turn to (as Em.targetYaw): set from rotY when a hit turns the player (Player_ReactToHit) */
    u8 isEm;                    /* 0x010: 0 for a hunter: copied to the effects' isEm (Explosion_Create); the Melynx steal checks it is 0 */
    u8 gender;                  /* 0x011: the hunter gender (from the save data) */
    u8 weaponOut;               /* 0x012: nonzero while the weapon is drawn (set per weapon type by state 0/04; the idle input branches on it) */
    u8 unk013[0x014 - 0x013];
    u8 stateGroup;              /* 0x014: the state group (as Em.stateGroup; Player_RunState switches on it) */
    u8 state;                   /* 0x015: the state within the group (Player_IsState; the group dispatchers switch on it) */
    u8 prevStateGroup;          /* 0x016: the previous stateGroup */
    u8 prevState;               /* 0x017: the previous state */
    u8 hitKind;                 /* 0x018: a hit to react to (SwordShield_CheckHit and the other weapons' copies): 1 a state change, 2 and 3 a flinch */
    u8 hitPending;              /* 0x019: nonzero: hitKind is to be handled */
    u8 unk01A[0x01C - 0x01A];
    u8 roundsLoaded;            /* 0x01C: bowgun: rounds loaded */
    u8 clipSize;                /* 0x01D: bowgun: clip size */
    u8 ownTask;                 /* 0x01E: as Em.ownTask: 0 for the players (Player_InitForQuest); an Em run by another runner (the lobby NPCs) has 1 */
    u8 playerMotions;           /* 0x01F: read when the object is an Em: with ownTask set, nonzero takes the player motion sets of slot index, not the monster ones (Player_SetPartAnim, Player_BeginLayerMotion) */
    f32 rotMtx[16];             /* 0x020: the rotation matrix from rot (Mtx_FromAnglesXYZ, Player_Reset; as Em.rotMtx) */
    f32 worldMtx[4][4];         /* 0x060: as Em.worldMtx (the hunters states turn vectors with it, Vec_Rotate) */
    s32 rot[3];                 /* 0x0A0: rotation angles as integers, ANGLE_TURN per turn; rot[1] the heading (as Em.rotX..rotZ) */
    f32 pos[3];                 /* 0x0AC: position (as Em) */
    f32 scale[3];               /* 0x0B8: set to 1.0 by Player_Reset (as Em.scaleX..scaleZ) */
    u8 unk0C4[0x0D0 - 0x0C4];
    struct PlRec *partRecs[32]; /* 0x0D0: the 32 records of 176 bytes in the 5632-byte block at 0x5DE5D0 (Player_InitPartTable, CamScript_AtPoint) */
    u8 unk150[0x154 - 0x150];
    MotionLayer motion[4];      /* 0x154: motion layers (include/motion.h; as Em.motion): Player_SetLayerMotion starts one, Player_UpdateMotion steps it */
    u16 yawCenter;              /* 0x294: as Em.yawCenter: the heading targetYaw is kept within a quarter turn of (Player_UpdateLean, Player_StateMountedGunAim) */
    u8 unk296[0x29A - 0x296];
    s16 msgTimer;               /* 0x29A: as Em.msgTimer (cleared by Player_StateFishReel) */
    u16 motionId[4];            /* 0x29C: motion layers (Player_SetLayerMotion): the motion id (as Em.motionId) */
    s16 motionFrame[4];         /* 0x2A4: motion layers: the start frame (as Em.motionFrame) */
    s16 motionBlend[4];         /* 0x2AC: motion layers: the blend frames, halved (as Em.motionBlend) */
    u16 unk2B4;                 /* 0x2B4 */
    u8 unk2B6[0x2B8 - 0x2B6];
    u8 unk2B8;                  /* 0x2B8 */
    u8 unk2B9[0x2BC - 0x2B9];
    u8 motionStarted[4];        /* 0x2BC: per motion layer: 0 after Player_SetLayerMotion; Player_Update then begins the layer motion (Player_BeginLayerMotion) and sets it */
    u16 motionLayerCount;       /* 0x2C0: number of motion layers */
    s16 hp;                     /* 0x2C2: health (as Em.hp) */
    u8 unk2C4[0x30C - 0x2C4];
    u16 weaponKind;             /* 0x30C: the weapon kind (EquipItem kind, Equip_GetKind) */
    u8 hairStyle;               /* 0x30E: the hair style (HairMenu_Apply, HunterCard_Apply, Player_SetupFromSave) */
    u8 emModelSlot;             /* 0x30F: read when the object is an Em: Em.modelSlot, the monster motion sets it takes (Player_SetPartAnim, Player_BeginLayerMotion) */
    u8 unk310;                  /* 0x310 */
    s8 unk311;                  /* 0x311 */
    EquipItem equip[7];         /* 0x312: 0, 2-5: armour; 6: the weapon (type 7: a bowgun); 3: the torso */
    u8 armourKind[6];           /* 0x366: per armour piece a kind: sub_main copies it from the lobby record colour[] and the model setup (Look_MapPartId) reads it; main armour rattle sound picks from it */
    PlayerInput input;          /* 0x36C: the pad input (Player_CopyPadInput) */
    u8 action;                  /* 0x390: the posture, PL_POSTURE_* (Player_SetPosture sets it) */
    u8 unk391;                  /* 0x391 */
    u8 unk392;                  /* 0x392 */
    u8 unk393[0x395 - 0x393];
    u8 hitReceived;             /* 0x395: nonzero: a hit taken, to react to (Player_ReactToHit; as Em.hitReceived) */
    u8 flinchPart;              /* 0x396: as Em.flinchPart: set to 0 at a hit (ObjAttach_DamagePlayer) */
    u8 padSwapLR;               /* 0x397: bit 0: replayed pad input swaps the d-pad's left and right (PAD_LEFT, PAD_RIGHT; Pad_ReadButtons) */
    u32 flags;                  /* 0x398: flag bits: Player_SetFlag/ClearFlag/TestFlag without bit 31 (stateFlags with it); the quest's player resets keep 0x100100 or 0x80100 */
    u32 stateFlags;             /* 0x39C: flags (Player_SetFlag, Player_ClearFlag, Player_TestFlag), cleared on every state change (Player_SetState) */
    u16 unk3A0;                 /* 0x3A0 */
    u16 stateRand;              /* 0x3A2: random value drawn on each state change */
    s32 motionTime;             /* 0x3A4: the motion time (Player_UpdateMotionTime adds len - blend, Player_TickTimers 2; wraps at 100000) */
    u8 unk3A8[0x3AC - 0x3A8];
    s32 targetAngle;            /* 0x3AC: the heading from target, plus a quarter turn (Player_UpdateInBattle) */
    f32 emDist;                 /* 0x3B0: the distance to a monster in its area: Field_PushApart writes it for each in turn, so the last one in gEms stays */
    void *target;               /* 0x3B4: the Em it deals with: the monster in battle with it that set inBattle (Player_UpdateInBattle), the one it hit (ObjAttach_DamagePlayer), in the lobby the townsperson it talks to */
    f32 vel[3];                 /* 0x3B8: velocity */
    f32 accel[3];               /* 0x3C4: acceleration */
    struct PlayerVt *handlers;  /* 0x3D0: the player handlers, gPlayerHandlers (Player_Init) */
    u8 hitDelay;                /* 0x3D4: frames before the player's own hit window opens (as ObjAttach.hitDelay): Player_UpdateHitTimers counts it down in hitPhase 1, a motion blend adds to it (player_stats.c) */
    u8 hitFrames;               /* 0x3D5: frames the hit window stays open (as ObjAttach.hitFrames), counted down in hitPhase 2 (Player_UpdateHitTimers) */
    u8 unk3D6[0x3E7 - 0x3D6];
    u8 nextRow;                 /* 0x3E7: the attack row started when the player's own hit window closes, 0: none (Player_UpdateHitTimers runs Player_StartNextHit with it); as ObjAttach.nextRow */
    u8 unk3E8[0x3EC - 0x3E8];
    u8 netActive;               /* 0x3EC: the lobby: set when a town message moves or places the player (TownMsg_OnActor, TownMsg_OnMove, TownMsg_OnArea) */
    u8 unk3ED[0x3EF - 0x3ED];
    u8 hitAttrs;                /* 0x3EF: attribute bits of the attacks taken, ORed in from the attack object +0x65 (ObjAttach_DamagePlayer): 0x80 cannot be guarded without SKILL_GUARD_PLUS20 or with the heavy bowgun (Player_CanGuardHit) */
    u16 hitAngle;               /* 0x3F0: direction of the last hit taken (ObjAttach_SetHitHeading; Player_CanGuardHit; as Em.hitAngle) */
    s16 hitForce;               /* 0x3F2: the force of a hit taken while guarding (Player_GuardHit) */
    u16 lastAttackKind;         /* 0x3F4: the kind of the attack taken (the attack object +0x63; Player_GuardHit compares 4, 15, 21) */
    u8 unk3F6[0x3F8 - 0x3F6];
    u8 unk3F8;                  /* 0x3F8 */
    u8 hitIds[16];              /* 0x3F9: the ids of the attacks that hit it last (as Em.hitIds) */
    u8 side;                    /* 0x409: side: players with the same bit 0 are on the same side */
    u8 hitStopAdd;              /* 0x40A: the hit stop of the last hit (the attack object +0x70, ObjAttach_DamageEm/DamagePlayer), added to hitStop (Player_UpdateHitStop) */
    u8 hitStop;                 /* 0x40B: counts down; nonzero stops the timers of Player_TickTimers */
    u8 unk40C[0x40E - 0x40C];
    u16 noHitTimer;             /* 0x40E: frames the player cannot be hit (as Em.noHitTimer): Player_SetEvadeFrames sets 6 (+2/+4 with the evasion skills), Player_Reset 30; counted down by Player_TickTimers; Player_UpdateEnvironment skips the area damage while > 0 */
    u16 noPushTimer;            /* 0x410: frames without body pushing (as Em.noPushTimer): Field_PushApart skips the player while nonzero; counted down by Player_TickTimers; states set 2/10 */
    u8 hitIdNext;               /* 0x412: the next slot of hitIds (as Em.hitIdNext) */
    u8 unk413[0x414 - 0x413];
    u8 hitPhase;                /* 0x414: the player's hit window: 0 off, 1 waiting hitDelay frames, 2 open for hitFrames (Player_UpdateHitTimers; as ObjAttach.hitPhase) */
    u8 unk415[0x416 - 0x415];
    u8 fxMade;                  /* 0x416: 1 once the state made its effect (WeaponTrail_Create) at its frame: LongSword_StateSlash, LongSword_StateSpirit1 (as Em.unk416) */
    u8 unk417;                  /* 0x417 */
    u8 unk418[0x420 - 0x418];
    u8 *hitTimers;              /* 0x420: the hit window timers in use: &hitDelay (Player_Reset), or the hitDelay of its attack object that hit (ObjAttach_LinkTarget) */
    f32 hitPos[3];              /* 0x424: where the last hit taken landed (ObjAttach_DamagePlayer) */
    u16 unk430;                 /* 0x430 */
    s16 noFlinchTimer;          /* 0x432: the attack states set it (60, 40, 2...), Player_TickTimers counts it down; while nonzero Player_IgnoresFlinch (pouch.c) returns 1 and Player_ReactToHit (player_hit.c) skips the flinch */
    u8 unk434;                  /* 0x434 */
    u8 unk435[0x437 - 0x435];
    u8 fieldCollide;            /* 0x437: 1: the player collides with the walls (as Em.fieldCollide): Player_RunState and TownPc_Spawn set 1; Player_StateBedLieDown..35, TownPc_State19 and Contest_Open clear it */
    PlayerWork work;            /* 0x438: the work area of Player_MotionFxStep, the motions' sounds and effects (player_motion_fx.c) */
    u8 unk44C[0x4B8 - 0x44C];
    u16 statusIcons;            /* 0x4B8: the HUD's status icons, PL_STATUS_ICON_* bits (Player_UpdateStatusIcons) */
    u16 unk4BA;                 /* 0x4BA */
    struct ObjAttach *hitAttach;/* 0x4BC: the attack object of the last hit taken (ObjAttach_DamagePlayer; as Em.hitAttach) */
    u8 unk4C0[0x4C1 - 0x4C0];
    u8 unk4C1;                  /* 0x4C1 */
    u8 unk4C2[0x4C4 - 0x4C2];
    u8 unk4C4;                  /* 0x4C4 */
    u8 unk4C5[0x4C6 - 0x4C5];
    u16 unk4C6;                 /* 0x4C6 */
    s8 meshShown[32];           /* 0x4C8: one per mesh of model, 1: drawn (as Em.meshShown): Player_InitRecords sets all 32; Player_DrawLocal and Player_Draw skip the others */
    s16 modelCount;             /* 0x4E8: model slots used by the body (as Em.modelCount; Player_LoadPartModels, Player_FreeModels) */
    s16 modelFirst;             /* 0x4EA: the body's first model slot (as Em.modelFirst) */
    struct Model *model;        /* 0x4EC: the skeleton (as Em.model): select_chara.c sets it to Model_Get(gAreaSubModelId[slot]) */
    u8 unk4F0[0x4F4 - 0x4F0];
    struct Model *weaponModel;  /* 0x4F4: the model: a 128-byte header of the pool at 0x6AE000 (Player_LoadWeaponModel) */
    s16 weaponModelSlotCount;   /* 0x4F8: model slots used by the weapon model (Player_AllocWeaponModel) */
    s16 weaponModelSlot;        /* 0x4FA: the first model slot of the weapon model (Player_AllocWeaponModel, Player_FreeWeaponModel) */
    struct Model *partModels[6];/* 0x4FC: parts 1-5 (the equipment): the model (Model_Get of its first slot; Player_LoadPartModels); [0] unused, the body's is model */
    s16 partModelCount[6];      /* 0x514: parts 1-5: model slots used */
    s16 partModelFirst[6];      /* 0x520: parts 1-5: the first model slot */
    struct DrawSlot *drawSlot;  /* 0x52C: its draw slot: an entry of the pool at 0x6A5AD0 (DrawSlot_Get), drawn by DrawCb_LocalPlayer (Player_InitAll) */
    s16 drawSlotId;             /* 0x530: its index, -1: none */
    u8 painted;                 /* 0x532: the Em paint mark read through Player (as Em.painted): Player_TickPaint runs only for isEm objects; Player_Reset sets 255 (players: never); Map_MonsterLocated tests it */
    u8 carry;                   /* 0x533: the carried item: its kind less one in the high nibble (PL_CARRY_KIND_MASK, from Item_CarryKind), the hold count in the low (PL_CARRY_HOLD_MASK, Player_UpdateCarryKind) */
    u8 ammoType;                /* 0x534: kind of the loaded ammo (ItemData.ammoKind) */
    u8 savedAmmoType;           /* 0x535: ammoType saved while the ammo menu is open (Player_BowgunAmmoSwitch) */
    u8 ammoMenuOpen;            /* 0x536: 1 while the ammo menu is open (Player_BowgunAmmoSwitch) */
    u8 spotCheckDelay;          /* 0x537: nonzero: no action spot check (Player_CheckActionSpot sets 20; Player_UpdateStatus counts it down) */
    u16 startAngle;             /* 0x538: its heading: gStartAngle, the action spot angle + ANGLE_TURN / 4 */
    s16 paintTimer;             /* 0x53A: frames until painted is cleared (as Em.paintTimer; Player_TickPaint) */
    u8 unk53C[0x540 - 0x53C];
    u32 lightFadeColors[3];     /* 0x540: per light of light set 1: the colour it fades from, 0xFFRRGGBB (LightSet_FadeColors moves it a fifth of the way to the set's colour each draw and keeps the result; 0: the set's own colour). Player_ClearLightColors clears it; the character maker sets 0x01000000 (from black) */
    u8 unk54C[0x568 - 0x54C];
    f32 prevPos[3];             /* 0x568: the position before the move of this frame (as Em.prevPosX..prevPosZ; TownPc_Update copies pos) */
    f32 floorY;                 /* 0x574: the floor height under the player (Player_Update; Player_Reset sets pos[1]; as Em.floorY) */
    u8 unk578[0x580 - 0x578];
    u32 hairColor;              /* 0x580: the hair colour (HairMenu_Apply, HunterCard_Build, Player_SetupFromSave) */
    u8 unk584[0x588 - 0x584];
    u8 rideState;               /* 0x588: 0 none, 1 standing on the monster rideEm, 3 landed on it from a jump, 2 leaving (Em_CarryRiders sets 1/3, the states set 2/0) */
    u8 rideEm;                  /* 0x589: the slotNo of the monster the player stands on (Em_CarryRiders, Player_CheckRideSlip) */
    u8 rideBone;                /* 0x58A: the bone of the monster rideEm the player stands on (Em_CarryRiders: Em_PosInFrame with it) */
    u8 lookVariant;             /* 0x58B: the look variant (0..3) from the colour (Player_BuildPartIds: gFaceBodyTypeMale/gFaceBodyTypeFemale by gender): picks the head model (Look_MapPartId k) and the texture set (+ gender * 4) */
    s16 unk58C;                 /* 0x58C */
    s16 unk58E;                 /* 0x58E */
    u8 unk590[0x594 - 0x590];
    s16 hitSlowTimer;           /* 0x594: from the attack object +0x70 (ObjAttach_DamageEm): while > 0 the motion runs at 0.2 (Player_UpdateMotion) and some attacks wait */
    u8 pushPriority;            /* 0x596: compared between two bodies to pick the one pushed (PlayerEm_PushApart, EmEm_PushApart); Player_Reset sets 1 */
    u8 nearLargeMonster;        /* 0x597: 1 while a large monster (pushPriority 3 or more) is near: within 300 horizontally, 500 for one above 3, and less than 300 below the player (Player_UpdateNearMonster) */
    s16 swordGaugeTimer;        /* 0x598: long sword: frames left after the gauge fills (900; LongSword_AddGauge) */
    u8 presetEquip;             /* 0x59A: nonzero: preset equipment, from the tables at 0x44AED0 (Player_InitForQuest) */
    u8 unk59B[0x59C - 0x59B];
    f32 rideDelta[3];           /* 0x59C: how far the monster carried the player this frame (Em_CarryRiders; Player_CheckRideSlip compares its length) */
    EquipFxSwing equipFx[4];    /* 0x5A8: the swinging parts of the equipment (Player_RunEquipEffects, Effect_RunList); cleared by Player_ClearFxBlock */
    s8 attackDrug;              /* 0x628: attack bonus of the drugs (Item_Use: at least 3 or 5), kept in gScene.mealAttack (Player_RecomputeStats) */
    s8 attackBuff;              /* 0x629: the timed attack buff (10, 25...; Item_Use, Item_UseTeamPowder) */
    s16 attackBuffTimer;        /* 0x62A: duration of attackBuff */
    s8 defenseDrug;             /* 0x62C: defense bonus of the drugs (Item_Use: at least 6 or 10), kept in gScene.mealDefense */
    s8 defenseBuff;             /* 0x62D: the timed defense buff (20, 40, 50) */
    u16 defenseBuffTimer;       /* 0x62E: duration of defenseBuff (Player_ClearBuffs, Item_Use) */
    u16 attack;                 /* 0x630: attack power, 1..800 */
    u16 defense;                /* 0x632: defense */
    u16 tradeGetItem;           /* 0x634: the trader kind 0x0A: the item it gives the player (Pouch_Add with 1, Notice_Push; EmTrader_RunTalk) */
    u16 tradeGiveItem;          /* 0x636: the trader kind 0x0A: the item the player hands over (Pouch_Add with tradeGiveCount; cleared from gQuest.unk158 when it matches) */
    u8 unk638[0x653 - 0x638];
    u8 stateRequest;            /* 0x653: 1: a state change was requested (Player_RequestState, Player_RequestStateSynced; Player_Update) */
    u8 unk654[0x660 - 0x654];
    u8 floorKind;               /* 0x660: the floor triangle under the player (as Em.floorKind; TownPc_Update passes it to Em_GetFloorInfo as a TriHdr) */
    u8 floorArg;                /* 0x661: byte 1 of the floor triangle (as Em.floorArg): passed with the voice SEs (bank 7, PlayerSe_VoiceAtFrame) */
    u8 floorAttr;               /* 0x662: the floor triangle attribute (Em.floorAttr, a u16 there): EmScript_IfScriptPlayerFloorAttr compares the byte, EmScript_PickNoticedPlayerSameFloor reads the u16 (1..3, 4..10) */
    u8 unk663[0x664 - 0x663];
    void *lightColors;          /* 0x664: the hunter's own colours of lights 0 and 1 (LightSet_SetColors reads it as rows of 4 floats); NULL: the area's */
    u8 mustCrouch;              /* 0x668: byte 2 of the floor kind's gAreaKindFlags row. While set, moving is PL_ACT_CROUCH_WALK (no walk, run, roll, standing up or weapon draw) */
    u8 floorHazard;             /* 0x669: byte 7 of the floor kind's gAreaKindFlags row. Player_UpdateEnvironment: 1 poison (at least 10 unless SKILL_POISON_IMMUNE), 2 damage every 8 frames */
    u8 unk66A[0x66C - 0x66A];
    u8 partAnimOn[4];           /* 0x66C: per part: a motion override is set (Player_SetAimLayers, Player_ClearPartOverrides) */
    u16 partAnimMotion[4];      /* 0x670: per part: the override motion (Player_SetPartAnim) */
    u16 partAnimWeight[4];      /* 0x678: per part: the override weight, 0..100 (from aimWeight) */
    u16 area;                   /* 0x680: the area the player is in (as Em.unk680; gathering compares it) */
    u8 areaMoveState;           /* 0x682: PL_AREA_MOVE_*: how Player_Reset places the hunter in the area it moves to (nextArea) */
    u8 nearCamera;              /* 0x683: 1: another player's hunter within 200 of the camera (Player_Update): drawn at alpha 0.2 in the sorted list (Player_Draw); 0 for the local player */
    u16 nextArea;               /* 0x684: the area the player goes to (Town_EnterArea, AreaMenu_GoTo; as Em.nextArea) */
    u8 unk686[0x688 - 0x686];
    f32 startPos[3];            /* 0x688: where the player starts in the area: gStartPos, the action spot target (Player_InitAll, Player_CheckActionSpot) */
    s16 stamina;                /* 0x694 */
    u8 unk696[0x698 - 0x696];
    u32 wallHitDirs;            /* 0x698: as Em.wallHitDirs: Em_CollideField also runs for the players (Field_Update); Motion_VoiceAtFrame tests 0xE0000007 (a wall ahead) to pick the voice from gEmWallContacts */
    u16 lean;                   /* 0x69C: the body lean while turning, a 16-bit turn kept within 2560 of upright either way (Player_UpdateAutoTurn, Actor_TurnToTarget); Player_TwistBoneToLook rolls a bone by it */
    u8 unk69E[0x6A0 - 0x69E];
    f32 mapPos[3];              /* 0x6A0: the position with the area record offsets, y at the middle of its range, for the map (Player_UpdateAreaPos; Map_DrawArea) */
    s16 dashDelay;              /* 0x6AC: frames before R1 with the stick dashes again (states PL_ACT_RUN_AFTER_DASH, PL_REACT_MASH_RUN, PL_CARRY_WALK): the states set 30, counted down (Player_UpdateStatus, TownPc_TickTimers) */
    u8 unk6AE[0x6AF - 0x6AE];
    u8 aimLockTimer;            /* 0x6AF: bowgun and bow: 5 or 10 frames after an aim change (Bow_UpdateAim); no shot while nonzero */
    u8 aimMode;                 /* 0x6B0: bowgun and bow: the aim mode (toggled; gScene.aimReverse applies in it) */
    u8 unk6B1[0x6B2 - 0x6B1];
    s16 hitDamage[8];           /* 0x6B2: the damage of the hit taken in [0] (ObjAttach_DamagePlayer; Player_ReactToHit); as Em.hitPartDamage, one per part; cleared by Hit_ClearStatusAll */
    u8 unk6C2[0x6D4 - 0x6C2];
    u8 partFlags[8];            /* 0x6D4: as Em.partFlags: the mark bits of the hits taken; ObjAttach_DamagePlayer ORs the attack's hitMarks into [0]; cleared by Hit_ClearStatusAll */
    s16 hpRecoverable;          /* 0x6DC: the red part of the health bar */
    s16 hpMax;                  /* 0x6DE: 1..150 */
    u8 unk6E0[0x6E4 - 0x6E0];
    f32 alpha;                  /* 0x6E4: set to 1.0 (Player_Reset, Chara_SetupNewPlayer; as Em.alpha) */
    u8 unk6E8[0x6EC - 0x6E8];
    void *contact;              /* 0x6EC: a Player or an Em: the other party of the last hit (ObjAttach_DamagePlayer), as Em.contact */
    struct ObjAttach *contactAttach;/* 0x6F0: that hit's attack object */
    u8 unk6F4[0x6F6 - 0x6F4];
    s16 status5Buildup;         /* 0x6F6: the status 5 buildup: status5Hit added per hit (Player_ReactToHit), full at 50 (Player_Status5Full); counted down after status5Delay (Player_UpdateStatus) */
    s16 status5Delay;           /* 0x6F8: frames before status5Buildup decays: 150 at a hit (Player_ReactToHit), counted down (Player_UpdateStatus) */
    s16 status5Hit;             /* 0x6FA: the hit status 5 amount, added to status5Buildup (Player_ReactToHit; as Em.status5Hit) */
    u8 unk6FC[0x6FE - 0x6FC];
    s16 status2Buildup;         /* 0x6FE: the status 2 buildup: status2Hit added per hit (Player_ReactToHit); > 0 picks the status state (Player_PickStatusState) */
    s16 status2Hit;             /* 0x700: the hit status 2 amount, added to status2Buildup (Player_ReactToHit; as Em.status2Hit) */
    u8 unk702[0x706 - 0x702];
    s16 poison;                 /* 0x706: poison left: counts down every poisonTimer frames (60 with SKILL_POISON_HALF, 15 with SKILL_POISON_DOUBLE, else 30); an area of kind 1 keeps it at least 10 (Player_UpdateEnvironment; as Em +0x706) */
    s16 poisonHit;              /* 0x708: the hit poison amount, passed to Player_AddPoison (Player_ReactToHit; as Em.poisonHit) */
    s16 poisonTimer;            /* 0x70A: frames to the next poison tick */
    u8 unk70C[0x710 - 0x70C];
    s16 status4Buildup;         /* 0x710: the status 4 buildup: status4Hit added per hit (Player_ReactToHit); > 0 picks the status state (Player_PickStatusState) */
    s16 status4Hit;             /* 0x712: the hit status 4 amount, added to status4Buildup (Player_ReactToHit; as Em.status4Hit) */
    u8 unk714[0x71A - 0x714];
    s16 status1Hit;             /* 0x71A: the hit status 1 amount (Em_ApplyHitStatus; as Em.status1Hit) */
    u8 unk71C[0x722 - 0x71C];
    u8 hideShadow;              /* 0x722: as Em.hideShadow: nonzero: Shadow_Draw (the shadow) skips the object; counted down (Player_TickTimers, TownPc_TickTimers) */
    u8 unk723[0x724 - 0x723];
    f32 attackFactor;           /* 0x724: attack factor: attack / 100 */
    f32 defenseFactor;          /* 0x728: defense factor: defense / 2, at least 1 */
    u8 unk72C[0x730 - 0x72C];
    f32 surfaceY;               /* 0x730: as Em.surfaceY (TownPc_Update passes it to Em_GetFloorInfo) */
    u8 unk734[0x738 - 0x734];
    u8 pushed;                  /* 0x738: as Em.pushed (same offset): Field_PushApart sets it on players too; Field_UpdateAll reads it as ((Em *) pl)->pushed */
    u8 weaponSeMute;            /* 0x739: nonzero: no weapon SEs (PlayerSe_WeaponAtFrame); the weapons hit checks set 2, Player_UpdateStatus counts it down */
    u8 unk73A_combat;           /* 0x73A: set by Em_SetCombat; cleared by Player_UpdateInBattle when the monster (its bit in Em +0x73A) resets */
    u8 unk73B[0x740 - 0x73B];
    f32 fadeAlpha;              /* 0x740: the local player's draw alpha in the field: eased a tenth of the way to CamCtl_GetPlayerFade's target each frame, 1.0 without one (Player_DrawLocal; Player_Reset sets 1.0) */
    s16 mashBuildup;            /* 0x744: the buildup toward the mash status: mashBuildupHit added per hit (Player_ReactToHit); Player_HasMashBuildup, Player_InMashStatus */
    s16 mashLeft;               /* 0x746: the status of state 2/0x1E: pressed buttons take 10 (SKILL_MASH_FAST twice, SKILL_MASH_SLOW half: mashTimer); at 0 PL_REACT_MASH_FREE; synced in qnet_sync message 3 */
    s16 mashBuildupHit;         /* 0x748: the hit amount added to mashBuildup (Player_ReactToHit) */
    s16 tradeGiveCount;         /* 0x74A: the trader kind 0x0A: the count taken with tradeGiveItem, -1 or -99 (Pouch_Add; EmTrader_RunTalk) */
    u8 contactHit;              /* 0x74C: as Em.contactHit: 1 once one of its attacks with ATKF_REPORT_CONTACT hits (ObjAttach_DamagePlayer, ObjAttach_DamageEm); each attack's Create clears it (PlayerAttack_Create) */
    u8 unk74D[0x754 - 0x74D];
    f32 moveTo[3];              /* 0x754: a point to move to (Player_StartMoveTo, Player_MoveToSpawn, Contest_TakeSeat, HairMenu_MovePlayer) */
    f32 moveToStep[3];          /* 0x760: added to pos each frame while moveToFrames counts (Player_UpdateMoveTo, pouch.c) */
    s16 moveToFrames;           /* 0x76C: frames to reach moveTo (Player_StartMoveTo, Player_UpdateMoveTo) */
    s16 lookYaw;                /* 0x76E: the head turn toward the nearest monster (Player_FindLookTarget; pouch.c reads it signed; the lobby unwinds it by ANGLE_TURN / 64 a frame through u16, TownPc_UnwindAngle76E) */
    u8 lookFound;               /* 0x770: 1: Player_FindLookTarget found a monster in range */
    u8 unk771;                  /* 0x771 */
    u8 inBattle;                /* 0x772: nonzero while a monster fights it (Player_UpdateInBattle; Bgm_IsBattle, Hud_DrawPlayerStatus) */
    u8 prevInBattle;            /* 0x773: the previous inBattle (Player_UpdateInBattle) */
    u8 weaponFlags;             /* 0x774: flags of the weapon type (WeaponType_Flags(weaponType): 1, 2, 4; Player_SkillApplies) */
    u8 coatingSlot;             /* 0x775: bow: the pouch slot of the loaded coating, 0xFF: none (sub_main Bow_SetCoatingSlot; a shot uses one up) */
    s16 swordGauge;             /* 0x776: long sword: the spirit gauge 0..100 (LongSword_AddGauge, LongSword_GaugeAtLeast, Hud_DrawWeaponBox) */
    struct Em *cart;            /* 0x778: the Felyne cart (an Em of kind EM_KIND_FELYNE_CART, its cartPlayer this player) that carries the player away (Player_StateCart); cleared when it leaves (QuestEm_LeaveArea) */
    ItemStack pouch[20];        /* 0x77C: the item pouch */
    s16 unk7CC;                 /* 0x7CC */
    s16 unk7CE;                 /* 0x7CE */
    struct ActionSpot *spot;    /* 0x7D0: the action spot the player stands at (Player_FindActionSpot; in the town, the one the action button uses) */
    s16 weaponTimer;            /* 0x7D4: great sword: the charge (frames); gunlance: the wyvern fire cooldown; bow: frames since the last shot (to 1000) */
    s16 sharpness;              /* 0x7D6 */
    s8 chargeLevel;             /* 0x7D8: great sword: the charge level 0-3 (GreatSword_ChargeLevel) */
    s8 fishTimer;               /* 0x7D9: fishing: frames (Lure_Update, Fish_StepBite), counted down by Player_UpdateStatus */
    s16 staminaMax;             /* 0x7DA: 75..450 */
    s16 sharpnessMax;           /* 0x7DC */
    u8 itemUseDelay;            /* 0x7DE: counted down (Player_UpdateStatus); the item button (PAD_SQUARE) does nothing while nonzero (idle and move states) */
    u8 sharpnessLevel;          /* 0x7DF: the colour (Player_GetSharpnessLevel) */
    u16 pouchCursor;            /* 0x7E0: the selected pouch slot */
    u16 useItem;                /* 0x7E2: the item being used: the selected pouch item (Player_UseItem, Item_Use compare it with item ids); copied to gSpecialMinePoint (SpecialMine_Create) */
    u8 itemSelect;              /* 0x7E4: 1 while the item selector is held open (Player_UpdateItemSelect); only buttons 0x90 get through */
    u8 questStartDone;          /* 0x7E5: 1 once Player_StateQuestStart (PL_EVENT_QUEST_START) ran; Player_Reset clears it. In questPhase 4, Player_IdleActions and Player_CrouchInput ask for the quest's start pose while it is 0 */
    u16 ammoSlot;               /* 0x7E6: bowgun: the pouch slot of the loaded ammo, 0xFF: none */
    f32 targetPos[3];           /* 0x7E8: the point the monsters aim at (Em_GetTargetPos) */
    PlayerPend pend[2];         /* 0x7F4: positions received over the network, applied when their timers run out (QSync_QueuePlayerPos, QSync_UpdatePlayerPos) */
    s16 ammoCount;              /* 0x814: bowgun: rounds of the loaded ammo left, 0xFF: unlimited */
    u8 ammoMenuLock;            /* 0x816: counted down to 0 by Player_UpdateStatus; Player_BowgunAmmoSwitch does nothing while nonzero. Player_Reset zeroes it; no C code sets it nonzero */
    u8 startItemList;           /* 0x817: 1 + the start item list (Pouch_SetStartItems, Quest_InitPlayers) */
    s16 staminaTimer;           /* 0x818: frames until the maximum stamina drops (10800) */
    u8 supplyBoxOpen;           /* 0x81A: Hud_SupplyBoxOpen, gHudState->supply; the supply box menu clears it on cancel (pouch.c) */
    u8 menuCursor;              /* 0x81B: menu cursor (the supply box and delivery menus) */
    u8 actRequest;              /* 0x81C: the gesture requested (Player_RequestGesture); Player_StartGesture starts state 6/gGestureStates[it] */
    u8 weaponShown;             /* 0x81D: nonzero: the weapon model is drawn (Player_DrawLocal, Player_Draw call Player_DrawWeapon); Player_Reset and the hunter setups set 1, plsel's preset players 0, the town hunters while not free (TownPc_IsFree) */
    u8 tradeMsgTimer;           /* 0x81E: EM_KIND_TRADER sets 5, 10, 60 (EmTrader_RunTalk, EmTrader_ShowMessage); Player_UpdateStatus counts it down */
    u8 tradeCount;              /* 0x81F: the trader kind 0x0A: +1 per trade up to 7 (7 at once from 4 at random), decremented by the trader's later steps */
    u8 unk820[0x821 - 0x820];
    s8 aimWeight;               /* 0x821: bowgun: -100..100, the weight of the second motion layers (Bowgun_SetAimLayers) */
    s16 cookTimer;              /* 0x822: the cooking timer (frames on the spit, Player_StateSpitRoast) */
    s16 staminaLockTimer;       /* 0x824: frames left of an item effect that keeps the stamina (Item_Use; Player_StaminaLocked) */
    s16 savedAmmoCount;         /* 0x826: ammoCount saved with savedCoatingSlot (Player_BowgunAmmoSwitch, Bow_StateFire) */
    u8 cookResult;              /* 0x828: the cooking result: 0 raw, 1 rare, 2 done, 3 burnt, 4 gourmet (Player_StateSpitRoast) */
    u8 savedRoundsLoaded;       /* 0x829: roundsLoaded kept over the reload and the weapon change (restored from it), as savedClipSize and savedAmmoCount */
    u8 savedClipSize;           /* 0x82A: clipSize saved with savedCoatingSlot (Player_BowgunAmmoSwitch, Bow_StateFire) */
    u8 voice;                   /* 0x82B: the voice: save data +0x20D8 and the hunter card (Player_SetupFromSave); gVoiceBanks[gender * 16 + it] (lobby), Chara_SetVoiceMotion */
    s8 name[18];                /* 0x82C: the hunter name (SaveData name, Player_LoadName) */
    s16 gotItem;                /* 0x83E: the item to give the player (Pouch_GiveItem: Item_Use, Player_GatherAtSpot, Contest_GivePrize, PoogieMenu_Update) */
    u16 gotItemId;              /* 0x840: the item id that goes with gotItem (Pouch_GiveItem in TownPc_State63 and the fishing states; the lure's bite code switches on it) */
    u16 baited;                 /* 0x842: the lure keeps its bait: 1 from the cast (Player_StateFishCast, the town's cast); a fish goes for the lure only while it is set (Fish_StepSwim) and a nibble clears it one time in four (Fish_StepBite); with it the action button picks PL_ACT_FISH_REEL_BAITED over PL_ACT_FISH_REEL_EMPTY (in the town PL_TOWN_FISH_REEL_BAITED over PL_TOWN_FISH_REEL_EMPTY) */
    u8 drunk;                   /* 0x844: lobby: 1 while drunk: set by the tenth drink at a seat online (TownPc_CountDrink) and by the drink contest; the town's idle, walk and run take their drunk forms; sent in the state messages; talks and menus clear it */
    u8 bowgunReady;             /* 0x845: 1 while the bowgun attack input runs with rounds loaded (Bowgun_AttackInput), cleared at each frame start (Player_Update) and by Player_Reset; Player_IsAiming tests it */
    s16 aimPitch;               /* 0x846: the aim pitch, 16-bit turn: the first-person camera (camera mode 1, Mtx_FromAngles16Pos) and Hud_DrawObjMarker use it with the heading rot[1]; up/down input adds +-ANGLE_TURN / 64 clamped to +-ANGLE_TURN / 8 (45 deg; Player_AddAimPitch, gScene.aimReverse flips it); Player_AddAimPitchPct sets it from aimPitchPct */
    u8 freeState;               /* 0x848: set to 1 each frame by the idle and move states, cleared at the frame start (Player_Update): items can be used (Player_IsFree, the pouch menu) */
    u8 menuPage;                /* 0x849: menu page */
    u8 pouchHudFlags;           /* 0x84A: pouch HUD flags: 2 next, 4 previous item (Player_UpdateItemSelect), 1, 8, 16 (ItemHud_Update, Hud_DrawPouchBar) */
    u8 menuDelay;               /* 0x84B: menu input delay */
    ItemStack delivered[4];     /* 0x84C: items delivered for the quest (Quest_CountDelivered) */
    u16 offerItem;              /* 0x85C: an item offered to another player (Player_TakeOfferedItem) */
    s16 offerCount;             /* 0x85E: its count */
    u8 offerRequest;            /* 0x860: the pouch menu's offer sets it with offerTo/offerItem/offerCount; the idle states then enter PL_ACT_OFFER_ITEM */
    u8 offerTo;                 /* 0x861: the index of the player it is offered to */
    u8 offerState;              /* 0x862: PL_OFFER_*: the answer to an item offer (the giver holds the asker's index for a moment; QSYNC_PL_ITEM_ASK and QSYNC_PL_ITEM_ANSWER) */
    u8 offerStarted;            /* 0x863: 1 in the frame an offer starts (offerRequest taken, PL_ACT_OFFER_ITEM): the pouch menu closes (PouchMenu_Update); cleared by Player_Update */
    s16 envTimer;               /* 0x864: frames to the next environment effect (30..180, Player_UpdateEnvironment) */
    u8 syncTimer;               /* 0x866: frames until the player state is sent again (QSync_SendPlayer sets it; counted down before each send) */
    u8 drinkCount;              /* 0x867: lobby: the drinks taken at a seat while not drunk (TownPc_CountDrink); at 10 (online) drunk is set; cleared with drunk */
    u8 skills[5];               /* 0x868: skill ids (PL_SKILL_SLOTS) */
    u8 unk86D[0x86E - 0x86D];
    u8 questStatus;             /* 0x86E: the lobby: low bits the quest category (QuestFlags_GetCategory), QUEST_STATUS_PAID accepted and paid, QUEST_STATUS_TAKEN accepted (QuestDesk_Accept), QUEST_STATUS_SET_OUT set out (Town_UpdateMode); cleared by QuestDesk_Drop; on the hunter card (HunterCard_Build) */
    u8 teamItemMask;            /* 0x86F: set to 15 when a team item (ITEM_ID_TEAM_HEAL to ITEM_ID_TEAM_DEFENSE) is used (Item_UseTeamPowder); the bit of each player reached is cleared (player_motion_fx.c) */
    u16 coolDrinkTimer;         /* 0x870: frames left of item ITEM_ID_COOL_DRINK's effect (18000; Item_Use); nonzero: Hud_HealthColor and Player_UpdateEnvironment skip the heat */
    u16 hotDrinkTimer;          /* 0x872: frames left of item ITEM_ID_HOT_DRINK's effect (18000; Item_Use); nonzero: no cold in zones 2 and 4 (Player_UpdateHunger, Hud_StaminaColor) */
    u16 regenTimer;             /* 0x874: frames left of items ITEM_ID_REGEN and ITEM_ID_FOOD_REGEN_SHORT to ITEM_ID_FOOD_REGEN_LONG (Item_Use, 3600..14400): doubles the health recovery step (Player_UpdateStatus); Notice 22 at the end; status icon PL_STATUS_ICON_REGEN */
    u8 faints;                  /* 0x876: times fainted in the quest, up to 99 (Player_StateFaint; QuestMenu_DrawFaints) */
    u8 supplyTaken;             /* 0x877: nonzero once the supply box was taken from (SupplyBox_Take, QSync_OnSupply) */
    f32 resist[5];              /* 0x878: element resistances (fire, water, thunder, ice, dragon), -100..100 */
    s16 detectTimer;            /* 0x88C: frames left of an item effect (Item_Use) that Player_DetectLevel and the HUD glow test */
    u16 supplyBoxTimer;         /* 0x88E: 900 frames after a supply box take (SupplyBox_Take, SupplyBox_Update) */
    s16 recoverTimer;           /* 0x890: state 4/x: 150 frames (skill 1 halves, skill 3 doubles), mashing takes 1 more each 3 frames; the hunter gets up at 0 */
    u8 offerNoticeTimer;        /* 0x892: 90 frames after the pouch-full notice for an offered item (no new take while nonzero) */
    s8 fallGrabCount;           /* 0x893: the ledge grabs out of a fall: Player_StateLedgeHang counts PL_ACT_LEDGE_GRAB after PL_ACT_FALL; Player_Update zeroes it in any other state; Player_StateLeapOrFall allows the grab while it is under 21 */
    u8 skillGroups[5];          /* 0x894: the skill group of each slot */
    s8 demonMode;               /* 0x899: dual swords: 1 in the demon mode (DualSwords_StateDemonToggle; PlayerAttack_Setup) */
    s16 noFlinchItemTimer;      /* 0x89A: item ITEM_ID_DEFENSE_BURST sets it with defenseBuffTimer (Item_Use); counted down; tested with noFlinchTimer and demonMode (Player_IgnoresFlinch) */
    u8 prevFreeState;           /* 0x89C: freeState of the last frame (Player_Update): the guard SKILL_AUTO_GUARD and the heavy bowgun shield guard only then */
    s8 skillPoints[5];          /* 0x89D */
    u8 attackPercent;           /* 0x8A2: the attack power in percent (great sword charge levels: 100..130) */
    u8 attackPercentBase;       /* 0x8A3: the weapon type's attack percent (gWeaponClassAttack[weaponType], Player_ResetAttackPercent) */
    u32 styleColor;             /* 0x8A4: gPlayerColorWords[style] (Player_SetColorWord) */
    u8 lookPartId[6];           /* 0x8A8: per armour part: the model part id Look_MapPartId picked (Player_LoadLookModels), PL_LOOK_PART_NONE: none (Player_InitPartTable) */
    u16 weaponModelKind;        /* 0x8AE: the kind of the loaded model, PL_WEAPON_MODEL_NONE: none (Player_LoadWeaponModel) */
    u8 headAltModel;            /* 0x8B0: 1: the head armour has no model for the gender and part slot 6 is used (Armor_LacksSexModel; Player_LoadLookModels, Player_QueueLookFiles) */
    u8 deliveryMenu;            /* 0x8B1: the delivery menu: 255, 1, 0 (Player_RunDeliveryMenu, Player_StateBedMenu); blocks the pad input */
    u8 yesNoCursor;             /* 0x8B2: the delivery menu: the yes or no cursor (gYesNoTexts[it], DeliveryMenu.yesNo; Player_RunDeliveryMenu, Quest_DeliveryMenu) */
    u8 unk8B3;                  /* 0x8B3 */
    f32 motionSpeed;            /* 0x8B4: motion speed factor (1.0; 0.5..1.54 per state: Player_SetState and the weapon states); motionStep0/1 are 2.0 times it */
    s8 mashTimer;               /* 0x8B8: a countdown between the button presses that shake off a stun (Player_UpdateMashEscape) */
    u8 savedCoatingSlot;        /* 0x8B9: bow: a copy of coatingSlot (taken at a shot, with savedAmmoCount, unk829 and savedClipSize, and by the ammo menu Player_BowgunAmmoSwitch, which restores it) */
    s16 actDetectTimer;         /* 0x8BA: 300 frames from the gesture state 6/2 (Player_StateGestureScout): as detectTimer (Player_DetectLevel, Map_MonsterLocated, AreaFlyer_Run) */
    u8 charType;                /* 0x8BC: the character's TYPE, 0-3 (SaveData.charType, HunterCard.charType, the preset characters): with gender it picks the row of gLookPartMap0/3/4 (gender * 16 + it * 4) */
    u8 hornNoteSlot;            /* 0x8BD: hunting horn: the slot of the last note in hornNotes (0..7) */
    u8 hornNotes[8];            /* 0x8BE: hunting horn: the last 8 notes played (HuntingHorn_AddNote) */
    s8 aimPitchPct;             /* 0x8C6: the bow's aim pitch, -100..100 percent: Player_AddAimPitchPct turns it into aimPitch (45 deg up, 35 deg down); the arrow's rotX (attack_families shot spawn); synced online (qnet_sync k1.unk22) */
    u8 bladeSpecial;            /* 0x8C7: the weapon blade data value unk13 (Equip_GetBladeSpecial of equip[6]): hunting horn: its note set (gBladeSpecialMarks[x * 3 + i]); gunlance: the shelling type (x % 3) and level (x / 3) */
    u8 hornSong;                /* 0x8C8: hunting horn: the song the last notes matched (HuntingHorn_CheckSongs) */
    u8 teamSkill;               /* 0x8C9: the team combo skill: Player_CheckTeamCombo sets it when the four hunters' armour styles make one of gArmorCombos; Player_HasSkill and Player_GetSkill count the local player's for every hunter, in any quest (gScene.inLobby 0), not in the lobby */
    u8 teamSkillGroup;          /* 0x8CA: its skill group */
    u8 style;                   /* 0x8CB: the style (save symbolColor; Player_SetColorWord); 255 none; the lobby draws its icon (11 cycles colours) */
    s16 status6Hit;             /* 0x8CC: the hit status 6 amount: > 0 starts status6Timer unless SKILL_STATUS6_IMMUNE (Player_ReactToHit) */
    s16 status6Timer;           /* 0x8CE: status 6 frames left: 1800 at a hit (900 with SKILL_STATUS6_HALF, 3600 with SKILL_STATUS6_DOUBLE; Player_ReactToHit, Item_Use, ObjAttach_DamagePlayer), counted down (Player_UpdateStatus), cleared with the other statuses (Player_Reset, Player_StateBedLieDown); an icon (Player_UpdateStatusIcons) */
    s16 status7Hit;             /* 0x8D0: the hit status 7 amount: > 0 starts status7Timer unless SKILL_STATUS7_IMMUNE (Player_ReactToHit) */
    s16 status7Timer;           /* 0x8D2: status 7 frames left: 1800 at a hit or a poisoning item (900 with SKILL_STATUS7_HALF; Player_ReactToHit, Item_Use), counted down (Player_UpdateStatus), cleared with the other statuses; an icon (Player_UpdateStatusIcons) */
    u8 status8Hit;              /* 0x8D4: the hit status 8 amount: > 0 lowers staminaMax unless SKILL_STATUS7_IMMUNE (Player_ReactToHit) */
    u8 deliveryOpen;            /* 0x8D5: Hud_DeliveryOpen, gHudState->unk23; Quest_DeliveryMenu clears it when closing (pouch.c) */
    u8 songEffect[12];          /* 0x8D6: the level of each effect kind, PL_SONG_* (Player_HasSongEffect, Player_ClearSongEffects; only the hunting horn's songs set them) */
    s16 songEffectTimer[12];    /* 0x8E2: per kind: frames left (1800, 2700, 3600; counted down by Player_UpdateStatus) */
    s16 tradeCooldown;          /* 0x8FA: 9000 frames set at a trade when 0 (EmTrader_RunTalk), counted down by Player_UpdateStatus; the trader clears it */
    s16 hpMaxBase;              /* 0x8FC: hpMax without the song effect 3: set from hpMax, raised by Player_AddMaxHp unless temporary (to 150); restored to hpMax when effect 3 ends (Player_UpdateStatus) */
    u8 scoutSpotted;            /* 0x8FE: set once by Player_StateGestureScout, with actDetectTimer 300, when sub_main's Area_GetFlyerPos point is within about 33 degrees of the facing; Player_Reset clears it (seen; what it spots is not settled) */
    u8 inPoisonArea;            /* 0x8FF: 1 while in an area of kind 1 without SKILL_POISON_IMMUNE, which keeps poison at least 10 (Player_UpdateEnvironment clears it each frame); Player_InPoisonArea tests it offline */
    u8 unk900[0x970 - 0x900];
} Player;
typedef char Player_size_check[(sizeof(Player) == 0x970) ? 1 : -1];

/* PLAYER_SLOTS slots: Player_ClearAll clears 10, and gPlayerSys follows them. */
extern Player gPlayers[PLAYER_SLOTS];

s32 Player_InSceneEvent(void);
void Player_UpdateEnvironment(Player *pl);
s32 Player_PouchHasSelectable(Player *pl);
u8 Player_NoticePouchFull(s32 item);
s32 Player_GetStickYaw(Player *pl, s32 i);
void Player_FaceStick(Player *pl);
u8 Player_StickMoveLevel(Player *pl, s32 unused);
s16 Player_CheckRideSlip(Player *pl);
s32 Player_WallClimbInput(Player *pl, s16 noTurn);
void Player_StartSpotAction(Player *pl, s32 mode);
void Player_HoldSpecialInput(Player *pl, u8 mode);
void Player_TryGather(Player *pl, s32 mode);
s32 Player_CrouchInput(Player *pl);
void Player_GuardInput(Player *pl);
s8 Player_TurnAlongWall(Player *pl, s16 side);
s32 Player_TurnAlongWallSide(Player *pl, s16 side);
void Player_UseItem(Player *pl, u8 mode);
s32 Player_TakeOfferedItem(Player *pl);
void Player_IdleActions(Player *pl, s32 arg);
void Player_BowgunAim(Player *pl);
void Player_UpdateLean(Player *pl, s32 mot);
void Player_StateIdle(Player *pl, s32 unused);
void Player_StateMove(Player *pl, s32 variant);
void Player_StateDrawSheathe(Player *pl, u32 variant);
void Player_StateLeapOrFall(Player *pl, s32 variant);
void Player_StateClimbAirborne(Player *pl, s32 variant);
void Player_StateCrouch(Player *pl, s32 variant);
void Player_StateBedMenu(Player *pl, s32 unused);
void Player_StateToIdle(Player *pl, s32 unused);
void Player_StateThrowItem(Player *pl, s32 variant);
void Player_StateLedgeHang(Player *pl, s32 variant);
void Player_StateRideStagger(Player *pl, s32 variant);
void Player_StatePlaceItem(Player *pl, s32 variant);
void Player_StateKneelToPlace(Player *pl, s32 unused);
void Player_StateRoll(Player *pl, s32 variant);
void Player_StateStepUp(Player *pl, s32 unused);
void Player_StateDash(Player *pl, s32 variant);
void Player_StateFleeDash(Player *pl, s32 variant);
void Player_StateOutOfBreath(Player *pl, s32 variant);
void Player_StateTiredStop(Player *pl, s32 unused);
void Player_StateHardLanding(Player *pl, s32 unused);
void Player_StateWallHold(Player *pl, s32 unused);
void Player_StateWallClimbUp(Player *player, s32 variant);
void Player_StateWallClimbDown(Player *pl, s32 variant);
void Player_StateWallDismount(Player *pl, s32 unused);
void Player_StateWallGrab(Player *pl, s32 unused);
void Player_StateRunStop(Player *pl, s32 unused);
void Player_StateSetTrap(Player *pl, s32 unused);
void Player_StateSetTrapDone(Player *pl, s32 unused);
void Player_StateSideEvade(Player *pl, s32 variant);
void Player_StateBedSleep(Player *pl, s32 variant);
void Player_StateMountedGunAim(Player *pl, s32 variant);
void Player_StateMountedGunFire(Player *pl, s32 variant);
void Player_StateCrouchStop(Player *pl, s32 variant);
void Player_StepBack(Player *pl, s32 fwd);
void Player_UpdateWallSidle(Player *pl);
void Player_StateDive(Player *pl, s32 unused);
s32 Player_GatherAtSpot(Player *pl, u16 kind, u16 stat);
void Player_StateGather(Player *pl, u32 variant);
void Player_StateGatherCrouched(Player *pl, s32 variant);
void Player_UpdateFishingInput(Player *pl);
void Player_StateFishReel(Player *pl, s32 variant);
void Player_StateFishReelEmpty(Player *pl, s32 variant);
void Player_StateSpitRoast(Player *pl, s32 unused);
void Player_StateTrade(Player *pl, s32 variant);
void Player_StateInstrumentPlay(Player *pl, s32 variant);
void Player_StateInstrumentEnd(Player *pl, s32 variant);
void Player_StateSharpen(Player *pl, s32 unused);
void Player_StateLanceHop(Player *pl, s32 variant);
void Player_StateHornItemPlay(Player *pl, s32 variant);
void Player_RunActionState(Player *pl);
void Player_RunWeaponState(Player *pl);
void Player_RunReactState(Player *pl);
void Player_StateFaint(Player *pl, s32 variant);
void Player_RunFaintState(Player *pl);
void Player_StateCart(Player *pl, s32 variant);
void Player_StateLeave(Player *pl, s32 unused);
void Player_RunEventState(Player *pl);
void Player_UpdateCarryKind(Player *pl);
void Player_UpdateCarryInput(Player *pl, s32 caller);
void Player_StateCarryWalk(Player *pl, s32 unused);
void Player_StateCarryRun(Player *pl, s32 unused);
void Player_StateCarryPutDown(Player *pl, s32 variant);
void Player_StateCarryAirborne(Player *pl, s32 variant);
void Player_RunCarryState(Player *pl);
void Player_StateGestureLieDown(Player *pl, s32 unused);
void Player_StateGestureGetUp(Player *pl, s32 unused);
void Player_StateGestureScout(Player *pl, s32 variant);
void Player_StateGesture03(Player *pl, s32 variant);
void Player_StateGesture03End(Player *pl, s32 unused);
void Player_StateGesture05(Player *pl, s32 variant);
void Player_StateGesture06(Player *pl, s32 unused);
void Player_StateGesture07(Player *pl, s32 unused);
void Player_StateGesture08(Player *pl, s32 unused);
void Player_StateGestureOneShot(Player *pl, s32 variant);
void Player_StateGesture0A(Player *pl, s32 unused);
void Player_StateGesture0B(Player *pl, s32 unused);
void Player_StateGesture0C(Player *pl, s32 unused);
void Player_StateGesture10(Player *pl, s32 unused);
void Game_UpdatePlayers(void);
void Player_Update(Player *pl);
void Player_UpdateAutoTurn(Player *pl);
void Player_FindLookTarget(Player *pl);
void Player_CheckPointMessages(u8 *script);
s16 Player_CountSpecialItems(Player *pl);
void Player_CheckTeamCombo(Player *self);
void Player_CheckTeamComboLost(Player *pl);
void Player_SetEvadeFrames(Player *pl);
s32 Player_Status6Active(Player *pl);
s32 Player_Status7Active(Player *pl);

void Player_InitForQuest(Player *pl);
void Player_ClearAll(void);
void PlayerSys_Update(void);
void Player_InitAll(s16 mode);
void Player_Init(Player *pl, u8 idx, PlayerVt *table);
void Player_AddSpawnOffset(Player *pl, f32 *pos, u16 angle);
s32 Player_GetHpSkillBonus(Player *pl);
void Player_CalcSharpnessMax(Player *pl);
void Player_Reset(Player *pl);
void Player_UpdateStatus(Player *pl);
void Player_TickPaint(Player *pl);
void Player_TickTimers(Player *pl);
void Player_UpdateHitStop(void *obj);
void Player_UpdateMotionStep(Player *pl);
void Pouch_SelectFirst(Player *pl);
u8 Pouch_AddEx(Player *pl, u16 item, s16 n, u8 silent);
s32 Item_IsOnePerPouch(u16 item);
s16 Pouch_Count(Player *pl, u16 item);
s16 Pouch_Room(Player *pl, u16 item);
s16 Pouch_Space(Player *pl, u16 item);
s16 Pouch_FindSlot(Player *pl, u16 item);
s16 Pouch_CursorCount(Player *pl);
s16 Pouch_CountSelectable(Player *pl);
s16 Pouch_HasFreeSlot(Player *pl);
s16 Pouch_ClearSlot(Player *pl, s16 slot);
s32 Pouch_SetAtLeast(Player *pl, u16 mode, u16 item, s16 n);
void Player_TwistBoneToLook(Player *pl, s16 bone);
void Player_StartMoveTo(Player *pl, s16 frames);
void Player_UpdateMoveTo(Player *pl);
s16 Player_GetAreaPitModel(Player *pl);
s32 Player_IgnoresFlinch(Player *pl);
void Stats_CountItemUse(Player *pl);
void Player_AddHp(Player *pl, s16 n);
s32 Player_Damage(Player *pl, s16 damage, s16 lose);
void Player_Heal(Player *pl, s16 n, s16 quiet);
void Player_AddMaxHp(Player *pl, s16 n, u8 transient);
u8 Player_GetSharpnessLevel(Player *pl);
void Player_AddSharpness(Player *pl, s16 n);
u16 Bowgun_NextAmmoSlot(Player *pl, u16 slot, u8 dir);
void Bow_SetCoatingSlot(Player *pl);
void Bow_ClearCoatingSlot(Player *pl);
void Bowgun_LoadAmmo(Player *pl, s32 unused);
u32 Bowgun_GetAmmoMask(Player *pl);
s32 Player_GetSkillAmmoMask(Player *pl);
u8 Bowgun_ClipSize(GunData *gun, u8 kind);
void Player_TickWeaponTimer(Player *pl);
void Player_AddStamina(Player *pl, s16 n);
void Player_UseStamina(Player *pl, s32 n);
void Player_AddMaxStamina(Player *pl, s16 n);
void Player_UpdateHunger(Player *pl);
void Player_RequestGesture(u8 gesture);
void Player_StartGesture(Player *pl);

void Player_UpdateNearMonster(Player *pl);
void Player_SetPosture(Player *pl, u16 posture, s16 setFlag1, s16 setFlag2);
s32 Player_IsLocal(void *obj);
s32 Player_OnMountedGun(Player *pl);
void Player_UpdateStatusIcons(Player *pl);
void Player_CalcAttack(Player *pl);
void Player_CalcAttackRemote(Player *pl);
void Player_CalcDefense(Player *pl);
void Player_CalcDefenseRemote(Player *pl);
void Player_CalcResists(Player *pl);
void Player_CalcResistsRemote(Player *pl);
s8 Equip_GetSkillPoints(EquipItem *piece, u8 group);
s8 Equip_TorsoUpBonus(EquipItem *piece);
s8 Player_GetTorsoSkillPoints(Player *pl, u8 group);
s8 Player_GetSkillPoints(Player *pl, u8 group);
void Player_CalcSkills(Player *pl);
s32 Player_HasSkill(Player *pl, u8 skill);
s32 Player_CanGuard(Player *pl);
void Player_StartMotionLayer(Player *pl, u16 motion, s32 blend, s16 frame, s32 layer);
void Player_RunInputHook(Player *pl);
s32 Player_StartJump(void *obj, s16 frames);
void Player_Move(void *obj);
void Player_MoveAccel(Player *pl);
void Player_Integrate(void *obj);
s32 Player_MoveFall(Player *pl);
void Player_StopMove(void *obj);
void Player_ClearVelocity(void *obj);
void Player_ClearAccel(Player *pl);
void Player_ClearAccel2(void *obj);
void Player_UpdateMotionTime(Player *pl, s16 step);
void Motion_InitTime(void *obj, s16 step);
void Player_StartNextHit(Player *unused0, u8 unused1);
s32 Player_FloorAheadAbove(Player *pl, f32 *out, f32 fwd, f32 rise, f32 range);
s32 Player_FloorAheadBelow(Player *pl, u16 above, f32 fwd, f32 rise);
s32 Obj_SameArea(void *obj, Player *other);
u16 Player_GetGatherPoint(Player *pl, u16 *action, f32 *pos);

void HuntingHorn_StateSwingLeft(Player *pl, s32 variant);
void HuntingHorn_StateSwingRight(Player *pl, s32 unused);
void HuntingHorn_StateSwingDown(Player *pl, s32 unused);
void HuntingHorn_StateSwingDownNext(Player *pl, s32 variant);
void HuntingHorn_StateBounce(Player *pl, s32 variant);
void HuntingHorn_StatePlayAttack(Player *pl, s32 variant);
void HuntingHorn_StateSlam(Player *pl, s32 unused);
void HuntingHorn_RunState(Player *pl);
s32 HuntingHorn_AttackInput(Player *pl);
void HuntingHorn_CheckInput(Player *pl, u32 caller);
void HuntingHorn_PlayInput(Player *pl, s32 stance);
void HuntingHorn_DrawAttack(Player *pl);
void HuntingHorn_StartPlaying(Player *pl, s32 sheathed);
u8 HuntingHorn_PlayingNote(Player *pl);
void HuntingHorn_PlaySong(Player *p, u8 song, u8 quiet);
void HuntingHorn_SongFxAllies(Player *p);
void HuntingHorn_SongFxSelf(Player *pl);
void HuntingHorn_CheckSongs(Player *pl);
void HuntingHorn_AddNote(Player *pl, u8 note);
void HuntingHorn_ClearNotes(Player *pl);
s16 HuntingHorn_PlayKind(Player *pl);
u8 HuntingHorn_PlayingNoteColour(Player *pl);
void Gunlance_StateThrust(Player *pl, s32 variant);
void Gunlance_StateShell(Player *pl, s32 variant);
void Gunlance_StateGuardThrust(Player *pl, s32 variant);
void Gunlance_StateWyvernFire(Player *pl, s32 variant);
void Gunlance_StateUpperThrust(Player *pl, s32 unused);
void Gunlance_StateLunge(Player *pl, s32 variant);
void Gunlance_StateBounce(Player *pl, s32 variant);
void Gunlance_RunState(Player *pl);
s32 Gunlance_AttackInput(Player *pl);
s32 Gunlance_CheckInput(Player *pl, s32 caller);
s32 Gunlance_GuardInput(Player *pl, s32 unused);
void Gunlance_DrawAttack(Player *pl);
s32 Bow_ArcVisible(void *obj, u16 ignoreButton);
u8 Bow_ChargeLevel(Player *pl);
void Bow_UpdateAim(Player *pl, s32 layers);
void Bow_StateFire(Player *pl, s32 variant);
void Bow_StateCharge(Player *pl, u32 variant);
void Bow_StateStab(Player *pl, s32 variant);
void Bow_StateAim(Player *pl, s32 variant);
void Bow_StateChargeWalk(Player *pl, s32 unused);
void Bow_StateEvade(Player *pl, s32 variant);
void Bow_StateLoadCoating(Player *pl, s32 unused);
void Bow_RunState(Player *pl);
s32 Bow_AttackInput(Player *pl, s32 needPress);
s32 Bow_CheckInput(Player *pl, s32 caller);
void Bow_DrawAttack(Player *pl);
void Bow_DrawToAim(Player *pl);
void Bowgun_FireShot(Player *pl, s32 ammo, s32 bone, f32 scale);
void BowgunShot_Create(Player *pl, u8 kind, s16 bone, f32 scale);

void GunFx_Update(StageObj *obj);
void GunFx_Setup(StageObj *obj);
s32 GunFx_HermitaurSpraying(Em *em);
void GunFx_Run(StageObj *obj);
void GunFx_End(StageObj *obj);
void GunFx_Free(StageObj *obj);
void GunFx_Draw(DrawSlot *node);
void GunFx_CreateShell(void *owner, u8 shellLevel);
void GunFx_CreateReload(void *owner, u8 casingType);
void GunFx_CreateType4(void *owner);
void GunFx_CreateType5(void *owner);
void GunFx_CreateType6(void *owner);
void VersusBait_Setup(StageObj *obj);
void VersusBait_Run(StageObj *obj);
void VersusBait_End(StageObj *obj);
void VersusBait_Free(StageObj *obj);
void VersusBait_Draw(DrawSlot *node);
void VersusBait_Create(Player *thrower, Em *em, u8 baitType, s32 unused, f32 *throwPoint);
void VersusBait_Move(StageObj *obj);
void TeostraBlastCharge_Setup(StageObj *obj);
void TeostraBlastCharge_Run(StageObj *obj);
s32 TeostraBlastCharge_AllGustsDone(StageObj *obj);
void TeostraBlastCharge_End(StageObj *obj);
void TeostraBlastCharge_FreePuffs(StageObj *obj);
void TeostraBlastCharge_Free(StageObj *obj);
void TeostraBlastCharge_Draw(DrawSlot *node);
void TeostraBlastCharge_Create(Em *em);
void PlayerFx_Setup(StageObj *obj);
void PlayerFx_Run(StageObj *obj);
void PlayerFx_End(StageObj *obj);
void PlayerFx_Free(StageObj *obj);
void PlayerFx_Draw(DrawSlot *node);
void PlayerFx_PlayStartSound(StageObj *obj);
s32 PlayerFx_OwnerGone(void *owner);
void PlayerFx_CreateOnPlayerDefault(Player *player, u8 fxType, f32 scale);
void PlayerFx_CreateOnPlayer(Player *player, u8 fxType, s32 variant, f32 scale);
void PlayerFx_CreateAtDefault(f32 *pos, u8 fxType, f32 scale);
void PlayerFx_CreateAt(f32 *src, u8 fxType, u8 variant, f32 scale);
void PlayerFx_TickStatus6(Player *player);
void PlayerFx_TickStatus7(Player *player);
void BowArc_Setup(StageObj *obj);
void BowArc_Run(StageObj *obj);
void BowArc_End(StageObj *obj);
void BowArc_Free(StageObj *obj);
void BowArc_Draw(DrawSlot *node);
void BowArc_Create(void *owner);

void Player_ClearPartOverrides(Player *pl);
void Player_ForceState(void *obj, s32 state, s32 sub);
void Player_RequestState(Player *pl, u16 state, u8 sub, u16 flags);
void Player_RequestStateSynced(Player *pl, u16 state, u8 sub, u16 flags);
u32 Player_TestFlag(void *obj, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);
void Player_SetPostureOnly(Player *pl, u8 posture);
s32 Player_TestFlags(void *obj, s32 mask);
s32 Player_TestSetFlags(void *obj, s32 mask);
void Player_SetAirIdle(Player *pl, s16 blend, s32 variant);
void Player_StartIdleMotion(Player *pl, s32 blend, s32 startFrame);
void Player_SetIdle(Player *pl, s32 blend, s32 startFrame, s32 quiet);
void Player_ClearActionState(Player *pl);
void Player_ReturnToIdle(Player *pl, s32 posture, s32 blend, s32 startFrame);
void Player_ReturnToIdleKeepParts(Player *pl, s32 posture, s32 blend, s32 startFrame);
void Player_UpdateInput(void);
void Player_CopyPadInput(s32 playerNo);
void Player_PlaySeAtSelf(u8 *obj, s32 seId);
void GreatSword_StateSweepLeft(Player *pl, s32 variant);
void GreatSword_StateSweepRight(Player *pl, s32 variant);
void GreatSword_StateOverhead(Player *pl, u32 variant);
void GreatSword_StateBounce(Player *pl, s32 unused);
u8 GreatSword_ChargeLevel(Player *pl);
void GreatSword_StateCharge(Player *pl, u32 variant);
void GreatSword_SetChargeAttack(Player *pl);
void GreatSword_StateChargeSlash(Player *pl, s32 variant);
void GreatSword_RunState(Player *pl);
s32 GreatSword_AttackInput(Player *pl);
void GreatSword_CheckInput(Player *pl, s32 caller);
void GreatSword_DrawAttack(Player *pl);
void Bowgun_StateFire(Player *pl, s32 row, s32 variant);
void Bowgun_StateReload(Player *pl, s32 unused);
void Bowgun_StateDrawReload(Player *pl, s32 variant);
void Bowgun_StateDryFire(Player *pl, s32 unused);
void Bowgun_RunState(Player *pl);
s32 Bowgun_AttackInput(Player *pl);
void Bowgun_DrawAttack(Player *pl, s32 moving);
void Hammer_StateDownAttack(Player *pl, s32 unused);
void Hammer_StateCombo1(Player *pl, s32 variant);
void Hammer_StateCombo2(Player *pl, s32 variant);
void Hammer_StateCharge(Player *pl, s32 variant);
void Hammer_StateChargeSpin(Player *pl, s32 unused);
void Hammer_StateChargeRelease(Player *pl, s32 variant);
void Hammer_StateChargeRelease2(Player *pl, s32 unused);
void Hammer_StateFinisher(Player *pl, s32 variant);
void Hammer_StateBounce(Player *pl, s32 unused);
void Hammer_RunState(Player *pl);
s32 Hammer_AttackInput(Player *pl);
void Hammer_CheckInput(Player *pl, s32 caller);
void Hammer_CheckChargeInput(Player *pl);
void Hammer_DrawAttack(Player *pl);
void Hammer_ChargeFromRoll(Player *pl);
void Hammer_StartCharge(Player *pl);
void Lance_StateThrust(Player *pl, s32 variant);
void Lance_StateUpperThrust(Player *pl, s32 variant);
void Lance_StateCharge(Player *pl, s32 variant);
void Lance_StateChargeFinish(Player *pl, s32 unused);
void Lance_StateBounce(Player *pl, s32 unused);
void Lance_RunState(Player *pl);
s32 Lance_AttackInput(Player *pl);
void Lance_CheckInput(Player *pl, s32 variant);
void Lance_GuardAttack(Player *pl, s32 thrust);
void Lance_DrawAttack(Player *pl);
s32 SwordShield_CheckHit(Player *pl);
void SwordShield_StateCombo(Player *pl, s32 variant);
void SwordShield_StateRisingSlash(Player *pl, s32 variant);
void SwordShield_StateBounce(Player *pl, s32 unused);
void SwordShield_RunState(Player *pl);
s32 SwordShield_AttackInput(Player *pl);
void SwordShield_CheckInput(Player *pl, s32 unused);
void SwordShield_DrawAttack(Player *pl);
void SwordShield_GuardSlash(Player *pl);
void SwordShield_EvadeAttack(Player *pl);
s32 DualSwords_CheckHit(Player *pl);
void DualSwords_StateCombo(Player *pl, s32 variant);
void DualSwords_StateSideSlash(Player *pl, s32 variant);
void DualSwords_StateRisingSlash(Player *pl, s32 variant);
void DualSwords_StateDemonToggle(Player *pl, s32 variant);
void DualSwords_StateDemonSideSlash(Player *pl, s32 variant);
void DualSwords_StateBounce(Player *pl, s32 unused);
void DualSwords_RunState(Player *pl);
void DualSwords_ComboInput(Player *pl, s32 sector, s32 linked);
s32 DualSwords_AttackInput(Player *pl);
void DualSwords_CheckInput(Player *pl, s32 variant);
void DualSwords_DrawAttack(Player *pl);
void DualSwords_EvadeAttack(Player *pl);
void DualSwords_DemonToggle(Player *pl, s32 end);
void LongSword_StateSlash(Player *pl, s32 variant, s32 stepForward);
void LongSword_StateSlash2(Player *pl, s32 variant);
void LongSword_StateFadeSlash(Player *pl, u32 variant);
void LongSword_StateBounce(Player *pl, s32 unused);
void LongSword_StateThrust(Player *pl, s32 unused);
void LongSword_StateSpirit1(Player *pl, u32 variant);
void LongSword_StateSpiritThrust(Player *pl, s32 unused);
void LongSword_StateSpirit2(Player *pl, s32 variant);
void LongSword_StateSpiritRising(Player *pl, s32 unused);
void LongSword_StateRisingSlash(Player *pl, s32 variant);
void LongSword_StateSpirit3(Player *pl, s32 variant);
void LongSword_StateExtraSlash(Player *pl, s32 unused);
void LongSword_RunState(Player *pl);
s32 LongSword_AttackInput(Player *pl);
s32 LongSword_ReadAttackKey(Player *pl);
void LongSword_EvadeInput(Player *pl);
u8 LongSword_ComboInput(Player *pl, u32 row, u8 key, u8 peek);
void LongSword_DrawAttack(Player *pl);
void LongSword_SpiritDraw(Player *pl);
void Player_TurnAndSetState(Player *pl, u8 group, u8 state, s32 mode, u8 turn);
s32 HuntingHorn_CheckHit(Player *pl);
void Player_StickTurnMove(Player *pl, s16 maxTurn, f32 dist);
void HuntingHorn_StatePlay(Player *pl, s32 stance, s32 variant);

s32 Player_Status5Full(Player *pl);
s32 Player_HasMashBuildup(Player *pl);
s32 Player_InMashStatus(Player *pl);
void Player_AddPoison(Player *pl, s16 amount);
s32 Player_InBlownAway(Player *pl);
void Player_Die(Player *pl);
s32 Player_InGuardArc(u16 angle, u16 hitAngle);
void Player_ReactToHit(Player *pl);

void Player_DrawHeldProp(void *obj);
void Player_DrawWeapon(Player *pl, s32 otherPlayer, f32 alpha);
u8 Player_PartFlags(Player *pl, u8 slot);
u32 Color_Cycle8(void);
void Player_SetLight(Player *pl, u8 slot, s32 setIndex, s32 matIndex, ModelMat *mat, f32 matAlpha);
void Player_DrawLocal(Player *pl, s32 unused);
void Player_DrawHeldItem(void *obj);
void Player_Draw(Player *pl, s32 unused);
void CharMake_DrawHunter(Player *pl, s32 unused);
void EquipPreview_Reset(void);
void EquipPreview_Draw(void);
void Player_UpdateWorldMatrix(Player *pl);

void PlayerSe_AtFrame(Player *pl, s32 frame, s32 seId);
void Motion_SeAtFrame(Player *pl, s32 frame, s32 seId);
void Motion_SeAtFrameLocal(Player *pl, s32 frame, s32 seId);
void Player_SeAtFrame(Player *pl, s32 frame, s32 seId);
void PlayerSe_WeaponAtFrame(Player *pl, s32 frame, s32 seId);
void Motion_VoiceAtFrame(Player *pl, f32 frame, s32 voice);
void PlayerSe_VoiceAtFrame(Player *pl, f32 frame, s32 voice);
void PlayerSe_OnlineAtFrame(Player *pl, s32 frame, s32 seId);
void PlayerSe_EquipAtFrame(Player *pl, f32 frame, s32 kind);
void Player_MotionSounds(Player *pl, PlayerWork *work);
void Player_NoHandler(Player *pl);
void Player_MotionFxStep(Player *pl);
void Player_MotionEffects(Player *pl, PlayerWork *work);

void Player_LoadLookModels(Player *pl);
s32 Player_QueueLookFiles(Player *pl);
void Player_InitRecords(Player *pl);
void Obj_ShowMeshInRange(u8 *obj, u8 kind, u8 sel);
void FxBlock_Clear(EquipFxSwing *swing);
void Player_ClearFxBlock(Player *pl);
void Players_RunEquipEffects(void);
s32 EquipFx_ChainUpdate(EquipFxSwing *swingsIn, EmBone *baseBoneIn, EmBone *pushBoneIn, EmBone *bonesIn, f32 **list, s16 countIn);
void Player_RunEquipEffects(Player *pl);
void Effect_RunFixedList(EquipFxSwing *swing, EmBone *baseBone, EmBone *pushBone, EmBone *bones);

#endif /* PLAYER_H */
