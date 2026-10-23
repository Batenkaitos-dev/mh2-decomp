/*
 * The equipment menus' skill lists and two helpers of the item box menu's
 * equipment menu:
 * - SkillList_DrawWithEquip draws a hunter's skills with a piece of
 *   equipment put on, and SkillList_DrawCompare the preview player's
 *   (gPreviewPlayer), each skill coloured against the hunter's own; the
 *   skill window (EquipDetail_Draw) and the equipment set screens draw
 *   them;
 * - BoxMenu_DrawSkillHint, the skill hint on the bottom line;
 * - BoxMenu_ListStyles, the list of the symbol colours the hunter has
 *   earned, for the equipment menu's colour choice (EQUIP_MENU_SYMBOL_COLOR).
 * The file has no data of its own.
 */

#include "common.h"
#include "player.h"
#include "text.h"
#include "box_menu.h"
#include "lobby_labels.h"
#include "box_skill_lists.h"
#include "equip_parts.h"
#include "font.h"
#include "game_util.h"
#include "menu_util.h"
#include "window.h"

/* Compiled at optimization level 4: the two skill lists match only there. */
#pragma optimization_level 4

extern u8 gSkillTryPlayer[];
/* The unit's own declaration: x is an s16 here, as the game's code passes it; the
 * definition's s32 x changes how both skill lists pass it. */
void Player_DrawSkills(Player *pl, s16 x, s16 y, s16 *colors); /* unit's own declaration: x is an s16 here (the definition's is an s32); the skill lists' calls match only so */
u8 Player_GetSkill(Player *p, u8 group);

/* The skills of pl with item put on (a copy of pl in gSkillTryPlayer; a weapon of the other
 * hunter type also takes off the armour that type can't wear), each coloured against pl's as
 * in SkillList_DrawCompare; without item, pl's own skills. */
void SkillList_DrawWithEquip(Player *pl, EquipItem *item, s16 x, s16 y)
{
    u32 i;
    s16 col[5];
    Player *tryPl;
    s8 pts;
    s16 *colPtr;
    s8 curPts;
    u8 curLevel;

    Font_SetSpacing(18, 18);
    Text_SetColor(0);
    col[4] = 0;
    col[3] = 0;
    col[2] = 0;
    col[1] = 0;
    col[0] = 0;
    if (item != NULL) {
        tryPl = (Player *)gSkillTryPlayer;
        *(PlayerCopy *)gSkillTryPlayer = *(PlayerCopy *)pl;
        if (item->type != EQUIP_BLADE && item->type != EQUIP_GUN) {
            tryPl->equip[item->type] = *item;
        } else {
            Player_SetWeapon(tryPl, item);
            if (pl->equip[EQUIP_SLOT_WEAPON].type != item->type) {
                Player_RemoveUnwearable(tryPl);
            }
        }
        Player_CalcSkills(tryPl);
        for (i = 0, colPtr = col; i < 5; i++, colPtr++) {
            if (tryPl->skillGroups[i] == 0) {
                break;
            }
            pts = Player_GetSkillPoints(tryPl, tryPl->skillGroups[i]);
            curLevel = Player_GetSkill(pl, tryPl->skillGroups[i]);
            if (curLevel == 0) {
                if (pts > 0) {
                    *colPtr = 14;
                } else if (pts < 0) {
                    *colPtr = 2;
                }
            } else if (curLevel != tryPl->skills[i]) {
                curPts = Player_GetSkillPoints(pl, tryPl->skillGroups[i]);
                if (curPts < pts) {
                    *colPtr = 4;
                } else if (pts < curPts) {
                    *colPtr = 15;
                }
            }
        }
        pl = tryPl;
    }
    Player_DrawSkills(pl, x, y, col);
}


/* The skill list of the preview player gPreviewPlayer at (x, y), each skill
 * coloured against pl's: 14 gained, 2 lost, 4 lower, 15 higher. */
void SkillList_DrawCompare(Player *pl, s16 x, s16 y)
{
    Player *preview = (Player *)&gPreviewPlayer;
    s16 col[5];
    u32 i;
    s8 pts;
    s16 *colPtr;
    s8 curPts;
    u8 curLevel;

    Font_SetSpacing(18, 18);
    Text_SetColor(0);
    col[4] = 0;
    col[3] = 0;
    col[2] = 0;
    col[1] = 0;
    col[0] = 0;
    if (pl != NULL) {
        for (i = 0, colPtr = col; i < 5; i++, colPtr++) {
            if (preview->skillGroups[i] == 0) {
                break;
            }
            pts = Player_GetSkillPoints(preview, preview->skillGroups[i]);
            curLevel = Player_GetSkill(pl, preview->skillGroups[i]);
            if (curLevel == 0) {
                if (pts > 0) {
                    *colPtr = 14;
                } else if (pts < 0) {
                    *colPtr = 2;
                }
            } else if (curLevel != preview->skills[i]) {
                curPts = Player_GetSkillPoints(pl, preview->skillGroups[i]);
                if (curPts < pts) {
                    *colPtr = 4;
                } else if (pts < curPts) {
                    *colPtr = 15;
                }
            }
        }
    }
    Player_DrawSkills(preview, x, y, col);
}

#pragma optimization_level reset

/* The skill hint on the bottom line (the third of gBoxMenuTexts' lines), drawn on the
 * equipment menu's pieces list while no help window is up. */
void BoxMenu_DrawSkillHint(void)
{
    Font_SetSpacing(17, 18);
    Font_SetPos(315, 398);
    Text_Printf(gBoxMenuTexts[BOX_MENU_TEXT_SKILL_HINT]);
}

/* Lists the symbol colours the hunter has earned into out, as rate flag numbers (each rate flag
 * gives one colour), and returns how many. With a rate (Save_HasRate), the save's rate flags
 * count; the style's colour (styleFlag) counts once a style is earned (styleBits), except online
 * with a rate, where the rate flags alone count. */
u8 BoxMenu_ListStyles(u8 *out)
{
    SaveData *save = &gSave;
    u16 mask = 0;
    u32 i;
    u8 n;

    if (Save_HasRate() == 1) {
        mask = save->rateFlags;
    }
    if ((Game_IsOnline() != 1 || Save_HasRate() != 1) && save->styleBits != 0) {
        mask |= (u16)(1 << save->styleFlag);
    }
    for (i = 0, n = 0; i < 12; i++) { /* the 12 rate flags (RATE_FLAGS_MASK) */
        if (mask & (1 << i)) {
            *out++ = i;
            n++;
        }
    }
    return n;
}
