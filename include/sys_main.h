#ifndef SYS_MAIN_H
#define SYS_MAIN_H

#include "common.h"

/*
 * The start of sub_main: its entry and main loop (sub_main_entry,
 * SubMain_Step, SubMain_BeginFrame, SubMain_EndFrame), the system settings'
 * reset, the GS colour table, the pads' frame state (Pad_UpdatePorts,
 * PadState_Update*), the memory layout (Mem_InitLayout), the version string
 * and the first loading screen.
 */

void sub_main_entry(void);
s32 SubMain_Step(void);
void SubMain_Init(void);
void SubMain_BeginFrame(void);
void SubMain_EndFrame(void);
void SysSettings_Reset(void);
void SubMain_ResetSystems(void);
void Gs_SetTableColor(s32 i);
void Pad_UpdatePorts(void);
void PadState_Update(s32 port);
void PadState_UpdateAll(void);
void SubMain_EmptyFrameHook(void);
void Mem_InitLayout(void);
void Version_Build(void);
void SubMain_OpenLoading(void);

#endif
