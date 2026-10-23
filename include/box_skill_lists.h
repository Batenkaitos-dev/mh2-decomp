#ifndef BOX_SKILL_LISTS_H
#define BOX_SKILL_LISTS_H

#include "common.h"
#include "player.h"

/*
 * The equipment menus' skill lists: SkillList_DrawWithEquip draws a
 * hunter's skills with a piece of equipment put on, and
 * SkillList_DrawCompare the preview player's, each skill coloured against
 * the hunter's own.
 */

void SkillList_DrawWithEquip(Player *pl, EquipItem *item, s16 x, s16 y);
void SkillList_DrawCompare(Player *pl, s16 x, s16 y);
void BoxMenu_DrawSkillHint(void);
u8 BoxMenu_ListStyles(u8 *out);

#endif
