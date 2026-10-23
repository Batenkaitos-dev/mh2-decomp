#ifndef NPC_MOTION_H
#define NPC_MOTION_H
#include "em.h"

/*
 * The motions of the townspeople's own models. A townsperson's body
 * (NpcWork.bodyKind, the area entry's model set) picks its model and
 * motion file:
 * - body 0, the free townspeople, uses the hunters' motions (player.h's
 *   PL_MOT_*, ids below 1000);
 * - bodies 1, 2 and 3 each have a motion file of their own, so the same
 *   number is a different motion in each. Body 1 is the stationed
 *   townspeople (the smith, the workshop helpers, the desk clerks), body 2
 *   the service townspeople (the ServiceNpc_ states: the hairdresser, the
 *   kitchen helpers), body 3 the pets (the Pet_ states).
 * Each name is a motion's row in its file: Em.motionId[0] and the calls
 * that set a motion take EM_MOTION_ID(0, NAME) (em.h), 1000 + the row.
 * The names come from the states that play the motions; a row no state
 * explains keeps its number where it is used.
 */

/* Body 3, the pets (the Poogie, the care pet, the house pet). */
enum PetMotion {
    PET_MOT_IDLE = 1,           /* stands: Pet_StateIdle, and after each other motion */
    PET_MOT_TURN_TO_PLAYER = 2, /* turns to the player who touched it or opened its menu (Pet_StatePetted, Pet_StateFace) */
    PET_MOT_WALK = 3,           /* walks its route or follows the player (Pet_StateWander, Pet_StateFollow) */
    PET_MOT_WAIT_A = 6,         /* one of the three waiting motions Pet_PickWaitMotion draws */
    PET_MOT_STOP = 7,           /* stops on the way: the wander's stops, after its joy, at the dig spot */
    PET_MOT_WAIT_B = 9,
    PET_MOT_STEP_BACK = 10,     /* backs off from the player after a petting (Pet_StatePetted) */
    PET_MOT_JOY = 11,           /* its joy (Pet_StateJoy) */
    PET_MOT_CHARGE = 14,        /* rushes at the player at three times the speed (Pet_StateAngry) */
    PET_MOT_RUN = 15,           /* runs at double speed (Pet_StateRunOff, Pet_StateRunRoute, Pet_StateDig) */
    PET_MOT_TACKLE = 18,        /* knocks the player down (Pet_StateAngry: the player's TOWN_PC_THROWN) */
    PET_MOT_WAIT_C = 20,
    PET_MOT_STAND_UP = 73,      /* gets up after sitting (Pet_StateSit, Pet_StateWait) */
    PET_MOT_SIT = 74,           /* sits down; seated from frame 88 (Pet_StateSit, Pet_StateHouseOpen) */
    PET_MOT_GREAT_JOY = 75      /* its great joy, as the Poogie comes back from an errand (Pet_StateGreatJoy) */
};

/* Body 2, the service townspeople. Look 82 (Npc_IsType52) has its own
 * idle, walk and talk (the _ALT_ names). */
enum ServiceNpcMotion {
    SERVICE_MOT_IDLE = 1,           /* stands (ServiceNpc_StateIdle) */
    SERVICE_MOT_WAIT = 2,           /* one of the idle motions while the player is in the hair menu (ServiceNpc_StateHairWait) */
    SERVICE_MOT_STEP_IN_PLACE = 3,  /* walks in place; its root motion is never applied (ServiceNpc_StateStepInPlace) */
    SERVICE_MOT_FIDGET = 4,         /* a fidget after the idle's wait (ServiceNpc_StateFidget) */
    SERVICE_MOT_WALK = 11,          /* walks to its route point (ServiceNpc_StateWalk, the route's state 3) */
    SERVICE_MOT_WALK_ALT = 12,      /* the route's state 2 walk; the code doesn't show how it differs */
    SERVICE_MOT_RUN = 15,           /* the route's state 4 */
    SERVICE_MOT_WALK_CARRY = 16,    /* walks carrying its cooking lesson object (the route's carrying state) */
    SERVICE_MOT_HAIR_PREPARE = 36,  /* brings out the hair menu's object at frame 48 (ServiceNpc_StateHairPrepare) */
    SERVICE_MOT_FINISH = 38,        /* the end of a service: after the bait, the haircut, the steps in place */
    SERVICE_MOT_TALK = 70,          /* held while a talk lasts (ServiceNpc_StateTalk) */
    SERVICE_MOT_GREET = 71,         /* nods to a player who comes near; also a talk's start (ServiceNpc_StateGreet) */
    SERVICE_MOT_BUSY = 73,          /* busy: doesn't turn, and a talk ends at once (ServiceNpc_StateBusy) */
    SERVICE_MOT_ALT_TALK = 76,
    SERVICE_MOT_ALT_WALK = 77,
    SERVICE_MOT_ALT_IDLE = 78,
    SERVICE_MOT_GREET_EFFECT = 79,  /* a greeting with an effect and a sound at frame 62 (ServiceNpc_StateGreetFx) */
    SERVICE_MOT_HAIR_WAIT = 81,     /* the hairdresser waits while the player gets ready; also its talk */
    SERVICE_MOT_HAIR_CUT = 84,      /* the haircut: the barber's particles at frame 120, the hair's at 200 (ServiceNpc_StateHairDo) */
    SERVICE_MOT_HAIR_CUT_NEXT = 85, /* the haircut's second part */
    SERVICE_MOT_HAIR_DONE = 86,     /* the hairdresser's finish (NPC_HAIRDRESSER; the others play SERVICE_MOT_FINISH) */
    SERVICE_MOT_BAIT_OFFER = 87,    /* held while the bait menu is open (ServiceNpc_StateBaitOffer) */
    SERVICE_MOT_BAIT_OFFER_END = 88 /* leaves the bait pose */
};

/* Body 1, the stationed townspeople. The BUSY_ names are the work poses of
 * the NPC_STATE_BUSY_* states, each with its talk; look 73 (Npc_IsType49,
 * NPC_NAMING_TALKER) has its own walk and talk. */
enum StationedNpcMotion {
    STATIONED_MOT_IDLE = 1,                 /* stands (Npc_StateIdle) */
    STATIONED_MOT_TALK_START = 7,           /* turns to the player as a talk starts; the paired townspeople's chat gesture */
    STATIONED_MOT_TALK = 8,                 /* held while a talk lasts (Npc_StateTalk) */
    STATIONED_MOT_WALK = 13,                /* walks to its route point (Npc_StateWalk) */
    STATIONED_MOT_BUSY_POSE = 15,           /* NPC_STATE_BUSY_POSE's pose */
    STATIONED_MOT_BUSY_POSE_TALK_START = 16,
    STATIONED_MOT_BUSY_POSE_TALK_TURN = 17, /* between the start and the talk; also some time after the talk */
    STATIONED_MOT_BUSY_POSE_TALK = 18,
    STATIONED_MOT_BUSY_LOOP = 19,           /* NPC_STATE_BUSY_LOOP's loop */
    STATIONED_MOT_BUSY_LOOP_TALK_START = 20,
    STATIONED_MOT_RUN = 21,                 /* runs to its route point (Npc_StateRun) */
    STATIONED_MOT_BUSY_IDLE = 23,           /* NPC_STATE_BUSY_IDLE's hold */
    STATIONED_MOT_BUSY_IDLE_FIDGET = 24,    /* once after a random wait */
    STATIONED_MOT_BUSY_TALK_START = 25,
    STATIONED_MOT_BUSY_TALK_TURN = 26,
    STATIONED_MOT_BUSY_TALK = 27,
    STATIONED_MOT_SMITH_IDLE = 30,          /* the smith and the workshop helpers stand */
    STATIONED_MOT_SMITH_IDLE_ALT = 31,      /* the smith's other stand, drawn at random or while its partner stands */
    STATIONED_MOT_SMITH_PARTNER_CUE = 32,   /* as the partner's hammering nears its end */
    STATIONED_MOT_SMITH_WAIT_ABOVE = 33,    /* the smith waits up high (Npc_StateSmithMeetAbove) */
    STATIONED_MOT_SMITH_TALK = 34,          /* turns to the player and talks (Npc_StateSmithTalk) */
    STATIONED_MOT_FORGE_START = 35,         /* forging's first strike */
    STATIONED_MOT_FORGE_HAMMER = 36,        /* forging's strike, played twice; a spark at frame 96 */
    STATIONED_MOT_FORGE_LAST = 37,          /* forging's last strike; a spark at frame 28 */
    STATIONED_MOT_FORGE_END = 38,           /* turns back to the player after forging */
    STATIONED_MOT_SMITH_LEAVE = 39,         /* leaves before it is moved to the ground */
    STATIONED_MOT_SMITH_ARRIVE_ABOVE = 40,  /* appears up high */
    STATIONED_MOT_SMITH_ARRIVE_BELOW = 41,  /* appears on the ground */
    STATIONED_MOT_FORGE_HELPER_TURN = 50,   /* the workshop helper's, between its strikes and its last */
    STATIONED_MOT_NAMING_TALKER_WALK = 55,
    STATIONED_MOT_NAMING_TALKER_TALK_START = 56,
    STATIONED_MOT_IDLE_B = 57,              /* Npc_State146's idle, three to six times over */
    STATIONED_MOT_IDLE_B_BREAK = 58,        /* once between those runs */
    STATIONED_MOT_IDLE_B_TALK_START = 59,   /* that townsperson's talk start (Npc_StateWaitTownFree) */
    STATIONED_MOT_RAID_WAIT = 60,           /* held while the elder raid is on (Npc_StateIdleByTime) */
    STATIONED_MOT_IDLE_PM = 61,             /* the afternoon's idle (Npc_StateIdleByTime) */
    STATIONED_MOT_IDLE_PM_BREAK = 62,       /* once after the afternoon's idle */
    STATIONED_MOT_NAMING_TALKER_TALK = 65
};

void Pet_RunMotionFx(Em *npc);
void Pet_SpawnShadow(Em *npc);
void ServiceNpc_RunMotionFx(Em *npc);
void ServiceNpc_SpawnShadow(Em *npc);
void ServiceNpc_NopHandler(void);
void StationedNpc_RunMotionFx(Em *npc);
void StationedNpc_SpawnShadow(Em *npc);
void StationedNpc_NopHandler(void);

void Npc_PlayTownSeAt(Em *npc, s32 frame, s32 id);
void Npc_PlayCommonSeAt(Em *npc, s32 frame, s32 id);
void Npc_RunMotionSounds(Em *npc);

#endif
