#ifndef FILE_LOAD_H
#define FILE_LOAD_H

#include "common.h"

/*
 * Loading the game's data files by id. LoadFile and LoadCompressedFile read
 * a file of the data archive; the loaders built on them fill the load buffer
 * with the fixed texture slots' files and the quest end's pictures
 * (Tex_Load*), the animation sets (Load_*Anim) and the model packs
 * (Load_AreaPack, Load_*ModelPack). The tables of file ids they read by
 * kind, area and gender are declared with them.
 */

void Tex_LoadSystemPack(s32 mode);
void Tex_LoadSystemPackB(void);
void Tex_LoadQuestEndFile(void);
void Tex_SetQuestEndImages(void);
void Tex_LoadFile(s32 file, s32 slot, s32 kind);
void Tex_LoadStageFile(s32 map, s32 unusedQuestId, s32 slot, s32 kind);
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 LoadFile(u8 *dst, s32 id);
void Load_PlayerAnim(s32 unusedIdx, s32 weaponType);
void Load_CommonAnim(s32 set);
void Load_EmAnim(s32 unusedIdx, s32 kind);
void Load_EmSlotAnim(s32 slot, s32 set);
void Load_AreaPack(s32 area);
void Load_CommonModelPack(s32 index);
void Load_WeaponModelPack(s32 weapon);
void Load_PartModelPack(s32 id, s32 type, s32 female);
void Load_EquipModelPack(s32 id, s32 female);
extern s32 *gPartModelFilesMale[8];
extern s32 *gPartModelFilesFemale[8];
extern s32 gWeaponModelPackFiles[340];
extern s32 gEmPartModelPackFiles[76];
extern s32 gAreaSubModelFiles[200];
extern s32 *gEmSlotAnimFiles[20];

#endif
