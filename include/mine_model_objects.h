#ifndef MINE_MODEL_OBJECTS_H
#define MINE_MODEL_OBJECTS_H

#include "common.h"
#include "sprite.h"
#include "menu_task.h"
#include "lobby_vobj.h"
#include "stageobj.h"

/*
 * The village mine's pickaxe list and the lobby's model objects. For the
 * mine (TOWN_MODE_MINE): the list's draws, and Mine_Dig, which takes the
 * pickaxe given and rolls the digs it lasts and the find. The model objects
 * (LOBBY_OBJ_MODEL) are short effects of up to 8 parts, spawned on a
 * townsperson, the player or a point (ModelObj_SpawnOn, ModelObj_SpawnAt,
 * ModelObj_SpawnOnScaled).
 */

void Mine_Dig(MineMenu *menu);
void Mine_DrawPickaxes(MineMenu *menu, s16 x, s16 y);
void Mine_DrawMenu(MenuTask *task);
void ModelObj_SpawnOn(void *owner, s32 type);
void ModelObj_SpawnAt(f32 *pos, s32 type, s32 colorIndex, f32 scale);
void ModelObj_SpawnOnScaled(u8 *owner, s32 type, f32 scale);
void ModelObj_Update(StageObj *obj);
void ModelObj_Init(StageObj *obj);
void ModelObj_Run(StageObj *obj);
void ModelObj_End(StageObj *obj);
void ModelObj_Free(StageObj *obj);
void ModelObj_DrawPart(DrawSlot *slot);
extern ModelObjTables gModelObjTables;

#endif
