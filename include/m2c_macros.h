#ifndef M2C_MACROS_H
#define M2C_MACROS_H

/* The helpers m2c's --valid-syntax output uses. They
 * stand for what m2c could not express: a field of a struct it doesn't
 * know (M2C_FIELD), a type it couldn't infer (M2C_UNK). Replace them with
 * real types and fields when a function is worked on. */
#define M2C_FIELD(expr, type, offset) (*(type)((s8 *)(expr) + (offset)))
#define M2C_ERROR(desc) (0)
#define M2C_TRAP_IF(cond) ((void)0)
#define M2C_BREAK() ((void)0)
#define M2C_SYNC() ((void)0)
#define M2C_CARRY 0
#define M2C_UNK s32
#define M2C_UNK8 s8
#define M2C_UNK16 s16
#define M2C_UNK32 s32
#define M2C_UNK64 s64
#define M2C_BITWISE(type, expr) ((type)(expr))
#define M2C_LWL(expr) (expr)
#define M2C_LWR(expr) (expr)
#define M2C_STRUCT_COPY(dst, src, size) (*(dst) = *(src))
#define M2C_MEMCPY_ALIGNED(dst, src, size) (*(dst) = *(src))
#define M2C_MEMCPY_UNALIGNED(dst, src, size) (*(dst) = *(src))

#endif /* M2C_MACROS_H */
