#ifndef VIBRATION_H
#define VIBRATION_H

#include "common.h"

/*
 * The pads' vibration: Vib_Start starts a pattern on a pad port and
 * Vib_StartLocal on the local player's; Vib_Stop and Vib_StopAll stop it.
 * Nothing starts while the vibration option is off.
 */

void Vib_Start(s32 port, s32 pattern);
void Vib_StartLocal(void *player, s32 pattern);
void Vib_Stop(s32 port);
void Vib_StopAll(void);

#endif
