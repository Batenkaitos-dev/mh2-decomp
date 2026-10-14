/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

s32 Tex_MakeResident(s32 n, u32 *handles);

s32 Tex_Use(s32 n, u32 *handles) {
    return Tex_MakeResident(n, handles);
}
