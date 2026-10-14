/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void *memcpy(void *dst, const void *src, u32 n);
extern M2C_UNK D_6ADAF0;
extern M2C_UNK D_6ADB10;

void TownMsg_On28(s32 data) {
    memcpy(&D_6ADAF0, (void *) data, 0x1C);
    memcpy(&D_6ADB10, (void *) (data + 0x1C), 4);
}
