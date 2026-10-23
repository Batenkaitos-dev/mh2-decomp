#ifndef CAMERA_H
#define CAMERA_H

/*
 * The camera modes: CamCtl.modes' index, in the order of the handler tables
 * gCamModeInit and gCamModeUpdate; CamCtl.areaMode holds the
 * one the area picks. CAM_MODE_SCRIPT runs the scripts (CamScript_Init).
 */
#define CAM_MODE_FOLLOW 0       /* behind the player (CamFollow_*) */
#define CAM_MODE_ORBIT 1        /* over the shoulder (CamOrbit_*) */
#define CAM_MODE_RAIL 2         /* the area's rail */
#define CAM_MODE_AIM 3
#define CAM_MODE_FIXED 4
#define CAM_MODE_SCRIPT 5
#define CAM_MODE_COUNT 6

/* CamArea.kind: the follow camera's areas, and the rail's kinds 1-3. */
#define CAM_AREA_FOLLOW 0
#define CAM_AREA_FIXED 1
#define CAM_AREA_PATH 2
#define CAM_AREA_OFFSET 3

#define CAM_AREA_ID_NONE 0xFF   /* CamCtl.areaId, prevAreaId: no area yet */

/* The projection's constants. */
#define CAM_ASPECT 1.4285715f       /* the aspect ratio, 640 / 448 (the frame buffer's width over height) */
#define CAM_FOV_DEFAULT 0.87266463f /* the default vertical field of view: 50 degrees, in radians */
#define CAM_PRESET_NONE 0xFF        /* gCameraPreset: none chosen; Camera_SetPreset(CAM_PRESET_NONE) only reapplies the projection */
#define CAM_YAW_FROM_NONE 0xFF  /* CamView.yawFrom: take no yaw from another mode */

/* The follow camera's distance settings (CamFollow.distNo, CamDistSet.dist[]). */
#define CAM_DIST_DEFAULT 2
#define CAM_DIST_MAX 3          /* the highest the pad steps to */
#define CAM_DIST_CROUCH 4       /* at CAM_DIST_DEFAULT while the player crouches */
#define CAM_DIST_BLOCKED 6      /* the blocked view (CamFollow_UpdateBlocked) */
#define CAM_DIST_FALL 7         /* the fall view (CamFollow_UpdateFallView) */

#define CAM_HEIGHT_NORMAL 0     /* CamFollow.heightState */
#define CAM_HEIGHT_FALL 1
#define CAM_BLOCKED_HOLD 15     /* frames: CamFollow.blockedTimer's start, held while blown away */
#define CAM_SWEEP_MASK 0x8009   /* the triangle flags the camera's sweeps skip (AreaGrid_SweepSphere) */
#define CAM_LINE_MASK 0x8001    /* the triangle flags Cam_LineHitDist's sweep skips (AreaGrid_SweepCells) */

/* CamView.flags in the script mode. */
#define CAM_SCRIPT_ENDED 0x01   /* the script ended */
#define CAM_SCRIPT_STARTED 0x02 /* started by CamReq_Start (CamReq_Started tests it) */
#define CAM_SCRIPT_CLAMP_EYE 0x04 /* CAMCMD_CLAMP_EYE: CamScript_Place keeps the eye out of
                                   * the walls and above the floor */

/* CamView.blend: a blend between two views (CamView_StartBlend, CamBlend_EaseStep). */
#define CAM_BLEND_NONE 0        /* no blend, or the blend is done */
#define CAM_BLEND_START 1       /* a blend starts */
#define CAM_BLEND_RUN 2         /* a blend runs */

/* CamView.blend in the script mode: the step of the script's run (CamScript_Run). */
#define CAM_RUN_START 0         /* the script is set up */
#define CAM_RUN_STEP 1          /* it runs (CamScript_Step) */
#define CAM_RUN_DONE 2          /* it has ended */

/* A script's id below this is a dmcam file's (looked up in the cache, Dmcam_Find);
 * from it on, the id is the address of a script built into the game. */
#define DMCAM_ID_LIMIT 0x80

/*
 * The camera script's commands (CamScript_Step: the first word of each command;
 * cmd[2] and on are its arguments). Commands 17 to 32 set the four swings:
 * the sideways sway (swing[0]), the upward sway (swing[1]), the roll's swing
 * (swing[2]) and the fov's swing (swing[3]); AMP0 and AMP1 are a swing's
 * amplitude at the move's start and end.
 */
#define CAMCMD_PLACE 0          /* CamScript.place = cmd[2] (CAM_PLACE_*) */
#define CAMCMD_EYE_SPACE 1      /* eyeSpace and eyePart */
#define CAMCMD_EYE_POINT 2      /* eye[cmd[2]] (CamScript_EyePoint) */
#define CAMCMD_AT_SPACE 3       /* atSpace and atPart */
#define CAMCMD_AT_POINT 4       /* at[cmd[2]] (CamScript_AtPoint) */
#define CAMCMD_PIVOT 5          /* pivotEye */
#define CAMCMD_ANGLE_X0 6       /* angle[0][0] */
#define CAMCMD_ANGLE_X1 7       /* angle[1][0] */
#define CAMCMD_ANGLE_Y0 8       /* angle[0][1] */
#define CAMCMD_ANGLE_Y1 9       /* angle[1][1] */
#define CAMCMD_DIST0 10         /* dist[0], in sixteenths */
#define CAMCMD_DIST1 11         /* dist[1] */
#define CAMCMD_ROLL0 12
#define CAMCMD_ROLL1 13
#define CAMCMD_FOV0 14
#define CAMCMD_FOV1 15
#define CAMCMD_LENGTH 16        /* the move's length in frames */
#define CAMCMD_SWAY_X_PHASE 17
#define CAMCMD_SWAY_X_SPEED 18
#define CAMCMD_SWAY_X_AMP0 19
#define CAMCMD_SWAY_X_AMP1 20
#define CAMCMD_SWAY_Y_PHASE 21
#define CAMCMD_SWAY_Y_SPEED 22
#define CAMCMD_SWAY_Y_AMP0 23
#define CAMCMD_SWAY_Y_AMP1 24
#define CAMCMD_ROLL_SWING_PHASE 25
#define CAMCMD_ROLL_SWING_SPEED 26
#define CAMCMD_ROLL_SWING_AMP0 27
#define CAMCMD_ROLL_SWING_AMP1 28
#define CAMCMD_FOV_SWING_PHASE 29
#define CAMCMD_FOV_SWING_SPEED 30
#define CAMCMD_FOV_SWING_AMP0 31
#define CAMCMD_FOV_SWING_AMP1 32
#define CAMCMD_SWING_RAND 33    /* swingRand[cmd[2]] = cmd[3] */
#define CAMCMD_KEEP 34          /* copies the start values to the end ones (CAM_KEEP_* in cmd[2]) */
#define CAMCMD_EASE 35          /* ease (CAM_EASE_*, the top 2 bits) and easePow (the low 14 bits / 1024) */
#define CAMCMD_LOOP 36          /* the move's length, and the loop's start */
#define CAMCMD_LOOP_FRAME 37    /* places the view (cmd[2]), counts a frame down, back to the loop */
#define CAMCMD_LOOP_PLACE 38    /* places the view, back to the loop while cmd[2] is set */
#define CAMCMD_END 40           /* the script ends */
#define CAMCMD_CLAMP_EYE 41     /* sets CAM_SCRIPT_CLAMP_EYE */
#define CAMCMD_GOTO_MARK 42     /* back to the mark */
#define CAMCMD_SET_MARK 43      /* the mark is the next command */

/* CAMCMD_KEEP's bits (CamScript_CopyToEnd): the end value takes the start value. */
#define CAM_KEEP_EYE 0x01
#define CAM_KEEP_AT 0x02
#define CAM_KEEP_ROLL 0x04
#define CAM_KEEP_FOV 0x08
#define CAM_KEEP_SWING0 0x10
#define CAM_KEEP_SWING1 0x20
#define CAM_KEEP_SWING2 0x40
#define CAM_KEEP_SWING3 0x80

/* CamScript.ease: how a move goes from its start to its end (CamScript_Place). */
#define CAM_EASE_LINEAR 0
#define CAM_EASE_IN 1
#define CAM_EASE_OUT 2
#define CAM_EASE_IN_OUT 3

/* CamScript.place: how CamScript_Place sets the view. */
#define CAM_PLACE_ORBIT 0       /* orbits the at point (the angles and the distance) */
#define CAM_PLACE_EASE 1        /* eases the eye and the at point */

/* CamCtl.override: the mode whose view overrides the current one. */
#define CAM_OVERRIDE_NONE (-1)

/* CamShake.kind: gCamShakeLengths' index, with a bit for a shake felt everywhere. */
#define CAM_SHAKE_EVERYWHERE 0x80
#define CAM_SHAKE_KIND_MASK 0x7F

/* CamDataHdr.version of the area's camera data. */
#define CAM_DATA_VERSION 0x102

/* CamArea.lookSpace: the look point's space (CamPoint_GetPos). */
#define CAM_LOOK_SPACE_WORLD 0  /* the area's point in the world */
#define CAM_LOOK_SPACE_PLAYER 1 /* in the player's frame (its worldMtx) */
#define CAM_LOOK_SPACE_OFFSET 2 /* the player's position plus an offset */

/* CamArea.lookLock: the look point's axes kept from the area's point. */
#define CAM_LOOK_LOCK_Y 0
#define CAM_LOOK_LOCK_XZ 1
#define CAM_LOOK_LOCK_NONE 2

/* CamArea.flags. */
#define CAM_AREA_ON_WALL 0x80   /* the area counts only while the local player is on a wall */

/* CamArea_TestObj's mask: the faces it skips. */
#define CAM_FACE_NO_ENTER 0x01  /* skipped when the player's new area is looked for */
#define CAM_FACE_NO_STAY 0x02   /* skipped when the current area is tested */

/* CamArea.trans: {area id, frames} pairs, the length of the blend into each area. */
#define CAM_TRANS_COUNT 16
#define CAM_TRANS_DEFAULT 15    /* frames: an area without an entry */

/* CamCtl.mode's bits (CamCtl_GetMode): the tests the camera skips. */
#define CAMCTL_NO_EM_PUSH 0x01  /* no push out of the monsters (Cam_PushOutOfEms) */
#define CAMCTL_NO_LINE_TEST 0x02 /* no wall line test (Cam_LineHitDist) */

/*
 * Camera requests (CamReq_Set's request, CamCtl.request): each one names a camera script
 * (gDmcamFileIds' index). Named from the code that requests them; a base takes the
 * caller's index added (CamReq_Workshop's level, CamReq_TownTour's tour ...).
 */
#define CAM_REQ_CART 0x01               /* the player was carted (not in the battle town) */
#define CAM_REQ_FORTRESS_INTRO 0x02     /* the area's opening view (CamReq_AreaIntro) */
#define CAM_REQ_CASTLE_INTRO 0x03
#define CAM_REQ_BATTLE_TOWN_INTRO 0x04
#define CAM_REQ_PLESIOTH 0x05           /* seen: Plesioth's state 2_12 */
#define CAM_REQ_PLAYER_DOWN 0x0A        /* the player fainted, or the quest ended */
#define CAM_REQ_CONTEST_SPOT 0x0B       /* + the contest: at a spot */
#define CAM_REQ_CONTEST 0x0E            /* + the contest: at spot 0 */
#define CAM_REQ_WORKSHOP 0x11           /* + the workshop's level */
#define CAM_REQ_HAIR_MENU 0x16
#define CAM_REQ_MINIGAME 0x17           /* + the minigame */
#define CAM_REQ_TOWN_AREA 0x1B          /* + the town's area */
#define CAM_REQ_VILLAGE_BOARD 0x20
#define CAM_REQ_KITCHEN 0x21            /* + the kitchen menu's index */
#define CAM_REQ_STAGE 0x25              /* the lobby stage's show starts */
#define CAM_REQ_TOWN_TOUR 0x44          /* + the tour */
#define CAM_REQ_LOBBY_EVENT 0x54
#define CAM_REQ_TOWN_PC 0x55
#define CAM_REQ_VERSUS 0x62             /* + the versus defeat's view (2 or 3) */
#define CAM_REQ_LAO_SHAN 0x7C           /* the monsters' views (CamReq_ForMonster) */
#define CAM_REQ_SHEN_GAOREN 0x7E
#define CAM_REQ_ELDER_DRAGON 0x7F       /* Kushala Daora, Rusted Kushala Daora, Chameleos */
#define CAM_REQ_LUNASTRA 0x80           /* Lunastra, Teostra */
#define CAM_REQ_FATALIS 0x83
#define CAM_REQ_FATALIS_AIR 0x89        /* Fatalis in the air (EM_STANCE_AIR) */
/*
 * CamReq_TownTour's tours (CAM_REQ_TOWN_TOUR + the tour), named from what
 * starts them: the first visit to an area after a facility's level rises,
 * or after an area opens (LobbyArea_StartEvent, each once by its event
 * flag), and the ship's departure (ShipMenu).
 */
#define TOWN_TOUR_TAVERN_1 0x0          /* the tavern at level 1 (VILLAGE_FACILITY_TAVERN) */
#define TOWN_TOUR_SMITHY_1 0x1          /* the smithy at level 1 */
#define TOWN_TOUR_PORT_1 0x2            /* the port at level 1 */
#define TOWN_TOUR_TAVERN_2 0x3          /* the tavern at level 2 */
#define TOWN_TOUR_SMITHY_2 0x4
#define TOWN_TOUR_PORT_2 0x5
#define TOWN_TOUR_VILLAGE_LEVEL_2 0x6   /* the village's total level reached 2 (EVENT_FLAG_VILLAGE_LEVEL_2) */
#define TOWN_TOUR_HUB_0xAA 0x7          /* the first visit to AREA_HUB_0xAA */
#define TOWN_TOUR_HUB_0xAB 0x8
#define TOWN_TOUR_HUB_0xAC 0x9
#define TOWN_TOUR_HOUSE_1 0xC           /* the online house's first visit at house level 1 */
#define TOWN_TOUR_HOUSE_2 0xD           /* at house level 2 */
#define TOWN_TOUR_SHIP 0xE              /* the ship leaves (ShipMenu) */
#define TOWN_TOUR_VILLAGE_RETURN 0xF    /* back in the village (AREA_HUB_0x89) from AREA_HUB_0x57, or while village timer 0 runs */
/* The system data's event flag (EventFlag_Set) that adds tour 0-5 to the
 * gallery: the gallery's entry is this + the tour. */
#define TOWN_TOUR_GALLERY_ENTRY 0x1F

/* The views when a quest's last target falls (CamReq_QuestTarget). */
#define CAM_REQ_ELDER_DOWN 0x07         /* Kushala Daora, its rusted kind, Chameleos, Lunastra, Teostra */
#define CAM_REQ_LAO_SHAN_DOWN 0x86      /* Lao-Shan Lung, in AREA_FORTRESS_5 */
#define CAM_REQ_SHEN_GAOREN_DOWN 0x87
#define CAM_REQ_FATALIS_DOWN 0x8A
#define CAM_REQ_NONE 0xFF               /* gQuestTargetCamIds: no view for the family */

/* CamScript.eyeSpace and atSpace: the space the points are in
 * (CamScript_EyePoint, CamScript_AtPoint). */
#define CAM_SPACE_BODY 0        /* the local player's body */
#define CAM_SPACE_POSITION 1    /* the local player's position */
#define CAM_SPACE_PART 2        /* a part of the local player (eyePart) */
#define CAM_SPACE_OBJECT 3      /* obj's matrix */
#define CAM_SPACE_WORLD 4
#define CAM_SPACE_AT 5          /* at[0] (eyeSpace only) */

#include "common.h"
#include "em.h"
#include "player.h"

/*
 * The camera (gCamera points at gCameraData; Camera_Update and
 * Camera_ApplyProjection set it up).
 */
typedef struct Camera {
    f32 eye[3];         /* 0x00 */
    f32 at[3];          /* 0x0C: target */
    f32 up[3];          /* 0x18: from the roll (Camera_ComputeUp) */
    f32 viewDir[3];     /* 0x24: the view direction (at - eye) when not vertical (Camera_Update) */
    f32 nearZ;          /* 0x30: the near depth (Frustum_BuildPlanes; the presets' smaller depth) */
    f32 farZ;           /* 0x34: the far depth (the projection's first depth argument) */
    f32 fov;            /* 0x38: radians (50 degrees by default) */
    f32 fovPrev;        /* 0x3C: fov at the last update (Camera_Update) */
    f32 roll;           /* 0x40 */
    s32 viewport[5];    /* 0x44: x, y, width (512), height (448), 0 */
    f32 viewportFarZ;   /* 0x58: 1.0: the viewport's far Z (RS_ORTHO reads viewport and it as a GsViewport) */
} Camera;

extern Camera *gCamera;
extern Camera gCameraData;

/*
 * The camera controller (gCamCtl, 1664 bytes) keeps one view per camera
 * mode (CamCtl.modes, by kind: 0 the
 * follow camera, 1 the orbit, 2 the area's rail, 3 the aim, 4 the area's fixed
 * camera and the focus, 5 the script). CamCtl_Update runs each mode's init
 * handler (gCamModeInit) once and its update handler (gCamModeUpdate) each
 * frame; CamCtl_RunMode copies the current mode's view (areaMode) to
 * CamCtl.out and hands gCamera the view that overrides it.
 */
/* A camera mode's step and the word after it, as CamScript_Update clears them: packed,
 * so the word (at +2) is written with swl/swr as in the original. */
#pragma pack(1)
typedef struct CamStep {
    u8 step;                    /* 0x0 */
    u8 unk1;
    s32 count;                  /* 0x2 */
} CamStep;
#pragma pack()

/* A camera mode's view (128 bytes; CamCtl.modes and CamCtl.out). */
typedef struct CamView {
    f32 eye[3];                 /* 0x00 */
    f32 at[3];                  /* 0x0C: target */
    f32 up[3];                  /* 0x18: from the roll (Camera_ComputeUp; CamView_KeepTargetOnScreen) */
    f32 dir[3];                 /* 0x24: target - eye (CAM_MODE_RAIL, CamView_KeepTargetOnScreen) */
    f32 eye0[3];                /* 0x30: eye, at, roll and fov when a blend starts */
    f32 at0[3];                 /* 0x3C */
    u8 unk48[0x60 - 0x48];
    f32 roll;                   /* 0x60 */
    f32 fov;                    /* 0x64: radians (0.96 when a mode starts) */
    f32 roll0;                  /* 0x68 */
    f32 fov0;                   /* 0x6C */
    u8 kind;                    /* 0x70: the mode (its index in CamCtl.modes; picks gCamModeUpdate) */
    u8 active;                  /* 0x71: the view is in use (CAM_MODE_AIM to CAM_MODE_SCRIPT: it overrides the current one, CamCtl_RunMode) */
    u8 yawFrom;                 /* 0x72: CAM_MODE_FOLLOW: the mode whose view it takes its yaw from (CamFollow_SetYawFrom; CAM_YAW_FROM_NONE: none) */
    u8 flags;                   /* 0x73: CAM_MODE_SCRIPT: CAM_SCRIPT_* (Select skips a started script) */
    s16 left;                   /* 0x74: the frames left of a blend (CamView_StartBlend) or of a script move */
    s16 length;                 /* 0x76: the script move's length in frames (CamScript_Step; -1: none) */
    u8 step;                    /* 0x78: the mode's step (0: start) */
    u8 unk79;
    u8 blend;                   /* 0x7A: CAM_BLEND_* (CamBlend_EaseStep); in CAM_MODE_SCRIPT, the run's step (CAM_RUN_*) */
    u8 unk7B;
    f32 rate;                   /* 0x7C: 1 / the blend's length (CamView_StartBlend) */
} CamView;
typedef char CamView_size_check[(sizeof(CamView) == 0x80) ? 1 : -1];

/* A distance setting of the follow camera (32 bytes). */
typedef struct CamDist {
    f32 unk0;
    f32 eyeHeight;              /* 0x04: the eye's height above the player */
    f32 eyeBack;                /* 0x08: the eye's distance behind the player */
    f32 unkC;
    f32 atY;                    /* 0x10: target height above the player */
    f32 unk14;
    f32 floorGap;               /* 0x18: minimum eye height above the floor */
    f32 unk1C;
} CamDist;
typedef char CamDist_size_check[(sizeof(CamDist) == 0x20) ? 1 : -1];

/* The follow camera's settings (256 bytes): a CAM_AREA_FOLLOW camera area's (CamArea.follow) or
 * gCamFollowDefaults (Cam_CopyDefaults copies it into the default area). */
typedef struct CamDistSet {
    f32 fov;                    /* 0x00 */
    f32 roll;                   /* 0x04 */
    u8 pad8[0x18];
    CamDist dist[5];            /* 0x20: by CamFollow.distNo (L2/R2), [4] while the player runs at distance 2 */
    u8 padC0[0x100 - 0xC0];
} CamDistSet;
typedef char CamDistSet_size_check[(sizeof(CamDistSet) == 0x100) ? 1 : -1];

/* The follow camera's work (CAM_MODE_FOLLOW, CamCtl.follow at +0x3B0; CamFollow_Init and CamFollow_Update). */
typedef struct CamFollow {
    f32 eye[3];                 /* 0x00 */
    f32 at[3];                  /* 0x0C */
    f32 unk18[3];               /* 0x18: the eye of the frame before, then where the line to it hits */
    f32 eyeGoal[3];             /* 0x24 */
    f32 atGoal[3];              /* 0x30 */
    CamDistSet *set;            /* 0x3C */
    CamDist *dist;              /* 0x40: the setting in use (set->dist[distNo]) */
    f32 fov;                    /* 0x44 */
    f32 fovGoal;                /* 0x48 */
    f32 roll;                   /* 0x4C */
    f32 floorY;                 /* 0x50: the floor under the target */
    s8 unk54;                   /* 0x54 */
    u8 heightState;             /* 0x55: CamFollow_UpdateFallView's state */
    s16 heightTimer;            /* 0x56: its frame count */
    s32 unk58;                  /* 0x58 */
    f32 hitDist;                /* 0x5C: Cam_LineHitDist's distance from the eye */
    s32 unk60;                  /* 0x60 */
    u8 pad64[4];
    s16 yawGoal;                /* 0x68 */
    s16 yaw;                    /* 0x6A */
    u16 held;                   /* 0x6C: the camera's pad keys (CamFollow_GetKeys) */
    u16 pressed;                /* 0x6E */
    u8 distNo;                  /* 0x70: the distance setting (0-3, L2/R2; 2 after CamReq_Reset) */
    u8 pad71;
    u8 wallHit;                 /* 0x72: the eye swung into a wall (AreaGrid_SweepSphere); the yaw then turns back toward the player */
    u8 unk73;                   /* 0x73 */
    u8 blockedTimer;            /* 0x74: 15 while the player is blown away and the view to him is blocked, then counts
                                   down (CamFollow_UpdateBlocked; the distance keys clear it): the blocked distance setting */
    u8 eyeHeld;                 /* 0x75: the eye is held where both sweeps hit a wall (CamFollow_Update;
                                   cleared when the target's sweep to eyeGoal is clear) */
    u8 unk76;                   /* 0x76: frames the pad's turn holds (8, 20 while the view is held) */
    u8 turningBehind;           /* 0x77: set while the yaw turns behind the player (CamFollow_ShouldTurnBehind; cleared
                                   within a sixteenth of a turn of it; CamCtl_IsTurningBehind) */
    u8 unk78;                   /* 0x78 */
    u8 unk79;                   /* 0x79: set when the eye was raised above the floor */
    s8 canFollow;               /* 0x7A: Cam_PlayerCanFollow (CamFollow_UpdateCanFollow; CamFollow_GetCanFollow) */
    s8 followDelay;             /* 0x7B: frames before it follows again (3) */
    u8 blownAway;               /* 0x7C: offline, the player is blown away (CamFollow_IsBlownAway) */
    u8 pad7D[3];
} CamFollow;
typedef char CamFollow_size_check[(sizeof(CamFollow) == 0x80) ? 1 : -1];

/* The orbit camera's work (CAM_MODE_ORBIT, CamCtl.orbit at +0x430; CamOrbit_Update). */
typedef struct CamOrbit {
    f32 eye[3];                 /* 0x00 */
    f32 at[3];                  /* 0x0C */
    f32 unk18[3];               /* 0x18 */
    s16 pitch;                  /* 0x24 */
    s16 yaw;                    /* 0x26: added to CamFollow.yaw */
    s16 dir;                    /* 0x28: where yaw heads, up to a thirty-second of a turn either way */
    u8 pad2A[0x40 - 0x2A];
} CamOrbit;
typedef char CamOrbit_size_check[(sizeof(CamOrbit) == 0x40) ? 1 : -1];

/* CAM_MODE_RAIL's work (CamCtl.rail at +0x470): the eye and target it moves along the
 * camera area's path (CamRail_Update). */
typedef struct CamRail {
    f32 pos[3];                 /* 0x00 */
    f32 at[3];                  /* 0x0C */
    f32 roll;                   /* 0x18 */
    f32 fov;                    /* 0x1C */
    s16 unk20;                  /* 0x20 */
    s16 unk22;                  /* 0x22 */
    u8 pad24[4];
    s16 rx;                     /* 0x28 */
    s16 ry;                     /* 0x2A */
    u8 pad2C[0x40 - 0x2C];
} CamRail;
typedef char CamRail_size_check[(sizeof(CamRail) == 0x40) ? 1 : -1];

/* The aim camera's work (CAM_MODE_AIM, CamCtl.aim at +0x4B0; CamAim_Update, CamAim_GetState and
 * CamAim_Set): while the player aims, a view fixed to the player by its class. */
typedef struct CamAim {
    /* 0x00 */ f32 mtx[16];             /* [12..14] the translation */
    /* 0x40 */ f32 pos[3];
    /* 0x4C */ f32 fovMin;              /* the zoom's field of view: 15 degrees */
    /* 0x50 */ f32 fovMax;              /* 60 degrees */
    /* 0x54 */ f32 fovScale;            /* 1 / (fovMax - fovMin): CamAim_GetState's zoom from 0 to 1 */
    /* 0x58 */ f32 fov0;                /* the zoomed field of view, class 0 */
    /* 0x5C */ f32 fov1;                /* class 1 */
    /* 0x60 */ s16 rx;                  /* class 3: the angles and pos CamAim_Set gives */
    /* 0x62 */ s16 ry;
    /* 0x64 */ s8 kind;                 /* the class (CamAim_Situation) */
    /* 0x65 */ u8 unk65;                /* 1, 17 or 0 by the class */
    /* 0x66 */ u8 zoom;                 /* set when the right stick zooms */
} CamAim;
typedef char CamAim_size_check[(sizeof(CamAim) == 0x68) ? 1 : -1];

/* The camera focus, gCamCtl + 0x530 (CamFixed.focus): the view eases to an object (the lobby's
 * NPCs and menus: CamFocus_Start, CamFocus_End), the player or a position. */
typedef struct CamFocus {
    /* 0x00 */ f32 pos[3];
    /* 0x0C */ f32 at[3];
    /* 0x18 */ u8 state;
    /* 0x19 */ u8 pad19[3];
    /* 0x1C */ struct Em *obj;
    /* 0x20 */ f32 height;             /* kind 1: the target's height above the player (the pad: 40 to 160) */
    /* 0x24 */ f32 dist;               /* kind 1: the eye's distance from the target (the pad: 120 to 375) */
    /* 0x28 */ f32 angle;
    /* 0x2C */ s16 timer;
    /* 0x2E */ u8 on;                  /* wanted: CamFocus_Start sets it, CamFocus_End clears it (CamFocus_IsOn);
                                          CamFocus_Update eases in over 20 frames while it is set, out after */
    /* 0x2F */ u8 kind;                /* 0 obj (pos its position), 1 the player (height, dist), 2 the position pos */
    /* 0x30 */ s16 asideTimer;         /* kind 0: frames into the turn aside (0 to 20) */
    /* 0x32 */ u8 aside;               /* kind 0: the target turns 0.45 fov aside from obj (CamFocus_SetAside: the
                                          lobby's menus open beside the NPC) */
} CamFocus;
typedef char CamFocus_size_check[(sizeof(CamFocus) == 0x34) ? 1 : -1];

/* CAM_MODE_FIXED's work (CamCtl.fixed at +0x520): the area's fixed camera (CamFixed_UpdateTarget) and the focus. */
typedef struct CamFixed {
    /* 0x00 */ u8 started;              /* cleared (as a word) while the player's flag 0x80000 is off */
    /* 0x01 */ u8 pad1[3];
    /* 0x04 */ s32 on;                  /* Player_IsFishing */
    /* 0x08 */ void *spot;              /* the player's action spot when it started (none: no fixed camera) */
    /* 0x0C */ u8 padC[4];
    /* 0x10 */ CamFocus focus;        /* a view eased to a position or an object (CamFocus_Start; CamFocus_Update) */
} CamFixed;
typedef char CamFixed_size_check[(sizeof(CamFixed) == 0x44) ? 1 : -1];

/* A position on a path, gCamCtl + 0x64C. */
typedef struct PathPos {
    /* 0x0 */ f32 dist;                 /* along segment k */
    /* 0x4 */ f32 t;                    /* dist / the segment's length */
    /* 0x8 */ u8 k;                     /* the segment */
} PathPos;

/* A camera shake request (20 bytes; CamCtl.shakeA/B/C): CamShake_Set* and
 * CamShake_Req* fill it, CamCtl_RunMode ticks it and CamShake_Apply plays it. */
typedef struct CamShake {
    /* 0x00 */ f32 pos[3];            /* where it comes from */
    /* 0x0C */ u8 active;             /* a request is pending (CamCtl_Init clears it) */
    /* 0x0D */ u8 kind;               /* gCamShakeLengths index; | 0x80: felt everywhere (CamShake_SetAEverywhere, no position) */
    /* 0x0E */ s16 length;            /* gCamShakeLengths[kind] */
    /* 0x10 */ u8 range;              /* the gCamShakeRanges index (CamShake_ReqARange's flag, else 0) */
    /* 0x11 */ u8 pad11[3];
} CamShake;
typedef char CamShake_size_check[(sizeof(CamShake) == 0x14) ? 1 : -1];
/* A swing of the camera script (CamScript_Place): a cosine wave whose phase steps by speed
 * each frame, its amplitude eased from amp[0] to amp[1] as the move goes. */
typedef struct CamSwing {
    s16 phase;                  /* 0x0: an angle, ANGLE_TURN to a turn (Math_CosAngle) */
    s16 speed;                  /* 0x2: added to phase each frame (0: no swing) */
    s16 amp[2];                 /* 0x4: the amplitude at the move's start and at its end */
} CamSwing;
typedef char CamSwing_size_check[(sizeof(CamSwing) == 0x8) ? 1 : -1];

/* A camera script's command (CamScript_Step), a whole number of 4-byte words. */
typedef struct CamCmd {
    s8 kind;                    /* 0x0: CAMCMD_* */
    s8 words;                   /* 0x1: the command's length in words */
    union {
        s8 byte[2];             /* 0x2: an index, a mode or a flag, and a second one */
        s16 half;               /* 0x2: an angle, a roll, a fov, a length, a swing's value, the ease */
    } arg;
    s32 point[3];               /* 0x4: CAMCMD_EYE_POINT's and CAMCMD_AT_POINT's point (in 1/4096) */
} CamCmd;

/* Mode 5's work: the camera script (CamCtl.script, 144 bytes; CamScript_Run, CamScript_Step,
 * CamScript_Place). A move eases each pair from [0], its start, to [1], its end, over the
 * script view's left / length (modes[5]); when it ends, CamScript_Step copies [1] into [0]. */
typedef struct CamScript {
    f32 eye[2][3];              /* 0x00: the eye at the move's start and end (CAMCMD_EYE_POINT; the view's at the start) */
    f32 at[2][3];               /* 0x18: the target's (CAMCMD_AT_POINT) */
    s8 *pc;                     /* 0x30: the next command (gDmcamFileIds[id], or the dmcam file loaded for it: Dmcam_Find) */
    s8 *loop;                   /* 0x34: the command after the loop's start (CAMCMD_LOOP; CAMCMD_LOOP_FRAME and CAMCMD_LOOP_PLACE go back to it) */
    u8 place;                   /* 0x38: CAM_PLACE_* (CAMCMD_PLACE; CAM_PLACE_EASE at the start) */
    u8 eyeSpace;                /* 0x39: the eye points' space (CAMCMD_EYE_SPACE; CamScript_EyePoint: CAM_SPACE_*) */
    u8 atSpace;                 /* 0x3A: the target points' (CAMCMD_AT_SPACE; CamScript_AtPoint, no CAM_SPACE_AT) */
    u8 eyePart;                 /* 0x3B: the player's part for CAM_SPACE_PART (CamScript_Place's orbit turns by
                                   atSpace with it, and by eyeSpace with atPart) */
    u8 atPart;                  /* 0x3C: the part for atSpace CAM_SPACE_PART */
    u8 pivotEye;                /* 0x3D: the orbit: 1 the target turns around the eye, else the eye around the target (CAMCMD_PIVOT) */
    u8 id;                      /* 0x3E: the script (CamCtl.request; the gDmcamFileIds index) */
    s8 state;                   /* 0x3F: 1 running, -1 held at a CAMCMD_END, 0 none (CamReq_State) */
    void *obj;                  /* 0x40: the request's object (CamCtl.requestArg: an Em or a Player; CAM_SPACE_OBJECT is its worldMtx) */
    f32 dist[2];                /* 0x44: the orbit's distance at the start and end (CAMCMD_DIST0, CAMCMD_DIST1: in 1/16) */
    s16 angle[2][2];            /* 0x4C: the orbit's x and y angles at the start and end (CAMCMD_ANGLE_*; CamScript_OrbitAngles) */
    s16 roll[2];                /* 0x54: CAMCMD_ROLL0, CAMCMD_ROLL1 (ANGLE_TURN to a turn) */
    s16 fov[2];                 /* 0x58: CAMCMD_FOV0, CAMCMD_FOV1 */
    CamSwing swing[4];          /* 0x5C: the view's sideways and upward sways, the roll's swing and the fov's (CAMCMD_SWAY_*, CAMCMD_*_SWING_*) */
    u8 swingRand[4];            /* 0x7C: per swing, nonzero adds noise to the roll's and the fov's (CAMCMD_SWING_RAND) */
    s8 *mark;                   /* 0x80: where CAMCMD_GOTO_MARK jumps (CAMCMD_SET_MARK sets it; the script's start) */
    f32 easePow;                /* 0x84: the ease's exponent (CAMCMD_EASE: its low 14 bits / 1024) */
    u8 ease;                    /* 0x88: CAM_EASE_* (CAMCMD_EASE: its top 2 bits) */
    u8 pad89[0x90 - 0x89];
} CamScript;
typedef char CamScript_size_check[(sizeof(CamScript) == 0x90) ? 1 : -1];

/* A key of a camera area's path (16 bytes): a point and the length of the segment that
 * starts there. */
typedef struct PathKey {
    f32 pos[3];
    f32 len;                    /* 0x0C */
} PathKey;

/* A camera area's path (kind CAM_AREA_PATH, at CamArea +0x20): the keys the eye moves along, a second set
 * the player's position is placed on (PathPos_PlaceNear), and a fov and a roll per key. */
typedef struct PathData {
    PathKey keys[16];           /* 0x000 */
    PathKey keys2[16];          /* 0x100: a second set (PathPos_PlaceNear) */
    f32 vals[16];               /* 0x200: a value per key (PathKey_Blend: the fov) */
    s16 angles[16];             /* 0x240: an angle per key (PathKey_BlendAngle: the roll) */
    u8 count;                   /* 0x260 */
    u8 nearest3D;               /* 0x261: PathSeg_Nearest's nearest point: 0 in the x-z plane, else in 3D */
} PathData; /* 0x264 */
typedef char PathData_size_check[(sizeof(PathData) == 0x264) ? 1 : -1];

/* A cubic segment of a path (Path_BuildSpline builds them from the keys):
 * ((c[0] t + c[1]) t + c[2]) t + c[3] (Vec_EvalCubic). */
typedef struct PathSeg {
    f32 c[4][3];
} PathSeg;
typedef char PathSeg_size_check[(sizeof(PathSeg) == 0x30) ? 1 : -1];

/* A complex number: the nearest point of a path's segment comes from the real
 * roots of a quintic, which Quintic_Roots finds in complex numbers. */
typedef struct Complex {
    f32 x;
    f32 y;
} Complex;

/* A face of a camera area (64 bytes, CamArea.faces): the player is in the area when in front
 * of one of its faces, no farther than depth, and inside its quad in the x-z plane
 * (CamArea_TestObj, CamFace_ContainsXZ). */
typedef struct CamFace {
    f32 quad[8];                /* 0x00: the corners (quad[0], quad[2]), (quad[4], quad[6]),
                                   (quad[1], quad[3]) and (quad[5], quad[7]) (x, z) */
    f32 pos[3];                 /* 0x20: a point on the face */
    u8 flags;                   /* 0x2C: a face with a flag in CamArea_TestObj's mask is skipped */
    u8 pad2D[3];
    f32 normal[3];              /* 0x30 */
    f32 depth;                  /* 0x3C: how far in front of the face the area reaches */
} CamFace;
typedef char CamFace_size_check[(sizeof(CamFace) == 0x40) ? 1 : -1];

/* A camera area of kind CAM_AREA_FIXED or CAM_AREA_OFFSET (at CamArea +0x20; CamRail_Update, CamPoint_GetPos). */
typedef struct CamAreaFixed {
    f32 eye[3];                 /* 0x20: CAM_AREA_FIXED: the eye in the world; CAM_AREA_OFFSET: its offset from the player */
    f32 at[3];                  /* 0x2C: the look point in the world (lookSpace CAM_LOOK_SPACE_WORLD); lookLock keeps its y, or its x and z */
    f32 atLocal[3];             /* 0x38: the look point in the player's frame (lookSpace CAM_LOOK_SPACE_PLAYER, through its worldMtx) */
    f32 atOffset[3];            /* 0x44: the look point's offset from the player's position (lookSpace CAM_LOOK_SPACE_OFFSET) */
    f32 fov;                    /* 0x50: CamRail.fov, scaled by the eye's distance (CamArea.fovDist) */
    f32 roll;                   /* 0x54: CamRail.roll */
} CamAreaFixed;

/* A camera area of CAM_AREA_PATH (at CamArea +0x20): the path and the look point
 * (CamRail_Update, CamPoint_GetPosOr: lookSpace CAM_LOOK_SPACE_WORLD is the path's point nearest the player). */
typedef struct CamAreaPath {
    PathData d;                 /* 0x020 */
    f32 unk284[3];              /* 0x284 */
    f32 atLocal[3];             /* 0x290: the look point in the player's frame (lookSpace CAM_LOOK_SPACE_PLAYER) */
    f32 atOffset[3];            /* 0x29C: its offset from the player's position (lookSpace CAM_LOOK_SPACE_OFFSET) */
} CamAreaPath;

/* An entry of a camera area's transition table (CamArea.trans). */
typedef struct CamTrans {
    u8 areaId;                  /* 0x0: the camera area the player comes from (CamCtl.prevAreaId) */
    u8 frames;                  /* 0x1: the blend's length from it (CamView_StartBlend; 0 and 1 cut) */
} CamTrans;

/* A camera area (768 bytes, then faceCount faces, then the 32-byte transition table when
 * trans is set: CamData_Relocate). The player's area (CamCtl.area: Cam_FindPlayerArea) picks
 * the camera by its kind. CamCtl_InitDefaultParams writes a default area of CAM_AREA_FOLLOW. */
typedef struct CamArea {
    u8 unk00;
    u8 id;                      /* 0x01: CamCtl.areaId */
    u8 kind;                    /* 0x02: 0 the follow camera (areaMode 0) with this area's follow settings; 1 a fixed
                                   eye, 2 the eye along the path, 3 an eye offset from the player (areaMode 2, CamRail_Update) */
    u8 lookLock;                /* 0x03: the look point's axes kept from the area's point (CamPoint_GetPos): 0 y,
                                   1 x and z, 2 none (the default) */
    u8 lookSpace;               /* 0x04: the look point's space: 0 the world, 1 the player's frame, 2 the player's
                                   position plus an offset */
    u8 faceCount;               /* 0x05: the faces of 64 bytes after the area's 768 bytes (CamData_Relocate) */
    u8 flags;                   /* 0x06: 0x80: the area counts only while the local player's action is in
                                   Player_IsOnWall's range (CamArea_IsEnabled) */
    u8 unk07;
    f32 fovDist[2];             /* 0x08: the eye's distances where the fov scale is fovScale[0] and fovScale[1]
                                   (CamArea_FovScale: linear between them; 400.0 and 2400.0 in the default) */
    f32 fovScale[2];            /* 0x10: 1.0 and 0.75 in the default */
    CamFace *faces;             /* 0x18: an offset from the area in the file, a pointer once relocated */
    CamTrans *trans;            /* 0x1C: the same, when set: CAM_TRANS_COUNT entries, the length of the
                                   transition from each area (CamView_SetTransition) */
    union {
        CamDistSet follow;      /* 0x20: CAM_AREA_FOLLOW (CamFollow.set; gCamFollowDefaults in the default) */
        CamAreaFixed fixed;     /* 0x20: kinds 1 and 3 */
        CamAreaPath path;       /* 0x20: CAM_AREA_PATH */
        u8 pad20[0x2E0];
    };
} CamArea; /* 0x300 */
typedef char CamArea_size_check[(sizeof(CamArea) == 0x300) ? 1 : -1];

/* The area's camera data (gSysSettings.camData, loaded by Area_LoadCamData; CamCtl.data): this
 * header, then areaCount camera areas from +0x30 (each 768 bytes, then faceCount faces of 64
 * bytes, then 32 more bytes when its unk1C is set), the grid's cells and their area lists. The
 * file holds offsets from the header where the pointers are: CamData_Relocate turns them into
 * pointers. CamCtl_InitDefaultParams writes a default header and one area when the area has none. */
typedef struct CamCell {
    s32 count;                  /* 0x00: the camera areas listed for the cell */
    CamArea **areas;            /* 0x04: the list (Cam_FindPlayerArea tests each with CamArea_TestObj) */
} CamCell;

typedef struct CamDataHdr {
    u16 version;                /* 0x00: CAM_DATA_VERSION in the default */
    u16 areaCount;              /* 0x02: the camera areas from +0x30 (CamData_Relocate) */
    u16 gridW;                  /* 0x04: the grid's cells per row (CamCtl.gridW) */
    u16 gridH;                  /* 0x06: its rows: gridW * gridH cells */
    /* 0x08-0x1B: seen only copied to CamCtl.unk394 ... unk3A4 (CamCtl_LoadParams), which
     * nothing reads; the default sets 20000, 20000, 0, 0, 20000, 20000 (a 1 x 1 grid). */
    u16 unk08;
    u16 unk0A;
    s32 unk0C;
    s32 unk10;
    s32 unk14;
    s32 unk18;
    CamCell *cells;             /* 0x1C: the grid (NULL: none) */
    CamArea **areaLists;        /* 0x20: the cells' area lists (pointers to areas once relocated) */
    s32 unk24;                  /* 0x24: seen: 0 in the default; nothing reads it */
    CamArea *areas;             /* 0x28: the first camera area (the header's end, +0x30) */
    s32 unk2C;
} CamDataHdr; /* 0x30 */

typedef char CamDataHdr_size_check[(sizeof(CamDataHdr) == 0x30) ? 1 : -1];

/* The camera controller, gCamCtl (0x5AF420, 1664 bytes), cleared by CamCtl_Clear. */
typedef struct CamCtl {
    /* 0x000 */ CamView out;            /* the current mode's view (CamCtl_RunMode: modes[areaMode]) */
    /* 0x080 */ CamView modes[6];       /* each mode's view, by kind; modes[5] is the script's (the state machine; CamCtl_Apply) */
    /* 0x380 */ u8 inited;             /* CamCtl_Update sets the modes up while it is 0 (CamCtl_Init clears it as a word) */
    /* 0x381 */ u8 pad381[3];
    /* 0x384 */ struct Player *player;  /* the player it follows (CamCtl_Init: the local one) */
    /* 0x388 */ CamDataHdr *data;       /* the loaded camera data (CamData_Relocate; CamCtl_InitDefaultParams: the default) */
    /* 0x38C */ CamArea *area;         /* the camera area the player is in (Cam_FindPlayerArea, from data):
                                          its kind picks areaMode (0: 0, 1-3: 2, CamRail_Update) */
    /* 0x390 */ u16 gridW;             /* the area grid's row length (Cam_FindPlayerArea: cell cellX + it * cellZ): data->gridW */
    /* 0x392 */ u16 gridH;             /* CamCtl_LoadParams copies 0x390-0x3A7 from the data's header (+0x04-0x1B) */
    /* 0x394 */ u16 unk394;            /* 0x394-0x3A7: the header's unk08 ... unk18; nothing reads them */
    /* 0x396 */ u16 unk396;
    /* 0x398 */ s32 unk398;
    /* 0x39C */ s32 unk39C;
    /* 0x3A0 */ s32 unk3A0;
    /* 0x3A4 */ s32 unk3A4;
    /* 0x3A8 */ u16 cellX;             /* the grid cell Cam_FindPlayerArea looks in (cleared by CamCtl_Update) */
    /* 0x3AA */ u16 cellZ;
    /* 0x3AC */ u8 keepFirstArea;            /* while set, Cam_FindPlayerArea keeps the map's first area (cleared each frame) */
    /* 0x3AD */ u8 areaId;             /* the current area's id (area[1]; 255 before the first) */
    /* 0x3AE */ u8 prevAreaId;         /* the one before (CAM_YAW_FROM_NONE: none) */
    /* 0x3AF */ u8 areaChanged;        /* set for the frame the area changed (CAM_MODE_RAIL turns at once) */
    /* 0x3B0 */ CamFollow follow;       /* CAM_MODE_FOLLOW's work */
    /* 0x430 */ CamOrbit orbit;         /* CAM_MODE_ORBIT's */
    /* 0x470 */ CamRail rail;           /* CAM_MODE_RAIL's */
    /* 0x4B0 */ CamAim aim;             /* CAM_MODE_AIM's */
    /* 0x518 */ u8 unk518[0x520 - 0x518];
    /* 0x520 */ CamFixed fixed;         /* CAM_MODE_FIXED's, with the focus */
    /* 0x564 */ u8 unk564[0x580 - 0x564];
    /* 0x580 */ CamScript script;       /* CAM_MODE_SCRIPT's: the camera script */
    /* 0x610 */ CamShake shakeA;        /* the shake request slots */
    /* 0x624 */ CamShake shakeB;
    /* 0x638 */ CamShake shakeC;
    /* 0x64C */ PathPos pathPos;
    /* 0x658 */ u8 pad658[0x65C - 0x658];
    /* 0x65C */ u8 prevAreaMode;       /* areaMode the frame before */
    /* 0x65D */ u8 areaMode;           /* the current mode: 0, or 2 where the area asks for its path (the orbit sets 1) */
    /* 0x65E */ u16 padHeld;           /* the pad as the camera reads it (CamCtl_ReadPad; all 0 while it is not allowed) */
    /* 0x660 */ u16 padPressed;
    /* 0x662 */ u16 padDir;
    /* 0x664 */ u16 padDegR;           /* the right stick's angle and magnitude (CAM_MODE_AIM zooms and turns by them) */
    /* 0x666 */ u16 padMagR;
    /* 0x668 */ u8 *targetObj;          /* CamCtl_SetTargetObj (the local player's) */
    /* 0x66C */ void *requestArg;       /* the request's object (CamReq_Set; CamScript.obj) */
    /* 0x670 */ u8 request;             /* the request to the state machine */
    /* 0x671 */ s8 override;            /* the mode whose view overrides the current one (5, 3 or 4; -1: none; CamCtl_RunMode) */
    /* 0x672 */ u8 followView;          /* set while the follow camera (CAM_MODE_FOLLOW) shows the view: CamCtl_CheckPlayerInView runs, CamCtl_GetPlayerFade gives playerFade */
    /* 0x673 */ u8 playerInView;        /* the player was in view (CamCtl_CheckPlayerInView; it then tests a wider frustum, 50 instead of 45) */
    /* 0x674 */ f32 playerFade;         /* for Player_DrawLocal when followView is set (0.3 while the monster is in view) */
    /* 0x678 */ s32 mode;               /* CamCtl_SetMode */
    /* 0x67C */ u8 pad67C[4];
} CamCtl;
typedef char CamCtl_size_check[(sizeof(CamCtl) == 0x680) ? 1 : -1];

/* A camera mode's init or update handler (gCamModeInit, gCamModeUpdate): the controller and
 * the mode's view. */
typedef void (*CamEntFn)(CamCtl *c, CamView *v);
s32 CamScript_Run(CamCtl *cam, CamView *view);
void CamScript_OrbitAngles(s16 *out, CamCtl *cam);
void CamScript_Place(CamCtl *cam, CamView *view, CamScript *camScript);
s32 CamScript_Step(CamCtl *cam, CamView *view, CamScript *script);
void CamShake_Apply(CamShake *shake);
void CamPoint_GetPos(CamCtl *ctl, f32 *out, CamArea *area);
void CamPoint_GetPosOr(CamCtl *ctl, f32 *out, CamArea *area, f32 *pathPoint);
void Path_PointAt(f32 *out, CamCtl *ctl, CamArea *area, PathSeg *segs);
s32 Path_NearestSeg(PathData *path, f32 *target);
s32 PathPos_Build(PathPos *pos, PathSeg *segs, PathData *path, f32 *target);
void PathPos_MoveOnSeg(PathPos *pos, PathData *path, s32 goalSeg, f32 goalDist);
s32 PathPos_MoveToward(PathPos *pos, PathData *path, PathSeg *segs, f32 *target);
s32 PathPos_PlaceNear(PathPos *pos, PathData *path, PathSeg *segs, f32 *target);
s32 CamCtl_PathPlace(CamCtl *ctl, PathSeg *segs, f32 *pos, s32 moveToward);
void Vec_EvalCubic(f32 *out, PathSeg *seg, f32 frac);
s32 PathSeg_Nearest(f32 *cands, PathSeg *seg, f32 *target, s32 axes);
f32 CamArea_FovScale(CamArea *area, f32 dist);
f32 PathKey_Blend(PathData *path, s32 keyNo, f32 frac);
f32 PathKey_BlendAngle(PathData *path, s32 keyNo, f32 frac);
void Path_BuildSpline(PathSeg *seg, PathKey *keys, s32 keyCount);
void Quintic_Roots(Complex *roots, f32 *coefs);
s32 Cubic_Roots(f32 *out, f32 *coefs);
f32 Cam_LineHitDist(f32 *fromPos, f32 *toPos, f32 *dist);
void Cam_PushOutOfEms(f32 *pos, f32 *unused1);
void CamReq_QuestTarget(s32 flag);
extern u32 *gDmcamFileIds[140];

extern CamCtl gCamCtl;

void CamCtl_Clear(void);
void CamReq_Reset(void);
void CamCtl_Init(void);
void CamData_Relocate(CamDataHdr *hdr);
void CamCtl_Update(s32 mode);
void CamCtl_Apply(void);
s32 CamCtl_GetMode(void);
void CamFollow_Init(CamCtl *ctl, CamView *view);
void CamFollow_Update(CamCtl *ctl, CamView *view);
s32 CamCtl_IsTurningBehind(void);
void CamCtl_SetTargetObj(u8 *obj);
void CamCtl_TurnToPlayerFront(u8 *obj);
void CamFollow_GetKeys(CamCtl *ctl, CamFollow *work);
void CamFollow_SetYawFrom(s32 mode);
void CamFollow_UpdateBlocked(CamCtl *ctl, CamFollow *work);
s32 CamFollow_UpdateFallView(CamFollow *work);
f32 CamFollow_FloorY(f32 *pos, Player *pl);
void CamFollow_UpdateCanFollow(CamFollow *work, Player *pl);
s8 CamFollow_GetCanFollow(void);
s32 CamFollow_IsBlownAway(void);
void CamOrbit_Init(CamCtl *ctl, CamView *view);
void CamOrbit_Update(CamCtl *ctl, CamView *view);
void CamScript_Init(CamCtl *ctl, CamView *view);
void CamScript_Update(CamCtl *ctl, CamView *view);
void CamReq_Set(s32 req, void *arg);
void CamReq_Start(s32 req, void *arg);
void CamReq_Change(s32 req, void *arg);
void CamReq_Cancel(void);
s32 CamReq_State(void);
s32 CamReq_Started(void);
void CamReq_BossHunted(void);
void CamReq_ForMonster(Em *em);
void CamReq_PlayerDown(void);
void CamReq_Cart(void);
void CamReq_AreaIntro(void);
void CamReq_PlesiothScene(void *em);
void CamReq_LobbyEvent(void);
void CamReq_TownPc(void);
void CamReq_Contest(s32 contest, s32 spot);
void CamReq_Minigame(s32 game);
void CamReq_HairMenu(void);
void CamReq_TownArea(s32 area);
void CamReq_VillageBoard(void);
void CamReq_Kitchen(s32 index);
void CamReq_StageStart(void);
void CamReq_StageShot(u16 track, u16 shot);
void CamReq_VersusDefeat(void *em, u8 view);
void CamReq_TownTour(s32 tour);
void Boot_ResetMenus(void);
s32 CamReq_ResetStub(void);
extern u8 gEffectColor0Green;
extern u8 gEffectColor0Blue;

void CamRail_Init(CamCtl *ctl, CamView *view);
void CamRail_Update(CamCtl *ctl, CamView *view);
void CamAim_Init(CamCtl *ctl, CamView *view);
void CamAim_Update(CamCtl *ctl, CamView *view);
s32 CamAim_IsSpecialMotion(Player *pl);
void Mtx_FromAngles16Pos(f32 *mtx, s16 pitch, s16 yaw, f32 *pos);
s32 CamAim_Situation(Player *pl);
s8 CamAim_GetState(s32 unused, u8 *aimFlags, u8 *zooms, f32 *zoomT);
void CamAim_Set(f32 *pos, s16 pitch, s16 yaw);
void CamFixed_Init(CamCtl *ctl, CamView *view);
void CamFixed_Update(CamCtl *ctl, CamView *view);
void CamFixed_UpdateTarget(CamCtl *ctl, CamView *view, CamFixed *fixedWork);
s32 CamFixed_AreaCamera(CamCtl *ctl, CamView *view, CamFixed *fixedWork);
void CamFocus_Start(struct Em *);
void CamFocus_End(void);
s32 CamFocus_IsOn(void);
void CamFocus_SetAside(s8 asideOn);
s32 CamFocus_GetState(void);
f32 CamFocus_Ease(s32 unused, s16 frame, u8 ease);
void CamFocus_Update(CamCtl *ctl, CamView *view, CamFocus *focusWork);

void Camera_Init(void);
void Camera_Reset(void);
void Camera_Update(void);
void Camera_SetPreset(s32 id);
void Camera_BuildFrustum(f32 farZ);
void Camera_ApplyProjection(void);
void Camera_SetFov(f32 fov);
void Camera_RestoreFov(void);
void Camera_SetRoll(f32 roll);
void Camera_StoreFov(f32 fov);
f32 Camera_Distance(void *pos);
u16 Camera_GetYaw(void);
void Camera_ComputeUp(f32 *up, f32 *eye, f32 *at, f32 roll);

#endif /* CAMERA_H */
