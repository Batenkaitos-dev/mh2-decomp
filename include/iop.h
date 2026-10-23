#ifndef IOP_H
#define IOP_H

#include "common.h"

/*
 * The IOP's start-up (Iop_Init, which Sys_Init calls): reboots the IOP and
 * loads the game's IOP modules.
 */

void Iop_Init(void);

#endif
