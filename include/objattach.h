#ifndef OBJATTACH_H
#define OBJATTACH_H

#include "common.h"

/* The attack kinds (ObjAttach.attackKind, the hit target's lastAttackKind):
 * how a player reacts to the hit (Player_ReactToHit). The kinds from
 * ATTACK_HEAL on are the support attacks: ObjAttach_DamagePlayer and
 * ObjAttach_DamageEm apply them in place of damage. In the air
 * (Player.action 2) every kind but ATTACK_HEAL, ATTACK_GIVE_ATTACK_UP,
 * ATTACK_GIVE_DEFENSE_UP, ATTACK_CURE_POISON, ATTACK_INHALE,
 * ATTACK_TOSS_HIGH and ATTACK_CURE_STATUS6 blows the player away. */
enum AttackKind {
    ATTACK_BLOW = 2,         /* blows the player away (PL_REACT_BLOWN_BACK, _FORWARD) */
    ATTACK_TOSS = 3,         /* tosses the player up (PL_REACT_TOSSED_BACK, _FORWARD) */
    ATTACK_ROAR = 4,         /* likely a roar: PL_REACT_ROAR; skills 0x8F, 0x90 and song effect 4 (PL_SONG_ROAR_GUARD) block it */
    ATTACK_WIND = 5,         /* likely wind pressure: PL_REACT_WIND; skill 0x5B blocks it */
    ATTACK_TRAP_A = 6,       /* every monster family reacts to it as to a trap; it hits only shapes with HITSHAPE_TRAP_HIT */
    ATTACK_PAINT = 7,        /* paints the monster hit (Em.painted) */
    ATTACK_HEAL = 8,         /* heals: the power is taken off the damage (ObjAttach_DamageEm, ObjAttach_DamagePlayer); the only kind that hits allies */
    ATTACK_TRAP_B = 9,       /* as ATTACK_TRAP_A */
    ATTACK_REVIVE = 0xA,     /* hits only fainted players: gets them up (PL_EVENT_REVIVE, versus) */
    ATTACK_GIVE_ATTACK_UP = 0xB,   /* sets the player's attackDrug to 3 (ObjAttach_DamagePlayer) */
    ATTACK_GIVE_DEFENSE_UP = 0xC,  /* sets the player's defenseDrug to 6 */
    ATTACK_CURE_POISON = 0xD,      /* clears the player's poison */
    ATTACK_WIND_WEAK = 0xE,  /* PL_REACT_WIND_SHORT; skill 0x5A or 0x5B blocks it */
    ATTACK_ROAR_WEAK = 0xF,  /* PL_REACT_ROAR_SHORT; skill 0x8E blocks it */
    ATTACK_CALM = 0x10,      /* ends the monster's rage (Em_EndRage, ObjAttach_DamageEm) */
    ATTACK_QUAKE_LONG = 0x12, /* PL_REACT_QUAKE_LONG */
    ATTACK_HOLD = 0x13,      /* the monster families' second hold reaction; hits only shapes with HITSHAPE2_HOLD_HIT */
    ATTACK_QUAKE = 0x14,     /* likely a tremor: PL_REACT_QUAKE; skill 0xC3 blocks it */
    ATTACK_ROAR_STRONG = 0x15, /* PL_REACT_ROAR_STRONG; only skill 0x90 blocks it */
    ATTACK_INHALE = 0x16,    /* pulls the player in (PL_REACT_INHALED) */
    ATTACK_TOSS_HIGH = 0x17, /* as ATTACK_TOSS (seen: the same reactions) */
    ATTACK_CURE_STATUS6 = 0x18 /* clears the player's status6Timer */
};

/* ObjAttach.kind: the families of attack objects (each family's Create sets
 * it; nothing reads it, ObjAttach_UpdateAll calls each object's update). Two
 * families can share a kind. No family of kinds 32, 35 or 36 is seen. */
enum ObjAttachKind {
    ATTACH_PLAYER_ATTACK = 0,      /* PlayerAttack: a player's weapon hit */
    ATTACH_RATHALOS_ATTACK = 1,    /* RathalosAttack: Rathalos's and Rathian's body attacks */
    ATTACH_EM_CONTACT_ATTACK = 2,  /* EmContactAttack: Kirin's, Melynx's, Anteka's and Mosswine's (ATKF_REPORT_CONTACT);
                                    * also BulldromeAttack: Bulldrome's charge and spin, monster kind 0x38's dash */
    ATTACH_ITEM_ATTACK = 3,        /* ItemAttack: an item a player throws or places (bombs, thrown items) */
    ATTACH_FATALIS_ATTACK = 4,     /* FatalisAttack */
    ATTACH_LAOSHAN_ATTACK = 5,     /* LaoShanAttack */
    ATTACH_BOWGUN_SHOT = 6,        /* BowgunShot: a bowgun's bullet and its pellets */
    ATTACH_EM_IMPACT_ATTACK = 7,   /* EmImpactAttack: a monster's hit zone made at a point (likely slams, landings, stomps) */
    ATTACH_EM_RANGED_ATTACK = 8,   /* EmRangedAttack: a monster's attack from a bone: fireballs, spit, breath streams, thrown rocks */
    ATTACH_EXPLOSION = 9,          /* Explosion: an explosion's hit zone (bombs, shells, the monsters' fire) */
    ATTACH_PLACED_ITEM = 10,       /* PlacedItem: an item set on the ground: barrel bombs, a bomb on a monster, a trap */
    /* 11: VespoidAttack, the melee hit zones of two small monsters (see their callers) */
    ATTACH_VESPOID_ATTACK = 11,    /* VespoidAttack: Vespoid's and Hornetaur's melee hit zones, also the Apceros's and Popo's */
    ATTACH_PITFALL_TRAP = 12,      /* PitfallTrap: likely the pit trap: a monster that steps in is held */
    ATTACH_VELOCIPREY_ATTACK = 13, /* VelocipreyAttack: melee hit zones, one per attack (as RathalosAttack) */
    ATTACH_EM_BOMB = 14,           /* EmBomb: a bomb a monster holds or throws; it makes an Explosion */
    ATTACH_DROME_ATTACK = 15,      /* DromeAttack: the dromes' melee hit zones */
    ATTACH_KHEZU_ATTACK = 16,      /* KhezuAttack */
    ATTACH_KUTKU_ATTACK = 17,      /* KutkuAttack: Kut-Ku's and Gypceros's (type 22: the flash) */
    ATTACH_DIABLOS_ATTACK = 18,    /* DiablosAttack */
    ATTACH_GRAVIOS_ATTACK = 19,    /* GraviosAttack */
    ATTACH_PLESIOTH_ATTACK = 20,   /* PlesiothAttack */
    ATTACH_CEPHADROME_ATTACK = 21, /* CephadromeAttack */
    ATTACH_STAGE_SHOT = 22,        /* StageShot: a stage mechanism's shot or falling object: the cannon, the mounted gun, falling rocks */
    ATTACH_CEPHALOS_ATTACK = 23,   /* CephalosAttack */
    ATTACH_BOW_ARROW = 24,         /* BowArrow: the bow's arrow */
    ATTACH_GARUGA_ATTACK = 25,     /* GarugaAttack */
    ATTACH_HERMITAUR_ATTACK = 26,  /* HermitaurAttack; also RemobraAttack (Remobra's) */
    ATTACH_BLANGONGA_ATTACK = 27,  /* BlangongaAttack: the Blangonga family's */
    ATTACH_BOOMERANG = 28,         /* Boomerang: a thrown item that curves out and flies back to the thrower */
    ATTACH_KUSHALA_BREATH = 29,    /* KushalaBreath: likely Kushala Daora's breath, from bone 0x25: skims the floor, aimed or diving; bursts in an Explosion */
    ATTACH_KUSHALA_ATTACK = 30,    /* KushalaAttack: Kushala Daora's body hit zones; also TeostraAttack (Lunastra's and Teostra's) */
    ATTACH_GIANT_ATTACK = 31,      /* ShenGaorenAttack (its body and stomps) and YamaTsukamiAttack */
    ATTACH_EM_EFFECT_ATTACK = 33,  /* EmEffectAttack: a monster's beam, breath, gas or shot drawn as parts (16 types, one per monster's attack) */
    ATTACH_SHAKALAKA_ATTACK = 34,  /* ShakalakaAttack */
    ATTACH_VOLCANO_HAZARD = 37,    /* VolcanoHazard: a timed hit zone AreaObj_K19_Run puts at a volcano area's points; players only */
    ATTACH_GUNLANCE_BLAST = 38,    /* GunlanceBlast: the gunlance's shelling (types 1-3) and wyvern fire (4) */
    ATTACH_TEOSTRA_BLAST_RING = 39, /* TeostraBlastRing: likely Lunastra's and Teostra's eight bursts in a ring after a charge */
    ATTACH_CHAMELEOS_ATTACK = 40   /* ChameleosAttack */
};

/* How an ItemAttack moves (ItemAttack_Modes[type]; Setup, Run and Draw switch on it). */
enum ItemAttackMode {
    ITEM_THROWN = 0,    /* thrown from the hand: flies and lands (ItemAttack_RunThrown) */
    ITEM_PLACED = 1,    /* put down at the feet: a fuse, then it bursts (ItemAttack_RunPlaced) */
    ITEM_HOPPING = 2    /* put down, then jumps up and bursts (ItemAttack_RunHopping) */
};

/* PlacedItem's ObjAttach.type: what was set on the ground (PlacedItem_Create;
 * Setup, Run, Hit and DrawMain switch on it). The items are item.h's. */
enum PlacedItemType {
    PLACED_LARGE_BARREL_BOMB = 0,   /* ITEM_ID_LARGE_BARREL_BOMB, and the areas' own barrels (no player) */
    PLACED_LARGE_BARREL_BOMB_B = 1, /* ITEM_ID_LARGE_BARREL_BOMB_B and _SUPPLY_: drawn larger, a bigger Explosion */
    PLACED_PLAYER_HIT_A = 2,        /* a hit shape for players only (ATKF_NO_EM); no caller seen */
    PLACED_PLAYER_HIT_B = 3,        /* the same; no caller seen */
    PLACED_TRAP = 4,                /* ITEM_ID_TRAP and _SUPPLY_TRAP: holds a monster (PlacedItem_RunTrap) */
    PLACED_MONSTER_BOMB = 5,        /* ITEM_ID_MONSTER_BOMB: set on the monster Em_HitQuadsVertical finds */
    PLACED_BOMB = 6                 /* ITEM_ID_PLACED_BOMB */
};

/* StageShot's ObjAttach.type: the shots of the stage's guns and objects
 * (StageShot_Create*; Setup, Run, Hit, Draw and PlaySound switch on it). */
enum StageShotType {
    STAGESHOT_MOUNTED_GUN = 0,      /* Player_StateMountedGunFire: two per firing; hits monsters only */
    STAGESHOT_CANNON = 1,           /* Player_StateCannonFire, from a cannon spot */
    STAGESHOT_AREA_BURST = 2,       /* AreaWeapon_Run: four objects with no owner and no model */
    STAGESHOT_AREA_PULSE = 3,       /* AreaWeapon_Run: one object that reopens its hit every 25 frames */
    STAGESHOT_LAOSHAN_ROCK = 4,     /* Lao-Shan Lung's falling rock, at a spot (EmLaoshan_MotionEffects); also hits players */
    STAGESHOT_AREA_DROP = 5,        /* CastleGate_Run: the hit under its dropping object; hits monsters only */
    STAGESHOT_FALLING_OBJECT = 6,   /* the objects Blangonga and Rajang bring down (EmBlangonga_DropFallingObjects); also hits players */
    STAGESHOT_ARENA_STRIKE = 7,     /* Player_StateSpotStrike: rises under a monster in the Great Arena */
    STAGESHOT_AREA_PAIR = 8         /* AreaWeapon_Run: two objects facing back (0x8000) */
};

/* Boomerang's ObjAttach.type: how it was thrown (Player_StateThrowItem). */
enum BoomerangType {
    BOOMERANG_FLAT = 0,             /* thrown standing: a flat curve */
    BOOMERANG_RISING = 1,           /* thrown walking: starts low and climbs; slides along the ground when it drops */
    BOOMERANG_SINKING = 2           /* thrown running: starts high and sinks */
};

/* EmRangedAttack's ObjAttach.type: the monster's move (the creators are the
 * monsters' attack states; EmRangedAttack_Setup and every other function of
 * the family switch on it). */
enum EmRangedAttackType {
    EMRANGED_FIREBALL = 0,              /* Rathalos's and Garuga's fireball; variants 1 and 2 the big one */
    EMRANGED_PLESIOTH_STREAM = 1,       /* Plesioth's water stream, held at its bone */
    EMRANGED_KUTKU_FIREBALL_SIDE = 2,   /* the two fireballs beside Kut-Ku's main one */
    EMRANGED_KUTKU_FIREBALL = 3,        /* Kut-Ku's fireball */
    EMRANGED_GRAVIOS_BEAM = 4,          /* Gravios's beam, held at its bone */
    EMRANGED_KHEZU_CEILING_SHOT = 5,    /* Khezu's shot from the ceiling: falls, sparks on the floor */
    EMRANGED_KHEZU_GROUND_BOLT = 6,     /* Khezu's ground lightning, five fanned out on the floor */
    EMRANGED_CEPHADROME_SHOT = 7,       /* Cephadrome's shot */
    EMRANGED_KIRIN_BOLTS = 8,           /* Kirin's lightning strikes at spots around it */
    EMRANGED_PREY_SPIT = 9,             /* the raptors' spit; variants 2 and up fly twice as fast */
    EMRANGED_PLESIOTH_FLOOR_WAVE = 10,  /* where Plesioth's stream meets the floor: runs along it, not drawn */
    EMRANGED_FATALIS_FIREBALL = 11,     /* Fatalis's fireball */
    EMRANGED_FATALIS_SWEEP = 12,        /* Fatalis's sweeping fire, at a sweep spot */
    EMRANGED_GRAVIOS_SHOT = 13,         /* Gravios's fire shot */
    EMRANGED_FLOOR_TRAIL = 14,          /* leaves fire on the floor every third frame; no creator seen */
    EMRANGED_BLANGONGA_LUMP = 15,       /* the lump Blangonga digs up and throws; breaks into three fragments */
    EMRANGED_THROW_FRAGMENT = 16,       /* a fragment of the lump */
    EMRANGED_BLANGO_THROW = 17,         /* a Blango's throw */
    EMRANGED_KUSHALA_BREATH = 18,       /* Kushala Daora's breath, held at its bone */
    EMRANGED_CONGALALA_THROW = 19,      /* Congalala's throw */
    EMRANGED_BIG_FRAGMENT = 20,         /* a fragment at five times the scale; no creator seen */
    EMRANGED_RAJANG_BEAM = 21,          /* Rajang's beam, held at its bone; variant 1 half as wide */
    EMRANGED_HERMITAUR_SHOT = 22,       /* Hermitaur's shot */
    EMRANGED_PREY_SPIT_B = 23,          /* the raptors' second spit; variant 1 flies twice as fast */
    EMRANGED_REMOBRA_SPIT = 24          /* Remobra's spit */
};

/* EmEffectAttack's ObjAttach.type (EmEffectAttack_Create*; every function of
 * the family switches on it). The callers are the monsters' states. */
enum EmEffectAttackType {
    EMFX_YAMA_INHALE = 0,           /* Yama Tsukami's inhale: a hit with no power along a capsule ahead */
    EMFX_CONGALALA_BREATH = 1,      /* Congalala's breath after eating; the variant is what it ate */
    EMFX_RAJANG_SHOT = 2,           /* Rajang's shot: an Explosion where it meets the floor */
    EMFX_SHEN_SHOT = 3,             /* Shen Gaoren's shot that lands in its own area */
    EMFX_DAIMYO_JET = 4,            /* EM_KIND_DAIMYO_HERMITAUR's water jet */
    EMFX_SHEN_SHOT_AWAY = 5,        /* Shen Gaoren's shot leaving its area (EMFX_SHEN_SHOT_DROP lands it) */
    EMFX_CONGALALA_GAS = 6,         /* Congalala's gas: a schedule of puffs */
    EMFX_CONGALALA_RAGE_GAS = 7,    /* an enraged Congalala's gas puff */
    EMFX_SHEN_SHOT_DROP = 8,        /* Shen Gaoren's shot falling in another area: hidden for 140 frames, then at a spot */
    EMFX_CHAMELEOS_BALL = 9,        /* Chameleos's ball: an EmImpactAttack where it meets the floor */
    EMFX_SHOGUN_JET = 10,           /* EM_KIND_SHOGUN_CEANATAUR's water jet */
    EMFX_CHAMELEOS_BREATH = 11,     /* Chameleos's breath: a capsule along its aim */
    EMFX_FATALIS_SWEEP_BLAST = 12,  /* EM_KIND_FATALIS_C's sweep blasts, at spots, after a ground mark */
    EMFX_FATALIS_FIREBALL = 13,     /* EM_KIND_FATALIS_C's fireball: an Explosion on the floor */
    EMFX_FATALIS_BLAST = 14,        /* EM_KIND_FATALIS_C's blasts at fixed spots, after a ground mark */
    EMFX_BREATH_CHAIN = 15          /* a chain of links from bone 0x25: likely Lunastra's and Teostra's fire breath */
};

/* Explosion's ObjAttach.type (Explosion_Create and its variants): the hit
 * zone a burst leaves. Named by their makers. A bomb's or burst's second
 * part, made by Explosion_Spawn, hits the monsters only (ATKF_NO_PLAYER).
 * Types 6, 0x23, 0x25 and 0x26 have no maker in the code. */
enum ExplosionType {
    EXPLOSION_BOMB = 0x00,                      /* the small barrel bomb, the launched and thrown bombs, the Melynx's bombs */
    EXPLOSION_FIREBALL = 0x01,                  /* a fireball's impact (EmRangedAttack type 0) */
    EXPLOSION_LARGE_BARREL_BOMB = 0x02,         /* PLACED_LARGE_BARREL_BOMB, the Melynx's large bomb */
    EXPLOSION_FIREBALL_AFTER = 0x03,            /* the second part of EXPLOSION_FIREBALL */
    EXPLOSION_FIREBALL_BIG = 0x04,              /* a big fireball's impact */
    EXPLOSION_FIREBALL_BIG_AFTER = 0x05,        /* the second part of EXPLOSION_FIREBALL_BIG */
    EXPLOSION_BOWGUN_FLOOR = 0x07,              /* a bowgun shot of hit kind 2 reaching the floor */
    EXPLOSION_BOWGUN_BURST_LV1 = 0x08,          /* a bowgun shot of hit kind 3 bursting after it hit */
    EXPLOSION_BURST_EM_PART = 0x09,             /* the second part of the bombs and bowgun bursts */
    EXPLOSION_LARGE_BARREL_BOMB_EM_PART = 0x0A, /* the second part of EXPLOSION_LARGE_BARREL_BOMB */
    EXPLOSION_KUTKU_FIREBALL_SIDE = 0x0B,       /* EmRangedAttack type 2 */
    EXPLOSION_KUTKU_FIREBALL = 0x0C,            /* EmRangedAttack type 3 */
    EXPLOSION_THROWN_BURST = 0x0D,              /* a thrown item bursting 16 frames after the throw */
    EXPLOSION_BOWGUN_BURST_LV2 = 0x0E,          /* as LV1, hit kind 4 */
    EXPLOSION_BOWGUN_BURST_LV3 = 0x0F,          /* as LV1, hit kind 5 */
    EXPLOSION_DROME_DEFEAT = 0x10,              /* a drome's defeat burst */
    EXPLOSION_CEPHADROME_SHOT = 0x11,           /* EmRangedAttack type 7 */
    EXPLOSION_FATALIS_FIREBALL = 0x12,          /* EmRangedAttack types 0xB and 0xC */
    EXPLOSION_THROWN_CLOUD_A = 0x13,            /* a thrown item's cloud on the floor (one item each, A to C): 80 frames */
    EXPLOSION_SMALL_BARREL_BOMB_B = 0x14,       /* ITEM_ID_SMALL_BARREL_BOMB_B placed */
    EXPLOSION_SMALL_BARREL_BOMB_B_EM_PART = 0x15, /* its second part */
    EXPLOSION_LARGE_BARREL_BOMB_B = 0x16,       /* PLACED_LARGE_BARREL_BOMB_B */
    EXPLOSION_LARGE_BARREL_BOMB_B_EM_PART = 0x17, /* its second part */
    EXPLOSION_KUTKU_FIREBALL_SIDE_B = 0x18,     /* EmRangedAttack type 2, the second variant */
    EXPLOSION_GRAVIOS_SHOT = 0x19,              /* EmRangedAttack type 0xD */
    EXPLOSION_THROWN_CLOUD_B = 0x1A,
    EXPLOSION_THROWN_PLAYERS_ONLY = 0x1B,       /* a thrown item with a side set: hits the players only (ATKF_NO_EM) */
    EXPLOSION_KUSHALA_BREATH = 0x1C,            /* KushalaBreath's impact, by its size */
    EXPLOSION_KUSHALA_BREATH_BIG = 0x1D,
    EXPLOSION_KUSHALA_GROUND_BLAST = 0x1E,      /* EmKushala_MotionEffects: the blast ahead on the floor */
    EXPLOSION_KUSHALA_BREATH_B = 0x1F,
    EXPLOSION_KUSHALA_BREATH_B_BIG = 0x20,
    EXPLOSION_PLACED_BOMB = 0x21,               /* PLACED_BOMB and the no-bomb-area item */
    EXPLOSION_DIVE_IMPACT = 0x22,               /* EmKind38's dive reaching its goal */
    EXPLOSION_THROWN_CLOUD_C = 0x24,
    EXPLOSION_RAJANG_SHOT = 0x27,               /* EMFX_RAJANG_SHOT's impact */
    EXPLOSION_FATALIS_C_FIREBALL = 0x28         /* EMFX_FATALIS_FIREBALL's impact */
};

/* ObjAttach.state. Each family's Update switches on it; the hit functions
 * (ObjAttach_HitObjects and the damage functions) set 3, 5 and 9. States 4 and 7 have no setter (the
 * Updates end the object there). */
enum ObjAttachState {
    ATTACH_SETUP = 0,           /* set up on the first update, then ATTACH_RUN */
    ATTACH_RUN = 1,             /* runs every frame */
    ATTACH_END = 2,             /* ends: frees its draw slots, then ATTACH_FREE */
    ATTACH_HIT = 3,             /* it hit a monster or a player this frame (ObjAttach_DamageEm, ObjAttach_DamagePlayer) */
    ATTACH_DEFLECTED = 5,       /* deflected: by a monster's wind (ATKF_WIND_DEFLECTS) or a hard part (ATKF_SHELL_DEFLECTS) */
    ATTACH_FREE = 6,            /* freed on the next update (ObjAttach_Free) */
    ATTACH_DEFLECTED_AURA = 9   /* deflected by the wind of monster kinds 0x40 and 0x41 (ATKF_WIND_DEFLECTS) */
};

/* ObjAttach.attackFlags: set at set up (Player_TestSetFlags), tested by the
 * hit and damage functions (Player_TestFlags). */
#define ATKF_SHELL_DEFLECTS  0x0001  /* deflected by a hurt shape whose hit flags have bit 2 (ATTACH_DEFLECTED) */
#define ATKF_HITS_OWNER      0x0004  /* may hit its own owner */
#define ATKF_SHOT            0x0008  /* takes the hit zone's shot value (the guns' damage) */
#define ATKF_WIND_DEFLECTS   0x0010  /* deflected by a hurt shape whose hit flags have bit 1, or by an open wind shape */
#define ATKF_OWNER_CONTACT   0x0020  /* a hit gives the owner its hit stop and its contact (Player.contact, contactAttach) */
#define ATKF_WEAPON          0x0040  /* a weapon's hit: criticals (hitMarks 1) and the motion value rows */
#define ATKF_NO_EM           0x0080  /* hits no monster */
#define ATKF_SHIELDED        0x0100  /* not hit by a monster's object without ATKF_PIERCES */
#define ATKF_PIERCES         0x0200  /* hits an ATKF_SHIELDED object */
#define ATKF_NO_PLAYER       0x0400  /* hits no player */
#define ATKF_REPORT_CONTACT  0x0800  /* as ATKF_OWNER_CONTACT, and sets the owner's contactHit (Em.contactHit) */

/* ObjAttach.elementFlags: what elementPower is (ObjAttach_DamageEm). The
 * status bits add it to the target's buildup of the monster status named
 * (Em.status numbers; Em_ApplyHitStatus). */
#define ELEMF_EM_STATUS2  0x0001
#define ELEMF_POISON      0x0002
#define ELEMF_EM_STATUS4  0x0004
#define ELEMF_EM_STATUS1  0x0008
#define ELEMF_FIRE        0x0010
#define ELEMF_WATER       0x0020
#define ELEMF_THUNDER     0x0040
#define ELEMF_ICE         0x0080
#define ELEMF_MASH        0x0100  /* adds the power to mashBuildupHit */
#define ELEMF_DRAGON      0x0200
#define ELEMF_STATUS6     0x0400  /* sets the target's status6Hit (30 s) */
#define ELEMF_STATUS8     0x0800  /* sets the target's status8Hit */
#define ELEMF_STATUS7     0x1000  /* sets the target's status7Hit (30 s) */

/* ObjAttach.hitFx: the low 6 bits are the hit effect's kind; the two high
 * bits mark the affinity's roll (PlayerAttack_Setup, ObjAttach_DamageEm). */
#define HITFX_KIND_MASK  0x3F
#define HITFX_WEAK       0x40  /* a weak hit (negative affinity): half the power */
#define HITFX_CRITICAL   0x80  /* a critical hit: 1.25 times the power */

/* ObjAttach.ownerIndex of an object no player or monster owns (ObjAttach_LoadRow) */
#define ATTACH_OWNER_NONE 0xFF

/*
 * An object of sub_main's second object pool (0x40 objects of 0xD4 bytes,
 * gObjAttachPool; ObjAttach_Alloc, ObjAttach_Free), attached to a player or a
 * monster: the game overlay's effects and the like. The struct is generated
 * from include/objattach.spec (tools/gen_struct.py): add fields there.
 */

struct ObjAttach;
/* An object's per-frame update and draw functions (ObjAttach_UpdateAll, ObjAttach_DrawAll). */
typedef void (*ObjAttachFn)(struct ObjAttach *o);
typedef struct ObjAttach {
    u8 active;                  /* 0x000: nonzero while the object is in use */
    u8 visible;                 /* 0x001 */
    u8 kind;                    /* 0x002: the family, enum ObjAttachKind, set by the creator (0x21: EmEffectAttack_Init) */
    u8 type;                    /* 0x003: the variant within the kind (the creator's argument; the updates switch on it) */
    u8 state;                   /* 0x004: the object's state, enum ObjAttachState (each family's Update switches on it) */
    u8 step;                    /* 0x005: as Em.step: the step within the update (switch on it, counted up; 0 at set up) */
    u8 stateVar0;               /* 0x006: as Em.stateVar0: per-kind scratch byte (switch on it, counters, 0xFF none) */
    u8 stateVar1;               /* 0x007: as Em.stateVar1: a second per-kind scratch byte (0xFF/0xFE marks, counters) */
    u8 ownerIndex;              /* 0x008: the owner's index (ObjHead.index), set with the attack row (ObjAttach_LoadOwnedRow); 0xFF: an attack with no owner (ObjAttach_LoadRow) */
    u8 keep;                    /* 0x009: not freed by ObjAttach_FreeUnkept */
    u8 idx;                     /* 0x00A: the index of the player or monster it follows (gEms, gPlayers) */
    u8 hitPhase;                /* 0x00B: the hit window: 0 off, 1 waiting hitDelay frames, 2 open for hitFrames (ObjAttach_UpdateTimers; ObjAttach_LoadOwnedRow restarts it); cleared when the owner motion changes (LaoShanAttack_Run, RathalosAttack_Run, FatalisAttack_Run end the object at 0) */
    struct ObjAttach *prev;     /* 0x00C */
    struct ObjAttach *next;     /* 0x010 */
    ObjAttachFn draw;           /* 0x014: drawn every frame while it, its owner and the object are visible (ObjAttach_DrawAll); NULL: none */
    u8 *buf;                    /* 0x018: its blocks of the object buffer */
    u8 blk;                     /* 0x01C */
    u8 nblk;                    /* 0x01D */
    u8 didHit;                  /* 0x01E: 1 once it hit a monster or a player (ObjAttach_DamageEm, ObjAttach_DamagePlayer); cleared before each hit pass; PlacedItem and PitfallTrap test it */
    u8 unk01F[0x020 - 0x01F];
    ObjAttachFn update;         /* 0x020: run every frame once delay is 0 (ObjAttach_UpdateAll) */
    s32 rotX;                   /* 0x024: its rotation (angles; the creators copy the owner's, Em +0xA0/+0xA4/+0xA8) */
    s32 rotY;                   /* 0x028 */
    s32 rotZ;                   /* 0x02C */
    f32 pos[3];                 /* 0x030: the position */
    f32 pos2[3];                /* 0x03C: the second point of the hit capsule: Capsule_FromPoints(pos, pos2, hitList, .., hitScale) (ObjAttach_HitObjects, ObjAttach_HitTarget); PlayerAttack_PlaceOnWeapon sets it from a bone matrix, ItemAttack_Setup from pos */
    f32 velX;                   /* 0x048: velocity x: ItemAttack/BowgunShot/BowArrow add it to pos; Vec_RotateY(&velX) and Vec_DirToAngles use velX..velZ as a vector */
    f32 velY;                   /* 0x04C: velocity y (see velX) */
    f32 velZ;                   /* 0x050: velocity z (see velX) */
    f32 homingTurn;             /* 0x054: homing: the turn toward the target, grown each frame (attack_families.c); zeroed at creation by the other kinds */
    f32 gravity;                /* 0x058: added to velY each frame (BowgunShot, EmEffectAttack: EmEffectAttack_Gravities[type], -1.0 / -1.5) */
    f32 homingSpeed;            /* 0x05C: homing: added to the speed, grown each frame (attack_families.c); zeroed at creation by the other kinds */
    u8 hitDelay;                /* 0x060: frames before the hit window opens: the Setup functions compute it from the owner motion frames (PlayerAttack_Setup: k - motionFrame / 2 - 1); ObjAttach_UpdateTimers counts it down in hitPhase 1 */
    u8 hitFrames;               /* 0x061: frames the hit window stays open (99: long; FatalisAttack 0x3C): ObjAttach_UpdateTimers counts it in hitPhase 2 */
    u8 power;                   /* 0x062: the attack power: ObjAttach_DamageEm (dmgf = it), ObjAttach_DamagePlayer (it * atk); BowgunShot_CalcPower, BowArrow_ApplyPower set it */
    u8 attackKind;              /* 0x063: the attack kind, copied to the target lastAttackKind (ObjAttach_DamageEm/DamagePlayer): 7 paints (Em.painted), 8 and 0x10 special cases, 0xA hits only state-3 players (ObjAttach_HitTarget); ObjAttach_HitObjects/HitAll switch on it */
    u8 unk064[0x065 - 0x064];
    u8 attrs;                   /* 0x065: attribute bits ORed into the target hitAttrs (ObjAttach_DamageEm, ObjAttach_DamagePlayer): 0x80 cannot be guarded; 1 tested by ObjAttach_HitAll, 0x10 by ObjAttach_DamagePlayer */
    s8 hitYawOfs;               /* 0x066: degrees added to the object's yaw for the target's hitAngle, unless attrs 0x10 (ObjAttach_SetHitHeading) */
    u8 unk067[0x068 - 0x067];
    u8 hitForce;                /* 0x068: copied to the target's hitForce (ObjAttach_DamagePlayer, ObjAttach_DamageEm) */
    u8 hitMarks;                /* 0x069: mark bits ORed into the target's partFlags: 1 a critical hit (with flag 0x40), 2, 8; on a player 4 fire, 0x10 water, 0x40 thunder, 0x20 ice, 0x80 dragon pick the resistance (ObjAttach_DamagePlayer) */
    u8 hitListNo;               /* 0x06A: the index into the kind tables hitList is taken from (hitList = BlangongaAttack_HitShapes[it] and the like, Setup functions) */
    u8 hitSe;                   /* 0x06B: the hit sound kind: 0xFF none; on a player 1 SE 0x20, others 0x21 (ObjAttach_DamagePlayer); on a monster by kind (ObjAttach_DamageEm) */
    u16 elementFlags;           /* 0x06C: the element bits: 0x10 fire, 0x20 water, 0x40 thunder, 0x80 ice, 0x200 dragon take elementPower (ObjAttach_DamageEm); the status bits 1 status 2, 2 poison, 4 status 4, 8 status 1, 0x100 mash buildup add elementPower to the target's hit amounts, 0x400, 0x1000 and 0x800 set status 6, 7 and 8 (Em_ApplyHitStatus); ObjAttach_HitEffects picks the hit effect from them; BowArrow_ApplyCoating sets them */
    u8 elementPower;            /* 0x06E: the element value for elementFlags (ObjAttach_DamageEm: fire = it, ...) */
    u8 hitFx;                   /* 0x06F: low 6 bits: the hit effect kind (HitSplash_CreateByHitFx, HitSpark_Spawn in ObjAttach_HitEffects); 0x80 a critical hit, 0x40 a critical miss (the debug prints of ObjAttach_DamageEm) */
    u8 hitStop;                 /* 0x070: the hit stop: the hitter hitStopAdd (times 3 in some cases) and hitSlowTimer, the target hitStopAdd (ObjAttach_DamageEm, ObjAttach_DamagePlayer) */
    u8 swordGaugeAdd;           /* 0x071: long sword: the spirit gauge gained by the hit, times gSwordGaugeHitFactors[hitKind] (LongSword_AddGauge in ObjAttach_DamageEm) */
    u8 multiHit;                /* 0x072: low 4 bits: the hits left; high 4 bits: the delay before the next (ObjAttach_NextMultiHit reopens the hit window with a new hitId) */
    u8 nextRow;                 /* 0x073: the attack row loaded when the hit window closes, 0: none (ObjAttach_UpdateTimers runs ObjAttach_LoadOwnedRow with it and reopens the window); AttackRow's nextRow */
    u8 stun;                    /* 0x074: the stun amount: the target status5Hit (ObjAttach_DamageEm), stun (ObjAttach_DamagePlayer) */
    u8 mvFramesA;               /* 0x075: frames of the first motion-value phase, counted down while the window is open (0xFF: held); ObjAttach_DamageEm picks motion-value kind 0 at 0xFF, 1 while it runs */
    u8 mvFramesB;               /* 0x076: frames of the second phase, counted down once mvFramesA is 0; ObjAttach_DamageEm picks kind 2 while it runs (3 on a shape of zone 1) */
    u8 unk077[0x078 - 0x077];
    u16 attackFlags;            /* 0x078: ATKF_* bits, set by the Setup functions (Player_TestSetFlags) and tested by the hit and damage functions (Player_TestFlags); cleared by ObjAttach_Free */
    u8 isEm;                    /* 0x07A: 0: follows a player, else a monster */
    u8 delay;                   /* 0x07B: frames before update runs */
    s16 ownerMotion;            /* 0x07C: the owner's motionId[0] when the object was made (LaoShanAttack_Run's family and the others that end with it); the hit window closes when the owner's motion differs; some kinds count frames in it (EmRangedAttack: the creator's bone, moved to its buffer at set up) */
    s16 aux;                    /* 0x07E: per-kind value: a bone of the owner (Em_GetBone, f06_BoneLaunchPos), a monster kind (EmSlot_FindKind), a frame counter */
    s16 unk080;                 /* 0x080 */
    u8 unk082[0x088 - 0x082];
    struct HitShape *hitList;   /* 0x088: the attack hit shapes (HitShape, include/capsule.h), a list ended by bone -1, read with pos/pos2 (ObjAttach_HitObjects, ObjAttach_HitTarget: no hit while NULL); PlacedItem_Shapes[type] and the kind tables */
    struct HitShape *hurtList;  /* 0x08C: the shapes other objects hit (HitShape, include/capsule.h) (ObjAttach_HitObjects: elist = e->hurtList, skipped while NULL) */
    void *attackRows;           /* 0x090: the family's attack rows (the address of its RowsPtr: ObjAttach_StartOwnedAttack stores it; the row picked by the creator's type is copied over 0x060-0x077) */
    void *owner;                /* 0x094: the player or monster it belongs to (EmEffectAttack_Init) */
    u8 unk098[0x09C - 0x098];
    struct Em *targetEm;        /* 0x09C: the monster it locked on or stuck to (found in range in attack_families.c), NULL: none */
    struct HitShape *targetPart;/* 0x0A0: the hit shape of targetEm it hit (ObjAttach_DamageEm): its bone and position */
    u8 unk0A4[0x0B4 - 0x0A4];
    u8 wasHit;                  /* 0x0B4: 1 once another object hit it (ObjAttach_HitObjects sets it and skips it after); PlacedItem_Run tests it */
    u8 hitId;                   /* 0x0B5: the attack id: an object is not hit twice by one id (compared with hitIds in ObjAttach_HitAll; stored there by the damage functions) */
    u8 unk0B6[0x0B8 - 0x0B6];
    u16 randBits;               /* 0x0B8: random bits from the owner stateRand (Em/Player), read by shifts (BowgunShot, BowArrow, TeostraBlastRing) */
    u8 unk0BA[0x0BC - 0x0BA];
    struct DrawSlot *drawSlotBuf;/* 0x0BC: the draw slot of drawSlot (DrawSlot_Get/DrawSlotB_Get(drawSlot)), NULL: none; drawn at its pos (EmImpactAttack_Run) */
    s16 drawSlot;               /* 0x0C0: the draw slot, -1: none (DrawSlot_Alloc/DrawSlotB_Alloc in the Setup functions) */
    u8 unk0C2[0x0C4 - 0x0C2];
    void *hitBy;                /* 0x0C4: set when another object hit it: gPlayers or gEms by the hitter's isEm, at its own idx (the game's indexing), NULL when the hitter's ownerIndex is 0xFF (ObjAttach_HitObjects) */
    f32 hitScale;               /* 0x0C8: the hit capsule scale: Capsule_FromPoints(pos, pos2, hitList, .., it) (1.0 for the object hit) */
    u16 yaw;                    /* 0x0CC: its yaw (the owner's rotation Y, Em/Player +0xA4, when made; +0x8000 turns it round) */
    u16 area;                   /* 0x0CE: the area it is in (the owner's +0x680); compared with the current area, gScene.unk014 */
    u8 hitKind;                 /* 0x0D0: the hitter's hitKind at the hit (ObjAttach_DamageEm), 0 on a player (ObjAttach_DamagePlayer); ObjAttach_HitEffects picks the effect by it */
    u8 side;                    /* 0x0D1: the owner side (Em/Player side, PlayerAttack_Create, Explosion_Create; copied to spawned objects): ObjAttach_DamagePlayer Side_Match(t->side, it) */
    u8 powerPct;                /* 0x0D2: nonzero: power is scaled by this percentage on a hit (ObjAttach_DamageEm, ObjAttach_DamagePlayer); the Explosion family sets 0xFA */
    u8 hitSpan;                 /* 0x0D3: LaoShanAttack type 0x47 (Lao-Shan's): twice its hitFrames (LaoShanAttack_CreateSpan's last argument; LaoShanAttack_Setup halves it); 0 from the other LaoShanAttack creators */
} ObjAttach;
typedef char ObjAttach_size_check[(sizeof(ObjAttach) == 0xD4) ? 1 : -1];

/*
 * The blocks of the object buffer (ObjAttach.buf) of kinds that more than one
 * file reads. A kind read in one file has its view in that file.
 */

/* BowgunShotParam.kind: how a bowgun shot flies (BowgunShot_Init and
 * BowgunShot_Fly switch on it). Kind 3 has no case. */
enum BowgunShotFlyKind {
    SHOT_FLY_STRAIGHT = 0,      /* straight, with random pushes */
    SHOT_FLY_STRAIGHT_B = 1,    /* the same flight */
    SHOT_FLY_SPRAY = 2,         /* sprayed from a bone in random directions */
    SHOT_FLY_PULSE = 4          /* a hit zone growing from the muzzle */
};

/* BowgunShotParam.hitKind: what a bowgun shot does when it hits
 * (BowgunShot_Fly and BowgunShot_Hit switch on it). */
enum BowgunShotHitKind {
    SHOT_HIT_END = 0,               /* ends at the hit */
    SHOT_HIT_STICK = 1,             /* sticks to the bone it hit for count frames */
    SHOT_HIT_FLOOR_BURST = 2,       /* bursts on the floor (EXPLOSION_BOWGUN_FLOOR) */
    SHOT_HIT_STICK_BURST_LV1 = 3,   /* sticks, then bursts (EXPLOSION_BOWGUN_BURST_LV1) */
    SHOT_HIT_STICK_BURST_LV2 = 4,   /* the same, level 2 */
    SHOT_HIT_STICK_BURST_LV3 = 5,   /* the same, level 3 */
    SHOT_HIT_RICOCHET = 6,          /* bounces off the part hit, count hits */
    SHOT_HIT_SCATTER = 7,           /* scatters into pellets at a hit or the floor */
    SHOT_HIT_PIERCE = 8,            /* goes through, count hits */
    SHOT_HIT_PASS = 9,              /* flies on through the hit */
    SHOT_HIT_SMOKE = 10             /* leaves a smoke screen at a hit or the floor */
};

/* A row of gBowgunShotParams (33 rows): an ammunition kind of the bowgun (BowgunShot: the object's type is the row). */
typedef struct BowgunShotParam {
    u8 kind;                    /* 0x00: how it flies (BowgunShot_Init and BowgunShot_Fly switch on it: 2 from a bone, scattering; 4 ...) */
    u8 model;                   /* 0x01: its mesh set of gCommonModels[0] (0x62: a spinning one; 0xFF: none) */
    s8 colorRow;                /* 0x02: the row of BowgunShot_Colors */
    u8 life;                    /* 0x03: the row of BowgunShot_StreakFrames (frames), 0xFF: none */
    s16 powerTbl;               /* 0x04: the row of BowgunShot_StageFrameTbls (power by distance), -1: none */
    u16 hasGlow;                /* 0x06: nonzero: BowgunShot_Fly starts and runs the glow */
    f32 speed;                  /* 0x08: the launch speed, times the level row's (BowgunShot_Init) */
    f32 speedRand;              /* 0x0C: the random part of the speed */
    f32 gravity;                /* 0x10: ObjAttach.gravity, negated */
    f32 gravityRand;            /* 0x14: the random part of the gravity */
    f32 wobbleY;                /* 0x18: kinds 0 and 1: a random vertical push, times the horizontal speed (BowgunShot_Fly) */
    f32 wobbleX;                /* 0x1C: kinds 0 and 1: a random sideways push in the first 20 frames, times the level row's +0x10 */
    s16 boostFrames;            /* 0x20: frames it still accelerates (ObjAttach_Move while ownerMotion is below it) */
    u8 hitKind;                 /* 0x22: BowgunShot_Fly and BowgunShot_Hit switch on it */
    u8 count;                   /* 0x23: the hits, or the row of BowgunShot_ScatterParams */
} BowgunShotParam;
typedef char BowgunShotParam_size_check[(sizeof(BowgunShotParam) == 0x24) ? 1 : -1];

/* The bowgun shot's glow: two camera-facing meshes behind the shot (BowgunShotBuf.glow; BowgunShot_GlowAlloc). */
typedef struct BowgunShotGlow {
    struct DrawSlot *slot;      /* 0x00: DrawSlot_Get(drawSlot), NULL: none */
    s16 drawSlot;               /* 0x04 */
    s16 unk06;                  /* 0x06 */
    s16 timer;                  /* 0x08: counted up by BowgunShot_GlowRun */
    s16 unk0A;                  /* 0x0A */
} BowgunShotGlow;

/* The bowgun shot's block (BowgunShot_Create, BowgunShot_Init and the shot's flight code). */
typedef struct BowgunShotBuf {
    u8 bounced;                 /* 0x00: 1 once BowgunShot_BounceStep ran (BowgunShot_DrawModel then stops turning it along its velocity) */
    u8 stage;                   /* 0x01: the entry of BowgunShot_StageFrameTbls[powerTbl] reached (BowgunShot_UpdatePowerStage), 0xFF: past the last */
    u8 flyStep;                 /* 0x02: BowgunShot_Fly switches on it */
    u8 nextHit;                 /* 0x03: the hit number (o->hitList = BowgunShot_HitShapes[it]), also counted down by BowgunShot_Fly */
    f32 drawPos[3];             /* 0x04: copied to the draw slot's pos (BowgunShot_Run, BowgunShot_Hit) */
    f32 power;                  /* 0x10: the object's power at set up (BowgunShot_ApplyStage scales it) */
    s16 glowFlashTimer;         /* 0x14: BowgunShot_GlowRun's frames, counted in stage 1: picks the flash sizes */
    u8 streakPhase;             /* 0x16: 0 the streak alone, 1 streak and mesh, 2 the mesh alone; BowgunShot_Fly, BowgunShot_Hit, BowgunShot_Deflect and BowgunShot_DrawModel switch on it */
    u8 glowState;               /* 0x17: 0 BowgunShot_Fly starts the glow, 1 it runs, 2 over */
    f32 attackFactor;           /* 0x18: the player's attackFactor */
    f32 scale;                  /* 0x1C: the charge scale (BowgunShot_Create's; BowgunShot_Scatter passes it on): the power is attackFactor times it */
    u8 *levelRow;               /* 0x20: the bowgun's level row of gGunLevelRows (24 bytes: f32 values at +4 to +0x14) */
    u8 level;                   /* 0x24: the bowgun's level (low 4 bits; GUN_OPT_SILENCER adds gSilencerShotSpeeds, GUN_OPT_BARREL gBarrelHeavyShotSpeeds or gBarrelLightShotSpeeds) */
    u8 skills;                  /* 0x25: BowgunShot_InitSkills: 1, 2, 4, 8 power skills; 0x10 range x2, 0x20 range x0.5 */
    u8 crit;                    /* 0x26: 1 a critical (Player_GetWeaponStat 8, by chance), 2 a weak hit */
    u8 isLightBowgun;           /* 0x27: 0 when the bowgun's Equip_GetClass is 1, else 1 */
    BowgunShotGlow glow;        /* 0x28 */
} BowgunShotBuf;

/*
 * Kushala Daora's effect 39's block (TeostraBlastRing_Setup, TeostraBlastRing_Run and
 * TeostraBlastRing_Draw). A piece spawns the next one
 * (TeostraBlastRing_SpawnNext, up to 7) at its origin when its first phase ends.
 */
typedef struct TeostraBlastRingBuf {
    u8 phase;                   /* 0x00: 0, 1, 2 (TeostraBlastRing_PhaseFrames[phase] frames each); TeostraBlastRing_Draw's model and colour keys */
    u8 pad01[2];
    u8 seDone;                  /* 0x03: 1 once phase 0's end played its sound (Em_PlaySeChecked 0x24) */
    s16 rotZ;                   /* 0x04: the model's turn about z (random at set up and when phase 2 starts) */
    s16 spin;                   /* 0x06: rotZ's step per frame in phase 2 */
    f32 rise;                   /* 0x08: pos[1]'s step per frame in phase 2 */
    f32 scale;                  /* 0x0C: the model's x and y scale (1.0 to 1.2) */
    f32 origin[3];              /* 0x10: where it was made (the owner's bone 1 for the first piece); the next piece spawns there */
} TeostraBlastRingBuf;
typedef char TeostraBlastRingBuf_size_check[(sizeof(TeostraBlastRingBuf) == 0x1C) ? 1 : -1];

#endif /* OBJATTACH_H */
