/*
 * Sys_EmptyHook: an empty function. main calls it once,
 * after loading the first overlay and before entering sub_main; the
 * lobby's NetMode_RunSetup and select's select_psp.c call it too.
 */
#include "common.h"

void Sys_EmptyHook(void)
{
}
