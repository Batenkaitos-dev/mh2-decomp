#ifndef TEX_FILES_H
#define TEX_FILES_H

#include "common.h"

/*
 * The textures loaded by id (Tex_LoadById, Tex_FindById,
 * Tex_ReleaseIdSlots), the common texture (Tex_LoadCommon), and the tables
 * of texture files.
 */

void Tex_ClearIdSlots(void);
void Tex_ReleaseIdSlots(void);
s32 Tex_LoadById(s32 id, s32 buf, s32 kind);
s32 Tex_IsIdSlot(s32 handle);
void Tex_LoadCommon(void);
extern s32 gSystemTexFiles[6];
extern s32 gPlselTexPack;
extern s32 D_0042F1DC;
extern s32 gNetSetupTexPack;

#endif
