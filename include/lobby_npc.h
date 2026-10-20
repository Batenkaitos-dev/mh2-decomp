#ifndef LOBBY_NPC_H
#define LOBBY_NPC_H

#include "common.h"

/*
 * A route entry (0x14 bytes): the point, the state to enter there
 * (Npc_EnterRoutePoint; 0 ends the route and it starts again) and an
 * argument: the frames to wait there, -1 no wait, -2 until the Em timer
 * +0x154 runs out (Npc_UpdateState); for state 0x83 the next route
 * (Npc_StateRouteJump) and for 0x81 an angle.
 */
typedef struct NpcPos {
    f32 v[3];
} NpcPos;
typedef struct NpcRouteEnt {
    /* 0x00 */ NpcPos pos;
    /* 0x0C */ s32 state;
    /* 0x10 */ s32 arg;
} NpcRouteEnt;

/* A line on the town's ground, from (x1, z1) to (x2, z2) (gNpcSideLines):
 * Npc_PlayerAboveLine tells which side of it the local player is on. */
typedef struct NpcSideLine {
    f32 x1, z1, x2, z2;
} NpcSideLine;

/* Two townspeople who take turns by a line (gNpcSidePairs, ended by line
 * 0xFFFF): Npc_PairIdBySide picks idA when the player is above the line,
 * idB below. */
typedef struct NpcSidePair {
    u8 idA;
    u8 idB;
    u16 line;           /* the index of its line in gNpcSideLines */
} NpcSidePair;

/*
 * The townspeople of the lobby are Em slots (include/em.h: gEms, 20 of
 * them) whose monster fields from +0x438 on are used as this work block
 * (Npc_Update runs them). The first fields (+0x14 group, +0x15 state, +0xA4
 * yaw, +0xAC position) are shared with the players, so the actor core
 * (Actor_SetState, Actor_SetMotion ...) takes either. Em +0x1B is the
 * townsperson's type (Npc_IsMusicActor ...; Npc_UpdateState picks the state
 * handlers by it).
 *
 * The files take the block as `NpcWork *m = (NpcWork *)(em + 0x438)`; the
 * accesses through the Em pointer (`*(s16 *)(a + 0x474)`) are left as byte
 * offsets: Em is shared with the game overlay's monsters.
 */
typedef struct NpcWork {
    /* 0x00 */ u8 id;           /* which townsperson (Npc_Find, Npc_IsPresent) */
    /* 0x01 */ u8 voice;            /* the first of its two voice sounds (Npc_PlayVoice adds 0 or 1) */
    /* 0x02 */ u8 noTalk;       /* 1: the player can't talk to it or lock on to it (TownPc_HandleInput, the lock-on); cleared by Npc_UpdateState each frame, set again by the states that keep it busy */
    /* 0x03 */ u8 talked;       /* 0 until the player first talks to it (the talk then takes its first-time line and sets it); cleared for every townsperson by Npc_ClearAllTalked (a new day) */
    /* 0x04 */ u8 offerMark;    /* the offer mark over it (Npc_RefreshOffers): 0 none, 1 an offer, 2 a ready one */
    /* 0x05 */ u8 unk5[0x8 - 0x5];
    /* 0x08 */ char *name;      /* its name: the area table's, or the Poogie's name (SaveData.poogieName) for townsperson 0x14 (Npc_SetupFromArea) */
    /* 0x0C */ NpcRouteEnt *route; /* the route (Em + 0x444) */
    /* 0x10 */ u8 unk10[0x14 - 0x10];
    /* 0x14 */ f32 step[3];     /* Npc_StartMoveTo: the move per frame (Em + 0x44C) */
    /* 0x20 */ NpcPos followSpot; /* a pet's goal point: a spot near the player it follows or the talker (Pet_StateFollow, Pet_StateFollowTalker); Pet_StateSit gets up when the player is 130 from it */
    /* 0x2C */ f32 headYaw;      /* the head turn toward the local player, in degrees within +-10 or +-20, eased by 0.2 a frame (Npc_UpdateMatrix) */
    /* 0x30 */ s32 wait;        /* frames spent at the route point (Npc_UpdateState) */
    /* 0x34 */ s16 turnDelta;   /* the angle left to turn (Npc_TurnToPlayer, Npc_StartTurnTo) */
    /* 0x36 */ s16 homeYaw;     /* the yaw to turn back to (Npc_StartTurnBack) */
    /* 0x38 */ s16 turnCount;   /* frames of turning left (Npc_StepTurn) */
    /* 0x3A */ s16 turnStep;    /* the turn per frame */
    /* 0x3C */ s16 routeIdx;    /* the current route point (Em + 0x474) */
    /* 0x3E */ u16 moveCount;   /* Npc_StartMoveTo: frames left (Em + 0x476) */
    /* 0x40 */ u16 motionPhase;     /* a motion's phase: 0, then 1 at frame 30, ended at 118 (Npc_IdleWorkshopHelper, Npc_SmithForgeAlone), 2 while the smith moves; also a greeting given (ServiceNpc_StateIdle) and the pets' frame counts (Pet_StateSit, Pet_StateFollow) */
    /* 0x42 */ union {
        u16 partSet;            /* the service townspeople (Npc_IsAct51to55): which of model parts 1-3 is drawn, 0-2 (Npc_Draw; ServiceNpc_StateIdle sets 0, ServiceNpc_StateBusy 2) */
        u16 petCount;           /* the pets: a frame count (frames pushed against a wall in Pet_StateFollow, over 120 it sits; Pet_StateWait's waits) */
    };
    /* 0x44 */ u8 petAct;           /* what a pet is busy with (PET_ACT_*): its menus wait for PET_ACT_NONE and clear it as they end */
    /* 0x45 */ u8 keepFacing;       /* 1: talked to, it neither turns to the player nor changes its pose (Npc_StateTalk); set by the seated, posing and working states, cleared by walking */
    /* 0x46 */ s8 fxStep;           /* NpcSetA/B/C_Run: 0, then 1 (the motion effects run: NpcSetA/B/C_MotionFx) */
    /* 0x47 */ u8 unk47;
    /* 0x48 */ u8 presence;         /* the area entry's presence rule (NpcAreaEntry.presence) */
    /* 0x49 */ u8 leaving;          /* set as the townsperson leaves the area (Npc_StateLeave, the swap with its pair); its time-of-day route changes stop */
    /* 0x4A */ u8 bodyKind;         /* its body: the area entry's modelSet (Npc_SetupFromArea), also Em.type and Em.modelSlot; 1 a stationed townsperson (not Npc_IsFree) */
    /* 0x4B */ u8 look;             /* its look: the area entry's type, also Em.variant and Em.npcLook; 0x4D-0x55 are the special ones (not Npc_IsFree) */
    /* 0x4C */ u8 holdRoute;    /* nonzero: Npc_UpdateState stays at the route point */
    /* 0x4D */ u8 unk4D[0x50 - 0x4D];
    /* 0x50 */ f32 markAnchor[3]; /* where the offer mark hangs: the position of bone 0x14 (free), 0xA (looks 0x51-0x55) or 0xC (body 1), else 200 * scaleY above it (Npc_Init); NameTags_DrawNpcs keeps the mark there while the bone moves less than 20, and stores the bone each frame */
    /* 0x5C */ union {
        s32 partnerId;          /* a side pair's townsperson parked as it leaves: its partner's id (Npc_CheckSideSwap) */
        s32 warpTimer;          /* the pets: 200 when it is put back at an entry point far from its spot, counted down each frame; while nonzero it skips the wall collision (Pet_StateFollow) */
    };
} NpcWork;
typedef char NpcWork_size_check[(sizeof(NpcWork) == 0x60) ? 1 : -1];

/* A townsperson's entry in an area set's table (gAreaNpcTables[set],
 * gAreaNpcCounts[set] of them; 0x48 bytes). Npc_SetupFromArea copies it into
 * the Em and its NpcWork. */
typedef struct NpcAreaEntry {
    /* 0x00 */ u8 id;               /* the townsperson's id (NpcWork.id) */
    /* 0x01 */ u8 type;             /* its type: Em.variant, NpcWork.look and its colour set (gNpcColorSets) */
    /* 0x02 */ u8 pad2[2];
    /* 0x04 */ s32 yaw;             /* the start yaw (Em.rotY), kept until a turn point of the route gives another */
    /* 0x08 */ f32 scale;
    /* 0x0C */ f32 motionSpeed;     /* the motion speed (Em.motionSpeed) */
    /* 0x10 */ void *route;         /* its route (an NpcRouteEnt list) */
    /* 0x14 */ u8 modelSet;         /* its model and handler set: Em.type, Em.modelSlot and NpcWork.bodyKind (1: a stationed townsperson, not free) */
    /* 0x15 */ u8 presence;         /* its presence rule (NpcWork.presence): 0 always, 1 mornings, 2 afternoons, 4 on the player's side of its pair */
    /* 0x16 */ char name[0x32];     /* its name (NpcWork.name) */
} NpcAreaEntry;
typedef char NpcAreaEntry_size_check[(sizeof(NpcAreaEntry) == 0x48) ? 1 : -1];

/* A colour for a part's materials (a list per townsperson type, ended by part -1):
 * sub -1 colours all of the part's materials with color, then entries for
 * single materials follow. */
typedef struct NpcColor {
    /* 0x0 */ s16 part;
    /* 0x2 */ s16 sub;
    /* 0x4 */ u32 color;
} NpcColor;

/* Two townspeople who take turns at one place: at most one of them is in town
 * (gAreaNpcSpawns[area]: the pairs of an area set, ended by NPC id 0xFF). */
typedef struct NpcPair {
    u8 first;
    u8 second;
} NpcPair;

/* A townsperson id (NpcWork.id, Npc_Find): the house's Poogie, whose name
 * is the one the player gave it and whose talk opens the pet menu. */
#define NPC_POOGIE              0x14
/* Townspeople the action button's talk acts on (TownPc_HandleInput). */
#define NPC_CONTEST_HOST        0x05    /* offline, the arm-wrestling contest is held with it (Contest_Open) */
#define NPC_DRINK_CONTEST_HOST  0x10    /* offline in the afternoon, a tavern seat opens its drinking contest */
#define NPC_POOGIE_CARE         0x22    /* the care pet (in AREA_HUB_0x83): its talk opens the care menu (PoogieMenu_OpenCare) */
#define NPC_HOUSE_PET           0x6D    /* the house pet (in AREA_HOUSE and AREA_HUB_0x57): its talk starts a petting (HousePet_Open) */
#define NPC_NAMING_TALKER       0x12    /* the townsperson who talks the player through the Poogie's naming (PoogieNaming_Open: no naming without it) */
#define NPC_HAIRDRESSER         0x70    /* its talk opens the hair menu (HairMenu_Open) */
#define NPC_GALLERY_KEEPER      0x39    /* its shop menu has a third line, the shooting gallery (ItemShop_SetRows); the gallery returns to its menu */

/* The townsperson whose quest desk offers the second quest list set
 * (QuestDesk.questSet 1). */
#define NPC_QUEST_DESK_2        0x4F

/* The townsperson at the fishing spot, whose talk opens the bait menu
 * (BaitMenu_Open): one in the village, another online. */
#define NPC_FISHING_SPOT        0x19
#define NPC_FISHING_SPOT_ONLINE 0x33

/* The house's cook, whose talk opens the ingredient list
 * (IngredientList_Open); the kitchen finds it too. */
#define NPC_HOUSE_COOK          0x71
#define NPC_OFFICIAL_DESK_LOW_HR 0x40   /* an official desk's clerk: HR gates 0-2, the raid's first quest */
#define NPC_OFFICIAL_DESK_HIGH_HR 0x4E  /* an official desk's clerk: HR gates 3-4, the raid's other two quests */

/* The workshop's townspeople (Workshop_Open, Workshop_Update). */
#define NPC_WORKSHOP_SMITH      0x0E    /* plays the forging scene in the village */
#define NPC_WORKSHOP_HELPER     0x0F    /* the village's other workshop townsperson: the shorter list; its forging is played by NPC_WORKSHOP_SMITH */
#define NPC_WORKSHOP_HELPER_ONLINE 0x58 /* online, the shorter list too; its forging is lesson NPC_SCENE_WORKSHOP_HELPER */

/* A townsperson's states (Em.state, Actor_SetState group 0), named by the
 * handler Npc_RunState runs for each; the numbers it runs through handlers
 * whose meaning the code doesn't show stay numbers. NPC_STATE_GONE ends the
 * slot (Npc_StateLeave sets it; the townsperson's task then frees it). The
 * pets (Pet_RunState, PET_STATE_*) and the service townspeople
 * (ServiceNpc_RunState) give some of the numbers other meanings. */
enum NpcState {
    NPC_STATE_IDLE                   = 0x01,
    NPC_STATE_WALK                   = 0x03,
    NPC_STATE_RUN                    = 0x04,
    NPC_STATE_BUSY_POSE              = 0x0E,
    NPC_STATE_SIT_DOWN               = 0x14,
    NPC_STATE_STAND_UP               = 0x15,
    NPC_STATE_MEAL_RISE_GOOD         = 0x18,
    NPC_STATE_SEATED                 = 0x25,
    NPC_STATE_MEAL_SERVED            = 0x29,
    NPC_STATE_TALK                   = 0x2C,
    NPC_STATE_BUSY_IDLE              = 0x30,
    NPC_STATE_BUSY_TALK              = 0x31,
    NPC_STATE_BUSY_POSE_TALK         = 0x32,
    NPC_STATE_BUSY_LOOP              = 0x33,
    NPC_STATE_BUSY_LOOP_TALK         = 0x34,
    NPC_STATE_SEAT_POSE_A            = 0x35,
    NPC_STATE_SEAT_POSE_A_1          = 0x36,
    NPC_STATE_SEAT_POSE_B            = 0x37,
    NPC_STATE_SEAT_POSE_B_1          = 0x38,
    NPC_STATE_SEAT_POSE_B_3          = 0x39,
    NPC_STATE_GREET                  = 0x3C,
    NPC_STATE_GREET_HOLD             = 0x3D,
    NPC_STATE_GREET_TALK             = 0x3E,
    NPC_STATE_SEATED_AT_TABLE        = 0x41,
    NPC_STATE_TALK_DELAY             = 0x42,
    NPC_STATE_PAIR_STAND             = 0x43,
    NPC_STATE_PAIR_STAND_1           = 0x44,
    NPC_STATE_PAIR_STAND_2           = 0x45,
    NPC_STATE_WARP_TO_ROUTE          = 0x5A,
    NPC_STATE_ROUTE_WAIT             = 0x5B,
    NPC_STATE_LEAVE                  = 0x5F,
    NPC_STATE_GONE                   = 0x60,
    NPC_STATE_STEP_AHEAD             = 0x66,
    NPC_STATE_STEP_BACK              = 0x67,
    NPC_STATE_SMITH_TALK             = 0x68,
    NPC_STATE_SMITH_FORGE            = 0x69,
    NPC_STATE_SMITH_MEET_BELOW       = 0x6A,
    NPC_STATE_SMITH_MEET_ABOVE       = 0x6B,
    NPC_STATE_PAIR_MEET              = 0x6D,
    NPC_STATE_CONTEST_SIT_DOWN       = 0x6E,
    NPC_STATE_CONTEST_GRIP           = 0x6F,
    NPC_STATE_CONTEST_SEAT           = 0x70,
    NPC_STATE_CONTEST_LEAVE          = 0x72,
    NPC_STATE_CONTEST_END            = 0x73,
    NPC_STATE_CONTEST_LOSE           = 0x74,
    NPC_STATE_CONTEST_WIN            = 0x75,
    NPC_STATE_PARTNER_WORK           = 0x76,
    NPC_STATE_SMITH_WORK             = 0x77,
    NPC_STATE_PARTNER_STRIKE         = 0x78,
    NPC_STATE_PAIR_WAIT              = 0x7A,
    NPC_STATE_WATCH_CONTEST          = 0x7E,
    NPC_STATE_SMITH_TALK_AFTER_FORGE = 0x7F,
    NPC_STATE_ROUTE_TURN             = 0x81,
    NPC_STATE_ROUTE_WALK             = 0x82,
    NPC_STATE_ROUTE_JUMP             = 0x83,
    NPC_STATE_RANDOMIZE_WAIT         = 0x84,
    NPC_STATE_PAIR_STAND_3           = 0x8B,
    NPC_STATE_CALL_OVER              = 0x8E,
    NPC_STATE_REACT_NEAR             = 0x8F,
    NPC_STATE_WAIT_TOWN_FREE         = 0x93,
    NPC_STATE_IDLE_BY_TIME           = 0x96,
    NPC_STATE_CONTEST_WAIT           = 0xA1,
    NPC_STATE_BUSY_TALK_HOLD         = 0xA2,
    NPC_STATE_CONTEST_REACT          = 0xA6,
    NPC_STATE_CONTEST_REACT_LATE     = 0xA7,
    NPC_STATE_ROUTE_JUMP_RANDOM      = 0xB3,
    NPC_STATE_FORGE                = 0xA4, /* NpcScene_RunNpc: the workshop scene's forging motion, then the route point's next state */
    NPC_STATE_HAMMER               = 0xA5, /* NpcScene_RunNpc: the workshop scene's hammering loop */
};

/* An actor state (Actor_SetState) the house access code gives the Poogie
 * while the house is open to visitors (gHouseAccess not closed); talking
 * to it then gives a notice instead of its menu. */
#define NPC_STATE_HOUSE_OPEN    0xB6

/* Townspeople of the scenes (NpcScene_RunNpc), by NpcWork.id. */
#define NPC_WORKSHOP_HAMMER_1   0x59    /* the three who hammer in both workshop scenes (NPC_STATE_HAMMER) */
#define NPC_WORKSHOP_HAMMER_2   0x5A
#define NPC_WORKSHOP_HAMMER_3   0x5B
#define NPC_KITCHEN_HELPER      0x72    /* the meal scene's only townspeople: each takes a seat of gNpcScene.seatMask */

/* Npc_PlaceOnRoute's route sets (NpcScene.routeSet), by the house's area. */
#define NPC_ROUTE_SET_ONLINE_HOUSE_0 0  /* AREA_ONLINE_HOUSE_0 */
#define NPC_ROUTE_SET_ONLINE_HOUSE_1 1
#define NPC_ROUTE_SET_ONLINE_HOUSE_2 2
#define NPC_ROUTE_SET_HOUSE     3       /* AREA_HOUSE and AREA_HUB_0x57 */

/* When a townsperson is in its area (NpcAreaEntry.presence, NpcWork.presence):
 * the area table's test and the route checks (Npc_CheckTimeRoute by the
 * town's clock, LobbyTime_IsAm; Npc_CheckSideSwap by Npc_PairIdBySide). */
#define NPC_PRESENCE_ALWAYS     0
#define NPC_PRESENCE_AM         1       /* mornings only */
#define NPC_PRESENCE_PM         2       /* afternoons only */
#define NPC_PRESENCE_SIDE       4       /* while the player is on its side of its pair's line */

/*
 * A village resident's event (gVillageGiftNpcs, gVillageMealVisitors; each
 * list ends with id 0): the event flag set when its offer points pass a
 * step (Offer_CheckResidentRewards), and its talk: the first of three gift
 * lines, or the meal visit's line (Village_MealVisitTalk).
 */
typedef struct VillageNpcEvent {
    u16 id;
    s16 eventFlag;
    s16 talk;
} VillageNpcEvent;
#define OFFER_POINTS_PER_GIFT       24  /* a resident's offer points per gift */
#define OFFER_POINTS_PER_MEAL_VISIT 32  /* and per meal visit */

/*
 * The actor states (Actor_SetState) the pets' menus give a pet (the
 * Poogie, the care pet, the house pet); its type's state handlers run them.
 */
#define PET_STATE_IDLE          0x01    /* stands, back on its route */
#define PET_STATE_PETTED        0x2C    /* comes up to the player, who takes the petting pose; also while it eats */
#define PET_STATE_ANGRY         0x54    /* the petting angered it: it runs off (PET_ACT_RUN_OFF) */
#define PET_STATE_JOY           0x55    /* pleased by the petting or the food (PET_ACT_REACT) */
#define PET_STATE_RUN_OFF       0x56    /* runs to a spot and stays away for a while: anger, or the Poogie's errand */
#define PET_STATE_FOLLOW        0x58    /* follows the player */
#define PET_STATE_GREAT_JOY     0x5E    /* very pleased (PET_ACT_REACT) */
#define PET_STATE_FACE          0xA9    /* turns to the player, then PET_STATE_IDLE: a pet menu opens */
#define PET_STATE_DIG           0xAC    /* goes to a dig spot and digs (PET_ACT_DIG) */

/* NpcWork.petAct: set by a pet's state handlers while it acts. */
#define PET_ACT_NONE            0       /* free: its menus can open */
#define PET_ACT_RUN_OFF         1       /* angered and running off */
#define PET_ACT_REACT           2       /* its joy or great joy plays */
#define PET_ACT_DIG             3       /* digging */

/*
 * Pet_AreaKind: which pet the area holds, the index of the pets'
 * points (LobbyState.petPoints) and their limits (petPointsMax).
 */
enum PetArea {
    PET_AREA_POOGIE = 0,        /* the Poogie (NPC_POOGIE): every area but the two below */
    PET_AREA_CARE = 1,          /* AREA_HUB_0x83: the care pet (NPC_POOGIE_CARE) */
    PET_AREA_HOUSE = 2          /* AREA_HOUSE and AREA_HUB_0x57: the house pet (NPC_HOUSE_PET) */
};

/* gPoogieErrand: the Poogie's errand, earned in its menu by two very joyful pettings in a row. */
enum PoogieErrand {
    POOGIE_ERRAND_NONE = 0,
    POOGIE_ERRAND_AWAY = 1,     /* it has gone off (not placed in the town) */
    POOGIE_ERRAND_BACK = 2      /* back after a quest: it hands over a find (gPoogieFinds) in its menu */
};

/* gPoogieDigState: a pet's dig (PET_STATE_DIG). */
enum PoogieDig {
    POOGIE_DIG_NONE = 0,
    POOGIE_DIG_GOING = 1,       /* on its way to the spot, or digging */
    POOGIE_DIG_DONE = 2         /* the find was taken or the player came near: the next petting comes first */
};

/* The workshop's townspeople (NPC_WORKSHOP_*: Workshop_Open, Workshop_Update) read their
 * block with their own meaning at +0x3A (NpcWork's turnStep). */
typedef struct NpcWorkshopWork {
    /* 0x00 */ u8 id;           /* NpcWork's id */
    /* 0x01 */ u8 unk01[0x3A - 0x01];
    /* 0x3A */ s16 menuSave;    /* the workshop menu's +4 halfword, kept while the menu is closed (Workshop_Update stores it, Workshop_Open restores it) */
    /* 0x3C */ u8 unk3C[0x60 - 0x3C];
} NpcWorkshopWork;
/* gNpcScene (lobby): a scripted scene of townspeople (NpcScene_Start sets it
 * up, NpcScene_IsDone waits for its end, NpcScene_RunNpc runs each
 * townsperson's part): the online workshop's forging and the house's meal. */
typedef struct NpcScene {
    u8 step;        /* 0x00 */
    u8 pad1;
    u8 active;      /* 0x02 */
    u8 done;        /* 0x03 */
    s16 timer;      /* 0x04 */
    s16 frames;     /* 0x06 */
    s32 seatMask;   /* 0x08: the meal's seats, one bit each (the starter's mask): each kitchen helper takes the next set bit as its seat (Em.sceneSeat); 0 for the workshop */
    s32 mealLevel;  /* 0x0C: NpcScene_Start's level: the meal's grade, which the serving kitchen helper passes to ThrownObj_Spawn with its seat */
    u8 kind;        /* 0x10: the scene (NpcScene_Start's first argument; NPC_SCENE_*) */
    u8 ticked;      /* 0x11 */
    u8 count;       /* 0x12: the workshop scenes: the hammering townspeople's calls left (3); the meal: the seats in seatMask */
    u8 helpers;     /* 0x13: the kitchen helpers (NPC_KITCHEN_HELPER) present and visible, counted each frame */
    u8 routeSet;    /* 0x14: Npc_PlaceOnRoute's set (NPC_ROUTE_SET_*), by the house's area, each frame */
    u8 turn;        /* 0x15 */
    u8 seatIdx;     /* 0x16: in step 0 the set bit of seatMask the next helper takes; in steps 1-2 the seated helpers counted; cleared each frame with turn */
    u8 pad17[5];
} NpcScene;
typedef char NpcScene_size_check[(sizeof(NpcScene) == 0x1C) ? 1 : -1];

/* NpcScene.kind: 0 and 1 are the workshop's two lessons (its townspeople start them), 2 the
 * cooking lesson, where the townspeople of NpcScene.seatMask take turns at the house's seats
 * (the meal scene and the kitchen start it). */
#define NPC_SCENE_WORKSHOP 0          /* online, the forging scene of the workshop's townsperson */
#define NPC_SCENE_WORKSHOP_HELPER 1   /* online, NPC_WORKSHOP_HELPER_ONLINE's forging scene */
#define NPC_SCENE_COOKING 2

extern NpcScene gNpcScene;

typedef char NpcWorkshopWork_size_check[(sizeof(NpcWorkshopWork) == sizeof(NpcWork)) ? 1 : -1];

/* A townsperson's talk (Talk_Say, Talk_Step, Talk_Update): its lines'
 * kinds (TalkLine.kind), the yes/no cursor (gTalkMode) and the gifts
 * handed over at its end (TalkGift.kind). */
#define TALK_LINE_ASK   1       /* a yes/no question: the answer skips ahead by the line's yesSkip or noSkip */
#define TALK_LINE_NEXT  2       /* another line follows; any other kind is the talk's last line */
#define TALK_CHOICE_YES 0
#define TALK_CHOICE_NO  1
#define TALK_GIFT_MONEY 1       /* money, with its notice */
#define TALK_GIFT_ITEM  2       /* an item into the pouch */
#define TALK_GIFT_EQUIP 3       /* a piece of equipment into the box */
#define TALK_GIFT_NONE  0xFF    /* TalkLine.gift: the line hands over nothing */

/* A line of a townsperson's talk (gTalkLines[script][line]). */
typedef struct TalkLine {
    /* 0x0 */ u8 kind;      /* TALK_LINE_*: a question, a line with more after it, or the last */
    /* 0x1 */ u8 yesSkip;   /* a question: the lines to skip ahead on yes (0: the talk ends) */
    /* 0x2 */ u8 noSkip;    /* and on no */
    /* 0x3 */ u8 gift;      /* the gift of gTalkGifts[script] at the talk's end, TALK_GIFT_NONE: none */
} TalkLine;

/* A gift handed over at a talk's end (gTalkGifts[script][TalkLine.gift]). */
typedef struct TalkGift {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 kind;         /* TALK_GIFT_* */
    /* 0x4 */ union {
        s16 value;              /* the money, or the item as the notice takes it */
        u16 item;               /* the pouch item */
        u8 type;                /* the equipment's type (EQUIP_*) */
    } a;
    /* 0x6 */ union {
        s16 count;              /* the pouch item's count */
        u16 id;                 /* the equipment's id */
    } b;
} TalkGift;

#endif
