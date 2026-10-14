#include "common.h"
#include "window.h"
#include "lobby_state.h"
#include "sprite.h"
#include "em.h"
#include "m2c_macros.h"

/* prototypes */
void Hud_FadeStart(s32 mode, s16 x, s16 y);
/* end of prototypes */
void Gallery_MarkAllTaken(void);

#include "lobby_menu.h"
#include "lobby_globals.h"
#include "player.h"
#include "lobby_vobj.h"
#include "save.h"
#include "lobby/unk_70E2A0.h"   /* the texts this file uses (tools/content_gen.py) */
s16 VObj_GetPoints(VObj *o, u8 i);

extern u8 gMenuWork[];
/* the calls cast to s8: this unit's code takes the value as s8 */
s32 SavePouch_Space(u16 item);
s16 SavePouch_CountFree(void);
void Gallery_MarkFullStacks(void);
u8 Gallery_IsRepeatPrize(u8 n);
s32 SavePouch_Room(u16 item);
u8 Gallery_IsPrizeChosen(u8 k);
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Font_SetSpacing(u8 a, u8 b);
extern u8 gSkillPreview[];
extern u16 *gHrGateQuests[];
extern s16 gGalleryCourseFees[];
void Save_AddMoney(s32 n);
void LobbySe_Play(s32 se);
void Equip_Refresh(SaveData *save);
void Player_CalcSkills(Player *p);
void SkillList_Build(Player *p, Player *q);
u8 SkillList_PageCount(void);
u8 Village_FacilityLevel(s32 k);
s32 Save_TestEventFlag(s32 flag);
s32 Save_TestListBit(u16 id);
u8 SavePouch_Add(u16 item, s16 n);
void *memset(void *, s32, u32);

typedef struct GalTri {
    s16 x0, y0, x1, y1, x2, y2;
    u32 color;
} GalTri;
typedef struct GalDrawArg {
    u8 unk0[0x18];
    s32 size;
} GalDrawArg;
s32 Sprite_Draw(Sprite *s);
struct PrimTri;
s32 Prim_FillTri(struct PrimTri *t);
s32 EntryPicture_GetUV(s32 *uv0, s32 *uv1, u16 id);
u16 LobbyPad_Get(s32 a);
void Gfx_SetState63(s32);
s32 Gs_SetState(u8, u32);
s32 Gs_GetState(u8 reg);
void Text_SetSize(s32 size);
void Text_SetColor(s16 color);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Gallery_DrawPrizeWindow(s16 x, u8 flag);
void Gallery_DrawPrizeList(s16 x);
void Gallery_DrawPrizePage(s16 x);
void Gallery_RunGunDebug(void);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
void Gallery_DrawShotMarks(s8 n, s8 hit);
s32 sprintf(char *buf, char *fmt, ...);
void Menu_DrawRowBar(s16 x, s16 y, s16 w, u32 color, u8 sel, u8 kind);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
struct Frame;
void WindowFrame_Draw(struct Frame *, s16 *);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Gfx_SetBlend(s32 a, s32 b);
void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32);
s16 Lobby_DrawSprite155(s16, s16, s32);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
extern s8 gGalleryCourseShots[];
extern Frame gGalleryFrame;
extern s32 gGalleryPrizes[];
extern char *gGalleryCourseTexts[];
extern char *gMonsterNames[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;

/* the shooting gallery's draw callback (LobbyMenu_SetDraw): by PrizeMenu.step the
 * menus (1, 2, 8, 9: Gallery_DrawPrizeList, Gallery_DrawPrizePage, Gallery_DrawPrizeWindow), the time,
 * points and controls during a game (4), the result with the hits per target
 * kind (6; 7 adds the "next" button). The rows' loop is an if and a do-while
 * (a for loop sets the two field pointers before its first test); 227.0f is
 * a float local, converted at each call. */
void Gallery_DrawShooting(GalDrawArg *arg)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    char buf[0x20];
    Sprite spr;
    GalTri tri;
    u8 rows;
    u16 height;
    s32 row0;
    u16 pad;
    s32 st5F;
    s32 i;
    s32 level;
    s16 *sy;
    char *str;
    u16 ybase;
    s16 y;
    s32 st5E;
    s32 cnt;
    s32 *prize;
    s16 *ty;
    f32 nameX;
    s32 top;

    pad = LobbyPad_Get(0);
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    Text_SetSize(arg->size);
    Text_SetColor(0);
    Font_SetSpacing(0x12, 0x12);
    switch (m->step) {
    case 1:
        if (m->toPrizes == 1) {
            Gallery_DrawPrizeWindow(m->win.pos, 0);
            return;
        }
        if (m->prizeView == 0) {
            Gallery_DrawPrizeList(m->win.pos);
            return;
        }
        Gallery_DrawPrizePage(m->win.pos);
        return;
    case 2:
        if (m->prizeView == 1) {
            if (pad & 0x3000) {
                if (m->page > 0) {
                    m->cursor = Menu_WrapUpDown(m->cursor, 6);
                } else {
                    m->cursor = Menu_WrapUpDown(m->cursor, 7);
                }
            } else if (pad & 0xC00) {
                m->page = Menu_WrapLeftRight(m->page, 2);
                m->cursor = 0;
            } else if (pad & 0x200) {
                if (m->details == 0) {
                    m->details = 1;
                } else {
                    m->details = 0;
                }
                LobbySe_Play(0x39);
            }
            Gallery_DrawPrizePage(0x118);
            return;
        }
        Gallery_DrawPrizeList(0x118);
        return;
    case 4:
        Gallery_DrawShotMarks(gGalleryCourseShots[m->course], m->shotsLeft);
        sprintf(buf, TXT(0x00838660) /* the time left, with its label */, m->time);
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawRowBar(0x1E, 0x10, 0xA0, 0xFFB0B000, 2, 1);
        Text_DrawShadowRgb(0x1E, 0x10, 0xFF202020, -1, buf);
        sprintf(buf, TXT(0x00838670) /* the points, with their label */, m->points);
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawRowBar(0x1AE, 0x10, 0xB4, 0xFFB0B000, 2, 1);
        Text_DrawShadowRgb(0x1AE, 0x10, 0xFF202020, -1, buf);
        y = 0x150;
        WindowFrame_Draw(&gGalleryFrame, 0);
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x1B0, y);
        Text_PrintfColored(TXT(0x00838680) /* the name of the control for aiming */);
        Font_SetPos(0x1B0, y += 0x14);
        Text_PrintfColored(TXT(0x008386A0) /* what that control does (aiming the gun) */);
        Font_SetPos(0x1B0, y += 0x16);
        Text_PrintfColored(TXT(0x008386B0) /* the name of the control for shooting */);
        Font_SetPos(0x1B0, y + 0x14);
        Text_PrintfColored(TXT(0x008386D0) /* what that control does (firing) */);
        Gallery_RunGunDebug();
        return;
    case 6:
    case 7:
        level = m->course;
        if (level == 0) {
            ybase = 0x50;
            height = 0xC8;
            level = 1;
            rows = 1;
        } else {
            ybase = 0x28;
            height = 0x122;
            rows = 4;
        }
        Icon_Draw(0x6E, ybase, 0x1A4, height, -0x2223, 0);
        Font_SetColor(0x80101010);
        Font_SetSpacing(0x14, 0x14);
        top = ybase;
        Font_SetPos(0x104, top + 0x19);
        Text_PrintfColored(gGalleryCourseTexts[m->course]);
        Menu_DrawRowBar(0xF5, top + 0x14, 0xA0, 0xBDB59F6C, 2, 1);
        y = top + 0x23;
        Gfx_SetState63(1);
        Gfx_SetBlend(4, 5);
        Tex_Prepare(1, 0x147);
        Tex_Bind(0x147);
        spr.color = -1;
        row0 = level * 4 - 3;
        spr.w = 0x19;
        tri.color = 0xFF000000;
        spr.h = 0x19;
        st5F = Gs_GetState(0x5F);
        st5E = Gs_GetState(0x5E);
        Gs_SetState(0x5F, 7);
        Gs_SetState(0x5E, 1);
        nameX = 227.0f;
        cnt = rows;
        i = 0;
        if (i < cnt) {
            sy = &spr.y;
            ty = &tri.y0;
            do {
            Font_SetPos(nameX, y += 0x20);
            prize = &gGalleryPrizes[row0 + i];
            Text_PrintfColored("%-20sx%3d", gMonsterNames[*prize], m->kindHits[i]);
            spr.x = 0xC0;
            *sy = y;
            tri.x0 = spr.x;
            *ty = *sy;
            tri.x1 = 25.0f + spr.x;
            tri.y1 = *sy;
            tri.x2 = spr.x;
            tri.y2 = 25.0f + *sy;
            Prim_FillTri((struct PrimTri *)&tri);
            tri.x0 = 25.0f + spr.x;
            *ty = 25.0f + *sy;
            Prim_FillTri((struct PrimTri *)&tri);
            EntryPicture_GetUV((s32 *)&spr.u0, (s32 *)&spr.u1, *prize);
            Sprite_Draw(&spr);
            } while (++i < cnt);
        }
        Gs_SetState(0x5F, st5F);
        Gs_SetState(0x5E, st5E);
        cnt = height;
        Font_SetPos(0xC0, top + cnt - 0x40);
        str = TXT(0x008386F0) /* the label of the points won */;
        if (m->points > 0) {
            Text_PrintfColored("%-20s      P", str);
        } else {
            Text_PrintfColored("                       0 P", str);
        }
        if (m->step == 7) {
            Lobby_DrawSprite155(0x19C, top + cnt - 0x23, 0);
            Font_SetPos(0x1BD, top + cnt - 0x23);
            Text_PrintfColored(TXT(0x00838730) /* the label of the next button */);
            return;
        }
        break;
    case 8:
        Gallery_DrawPrizeWindow(0x118, 0);
        return;
    case 9:
        Gallery_DrawPrizeWindow(0x118, 1);
        sprintf(buf, TXT(0x00838740) /* the question whether to quit */);
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, buf);
        Font_SetPos(0x1A2, 0x192);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

s32 Gallery_AddObj(VObj *v, u8 mode)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    s32 i;

    if (mode == 0) {
        m->targets = v;
        return 1;
    }
    for (i = 0; i < 10; i++) {
        if (m->shots[i] == NULL) {
            m->shots[i] = v;
            return 1;
        }
    }
    return 0;
}

s32 Gallery_CountShots(s32 arg0)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    s32 i;
    s32 ret;

    ret = 0;
    if (!(arg0 & 0xFF)) {
        return 1;
    }
    i = 0;
    do {
        if (m->shots[i] != NULL) {
            ret += 1;
        }
        i += 1;
    } while (i < 0xA);
    return ret;
}

void Gallery_ScoreTarget(void *g, u8 i)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;

    m->points += VObj_GetPoints(g, i);
    Hud_FadeStart(1, 0x1FE, 0x46);
}

void Gallery_ClearShots(void)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;

    memset(m->shots, 0, sizeof(m->shots));
}

typedef struct GVec {
    f32 x;
    f32 y;
    f32 z;
} GVec;
typedef struct GMtx {
    f32 m[16];
} GMtx;
s32 Gallery_CountShots(s32 arg0);
void VObj_GetCapsule(f32 *out, f32 *a, VObj *o, f32 f);
s32 Seg_ClipYRange(void *arg0, f32 lo, f32 hi, void *arg1, f32 *arg2, void *arg3);
void VObj_SetHit(VObj *o);
void VFx_SpawnRing(struct Vec3f *hit, struct Vec3f *dir);
void LobbySe_Play(s32 se);
u8 VObj_GetPartSizeKind(VObj *o, u8 i);
f32 VObj_GetPartAngle(VObj *o, u8 i);
f32 Math_Cos(f32);
void VObj_GetPartPos(struct Vec3f *out, VObj *o, u8 i);
u16 func_006B9040(VObj *o, u8 i);
u8 Capsule_HitsRect(f32 *ray, GVec *pos, s32 a, f32 c, f32 r); /* unit's own declaration: with the definition's u8 (Capsule *cap, f32 *c, u16 ang, f32 h, f32 w) (sub_main/unk_3BE060.c), Gallery_UpdateShots 3 of 228 words differ */
u8 VObj_GetFlag(VObj *o, u8 i);
s32 VObj_IsFinished(VObj *o);
s32 VObj_TickTimer(VObj *o, u8 i);
void VObj_SetFlag(VObj *o, u8 i);
void Gallery_ScoreTarget(void *g, u8 i);
void VNumber_Spawn(struct Vec3f *pos, s16 n);
void VObj_SpawnGroup(struct Vec3f *pos);
u16 VObj_GetPartSe(VObj *o, u8 i);
void Gallery_PlayHitSe(u16 id);
void VObj_SpawnList(struct Vec3f *pos);
void func_006B91A0(VObj *o, u8 i);
void Gallery_ClearShots(void);
extern GVec gGalleryWalls[];
extern f32 gGalleryWallYRange[];
extern f32 gVObjHeights[];

/* the gallery's shots: each shot in flight (tgt[i]) as a ray; a hit on
 * one of the 4 walls (gGalleryWalls, gGalleryWallYRange) ends it (sound 0x37); a hit
 * on one of the 15 targets of the group counts it (its kind's count, the
 * points, the bonus kind cfg[4]) or knocks it (sound 0x38).
 * The 0x40-byte buffer goes to VObj_GetCapsule as a pointer: passed as an
 * int (`(s32)mtx`), its address was kept in a saved register. */
void Gallery_UpdateShots(void)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    GVec at;
    GVec p;
    GVec dir;
    GVec hit;
    GVec pos;
    f32 ray[8];
    f32 mtx[16];
    s32 i;
    s32 j;
    s32 k;
    f32 c;
    f32 r;
    u8 lv;
    u8 *h;

    m->shotsLive = Gallery_CountShots(1);
    for (i = 0; i < 10; i++) {
        if (m->shots[i] == NULL) {
            continue;
        }
        VObj_GetCapsule(ray, mtx, m->shots[i], 3.0f);
        dir.x = 93.97f;
        dir.y = 0.0f;
        dir.z = -34.2f;
        for (j = 0; j < 4; j++) {
            p.x = gGalleryWalls[j].x;
            p.y = gGalleryWalls[j].y;
            p.z = gGalleryWalls[j].z;
            if (Seg_ClipYRange(ray, gGalleryWallYRange[j], 0.0f, &p, (f32 *) &dir, &hit)) {
                VObj_SetHit(m->shots[i]);
                VFx_SpawnRing((struct Vec3f *)&hit, (struct Vec3f *)&dir);
                LobbySe_Play(0x37);
                break;
            }
        }
        if (m->targets == NULL || ((s32 *)m->targets)[6] == 0) {
            return;
        }
        for (k = 0; k < 15; k++) {
            r = 0.5f * gVObjHeights[VObj_GetPartSizeKind(m->targets, k)];
            c = r * Math_Cos(VObj_GetPartAngle(m->targets, k));
            VObj_GetPartPos((struct Vec3f *)&pos, m->targets, k);
            if (Capsule_HitsRect(ray, &pos, func_006B9040(m->targets, k), c, r) && *(h = &m->slots[k]) > 0
                && VObj_GetFlag(m->targets, k) == 0 && VObj_IsFinished(m->targets) == 0) {
                at = pos;
                VObj_SetHit(m->shots[i]);
                if (VObj_TickTimer(m->targets, k)) {
                    VObj_SetFlag(m->targets, k);
                    Gallery_ScoreTarget(m->targets, k);
                    if (m->course == 0) {
                        lv = 1;
                    } else {
                        lv = m->course;
                    }
                    m->kindHits[(u8)(*h - (lv * 4 - 3))]++;
                    VNumber_Spawn((struct Vec3f *)&at, VObj_GetPoints(m->targets, k));
                    if (*h == m->cfg->kindB) {
                        m->hitsB++;
                    }
                    VObj_SpawnGroup((struct Vec3f *)&at);
                    LobbySe_Play(0x1A);
                    Gallery_PlayHitSe(VObj_GetPartSe(m->targets, k));
                } else {
                    VObj_SpawnList((struct Vec3f *)&at);
                    func_006B91A0(m->targets, k);
                    LobbySe_Play(0x38);
                }
            }
        }
    }
    Gallery_ClearShots();
}

void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32);

/* a row of shot marks: n of them, 20 to a line, the first "hit" of them lit */
void Gallery_DrawShotMarks(s8 n, s8 hit)
{
    Sprite spr;
    u8 i;
    u8 col = 0;
    s16 y = 0x32;
    s16 x;

    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    for (i = 0; i < n; i++) {
        if (col < 0x14) {
            x = col * 0x14 + 0x1E;
            col++;
        } else {
            y += 0x14;
            col = 1;
            x = 0x1E;
        }
        if (i < hit) {
            spr.u0 = 0x89;
            spr.v0 = 0xA4;
            spr.u1 = 0x97;
            spr.v1 = 0xBC;
        } else {
            spr.u0 = 0x95;
            spr.v0 = 0xA4;
            spr.u1 = 0xA3;
            spr.v1 = 0xBA;
        }
        spr.color = -1;
        spr.w = 0x14;
        spr.h = 0x14;
        spr.y = y;
        spr.x = x;
        Sprite_Draw(&spr);
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_70E2A0", D_00838788);

extern u8 D_00838788[];
extern s8 gGalleryCourseShots[];
extern s8 gGalleryCourseTimes[];
extern s16 gGalleryCourseBest[];
extern u8 gGalleryCourseLevels[];
extern char *gPriceMarks[];
extern char *gCoursePromptTexts[];
extern char *gGalleryCourseTexts[];
void Gallery_DrawLabelBar(s32 arg0);
void Gallery_DrawCourseCard(s16 x, s16 y, s16 time, s8 shots, s16 best, u8 level);
s32 Gallery_IsPouchFull(void);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 color);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);

/* the prize list of the shooting gallery: each prize, its points (or free),
 * the cursor; the round card below. Level 4 (the file is at 3): at 3, xs is re-extended
 * after the loop. */
#pragma optimization_level 4
void Gallery_DrawPrizeList(s16 x)
{
    s16 k;
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    s16 d;
    s16 y;
    s16 i;
    s16 *pts;
    char **name;
    s16 xs;
    s16 p;
    s8 sel;


    Gallery_DrawLabelBar((s32)D_00838788);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    y = 0x32;
    Icon_Draw(x, y, 0x15E, 0x132, -0x2223, 0);
    Font_SetSpacing(0x14, 0x14);
    xs = x + 0;
    y += 0x14;
    Font_SetPos(xs + 0x18, y);
    for (i = 0, pts = gGalleryCourseFees, name = gGalleryCourseTexts; i < 6; i++) {
        k = i;
        if (k == m->course) {
            d = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(xs + 0x28, y + 0xE, 0x112, 8, 0x99106060, 0x6F);
            Menu_DrawCursorIcon(xs - d, y - 2, 0x18, 0xFF20C0C0);
        }
        p = *pts;
        if (p > 0) {
            if (m->tooDear[k] > 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Text_PrintfColored("%-21s %6d%s", *name, p, gPriceMarks[0]);
        } else {
            Font_SetColor(0x80101010);
            Text_PrintfColored("%-24s %s", *name, TXT(0x008387B0) /* the price shown when it costs nothing */);
        }
        Font_SetPos(xs + 0x18, y += 0x16);
        pts++;
        name++;
    }
    sel = m->course;
    Gallery_DrawCourseCard(xs + 0x18, 0xF1, gGalleryCourseTimes[sel], gGalleryCourseShots[sel], gGalleryCourseBest[sel], gGalleryCourseLevels[sel]);
    Menu_DrawDashLine(xs + 0x18, 0xE6, 0x12C, 0xFF000000);
    if (Gallery_IsPouchFull() == 1) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gCoursePromptTexts[0]);
        m->pouchFull = 1;
    }
}
#pragma optimization_level reset

void Gallery_DrawLabelBar(s32 label)
{
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x12E, 0x12, 0xA0, -0x4F5000, (char *)label, 2, 1);
}

void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
s16 Lobby_DrawSprite155(s16, s16, s32);
extern char *D_0080C174;

/* a shooting game's card: its time limit and shots, level and best score */
void Gallery_DrawCourseCard(s16 x, s16 y, s16 time, s8 shots, s16 best, u8 level)
{
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    Font_SetPos(x + 0x12, y);
    Text_PrintfColored(TXT(0x008387C0) /* the time limit and the number of shots */, time, shots);
    Font_SetPos(x + 0x12, y += 0x16);
    Text_PrintfColored(TXT(0x008387E0) /* the difficulty and the best score */, level, best);
    Font_SetPos(x + 0xD2, y + 0x16);
    Font_SetPos(x + 0xDC, y + 0x32);
    Text_PrintfColored(D_0080C174);
    Lobby_DrawSprite155(x + 0xBE, y + 0x2D, 3);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_70E2A0", D_008387F8);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_70E2A0", D_00838808);

extern char D_008387F8[];
extern char D_00838808[];
extern char *D_0080D19C;
extern char *gItemNames[];
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);

/* the gallery's prize page: the 7 prizes of the page (item, points), the
 * cursor; the chosen prize's description below */
void Gallery_DrawPrizePage(s16 x)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    s16 xs;
    s16 y;
    s16 d;
    u8 first;
    u8 last;
    u8 i;

    Gallery_DrawLabelBar((s32)D_00838788);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    y = 0x32;
    Icon_Draw(x, y, 0x15E, 0x132, -0x2223, 0);
    xs = x + 0;
    Menu_DrawPageNum(xs + 0x109, 0x37, m->page, 2, 1);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);

    Font_SetPos(xs + 0x14, y += 0x14);
    Text_PrintfColored(D_008387F8);
    Font_SetPos(x, y += 0x16);
    first = m->page * 7;
    last = first + 6;
    if (last >= 0xD) {
        last = 0xC;
    }
    for (i = first; i <= last; i++) {
        if (i % 7 == m->cursor) {
            d = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(xs + 0x28, y + 0xE, 0x112, 8, 0x99106060, 0x6F);
            Menu_DrawCursorIcon(xs - d, y - 2, 0x18, 0xFF20C0C0);
        }
        if (m->full[i] <= 0) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Item_DrawIcon(xs + 0x14, y, 0x14, m->list[i].item);
        Font_SetPos(xs + 0x32, y);
        Text_PrintfColored(D_00838808, gItemNames[m->list[i].item], m->list[i].points);
        y += 0x16;
    }
    Font_SetColor(0x80101010);
    ItemInfo_Draw(x + 8, 0xFC, 0x14A, m->details, m->list[(u8)(m->cursor + m->page * 7)].item, 0, -1, 0x80101010);
    Menu_DrawDashLine(xs + 0x14, 0xFA, 0x12C, 0xFF000000);
    if (m->pouchFull == 1) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080D19C);
    }
}

extern char *D_0080D194;
extern Frame gGalleryPrizeFrame;
void WindowFrame_Draw(struct Frame *, s16 *);

/* the prize page with its header: the gallery's name (gGalleryCourseTexts[unk2])
 * and the points earned, then as Gallery_DrawPrizePage, with the count of the
 * chosen prize already taken; the message only when flag is 0 */
void Gallery_DrawPrizeWindow(s16 x, u8 flag)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    s16 xs;
    s16 y;
    s16 d;
    u8 first;
    u8 last;
    u8 i;
    u8 sel;
    u16 have;

    WindowFrame_Draw(&gGalleryPrizeFrame, 0);
    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(0x46, 0x46);
    Text_PrintfColored(gGalleryCourseTexts[m->course]);
    Font_SetPos(0x28, 0x5A);
    Text_PrintfColored(TXT(0x00838820) /* the points won, with their label and unit */, m->points);
    Gallery_DrawLabelBar((s32)D_00838788);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    y = 0x32;
    Icon_Draw(x, y, 0x15E, 0x132, -0x2223, 0);
    xs = x + 0;
    Menu_DrawPageNum(xs + 0x109, 0x37, m->page, 2, 1);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);

    Font_SetPos(xs + 0x14, y += 0x14);
    Text_PrintfColored(D_008387F8);
    Font_SetPos(x, y += 0x16);
    first = m->page * 7;
    last = first + 6;
    if (last >= 0xD) {
        last = 0xC;
    }
    for (i = first; i <= last; i++) {
        if (i % 7 == m->cursor) {
            d = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(xs + 0x28, y + 0xE, 0x112, 8, 0x99106060, 0x6F);
            Menu_DrawCursorIcon(xs - d, y - 2, 0x18, 0xFF20C0C0);
        }
        if (m->full[i] <= 0) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Item_DrawIcon(xs + 0x14, y, 0x14, m->list[i].item);
        Font_SetPos(xs + 0x32, y);
        Text_PrintfColored(D_00838808, gItemNames[m->list[i].item], m->list[i].points);
        y += 0x16;
    }
    Font_SetColor(0x80101010);
    have = 0;
    sel = m->cursor + m->page * 7;
    for (i = 0; i < m->count; i++) {
        if (m->items[i] == ((PrizeEntry *)((u8 *)m->list + sel * 4))->item) {
            have++;
        }
    }
    ItemInfo_Draw(x + 8, 0xFC, 0x14A, m->details, m->list[sel].item, have, -1, 0x80101010);
    Menu_DrawDashLine(xs + 0x14, 0xFA, 0x12C, 0xFF000000);
    if (m->checked == 1 && flag == 0) {
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080D194);
    }
}

s32 Gallery_InMenuOrClose(void)
{
    if (gMenuWork[0] == 1 || gMenuWork[0] == 10) {
        return 1;
    }
    return 0;
}

s32 Gallery_InResults(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;

    if (c->step == 7 && c->skipped == 1) {
        return 1;
    }
    return 0;
}

void Gallery_MarkTooDear(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    u8 i;

    for (i = 0; i < 13; i++) {
        if (c->list[i].points > c->points) {
            if (c->full[i] <= 0) {
                c->full[i] = 1;
            }
        }
    }
}

void Gallery_MarkFullPrizes(void)
{
    PrizeMenu *s = (PrizeMenu *)gMenuWork;
    u8 i;
    u8 n;
    u8 k;

    n = 0;
    for (i = 0; i < 13; i++) {
        for (k = 0; k < s->count; k++) {
            if (s->items[k] == s->list[i].item) {
                n++;
            }
        }
        if ((s8)SavePouch_Space(s->list[i].item) <= n) {
            if (s->full[i] <= 0) {
                s->full[i] = 1;
            }
        }
        n = 0;
    }
}

/* when the chosen prizes that fill their stack (each item counted once) are
 * at least the pouch's free slots, marks the full stacks */
void Gallery_CheckPouchFull(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    s16 n;
    u8 i;
    s16 room;

    n = 0;
    for (i = 0; i < c->count; i++) {
        room = SavePouch_Room(c->items[i]);
        if (room == gItemData[c->items[i]].max && Gallery_IsRepeatPrize(i) == 0) {
            n++;
        }
    }
    if (SavePouch_CountFree() <= n) {
        Gallery_MarkFullStacks();
    }
}

/* 1 when the chosen prizes that fill their stack are at least the pouch's
 * free slots (the test of Gallery_CheckPouchFull) */
s32 Gallery_IsPouchFull(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    s16 n;
    u8 i;
    s16 room;

    n = 0;
    for (i = 0; i < c->count; i++) {
        room = SavePouch_Room(c->items[i]);
        if (room == gItemData[c->items[i]].max && Gallery_IsRepeatPrize(i) == 0) {
            n++;
        }
    }
    return n >= SavePouch_CountFree();
}

/* 1 when chosen prize n repeats an earlier one */
u8 Gallery_IsRepeatPrize(u8 n)
{
    PrizeMenu *s = (PrizeMenu *)gMenuWork;
    u8 r = 0;
    u8 i;

    for (i = 0; i < n; i++) {
        if (s->items[i] == s->items[n]) {
            r = 1;
        }
    }
    return r;
}

u8 Gallery_IsPrizeChosen(u8 k)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    u8 r = 0;
    u8 i = 0;

    if (0 < c->count) {
        do {
            /* c->list[k].item scales k before loading c->list */
            if (c->items[i] == *(u16 *)((u8 *)c->list + k * 4)) {
                r = 1;
            }
            i++;
        } while (i < c->count);
    }
    return r;
}

/* the prize under the cursor, when not taken yet, into the list at +0x5E
 * (a full list of 40 marks every prize taken) */
void Gallery_ChoosePrize(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    u16 k;

    k = c->cursor + c->page * 7;
    if (c->full[k] <= 0) {
        c->items[c->count] = c->list[k].item;
        c->count++;
        if (c->count == 40) {
            Gallery_MarkAllTaken();
        }
    }
}

void Gallery_MarkTooDearCourses(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    u8 i;
    s16 *t;

    for (i = 0, t = gGalleryCourseFees; i < 6; i++, t++) {
        if (*t > gSave.money) {
            c->tooDear[i] = 1;
        }
    }
}

void Gallery_TakePrize(void)
{
    PrizeMenu *s = (PrizeMenu *)gMenuWork;
    u8 i;

    if (s->played != 0) {
        for (i = 0; i < s->count; i++) {
            SavePouch_Add(s->items[i], 1);
        }
        Save_AddMoney(-gGalleryCourseFees[s->playedCourse]);
    }
}

void Gallery_MarkFullStacks(void)
{
    PrizeMenu *s = (PrizeMenu *)gMenuWork;
    u8 i;
    u8 room;

    for (i = 0; i < 13; i++) {
        room = SavePouch_Room(s->list[i].item);
        if (room == gItemData[s->list[i].item].max && (u8)Gallery_IsPrizeChosen(i) == 0) {
            s->full[i] = 1;
        }
    }
    s->checked = 1;
}

/* marks the 13 prizes taken */
void Gallery_MarkAllTaken(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    u8 i;

    for (i = 0; i < 13; i++) {
        c->full[i] = 1;
    }
    c->allTaken = 1;
}

void Gallery_PlayHitSe(u16 id)
{
    switch (id) {
    case 0x10:
    case 0x1B:
    case 0x1C:
    case 0x1E:
    case 0x1F:
    case 0xD:
        LobbySe_Play(0x1D);
        break;
    case 0x6:
        LobbySe_Play(0x1E);
        break;
    case 0x14:
        LobbySe_Play(0x1F);
        break;
    case 0x11:
    case 0x12:
    case 0xE:
    case 0x16:
        LobbySe_Play(0x20);
        break;
    case 0xF:
        LobbySe_Play(0x21);
        break;
    case 0x1:
        LobbySe_Play(0x22);
        break;
    case 0x21:
        LobbySe_Play(0x23);
        break;
    case 0x33:
        LobbySe_Play(0x24);
        break;
    case 0x34:
        LobbySe_Play(0x25);
        break;
    case 0x30:
        LobbySe_Play(0x26);
        break;
    case 0x43:
        LobbySe_Play(0x27);
        break;
    case 0x44:
        LobbySe_Play(0x28);
        break;
    }
}

/* unk_70E2A0's .data (0x7EC5C8-0x7EC5F0) */
s16 gGalleryCourseBest[6] = {
#include "lobby/007EC5C8.inc"
};
u8 gGalleryCourseLevels[8] = {
#include "lobby/007EC5D8.inc"
};
u32 gGalleryGunPos[2] = {
#include "lobby/007EC5E0.inc"
};
u32 D_007EC5E8[2] = {
#include "lobby/007EC5E8.inc"
};
