#ifndef ARENA_CUP_H
#define ARENA_CUP_H

#include "common.h"
#include "lobby_menu.h"
#include "arena_em.h"
#include "menu_task.h"

/*
 * The arena cup's desk (TOWN_MODE_ARENA_CUP): the online desk where the
 * cup's quest is entered with a monster kept from the arena, the kept
 * monsters are checked, renamed or released, and the cup's record is
 * shown. ArenaCup_Open, ArenaCup_Update and ArenaCup_Close run it and
 * ArenaCup_Draw and ArenaCup_Draw2 draw it; Arena_DrawMonsterInfo is
 * shared with the arena menu.
 */

s32 ArenaCup_QuestTaken(void);
void ArenaCup_Open(u8 *npc);
void ArenaCup_Close(void);
void ArenaCup_Update(void);
s32 ArenaCup_Post(u16 pad);
s32 ArenaCup_ChooseQuest(u16 pad);
s32 ArenaCup_MonsterInput(u16 pad);
s32 ArenaCup_RenameMonster(u16 pad, s32 name);
void ArenaCup_ReleaseMonster(void);
s32 ArenaCup_IsFormRowOpen(s8 row);
s32 ArenaCup_PostInput(u16 pad);
void ArenaCup_DrawForm(s16 x, s16 y);
void ArenaCup_DrawRecord(s16 x, s16 y);
void ArenaCup_DrawMonsterCheck(s16 x, s16 y);
void ArenaCup_DrawMonsterMenu(s16 x, s16 y);
void Arena_DrawMonsterInfo(s16 x, s16 y, ArenaEm *em);
void ArenaCup_DrawHeldQuest(ArenaCupMenu *cup, s16 x, s16 y);
void ArenaCup_Draw(MenuTask *task);
void ArenaCup_Draw2(MenuTask *task);

#endif
