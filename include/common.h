#ifndef COMMON_H
#define COMMON_H

/* Basic types. The EE is little-endian; int and pointers are 32-bit, long
 * long is 64-bit, and GPRs are 128-bit (u128 is not a C type for MWCC). */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

/* A point or a direction in the world: x, y and z, which the code copies
 * as one struct. */
typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;

#ifndef NULL
#define NULL 0
#endif

/* A full turn in the game's 16-bit angles (the monsters' and the player's
 * rotations, the cameras', Math_CosAngle): ANGLE_TURN / 4 is a quarter turn. */
#define ANGLE_TURN 0x10000

/*
 * INCLUDE_DATA(dir, name): a .data object that must never enter the
 * repository (compressed data, a library's data), among a file's .data
 * definitions. The compiler never sees this macro: the build writes
 * dir/name.s (dir is build/blobs/MODULE, git-ignored) with tools/data_blob.py,
 * which takes the bytes from the user's original, and tools/mwcc_cc.py
 * replaces the line with a marker, compiles the C, and splices the assembled
 * data into the object where the line is. For other tools it expands to
 * nothing. The file declares the object with an extern for its code.
 * Every .rodata object is C. CONTRIBUTING.md, "Data in C".
 */
#define INCLUDE_DATA(dir, name)

/*
 * TXT(0xADDR): the game's text at ADDR in the module's .rodata, a string
 * literal. The repository holds no text of the game: tools/content_gen.py
 * writes the literal from the user's copy into build/gen/MODULE/FILE.h,
 * which the file includes. The compiler still sees a literal, so it pools
 * and places it as the game's did. Data initializers come from the user's
 * copy the same way (#include "MODULE/ADDR.inc"). CONTRIBUTING.md, "Content".
 */
#define TXT(addr) TXT_##addr

#endif /* COMMON_H */
