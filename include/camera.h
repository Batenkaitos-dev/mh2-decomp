#ifndef CAMERA_H
#define CAMERA_H

/*
 * The camera modes: CamCtl.modes' index, in the order of the handler tables
 * gCamModeInit and gCamModeUpdate (unk_31E190.c); CamCtl.areaMode holds the
 * one the area picks. CAM_MODE_SCRIPT runs the scripts (CamScript_Init;
 * unk_31DF70.c).
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

/* CamView.flags in the script mode. */
#define CAM_SCRIPT_ENDED 0x01   /* the script ended */
#define CAM_SCRIPT_STARTED 0x02 /* started by CamReq_Start (CamReq_Started tests it) */

/* CamScript.eyeSpace and atSpace: the space the points are in
 * (CamScript_EyePoint, CamScript_AtPoint). */
#define CAM_SPACE_BODY 0        /* the local player's body */
#define CAM_SPACE_POSITION 1    /* the local player's position */
#define CAM_SPACE_PART 2        /* a part of the local player (eyePart) */
#define CAM_SPACE_OBJECT 3      /* obj's matrix */
#define CAM_SPACE_WORLD 4
#define CAM_SPACE_AT 5          /* at[0] (eyeSpace only) */

#include "common.h"

/*
 * The camera (gCamera points at gCameraData; unk_376C00.c sets it up,
 * Camera_Update, Camera_ApplyProjection).
 */
typedef struct Camera {
    f32 eye[3];         /* 0x00 */
    f32 at[3];          /* 0x0C: target */
    f32 up[3];          /* 0x18: from the roll (Camera_ComputeUp) */
    f32 unk24[3];       /* 0x24: the view direction when not vertical */
    f32 unk30;          /* 0x30: projection parameter (with unk34) */
    f32 unk34;          /* 0x34 */
    f32 fov;            /* 0x38: radians (50 degrees by default) */
    f32 fovPrev;        /* 0x3C: fov at the last update (Camera_Update) */
    f32 roll;           /* 0x40 */
    s32 viewport[5];    /* 0x44: x, y, width (512), height (448), 0 */
    f32 unk58;          /* 0x58: 1.0 */
} Camera;

extern Camera *gCamera;
extern Camera gCameraData;

/*
 * The camera controller (gCamCtl, 0x680 bytes; unk_314E00.c ..
 * unk_31E190.c) keeps one view per camera mode (CamCtl.modes, by kind: 0 the
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

/* A camera mode's view (0x80 bytes; CamCtl.modes and CamCtl.out). */
typedef struct CamView {
    f32 eye[3];                 /* 0x00 */
    f32 at[3];                  /* 0x0C: target */
    f32 up[3];                  /* 0x18: from the roll (Camera_ComputeUp; CamView_KeepTargetOnScreen) */
    f32 dir[3];                 /* 0x24: target - eye (camera mode 2, CamView_KeepTargetOnScreen) */
    f32 eye0[3];                /* 0x30: eye, at, roll and fov when a blend starts */
    f32 at0[3];                 /* 0x3C */
    u8 unk48[0x60 - 0x48];
    f32 roll;                   /* 0x60 */
    f32 fov;                    /* 0x64: radians (0.96 when a mode starts) */
    f32 roll0;                  /* 0x68 */
    f32 fov0;                   /* 0x6C */
    u8 kind;                    /* 0x70: the mode (its index in CamCtl.modes; picks gCamModeUpdate) */
    u8 active;                  /* 0x71: the view is in use (modes 3-5: it overrides the current one, CamCtl_RunMode) */
    u8 yawFrom;                 /* 0x72: mode 0: the mode whose view it takes its yaw from (CamFollow_SetYawFrom; 0xFF: none) */
    u8 flags;                   /* 0x73: mode 5: 1 the script ended, 2 Start skips it (CamReq_Start), 4 (CamScript_Step) */
    s16 left;                   /* 0x74: the frames left of a blend (CamView_StartBlend) or of a script move */
    s16 length;                 /* 0x76: the script move's length in frames (CamScript_Step; -1: none) */
    u8 step;                    /* 0x78: the mode's step (0: start) */
    u8 unk79;
    u8 blend;                   /* 0x7A: 1 a blend starts, 2 it runs (CamBlend_EaseStep: 0 when done); mode 5: the run's step */
    u8 unk7B;
    f32 rate;                   /* 0x7C: 1 / the blend's length (CamView_StartBlend) */
} CamView;
typedef char CamView_size_check[(sizeof(CamView) == 0x80) ? 1 : -1];

/* A distance setting of the follow camera (0x20 bytes). */
typedef struct CamDist {
    f32 unk0;
    f32 back;                   /* 0x04: distance behind the player */
    f32 up;                     /* 0x08: eye height */
    f32 unkC;
    f32 atY;                    /* 0x10: target height above the player */
    f32 unk14;
    f32 floorGap;               /* 0x18: minimum eye height above the floor */
    f32 unk1C;
} CamDist;
typedef char CamDist_size_check[(sizeof(CamDist) == 0x20) ? 1 : -1];

/* The follow camera's settings (0x100 bytes): a kind 0 camera area's (CamArea.follow) or
 * gCamFollowDefaults (Cam_CopyDefaults copies it into the default area). */
typedef struct CamDistSet {
    f32 fov;                    /* 0x00 */
    f32 roll;                   /* 0x04 */
    u8 pad8[0x18];
    CamDist dist[5];            /* 0x20: by CamFollow.distNo (L2/R2), [4] while the player runs at distance 2 */
    u8 padC0[0x100 - 0xC0];
} CamDistSet;
typedef char CamDistSet_size_check[(sizeof(CamDistSet) == 0x100) ? 1 : -1];

/* The follow camera's work (mode 0, CamCtl.follow at +0x3B0; CamFollow_Init and CamFollow_Update). */
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
    u8 unk75;                   /* 0x75 */
    u8 unk76;                   /* 0x76: frames the pad's turn holds (8, 20 while the view is held) */
    u8 turningBehind;           /* 0x77: set while the yaw turns behind the player (CamFollow_ShouldTurnBehind; cleared
                                   within 0x1000 of it; CamCtl_IsTurningBehind) */
    u8 unk78;                   /* 0x78 */
    u8 unk79;                   /* 0x79: set when the eye was raised above the floor */
    s8 canFollow;               /* 0x7A: Cam_PlayerCanFollow (CamFollow_UpdateCanFollow; CamFollow_GetCanFollow) */
    s8 followDelay;             /* 0x7B: frames before it follows again (3) */
    u8 blownAway;               /* 0x7C: offline, the player is blown away (CamFollow_IsBlownAway) */
    u8 pad7D[3];
} CamFollow;
typedef char CamFollow_size_check[(sizeof(CamFollow) == 0x80) ? 1 : -1];

/* The orbit camera's work (mode 1, CamCtl.orbit at +0x430; CamOrbit_Update). */
typedef struct CamOrbit {
    f32 eye[3];                 /* 0x00 */
    f32 at[3];                  /* 0x0C */
    f32 unk18[3];               /* 0x18 */
    s16 pitch;                  /* 0x24 */
    s16 yaw;                    /* 0x26: added to CamFollow.yaw */
    s16 dir;                    /* 0x28: where yaw heads, +-0x800 */
    u8 pad2A[0x40 - 0x2A];
} CamOrbit;
typedef char CamOrbit_size_check[(sizeof(CamOrbit) == 0x40) ? 1 : -1];

/* Camera mode 2's work (CamCtl.rail at +0x470): the eye and target it moves along the
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

/* The aim camera's work (mode 3, CamCtl.aim at +0x4B0; CamAim_Update, CamAim_GetState and
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
    /* 0x65 */ u8 unk65;                /* 1, 0x11 or 0 by the class */
    /* 0x66 */ u8 zoom;                 /* set when the right stick zooms */
} CamAim;
typedef char CamAim_size_check[(sizeof(CamAim) == 0x68) ? 1 : -1];

/* The camera focus, gCamCtl + 0x530 (CamFixed.focus): the view eases to an object (the lobby's
 * NPCs and menus: CamFocus_Start, CamFocus_End), the player or a position (unk_31C7E0.c). */
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

/* Camera mode 4's work (CamCtl.fixed at +0x520): the area's fixed camera (CamFixed_UpdateTarget) and the focus. */
typedef struct CamFixed {
    /* 0x00 */ u8 started;              /* cleared (as a word) while the player's flag 0x80000 is off */
    /* 0x01 */ u8 pad1[3];
    /* 0x04 */ s32 on;                  /* Cam_PlayerHasFlag80000 */
    /* 0x08 */ void *spot;              /* the player's action spot when it started (none: no fixed camera) */
    /* 0x0C */ u8 padC[4];
    /* 0x10 */ CamFocus focus;        /* a view eased to a position or an object (CamFocus_Start; CamFocus_Update) */
} CamFixed;
typedef char CamFixed_size_check[(sizeof(CamFixed) == 0x44) ? 1 : -1];

/* A position on a path (unk_31E190.c), gCamCtl + 0x64C. */
typedef struct PathPos {
    /* 0x0 */ f32 dist;                 /* along segment k */
    /* 0x4 */ f32 t;                    /* dist / the segment's length */
    /* 0x8 */ u8 k;                     /* the segment */
} PathPos;

/* A camera shake request (0x14 bytes; CamCtl.shakeA/B/C): CamShake_Set* and
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
    s16 phase;                  /* 0x0: an angle, 0x10000 per turn (Math_CosAngle) */
    s16 speed;                  /* 0x2: added to phase each frame (0: no swing) */
    s16 amp[2];                 /* 0x4: the amplitude at the move's start and at its end */
} CamSwing;
typedef char CamSwing_size_check[(sizeof(CamSwing) == 0x8) ? 1 : -1];

/* Mode 5's work: the camera script (CamCtl.script, 0x90 bytes; CamScript_Run, CamScript_Step,
 * CamScript_Place). A move eases each pair from [0], its start, to [1], its end, over the
 * script view's left / length (modes[5]); when it ends, CamScript_Step copies [1] into [0]. */
typedef struct CamScript {
    f32 eye[2][3];              /* 0x00: the eye at the move's start and end (command 2; the view's at the start) */
    f32 at[2][3];               /* 0x18: the target's (command 4) */
    s8 *pc;                     /* 0x30: the next command (gDmcamFileIds[id], or the dmcam file loaded for it: Dmcam_Find) */
    s8 *loop;                   /* 0x34: the command after the loop's start (command 36; 37 and 38 go back to it) */
    u8 place;                   /* 0x38: 0 orbits (angle, dist), 1 eases eye and at (command 0; 1 at the start) */
    u8 eyeSpace;                /* 0x39: the eye points' space (command 1; CamScript_EyePoint: 0 the local player's body,
                                   1 its position, 2 its part eyePart, 3 obj's matrix, 4 the world, 5 at[0]) */
    u8 atSpace;                 /* 0x3A: the target points' (command 3; CamScript_AtPoint, no 5) */
    u8 eyePart;                 /* 0x3B: the player's part for eyeSpace 2 (CamScript_Place's orbit turns by
                                   atSpace with it, and by eyeSpace with atPart) */
    u8 atPart;                  /* 0x3C: the part for atSpace 2 */
    u8 pivotEye;                /* 0x3D: the orbit: 1 the target turns around the eye, else the eye around the target (command 5) */
    u8 id;                      /* 0x3E: the script (CamCtl.request; the gDmcamFileIds index) */
    s8 state;                   /* 0x3F: 1 running, -1 held at a command 40, 0 none (CamReq_State) */
    void *obj;                  /* 0x40: the request's object (CamCtl.requestArg: an Em or a Player; space 3 is its worldMtx) */
    f32 dist[2];                /* 0x44: the orbit's distance at the start and end (commands 10, 11: in 1/16) */
    s16 angle[2][2];            /* 0x4C: the orbit's x and y angles at the start and end (commands 6-9; CamScript_OrbitAngles) */
    s16 roll[2];                /* 0x54: commands 12, 13 (0x10000 per turn) */
    s16 fov[2];                 /* 0x58: commands 14, 15 */
    CamSwing swing[4];          /* 0x5C: the view's sideways and upward sways, the roll's swing and the fov's (commands 17-32) */
    u8 swingRand[4];            /* 0x7C: per swing, nonzero adds noise to the roll's and the fov's (command 33) */
    s8 *mark;                   /* 0x80: where command 42 jumps (command 43 sets it; the script's start) */
    f32 easePow;                /* 0x84: the ease's exponent (command 35: its low 14 bits / 1024) */
    u8 ease;                    /* 0x88: 0 linear, 1 in, 2 out, 3 in and out (command 35: its top 2 bits) */
    u8 pad89[0x90 - 0x89];
} CamScript;
typedef char CamScript_size_check[(sizeof(CamScript) == 0x90) ? 1 : -1];

/* A key of a camera area's path (16 bytes): a point and the length of the segment that
 * starts there. */
typedef struct PathKey {
    f32 pos[3];
    f32 len;                    /* 0x0C */
} PathKey;

/* A camera area's path (kind 2, at CamArea +0x20): the keys the eye moves along, a second set
 * the player's position is placed on (PathPos_PlaceNear), and a fov and a roll per key. */
typedef struct PathData {
    PathKey keys[16];           /* 0x000 */
    PathKey keys2[16];          /* 0x100: a second set (PathPos_PlaceNear) */
    f32 vals[16];               /* 0x200: a value per key (PathKey_Blend: the fov) */
    s16 angles[16];             /* 0x240: an angle per key (PathKey_BlendAngle: the roll) */
    u8 count;                   /* 0x260 */
    u8 unk261;                  /* 0x261: passed to PathSeg_Nearest */
} PathData; /* 0x264 */
typedef char PathData_size_check[(sizeof(PathData) == 0x264) ? 1 : -1];

/* A cubic segment of a path (Path_BuildSpline builds them from the keys):
 * ((c[0] t + c[1]) t + c[2]) t + c[3] (Vec_EvalCubic). */
typedef struct PathSeg {
    f32 c[4][3];
} PathSeg;
typedef char PathSeg_size_check[(sizeof(PathSeg) == 0x30) ? 1 : -1];

/* A face of a camera area (0x40 bytes, CamArea.faces): the player is in the area when in front
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

/* A camera area of kind 1 or 3 (at CamArea +0x20; CamRail_Update, CamPoint_GetPos). */
typedef struct CamAreaFixed {
    f32 eye[3];                 /* 0x20: kind 1 the eye in the world, kind 3 its offset from the player */
    f32 at[3];                  /* 0x2C: the look point in the world (lookSpace 0); lookLock keeps its y, or its x and z */
    f32 atLocal[3];             /* 0x38: the look point in the player's frame (lookSpace 1, through its worldMtx) */
    f32 atOffset[3];            /* 0x44: the look point's offset from the player's position (lookSpace 2) */
    f32 fov;                    /* 0x50: CamRail.fov, scaled by the eye's distance (CamArea.fovDist) */
    f32 roll;                   /* 0x54: CamRail.roll */
} CamAreaFixed;

/* A camera area of kind 2 (at CamArea +0x20): the path and the look point
 * (CamRail_Update, CamPoint_GetPosOr: lookSpace 0 is the path's point nearest the player). */
typedef struct CamAreaPath {
    PathData d;                 /* 0x020 */
    f32 unk284[3];              /* 0x284 */
    f32 atLocal[3];             /* 0x290: the look point in the player's frame (lookSpace 1) */
    f32 atOffset[3];            /* 0x29C: its offset from the player's position (lookSpace 2) */
} CamAreaPath;

/* A camera area (0x300 bytes, then faceCount faces, then the 0x20-byte transition table when
 * trans is set: CamData_Relocate). The player's area (CamCtl.area: Cam_FindPlayerArea) picks
 * the camera by its kind. CamCtl_InitDefaultParams writes a default area of kind 0. */
typedef struct CamArea {
    u8 unk00;
    u8 id;                      /* 0x01: CamCtl.areaId */
    u8 kind;                    /* 0x02: 0 the follow camera (areaMode 0) with this area's follow settings; 1 a fixed
                                   eye, 2 the eye along the path, 3 an eye offset from the player (areaMode 2, CamRail_Update) */
    u8 lookLock;                /* 0x03: the look point's axes kept from the area's point (CamPoint_GetPos): 0 y,
                                   1 x and z, 2 none (the default) */
    u8 lookSpace;               /* 0x04: the look point's space: 0 the world, 1 the player's frame, 2 the player's
                                   position plus an offset */
    u8 faceCount;               /* 0x05: the faces of 0x40 bytes after the area's 0x300 bytes (CamData_Relocate) */
    u8 flags;                   /* 0x06: 0x80: the area counts only while the local player's action is in
                                   Player_ActionInCamRange_Unk's range (CamArea_IsEnabled) */
    u8 unk07;
    f32 fovDist[2];             /* 0x08: the eye's distances where the fov scale is fovScale[0] and fovScale[1]
                                   (CamArea_FovScale: linear between them; 400.0 and 2400.0 in the default) */
    f32 fovScale[2];            /* 0x10: 1.0 and 0.75 in the default */
    CamFace *faces;             /* 0x18: an offset from the area in the file, a pointer once relocated */
    u8 *trans;                  /* 0x1C: the same, when set: 16 {area id, frames} pairs, the length of the
                                   transition from that area (CamView_SetTransition) */
    union {
        CamDistSet follow;      /* 0x20: kind 0 (CamFollow.set; gCamFollowDefaults in the default) */
        CamAreaFixed fixed;     /* 0x20: kinds 1 and 3 */
        CamAreaPath path;       /* 0x20: kind 2 */
        u8 pad20[0x2E0];
    };
} CamArea; /* 0x300 */
typedef char CamArea_size_check[(sizeof(CamArea) == 0x300) ? 1 : -1];

/* The area's camera data (gSysSettings.camData, loaded by Area_LoadCamData; CamCtl.data): this
 * header, then areaCount camera areas from +0x30 (each 0x300 bytes, then faceCount faces of 0x40
 * bytes, then 0x20 more bytes when its unk1C is set), the grid's cells and their area lists. The
 * file holds offsets from the header where the pointers are: CamData_Relocate turns them into
 * pointers. CamCtl_InitDefaultParams writes a default header and one area when the area has none. */
typedef struct CamCell {
    s32 count;                  /* 0x00: the camera areas listed for the cell */
    CamArea **areas;            /* 0x04: the list (Cam_FindPlayerArea tests each with CamArea_TestObj) */
} CamCell;

typedef struct CamDataHdr {
    u16 version;                /* 0x00: 0x102 in the default */
    u16 areaCount;              /* 0x02: the camera areas from +0x30 (CamData_Relocate) */
    u16 gridW;                  /* 0x04: the grid's cells per row (CamCtl.gridW) */
    u16 gridH;                  /* 0x06: its rows: gridW * gridH cells */
    u16 unk08;                  /* 0x08: 20000 in the default; CamCtl_LoadParams copies 0x04-0x1B to CamCtl +0x390 */
    u16 unk0A;
    s32 unk0C;
    s32 unk10;
    s32 unk14;                  /* 0x14: 20000 in the default */
    s32 unk18;
    CamCell *cells;             /* 0x1C: the grid (NULL: none) */
    CamArea **areaLists;        /* 0x20: the cells' area lists (pointers to areas once relocated) */
    s32 unk24;
    CamArea *areas;             /* 0x28: the first camera area (the header's end, +0x30) */
    s32 unk2C;
} CamDataHdr; /* 0x30 */

typedef char CamDataHdr_size_check[(sizeof(CamDataHdr) == 0x30) ? 1 : -1];

/* The camera controller (0x680 bytes at 0x5AF420, cleared by CamCtl_Clear). */
typedef struct CamCtl {
    /* 0x000 */ CamView out;            /* the current mode's view (CamCtl_RunMode: modes[areaMode]) */
    /* 0x080 */ CamView modes[6];       /* each mode's view, by kind; modes[5] is the script's (the state machine, unk_323C90.c; CamCtl_Apply) */
    /* 0x380 */ u8 inited;             /* CamCtl_Update sets the modes up while it is 0 (CamCtl_Init clears it as a word) */
    /* 0x381 */ u8 pad381[3];
    /* 0x384 */ struct Player *player;  /* the player it follows (CamCtl_Init: the local one) */
    /* 0x388 */ CamDataHdr *data;       /* the loaded camera data (CamData_Relocate; CamCtl_InitDefaultParams: the default) */
    /* 0x38C */ CamArea *area;         /* the camera area the player is in (Cam_FindPlayerArea, from data):
                                          its kind picks areaMode (0: 0, 1-3: 2, CamRail_Update) */
    /* 0x390 */ u16 gridW;             /* the area grid's row length (Cam_FindPlayerArea: cell cellX + it * cellZ): data->gridW */
    /* 0x392 */ u16 gridH;             /* CamCtl_LoadParams copies 0x390-0x3A7 from the data's header (+0x04-0x1B) */
    /* 0x394 */ u16 unk394;
    /* 0x396 */ u16 unk396;
    /* 0x398 */ s32 unk398;
    /* 0x39C */ s32 unk39C;
    /* 0x3A0 */ s32 unk3A0;
    /* 0x3A4 */ s32 unk3A4;
    /* 0x3A8 */ u16 cellX;             /* the grid cell Cam_FindPlayerArea looks in (cleared by CamCtl_Update) */
    /* 0x3AA */ u16 cellZ;
    /* 0x3AC */ u8 unk3AC;             /* while set, Cam_FindPlayerArea keeps the map's first area (cleared each frame) */
    /* 0x3AD */ u8 areaId;             /* the current area's id (area[1]; 0xFF before the first) */
    /* 0x3AE */ u8 prevAreaId;         /* the one before (0xFF: none) */
    /* 0x3AF */ u8 areaChanged;        /* set for the frame the area changed (mode 2 turns at once) */
    /* 0x3B0 */ CamFollow follow;       /* mode 0's work */
    /* 0x430 */ CamOrbit orbit;         /* mode 1's */
    /* 0x470 */ CamRail rail;           /* mode 2's */
    /* 0x4B0 */ CamAim aim;             /* mode 3's */
    /* 0x518 */ u8 unk518[0x520 - 0x518];
    /* 0x520 */ CamFixed fixed;         /* mode 4's, with the focus */
    /* 0x564 */ u8 unk564[0x580 - 0x564];
    /* 0x580 */ CamScript script;       /* mode 5's: the camera script */
    /* 0x610 */ CamShake shakeA;        /* the shake request slots (unk_31E190.c) */
    /* 0x624 */ CamShake shakeB;
    /* 0x638 */ CamShake shakeC;
    /* 0x64C */ PathPos pathPos;
    /* 0x658 */ u8 pad658[0x65C - 0x658];
    /* 0x65C */ u8 prevAreaMode;       /* areaMode the frame before */
    /* 0x65D */ u8 areaMode;           /* the current mode: 0, or 2 where the area asks for its path (the orbit sets 1) */
    /* 0x65E */ u16 padHeld;           /* the pad as the camera reads it (CamCtl_ReadPad; all 0 while it is not allowed) */
    /* 0x660 */ u16 padPressed;
    /* 0x662 */ u16 padDir;
    /* 0x664 */ u16 padDegR;           /* the right stick's angle and magnitude (mode 3 zooms and turns by them) */
    /* 0x666 */ u16 padMagR;
    /* 0x668 */ u8 *targetObj;          /* CamCtl_SetTargetObj (the local player's) */
    /* 0x66C */ void *requestArg;       /* the request's object (CamReq_Set; CamScript.obj) */
    /* 0x670 */ u8 request;             /* the request to the state machine */
    /* 0x671 */ s8 override;            /* the mode whose view overrides the current one (5, 3 or 4; -1: none; CamCtl_RunMode) */
    /* 0x672 */ u8 followView;          /* set while the follow camera (mode 0) shows the view: CamCtl_CheckEmInView runs, CamCtl_GetPlayerFade gives playerFade */
    /* 0x673 */ u8 emInView;            /* the monster was in view (CamCtl_CheckEmInView; it then tests a 50-degree frustum instead of 45) */
    /* 0x674 */ f32 playerFade;         /* for Player_DrawLocal when followView is set (0.3 while the monster is in view) */
    /* 0x678 */ s32 mode;               /* CamCtl_SetMode */
    /* 0x67C */ u8 pad67C[4];
} CamCtl;
typedef char CamCtl_size_check[(sizeof(CamCtl) == 0x680) ? 1 : -1];

/* A camera mode's init or update handler (gCamModeInit, gCamModeUpdate): the controller and
 * the mode's view. */
typedef void (*CamEntFn)(CamCtl *c, CamView *v);
extern CamEntFn gCamModeInit[8];
extern CamEntFn gCamModeUpdate[6];

extern CamCtl gCamCtl;

#endif /* CAMERA_H */
