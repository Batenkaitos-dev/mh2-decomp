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

#ifndef NULL
#define NULL 0
#endif

/*
 * INCLUDE_ASM(dir, name): the function `name` is still assembly, in
 * dir/name.s. The compiler never sees this macro. tools/mwcc_cc.py replaces
 * each use with a marker, compiles the C, and splices the assembled function
 * into the object in its original position (with its jump tables and
 * strings, when the file owns its .rodata). For other tools (IDEs, m2c
 * context) it expands to nothing.
 *
 * INCLUDE_RODATA(dir, name): the same for a data object of the file's
 * .rodata that is still assembly (one that several functions share, or
 * that splat doesn't migrate into its function's .s): it goes where the
 * line is. CONTRIBUTING.md, "Functions with switch tables or strings".
 *
 * INCLUDE_DATA(dir, name): a .data object that must never enter the
 * repository (compressed data, a library's data), among a file's .data
 * definitions. The build writes dir/name.s (dir is build/blobs/MODULE,
 * git-ignored) with tools/data_blob.py, which takes the bytes from the
 * user's original, and mwcc_cc.py splices it in where the line is. The file
 * declares the object with an extern for its code.
 * CONTRIBUTING.md, "Data in C".
 */
#define INCLUDE_ASM(dir, name)
#define INCLUDE_RODATA(dir, name)
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
