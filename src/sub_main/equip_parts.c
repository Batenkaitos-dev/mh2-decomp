/*
 * The equipment windows' parts, for the equipment menus (whose windows have their own code):
 * whether an armour fits the player, the weapon's element, the skill points, the sharpness
 * gauge, the compare arrow, the page tabs, the button hints and the equipment icons.
 *
 * Text colours given to Text_SetColor here: 4 marks a value higher than
 * the compared equipment's, 2 a lower one.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "player.h"
#include "pad.h"
#include "prim.h"
#include "syssave.h"
#include "save.h"
#include "hud.h"
#include "shake.h"
#include "sound.h"
#include "texture.h"
#include "render_state.h"
#include "area_draw.h"
#include "kbd.h"
#include "model.h"
#include "text.h"
#include "window.h"
#include "equip_parts.h"
#include "engine_math.h"
#include "font.h"
#include "menu_util.h"
#include "libc.h"
/* The whole file is compiled at optimization level 4: its functions match only so. */
#pragma optimization_level 4


/* A weapon icon's texture rectangle in gWeaponIconUVs: two corners. */


extern u8 gUnwearableCheckSlots[];
extern char *gDetailsHintText;
extern u8 gButtonHintDetailIcons[];
extern WeaponIconUV gWeaponIconUVs[];
extern char *gSkillPointsFmt;
extern Pt gEquipSkillPos[];
extern char *gElementLineFmt;
extern char *gDefenseBonusFmt;
extern u16 gFrameTick;
extern Pt gButtonHintSortPos[];

extern u8 gButtonHintSortIcons[][8];

u8 Player_GetSkill(Player *p, u8 group);
u8 Equip_GetClass(EquipItem *e);
/* unit's own declaration: declared here as EquipCompare_Draw calls it; with the definition's void (Player *p, EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other, u8 c), the call compiles differently. */
void EquipWindow_Draw(Player *p, EquipItem *e, s32 b, s16 y, u8 flags, EquipItem *other, s32 d);

/* Whether armour piece fits pl: its data's flags must hold the hunter's
 * gender (1 male, 2 female) and the weapon's kind (4 blade, 8 gun). */
s32 Equip_DataWearable(Player *pl, EquipItem *piece)
{
    ArmorData *data = &Equip_GetData(piece)->armor;
    u8 mask;

    if (gSave.gender == 0) {
        mask = 1;
    } else {
        mask = 2;
    }
    if (pl->equip[6].type == 6) {
        mask |= 4;
    } else {
        mask |= 8;
    }
    if ((mask & data->flags) == mask) {
        return 1;
    }
    return 0;
}

/* Takes off the armour pieces (the five slots of gUnwearableCheckSlots)
 * that don't fit pl (Equip_DataWearable). */
void Player_RemoveUnwearable(Player *pl)
{
    u32 i;

    for (i = 0; i < 5; i++) {
        u8 idx = gUnwearableCheckSlots[i];
        if (pl->equip[idx].id != 0 && (u8)Equip_DataWearable(pl, &pl->equip[idx]) == 0) {
            pl->equip[idx].id = 0;
        }
    }
}

/* A blade's element or status: *out gets its kind (elements 0..4, the
 * statuses from 5; -1 none) and the value is returned (-1 none). */
static s32 BladeData_ElementKind(BladeData *data, s16 *out)
{
    if (data->elementType != 0) {
        *out = data->elementType - 1;
        return data->elementValue;
    }
    if (data->statusType != 0) {
        *out = data->statusType + 4;
        return data->statusValue;
    }
    *out = -1;
    return -1;
}

/* A gun's element, as BladeData_ElementKind. */
static s32 GunData_ElementKind(GunData *data, s16 *out)
{
    if (data->elementType != 0) {
        *out = data->elementType - 1;
        return data->elementValue;
    }
    *out = -1;
    return -1;
}

/* The weapon's element value (*value), its kind (*kind, as
 * BladeData_ElementKind; a gun has one only in class 10) and its defense
 * bonus (*defenseBonus). Type 6 is a blade, 7 a gun. */
void Weapon_GetElementInfo(EquipItem *weapon, s16 *value, s16 *kind, s16 *defenseBonus)
{
    EquipData *data;

    if (weapon->type == 6) {
        data = Equip_GetData(weapon);
        *value = BladeData_ElementKind(&data->blade, kind);
        *defenseBonus = data->blade.defense;
    } else {
        data = Equip_GetData(weapon);
        *defenseBonus = data->gun.defense;
        *value = -1;
        *kind = -1;
        if ((u8)Equip_GetClass(weapon) == 10) {
            *value = GunData_ElementKind(&data->gun, kind);
        }
    }
}

/* Draws the weapon's element line and defense bonus at (x, y), each value
 * coloured against weapon cmp's when one is given. */
void EquipMenu_DrawElement(EquipItem *weapon, s16 x, s16 y, EquipItem *cmp)
{
    char buf[48];
    s16 kind, value, defense;
    s16 cmpKind, cmpValue, cmpDefense;
    s16 kindColor, defColor, defLabelColor, valueColor;

    Font_SetSpacing(18, 18);
    Weapon_GetElementInfo(weapon, &value, &kind, &defense);
    kindColor = valueColor = defLabelColor = defColor = 0;
    if (cmp != NULL) {
        Weapon_GetElementInfo(cmp, &cmpValue, &cmpKind, &cmpDefense);
        if (kind >= 0) {
            if (kind == cmpKind) {
                if (value > cmpValue) {
                    valueColor = 4;
                } else if (value < cmpValue) {
                    valueColor = 2;
                }
            } else {
                kindColor = valueColor = 4;
            }
        }
        if (defense > 0) {
            if (cmpDefense > 0) {
                if (cmpDefense < defense) {
                    defColor = 4;
                } else if (defense < cmpDefense) {
                    defColor = 2;
                }
            } else {
                defLabelColor = defColor = 4;
            }
        }
    }
    if (kind >= 0) {
        s16 msg = (kind < 5) ? 12 : 13;

        sprintf(buf, gEquipNameFmt[msg], valueColor, value);
        Font_SetPos(x, y);
        Text_Printf(gElementLineFmt, kindColor, gElementNames[kind], buf);
        y += 20;
    }
    if (defense != 0) {
        Font_SetPos(x, y);
        Text_Printf(gDefenseBonusFmt, defLabelColor, defColor, defense);
    }
}

/* Draws the equipment's skill points (Equip_ListSkillPoints) at x, y: up to 9 rows at
 * gEquipSkillPos, each coloured 4 or 2 when cmp's points are higher or lower
 * (SkillPts_Compare). With flags 8 everything is drawn in colour 10 and nothing compared.
 * Skills 41, 42 and 43 show their name without points.
 * The locals' order (skillPt, i, col, top first) and the loops' forms (the compare loop
 * walks the colours with colorOut set in its for, the draw loop steps skillPt in its for)
 * are what the saved registers and the loop order of the original need. */
void EquipMenu_DrawSkills(EquipItem *equip, s16 x, s32 y, EquipItem *cmp, u8 flags)
{
    SkillPt pts[16];
    SkillPt cmpList[16];
    u8 cols[9];
    SkillPt *skillPt;
    u32 i;
    u8 col;
    s32 top;
    u8 titleColor;
    SkillPt *cmpPts;
    u8 *colorOut;

    if (!(flags & 8)) {
        col = 0;
        titleColor = 5;
    } else {
        col = 10;
        titleColor = col;
        cmp = NULL;
    }
    Text_SetColor(titleColor);
    top = (s16)y;
    Font_SetPos(x, top);
    Text_Draw(gSkillPointsTitle[0]);
    skillPt = pts;
    cols[0] = cols[1] = cols[2] = cols[3] = cols[4] = cols[5] = cols[6] = cols[7] = cols[8] = col;
    Equip_ListSkillPoints(equip, skillPt);
    if (cmp != NULL) {
        if (equip != cmp) {
            cmpPts = cmpList;
            Equip_ListSkillPoints(cmp, cmpPts);
            for (i = 0, colorOut = cols; i < 9; i++) {
                s16 order = SkillPts_Compare(skillPt, cmpPts);

                if (order > 0) {
                    *colorOut = 4;
                } else if (order < 0) {
                    *colorOut = 2;
                }
                colorOut++;
                skillPt++;
            }
        }
        skillPt = pts;
    }
    for (i = 0; i < 9; i++, skillPt++) {
        Font_SetPos(x + gEquipSkillPos[i].x, top + gEquipSkillPos[i].y);
        if (skillPt->id == 0) {
            if (i == 0) {
                Text_SetColor(col);
                Text_Draw(gNoSkillText[0]);
            }
            break;
        }
        Text_SetColor(col);
        if ((u32)(skillPt->id - 41) <= 1 || skillPt->id == 43) {
            Text_Draw(gSkillNames[skillPt->id]);
        } else {
            Text_Printf(gSkillPointsFmt, gSkillNames[skillPt->id], cols[i], skillPt->pts);
        }
    }
}

/* Lists equip's skill points (skills 1 to 81 with nonzero points) into
 * out: at most 9 entries, the rest cleared (id 0). NULL lists none. */
void Equip_ListSkillPoints(EquipItem *equip, SkillPt *out)
{
    s32 n = 0;
    s32 skill;

    if (equip != NULL) {
        for (skill = 1; skill < 82; skill++) {
            s8 pts = Equip_GetSkillPoints(equip, skill);
            if (pts != 0) {
                out->id = skill;
                out->pts = pts;
                if (n > 7) {
                    return;
                }
                n++;
                out++;
            }
        }
    }
    for (; n < 9; n++) {
        out->id = 0;
        out->pts = 0;
        out++;
    }
}

/* Compares skillPt's points with the same skill's in list (9 entries, ended
 * by id 0; a skill missing counts as 0 points): 1 higher, -1 lower, 0 equal. */
s32 SkillPts_Compare(SkillPt *skillPt, SkillPt *list)
{
    s32 n = 9;

    do {
        if (list->id == 0) {
            break;
        }
        if (skillPt->id == list->id) {
            if (skillPt->pts > list->pts) {
                return 1;
            } else if (skillPt->pts < list->pts) {
                return -1;
            }
            return 0;
        }
        list++;
    } while (--n != 0);
    if (skillPt->pts > 0) {
        return 1;
    } else if (skillPt->pts < 0) {
        return -1;
    }
    return 0;
}

/* Draws the compare windows' arrow between them, pulsing in colour and
 * moving between two heights every 16 frames. */
void StatusPage_DrawArrow(s32 x, s16 y)
{
    f32 pulse;
    s8 red, green, blue;
    u8 blueByte;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    pulse = Math_Sin(0.0000958738f * (f32)(u32)(u16)((gFrameTick & 0x1F) << 11));
    red = 96.0f * pulse;
    green = 20.0f * pulse;
    blue = 7.0f * pulse;
    blueByte = blue + 247;
    y += (gFrameTick & 0x10) == 0 ? 125 : 140;
    Menu_DrawArrow((s16)x + 137, y, 32, 16,
                  0xFF000000 | ((u8)(red + 144) << 16) | ((u8)(green + 228) << 8) | blueByte, 3);
}

/* Draws two equipment windows to compare, shown above other (162 lower),
 * each compared with the other, and the arrow; in the town the local player's
 * skills are worked out again first. */
void EquipCompare_Draw(EquipItem *shown, EquipItem *other, s32 x, s16 y, u8 flags)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    if (gScene.inLobby != 0) {
        Player_CalcSkills(pl);
    }
    EquipWindow_Draw(pl, shown, x, y, (flags | 0x20) & ~8, other, 178);
    EquipWindow_Draw(pl, other, x, y + 162, flags, shown, 178);
    StatusPage_DrawArrow(x, y);
}

/* Draws the two empty compare windows and the arrow. */
void StatusPage_DrawBoxes(s32 x, s16 y, s32 width)
{
    StatusPage_DrawBox(x, y, width);
    StatusPage_DrawBox(x, y + 162, width);
    StatusPage_DrawArrow(x, y);
}

/* A sharpness level after pl's sharpness skill (skill group 32): one up
 * at value 33, SKILL_SHARPNESS_PLUS50 (at most 5), else one down (at least 0). */
s32 Player_ApplySharpnessSkill(Player *pl, s32 level)
{
    u8 skill = Player_GetSkill(pl, 32);

    if (skill != 0) {
        if (skill == 33) {
            level++;
            if (level > 5) {
                level = 5;
            }
        } else {
            level--;
            if (level < 0) {
                level = 0;
            }
        }
    }
    return level;
}

extern u32 gSharpnessColors[];

/* Draws the weapon's sharpness gauge at (x, y) for player pl: the frame, the six colour
 * segments (gSharpnessTables, 0.35 pixel a unit), a black cover past the level after the
 * sharpness skill (Player_ApplySharpnessSkill) and, when the skill changes it, a +1 or -1. */
void EquipMenu_DrawSharpness(Player *pl, EquipItem *weapon, s16 x, s16 y)
{
    BladeData *data;
    s16 *bar;
    u32 *col;
    u32 color;
    s32 level;
    s32 delta;
    PrimTri tri;
    PrimLine rect;
    s32 i;
    s32 base;
    f32 left;
    s16 *rectRight;
    s16 top;

    data = &Equip_GetData(weapon)->blade;
    base = data->sharpMax;
    level = Player_ApplySharpnessSkill(Player_PreviewWith(pl, weapon), base);
    if (level == base) {
        color = 0xFF968A63;
    } else {
        if (base < level) {
            color = 0xFF57FFFF;
            Text_SetColor(4);
            delta = 1;
        } else {
            color = 0xFFFF435D;
            Text_SetColor(2);
            delta = -1;
        }
        Font_SetSpacing(18, 18);
        Font_SetPos(163.0f + (s16)(x - 36), (s16)(y - 4) + 2);
        Text_PrintfColored("%+d", delta);
    }
    x -= 36;
    tri.color = color;
    tri.x0 = x;
    tri.x1 = tri.x2 = x + 5;
    top = y - 4;
    tri.y1 = top;
    tri.y0 = top + 11;
    tri.y2 = top + 21;
    Prim_FillTri(&tri);
    left = 146.0f + x;
    tri.x0 = 10.0f + left;
    tri.x1 = tri.x2 = 5.0f + left;
    Prim_FillTri(&tri);
    left = 5.0f + x;
    rect.x0 = left;
    rect.x1 = 146.0f + left;
    rect.y0 = top;
    rect.y1 = rect.y0 + 21;
    rect.color = color;
    Prim_FillRect(&rect);
    left = 6.0f + x;
    rect.x0 = left;
    rect.x1 = 144.0f + left;
    rect.y0 = top + 2;
    rect.y1 = rect.y0 + 17;
    rect.color = 0xFF000000;
    Prim_FillRect(&rect);
    bar = (s16 *)gSharpnessTables[gWeaponKindClass[data->kind]];
    bar += data->sharpType * 6;
    rect.y0 = top + 4;
    rect.y1 = top + 17;
    left = 8.0f + x;
    rect.x1 = left;
    rectRight = &rect.x1;
    col = gSharpnessColors;
    for (i = 0; i < 6; i++, bar++) {
        rect.x0 = *rectRight;
        rect.x1 = left + 0.35f * *bar;
        rect.color = col[i];
        Prim_FillRect(&rect);
    }
    if (level < 5) {
        rect.x0 = left + 0.35f * (150.0f + level * 50);
        rect.x1 = 140.0f + left;
        rect.color = 0xFF000000;
        Prim_FillRect(&rect);
    }
}

extern s16 gHudTabLayout[];
extern s16 gHudTabLayoutBig[6];
/* Draws a two-part tab at (x, y + 100) from the offsets at gHudTabLayout (gHudTabLayoutBig and two
 * more sprites when flags has flag 0x40), then the format gTabPageFmt (gPageFmt) with flags & 0xF
 * and color, in rgba. */
void Hud_DrawTab(s16 x, s16 y, u8 flags, u8 color, u32 rgba)
{
    Sprite spr;
    s16 *pos;
    char *text;
    u8 num = flags & 0xF;
    s32 big = flags & 0x40;

    y += 100;
    pos = gHudTabLayout;
    if (big) {
        pos = gHudTabLayoutBig;
    }
    spr.w = pos[3];
    spr.y = y;
    spr.h = 18;
    spr.color = 0xFF20FF30;
    spr.x = x + pos[1];
    *(s32 *)&spr.u0 = 0x1A00A6;
    *(s32 *)&spr.u1 = 0x2E0094;
    Sprite_Draw(&spr);
    spr.x = x + pos[2];
    spr.u0 = 148;
    spr.u1 = 166;
    Sprite_Draw(&spr);
    text = gTabPageFmt[0];
    if (big) {
        spr.y = y - 1;
        spr.w = 24;
        spr.h = 20;
        spr.color = -1;
        spr.x = x + pos[4];
        *(s32 *)&spr.u0 = 97;
        *(s32 *)&spr.u1 = 0x180078;
        Sprite_Draw(&spr);
        spr.x = x + pos[5];
        spr.u0 = 145;
        spr.u1 = 168;
        Sprite_Draw(&spr);
        text = gPageFmt;
    }
    Font_SetSpacing(18, 18);
    Font_SetColor(rgba);
    Font_SetPos(x + pos[0], y);
    Text_PrintfColored(text, num, color);
}

/* Draws the button hint for the details page, with its button icon. */
void ButtonHint_DrawDetails(void)
{
    Font_SetSpacing(18, 18);
    Text_PrintfAt(558, 398, 0, gDetailsHintText);
    Draw_Icons((Icon *) gButtonHintDetailIcons, 1);
}

/* Draws the sort hint of menu index (gButtonHintSortPos): the select
 * button's label, the sort text and the button icon. */
void ButtonHint_DrawSort(u8 index)
{
    Text_SetColor(0);
    Font_SetSpacing(16, 14);
    Font_SetPos(gButtonHintSortPos[index].x - 79, gButtonHintSortPos[index].y + 2);
    Text_Draw(gSelectText[0]);
    Font_SetSpacing(18, 18);
    Font_SetPos(gButtonHintSortPos[index].x, gButtonHintSortPos[index].y);
    Text_Draw(gSortHintText[0]);
    Draw_Icons((Icon *) gButtonHintSortIcons[index], 1);
}

/* The texture rectangle of equip's icon on the 32-pixel icon sheet: a
 * weapon's by its class (cells from 63), an armour's by its type (cells
 * from 74, in row 9). */
void Equip_GetIconUV(EquipItem *equip, Pt *topLeft, Pt *bottomRight)
{
    if (equip->type == 6 || equip->type == 7) {
        s32 cell = (u8)Equip_GetClass(equip) + 63;
        topLeft->x = (cell & 7) * 32 + 1;
        topLeft->y = (cell >> 3) * 32 + 1;
        bottomRight->x = topLeft->x + 30;
        bottomRight->y = topLeft->y + 30;
    } else {
        topLeft->x = (((u16)equip->type + 74) & 7) * 32 + 1;
        topLeft->y = 289;
        bottomRight->x = topLeft->x + 30;
        bottomRight->y = 319;
    }
}

/* The weapon icon's texture rectangle for the byte at +2 of equip (an
 * EquipItem's id, its low byte), from gWeaponIconUVs. */
void Hud_WeaponIconUV(u8 *equip, Pt *topLeft, Pt *bottomRight)
{
    *topLeft = gWeaponIconUVs[equip[2]].uv0;
    *bottomRight = gWeaponIconUVs[equip[2]].uv1;
}

/* The equipment windows' initialised data. */
u8 gUnwearableCheckSlots[8] = {
#include "sub_main/0043B028.inc"
};
Pt gEquipSkillPos[12] = {
#include "sub_main/0043B030.inc"
};
u32 gSharpnessColors[8] = {
#include "sub_main/0043B060.inc"
};
s16 gHudTabLayout[12] = {
#include "sub_main/0043B080.inc"
};
u8 gButtonHintDetailIcons[8] = {
#include "sub_main/0043B098.inc"
};
u8 gButtonHintSortIcons[4][8] = {
#include "sub_main/0043B0A0.inc"
};
Pt gButtonHintSortPos[4] = {
#include "sub_main/0043B0C0.inc"
};
u8 gChatClassColors[16] = {
#include "sub_main/0043B0D0.inc"
};
