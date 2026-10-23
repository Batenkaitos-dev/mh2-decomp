#ifndef EQUIP_PARTS_H
#define EQUIP_PARTS_H

#include "common.h"
#include "item.h"
#include "player.h"

/*
 * The parts the equipment windows are drawn from: whether an armour piece
 * fits the player (Equip_DataWearable, Player_RemoveUnwearable), the
 * weapon's element, the sharpness gauge, the skill points compared with
 * another piece's (EquipCompare_Draw), the compare arrow, the page tabs and
 * the button hints.
 */

/* A point or a texture coordinate. */
typedef struct Pt {
    s16 x;
    s16 y;
} Pt;

/* A weapon icon's texture corners (gWeaponIconUVs, Hud_WeaponIconUV). */
typedef struct WeaponIconUV {
    Pt uv0;
    Pt uv1;
} WeaponIconUV;

/* A skill and its points (Equip_ListSkillPoints fills 9 of them). */
typedef struct SkillPt {
    u8 id;
    s8 pts;
} SkillPt;

s32 Equip_DataWearable(Player *pl, EquipItem *piece);
void Player_RemoveUnwearable(Player *pl);
void EquipMenu_DrawElement(EquipItem *weapon, s16 x, s16 y, EquipItem *cmp);
void Equip_ListSkillPoints(EquipItem *equip, SkillPt *out);
s32 SkillPts_Compare(SkillPt *skillPt, SkillPt *list);
void StatusPage_DrawArrow(s32 x, s16 y);
void EquipCompare_Draw(EquipItem *shown, EquipItem *other, s32 x, s16 y, u8 flags);
void StatusPage_DrawBoxes(s32 x, s16 y, s32 width);
void EquipMenu_DrawSharpness(Player *pl, EquipItem *weapon, s16 x, s16 y);
void Hud_DrawTab(s16 x, s16 y, u8 flags, u8 color, u32 rgba);
void ButtonHint_DrawDetails(void);
void ButtonHint_DrawSort(u8 index);
void Equip_GetIconUV(EquipItem *equip, Pt *topLeft, Pt *bottomRight);
void Hud_WeaponIconUV(u8 *equip, Pt *topLeft, Pt *bottomRight);

#endif
