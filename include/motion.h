#ifndef MOTION_H
#define MOTION_H

#include "common.h"

/*
 * A motion layer (0x50 bytes): four from +0x154 of a player (Player.motion)
 * and of a monster (Em.motion). Player_SetLayerMotion and Em_SetMotion start
 * a motion on a layer, blending into it over blendFrames frames when the
 * layer was playing; Player_UpdateMotion steps the frames and the blend, and
 * Motion_PassedFrame and Motion_ReachedFrame test a layer's frame.
 */
typedef struct MotionLayer {
    s32 playing;                /* 0x00: nonzero until the motion ends (state steps wait for 0) */
    u32 unk04;                  /* 0x04 */
    f32 frame;                  /* 0x08: the current frame (frame / 32.0f as a phase; the weapons' hit checks) */
    f32 step;                   /* 0x0C: frames advanced per update (2.0 * the motion speed) */
    f32 loopFrame;              /* 0x10: where a looping motion goes on from its end (Anim_GetLoopTime) */
    f32 frameCount;             /* 0x14: the motion length in frames (Motion_GetLength) */
    s32 loop;                   /* 0x18: nonzero for a looping motion; Em_SetMotion keeps no blend when it is set and the layer is not playing */
    f32 blendFrame;             /* 0x1C: the frame of the motion it blends into (it becomes frame when the blend ends) */
    f32 blendStep;              /* 0x20: blendT's step per frame, 1 / (the blend frames + 1) */
    f32 blendLoopFrame;         /* 0x24: loopFrame of the motion it blends into */
    f32 blendFrameCount;        /* 0x28: frameCount of the motion it blends into */
    s32 blendLoop;              /* 0x2C: loop of the motion it blends into */
    s32 blend;                  /* 0x30: nonzero while the layer blends into the next motion (the states wait for 0); negative: no root motion; layer 0's nonzero holds every frame test (Motion_PassedFrame) */
    s32 blendFrames;            /* 0x34: the blend's frames left */
    f32 blendT;                 /* 0x38: the blend progress, 0..1 through Math_Hermite */
    s32 *blendMotion;           /* 0x3C: the gMotions entry of the motion it blends into */
    f32 move[3];                /* 0x40: the root's motion (layer 0) */
    f32 prevFrame;              /* 0x4C: the frame before the step */
} MotionLayer;
typedef char MotionLayer_size_check[(sizeof(MotionLayer) == 0x50) ? 1 : -1];

/*
 * Animation data: the motion file readers (AnimFile_*, Anim_ConvertKeys*)
 * build it, the Anim_* functions evaluate it, and the Bone_* functions pose the bones
 * with it. A bone's animation is an AnimHeader; its `count` curves are
 * AnimCurves, each driving one value of the bone's pose (EmBone.pose[idx]:
 * 0..2 the scale, 3..5 the rotation, 6..8 the translation) through `count`
 * keys of one format. The offsets are from a base the user keeps:
 * gAnimData for the curves, gAnimCurveKeys, gAnimKeyBase or gAnimCurveBuf
 * for the keys.
 */
#define ANIM_TYPE_MASK 0xF000           /* AnimHeader.flags: the motion file's type, ANIM_TYPE_* */
#define ANIM_TYPE_FLOAT 0x1000          /* type 1: the curves give the pose values as they are */
#define ANIM_TYPE_FIXED 0x2000          /* type 2: fixed point, angles in 0x4000ths of a turn, the rest in 16ths (Anim_EvalPose) */

typedef struct AnimHeader {
    u16 flags;                  /* 0x0: ANIM_TYPE_MASK; 0x8000 */
    s16 count;                  /* 0x2: curves */
    s32 curves;                 /* 0x4: offset of the curves (AnimCurve) */
} AnimHeader;

typedef struct AnimCurve {
    u8 type;                    /* 0x0: the key format, ANIM_KEY_* (Anim_EvalCurve) */
    u8 idx;                     /* 0x1: the pose value it drives */
    u16 count;                  /* 0x2: keys */
    s32 keys;                   /* 0x4: offset of the keys */
} AnimCurve;

/* AnimCurve.type: the key format (Anim_EvalCurve's switch, Anim_ConvertKeys'). */
#define ANIM_KEY_S16_NONE 0x10          /* read as ANIM_KEY_NONE (AnimCurve_StartTime, AnimCurve_EndTime); nothing seen sets it */
#define ANIM_KEY_S16_LINEAR 0x11        /* KeyS2 */
#define ANIM_KEY_S16_CUBIC 0x12         /* KeyS4 */
#define ANIM_KEY_S16_MIXED 0x13         /* KeySM: linear or cubic per key */
#define ANIM_KEY_NONE 0x20              /* no keys (Anim_ConvertChunk's first value) */
#define ANIM_KEY_F32_LINEAR 0x21        /* KeyF2 */
#define ANIM_KEY_F32_CUBIC 0x22         /* KeyF4 */
#define ANIM_KEY_F32_MIXED 0x23         /* KeyFM: linear or cubic per key */

/* KeyFM.mode and KeySM.mode: how the curve goes from this key to the next. */
#define ANIM_KEY_MODE_LINEAR 0x10000    /* Anim_LerpKeys */
#define ANIM_KEY_MODE_CUBIC 0x20000     /* Anim_CubicKeys, with this key's out and the next key's in */

/* The keys: a value v at time t, with the slopes in and out for a cubic key.
 * The 16-bit formats hold signed values. */
typedef struct KeyF2 { f32 v, t; } KeyF2;
typedef struct KeyF4 { f32 v, t, in, out; } KeyF4;
typedef struct KeyFM { u32 mode; f32 v, t, in, out; } KeyFM;
typedef struct KeyS2 { s16 v, t; } KeyS2;
typedef struct KeyS4 { s16 v, t, in, out; } KeyS4;
typedef struct KeySM { u32 mode; s16 v, t, in, out; } KeySM;

/*
 * A motion file, as Anim_LoadSet reads it: a header (AnimFileHeader), then `count`
 * chunks (AnimFileChunk), one per animation, each followed by its `count` curve
 * records (AnimFileCurve), each followed by its keys in the record's format.
 */
#define ANIM_FILE_FLOAT 1               /* AnimFileHeader.type: float keys (ANIM_TYPE_FLOAT) */
#define ANIM_FILE_FIXED 2               /* fixed-point keys (ANIM_TYPE_FIXED) */

typedef struct AnimFileHeader {
    u8 type;                    /* 0x00: ANIM_FILE_FLOAT or ANIM_FILE_FIXED */
    u8 pad01[3];
    s32 count;                  /* 0x04: the chunks */
    s32 unk08;
    s32 loops;                  /* 0x0C: nonzero when the file gives a loop time */
    f32 loopTime;               /* 0x10 */
} AnimFileHeader;

typedef struct AnimFileChunk {
    s32 unk00;
    s32 count;                  /* 0x04: the curve records that follow */
    s32 size;                   /* 0x08: the whole chunk's bytes: the next chunk follows */
} AnimFileChunk;

typedef struct AnimFileCurve {
    u32 info;                   /* 0x00: ANIM_FILE_CURVE_USED, the key format and the pose value's bit */
    s32 count;                  /* 0x04: the keys that follow */
    s32 size;                   /* 0x08: the whole record's bytes: the next record follows */
} AnimFileCurve;

#define ANIM_FILE_CURVE_USED 0x80000000 /* AnimFileCurve.info: the curve is converted */
#define ANIM_FILE_KEY_FORMAT(info) (((info) & 0xFF0000) >> 16) /* the key format, ANIM_KEY_* */
#define ANIM_FILE_POSE_BITS 0x1FF       /* one bit, 1 << n, for the pose value n (AnimCurve.idx) it drives */

/*
 * An animation set (0x20 bytes): the animations of one file, converted by
 * Anim_LoadSet into a buffer that starts with the set
 * itself. The Anim_Setup* loaders copy that block into a
 * memory handle of its own (AnimSet_CopyToHandle): gMotions[] holds them.
 */
typedef struct AnimSet {
    u16 flags;                  /* 0x00: ANIM_SET_LOOPS */
    s16 count;                  /* 0x02: animations */
    f32 start;                  /* 0x04: the earliest key */
    f32 end;                    /* 0x08: the latest key */
    f32 loop;                   /* 0x0C: the loop time */
    s32 anims;                  /* 0x10: AnimHeader pointers, as an offset from gAnimCurveBuf */
    s32 base;                   /* 0x14: the buffer (gAnimCurveBuf), whose first object is the set */
    s32 size;                   /* 0x18: bytes used in the buffer */
    u8 pad1C[4];
} AnimSet;
typedef char AnimSet_size_check[(sizeof(AnimSet) == 0x20) ? 1 : -1];

#define ANIM_SET_LOOPS 0x8000   /* AnimSet.flags: the file gives a loop time */

/*
 * Motion ids and the table of loaded motions (gMotions, filled by the Anim_Setup*
 * loaders). A motion id is set * MOTION_ID_SET + group *
 * MOTION_GROUP_SIZE + entry: set 0 is the common set, any other the object's own
 * (Player_SetPartAnim). gMotions holds the common set's motions, then
 * MOTIONS_PER_PLAYER for each player slot from MOTIONS_PLAYER_FIRST, then
 * MOTIONS_PER_EM for each monster slot from MOTIONS_EM_FIRST.
 */
#define MOTION_ID_SET 1000
#define MOTION_GROUP_SIZE 100
#define MOTIONS_COMMON_GROUPS 10
#define MOTIONS_PLAYER_GROUPS 6
#define MOTIONS_PLAYER_FIRST 710
#define MOTIONS_PER_PLAYER 300
#define MOTIONS_EM_FIRST 1910
#define MOTIONS_PER_EM 600
#define MOTIONS_EM_SLOTS 4
#define MOTIONS_TOTAL (MOTIONS_EM_FIRST + MOTIONS_EM_SLOTS * MOTIONS_PER_EM) /* gMotions' entries */

/* Keyframe tracks (Key1_Eval, Key2_Eval, Key3_Eval): a time in frames
 * and a value of one to three floats per key, ended by a time of -1; from
 * the last key on, its value is used as it is. */
typedef struct Key1 {
    f32 t;                      /* the key's time */
    f32 v;                      /* its value */
} Key1;

typedef struct Key2 {
    f32 t;
    f32 v[2];
} Key2;

typedef struct Key3 {
    f32 t;
    f32 v[3];
} Key3;

#endif /* MOTION_H */
