#ifndef LOBBY_NPC_H
#define LOBBY_NPC_H

#include "common.h"

/*
 * A route entry (0x14 bytes): the point, the state to enter there
 * (Npc_EnterRoutePoint; 0 ends the route and it starts again) and an
 * argument: the frames to wait there, -1 no wait, -2 until the Em timer
 * +0x154 runs out (Npc_UpdateState); for state 0x83 the next route
 * (Npc_State83) and for 0x81 an angle.
 */
typedef struct NpcPos {
    f32 v[3];
} NpcPos;
typedef struct NpcRouteEnt {
    /* 0x00 */ NpcPos pos;
    /* 0x0C */ s32 state;
    /* 0x10 */ s32 arg;
} NpcRouteEnt;

/*
 * The townspeople of the lobby are Em slots (include/em.h: gEms, 20 of
 * them) whose monster fields from +0x438 on are used as this work block
 * (Npc_Update runs them). The first fields (+0x14 group, +0x15 state, +0xA4
 * yaw, +0xAC position) are shared with the players, so the actor core
 * (Actor_SetState, Actor_SetMotion ...) takes either. Em +0x1B is the
 * townsperson's type (Npc_IsType4E ...; Npc_UpdateState picks the state
 * handlers by it).
 *
 * The files take the block as `NpcWork *m = (NpcWork *)(em + 0x438)`; the
 * accesses through the Em pointer (`*(s16 *)(a + 0x474)`) are left as byte
 * offsets: Em is shared with the game overlay's monsters.
 */
typedef struct NpcWork {
    /* 0x00 */ u8 id;           /* which townsperson (Npc_Find, Npc_IsPresent) */
    /* 0x01 */ u8 voice;            /* the first of its two voice sounds (Npc_PlayVoice adds 0 or 1) */
    /* 0x02 */ u8 unk2;         /* cleared by Npc_UpdateState each frame; set again for townsperson 0x72 */
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 unk5[0x8 - 0x5];
    /* 0x08 */ char *name;      /* its name: the area table's, or the save's +0x2C98 for townsperson 0x14 (func_0077DA10) */
    /* 0x0C */ NpcRouteEnt *route; /* the route (Em + 0x444) */
    /* 0x10 */ u8 unk10[0x14 - 0x10];
    /* 0x14 */ f32 step[3];     /* Actor_StartMoveTo: the move per frame (Em + 0x44C) */
    /* 0x20 */ NpcPos unk20;
    /* 0x2C */ f32 headYaw;      /* the head turn toward the local player, in degrees within +-10 or +-20, eased by 0.2 a frame (Npc_UpdateMatrix) */
    /* 0x30 */ s32 wait;        /* frames spent at the route point (Npc_UpdateState) */
    /* 0x34 */ s16 turnDelta;   /* the angle left to turn (Npc_TurnToPlayer, Npc_StartTurnTo) */
    /* 0x36 */ s16 homeYaw;     /* the yaw to turn back to (Npc_StartTurnBack) */
    /* 0x38 */ s16 turnCount;   /* frames of turning left (Npc_StepTurn) */
    /* 0x3A */ s16 turnStep;    /* the turn per frame */
    /* 0x3C */ s16 routeIdx;    /* the current route point (Em + 0x474) */
    /* 0x3E */ u16 moveCount;   /* Actor_StartMoveTo: frames left (Em + 0x476) */
    /* 0x40 */ u16 motionPhase;     /* a motion's phase: 0, then 1 at frame 30, ended at 118 (Npc_Idle0F, Npc_State69Solo) */
    /* 0x42 */ u16 unk42;
    /* 0x44 */ u8 unk44;
    /* 0x45 */ u8 unk45;
    /* 0x46 */ s8 fxStep;           /* NpcSetA/B/C_Run: 0, then 1 (the motion effects run: NpcSetA/B/C_MotionFx) */
    /* 0x47 */ u8 unk47;
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u8 unk49;
    /* 0x4A */ u8 busy;             /* 1: busy (Npc_IsFree), with the activity */
    /* 0x4B */ u8 activity;         /* an activity (Em + 0x483); 0x4D-0x55: not free */
    /* 0x4C */ u8 holdRoute;    /* nonzero: Npc_UpdateState stays at the route point */
    /* 0x4D */ u8 unk4D[0x50 - 0x4D];
    /* 0x50 */ f32 unk50[3];     /* Npc_Init: the position of bone 0x14 (free), 0xA (activities 0x51-0x55) or 0xC (busy), else 200 * scaleY above it (no reader seen) */
    /* 0x5C */ s32 unk5C;
} NpcWork;
typedef char NpcWork_size_check[(sizeof(NpcWork) == 0x60) ? 1 : -1];

/* The workshop's townspeople (ids 0xE, 0xF, 0x58: Workshop_Open, Workshop_Update) read their
 * block with their own meaning at +0x3A (NpcWork's turnStep). */
typedef struct NpcWorkshopWork {
    /* 0x00 */ u8 id;           /* NpcWork's id */
    /* 0x01 */ u8 unk01[0x3A - 0x01];
    /* 0x3A */ s16 menuSave;    /* the workshop menu's +4 halfword, kept while the menu is closed (Workshop_Update stores it, Workshop_Open restores it) */
    /* 0x3C */ u8 unk3C[0x60 - 0x3C];
} NpcWorkshopWork;
/* gTutorial (lobby): the tutorial NPC's lesson (Tutorial_Start, Tutorial_IsDone, Tutorial_RunNpc). */
typedef struct Tutorial {
    u8 step;        /* 0x00 */
    u8 pad1;
    u8 active;      /* 0x02 */
    u8 done;        /* 0x03 */
    s16 timer;      /* 0x04 */
    s16 frames;     /* 0x06 */
    s32 mask;       /* 0x08 */
    s32 unkC;       /* 0x0C */
    u8 kind;        /* 0x10 */
    u8 ticked;      /* 0x11 */
    u8 count;       /* 0x12 */
    u8 npcs;        /* 0x13 */
    u8 set;         /* 0x14 */
    u8 turn;        /* 0x15 */
    u8 idx;         /* 0x16 */
    u8 pad17[5];
} Tutorial;
typedef char Tutorial_size_check[(sizeof(Tutorial) == 0x1C) ? 1 : -1];

extern Tutorial gTutorial;

typedef char NpcWorkshopWork_size_check[(sizeof(NpcWorkshopWork) == sizeof(NpcWork)) ? 1 : -1];

#endif
