#ifndef LIBM_H
#define LIBM_H

#include "common.h"

/*
 * The C library's maths functions that the game's code calls: the
 * trigonometry, powers, roots and rounding of floats and doubles (newlib's
 * libm).
 */

f64 tan(f64 x); /* the C library's tangent (main) */
f32 atanf(f32 x);
f32 ceilf(f32 x);
f32 cosf(f32 x);
f32 floorf(f32 x);
f32 sinf(f32 x);
double atan2(double y, double x);
double sqrt(double x);
f32 acosf(f32 x);
f32 atan2f(f32 y, f32 x);
f32 expf(f32 x);
f32 powf(f32 x, f32 y);

#endif
