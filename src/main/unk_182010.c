/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

extern s32 gPacketMode;
extern s32 gSysNextFlipAt;

void Packet_SetMode0(void) {
    gPacketMode = 0;
}

void Packet_SetMode2(void) {
    gPacketMode = 2;
}

void Sys_SetNextFlipAt(s32 t) {
    gSysNextFlipAt = t;
}
