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

#endif /* MOTION_H */
