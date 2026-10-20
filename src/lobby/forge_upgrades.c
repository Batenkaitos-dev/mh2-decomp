/*
 * The forge's lookups: the recipe list the forge menus filter their make
 * lists by, the forge condition of a weapon an upgrade leads to, and the
 * armour upgrades.
 *
 * Each armour piece's ArmorData.upgrade picks a table of gArmorUpgradeRows:
 * a list of materials (ArmorUpgradeRow), each with a count and a condition
 * per level. Level 0 is the cost of making the piece from the one it derives
 * from; level n + 1 is the cost of the upgrade to it. Once a piece reaches
 * ArmorData.deriveLevel it can be made into the pieces in ArmorData.derives.
 * The armour forge's upgrade page (ArmorForge_AddUpgrade) asks these
 * functions for all three, and both forges draw their materials window with
 * Forge_DrawMaterials.
 *
 * The file's data is the recipe list and the armour upgrade tables.
 */
#include "common.h"
#include "lobby_menu.h"
#pragma optimization_level 4

extern RecipeId gForgeRecipeList[];
extern ArmorUpgradeRow *gArmorUpgradeRows[];
#include "save.h"
void *memset(void *, s32, u32);
s32 Save_IsItemObtained(SaveData *save, u16 item);

/*
 * Returns 1 when the recipe (an equipment type and id) is in gForgeRecipeList,
 * else 0. The forge menus' make lists show either the recipes outside the
 * list or those in it, by the menu's filter.
 */
s32 Recipe_IsInList(s32 type, s32 id)
{
    RecipeId *entry = gForgeRecipeList;
    s32 entryId = gForgeRecipeList[0].id;

    while (entryId != 0) {
        if (type == entry->type && id == entryId) {
            return 1;
        }
        entry++;
        entryId = entry->id;
    }
    return 0;
}

#include "item.h"
#include "lobby_labels.h"
s32 Forge_CheckCondition(u8 cond); /* unit's own declaration: the condition is passed as an unsigned byte here; with the definition's s16, the call extends it differently */

/*
 * Returns 1 when the forge condition (FORGE_COND_*) of the weapon of that
 * type (EQUIP_BLADE or EQUIP_GUN) and id is met, else 0. The weapon forge
 * asks it for each weapon an upgrade leads to.
 */
s32 WeaponForge_IsUnlocked(s32 type, s32 id)
{
    s32 cond;
    BladeData *blade;
    GunData *gun;

    switch (type) {
    case EQUIP_BLADE:
        blade = &gBladeData[id];
        cond = blade->forgeCond;
        break;
    case EQUIP_GUN:
        gun = &gGunData[id];
        cond = gun->forgeCond;
        break;
    }
    return Forge_CheckCondition(cond);
}

/* The ArmorData of an armour piece, by its type; NULL for a type that isn't
 * armour. */
static ArmorData *ArmorPiece_GetData(EquipItem *piece)
{
    switch (piece->type) {
    case EQUIP_HEAD:
        return &gArmorHeadData[piece->id];
    case EQUIP_BODY:
        return &gArmorBodyData[piece->id];
    case EQUIP_ARMS:
        return &gArmorArmsData[piece->id];
    case EQUIP_WAIST:
        return &gArmorWaistData[piece->id];
    case EQUIP_LEGS:
        return &gArmorLegsData[piece->id];
    default:
        return NULL;
    }
}

s32 ArmorUpgrade_GetMaterials(EquipItem *piece, ItemStack *out, s8 *cond);

/*
 * Returns 1 when the piece can't be upgraded yet: it isn't armour, it is at
 * its last level, or a material of its next level has a condition and hasn't
 * been obtained yet. Returns 0 when the upgrade can be shown.
 */
s32 ArmorUpgrade_IsLocked(EquipItem *piece)
{
    ItemStack need[7];
    s8 cond[4];
    ArmorData *armor;
    s32 count;
    s32 i;

    armor = ArmorPiece_GetData(piece);
    if (armor == NULL) {
        return 1;
    }
    if (armor->maxLevel <= piece->level + 1) {
        return 1;
    }
    count = ArmorUpgrade_GetMaterials(piece, need, cond);
    for (i = 0; i < count; i++) {
        if (cond[i] != 0 && Save_IsItemObtained(&gSave, need[i].item) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Fills out (ARMOR_UPGRADE_MATERIALS stacks, cleared first) and cond with the
 * materials of the piece's next level: each row of its upgrade table with a
 * count at that level. Returns how many.
 */
s32 ArmorUpgrade_GetMaterials(EquipItem *piece, ItemStack *out, s8 *cond)
{
    ArmorUpgradeRow *row;
    u16 level;
    u16 item;
    s32 count;
    ArmorData *armor;

    armor = ArmorPiece_GetData(piece);
    if (armor == NULL) {
        return 0;
    }
    memset(out, 0, ARMOR_UPGRADE_MATERIALS * sizeof(ItemStack));
    /* item holds the next level first: with this order the compiler gives
     * the values the game's registers */
    item = piece->level + 1;
    row = gArmorUpgradeRows[armor->upgrade];
    level = item;
    count = 0;
    item = row->item;
    while (item != 0) {
        if (row->lv[level].count != 0) {
            out->item = item;
            count++;
            out->count = row->lv[level].count;
            out++;
            *cond = row->lv[level].cond;
            cond++;
        }
        row++;
        item = row->item;
    }
    return count;
}

/*
 * As ArmorUpgrade_GetMaterials for level 0: the materials that make the piece
 * from the one it derives from. Returns how many.
 */
s32 ArmorUpgrade_GetMakeMaterials(EquipItem *piece, ItemStack *out, s8 *cond)
{
    ArmorData *armor;
    ArmorUpgradeRow *row;
    u16 item;
    s32 count;

    armor = ArmorPiece_GetData(piece);
    if (armor == NULL) {
        return 0;
    }
    memset(out, 0, ARMOR_UPGRADE_MATERIALS * sizeof(ItemStack));
    row = gArmorUpgradeRows[armor->upgrade];
    item = row->item;
    count = 0;
    while (item != 0) {
        if (row->lv[0].count != 0) {
            out->item = item;
            count++;
            out->count = row->lv[0].count;
            out++;
            *cond = row->lv[0].cond;
            cond++;
        }
        row++;
        item = row->item;
    }
    return count;
}

/*
 * Once the piece's next level reaches ArmorData.deriveLevel, writes the ids
 * of the pieces it can be made into (ArmorData.derives, the same type; 0
 * none) to out, when out isn't NULL, and returns how many there are. Below
 * that level, returns 0.
 */
s32 ArmorUpgrade_GetDerived(EquipItem *piece, u16 *out)
{
    ArmorData *armor;
    s32 count;

    armor = ArmorPiece_GetData(piece);
    if (armor == NULL) {
        return 0;
    }
    count = 0;
    if ((u16)(piece->level + 1) >= armor->deriveLevel) {
        if (armor->derives[0] != 0) {
            count++;
        }
        if (out != NULL) {
            *out++ = armor->derives[0];
        }
        if (armor->derives[1] != 0) {
            count++;
        }
        if (out != NULL) {
            *out++ = armor->derives[1];
        }
        if (armor->derives[2] != 0) {
            count++;
        }
        if (out != NULL) {
            *out = armor->derives[2];
        }
    }
    return count;
}

s32 Items_DrawNeedLine(s16, s16, u16, s32, u32);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: the coordinates are passed as s16 and the kind as s32 here; with the definition's s32 coordinates and s16 kind, the call converts them differently */
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);

/*
 * Draws the forges' materials window at x, y: the materials header, then a
 * line for each of the count stacks of need that holds an item
 * (Items_DrawNeedLine: the item, the counts in the pouch and the box, and the
 * count needed), or a line saying there are none.
 */
void Forge_DrawMaterials(s16 x, s16 y, ItemStack *need, s8 count)
{
    s16 i;
    s16 shown;

    Win_Draw(x, y, 0x132, 0x72, 0xBDB59F6C, 0xA0202020, 6);
    y += 6;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0xE, y);
    Text_PrintfColored(gForgeTexts[FORGE_TEXT_MATERIALS_HEAD]);
    shown = 0;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < count; i++, need++) {
        if (need->item != 0) {
            y += 0x14;
            Items_DrawNeedLine(x + 0x22, y, need->item, need->count, 0x807F7F7F);
            shown++;
        }
    }
    if (shown == 0) {
        Font_SetPos(x + 0x28, y + 0x28);
        Text_PrintfColored(gForgeTexts[FORGE_TEXT_NO_MATERIALS]);
    }
}

/* The recipe list and the armour upgrade tables. */
RecipeId gForgeRecipeList[62] = {
#include "lobby/007DD2B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_0[17] = {
#include "lobby/007DD4A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_3[17] = {
#include "lobby/007DD5B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_6[17] = {
#include "lobby/007DD6C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_9[17] = {
#include "lobby/007DD7D0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_12[17] = {
#include "lobby/007DD8E0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_15[17] = {
#include "lobby/007DD9F0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_18[17] = {
#include "lobby/007DDB00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_21[17] = {
#include "lobby/007DDC10.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_24[17] = {
#include "lobby/007DDD20.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_27[17] = {
#include "lobby/007DDE30.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_30[17] = {
#include "lobby/007DDF40.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_33[17] = {
#include "lobby/007DE050.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_36[17] = {
#include "lobby/007DE160.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_39[17] = {
#include "lobby/007DE270.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_1[17] = {
#include "lobby/007DE380.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_4[17] = {
#include "lobby/007DE490.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_7[17] = {
#include "lobby/007DE5A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_10[17] = {
#include "lobby/007DE6B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_13[17] = {
#include "lobby/007DE7C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_16[17] = {
#include "lobby/007DE8D0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_19[17] = {
#include "lobby/007DE9E0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_22[17] = {
#include "lobby/007DEAF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_25[17] = {
#include "lobby/007DEC00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_28[17] = {
#include "lobby/007DED10.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_31[17] = {
#include "lobby/007DEE20.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_34[17] = {
#include "lobby/007DEF30.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_37[17] = {
#include "lobby/007DF040.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_2[17] = {
#include "lobby/007DF150.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_5[17] = {
#include "lobby/007DF260.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_8[17] = {
#include "lobby/007DF370.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_11[17] = {
#include "lobby/007DF480.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_14[17] = {
#include "lobby/007DF590.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_17[17] = {
#include "lobby/007DF6A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_20[17] = {
#include "lobby/007DF7B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_23[17] = {
#include "lobby/007DF8C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_26[17] = {
#include "lobby/007DF9D0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_29[17] = {
#include "lobby/007DFAE0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_32[17] = {
#include "lobby/007DFBF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_35[17] = {
#include "lobby/007DFD00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_38[17] = {
#include "lobby/007DFE10.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_40[17] = {
#include "lobby/007DFF20.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_43[17] = {
#include "lobby/007E0030.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_46[35] = {
#include "lobby/007E0140.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_50[17] = {
#include "lobby/007E0370.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_53[17] = {
#include "lobby/007E0480.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_56[17] = {
#include "lobby/007E0590.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_59[17] = {
#include "lobby/007E06A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_62[17] = {
#include "lobby/007E07B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_65[17] = {
#include "lobby/007E08C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_68[17] = {
#include "lobby/007E09D0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_71[17] = {
#include "lobby/007E0AE0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_74[17] = {
#include "lobby/007E0BF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_77[17] = {
#include "lobby/007E0D00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_80[17] = {
#include "lobby/007E0E10.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_82[17] = {
#include "lobby/007E0F20.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_41[17] = {
#include "lobby/007E1030.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_44[17] = {
#include "lobby/007E1140.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_47[17] = {
#include "lobby/007E1250.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_49[17] = {
#include "lobby/007E1360.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_51[17] = {
#include "lobby/007E1470.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_54[17] = {
#include "lobby/007E1580.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_57[17] = {
#include "lobby/007E1690.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_60[17] = {
#include "lobby/007E17A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_63[17] = {
#include "lobby/007E18B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_66[17] = {
#include "lobby/007E19C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_69[17] = {
#include "lobby/007E1AD0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_72[17] = {
#include "lobby/007E1BE0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_75[17] = {
#include "lobby/007E1CF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_78[35] = {
#include "lobby/007E1E00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_83[17] = {
#include "lobby/007E2030.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_85[17] = {
#include "lobby/007E2140.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_42[17] = {
#include "lobby/007E2250.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_45[17] = {
#include "lobby/007E2360.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_48[35] = {
#include "lobby/007E2470.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_52[17] = {
#include "lobby/007E26A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_55[17] = {
#include "lobby/007E27B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_58[17] = {
#include "lobby/007E28C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_61[17] = {
#include "lobby/007E29D0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_64[17] = {
#include "lobby/007E2AE0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_67[17] = {
#include "lobby/007E2BF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_70[17] = {
#include "lobby/007E2D00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_73[17] = {
#include "lobby/007E2E10.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_76[17] = {
#include "lobby/007E2F20.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_79[17] = {
#include "lobby/007E3030.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_81[17] = {
#include "lobby/007E3140.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_84[17] = {
#include "lobby/007E3250.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_86[17] = {
#include "lobby/007E3360.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_89[35] = {
#include "lobby/007E3470.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_93[17] = {
#include "lobby/007E36A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_94[17] = {
#include "lobby/007E37B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_95[17] = {
#include "lobby/007E38C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_96[17] = {
#include "lobby/007E39D0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_97[17] = {
#include "lobby/007E3AE0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_98[17] = {
#include "lobby/007E3BF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_99[17] = {
#include "lobby/007E3D00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_87[17] = {
#include "lobby/007E3E10.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_90[17] = {
#include "lobby/007E3F20.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_92[17] = {
#include "lobby/007E4030.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_88[17] = {
#include "lobby/007E4140.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_91[17] = {
#include "lobby/007E4250.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_100[17] = {
#include "lobby/007E4360.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_103[17] = {
#include "lobby/007E4470.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_106[17] = {
#include "lobby/007E4580.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_109[17] = {
#include "lobby/007E4690.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_110[17] = {
#include "lobby/007E47A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_111[17] = {
#include "lobby/007E48B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_112[17] = {
#include "lobby/007E49C0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_113[17] = {
#include "lobby/007E4AD0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_114[17] = {
#include "lobby/007E4BE0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_115[17] = {
#include "lobby/007E4CF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_116[17] = {
#include "lobby/007E4E00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_117[17] = {
#include "lobby/007E4F10.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_118[17] = {
#include "lobby/007E5020.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_119[17] = {
#include "lobby/007E5130.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_133[17] = {
#include "lobby/007E5240.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_134[17] = {
#include "lobby/007E5350.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_135[17] = {
#include "lobby/007E5460.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_101[17] = {
#include "lobby/007E5570.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_104[17] = {
#include "lobby/007E5680.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_107[17] = {
#include "lobby/007E5790.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_102[17] = {
#include "lobby/007E58A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_105[17] = {
#include "lobby/007E59B0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_108[17] = {
#include "lobby/007E5AC0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_120[17] = {
#include "lobby/007E5BD0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_121[17] = {
#include "lobby/007E5CE0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_122[17] = {
#include "lobby/007E5DF0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_123[17] = {
#include "lobby/007E5F00.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_124[17] = {
#include "lobby/007E6010.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_125[17] = {
#include "lobby/007E6120.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_126[17] = {
#include "lobby/007E6230.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_127[17] = {
#include "lobby/007E6340.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_128[17] = {
#include "lobby/007E6450.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_129[17] = {
#include "lobby/007E6560.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_130[17] = {
#include "lobby/007E6670.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_131[17] = {
#include "lobby/007E6780.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_132[17] = {
#include "lobby/007E6890.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_139[17] = {
#include "lobby/007E69A0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_136[17] = {
#include "lobby/007E6AB0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_137[17] = {
#include "lobby/007E6BC0.inc"
};
ArmorUpgradeRow gArmorUpgradeRows_138[17] = {
#include "lobby/007E6CD0.inc"
};
ArmorUpgradeRow *gArmorUpgradeRows[140] = {
    gArmorUpgradeRows_0, gArmorUpgradeRows_1, gArmorUpgradeRows_2, gArmorUpgradeRows_3,
    gArmorUpgradeRows_4, gArmorUpgradeRows_5, gArmorUpgradeRows_6, gArmorUpgradeRows_7,
    gArmorUpgradeRows_8, gArmorUpgradeRows_9, gArmorUpgradeRows_10, gArmorUpgradeRows_11,
    gArmorUpgradeRows_12, gArmorUpgradeRows_13, gArmorUpgradeRows_14, gArmorUpgradeRows_15,
    gArmorUpgradeRows_16, gArmorUpgradeRows_17, gArmorUpgradeRows_18, gArmorUpgradeRows_19,
    gArmorUpgradeRows_20, gArmorUpgradeRows_21, gArmorUpgradeRows_22, gArmorUpgradeRows_23,
    gArmorUpgradeRows_24, gArmorUpgradeRows_25, gArmorUpgradeRows_26, gArmorUpgradeRows_27,
    gArmorUpgradeRows_28, gArmorUpgradeRows_29, gArmorUpgradeRows_30, gArmorUpgradeRows_31,
    gArmorUpgradeRows_32, gArmorUpgradeRows_33, gArmorUpgradeRows_34, gArmorUpgradeRows_35,
    gArmorUpgradeRows_36, gArmorUpgradeRows_37, gArmorUpgradeRows_38, gArmorUpgradeRows_39,
    gArmorUpgradeRows_40, gArmorUpgradeRows_41, gArmorUpgradeRows_42, gArmorUpgradeRows_43,
    gArmorUpgradeRows_44, gArmorUpgradeRows_45, gArmorUpgradeRows_46, gArmorUpgradeRows_47,
    gArmorUpgradeRows_48, gArmorUpgradeRows_49, gArmorUpgradeRows_50, gArmorUpgradeRows_51,
    gArmorUpgradeRows_52, gArmorUpgradeRows_53, gArmorUpgradeRows_54, gArmorUpgradeRows_55,
    gArmorUpgradeRows_56, gArmorUpgradeRows_57, gArmorUpgradeRows_58, gArmorUpgradeRows_59,
    gArmorUpgradeRows_60, gArmorUpgradeRows_61, gArmorUpgradeRows_62, gArmorUpgradeRows_63,
    gArmorUpgradeRows_64, gArmorUpgradeRows_65, gArmorUpgradeRows_66, gArmorUpgradeRows_67,
    gArmorUpgradeRows_68, gArmorUpgradeRows_69, gArmorUpgradeRows_70, gArmorUpgradeRows_71,
    gArmorUpgradeRows_72, gArmorUpgradeRows_73, gArmorUpgradeRows_74, gArmorUpgradeRows_75,
    gArmorUpgradeRows_76, gArmorUpgradeRows_77, gArmorUpgradeRows_78, gArmorUpgradeRows_79,
    gArmorUpgradeRows_80, gArmorUpgradeRows_81, gArmorUpgradeRows_82, gArmorUpgradeRows_83,
    gArmorUpgradeRows_84, gArmorUpgradeRows_85, gArmorUpgradeRows_86, gArmorUpgradeRows_87,
    gArmorUpgradeRows_88, gArmorUpgradeRows_89, gArmorUpgradeRows_90, gArmorUpgradeRows_91,
    gArmorUpgradeRows_92, gArmorUpgradeRows_93, gArmorUpgradeRows_94, gArmorUpgradeRows_95,
    gArmorUpgradeRows_96, gArmorUpgradeRows_97, gArmorUpgradeRows_98, gArmorUpgradeRows_99,
    gArmorUpgradeRows_100, gArmorUpgradeRows_101, gArmorUpgradeRows_102, gArmorUpgradeRows_103,
    gArmorUpgradeRows_104, gArmorUpgradeRows_105, gArmorUpgradeRows_106, gArmorUpgradeRows_107,
    gArmorUpgradeRows_108, gArmorUpgradeRows_109, gArmorUpgradeRows_110, gArmorUpgradeRows_111,
    gArmorUpgradeRows_112, gArmorUpgradeRows_113, gArmorUpgradeRows_114, gArmorUpgradeRows_115,
    gArmorUpgradeRows_116, gArmorUpgradeRows_117, gArmorUpgradeRows_118, gArmorUpgradeRows_119,
    gArmorUpgradeRows_120, gArmorUpgradeRows_121, gArmorUpgradeRows_122, gArmorUpgradeRows_123,
    gArmorUpgradeRows_124, gArmorUpgradeRows_125, gArmorUpgradeRows_126, gArmorUpgradeRows_127,
    gArmorUpgradeRows_128, gArmorUpgradeRows_129, gArmorUpgradeRows_130, gArmorUpgradeRows_131,
    gArmorUpgradeRows_132, gArmorUpgradeRows_133, gArmorUpgradeRows_134, gArmorUpgradeRows_135,
    gArmorUpgradeRows_136, gArmorUpgradeRows_137, gArmorUpgradeRows_138, gArmorUpgradeRows_139,
};
