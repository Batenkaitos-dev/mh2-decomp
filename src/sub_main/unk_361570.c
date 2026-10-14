#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "player.h"
#include "pad.h"
#include "prim.h"
#include "syssave.h"
#include "save.h"
#pragma optimization_level 4

typedef struct Pt {
    s16 x;
    s16 y;
} Pt;

typedef struct Pos8 {
    Pt p;
    s16 unk4;
    s16 unk6;
} Pos8;

/* A skill and its points (Equip_ListSkillPoints fills 9 of them). */
typedef struct SkillPt {
    u8 id;
    s8 pts;
} SkillPt;

/* A name of 12 bytes and its source string. */
typedef struct NameEntry {
    char name[12];
    char *src;
} NameEntry;

extern u8 gUnwearableCheckSlots[];
extern char *gDetailsHintText;
extern u8 gButtonHintDetailIcons[];
extern Pos8 gWeaponIconUVs[];
extern Pos8 D_00432664[];
extern char gDefaultPhrases[][45];
extern NameEntry gQuestListEntries[];
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
extern Pt D_0043B0C2[];
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

s32 Equip_DataWearable(Player *p, EquipItem *e)
{
    ArmorData *data = &Equip_GetData(e)->armor;
    u8 mask;

    if (gSave.gender == 0) {
        mask = 1;
    } else {
        mask = 2;
    }
    if (p->equip[6].type == 6) {
        mask |= 4;
    } else {
        mask |= 8;
    }
    if ((mask & data->flags) == mask) {
        return 1;
    }
    return 0;
}

void Player_RemoveUnwearable(Player *p)
{
    u32 i;

    for (i = 0; i < 5; i++) {
        u8 idx = gUnwearableCheckSlots[i];
        if (p->equip[idx].id != 0 && (u8)Equip_DataWearable(p, &p->equip[idx]) == 0) {
            p->equip[idx].id = 0;
        }
    }
}

static s32 BladeData_ElementKind(BladeData *d, s16 *out)
{
    if (d->elementType != 0) {
        *out = d->elementType - 1;
        return d->elementValue;
    }
    if (d->statusType != 0) {
        *out = d->statusType + 4;
        return d->statusValue;
    }
    *out = -1;
    return -1;
}

static s32 GunData_ElementKind(GunData *d, s16 *out)
{
    if (d->elementType != 0) {
        *out = d->elementType - 1;
        return d->elementValue;
    }
    *out = -1;
    return -1;
}

void Weapon_GetElementInfo(EquipItem *e, s16 *a, s16 *b, s16 *c)
{
    EquipData *d;

    if (e->type == 6) {
        d = Equip_GetData(e);
        *a = BladeData_ElementKind(&d->blade, b);
        *c = d->blade.defense;
    } else {
        d = Equip_GetData(e);
        *c = d->gun.defense;
        *a = -1;
        *b = -1;
        if ((u8)Equip_GetClass(e) == 10) {
            *a = GunData_ElementKind(&d->gun, b);
        }
    }
}

void EquipMenu_DrawElement(EquipItem *e, s16 x, s16 y, EquipItem *cmp)
{
    char buf[0x30];
    s16 b, a, c;
    s16 b2, a2, c2;
    s16 col1, col2, col3, col0;

    Font_SetSpacing(0x12, 0x12);
    Weapon_GetElementInfo(e, &a, &b, &c);
    col1 = col0 = col3 = col2 = 0;
    if (cmp != NULL) {
        Weapon_GetElementInfo(cmp, &a2, &b2, &c2);
        if (b >= 0) {
            if (b == b2) {
                if (a > a2) {
                    col0 = 4;
                } else if (a < a2) {
                    col0 = 2;
                }
            } else {
                col1 = col0 = 4;
            }
        }
        if (c > 0) {
            if (c2 > 0) {
                if (c2 < c) {
                    col2 = 4;
                } else if (c < c2) {
                    col2 = 2;
                }
            } else {
                col3 = col2 = 4;
            }
        }
    }
    if (b >= 0) {
        s16 msg = (b < 5) ? 0xC : 0xD;

        sprintf(buf, gEquipNameFmt[msg], col0, a);
        Font_SetPos(x, y);
        Text_Printf(gElementLineFmt, col1, gElementNames[b], buf);
        y += 0x14;
    }
    if (c != 0) {
        Font_SetPos(x, y);
        Text_Printf(gDefenseBonusFmt, col3, col2, c);
    }
}

/* Draws the equipment's skill points (Equip_ListSkillPoints) at x, y: up to 9 rows at
 * gEquipSkillPos, each coloured 4 or 2 when cmp's points are higher or lower (SkillPts_Compare).
 * Matched in round 29 (nm-other: cols and gEquipSkillPos indexed by i, y narrowed into ty,
 * e != cmp; nm-sub: p, i, col, ty declared first, in that order, as the saved registers
 * need; the compare loop walks cols with cp set in its for, and the draw loop steps p in
 * its for, so that p is incremented after the walkers MWCC creates, as in the game). */
void EquipMenu_DrawSkills(EquipItem *e, s16 x, s32 y, EquipItem *cmp, u8 flags)
{
    SkillPt pts[16];
    SkillPt pts2[16];
    u8 cols[9];
    SkillPt *p;
    u32 i;
    u8 col;
    s32 ty;
    u8 tc;
    SkillPt *q;
    u8 *cp;

    if (!(flags & 8)) {
        col = 0;
        tc = 5;
    } else {
        col = 10;
        tc = col;
        cmp = NULL;
    }
    Text_SetColor(tc);
    ty = (s16)y;
    Font_SetPos(x, ty);
    Text_Draw(gSkillPointsTitle[0]);
    p = pts;
    cols[0] = cols[1] = cols[2] = cols[3] = cols[4] = cols[5] = cols[6] = cols[7] = cols[8] = col;
    Equip_ListSkillPoints(e, p);
    if (cmp != NULL) {
        if (e != cmp) {
            q = pts2;
            Equip_ListSkillPoints(cmp, q);
            for (i = 0, cp = cols; i < 9; i++) {
                s16 r = SkillPts_Compare(p, q);

                if (r > 0) {
                    *cp = 4;
                } else if (r < 0) {
                    *cp = 2;
                }
                cp++;
                p++;
            }
        }
        p = pts;
    }
    for (i = 0; i < 9; i++, p++) {
        Font_SetPos(x + gEquipSkillPos[i].x, ty + gEquipSkillPos[i].y);
        if (p->id == 0) {
            if (i == 0) {
                Text_SetColor(col);
                Text_Draw(gNoSkillText[0]);
            }
            break;
        }
        Text_SetColor(col);
        if ((u32)(p->id - 0x29) <= 1 || p->id == 0x2B) {
            Text_Draw(gSkillNames[p->id]);
        } else {
            Text_Printf(gSkillPointsFmt, gSkillNames[p->id], cols[i], p->pts);
        }
    }
}

void Equip_ListSkillPoints(EquipItem *e, SkillPt *out)
{
    s32 n = 0;
    s32 k;

    if (e != NULL) {
        for (k = 1; k < 0x52; k++) {
            s8 pts = Equip_GetSkillPoints(e, k);
            if (pts != 0) {
                out->id = k;
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

s32 SkillPts_Compare(SkillPt *a, SkillPt *b)
{
    s32 n = 9;

    do {
        if (b->id == 0) {
            break;
        }
        if (a->id == b->id) {
            if (a->pts > b->pts) {
                return 1;
            } else if (a->pts < b->pts) {
                return -1;
            }
            return 0;
        }
        b++;
    } while (--n != 0);
    if (a->pts > 0) {
        return 1;
    } else if (a->pts < 0) {
        return -1;
    }
    return 0;
}


void StatusPage_DrawArrow(s32 x, s16 y)
{
    f32 s;
    s8 r, g, b;
    u8 bc;

    Gfx_SetState63(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s = Math_Sin(0.0000958738f * (f32)(u32)(u16)((gFrameTick & 0x1F) << 11));
    r = 96.0f * s;
    g = 20.0f * s;
    b = 7.0f * s;
    bc = b + 0xF7;
    y += (gFrameTick & 0x10) == 0 ? 0x7D : 0x8C;
    Menu_DrawArrow((s16)x + 0x89, y, 0x20, 0x10,
                  0xFF000000 | ((u8)(r + 0x90) << 16) | ((u8)(g + 0xE4) << 8) | bc, 3);
}

void EquipCompare_Draw(EquipItem *a, EquipItem *b, s32 c, s16 y, u8 flags)
{
    Player *p = &gPlayers[gScene.localPlayer];

    if (gScene.online != 0) {
        Player_CalcSkills(p);
    }
    EquipWindow_Draw(p, a, c, y, (flags | 0x20) & ~8, b, 0xB2);
    EquipWindow_Draw(p, b, c, y + 0xA2, flags, a, 0xB2);
    StatusPage_DrawArrow(c, y);
}

void StatusPage_DrawBoxes(s32 a, s16 y, s32 c)
{
    StatusPage_DrawBox(a, y, c);
    StatusPage_DrawBox(a, y + 0xA2, c);
    StatusPage_DrawArrow(a, y);
}

s32 Player_ApplySharpnessSkill(Player *p, s32 n)
{
    u8 s = Player_GetSkill(p, 0x20);

    if (s != 0) {
        if (s == 0x21) {
            n++;
            if (n > 5) {
                n = 5;
            }
        } else {
            n--;
            if (n < 0) {
                n = 0;
            }
        }
    }
    return n;
}

typedef s16 SharpRow[6];
extern u8 gWeaponKindClass[];
extern SharpRow *gSharpnessTables[];
extern u32 gSharpnessColors[];
Player *Player_PreviewWith(Player *p, EquipItem *e);
s32 Prim_FillTri(PrimTri *t);
s32 Prim_FillRect(PrimLine *r);
void Text_PrintfColored(const char *fmt, ...);

/* Draws weapon e's sharpness gauge at (x, y) for player p: the frame, the six colour
 * segments (gSharpnessTables, 0.35 pixel a unit), a black cover past the level after the
 * sharpness skill (Player_ApplySharpnessSkill) and, when the skill changes it, a +1 or -1. */
void EquipMenu_DrawSharpness(Player *p, EquipItem *e, s16 x, s16 y)
{
    BladeData *data;
    s16 *bar;
    u32 *col;
    u32 color;
    s32 lv;
    s32 d;
    PrimTri tri;
    PrimLine r;
    s32 i;
    s32 base;
    f32 fx;
    s16 *px1;
    s16 sy;

    data = &Equip_GetData(e)->blade;
    base = data->sharpMax;
    lv = Player_ApplySharpnessSkill(Player_PreviewWith(p, e), base);
    if (lv == base) {
        color = 0xFF968A63;
    } else {
        if (base < lv) {
            color = 0xFF57FFFF;
            Text_SetColor(4);
            d = 1;
        } else {
            color = 0xFFFF435D;
            Text_SetColor(2);
            d = -1;
        }
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(163.0f + (s16)(x - 0x24), (s16)(y - 4) + 2);
        Text_PrintfColored("%+d", d);
    }
    x -= 0x24;
    tri.color = color;
    tri.x0 = x;
    tri.x1 = tri.x2 = x + 5;
    sy = y - 4;
    tri.y1 = sy;
    tri.y0 = sy + 0xB;
    tri.y2 = sy + 0x15;
    Prim_FillTri(&tri);
    fx = 146.0f + x;
    tri.x0 = 10.0f + fx;
    tri.x1 = tri.x2 = 5.0f + fx;
    Prim_FillTri(&tri);
    fx = 5.0f + x;
    r.x0 = fx;
    r.x1 = 146.0f + fx;
    r.y0 = sy;
    r.y1 = r.y0 + 0x15;
    r.color = color;
    Prim_FillRect(&r);
    fx = 6.0f + x;
    r.x0 = fx;
    r.x1 = 144.0f + fx;
    r.y0 = sy + 2;
    r.y1 = r.y0 + 0x11;
    r.color = 0xFF000000;
    Prim_FillRect(&r);
    bar = (s16 *)gSharpnessTables[gWeaponKindClass[data->kind]];
    bar += data->sharpType * 6;
    r.y0 = sy + 4;
    r.y1 = sy + 0x11;
    fx = 8.0f + x;
    r.x1 = fx;
    px1 = &r.x1;
    col = gSharpnessColors;
    for (i = 0; i < 6; i++, bar++) {
        r.x0 = *px1;
        r.x1 = fx + 0.35f * *bar;
        r.color = col[i];
        Prim_FillRect(&r);
    }
    if (lv < 5) {
        r.x0 = fx + 0.35f * (150.0f + lv * 50);
        r.x1 = 140.0f + fx;
        r.color = 0xFF000000;
        Prim_FillRect(&r);
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
    Sprite s;
    s16 *pos;
    char *text;
    u8 num = flags & 0xF;
    s32 big = flags & 0x40;

    y += 100;
    pos = gHudTabLayout;
    if (big) {
        pos = gHudTabLayoutBig;
    }
    s.w = pos[3];
    s.y = y;
    s.h = 0x12;
    s.color = 0xFF20FF30;
    s.x = x + pos[1];
    *(s32 *)&s.u0 = 0x1A00A6;
    *(s32 *)&s.u1 = 0x2E0094;
    Sprite_Draw(&s);
    s.x = x + pos[2];
    s.u0 = 0x94;
    s.u1 = 0xA6;
    Sprite_Draw(&s);
    text = gTabPageFmt[0];
    if (big) {
        s.y = y - 1;
        s.w = 0x18;
        s.h = 0x14;
        s.color = -1;
        s.x = x + pos[4];
        *(s32 *)&s.u0 = 0x61;
        *(s32 *)&s.u1 = 0x180078;
        Sprite_Draw(&s);
        s.x = x + pos[5];
        s.u0 = 0x91;
        s.u1 = 0xA8;
        Sprite_Draw(&s);
        text = gPageFmt;
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(rgba);
    Font_SetPos(x + pos[0], y);
    Text_PrintfColored(text, num, color);
}

void ButtonHint_DrawDetails(void)
{
    Font_SetSpacing(0x12, 0x12);
    Text_PrintfAt(0x22E, 0x18E, 0, gDetailsHintText);
    Draw_Icons((Icon *) gButtonHintDetailIcons, 1);
}

void ButtonHint_DrawSort(u8 i)
{
    Text_SetColor(0);
    Font_SetSpacing(0x10, 0xE);
    Font_SetPos(gButtonHintSortPos[i].x - 0x4F, D_0043B0C2[i].x + 2);
    Text_Draw(gSelectText[0]);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(gButtonHintSortPos[i].x, D_0043B0C2[i].x);
    Text_Draw(gSortHintText[0]);
    Draw_Icons((Icon *) gButtonHintSortIcons[i], 1);
}

void Equip_GetIconUV(EquipItem *e, Pt *uv0, Pt *uv1)
{
    if (e->type == 6 || e->type == 7) {
        s32 c = (u8)Equip_GetClass(e) + 0x3F;
        uv0->x = (c & 7) * 32 + 1;
        uv0->y = (c >> 3) * 32 + 1;
        uv1->x = uv0->x + 0x1E;
        uv1->y = uv0->y + 0x1E;
    } else {
        uv0->x = (((u16)e->type + 0x4A) & 7) * 32 + 1;
        uv0->y = 0x121;
        uv1->x = uv0->x + 0x1E;
        uv1->y = 0x13F;
    }
}

void Hud_WeaponIconUV(u8 *a, Pt *p0, Pt *p1)
{
    *p0 = gWeaponIconUVs[a[2]].p;
    *p1 = D_00432664[a[2]].p;
}

void Chat_FilterWords(char *s)
{
    char **w = gChatNgWordsA;

    do {
        Chat_MaskWord(s, *w, 0);
        w++;
    } while (*w != 0);
    w = gChatNgWordsB;
    do {
        Chat_MaskWord(s, *w, 2);
        w++;
    } while (*w != 0);
}

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

s32 Sjis_IsSymbolOrDigit(u8 *s)
{
    if (s[0] == 0x81 && s[1] >= 0x40 && s[1] < 0xED) {
        return 1;
    }
    if (s[0] == 0x82 && s[1] >= 0x4F && s[1] < 0x59) {
        return 1;
    }
    return 0;
}

s32 Chat_FilterIfNotBlank(u8 *s)
{
    u8 *p = s;
    u8 c = *p;

    while (c != 0) {
        if (c == 0x20) {
            p++;
        } else if (c == 0x81 && p[1] == 0x40) {
            p += 2;
        } else {
            Chat_FilterWords((char *)s);
            return 1;
        }
        c = *p;
    }
    return 0;
}

void Phrase_ResetDefaults(void)
{
    memcpy(gSysSave.phrases[0], gDefaultPhrases, 0x438);
    Phrase_TruncateNames();
}

void Phrase_TruncateNames(void)
{
    NameEntry *e = gQuestListEntries;
    s32 n = 0x18;

    do {
        Sjis_Truncate(e->name, 10, e->src);
        e++;
    } while (--n);
}

void ChatPhrase_Set(char *s, u8 idx)
{
    NameEntry *e;
    s32 n;
    char *p;
    char *d;

    p = s;
    e = &gQuestListEntries[idx];
    n = 0x2C;
    d = e->src;

    do {
        if (*p == 0) {
            break;
        }
        *d = *p;
        n--;
        p++;
        d++;
    } while (n > 0);
    *d = 0;
    Sjis_Truncate(e->name, 10, s);
}


s32 PhraseMenu_OpenKbd(u8 idx)
{
    if (Field_ControlEnabled() == 0) {
        return 0;
    }
    Kbd_SetPos(0x50, 80.0f);
    Chat_OpenKbd(2, 0xE, 0x2C, gQuestListEntries[idx].src);
    Sound_PlaySeFlat(0, 0x11, 0);
    return 1;
}

s8 PhraseMenu_KbdUpdate(u8 idx)
{
    char buf[0x30];
    s8 r;

    if (gPadState[0].pressed & 0x4000) {
        Kbd_Cancel();
        return 1;
    }
    buf[0] = 0;
    r = Kbd_Run(buf, gPadState[0].held, gPadState[0].pressed);
    if (r != 0) {
        if (r > 0) {
            ChatPhrase_Set(buf, idx);
        }
        Kbd_Cancel();
        return 1;
    }
    return 0;
}

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
    char *s = dst;
    char *f = flags;

    while (left > 0) {
        u8 c;

        *dst = *src;
        c = *dst;
        if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFF)) {
            dst[1] = src[1];
            left--;
            src += 2;
            f[0] = 0;
            dst += 2;
            f[1] = 1;
            f += 2;
        } else {
            if (*src == 0) {
                return;
            }
            f[0] = 0;
            dst++;
            src++;
            f++;
        }
        left--;
    }
    if (flags[n - 2] == 0) {
        s[n - 1] = 0xA5;
        s[n - 2] = 0xA5;
    } else {
        s[n - 1] = 0xA5;
        s[n - 2] = 0xA5;
        s[n - 3] = 0xA5;
    }
    s[n] = 0;
}

void Hud_DrawPouchBarEnds(s16 x, s16 y, s16 h)
{
    Sprite s;

    s.w = 5;
    s.x = x;
    s.color = -1;
    s.y = y;
    s.h = h;
    *(s32 *)&s.u0 = 0x200074;
    *(s32 *)&s.u1 = 0x340079;
    Sprite_Draw(&s);
    s.x = x + 0x15;
    *(s32 *)&s.u0 = 0x200078;
    *(s32 *)&s.u1 = 0x340073;
    Sprite_Draw(&s);
}

void Hud_AmmoIconSprite(u8 *a, Sprite *s)
{
    if (a[2] != 10) {
        s->x = 0x1A8;
        s->y = 0xFD;
        s->w = 0xB;
        s->h = 0x14;
        *(s32 *)&s->u0 = 0xAD0015;
        *(s32 *)&s->u1 = 0xC10020;
    } else {
        s->x = 0x1B0;
        s->y = 0xFE;
        s->w = 0x10;
        s->h = 0x14;
        *(s32 *)&s->u0 = 0xAD0004;
        *(s32 *)&s->u1 = 0xC10014;
    }
}

void Shake_Start(u8 id, u8 flags, f32 amp)
{
    gShakeId = id;
    *(s32 *)&gShakeOffset = 0;
    gShakeOn = 1;
    gShakeTimer = 6;
    gShakeFlags = flags;
    gShakeAmp = amp;
    if (flags & 0x80) {
        amp = 0.0f;
    }
    switch (flags & 0xF) {
    case 0:
        gShakeOffset.x = amp;
        break;
    case 1:
        gShakeOffset.y = amp;
        break;
    }
}

s32 Shake_Update(u8 id)
{
    f32 t;

    if (gShakeId == id) {
        if (gShakeTimer != 0) {
            t = 0.16666667f * gShakeTimer;
            if ((gShakeFlags & 0x80) == 0) {
                t = Math_Pow(t, 2.25f);
            } else {
                t = 1.0f - t;
            }
            switch (gShakeFlags & 0xF) {
            case 0:
                gShakeOffset.x = gShakeAmp * t;
                break;
            case 1:
                gShakeOffset.y = gShakeAmp * t;
                break;
            }
            gShakeTimer--;
            return 1;
        }
        if (gShakeOn != 0) {
            if ((gShakeFlags & 0x80) == 0) {
                switch (gShakeFlags & 0xF) {
                case 0:
                    gShakeOffset.x = 0;
                    break;
                case 1:
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
