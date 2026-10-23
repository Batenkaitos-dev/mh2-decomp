#ifndef STAGEOBJ_H
#define STAGEOBJ_H

#include "common.h"
#include "model.h"
#include "sprite.h"

/*
 * An object of sub_main's third object list (StageObj_Alloc(nblk) allocates
 * one; StageObj_UpdateAll runs its update every frame), which the game
 * overlay uses for the objects that live in the stage. Its header up to 0x1F is laid out like Obj's.
 * The struct is generated from include/stageobj.spec
 * (tools/gen_struct.py): add fields there.
 */
struct StageObj;
/* An object update callback (StageObj_UpdateAll runs it every frame with the object). */
typedef void (*StageObjFn)(struct StageObj *o);

typedef struct StageObj {
    u8 active;                  /* 0x000: nonzero while the object is in use */
    u8 visible;                 /* 0x001 */
    u8 kind;                    /* 0x002: set by the creator (enum StageObjKind) */
    u8 type;                    /* 0x003: the variant within the kind (the updates switch on it) */
    u8 state;                   /* 0x004: the update's state */
    u8 var0;                    /* 0x005: per-kind scratch byte (counters, flags) */
    u8 var1;                    /* 0x006: per-kind scratch byte (a bone index for Em_GetBonePos, an angle << 8 ...) */
    u8 var2;                    /* 0x007: per-kind scratch byte (0xFF: none in many kinds; an angle << 8 ...) */
    s16 timer;                  /* 0x008: a frame counter */
    s16 angle;                  /* 0x00A: a 16-bit angle in most kinds (Vec_RotateY, Mtx_RotateY, MonsterFx_CreateAtYaw; yaw from Vec_DirToAngles); a bone index in some */
    struct StageObj *prev;      /* 0x00C */
    struct StageObj *next;      /* 0x010 */
    void *draw;                 /* 0x014 */
    u8 *buf;                    /* 0x018: its blocks of the object buffer (the kind's data) */
    u8 blk;                     /* 0x01C */
    u8 nblk;                    /* 0x01D: the block count given to StageObj_Alloc */
    u8 ownerIndex;              /* 0x01E: the owner's index (Player.index, Em.index; compared with gScene.localPlayer) */
    u8 keep;                    /* 0x01F: not freed by StageObj_FreeUnkept */
    StageObjFn update;          /* 0x020: run every frame (StageObj_UpdateAll) */
    f32 pos[3];                 /* 0x024: the position */
    f32 scale;                  /* 0x030: the size the creator gives (1.0f by default) */
    void *owner;                /* 0x034: the monster, player or object it follows (NULL: none), in some kinds */
    struct DrawSlot *node;      /* 0x038: its draw slot, DrawSlot_Get(slot), when the kind draws one (DrawSlot: include/sprite.h) */
    s16 slot;                   /* 0x03C: its draw slot (DrawSlot_Alloc; -1: none was free), freed by the end state (DrawSlot_Free) */
    u16 angle2;                 /* 0x03E: a second 16-bit angle (pitch from Vec_DirToAngles) in some kinds; an area in others */
} StageObj;
typedef char StageObj_size_check[(sizeof(StageObj) == 0x40) ? 1 : -1];

/* sub_main: allocates an object with nblk blocks of the object buffer (+0x18), NULL when none is free */
void StageObj_InitPool(void);
void StageObj_FreeUnkept(void);
StageObj *StageObj_Alloc(s32 nblk);
StageObj *StageObj_AllocTail(s32 blocks);
/* sub_main: frees an object (its blocks too) */
void StageObj_Free(StageObj *o);
void StageObj_UpdateAll(void);
void StageObj_DrawAll(void);
void StageObj_DrawKind4(void);

/*
 * StageObj.state: every stage object's update switches on it. The set-up
 * moves the object to STAGEOBJ_RUN, the run moves it to STAGEOBJ_END when it
 * is over, and the end frees its draw slots and moves it to STAGEOBJ_FREE,
 * where StageObj_Free returns it to the free list.
 */
enum StageObjState {
    STAGEOBJ_SETUP = 0,
    STAGEOBJ_RUN = 1,
    STAGEOBJ_END = 2,
    STAGEOBJ_FREE = 3
};

/*
 * StageObj.kind, which each family's creator sets. Nothing in the code
 * reads it back: the update callback picks the behaviour. Kinds 6,
 * 23 and 44 have creators whose families have no name yet; kinds 8,
 * 25, 30, 32-36, 38, 40 and 42 have no creator seen.
 */
enum StageObjKind {
    STAGEOBJ_KIND_IMPACT_ROCKS = 0x00,        /* the great sword's ground impact */
    STAGEOBJ_KIND_SHADOW = 0x01,              /* a shadow under a hunter or a monster (Shadow_Update) */
    STAGEOBJ_KIND_HIT_SPARK = 0x02,           /* a hit's spark (HitSpark_Update) */
    STAGEOBJ_KIND_DEMON_GLOW = 0x03,          /* the dual swords' demon mode glow */
    STAGEOBJ_KIND_THUNDER_SPARK = 0x04,       /* the Khezu's and the Kirin's electric sparks */
    STAGEOBJ_KIND_WEAPON_TRAIL = 0x05,        /* a blade's swing trail */
    STAGEOBJ_KIND_EM_BODY_FX = 0x07,          /* a monster's glow, cover or prop drawn on its bones */
    STAGEOBJ_KIND_EM_TAIL = 0x09,             /* a monster's severable tail */
    STAGEOBJ_KIND_LAOSHAN_DUST = 0x0A,        /* the Lao-Shan Lung's dust clouds */
    STAGEOBJ_KIND_PLAYER_HIT_BURST = 0x0B,    /* a burst on a hunter who was hit */
    STAGEOBJ_KIND_ITEM_PROP = 0x0C,           /* what an item leaves in the stage: smoke, bait, a dropped item, puffs */
    STAGEOBJ_KIND_PART_FX = 0x0D,
    STAGEOBJ_KIND_FIRE_BLAST = 0x0E,
    STAGEOBJ_KIND_ELEMENT_BURST = 0x0F,
    STAGEOBJ_KIND_HIT_SPLASH = 0x10,
    STAGEOBJ_KIND_ATTACK_BURST = 0x11,
    STAGEOBJ_KIND_SHOT_FLASH = 0x12,
    STAGEOBJ_KIND_ALERT_MARK = 0x13,
    STAGEOBJ_KIND_DUST_CLOUD = 0x14,
    STAGEOBJ_KIND_FALLING_DEBRIS = 0x15,
    STAGEOBJ_KIND_LURE = 0x16,                /* the fishing lure */
    STAGEOBJ_KIND_STUN_STARS = 0x18,
    STAGEOBJ_KIND_LOCK_ON_MARK = 0x1A,
    STAGEOBJ_KIND_MASH_DEBRIS = 0x1B,
    STAGEOBJ_KIND_MONSTER_FX = 0x1C,
    STAGEOBJ_KIND_AREA_FLYER = 0x1D,
    STAGEOBJ_KIND_GUN_FX = 0x1F,
    STAGEOBJ_KIND_VERSUS_BAIT = 0x25,
    STAGEOBJ_KIND_TEOSTRA_BLAST_CHARGE = 0x27,
    STAGEOBJ_KIND_PLAYER_FX = 0x29,
    STAGEOBJ_KIND_BOW_ARC = 0x2B
};

/* ImpactRocks' StageObj.type (ImpactRocks_Create's type): how high the rocks are thrown. */
enum ImpactRocksType {
    IMPACT_ROCKS_LOW = 0,       /* the great sword's impact */
    IMPACT_ROCKS_HIGH = 1       /* thrown 1.5 times higher; no caller passes it */
};
/* ImpactRocks' StageObj.var2 (ImpactRocks_Create's meshChoice): the rocks' mesh. */
enum ImpactRocksMesh {
    IMPACT_ROCKS_MESH_RANDOM = 0,   /* one of five rock meshes each; no caller passes it */
    IMPACT_ROCKS_MESH_DEBRIS = 1,   /* the mesh FallingDebris draws too; the great sword's */
    IMPACT_ROCKS_MESH_ROCK = 2      /* (any other value) the mesh AttackBurst's rock draws; no caller passes it */
};

/* ThunderSpark's StageObj.type: which sparks, by their creator. */
enum ThunderSparkType {
    SPARK_KHEZU_DISCHARGE = 0,      /* over the Khezu's body during its discharge (part 0 at bone 3, the others jump over ThunderSpark_BodyPoints) */
    SPARK_KHEZU_SHOT_FLASH = 1,     /* a flash where the Khezu's ceiling shot or a SPARK_RED_KHEZU_DROP lands */
    SPARK_KHEZU_SHOT_RISE = 2,      /* rising sparks with SPARK_KHEZU_SHOT_FLASH */
    SPARK_KHEZU_FAN_CHARGE = 3,     /* at the Khezu's head while it charges its three shots */
    SPARK_KHEZU_FAN_BURST = 4,      /* a burst at its head just before the three shots */
    SPARK_KHEZU_GROUND_BOLT = 5,    /* along the Khezu's bolt on the floor */
    SPARK_KIRIN_BOLT_SKY = 6,       /* above each spot a Kirin's lightning strikes */
    SPARK_KIRIN_BOLT_GROUND = 7,    /* on the floor at each strike */
    SPARK_KIRIN_CHARGE = 8,         /* at the Kirin's horn before its lightning */
    SPARK_RED_KHEZU_DROP = 9        /* a drop from the Red Khezu's head that falls and sparks on the floor */
};
#define SPARK_LOOK_LANDED 0xFF      /* ThunderSparkPart.look of a SPARK_RED_KHEZU_DROP that has landed */
/* ThunderSparkPart.sorted: how a part is drawn. */
enum ThunderSparkDraw {
    SPARK_DRAW_ADD = 0,             /* additive, in the gDrawOt32 list */
    SPARK_DRAW_SORTED = 1,          /* alpha-blended, in gDrawOtSorted */
    SPARK_DRAW_SORTED_DIM = 2       /* as SPARK_DRAW_SORTED at half alpha and 0.8 the size (SPARK_KHEZU_DISCHARGE before the hit) */
};

/* WeaponTrail's StageObj.type: whose blade it follows and its colour. */
enum WeaponTrailType {
    WTRAIL_GREAT_SWORD = 0,         /* the great sword's swings; the area's effect colour */
    WTRAIL_BLADE = 1,               /* the sword and shield, and the dual swords' first blade; white */
    WTRAIL_BLADE_B = 2,             /* the dual swords' second blade; refused in demon mode */
    WTRAIL_DEMON = 3,               /* the dual swords' demon mode: both blades, red, made once and kept */
    WTRAIL_BOW = 4,                 /* the bow's melee slashes: on the player's PL_BONE_OTHER_HAND, turned half round */
    WTRAIL_LONG_SWORD = 5,          /* the long sword's slashes, reddening with its spirit gauge */
    WTRAIL_LONG_SWORD_SPIRIT = 6    /* the long sword's spirit slashes (or WTRAIL_LONG_SWORD at a full gauge): two trails, red */
};
#define WTRAIL_LIFE_FOREVER (-1)    /* WeaponTrail_Create's life: no countdown and no motion check (WTRAIL_DEMON) */

/* EmBodyFx's StageObj.type: the monster and what is drawn on its bones. */
enum EmBodyFxType {
    EMBODYFX_KIRIN_FLASH = 0,       /* the Kirin's flashes while enraged */
    EMBODYFX_KIRIN_GLOW = 1,        /* the Kirin's pulsing glow while enraged */
    EMBODYFX_KIND38_CHARGE = 2,     /* monster kind 56's charge glow */
    EMBODYFX_SHAKALAKA_COVER = 3,   /* a hiding Shakalaka's cover (its mesh by area), thrown off as it springs out */
    EMBODYFX_SHAKALAKA_PROP = 4,    /* a prop on a Shakalaka's bone, with a flame */
    EMBODYFX_YAMATSUKAMI_RINGS = 5  /* three rings around the second Yama Tsukami kind */
};

/* EmTail's StageObj.type and var0. */
enum EmTailStage {
    EMTAIL_ON_BODY = 0,             /* on the monster (type) */
    EMTAIL_CUT = 1                  /* cut off and lying (type) */
};
enum EmTailPhase {
    EMTAIL_WAIT = 0,                /* on the monster, waiting for its part-break reaction */
    EMTAIL_CARVABLE = 1,            /* lying, with a carve point */
    EMTAIL_LYING = 2                /* lying, carved out or with no point */
};
#define EMTAIL_NO_POINT 0xFF        /* StageObj.var2 of a tail with no carve point */

/* ItemProp's StageObj.type: what the item left. */
enum ItemPropType {
    ITEMPROP_MEAT = 0,              /* a meat model on a monster's bone; no creator passes it */
    ITEMPROP_SMOKE = 1,             /* a smoke screen that blocks the monsters' sight */
    ITEMPROP_UNUSED = 2,            /* freed at once by the set-up */
    ITEMPROP_BAIT = 3,              /* a meat or bait put down */
    ITEMPROP_DROPPED = 4,           /* a carried item let go */
    ITEMPROP_CLOUD = 5,             /* a thrown item's rising cloud */
    ITEMPROP_EAT_PUFF = 6           /* the puff of eating */
};

/* The families' own records, in their objects' buffers (StageObj.buf) or their tables. */
/* One of ImpactRocks' nine rocks, in the object's buffer (56 bytes). */
typedef struct ImpactRock {
    s16 mesh;           /* 0x00: which of the five rock meshes (IMPACT_ROCKS_MESH_RANDOM) */
    s16 slot;           /* 0x02: its draw slot */
    f32 pos[3];         /* 0x04 */
    f32 vel[3];         /* 0x10 */
    f32 acc[3];         /* 0x1C */
    s32 rot[3];         /* 0x28: angles, ANGLE_TURN to a turn */
    struct DrawSlot *node; /* 0x34 */
} ImpactRock;
/* WeaponTrail's buffer: the blade's matrices of the last six frames (hist[0] the newest) for up to two trails, and each trail's draw slot. */
typedef struct WeaponTrailBuf {
    f32 hist[6][2][16]; /* 0x000 */
    struct DrawSlot *node[2]; /* 0x300 */
    s16 slot[2];        /* 0x308 */
} WeaponTrailBuf;
/* EmBodyFx's buffer (only EMBODYFX_KIND38_CHARGE, _SHAKALAKA_COVER and _YAMATSUKAMI_RINGS have one). */
typedef struct EmBodyFxBuf {
    f32 m[16];          /* 0x00: a copy of the bone matrix the cover sits on (EMBODYFX_SHAKALAKA_COVER) */
    f32 ofs[3];         /* 0x40: a scratch offset (EMBODYFX_YAMATSUKAMI_RINGS) */
    s16 slot[3];        /* 0x4C: three draw slots (the charge's three parts, the three rings) */
    u8 pad52[0x54 - 0x52];
    struct DrawSlot *node[3]; /* 0x54 */
} EmBodyFxBuf;
/* One blade's glow of DemonGlow, in the object's buffer (24 bytes). */
typedef struct DemonGlowBlade {
    s16 side;           /* 0x00: 0 the left blade (PL_BONE_HAND), 1 the right (PL_BONE_OTHER_HAND) */
    s16 slot;           /* 0x02: its draw slot (the kept pool, DrawSlotB) */
    struct DrawSlot *node; /* 0x04 */
    f32 pos[3];         /* 0x08 */
    s16 blink;          /* 0x14: toggled every frame; drawn while bit 0 is set */
    u8 pad16[0x18 - 0x16];
} DemonGlowBlade;
/* DemonGlow's buffer: the two blades (DemonGlow_BladeCounts[type]) and the sword. */
typedef struct DemonGlowBuf {
    DemonGlowBlade blade[2]; /* 0x00 */
    s16 sword;          /* 0x30: the owner's dual sword index (Player_DualSwordIndex): the row of the offset and scale tables */
} DemonGlowBuf;
/* One of LaoshanDust's two clouds, in the object's buffer (48 bytes). */
typedef struct LaoshanDustPiece {
    s16 slot;           /* 0x00: its draw slot */
    s16 side;           /* 0x02: 0 or 1; the offset's x is mirrored for 1 */
    f32 base[3];        /* 0x04: the floor under the bone; the node goes at base + the keyed offset */
    f32 keyScale[3];    /* 0x10: the scale keys' value */
    f32 alpha;          /* 0x1C: the alpha keys' value */
    f32 size;           /* 0x20: the creator's scale */
    struct DrawSlot *node; /* 0x24 */
    s16 frame;          /* 0x28: frames since the start; drawn from 1 to the piece's end */
    u16 spin;           /* 0x2A: the angle about the view axis */
    u16 spinSpeed;      /* 0x2C: added to spin each frame (random, -128 to 127) */
    u8 pad2E[0x30 - 0x2E];
} LaoshanDustPiece;
/* A row of LaoshanDust_Bones (16 bytes): the bone the dust starts at and an offset added to every piece's. */
typedef struct LaoshanDustBone {
    s16 bone;           /* 0x00 */
    s16 pad;
    f32 ofs[3];         /* 0x04 */
} LaoshanDustBone;
/* One part of a ThunderSpark, in the object's buffer (60 bytes; ThunderSpark_Setup sets them up by type). */
typedef struct ThunderSparkPart {
    s16 slot;           /* 0x00: its draw slot */
    s16 look;           /* 0x02: picks the mesh, the colour keys and the restart (starts as the part's index); SPARK_LOOK_LANDED */
    f32 pos[3];         /* 0x04 */
    s16 frame;          /* 0x10: the part's frame, from 0 or a negative start delay: the time of its key curves */
    u16 rot[3];         /* 0x12: angles (z: the spin about the view axis) */
    f32 scale;          /* 0x18 */
    struct DrawSlot *node; /* 0x1C */
    f32 keyScale[3];    /* 0x20: the scale keys' value, multiplied into scale */
    u8 sorted;          /* 0x2C: how it is drawn, enum ThunderSparkDraw */
    u8 point;           /* 0x2D: the row of ThunderSpark_BodyPoints it sits on (ThunderSpark_PickBodyPoints) */
    u8 alpha;           /* 0x2E */
    u8 rise;            /* 0x2F: SPARK_KHEZU_SHOT_RISE: the height it rises over its life */
    f32 vel[3];         /* 0x30: SPARK_RED_KHEZU_DROP: the drop's velocity */
} ThunderSparkPart;
/* A row of DemonGlow's per-sword tables: an offset from the hand bone (x out from it, z along the blade) or the glow's two scales (width, length). */
typedef struct DemonGlowPair {
    f32 x;
    f32 z;
} DemonGlowPair;
/* A point on the Khezu's body for SPARK_KHEZU_DISCHARGE (ThunderSpark_BodyPoints): a bone, an offset in it and the spark's size there. */
typedef struct ThunderSparkPoint {
    s16 bone;           /* 0x00 */
    s16 pad;
    f32 ofs[3];         /* 0x04 */
    f32 scale;          /* 0x10 */
} ThunderSparkPoint;

/* a part of a MashDebris object (STAGEOBJ_KIND_MASH_DEBRIS) (64 bytes each, in its blocks) */
typedef struct MashDebrisPiece {
    u16 rot[3];                 /* 0x00 */
    s16 t;                      /* 0x06: its frame */
    f32 fall;                   /* 0x08: its fall speed */
    f32 scale[3];               /* 0x0C: from a curve by its frame */
    f32 pos[3];                 /* 0x18 */
    f32 vel[3];                 /* 0x24 */
    u8 model;                   /* 0x30: its model of the common set */
    u8 alpha;                   /* 0x31 */
    s16 life;                   /* 0x32: its frames */
    f32 ground;                 /* 0x34: type 0: a floor height */
    struct DrawSlot *node;      /* 0x38 */
    s16 slot;                   /* 0x3C */
    u8 unk3E[0x40 - 0x3E];
} MashDebrisPiece;

/* One ring of ITEMPROP_SMOKE's smoke, in its buffer (52 bytes; ItemProp_SetupSmoke sets four up,
 * ItemProp_RunSmoke places them in front of the camera, ItemProp_DrawSmoke draws them). */
typedef struct ItemPropRing {
    s16 slot;           /* 0x00 */
    s16 view;           /* 0x02: 0 drawn round the camera (the one seen from inside), else the ring's number + 1 */
    u8 unk04[0x14 - 0x04];
    f32 alpha;          /* 0x14: its opacity, 0 to 1 (timer / 120 while it fades in or out); ItemProp_DrawSmoke draws 255 times it */
    u8 unk18[0x1C - 0x18];
    s16 unk1C;          /* 0x1C */
    u16 spin;           /* 0x1E: its angle about the view axis, turned by ItemProp_SmokeRingSpins[ring] each frame */
    struct DrawSlot *node; /* 0x20 */
    u8 unk24[0x34 - 0x24];
} ItemPropRing;
typedef char ItemPropRing_size_check[(sizeof(ItemPropRing) == 0x34) ? 1 : -1];

/* ITEMPROP_DROPPED's buffer: the carried object (ItemProp_CreateAtPlayer sets it up from
 * the player's PL_BONE_HAND; ItemProp_RunDropped drops it, ItemProp_DrawDropped draws it). */
typedef struct ItemPropCarry {
    f32 mtx[16];        /* 0x00 */
    u8 pad40[0x4C - 0x40];
    u8 kind;            /* 0x4C: the player's carry >> 4: picks its scale, rotation and offset, and its model */
} ItemPropCarry;
typedef char ItemPropCarry_size_check[(sizeof(ItemPropCarry) == 0x50) ? 1 : -1];

void ImpactRocks_Setup(StageObj *obj);
void ImpactRocks_Run(StageObj *obj);
void ImpactRocks_End(StageObj *obj);
void ImpactRocks_Free(StageObj *obj);
void ImpactRocks_DrawRock(DrawSlot *node);
void ImpactRocks_Create(void *owner, u8 type, f32 *pos, u8 meshChoice);
void DemonGlow_Setup(StageObj *obj);
void DemonGlow_Run(StageObj *obj);
void DemonGlow_End(StageObj *obj);
void DemonGlow_Free(StageObj *obj);
void DemonGlow_DrawBlade(DrawSlot *node);
void ThunderSpark_Setup(StageObj *obj);
void ThunderSpark_Run(StageObj *obj);
void ThunderSpark_End(StageObj *obj);
void ThunderSpark_Free(StageObj *obj);
void ThunderSpark_DrawPart(DrawSlot *node);
void ThunderSpark_PickBodyPoints(StageObj *obj, ThunderSparkPart *part);
void ThunderSpark_RestartFanCharge(StageObj *obj, ThunderSparkPart *part);
void ThunderSpark_RestartKirinCharge(StageObj *obj, ThunderSparkPart *part);
void ThunderSpark_TurnToCamera(f32 *mtx, void *pos);
void ThunderSpark_CreateAt(void *pos, u8 type, u8 emKind, f32 scale);
void WeaponTrail_Setup(StageObj *obj);
void WeaponTrail_Run(StageObj *obj);
void WeaponTrail_End(StageObj *obj);
void WeaponTrail_Free(StageObj *obj);
void WeaponTrail_Draw(DrawSlot *node);
void WeaponTrail_GetBladeOffset(f32 *out, StageObj *obj, void *owner, s16 part);
StageObj *WeaponTrail_Create(void *owner, s32 life, s32 type);
void EmBodyFx_Setup(StageObj *obj);
void EmBodyFx_Run(StageObj *obj);
void EmBodyFx_End(StageObj *obj);
void EmBodyFx_Free(StageObj *obj);
void EmBodyFx_Draw(DrawSlot *node);
void EmTail_Setup(StageObj *obj);
void EmTail_AddCarvePoint(StageObj *obj);
void EmTail_Drop(StageObj *obj);
void EmTail_Run(StageObj *obj);
void EmTail_End(StageObj *obj);
void EmTail_Free(StageObj *obj);
void EmTail_Draw(DrawSlot *node);
void LaoshanDust_Setup(StageObj *obj);
void LaoshanDust_Run(StageObj *obj);
void LaoshanDust_End(StageObj *obj);
void LaoshanDust_Free(StageObj *obj);
void LaoshanDust_Draw(DrawSlot *node);
void PlayerHitBurst_Create(void *pl);

/*
 * The effects' key tracks: arrays of keys in rising frame order, ended by a
 * key whose frame is -1. The readers give the value at a frame by linear
 * interpolation between the two keys around it, a key's own value at its
 * frame, and the last key's value past the end.
 * - FloatKey: one value (Key_LerpFloat), mostly an alpha.
 * - Vec2Key: two values (Key_LerpVec2).
 * - Vec3Key: a vector (Key_LerpVec3), mostly a scale per axis or an offset.
 * - ColorKey: a colour (Color_EvalKeys), each channel interpolated.
 * The readers take the track as plain floats or words (f32 *, s32 *): the
 * tables that list tracks of several kinds hold them as void *.
 */
typedef struct FloatKey {
    f32 frame;                  /* the key's frame; -1.0f ends the track */
    f32 value;
} FloatKey;

typedef struct Vec2Key {
    f32 frame;                  /* the key's frame; -1.0f ends the track */
    f32 value[2];
} Vec2Key;

typedef struct Vec3Key {
    f32 frame;                  /* the key's frame; -1.0f ends the track */
    f32 value[3];
} Vec3Key;

typedef struct ColorKey {
    s32 frame;                  /* the key's frame; -1 ends the track */
    u32 color;                  /* 0xAARRGGBB */
} ColorKey;

void Key_LerpVec3(f32 *keys, f32 *out, f32 time);
void Key_LerpFloat(f32 *keys, f32 *out, f32 time);
void Color_EvalKeys(s32 *keys, s32 time, u32 *out);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 rotOrder, f32 *mtx);
s32 Model_DrawFade(void *set, f32 *mtx, u16 flags, void *mats, f32 alpha);
s32 Effect_DrawModel(void *set, f32 *mtx, s32 color, u16 flags, void *mats);
s32 Effect_DrawModelAdd(void *set, f32 *mtx, void *mats);
void Effect_GetColor(s32 area, u8 *red, u8 *green, u8 *blue, u8 *alpha);
s32 Scene_CanPlaceKind3(void);
s32 Lure_PlayerHasMonsterBait(void *pl);
s16 Player_DualSwordIndex(void *obj);
void Effect_ScaleVec(f32 *vec);
f32 Effect_Scale(void);
s32 Effect_OwnerColorRow(s32 ownerIndex);
f32 Rand_Unit(void);
f32 Rand_Signed(void);
s16 Effect_MapTexId(s16 id);

#endif /* STAGEOBJ_H */
