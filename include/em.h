#ifndef EM_H
#define EM_H

/*
 * Monsters in the field (game overlay). sub_main keeps 20 of them in an
 * array at 0x5EC580 (gEms), 0x9D0 bytes each; the players follow at
 * 0x5F89D0. `type` selects the kind, and per-kind tables in the game
 * overlay's data hold its parameters.
 *
 * Status effects (src/game/unk_7077A0.c): +0x71E holds the current one (1,
 * 2, 4 or 5; 0 for none) and +0x71F has one flag bit per status. Each
 * status has a threshold, initialised from a per-kind EmStatusParam table
 * and raised by `increment` each time the status ends, at most 4 times (the
 * counts at +0x73B, +0x73D, +0x73E and +0x7D7).
 */
#include "common.h"
#include "scene.h"
#include "model.h"
#include "motion.h"

/*
 * Monster kinds (Em.type), the game's own monster numbers. The quest data
 * names them: each quest lists its targets as {kind, count} records next
 * to the target's name. Kinds 7, 0x29, 0x2A, 0x32, 0x47 and 0x48 have no quest;
 * their family (the per-kind setup dispatch Em_TypeSetup, gm_em.c's
 * dispatch) places them. 0x12 is the Felyne cart (Em.cartPlayer).
 */
enum EmKind {
    EM_KIND_RATHIAN = 0x01,
    EM_KIND_FATALIS = 0x02,
    EM_KIND_MOSSWINE = 0x04,
    EM_KIND_BULLFANGO = 0x05,
    EM_KIND_YIAN_KUT_KU = 0x06,
    EM_KIND_LAO_SHAN_LUNG = 0x07,
    EM_KIND_CEPHADROME = 0x08,
    EM_KIND_RATHALOS = 0x0B,
    EM_KIND_GENPREY = 0x0D,
    EM_KIND_DIABLOS = 0x0E,
    EM_KIND_KHEZU = 0x0F,
    EM_KIND_VELOCIPREY = 0x10,
    EM_KIND_GRAVIOS = 0x11,
    EM_KIND_FELYNE_CART = 0x12,
    EM_KIND_VESPOID = 0x13,
    EM_KIND_GYPCEROS = 0x14,
    EM_KIND_PLESIOTH = 0x15,
    EM_KIND_BASARIOS = 0x16,
    EM_KIND_MELYNX = 0x17,
    EM_KIND_HORNETAUR = 0x18,
    EM_KIND_APCEROS = 0x19,
    EM_KIND_MONOBLOS = 0x1A,
    EM_KIND_VELOCIDROME = 0x1B,
    EM_KIND_GENDROME = 0x1C,
    EM_KIND_IOPREY = 0x1E,
    EM_KIND_IODROME = 0x1F,
    EM_KIND_KIRIN = 0x21,
    EM_KIND_CEPHALOS = 0x22,
    EM_KIND_GIAPREY = 0x23,
    EM_KIND_FATALIS_B = 0x24,           /* a second Fatalis kind (its quests name it as Fatalis) */
    EM_KIND_PINK_RATHIAN = 0x25,
    EM_KIND_BLUE_YIAN_KUT_KU = 0x26,
    EM_KIND_PURPLE_GYPCEROS = 0x27,
    EM_KIND_YIAN_GARUGA = 0x28,
    EM_KIND_SILVER_RATHALOS = 0x29,
    EM_KIND_GOLD_RATHIAN = 0x2A,
    EM_KIND_BLACK_DIABLOS = 0x2B,
    EM_KIND_WHITE_MONOBLOS = 0x2C,
    EM_KIND_RED_KHEZU = 0x2D,
    EM_KIND_GREEN_PLESIOTH = 0x2E,
    EM_KIND_BLACK_GRAVIOS = 0x2F,
    EM_KIND_DAIMYO_HERMITAUR = 0x30,
    EM_KIND_AZURE_RATHALOS = 0x31,
    EM_KIND_ASHEN_LAO_SHAN_LUNG = 0x32,
    EM_KIND_BLANGONGA = 0x33,
    EM_KIND_CONGALALA = 0x34,
    EM_KIND_RAJANG = 0x35,
    EM_KIND_KUSHALA_DAORA = 0x36,
    EM_KIND_SHEN_GAOREN = 0x37,
    EM_KIND_SHAKALAKA = 0x39,
    EM_KIND_YAMA_TSUKAMI = 0x3A,
    EM_KIND_CHAMELEOS = 0x3B,
    EM_KIND_RUSTED_KUSHALA_DAORA = 0x3C,
    EM_KIND_BLANGO = 0x3D,
    EM_KIND_CONGA = 0x3E,
    EM_KIND_REMOBRA = 0x3F,
    EM_KIND_LUNASTRA = 0x40,
    EM_KIND_TEOSTRA = 0x41,
    EM_KIND_HERMITAUR = 0x42,
    EM_KIND_SHOGUN_CEANATAUR = 0x43,
    EM_KIND_BULLDROME = 0x44,
    EM_KIND_ANTEKA = 0x45,
    EM_KIND_POPO = 0x46,
    EM_KIND_FATALIS_C = 0x47,           /* a third Fatalis kind (gm_em.c's dispatch); no quest names it */
    EM_KIND_YAMA_TSUKAMI_B = 0x48,      /* the second kind of Yama Tsukami's family */
    EM_KIND_CEANATAUR = 0x49
};

/*
 * State groups (Em.stateGroup). Each family's RunState switches on the
 * group, then a per-group dispatcher on Em.state; the families' state
 * functions settle what each group holds (seen in Rathalos's and the
 * common code):
 */
enum EmStateGroup {
    EM_GROUP_IDLE = 0,      /* standing, idle actions, eating, resting, lying under a status */
    EM_GROUP_MOVE = 1,      /* walking, running, limping and turning on the ground */
    EM_GROUP_FLY = 2,       /* take-off, flight, hovering and landing (Diablos: underground) */
    EM_GROUP_ATTACK = 3,    /* attacks and threats aimed at the target */
    EM_GROUP_REACT = 4,     /* reactions to hits, traps and statuses (Em_UpdateDamage's results) */
    EM_GROUP_DEFEAT = 5,    /* hunted (QuestEm_Hunted), captured, and appearing again */
    EM_GROUP_EVENT = 6,     /* scripted scenes: the quest's opening flight, captures */
    EM_GROUP_EVENT2 = 7,    /* runs group 6's dispatcher */
    EM_GROUP_VERSUS = 8     /* the end of a versus match (Rathalos: Versus_DefeatCamera); GmEm_Update keeps the monster unhittable */
};

/* Em.stance: where the monster's body is (set by the families' states). */
enum EmStance {
    EM_STANCE_GROUND = 0,
    EM_STANCE_FAMILY = 1,   /* family-specific: Kushala kinds hovering, Lao-Shan, Shen Gaoren */
    EM_STANCE_AIR = 2,      /* flying, or leaping out of the sand */
    EM_STANCE_DEAD = 3,     /* QuestEm_Hunted */
    EM_STANCE_UNDER = 4     /* under the sand or water (Diablos, Cephadrome, Plesioth) */
};

/*
 * How a state change reaches the other players online (Em_SetStateSync's
 * mode, passed on by the families' ChangeState and Set*State functions).
 * Offline every mode sets the state here. Em_SyncState sends nothing for a
 * remote monster (another player's copy), and the families' ChangeState
 * functions drop changes of modes 3 and 4 there.
 */
enum EmSyncMode {
    EM_SYNC_LOCAL = 0,          /* set here only */
    EM_SYNC_SEND = 1,           /* sent (Em_SyncState); set here at once only for a small monster (Em.large 0) */
    EM_SYNC_SEND_AND_SET = 2,   /* sent and set here */
    EM_SYNC_OWNER = 3,          /* sent and set here, by the monster's owner only */
    EM_SYNC_OWNER_FLAGGED = 4   /* as EM_SYNC_OWNER, sent with QSync_SendEm's flag 1 */
};

/*
 * Em_UpdateDamage's result (unk_70A290.c): what the last hits and statuses
 * ask of the monster. Each family's Update switches on it to pick a
 * reaction (group 4), a status or a death. Attack kinds are
 * Em.lastAttackKind's; statuses are Em.status's numbers.
 */
enum EmDamageResult {
    EM_DMG_NONE = 0x00,
    EM_DMG_KILLED = 0x01,           /* a hit took the last hp */
    EM_DMG_DEAD = 0x02,             /* hp 0 without a hit */
    EM_DMG_ATTACK9 = 0x03,          /* a hit of attack kind 9 while not held */
    EM_DMG_ATTACK6 = 0x04,          /* a hit of attack kind 6 while not held */
    EM_DMG_STATUS_IN_AIR = 0x05,    /* a status began in the air (Em_PickStatus ended statuses 1 and 2): the families fall */
    EM_DMG_STATUS4 = 0x06,
    EM_DMG_CAPTURED = 0x07,         /* status 1 while held and near low hp (Em_NearLowHp) */
    EM_DMG_STATUS2 = 0x08,
    EM_DMG_FLASH = 0x09,            /* a flash in sight (Em_ApplyFlash) */
    EM_DMG_HIT_LYING = 0x0A,        /* a hit with hitReactMode 2: the end of a lying state */
    EM_DMG_PART8_DOWN = 0x0B,       /* part 8's hp ran out and the kind's handler took it (Em.part8Down) */
    EM_DMG_PART_FLINCH = 0x0C,      /* a part's hp ran out (Em.flinchPart) */
    EM_DMG_HIT_MODE3 = 0x0D,        /* a hit with hitReactMode 3 */
    EM_DMG_HIT = 0x0E,              /* any other hit while not held */
    EM_DMG_ATTACK5 = 0x0F,          /* a hit of attack kind 5 */
    EM_DMG_ATTACK5_FLINCH = 0x10,   /* a hit of attack kind 5 that ran a part's hp out */
    EM_DMG_STATUS5 = 0x11,          /* the stun */
    EM_DMG_ATTACK13 = 0x12          /* a hit of attack kind 0x13 while not held, with the kind's timer (Em_HasKindTimer) */
};

/* A layer's motion id (Em.motionId[layer]) for a motion of the kind's table. */
#define EM_MOTION_ID(layer, motion) (1000 + 200 * (layer) + (motion))

typedef struct EmStatusParam {
    s16 threshold;      /* 0x0: initial threshold; -1 when the kind is immune */
    s16 decayInterval;  /* 0x2: frames between decays of the buildup */
    s16 decayAmount;    /* 0x4: taken off the buildup at each decay */
    s16 duration;       /* 0x6: in frames */
    s16 increment;      /* 0x8: added to the threshold each time it ends */
} EmStatusParam;

/* The per-kind status parameters, indexed by Em.type (-1 thresholds: immune). */
extern EmStatusParam *gEmStatus1Params[];
extern EmStatusParam *gEmStatus2Params[];
extern EmStatusParam *gEmStatus4Params[];
extern EmStatusParam *gEmStatus5Params[];

/* The model (struct Model, EmBone, ModelMat): include/model.h. */

/*
 * A row of a kind's motion table (gEmMotionRows[kind], 12 bytes), read by Em_SetMotion and
 * Em_SetMotionRow. A layer's motion id is 1000 + 200 * layer + motion.
 */
typedef struct EmMotionRow {
    u8 motion;          /* 0x0 */
    s8 layers;          /* 0x1: which layers (0-2) get it; 0x10-0x12: all but one; -1: all */
    u8 unk2;            /* 0x2 */
    u8 trans;           /* 0x3: row of the kind's transition table (gEmMotionTransRows) for the motion that follows */
    s16 frame;          /* 0x4: start frame when the caller passes 0 */
    s8 blend;           /* 0x6: blend frames when the caller passes 0 (Em +0x418 when this is 0) */
    s8 unk7;            /* 0x7: Em +0x418 once the motion is set */
    u8 remap;           /* 0x8: with Em +0x652 set, the row of the alternate table (gEmMotionAltRows) to use */
    u8 unk9[3];
} EmMotionRow;

/*
 * A key of a curve table (EmGaruga_CurveVelY and its copies in ten other
 * families): the value `v` at frame `t`. A table ends with a key whose t is
 * 0 or negative; the families keep one table per curve, indexed by a byte
 * of their work area.
 */
typedef struct EmCurveKey {
    f32 t;
    f32 v;
} EmCurveKey;

/*
 * A flying family's flight parameters (Em_UpdateFlight's third argument; a
 * table of 0x60 bytes in each family's .data).
 */
typedef struct EmFlyParam {
    s32 roll;                   /* 0x00: unkA0 back toward 0 per frame */
    s32 turn;                   /* 0x04: yaw step */
    s32 pitch;                  /* 0x08: unkA8 step */
    s32 lim0;                   /* 0x0C */
    s32 a0;                     /* 0x10 */
    s32 b0;                     /* 0x14 */
    s32 lim1;                   /* 0x18 */
    s32 a1;                     /* 0x1C */
    s32 b1;                     /* 0x20 */
    s32 lim2;                   /* 0x24 */
    s32 a2;                     /* 0x28 */
    s32 b2;                     /* 0x2C */
    s32 lim3;                   /* 0x30 */
    s32 a3;                     /* 0x34 */
    s32 b3;                     /* 0x38 */
    u16 mot[6];                 /* 0x3C */
    s32 up0;                    /* 0x48 */
    s32 up1;                    /* 0x4C */
    s32 down0;                  /* 0x50 */
    s32 down1;                  /* 0x54 */
    f32 speedMax;               /* 0x58 */
    f32 speedMin;               /* 0x5C */
} EmFlyParam;

/*
 * The work area that a family's state dispatcher passes to each state
 * function with the monster (the second argument). Fields generated from
 * include/em_work.spec.
 */
typedef struct EmWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 (runs MotionEffects): Kirin, Anteka, kind 0x1D (the families with a view keep their own, at +0 or elsewhere) */
    u8 unk001[0x002 - 0x001];
    s16 sndMotion;              /* 0x002: the motion id the MotionEffects functions keep (motionId[0], stored when it changes) and the MotionSounds copies (Kut-Ku, Garuga, Rathalos, Gravios, Diablos, Khezu, Chameleos, Fatalis) switch on; Rathalos and Fatalis write it through their views (EmFlyerWork.sndMotion, EmFatalisWork.fxMotion); Yama Tsukami keeps a timer here (EmYamaTsukamiWork) */
    s16 prevMotion;             /* 0x004: a motion id kept to see it change (motionId[0] stored and compared; switched on) */
    s16 fxMotion;               /* 0x006: the motion id the MotionEffects and MotionSounds functions last saw (motionId[0] stored and compared; switched on); the flying families (Rathalos, Kutku, Garuga, Gravios, Diablos) keep frames since landing here instead (150 set by the landing states, counted down by the update) */
    s32 accelAngX;              /* 0x008: angles X, Y, Z for Em_MoveRotatedAccel (&accelAngX; set from rotX, rotY and 0) */
    s32 accelAngY;              /* 0x00C: set from rotY, or rotY +- 0x4000, + 0x8000 */
    s32 accelAngZ;              /* 0x010: 0; some states keep an f32 distance here */
    u16 goalYaw;                /* 0x014: Math_AngleXZ to the goal; copied to rotY */
    u8 unk016[0x017 - 0x016];
    u8 unk017;                  /* 0x017 */
    u8 flyGuard;                /* 0x018: flying families: 1 in states of group 2 (flight; Diablos underground): the update then keeps guardTimer at 2 (read as s8 w[0x18]); Velocidrome and Velociprey keep their curve frame here (EmDromeWork, EmVelocipreyWork) */
    u8 unk019;                  /* 0x019 */
    u8 countdown;               /* 0x01A: s8 counts down to 0 (--*(s8 *)&countdown <= 0) */
    u8 unk01B;                  /* 0x01B */
    s32 moveAngBX;              /* 0x01C: angles X, Y, Z for Em_MoveRotated and Em_GetPitchToGoal (the second set; see moveAngX) */
    s32 moveAngBY;              /* 0x020: set from rotY */
    s32 moveAngBZ;              /* 0x024: 0; some states keep a position (Vec_Set) from here */
    s32 moveAngCX;              /* 0x028: Chameleos: angles X, Y, Z for Em_MoveRotated and Em_GetPitchToGoal (the third set: X, Y here, Z at moveAngX); Rathalos keeps an f32 target height here */
    s32 moveAngCY;              /* 0x02C: Chameleos: set from rotY or Math_AngleXZ to the goal */
    s32 moveAngX;               /* 0x030: angles X, Y, Z for Em_MoveRotated, Em_MoveRotatedAccel, Em_GetPitchToGoal/Target (&moveAngX: Mtx_FromAnglesYXZ); set from rotX */
    s32 moveAngY;               /* 0x034: set from rotY or Math_AngleXZ to the goal */
    s32 moveAngZ;               /* 0x038: 0 */
    s32 pitchStep;              /* 0x03C: added to or taken from rotX each frame */
    s32 turnSpeed;              /* 0x040: Em_TurnToGoal speed (0x100) */
    s32 rollSpeed;              /* 0x044: flying families: rotZ is stepped back to 0 by it (0x100 at init); Velocidrome and Velociprey State3: an f32 velZ kept while a wall stops the jump */
    s32 unk048;                 /* 0x048 */
    s32 unk04C;                 /* 0x04C */
    s16 unk050;                 /* 0x050 */
    s16 wallTimer;              /* 0x052: Rathalos swoops: frames (30) after a wall hit in which the pitch is not aimed at the target */
    f32 startX;                 /* 0x054: Velocidrome State1_01: the position the walk started from (X, Z) and the frames walked */
    f32 startZ;                 /* 0x058 */
    s16 walkFrames;             /* 0x05C: compared with 60 when the goal is reached near the start */
    u8 hunted;                  /* 0x05E: Velocidrome: 1 once a hunted state (group 5) ran; cleared at init */
    u8 unk05F[0x064 - 0x05F];
    f32 startXB;                /* 0x064: Velociprey State1_01: as startX, startZ, walkFrames */
    f32 startZB;                /* 0x068 */
    s16 walkFramesB;            /* 0x06C */
    u8 huntedB;                 /* 0x06E: Velociprey: as hunted */
    u8 unk06F[0x100 - 0x06F];
} EmWork;
typedef char EmWork_size_check[(sizeof(EmWork) == 0x100) ? 1 : -1];

/* A row of a family's turn table (X_TurnParams): how Em_StartTurnToTarget/Away and
 * Em_TurnStep turn a monster toward the goal with a turning motion. */
typedef struct EmTurnParams {
    f32 from;                   /* 0x00: the motion frames the turn is spread over (from, to); both 0: the whole motion */
    f32 to;                     /* 0x04 */
    f32 frame;                  /* 0x08: above 0: the turn may end from this frame on */
    u16 angle;                  /* 0x0C: the angle the turning motion turns */
    u16 range;                  /* 0x0E: nonzero: within this angle of the goal, motFront plays instead */
    s32 motFront;               /* 0x10: the motion within range */
    s32 motLeft;                /* 0x14: the motion when the goal is to one side (angle to it 0x8000 or more) */
    s32 motRight;               /* 0x18: the motion for the other side */
} EmTurnParams;
typedef char EmTurnParams_size_check[(sizeof(EmTurnParams) == 0x1C) ? 1 : -1];

/* The flying families' (Kut-Ku, Garuga, Rathalos, Gravios) own meanings of the work area's
   bytes (EmWork has the shared ones): their copies of the same functions read one layout. */
typedef struct EmFlyerWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x002 - 0x001];
    s16 sndMotion;              /* 0x002: as EmWork's: the motion id MotionEffects keeps (Rathalos) */
    u8 earOpen;                 /* 0x004: Kut-Ku and Garuga (kinds 6, 0x26, 0x28): the ears' opening, 0 to 30 frames: rises a frame at a time in combat while hp is at least a fifth, falls otherwise; sub_main Em_AdjustKindBones turns bones 0x25 to 0x27 by it */
    u8 legsLeft;                /* 0x005: State2_06: the flight's goals left (SetGroup2State: 4 or 1); each goal reached takes one, 0 ends the flight */
    s16 fxMotion;               /* 0x006: as EmWork's: frames since landing (Init: 0; Update counts it down) */
    u8 unk008;                  /* 0x008: Kut-Ku: Update toggles it each frame while enraged (else 0); State3_1D sets it during its motion; the draw shows a mesh of set 3 only while it and partWhole are set */
    s8 minePoint;               /* 0x009: Kut-Ku: a MinePoint id (Init: -1); ChangeState removes it and sets -1 */
    u8 unk00A[0x010 - 0x00A];
    f32 runDist;                /* 0x010: Kut-Ku: SetGroup3State: the distance to run (2500, 5000 or the goal's); State3_02 takes velZ from it each frame */
    u8 unk014[0x018 - 0x014];
    u8 flyGuard;                /* 0x018: as EmWork's (read as s8) */
    u8 unk019[0x01A - 0x019];
    s8 countdown;               /* 0x01A: Rathalos, Garuga: SetGroup3State: 1 or 2; the group 3 states count it down */
    u8 partWhole;               /* 0x01B: Kut-Ku: Init: 1; SetGroup4State (20) clears it as Em_BreakPart(em, 1) breaks the part; the draw shows the whole or the broken meshes by it */
    s32 moveAngBX;              /* 0x01C: as EmWork's */
    s32 moveAngBY;              /* 0x020 */
    f32 startPos[3];            /* 0x024: Rathalos State3_0B: the position the descent started from (Vec_Set); the climb back aims at it */
    s32 moveAngX;               /* 0x030: as EmWork's */
    s32 moveAngY;               /* 0x034 */
    s32 moveAngZ;               /* 0x038 */
    s32 pitchStep;              /* 0x03C */
    u16 turnSpeed;              /* 0x040: Em_TurnToGoal's speed (Init stores the word, EmWork's turnSpeed: 0x200); the states read the halfword */
    u8 unk042[0x044 - 0x042];
    s32 rollSpeed;              /* 0x044: as EmWork's */
    u8 unk048;                  /* 0x048: Gravios Reset: 0 */
    u8 unk049;                  /* 0x049: Gravios Reset: 0 */
    u8 unk04A;                  /* 0x04A: Gravios Reset: 0 */
    u8 unk04B[0x04C - 0x04B];
    u8 curveNone;               /* 0x04C: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x04D */
    u8 curveSpeed;              /* 0x04E */
    u8 curveKind;               /* 0x04F: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x050: the curve frame (StartCurves, UpdateCurves, CurveVelY, CurveSpeed) */
    u8 unk052[0x100 - 0x052];
} EmFlyerWork;
typedef char EmFlyerWork_size_check[(sizeof(EmFlyerWork) == sizeof(EmWork)) ? 1 : -1];

/* Kushala Daora's own meanings of the work area's bytes (EmWork has the shared ones;
   em_blangonga.c holds its curve functions). */
typedef struct EmKushalaWork {
    u8 unk000[0x001 - 0x000];
    s8 fortHits;                /* 0x001: State3_18: counts the hits on the fort up to 10 (then Quest_TriggerEvent) and copies the count to gScene.fortStage */
    u8 unk002[0x004 - 0x002];
    u8 fxStep;                  /* 0x004: UpdateMotionEffects: 0, then 1 */
    u8 unk005[0x008 - 0x005];
    u8 windOn;                  /* 0x008: WindActive and WindActive2 return it; SetGroup0State reads it; Init: 0 */
    u8 legsLeft;                /* 0x009: State2_0D: the flight's goals left (SetGroup2State: 1 or 4); each goal reached takes one */
    s16 unk00A;                 /* 0x00A: Init: 0 */
    u8 unk00C[0x00E - 0x00C];
    u16 unk00E;                 /* 0x00E: EffEm14State returns it != 0 */
    u8 unk010[0x018 - 0x010];
    u8 flyGuard;                /* 0x018: as EmWork's (read as s8) */
    u8 unk019[0x01B - 0x019];
    u8 unk01B;                  /* 0x01B: Init: 0 */
    s32 moveAngBX;              /* 0x01C: as EmWork's */
    s32 moveAngBY;              /* 0x020 */
    f32 dashGoal[3];            /* 0x024: SetGroup1State (21, 22): the point the dash ends at (the goal; a remote copy: timer units ahead along rotY) */
    s32 moveAngX;               /* 0x030: as EmWork's */
    s32 moveAngY;               /* 0x034 */
    s32 moveAngZ;               /* 0x038 */
    s32 pitchStep;              /* 0x03C */
    s32 turnSpeed;              /* 0x040 */
    s32 rollSpeed;              /* 0x044 */
    u8 unk048[0x04C - 0x048];
    u8 curveNone;               /* 0x04C: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x04D */
    u8 curveSpeed;              /* 0x04E */
    u8 curveKind;               /* 0x04F: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x050: the curve frame (StartCurves, UpdateCurves, CurveVelY, CurveSpeed) */
    u8 unk052[0x100 - 0x052];
} EmKushalaWork;
typedef char EmKushalaWork_size_check[(sizeof(EmKushalaWork) == sizeof(EmWork)) ? 1 : -1];

/* Fatalis's own meanings of the work area's bytes (EmWork has the shared ones;
   em_bulldrome.c holds its curve functions; include/em_fatalis.h its states). */
typedef struct EmFatalisWork {
    u8 unk000[0x001 - 0x000];
    u8 fxStep;                  /* 0x001: UpdateMotionEffects: 0, then 1 */
    s16 fxMotion;               /* 0x002: the motion id EmFatalis_MotionEffects keeps (EmWork.sndMotion, which MotionSounds switches on) */
    u8 unk004[0x010 - 0x004];
    f32 runDist;                /* 0x010: SetAttackState: the dives' distance (100, or the goal's); EmFatalis_Dive and DiveB take velZ from it each frame */
    u8 unk014[0x017 - 0x014];
    u8 diveToGoal;              /* 0x017: SetAttackState: 1 when a dive aims at a goal (Em.curGoalKind set), else 0; nothing reads it */
    u8 unk018[0x01A - 0x018];
    s8 countdown;               /* 0x01A: SetAttackState: the air fireballs left (1, or 3 for FAT_ATK_AIR_FIREBALL_TRIPLE) */
    u8 unk01B[0x024 - 0x01B];
    f32 startPos[3];            /* 0x024: EmFatalis_Hover: where the hover started (Vec_Set); it never rises above that height */
    s32 moveAngX;               /* 0x030: as EmWork's */
    s32 moveAngY;               /* 0x034 */
    s32 moveAngZ;               /* 0x038 */
    u8 unk03C[0x04C - 0x03C];
    u8 curveNone;               /* 0x04C: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x04D */
    u8 curveSpeed;              /* 0x04E */
    u8 curveKind;               /* 0x04F: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x050: the curve frame (StartCurves, UpdateCurves, CurveVelY, CurveSpeed) */
    s16 initHp;                 /* 0x052: Init: hp */
    s8 minePoints[3];           /* 0x054: AddMinePoints: the carcass's three mine points (MinePoint ids, -1 once removed) */
    s8 rageTimer;               /* 0x057: UpdateRageTimer (EM_KIND_FATALIS_B and _C): frames enraged until rageStep 1; once calm, counts up to -1 (off) */
    u8 unk058[0x100 - 0x058];
} EmFatalisWork;
typedef char EmFatalisWork_size_check[(sizeof(EmFatalisWork) == sizeof(EmWork)) ? 1 : -1];

/* Yama Tsukami's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmYamaTsukamiWork {
    s8 unk000;                  /* 0x000: cleared by Setup and Start; ScriptQuery returns it */
    u8 unk001[0x002 - 0x001];
    s16 syncTimer;              /* 0x002: Start: 150; Update counts it down, then sends the monster's state (QSync_SendEm kind 2) and restarts it */
    u8 unk004[0x005 - 0x004];
    u8 fxStep;                  /* 0x005: UpdateMotionEffects: 0, then 1 (runs MotionEffects and EaseToTarget) */
    u8 unk006[0x008 - 0x006];
    s16 unk008;                 /* 0x008: MotionSounds reads it as a motion id */
    u8 unk00A[0x00B - 0x00A];
    s8 unk00B;                  /* 0x00B: State2_04: 30, counted down */
    s32 unk00C;                 /* 0x00C: State1_02: an angle, halved and stepped down by unk01C */
    s32 accelAngX;              /* 0x010: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x014 */
    s32 accelAngZ;              /* 0x018 */
    s32 unk01C;                 /* 0x01C: State1_02, State3_01: 0, then += 0x20 */
    f32 ease[3];                /* 0x020: EaseToTarget: eased toward EmYamatsukami_EaseTargets[easeIdx] (Start: 1.0) */
    s16 curveFrame;             /* 0x02C: the curve frame (UpdateCurves adds the motion's step) */
    u8 unk02E;                  /* 0x02E: a step (Update switches on it; Start and SetGroup4State clear it) */
    u8 curveKind;               /* 0x02F: the curve tables' index (StartCurves) */
    u8 curveNone;               /* 0x030: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x031 */
    u8 curveSpeed;              /* 0x032 */
    s8 unk033;                  /* 0x033: a countdown (Start 15, SetGroup4State 30; State2_01) */
    s8 easeIdx;                 /* 0x034: EaseToTarget's row (Start 2; Update switches on it) */
    s8 easeMode;                /* 0x035: EaseToTarget runs in mode 0 (Start clears it) */
    s8 mineSlot;                /* 0x036: the mine point AddMinePoint made (-1: none) */
    s8 mineCount;               /* 0x037: AddMinePoint's count (none at 0) */
    u8 unk038;                  /* 0x038: set and cleared by the states */
    u8 unk039[0x100 - 0x039];
} EmYamaTsukamiWork;
typedef char EmYamaTsukamiWork_size_check[(sizeof(EmYamaTsukamiWork) == sizeof(EmWork)) ? 1 : -1];

/* Shen Gaoren's own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmShenGaorenWork {
    u8 unk000;                  /* 0x000: cleared by Setup and Init; State3_03 sets 8 (in area day 0x1C, when below 8); ScriptQuery returns it */
    s8 fortHits;                /* 0x001: State3_01 to State3_03: the hits on the fort, up to 10, copied to gScene.fortStage */
    s16 syncTimer;              /* 0x002: Init: 150; Update counts it down, then sends the monster's state (QSync_SendEm kind 2) and restarts it */
    u8 unk004[0x005 - 0x004];
    u8 fxStep;                  /* 0x005: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk006[0x008 - 0x006];
    s16 fxMotion;               /* 0x008: the motion id MotionEffects last saw (motionId[0] stored and compared; MotionSounds and func_008261F0 switch on it) */
    u8 unk00A[0x00C - 0x00A];
    f32 unk00C;                 /* 0x00C: Init: 1500.0 */
    f32 ease[4];                /* 0x010: func_008261F0 eases them toward a row of D_008CA0C0, by a 120th of the difference a frame (Init: 1.0) */
    u8 unk020;                  /* 0x020: MotionEffects clears it on a new motion */
    u8 unk021[0x022 - 0x021];
    s8 unk022;                  /* 0x022: Init: 0 */
    s8 mineSlot;                /* 0x023: the mine point AddMinePoint made (-1: none) */
    s8 mineCount;               /* 0x024: the mine point's count (Init: 3; Update copies MinePoint_Count) */
    u8 unk025[0x026 - 0x025];
    s8 partFlinch[7];           /* 0x026: by flinch part, parts 3 to 6: 0, then 1 after a first flinch (SetGroup4State states 2-5 and 9-12), 2 after a second (17-20); Update picks the next flinch state by it, and when all four are past 1, states 6 and 8 break part 1 and clear them. RemoveMinePoints (a copy of Lao-Shan's) removes entries 0 to 2 as mine points */
    u8 unk02D[0x100 - 0x02D];
} EmShenGaorenWork;
typedef char EmShenGaorenWork_size_check[(sizeof(EmShenGaorenWork) == sizeof(EmWork)) ? 1 : -1];

/* Lao-Shan Lung's own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmLaoshanWork {
    u8 unk000;                  /* 0x000: Setup and Init: 0; State6_00: 1; State3_04: 2; State3_05: 3 */
    s8 fortHits;                /* 0x001: State3_02: the hits on the fort, up to 10 (then Quest_TriggerEvent), copied to gScene.fortStage */
    s16 syncTimer;              /* 0x002: Init: 150; Update counts it down, then sends the monster's state (QSync_SendEm kind 2) and restarts it */
    u8 unk004[0x005 - 0x004];
    u8 fxStep;                  /* 0x005: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk006[0x008 - 0x006];
    s16 fxMotion;               /* 0x008: the motion id MotionEffects last saw (motionId[0] stored and compared; MotionSounds switches on it) */
    u8 unk00A;                  /* 0x00A: MotionEffects clears it on a new motion */
    u8 unk00B[0x00C - 0x00B];
    f32 unk00C;                 /* 0x00C: Init: 1500.0 */
    s16 unk010;                 /* 0x010: Init: 0; Update counts it up while unk01C is set */
    s8 mineSlot;                /* 0x012: the mine point AddMinePoint made (-1: none) */
    s8 mineCount;               /* 0x013: the mine point's count (Init: 3; Update copies MinePoint_Count) */
    s32 unk014;                 /* 0x014: Init: -1 */
    s32 unk018;                 /* 0x018: Init: -1 */
    u8 unk01C;                  /* 0x01C: State1_04: 1 when its turn (rotY up) ends; State1_08: 2 (rotY down); Init and the other group 1 states: 0 */
    u8 fxSpot;                  /* 0x01D: MotionEffects: the Eff22_CreateAtSpot spot, 0 to 3 in turn */
    s8 minePts[3];              /* 0x01E: AddMinePoints' three mine points at the bones of EmLaoshan_MinePtBones (-1: none) */
    u8 unk021[0x100 - 0x021];
} EmLaoshanWork;
typedef char EmLaoshanWork_size_check[(sizeof(EmLaoshanWork) == sizeof(EmWork)) ? 1 : -1];

/* Mosswine's own meanings of the work area's bytes (EmWork has the shared ones); kind 0x12's copies of its functions read the same layout. */
typedef struct EmMosswineWork {
    u8 fxStep;                  /* 0x000: kind 0x12's UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk001[0x00C - 0x001];
    u16 unk00C;                 /* 0x00C: Init: 0; Update counts it down to 0 (nothing in C sets it) */
    s16 unk00E;                 /* 0x00E: Init: 0 */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the position (Y stored as 0; kind 0x12's Init stores posY in Z's place); Mosswine's State1_05 turns toward it */
    u8 unk020[0x100 - 0x020];
} EmMosswineWork;
typedef char EmMosswineWork_size_check[(sizeof(EmMosswineWork) == sizeof(EmWork)) ? 1 : -1];

/* Khezu's own meanings of the work area's bytes (EmWork has the shared ones; types 0xF and 0x2D).
 * gm_sub qnet_sync.c sends and applies its bytes 0x44 and 0x45 (QSync_PackEmState, QSync_ApplyEmState). */
typedef struct EmKhezuWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x002 - 0x001];
    s16 fxMotion;               /* 0x002: the motion id MotionEffects and MotionSounds compare */
    u8 curveNone;               /* 0x004: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x005 */
    s8 curveSpeed;              /* 0x006 */
    u8 curveKind;               /* 0x007: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x008: the curve frame (StartCurves, UpdateCurves) */
    s8 unk00A;                  /* 0x00A: State2_1D counts it down */
    u8 unk00B[0x010 - 0x00B];
    f32 unk010;                 /* 0x010: SetGroup2State; SetGroup3State: 2500 */
    s16 unk014;                 /* 0x014: Init: 0x4000 */
    u8 unk016[0x017 - 0x016];
    u8 unk017;                  /* 0x017: SetGroup2State: 0 or 1 */
    u8 unk018[0x01C - 0x018];
    s32 unk01C;                 /* 0x01C: Init: 0 */
    s32 accelAngX;              /* 0x020: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x024 */
    s32 accelAngZ;              /* 0x028 */
    s32 unk02C;                 /* 0x02C: Init: 0x100 */
    u16 turnSpeed;              /* 0x030: Em_TurnToGoal's speed (Init stores 0x200 as a word) */
    u8 unk032[0x034 - 0x032];
    s32 unk034;                 /* 0x034: Init: 0x100 */
    u8 unk038[0x042 - 0x038];
    u16 zone;                   /* 0x042: SetGroup2State: compared with the area info's zone */
    u8 unk044;                  /* 0x044: Init: 0; Update clears it and unk045 when the monster is hit; synced online (message 8's state0) */
    u8 unk045;                  /* 0x045: synced online (message 8's state1); QSync_ApplyEmState kind 4 restarts the script on the new host while it is nonzero */
    u8 unk046[0x100 - 0x046];
} EmKhezuWork;
typedef char EmKhezuWork_size_check[(sizeof(EmKhezuWork) == sizeof(EmWork)) ? 1 : -1];

/* One of the 9 parts of a monster at +0x2C4 (Em_InitParts). */
typedef struct EmPart {
    u8 active;          /* 0x0: the part exists for this kind */
    u8 index;           /* 0x1 */
    u8 unk2;
    u8 unk3;
    s16 hp;             /* 0x4: the part's hit points (Em_InitParts, Em_ScalePartHp) */
    u8 state;           /* 0x6: its break state */
    u8 unk7;
} EmPart;

/* A pending placement of the monster (two at +0x8B4, 0x4C bytes each). */
typedef struct EmSpawn {
    s16 pending;        /* 0x00: nonzero until applied */
    u8 arg;                     /* 0x02: the argument given with the record (tested 0 before a new one) */
    u8 unk03;
    f32 pos[3];         /* 0x04 */
    u16 rot[3];         /* 0x10 */
    u8 stateGroup;              /* 0x16: Em.stateGroup or syncKind, restored as a state group */
    u8 state;                   /* 0x17: Em.state or syncArg */
    u8 unk18;
    u8 goalNo; /* 0x19: Em.syncGoalNo; restored to curGoalNo */
    u16 goalKind;               /* 0x1A: Em.syncGoalKind; restored to curGoalKind (Player code typed Em: a 900-3600 frame timer) */
    u16 goalArg;                /* 0x1C: Em.syncGoalArg; restored to curGoalArg */
    s16 timer;                  /* 0x1E: Em.timer; restored */
    u8 area;                    /* 0x20: Em.area; restored */
    u8 unk21;
    u16 rand;                   /* 0x22: Em.syncRand; restored to stateRand */
    u8 unk24[0x4C - 0x24];
} EmSpawn;

/* Per-kind bytes at gEmKindInfo (8 per kind). */
typedef struct EmKindInfo {
    u8 lowHpRequest; /* 0x0: the request made at low health (Em.lowHpNeed, priority 0x80) */
    u8 need1Request; /* 0x1: for Em.need1, priority 0x60 */
    u8 need2Request; /* 0x2: for Em.need2, priority 0x50 */
    u8 need3Request; /* 0x3: for Em.need3 out of combat, priority 0x40 */
    u8 need4Request; /* 0x4: for Em.need4, priority 0x70 */
    u8 pad5[3];      /* 0x5: 0 in all 74 entries; nothing reads them */
} EmKindInfo;

/* An object with a position and a radius (sub_main's tables of 32 at 0x5D8A70 and 0x5D8BF0). */
typedef struct EmObj {
    f32 pos[3];         /* 0x00 */
    f32 radius;         /* 0x0C */
    u8 goalArg; /* 0x10: matched with Em.curGoalArg (the monsters goal search); a player index for the sense objects */
    u8 goalNo; /* 0x11: matched with Em.curGoalNo */
    u8 kind;            /* 0x12 */
    u8 done;            /* 0x13: StageObj_K0C type 3 (placed by a player, gScene.placed3) clears it; nonzero ends it (StageObj_K0C_RunType3 removes it from gAreaObjList) */
    s8 timer;           /* 0x14: frames left in gSightBlockObjs/gFlashObjs (SightBlock_Tick, FlashObj_Tick remove it at 0); 3 for the Bulldrome dust, 5 for flashes */
    u8 pad15;
    s16 unk_Param16;    /* 0x16: StageObj_K0C type 3 sets it from StageObj_K0C_PlayerVals[var1] (0 or 200); no reader found */
    u16 area; /* 0x18: the area it is in (compared with Em.area) */
} EmObj;

/* Plesioth's own meanings of the work area's bytes (EmWork has the shared ones); em_plesioth.c, and sub_main
 * unk_3E0EC0.c's Em_AdjustKindBones (the fins). */
typedef struct EmPlesiothWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x007 - 0x001];
    u8 unk007;                  /* 0x007: State2_0A counts it down */
    u8 unk008[0x00A - 0x008];
    s16 unk00A;                 /* 0x00A: Reset: 0x4000 */
    u8 unk00C[0x00F - 0x00C];
    u8 finMode;                 /* 0x00F: EmPlesioth_SetFinMode: 0 still, 1 waving, 2 waving with the fins folded (hp at most a fifth), 3 folded */
    s16 unk010;                 /* 0x010 */
    u8 unk012[0x013 - 0x012];
    u8 unk013;                  /* 0x013 */
    s32 unk014;                 /* 0x014 */
    s32 unk018;                 /* 0x018 */
    s32 moveAngBX;              /* 0x01C: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 moveAngBY;              /* 0x020 */
    s32 moveAngBZ;              /* 0x024 */
    s32 unk028;                 /* 0x028: Reset: 0x100 */
    s32 unk02C;                 /* 0x02C: Reset: 0x200 */
    s32 unk030;                 /* 0x030: Reset: 0x100 */
    s8 unk034;                  /* 0x034 */
    s8 unk035;                  /* 0x035 */
    s8 unk036;                  /* 0x036 */
    u8 unk037[0x038 - 0x037];
    u8 curveNone;               /* 0x038: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x039 */
    u8 curveSpeed;              /* 0x03A */
    u8 curveKind;               /* 0x03B: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x03C: the curve frame (UpdateCurves adds the motion's step) */
    s16 finWait[4];             /* 0x03E: per fin (bones 37, 38, 39, 16), the frames before its wave starts again (EmPlesioth_FinWait1/2, FinRewait1/2) */
    u8 finState[4][2];          /* 0x046: per fin, [0] its wave's state (EmPlesioth_UpdateFinWave: 0 still, 1 waiting, 2 and 3 the two key tables), [1] its fold's (EmPlesioth_UpdateFinFold: 1 opening, 2 folding) */
    struct {
        u16 fold;               /* turn about x (folded to EmPlesioth_FinFoldTarget[i] in fin modes 2 and 3, 0 in mode 1) */
        u16 wave;               /* turn about y (EmPlesioth_FinKeys1/2's keys) */
    } finRot[4];                /* 0x04E: per fin, read by Em_AdjustKindBones (sub_main unk_3E0EC0.c) */
    s16 finFrame[4];            /* 0x05E: per fin, its wave's key frame */
    s8 finCount[4];             /* 0x066: per fin, the frames left in its fold */
    u8 unk06A[0x100 - 0x06A];
} EmPlesiothWork;
typedef char EmPlesiothWork_size_check[(sizeof(EmPlesiothWork) == sizeof(EmWork)) ? 1 : -1];

/* Blangonga's own meanings of the work area's bytes (EmWork has the shared ones); em_blangonga.c, and sub_main
 * unk_3E0EC0.c's Em_AdjustKindBones (rageTimer). */
typedef struct EmBlangongaWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x006 - 0x001];
    s16 curveFrame;             /* 0x006: the curve frame (StartCurves, UpdateCurves) */
    s32 accelAngX;              /* 0x008: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x00C */
    s32 accelAngZ;              /* 0x010 */
    u8 curveNone;               /* 0x014: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x015 */
    u8 curveSpeed;              /* 0x016 */
    u8 curveKind;               /* 0x017: the curve tables' index (StartCurves) */
    u8 unk018;                  /* 0x018: a count ChangeState advances when unk019 is set (Init: 0) */
    u8 unk019;                  /* 0x019 */
    u8 unk01A[0x01C - 0x01A];
    EmObj sense;                /* 0x01C: the object Em_SenseObjects fills (ChangeState removes it from the list) */
    s16 rageTimer;              /* 0x038: UpdateRageTimer counts it up to its limit */
    s16 unk03A;                 /* 0x03A */
    u8 unk03C[0x03E - 0x03C];
    u8 partSet[3];              /* 0x03E: SetPartFlag(part) sets one (Init: 1) */
    u8 tint[3];                 /* 0x041: Init: one of two colors (0x73 0xEB 0xFF, 0xCD 0x91 0xC3) */
    u8 unk044;                  /* 0x044: a mode 0 to 4 (Init, ChangeState) */
    u8 unk045;                  /* 0x045: Update counts it down (set to 30) */
    u16 pickSeed;               /* 0x046: a Quest_Random value (Start) EmBlangonga_PickWeightedPart picks with */
    u8 unk048;                  /* 0x048: State6_02: a flag */
    u8 unk049[0x04C - 0x049];
    f32 bonePos[3];             /* 0x04C: Init: Em_BonePos of bone 15 */
    u8 unk058[0x100 - 0x058];
} EmBlangongaWork;
typedef char EmBlangongaWork_size_check[(sizeof(EmBlangongaWork) == sizeof(EmWork)) ? 1 : -1];

/* A ring of StageObj_K0C type 1's buffer (0x34 bytes; StageObj_K0C_SetupType1 sets four up,
 * RunType1 places them in front of the camera, DrawType1 draws them). */
typedef struct EmFxRing {
    s16 slot;           /* 0x00 */
    s16 view;           /* 0x02: 0 drawn round the camera (the one seen from inside), else the ring's number + 1 */
    u8 unk04[0x14 - 0x04];
    f32 alpha;          /* 0x14: its opacity, 0 to 1 (timer / 120 while it fades in or out); DrawType1 draws 255 times it */
    u8 unk18[0x1C - 0x18];
    s16 unk1C;          /* 0x1C */
    u16 spin;           /* 0x1E: its angle about the view axis, turned by StageObj_K0C_Angles[ring] each frame */
    struct DrawSlot *node; /* 0x20 */
    u8 unk24[0x34 - 0x24];
} EmFxRing;
typedef char EmFxRing_size_check[(sizeof(EmFxRing) == 0x34) ? 1 : -1];

/* StageObj_K0C's buffer (StageObj.buf) for types 1 and 3: type 1's rings, then the object the
 * monsters see: type 1's sight block (SightBlock_Add), type 3's area object (made by
 * StageObj_K0C_CreateOnPlayer, ObjList_Add; StageObj_K0C_RunType3 ends when it is done). */
typedef struct StageObjK0CBuf {
    EmFxRing ring[6];   /* 0x000 */
    EmObj obj;          /* 0x138 */
} StageObjK0CBuf;
typedef char StageObjK0CBuf_size_check[(sizeof(StageObjK0CBuf) == 0x154) ? 1 : -1];

/* StageObj_K0C type 4's buffer: the carried object (StageObj_K0C_CreateAtPlayer sets it up from
 * the player's bone 0xE; RunType4 drops it, DrawType4 draws it). */
typedef struct StageObjK0CCarry {
    f32 mtx[16];        /* 0x00 */
    u8 pad40[0x4C - 0x40];
    u8 kind;            /* 0x4C: the player's carry >> 4: picks its scale, rotation and offset, and its model */
} StageObjK0CCarry;
typedef char StageObjK0CCarry_size_check[(sizeof(StageObjK0CCarry) == 0x50) ? 1 : -1];

/* A monster's routes (Em_FindAreaEntry's result for one area set): per area, a list of points. */
typedef struct EmRoutePt {
    f32 pos[3];                 /* 0x00 */
    f32 radius;                 /* 0x0C: the point counts as reached within it */
    u16 unk10;                  /* 0x10: nonzero for a point a route can start at */
    u16 unk12;                  /* 0x12: passed to EmScript_RoutePointScript */
} EmRoutePt;                    /* 0x14 */
typedef struct EmRoute {
    u16 area;                   /* 0x00: area id (Area_IdForCurrentTime) */
    s16 n;                      /* 0x02: number of points */
    EmRoutePt *pts;             /* 0x04 */
    f32 pos[3];                 /* 0x08 */
    u16 unk14;                  /* 0x14 */
    u8 unk16[2];
} EmRoute;                      /* 0x18 */
typedef struct EmRouteSet {
    u8 unk00[2];
    s16 n;                      /* 0x02: number of routes */
    EmRoute *routes;            /* 0x04 */
} EmRouteSet;

struct Em;
/*
 * A monster's handler table (Em.handlers): one per kind (sub_main's default
 * gEmNopHandlers, the game kinds' tables, gm_sub's D_0090DE10, the lobby's
 * townspeople gNpcHandlers). The player has the same layout (PlayerVt).
 */
typedef struct EmVt {
    void (*init)(struct Em *em);            /* 0x00: called when it is set (Em_InitHandlers) */
    void (*update)(struct Em *em);          /* 0x04: every frame (Em_RunAll) */
    void (*unk08)(struct Em *em);
    void (*unk0C)(struct Em *em);           /* 0x0C: Em_RunHook3 */
    u8 (*unk10)(struct Em *em, u8 arg);     /* 0x10: EmScript_Op79 */
    void (*appear)(struct Em *em);          /* 0x14: Em_RunAppearHook */
    s32 (*unk18)(struct Em *em);            /* 0x18: Em_UpdateDamage */
} EmVt;

/* A record that Em_ApplyActionStatus acts on. */
typedef struct EmAction {
    u8 unk00[0x12];
    u8 kind;            /* 0x12 */
    u8 unk13;           /* 0x13 */
    u8 unk14[2];
    s16 arg;            /* 0x16 */
} EmAction;

/* A row of gEmTraderMsgs (sub_main), the messages of the trader monster kind 0x0A: one per
 * Em.msgId. EmKind0A_ShowMessage copies the row into the message head at work + 0x20. */
typedef struct EmTraderMsg {
    u8 kind;            /* 0x0: what the talk does after it (EmKind0A_Trade): 0 closes it, 1 a choice (next or nextNo), 2 goes on to next, 3 the trade, 8 and 9 a random chat line; 1, 4, 5 and 0xA wait 30 frames first */
    u8 next;            /* 0x1: the message shown after it (for a choice, after the first answer); 0xFF ends the talk */
    u8 nextNo;          /* 0x2: for a choice, the message after the second answer; 0xFF ends the talk */
    u8 mode;            /* 0x3: Notice_OpenTalk's mode */
    char *text;         /* 0x4: the text; a "||" in it prints Em.msgItem's name (EmKind0A_SetMessage) */
} EmTraderMsg;
typedef char EmTraderMsg_size_check[(sizeof(EmTraderMsg) == 0x8) ? 1 : -1];

/* A kind's turning and bending limits (game gEmBendOrigins[kind]): Em_TurnToward turns the heading
 * toward a point and Em_BendStep bends the body by Em.bendAngle within them. Angles are 0x10000
 * to the turn. */
typedef struct EmBendRow {
    f32 ahead;          /* 0x00: the turn's origin, this far ahead of the monster along its facing */
    u8 unk04[2];        /* 0x04: not read in C (4, 0) */
    s8 bendBone[4];     /* 0x06: the bones Em.bendAngle[0..3] turn (sub_main Em_AdjustKindBones, for the kinds that bend) */
    u16 turnSpeed;      /* 0x0A: the turn's starting step per frame (Em_TurnToward's phase 1 and its resets) */
    u16 turnSpeedMax;   /* 0x0C: phase 2's top step (it eases to a half, then three eighths of it) */
    u16 turnAccel;      /* 0x0E: phase 2's change of the step per frame */
    u16 bendMax[4];     /* 0x10: the caps of the four bend angles (the excess goes to the next) */
    u16 slowAngle;      /* 0x18: the angle left under which the turn slows (/ 4, / 8) and the 0x8000 line test's margin */
    u8 pad1A[6];
} EmBendRow;
typedef char EmBendRow_size_check[(sizeof(EmBendRow) == 0x20) ? 1 : -1];

/* A kind's rage (game gEmRageTbl[kind], NULL for a kind that never enrages). */
typedef struct EmRageRow {
    s16 threshold;      /* 0x00: the rage buildup that enrages it (Em.rageThreshold; Em_InitRage) */
    s16 time;           /* 0x02: how long the rage lasts (Em.rageTimer; Em_StartRage, Em_StartRage2) */
    f32 motionSpeed;    /* 0x04: the motion speed while enraged (gm_sub's update) */
    f32 attackFactor;   /* 0x08: Em.attackFactor while enraged (times Em_RageScale1) */
    f32 damageFactor;   /* 0x0C: Em.damageFactor while enraged (times Em_RageScale2) */
    f32 buildupByHp[11]; /* 0x10: the factor on a hit's rage buildup by the hp left: 100%, from 90%, from 80% ... under 10% (Em_CheckRageByHp) */
    u32 pad3C;
} EmRageRow;
typedef char EmRageRow_size_check[(sizeof(EmRageRow) == 0x40) ? 1 : -1];

typedef struct Em {
    u8 active;                  /* 0x000: nonzero while the slot is in use */
    u8 visible;                 /* 0x001: nonzero while the monster is in the field (drawn; cleared when the carcass goes) */
    u8 type;                    /* 0x002: monster kind: index into the per-kind tables */
    u8 subType;                 /* 0x003: a sub-kind: effect and stage objects typed Em index their per-kind count tables with it (EmFxSeg_End, AreaObj_K02_End, EffKushala29_End); func_0070DBB0 tests 1 */
    u8 taskPhase;               /* 0x004: GmEm_Task: 0 setup, 1 update, 2 finish, 3 free */
    u8 step;                    /* 0x005: step within the current state (0 on entry) */
    u8 stateVar0;               /* 0x006: per-state scratch byte (flag or counter: "once" flags with Player_PassedFrameScaled, hit counts, table indexes); cleared with the step on every state change */
    u8 stateVar1;               /* 0x007: second per-state scratch byte like stateVar0 (flags, counters, 0xFF: none, a motion layer); cleared with the step on every state change */
    s32 timer;                  /* 0x008: counts down in many states (`if (--em->timer <= 0)`) */
    u16 index;                  /* 0x00C: slot number in the monster array (EmSlot_Alloc; o->idx) */
    u16 targetYaw;              /* 0x00E: heading to turn to, 0x10000 per turn: EmKind0A_TurnToYaw steps rotY to it; set from rotY, Vec_Heading(...) - 0x4000 or an action spot angle (func_0070E6C0); the player code (Player_BowgunAim, Bow_UpdateAim) moves it by 0x240 steps */
    u8 isEm;                    /* 0x010: 1 for a monster (EmSlot_Alloc; as Player.isEm, 0 for a hunter) */
    u8 gender;                  /* 0x011: as Player.gender (same offset): Anteka picks it at random or from the variant (EmAnteka_Init) and uses other motions when set (EmAnteka_SetMotion); Kirin and Hermitaur Init clear it */
    u8 weaponOut;               /* 0x012: as Player.weaponOut (same offset): only player code typed Em uses it (the weapon states set 1, Player_State* 0) */
    u8 slotNo;                  /* 0x013: the slot number again, as a byte (EmSlot_Alloc) */
    u8 stateGroup;              /* 0x014: state group, set with the state (sub_main 0x3AB7F0) */
    u8 state;                   /* 0x015: current state: the family's state dispatcher switches on it */
    u8 prevStateGroup;          /* 0x016: the state group before the last change */
    u8 prevState;               /* 0x017: the state before the last change */
    u8 hitKind;                 /* 0x018: as Player.hitKind (same offset): only player code typed Em uses it (the weapons' CheckHit functions) */
    u8 hitPending;              /* 0x019: as Player.hitPending (same offset, same writer): set when one of its attacks hits (ObjAttach_DamagePlayer); the attack creators (Eff00..Eff06_Create, EffEm07_CreateAt) clear it */
    u8 unk01A[0x01B - 0x01A];
    u8 variant;                 /* 0x01B: the variant from the quest monster entry (QuestEm_Spawn): size row for Em_ComputeSize and D_008C5370 (Vespoid), row of D_008C4140 (Drome kind 1D), Melynx 0xC/0x80, Hermitaur's shell (level % 3) */
    u8 unk01C[0x01E - 0x01C];
    u8 unk01E;                  /* 0x01E: cleared by GmEm_Setup (Player.unk01E, ObjHead.unk01E) */
    u8 unk01F[0x020 - 0x01F];
    f32 rotMtx[4][4];           /* 0x020: rotation matrix built from rotX..rotZ (Mtx_FromAnglesYXZ) */
    f32 worldMtx[4][4];         /* 0x060: the world matrix: scale, rotation, position (Em_UpdateWorldMatrix, Em_Draw: Mtx_Copy into it); Vec_Rotate turns by it (EmKind0A_State0_02, Player_StateLedgeHang/15/1E/44); the effects typed Em keep a counter in its byte 1 (Eff00_Update, Eff06_Fall) */
    s32 rotX;                   /* 0x0A0: rotation angles as integers, 0x10000 per turn (rotX..rotZ are read as a vector: Mtx_FromAnglesYXZ) */
    s32 rotY;                   /* 0x0A4: heading: rotation about y, 0x10000 per turn (Math_AngleXZ to the goal) */
    s32 rotZ;                   /* 0x0A8: rotation about z, 0x10000 per turn */
    f32 posX;                   /* 0x0AC */
    f32 posY;                   /* 0x0B0 */
    f32 posZ;                   /* 0x0B4 */
    f32 scaleX;                 /* 0x0B8: scale (scaleX..scaleZ; EM_BASESCALE times the size) */
    f32 scaleY;                 /* 0x0BC */
    f32 scaleZ;                 /* 0x0C0 */
    u8 unk0C4[0x154 - 0x0C4];
    MotionLayer motion[4];      /* 0x154: motion layers (include/motion.h; as motionId[4]: Em_SetupKind sets the fourth one's step) */
    u16 yawCenter;              /* 0x294: the heading targetYaw is kept within +-0x4000 of (Player_UpdateLean, motion 0x19E); Player_StateMountedGunAim sets it from the action spot angle (kind 6) or targetYaw */
    s16 tradeStep;              /* 0x296: step of EmKind0A_Trade (switch), 0 when EmKind0A_ShowMessage starts a message */
    u16 msgId;                  /* 0x298: the trader kind 0x0A: the message shown, a row of gEmTraderMsgs (EmKind0A_ShowMessage, EmKind0A_Trade) */
    s16 msgTimer;               /* 0x29A: the trader kind 0x0A: frames since msgId was shown (EmKind0A_Trade counts to 999) */
    u16 motionId[4];            /* 0x29C: motion id per layer: 1000 + 200 * layer + the row's motion (Em_SetMotion, Em_SetLayerMotion) */
    u16 motionFrame[4];         /* 0x2A4: start frame per layer (Em_SetMotion) */
    s16 motionBlend[4];         /* 0x2AC: blend frames per layer, halved (Em_SetMotion) */
    u8 unk2B4[0x2B8 - 0x2B4];
    u8 unk2B8;                  /* 0x2B8: as Player.unk2B8 (same offset): Player_UpdateAutoTurn clears it */
    u8 unk2B9[0x2BC - 0x2B9];
    u8 motionStarted[4];        /* 0x2BC: as Player.motionStarted (same offset): the lobby townspeople (Npc_UpdateMotion) begin the layer motion (Player_BeginLayerMotion) while it is 0, then set it */
    u16 motionLayerCount;       /* 0x2C0: as Player.motionLayerCount: number of motion layers in use (Em_SetMotion, Em_SetMotionRow loop over them; EmKind_MotionGroupCount(type) in GmEm_Setup, 1 or 2 in Em_SetupKind) */
    s16 hp;                     /* 0x2C2 */
    EmPart parts[9];            /* 0x2C4: 9 parts (Em_InitParts): active +0, index +1, hp +4, state +6 */
    u8 unk30C[0x30F - 0x30C];
    u8 modelSlot;               /* 0x30F: the scene's monster model slot (Quest_SpawnMonsters, QuestEm_Spawn: type = gScene.emKind[slot]); its textures are gTexHandles[slot * 20 + 0xA3 ...] (GmEm_SetMeshAlphas, StageObj_K09_Draw) */
    u8 unk310;                  /* 0x310: as Player.unk310 (same offset): Player_RunWeaponState clears it */
    u8 unk311[0x390 - 0x311];
    u8 stance;                  /* 0x390: 0: on the ground, 1: family-specific (Kushala kinds hovering, Lao-Shan, Shen Gaoren), 2: in the air (flying, leaping out of the sand), 3: dead (QuestEm_Hunted), 4: under the sand or water (Diablos, Cephadrome, Plesioth) */
    u8 unk391[0x395 - 0x391];
    u8 hitReceived;             /* 0x395: nonzero when the monster took a hit: Em_UpdateDamage handles the hit (or reports the death when 0 and hp <= 0), Player_ReactToHit reacts to it */
    u8 flinchPart;              /* 0x396: the part whose hp ran out on the last hit (the most damaged one); the families pick the flinch from it */
    u8 unk397[0x398 - 0x397];
    u32 flags;                  /* 0x398: as Player.flags: flag bits (Player_SetFlag/ClearFlag/TestFlag without bit 31; stateFlags at 0x39C with it); GmEm_Update and gm_sub func_00904CF0 set 0x20000 */
    u8 unk39C[0x3A2 - 0x39C];
    u16 stateRand;              /* 0x3A2: Rand_Next(1) drawn when a state starts; states pick variants from it */
    s32 motionTime;             /* 0x3A4: as Player.motionTime (same offset): Player_TickTimers adds 2 a frame, wraps 100000 to 10000; states reset it to 0 and compare it (> 18, >= 240, == 300, == 600) */
    s32 targetAngle;            /* 0x3A8: angle to the target player - 0x4000 (Em_FindTargetPlayer) */
    u8 unk3AC[0x3B0 - 0x3AC];
    f32 targetDist;             /* 0x3B0: distance to the target player (Em_FindTargetPlayer) */
    void *target;               /* 0x3B4: the nearest player in range, NULL when none (Em_FindTargetPlayer) */
    f32 velX;                   /* 0x3B8: velocity x, in the monster frame (EmKind38_MoveByVelocity adds accel to it, rotates it by the heading and adds it to the position) */
    f32 velY;                   /* 0x3BC: vertical velocity (the curves of the families UpdateCurves) */
    f32 velZ;                   /* 0x3C0: forward speed (the curves of the families UpdateCurves) */
    f32 accelX;                 /* 0x3C4: acceleration x, added to velX each frame */
    f32 accelY;                 /* 0x3C8: acceleration y: gravity in the jump and fall states (-10.0f, -2.0f...) */
    f32 accelZ;                 /* 0x3CC: acceleration z */
    struct EmVt *handlers;      /* 0x3D0: the kind's handler table (EmVt): init, update, ...; Em_InitHandlers sets the default one */
    u8 unk3D4[0x3EB - 0x3D4];
    u8 unk3EB;                  /* 0x3EB: lobby Npc4F_State58 sets 1 while the player is in gNpc4FArea rising or above the NPC, Npc4F_State5D tests it */
    u8 unk3EC[0x3EF - 0x3EC];
    u8 hitAttrs;                /* 0x3EF: as Player.hitAttrs (same offset): the attack attribute bits ORed in by ObjAttach_DamageEm; bit 0x80 makes Player_ReactToHit halve the damage */
    u16 hitAngle;               /* 0x3F0: direction of the last hit taken, 0x10000 per turn (ObjAttach_SetHitHeading); states face it with rotY = hitAngle + 0x8000 */
    s16 hitForce;               /* 0x3F2: the attack's hitForce at the last hit (ObjAttach_DamageEm; as Player.hitForce) */
    u16 lastAttackKind;         /* 0x3F4: as Player.lastAttackKind: the kind of the attack of the last hit (ObjAttach_DamageEm: attack object +0x63); Em_UpdateDamage and Em_PickStatus test 5, 6, 9, 0x13 */
    u8 unk3F6[0x3F8 - 0x3F6];
    u8 lookAtTarget;            /* 0x3F8: nonzero: Em_UpdateBendTarget bends the body toward the target player (toward lookPoint when lookAtPoint is set), else straight; 1 while Em_TurnToTargetMotion and some group-3 states run, cleared on entry of almost every state */
    u8 hitIds[16];              /* 0x3F9: the ids of the attacks that hit it last (a ring, hitIdNext the next slot; ObjAttach skips an attack whose id is in it) */
    s8 side;                    /* 0x409: as Player.side (same offset): copied into the attacks (Eff*_Create o->unk0D1), Side_Match against players (EmScript_Op52, GmEm_Setup), versus team (Versus_Team: size and rage scales); the carving player is gPlayers[side - 1] (State8_00) */
    u8 hitStopAdd;              /* 0x40A: as Player.hitStopAdd (same offset): only player code typed Em uses it (Lance, Gunlance, Bow CheckHit) */
    u8 hitStop;                 /* 0x40B: as Player.hitStop (same offset): only player code typed Em uses it */
    u8 unk40C[0x40E - 0x40C];
    s16 noHitTimer;             /* 0x40E: frames the monster cannot be hit: ObjAttach_HitPlayer skips it while nonzero; counted down by Player_TickTimers (GmEm_Update); states set 5/10/2, GmEm_Update 2 in group 8 (carving); Em_CheckFlash ignores flashes while > 0; players: Player_SetEvadeFrames */
    s16 noPushTimer;            /* 0x410: frames without body pushing: Field_PushApart skips the object while nonzero; counted down by Player_TickTimers; states set 5/2/10 */
    u8 hitIdNext;               /* 0x412: the next slot of hitIds, 0..15 (cleared by EmSlot_Alloc) */
    u8 unk413[0x414 - 0x413];
    u8 unk414;                  /* 0x414 */
    u8 unk415[0x416 - 0x415];
    u8 unk416;                  /* 0x416 */
    u8 unk417;                  /* 0x417 */
    s16 rowBlend;               /* 0x418: blend frames of the motion row (+7 of the row: Em_SetMotion, Em_SetMotionRow store it); Em_SetMotion uses it instead of the given blend when nonzero */
    u8 unk41A[0x436 - 0x41A];
    u8 unk436;                  /* 0x436: Quest_SpawnMonsters sets 1 for its sixth spawn (j == 5), 0 for the other model-slot-3 ones */
    u8 fieldCollide;            /* 0x437: 1: func_00330EB0 collides the monster with the walls (spheres per kind) and fills wallHitDirs; 0 in hunted, captured and some entrance states; 1 from Em_SetupKind, GmEm_ResetState, the EndState functions */
    u8 work;                    /* 0x438: first byte of the state functions' work area (EmWork, em_work.spec); the families also read its bytes as Em fields */
    s8 fortStage;               /* 0x439: the fort monsters: the hits on the fort (their views' fortHits), copied to gScene.fortStage; sent in message 8 (QSyncEmData state1, QSync_PackEmState); EmWork byte 1 */
    s16 syncTimer;              /* 0x43A: the fort monsters (their views' syncTimer, work +2): frames to the next state message; QSync_PackEmState restarts it at 150 when it sends one */
    u8 unk43C;                  /* 0x43C */
    u8 unk43D;                  /* 0x43D */
    s16 unk43E;                 /* 0x43E */
    u8 unk440;                  /* 0x440 */
    u8 minePoint;               /* 0x441: Gypceros: the mine point it leaves (MinePoint_AddUntimed; EmKutku_AddMinePoint) */
    s16 unk442;                 /* 0x442 */
    s16 unk444;                 /* 0x444 */
    s16 unk446;                 /* 0x446 */
    s16 unk448;                 /* 0x448 */
    u8 unk44A;                  /* 0x44A */
    u8 unk44B;                  /* 0x44B */
    s32 unk44C;                 /* 0x44C */
    u8 unk450;                  /* 0x450 */
    u8 unk451;                  /* 0x451 */
    u8 unk452;                  /* 0x452 */
    u8 unk453;                  /* 0x453: Gravios (0x1A, 0x2C): GmEm_SetPartFx fades the part colours by it (read as s8, / 60); EmGravios_PlaySeOnFrame tests 1 */
    u8 unk454;                  /* 0x454 */
    u8 unk455[0x456 - 0x455];
    u8 unk456;                  /* 0x456 */
    u8 unk457[0x458 - 0x457];
    s32 unk458;                 /* 0x458 */
    s32 unk45C;                 /* 0x45C */
    f32 unk460;                 /* 0x460 */
    s32 unk464;                 /* 0x464 */
    s32 unk468;                 /* 0x468 */
    s32 unk46C;                 /* 0x46C */
    u8 unk470;                  /* 0x470 */
    u8 unk471;                  /* 0x471 */
    u8 unk472;                  /* 0x472 */
    u8 unk473;                  /* 0x473 */
    s32 unk474;                 /* 0x474 */
    s32 unk478;                 /* 0x478 */
    s32 unk47C;                 /* 0x47C */
    u16 unk480;                 /* 0x480 */
    u8 unk482;                  /* 0x482 */
    u8 npcAct;                  /* 0x483: lobby NPCs (an Em): the NPC's act (Npc_IsFree, Npc_IsAct51to55, Npc_IsAct4For50) */
    u16 unk484;                 /* 0x484 */
    u8 unk486;                  /* 0x486 */
    u8 unk487;                  /* 0x487 */
    s16 unk488;                 /* 0x488 */
    s16 unk48A;                 /* 0x48A */
    s16 unk48C;                 /* 0x48C */
    s16 unk48E;                 /* 0x48E */
    s16 unk490;                 /* 0x490 */
    s16 unk492;                 /* 0x492 */
    s16 unk494;                 /* 0x494 */
    u8 unk496;                  /* 0x496 */
    u8 unk497[0x498 - 0x497];
    s32 unk498;                 /* 0x498 */
    u8 unk49C[0x4A6 - 0x49C];
    u8 unk4A6;                  /* 0x4A6 */
    u8 unk4A7[0x4B8 - 0x4A7];
    u16 statusIcons;            /* 0x4B8: as Player.statusIcons: StageObj_K05_* and EmFxSeg_Move read bit 0x8000 of the player they are given */
    u8 unk4BA[0x4BC - 0x4BA];
    struct ObjAttach *hitAttach;/* 0x4BC: the attack object of the last hit taken (ObjAttach_DamageEm; as Player.hitAttach) */
    u8 unk4C0[0x4C8 - 0x4C0];
    u8 meshShown[32];           /* 0x4C8: one per model mesh, 1: drawn (in the EmWork area: work + 0x90): Em_SetupKind fills it from the variant tables gEmVariantShownMeshes/gEmVariantMeshes, GmEm_Setup sets all 32, Em_UpdateState cycles the variants; Diablos/Garuga/Gravios/Rathalos State4_04 clear element 1 at frame 6 */
    s16 modelCount;             /* 0x4E8: model slots used (Model_Alloc; EmSlot_SetupModel, EmSlot_Free) */
    s16 modelFirst;             /* 0x4EA: the first model slot */
    struct Model *model;        /* 0x4EC: skeleton: bones and their matrices */
    u8 unk4F0[0x52C - 0x4F0];
    struct DrawSlot *drawSlot;  /* 0x52C: its draw slot (DrawSlot_Get), positioned at the Em every frame and put into an ordering table (gm_sub func_00905930, the lobby's townspeople) */
    s16 drawSlotId;             /* 0x530: an entry of the 0x6A5AD0 pool, -1: none (DrawSlot_Alloc; gm_sub func_00903EE0) */
    u8 painted;                 /* 0x532: 1 while marked by a paint hit: ObjAttach_DamageEm sets it (attack kind 7) with paintTimer = 0x4650 frames; Player_TickTimers-side code (unk_399F70) counts paintTimer down and clears it (0xFF: never); Map_MonsterLocated shows the monster while set; synced online (QSync flag 0x10) */
    u8 carry;                   /* 0x533: as Player.carry (same offset): only player code typed Em uses it */
    u8 ammoType;                /* 0x534: as Player.ammoType (same offset): only player code typed Em uses it (func_0070DBB0) */
    u8 savedAmmoType;           /* 0x535: as Player.savedAmmoType (same offset): only player code typed Em uses it */
    u8 ammoMenuOpen;            /* 0x536: as Player.ammoMenuOpen (same offset): only player code typed Em uses it */
    u8 unk537[0x53A - 0x537];
    s16 paintTimer;             /* 0x53A: frames the paint mark lasts: 0x4650 (ObjAttach_DamageEm, QSync_OnEm), counted down while painted is set, which clears at 0 */
    u8 unk53C[0x564 - 0x53C];
    u8 collideFlags;            /* 0x564: Em_CollideField clears bit 0 first */
    u8 unk565[0x568 - 0x565];
    f32 prevPosX;               /* 0x568: position before this frame's move (prevPosX..prevPosZ, read as a vector: Vec_Copy): GmEm_Update and Em_UpdateState store posX..posZ, Player_Update the player position; func_00330EB0 sweeps the wall collision from it; func_00708850 puts the monster back there */
    f32 prevPosY;               /* 0x56C: y of the position before this frame's move (see prevPosX) */
    f32 prevPosZ;               /* 0x570: z of the position before this frame's move (see prevPosX) */
    f32 floorY;                 /* 0x574: height of the floor under the monster (Em_GetFloorY, Em_GetFloorInfoEx) */
    u8 unk578[0x584 - 0x578];
    f32 bodyScale;              /* 0x584: scale of the wall collision shapes (func_00330EB0) and of the carcass mine radius (EmMine_Create); D_8B1620[type] (GmEm_ResetState), the scale for the small kinds (Vespoid, Anteka, Apceros, Drome kind 1D Init) */
    u8 rideState;               /* 0x588: as Player.rideState (same offset): Em_GetFloorInfo tests it; the player states typed Em set it */
    u8 unk589[0x590 - 0x589];
    u16 turnTarget;             /* 0x590: the angle Em_TurnToward turns to (EmTurnView of unk_701810.c) */
    u16 turnHeading;            /* 0x592: the current heading of Em_TurnToward (EmTurnView) */
    u8 unk594[0x596 - 0x594];
    u8 pushPriority;            /* 0x596: as Player.pushPriority (same offset): compared between two bodies (PlayerEm_PushApart, EmEm_PushApart); Player_UpdateNearMonster reads the monster's */
    u8 unk597[0x59A - 0x597];
    u8 cartPlayer;              /* 0x59A: the Felyne cart (kind 0x12): the player it carries (Player_StateCart sets it; EmKind12_State1_01 waits while that player's stateGroup is 4) */
    s8 targetNo;                /* 0x59B: index in gPlayers of the target player, -1: none */
    u8 unk59C[0x628 - 0x59C];
    s8 tutorialSlot;            /* 0x628: lobby NPCs in the village tutorial: the bit of gTutorial.mask that is this NPC (Tutorial_RunNpc; -1 none): its point on the route set, and the kind ThrownObj_Spawn gets in Npc51 state 3 */
    u8 unk629[0x634 - 0x629];
    s16 unk634;                 /* 0x634 */
    u16 msgItem;                /* 0x636: the item EmKind0A_SetMessage prints in its message (gItemNames), 0 at EmKind0A_Init */
    void *sightTable;           /* 0x638: the kind's sight and sense ranges (an EmSightRange: Em_SeePlayers; f32 [6] the action reach of Em_NearestActionToBone, [7] the sense reach of Em_SensePlayers) */
    u32 bendAngle[4];           /* 0x63C: the four bend angles Em_BendStep turns toward an angle (|angle| / 2, / 4, / 8, / 16, each capped by the kind's gEmBendOrigins row; EmBendView); GmEm_ResetState clears them */
    u16 turnSpeed;              /* 0x64C: Em_TurnToward's speed (EmTurnView) */
    u16 turnHold;               /* 0x64E: EmTurnView: hold frames of Em_TurnToward */
    u8 turnPhase;               /* 0x650: EmTurnView: the phase of Em_TurnToward */
    u8 lookAtPoint;             /* 0x651: 1: Em_UpdateBendTarget bends toward lookPoint instead of the target player (Rathalos State3_0C) */
    u8 motionRemap;             /* 0x652: nonzero: Em_SetMotion takes the row's remap row of the alternate table once, then clears it */
    u8 stateRequest;            /* 0x653: as Player.stateRequest: 1 after Em_SetState (Player_ForceState): the family Update runs the new state a second time in the same frame, then clears it */
    f32 lookPoint[3];           /* 0x654: f32[3]: the point bent toward when lookAtPoint is set (Em_NearestActionToBone with bone 0x23; Rathalos State3_0C) */
    u8 floorKind;               /* 0x660: the floor triangle under the monster (TriHdr, unk_328CD0.c): Em_GetFloorInfo and Em_GetFloorInfoEx take 0x660..0x663 as their info: kind (TriHdr.kind) */
    u8 floorArg;                /* 0x661: byte 1 of that floor triangle (TriHdr.unk01; Player +0x661 goes with the voice SEs) */
    u16 floorAttr;              /* 0x662: the u16 at +2 of the floor triangle (TriHdr.unk02, Em_GetFloorInfo): EmScript_Op5F and Op62 test 1..3, EmBlangonga_ScriptQuery 1, func_00708640 nonzero without bit 0x8000; GmEm_ResetState sets 1 (Player +0x662: compared by EmScript_Op5A) */
    u8 unk664[0x668 - 0x664];
    u8 unk668;                  /* 0x668: as Player.unk668 (same offset): Player_StateCrouchWalk/3D test it with the dodge button */
    u8 unk669[0x680 - 0x669];
    u16 area;                   /* 0x680: the area (map zone) the monster is in (gScene.unk014 is the current one) */
    u8 areaMoveState;           /* 0x682: as Player.areaMoveState (same offset): only player code typed Em uses it */
    u8 unk683[0x684 - 0x683];
    u16 nextArea;               /* 0x684: the area the monster moves to (goalKind 3; GmEm and Em_PickRoutePoint move it there) */
    u8 unk686[0x688 - 0x686];
    f32 startPos[3];            /* 0x688: where the monster starts (as Player.startPos): EmKind0A_Init copies posX..posZ */
    u8 unk694[0x698 - 0x694];
    s32 wallHitDirs;            /* 0x698: directions of the walls touched this frame, one bit per 1/32 turn relative to the heading (func_00330EB0: 1 << ((h - rotY) >> 11)); states test 0xF000000F (ahead) to stop a charge */
    u16 unk69C;                 /* 0x69C: as Player.unk69C (same offset): Player_UpdateAutoTurn's turn angle (moved by 0x300 a frame, kept in 0xA00..0xF600) */
    u8 unk69E[0x6A0 - 0x69E];
    f32 mapPos[3];              /* 0x6A0: as Player.mapPos: the position for the map (Map draws the icons from it) */
    u8 unk6AC[0x6AE - 0x6AC];
    u8 hitReactMode;            /* 0x6AE: how Em_UpdateDamage reacts to a hit: 0 normal flinch, 2 no flinch (reaction 0xA; Em_BeginStatus1/2, captured states set it every frame through Em_SetHitReactMode), 3 reaction 0xD (EmFatalis_KnockDown), 4 kept through the hit (Em_BeginStatus4/5); Em_ClearHitReactMode resets it each frame; EmScript_Op0A sets, Op32/Op33 test it */
    u8 unk6AF[0x6B0 - 0x6AF];
    u8 aimMode;                 /* 0x6B0: as Player.aimMode (same offset): only player code typed Em uses it */
    u8 unk_InitFlag;            /* 0x6B1: every family Init (or Reset) sets 1 (Cephadrome and Yama Tsukami also 0); no reader found in C */
    s16 hitPartDamage[9];       /* 0x6B2: the hit's damage per part (EmDamageView of unk_70A290.c): Em_PickDamagedPart and Em_UpdateDamage scale it by damageFactor into parts[i].hp; [8] takes the damage of the part partFlags names */
    s16 reactValue[4];          /* 0x6C4: one per player: Em_UpdateReactions runs the reaction of a player whose value changed (EmReactView) */
    s16 reactValueLast[4];      /* 0x6CC: the last seen copies of reactValue (EmReactView) */
    u8 partFlags[9];            /* 0x6D4: one per part (as parts[k]): bit 2 lets the part break (EmBlangonga_BreakParts, Velociprey), 0x20 (Kushala BreakParts below 60% hp, func_0083B8F0) */
    u8 unk6DD[0x6DE - 0x6DD];
    s16 hpMax;                  /* 0x6DE */
    u8 syncKind;                /* 0x6E0: kind of the last synced state change (Em_SyncState, large monsters), sent online (QSync_SendEm msg 0x16, QSync_QueueEmState) */
    u8 syncArg;                 /* 0x6E1: second argument of the last synced state change (Em_SyncState), sent online (msg 0x17) */
    u8 unk6E2[0x6E3 - 0x6E2];
    u8 syncGoalKind;            /* 0x6E3: curGoalKind at the last synced state change (Em_SyncState), sent online */
    f32 alpha;                  /* 0x6E4: draw opacity, 1.0: opaque (fades in and out; GmEm_Task sorts by it) */
    u8 unk6E8[0x6EC - 0x6E8];
    struct Player *contact;     /* 0x6EC: the player of the last contact: the one it hit or that hit it (ObjAttach_DamagePlayer, with the attack at +0x6F0); Kut-Ku and Chameleos steal from it; in the lobby the player talking to the townsperson */
    struct ObjAttach *contactAttach;/* 0x6F0: the attack of that contact (ObjAttach_DamagePlayer); Kut-Ku State3_0B tests its kind 0x11 */
    u8 syncGoalArg;             /* 0x6F4: curGoalArg at the last synced state change (Em_SyncState), sent online */
    u8 syncGoalNo;              /* 0x6F5: curGoalNo at the last synced state change (Em_SyncState), sent online */
    s16 status5Buildup;         /* 0x6F6: raised by Em_AddStatus5; reaching status5Threshold sets statusFlags 0x10 */
    s16 status5Decay;           /* 0x6F8: frames to the next decay of status5Buildup */
    s16 status5Hit;             /* 0x6FA: the hit's status 5 amount: Em_PickDamagedPart passes it to Em_AddStatus5 (EmDamageView) */
    s16 status2Threshold;       /* 0x6FC */
    s16 status2Buildup;         /* 0x6FE: raised by Em_AddStatus2; reaching status2Threshold sets statusFlags 2 */
    s16 status2Hit;             /* 0x700: the hit's status 2 amount (Em_AddStatus2; EmDamageView) */
    s16 status2Decay;           /* 0x702: frames to the next decay of status2Buildup */
    s16 poisonThreshold;        /* 0x704: from D_008BCCB0 (per kind); poisonBuildup reaching it poisons the monster */
    s16 poisonBuildup;          /* 0x706: raised by Em_AddPoison; reaching poisonThreshold poisons the monster (statusFlags 4) */
    s16 poisonHit;              /* 0x708: the hit's poison amount (Em_AddPoison; EmDamageView) */
    s16 poisonTimer;            /* 0x70A: set from parameter +2 of the kind while the buildup rises, 0 once poisoned */
    s16 poisonDamage;           /* 0x70C: parameter +8 of the kind, set when poisoned (Em_AddPoison) */
    s16 status4Threshold;       /* 0x70E */
    s16 status4Buildup;         /* 0x710: raised by Em_AddStatus4; reaching status4Threshold sets statusFlags 8 */
    s16 status4Hit;             /* 0x712: the hit's status 4 amount (Em_AddStatus4; EmDamageView) */
    s16 status4Decay;           /* 0x714: frames to the next decay of status4Buildup */
    s16 status1Threshold;       /* 0x716 */
    s16 status1Buildup;         /* 0x718: raised by Em_AddStatus1; reaching status1Threshold sets statusFlags 1; decays by the kind's decayAmount every decayInterval frames */
    s16 status1Hit;             /* 0x71A: the hit's status 1 amount (Em_AddStatus1; EmDamageView) */
    s16 status1Decay;           /* 0x71C: frames to the next decay of status1Buildup (Em_UpdateStatus) */
    u8 status;                  /* 0x71E: current status (1, 2, 4, 5), 0 when none; 5 is the stun (dizzy: the families stun states make StageObj_Stun); poison is statusFlags 4 */
    u8 statusFlags;             /* 0x71F: 1: status 1, 2: status 2, 8: status 4, 0x10: status 5 */
    u16 partMask;               /* 0x720: parts whose hp scales with level, one bit each (Em_SetPartScaled, Em_ScalePartHp) */
    u8 hideShadow;              /* 0x722: nonzero: Effect01_Draw (the flat model drawn under the object, probably the shadow) skips it; GmEm_Update sets it from unk723 or stance 4 (under sand or water) unless hideShadowHold holds the value states set (2); counted down by Player_TickTimers */
    u8 shadowOff;               /* 0x723: nonzero: GmEm_Update sets hideShadow (as under the sand, stance 4) unless hideShadowHold runs */
    f32 attackFactor;           /* 0x724: as Player.attackFactor (same offset): the attack factor ObjAttach_DamagePlayer reads; the kind's rage row +8 times Em_RageScale1 while enraged, Em_RageScale1 (gEmScaleRows row, versus team scale) otherwise */
    f32 damageFactor;           /* 0x728: multiplies the part damage the monster takes (Em_UpdateDamage, Em_PickDamagedPart); the rage row +0xC times Em_RageScale2 while enraged, Em_RageScale2 otherwise (Player has defenseFactor at this offset) */
    s32 surfaceDepth;           /* 0x72C: an f32 kept in an s32 (read through casts): how far below surfaceY a swimmer stays */
    f32 surfaceY;               /* 0x730: height of the special floor found by Em_GetFloorInfoEx (water or sand); swimmers keep posY at surfaceY - surfaceDepth */
    u8 floorUnderSurface;       /* 0x734: 1 when Em_GetFloorInfoEx found the floor below the surface depth (on: low && floor < depth), else 0; Plesioth (EmPlesioth_OverDeepWater) tests it */
    u8 surfaceKind;             /* 0x735: nonzero when the floor under the monster is a special surface (byte 6 of the floor material row D_0043FB70[area][kind]: sand or water; Em_GetFloorY, Em_GetFloorInfoEx); GmEm_Update then puts floorY at surfaceY for most kinds; Diablos and Cephadrome dive only where it is set */
    s16 unk736;                 /* 0x736: 2 while Em.wallHitDirs is set (gm_sub func_00904CF0) */
    u8 pushed;                  /* 0x738: set on the monsters and players the push-apart moved (Field_PushApart and the pair pushes of unk_3AC9C0.c); Field_UpdateAll collides those with the field again, then clears it */
    u8 unk739[0x73A - 0x739];
    u8 combatPlayers;           /* 0x73A: bit per player (1 << index) the monster fights: Player_UpdateInBattle sets them, Em_SetCombat and Hud_DrawPlayerStatus test them, Em_UpdateDamage clears them; 0xF (all) for Fatalis and Shen Gaoren */
    u8 status2Count;            /* 0x73B */
    u8 poisonCount;             /* 0x73C: times poisoned: Em_UpdateStatus raises poisonThreshold each time up to 4 (as status2Count, status4Count) */
    u8 status4Count;            /* 0x73D */
    u8 status1Count;            /* 0x73E */
    u8 unk73F[0x746 - 0x73F];
    s16 mashLeft;               /* 0x746: as Player.mashLeft (same offset): only player code typed Em uses it */
    s16 mashBuildupHit;         /* 0x748: as Player.mashBuildupHit: the hit amount Em_ApplyHitStatus adds (flag 0x100); cleared by Hit_ClearStatusAll */
    u8 unk74A[0x74C - 0x74A];
    u8 attackHit800;            /* 0x74C: set to 1 when one of its attacks with flag 0x800 hits (ObjAttach_DamagePlayer); the attack creators clear it; Melynx steal states (State1_04, State3_01) test it with contact */
    u8 unk74D[0x754 - 0x74D];
    u8 ***script;               /* 0x754: the monster's script set (GmLoad_GetEmData): tables of script pointers by kind: [0] the main scripts (scriptNo, Em_StartScript), [1] the subscripts (EmScript_Op81), [2] to [14] the loop, trigger and area tables (unk_733B90.c), [0xF + k] EmScript_Op82's jump tables */
    u8 *mainScriptPos;          /* 0x758: the main script's position (Em_StartScript's result; EmScript_GetPos/SetPos with no scriptFlags bit 0x01, 0x10, 0x20) */
    u8 *scriptCallReturn;       /* 0x75C: the script position after an EmScript_Op81 call (Op81 saves p + 1) */
    u8 *scriptJumpReturn;       /* 0x760: the script position EmScript_Op82 saves before its jump */
    f32 reactDist[2];           /* 0x764: the reaction set's distance limits 0 and 1 (Em_LoadReactionSet, script command 0x83); limit 2 is nearRadius, limit 3 at +0x770 lies under the Player fields inBattle, prevInBattle */
    f32 nearRadius;             /* 0x76C: Velocidrome and Velociprey: a radius: the player distance (playerDist[curGoalNo]) is compared with it, and EmList_FindKindNear looks for its kind within it */
    u8 unk770[0x772 - 0x770];
    u8 inBattle;                /* 0x772: as Player.inBattle (same offset): Player_EnteredBattle compares it with prevInBattle */
    u8 prevInBattle;            /* 0x773: as Player.prevInBattle (same offset) */
    u8 scriptNo;                /* 0x774: the script to run next (EmScript_Start) */
    u8 scriptCallNo;            /* 0x775: the subscript EmScript_Op81 calls (script[1][scriptCallNo]); EmScript_Init clears it */
    u8 scriptCallEntry;         /* 0x776: entry of the EmScript_Op82 jump (script table k + 0xF); em_bulldrome.c reads 0x776..0x777 as one s16 */
    u8 scriptCallEntryHi;       /* 0x777: cleared when the Em is set up (gm_em.c); the high byte of em_bulldrome.c s16 read */
    u8 scriptCallDepth;         /* 0x778: cleared when a script starts; EmScript_Op81 sets 1 (saving the return at unk758 + 4 from 0), EmScript_Op82 sets 2 (saving it at unk758 + 8 from 1) */
    u8 goalKind;                /* 0x779: what the goal is (Em_ComputeGoal): 1 a player, 2 a point of the area, 3 another area, 5 the kind's point, 6 relative to the heading, 8 objectPos, 9 a route point */
    s16 goalArg;                /* 0x77A: first argument of goalKind (table, route or player) */
    s16 goalNo;                 /* 0x77C: second argument of goalKind (point or player index) */
    u8 scriptCaseCount;         /* 0x77E: EmScript_Op83: the number of cases it skips through */
    u8 routedStateArg;          /* 0x77F: third argument of script command 5 (Em_ChangeStateRouted, EmScript_Run); func_00733A30 switches on it */
    u8 *loopScriptPos;          /* 0x780: the position of the loop script EmScript_Op0F starts (scriptFlags 0x01; EmScript_GetPos/SetPos) */
    u8 loopMode;                /* 0x784: the script loop kind (EmScript_Op0F byte 0: 0, 1, 2 loop over the area points; Op10 and OpFF switch on it) */
    u8 loopPointTable;          /* 0x785: the point list of the loop (EmScript_Op0F; EmScript_LoopPointList, read by Op10 and OpFF) */
    u8 loopScript;              /* 0x786: the script table entry run at each loop point (EmScript_Op0F; EmScript_LoopScript) */
    u8 loopEndScript;           /* 0x787: the script table entry run when the loop ends (EmScript_Op0F; EmScript_OpFF, EmScript_LoopScript) */
    u8 *trigger10Pos;           /* 0x788: the script of trigger 0x10 (EmScript_PickTrigger; scriptFlags 0x10) */
    u8 scriptCombat;            /* 0x78C: combat when the EmScript_Op0B block started (Op0B, OpFF after Em_SetCombat) */
    u8 scriptPending;           /* 0x78D: 1: run the script once (Em_UpdatePendingScript calls EmScript_Run when scriptDelay is down to 0, then clears it); set by Init and the EndState functions, 0 by ChangeState and captured/hunted states */
    u8 scriptVar;               /* 0x78E: a script value: EmScript_Op0C sets it, Op0D clears it, Op2B compares it */
    u8 scriptFlags;             /* 0x78F: the running script thread: 0x20, 0x10 trigger scripts, 1 a loop (EmScript_Op0F); EmScript_GetPos picks the resume position by it */
    u8 *trigger20Pos;           /* 0x790: the script of trigger 0x20 (EmScript_PickTrigger; scriptFlags 0x20) */
    u8 *subScriptReturn;        /* 0x794: the position after an EmScript_Call (it saves p + 1; EmScript_OpFF command 3 returns there) */
    u8 scriptPlayer;            /* 0x798: the player the script picked (s8, -1: none; low nibble a gPlayers index): EmScript_Op45/Op52/Op53/Op5F pick it, Op46 and Op49 use it, Em_ComputeGoal makes it the goal in combat */
    u8 subScriptNo;             /* 0x799: the subscript EmScript_Call runs (EmScript_SubScript) */
    u8 areaSeqTable;            /* 0x79A: the area list of the area sequence (EmScript_Op17 byte 0; Op18 reads table 14 at areaSeqPos) */
    u8 areaSeqScript;           /* 0x79B: the script table entry of the area sequence (EmScript_Op17; Op18, EmScript_AreaSeqScript) */
    u8 *areaSeqNext;            /* 0x79C: the script position EmScript_Op18 saves */
    u8 areaSeqMode;             /* 0x7A0: EmScript_Op17 byte 1: Op18 and OpFF test it for 0 */
    u8 areaSeqActive;           /* 0x7A1: 1 while the area sequence of EmScript_Op17/Op18 runs (Op18 sets it, OpFF clears it at the end); the SetGroup2State functions then aim the takeoff and landing at point 0 of table 3 */
    u8 scriptRestart;           /* 0x7A2: 1: EmScript_Restart restarts the script (clears scriptPending, scriptFlags); set by Em_SetRequest, Em_RestartScript, func_00708640/func_00708750 (with trigger 0x80); EmScript_Init clears it */
    s8 repeatCount;             /* 0x7A3: the repeat count of EmScript_Op24; EmScript_Op25 clears it */
    u8 unk7A4[0x7A8 - 0x7A4];
    s32 unk7A8;                 /* 0x7A8: only cleared: by EmScript_Init and EmScript_PickTrigger */
    u8 *repeatPos;              /* 0x7AC: the command a repeat returns to (EmScript_Op24) */
    u8 *trigger02Pos;           /* 0x7B0: the script of trigger 2 (EmScript_PickTrigger; scriptFlags 2) */
    u8 *trigger04Pos;           /* 0x7B4: the script of trigger 4 (EmScript_PickTrigger; scriptFlags 4) */
    u8 *trigger08Pos;           /* 0x7B8: the script of trigger 8 (EmScript_PickTrigger; scriptFlags 8) */
    u8 *routeResume;            /* 0x7BC: the script position after the route command (EmScript_Op3F); EmScript_OpFF resumes there at the end of the route and clears it */
    u8 routePointScript;        /* 0x7C0: the current route point's script table entry (point +0x12; EmScript_RoutePointScript picks the commands to run there: EmScript_Op3F, EmScript_OpFF) */
    u8 routeNo;                 /* 0x7C1: route index (EmScript_Op3F; goalArg of the route goal, Em_ComputeGoal copies curGoalArg back); EmScript_OpFF walks it */
    u8 routePoint;              /* 0x7C2: point index in the route (EmScript_Op3F, EmScript_OpFF steps it down to 0; goalNo of the route goal) */
    u8 syncRequest;             /* 0x7C3: a request to send the monster state (QSync_SendEm sends it and clears it): 1 by Em_RequestSync, 2 by it or EmScript_OpFF unless 1 is pending; Em_RestartScript runs only when 0 on the host */
    u8 *scriptPos;              /* 0x7C4: the current script command (Em_StartScript) */
    s16 scriptOffset;           /* 0x7C8: EmScript_Run: the start of the script minus scriptPos when it stops */
    u8 mineBone;                /* 0x7CA: bone the carcass mine point follows, 0: the position (EmMine_Create) */
    u8 inSceneArea;             /* 0x7CB: 1 while the monster is in the scene's current area (GmEm_Update sets and clears it) */
    struct StageObj *k09Obj;    /* 0x7CC: its stage object of kind 9 (StageObj_K09_Create); Em_ResetToBasePose places it (StageObj_K09_Place), GmEm_ResetState clears it */
    u8 hookedBy;                /* 0x7D0: the player whose bait Plesioth took (targetNo; EmPlesioth_State2_11/2_12), compared with the player index in Player_StateFishReel */
    u8 hitPartMask;             /* 0x7D1: parts whose hit effects this hit spawned, one bit each (Em_UpdateDamage clears it, sets bits, passes it to Em_SpawnPartEffects) */
    u8 unk7D2[0x7D3 - 0x7D2];
    s8 quakeTimer;              /* 0x7D3: set to 5 while the giant kinds stomp (Lao-Shan State0_02, Shen Gaoren and Yama Tsukami Update), counted down in their Update; Player_IdleActions knocks a player over (state 2/7) near a giant with it set */
    s16 status5Threshold;       /* 0x7D4 */
    u8 turnStep;                /* 0x7D6: step of Em_TurnToTargetMotion (Em_SetTargetAngle clears it) */
    u8 status5Count;            /* 0x7D7 */
    s8 syncPulse[2];            /* 0x7D8: set to 2 by EmScript_Op11 ([0]) and Op12 ([1]), or by the sync message bits 4 and 8 (QSync_OnEm); GmEm_Update counts them down; QSync_SendEm sends them while nonzero. Player code reads Player.chargeLevel/fishTimer here */
    s16 combatTimer;            /* 0x7DA: frames of combat left without a seen player (func_00708990 counts it down; at 0 the monster calms down or leaves) */
    u8 combat;                  /* 0x7DC: 1 while the monster fights the hunters (Em_SetCombat); back to 0 it forgets them (seen bits, alertBuildup, unk86C, unk7E4 cleared) */
    u8 requested;               /* 0x7DD: 1 while a request (Em_SetRequest) is pending: the families' state choice leaves it alone */
    u8 requestId;               /* 0x7DE: the pending request: an id of the kind's gEmKindInfo entry (Em_PickRequest) */
    u8 blind;                   /* 0x7DF: nonzero: Em_SeePlayers sees nobody; counted down each frame (GmEm_Update), many states set 2 every frame */
    u8 seen;                    /* 0x7E0: players in sight this frame, one bit each (Em_SeePlayers); Em_UpdateNoticed makes them noticed */
    s8 mineSlot;                /* 0x7E1: mine point on the carcass (EmMine_Create), -1 when none */
    u8 hostNo;                  /* 0x7E2: the player whose console runs the monster (online; GmEm_Setup, func_00707060) */
    u8 noticed;                 /* 0x7E3: players the monster has noticed, one bit each (Em_AddNotice, Em_UpdateNoticed) */
    s16 noticeTimer[4];         /* 0x7E4: one per player: frames the player stays noticed (from Em_NoticeTime) */
    s32 need2Gauge;             /* 0x7EC: gauge of need 2 (request gEmKindInfo.need2Request): Em_DecayNeed2 lowers it (D_008BC690 rates, out of and in combat) and sets need2 at 0; Kut-Ku State0_0E refills it by 0xDE a frame up to need2Max; EmScript_Op21 compares it with 30% of the maximum; quest entry +0x10 */
    s32 need1Gauge;             /* 0x7F0: gauge of need 1 (named after gEmKindInfo.need1Request, the request it makes): Em_DecayNeed1 lowers it by the kind rate (D_008BC520) and sets need1 at 0; refilled to need1Max by eating-like states (Kut-Ku State0_1A, with +5% hp); func_00702830 looks for an action spot under the kind threshold; from the quest entry (+0xC) */
    s32 need3Gauge;             /* 0x7F4: gauge of need 3 (request gEmKindInfo.need3Request): Em_DecayNeed3 lowers it and sets need3 at 0; refilled to need3Max by Em_RefillNeed3, Em_BeginStatus1/2, Em_ResetNotice; quest entry +0x14 */
    s32 need2Max;               /* 0x7F8: maximum of need2Gauge (need2Gauge): Em_AddNeed2 caps at it */
    s32 need1Max;               /* 0x7FC: maximum of need1Gauge (need1Gauge): Em_AddNeed1 caps at it, refills set it; Em_CheckRageByHp takes the percentage of it (kind 0x34) */
    s32 need3Max;               /* 0x800: maximum of need3Gauge (need3Gauge): Em_Add7F4 caps at it, the refills set it */
    s16 rageThreshold;          /* 0x804: the kind's EmRageRow threshold (gEmRageTbl[type]), -1 without one (Em_InitRage); Em_CheckRageByHp raises rageBuildup to it; the families pass it to Em_CheckRageByHp */
    s16 rageBuildup;            /* 0x806: raised by Em_CheckRageByHp (damage times a factor picked by the hp percent, doubled for some kinds with part 0 broken); reaching rageThreshold sets ragePending; Em_InitRage, Em_EndRage clear it */
    s16 rageTimer;              /* 0x808: frames left of the rage, counted down by GmEm_Update (Hermitaur sets 0x708) */
    u8 rage;                    /* 0x80A: nonzero while enraged: the kind's rage motion speed (GmEm_Update), Blangonga tints red (EmBlangonga_UpdateTint); func_00709A40 ends it */
    u8 rageStep;                /* 0x80B: step of the rage timers (EmBlangonga_UpdateRageTimer, EmFatalis_UpdateRageTimer, EmDiablos_Update switch on it and step it); GmEm_SetMeshAlphas draws by it; Em_InitRage, GmEm_ResetState clear it */
    u8 ragePending;             /* 0x80C: 1 when rageBuildup reached rageThreshold (Em_CheckRageByHp); GmEm_Update then starts the rage (func_703600) outside group 4 and noReact; cleared by Em_StartRage2, Em_EndRage */
    u8 hooked;                  /* 0x80D: 1 while Plesioth has taken the bait of a player in state 0/0x53 (the line: Lure_PlayerHasMonsterBait): EmPlesioth_State2_11 sets it with hookedBy = targetNo; Player_StateFishReel finds the monster on its line by it; EmScript_Op47, State4_0F clear it */
    s8 enterDelay;              /* 0x80E: frames: GmEm_Update sets 2 while the monster is outside the scene's area; in it, func_00708640 and func_00708750 count it down and raise no trigger 0x80 until it is 0; GmEm_ResetState clears it */
    u8 guardTimer;              /* 0x80F: frames the hits cannot kill (hp kept at 1, also against poison), flinch or add status buildup (Em_UpdateDamage, Em_PickDamagedPart, Em_UpdateStatus), and Em_CheckFlash ignores flashes; counted down by GmEm_Update; states set 2 or 5, Em_BeginStatus2 30 */
    u8 flashed;                 /* 0x810: 1 when a flash object is in the sight cone (Em_CheckFlash, unk_6FF630.c); cleared when Em_ApplyFlash applies it */
    u8 noReact;                 /* 0x811: 1 while a status (Em_BeginStatus1/2/4/5 .. EndStatus) or a special state runs: GmEm_Update raises no triggers or rage actions, Em_CheckPartBuildup and func_00708640 do nothing; GmEm_ResetState clears it */
    u8 routeMissCount;          /* 0x812: frames the route point was not reached (EmScript_OpFF: at 10 it gives up the point); EmScript_Op3F clears it */
    u8 need3State;              /* 0x813: 0 until need3Gauge first runs out (Em_DecayNeed3 sets 1 with need3); EmScript_Op1E clears it */
    u8 need2State;              /* 0x814: 0 until need2Gauge first runs out (Em_DecayNeed2 sets 1 with need2); EmScript_Op0C sets, Op2B compares, Op0D/Op1E and Em_ClearNeed2State clear it */
    u8 need1State;              /* 0x815: 0 until need1Gauge first runs out (Em_DecayNeed1 sets 1 with need1); Em_ClearNeed1State, EmScript_Op1E clear it */
    u8 noAlert;                 /* 0x816: set by EmScript_Op26 (the script byte), 0 at reset: while 1, GmEm_Update raises no sight trigger (0x20) out of combat and the kind Update functions (Blangonga, Cephadrome, Kushala, Khezu) start no rage; func_00708640 does nothing unless 0 */
    u8 remote;                  /* 0x817: online: 1 when another player's console runs the monster (hostNo), 0 when this one does (EmSlot_Alloc, GmEm_Setup); most AI code runs only when 0 */
    f32 playerDist[4];          /* 0x818: per player, the distance from the monster (Em_SensePlayers, EmSenseView.dist); Velocidrome and Velociprey compare it with nearRadius */
    f32 playerDistXZ[4];        /* 0x828: per player, the same in the ground plane (EmSenseView.distXZ); Rathalos and Garuga read the target's */
    f32 reactRange[3];          /* 0x838: the reaction set's ranges (Em_LoadReactionSet; Em_GetReaction kinds 3 to 6, EmSenseView); the fourth at +0x844 shares bytes with the Player field aimPitch */
    u8 unk844[0x846 - 0x844];
    s16 aimPitch;               /* 0x846: as Player.aimPitch (same offset): Player_AddLean846 (player code typed Em) adds to it, clamped to +-0x2000 */
    s32 alertBuildup[4];        /* 0x848: one per player, out of combat: func_00703D60 adds the kind table values for the player action (Em_PlayerActionKind), range and sector while the player is near (Em_SensePlayers); Em_CheckPartBuildup raises trigger 0x10 and alertedPlayers bit at the kind limit D_008BB660 and clears it; Em_SetCombat clears all */
    u16 playerAngle[4];         /* 0x858: u16[4], one per player: direction from the monster to the player (Em_SensePlayers: Math_AngleXZ, as v->dir); Em_AngleSector finds the sense sector it falls in */
    u16 sectorHalfAngle[4];     /* 0x860: u16[4]: half widths of the 4 sense sectors around the heading, from the kind's sense row D_008BB150[type] (func_00704060, with the ranges at 0x838..0x844); Em_AngleSector */
    u8 sensedPlayers;           /* 0x868: players within the sense reach, one bit each (Em_SensePlayers sets and clears them; Em_DecayNotice, Em_CheckPartBuildup, EmScript_Op01 test them) */
    u8 alertedPlayers;          /* 0x869: one bit per player whose alertBuildup (alertBuildup) reached the kind limit (Em_CheckPartBuildup, with trigger 0x10); EmScript_Op11 tests the bits */
    u8 alertLimitRow;           /* 0x86A: the row of gEmPartBuildupLimits[type] the alertBuildup limit comes from (func_00703D60, Em_CheckPartBuildup) */
    u8 triggers;                /* 0x86B: script triggers pending, one bit each (0x02..0x80): EmScript_PickTrigger starts the script of the first */
    s32 noticeCount[4];         /* 0x86C: one per player: notice count (Em_AddNotice; at 9000 the player is noticed) */
    s8 loopPoint;               /* 0x87C: the area point the script loop goes to (EmScript_Op10: goalKind 2, goalArg 1, goalNo = it), -1: none; picked at random, from a table or in turn (EmScript_Op10, EmScript_OpFF); EmScript_Init, EmScript_Restart, Em_TypeSetup set -1 */
    s8 loopCount;               /* 0x87D: passes of the script loop (EmScript_Op0F: at most the area entry's point count); EmScript_OpFF loops while the pass count (+0x87E) is below it; -1: none */
    s8 loopIndex;               /* 0x87E: the script loop's pass count (EmScript_Op0F clears it, OpFF steps it and compares it with loopCount) */
    s8 areaPointCount;          /* 0x87F: the number of points of the area entry (EmScript_Op0F e[1]); EmScript_Op10 and OpFF bound loopPoint by it */
    s8 areaSeqLen;              /* 0x880: length of the area sequence of EmScript_Op17, -1: none; EmScript_Op18 and OpFF end the sequence when areaSeqPos reaches it */
    s8 areaSeqPos;              /* 0x881: position in the area sequence (EmScript_Op18 reads the next area from table 14 at it and makes it destArea), -1: none */
    u16 prevArea;               /* 0x882: the area the monster came from, 0xFFFF: none: Em_EnterNextArea sets it to area before moving to nextArea; Em_PickRoutePoint skips routes back to it; EmScript_Op3D/Op3E test it; sent online (QSync_SendEm) */
    f32 motionSpeed;            /* 0x884 */
    f32 goal[3];                /* 0x888: the point the monster moves or turns to, computed from goalKind (Em_ComputeGoal) */
    void **tables;              /* 0x894: the per-kind area tables (D_8BB280[type]): [0] points, [6] route sets; func_00707D70 finds the entry of an area, func_00707E40 its points */
    u8 unk898[0x899 - 0x898];
    s8 demonMode;               /* 0x899: as Player.demonMode (same offset): only player code typed Em uses it (DualSwords_ComboInput) */
    u8 unk89A[0x89C - 0x89A];
    u8 fxFlags;                 /* 0x89C: bit 1: set once a state's motion effect ran (Player_PassedFrameScaled ... |= 1) and by Em_ResetToBasePose; GmEm_Fx3B (mode 7) and StageObj_K09_Run test it; the ChangeState functions keep only bit 1; Hermitaur uses bits 2, 4, Shen Gaoren 8 */
    u8 itemUseKind;             /* 0x89D: 0x80 | Side_ItemUseKind(side) (GmEm_Update), 0x88 in versus quests (gm_sub func_00904CF0); EmScript_Op73 and EmScript_Op74 read the kind and keep 0x80 */
    s16 areaTimer1;             /* 0x89E: a countdown set from the area values (Em_SetAreaValue89E), counted down by GmEm_Update; EmScript_Op29 tests it > 0 (twin of areaTimer2) */
    s16 areaTimer2;             /* 0x8A0: a countdown set from the area values (Em_SetAreaValue8A0; Hermitaur 0x708 in Setup and Update), counted down by GmEm_Update; EmScript_Op2A tests it > 0 */
    s16 blindTimer;             /* 0x8A2: frames of flash blindness: Em_StartBlind sets D_008BEAC0[type] (and makes the stars, StageObj_Stun_CreateEm), Em_ApplyFlash copies it into timer; StageObj_Stun_Run ends at <= 0; Em_ClearPoison and GmEm_ResetState clear it; Kushala wind stops while > 0 */
    u8 actionGoal[3];           /* 0x8A4: the action spot func_00702830 found: kind, goalArg, goalNo, 0xFF: none (Em_StartGoalAction, EmScript_Op2D copy [1], [2] to the goal) */
    u8 partDamageMode;          /* 0x8A7: from the per-kind part table after the 9 hp values (Em_InitParts) */
    u16 syncRand;               /* 0x8A8: a random value drawn at each synced state change (Em_SyncState: Rand_Next(0)) and sent with it online (QSync_SendEm, QSync_OnEm, QSync_QueueEmState) */
    u8 unk8AA[0x8AB - 0x8AA];
    u8 part8Down;               /* 0x8AB: Em_UpdateDamage (reaction 9): part 8's hp ran out and the kind's handler took it: reaction 0xB; GmEm_ResetState clears it */
    s8 hit8Count;               /* 0x8AC: times the group-4 state 4 reaction ran (the ChangeState functions add 1 entering it, with unk89C |= 1); the AllowHit8 functions allow it while 0 (Hermitaur < 2; Kushala at 30% and 40% hp, Init sets 1 or 2); read as signed; GmEm_SetMeshAlphas hides parts while <= 0 */
    u8 unk8AD;                  /* 0x8AD */
    u8 speedBonus;              /* 0x8AE: percent added to the speed: GmEm_Update scales the motion speed by (speedBonus + 100) / 100, Em_TurnStep the turn step; walking states (group 1) set 10..200; Em_SetState and GmEm_ResetState clear it */
    u8 spawnParam;              /* 0x8AF: byte +6 of the quest monster entry (QuestEm_Spawn): Velociprey/Anteka/Melynx Init switch on it, Shakalaka picks 2 or Rand_Next(1) & 1 and reads D_008CA310[it] (a distance), Kind38 bit 1, EmScript_Op75 compares it */
    u8 hideShadowHold;          /* 0x8B0: frames GmEm_Update leaves hideShadow as the state set it (states set 2 with hideShadow = 2); counted down there */
    u8 wallHitSpecial;          /* 0x8B1: 1 when a wall touched this frame has flag b[1] in its material row D_004407F0 (func_003294A0, which fills wallHitDirs too); Diablos func_007AC3D0 tests it with wallHitDirs 0xC0000003 at frame 60 (Player has its input lock here) */
    u16 collMask;               /* 0x8B2: per-kind collision mask gEmCollMasks[type] (Em_InitCollMask): func_00330EB0 (wall collision) and func_32E9D0 (path checks of func_00702830, EmScript_Op3F) use it; Apceros State1_05 adds 0x80 */
    EmSpawn spawns[2];          /* 0x8B4 */
    f32 objectPos[3];           /* 0x94C: f32[3]: position of the object (bait, meat...) Em_SenseObjects found within its radius; the goal of goalKind 8 (Em_ComputeGoal, Em_ComputeGoal2) */
    struct Em *link;            /* 0x958: the monster of the linked kind (Em_FindLink) */
    u8 spawnQueued;             /* 0x95C: QSync_QueueEmState sets it when the local machine queues the monster's sync state into a spawn */
    u8 hostTimer;               /* 0x95D: 60 when this machine takes the monster over as host, counted down each frame (Em_MigrateHost) */
    u8 scriptAbort;             /* 0x95E: nonzero: EmScript_Run stops and restarts the script (Em_RestartScript); EmScript_SetVar95E sets it (2 after 1000 commands in a frame) */
    u8 routeResult;             /* 0x95F: EmScript_Op3F: 0 found, 2 no route point near, 7 a route without points */
    u8 unk960[0x964 - 0x960];
    u8 questKey;                /* 0x964: byte +0x18 of the quest monster entry (QuestEm_Spawn): key of the id sets (Em_IdSetLookup, Em_IdSetHas match it or -1); EmScript_Op57 compares it; Rathalos Update tests 1 */
    s8 senseDelay;              /* 0x965: while nonzero (counted down each frame), GmEm_Update skips sensing the players (Em_SensePlayers, Em_SeePlayers); states set 5 */
    u8 large;                   /* 0x966: per-kind flag gEmIsLarge[type] (Em_InitKindParams), nonzero for the large monsters: synced online (GmEm_Setup, GmEm_Update, Em_SyncState, Em_IsSyncFree), death handled by the area owner (Em_UpdateDamage, Em_UpdateStatus), target point Player.targetPos (Em_GetTargetPos); EmScript_Op23 kind 0x4A matches any of them */
    u8 requestPrio;             /* 0x967: priority of the pending request (0x80 low HP ... 0x40 idle); a new one needs at least this */
    u8 lowHpNeed;               /* 0x968: 1 when hp fell under the kind's ratio (Em_CheckLowHp): Em_PickRequest makes it a request of priority 0x80 */
    u8 need4;                   /* 0x969: 1: Em_PickRequest makes request gEmKindInfo.need4Request at priority 0x70 (then clears it); set by func_007065C0 and EmScript_Op30, cleared by EmScript_Op1E */
    u8 need1;                   /* 0x96A: 1 when need1Gauge ran out (Em_DecayNeed1): Em_PickRequest makes request gEmKindInfo.need1Request at priority 0x60; Em_ClearNeed1State, EmScript_Op1E clear it */
    u8 need2;                   /* 0x96B: 1 when need2Gauge ran out (Em_DecayNeed2): Em_PickRequest makes request gEmKindInfo.need2Request at priority 0x50; Em_ClearNeed2State, EmScript_Op1E clear it */
    u8 need3;                   /* 0x96C: 1 when need3Gauge ran out (Em_DecayNeed3, kinds with gEmKindInfo.need3Request): Em_PickRequest makes request need3Request at priority 0x40 out of combat; the status begin/end functions, Em_RefillNeed3, EmScript_Op1E clear it */
    u8 showOnMap;               /* 0x96D: per kind (D_008BF2D0[type], Em_InitKindParams): nonzero: Map_DrawArea draws the monster, Hud_DrawPlayerStatus marks its target player */
    u8 holdTimer;               /* 0x96E: nonzero: the monster is held (Em_IsHeld); Em_SetCaptured(em, 0) and the trapped group-4 states set 5 every frame, Kut-Ku SetGroup4State 50; counted down by GmEm_Update; the trap Eff12_Run catches a monster with it > 0 within 700 and lets go at 0; the families pick other reactions while it is set */
    u8 questIndex;              /* 0x96F: index of the quest monster entry it came from (+0x2C, QuestEm_Spawn); Quest_OnRemoteKill matches it with the entry index */
    u8 startsBattle;            /* 0x970: per kind (D_008BF970[type], Em_InitKindParams): nonzero: Player_UpdateInBattle counts the visible monster in the player area as a battle */
    u8 hostChanged;             /* 0x971: set when func_00707060 hands the monster to another player (hostNo, remote, QSync_SendEm kind 4); GmEm_ResetState clears it */
    u8 reactionSet;             /* 0x972: the reaction set Em_LoadReactionSet loaded */
    u8 ceilingTimer;            /* 0x973: nonzero while on the ceiling: the ceiling states (Khezu State2_18..2_22, Hermitaur 0x43 State2_04..2_08) set 5 every frame; counted down by GmEm_Update; a flinch then makes Khezu fall (state 4/1); EmScript_Op59 branches on it */
    u8 routeHeight;             /* 0x974: EmScript_Op3F byte 2: a height in tenths (h = 10.0f * it) */
    u8 moveFrozen;              /* 0x975: 1 when func_00708850 undid the move (in combat, no hunter in its area, the state not the given one: back to prevPosX/prevPosZ); GmEm_Update then keeps prevPos and runs the motion with flag 0x20000; GmEm_ResetState, func_00708850, Khezu and Plesioth Update clear it */
    u8 unk976[0x977 - 0x976];
    u8 hasCeiling;              /* 0x977: 1 when there is a ceiling over the monster (AreaGrid2_GetCeiling, height in surfaceY; 1 with 1500 in the special areas): Khezu and Hermitaur kind 0x43 (EmKhezu_Update, EmHermitaur_Update); ChangeState and ScriptQuery test it */
    u8 unk978[0x984 - 0x978];
    u16 sizePct;                /* 0x984: size in percent (quest entry +0x30, QuestEm_Spawn; 100 by default; versus team): quest size records (QuestEm_Hunted, QuestEm_Captured: gQuest.sizeMax/sizeMin), QuestEm_Appear grows it by 5, the mine radius and collision shapes scale with it */
    s16 destArea;               /* 0x986: the area the monster travels to, 0xFFFF: none: set by EmScript_Op18/Op1A (Area_IdForCurrentTime), goalArg of Em_ComputeGoal; the State2_08/2_0D travel states take it as nextArea, State2_09 clears it on arrival; Em_PickRoutePoint defaults it to nextArea */
    u8 unk988[0x989 - 0x988];
    u8 hitzoneSet;              /* 0x989: which of the kind's damage tables ObjAttach_DamageEm uses (D_90CBD0[type][hitzoneSet]): 0 normal, 1 while Chameleos is invisible, the Kushala wind blows, after a part break with flag 1 (Em_SpawnPartEffects), Hermitaur 2 in State3_04; ChangeState and GmEm_ResetState clear it */
    s8 level;                   /* 0x98A: strength level from the quest monster entry (+0x36, QuestEm_Spawn; versus: Versus_AssignEmTeam): row of the size, hp and rage scale tables (Em_ComputeSize, Em_RageScale1/2), part hp scaling for 11..17 (Em_ScalePartHp); QuestEm_Appear raises it up to 15; EmScript_Op71 compares it */
    u8 hasGoal;                 /* 0x98B: 1: the state walks to goal (Em_StepToGoal, ends near it, goalDist from Vec_DistanceXZ), 0: no goal, the state runs on timer instead; set by the SetGroup1State/SetGroup3State functions from curGoalKind or targetNo */
    s16 trapTimer;              /* 0x98C: frames the trapped states (State4_0D, 4_11, 4_17 ...) last, counted down there: Em_SetGoal12Dist sets 0x340, 0x208 or 0x138 when enraged (0x9C kind 0x35); goalKind 12 carries it as goalNo (/ 4, Em_SetGoal12, Em_ComputeGoal2) */
    u16 aimAngle;               /* 0x98E: angle the monster turns to, 0x10000 per turn (Math_AngleXZ to the goal; many states make it relative to rotY) */
    f32 goalDist;               /* 0x990: distance left to the goal (Vec_DistanceXZ, then counted down by velZ) */
    s16 scriptDelay;            /* 0x994: frames before the pending script runs: Em_UpdatePendingScript counts it down, then runs the script if scriptPending (scriptPending); set by EmScript_Op48, cleared by EmScript_Restart */
    u8 targetFound;             /* 0x996: Em_TargetPlayerInRange: 1 when a player was in range (curGoalKind 1), 0 otherwise */
    u8 curGoalKind;             /* 0x997: the goalKind the goal was last computed from (Em_ComputeGoal) */
    u16 curGoalArg;             /* 0x998: the goalArg the goal was last computed from */
    u16 curGoalNo;              /* 0x99A: the goalNo the goal was last computed from */
    s32 flightTurn;             /* 0x99C: turn left to make in flight, 0x10000 per turn: Em_UpdateFlight takes it from aimAngle and turns rotY toward it by a step each frame until 0 */
    u8 versusGauge;             /* 0x9A0: versus gauge 0..90, 0xFF: none: raised by Versus_AddGauge, lowered by 30 by Versus_MonsterDown; Em_StartPendingRage makes a rage of gauge * 30 + 450 frames from it and clears it; synced (QSync_PackEmState) */
    u8 rageStarted;             /* 0x9A1: 1 from Em_StartPendingRage to Em_EndRage; GmEm_Update then sets itemUseKind to 0x88 */
    u8 stateArmor;              /* 0x9A2: 1 while the state is in a move the monster does not break off: set by the family states, cleared each frame by Em_ClearHitReactMode (with hitReactMode); ObjAttach_HitAll ignores attack kind 0x13 then, Blangonga ignores the flash (EmBlangonga_IgnoresFlash), Hermitaur takes no group-4 reaction (EmHermitaur_AllowHit8); the ScriptQuery functions report it */
    u8 holdTimer2;              /* 0x9A3: the second hold (Em_IsHeld): Em_SetCaptured(em, 1) and Em_SetHold2 set 5; counted down by GmEm_Update; the trap Eff10_RunTrap catches a monster with it > 0 within 500 and lets go at 0; cleared by the SetGroup0State functions */
    u16 partsBroken;            /* 0x9A4: broken parts, one bit each (Em_BreakPart; Em_PartBroken tests them) */
    u16 partsToBreak;           /* 0x9A6: parts to break, one bit each (Em_FlagPartBreak; Em_ApplyPartBreaks breaks the new ones) */
    s8 mineSlot2;               /* 0x9A8: second mine point (EmMine_CreateTimed), -1 when none */
    s8 mineCount2;              /* 0x9A9: gathers for the second mine point */
    s16 mineTimer2;             /* 0x9AA: frames left for the second mine point */
    u8 hermitTimer;             /* 0x9AC: frames (read as s8): EmHermitaur_State3_04 sets 10, GmEm_Update counts it down; while nonzero EmHermitaur_Update goes to state 4/06 (not in the air, no stateArmor) and kind 0x30 plays SE 0x2D; GmEm_ResetState clears it */
    s8 windTimer;               /* 0x9AD: 2 while the Kushala wind blows (EmKushala_Update with EmKushala_WindActive), counted down by GmEm_Update; Player_NearEm40 looks for a monster near the player with it set */
    u8 unk9AE[0x9D0 - 0x9AE];
} Em;
typedef char Em_size_check[(sizeof(Em) == 0x9D0) ? 1 : -1];

extern Em gEms[20];
extern EmKindInfo gEmKindInfo[];
extern EmTraderMsg gEmTraderMsgs[106];

s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
s32 Em_SetCombat(Em *em, u8 a, s16 b);

#endif /* EM_H */
