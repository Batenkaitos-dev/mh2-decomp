#ifndef ANIM_CURVE_H
#define ANIM_CURVE_H

#include "common.h"
#include "motion.h"

/*
 * The animation curves' evaluation. Anim_EvalCurve gives a curve's value at
 * a time through the evaluator of its key kind (Anim_Eval*: linear, cubic or
 * mixed keys, as floats or shorts); Anim_FindKeys* find the two keys around
 * the time, and Anim_LerpKeys and Anim_CubicKeys step between them.
 * Anim_SetCurveKeys sets the animation data the curves' key offsets count
 * from.
 */

void Anim_SetCurveKeys(u8 *keys);
f32 Anim_EvalCurve(f32 time, AnimCurve *curve, s16 *hint);
f32 Anim_EvalLinearF(AnimCurve *curve, s16 *hint, f32 time);
f32 Anim_EvalCubicF(AnimCurve *curve, s16 *hint, f32 time);
f32 Anim_EvalMixedF(AnimCurve *curve, s16 *hint, f32 time);
f32 Anim_EvalLinearS(AnimCurve *curve, s16 *hint, f32 time);
f32 Anim_EvalCubicS(AnimCurve *curve, s16 *hint, f32 time);
f32 Anim_EvalMixedS(AnimCurve *curve, s16 *hint, f32 time);
s16 Anim_FindKeysF2(AnimCurve *curve, KeyF2 **prevKeyOut, KeyF2 **nextKeyOut, s16 hint, f32 time);
s16 Anim_FindKeysF4(AnimCurve *curve, KeyF4 **prevKeyOut, KeyF4 **nextKeyOut, s16 hint, f32 time);
s16 Anim_FindKeysFM(AnimCurve *curve, KeyFM **prevKeyOut, KeyFM **nextKeyOut, s16 hint, f32 time);
s16 Anim_FindKeysS2(AnimCurve *curve, KeyS2 **prevKeyOut, KeyS2 **nextKeyOut, s16 hint, f32 time);
s16 Anim_FindKeysS4(AnimCurve *curve, KeyS4 **prevKeyOut, KeyS4 **nextKeyOut, s16 hint, f32 time);
s16 Anim_FindKeysSM(AnimCurve *curve, KeySM **prevKeyOut, KeySM **nextKeyOut, s16 hint, f32 time);
f32 Anim_LerpKeys(f32 time, f32 prevValue, f32 prevTime, f32 nextValue, f32 nextTime);
f32 Anim_CubicKeys(f32 time, f32 prevValue, f32 prevTime, f32 prevOutSlope, f32 nextValue, f32 nextTime, f32 nextInSlope);

#endif
