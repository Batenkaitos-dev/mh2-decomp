#ifndef MAIN_H
#define MAIN_H

#include "common.h"

/*
 * The boot executable's main and the services every overlay uses: reading a
 * file of the disc's AFS archives by id (Afs_ReadId), loading an overlay
 * into its window and relocating it (LoadOverlay, Overlay_LoadGmSub,
 * Overlay_IsLoaded), and the set-up of CRI's file systems
 * (Adx_InitFileSystems).
 */

void Sys_EmptyHook(void);
void Adx_InitFileSystems(void);
s32 Afs_ReadId(u32 id, void *dst);
s32 Overlay_Relocate(void *data, s32 *bss);
s32 Overlay_ReadFile(s32 flid, s32 unused, s32 buf);
void Overlay_LoadGmSub(void);
void LoadOverlay(s32 id);
s32 Overlay_IsLoaded(s32 id);

#endif
