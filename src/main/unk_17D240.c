/*
 * Tex_Use (main 0x17D240): the texture handles' entry point for making
 * them resident in GS memory (texconv.c's Tex_MakeResident).
 */
#include "common.h"

s32 Tex_MakeResident(s32 n, u32 *handles);

/* Makes n texture handles resident in GS memory; returns what
 * Tex_MakeResident returns. */
s32 Tex_Use(s32 n, u32 *handles)
{
    return Tex_MakeResident(n, handles);
}
