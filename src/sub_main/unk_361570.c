/*
 * The equipment windows' parts and the chat's word filter and phrases.
 * For the equipment menus (unk_356380.c draws the windows): whether an
 * armour fits the player, the weapon's element, the skill points, the
 * sharpness gauge, the compare arrow, the page tabs, the button hints and
 * the equipment icons. For the chat: masking the filtered words, the
 * phrase list (SysSave.phrases and their short names, gPhraseEntries) and
 * its keyboard, cutting a Shift-JIS string to a width. And the HUD's
 * shake (gShakeOffset).
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
/* The whole file is compiled at optimization level 4: its functions match only so. */
#pragma optimization_level 4

/* A point or a texture coordinate. */
typedef struct Pt {
    s16 x;
    s16 y;
} Pt;

/* A weapon icon's texture rectangle in gWeaponIconUVs: two corners. */
typedef struct WeaponIconUV {
    Pt uv0;
    Pt uv1;
} WeaponIconUV;

/* A skill and its points (Equip_ListSkillPoints fills 9 of them). */
typedef struct SkillPt {
    u8 id;
    s8 pts;
} SkillPt;


extern u8 gUnwearableCheckSlots[];
extern char *gDetailsHintText;
extern u8 gButtonHintDetailIcons[];
extern WeaponIconUV gWeaponIconUVs[];
extern char gDefaultPhrases[][45];
extern PhraseEntry gPhraseEntries[];
extern char *gSkillPointsTitle[];
extern char *gSkillPointsFmt;
extern char *gNoSkillText[];
extern char *gSkillNames[];
extern Pt gEquipSkillPos[];
extern char *gEquipNameFmt[];
extern char *gElementNames[];
extern char *gElementLineFmt;
extern char *gDefenseBonusFmt;
extern u16 gFrameTick;
extern Pt gButtonHintSortPos[];

extern u8 gButtonHintSortIcons[][8];
extern char *gSortHintText[];
extern char *gSelectText[];
extern char *gChatNgWordsA[];
extern char *gChatNgWordsB[];
extern Pt gShakeOffset;
extern f32 gShakeAmp;
extern u8 gShakeId;
extern u8 gShakeFlags;
extern s8 gShakeTimer;
extern s8 gShakeOn;

u8 Player_GetSkill(Player *p, u8 group);
void Font_SetSpacing(u8 a, u8 b);
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
void Text_Draw(char *s);
void Draw_Icons(Icon *icons, u8 count);
void StatusPage_DrawBox(s32 a, s16 y, s32 c);
void StatusPage_DrawArrow(s32 a, s16 y);
void *memcpy(void *dst, const void *src, u32 n);
void Phrase_TruncateNames(void);
void Sjis_Truncate(char *dst, u8 n, char *src);
s32 Sprite_Draw(Sprite *s);
u8 Equip_GetClass(EquipItem *e);
s8 Equip_GetSkillPoints(EquipItem *e, u8 kind);
void Text_SetColor(s16 color);
void Equip_ListSkillPoints(EquipItem *e, SkillPt *out);
s32 SkillPts_Compare(SkillPt *a, SkillPt *b);
void Font_SetPos(s16 x, s16 y);
void Player_CalcSkills(Player *p);
/* unit's own declaration: with the definition's void (Player *p, EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other, u8 c) (sub_main/unk_356380.c), EquipCompare_Draw: 38 of 64 words differ */
void EquipWindow_Draw(Player *p, EquipItem *e, s32 b, s16 y, u8 flags, EquipItem *other, s32 d);
s32 Field_ControlEnabled(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Kbd_Cancel(void);
s8 Kbd_Run(char *buf, s16 held, s16 pressed);
void ChatPhrase_Set(char *s, u8 idx);
s32 Chat_MaskWord(char *s, char *word, s8 mode);
void Chat_FilterWords(char *s);
f32 Math_Pow(f32 x, f32 y);
char *strstr(char *s, char *w);
s32 strlen(char *s);
s32 Sjis_IsSymbolOrDigit(u8 *s);
f32 Math_Sin(f32 a);
s32 sprintf(char *buf, const char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Gfx_SetState63(s32 a);
void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32 a);
void Menu_DrawArrow(s16 x, s16 y, s16 w, s16 h, u32 color, u8 flags);

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
    char buf[0x30];
    s16 kind, value, defense;
    s16 cmpKind, cmpValue, cmpDefense;
    s16 kindColor, defColor, defLabelColor, valueColor;

    Font_SetSpacing(0x12, 0x12);
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
        s16 msg = (kind < 5) ? 0xC : 0xD;

        sprintf(buf, gEquipNameFmt[msg], valueColor, value);
        Font_SetPos(x, y);
        Text_Printf(gElementLineFmt, kindColor, gElementNames[kind], buf);
        y += 0x14;
    }
    if (defense != 0) {
        Font_SetPos(x, y);
        Text_Printf(gDefenseBonusFmt, defLabelColor, defColor, defense);
    }
}

/* Draws the equipment's skill points (Equip_ListSkillPoints) at x, y: up to 9 rows at
 * gEquipSkillPos, each coloured 4 or 2 when cmp's points are higher or lower
 * (SkillPts_Compare). With flags 8 everything is drawn in colour 10 and nothing compared.
 * Skills 0x29, 0x2A and 0x2B show their name without points.
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
        if ((u32)(skillPt->id - 0x29) <= 1 || skillPt->id == 0x2B) {
            Text_Draw(gSkillNames[skillPt->id]);
        } else {
            Text_Printf(gSkillPointsFmt, gSkillNames[skillPt->id], cols[i], skillPt->pts);
        }
    }
}

/* Lists equip's skill points (skills 1 to 0x51 with nonzero points) into
 * out: at most 9 entries, the rest cleared (id 0). NULL lists none. */
void Equip_ListSkillPoints(EquipItem *equip, SkillPt *out)
{
    s32 n = 0;
    s32 skill;

    if (equip != NULL) {
        for (skill = 1; skill < 0x52; skill++) {
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

    Gfx_SetState63(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    pulse = Math_Sin(0.0000958738f * (f32)(u32)(u16)((gFrameTick & 0x1F) << 11));
    red = 96.0f * pulse;
    green = 20.0f * pulse;
    blue = 7.0f * pulse;
    blueByte = blue + 0xF7;
    y += (gFrameTick & 0x10) == 0 ? 0x7D : 0x8C;
    Menu_DrawArrow((s16)x + 0x89, y, 0x20, 0x10,
                  0xFF000000 | ((u8)(red + 0x90) << 16) | ((u8)(green + 0xE4) << 8) | blueByte, 3);
}

/* Draws two equipment windows to compare, shown above other (0xA2 lower),
 * each compared with the other, and the arrow; online the local player's
 * skills are worked out again first. */
void EquipCompare_Draw(EquipItem *shown, EquipItem *other, s32 x, s16 y, u8 flags)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    if (gScene.online != 0) {
        Player_CalcSkills(pl);
    }
    EquipWindow_Draw(pl, shown, x, y, (flags | 0x20) & ~8, other, 0xB2);
    EquipWindow_Draw(pl, other, x, y + 0xA2, flags, shown, 0xB2);
    StatusPage_DrawArrow(x, y);
}

/* Draws the two empty compare windows and the arrow. */
void StatusPage_DrawBoxes(s32 x, s16 y, s32 width)
{
    StatusPage_DrawBox(x, y, width);
    StatusPage_DrawBox(x, y + 0xA2, width);
    StatusPage_DrawArrow(x, y);
}

/* A sharpness level after pl's sharpness skill (skill group 0x20): one up
 * at value 0x21 (at most 5), else one down (at least 0). */
s32 Player_ApplySharpnessSkill(Player *pl, s32 level)
{
    u8 skill = Player_GetSkill(pl, 0x20);

    if (skill != 0) {
        if (skill == 0x21) {
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

typedef s16 SharpRow[6];
extern u8 gWeaponKindClass[];
extern SharpRow *gSharpnessTables[];
extern u32 gSharpnessColors[];
Player *Player_PreviewWith(Player *p, EquipItem *e);
s32 Prim_FillTri(PrimTri *t);
s32 Prim_FillRect(PrimLine *r);
void Text_PrintfColored(const char *fmt, ...);

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
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(163.0f + (s16)(x - 0x24), (s16)(y - 4) + 2);
        Text_PrintfColored("%+d", delta);
    }
    x -= 0x24;
    tri.color = color;
    tri.x0 = x;
    tri.x1 = tri.x2 = x + 5;
    top = y - 4;
    tri.y1 = top;
    tri.y0 = top + 0xB;
    tri.y2 = top + 0x15;
    Prim_FillTri(&tri);
    left = 146.0f + x;
    tri.x0 = 10.0f + left;
    tri.x1 = tri.x2 = 5.0f + left;
    Prim_FillTri(&tri);
    left = 5.0f + x;
    rect.x0 = left;
    rect.x1 = 146.0f + left;
    rect.y0 = top;
    rect.y1 = rect.y0 + 0x15;
    rect.color = color;
    Prim_FillRect(&rect);
    left = 6.0f + x;
    rect.x0 = left;
    rect.x1 = 144.0f + left;
    rect.y0 = top + 2;
    rect.y1 = rect.y0 + 0x11;
    rect.color = 0xFF000000;
    Prim_FillRect(&rect);
    bar = (s16 *)gSharpnessTables[gWeaponKindClass[data->kind]];
    bar += data->sharpType * 6;
    rect.y0 = top + 4;
    rect.y1 = top + 0x11;
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
extern char *gTabPageFmt[];
extern char *gPageFmt;
void Font_SetColor(s32 color);
/* Draws a two-part tab at (x, y + 100) from the offsets at gHudTabLayout (gHudTabLayoutBig and two
 * more sprites when flags has 0x40), then the format gTabPageFmt (gPageFmt) with flags & 0xF
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
    spr.h = 0x12;
    spr.color = 0xFF20FF30;
    spr.x = x + pos[1];
    *(s32 *)&spr.u0 = 0x1A00A6;
    *(s32 *)&spr.u1 = 0x2E0094;
    Sprite_Draw(&spr);
    spr.x = x + pos[2];
    spr.u0 = 0x94;
    spr.u1 = 0xA6;
    Sprite_Draw(&spr);
    text = gTabPageFmt[0];
    if (big) {
        spr.y = y - 1;
        spr.w = 0x18;
        spr.h = 0x14;
        spr.color = -1;
        spr.x = x + pos[4];
        *(s32 *)&spr.u0 = 0x61;
        *(s32 *)&spr.u1 = 0x180078;
        Sprite_Draw(&spr);
        spr.x = x + pos[5];
        spr.u0 = 0x91;
        spr.u1 = 0xA8;
        Sprite_Draw(&spr);
        text = gPageFmt;
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(rgba);
    Font_SetPos(x + pos[0], y);
    Text_PrintfColored(text, num, color);
}

/* Draws the button hint for the details page, with its button icon. */
void ButtonHint_DrawDetails(void)
{
    Font_SetSpacing(0x12, 0x12);
    Text_PrintfAt(0x22E, 0x18E, 0, gDetailsHintText);
    Draw_Icons((Icon *) gButtonHintDetailIcons, 1);
}

/* Draws the sort hint of menu index (gButtonHintSortPos): the select
 * button's label, the sort text and the button icon. */
void ButtonHint_DrawSort(u8 index)
{
    Text_SetColor(0);
    Font_SetSpacing(0x10, 0xE);
    Font_SetPos(gButtonHintSortPos[index].x - 0x4F, gButtonHintSortPos[index].y + 2);
    Text_Draw(gSelectText[0]);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(gButtonHintSortPos[index].x, gButtonHintSortPos[index].y);
    Text_Draw(gSortHintText[0]);
    Draw_Icons((Icon *) gButtonHintSortIcons[index], 1);
}

/* The texture rectangle of equip's icon on the 32-pixel icon sheet: a
 * weapon's by its class (cells from 0x3F), an armour's by its type (cells
 * from 0x4A, in row 9). */
void Equip_GetIconUV(EquipItem *equip, Pt *topLeft, Pt *bottomRight)
{
    if (equip->type == 6 || equip->type == 7) {
        s32 cell = (u8)Equip_GetClass(equip) + 0x3F;
        topLeft->x = (cell & 7) * 32 + 1;
        topLeft->y = (cell >> 3) * 32 + 1;
        bottomRight->x = topLeft->x + 0x1E;
        bottomRight->y = topLeft->y + 0x1E;
    } else {
        topLeft->x = (((u16)equip->type + 0x4A) & 7) * 32 + 1;
        topLeft->y = 0x121;
        bottomRight->x = topLeft->x + 0x1E;
        bottomRight->y = 0x13F;
    }
}

/* The weapon icon's texture rectangle for the byte at +2 of equip (an
 * EquipItem's id, its low byte), from gWeaponIconUVs. */
void Hud_WeaponIconUV(u8 *equip, Pt *topLeft, Pt *bottomRight)
{
    *topLeft = gWeaponIconUVs[equip[2]].uv0;
    *bottomRight = gWeaponIconUVs[equip[2]].uv1;
}

/* Masks the filtered words in a chat text: list A wherever found, list B
 * only as whole words (Chat_MaskWord's modes 0 and 2). */
void Chat_FilterWords(char *text)
{
    char **word = gChatNgWordsA;

    do {
        Chat_MaskWord(text, *word, 0);
        word++;
    } while (*word != 0);
    word = gChatNgWordsB;
    do {
        Chat_MaskWord(text, *word, 2);
        word++;
    } while (*word != 0);
}

/* Replaces each match of word in text by full-width asterisks (Shift-JIS
 * 0x8196, one per two bytes). Modes 0 and 1 mask every match; mode 2 only
 * one that starts the text or follows a symbol or digit, or ends the text
 * or is followed by one (Sjis_IsSymbolOrDigit). Returns 0. */
s32 Chat_MaskWord(char *text, char *word, s8 mode)
{
    s32 mask;
    s32 len;
    char *from = text;
    char *hit;
    char *end;

    while ((hit = strstr(from, word)) != NULL) {
        switch (mode) {
        case 0:
            len = strlen(word);
            mask = 1;
            break;
        case 1:
            len = strlen(word);
            mask = 1;
            break;
        case 2:
            mask = 0;
            len = strlen(word);
            if (hit == text) {
                mask = 1;
            } else if (Sjis_IsSymbolOrDigit((u8 *)hit - 2) == 1) {
                mask = 1;
            } else {
                end = from + strlen(from);
                if (hit + len == end) {
                    mask = 1;
                } else if (Sjis_IsSymbolOrDigit((u8 *)hit + len) == 1) {
                    mask = 1;
                }
            }
            break;
        }
        from = hit + 1;
        if (mask) {
            for (len >>= 1; len > 0; len--) {
                hit[0] = 0x81;
                hit[1] = 0x96;
                hit += 2;
            }
            from = hit;
        }
    }
    return 0;
}

/* Whether the Shift-JIS character at sjis is a full-width symbol (lead
 * byte 0x81) or a full-width digit (0x824F to 0x8258). */
s32 Sjis_IsSymbolOrDigit(u8 *sjis)
{
    if (sjis[0] == 0x81 && sjis[1] >= 0x40 && sjis[1] < 0xED) {
        return 1;
    }
    if (sjis[0] == 0x82 && sjis[1] >= 0x4F && sjis[1] < 0x59) {
        return 1;
    }
    return 0;
}

/* Filters a chat text (Chat_FilterWords) unless it holds only spaces
 * (half-width or full-width); returns 1 if it held anything else. */
s32 Chat_FilterIfNotBlank(u8 *text)
{
    u8 *cursor = text;
    u8 byte = *cursor;

    while (byte != 0) {
        if (byte == 0x20) {
            cursor++;
        } else if (byte == 0x81 && cursor[1] == 0x40) {
            cursor += 2;
        } else {
            Chat_FilterWords((char *)text);
            return 1;
        }
        byte = *cursor;
    }
    return 0;
}

/* Puts the default chat phrases back (gDefaultPhrases) and remakes their
 * names. */
void Phrase_ResetDefaults(void)
{
    memcpy(gSysSave.phrases[0], gDefaultPhrases, sizeof(gSysSave.phrases));
    Phrase_TruncateNames();
}

/* Makes each phrase's name: the phrase cut to PHRASE_NAME_LEN bytes. */
void Phrase_TruncateNames(void)
{
    PhraseEntry *entry = gPhraseEntries;
    s32 n = PHRASE_COUNT;

    do {
        Sjis_Truncate(entry->name, PHRASE_NAME_LEN, entry->text);
        entry++;
    } while (--n);
}

/* Sets phrase index to text (at most PHRASE_LEN - 1 bytes) and remakes its
 * name. */
void ChatPhrase_Set(char *text, u8 index)
{
    PhraseEntry *entry;
    s32 n;
    char *src;
    char *dst;

    src = text;
    entry = &gPhraseEntries[index];
    n = PHRASE_LEN - 1;
    dst = entry->text;

    do {
        if (*src == 0) {
            break;
        }
        *dst = *src;
        n--;
        src++;
        dst++;
    } while (n > 0);
    *dst = 0;
    Sjis_Truncate(entry->name, PHRASE_NAME_LEN, text);
}

/* Opens the software keyboard on phrase index to edit it, when the field
 * control allows; returns 1 if it opened. */
s32 PhraseMenu_OpenKbd(u8 index)
{
    if (Field_ControlEnabled() == 0) {
        return 0;
    }
    Kbd_SetPos(0x50, 80.0f);
    Chat_OpenKbd(2, 0xE, PHRASE_LEN - 1, gPhraseEntries[index].text);
    Sound_PlaySeFlat(0, 0x11, 0);
    return 1;
}

/* Runs the phrase keyboard for phrase index: button 0x4000 cancels; an
 * entered text replaces the phrase. Returns 1 once the keyboard closed,
 * else 0. */
s8 PhraseMenu_KbdUpdate(u8 index)
{
    char buf[0x30];
    s8 result;

    if (gPadState[0].pressed & 0x4000) {
        Kbd_Cancel();
        return 1;
    }
    buf[0] = 0;
    result = Kbd_Run(buf, gPadState[0].held, gPadState[0].pressed);
    if (result != 0) {
        if (result > 0) {
            ChatPhrase_Set(buf, index);
        }
        Kbd_Cancel();
        return 1;
    }
    return 0;
}

/* Draws src cut to n bytes (Sjis_Truncate). */
void Text_DrawTruncated(u8 n, char *src)
{
    char buf[0x30];

    Sjis_Truncate(buf, n, src);
    Text_Draw(buf);
}

/* Copies the Shift-JIS string src to dst cut at n bytes (a two-byte character is kept whole) and
 * ends it with half-width dots (0xA5) and a terminator. */
void Sjis_Truncate(char *dst, u8 n, char *src)
{
    char flags[0x30];
    s32 left = n + 1;
    char *start = dst;
    char *wide = flags;

    while (left > 0) {
        u8 lead;

        *dst = *src;
        lead = *dst;
        if ((lead >= 0x80 && lead <= 0x9F) || (lead >= 0xE0 && lead <= 0xFF)) {
            dst[1] = src[1];
            left--;
            src += 2;
            wide[0] = 0;
            dst += 2;
            wide[1] = 1;
            wide += 2;
        } else {
            if (*src == 0) {
                return;
            }
            wide[0] = 0;
            dst++;
            src++;
            wide++;
        }
        left--;
    }
    if (flags[n - 2] == 0) {
        start[n - 1] = 0xA5;
        start[n - 2] = 0xA5;
    } else {
        start[n - 1] = 0xA5;
        start[n - 2] = 0xA5;
        start[n - 3] = 0xA5;
    }
    start[n] = 0;
}

/* Draws the two end caps of the pouch bar, height high, at x and x + 0x15. */
void Hud_DrawPouchBarEnds(s16 x, s16 y, s16 height)
{
    Sprite spr;

    spr.w = 5;
    spr.x = x;
    spr.color = -1;
    spr.y = y;
    spr.h = height;
    *(s32 *)&spr.u0 = 0x200074;
    *(s32 *)&spr.u1 = 0x340079;
    Sprite_Draw(&spr);
    spr.x = x + 0x15;
    *(s32 *)&spr.u0 = 0x200078;
    *(s32 *)&spr.u1 = 0x340073;
    Sprite_Draw(&spr);
}

/* Sets spr to the ammo icon for the byte at +2 of equip: one icon for 10,
 * another for the rest. */
void Hud_AmmoIconSprite(u8 *equip, Sprite *spr)
{
    if (equip[2] != 10) {
        spr->x = 0x1A8;
        spr->y = 0xFD;
        spr->w = 0xB;
        spr->h = 0x14;
        *(s32 *)&spr->u0 = 0xAD0015;
        *(s32 *)&spr->u1 = 0xC10020;
    } else {
        spr->x = 0x1B0;
        spr->y = 0xFE;
        spr->w = 0x10;
        spr->h = 0x14;
        *(s32 *)&spr->u0 = 0xAD0004;
        *(s32 *)&spr->u1 = 0xC10014;
    }
}

/* Starts a shake of the HUD element id (gShakeOffset) for SHAKE_FRAMES
 * frames along the axis in flags (SHAKE_AXIS_*): it eases from amp back
 * to 0, or with SHAKE_SLIDE it slides from 0 out to amp and stays. */
void Shake_Start(u8 id, u8 flags, f32 amp)
{
    gShakeId = id;
    *(s32 *)&gShakeOffset = 0;
    gShakeOn = 1;
    gShakeTimer = SHAKE_FRAMES;
    gShakeFlags = flags;
    gShakeAmp = amp;
    if (flags & SHAKE_SLIDE) {
        amp = 0.0f;
    }
    switch (flags & SHAKE_AXIS_MASK) {
    case SHAKE_AXIS_X:
        gShakeOffset.x = amp;
        break;
    case SHAKE_AXIS_Y:
        gShakeOffset.y = amp;
        break;
    }
}

/* Steps the shake of element id: returns 1 while it moves. When it ends,
 * the offset goes back to 0 (or, after a slide, the shake is let go). */
s32 Shake_Update(u8 id)
{
    f32 progress;

    if (gShakeId == id) {
        if (gShakeTimer != 0) {
            progress = 0.16666667f * gShakeTimer /* 1 / SHAKE_FRAMES */;
            if ((gShakeFlags & SHAKE_SLIDE) == 0) {
                progress = Math_Pow(progress, 2.25f);
            } else {
                progress = 1.0f - progress;
            }
            switch (gShakeFlags & SHAKE_AXIS_MASK) {
            case SHAKE_AXIS_X:
                gShakeOffset.x = gShakeAmp * progress;
                break;
            case SHAKE_AXIS_Y:
                gShakeOffset.y = gShakeAmp * progress;
                break;
            }
            gShakeTimer--;
            return 1;
        }
        if (gShakeOn != 0) {
            if ((gShakeFlags & SHAKE_SLIDE) == 0) {
                switch (gShakeFlags & SHAKE_AXIS_MASK) {
                case SHAKE_AXIS_X:
                    gShakeOffset.x = 0;
                    break;
                case SHAKE_AXIS_Y:
                    gShakeOffset.y = 0;
                    break;
                }
            } else {
                gShakeId = 0xFF;
            }
            gShakeOn = 0;
        }
    }
    return 0;
}

/* unk_361570's .data (0x43B028-0x43B0E0) */
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
u32 gChatClassColors[4] = {
#include "sub_main/0043B0D0.inc"
};
