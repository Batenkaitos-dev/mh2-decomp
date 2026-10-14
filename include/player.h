#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include "item.h"
#include "motion.h"

/* A player's copy of its pad input (0x24 bytes at Player + 0x36C), filled
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
    u16 stickDir;           /* 0x14: the left stick's direction as pad bits (0x3C00; gPadState's dir) */
    u16 prevStickDir;       /* 0x16 */
    u16 stickDirPressed;    /* 0x18: stickDir and not prevStickDir */
    u16 prevStickDirPressed; /* 0x1A */
    u16 stickAngle;         /* 0x1C: the left stick's angle, 0x10000 a turn (gPadState's degL; the
                             * hunting horn, qnet_sync adds the camera's yaw) */
    u16 prevStickAngle;     /* 0x1E */
    u16 stickTilt;          /* 0x20: the left stick's tilt (gPadState's magL; >= 0x28 counts as held) */
    u16 prevStickTilt;      /* 0x22 */
} PlayerInput;

/* A player's position received over the network, applied when the timer
 * runs out (Player.pend, qnet_sync). */
typedef struct PlayerPend {
    s16 timer;
    s16 unk2;
    f32 pos[3];
} PlayerPend;

struct Player; /* declared before PlayerVt, so that its handlers take the same struct (host compilers scope a tag first seen in a parameter list to it) */

/* The player's handlers (Player.handlers; the layout of EmVt's first entries). */
typedef struct PlayerVt {
    void (*unk0)(struct Player *p);
    void (*unk4)(struct Player *p);
    void (*unk8)(struct Player *p);
    void (*unkC)(struct Player *p);
} PlayerVt;

/* A player part's record (0xB0 bytes): Player.partRecs[i] points at the 32 of each player in
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

/* The state of a swinging equipment part's bone chain (0x20 bytes; four of them, one per chain
 * of the effect list: Player.equipFx, the equipment preview's at gEquipPreview + 0x28).
 * EquipFx_ChainUpdate moves the chain's bones with it; FxBlock_Clear clears the four. */
typedef struct EquipFxSwing {
    u8 calls;                   /* 0x00: the updates so far, counted to 2: the chain moves from the third */
    u8 unk01[3];                /* 0x01: cleared with it (FxBlock_Clear) */
    f32 vel[2][3];              /* 0x04: per bone of the chain, its swing velocity (damped by gEquipFxDamps) */
    u8 pad1C[4];
} EquipFxSwing;
typedef char EquipFxSwing_size_check[(sizeof(EquipFxSwing) == 0x20) ? 1 : -1];

/*
 * A player (0x970 bytes); sub_main's gPlayers holds one per player slot
 * right after the monsters (include/em.h), whose
 * first fields have the same layout. The struct is generated from
 * include/player.spec (tools/gen_struct.py): add fields there.
 */
typedef struct Player {
    u8 active;                  /* 0x000: nonzero while the slot is in use */
    u8 visible;                 /* 0x001: nonzero while drawn: checked with active before the draw calls (DrawCb_CharMake, Hud_DrawPlayerNames); Player_Reset sets 1 (as Em.visible) */
    u8 weaponType;              /* 0x002: the weapon type: 0 great sword, 1 heavy bowgun, 2 hammer, 3 lance, 4 sword and shield, 5 light bowgun, 6 dual swords, 7 long sword, 8 hunting horn, 9 gunlance, 10 bow (21 coating slots) */
    u8 isGunner;                /* 0x003: 1: a gunner (bowgun: ammo, no sharpness) */
    u8 unk004;                  /* 0x004: with step, stateVar0, stateVar1: cleared together (Player_ClearActionState) */
    u8 step;                    /* 0x005: as Em.step: the step within the state (counted up by the states), cleared on every state change (Player_ClearActionState) */
    u8 stateVar0;               /* 0x006: as Em.stateVar0: per-state scratch byte (a counter to 5 in state 0/0x27), cleared on every state change */
    u8 stateVar1;               /* 0x007: as Em.stateVar1: per-state scratch byte (a "hit once" flag with hitPending; a countdown in the lobby states) */
    s32 timer;                  /* 0x008: as Em.timer: per-state counter (a "hit once" flag in unk_6AF280.c, a countdown from 10 in the lobby states) */
    u16 index;                  /* 0x00C: the slot in gPlayers (== gScene.localPlayer, gNetUsers[index]; as Em.index) */
    u16 targetYaw;              /* 0x00E: heading to turn to (as Em.targetYaw): set from rotY when a hit turns the player (Player_ReactToHit) */
    u8 isEm;                    /* 0x010: 0 for a hunter: copied to the effects' isEm (Eff09_Create); the Melynx steal checks it is 0 */
    u8 gender;                  /* 0x011: the hunter gender (from the save data) */
    u8 weaponOut;               /* 0x012: nonzero while the weapon is drawn (set per weapon type by state 0/04; the idle input branches on it) */
    u8 unk013[0x014 - 0x013];
    u8 stateGroup;              /* 0x014: the state group (as Em.stateGroup; Player_RunState switches on it) */
    u8 state;                   /* 0x015: the state within the group (Player_IsState; the group dispatchers switch on it) */
    u8 prevStateGroup;          /* 0x016: the previous stateGroup */
    u8 prevState;               /* 0x017: the previous state */
    u8 hitKind;                 /* 0x018: a hit to react to (func_006B5710 and its copies): 1 a state change, 2 and 3 a flinch */
    u8 hitPending;              /* 0x019: nonzero: hitKind is to be handled */
    u8 unk01A[0x01C - 0x01A];
    u8 roundsLoaded;            /* 0x01C: bowgun: rounds loaded */
    u8 clipSize;                /* 0x01D: bowgun: clip size */
    u8 unk01E;                  /* 0x01E */
    u8 unk01F[0x020 - 0x01F];
    f32 rotMtx[16];             /* 0x020: the rotation matrix from rot (Mtx_FromAnglesXYZ, Player_Reset; as Em.rotMtx) */
    f32 worldMtx[4][4];         /* 0x060: as Em.worldMtx (the hunters states turn vectors with it, Vec_Rotate) */
    s32 rot[3];                 /* 0x0A0: rotation angles as integers, 0x10000 per turn; rot[1] the heading (as Em.rotX..rotZ) */
    f32 pos[3];                 /* 0x0AC: position (as Em) */
    f32 scale[3];               /* 0x0B8: set to 1.0 by Player_Reset (as Em.scaleX..scaleZ) */
    u8 unk0C4[0x0D0 - 0x0C4];
    struct PlRec *partRecs[32]; /* 0x0D0: the 32 records of 0xB0 bytes in the 0x1600-byte block at 0x5DE5D0 (Player_InitPartTable, CamScript_AtPoint) */
    u8 unk150[0x154 - 0x150];
    MotionLayer motion[4];      /* 0x154: motion layers (include/motion.h; as Em.motion): Player_SetLayerMotion starts one, Player_UpdateMotion steps it */
    u16 yawCenter;              /* 0x294: as Em.yawCenter: the heading targetYaw is kept within 0x4000 of (Player_UpdateLean, Player_State0_36) */
    u8 unk296[0x29A - 0x296];
    s16 msgTimer;               /* 0x29A: as Em.msgTimer (cleared by Player_State0_53) */
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
    u8 unk30F[0x310 - 0x30F];
    u8 unk310;                  /* 0x310 */
    s8 unk311;                  /* 0x311 */
    EquipItem equip[7];         /* 0x312: 0, 2-5: armour; 6: the weapon (type 7: a bowgun); 3: the torso */
    u8 armourKind[6];           /* 0x366: per armour piece a kind: sub_main copies it from the lobby record colour[] and the model setup (Look_MapPartId) reads it; main armour rattle sound picks from it */
    PlayerInput input;          /* 0x36C: the pad input (Player_CopyPadInput) */
    u8 action;                  /* 0x390 */
    u8 unk391;                  /* 0x391 */
    u8 unk392;                  /* 0x392 */
    u8 unk393[0x395 - 0x393];
    u8 hitReceived;             /* 0x395: nonzero: a hit taken, to react to (Player_ReactToHit; as Em.hitReceived) */
    u8 flinchPart;              /* 0x396: as Em.flinchPart: set to 0 at a hit (ObjAttach_DamagePlayer) */
    u8 padSwapLR;               /* 0x397: bit 0: replayed pad input swaps L1 and R1 (buttons 0x400 and 0x800; Pad_ReadButtons) */
    u32 flags;                  /* 0x398: flag bits: Player_SetFlag/ClearFlag/TestFlag without bit 31 (stateFlags at 0x39C with it); the state resets keep 0x100100 or 0x80100 (sub_main unk_3AAD20) */
    u32 stateFlags;             /* 0x39C: flags (Player_SetFlag, Player_ClearFlag, Player_TestFlag), cleared on every state change (Player_SetState) */
    u16 unk3A0;                 /* 0x3A0 */
    u16 stateRand;              /* 0x3A2: random value drawn on each state change */
    s32 motionTime;             /* 0x3A4: the motion time (Player_UpdateMotionTime adds len - blend, Player_TickTimers 2; wraps at 100000) */
    u8 unk3A8[0x3AC - 0x3A8];
    s32 targetAngle;            /* 0x3AC: the heading from target, plus a quarter turn (func_0070E1E0) */
    u8 unk3B0[0x3B4 - 0x3B0];
    void *target;               /* 0x3B4: the Em it deals with: the monster in battle with it that set inBattle (func_0070E1E0), the one it hit (ObjAttach_DamagePlayer), in the lobby the townsperson it talks to */
    f32 vel[3];                 /* 0x3B8: velocity */
    f32 accel[3];               /* 0x3C4: acceleration */
    struct PlayerVt *handlers;  /* 0x3D0: the player handlers, gPlayerHandlers (Player_Init) */
    u8 hitDelay;                /* 0x3D4: frames before the player's own hit window opens (as ObjAttach.hitDelay): Player_UpdateHitTimers counts it down in hitPhase 1, a motion blend adds to it (unk_3AC9C0.c) */
    u8 hitFrames;               /* 0x3D5: frames the hit window stays open (as ObjAttach.hitFrames), counted down in hitPhase 2 (Player_UpdateHitTimers) */
    u8 unk3D6[0x3EC - 0x3D6];
    u8 netActive;               /* 0x3EC: the lobby: set when a town message moves or places the player (TownMsg_OnActor, TownMsg_OnMove, TownMsg_OnArea) */
    u8 unk3ED[0x3EF - 0x3ED];
    u8 hitAttrs;                /* 0x3EF: attribute bits of the attacks taken, ORed in from the attack object +0x65 (ObjAttach_DamagePlayer): 0x80 cannot be guarded without skill 0x29 or with the heavy bowgun (Player_CanGuardHit) */
    u16 hitAngle;               /* 0x3F0: direction of the last hit taken (ObjAttach_SetHitHeading; Player_CanGuardHit; as Em.hitAngle) */
    u8 hitForce;                /* 0x3F2: the force of a hit taken while guarding (func_00734920) */
    u8 unk3F3[0x3F4 - 0x3F3];
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
    u8 fxMade;                  /* 0x416: 1 once the state made its effect (StageObj_K05_Create) at its frame: LongSword_State00, LongSword_State07 (as Em.unk416) */
    u8 unk417;                  /* 0x417 */
    u8 unk418[0x420 - 0x418];
    u8 *hitTimers;              /* 0x420: the hit window timers in use: &hitDelay (Player_Reset), or the hitDelay of its attack object that hit (ObjAttach_LinkTarget) */
    f32 hitPos[3];              /* 0x424: where the last hit taken landed (ObjAttach_DamagePlayer) */
    u16 unk430;                 /* 0x430 */
    s16 noFlinchTimer;          /* 0x432: the attack states set it (0x3C, 0x28, 2...), Player_TickTimers counts it down; while nonzero Player_IgnoresFlinch (pouch.c) returns 1 and Player_ReactToHit (unk_733EA0.c) skips the flinch */
    u8 unk434;                  /* 0x434 */
    u8 unk435[0x437 - 0x435];
    u8 fieldCollide;            /* 0x437: 1: the player collides with the walls (as Em.fieldCollide): Player_RunState and TownPc_Spawn set 1; Player_State0_33..35, TownPc_State19 and Contest_Open clear it */
    u8 unk438[0x4B8 - 0x438];
    u16 statusIcons;            /* 0x4B8: HUD status icon flags (Player_UpdateStatusIcons; 0x40, 0x400, 0x800 tested by the HUD and the map) */
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
    struct Model *weaponModel;  /* 0x4F4: the model: a 0x80-byte header of the pool at 0x6AE000 (Player_LoadWeaponModel) */
    s16 weaponModelSlotCount;   /* 0x4F8: model slots used by the weapon model (Player_AllocWeaponModel) */
    s16 weaponModelSlot;        /* 0x4FA: the first model slot of the weapon model (Player_AllocWeaponModel, Player_FreeWeaponModel) */
    struct Model *partModels[6];/* 0x4FC: parts 1-5 (the equipment): the model (Model_Get of its first slot; Player_LoadPartModels); [0] unused, the body's is model */
    s16 partModelCount[6];      /* 0x514: parts 1-5: model slots used */
    s16 partModelFirst[6];      /* 0x520: parts 1-5: the first model slot */
    struct DrawSlot *drawSlot;  /* 0x52C: its draw slot: an entry of the pool at 0x6A5AD0 (DrawSlot_Get), drawn by DrawCb_LocalPlayer (Player_InitAll) */
    s16 drawSlotId;             /* 0x530: its index, -1: none */
    u8 painted;                 /* 0x532: the Em paint mark read through Player (as Em.painted): Player_TickPaint runs only for isEm objects; Player_Reset sets 0xFF (players: never); Hud_GlowCond_Unk tests it */
    u8 carry;                   /* 0x533: low nibble: the kind of the carried item (Player_UpdateCarryKind, Item_CarryKind) */
    u8 ammoType;                /* 0x534: kind of the loaded ammo (ItemData.ammoKind) */
    u8 savedAmmoType;           /* 0x535: ammoType saved while the ammo menu is open (Player_BowgunAmmoSwitch) */
    u8 ammoMenuOpen;            /* 0x536: 1 while the ammo menu is open (Player_BowgunAmmoSwitch) */
    u8 spotCheckDelay;          /* 0x537: nonzero: no action spot check (Player_CheckActionSpot sets 20; Player_UpdateStatus counts it down) */
    u16 startAngle;             /* 0x538: its heading: gStartAngle, the action spot angle + 0x4000 */
    s16 paintTimer;             /* 0x53A: frames until painted is cleared (as Em.paintTimer; Player_TickPaint) */
    u8 unk53C[0x568 - 0x53C];
    f32 prevPos[3];             /* 0x568: the position before the move of this frame (as Em.prevPosX..prevPosZ; TownPc_Update copies pos) */
    f32 floorY;                 /* 0x574: the floor height under the player (Player_Update; Player_Reset sets pos[1]; as Em.floorY) */
    u8 unk578[0x580 - 0x578];
    u32 hairColor;              /* 0x580: the hair colour (HairMenu_Apply, HunterCard_Build, Player_SetupFromSave) */
    u8 unk584[0x588 - 0x584];
    u8 rideState;               /* 0x588: 0 none, 1 standing on the monster rideEm, 3 landed on it from a jump, 2 leaving (Em_CarryRiders sets 1/3, the states set 2/0) */
    u8 rideEm;                  /* 0x589: the slotNo of the monster the player stands on (Em_CarryRiders, func_0070E370) */
    u8 rideBone;                /* 0x58A: the bone of the monster rideEm the player stands on (Em_CarryRiders: Em_PosInFrame with it) */
    u8 lookVariant;             /* 0x58B: the look variant (0..3) from the colour (Player_BuildPartIds: gFaceBodyTypeMale/gFaceBodyTypeFemale by gender): picks the head model (Look_MapPartId k) and the texture set (+ gender * 4) */
    s16 unk58C;                 /* 0x58C */
    s16 unk58E;                 /* 0x58E */
    u8 unk590[0x594 - 0x590];
    s16 hitSlowTimer;           /* 0x594: from the attack object +0x70 (ObjAttach_DamageEm): while > 0 the motion runs at 0.2 (Player_UpdateMotion) and some attacks wait */
    u8 pushPriority;            /* 0x596: compared between two bodies to pick the one pushed (PlayerEm_PushApart, EmEm_PushApart); Player_Reset sets 1 */
    u8 unk597[0x598 - 0x597];
    s16 swordGaugeTimer;        /* 0x598: long sword: frames left after the gauge fills (900; LongSword_AddGauge) */
    u8 presetEquip;             /* 0x59A: nonzero: preset equipment, from the tables at 0x44AED0 (Player_InitForQuest) */
    u8 unk59B[0x59C - 0x59B];
    f32 rideDelta[3];           /* 0x59C: how far the monster carried the player this frame (Em_CarryRiders; func_0070E370 compares its length) */
    EquipFxSwing equipFx[4];    /* 0x5A8: the swinging parts of the equipment (Players_RunEquipEffects, Effect_RunList); cleared by Player_ClearFxBlock */
    s8 attackDrug;              /* 0x628: attack bonus of the drugs (Item_Use: at least 3 or 5), kept in gScene.mealAttack (Player_RecomputeStats) */
    s8 attackBuff;              /* 0x629: the timed attack buff (10, 25...; Item_Use, Item_UseTeamPowder) */
    s16 attackBuffTimer;        /* 0x62A: duration of attackBuff */
    s8 defenseDrug;             /* 0x62C: defense bonus of the drugs (Item_Use: at least 6 or 10), kept in gScene.mealDefense */
    s8 defenseBuff;             /* 0x62D: the timed defense buff (20, 40, 50) */
    u16 defenseBuffTimer;       /* 0x62E: duration of defenseBuff (Player_ClearBuffs, Item_Use) */
    u16 attack;                 /* 0x630: attack power, 1..800 */
    u16 defense;                /* 0x632: defense */
    u16 tradeGetItem;           /* 0x634: the trader kind 0x0A: the item it gives the player (Pouch_Add with 1, Notice_Push; EmKind0A_Trade) */
    u16 tradeGiveItem;          /* 0x636: the trader kind 0x0A: the item the player hands over (Pouch_Add with tradeGiveCount; cleared from gQuest.unk158 when it matches) */
    u8 unk638[0x653 - 0x638];
    u8 stateRequest;            /* 0x653: 1: a state change was requested (Player_RequestState, Player_RequestStateSynced; Player_Update) */
    u8 unk654[0x660 - 0x654];
    u8 floorKind;               /* 0x660: the floor triangle under the player (as Em.floorKind; TownPc_Update passes it to Em_GetFloorInfo as a TriHdr) */
    u8 floorArg;                /* 0x661: byte 1 of the floor triangle (as Em.floorArg): passed with the voice SEs (bank 7, PlayerSe_VoiceAtFrame) */
    u8 floorAttr;               /* 0x662: the floor triangle attribute (Em.floorAttr, a u16 there): EmScript_Op5A compares the byte, EmScript_Op5F reads the u16 (1..3, 4..10) */
    u8 unk663[0x668 - 0x663];
    u8 unk668;                  /* 0x668: nonzero: state 0/0x3B follows the run (Player_State0_1F) */
    u8 unk669;                  /* 0x669 */
    u8 unk66A[0x66C - 0x66A];
    u8 partAnimOn[4];           /* 0x66C: per part: a motion override is set (Player_SetAimLayers, Player_ClearPartOverrides) */
    u16 partAnimMotion[4];      /* 0x670: per part: the override motion (Player_SetPartAnim) */
    u16 partAnimWeight[4];      /* 0x678: per part: the override weight, 0..100 (from aimWeight) */
    u16 area;                   /* 0x680: the area the player is in (as Em.unk680; gathering compares it) */
    u8 areaMoveState;           /* 0x682: 1, 2, 3 while moving to nextArea (Player_State0_0B 1, Player_State3_00 3; Scene_UpdateQuest) */
    u8 nearCamera;              /* 0x683: 1: another player's hunter within 200 of the camera (Player_Update): drawn at alpha 0.2 in the sorted list (Player_Draw); 0 for the local player */
    u16 nextArea;               /* 0x684: the area the player goes to (Town_EnterArea, AreaMenu_GoTo; as Em.nextArea) */
    u8 unk686[0x688 - 0x686];
    f32 startPos[3];            /* 0x688: where the player starts in the area: gStartPos, the action spot target (Player_InitAll, Player_CheckActionSpot) */
    s16 stamina;                /* 0x694 */
    u8 unk696[0x698 - 0x696];
    u32 wallHitDirs;            /* 0x698: as Em.wallHitDirs: func_00330EB0 also runs for the players (Field_Update); Motion_VoiceAtFrame tests 0xE0000007 (a wall ahead) to pick the voice from gEmWallContacts */
    u16 lean;                   /* 0x69C: the body lean while turning, a 16-bit turn kept within 0xF600..0xA00 (Player_UpdateAutoTurn, Actor_TurnToTarget); Player_TwistBone_Unk rolls a bone by it */
    u8 unk69E[0x6A0 - 0x69E];
    f32 mapPos[3];              /* 0x6A0: the position with the area record offsets, y at the middle of its range, for the map (Player_UpdateAreaPos; Map_DrawArea) */
    s16 dashDelay;              /* 0x6AC: frames before R1 with the stick dashes again (states 0/0x24, 2/0x20, 5/1): the states set 30, counted down (Player_UpdateStatus, TownPc_TickTimers) */
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
    s16 poison;                 /* 0x706: poison left: counts down every poisonTimer frames (60 with skill 0xA, 15 with 0xC, else 30); an area of kind 1 keeps it at least 10 (Player_UpdateEnvironment; as Em +0x706) */
    s16 poisonHit;              /* 0x708: the hit poison amount, passed to Player_AddPoison (Player_ReactToHit; as Em.poisonHit) */
    s16 poisonTimer;            /* 0x70A: frames to the next poison tick */
    u8 unk70C[0x710 - 0x70C];
    s16 status4Buildup;         /* 0x710: the status 4 buildup: status4Hit added per hit (Player_ReactToHit); > 0 picks the status state (Player_PickStatusState) */
    s16 status4Hit;             /* 0x712: the hit status 4 amount, added to status4Buildup (Player_ReactToHit; as Em.status4Hit) */
    u8 unk714[0x71A - 0x714];
    s16 status1Hit;             /* 0x71A: the hit status 1 amount (Em_ApplyHitStatus; as Em.status1Hit) */
    u8 unk71C[0x722 - 0x71C];
    u8 hideShadow;              /* 0x722: as Em.hideShadow: nonzero: Effect01_Draw (the shadow) skips the object; counted down (Player_TickTimers, TownPc_TickTimers) */
    u8 unk723[0x724 - 0x723];
    f32 attackFactor;           /* 0x724: attack factor: attack / 100 */
    f32 defenseFactor;          /* 0x728: defense factor: defense / 2, at least 1 */
    u8 unk72C[0x730 - 0x72C];
    f32 surfaceY;               /* 0x730: as Em.surfaceY (TownPc_Update passes it to Em_GetFloorInfo) */
    u8 unk734[0x739 - 0x734];
    u8 weaponSeMute;            /* 0x739: nonzero: no weapon SEs (PlayerSe_WeaponAtFrame); the weapons hit checks set 2, Player_UpdateStatus counts it down */
    u8 unk73A_combat;           /* 0x73A: set by Em_SetCombat; cleared by func_0070E1E0 when the monster (its bit in Em +0x73A) resets */
    u8 unk73B[0x740 - 0x73B];
    f32 fadeAlpha;              /* 0x740: the local player's draw alpha in the field: eased a tenth of the way to CamCtl_GetPlayerFade's target each frame, 1.0 without one (Player_DrawLocal; Player_Reset sets 1.0) */
    s16 mashBuildup;            /* 0x744: the buildup toward the mash status: mashBuildupHit added per hit (Player_ReactToHit); Player_HasMashBuildup, Player_InMashStatus */
    s16 mashLeft;               /* 0x746: the status of state 2/0x1E: pressed buttons take 10 (skill 0x10 twice, 0x12 half: mashTimer); at 0 state 2/0x1C; synced in qnet_sync message 3 */
    s16 mashBuildupHit;         /* 0x748: the hit amount added to mashBuildup (Player_ReactToHit) */
    s16 tradeGiveCount;         /* 0x74A: the trader kind 0x0A: the count taken with tradeGiveItem, -1 or -99 (Pouch_Add; EmKind0A_Trade) */
    u8 attackHit800;            /* 0x74C: as Em.attackHit800: set to 1 when one of its attacks with flag 0x800 hits (ObjAttach_DamagePlayer, ObjAttach_DamageEm); cleared by the attack creators; copied into the attack by Eff00_Create */
    u8 unk74D[0x754 - 0x74D];
    f32 moveTo[3];              /* 0x754: a point to move to (Player_StartMoveTo, Player_MoveToSpawn, Contest_TakeSeat, HairMenu_MovePlayer) */
    f32 moveToStep[3];          /* 0x760: added to pos each frame while moveToFrames counts (Player_UpdateMoveTo, pouch.c) */
    s16 moveToFrames;           /* 0x76C: frames to reach moveTo (Player_StartMoveTo, Player_UpdateMoveTo) */
    s16 lookYaw;                /* 0x76E: the head turn toward the nearest monster (Player_FindLookTarget; pouch.c reads it signed; the lobby unwinds it by 0x400 a frame through u16, TownPc_UnwindAngle76E) */
    u8 lookFound;               /* 0x770: 1: Player_FindLookTarget found a monster in range */
    u8 unk771;                  /* 0x771 */
    u8 inBattle;                /* 0x772: nonzero while a monster fights it (func_0070E1E0; Bgm_IsBattle, Hud_DrawPlayerStatus) */
    u8 prevInBattle;            /* 0x773: the previous inBattle (func_0070E1E0) */
    u8 weaponFlags;             /* 0x774: flags of the weapon type (WeaponType_Flags(weaponType): 1, 2, 4; Player_SkillApplies) */
    u8 coatingSlot;             /* 0x775: bow: the pouch slot of the loaded coating, 0xFF: none (sub_main Bow_SetCoatingSlot; a shot uses one up) */
    s16 swordGauge;             /* 0x776: long sword: the spirit gauge 0..100 (LongSword_AddGauge, LongSword_GaugeAtLeast, Hud_DrawWeaponBox) */
    struct Em *cart;            /* 0x778: the Felyne cart (an Em of kind 0x12, its cartPlayer this player) that carries the player away (Player_State4_00); cleared when it leaves (QuestEm_LeaveArea) */
    ItemStack pouch[20];        /* 0x77C: the item pouch */
    s16 unk7CC;                 /* 0x7CC */
    s16 unk7CE;                 /* 0x7CE */
    void *spot;                 /* 0x7D0: the action spot the player stands at (gather.c) */
    s16 weaponTimer;            /* 0x7D4: great sword: the charge (frames); gunlance: the wyvern fire cooldown; bow: frames since the last shot (to 1000) */
    s16 sharpness;              /* 0x7D6 */
    s8 chargeLevel;             /* 0x7D8: great sword: the charge level 0-3 (GreatSword_ChargeLevel) */
    s8 fishTimer;               /* 0x7D9: fishing: frames (Lure_Update, Fish_StepBite), counted down by Player_UpdateStatus */
    s16 staminaMax;             /* 0x7DA: 75..450 */
    s16 sharpnessMax;           /* 0x7DC */
    u8 itemUseDelay;            /* 0x7DE: counted down (Player_UpdateStatus); the item button (0x200) does nothing while nonzero (idle and move states) */
    u8 sharpnessLevel;          /* 0x7DF: the colour (Player_GetSharpnessLevel) */
    u16 pouchCursor;            /* 0x7E0: the selected pouch slot */
    u16 useItem;                /* 0x7E2: the item being used: the selected pouch item (Player_UseItem, Item_Use compare it with item ids); copied to gSpecialMinePoint (SpecialMine_Create) */
    u8 itemSelect;              /* 0x7E4: 1 while the item selector is held open (Player_UpdateItemSelect); only buttons 0x90 get through */
    u8 unk7E5;                  /* 0x7E5 */
    u16 ammoSlot;               /* 0x7E6: bowgun: the pouch slot of the loaded ammo, 0xFF: none */
    f32 targetPos[3];           /* 0x7E8: the point the monsters aim at (Em_GetTargetPos) */
    PlayerPend pend[2];         /* 0x7F4: positions received over the network, applied when their timers run out (QSync_QueuePlayerPos, QSync_UpdatePlayerPos) */
    s16 ammoCount;              /* 0x814: bowgun: rounds of the loaded ammo left, 0xFF: unlimited */
    u8 unk816;                  /* 0x816 */
    u8 startItemList;           /* 0x817: 1 + the start item list (Pouch_SetStartItems, Quest_InitPlayers) */
    s16 staminaTimer;           /* 0x818: frames until the maximum stamina drops (10800) */
    u8 supplyBoxOpen;           /* 0x81A: Hud_SupplyBoxOpen, gHudState->supply; the supply box menu clears it on cancel (pouch.c) */
    u8 menuCursor;              /* 0x81B: menu cursor (the supply box and delivery menus) */
    u8 actRequest;              /* 0x81C: the gesture requested (Player_RequestAct); Player_StartAct starts state 6/gGestureStates[it] */
    u8 weaponShown;             /* 0x81D: nonzero: the weapon model is drawn (Player_DrawLocal, Player_Draw call Player_DrawWeapon); Player_Reset and the hunter setups set 1, plsel's preset players 0, the town hunters while not free (TownPc_IsFree) */
    u8 tradeMsgTimer;           /* 0x81E: the trader kind 0x0A sets 5, 10, 60 (EmKind0A_Trade, EmKind0A_ShowMessage); Player_UpdateStatus counts it down */
    u8 tradeCount;              /* 0x81F: the trader kind 0x0A: +1 per trade up to 7 (7 at once from 4 at random), decremented by the trader's later steps */
    u8 unk820[0x821 - 0x820];
    s8 aimWeight;               /* 0x821: bowgun: -100..100, the weight of the second motion layers (Bowgun_SetAimLayers) */
    s16 cookTimer;              /* 0x822: the cooking timer (frames on the spit, Player_State0_55) */
    s16 staminaLockTimer;       /* 0x824: frames left of an item effect that keeps the stamina (Item_Use; Player_StaminaLocked) */
    s16 savedAmmoCount;         /* 0x826: ammoCount saved with savedCoatingSlot (Player_BowgunAmmoSwitch, Bow_State00) */
    u8 cookResult;              /* 0x828: the cooking result: 0 raw, 1 rare, 2 done, 3 burnt, 4 gourmet (Player_State0_55) */
    u8 savedRoundsLoaded;       /* 0x829: roundsLoaded kept over the reload and the weapon change (restored from it), as savedClipSize and savedAmmoCount */
    u8 savedClipSize;           /* 0x82A: clipSize saved with savedCoatingSlot (Player_BowgunAmmoSwitch, Bow_State00) */
    u8 voice;                   /* 0x82B: the voice: save data +0x20D8 and the hunter card (Player_SetupFromSave); gVoiceBanks[gender * 0x10 + it] (lobby), Chara_SetVoiceMotion */
    s8 name[18];                /* 0x82C: the hunter name (SaveData name, Player_LoadName) */
    s16 gotItem;                /* 0x83E: the item to give the player (Pouch_GiveItem: Item_Use, Player_GatherAtSpot, Contest_GivePrize, PoogieMenu_Update) */
    u16 gotItemId;              /* 0x840: the item id that goes with gotItem (Pouch_GiveItem in TownPc_State63 and unk_70A290.c; unk_3E3D00.c switches on it) */
    u16 baited;                 /* 0x842: the lure keeps its bait: 1 from the cast (game unk_70A290.c, lobby unk_79AB50.c); a fish goes for the lure only while it is set (Fish_StepSwim) and a nibble clears it one time in four (Fish_StepBite); with it the action button picks state 0x64 over 0x54 (game) and 0x65 over 0x64 (lobby TownPc_FishInput) */
    u8 unk844;                  /* 0x844: lobby: the item of a remote player action message (TownPc, unk_779760.c); cleared with unk867 */
    u8 bowgunReady;             /* 0x845: 1 while the bowgun attack input runs with rounds loaded (Bowgun_AttackInput), cleared at each frame start (Player_Update) and by Player_Reset; func_00355E90 tests it */
    s16 aimPitch;               /* 0x846: the aim pitch, 16-bit turn: the first-person camera (camera mode 1, Mtx_FromAngles16Pos) and Hud_DrawObjMarker use it with the heading rot[1]; up/down input adds +-0x400 clamped to +-0x2000 (45 deg; Player_AddAimPitch, gScene.aimReverse flips it); func_00711F80 sets it from aimPitchPct */
    u8 freeState;               /* 0x848: set to 1 each frame by the idle and move states, cleared at the frame start (Player_Update): items can be used (Player_IsFree, the pouch menu) */
    u8 menuPage;                /* 0x849: menu page */
    u8 pouchHudFlags;           /* 0x84A: pouch HUD flags: 2 next, 4 previous item (Player_UpdateItemSelect), 1, 8, 0x10 (ItemHud_Update, Hud_DrawPouchBar) */
    u8 menuDelay;               /* 0x84B: menu input delay */
    ItemStack delivered[4];     /* 0x84C: items delivered for the quest (Quest_CountDelivered) */
    u16 offerItem;              /* 0x85C: an item offered to another player (Player_TakeOfferedItem) */
    s16 offerCount;             /* 0x85E: its count */
    u8 offerRequest;            /* 0x860: the pouch menu's offer sets it with offerTo/offerItem/offerCount; the idle states then enter state 0/0x67 */
    u8 offerTo;                 /* 0x861: the index of the player it is offered to */
    u8 offerState;              /* 0x862: the asking player, then 1: received, 2: refused, 0xFF: given (qnet_sync message 7) */
    u8 offerStarted;            /* 0x863: 1 in the frame an offer starts (offerRequest taken, state 0/0x67): the pouch menu closes (PouchMenu_Update); cleared by Player_Update */
    s16 envTimer;               /* 0x864: frames to the next environment effect (30..180, Player_UpdateEnvironment) */
    u8 syncTimer;               /* 0x866: frames until the player state is sent again (QSync_SendPlayer sets it; counted down before each send) */
    u8 unk867;                  /* 0x867: lobby: counted up by func_00796DE0 while unk844 is 0; at 10 (online) unk844 is set */
    u8 skills[5];               /* 0x868: skill ids */
    u8 unk86D[0x86E - 0x86D];
    u8 questStatus;             /* 0x86E: the lobby: low bits the quest category (QuestFlags_GetCategory), 0x80 accepted and paid, 0x40 accepted (QuestDesk_Accept), 0x20 set out (Town_UpdateMode); cleared by QuestDesk_Drop; on the hunter card (HunterCard_Build) */
    u8 teamItemMask;            /* 0x86F: set to 15 when a team item (0xB0-0xB3) is used (Item_UseTeamPowder); the bit of each player reached is cleared (unk_3885D0.c) */
    u16 coolDrinkTimer;         /* 0x870: frames left of item 0x15's effect (18000; Item_Use); nonzero: Hud_HealthColor and Player_UpdateEnvironment skip the heat */
    u16 hotDrinkTimer;          /* 0x872: frames left of item 0x16's effect (18000; Item_Use); nonzero: no cold in zones 2 and 4 (Player_UpdateHunger, Hud_StaminaColor) */
    u16 regenTimer;             /* 0x874: frames left of items 0x0C, 0x3A-0x3D (Item_Use, 3600..14400): doubles the health recovery step (Player_UpdateStatus); Notice 0x16 at the end; status icon 0x1000 */
    u8 faints;                  /* 0x876: times fainted in the quest, up to 99 (Player_State3_00; QuestMenu_DrawFaints) */
    u8 supplyTaken;             /* 0x877: nonzero once the supply box was taken from (SupplyBox_Take, QSync_OnSupply) */
    f32 resist[5];              /* 0x878: element resistances (fire, water, thunder, ice, dragon), -100..100 */
    s16 detectTimer;            /* 0x88C: frames left of an item effect (Item_Use) that Player_DetectLevel and the HUD glow test */
    u16 supplyBoxTimer;         /* 0x88E: 900 frames after a supply box take (SupplyBox_Take, SupplyBox_Update) */
    s16 recoverTimer;           /* 0x890: state 4/x: 150 frames (skill 1 halves, skill 3 doubles), mashing takes 1 more each 3 frames; the hunter gets up at 0 */
    u8 offerNoticeTimer;        /* 0x892: 0x5A frames after the pouch-full notice for an offered item (no new take while nonzero) */
    s8 unk893;                  /* 0x893: counted up when state 0/0x11 follows state 0/9 (Player_State0_11) */
    u8 skillGroups[5];          /* 0x894: the skill group of each slot */
    s8 demonMode;               /* 0x899: dual swords: 1 in the demon mode (DualSwords_State04; Eff00_Setup) */
    s16 noFlinchItemTimer;      /* 0x89A: item 0x14 sets it with defenseBuffTimer (Item_Use); counted down; tested with noFlinchTimer and demonMode (Player_IgnoresFlinch) */
    u8 prevFreeState;           /* 0x89C: freeState of the last frame (Player_Update): the guard skill 0x2C and the heavy bowgun shield guard only then */
    s8 skillPoints[5];          /* 0x89D */
    u8 attackPercent;           /* 0x8A2: the attack power in percent (great sword charge levels: 100..130) */
    u8 attackPercentBase;       /* 0x8A3: the weapon type's attack percent (D_0045EE00[weaponType], Player_ResetAttackPercent) */
    u32 styleColor;             /* 0x8A4: gPlayerColorWords[style] (Player_SetColorWord) */
    u8 lookPartId[6];           /* 0x8A8: per armour part: the model part id Look_MapPartId picked (Player_LoadLookModels), 0xFF: none (Player_InitPartTable) */
    u16 weaponModelKind;        /* 0x8AE: the kind of the loaded model, 0xFFFF: none (Player_LoadWeaponModel) */
    u8 torsoAltModel;           /* 0x8B0: 1: the torso armour has no model for the gender and part slot 6 is used (Armor_LacksSexModel; Player_LoadLookModels, Player_QueueLookFiles) */
    u8 deliveryMenu;            /* 0x8B1: the delivery menu: 0xFF, 1, 0 (Player_RunDeliveryMenu, Player_State0_0C); blocks the pad input */
    u8 yesNoCursor;             /* 0x8B2: the delivery menu: the yes or no cursor (gYesNoTexts[it], DeliveryMenu.yesNo; Player_RunDeliveryMenu, Quest_DeliveryMenu) */
    u8 unk8B3;                  /* 0x8B3 */
    f32 motionSpeed;            /* 0x8B4: motion speed factor (1.0; 0.5..1.54 per state: Player_SetState and the weapon states); motionStep0/1 are 2.0 times it */
    s8 mashTimer;               /* 0x8B8: a countdown between the button presses that shake off a stun (Player_UpdateMashEscape) */
    u8 savedCoatingSlot;        /* 0x8B9: bow: a copy of coatingSlot (taken at a shot, with savedAmmoCount, unk829 and savedClipSize, and by the ammo menu Player_BowgunAmmoSwitch, which restores it) */
    s16 actDetectTimer;         /* 0x8BA: 300 frames from the gesture state 6/2 (Player_State6_02): as detectTimer (Player_DetectLevel, Hud_GlowCond_Unk, StageObj_AreaFlyer_Run) */
    u8 charType;                /* 0x8BC: the character's TYPE, 0-3 (SaveData.charType, HunterCard.charType, the preset characters): with gender it picks the row of gLookPartMap0/3/4 (gender * 16 + it * 4) */
    u8 hornNoteSlot;            /* 0x8BD: hunting horn: the slot of the last note in hornNotes (0..7) */
    u8 hornNotes[8];            /* 0x8BE: hunting horn: the last 8 notes played (HuntingHorn_AddNote) */
    s8 aimPitchPct;             /* 0x8C6: the bow's aim pitch, -100..100 percent: func_00711F80 turns it into aimPitch (45 deg up, 35 deg down); the arrow's rotX (unk_6D3B50 shot spawn); synced online (qnet_sync k1.unk22) */
    u8 bladeSpecial;            /* 0x8C7: the weapon blade data value unk13 (Equip_GetBladeSpecial of equip[6]): hunting horn: its note set (D_44AE30[x * 3 + i]); gunlance: the shelling type (x % 3) and level (x / 3) */
    u8 hornSong;                /* 0x8C8: hunting horn: the song the last notes matched (HuntingHorn_CheckSongs) */
    u8 mealSkill;               /* 0x8C9: the meal's skill (offline; Player_HasSkill, Player_GetSkill) */
    u8 mealSkillGroup;          /* 0x8CA: its skill group */
    u8 style;                   /* 0x8CB: the style (save symbolColor; Player_SetColorWord); 0xFF none; the lobby draws its icon (0xB cycles colours) */
    s16 status6Hit;             /* 0x8CC: the hit status 6 amount: > 0 starts status6Timer unless skill 0xE (Player_ReactToHit) */
    s16 status6Timer;           /* 0x8CE: status 6 frames left: 1800 at a hit (900 with skill 0xD, 3600 with 0xF; Player_ReactToHit, Item_Use, ObjAttach_DamagePlayer), counted down (Player_UpdateStatus), cleared with the other statuses (Player_Reset, Player_State0_33); an icon (Player_UpdateStatusIcons) */
    s16 status7Hit;             /* 0x8D0: the hit status 7 amount: > 0 starts status7Timer unless skill 0xC5 (Player_ReactToHit) */
    s16 status7Timer;           /* 0x8D2: status 7 frames left: 1800 at a hit or a poisoning item (900 with skill 0xC4; Player_ReactToHit, Item_Use), counted down (Player_UpdateStatus), cleared with the other statuses; an icon (Player_UpdateStatusIcons) */
    u8 status8Hit;              /* 0x8D4: the hit status 8 amount: > 0 lowers staminaMax unless skill 0xC5 (Player_ReactToHit) */
    u8 deliveryOpen;            /* 0x8D5: Hud_PlayerBusy8D5_Unk, gHudState->unk23; Quest_DeliveryMenu clears it when closing (pouch.c) */
    u8 mealEffect[12];          /* 0x8D6: meal effects by kind (Player_HasMealEffect, Player_ClearMealEffects; the hunting horn songs set some) */
    s16 mealEffectTimer[12];    /* 0x8E2: per kind: frames left (0x708, 0xA8C, 0xE10; counted down by Player_UpdateStatus) */
    s16 tradeCooldown;          /* 0x8FA: 9000 frames set at a trade when 0 (EmKind0A_Trade), counted down by Player_UpdateStatus; the trader clears it */
    s16 hpMaxBase;              /* 0x8FC: hpMax without the meal effect 3: set from hpMax, raised by Player_AddMaxHp unless temporary (to 150); restored to hpMax when effect 3 ends (Player_UpdateStatus) */
    u8 unk8FE;                  /* 0x8FE */
    u8 inPoisonArea;            /* 0x8FF: 1 while in an area of kind 1 without skill 0xB, which keeps poison at least 10 (Player_UpdateEnvironment clears it each frame); func_003B5F20 tests it offline */
    u8 unk900[0x970 - 0x900];
} Player;
typedef char Player_size_check[(sizeof(Player) == 0x970) ? 1 : -1];

/* 10 slots: Player_ClearAll clears 10, and gPlayerSys follows them (0x5FE830). */
extern Player gPlayers[10];

#endif /* PLAYER_H */
