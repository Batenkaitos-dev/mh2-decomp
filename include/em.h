#ifndef EM_H
#define EM_H

/*
 * Monsters in the field (game overlay). sub_main keeps 20 of them in an
 * array, gEms (0x5EC580), 2512 bytes each; the players follow at
 * gPlayers (0x5F89D0). `type` selects the kind, and per-kind tables in the game
 * overlay's data hold its parameters.
 *
 * Status effects (Em_BeginStatus1/2/4/5, Em_EndStatus*): +0x71E holds the current one (1,
 * 2, 4 or 5; 0 for none) and +0x71F has one flag bit per status. Each
 * status has a threshold, initialised from a per-kind EmStatusParam table
 * and raised by `increment` each time the status ends, at most 4 times (the
 * counts at +0x73B, +0x73D, +0x73E and +0x7D7).
 */
#include "common.h"
#include "scene.h"
#include "model.h"
#include "motion.h"
#include "stageobj.h"

/*
 * Monster kinds (Em.type), the game's own monster numbers. The quest data
 * names them: each quest lists its targets as {kind, count} records next
 * to the target's name. EM_KIND_LAO_SHAN_LUNG, EM_KIND_SILVER_RATHALOS, EM_KIND_GOLD_RATHIAN, EM_KIND_ASHEN_LAO_SHAN_LUNG, EM_KIND_FATALIS_C and EM_KIND_YAMA_TSUKAMI_B have no quest;
 * their family (the per-kind setup dispatch Em_TypeSetup, gm_em.c's
 * dispatch) places them. EM_KIND_FELYNE_CART is the Felyne cart (Em.cartPlayer).
 *
 * The "Capcom:" notes are Capcom's names for the kinds, from Monster Hunter
 * Frontier's engine (mhfo.dll), which was forked from this code: a table of
 * 177 name strings there is indexed by the same kind numbers (0 is EM_NOT,
 * no monster; 74 on are MHF's own monsters). The names are notes only.
 */
enum EmKind {
    EM_KIND_RATHIAN = 0x01,             /* Capcom: EM_FEMALE_WYVERN */
    EM_KIND_FATALIS = 0x02,             /* Capcom: EM_F_DRAGON */
    EM_KIND_KELBI = 0x03,               /* the Kelbi: no quest names this kind, but Capcom's name, EM_DEER, confirms it. The code shows a kind that runs the Anteka family (EmAnteka_*) and is the only kind with Em.gender, male and female, with a smaller body */
    EM_KIND_MOSSWINE = 0x04,            /* Capcom: EM_PIG */
    EM_KIND_BULLFANGO = 0x05,           /* Capcom: EM_BOAR */
    EM_KIND_YIAN_KUT_KU = 0x06,         /* Capcom: EM_MIMI_WYVERN */
    EM_KIND_LAO_SHAN_LUNG = 0x07,       /* Capcom: EM_RED_DRAGON */
    EM_KIND_CEPHADROME = 0x08,          /* Capcom: EM_DESERT_WYVERN */
    EM_KIND_FELYNE = 0x09,              /* the Felyne: the per-kind hooks run it in the Melynx family (Capcom's Melynx is EM_B_CATGOBLIN). Capcom: EM_CATGOBLIN */
    EM_KIND_TRADER = 0x0A,              /* the trader met in the field (EmTrader_Trades* tables; the quest code). Capcom: EM_MT_HERB, probably "mountain herbs" (sansai): the Veggie Elder, who is the trader */
    EM_KIND_RATHALOS = 0x0B,            /* Capcom: EM_MALE_WYVERN */
    EM_KIND_HERBIVORE_0 = 0x0C,         /* a grazing herbivore of the Apceros family (em_apceros.c); no code shows the species. Capcom: EM_HERBIVOROUS0 */
    EM_KIND_GENPREY = 0x0D,             /* Capcom: EM_MAHI_RAPTER */
    EM_KIND_DIABLOS = 0x0E,             /* Capcom: EM_TSUNO_WYVERN */
    EM_KIND_KHEZU = 0x0F,               /* Capcom: EM_MEKURA_WYVERN */
    EM_KIND_VELOCIPREY = 0x10,          /* Capcom: EM_RAPTER */
    EM_KIND_GRAVIOS = 0x11,             /* Capcom: EM_ARMOR_WYVERN */
    EM_KIND_FELYNE_CART = 0x12,         /* Capcom: EM_NIGURUMA */
    EM_KIND_VESPOID = 0x13,             /* Capcom: EM_INSECT */
    EM_KIND_GYPCEROS = 0x14,            /* Capcom: EM_TOSAKA_WYVERN */
    EM_KIND_PLESIOTH = 0x15,            /* Capcom: EM_FISH_WYVERN */
    EM_KIND_BASARIOS = 0x16,            /* Capcom: EM_GITAI_WYVERN */
    EM_KIND_MELYNX = 0x17,              /* Capcom: EM_B_CATGOBLIN */
    EM_KIND_HORNETAUR = 0x18,           /* Capcom: EM_B_INSECT */
    EM_KIND_APCEROS = 0x19,             /* Capcom: EM_ANKYLO */
    EM_KIND_MONOBLOS = 0x1A,            /* Capcom: EM_TSUNO2_WYVERN */
    EM_KIND_VELOCIDROME = 0x1B,         /* Capcom: EM_BOSS_RAPTER */
    EM_KIND_GENDROME = 0x1C,            /* Capcom: EM_BOSS_MAHI_RAPTER */
    EM_KIND_ROCK = 0x1D,                /* EmKind1D_*: never moves or attacks; a hit on it gives a guard spark (ObjAttach_HitEffects); five meshes by variant; dies with its variant's sound and effects. Capcom: EM_ROCK */
    EM_KIND_IOPREY = 0x1E,              /* Capcom: EM_POISON_RAPTER */
    EM_KIND_IODROME = 0x1F,             /* Capcom: EM_BOSS_POISON_RAPTER */
    EM_KIND_PIGLET = 0x20,              /* a little pig of the Mosswine family (em_mosswine.c); not the Poogie, a town NPC (NPC_POOGIE). Capcom: EM_LITTLE_PIG */
    EM_KIND_KIRIN = 0x21,               /* Capcom: EM_KYLIN */
    EM_KIND_CEPHALOS = 0x22,            /* Capcom: EM_S_DESERT_WYVERN */
    EM_KIND_GIAPREY = 0x23,             /* Capcom: EM_WHITE_RAPTER */
    EM_KIND_FATALIS_B = 0x24,           /* a second Fatalis kind (its quests name it as Fatalis). Capcom: EM_F_DRAGON_RED */
    EM_KIND_PINK_RATHIAN = 0x25,        /* Capcom: EM_TAMA_B_INSECT, which does not fit: the code runs this kind in the Rathian family */
    EM_KIND_BLUE_YIAN_KUT_KU = 0x26,    /* Capcom: EM_BLUE_MIMI_WYVERN */
    EM_KIND_PURPLE_GYPCEROS = 0x27,     /* Capcom: EM_TOSAKA2_WYVERN */
    EM_KIND_YIAN_GARUGA = 0x28,         /* Capcom: EM_DOS_MIMI_WYVERN */
    EM_KIND_SILVER_RATHALOS = 0x29,     /* Capcom: EM_SILVER_MALE_WYVERN */
    EM_KIND_GOLD_RATHIAN = 0x2A,        /* Capcom: EM_GOLD_FEMALE_WYVERN */
    EM_KIND_BLACK_DIABLOS = 0x2B,       /* Capcom: EM_MOS_TSUNO_WYVERN */
    EM_KIND_WHITE_MONOBLOS = 0x2C,      /* Capcom: EM_WHITE_TSUNO2_WYVERN */
    EM_KIND_RED_KHEZU = 0x2D,           /* Capcom: EM_MEKURA2_WYVERN */
    EM_KIND_GREEN_PLESIOTH = 0x2E,      /* Capcom: EM_FISH2_WYVERN */
    EM_KIND_BLACK_GRAVIOS = 0x2F,       /* Capcom: EM_BLACK_ARMOR_WYVERN */
    EM_KIND_DAIMYO_HERMITAUR = 0x30,    /* Capcom: EM_YADOKARI */
    EM_KIND_AZURE_RATHALOS = 0x31,      /* Capcom: EM_BLUE_MALE_WYVERN */
    EM_KIND_ASHEN_LAO_SHAN_LUNG = 0x32, /* Capcom: EM_RED_DRAGON2 */
    EM_KIND_BLANGONGA = 0x33,           /* Capcom: EM_GORILION */
    EM_KIND_CONGALALA = 0x34,           /* Capcom: EM_PINK_GORILION */
    EM_KIND_RAJANG = 0x35,              /* Capcom: EM_GOLD_GORILION */
    EM_KIND_KUSHALA_DAORA = 0x36,       /* Capcom: EM_METAL_DRAGON */
    EM_KIND_SHEN_GAOREN = 0x37,         /* Capcom: EM_RAO_YADOKARI */
    EM_KIND_THUNDERBUG = 0x38,          /* EmKind38_*: the hovering monster Yama Tsukami calls up; Capcom's word is the Thunderbug's Japanese name (raikouchuu). Capcom: EM_RAIKOUCHUU */
    EM_KIND_SHAKALAKA = 0x39,           /* Capcom: EM_HIGERU */
    EM_KIND_YAMA_TSUKAMI = 0x3A,        /* Capcom: EM_GAS_DRAGON */
    EM_KIND_CHAMELEOS = 0x3B,           /* Capcom: EM_STEALTH_DRAGON */
    EM_KIND_RUSTED_KUSHALA_DAORA = 0x3C,/* Capcom: EM_SABI_METAL_DRAGON */
    EM_KIND_BLANGO = 0x3D,              /* Capcom: EM_ZAKO_GORILION */
    EM_KIND_CONGA = 0x3E,               /* Capcom: EM_ZAKO_PINK_GORILION */
    EM_KIND_REMOBRA = 0x3F,             /* Capcom: EM_ZAKO_WYVERN */
    EM_KIND_LUNASTRA = 0x40,            /* Capcom: EM_OFF_FIRE_DRAGON */
    EM_KIND_TEOSTRA = 0x41,             /* Capcom: EM_ON_FIRE_DRAGON */
    EM_KIND_HERMITAUR = 0x42,           /* Capcom: EM_ZAKO_YADOKARI */
    EM_KIND_SHOGUN_CEANATAUR = 0x43,    /* Capcom: EM_BLADE_YADOKARI */
    EM_KIND_BULLDROME = 0x44,           /* Capcom: EM_DOS_FANGO */
    EM_KIND_ANTEKA = 0x45,              /* Capcom: EM_TONAKAI */
    EM_KIND_POPO = 0x46,                /* Capcom: EM_MANMOS */
    EM_KIND_FATALIS_C = 0x47,           /* a third Fatalis kind (gm_em.c's dispatch); no quest names it. Capcom: EM_F_DRAGON_WHITE */
    EM_KIND_YAMA_TSUKAMI_B = 0x48,      /* the second kind of Yama Tsukami's family. Capcom: EM_GAS_DRAGON_DUMMY */
    EM_KIND_CEANATAUR = 0x49            /* Capcom: EM_ZAKO_BLADE_YADOKARI */
    /* Kinds 9, 12, 29, 32 and 56 are named from Capcom's names (above),
     * with the code's evidence: the per-kind table of family hooks runs
     * 9, 12 and 32 with the Melynx, Apceros and Mosswine families, and 29
     * and 56 have their own (EmKind1D_*, EmKind38_*). No quest names them. */
};

/*
 * State groups (Em.stateGroup). Each family's RunState switches on the
 * group, then a per-group dispatcher on Em.state; the families' state
 * functions settle what each group holds (seen in Rathalos's and the
 * common code). The "Capcom:" notes are likely Capcom's names: mhfo.dll
 * (Monster Hunter Frontier, forked from this code; its tables at file
 * offsets 0x1A33CCC and 0x1A5ABE4) lists EM_ACT .. EM_VSDEMO in this order, then EM_EXCEPT and EM_ACT_ST_NUM. The pairing is by order
 * only, though each name's meaning fits the group; Capcom's group 9,
 * EM_EXCEPT, has no counterpart here.
 */
enum EmStateGroup {
    EM_GROUP_IDLE = 0,      /* standing, idle actions, eating, resting, lying under a status. Capcom (likely): EM_ACT */
    EM_GROUP_MOVE = 1,      /* walking, running, limping and turning on the ground. Capcom (likely): EM_MOVE */
    EM_GROUP_FLY = 2,       /* take-off, flight, hovering and landing (Diablos: underground). Capcom (likely): EM_FLY */
    EM_GROUP_ATTACK = 3,    /* attacks and threats aimed at the target. Capcom (likely): EM_ATK */
    EM_GROUP_REACT = 4,     /* reactions to hits, traps and statuses (Em_UpdateDamage's results). Capcom (likely): EM_DMG */
    EM_GROUP_DEFEAT = 5,    /* hunted (QuestEm_Hunted), captured, and appearing again. Capcom (likely): EM_DIE */
    EM_GROUP_EVENT = 6,     /* scripted scenes: the quest's opening flight, captures. Capcom (likely): EM_DEMO */
    EM_GROUP_EVENT2 = 7,    /* most families run group 6's dispatcher; Remobra has its own (its reappearance). Capcom (likely): EM_REVIVAL */
    EM_GROUP_VERSUS = 8     /* the end of a versus match (Rathalos: Versus_DefeatCamera); GmEm_Update keeps the monster unhittable. Capcom (likely): EM_VSDEMO */
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
 * Em_UpdateDamage's result: what the last hits and statuses
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
    EM_DMG_ATTACK13 = 0x12          /* a hit of ATTACK_HOLD while not held, with the kind's timer (Em_HasKindTimer) */
};

/*
 * Em.status: the status the monster is under (Em_BeginStatus1/2/4/5 set
 * it, the Em_EndStatus functions clear it). Only the stun
 * is certain from the code; the others stay numbers. Seen: status 1 begun
 * in a hold near low hp captures the monster (EM_DMG_CAPTURED); statuses
 * 1 and 2 lay it down (the families' STATUS1_COLLAPSE and STATUS2_COLLAPSE
 * states), and lying under status 2 it regains hp
 * (EmRathalos_Status2LieDown); status 4 it shakes off standing (the
 * families' STATUS4_SHAKE_OFF states).
 */
enum EmStatus {
    EM_STATUS_NONE = 0,
    EM_STATUS_STUN = 5      /* dizzy: the families' stun states make StageObj_Stun (EM_DMG_STATUS5) */
};

/*
 * Em.statusFlags: one bit per status whose buildup reached its threshold.
 * 0x01, 0x02 and 0x08 are statuses 1, 2 and 4, which have no names yet
 * (see enum EmStatus). Poison is a bit of its own, with no Em.status value:
 * QSync_SendEm and QSync_OnEm carry it between machines.
 */
enum EmStatusFlag {
    EM_STATUSF_POISONED = 0x04,     /* poisonBuildup reached poisonThreshold (Em_AddPoison) */
    EM_STATUSF_STUN = 0x10          /* status5Buildup reached status5Threshold (Em_AddStatus5) */
};

/* Em.paintTimer's start: the paint mark lasts 10 minutes (ObjAttach_DamageEm, QSync_OnEm). */
#define EM_PAINT_FRAMES 18000

/* Em.hitReactMode: how Em_UpdateDamage treats a hit (Em_SetHitReactMode; Em_ClearHitReactMode resets it each frame). */
enum EmHitReactMode {
    EM_HIT_REACT_FLINCH = 0,            /* normal: a hit can flinch the monster */
    EM_HIT_REACT_WAKE = 2,              /* lying (resting, statuses 1 and 2, captured): a hit gives EM_DMG_HIT_LYING */
    EM_HIT_REACT_NO_FLINCH = 3,         /* a hit gives EM_DMG_HIT_MODE3, which most families ignore (eating, trapped) */
    EM_HIT_REACT_STATUS = 4             /* under status 4 or 5: kept through the hit (Em_BeginStatus4/5) */
};

/*
 * The monster script's triggers: Em.triggers bits, one per event that can
 * start a script (EmScript_PickTrigger starts the first pending one). The
 * same bits are the script threads of Em.scriptFlags (Em_ScriptFlagsFrom
 * tests them before raising a trigger). Named from the code that raises
 * each: the script ops and GmEm_Update.
 */
enum EmTrigger {
    EM_TRIGGER_AREA_OBJ = 0x02,         /* an area object near the monster (GmEm_Update: Em_FindNearAreaObj) */
    EM_TRIGGER_SENSED = 0x04,           /* an object sensed (the families' Em_SenseObjects) */
    EM_TRIGGER_RAGE = 0x08,             /* rage begins (Em_StartRage2) */
    EM_TRIGGER_PART_BUILDUP = 0x10,     /* Em_CheckPartBuildup */
    EM_TRIGGER_NOTICED = 0x20,          /* a player noticed (GmEm_Update: Em_UpdateNoticed) */
    EM_TRIGGER_BLOCK_AREA = 0x40,       /* in a block area (GmEm_Update: Em_InBlockArea2) */
    EM_TRIGGER_PLAYER_AREA = 0x80       /* on the ground in the player's area (Em_RaisePlayerAreaTrigger, Em_RaisePlayerAreaTriggerLax) */
};

/*
 * Em.flags bit (the same bit as Player.flags'; Player_SetFlag and
 * Player_ClearFlag take a monster too): the motion's root movement is not
 * applied to the position (Player_MoveLocal skips it). GmEm_Update sets it
 * while Em.moveFrozen; the families set and clear it around turns and
 * landings.
 */
#define EM_FLAG_NO_ROOT_MOTION 0x20000

/* A layer's motion id (Em.motionId[layer]) for a motion of the kind's table. */
#define EM_MOTION_ID(layer, motion) (1000 + 200 * (layer) + (motion))

typedef struct EmStatusParam {
    s16 threshold;      /* 0x0: initial threshold; -1 when the kind is immune */
    s16 decayInterval;  /* 0x2: frames between decays of the buildup */
    s16 decayAmount;    /* 0x4: taken off the buildup at each decay */
    s16 duration;       /* 0x6: in frames */
    s16 increment;      /* 0x8: added to the threshold each time it ends */
} EmStatusParam;

/* A kind's poison parameters (gEmStatusParams): Em_SetupStatus, Em_AddPoison
 * and Em_UpdateStatus read them (the last two by byte offset). */
typedef struct EmPoisonParam {
    s16 threshold;      /* 0x0: the initial poisonThreshold */
    s16 decayInterval;  /* 0x2: frames between decays of poisonBuildup (poisonTimer) */
    s16 decayAmount;    /* 0x4: taken off poisonBuildup at each decay */
    s16 damage;         /* 0x6: taken off hp at each poison tick */
    s16 duration;       /* 0x8: poisonDamage when poisoned: the frames the poison lasts */
    s16 tickInterval;   /* 0xA: frames between poison ticks (poisonTimer while poisoned) */
    s16 thresholdStep;  /* 0xC: added to poisonThreshold when the poison ends, up to four times */
} EmPoisonParam;

/* The per-kind status parameters, indexed by Em.type (-1 thresholds: immune). */
extern EmStatusParam *gEmStatus1Params[];
extern EmStatusParam *gEmStatus2Params[];
extern EmStatusParam *gEmStatus4Params[];
extern EmStatusParam *gEmStatus5Params[];

/* The rates of a kind's need gauge (gEmRateTbl, gEmDecay7ECRates; 24 bytes):
 * the gauge's maximum and starting value, the level at which
 * Em_FindNearAreaObj seeks an area object, the drain per frame out of and in
 * combat, and the cut Em_TypeSetup takes off once. */
typedef struct EmRate {
    s32 max;            /* 0x00: GmEm_Setup copies it to the need's ...Max */
    s32 start;          /* 0x04: GmEm_Setup's value for a gauge at 0 or below */
    s32 seekLevel;      /* 0x08 */
    s32 calmDrain;      /* 0x0C */
    s32 combatDrain;    /* 0x10 */
    s32 setupCut;       /* 0x14 */
} EmRate;

/* A row of the poison parameters (gEmStatusParams), 16 bytes: EmPoisonParam
 * and padding. */
typedef struct EmStatusRow {
    EmPoisonParam param;
    s16 pad0E;
} EmStatusRow;

/* The need-3 rates, one pointer per kind (GmEm_Setup, Em_DecayNeed3), and
 * the poison parameter rows after them: the table runs straight into the
 * rows, so the two are one object. A need-3 row is 16 bytes: the maximum, the
 * starting value, then the calm and combat drains at +0x08 and +0x0C (read
 * through EmRate as seekLevel and calmDrain). */
typedef struct EmRateStatusTbl {
    EmRate *rate[74];
    EmStatusRow status[58];
} EmRateStatusTbl;
extern EmRateStatusTbl gEmRateStatus;

/* A row of a kind's reaction table (gEmReactions[kind], by reaction kind,
 * Em_GetReaction): what the reaction adds to the player's noticeCount, and
 * the cap the sum must stay under (Em_AddNotice). */
typedef struct EmReaction {
    s32 noticeAdd;
    s32 noticeMax;
} EmReaction;

/* A kind's reaction set k (gEmReactionDists[kind][k], Em_LoadReactionSet). */
typedef struct EmReactionDists {
    f32 dist[4];        /* 0x00: the distance limits: Em.reactDist[0] and [1], nearRadius, and the fourth at Em +0x770 */
    f32 range[4];       /* 0x10: the reaction ranges: Em.reactRange[0] to [2], and the fourth at Em +0x844 */
} EmReactionDists;

/* A kind's reaction set k (gEmReactionAngles[kind][k], Em_LoadReactionSet). */
typedef struct EmReactionAngles {
    u16 sectorHalfAngle[4]; /* Em.sectorHalfAngle: the sense sectors' half widths */
} EmReactionAngles;

/* The model (struct Model, EmBone, ModelMat): include/model.h. */

/*
 * A row of a kind's motion table (gEmMotionRows[kind], 12 bytes), read by Em_SetMotion and
 * Em_SetMotionRow. A layer's motion id is 1000 + 200 * layer + motion.
 */
typedef struct EmMotionRow {
    u8 motion;          /* 0x0 */
    s8 layers;          /* 0x1: which layers (0-2) get it; 16-18: all but one; -1: all */
    u8 unk2;            /* 0x2 */
    u8 trans;           /* 0x3: row of the kind's transition table (gEmMotionTransRows) for the motion that follows */
    s16 frame;          /* 0x4: start frame when the caller passes 0 */
    s8 blend;           /* 0x6: blend frames when the caller passes 0 (Em +0x418 when this is 0) */
    s8 nextBlend;       /* 0x7: copied to Em.rowBlend once the motion is set: the blend of the motion set after it */
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
 * table of 96 bytes in each family's .data).
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
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 (runs MotionEffects): Kirin, Anteka, kind 29 (the families with a view keep their own, at +0 or elsewhere) */
    u8 unk001[0x002 - 0x001];
    s16 sndMotion;              /* 0x002: the motion id the MotionEffects functions keep (motionId[0], stored when it changes) and the MotionSounds copies (Kut-Ku, Garuga, Rathalos, Gravios, Diablos, Khezu, Chameleos, Fatalis) switch on; Rathalos and Fatalis write it through their views (EmFlyerWork.sndMotion, EmFatalisWork.fxMotion); Yama Tsukami keeps a timer here (EmYamaTsukamiWork) */
    s16 prevMotion;             /* 0x004: a motion id kept to see it change (motionId[0] stored and compared; switched on) */
    s16 fxMotion;               /* 0x006: the motion id the MotionEffects and MotionSounds functions last saw (motionId[0] stored and compared; switched on); the flying families (Rathalos, Kutku, Garuga, Gravios, Diablos) keep frames since landing here instead (150 set by the landing states, counted down by the update) */
    s32 accelAngX;              /* 0x008: angles X, Y, Z for Em_MoveRotatedAccel (&accelAngX; set from rotX, rotY and 0) */
    s32 accelAngY;              /* 0x00C: set from rotY, or rotY plus or minus a quarter turn, plus half a turn */
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
    s32 turnSpeed;              /* 0x040: Em_TurnToGoal speed (256) */
    s32 rollSpeed;              /* 0x044: flying families: rotZ is stepped back to 0 by it (256 at init); Velocidrome and Velociprey State3: an f32 velZ kept while a wall stops the jump */
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
    s32 motLeft;                /* 0x14: the motion when the goal is to one side (angle to it half a turn or more) */
    s32 motRight;               /* 0x18: the motion for the other side */
} EmTurnParams;
typedef char EmTurnParams_size_check[(sizeof(EmTurnParams) == 0x1C) ? 1 : -1];

/* The flying families' (Kut-Ku, Garuga, Rathalos, Gravios) own meanings of the work area's
   bytes (EmWork has the shared ones): their copies of the same functions read one layout. */
typedef struct EmFlyerWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x002 - 0x001];
    s16 sndMotion;              /* 0x002: as EmWork's: the motion id MotionEffects keeps (Rathalos) */
    u8 earOpen;                 /* 0x004: EM_KIND_YIAN_KUT_KU, EM_KIND_BLUE_YIAN_KUT_KU and EM_KIND_YIAN_GARUGA: the ears' opening, 0 to 30 frames: rises a frame at a time in combat while hp is at least a fifth, falls otherwise; sub_main Em_AdjustKindBones turns bones 37 to 39 by it */
    u8 legsLeft;                /* 0x005: State2_06: the flight's goals left (SetGroup2State: 4 or 1); each goal reached takes one, 0 ends the flight */
    s16 fxMotion;               /* 0x006: as EmWork's: frames since landing (Init: 0; Update counts it down) */
    u8 crestGlow;               /* 0x008: Kut-Ku: Update toggles it each frame while enraged (else 0); State3_1D sets it during its motion; the draw shows a mesh of set 3 only while it and partWhole are set */
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
    u16 turnSpeed;              /* 0x040: Em_TurnToGoal's speed (Init stores the word, EmWork's turnSpeed: 512); the states read the halfword */
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
    s8 fortHits;                /* 0x001: EmKushala_BreathAttack and EmKushala_BlastRingFort count the hits on the fort up to 10 (then Quest_TriggerEvent) and copy the count to gScene.fortStage */
    u8 unk002[0x004 - 0x002];
    u8 fxStep;                  /* 0x004: UpdateMotionEffects: 0, then 1 */
    u8 unk005[0x008 - 0x005];
    u8 windOn;                  /* 0x008: the wind (Kushala Daora) or aura (Lunastra, Teostra) is up: EmKushala_SetWind sets it (KUSH_IDLE_WIND_ROAR ...) and clears it (the rest, the knock-downs, the death); EmKushala_WindActive tests it */
    u8 legsLeft;                /* 0x009: EmKushala_DashCircuit's legs left (EmKushala_SetFlyState: 4); each point reached takes one */
    s16 windFrames;             /* 0x00A: EmKushala_SetWind: 90 when the wind starts, 0 when it stops; nothing in C reads it */
    u8 unk00C[0x00E - 0x00C];
    u16 unk00E;                 /* 0x00E: EmBombState returns it != 0 */
    u8 unk010[0x018 - 0x010];
    u8 flyGuard;                /* 0x018: as EmWork's (read as s8) */
    u8 unk019[0x01B - 0x019];
    u8 unk01B;                  /* 0x01B: Init: 0 */
    s32 moveAngBX;              /* 0x01C: as EmWork's */
    s32 moveAngBY;              /* 0x020 */
    f32 dashGoal[3];            /* 0x024: EmKushala_SetMoveState, the side dashes: the goal (a remote copy: a point ahead along rotY) */
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

/* Yama Tsukami's own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmYamaTsukamiWork {
    s8 queryValue;              /* 0x000: EmYamaTsukami_ScriptQuery's answer to every query but 1; Setup and Start clear it, nothing in C sets it */
    s8 fortHits;                /* 0x001: YAMA_ATK_SMASH_FORT's hits on the fortress, up to 10 (the tenth raises the quest's event); copied to gScene.fortStage */
    s16 syncTimer;              /* 0x002: Start: 150; Update counts it down, then sends the monster's state (QSync_SendEm) and restarts it */
    u8 unk004[0x005 - 0x004];
    u8 fxStep;                  /* 0x005: UpdateMotionEffects: 0, then 1 (runs MotionEffects and EaseToSize) */
    u8 unk006[0x008 - 0x006];
    s16 fxMotion;               /* 0x008: the motion id (motionId[0]) MotionEffects last saw; MotionSounds switches on it */
    s8 unk00A;                  /* 0x00A: MotionEffects clears it when the motion changes; nothing reads it */
    s8 runUpTimer;              /* 0x00B: YAMA_FLY_DASH_TO_GOAL: 30 frames before the cruise */
    s32 turnLeft;               /* 0x00C: EmYamaTsukami_Turn: half the turn, less turnSpeed each frame until it runs out */
    s32 accelAngX;              /* 0x010: angles X, Y, Z for Em_MoveRotated and Em_MoveRotatedAccel (AimAtGoal; UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x014 */
    s32 accelAngZ;              /* 0x018 */
    s32 turnSpeed;              /* 0x01C: EmYamaTsukami_Turn: grows by 32 a frame, then shrinks; EmYamaTsukami_Spin: the spin's step */
    f32 bodyScale[3];           /* 0x020: eased toward EmYamaTsukami_SizeScales[bodySize] (EaseToSize; Start: 1.0); ScaleBones grows bones 6, 4 and 35 by them */
    s16 curveFrame;             /* 0x02C: the curve frame (UpdateCurves adds the motion's step) */
    u8 flinchStage;             /* 0x02E: enum EmYamaTsukamiFlinchStage, the next part flinch (Start and SetReactState set it) */
    u8 curveKind;               /* 0x02F: enum EmYamaTsukamiCurve, the curve tables' index (StartCurves) */
    u8 curveNone;               /* 0x030: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x031 */
    u8 curveSpeed;              /* 0x032 */
    s8 pauseCycles;             /* 0x033: YAMA_FLY_RECOVER_PAUSE's motion cycles (Start 15; the falls 30) */
    s8 bodySize;                /* 0x034: enum EmYamaTsukamiSize, the size the body eases to (Start: YAMA_SIZE_FULL) */
    s8 easeMode;                /* 0x035: EaseToSize eases in mode 0 only (Start clears it; nothing sets another) */
    s8 mineSlot;                /* 0x036: the mine point AddMinePoint made (-1: none) */
    s8 mineCount;               /* 0x037: AddMinePoint's count (none at 0) */
    u8 flyGuard;                /* 0x038: set by the _GUARDED states and the second kind: Update then keeps guardTimer at 2 (no kill, no flinch); ChangeState clears it unless the next state is YAMA_IDLE or YAMA_FLY_IDLE */
    s8 minePoints[3];           /* 0x039: mine points RemoveMinePoints removes (-1: none); nothing in C sets them */
    s8 dustState[4][2];         /* 0x03C: per tentacle bone (EmYamaTsukami_TentacleBones): 0 at or under the floor, 1 above it, 2 once the pair made its dust */
    u8 unk044[0x100 - 0x044];
} EmYamaTsukamiWork;
typedef char EmYamaTsukamiWork_size_check[(sizeof(EmYamaTsukamiWork) == sizeof(EmWork)) ? 1 : -1];

/* Shen Gaoren's own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmShenGaorenWork {
    u8 stage;                   /* 0x000: the fight's stage, as the fort monsters' (synced in their state message): Setup and Init 0; SHEN_ATK_SMASH sets 8 in AREA_FORTRESS_3 when below 8; EmShenGaoren_ScriptQuery returns it to the script */
    s8 fortHits;                /* 0x001: SHEN_ATK_SHOT_FORT, SHEN_ATK_STRIKE_FORT and SHEN_ATK_SMASH_FORT: the hits on the fort, up to 10, copied to gScene.fortStage */
    s16 syncTimer;              /* 0x002: Init: 150; Update counts it down, then sends the monster's state (QSync_SendEm kind 2) and restarts it */
    u8 unk004[0x005 - 0x004];
    u8 fxStep;                  /* 0x005: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk006[0x008 - 0x006];
    s16 fxMotion;               /* 0x008: the motion id MotionEffects last saw (motionId[0] stored and compared; MotionSounds and the ease step switch on it) */
    u8 unk00A[0x00C - 0x00A];
    f32 unk00C;                 /* 0x00C: Init: 1500.0; no other use */
    f32 ease[4];                /* 0x010: EmShenGaoren_EaseDownPose eases them toward a row of EmShenGaoren_DownEaseTargets (the lying row in the knockdowns and collapses, else the standing row), by a 120th of the difference a frame (Init: 1.0) */
    u8 unk020;                  /* 0x020: MotionEffects clears it on a new motion; nothing reads it */
    u8 unk021[0x022 - 0x021];
    s8 unk022;                  /* 0x022: Init: 0; no other use */
    s8 mineSlot;                /* 0x023: the mine point AddMinePoint made (-1: none) */
    s8 mineCount;               /* 0x024: the mine point's count (Init: 3; Update copies MinePoint_Count) */
    u8 unk025[0x026 - 0x025];
    s8 partFlinch[7];           /* 0x026: by flinch part, parts 3 to 6: 0, then 1 after a first flinch (SHEN_REACT_LEG_A_LOW to _D_LOW and SHEN_REACT_LEG_A_TALL to _D_TALL), 2 after a second (SHEN_REACT_LEG_A_AGAIN_LOW to _D_AGAIN_LOW); Update picks the next flinch state by it, and when all four are past 1, SHEN_REACT_LEGS_COLLAPSE and SHEN_REACT_LEGS_COLLAPSE_SHOT break part 1 and clear them. EmShenGaoren_RemoveMinePoints (a copy of Lao-Shan's) removes entries 0 to 2 as mine points */
    u8 unk02D[0x100 - 0x02D];
} EmShenGaorenWork;
typedef char EmShenGaorenWork_size_check[(sizeof(EmShenGaorenWork) == sizeof(EmWork)) ? 1 : -1];

/* Lao-Shan Lung's own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmLaoshanWork {
    u8 stage;                   /* 0x000: the fight's stage, as the fort monsters' (synced in their state message): Setup and Init 0, LAOSHAN_EVENT_INTRO 1, LAOSHAN_ATK_RAM_FORT_STAGE2 2, LAOSHAN_ATK_RAM_FORT_STAGE3 3 */
    s8 fortHits;                /* 0x001: State3_02: the hits on the fort, up to 10 (then Quest_TriggerEvent), copied to gScene.fortStage */
    s16 syncTimer;              /* 0x002: Init: 150; Update counts it down, then sends the monster's state (QSync_SendEm kind 2) and restarts it */
    u8 unk004[0x005 - 0x004];
    u8 fxStep;                  /* 0x005: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk006[0x008 - 0x006];
    s16 fxMotion;               /* 0x008: the motion id MotionEffects last saw (motionId[0] stored and compared; MotionSounds switches on it) */
    u8 unk00A;                  /* 0x00A: MotionEffects clears it on a new motion; nothing reads it */
    u8 unk00B[0x00C - 0x00B];
    f32 unk00C;                 /* 0x00C: Init: 1500.0; no other use */
    s16 turnHeldFrames;         /* 0x010: Init: 0; Update counts it up while turnHeld is set; nothing reads it */
    s8 mineSlot;                /* 0x012: the mine point AddMinePoint made (-1: none) */
    s8 mineCount;               /* 0x013: the mine point's count (Init: 3; Update copies MinePoint_Count) */
    s32 unk014;                 /* 0x014: Init: -1; no other use */
    s32 unk018;                 /* 0x018: Init: -1; no other use */
    u8 turnHeld;                /* 0x01C: 1 when LAOSHAN_MOVE_TURN_RIGHT_START ends (rotY risen, the pose held), 2 when LAOSHAN_MOVE_TURN_LEFT_START ends; Init and the states that leave the pose: 0 */
    u8 fxSpot;                  /* 0x01D: MotionEffects: the StageShot_CreateAtSpot spot, 0 to 3 in turn */
    s8 minePts[3];              /* 0x01E: AddMinePoints' three mine points at the bones of EmLaoshan_MinePtBones (-1: none) */
    u8 unk021[0x100 - 0x021];
} EmLaoshanWork;
typedef char EmLaoshanWork_size_check[(sizeof(EmLaoshanWork) == sizeof(EmWork)) ? 1 : -1];

/* Chameleos's own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmChameleosWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk001[0x002 - 0x001];
    s16 fxMotion;               /* 0x002: the motion id MotionSounds compares */
    u8 unk004[0x005 - 0x004];   /* 0x004: Em.fullCloak (the same byte), used through the Em */
    u8 routeLegs;               /* 0x005: the legs left of a route: EmChameleos_SetFlyState sets 4 for CHAM_FLY_ROUTE and CHAM_FLY_ROUTE_LOOP, 1 for CHAM_FLY_TO_GOAL; EmChameleos_FlyRoute counts it down at each goal and glides on (CHAM_FLY_FORWARD) at 0 */
    u8 unk006[0x010 - 0x006];
    s8 flyGuard;                /* 0x010: as EmWork.flyGuard of the other flyers: the forward flights set 1, hovering, landing and the leaving events 0; while it is 1 in EM_GROUP_FLY, Update keeps guardTimer at 2 */
    u8 unk011[0x013 - 0x011];
    u8 wantInvisible;           /* 0x013: Init: 2; UpdateInvisibility sets it from EmChameleos_WantsInvisible when 0 */
    s32 unk014;                 /* 0x014: Init: 0; no other use */
    s32 unk018;                 /* 0x018: Init: 8192; no other use */
    u8 unk01C[0x028 - 0x01C];
    s32 moveAngCX;              /* 0x028: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves; Z at moveAngX) */
    s32 moveAngCY;              /* 0x02C */
    s32 moveAngX;               /* 0x030 */
    s32 moveAngY;               /* 0x034 */
    s32 moveAngZ;               /* 0x038 */
    s32 pitchStep;              /* 0x03C */
    u8 unk040[0x044 - 0x040];
    s8 curveNone;               /* 0x044: the three curves' run flags (StartCurves, UpdateCurves) */
    s8 curveVelY;               /* 0x045 */
    s8 curveSpeed;              /* 0x046 */
    u8 curveKind;               /* 0x047: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x048: the curve frame (StartCurves, UpdateCurves) */
    u8 unk04A[0x04C - 0x04A];
    u16 invisFade;              /* 0x04C: Em.invisFade (the same bytes): the cloak's shimmer, counted 1 to 32 by EmChameleos_UpdateInvisibility; 0xFFFF when none (Init) */
    s16 shimmerTimer;           /* 0x04E: Init: 900; EmChameleos_UpdateInvisibility counts it down and restarts it at 900; under the full cloak the shimmer replays at each restart (every 900 frames) */
    u8 unk050[0x100 - 0x050];
} EmChameleosWork;
typedef char EmChameleosWork_size_check[(sizeof(EmChameleosWork) == sizeof(EmWork)) ? 1 : -1];

/* Remobra's own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmRemobraWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x014 - 0x001];
    f32 home[3];                /* 0x014: Init: the position; ChangeState copies it to the goal */
    u8 curveNone;               /* 0x020: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x021 */
    u8 curveSpeed;              /* 0x022 */
    u8 curveKind;               /* 0x023: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x024: the curve frame (StartCurves, UpdateCurves) */
    u8 unk026[0x028 - 0x026];
    f32 distLeft;               /* 0x028: ChangeState: the distance to the goal (Vec_DistanceXZ, or 1000) */
    u16 turnYaw;                /* 0x02C: the yaw to turn to: EmRemobra_ChangeState sets it (Math_AngleXZ to the goal for REMOBRA_FLY_TURN_TO_GOAL, rotY plus a random part for REMOBRA_FLY_TURN_RANDOM); EmRemobra_TurnInAir turns rotY to it */
    u8 unk02E[0x02F - 0x02E];
    u8 steerToGoal;             /* 0x02F: EmRemobra_ChangeState: 1 when the state has a goal (for the target goal, only while a target exists); the swoops and glides turn rotY toward the goal only while it is 1 */
    f32 hopVel;                 /* 0x030: UpdateHop: 14, then less hopAccel each frame */
    f32 hopAccel;               /* 0x034: UpdateHop: -hopVel / (hopFrames / 2) */
    u8 unk038[0x03A - 0x038];
    s16 hopFrames;              /* 0x03A: UpdateHop: 34 */
    u8 unk03C[0x100 - 0x03C];
} EmRemobraWork;
typedef char EmRemobraWork_size_check[(sizeof(EmRemobraWork) == sizeof(EmWork)) ? 1 : -1];

/* Mosswine's own meanings of the work area's bytes (EmWork has the shared ones); EM_KIND_FELYNE_CART's copies of its functions read the same layout. */
typedef struct EmMosswineWork {
    u8 fxStep;                  /* 0x000: EM_KIND_FELYNE_CART's UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk001[0x00C - 0x001];
    u16 unk00C;                 /* 0x00C: Init: 0; Update counts it down to 0 (nothing in C sets it) */
    s16 unk00E;                 /* 0x00E: Init: 0 */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the position (Y stored as 0; EM_KIND_FELYNE_CART's Init stores posY in Z's place); EmMosswine_WanderHome turns toward it */
    u8 unk020[0x100 - 0x020];
} EmMosswineWork;
typedef char EmMosswineWork_size_check[(sizeof(EmMosswineWork) == sizeof(EmWork)) ? 1 : -1];

/* Khezu's own meanings of the work area's bytes (EmWork has the shared ones; EM_KIND_KHEZU and EM_KIND_RED_KHEZU).
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
    s8 legsLeft;                /* 0x00A: the ceiling circuit's legs left (EmKhezu_SetFlyState: 4; EmKhezu_CrawlCircuit counts them down) */
    u8 unk00B[0x010 - 0x00B];
    f32 pickDist;               /* 0x010: a distance kept when a state is picked: to the chosen ceiling point (KHEZU_FLY_CEIL_TURN_TO_ZONE), to the target clamped to 2500-5000 (KHEZU_ATK_ACTION_A); no reader found */
    s16 goalYaw;                /* 0x014: EmWork.goalYaw: the turns to the goal keep the goal's heading here; Init: ANGLE_TURN / 4 */
    u8 unk016[0x017 - 0x016];
    u8 zonePicked;              /* 0x017: EmKhezu_SetFlyState (KHEZU_FLY_CEIL_TURN_TO_ZONE): 1 once a ceiling point toward the target's zone was picked; no reader found */
    u8 unk018[0x01C - 0x018];
    s32 unk01C;                 /* 0x01C: Init: 0 */
    s32 accelAngX;              /* 0x020: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x024 */
    s32 accelAngZ;              /* 0x028 */
    s32 unk02C;                 /* 0x02C: Init: 256 */
    u16 turnSpeed;              /* 0x030: Em_TurnToGoal's speed (Init stores 512 as a word) */
    u8 unk032[0x034 - 0x032];
    s32 unk034;                 /* 0x034: Init: 256 */
    u8 unk038[0x040 - 0x038];
    u8 ceilKind;                /* 0x040: the ceiling triangle over the monster, as AreaGrid2_GetCeiling fills it (EmKhezu_Update): its kind, */
    u8 unk041;                  /* 0x041: its second byte, */
    u16 zone;                   /* 0x042: and its zone (1-4): EmKhezu_SetFlyState picks the ceiling point of the target's zone by it */
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

/* A pending placement of the monster (two at +0x8B4, 76 bytes each). */
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
    u8 lowHpRequest; /* 0x0: the request made at low health (Em.lowHpNeed, priority 128) */
    u8 need1Request; /* 0x1: for Em.need1, priority 96 */
    u8 need2Request; /* 0x2: for Em.need2, priority 80 */
    u8 need3Request; /* 0x3: for Em.need3 out of combat, priority 64 */
    u8 need4Request; /* 0x4: for Em.need4, priority 112 */
    u8 pad5[3];      /* 0x5: 0 in all 74 entries; nothing reads them */
} EmKindInfo;

/* An object with a position and a radius (sub_main's tables of 32 at gEmSenseObjs (0x5D8A70) and gSightBlockObjs (0x5D8BF0)). */
typedef struct EmObj {
    f32 pos[3];         /* 0x00 */
    f32 radius;         /* 0x0C */
    u8 goalArg; /* 0x10: matched with Em.curGoalArg (the monsters goal search); a player index for the sense objects */
    u8 goalNo; /* 0x11: matched with Em.curGoalNo */
    u8 kind;            /* 0x12: a bait's kind (Em_ApplyActionStatus): 0 plain meat, 1 poison, 2 status 2, 3 status 4 */
    u8 done;            /* 0x13: StageObj_K0C type 3 (placed by a player, gScene.placed3) clears it; set when a monster has used it (Em_UseGoalAction); nonzero ends it (ItemProp_RunBait removes it from gAreaObjList) */
    s8 timer;           /* 0x14: frames left in gSightBlockObjs/gFlashObjs (SightBlock_Tick, FlashObj_Tick remove it at 0); 3 for the Bulldrome dust, 5 for flashes */
    u8 pad15;
    s16 statusAmount;   /* 0x16: a bait's status amount: StageObj_K0C type 3 sets it from ItemProp_BaitStatusAmounts[var1] (0 or 200); Em_ApplyActionStatus adds it to the status of the bait's kind */
    u16 area; /* 0x18: the area it is in (compared with Em.area) */
} EmObj;

/* Plesioth's own meanings of the work area's bytes (EmWork has the shared ones); em_plesioth.c, and sub_main's
 * Em_AdjustKindBones (the fins). */
typedef struct EmPlesiothWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x007 - 0x001];
    u8 roundLegsLeft;           /* 0x007: the legs left of a round swim: EmPlesioth_SetSwimState sets 4 for PLES_SWIM_ROUND_NEXT and _PREV, SwimRoundNext and SwimRoundPrev count it down at each area point and end the state at 0 */
    u8 unk008[0x00A - 0x008];
    s16 unk00A;                 /* 0x00A: Reset: 16384 */
    u8 unk00C[0x00F - 0x00C];
    u8 finMode;                 /* 0x00F: EmPlesioth_SetFinMode: 0 still, 1 waving, 2 waving with the fins folded (hp at most a fifth), 3 folded */
    s16 unk010;                 /* 0x010 */
    u8 unk012[0x013 - 0x012];
    u8 unk013;                  /* 0x013: EmPlesioth_SetAttackState sets 1 entering PLES_ATK_BREATH; no reader found */
    s32 unk014;                 /* 0x014 */
    s32 unk018;                 /* 0x018 */
    s32 moveAngBX;              /* 0x01C: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 moveAngBY;              /* 0x020 */
    s32 moveAngBZ;              /* 0x024 */
    s32 unk028;                 /* 0x028: Reset: 256 */
    s32 unk02C;                 /* 0x02C: Reset: 512 */
    s32 unk030;                 /* 0x030: Reset: 256 */
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
    } finRot[4];                /* 0x04E: per fin, read by Em_AdjustKindBones (sub_main) */
    s16 finFrame[4];            /* 0x05E: per fin, its wave's key frame */
    s8 finCount[4];             /* 0x066: per fin, the frames left in its fold */
    u8 unk06A[0x100 - 0x06A];
} EmPlesiothWork;
typedef char EmPlesiothWork_size_check[(sizeof(EmPlesiothWork) == sizeof(EmWork)) ? 1 : -1];

/* Cephadrome's own meanings of the work area's bytes (EmWork has the shared ones); em_cephadrome.c. The layout
 * is Plesioth's (EmPlesiothWork) with the curves and the fins 4 bytes further on. */
typedef struct EmCephadromeWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x007 - 0x001];
    u8 roundLegsLeft;           /* 0x007: the legs left of a round swim: EmCephadrome_SetSandState sets 4 for CEPH_SAND_ROUND_NEXT and _PREV, SandRoundNext and SandRoundPrev count it down at each area point and end the state at 0 */
    u8 unk008[0x00A - 0x008];
    u16 turnYaw;                /* 0x00A: the heading EmCephadrome_EntranceScene turns to in place (Math_AngleXZ to the goal); Init: ANGLE_TURN / 4 */
    u8 unk00C[0x00F - 0x00C];
    u8 finMode;                 /* 0x00F: EmCephadrome_SetFinMode: 0 still, 1 waving, 2 waving with the fins folded (hp at most a tenth), 3 folded */
    s16 unk010;                 /* 0x010 */
    u8 unk012[0x013 - 0x012];
    u8 unk013;                  /* 0x013: EmCephadrome_SetAttackState sets 1 entering CEPH_ATK_BREATH; no reader found */
    s32 unk014;                 /* 0x014: Init: 0; no other use */
    s32 unk018;                 /* 0x018: Init: 8192; no other use */
    s32 moveAngBX;              /* 0x01C: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 moveAngBY;              /* 0x020 */
    s32 moveAngBZ;              /* 0x024 */
    s32 unk028;                 /* 0x028: Init: 256; no other use */
    s32 unk02C;                 /* 0x02C: Init: 512; no other use */
    s32 unk030;                 /* 0x030: Init: 256; no other use */
    f32 distLeft;               /* 0x034: the distance left to the goal (Vec_DistanceXZ, or 1000), less velZ each frame */
    u8 unk038;                  /* 0x038: Init: 0; no other use */
    u8 unk039;                  /* 0x039: Init: 0; no other use */
    u8 unk03A;                  /* 0x03A: Init: 0; no other use */
    u8 unk03B[0x03C - 0x03B];
    u8 curveNone;               /* 0x03C: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x03D */
    u8 curveSpeed;              /* 0x03E */
    u8 curveKind;               /* 0x03F: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x040: the curve frame (UpdateCurves adds the motion's step) */
    s16 finWait[4];             /* 0x042: per fin, the frames before its wave starts again (EmCephadrome_FinWait1/2, FinRewait1/2) */
    u8 finState[4][2];          /* 0x04A: per fin, [0] its wave's state (EmCephadrome_UpdateFinWave: 0 still, 1 waiting, 2 and 3 the two key tables), [1] its fold's (EmCephadrome_UpdateFinFold: 1 opening, 2 folding) */
    struct {
        u16 fold;               /* turn about x (folded to EmCephadrome_FinFoldTarget[i] in fin modes 2 and 3, 0 in mode 1) */
        u16 wave;               /* turn about y (EmCephadrome_FinKeys1/2's keys) */
    } finRot[4];                /* 0x052: per fin */
    s16 finFrame[4];            /* 0x062: per fin, its wave's key frame */
    s8 finCount[4];             /* 0x06A: per fin, the frames left in its fold */
    u8 unk06E[0x100 - 0x06E];
} EmCephadromeWork;
typedef char EmCephadromeWork_size_check[(sizeof(EmCephadromeWork) == sizeof(EmWork)) ? 1 : -1];

/* The Melynx's own meanings of the work area's bytes (EmWork has the shared ones); em_melynx.c. */
typedef struct EmMelynxWork {
    u8 faceStep;                /* 0x000: EmMelynx_UpdateFace: 0, then 1 (seeds blinkTimer) */
    u8 unk001[0x004 - 0x001];
    s16 stateTimer;             /* 0x004: MELYNX_REACT_STATUS4 counts down its 58 frames; MELYNX_MOVE_CHASE sets 138 and nothing reads it there */
    u8 unk006[0x00C - 0x006];
    u16 unk00C;                 /* 0x00C: Init: 0; EmMelynx_Update counts it down while nonzero; nothing sets it nonzero */
    s16 holdBomb;               /* 0x00E: 1 while it holds a bomb: EmMelynx_ChangeState sets it entering most move states, HoldBomb and HoldBigBomb when they make the bomb; ThrowBomb, and Update when hit, clear it; EndState keeps it across its ChangeState */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the spawn position (posX, 0, posZ); EmMelynx_WalkHome walks back to it */
    struct ObjAttach *eff;      /* 0x020: EmMelynx_StealAttack: its contact attack (EmContactAttack_Create, kind 7 or 2) */
    struct Player *stolenFrom;  /* 0x024: EmMelynx_StealItem: the player robbed; ReturnItem gives the item back to it; DeductReward passes it to Quest_DeductReward */
    EmObj sense;                /* 0x028: Em_SenseObjects' object; EmMelynx_Update places it at the position when hit (radius 3000, kind 1) and lists it (alertState 1) */
    u8 unk044[0x048 - 0x044];
    u16 blinkTimer;             /* 0x048: EmMelynx_UpdateFace: frames, the eyes' frame by (blinkTimer / 2) % 20 */
    s16 stolenItem;             /* 0x04A: the item EmMelynx_StealItem took (-1 none); ReturnItem gives it back, DeductReward takes it from the quest's reward */
    s16 stolenCount;            /* 0x04C: EmMelynx_StealItem: -1 (Pouch_Add takes one); ReturnItem adds -stolenCount */
    u8 eyeFrame;                /* 0x04E: EmMelynx_UpdateFace: 0 open, 1 and 2 the blink, 3 in groups 4 and 5 (SetMeshSetAlpha shows that eye mesh) */
    u8 alertState;              /* 0x04F: 0 calm; 1 when hit (sense listed); 2 alerted (EmMelynx_Update: something sensed while calm, or sense removed) */
    u8 unk050[0x100 - 0x050];
} EmMelynxWork;
typedef char EmMelynxWork_size_check[(sizeof(EmMelynxWork) == sizeof(EmWork)) ? 1 : -1];

/* The Apceros's own meanings of the work area's bytes (EmWork has the shared ones); em_apceros.c. */
typedef struct EmApcerosWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x00A - 0x001];
    u8 turnOn;                  /* 0x00A: the turn motions (EmApceros_WatchHunter, WalkTurnA and B, FlinchRandom): set at frame 6, cleared at 122; rotY steps while set */
    u8 unk00B[0x00E - 0x00B];
    s16 turnDelta;              /* 0x00E: TurnToGoal: targetAngle - rotY, turned by a 66th of it each frame */
    f32 home[3];                /* 0x010: Init: the spawn position; ChangeState copies it to the goal */
    u8 unk01C[0x020 - 0x01C];
    EmObj sense;                /* 0x020: Em_SenseObjects' object; Update places it at the position (radius 2000, kind 2) and adds it to the sensed-objects list (objListed); the hunted states remove it */
    u8 objListed;               /* 0x03C: 1 while sense is in the sensed-objects list (Update adds it, RunOffAndLeave and the hunted states remove it) */
    u8 alarmed;                 /* 0x03D: CheckHerdHurt: 1 when the herd's leader (variant 2) in the area is in group 4 or 5; Update sets it too; SetRequest drops request 2 while set; ScriptQuery returns it */
    u8 unk03E[0x100 - 0x03E];
} EmApcerosWork;
typedef char EmApcerosWork_size_check[(sizeof(EmApcerosWork) == sizeof(EmWork)) ? 1 : -1];

/* Blangonga's own meanings of the work area's bytes (EmWork has the shared ones); em_blangonga.c, and sub_main's
 * Em_AdjustKindBones (rageTimer). */
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
    u8 moveCount;               /* 0x018: the moves in a row: ChangeState adds one on entering BLANG_MOVE_STEP_A..D (when Em_IsSyncFree), keeps it for BLANG_IDLE in EM_SYNC_LOCAL and clears it on any other change; ScriptQuery 0 asks whether it reached 4 */
    u8 senseListed;             /* 0x019: 1 while `sense` is in the object list (EmBlangonga_Roar adds it; ChangeState removes it) */
    u8 unk01A[0x01C - 0x01A];
    EmObj sense;                /* 0x01C: the object Em_SenseObjects fills (ChangeState removes it from the list) */
    s16 rageTimer;              /* 0x038: UpdateRageTimer counts it up to its limit */
    s16 unk03A;                 /* 0x03A: Init clears it; no other use seen */
    u8 unk03C[0x03E - 0x03C];
    u8 carryHidden[3];          /* 0x03E: the Congalala kinds' three carried items, 1 while item i is not shown (Init hides all and shows the one EmBlangonga_PickCarried picks); gm_em.c shows a mesh per shown item; a broken part 4 drops it as a mine point (BLANG_REACT_DROP_*) */
    u8 tint[3];                 /* 0x041: the body colour, RGB: Init sets the kind's own, EmBlangonga_UpdateTint steps it toward the rage or low-hp colour */
    u8 eatenKind;               /* 0x044: 0, or 1-4: what the Congalala kinds last ate (BLANG_IDLE_EAT_KIND_A..D; Init sets it from the versus team flags); the breath (EmBlangonga_Breath) makes its effects with eatenKind - 1; ScriptQuery 3 returns it */
    u8 hitSeCooldown;           /* 0x045: frames: Update plays a sound when Congalala is hit in its armoured attack and waits 30 before the next */
    u16 pickSeed;               /* 0x046: a Quest_Random value (Start) EmBlangonga_PickCarried picks with */
    u8 sceneEating;             /* 0x048: Congalala's story scene: 1 from frame 34 of its eating motion until it roars (EmBlangonga_EatPropMode returns 1 then) */
    u8 unk049[0x04C - 0x049];
    f32 bonePos[3];             /* 0x04C: Init: Em_BonePos of bone 15; not read in em_blangonga.c */
    u8 unk058[0x100 - 0x058];
} EmBlangongaWork;
typedef char EmBlangongaWork_size_check[(sizeof(EmBlangongaWork) == sizeof(EmWork)) ? 1 : -1];

/* ItemProp's buffer (StageObj.buf) for ITEMPROP_SMOKE and ITEMPROP_BAIT: the smoke's rings, then the object the
 * monsters see: type 1's sight block (SightBlock_Add), type 3's area object (made by
 * ItemProp_CreateBait, ObjList_Add; ItemProp_RunBait ends when it is done). */
typedef struct ItemPropBuf {
    ItemPropRing ring[6];   /* 0x000 */
    EmObj obj;          /* 0x138 */
} ItemPropBuf;
typedef char ItemPropBuf_size_check[(sizeof(ItemPropBuf) == 0x154) ? 1 : -1];

/*
 * One area's value in a per-kind list of area values (the lists of
 * gEmAreaTimer1Values and gEmAreaTimer2Values, em_status.c):
 * Em_SetAreaTimer1 and Em_SetAreaTimer2 take the value of the monster's area,
 * else the one of the -1 entry that ends the list.
 */
typedef struct EmAreaValue {
    s16 id;             /* 0x0: >= 10000: Em.area + 10000, that area only; else an area id, matched through Area_DayId; -1 ends the list */
    s16 value;          /* 0x2: the timer's start value (Em.areaTimer1 or areaTimer2) */
} EmAreaValue;

/*
 * A kind's next-area lists (gm_em.c's gEmIdSets_<Kind>, which the game's
 * gEmIdSets holds by kind, em_status.c): by stage map (gScene.map), a list of
 * sets; Em_NextAreas and Em_NextAreasFrom take the set whose key is the
 * monster's questKey, else the -1 one, then the entry of the monster's area
 * (Area_DayId), and return its list of areas, ended by -1.
 */
typedef struct EmNextAreaEntry {
    u16 area;           /* 0x0: an area id, matched through Area_DayId; 255 ends the entries for the code */
    u16 pad;            /* 0x2: not read */
    s16 *next;          /* 0x4: the areas the monster can move on to from it, ended by -1 */
} EmNextAreaEntry;

typedef struct EmNextAreaSet {
    s32 key;            /* 0x0: matched with Em.questKey; -1: the default, the last set */
    EmNextAreaEntry *entries;   /* 0x4 */
} EmNextAreaSet;

/* gEmIdSets_KushalaDaora and gEmIdSets_Lunastra: the map table follows the
 * map's first set directly, in one object that starts with the set's
 * entries (the table is 8 bytes past a boundary of 16, where MWCC places no
 * object of its own; the game's gEmIdSets points at the table). */
typedef struct EmNextAreaBlock3 {
    EmNextAreaEntry entries[3]; /* 0x00: set's entries */
    EmNextAreaSet set[1];       /* 0x18 */
    EmNextAreaSet *maps[26];    /* 0x20: by stage map (MAP_COUNT, area.h) */
} EmNextAreaBlock3;

typedef struct EmNextAreaBlock7 {
    EmNextAreaEntry entries[7]; /* 0x00: set's entries */
    EmNextAreaSet set[1];       /* 0x38 */
    EmNextAreaSet *maps[26];    /* 0x40: by stage map (MAP_COUNT, area.h) */
} EmNextAreaBlock7;

/* One area's list of points in a per-kind area table (Em.tables[2] for the sand and water swimmers):
 * Em_FindAreaEntry finds the entry whose area is the monster's. */
typedef struct EmAreaPoints {
    s16 area;                   /* 0x00: area id */
    s16 n;                      /* 0x02: number of points */
    f32 *pts;                   /* 0x04: the points, three floats each */
} EmAreaPoints;

/* A monster's routes (Em_FindAreaEntry's result for one area set): per area, a list of points. */
typedef struct EmRoutePt {
    f32 pos[3];                 /* 0x00 */
    f32 radius;                 /* 0x0C: the point counts as reached within it */
    u16 canStart;               /* 0x10: nonzero for a point a route can start at */
    u16 script;                 /* 0x12: the point's script (Em.routePointScript; EmScript_RoutePointScript) */
} EmRoutePt;                    /* 0x14 */
typedef struct EmRoute {
    u16 area;                   /* 0x00: area id (Area_IdForCurrentTime) */
    s16 n;                      /* 0x02: number of points */
    EmRoutePt *pts;             /* 0x04 */
    f32 pos[3];                 /* 0x08: where a large monster is placed when it takes the route in another area */
    u16 rotY;                   /* 0x14: the heading it is given there */
    u8 unk16[2];
} EmRoute;                      /* 0x18 */
typedef struct EmRouteSet {
    s16 area;                   /* 0x00: area id, the one Em_FindAreaEntry matches */
    s16 n;                      /* 0x02: number of routes */
    EmRoute *routes;            /* 0x04 */
} EmRouteSet;

struct Em;
/*
 * A monster's handler table (Em.handlers): one per kind (sub_main's default
 * gEmNopHandlers, the game kinds' tables, gm_sub's gGmEmCallbackTbl, the lobby's
 * townspeople gNpcHandlers). The player has the same layout (PlayerVt).
 */
typedef struct EmVt {
    void (*init)(struct Em *em);            /* 0x00: called when it is set (Em_InitHandlers) */
    void (*update)(struct Em *em);          /* 0x04: every frame (Em_RunAll) */
    void (*unk08)(struct Em *em);
    void (*motionFx)(struct Em *em);        /* 0x0C: every frame, the kind's motion effects and sounds (Em_RunMotionFx) */
    u8 (*scriptQuery)(struct Em *em, u8 arg); /* 0x10: answers the script's query byte (EmScript_SwitchKindQuery) */
    void (*appear)(struct Em *em);          /* 0x14: Em_RunAppearHook */
    s32 (*part8DownQuery)(struct Em *em);   /* 0x18: nonzero lets part 8 go down when its hp runs out (Em_UpdateDamage; sets part8Down) */
} EmVt;

/* A row of gEmTraderMsgs (sub_main), the messages of the trader monster (EM_KIND_TRADER): one per
 * Em.msgId. EmTrader_ShowMessage copies the row into the message head at work + 0x20. */
typedef struct EmTraderMsg {
    u8 kind;            /* 0x0: what the talk does after it (EmTrader_RunTalk): 0 closes it, 1 a choice (next or nextNo), 2 goes on to next, 3 the trade, 8 and 9 a random chat line; 1, 4, 5 and 10 wait 30 frames first */
    u8 next;            /* 0x1: the message shown after it (for a choice, after the first answer); 255 ends the talk */
    u8 nextNo;          /* 0x2: for a choice, the message after the second answer; 255 ends the talk */
    u8 mode;            /* 0x3: Notice_OpenTalk's mode */
    char *text;         /* 0x4: the text; a "||" in it prints Em.msgItem's name (EmTrader_FormatMessage) */
} EmTraderMsg;
typedef char EmTraderMsg_size_check[(sizeof(EmTraderMsg) == 0x8) ? 1 : -1];

/* A kind's turning and bending limits (game gEmBendOrigins[kind]): Em_TurnToward turns the heading
 * toward a point and Em_BendStep bends the body by Em.bendAngle within them. Angles are ANGLE_TURN
 * to the turn. */
typedef struct EmBendRow {
    f32 ahead;          /* 0x00: the turn's origin, this far ahead of the monster along its facing */
    u8 unk04[2];        /* 0x04: not read in C (4, 0) */
    s8 bendBone[4];     /* 0x06: the bones Em.bendAngle[0..3] turn (sub_main Em_AdjustKindBones, for the kinds that bend) */
    u16 turnSpeed;      /* 0x0A: the turn's starting step per frame (Em_TurnToward's phase 1 and its resets) */
    u16 turnSpeedMax;   /* 0x0C: phase 2's top step (it eases to a half, then three eighths of it) */
    u16 turnAccel;      /* 0x0E: phase 2's change of the step per frame */
    u16 bendMax[4];     /* 0x10: the caps of the four bend angles (the excess goes to the next) */
    u16 slowAngle;      /* 0x18: the angle left under which the turn slows (/ 4, / 8) and the half-turn line test's margin */
    u8 pad1A[6];
} EmBendRow;
typedef char EmBendRow_size_check[(sizeof(EmBendRow) == 0x20) ? 1 : -1];

/* A kind's rage (game gEmRageTbl[kind], NULL for a kind that never enrages). */
typedef struct EmRageRow {
    s16 threshold;      /* 0x00: the rage buildup that enrages it (Em.rageThreshold; Em_InitRage) */
    s16 time;           /* 0x02: how long the rage lasts (Em.rageTimer; Em_StartRage, Em_StartRage2) */
    f32 motionSpeed;    /* 0x04: the motion speed while enraged (gm_sub's update) */
    f32 attackFactor;   /* 0x08: Em.attackFactor while enraged (times Em_LevelAttackFactor) */
    f32 damageFactor;   /* 0x0C: Em.damageFactor while enraged (times Em_LevelDamageFactor) */
    f32 buildupByHp[11]; /* 0x10: the factor on a hit's rage buildup by the hp left: 100%, from 90%, from 80% ... under 10% (Em_CheckRageByHp) */
    u32 pad3C;
} EmRageRow;
extern EmRageRow *gEmRageTbl[74];
extern u8 gEmIsLarge[80];
typedef char EmRageRow_size_check[(sizeof(EmRageRow) == 0x40) ? 1 : -1];

/* One hit zone of a monster: the percentage of each kind of damage a hit on a shape of that zone
 * (HitShape.zone) takes. A kind's rows are gEmHitzoneTables[type][Em.hitzoneSet], one per zone;
 * ObjAttach_DamageEm scales the hit by them. */
typedef struct EmHitzone {
    u8 unk0;            /* 0x0: not read by the damage code */
    u8 cut;             /* 0x1: a cutting hit's raw damage */
    u8 impact;          /* 0x2: an impact hit's raw damage */
    u8 shot;            /* 0x3: a shot's raw damage (ATKF_SHOT) */
    u8 fire;            /* 0x4 */
    u8 water;           /* 0x5 */
    u8 dragon;          /* 0x6 */
    u8 thunder;         /* 0x7 */
    u8 ice;             /* 0x8 */
    u8 stun;            /* 0x9: the stun buildup (Em.status5Hit) */
} EmHitzone;
typedef char EmHitzone_size_check[(sizeof(EmHitzone) == 0xA) ? 1 : -1];

typedef struct Em {
    u8 active;                  /* 0x000: nonzero while the slot is in use */
    u8 visible;                 /* 0x001: nonzero while the monster is in the field (drawn; cleared when the carcass goes) */
    u8 type;                    /* 0x002: monster kind: index into the per-kind tables */
    u8 subType;                 /* 0x003: a sub-kind: effect and stage objects typed Em index their per-kind count tables with it (DemonGlow_End, SkyStreak_End, KushalaBreath_End); Player_AmmoTypeChanged tests 1 */
    u8 taskPhase;               /* 0x004: GmEm_Task: 0 setup, 1 update, 2 finish, 3 free */
    u8 step;                    /* 0x005: step within the current state (0 on entry) */
    u8 stateVar0;               /* 0x006: per-state scratch byte (flag or counter: "once" flags with EmMotion_PassedFrameScaled, hit counts, table indexes); cleared with the step on every state change */
    u8 stateVar1;               /* 0x007: second per-state scratch byte like stateVar0 (flags, counters, 0xFF: none, a motion layer); cleared with the step on every state change */
    s32 timer;                  /* 0x008: counts down in many states (`if (--em->timer <= 0)`) */
    u16 index;                  /* 0x00C: slot number in the monster array (EmSlot_Alloc; o->idx) */
    u16 targetYaw;              /* 0x00E: heading to turn to, ANGLE_TURN per turn: EmTrader_TurnToYaw steps rotY to it; set from rotY, Vec_Heading(...) minus a quarter turn or an action spot angle (Player_StartSpotAction); the player code (Player_BowgunAim, Bow_UpdateAim) moves it by 576 steps */
    u8 isEm;                    /* 0x010: 1 for a monster (EmSlot_Alloc; as Player.isEm, 0 for a hunter) */
    u8 gender;                  /* 0x011: as Player.gender (same offset): Anteka picks it at random or from the variant (EmAnteka_Init) and uses other motions when set (EmAnteka_SetMotion); Kirin and Hermitaur Init clear it */
    u8 weaponOut;               /* 0x012: as Player.weaponOut (same offset): only player code typed Em uses it (the weapon states set 1, Player_State* 0) */
    u8 slotNo;                  /* 0x013: the slot number again, as a byte (EmSlot_Alloc) */
    u8 stateGroup;              /* 0x014: state group, set with the state (sub_main Player_ForceState (0x3AB7F0)) */
    u8 state;                   /* 0x015: current state: the family's state dispatcher switches on it */
    u8 prevStateGroup;          /* 0x016: the state group before the last change */
    u8 prevState;               /* 0x017: the state before the last change */
    u8 hitKind;                 /* 0x018: as Player.hitKind (same offset): only player code typed Em uses it (the weapons' CheckHit functions) */
    u8 hitPending;              /* 0x019: as Player.hitPending (same offset, same writer): set when one of its attacks hits (ObjAttach_DamagePlayer); the attack creators (PlayerAttack..BowgunShot_Create, EmImpactAttack_CreateAt) clear it */
    u8 unk01A[0x01B - 0x01A];
    u8 variant;                 /* 0x01B: the variant from the quest monster entry (QuestEm_Spawn): size row for Em_ComputeMaxHp and EmVespoid_Scales (Vespoid), row of EmKind1D_Variants (Drome kind 1D), Melynx 0xC/0x80, Hermitaur's shell (level % 3) */
    u8 unk01C[0x01E - 0x01C];
    u8 ownTask;                 /* 0x01E: 0: the game overlay runs the monster (GmEm_Setup clears it; the scene loop calls GmEm_Task); 1: another runner does (Em_SetupKind, the lobby NPCs) (Player.ownTask, ObjHead.ownTask) */
    u8 unk01F[0x020 - 0x01F];
    f32 rotMtx[4][4];           /* 0x020: rotation matrix built from rotX..rotZ (Mtx_FromAnglesYXZ) */
    f32 worldMtx[4][4];         /* 0x060: the world matrix: scale, rotation, position (Em_UpdateWorldMatrix, Em_Draw: Mtx_Copy into it); Vec_Rotate turns by it (EmTrader_JumpOut, Player_StateLedgeHang/15/1E/44); the effects typed Em keep a counter in its byte 1 (PlayerAttack_Update, BowgunShot_Fall) */
    s32 rotX;                   /* 0x0A0: rotation angles as integers, ANGLE_TURN per turn (rotX..rotZ are read as a vector: Mtx_FromAnglesYXZ) */
    s32 rotY;                   /* 0x0A4: heading: rotation about y, ANGLE_TURN per turn (Math_AngleXZ to the goal) */
    s32 rotZ;                   /* 0x0A8: rotation about z, ANGLE_TURN per turn */
    f32 posX;                   /* 0x0AC */
    f32 posY;                   /* 0x0B0 */
    f32 posZ;                   /* 0x0B4 */
    f32 scaleX;                 /* 0x0B8: scale (scaleX..scaleZ; EM_BASESCALE times the size) */
    f32 scaleY;                 /* 0x0BC */
    f32 scaleZ;                 /* 0x0C0 */
    u8 unk0C4[0x154 - 0x0C4];
    MotionLayer motion[4];      /* 0x154: motion layers (include/motion.h; as motionId[4]: Em_SetupKind sets the fourth one's step) */
    u16 yawCenter;              /* 0x294: the heading targetYaw is kept within a quarter turn either side of (Player_UpdateLean, PL_MOT_MOUNTED_GUN); Player_StateMountedGunAim sets it from the action spot angle (kind 6) or targetYaw */
    s16 tradeStep;              /* 0x296: step of EmTrader_RunTalk (switch), 0 when EmTrader_ShowMessage starts a message */
    u16 msgId;                  /* 0x298: the trader (EM_KIND_TRADER): the message shown, a row of gEmTraderMsgs (EmTrader_ShowMessage, EmTrader_RunTalk) */
    s16 msgTimer;               /* 0x29A: the trader (EM_KIND_TRADER): frames since msgId was shown (EmTrader_RunTalk counts to 999) */
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
    u8 modelSlot;               /* 0x30F: the scene's monster model slot (Quest_SpawnMonsters, QuestEm_Spawn: type = gScene.emKind[slot]); its textures are gTexHandles[slot * 20 + 163 ...] (GmEm_PrepareMeshSet, EmTail_Draw) */
    u8 unk310;                  /* 0x310: as Player.unk310 (same offset): Player_RunWeaponState clears it */
    u8 unk311[0x390 - 0x311];
    u8 stance;                  /* 0x390: 0: on the ground, 1: family-specific (Kushala kinds hovering, Lao-Shan, Shen Gaoren), 2: in the air (flying, leaping out of the sand), 3: dead (QuestEm_Hunted), 4: under the sand or water (Diablos, Cephadrome, Plesioth) */
    u8 unk391[0x395 - 0x391];
    u8 hitReceived;             /* 0x395: nonzero when the monster took a hit: Em_UpdateDamage handles the hit (or reports the death when 0 and hp <= 0), Player_ReactToHit reacts to it */
    u8 flinchPart;              /* 0x396: the part whose hp ran out on the last hit (the most damaged one); the families pick the flinch from it */
    u8 unk397[0x398 - 0x397];
    u32 flags;                  /* 0x398: as Player.flags: flag bits (Player_SetFlag/ClearFlag/TestFlag without bit 31; stateFlags at 0x39C with it); gm_sub's GmEm_Update sets 0x20000 (EM_FLAG_NO_ROOT_MOTION) */
    u8 unk39C[0x3A2 - 0x39C];
    u16 stateRand;              /* 0x3A2: Rand_Next(1) drawn when a state starts; states pick variants from it */
    s32 motionTime;             /* 0x3A4: as Player.motionTime (same offset): Player_TickTimers adds 2 a frame, wraps 100000 to 10000; states reset it to 0 and compare it (> 18, >= 240, == 300, == 600) */
    s32 targetAngle;            /* 0x3A8: angle to the target player minus a quarter turn (Em_FindTargetPlayer) */
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
    u8 petWaitBelow;            /* 0x3EB: lobby pets: Pet_StateFollow sets 1 while the player is in gNpc4FArea rising or above the pet (always for NPC_HOUSE_PET in AREA_HOUSE), and Pet_StateWait waits until the player comes down */
    u8 unk3EC[0x3EF - 0x3EC];
    u8 hitAttrs;                /* 0x3EF: as Player.hitAttrs (same offset): the attack attribute bits ORed in by ObjAttach_DamageEm; bit 0x80 makes Player_ReactToHit halve the damage */
    u16 hitAngle;               /* 0x3F0: direction of the last hit taken, ANGLE_TURN per turn (ObjAttach_SetHitHeading); states face it with rotY = hitAngle + ANGLE_TURN / 2 */
    s16 hitForce;               /* 0x3F2: the attack's hitForce at the last hit (ObjAttach_DamageEm; as Player.hitForce) */
    u16 lastAttackKind;         /* 0x3F4: as Player.lastAttackKind: the kind of the attack of the last hit (ObjAttach_DamageEm: attack object +0x63); Em_UpdateDamage and Em_PickStatus test ATTACK_WIND, ATTACK_TRAP_A, ATTACK_TRAP_B and ATTACK_HOLD */
    u8 unk3F6[0x3F8 - 0x3F6];
    u8 lookAtTarget;            /* 0x3F8: nonzero: Em_UpdateBendTarget bends the body toward the target player (toward lookPoint when lookAtPoint is set), else straight; 1 while Em_TurnToTargetMotion and some group-3 states run, cleared on entry of almost every state */
    u8 hitIds[16];              /* 0x3F9: the ids of the attacks that hit it last (a ring, hitIdNext the next slot; ObjAttach skips an attack whose id is in it) */
    s8 side;                    /* 0x409: as Player.side (same offset): copied into the attacks (Eff*_Create o->unk0D1), Side_Match against players (EmScript_PickNoticedPlayerReachable, GmEm_Setup), versus team (Versus_Team: size and rage scales); the carving player is gPlayers[side - 1] (State8_00) */
    u8 hitStopAdd;              /* 0x40A: as Player.hitStopAdd (same offset): only player code typed Em uses it (Lance, Gunlance, Bow CheckHit) */
    u8 hitStop;                 /* 0x40B: as Player.hitStop (same offset): only player code typed Em uses it */
    u8 unk40C[0x40E - 0x40C];
    s16 noHitTimer;             /* 0x40E: frames the monster cannot be hit: ObjAttach_HitTarget skips it while nonzero; counted down by Player_TickTimers (GmEm_Update); states set 5/10/2, GmEm_Update 2 in group 8 (carving); Em_CheckFlash ignores flashes while > 0; players: Player_SetEvadeFrames */
    s16 noPushTimer;            /* 0x410: frames without body pushing: Field_PushApart skips the object while nonzero; counted down by Player_TickTimers; states set 5/2/10 */
    u8 hitIdNext;               /* 0x412: the next slot of hitIds, 0..15 (cleared by EmSlot_Alloc) */
    u8 unk413[0x414 - 0x413];
    u8 hitPhase;                /* 0x414: as ObjHead.hitPhase (Player.hitPhase): the hit window; cleared by Obj_ClearHitMarks and at the set-up */
    u8 unk415[0x416 - 0x415];
    u8 unk416;                  /* 0x416 */
    u8 unk417;                  /* 0x417 */
    s16 rowBlend;               /* 0x418: blend frames of the motion row (+7 of the row: Em_SetMotion, Em_SetMotionRow store it); Em_SetMotion uses it instead of the given blend when nonzero */
    u8 unk41A[0x436 - 0x41A];
    u8 unk436;                  /* 0x436: Quest_SpawnMonsters sets 1 for its sixth spawn (j == 5), 0 for the other model-slot-3 ones */
    u8 fieldCollide;            /* 0x437: 1: Em_CollideField collides the monster with the walls (spheres per kind) and fills wallHitDirs; 0 in hunted, captured and some entrance states; 1 from Em_SetupKind, GmEm_ResetState, the EndState functions */
    u8 work;                    /* 0x438: first byte of the state functions' work area (EmWork, em_work.spec); the families also read its bytes as Em fields */
    s8 fortStage;               /* 0x439: the fort monsters: the hits on the fort (their views' fortHits), copied to gScene.fortStage; sent in message 8 (QSyncEmData state1, QSync_PackEmState); EmWork byte 1 */
    s16 syncTimer;              /* 0x43A: the fort monsters (their views' syncTimer, work +2): frames to the next state message; QSync_PackEmState restarts it at 150 when it sends one */
    u8 fullCloak;               /* 0x43C: Chameleos: 1 allows the full cloak (EmChameleos_WantsInvisible then answers 2, invisible with a shimmer every 900 frames), 0 only the shimmering one; set by CHAM_IDLE_CLOAK_FULL, cleared by CHAM_IDLE_CLOAK_SHIMMER, the decloak reactions, the defeat and the leaving; work byte 4 (EmChameleosWork); EmKind1D_Update reads this byte and the next as a halfword */
    u8 unk43D;                  /* 0x43D */
    s16 unk43E;                 /* 0x43E */
    u8 unk440;                  /* 0x440: the work area's word +8 (EmWork.accelAngX, a number; the townspeople's NpcWork.name, an address): left bytes */
    u8 minePoint;               /* 0x441: Gypceros: the mine point it leaves (MinePoint_AddUntimed; EmKutku_AddMinePoint) */
    s16 unk442;                 /* 0x442 */
    s16 unk444;                 /* 0x444: the work area's word +0xC (EmWork.accelAngY, a number; NpcWork.route, an address) */
    s16 unk446;                 /* 0x446 */
    s16 unk448;                 /* 0x448 */
    u8 unk44A;                  /* 0x44A */
    u8 unk44B;                  /* 0x44B */
    s32 unk44C;                 /* 0x44C */
    u8 unk450;                  /* 0x450 */
    u8 unk451;                  /* 0x451 */
    u8 unk452;                  /* 0x452 */
    u8 rageFade;                /* 0x453: work + 0x1B: the Diablos family's rage fade (EmDiablosWork.rageFade: counts up to 60 while enraged, down to 0 otherwise); GmEm_UploadMaterial tints the Monoblos kinds' material 0 red by it (read as s8, / 60). Gravios's code keeps another byte there (EmGravios_PlaySeOnFrame tests 1) */
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
    u8 npcLook;                 /* 0x483: lobby NPCs (an Em): the townsperson's look, NpcWork.look (Npc_IsFree, Npc_IsService, Npc_IsPet) */
    u16 invisFade;              /* 0x484: work + 0x4C: Chameleos's invisibility fade, 0xFFFF when none (EmChameleos_IsInvisible; GmEm_ChameleosMeshUv shifts the meshes' U by 1 - n / 32) */
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
    u16 statusIcons;            /* 0x4B8: as Player.statusIcons: StageObj_K05_* and DemonGlow_Run read bit 0x8000 of the player they are given */
    u8 unk4BA[0x4BC - 0x4BA];
    struct ObjAttach *hitAttach;/* 0x4BC: the attack object of the last hit taken (ObjAttach_DamageEm; as Player.hitAttach) */
    u8 unk4C0[0x4C8 - 0x4C0];
    u8 meshShown[32];           /* 0x4C8: one per model mesh, 1: drawn (in the EmWork area: work + 0x90): Em_SetupKind fills it from the variant tables gEmVariantShownMeshes/gEmVariantMeshes, GmEm_Setup sets all 32, Em_UpdateState cycles the variants; Diablos/Garuga/Gravios/Rathalos State4_04 clear element 1 at frame 6 */
    s16 modelCount;             /* 0x4E8: model slots used (Model_Alloc; EmSlot_SetupModel, EmSlot_Free) */
    s16 modelFirst;             /* 0x4EA: the first model slot */
    struct Model *model;        /* 0x4EC: skeleton: bones and their matrices */
    u8 unk4F0[0x52C - 0x4F0];
    struct DrawSlot *drawSlot;  /* 0x52C: its draw slot (DrawSlot_Get), positioned at the Em every frame and put into an ordering table (gm_sub's GmEm_Task, the lobby's townspeople) */
    s16 drawSlotId;             /* 0x530: an entry of the 0x6A5AD0 pool, -1: none (DrawSlot_Alloc; gm_sub's GmEm_SetupKind) */
    u8 painted;                 /* 0x532: 1 while marked by a paint hit: ObjAttach_DamageEm sets it (attack kind 7) with paintTimer = 18000 frames; Player_TickPaint counts paintTimer down and clears it (0xFF: never); Map_MonsterLocated shows the monster while set; synced online (QSync flag 0x10) */
    u8 carry;                   /* 0x533: as Player.carry (same offset): only player code typed Em uses it */
    u8 ammoType;                /* 0x534: as Player.ammoType (same offset): only player code typed Em uses it (Player_AmmoTypeChanged) */
    u8 savedAmmoType;           /* 0x535: as Player.savedAmmoType (same offset): only player code typed Em uses it */
    u8 ammoMenuOpen;            /* 0x536: as Player.ammoMenuOpen (same offset): only player code typed Em uses it */
    u8 unk537[0x53A - 0x537];
    s16 paintTimer;             /* 0x53A: frames the paint mark lasts: 18000 (ObjAttach_DamageEm, QSync_OnEm), counted down while painted is set, which clears at 0 */
    u8 unk53C[0x564 - 0x53C];
    u8 collideFlags;            /* 0x564: Em_CollideField clears bit 0 first */
    u8 unk565[0x568 - 0x565];
    f32 prevPosX;               /* 0x568: position before this frame's move (prevPosX..prevPosZ, read as a vector: Vec_Copy): GmEm_Update and Em_UpdateState store posX..posZ, Player_Update the player position; Em_CollideField sweeps the wall collision from it; Em_HoldPosNoPlayers puts the monster back there */
    f32 prevPosY;               /* 0x56C: y of the position before this frame's move (see prevPosX) */
    f32 prevPosZ;               /* 0x570: z of the position before this frame's move (see prevPosX) */
    f32 floorY;                 /* 0x574: height of the floor under the monster (Em_GetFloorY, Em_GetFloorInfoEx) */
    u8 unk578[0x584 - 0x578];
    f32 bodyScale;              /* 0x584: scale of the wall collision shapes (Em_CollideField) and of the carcass mine radius (EmMine_Create); gEmBodyScales[type] (GmEm_ResetState), the scale for the small kinds (Vespoid, Anteka, Apceros, Drome kind 1D Init) */
    u8 rideState;               /* 0x588: as Player.rideState (same offset): Em_GetFloorInfo tests it; the player states typed Em set it */
    u8 unk589[0x590 - 0x589];
    u16 turnTarget;             /* 0x590: the angle Em_TurnToward turns to (Em_TurnToward's view of the monster) */
    u16 turnHeading;            /* 0x592: the current heading of Em_TurnToward (EmTurnView) */
    u8 unk594[0x596 - 0x594];
    u8 pushPriority;            /* 0x596: as Player.pushPriority (same offset): compared between two bodies (PlayerEm_PushApart, EmEm_PushApart); Player_UpdateNearMonster reads the monster's */
    u8 unk597[0x59A - 0x597];
    u8 cartPlayer;              /* 0x59A: the Felyne cart (EM_KIND_FELYNE_CART): the player it carries (Player_StateCart sets it; EmFelyneCart_CarryPlayer runs while that player is in PL_GROUP_EVENT) */
    s8 targetNo;                /* 0x59B: index in gPlayers of the target player, -1: none */
    u8 unk59C[0x628 - 0x59C];
    s8 sceneSeat;               /* 0x628: lobby townspeople in a scene (NpcScene_RunNpc): a kitchen helper's seat in the house's meal, the bit of gNpcScene.seatMask it takes (-1 none): its point on the route set, and what the serving state passes to ThrownObj_Spawn */
    u8 unk629[0x634 - 0x629];
    s16 unk634;                 /* 0x634 */
    u16 msgItem;                /* 0x636: the item EmTrader_FormatMessage prints in its message (gItemNames), 0 at EmTrader_Init */
    void *sightTable;           /* 0x638: the kind's sight and sense ranges (an EmSightRange: Em_SeePlayers; f32 [6] the action reach of Em_NearestActionToBone, [7] the sense reach of Em_SensePlayers) */
    u32 bendAngle[4];           /* 0x63C: the four bend angles Em_BendStep turns toward an angle (|angle| / 2, / 4, / 8, / 16, each capped by the kind's gEmBendOrigins row; EmBendView); GmEm_ResetState clears them */
    u16 turnSpeed;              /* 0x64C: Em_TurnToward's speed (EmTurnView) */
    u16 turnHold;               /* 0x64E: EmTurnView: hold frames of Em_TurnToward */
    u8 turnPhase;               /* 0x650: EmTurnView: the phase of Em_TurnToward */
    u8 lookAtPoint;             /* 0x651: 1: Em_UpdateBendTarget bends toward lookPoint instead of the target player (Rathalos State3_0C) */
    u8 motionRemap;             /* 0x652: nonzero: Em_SetMotion takes the row's remap row of the alternate table once, then clears it */
    u8 stateRequest;            /* 0x653: as Player.stateRequest: 1 after Em_SetState (Player_ForceState): the family Update runs the new state a second time in the same frame, then clears it */
    f32 lookPoint[3];           /* 0x654: f32[3]: the point bent toward when lookAtPoint is set (Em_NearestActionToBone with bone 35; Rathalos State3_0C) */
    u8 floorKind;               /* 0x660: the floor triangle under the monster (TriHdr): Em_GetFloorInfo and Em_GetFloorInfoEx take 0x660..0x663 as their info: kind (TriHdr.kind) */
    u8 floorArg;                /* 0x661: the floor triangle's sound argument (TriHdr.seArg; Player.floorArg goes with the voice sounds) */
    u16 floorAttr;              /* 0x662: the u16 at +2 of the floor triangle (TriHdr.attr, AREA_TRI_ATTR_*, Em_GetFloorInfo): EmScript_PickNoticedPlayerSameFloor and Op62 test 1..3, EmBlangonga_ScriptQuery 1, Em_RaisePlayerAreaTriggerLax nonzero without bit 0x8000; GmEm_ResetState sets 1 (Player +0x662: compared by EmScript_IfScriptPlayerFloorAttr) */
    void *lightColors;          /* 0x664: as ObjHead.lightColors: the monster's own colours of lights 0 and 1 (Em_UpdateLightColors, for kind 9 and EM_KIND_MELYNX); NULL: the area's */
    u8 unk668;                  /* 0x668: at Player.mustCrouch's offset: Player_StateCrouchWalk/3D test it with the dodge button */
    u8 unk669[0x680 - 0x669];
    u16 area;                   /* 0x680: the area (map zone) the monster is in (gScene.unk014 is the current one). Capcom: my_stage_no (mhfo.dll's trace in EmScript_IfInArea) */
    u8 areaMoveState;           /* 0x682: as Player.areaMoveState (same offset): only player code typed Em uses it */
    u8 unk683[0x684 - 0x683];
    u16 nextArea;               /* 0x684: the area the monster moves to (goalKind 3; GmEm and Em_PickRoutePoint move it there) */
    u8 unk686[0x688 - 0x686];
    f32 startPos[3];            /* 0x688: where the monster starts (as Player.startPos): EmTrader_Init copies posX..posZ */
    u8 unk694[0x698 - 0x694];
    s32 wallHitDirs;            /* 0x698: directions of the walls touched this frame, one bit per 1/32 turn relative to the heading (Em_CollideField: 1 << ((h - rotY) >> 11)); states test 0xF000000F (ahead) to stop a charge */
    u16 unk69C;                 /* 0x69C: as Player.unk69C (same offset): Player_UpdateAutoTurn's turn angle (moved by 768 a frame, kept in 2560..62976) */
    u8 unk69E[0x6A0 - 0x69E];
    f32 mapPos[3];              /* 0x6A0: as Player.mapPos: the position for the map (Map draws the icons from it) */
    u8 unk6AC[0x6AE - 0x6AC];
    u8 hitReactMode;            /* 0x6AE: how Em_UpdateDamage reacts to a hit: 0 normal flinch, 2 no flinch (reaction 10; Em_BeginStatus1/2, captured states set it every frame through Em_SetHitReactMode), 3 reaction 13 (EmFatalis_KnockDown), 4 kept through the hit (Em_BeginStatus4/5); Em_ClearHitReactMode resets it each frame; EmScript_SetHitReactMode sets, Op32/Op33 test it */
    u8 unk6AF[0x6B0 - 0x6AF];
    u8 aimMode;                 /* 0x6B0: as Player.aimMode (same offset): only player code typed Em uses it */
    u8 unk_InitFlag;            /* 0x6B1: every family Init (or Reset) sets 1 (Cephadrome and Yama Tsukami also 0); no reader found in C */
    s16 hitPartDamage[9];       /* 0x6B2: the hit's damage per part (Em_UpdateDamage's view of the monster): Em_PickDamagedPart and Em_UpdateDamage scale it by damageFactor into parts[i].hp; [8] takes the damage of the part partFlags names */
    s16 reactValue[4];          /* 0x6C4: one per player: Em_UpdateReactions runs the reaction of a player whose value changed (EmReactView) */
    s16 reactValueLast[4];      /* 0x6CC: the last seen copies of reactValue (EmReactView) */
    u8 partFlags[9];            /* 0x6D4: one per part (as parts[k]): bit 2 lets the part break (EmBlangonga_BreakParts, Velociprey), 0x20 (Kushala BreakParts below 60% hp) */
    u8 unk6DD[0x6DE - 0x6DD];
    s16 hpMax;                  /* 0x6DE */
    u8 syncKind;                /* 0x6E0: kind of the last synced state change (Em_SyncState, large monsters), sent online (QSync_SendEm msg 22, QSync_QueueEmState) */
    u8 syncArg;                 /* 0x6E1: second argument of the last synced state change (Em_SyncState), sent online (msg 23) */
    u8 unk6E2[0x6E3 - 0x6E2];
    u8 syncGoalKind;            /* 0x6E3: curGoalKind at the last synced state change (Em_SyncState), sent online */
    f32 alpha;                  /* 0x6E4: draw opacity, 1.0: opaque (fades in and out; GmEm_Task sorts by it) */
    u8 unk6E8[0x6EC - 0x6E8];
    struct Player *contact;     /* 0x6EC: the player of the last contact: the one it hit or that hit it (ObjAttach_DamagePlayer, with the attack at +0x6F0); Kut-Ku and Chameleos steal from it; in the lobby the player talking to the townsperson */
    struct ObjAttach *contactAttach;/* 0x6F0: the attack of that contact (ObjAttach_DamagePlayer); Kut-Ku State3_0B tests its kind for ATTACH_KUTKU_ATTACK */
    u8 syncGoalArg;             /* 0x6F4: curGoalArg at the last synced state change (Em_SyncState), sent online */
    u8 syncGoalNo;              /* 0x6F5: curGoalNo at the last synced state change (Em_SyncState), sent online */
    s16 status5Buildup;         /* 0x6F6: raised by Em_AddStatus5; reaching status5Threshold sets statusFlags 0x10 */
    s16 status5Decay;           /* 0x6F8: frames to the next decay of status5Buildup */
    s16 status5Hit;             /* 0x6FA: the hit's status 5 amount: Em_PickDamagedPart passes it to Em_AddStatus5 (EmDamageView) */
    s16 status2Threshold;       /* 0x6FC */
    s16 status2Buildup;         /* 0x6FE: raised by Em_AddStatus2; reaching status2Threshold sets statusFlags 2 */
    s16 status2Hit;             /* 0x700: the hit's status 2 amount (Em_AddStatus2; EmDamageView) */
    s16 status2Decay;           /* 0x702: frames to the next decay of status2Buildup */
    s16 poisonThreshold;        /* 0x704: from gEmStatusParams (per kind); poisonBuildup reaching it poisons the monster */
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
    u8 hideShadow;              /* 0x722: nonzero: Shadow_Draw (the flat model drawn under the object, probably the shadow) skips it; GmEm_Update sets it from unk723 or stance 4 (under sand or water) unless hideShadowHold holds the value states set (2); counted down by Player_TickTimers */
    u8 shadowOff;               /* 0x723: nonzero: GmEm_Update sets hideShadow (as under the sand, stance 4) unless hideShadowHold runs */
    f32 attackFactor;           /* 0x724: as Player.attackFactor (same offset): the attack factor ObjAttach_DamagePlayer reads; the kind's rage row +8 times Em_LevelAttackFactor while enraged, Em_LevelAttackFactor (gEmScaleRows row, versus team scale) otherwise */
    f32 damageFactor;           /* 0x728: multiplies the part damage the monster takes (Em_UpdateDamage, Em_PickDamagedPart); the rage row +0xC times Em_LevelDamageFactor while enraged, Em_LevelDamageFactor otherwise (Player has defenseFactor at this offset) */
    s32 surfaceDepth;           /* 0x72C: an f32 kept in an s32 (read through casts): how far below surfaceY a swimmer stays */
    f32 surfaceY;               /* 0x730: height of the special floor found by Em_GetFloorInfoEx (water or sand); swimmers keep posY at surfaceY - surfaceDepth */
    u8 floorUnderSurface;       /* 0x734: 1 when Em_GetFloorInfoEx found the floor below the surface depth (on: low && floor < depth), else 0; Plesioth (EmPlesioth_OverDeepWater) tests it */
    u8 surfaceKind;             /* 0x735: nonzero when the floor under the monster is a special surface (byte 6 of the floor material row gAreaKindFlags[area][kind]: sand or water; Em_GetFloorY, Em_GetFloorInfoEx); GmEm_Update then puts floorY at surfaceY for most kinds; Diablos and Cephadrome dive only where it is set */
    s16 wallTouch;              /* 0x736: 2 while Em.wallHitDirs is set, else 0 (GmEm_Update); Field_PushApart pushes two monsters this console runs apart only while neither has it set (read unsigned) */
    u8 pushed;                  /* 0x738: set on the monsters and players the push-apart moved (Field_PushApart and its pair pushes); Field_UpdateAll collides those with the field again, then clears it */
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
    u8 contactHit;              /* 0x74C: 1 once one of its attacks with ATKF_REPORT_CONTACT hits (ObjAttach_DamagePlayer); the attack creators clear it; Melynx steal states (State1_04, State3_01) and Cephadrome test it with contact (as Player.contactHit, the same offset) */
    u8 unk74D[0x754 - 0x74D];
    u8 ***script;               /* 0x754: the monster's script set (GmLoad_GetEmData): tables of script pointers by kind: [0] the main scripts (scriptNo, Em_StartScript), [1] the subscripts (EmScript_CallScript), [2] to [14] the loop, trigger and area tables (the script ops), [15 + k] EmScript_JumpEntry's jump tables */
    u8 *mainScriptPos;          /* 0x758: the main script's position (Em_StartScript's result; EmScript_GetPos/SetPos with no scriptFlags bit 0x01, 0x10, 0x20) */
    u8 *scriptCallReturn;       /* 0x75C: the script position after an EmScript_CallScript call (Op81 saves p + 1) */
    u8 *scriptJumpReturn;       /* 0x760: the script position EmScript_JumpEntry saves before its jump */
    f32 reactDist[2];           /* 0x764: the reaction set's distance limits 0 and 1 (Em_LoadReactionSet, script command EMOP_SWITCH_TARGET_DIST); limit 2 is nearRadius, limit 3 at +0x770 lies under the Player fields inBattle, prevInBattle */
    f32 nearRadius;             /* 0x76C: Velocidrome and Velociprey: a radius: the player distance (playerDist[curGoalNo]) is compared with it, and EmList_FindKindNear looks for its kind within it */
    u8 reactDist3Lo[2];         /* 0x770: the low half of the reaction set's fourth distance limit: Em_LoadReactionSet writes it as an f32 at +0x770, over these two bytes and inBattle, prevInBattle */
    u8 inBattle;                /* 0x772: as Player.inBattle (same offset): Player_EnteredBattle compares it with prevInBattle */
    u8 prevInBattle;            /* 0x773: as Player.prevInBattle (same offset) */
    u8 scriptNo;                /* 0x774: the script to run next (EmScript_Start) */
    u8 scriptCallNo;            /* 0x775: the subscript EmScript_CallScript calls (script[1][scriptCallNo]); EmScript_Init clears it */
    u8 scriptCallEntry;         /* 0x776: entry of the EmScript_JumpEntry jump (script table k + 15); em_bulldrome.c reads 0x776..0x777 as one s16 */
    u8 scriptCallEntryHi;       /* 0x777: cleared when the Em is set up (gm_em.c); the high byte of em_bulldrome.c s16 read */
    u8 scriptCallDepth;         /* 0x778: cleared when a script starts; EmScript_CallScript sets 1 (saving the return at unk758 + 4 from 0), EmScript_JumpEntry sets 2 (saving it at unk758 + 8 from 1) */
    u8 goalKind;                /* 0x779: what the goal is (Em_ComputeGoal): 1 a player, 2 a point of the area, 3 another area, 5 the kind's point, 6 relative to the heading, 8 objectPos, 9 a route point. Capcom: T_KIND, target kind: 3 is T_KIND_AREA (EmScript_GoalDestArea), 9 T_KIND_AREA_ROUTE (EmScript_End); likely, by the order of mhfo.dll's name table, 0-13 are KIND_NOT, PLY, GND, ARA, ITM, DEM, MYP, SML, YOB, ART, APL, LND, DMG, EMY */
    s16 goalArg;                /* 0x77A: first argument of goalKind (table, route or player) */
    s16 goalNo;                 /* 0x77C: second argument of goalKind (point or player index) */
    u8 scriptCaseCount;         /* 0x77E: EmScript_SwitchTargetDist: the number of cases it skips through */
    u8 routedStateArg;          /* 0x77F: third argument of script command 5 (Em_ChangeStateRouted, EmScript_Run); Em_TargetPlayerInRange switches on it */
    u8 *loopScriptPos;          /* 0x780: the position of the loop script EmScript_StartLoop starts (scriptFlags 0x01; EmScript_GetPos/SetPos) */
    u8 loopMode;                /* 0x784: the script loop kind (EmScript_StartLoop byte 0: 0, 1, 2 loop over the area points; Op10 and OpFF switch on it) */
    u8 loopPointTable;          /* 0x785: the point list of the loop (EmScript_StartLoop; EmScript_LoopPointList, read by Op10 and OpFF) */
    u8 loopScript;              /* 0x786: the script table entry run at each loop point (EmScript_StartLoop; EmScript_LoopScript) */
    u8 loopEndScript;           /* 0x787: the script table entry run when the loop ends (EmScript_StartLoop; EmScript_End, EmScript_LoopScript). Capcom: cmd_route_act, passed to route_ptr_set, our EmScript_LoopScript (mhfo.dll's EmScript_End, 0xFD) */
    u8 *trigger10Pos;           /* 0x788: the script of trigger EM_TRIGGER_PART_BUILDUP (EmScript_PickTrigger; scriptFlags 0x10) */
    u8 scriptCombat;            /* 0x78C: combat when the EmScript_IfCombatIs block started (Op0B, OpFF after Em_SetCombat) */
    u8 scriptPending;           /* 0x78D: 1: run the script once (Em_UpdatePendingScript calls EmScript_Run when scriptDelay is down to 0, then clears it); set by Init and the EndState functions, 0 by ChangeState and captured/hunted states */
    u8 scriptVar;               /* 0x78E: a script value: EmScript_SetVar sets it, Op0D clears it, Op2B compares it */
    u8 scriptFlags;             /* 0x78F: the running script thread: 0x20, 0x10 trigger scripts, 1 a loop (EmScript_StartLoop); EmScript_GetPos picks the resume position by it */
    u8 *trigger20Pos;           /* 0x790: the script of trigger EM_TRIGGER_NOTICED (EmScript_PickTrigger; scriptFlags 0x20) */
    u8 *subScriptReturn;        /* 0x794: the position after an EmScript_Call (it saves p + 1; EmScript_End command 3 returns there) */
    u8 scriptPlayer;            /* 0x798: the player the script picked (s8, -1: none; low nibble a gPlayers index): EmScript_IfLonePlayerBait/Op52/Op53/Op5F pick it, Op46 and Op49 use it, Em_ComputeGoal makes it the goal in combat. Capcom: cmd_pl_target, and -1 is NOT_FOUND_PL (mhfo.dll's EmScript_End, 0xF7) */
    u8 subScriptNo;             /* 0x799: the subscript EmScript_Call runs (EmScript_SubScript) */
    u8 areaSeqTable;            /* 0x79A: the area list of the area sequence (EmScript_SetAreaSeq byte 0; Op18 reads table 14 at areaSeqPos) */
    u8 areaSeqScript;           /* 0x79B: the script table entry of the area sequence (EmScript_SetAreaSeq; Op18, EmScript_AreaSeqScript) */
    u8 *areaSeqNext;            /* 0x79C: the script position EmScript_StepAreaSeq saves */
    u8 areaSeqMode;             /* 0x7A0: EmScript_SetAreaSeq byte 1: Op18 and OpFF test it for 0 */
    u8 areaSeqActive;           /* 0x7A1: 1 while the area sequence of EmScript_SetAreaSeq/Op18 runs (Op18 sets it, OpFF clears it at the end); the SetGroup2State functions then aim the takeoff and landing at point 0 of table 3 */
    u8 scriptRestart;           /* 0x7A2: 1: EmScript_Restart restarts the script (clears scriptPending, scriptFlags); set by Em_SetRequest, Em_RestartScript, Em_RaisePlayerAreaTriggerLax/Em_RaisePlayerAreaTrigger (with trigger EM_TRIGGER_PLAYER_AREA); EmScript_Init clears it */
    s8 repeatCount;             /* 0x7A3: the repeat count of EmScript_Repeat; EmScript_StopRepeat clears it */
    u8 unk7A4[0x7A8 - 0x7A4];
    s32 unk7A8;                 /* 0x7A8: only cleared: by EmScript_Init and EmScript_PickTrigger */
    u8 *repeatPos;              /* 0x7AC: the command a repeat returns to (EmScript_Repeat) */
    u8 *trigger02Pos;           /* 0x7B0: the script of trigger 2 (EmScript_PickTrigger; scriptFlags 2) */
    u8 *trigger04Pos;           /* 0x7B4: the script of trigger 4 (EmScript_PickTrigger; scriptFlags 4) */
    u8 *trigger08Pos;           /* 0x7B8: the script of trigger 8 (EmScript_PickTrigger; scriptFlags 8) */
    u8 *routeResume;            /* 0x7BC: the script position after the route command (EmScript_StartRoute); EmScript_End resumes there at the end of the route and clears it */
    u8 routePointScript;        /* 0x7C0: the current route point's script table entry (point +0x12; EmScript_RoutePointScript picks the commands to run there: EmScript_StartRoute, EmScript_End) */
    u8 routeNo;                 /* 0x7C1: route index (EmScript_StartRoute; goalArg of the route goal, Em_ComputeGoal copies curGoalArg back); EmScript_End walks it */
    u8 routePoint;              /* 0x7C2: point index in the route (EmScript_StartRoute, EmScript_End steps it down to 0; goalNo of the route goal) */
    u8 syncRequest;             /* 0x7C3: a request to send the monster state (QSync_SendEm sends it and clears it): 1 by Em_RequestSync, 2 by it or EmScript_End unless 1 is pending; Em_RestartScript runs only when 0 on the host. Capcom: setting 2 is em_g_init_flag_set(em,EM_NET_CMD) (mhfo.dll's EmScript_End, 0xFD) */
    u8 *scriptPos;              /* 0x7C4: the current script command (Em_StartScript) */
    s16 scriptOffset;           /* 0x7C8: EmScript_Run: the start of the script minus scriptPos when it stops */
    u8 mineBone;                /* 0x7CA: bone the carcass mine point follows, 0: the position (EmMine_Create) */
    u8 inSceneArea;             /* 0x7CB: 1 while the monster is in the scene's current area (GmEm_Update sets and clears it) */
    struct StageObj *tailObj;   /* 0x7CC: its severable tail, a stage object of STAGEOBJ_KIND_EM_TAIL (EmTail_Create); Em_ResetToBasePose places it (EmTail_Drop), GmEm_ResetState clears it */
    u8 hookedBy;                /* 0x7D0: the player whose bait Plesioth took (targetNo; EmPlesioth_WaitForBait, EmPlesioth_HookedPulled), compared with the player index in Player_StateFishReel */
    u8 hitPartMask;             /* 0x7D1: parts whose hit effects this hit spawned, one bit each (Em_UpdateDamage clears it, sets bits, passes it to Em_SpawnPartEffects) */
    u8 unk7D2[0x7D3 - 0x7D2];
    s8 quakeTimer;              /* 0x7D3: 5 while a giant shakes the ground, counted down in its Update: set by Lao-Shan's roar (LAOSHAN_IDLE_ROAR), and by the Shen Gaoren and Yama Tsukami Update outside their quiet states; Player_IdleActions throws a player riding the giant off (PL_REACT_TOSSED_BACK) while it is set */
    s16 status5Threshold;       /* 0x7D4 */
    u8 turnStep;                /* 0x7D6: step of Em_TurnToTargetMotion (Em_SetTargetAngle clears it) */
    u8 status5Count;            /* 0x7D7 */
    s8 syncPulse[2];            /* 0x7D8: set to 2 by EmScript_GoalAlertedPlayer ([0]) and Op12 ([1]), or by the sync message bits 4 and 8 (QSync_OnEm); GmEm_Update counts them down; QSync_SendEm sends them while nonzero. Player code reads Player.chargeLevel/fishTimer here */
    s16 combatTimer;            /* 0x7DA: frames of combat left without a seen player (Em_UpdateCombatTimer counts it down; at 0 the monster calms down or leaves) */
    u8 combat;                  /* 0x7DC: 1 while the monster fights the hunters (Em_SetCombat); back to 0 it forgets them (seen bits, alertBuildup, unk86C, unk7E4 cleared). Capcom: the "mode" Em_Mode_Chg (our Em_SetCombat) sets; 1 is EM_MODE_ATTACK (mhfo.dll's EmScript_End, 0xFC) */
    u8 requested;               /* 0x7DD: 1 while a request (Em_SetRequest) is pending: the families' state choice leaves it alone. Capcom: the "mind" em_cmd_mind_ck tests (EmScript_IfRequestedIs; its trace calls 1 ESC, other values NML) */
    u8 requestId;               /* 0x7DE: the pending request: an id of the kind's gEmKindInfo entry (Em_PickRequest) */
    u8 blind;                   /* 0x7DF: nonzero: Em_SeePlayers sees nobody; counted down each frame (GmEm_Update), many states set 2 every frame */
    u8 seen;                    /* 0x7E0: players in sight this frame, one bit each (Em_SeePlayers); Em_UpdateNoticed makes them noticed */
    s8 mineSlot;                /* 0x7E1: mine point on the carcass (EmMine_Create), -1 when none */
    u8 hostNo;                  /* 0x7E2: the player whose console runs the monster (online; GmEm_Setup, Em_MigrateHost) */
    u8 noticed;                 /* 0x7E3: players the monster has noticed, one bit each (Em_AddNotice, Em_UpdateNoticed) */
    s16 noticeTimer[4];         /* 0x7E4: one per player: frames the player stays noticed (from Em_NoticeTime) */
    s32 need2Gauge;             /* 0x7EC: gauge of need 2 (request gEmKindInfo.need2Request): Em_DecayNeed2 lowers it (gEmDecay7ECRates rates, out of and in combat) and sets need2 at 0; Kut-Ku State0_0E refills it by 222 a frame up to need2Max; EmScript_IfNeed2Low compares it with 30% of the maximum; quest entry +0x10 */
    s32 need1Gauge;             /* 0x7F0: gauge of need 1 (named after gEmKindInfo.need1Request, the request it makes): Em_DecayNeed1 lowers it by the kind rate (gEmRateTbl) and sets need1 at 0; refilled to need1Max by eating-like states (Kut-Ku State0_1A, with +5% hp); Em_FindNearAreaObj looks for an action spot under the kind threshold; from the quest entry (+0xC) */
    s32 need3Gauge;             /* 0x7F4: gauge of need 3 (request gEmKindInfo.need3Request): Em_DecayNeed3 lowers it and sets need3 at 0; refilled to need3Max by Em_RefillNeed3, Em_BeginStatus1/2, Em_ResetNotice; quest entry +0x14 */
    s32 need2Max;               /* 0x7F8: maximum of need2Gauge (need2Gauge): Em_AddNeed2 caps at it */
    s32 need1Max;               /* 0x7FC: maximum of need1Gauge (need1Gauge): Em_AddNeed1 caps at it, refills set it; Em_CheckRageByHp takes the percentage of it (EM_KIND_CONGALALA) */
    s32 need3Max;               /* 0x800: maximum of need3Gauge (need3Gauge): Em_AddNeed3 caps at it, the refills set it */
    s16 rageThreshold;          /* 0x804: the kind's EmRageRow threshold (gEmRageTbl[type]), -1 without one (Em_InitRage); Em_CheckRageByHp raises rageBuildup to it; the families pass it to Em_CheckRageByHp */
    s16 rageBuildup;            /* 0x806: raised by Em_CheckRageByHp (damage times a factor picked by the hp percent, doubled for some kinds with part 0 broken); reaching rageThreshold sets ragePending; Em_InitRage, Em_EndRage clear it */
    s16 rageTimer;              /* 0x808: frames left of the rage, counted down by GmEm_Update (Hermitaur sets 1800) */
    u8 rage;                    /* 0x80A: nonzero while enraged: the kind's rage motion speed (GmEm_Update), Blangonga tints red (EmBlangonga_UpdateTint); Em_EndRage ends it */
    u8 rageStep;                /* 0x80B: step of the rage timers (EmBlangonga_UpdateRageTimer, EmFatalis_UpdateRageTimer, EmDiablos_Update switch on it and step it); GmEm_PrepareMeshSet draws by it; Em_InitRage, GmEm_ResetState clear it */
    u8 ragePending;             /* 0x80C: 1 when rageBuildup reached rageThreshold (Em_CheckRageByHp); GmEm_Update then starts the rage (Em_StartRage2) outside group 4 and noReact; cleared by Em_StartRage2, Em_EndRage */
    u8 hooked;                  /* 0x80D: 1 while Plesioth has taken the bait of a player in PL_ACT_FISH_REEL (the line: Lure_PlayerHasMonsterBait): EmPlesioth_WaitForBait sets it with hookedBy = targetNo; Player_StateFishReel finds the monster on its line by it; EmScript_IfHooked, EmPlesioth_PulledAshore clear it */
    s8 enterDelay;              /* 0x80E: frames: GmEm_Update sets 2 while the monster is outside the scene's area; in it, Em_RaisePlayerAreaTriggerLax and Em_RaisePlayerAreaTrigger count it down and raise no trigger EM_TRIGGER_PLAYER_AREA until it is 0; GmEm_ResetState clears it */
    u8 guardTimer;              /* 0x80F: frames the hits cannot kill (hp kept at 1, also against poison), flinch or add status buildup (Em_UpdateDamage, Em_PickDamagedPart, Em_UpdateStatus), and Em_CheckFlash ignores flashes; counted down by GmEm_Update; states set 2 or 5, Em_BeginStatus2 30 */
    u8 flashed;                 /* 0x810: 1 when a flash object is in the sight cone (Em_CheckFlash); cleared when Em_ApplyFlash applies it */
    u8 noReact;                 /* 0x811: 1 while a status (Em_BeginStatus1/2/4/5 .. EndStatus) or a special state runs: GmEm_Update raises no triggers or rage actions, Em_CheckPartBuildup and Em_RaisePlayerAreaTriggerLax do nothing; GmEm_ResetState clears it */
    u8 routeMissCount;          /* 0x812: frames the route point was not reached (EmScript_End: at 10 it gives up the point); EmScript_StartRoute clears it */
    u8 need3State;              /* 0x813: 0 until need3Gauge first runs out (Em_DecayNeed3 sets 1 with need3); EmScript_SetRequest clears it */
    u8 need2State;              /* 0x814: 0 until need2Gauge first runs out (Em_DecayNeed2 sets 1 with need2); EmScript_SetVar sets, Op2B compares, Op0D/Op1E and Em_ClearNeed2State clear it */
    u8 need1State;              /* 0x815: 0 until need1Gauge first runs out (Em_DecayNeed1 sets 1 with need1); Em_ClearNeed1State, EmScript_SetRequest clear it */
    u8 noAlert;                 /* 0x816: set by EmScript_SetNoAlert (the script byte), 0 at reset: while 1, GmEm_Update raises no sight trigger (EM_TRIGGER_NOTICED) out of combat and the kind Update functions (Blangonga, Cephadrome, Kushala, Khezu) start no rage; Em_RaisePlayerAreaTriggerLax does nothing unless 0 */
    u8 remote;                  /* 0x817: online: 1 when another player's console runs the monster (hostNo), 0 when this one does (EmSlot_Alloc, GmEm_Setup); most AI code runs only when 0 */
    f32 playerDist[4];          /* 0x818: per player, the distance from the monster (Em_SensePlayers, EmSenseView.dist); Velocidrome and Velociprey compare it with nearRadius */
    f32 playerDistXZ[4];        /* 0x828: per player, the same in the ground plane (EmSenseView.distXZ); Rathalos and Garuga read the target's */
    f32 reactRange[3];          /* 0x838: the reaction set's ranges (Em_LoadReactionSet; Em_GetReaction kinds 3 to 6, EmSenseView); the fourth at +0x844 shares bytes with the Player field aimPitch */
    u8 reactRange3Lo[2];        /* 0x844: the low half of the reaction set's fourth range: Em_LoadReactionSet writes it as an f32 at +0x844 (Em_SensePlayers reads range 3); its high half is aimPitch */
    s16 aimPitch;               /* 0x846: as Player.aimPitch (same offset): Player_AddLean846 (player code typed Em) adds to it, kept within an eighth of a turn either way */
    s32 alertBuildup[4];        /* 0x848: one per player, out of combat: Em_AddAlertBuildup adds the kind table values for the player action (Em_PlayerActionKind), range and sector while the player is near (Em_SensePlayers); Em_CheckPartBuildup raises trigger EM_TRIGGER_PART_BUILDUP and alertedPlayers bit at the kind limit gEmPartBuildupLimits and clears it; Em_SetCombat clears all */
    u16 playerAngle[4];         /* 0x858: u16[4], one per player: direction from the monster to the player (Em_SensePlayers: Math_AngleXZ, as v->dir); Em_AngleSector finds the sense sector it falls in */
    u16 sectorHalfAngle[4];     /* 0x860: u16[4]: half widths of the 4 sense sectors around the heading, from the kind's sense row gEmReactionAngles[type] (Em_LoadReactionSet, with the ranges at 0x838..0x844); Em_AngleSector */
    u8 sensedPlayers;           /* 0x868: players within the sense reach, one bit each (Em_SensePlayers sets and clears them; Em_DecayNotice, Em_CheckPartBuildup, EmScript_IfSensed test them) */
    u8 alertedPlayers;          /* 0x869: one bit per player whose alertBuildup (alertBuildup) reached the kind limit (Em_CheckPartBuildup, with trigger EM_TRIGGER_PART_BUILDUP); EmScript_GoalAlertedPlayer tests the bits */
    u8 alertLimitRow;           /* 0x86A: the row of gEmPartBuildupLimits[type] the alertBuildup limit comes from (Em_AddAlertBuildup, Em_CheckPartBuildup) */
    u8 triggers;                /* 0x86B: script triggers pending, one bit each (0x02..0x80): EmScript_PickTrigger starts the script of the first */
    s32 noticeCount[4];         /* 0x86C: one per player: notice count (Em_AddNotice; at 9000 the player is noticed) */
    s8 loopPoint;               /* 0x87C: the area point the script loop goes to (EmScript_NextLoopPoint: goalKind 2, goalArg 1, goalNo = it), -1: none; picked at random, from a table or in turn (EmScript_NextLoopPoint, EmScript_End); EmScript_Init, EmScript_Restart, Em_TypeSetup set -1 */
    s8 loopCount;               /* 0x87D: passes of the script loop (EmScript_StartLoop: at most the area entry's point count); EmScript_End loops while the pass count (+0x87E) is below it; -1: none */
    s8 loopIndex;               /* 0x87E: the script loop's pass count (EmScript_StartLoop clears it, OpFF steps it and compares it with loopCount) */
    s8 areaPointCount;          /* 0x87F: the number of points of the area entry (EmScript_StartLoop e[1]); EmScript_NextLoopPoint and OpFF bound loopPoint by it */
    s8 areaSeqLen;              /* 0x880: length of the area sequence of EmScript_SetAreaSeq, -1: none; EmScript_StepAreaSeq and OpFF end the sequence when areaSeqPos reaches it. Capcom: max_area_route_suu (mhfo.dll's trace in EmScript_StepAreaSeq; Capcom's "area route" is the area sequence) */
    s8 areaSeqPos;              /* 0x881: position in the area sequence (EmScript_StepAreaSeq reads the next area from table 14 at it and makes it destArea), -1: none */
    u16 prevArea;               /* 0x882: the area the monster came from, 0xFFFF: none: Em_EnterNextArea sets it to area before moving to nextArea; Em_PickRoutePoint skips routes back to it; EmScript_IfPrevArea/Op3E test it; sent online (QSync_SendEm) */
    f32 motionSpeed;            /* 0x884 */
    f32 goal[3];                /* 0x888: the point the monster moves or turns to, computed from goalKind (Em_ComputeGoal). Capcom: horm_pos (mhfo.dll's trace in EmScript_IfGoalValid, SMELL_SET_CK) */
    void **tables;              /* 0x894: the per-kind area tables (gEmKindTables[type]): [0] points, [6] route sets; Em_FindAreaEntry finds the entry of an area, Em_AreaEntryValue its points */
    u8 unk898[0x899 - 0x898];
    s8 demonMode;               /* 0x899: as Player.demonMode (same offset): only player code typed Em uses it (DualSwords_ComboInput) */
    u8 unk89A[0x89C - 0x89A];
    u8 fxFlags;                 /* 0x89C: bit 1: set once a state's motion effect ran (EmMotion_PassedFrameScaled ... |= 1) and by Em_ResetToBasePose; GmEm_ChameleosMeshUv (mode 7) and EmTail_Run test it; the ChangeState functions keep only bit 1; Hermitaur uses bits 2, 4, Shen Gaoren 8 */
    u8 itemUseKind;             /* 0x89D: 0x80 | Side_HornItemPlaying(side) (GmEm_Update), 0x88 in versus quests; EmScript_SwitchItemUseKind and EmScript_IfVersusItemUsed read the kind and keep 0x80 */
    s16 areaTimer1;             /* 0x89E: a countdown set from the area values (Em_SetAreaTimer1), counted down by GmEm_Update; EmScript_IfAreaTimer1Over tests it > 0 (twin of areaTimer2) */
    s16 areaTimer2;             /* 0x8A0: a countdown set from the area values (Em_SetAreaTimer2; Hermitaur 1800 in Setup and Update), counted down by GmEm_Update; EmScript_IfAreaTimer2Over tests it > 0 */
    s16 blindTimer;             /* 0x8A2: frames of flash blindness: Em_StartBlind sets gEmBlindTimes[type] (and makes the stars, StunStars_CreateEm), Em_ApplyFlash copies it into timer; StunStars_Run ends at <= 0; Em_ClearPoison and GmEm_ResetState clear it; Kushala wind stops while > 0 */
    u8 actionGoal[3];           /* 0x8A4: the action spot Em_FindNearAreaObj found: kind, goalArg, goalNo, 0xFF: none (Em_StartGoalAction, EmScript_GoalActionGoal copy [1], [2] to the goal) */
    u8 partDamageMode;          /* 0x8A7: from the per-kind part table after the 9 hp values (Em_InitParts) */
    u16 syncRand;               /* 0x8A8: a random value drawn at each synced state change (Em_SyncState: Rand_Next(0)) and sent with it online (QSync_SendEm, QSync_OnEm, QSync_QueueEmState) */
    u8 unk8AA[0x8AB - 0x8AA];
    u8 part8Down;               /* 0x8AB: Em_UpdateDamage (reaction 9): part 8's hp ran out and the kind's handler took it: reaction 11; GmEm_ResetState clears it */
    s8 partBreakCount;          /* 0x8AC: times the part-break reaction ran (EM_DMG_PART8_DOWN: each family's ChangeState adds 1 entering its REACT_PART_BREAK state, Rathalos with fxFlags |= 1); the families' part-break callbacks (EmGaruga_AllowHit8 and the like) allow the break while 0 (Hermitaur while < 2; Kushala at 30% and 40% hp, its Init sets 1 or 2; Chameleos's Init sets 1); read as signed; GmEm_PrepareMeshSet hides the broken part's meshes while <= 0 */
    u8 trapHit;                 /* 0x8AD: PitfallTrap_Run sets 6 when its attack hit the monster directly; Plesioth's Update tests 6 on EM_DMG_STATUS2; the families' trap-struggle states and GmEm_ResetState clear it */
    u8 speedBonus;              /* 0x8AE: percent added to the speed: GmEm_Update scales the motion speed by (speedBonus + 100) / 100, Em_TurnStep the turn step; walking states (group 1) set 10..200; Em_SetState and GmEm_ResetState clear it */
    u8 spawnParam;              /* 0x8AF: byte +6 of the quest monster entry (QuestEm_Spawn): Velociprey/Anteka/Melynx Init switch on it, Shakalaka picks 2 or Rand_Next(1) & 1 and reads EmShakalaka_NearDist[it] (a distance), Kind38 bit 1, EmScript_SwitchSpawnParam compares it */
    u8 hideShadowHold;          /* 0x8B0: frames GmEm_Update leaves hideShadow as the state set it (states set 2 with hideShadow = 2); counted down there */
    u8 wallHitSpecial;          /* 0x8B1: 1 when a wall touched this frame has flag b[1] in its material row gAreaTriKindFlags (AreaHit_CollideSweep, which fills wallHitDirs too); Diablos EmDiablos_ChargeHitWall tests it with wallHitDirs 0xC0000003 at frame 60 (Player has its input lock here) */
    u16 collMask;               /* 0x8B2: per-kind collision mask gEmCollMasks[type] (Em_InitCollMask): Em_CollideField (wall collision) and AreaHit_Raycast (path checks of Em_FindNearAreaObj, EmScript_StartRoute) use it; Apceros State1_05 adds 128 */
    EmSpawn spawns[2];          /* 0x8B4 */
    f32 objectPos[3];           /* 0x94C: f32[3]: position of the object (bait, meat...) Em_SenseObjects found within its radius; the goal of goalKind 8 (Em_ComputeGoal, Em_ComputeGoal2) */
    struct Em *link;            /* 0x958: the monster of the linked kind (Em_FindLink) */
    u8 spawnQueued;             /* 0x95C: QSync_QueueEmState sets it when the local machine queues the monster's sync state into a spawn */
    u8 hostTimer;               /* 0x95D: 60 when this machine takes the monster over as host, counted down each frame (Em_MigrateHost) */
    u8 scriptAbort;             /* 0x95E: nonzero: EmScript_Run stops and restarts the script (Em_RestartScript); EmScript_SetAbort sets it (2 after 1000 commands in a frame) */
    u8 routeResult;             /* 0x95F: EmScript_StartRoute: 0 found, 2 no route point near, 7 a route without points */
    u8 unk960[0x964 - 0x960];
    u8 questKey;                /* 0x964: byte +0x18 of the quest monster entry (QuestEm_Spawn): key of the id sets (Em_NextAreas, Em_NextAreasFrom match it or -1); EmScript_SwitchQuestKey compares it; Rathalos Update tests 1 */
    s8 senseDelay;              /* 0x965: while nonzero (counted down each frame), GmEm_Update skips sensing the players (Em_SensePlayers, Em_SeePlayers); states set 5 */
    u8 large;                   /* 0x966: per-kind flag gEmIsLarge[type] (Em_InitKindParams), nonzero for the large monsters: synced online (GmEm_Setup, GmEm_Update, Em_SyncState, Em_IsSyncFree), death handled by the area owner (Em_UpdateDamage, Em_UpdateStatus), target point Player.targetPos (Em_GetTargetPos); EmScript_SwitchKindInArea kind 74 matches any of them */
    u8 requestPrio;             /* 0x967: priority of the pending request (128 low HP ... 64 idle); a new one needs at least this */
    u8 lowHpNeed;               /* 0x968: 1 when hp fell under the kind's ratio (Em_CheckLowHp): Em_PickRequest makes it a request of priority 128 */
    u8 need4;                   /* 0x969: 1: Em_PickRequest makes request gEmKindInfo.need4Request at priority 112 (then clears it); set by Em_CheckCarryRequest and EmScript_SetNeed4, cleared by EmScript_SetRequest */
    u8 need1;                   /* 0x96A: 1 when need1Gauge ran out (Em_DecayNeed1): Em_PickRequest makes request gEmKindInfo.need1Request at priority 96; Em_ClearNeed1State, EmScript_SetRequest clear it */
    u8 need2;                   /* 0x96B: 1 when need2Gauge ran out (Em_DecayNeed2): Em_PickRequest makes request gEmKindInfo.need2Request at priority 80; Em_ClearNeed2State, EmScript_SetRequest clear it */
    u8 need3;                   /* 0x96C: 1 when need3Gauge ran out (Em_DecayNeed3, kinds with gEmKindInfo.need3Request): Em_PickRequest makes request need3Request at priority 64 out of combat; the status begin/end functions, Em_RefillNeed3, EmScript_SetRequest clear it */
    u8 showOnMap;               /* 0x96D: per kind (gEmShowsOnMap[type], Em_InitKindParams): nonzero: Map_DrawArea draws the monster, Hud_DrawPlayerStatus marks its target player */
    u8 holdTimer;               /* 0x96E: nonzero: the monster is held (Em_IsHeld); Em_SetCaptured(em, 0) and the trapped group-4 states set 5 every frame, Kut-Ku SetGroup4State 50; counted down by GmEm_Update; the trap PitfallTrap_Run catches a monster with it > 0 within 700 and lets go at 0; the families pick other reactions while it is set */
    u8 questIndex;              /* 0x96F: index of the quest monster entry it came from (+0x2C, QuestEm_Spawn); Quest_OnRemoteKill matches it with the entry index */
    u8 startsBattle;            /* 0x970: per kind (gEmStartsBattle[type], Em_InitKindParams): nonzero: Player_UpdateInBattle counts the visible monster in the player area as a battle */
    u8 hostChanged;             /* 0x971: set when Em_MigrateHost hands the monster to another player (hostNo, remote, QSync_SendEm kind 4); GmEm_ResetState clears it */
    u8 reactionSet;             /* 0x972: the reaction set Em_LoadReactionSet loaded */
    u8 ceilingTimer;            /* 0x973: nonzero while on the ceiling: the ceiling states (Khezu State2_18..2_22, EM_KIND_SHOGUN_CEANATAUR's State2_04..2_08) set 5 every frame; counted down by GmEm_Update; a flinch then makes Khezu fall (state 4/1); EmScript_IfOnCeiling branches on it */
    u8 routeHeight;             /* 0x974: EmScript_StartRoute byte 2: a height in tenths (h = 10.0f * it) */
    u8 moveFrozen;              /* 0x975: 1 when Em_HoldPosNoPlayers undid the move (in combat, no hunter in its area, the state not the given one: back to prevPosX/prevPosZ); GmEm_Update then keeps prevPos and runs the motion with flag 0x20000; GmEm_ResetState, Em_HoldPosNoPlayers, Khezu and Plesioth Update clear it */
    u8 unk976[0x977 - 0x976];
    u8 hasCeiling;              /* 0x977: 1 when there is a ceiling over the monster (AreaGrid2_GetCeiling, height in surfaceY; 1 with 1500 in the special areas): Khezu and EM_KIND_SHOGUN_CEANATAUR (EmKhezu_Update, EmHermitaur_Update); ChangeState and ScriptQuery test it */
    u8 unk978[0x984 - 0x978];
    u16 sizePct;                /* 0x984: size in percent (quest entry +0x30, QuestEm_Spawn; 100 by default; versus team): quest size records (QuestEm_Hunted, QuestEm_Captured: gQuest.sizeMax/sizeMin), QuestEm_Appear grows it by 5, the mine radius and collision shapes scale with it */
    s16 destArea;               /* 0x986: the area the monster travels to, AREA_NONE: none: set by EmScript_StepAreaSeq/Op1A (Area_IdForCurrentTime), goalArg of Em_ComputeGoal; the State2_08/2_0D travel states take it as nextArea, State2_09 clears it on arrival; Em_PickRoutePoint defaults it to nextArea. Capcom: maji_next_stage_no (mhfo.dll's traces in EmScript_StepAreaSeq and GoalDestArea) */
    u8 unk988[0x989 - 0x988];
    u8 hitzoneSet;              /* 0x989: which of the kind's damage tables ObjAttach_DamageEm uses (the hitzone tables, indexed [type][hitzoneSet]): 0 normal, 1 while Chameleos is invisible, the Kushala wind blows, after a part break with flag 1 (Em_SpawnPartEffects), Hermitaur 2 in State3_04; ChangeState and GmEm_ResetState clear it */
    s8 level;                   /* 0x98A: strength level from the quest monster entry (+0x36, QuestEm_Spawn; versus: Versus_AssignEmTeam): row of the size, hp and rage scale tables (Em_ComputeMaxHp, Em_LevelAttackFactor/2), part hp scaling for 11..17 (Em_ScalePartHp); QuestEm_Appear raises it up to 15; EmScript_IfLevelAtLeast compares it */
    u8 hasGoal;                 /* 0x98B: 1: the state walks to goal (Em_StepToGoal, ends near it, goalDist from Vec_DistanceXZ), 0: no goal, the state runs on timer instead; set by the SetGroup1State/SetGroup3State functions from curGoalKind or targetNo */
    s16 trapTimer;              /* 0x98C: frames the trapped states (State4_0D, 4_11, 4_17 ...) last, counted down there: Em_SetTrapTime sets 832, 520 or 312 when enraged (156 for EM_KIND_RAJANG); goalKind 12 carries it as goalNo (/ 4, Em_SetTrapTimeGoal, Em_ComputeGoal2) */
    u16 aimAngle;               /* 0x98E: angle the monster turns to, ANGLE_TURN per turn (Math_AngleXZ to the goal; many states make it relative to rotY) */
    f32 goalDist;               /* 0x990: distance left to the goal (Vec_DistanceXZ, then counted down by velZ) */
    s16 scriptDelay;            /* 0x994: frames before the pending script runs: Em_UpdatePendingScript counts it down, then runs the script if scriptPending (scriptPending); set by EmScript_SetDelay, cleared by EmScript_Restart */
    u8 targetFound;             /* 0x996: Em_TargetPlayerInRange: 1 when a player was in range (curGoalKind 1), 0 otherwise */
    u8 curGoalKind;             /* 0x997: the goalKind the goal was last computed from (Em_ComputeGoal) */
    u16 curGoalArg;             /* 0x998: the goalArg the goal was last computed from */
    u16 curGoalNo;              /* 0x99A: the goalNo the goal was last computed from */
    s32 flightTurn;             /* 0x99C: turn left to make in flight, ANGLE_TURN per turn: Em_UpdateFlight takes it from aimAngle and turns rotY toward it by a step each frame until 0 */
    u8 versusGauge;             /* 0x9A0: versus gauge 0..90, 0xFF: none: raised by Versus_AddGauge, lowered by 30 by Versus_MonsterDown; Em_StartPendingRage makes a rage of gauge * 30 + 450 frames from it and clears it; synced (QSync_PackEmState) */
    u8 rageStarted;             /* 0x9A1: 1 from Em_StartPendingRage to Em_EndRage; GmEm_Update then sets itemUseKind to 0x88 */
    u8 stateArmor;              /* 0x9A2: 1 while the state is in a move the monster does not break off: set by the family states, cleared each frame by Em_ClearHitReactMode (with hitReactMode); ObjAttach_HitAll ignores ATTACK_HOLD then, Blangonga ignores the flash (EmBlangonga_IgnoresFlash), Hermitaur takes no group-4 reaction (EmHermitaur_AllowHit8); the ScriptQuery functions report it */
    u8 holdTimer2;              /* 0x9A3: the second hold (Em_IsHeld): Em_SetCaptured(em, 1) and Em_SetHold2 set 5; counted down by GmEm_Update; the trap PlacedItem_RunTrap catches a monster with it > 0 within 500 and lets go at 0; cleared by the SetGroup0State functions */
    u16 partsBroken;            /* 0x9A4: broken parts, one bit each (Em_BreakPart; Em_PartBroken tests them) */
    u16 partsToBreak;           /* 0x9A6: parts to break, one bit each (Em_FlagPartBreak; Em_ApplyPartBreaks breaks the new ones) */
    s8 mineSlot2;               /* 0x9A8: second mine point (EmMine_CreateTimed), -1 when none */
    s8 mineCount2;              /* 0x9A9: gathers for the second mine point */
    s16 mineTimer2;             /* 0x9AA: frames left for the second mine point */
    u8 hermitTimer;             /* 0x9AC: frames (read as s8): EmHermitaur_Guard sets 10, GmEm_Update counts it down; while nonzero EmHermitaur_Update goes to HERM_REACT_GUARD_RECOIL (not in the air, no stateArmor) and EM_KIND_DAIMYO_HERMITAUR plays SE 45; GmEm_ResetState clears it */
    s8 windTimer;               /* 0x9AD: 2 while the Kushala wind blows (EmKushala_Update with EmKushala_WindActive), counted down by GmEm_Update; Player_InEmAura looks for a monster near the player with it set */
    u8 unk9AE[0x9D0 - 0x9AE];
} Em;
void Em_InitRage(Em *em);
void Em_InitStatusThresholds(Em *em);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus1And2(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus5(Em *em);
void Em_InitCollMask(Em *em);
void Em_FindLink(Em *em);
void Em_ApplyActionStatus(Em *em, EmObj *action);
void Em_InitKindParams(Em *em);
void Em_ClearLowHpNeed(Em *em);
void Em_CheckKindRequest(Em *em);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
void Em_RefillNeed3(Em *em);
void Em_ResetNotice(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 bone);
s16 *Em_NextAreas(Em *em);
s16 *Em_NextAreasFrom(Em *em, u16 area);
void Em_AddNeed1(Em *em, s32 amount);
void Em_AddNeed2(Em *em, s32 amount);
void Em_AddNeed3(Em *em, s32 amount);
void Em_RaisePlayerAreaTriggerLax(Em *em);
void Em_RaisePlayerAreaTrigger(Em *em);
typedef char Em_size_check[(sizeof(Em) == 0x9D0) ? 1 : -1];

/* The monster slots (gEms): GmEm_Setup and the scene's loops go through all 20. */
#define EM_COUNT 20

extern Em gEms[EM_COUNT];
extern EmKindInfo gEmKindInfo[];
extern s16 *gEmRequestIds[76];
extern u8 gEmShowsOnMap[80];
extern u16 **gEmEnterAreaLists[76];
extern EmTraderMsg gEmTraderMsgs[106];

void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_UpdateBendTarget(Em *em);
void Em_TurnToward(Em *em, f32 *target, s32 mode);
void Em_BendStep(Em *em, u32 speed, s32 angle);
s8 Em_NearestActionToBone(Em *em, s32 bone, f32 *out);
s32 Em_FindNearAreaObj(Em *em);
EmObj *Em_FindGoalAction(Em *em);
void Em_StartGoalAction(Em *em, s32 need1Gain);
void Em_MoveRotated(Em *em, void *ang);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetState(Em *em, s32 group, s32 state);
void Em_SetStateRand(Em *em, s32 group, s32 state);
void Em_MotionDelta(Em *em, f32 *out);
void Em_CheckRageByHp(Em *em, s16 amount);
void Em_StartRage2(Em *em);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Em_UpdateReactions(Em *em);
void Em_AddNotice(Em *em, s32 add, s32 max, u8 player);
void Em_DecayNotice(Em *em);
void Em_AddAlertBuildup(Em *em, u8 player, u8 range, s8 sector);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_ComputeGoal2(Em *em, f32 *target);
u8 Em_UpdateNoticed(Em *em);
void Em_SensePlayers(Em *em);
void Em_CheckPartBuildup(Em *em);
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
void Em_TypeSetup(Em *em);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Em_GetPitchToTarget(Em *em, s32 *out, f32 rise, f32 reach);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 rise, f32 reach);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_PickRequest(Em *em);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_CheckCarryRequest(Em *em);
void Em_DecayNeed1(Em *em);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed2(Em *em);
void Em_DecayNeed3(Em *em);
void Em_Reduce7F0(Em *em);
void Em_Reduce7EC(Em *em);
s8 Em_AngleSector(Em *em, s32 player);
void Em_SyncState(Em *em, s32 group, s32 state, s32 flag);
s32 Em_InBlockArea1(Em *em);
s32 Em_InBlockArea2(Em *em);
u8 EmObj_AlertMonsters(EmObj *obj);
void Em_MigrateHost(Em *em);
u8 Em_SenseObjects(Em *em, EmObj *self);
extern f32 *gEmSenseLimits[76];
extern s32 *gEmPartBuildupLimits[76];
extern s32 *gEmNoticeDecayRates[76];
s32 Em_SetCombat(Em *em, u8 a, s16 b);

s8 Em_FindTargetPlayer(Em *em, u16 range, f32 maxDist, f32 maxHeight);
void Em_SetPartScaled(Em *em, s32 bit);
void Em_InitParts(Em *em);
void Em_ResetPartHp(Em *em, u32 i);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
EmBone *Em_GetBone(void *obj, s16 bone);
f32 *Em_BoneMatrix(void *obj, s16 bone);
f32 *Em_BoneMatrixB(void *obj, s16 bone);
s32 Tri_SegmentHit(f32 *tri, f32 *start, f32 *end, f32 *out);
void Em_CopyBoneMatrices(Em *em);
void Em_AdjustKindBones(Em *em, EmBone *boneList);
void Bone_UpdateTree(Em *em, EmBone *bone, f32 *parent, f32 parentScaleX, f32 parentScaleY, f32 parentScaleZ);
void Em_UpdateMatrix(Em *em);
void Em_CarryRiders(Em *em);
s32 Em_HitQuadsVertical(f32 *point, Em **hitEm, s16 *hitBone, f32 *hit);
void Em_RunAll(void);
u8 EmKind_MotionGroupCount(s16 i);
s16 EmSlot_FindKind(s16 k);

s8 Em_SeePlayers(Em *em);
s32 Em_SightBlocked(Em *em, void **ends);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_SetMotion(Em *em, s32 motion, s32 blend, s32 frame);
void Em_HoldPosNoPlayers(Em *em, u16 exceptGroup, u16 exceptState);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_AllPlayersHere(Em *em);
void Em_SetHitReactMode(Em *em, u8 mode);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
f32 Em_LevelAttackFactor(Em *em);
f32 Em_LevelDamageFactor(Em *em);
void Em_SetTrapTime(Em *em, u8 *wasInCombat);
void Em_SetTrapTimeGoal(Em *em);
s32 Em_NearLowHp(Em *em);
void Em_SpawnPartEffects(Em *em, u8 mask);
void Em_ResetToBasePose(Em *em);
void Em_RequestSync(Em *em, u8 mode);
void Em_UseGoalAction(Em *em);
void Em_SetMotionRow(Em *em, s32 row, s32 unused2, s32 unused3);
s32 Em_KindMatches(u8 kindA, u8 kindB);
void Em_ApplyAreaValues(Em *em);
void Em_EndRage(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 useHold2);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 endFrame, s32 se, s32 bone, s32 vol, s32 layer);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeAtBoneOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 layer);
void Em_PlayCommonSeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeAtBoneVol(Em *em, s32 se, s32 bone, s32 vol);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
void Em_PlaySeAtBoneVolOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 layer, s32 vol);
void Em_PlaySe(Em *em, s32 se);
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se);
s32 Em_IsSyncFree(Em *em, u8 mode);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *fly);
s32 EmList_FindKindNear(Em *em, u16 kind, s32 mode, f32 dist);
void Em_AimAtTarget(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurn(Em *em, EmTurnParams *turn, s32 back);
void Em_StartTurnToTarget(Em *em, EmTurnParams *turn);
void Em_StartTurnAway(Em *em, EmTurnParams *turn);
s32 Em_TurnStep(Em *em, EmTurnParams *w);
void Em_ClearTilt(Em *em, s32 unused);
extern f32 gAreaGroundY[200];
extern u8 gEmPushPriorities[74];
extern f32 gEmBodyScales[74];
extern f32 *gEmWallSpheres[];
extern f32 *gEmPushSpheres[];
/* The sight ranges of a sight row (Em_SeePlayers): the ground-distance and
 * height limits (a zero limit means none) and the half angle of the cone. */
typedef struct EmSightRange {
    f32 maxXZ;
    f32 minXZ;
    f32 above;
    f32 below;
    u16 half;
} EmSightRange;

/* A row of a kind's sight table (gEmSightRows[kind][row]; Em_SelectSightRow
 * points Em.sightTable at it), 32 bytes. */
typedef struct EmSightRow {
    EmSightRange range; /* 0x00: Em_SeePlayers */
    f32 unk14;          /* 0x14: no reader seen */
    f32 actionReach;    /* 0x18: Em_NearestActionToBone's reach (sightTable value 6) */
    f32 senseReach;     /* 0x1C: Em_SensePlayers' reach (sightTable value 7) */
} EmSightRow;
extern EmSightRow *gEmSightRows[76];
extern EmBendRow *gEmBendOrigins[76];
extern EmMotionRow *gEmMotionRows[76];
extern EmMotionRow *gEmMotionTransRows[76];
extern EmMotionRow *gEmMotionAltRows[76];

void ObjList_Update(void);
s32 ObjList_Add(EmObj *obj);
void ObjList_Remove(EmObj *obj);
void SightBlock_ClearAll(void);
s32 SightBlock_Add(EmObj *obj);
void SightBlock_Remove(EmObj *obj);
void SightBlock_Tick(void);
void FlashObj_ClearAll(void);
s32 FlashObj_Add(EmObj *obj);
void FlashObj_Remove(EmObj *obj);
void FlashObj_Tick(void);
void UnusedObjs_Clear(void);
void EmSenseObjs_Clear(void);
s32 EmSenseObjs_Add(EmObj *obj);
void EmSenseObjs_Remove(EmObj *obj);

void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);

s32 Em_IsHeld(Em *em);
void Em_ApplyFlash(Em *em);
void Em_ForceState19(Em *em);
void Em_UpdateStatus(Em *em);
void Em_AddPoison(Em *em, s32 amount);
void Em_AddStatus4(Em *em, s32 amount);
void Em_AddStatus2(Em *em, s32 amount);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus1Timer(Em *em);
void Em_ResetKindTimer(Em *em);
void Em_ApplyPartBreaks(Em *em);
void Em_FlagPartBreak(Em *em, s32 part);
s32 Em_DamageResultIsReaction(Em *em, s32 result);
u8 Em_UpdateDamage(Em *em, u8 *out);

void EmSlot_ClearAll(void);
void EmSlot_Free(Em *em); /* sub_main: e is an EmSlot * */
void EmSlot_FreeAll(void);
void EmSlot_SetupModel(Em *em);
Em *EmSlot_Alloc(void);

void Em_Draw(DrawSlot *slot);
void Em_InitHandlers(Em *em);
void Em_SetupKind(Em *em);
void Em_UpdateState(Em *em);
s32 Em_RunByState(Em *em);
void Em_UpdateWorldMatrix(Em *em);
void Em_RunMotionFx(Em *em);

#endif /* EM_H */
