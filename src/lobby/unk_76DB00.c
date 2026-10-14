/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Text_Printf(const char *fmt, ...);
extern char *D_0080CA98;

void BoxMenu_DrawSkillHint(void) {
    Font_SetSpacing(0x11, 0x12);
    Font_SetPos(0x13B, 0x18E);
    Text_Printf(D_0080CA98);
}
