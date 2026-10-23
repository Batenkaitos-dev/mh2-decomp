#ifndef LIBGCC_H
#define LIBGCC_H

#include "common.h"

/*
 * GCC's run-time routines that the game's code calls: the conversions
 * between float and double.
 */

f64 fptodp(f32 x);
f32 dptofp(f64 x); /* double to float (main) */

#endif
