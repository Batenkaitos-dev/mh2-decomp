#include "common.h"
#include "sprite.h"
#include "m2c_macros.h"

#include "lobby_menu.h"
#include "lobby_globals.h"
#include "pad.h"
#include "item.h"
#include "save.h"
#include "scene.h"
#include "box_menu.h"
#include "player.h"

extern u8 gSkillPreview[];
extern u16 *gHrGateQuests[];
extern s16 gGalleryCourseFees[];
extern u8 gMenuWork[];
void Save_AddMoney(s32 n);
void LobbySe_Play(s32 se);
void Equip_Refresh(SaveData *save);
void Player_CalcSkills(struct Player *pl);
void SkillList_Build(struct Player *p, struct Player *q);
u8 SkillList_PageCount(void);
u8 Village_FacilityLevel(s32 k);
s32 Save_TestEventFlag(s32 flag);
s32 Save_TestListBit(u16 id);
u8 SavePouch_Add(u16 item, s16 n);
u8 Equip_PreviewSkills(Player *player, EquipItem *e);
u8 EquipSet_RemoveSlot(u16 slot, s32 check);
void Town_ReloadLocalPlayer(void);
void Sound_PlaySeFlat(u32 a, u32 b, s32 c);
void CamFocus_Start(struct Em *);
s32 StatusPage_HasSymbolColor(void);
void Shake_Start(u8 id, u8 flags, f32 amp);
void Icon_DrawRotatedAt(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s32 i, s16 cx, s16 cy);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, s32 color, s32 f); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, s16 h, u32 color, s32 i) (lobby/unk_790EC0.c), Menu_DrawArrows 73 of 82 words, Menu_DrawArrowsRev 73 of 82 words, Menu_DrawArrowsSized 39 of 83 words differ */
s32 Text_DisplayLength(char *s);

typedef struct Pos16 {
    s16 x;
    s16 y;
} Pos16;
extern Pos16 gLabelIconOffsets[];
extern char *gLabelIconsPos[];

void Menu_DrawLabelsPos(s16 x, s16 y, s32 a, s16 b, s16 c, s16 *list)
{
    s16 i;

    y += 0x14;
    for (i = 0; i < 12; i++) {
        if (list[i] == 0) {
            break;
        }
        if (list[i] > 0) {
            Icon_DrawRotatedAt(x + gLabelIconOffsets[i].x, y, gLabelIconOffsets[i].y, 0x1C, (u32)((list[i] == 99) ? gLabelIconsPos[0] : gLabelIconsPos[list[i]]), a, i % 12 + 15, b, c);
        }
        y += 0x1C;
    }
}

extern char *gLabelIconsNeg[];

void Menu_DrawLabelsNeg(s16 x, s16 y, s32 a, s16 b, s16 c, s16 *list)
{
    s16 i;

    y += 0x14;
    for (i = 0; i < 12; i++) {
        if (list[i] == 0) {
            break;
        }
        if (list[i] < 0) {
            Icon_DrawRotatedAt(x + gLabelIconOffsets[i].x, y, gLabelIconOffsets[i].y, 0x1C, (u32)((list[i] * -1 == 99) ? gLabelIconsNeg[0] : gLabelIconsNeg[list[i] * -1]), a, i % 12 + 15, b, c);
        }
        y += 0x1C;
    }
}

extern s32 *gDecoFrameColors[];

void Menu_DrawDecoFrame(s16 x0, s16 y0, s16 w0, s16 h, s8 kind, s8 b)
{
    s32 w;
    s32 x;
    s32 c2;
    s32 x2;
    s16 cx;
    s32 k;
    s16 cy;
    s32 a;
    s32 c1;
    s32 yy;
    s32 py;
    s32 y;
    s32 *p;
    s32 ym;
    s32 yp;

    w = w0;
    x = x0;
    cx = x + w / 2;
    y = y0;
    cy = y + h / 2;
    switch (kind) {
    case 2:
        c1 = 0xFF445C79;
        a = 0;
        c2 = 0xFFB57510;
        break;
    case 3:
        a = 0;
        p = gDecoFrameColors[b];
        c1 = p[0];
        c2 = p[1];
        break;
    case 5:
    case 4:
        c1 = 0xFF7B400E;
        a = 0;
        c2 = 0x80B57510;
        break;
    }
    x2 = (s16)(x + 10);
    Icon_DrawRotatedAt(x2 + 0x10, y + 3, w - 0x20, h, (u32)((char *)c1), a, 3, cx, cy);
    Icon_DrawRotatedAt(x + 10, y + 3, 0x10, h, (u32)((char *)c1), a, 7, cx, cy);
    Icon_DrawRotatedAt(x2 + w - 0x10, y + 3, 0x10, h, (u32)((char *)c1), a, 0xB, cx, cy);
    if (kind == 3) {
        yy = cy;
        py = yy - 0x30;
        k = b * 2;
        Icon_DrawRotatedAt(cx - 0x38, py, 0x38, 0x70, (u32)((char *)c2), a, k + 0x7A, cx, cy);
        Icon_DrawRotatedAt(cx, py, 0x38, 0x70, (u32)((char *)c2), a, k + 0x7B, cx, cy);
    } else {
        yy = cy;
        Icon_DrawRotatedAt(cx - 0x38, yy - 0x38, 0x70, 0x70, (u32)((char *)c2), a, 0x1F, cx, cy);
    }
    ym = yy - 0x78;
    Icon_DrawRotatedAt(x + 0x14, ym, 0x100, 0x20, (u32)((char *)c2), a, 0x1B, cx, cy);
    yp = yy + 0x78;
    Icon_DrawRotatedAt(x + 0x14, yp, 0x100, 0x20, (u32)((char *)c2), a, 0x1B, cx, cy);
    Icon_DrawRotatedAt(x + 0x114, ym, 0x2E, 0x20, (u32)((char *)c2), a, 0x1C, cx, cy);
    Icon_DrawRotatedAt(x + 0x114, yp, 0x2E, 0x20, (u32)((char *)c2), a, 0x1C, cx, cy);
}

extern s16 gQuestMapGroups[];

void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind)
{
    s16 cx = x + w / 2;
    s16 cy = y + h / 2;
    s32 a;
    s16 x2;

    Icon_Draw(x, y, w, h, 0xFFA0A0A0, 0);
    switch (kind) {
    case 2:
        a = 0;
        break;
    case 3:
        a = 0;
        break;
    case 5:
    case 4:
        a = 0;
        break;
    }
    x2 = x + 10;
    Menu_DrawLabelsPos(x2 - 50, y + 3, a, cx, cy, gQuestMapGroups);
}

typedef struct FrameRec {
    s16 v;
    s16 pad[4];
} FrameRec;
typedef struct IconTex {
    s32 tex;
    UV uv0;
    UV uv1;
} IconTex;
void TexRect_DrawRotated2(TexRect *r, s16 cx, s16 cy, s16 cx2, s16 cy2, u32 angle1, u32 angle2);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
extern IconTex gIconSprites[];
extern s32 gFrameStripColors[];
extern FrameRec gStripTurnOffsets[];
extern FrameRec D_007FE932[];
extern FrameRec D_007FE934[];
extern FrameRec D_007FE936[];
extern FrameRec D_007FE938[];

void Menu_DrawFrameStrip(s16 x, s16 y, s16 w, s16 h, u8 kind, s8 sel);
/* Level 4 (the pragma pair). x + w / 2 is written out at each use (a w / 2
 * local made MWCC spill an extended copy of x), (y + h / 2) - 0x30 at both
 * stamp calls (a py local numbered h's extension after the D_007FE934 load),
 * and k = b; k *= 2 (a permuter find: b * 2 in one statement swaps s3/s4).
 * FAKEMATCH: the stamp colour col is set twice, the second time in the labels
 * block. With two definitions MWCC's copy propagation keeps col a value of its
 * own; a later pass drops the repeated one, and the colouring spills col and
 * rematerialises it as `lui t0` at each of the two stamp calls. That makes the
 * second call's t0 the move that ends col in the first scheduling pass, so it
 * goes second as in the game; with the constant written at both calls (or col
 * set once, which MWCC propagates) it goes last. The else call takes the
 * constant itself: col there moves its t0 too
 * (docs/notes/nm/Menu_DrawQuestSheet.md). */
#pragma optimization_level 4
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b)
{
    s32 cx;
    s32 cy;
    s16 tx;
    s16 ty;
    u16 angle;
    s32 k;
    s16 j;
    u32 col;

    Icon_Draw(x, y, w, h, 0xFFA0A0A0, 0);
    j = (sel + 2) % 12;
    col = 0x40000000;
    if (offs != NULL) {
        col = 0x40000000; /* FAKEMATCH: set twice (see above) */
        Menu_DrawLabelsPos(x - 0x40, y, 0, gStripTurnOffsets[j].v, D_007FE932[j].v, offs);
    }
    x += gStripTurnOffsets[sel].v;
    y += D_007FE932[sel].v;
    cx = x + w / 2;
    tx = cx + D_007FE934[sel].v;
    cy = y + h / 2;
    ty = cy + D_007FE936[sel].v;
    angle = D_007FE938[sel].v;
    Icon_DrawRotatedAt(x, y, w, h, 0xFFE0E0E0, angle, 1 - (sel & 1), tx, ty);
    switch (kind) {
    default:
        Menu_DrawFrameStrip(x, y, w, h, kind, sel);
        break;
    case -1:
        break;
    }
    if (offs != NULL) {
        Menu_DrawLabelsNeg(x - 0x40, y, angle, tx, ty, offs);
    }
    if (b != -2) {
        if (b != -1) {
            k = b;
            k *= 2;
            Icon_DrawRotatedAt((x + w / 2) - 0x38, (y + h / 2) - 0x30, 0x38, 0x70, col, angle, k + 0x7A, tx, ty);
            Icon_DrawRotatedAt(x + w / 2, (y + h / 2) - 0x30, 0x38, 0x70, col, angle, k + 0x7B, tx, ty);
        } else {
            Icon_DrawRotatedAt(cx - 0x38, cy - 0x38, 0x70, 0x70, 0x40000000, angle, 0x1F, tx, ty);
        }
    }
}
#pragma optimization_level reset

/* Draws the 11 tiles of a frame strip (two rows, texture kind), turned with
 * the frame at sel. */
void Menu_DrawFrameStrip(s16 x, s16 y, s16 w, s16 h, u8 kind, s8 sel)
{
    s16 px;
    s16 py;
    s16 cx = x + w / 2;
    s16 cy = y + h / 2;
    s16 y0;
    TexRect r;
    s16 i;
    IconTex *e;
    u32 angle;
    s16 t;

    e = &gIconSprites[kind + 100];
    if (kind < 7) {
        r.color = gFrameStripColors[kind];
    }
    r.x = x + 0x1A;
    r.y = y + 0x28;
    y0 = r.y;
    r.h = 0x18;
    r.w = 0x18;
    r.uv0 = e->uv0;
    r.uv1 = e->uv1;
    r.x += gStripTurnOffsets[sel].v;
    r.y += D_007FE932[sel].v;
    cx += D_007FE934[sel].v;
    cy += D_007FE936[sel].v;
    angle = (u16)D_007FE938[sel].v;
    Tex_Prepare(1, e->tex);
    Tex_Bind(e->tex);
    for (i = 0; i < 11; i++) {
        r.y = y0;
        switch (kind) {
        case 0:
            t = r.uv0.u;
            r.uv0.u = r.uv1.u;
            r.uv1.u = t;
            break;
        case 1:
            r.w = 0x1E;
            if (i > 8) {
                return;
            }
            break;
        case 2:
            if (i & 1) {
                r.h = 0x18;
            } else {
                r.y = y0 + 8;
                r.h = 0x10;
            }
            break;
        case 3:
        case 5:
            t = r.uv0.v;
            r.uv0.v = r.uv1.v;
            r.uv1.v = t;
            break;
        }
        px = r.x + r.w / 2;
        py = r.y + r.h / 2;
        TexRect_DrawRotated2(&r, px, py, cx, cy, 0, angle);
        r.y = r.y + (h - 0x39);
        TexRect_DrawRotated2(&r, px, py, cx, cy, 0, angle);
        r.x += r.w;
    }
}

s32 abs(s32);
void Icon_DrawRotated(s16 x, s16 y, s16 w, s16 h, u32 colour, u16 flags, s32 spr); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 color, s32 angle, s32 i) (lobby/unk_790EC0.c), Menu_DrawStamp 32 of 134 words, QuestBoard_DrawStamp 36 of 137 words differ */

void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl)
{
    u16 angle;
    s16 size;
    s16 dx;
    s16 dy;
    f32 half;

    angle = seed % 0x4000 + 0xE001;
    x += (s16)(abs(seed % (w - 0xCC)) + 10);
    y += (s16)(abs((seed >> 3) % (h - 0xCC)) + 10);
    dx = 0;
    dy = 0;
    if (lvl > 3) {
        size = (lvl - 3) * 20;
    } else {
        size = 0;
        switch (lvl) {
        case 3:
            dx = -2;
            break;
        case 2:
            dy = 1;
            break;
        }
    }
    x += dx;
    y += dy;
    half = 0.5f * size;
    x = x - half;
    y = y - half;
    size += 0xA4;
    Icon_DrawRotated(x, y, size, size, ((angle % 8) * 16 + 0x80) << 24 | 0x50140A, angle, 0x23);
}

void QuestBoard_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl)
{
    u16 angle;
    s16 px;
    s16 size;
    s16 dx;
    s16 py;
    s16 dy;
    f32 half;

    angle = (seed >> 2) % 0x4000 + 0xE001;
    px = x + (abs((seed >> 3) % (w - 0x92)) + 10);
    py = y + (abs((seed >> 1) % (h - 0x92)) + 10);
    dx = 0;
    dy = 0;
    if (lvl > 3) {
        size = (lvl - 3) * 20;
    } else {
        size = 0;
        switch (lvl) {
        case 3:
            dx = -2;
            break;
        case 2:
            dy = 1;
            break;
        }
    }
    px += dx;
    py += dy;
    half = 0.5f * size;
    px -= half;
    py -= half;
    size += 0x60;
    Icon_DrawRotated(px, py, size, size, ((angle % 8) * 16 + 0x80) << 24 | 0xA1450, angle, 0x1F);
}

void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color)
{
    s16 x1;
    s16 y1;
    s16 w;
    s16 off;
    s16 x2;

   w = size + 8;
    y1 = y - (w - size) / 2;
    x1 = x - w;
    off = (gFrameTick & 0x10) ? 6 : 0;
    Icon_Draw(x1 - 4 - off, y1, w, w, color, 0x28);
    if (text != NULL) {
        x2 = x1 + w + size * Text_DisplayLength(text) / 2;
        Icon_Draw(x2 + 4 + off, y1, w, w, color, 0x29);
    }
}

void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color)
{
    s16 x1;
    s16 y1;
    s16 w;
    s16 off;
    s16 x2;

   w = size + 8;
    y1 = y - (w - size) / 2;
    x1 = x - w;
    off = (gFrameTick & 0x10) ? 6 : 0;
    Icon_Draw(x1 - 4 - off, y1, w, w, color, 0x29);
    if (text != NULL) {
        x2 = x1 + w + size * Text_DisplayLength(text) / 2;
        Icon_Draw(x2 + 4 + off, y1, w, w, color, 0x28);
    }
}

void Menu_DrawArrowsSized(s16 x, s16 y, s16 w, s16 size, char *text, s32 color)
{
    s16 x1;
    s16 y1;
    s16 off;
    s16 x2;

    y1 = y - (w - size) / 2;
    x1 = x - w;
    off = (gFrameTick & 0x10) ? 6 : 0;
    Icon_Draw(x1 - 4 - off, y1, w, w, color, 0x28);
    if (text != NULL) {
        x2 = x1 + w + size * Text_DisplayLength(text) / 2;
        Icon_Draw(x2 + 4 + off, y1, w, w, color, 0x29);
    }
}

void BoxMenu_InitWork(void)
{
    gBoxWork = &gBoxWorkMem;
    gBoxWork->steps = 0;
    gBoxWork->menuKind = 0;
    gBoxWork->player = &gPlayers[gScene.localPlayer];
}

s32 BoxMenu_Open(void)
{
    gBoxWork->steps = 0;
    gBoxWork->menuKind = 0;
    *(s32 *)gBoxWork->slots = 0;
    gBoxWork->pouch = 0;
    gBoxWork->yesNo = 0xFF;
    gBoxWork->help = 0;
    gBoxWork->blink = 0xFF;
    gBoxWork->skillPage = 0;
    gHelpPage = 5;
    gHelpShow = 0;
    gHelpLine = 0;
    gHelpColumn = 0;
    CamFocus_Start(0);
    gBoxWork->inputOn = 1;
    gBoxWork->setEntries = (StatusPage_HasSymbolColor() != 1) ? 4 : 5;
    Player_CalcSkills((struct Player *)gBoxWork->player);
    Shake_Start(2, 0, -200.0f);
    Sound_PlaySeFlat(0, 0x11, 0);
    return 1;
}

s16 Box_GetPageCount(SaveData *save);
s16 Box_WrapIndex(SaveData *save, s16 idx);
void BoxMenu_MoveCursor(u16 *cur, u16 pad);
void Equip_SetEquipped(EquipIdx *e);
u8 BoxMenu_CloseStep(u16 pad, u8 step);
void BoxMenu_UpdateSkillPreview(void);
u8 EquipSet_DropUnfit(EquipIdx *out, EquipIdx *set, s32 flags);
s32 ItemBox_SlotHasItem(u16 slot);
u16 BoxMenu_SortPrompt(u16 pad, u8 kind);
s32 EquipBox_TryRemove(u16 slot);
static s32 BoxMenu_CanSplit(void);
s32 BoxMenu_CanTakeSeveral(void);
void BoxMenu_InputAmount(u16 pad, u8 noLimit);
void Equip_SwapSlots(u16 a, u16 b);
s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
void Chat_SetColor(s16 color);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Shake_Update(u8 id);
void CamFocus_End(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
u16 BoxMenu_StoreFromPouch(u16 pad);
u16 BoxMenu_TakeToPouch(u16 pad);
u16 BoxMenu_EquipMenu(u16 pad);
u16 BoxMenu_ArrangeMenu(u16 pad);
u16 BoxMenu_SellMenu(u16 pad);
extern u16 gChatRecipients;

s32 BoxMenu_Update(u16 pad)
{
    u16 res;
    s32 b;

    gBoxWork->unk0 = 0;
    Chat_SetColor(-1);
    if (pad & 0x8000) {
        gBoxWork->inputOn ^= 1;
    }
    if (!(gBoxWork->inputOn & 1)) {
        pad = 0;
    }
    res = pad;
    switch (gBoxWork->state) {
    case 0:
        Chat_SetWindowMode(1, 2);
        if (Shake_Update(2) == 1) {
            pad &= 0x3C40;
        }
        b = pad;
        gHelpShow = 0;
        if (b & 0x40) {
            Shake_Start(2, 0x80, -200.0f);
            CamFocus_End();
            Sound_PlaySeFlat(0, 0x14, 0);
            gBoxWork->state = 2;
            break;
        }
        Menu_CursorUpDown(&gBoxWork->menu, pad, 5);
        if (!(b & 0x20)) {
            break;
        }
        pad = 0;
        gBoxWork->pick = 0;
        gBoxWork->kind = 0;
        *(s32 *)gBoxWork->slots = gBoxWork->pouch = 0;
        gBoxWork->help = 0;
        gBoxWork->levelSel = 0;
        gBoxWork->msg = -1;
        gBoxWork->prompt = 0;
        gBoxWork->cursorClass = 0;
        gBoxWork->set = 0;
        gBoxWork->part = 0;
        gBoxWork->state++;
        gBoxWork->step = 0;
        gHelpColumn = 0;
        Sound_PlaySeFlat(0, 0x13, 0);
        switch (gBoxWork->menu) {
        case 0:
            Shake_Start(1, 0, 180.0f);
        }
    case 1:
        Chat_SetWindowMode(2, 2);
        /* 0x5BB0AC reached as a named object (the chat state after
         * gChatRecipients): the original loads gBoxWork above this store */
        ((u8 *)&gChatRecipients)[0x14] = 1;
        switch (gBoxWork->menu) {
        case 0:
            res = BoxMenu_StoreFromPouch(pad);
            break;
        case 1:
            res = BoxMenu_TakeToPouch(pad);
            break;
        case 2:
            res = BoxMenu_EquipMenu(pad);
            break;
        case 3:
            res = BoxMenu_ArrangeMenu(pad);
            break;
        case 4:
            res = BoxMenu_SellMenu(pad);
            break;
        }
        if (res & 0x40) {
            Chat_SetColor(-1);
            gHelpShow = 0;
            gBoxWork->state = 0;
        }
        break;
    case 2:
        if (Shake_Update(2) == 0) {
            return 0;
        }
        break;
    }
    gBoxWork->unk0 = 1;
    return 1;
}

s16 Box_GetPageCount(SaveData *save);
s16 Box_WrapIndex(SaveData *save, s16 idx);

void BoxMenu_MoveCursor(u16 *cur, u16 pad)
{
    s16 idx;
    s16 page;
    u8 paged;
    s16 pos;
    s16 n;
    u8 col;
    u8 row;

    idx = Box_WrapIndex(&gSave, *cur);
    paged = 0;
    page = idx / 100;
    pos = idx % 100;
    n = Box_GetPageCount(&gSave);
    if (n > 1 && (pad & 0x14)) {
        paged = 1;
        if (pad & 0x10) {
            page++;
            if (page >= n) {
                page = 0;
            }
        } else if (page == 0) {
            page = n - 1;
        } else {
            page--;
        }
    }
    col = pos % 10;
    row = pos / 10;
    if (pad & 0x800) {
        if (col == 0) {
            col = 9;
        } else {
            col--;
        }
    }
    if (pad & 0x400) {
        if (col >= 9) {
            col = 0;
        } else {
            col++;
        }
    }
    if (pad & 0x2000) {
        if (row == 0) {
            row = 9;
        } else {
            row--;
        }
    }
    if (pad & 0x1000) {
        if (row >= 9) {
            row = 0;
        } else {
            row++;
        }
    }
    idx = col % 10 + page * 100 + row % 10 * 10;
    if (*cur != idx) {
        *cur = idx;
        if (paged == 0) {
            Sound_PlaySeFlat(0, 0x17, 0);
        } else {
            Sound_PlaySeFlat(0, 0x11, 0);
        }
    }
}

u8 YesNo_Toggle(u16 pad, u8 cur, u16 mask);
void Pouch_Compact(u8 box);
s32 ItemBox_Fits(u16 item);
void Pouch_MoveCursor(u8 *cur, u16 buttons);
void PouchMenu_DecoCursor(u16 item, u16 pad);
u32 ItemBox_Add(u16 item, u32 count);
s16 Box_GetCapacity(SaveData *save);

u16 BoxMenu_StoreFromPouch(u16 pad)
{
    SaveData *save = &gSave;
    u16 ng;
    s32 b;
    ItemBoxMenu *m;
    u8 *p;
    s32 r;
    ItemStack *e;
    u32 i;
    s16 n;
    s32 c;

    switch (gBoxWork->step) {
    case 0:
        if (Shake_Update(1) == 1) {
            gHelpShow = 0;
            pad &= 0x3C40;
        }
        gHelpLine = 0x38;
        p = &gBoxWork->prompt;
        if (*p != 0) {
            c = pad;
            if (c & 0x40) {
                pad = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
                p = &gBoxWork->yesNo;
                if (*p != 0) {
                    *p = 0xFF;
                    goto close;
                }
                *p = 1;
                break;
            } else if (!(c & 0x20)) {
                gBoxWork->yesNo = YesNo_Toggle(pad, gBoxWork->yesNo, 0xC00);
                break;
            }
            pad = 0;
            if (gBoxWork->yesNo == 0) {
                Pouch_Compact(1);
                Sound_PlaySeFlat(0, 0x13, 0);
                gBoxWork->pouch = gBoxWork->pouch % 10;
            } else {
                Sound_PlaySeFlat(0, 0x14, 0);
            }
            gBoxWork->yesNo = 0xFF;
        close:
            gBoxWork->prompt = 0;
        } else if (pad & 0x4000) {
            *p = 1;
            pad = 0;
            gBoxWork->yesNo = 0;
            gBoxWork->help = 0;
            Sound_PlaySeFlat(0, 0xF, 0);
            break;
        }
        if (pad & 0x40) {
            Shake_Start(1, 0x80, 180.0f);
            pad = 0;
            gBoxWork->step = 2;
            gHelpShow = 0;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        gHelpLine = 0;
        ng = 0;
        if (save->pouch[gBoxWork->pouch].item != 0) {
            if (ItemBox_Fits(save->pouch[gBoxWork->pouch].item) == 0) {
                ng = 1;
                gHelpLine = 2;
            }
        } else {
            ng = 1;
        }
        b = pad;
        if (b & 0x200) {
            p = &gBoxWork->help;
            if (*p != 0) {
                *p = 0;
            } else {
                *p = 1;
                gHelpColumn = 0;
            }
            Sound_PlaySeFlat(0, 9, 0);
        }
        Pouch_MoveCursor(&gBoxWork->pouch, pad);
        gBoxWork->pouchItem = save->pouch[gBoxWork->pouch].item;
        if (gBoxWork->help != 0) {
            PouchMenu_DecoCursor(gBoxWork->pouchItem, pad);
        }
        if (b & 0x20) {
            if (ng == 0) {
                r = ItemBox_Add(gBoxWork->pouchItem, save->pouch[gBoxWork->pouch].count);
                if (r == 0) {
                    save->pouch[gBoxWork->pouch].count = 0;
                    save->pouch[gBoxWork->pouch].item = 0;
                    gBoxWork->msg = 1;
                    Sound_PlaySeFlat(1, 0x88, 0);
                } else {
                    save->pouch[gBoxWork->pouch].count = r;
                    gBoxWork->msg = 2;
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
                m = gBoxWork;
                gHelpLine = (s8)m->msg;
                m->help = 0;
                gBoxWork->step++;
                gBoxWork->slots[0] = 0;
                n = Box_GetCapacity(save);
                for (i = 0, e = save->itemBox; i < n; i++, e++) {
                    if (e->item == gBoxWork->pouchItem) {
                        gBoxWork->slots[0] = i;
                        break;
                    }
                }
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
        }
        break;
    case 1:
        b = pad;
        BoxMenu_MoveCursor(&gBoxWork->slots[0], b & 0x14);
        m = gBoxWork;
        gHelpLine = (s8)m->msg;
        m->blink++;
        if (b & 0x20) {
            gBoxWork->step = 0;
            gBoxWork->help = 0;
            gBoxWork->blink = 0xFF;
            Sound_PlaySeFlat(0, 9, 0);
        }
        pad = 0;
        break;
    case 2:
        if (Shake_Update(1) == 0) {
            return 0x40;
        }
        gHelpShow = 0;
        pad = 0;
        break;
    }
    return pad;
}

s32 Item_IsSpecial(u16 item);
s32 SavePouch_Room(u16 item);

/* gBoxWork as the box-to-pouch screen reads it */


/* ItemBoxMenu seen by BoxMenu_TakeToPouch: the ItemBoxMenu form changes the code (MWCC reuses the load of gBoxWork) */
typedef struct TakeView {
    /* 0x00 */ u8 unk0[5];
    /* 0x05 */ u8 step;
    /* 0x06 */ u8 sub;
    /* 0x07 */ u8 unk7[0x15];
    /* 0x1C */ u16 slot;
    /* 0x1E */ u8 unk1E[4];
    /* 0x22 */ u8 pouch;
    /* 0x23 */ u8 atMax;
    /* 0x24 */ s16 amount;
    /* 0x26 */ u8 unk26[7];
    /* 0x2D */ s8 msg;
} TakeView;
#define TV ((TakeView *)gBoxWork)

u16 BoxMenu_TakeToPouch(u16 pad)
{
    SaveData *save = &gSave;
    u8 *p;
    TakeView *m = TV;
    s32 b;
    s32 c;
    u32 i;
    ItemStack *s;
    ItemStack *sp;
    ItemStack st;

    switch (m->step) {
    case 0:
        pad = BoxMenu_SortPrompt(pad, 0);
        m = TV;
        if (gBoxWork->prompt != 0) {
            break;
        }
        b = pad;
        if (b & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        gHelpLine = 3;
        if (b & 0x200) {
            p = &((ItemBoxMenu *)m)->help;
            if (*p != 0) {
                *p = 0;
            } else {
                *p = 1;
                gHelpColumn = 0;
            }
            Sound_PlaySeFlat(0, 9, 0);
        }
        BoxMenu_MoveCursor(&TV->slot, pad);
        if (gBoxWork->help != 0) {
            PouchMenu_DecoCursor(save->itemBox[TV->slot].item, pad);
        }
        if (!(b & 0x20)) {
            break;
        }
        if (ItemBox_SlotHasItem(TV->slot) == 0) {
            Sound_PlaySeFlat(0, 0x15, 0);
            break;
        }
        gBoxWork->help = 0;
        gHelpColumn = 0;
        if (Item_IsSpecial(save->itemBox[TV->slot].item) != 1) {
            TV->pouch = 0;
          { u8 *q;
            for (m = TV, q = &m->pouch; m->pouch < 20; m = TV, q = &m->pouch) {
                if (save->pouch[m->pouch].item == save->itemBox[m->slot].item) {
                    break;
                }
                (*q)++;
            } }
        } else {
            TV->pouch = 0;
            for (m = TV; m->pouch < 20; m = TV) {
                if (Item_IsSpecial(save->pouch[m->pouch].item) == 1) {
                    break;
                }
                TV->pouch++;
            }
        }
        p = &TV->pouch;
        if (*p < 20) {
            if (gItemData[save->pouch[TV->pouch].item].max == 0xFF || SavePouch_Room(save->itemBox[TV->slot].item) <= 0) {
                TV->msg = 6;
                gHelpLine = 6;
                TV->step = 4;
                Sound_PlaySeFlat(0, 0x15, 0);
            } else if (BoxMenu_CanTakeSeveral() == 0) {
                save->pouch[TV->pouch].count++;
                save->itemBox[TV->slot].count--;
                if (save->itemBox[TV->slot].count == 0) {
                    save->itemBox[TV->slot].item = 0;
                }
                TV->msg = 7;
                gHelpLine = 7;
                TV->step = 4;
                Sound_PlaySeFlat(1, 0x88, 0);
            } else {
                gHelpLine = 5;
                TV->amount = 1;
                TV->atMax = 0;
                TV->step = 2;
                TV->sub = 0;
                TV->msg = 3;
                Sound_PlaySeFlat(0, 0x13, 0);
            }
            break;
        }
        *p = 0;
        pad = 0;
        for (i = 0; i < 20; i++) {
            if (save->pouch[i].item == 0) {
                TV->pouch = i;
                break;
            }
        }
        TV->step = 1;
        Sound_PlaySeFlat(0, 0x13, 0);
    case 1:
      { u8 *q; s32 d; TakeView *n;
        n = TV;
        q = &((ItemBoxMenu *)n)->help;
        if (*q != 0) {
            if (pad & 0x240) {
                *q = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
            }
            pad &= ~0x40;
        } else {
            d = pad;
            if (d & 0x40) {
                gHelpColumn = 0;
                gHelpLine = 3;
                n->step = 0;
                pad = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
                break;
            }
            if (d & 0x200) {
                *q = 1;
                gHelpColumn = 0;
                Sound_PlaySeFlat(0, 9, 0);
            }
        }
      }
        gHelpLine = 4;
        Pouch_MoveCursor(&TV->pouch, pad);
        if (gBoxWork->help != 0) {
            PouchMenu_DecoCursor(save->pouch[TV->pouch].item, pad);
        }
        if (!(pad & 0x20)) {
            break;
        }
        gBoxWork->help = 0;
        m = TV;
        if (save->pouch[m->pouch].item != 0) {
            gHelpLine = 8;
            m->step = 3;
            gBoxWork->yesNo = 0;
            Sound_PlaySeFlat(0, 0x13, 0);
            break;
        }
        if (BoxMenu_CanSplit() == 0) {
            save->pouch[TV->pouch].item = save->itemBox[TV->slot].item;
            save->pouch[TV->pouch].count = save->itemBox[TV->slot].count;
            save->itemBox[TV->slot].count = 0;
            save->itemBox[TV->slot].item = 0;
            TV->msg = 7;
            gHelpLine = 7;
            TV->step = 4;
            Sound_PlaySeFlat(1, 0x88, 0);
            break;
        }
        pad = 0;
        save->pouch[TV->pouch].count = 0;
        TV->amount = 1;
        TV->atMax = 0;
        TV->step = 2;
        Sound_PlaySeFlat(0, 0x13, 0);
        TV->sub = 1;
        TV->msg = 4;
    case 2:
        b = pad;
        if (b & 0x40) {
            pad &= ~0x40;
            m = TV;
            gHelpLine = m->msg;
            m->step = m->sub;
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        gHelpLine = 5;
        BoxMenu_InputAmount(pad, TV->sub);
        if (!(b & 0x20)) {
            break;
        }
        save->pouch[TV->pouch].item = save->itemBox[TV->slot].item;
        save->pouch[TV->pouch].count += TV->amount;
        save->itemBox[TV->slot].count -= TV->amount;
        if (save->itemBox[TV->slot].count == 0) {
            save->itemBox[TV->slot].item = 0;
        }
        TV->msg = 7;
        gHelpLine = 7;
        TV->step = 4;
        Sound_PlaySeFlat(1, 0x88, 0);
        break;
    case 3:
        b = pad;
        if (b & 0x40) {
            gHelpLine = 4;
            ((ItemBoxMenu *)m)->yesNo = 0xFF;
            TV->step = 1;
            pad &= ~0x40;
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        gHelpLine = 8;
        gBoxWork->yesNo = YesNo_Toggle(pad, ((ItemBoxMenu *)m)->yesNo, 0xC00);
        if (!(b & 0x20)) {
            break;
        }
        pad = 0;
        p = &gBoxWork->yesNo;
        if (*p != 0) {
            gHelpLine = 4;
            *p = 0xFF;
            TV->step = 1;
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        m = TV;
        s = (ItemStack *)((u8 *)save->pouch + m->pouch * 4);
        *(sp = &st) = *s;
        *s = save->itemBox[m->slot];
        save->itemBox[TV->slot].item = 0;
        save->itemBox[TV->slot].count = 0;
        ItemBox_Add(st.item, st.count);
        TV->msg = 7;
        gBoxWork->yesNo = 0xFF;
        TV->step = 4;
        Sound_PlaySeFlat(1, 0x88, 0);
    case 4:
        m = TV;
        gHelpLine = m->msg;
        ((ItemBoxMenu *)m)->blink++;
        if (pad & 0x20) {
            TV->step = 0;
            gBoxWork->blink = 0xFF;
            gHelpColumn = 0;
            Sound_PlaySeFlat(0, 9, 0);
        }
        pad = 0;
        break;
    }
    return pad;
}

extern u8 gHelpShow;
#define HELP_ON gHelpShow
extern s16 gHelpLine;
#define HELP_MSG gHelpLine

s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);
s32 Equip_UnequipType(SaveData *save, s32 type);
s32 Equip_EquipSlot(SaveData *save, u16 idx);
s32 Box_YesNoInput(u16 *pad);
s8 BoxList_Build(u8 kind, u8 sorted);
u8 BoxMenu_ListStyles(u8 *out);
void func_3BCC70(u8 a);
void Player_SetColorWord(struct Player *pl, u8 idx);
void EquipSet_RecalcStats(EquipIdx *in, s32 b);
s32 EquipSet_RenameInput(u16 pad);
u16 BoxMenu_EquipListInput(u16 *sel, u16 pad, s32 *ok, s32 flags);
u16 BoxMenu_EquipPickInput(u16 *set, u16 pad, s32 *ok, s32 flags);
extern u8 gBoxPartIdx[];
extern s8 gLobbyTextDirty;

u16 BoxMenu_EquipMenu(u16 pad)
{
    SaveData *save = &gSave;
    EquipSet *sets;
    EquipSet *s;
    EquipSet *cur;
    s32 ok;
    s8 r;
    u8 old;
    u16 idx;
    u8 *p;
    u8 *q;

    switch (gBoxWork->step) {
    case 0:
        if (pad & 0x40) {
            HELP_ON = 0;
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        Menu_CursorUpDown(&gBoxWork->kind, pad, gBoxWork->setEntries);
        HELP_ON = 1;
        HELP_MSG = gBoxWork->kind + 0x33;
        if (pad & 0x20) {
            pad = 0;
            gBoxWork->help = 0;
            gBoxWork->levelSel = 0;
            gBoxWork->msg = -1;
            gBoxWork->prompt = 0;
            gBoxWork->set = 0;
            s = (EquipSet *)((u8 *)save + 0x25E4) + gBoxWork->set;
            switch (gBoxWork->kind) {
            case 0:
                BoxMenu_EquipPickInput(save->equipIdx, 0, &ok, 0);
                break;
            case 2:
                EquipSet_RecalcStats((EquipIdx *)s, (s32)gBoxWork->player);
                HELP_MSG = 0x1F;
                break;
            case 1:
                EquipSet_RecalcStats((EquipIdx *)s, 0);
                HELP_MSG = 0x20;
                break;
            case 3:
                EquipSet_RecalcStats((EquipIdx *)s, 0);
                HELP_MSG = 0x28;
                break;
            case 4:
                HELP_MSG = 0x32;
                gBoxWork->help = 0;
                gBoxWork->count = BoxMenu_ListStyles(gBoxWork->list);
                break;
            }
            gBoxWork->step++;
            gBoxWork->sub = 0;
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 1:
        switch (gBoxWork->kind) {
        case 0:
            Chat_SetColor(0);
            switch (gBoxWork->sub) {
            case 0:
                pad = BoxMenu_EquipPickInput(save->equipIdx, pad, &ok, 0);
                if (pad & 0x40) {
                    HELP_MSG = 0x33;
                    pad = 0;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                    break;
                }
                if (!(pad & 0x20)) {
                    break;
                }
                pad = 0;
                if (ok != 1) {
                    Sound_PlaySeFlat(0, 0x15, 0);
                    break;
                }
                gBoxWork->sub++;
                gBoxWork->help = 0;
                gBoxWork->yesNo = 0;
                Sound_PlaySeFlat(0, 0x13, 0);
            case 1:
                r = Box_YesNoInput(&pad);
                if (r < 0) {
                    HELP_MSG = 0xA;
                    pad = 0;
                    gBoxWork->sub = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                    break;
                }
                if (gBoxWork->boxItem == NULL) {
                    HELP_MSG = 0x10;
                } else if (gBoxWork->equipped == NULL) {
                    HELP_MSG = 0x11;
                } else if (gBoxWork->cursorClass != 2) {
                    HELP_MSG = 0xF;
                } else {
                    HELP_MSG = 0xB;
                }
                Menu_CursorLeftRight(&gBoxWork->levelSel, pad & 0x88, gBoxWork->levelCount);
                if (r > 0) {
                    if (gBoxWork->boxItem == NULL) {
                        ok = Equip_UnequipType(save, gBoxWork->equipped->type);
                    } else {
                        ok = Equip_EquipSlot(save, gBoxWork->slots[1]);
                    }
                    if (ok == 1) {
                        HELP_MSG = 0x12;
                        gBoxWork->levelSel = 0;
                        gBoxWork->sub++;
                        gBoxWork->close = 0;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                }
                break;
            case 2:
                HELP_MSG = 0x12;
                gBoxWork->close = BoxMenu_CloseStep(pad, gBoxWork->close);
                if (gBoxWork->close == 0) {
                    BoxMenu_EquipPickInput(save->equipIdx, 0, &ok, 0);
                    gBoxWork->sub = 0;
                }
                pad = 0;
                break;
            }
            break;
        case 2:
            sets = save->equipSets;
            switch (gBoxWork->sub) {
            case 0:
                HELP_ON = 1;
                if (pad & 0x40) {
                    pad = 0;
                    gBoxWork->levelSel = 0;
                    HELP_MSG = 0x35;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                    break;
                }
                s = &sets[gBoxWork->set];
                if (pad & 0x20) {
                    pad = 0;
                    if (s->idx[0] == 0xFFFF) {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    } else {
                        gBoxWork->levelSel = 0;
                        gBoxWork->yesNo = 0;
                        gBoxWork->sub++;
                        HELP_MSG = 0x22;
                        gBoxWork->levelSel = 0;
                        Sound_PlaySeFlat(0, 0x13, 0);
                        break;
                    }
                }
                HELP_MSG = 0x1F;
                old = gBoxWork->set;
                Menu_CursorLeftRight(&gBoxWork->set, pad & 0xC00, 10);
                s = &sets[gBoxWork->set];
                if (old != gBoxWork->set) {
                    EquipSet_RecalcStats((EquipIdx *)s, (s32)gBoxWork->player);
                    pad = 0;
                }
                if (pad & 0x200) {
                    gBoxWork->levelSel++;
                    if (gBoxWork->levelSel > 2) {
                        gBoxWork->levelSel = 0;
                    }
                    Sound_PlaySeFlat(0, 9, 0);
                    pad = 0;
                }
                if (gBoxWork->levelSel == 2) {
                    Menu_CursorLeftRight(&gBoxWork->skillPage, pad & 0x88, gBoxWork->skillPages);
                    HELP_ON = 0;
                }
                break;
            case 1:
                s = &sets[gBoxWork->set];
                r = Box_YesNoInput(&pad);
                HELP_MSG = 0x22;
                if (r > 0) {
                    Equip_SetEquipped((EquipIdx *)s);
                    HELP_MSG = 0x12;
                    gBoxWork->sub++;
                    gBoxWork->close = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                } else if (r < 0) {
                    HELP_MSG = 0x1F;
                    gBoxWork->sub = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                }
                break;
            case 2:
                gBoxWork->close = BoxMenu_CloseStep(pad, gBoxWork->close);
                if (gBoxWork->close == 0) {
                    EquipSet_RecalcStats((EquipIdx *)&sets[gBoxWork->set], (s32)gBoxWork->player);
                    HELP_MSG = 0x1F;
                    gBoxWork->sub = 0;
                }
                pad = 0;
                break;
            }
            break;
        case 1:
            sets = save->equipSets;
            switch (gBoxWork->sub) {
            case 0:
                HELP_ON = 1;
                if (pad & 0x40) {
                    pad = 0;
                    gBoxWork->levelSel = 0;
                    HELP_MSG = 0x34;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else if (pad & 0x20) {
                    gBoxWork->sub++;
                    gBoxWork->help = 0;
                    gBoxWork->levelSel = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                    gBoxWork->cur = sets[gBoxWork->set];
                    gBoxWork->part = 0;
                    gBoxWork->pageCount = BoxList_Build(gBoxWork->part, 1);
                    gBoxWork->page = 0;
                    gBoxWork->row = 0;
                    if (gBoxWork->cur.idx[0] != 0xFFFF) {
                        gBoxWork->unk54 = 0;
                        HELP_MSG = 0x2B;
                        gBoxWork->sub2 = 0;
                    } else {
                        gBoxWork->unk54 = 1;
                        HELP_MSG = 0x24;
                        gBoxWork->sub2 = 1;
                        gBoxWork->equipped = NULL;
                        BoxMenu_EquipListInput(sets[gBoxWork->set].idx, 0, &ok, 0);
                        EquipSet_RecalcStats((EquipIdx *)&gBoxWork->cur, 0);
                    }
                } else {
                    HELP_MSG = 0x20;
                    old = gBoxWork->set;
                    Menu_CursorLeftRight(&gBoxWork->set, pad & 0xC00, 10);
                    s = &save->equipSets[gBoxWork->set];
                    if (old != gBoxWork->set) {
                        EquipSet_RecalcStats((EquipIdx *)s, 0);
                        pad = 0;
                    }
                    if (pad & 0x200) {
                        gBoxWork->levelSel++;
                        if (gBoxWork->levelSel > 2) {
                            gBoxWork->levelSel = 0;
                        }
                        Sound_PlaySeFlat(0, 9, 0);
                        pad = 0;
                    }
                    if (gBoxWork->levelSel == 2) {
                        Menu_CursorLeftRight(&gBoxWork->skillPage, pad & 0x88, gBoxWork->skillPages);
                        HELP_ON = 0;
                    }
                }
                break;
            case 1:
                Chat_SetColor(0);
                cur = &gBoxWork->cur;
                EquipSet_RecalcStats((EquipIdx *)cur, 0);
                p = &gBoxWork->sub2;
                switch (*p) {
                case 0:
                    if (pad & 0x40) {
                        EquipSet_RecalcStats((EquipIdx *)&save->equipSets[gBoxWork->set], 0);
                        HELP_MSG = 0x20;
                        gBoxWork->sub = 0;
                        gBoxWork->levelSel = 0;
                        Sound_PlaySeFlat(0, 0x14, 0);
                        pad = 0;
                        break;
                    }
                    HELP_ON = 1;
                    if (gBoxWork->part != 6) {
                        HELP_MSG = 0x2B;
                    } else {
                        HELP_MSG = 0x2D;
                    }
                    if (pad & 0x20) {
                        pad = 0;
                        gBoxWork->levelSel = 0;
                        if (gBoxWork->part == 6) {
                            if (EquipSet_DropUnfit((EquipIdx *)&save->equipSets[gBoxWork->set], (EquipIdx *)&gBoxWork->cur, 1) == 0) {
                                EquipSet_DropUnfit((EquipIdx *)&save->equipSets[gBoxWork->set], (EquipIdx *)&gBoxWork->cur, 0);
                                EquipSet_RecalcStats((EquipIdx *)&save->equipSets[gBoxWork->set], 0);
                                gBoxWork->sub++;
                                HELP_MSG = 0x26;
                                gBoxWork->yesNo = 0;
                                Sound_PlaySeFlat(0, 0x19, 0);
                            } else {
                                gBoxWork->sub2 = 2;
                                HELP_MSG = 0x2C;
                                gBoxWork->yesNo = 0;
                                Sound_PlaySeFlat(0, 0x13, 0);
                            }
                        } else {
                            gBoxWork->pageCount = BoxList_Build(gBoxWork->part, 1);
                            if (gBoxWork->pageCount == 0) {
                                Sound_PlaySeFlat(0, 0x15, 0);
                            } else {
                                gBoxWork->sub2++;
                                HELP_MSG = 0x23;
                                gBoxWork->help = 0;
                                gBoxWork->row = 0;
                                gBoxWork->page = 0;
                                idx = cur->idx[gBoxPartIdx[gBoxWork->part]];
                                gBoxWork->equipped = idx != 0xFFFF ? &save->box[idx] : NULL;
                                BoxMenu_EquipListInput(cur->idx, 0, &ok, 0);
                                Sound_PlaySeFlat(0, 0x13, 0);
                            }
                        }
                    } else {
                        if (pad & 0x200) {
                            gBoxWork->levelSel++;
                            if (gBoxWork->levelSel > 2) {
                                gBoxWork->levelSel = 0;
                            }
                            Sound_PlaySeFlat(0, 9, 0);
                            pad = 0;
                        }
                        if (gBoxWork->levelSel < 2) {
                            old = gBoxWork->part;
                            Menu_CursorUpDown(&gBoxWork->part, pad, 7);
                            if (old != gBoxWork->part) {
                                gBoxWork->pageCount = BoxList_Build(gBoxWork->part, 1);
                                gBoxWork->row = 0;
                                gBoxWork->page = 0;
                            }
                        } else {
                            Menu_CursorLeftRight(&gBoxWork->skillPage, pad & 0x88, gBoxWork->skillPages);
                            HELP_ON = 0;
                        }
                    }
                    break;
                case 1:
                    if (pad & 0x40) {
                        if (gBoxWork->unk54 == 0) {
                            HELP_MSG = 0x2B;
                            *p = 0;
                        } else {
                            HELP_MSG = 0x20;
                            gBoxWork->sub = 0;
                        }
                        gBoxWork->levelSel = 0;
                        pad = 0;
                        Sound_PlaySeFlat(0, 0x14, 0);
                        break;
                    }
                    pad = BoxMenu_EquipListInput(cur->idx, pad, &ok, 0);
                    if (pad & 0x20) {
                        if (ok == 0) {
                            Sound_PlaySeFlat(0, 0x15, 0);
                        } else {
                            if (gBoxWork->cursorClass != 0) {
                                cur->idx[0] = gBoxWork->slots[1];
                                gBoxWork->unk54 = 0;
                            } else if (gBoxWork->boxItem == gBoxWork->equipped) {
                                switch (gBoxWork->equipped->type) {
                                case 0:
                                    cur->idx[1] = 0xFFFF;
                                    break;
                                case 2:
                                    cur->idx[2] = 0xFFFF;
                                    break;
                                case 3:
                                    cur->idx[3] = 0xFFFF;
                                    break;
                                case 4:
                                    cur->idx[4] = 0xFFFF;
                                    break;
                                case 5:
                                    cur->idx[5] = 0xFFFF;
                                    break;
                                }
                            } else {
                                switch (gBoxWork->boxItem->type) {
                                case 0:
                                    cur->idx[1] = gBoxWork->slots[1];
                                    break;
                                case 2:
                                    cur->idx[2] = gBoxWork->slots[1];
                                    break;
                                case 3:
                                    cur->idx[3] = gBoxWork->slots[1];
                                    break;
                                case 4:
                                    cur->idx[4] = gBoxWork->slots[1];
                                    break;
                                case 5:
                                    cur->idx[5] = gBoxWork->slots[1];
                                    break;
                                }
                            }
                            EquipSet_RecalcStats((EquipIdx *)cur, 0);
                            HELP_MSG = 0x2B;
                            gBoxWork->sub2 = 0;
                            gBoxWork->levelSel = 0;
                            Sound_PlaySeFlat(0, 0x13, 0);
                        }
                    }
                    break;
                case 2:
                    r = Box_YesNoInput(&pad);
                    if (r != 0) {
                        if (r > 0) {
                            EquipSet_DropUnfit((EquipIdx *)&save->equipSets[gBoxWork->set], (EquipIdx *)&gBoxWork->cur, 0);
                            EquipSet_RecalcStats((EquipIdx *)&save->equipSets[gBoxWork->set], 0);
                            gBoxWork->sub++;
                            HELP_MSG = 0x26;
                            gBoxWork->yesNo = 0;
                            Sound_PlaySeFlat(0, 0x19, 0);
                        } else {
                            gBoxWork->sub2 = 0;
                            HELP_MSG = 0x2D;
                            Sound_PlaySeFlat(0, 0x14, 0);
                        }
                    }
                    pad = 0;
                    break;
                }
                break;
            case 2:
                r = Box_YesNoInput(&pad);
                HELP_MSG = 0x26;
                if (r != 0) {
                    if (r > 0) {
                        HELP_MSG = 0x2F;
                        gBoxWork->sub++;
                        gBoxWork->sub2 = 0;
                        gBoxWork->yesNo = 0xFF;
                        EquipSet_RenameInput(0);
                    } else {
                        HELP_MSG = 0x21;
                        gBoxWork->sub = 4;
                        gBoxWork->yesNo = 0;
                        Sound_PlaySeFlat(0, 0x14, 0);
                    }
                }
                pad = 0;
                break;
            case 3:
                if (EquipSet_RenameInput(pad) == 0) {
                    gBoxWork->sub++;
                    gBoxWork->yesNo = 0;
                    HELP_MSG = 0x21;
                }
                pad = 0;
                break;
            case 4:
                r = Box_YesNoInput(&pad);
                HELP_MSG = 0x21;
                if (r != 0) {
                    if (r > 0) {
                        Equip_SetEquipped((EquipIdx *)&sets[gBoxWork->set]);
                        gBoxWork->close = 0;
                        gBoxWork->sub++;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        pad = 0;
                        HELP_MSG = 0x20;
                        gBoxWork->sub = 0;
                        Sound_PlaySeFlat(0, 0x14, 0);
                    }
                    gBoxWork->yesNo = 0xFF;
                }
                pad = 0;
                break;
            case 5:
                gBoxWork->close = BoxMenu_CloseStep(pad, gBoxWork->close);
                if (gBoxWork->close == 0) {
                    HELP_MSG = 0x20;
                    gBoxWork->sub = 0;
                }
                pad = 0;
                break;
            }
            break;
        case 3:
            sets = save->equipSets;
            switch (gBoxWork->sub) {
            case 0:
                HELP_ON = 1;
                if (pad & 0x40) {
                    HELP_MSG = 0x36;
                    pad = 0;
                    gBoxWork->levelSel = 0;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else if (pad & 0x20) {
                    HELP_MSG = 0x29;
                    pad = 0;
                    gBoxWork->yesNo = 0;
                    gBoxWork->levelSel = 0;
                    gBoxWork->sub++;
                    Sound_PlaySeFlat(0, 0x13, 0);
                } else {
                    HELP_MSG = 0x28;
                    old = gBoxWork->set;
                    Menu_CursorLeftRight(&gBoxWork->set, pad & 0xC00, 10);
                    s = &save->equipSets[gBoxWork->set];
                    if (old != gBoxWork->set) {
                        EquipSet_RecalcStats((EquipIdx *)s, 0);
                        pad = 0;
                    }
                    if (pad & 0x200) {
                        gBoxWork->levelSel++;
                        if (gBoxWork->levelSel > 2) {
                            gBoxWork->levelSel = 0;
                        }
                        Sound_PlaySeFlat(0, 9, 0);
                        pad = 0;
                    }
                    if (gBoxWork->levelSel == 2) {
                        Menu_CursorLeftRight(&gBoxWork->skillPage, pad & 0x88, gBoxWork->skillPages);
                        HELP_ON = 0;
                    }
                }
                break;
            case 1:
                r = Box_YesNoInput(&pad);
                HELP_MSG = 0x29;
                if (r != 0) {
                    if (r > 0) {
                        q = &gBoxWork->set;
                        *(EquipIdx *)&sets[*q] = *(EquipIdx *)save->equipIdx;
                        gBoxWork->cur = sets[*q];
                        EquipSet_RecalcStats((EquipIdx *)&gBoxWork->cur, 0);
                        HELP_MSG = 0x26;
                        gBoxWork->sub++;
                        gBoxWork->yesNo = 0;
                        Sound_PlaySeFlat(0, 0x19, 0);
                    } else {
                        HELP_MSG = 0x28;
                        gBoxWork->sub--;
                        gBoxWork->yesNo = 0xFF;
                        Sound_PlaySeFlat(0, 0x14, 0);
                    }
                }
                pad = 0;
                break;
            case 2:
                r = Box_YesNoInput(&pad);
                HELP_MSG = 0x26;
                if (r != 0) {
                    if (r > 0) {
                        HELP_MSG = 0x2F;
                        gBoxWork->sub++;
                        gBoxWork->sub2 = 0;
                        EquipSet_RenameInput(0);
                    } else {
                        gBoxWork->sub = 4;
                        HELP_MSG = 0x2A;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    }
                }
                pad = 0;
                break;
            case 3:
                if (EquipSet_RenameInput(pad) == 0) {
                    HELP_MSG = 0x2A;
                    gBoxWork->sub++;
                }
                pad = 0;
                break;
            case 4:
                HELP_MSG = 0x2A;
                gBoxWork->blink++;
                if (pad & 0x20) {
                    HELP_MSG = 0x36;
                    gBoxWork->blink = 0xFF;
                    gBoxWork->step = 0;
                    Sound_PlaySeFlat(0, 9, 0);
                }
                pad = 0;
                break;
            }
            break;
        case 4:
            if (pad & 0x40) {
                pad = 0;
                HELP_MSG = 0x37;
                gBoxWork->step = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else {
                gBoxWork->count = BoxMenu_ListStyles(gBoxWork->list);
                Menu_CursorUpDown(&gBoxWork->pick, pad, gBoxWork->count);
                if (pad & 0x200) {
                    gBoxWork->help ^= 0x81;
                    Sound_PlaySeFlat(0, 9, 0);
                }
                if (gBoxWork->help == 0) {
                    HELP_MSG = 0x32;
                } else {
                    HELP_MSG = gBoxWork->list[gBoxWork->pick] + 0x3C;
                }
                if (pad & 0x20) {
                    func_3BCC70(gBoxWork->list[gBoxWork->pick]);
                    Player_SetColorWord((struct Player *)gBoxWork->player, gBoxWork->list[gBoxWork->pick]);
                    Sound_PlaySeFlat(0, 0x13, 0);
                    gLobbyTextDirty = 1;
                }
            }
            break;
        }
        break;
    }
    return pad;
}

void Player_RecomputeStats(struct Player *pl);
void Player_SetWeapon(struct Player *pl, EquipItem *e);
s32 Player_GetHpSkillBonus(struct Player *pl);
void Player_CalcAttackRemote(struct Player *pl);
void Player_CalcDefenseRemote(struct Player *pl);
void Player_CalcResistsRemote(struct Player *pl);
u8 SkillView_Init(void *pl, s32 b);
typedef struct PlayerV {
    /* 0x000 */ u8 active;
    /* 0x001 */ u8 unk1[0x2C1];
    /* 0x2C2 */ s16 hp;
    /* 0x2C4 */ u8 unk2C4[0x312 - 0x2C4];
    /* 0x312 */ EquipItem equip[7];
    /* 0x366 */ u8 unk366[0x628 - 0x366];
    /* 0x628 */ s8 unk628;          /* attack buffs */
    /* 0x629 */ u8 unk629;
    /* 0x62A */ u8 unk62A[2];
    /* 0x62C */ s8 unk62C;          /* defense buffs */
    /* 0x62D */ u8 unk62D;
    /* 0x62E */ u8 unk62E[0x6DE - 0x62E];
    /* 0x6DE */ s16 hpMax;
    /* 0x6E0 */ u8 unk6E0[0x7DA - 0x6E0];
    /* 0x7DA */ s16 staminaMax;
    /* 0x7DC */ u8 unk7DC[0x970 - 0x7DC];
} __attribute__((aligned(16))) PlayerV;
extern PlayerV gPreviewPlayer;

void EquipSet_RecalcStats(EquipIdx *in, s32 b)
{
    EquipIdx set;
    s16 hp;
    SaveData *save = &gSave;
    PlayerV *pl = (PlayerV *)&gPreviewPlayer;

    pl->active = 0;
    if (in->idx[0] == 0xFFFF) {
        pl->equip[6].id = 0;
        pl->equip[2].id = 0;
        pl->equip[3].id = 0;
        pl->equip[4].id = 0;
        pl->equip[5].id = 0;
        pl->equip[0].id = 0;
        Player_CalcSkills((struct Player *)pl);
        gBoxWork->skillPages = 0;
        gBoxWork->skillPage = 0;
        return;
    }
    Player_RecomputeStats((struct Player *)gBoxWork->player);
    *pl = *(PlayerV *)gBoxWork->player;
    set = *in;
    EquipSet_DropUnfit(&set, &set, 0);
    in = &set;
    Player_SetWeapon((struct Player *)pl, &save->box[set.idx[0]]);
    if (set.idx[2] != 0xFFFF) {
        pl->equip[2] = save->box[in->idx[2]];
    } else {
        pl->equip[2].id = 0;
    }
    if (in->idx[3] != 0xFFFF) {
        pl->equip[3] = save->box[in->idx[3]];
    } else {
        pl->equip[3].id = 0;
    }
    if (in->idx[4] != 0xFFFF) {
        pl->equip[4] = save->box[in->idx[4]];
    } else {
        pl->equip[4].id = 0;
    }
    if (in->idx[5] != 0xFFFF) {
        pl->equip[5] = save->box[in->idx[5]];
    } else {
        pl->equip[5].id = 0;
    }
    if (in->idx[1] != 0xFFFF) {
        pl->equip[0] = save->box[in->idx[1]];
    } else {
        pl->equip[0].id = 0;
    }
    Player_CalcSkills((struct Player *)pl);
    hp = gScene.mealHealth + 100 + (s16)Player_GetHpSkillBonus((struct Player *)pl);
    if (hp > 150) {
        hp = 150;
    }
    pl->hp = pl->hpMax = hp;
    pl->staminaMax = gScene.mealStamina + 300;
    if (pl->staminaMax <= 0x4B) {
        pl->staminaMax = 0x4B;
    } else if (pl->staminaMax > 0x1C2) {
        pl->staminaMax = 0x1C2;
    }
    pl->unk628 = gScene.mealAttack;
    pl->unk629 = 0;
    Player_CalcAttackRemote((struct Player *)pl);
    pl->unk62C = gScene.mealDefense;
    pl->unk62D = 0;
    Player_CalcDefenseRemote((struct Player *)pl);
    Player_CalcResistsRemote((struct Player *)pl);
    gBoxWork->skillPages = SkillView_Init(pl, b);
    if (gBoxWork->skillPage > gBoxWork->skillPages - 1) {
        gBoxWork->skillPage = gBoxWork->skillPages - 1;
    }
}

void Equip_SetEquipped(EquipIdx *e);
u8 BoxMenu_CloseStep(u16 pad, u8 step);
void BoxMenu_UpdateSkillPreview(void);
u8 EquipSet_DropUnfit(EquipIdx *out, EquipIdx *set, s32 flags);
s32 ItemBox_SlotHasItem(u16 slot);
s32 EquipBox_TryRemove(u16 slot);
static s32 BoxMenu_CanSplit(void);
s32 BoxMenu_CanTakeSeveral(void);
void BoxMenu_InputAmount(u16 pad, u8 noLimit);
void Equip_SwapSlots(u16 a, u16 b);
s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
s32 Field_ControlEnabled(void);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
u8 YesNo_Toggle(u16 pad, u8 cur, u16 mask);
void *memcpy(void *dst, const void *src, u32 n);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_760C70", D_0083B348);
extern u8 D_0083B348;

s32 EquipSet_RenameInput(u16 pad)
{
    EquipSet *sets = gSave.equipSets;
    s8 r;
    u8 name[0x14];
    s16 x;
    s16 y;

    switch (gBoxWork->sub2) {
    case 0:
        if (Field_ControlEnabled() != 1) {
            return 0;
        }
        gBoxWork->sub2++;
        memcpy(gBoxWork->cur.name, sets[gBoxWork->set].name, 0x14);
        Kbd_SetPos(0x3C, 76.0f);
        Chat_OpenKbd(2, 0xE, 0x14, &D_0083B348);
        Sound_PlaySeFlat(0, 0x11, 0);
    case 1:
        x = *(s16 *)&gPadState[0].held;
        gHelpLine = 0x2F;
        y = *(s16 *)&gPadState[0].pressed;
        name[0] = 0;
        r = Kbd_Run((char *) name, x, y);
        if (r != 0) {
            if (r > 0) {
                memcpy(gBoxWork->cur.name, name, 0x14);
            }
            gBoxWork->sub2++;
        }
        break;
    case 2:
        Kbd_Cancel();
        gBoxWork->yesNo = 0;
        gBoxWork->sub2++;
    case 3:
        r = 0;
        if (pad & 0x40) {
            if (gBoxWork->yesNo == 0) {
                gBoxWork->yesNo = 1;
                Sound_PlaySeFlat(0, 0x17, 0);
            } else {
                r = -1;
            }
        } else if (pad & 0x20) {
            r = gBoxWork->yesNo == 0 ? 1 : -1;
        } else {
            gBoxWork->yesNo = YesNo_Toggle(pad, gBoxWork->yesNo, 0xC00);
        }
        gHelpLine = 0x27;
        if (r != 0) {
            gBoxWork->yesNo = 0xFF;
            if (r > 0) {
                memcpy(sets[gBoxWork->set].name, gBoxWork->cur.name, 0x14);
                Sound_PlaySeFlat(0, 0x13, 0);
                return 0;
            }
            gBoxWork->sub2 = 0;
            gHelpLine = 0x2F;
        }
        break;
    }
    return 1;
}

void Equip_SetEquipped(EquipIdx *e)
{
    gSave.equipIdx[0] = e->idx[0];
    Equip_Refresh(&gSave);
    *(EquipIdx *)gSave.equipIdx = *e;
}

u8 BoxMenu_CloseStep(u16 pad, u8 step)
{
    switch (step) {
    case 0:
        Sound_PlaySeFlat(1, 0x89, 0);
        step++;
        break;
    case 1:
        Town_ReloadLocalPlayer();
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        step++;
        break;
    case 2:
        gHelpLine = 0x13;
        gBoxWork->blink++;
        if (pad & 0x20) {
            step = 0;
            gBoxWork->blink = 0xFF;
            Sound_PlaySeFlat(0, 9, 0);
        }
        break;
    }
    return step;
}

u8 Equip_LevelMark(struct EquipItem *e, u8 flags, struct EquipItem *other);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);

u16 BoxMenu_EquipPickInput(u16 *set, u16 pad, s32 *ok, s32 flags)
{
    SaveData *save = &gSave;
    u8 mode;
    u16 idx;
    u16 prev;
    u8 st;
    u8 *p;
    u8 fl;

    pad = BoxMenu_SortPrompt(pad, 1);
    if (gBoxWork->prompt != 0) {
        return pad;
    }
    mode = flags & 1;
    if (mode == 1) {
        p = &gBoxWork->help;
        st = *p;
        if (st == 0) {
            if (!(pad & 0x60) && (pad & 0x200)) {
                *p = 0x81;
                Sound_PlaySeFlat(0, 0x11, 0);
                return 0;
            }
        } else if (pad & 0x40) {
            *p = 0;
            pad = 0;
            Sound_PlaySeFlat(0, 0x14, 0);
        } else if (!(pad & 0x20)) {
            if (st == 0x81) {
                if (pad & 0x200) {
                    *p = 2;
                    BoxMenu_UpdateSkillPreview();
                    Sound_PlaySeFlat(0, 0x11, 0);
                } else {
                    Menu_CursorLeftRight(&gBoxWork->levelSel, pad & 0x88, gBoxWork->levelCount);
                }
            } else if (st == 2) {
                if (pad & 0x200) {
                    *p = 0x81;
                    Sound_PlaySeFlat(0, 0x11, 0);
                } else {
                    Menu_CursorLeftRight(&gBoxWork->skillPage, pad & 0x88, gBoxWork->skillPages);
                }
            }
            return 0;
        }
    }
    if (pad & 0x40) {
        return 0x40;
    }
    if (mode == 0) {
        gHelpLine = 0xA;
    } else if (set[0] != 0xFFFF) {
        gHelpLine = 0x23;
    } else {
        gHelpLine = 0x24;
    }
    prev = gBoxWork->slots[1];
    BoxMenu_MoveCursor(&gBoxWork->slots[1], pad);
    gBoxWork->boxItem = &save->box[gBoxWork->slots[1]];
    if (gBoxWork->boxItem->used != 0) {
        *ok = 1;
        gBoxWork->cursorClass = 0;
        gBoxWork->canEquip = 1;
        switch (gBoxWork->boxItem->type) {
        case 0:
            idx = set[1];
            break;
        case 2:
            idx = set[2];
            break;
        case 3:
            idx = set[3];
            break;
        case 4:
            idx = set[4];
            break;
        case 5:
            idx = set[5];
            break;
        case 6:
        case 7:
            idx = set[0];
            gBoxWork->cursorClass = 1;
            break;
        }
        if (idx != 0xFFFF) {
            gBoxWork->equipped = &save->box[idx];
        } else {
            gBoxWork->equipped = NULL;
        }
        if (mode == 0) {
            if (gBoxWork->equipped == gBoxWork->boxItem) {
                if (gBoxWork->cursorClass != 0) {
                    *ok = 0;
                    gHelpLine = 0xC;
                    gBoxWork->canEquip = 0;
                }
                gBoxWork->boxItem = NULL;
            } else if (gBoxWork->cursorClass == 0) {
                fl = Equip_GetData(gBoxWork->boxItem)->armor.flags;
                if (!(fl & ((save->gender == 0) ? (u8)1 : (u8)2))) {
                    *ok = 0;
                    gHelpLine = 0xE;
                    gBoxWork->canEquip = 0;
                    goto done;
                }
                if (!(fl & (u8)((save->box[set[0]].type == 6) ? 4 : 8))) {
                    *ok = 0;
                    gHelpLine = 0xD;
                    gBoxWork->canEquip = 0;
                    goto done;
                }
            } else if (gBoxWork->equipped->type != gBoxWork->boxItem->type) {
                gBoxWork->cursorClass = 2;
            }
        } else if (gBoxWork->cursorClass != 0) {
            if (gBoxWork->equipped == gBoxWork->boxItem) {
                *ok = 0;
                gHelpLine = 0xC;
                gBoxWork->boxItem = NULL;
            } else if (gBoxWork->equipped != NULL && gBoxWork->equipped->type != gBoxWork->boxItem->type) {
                gBoxWork->cursorClass = 2;
            }
        } else if (set[0] != 0xFFFF) {
            if (gBoxWork->equipped == gBoxWork->boxItem) {
                gBoxWork->boxItem = NULL;
            } else {
                fl = Equip_GetData(gBoxWork->boxItem)->armor.flags;
                if (!(fl & ((save->gender == 0) ? (u8)1 : (u8)2))) {
                    *ok = 0;
                    gHelpLine = 0xE;
                    gBoxWork->canEquip = 0;
                    goto done;
                }
                if (!(fl & (u8)((save->box[set[0]].type == 6) ? 4 : 8))) {
                    *ok = 0;
                    gHelpLine = 0x25;
                    gBoxWork->canEquip = 0;
                    goto done;
                }
            }
        } else {
            *ok = 0;
            gBoxWork->canEquip = 0;
        }
    done:
        gBoxWork->levelCount = Equip_LevelMark((struct EquipItem *)gBoxWork->boxItem, 0x10, (struct EquipItem *)gBoxWork->equipped);
        if (gBoxWork->levelSel >= gBoxWork->levelCount) {
            gBoxWork->levelSel = gBoxWork->levelCount - 1;
        }
    } else {
        *ok = 0;
        gBoxWork->canEquip = 0;
    }
    if (mode == 0) {
        if (gBoxWork->help != 0) {
            if (prev != gBoxWork->slots[1] && gBoxWork->canEquip != 0) {
                BoxMenu_UpdateSkillPreview();
            }
            if (!(pad & 0x20)) {
                if (pad & 0x240) {
                    gBoxWork->help = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else {
                    Menu_CursorLeftRight(&gBoxWork->skillPage, pad & 0x88, gBoxWork->skillPages);
                }
                pad = 0;
            }
        } else if (pad & 0x200) {
            gBoxWork->help = 1;
            BoxMenu_UpdateSkillPreview();
            pad = 0;
            Sound_PlaySeFlat(0, 0x11, 0);
        } else {
            Menu_CursorLeftRight(&gBoxWork->levelSel, pad & 0x88, gBoxWork->levelCount);
        }
    }
    return pad;
}

void BoxMenu_UpdateSkillPreview(void)
{
    ItemBoxMenu *m = gBoxWork;
    EquipItem *src = m->boxItem;
    Player *player = m->player;
    EquipItem e;

    if (src == NULL) {
        e.type = m->equipped->type;
        e.id = 0;
    } else {
        e = *src;
    }
    gBoxWork->skillPages = Equip_PreviewSkills(player, &e);
    gBoxWork->skillPage = 0;
}


typedef struct SkillPreview {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ EquipItem e;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ s8 unk11;
    /* 0x12 */ u8 unk12;
} SkillPreview;
extern Player gSkillTryPlayer; /* a scratch Player: the equipment tried on */
void Player_RemoveUnwearable(struct Player *pl);
s32 Equip_DataWearable(struct Player *pl, EquipItem *e);
u8 Player_ArmorHasTorsoUp(Player *p);

u8 Equip_PreviewSkills(Player *player, EquipItem *e)
{
    Player *pl = player;
    Player *tmp = (Player *)&gSkillTryPlayer; /* the cast keeps the address in a register */
    SkillPreview *v;

    gSkillTryPlayer.equip[6] = pl->equip[6];
    tmp->equip[0] = pl->equip[0];
    tmp->equip[1] = pl->equip[1];
    tmp->equip[2] = pl->equip[2];
    tmp->equip[3] = pl->equip[3];
    tmp->equip[4] = pl->equip[4];
    tmp->equip[5] = pl->equip[5];
    if (e->type == 6 || e->type == 7) {
        tmp->equip[6] = *e;
        if (pl->equip[6].type != e->type) {
            Player_RemoveUnwearable(tmp);
        }
    } else {
        tmp->equip[e->type] = *e;
    }
    Player_CalcSkills(tmp);
    SkillList_Build(tmp, player);
    v = (SkillPreview *)gSkillPreview;
    v->unk0 = 0;
    v->e = *e;
    v->e.used = 1;
    v->unk10 = SkillList_PageCount();
    if (e->type == 6 || e->type == 7) {
        v->unk11 = 1;
    } else {
        v->unk11 = Equip_DataWearable(tmp, e);
    }
    v->unk12 = Player_ArmorHasTorsoUp(tmp);
    return v->unk10;
}

u8 SkillView_Init(void *pl, s32 b)
{
    SkillView *v;

    Player_CalcSkills((struct Player *)pl);
    SkillList_Build((struct Player *)pl, (struct Player *)b);
    v = (SkillView *)gSkillPreview;
    v->unk0 = b;
    v->unk4 = 0;
    v->unk10 = SkillList_PageCount();
    v->unk11 = 1;
    return v->unk10;
}

u8 EquipSet_RemoveSlot(u16 slot, s32 check)
{
    SaveData *save = &gSave;
    EquipSet *set;
    s32 n;
    s32 i;
    u32 j;

    n = 0;
    set = gSave.equipSets;
    for (i = 10; i != 0; i--, set++) {
        for (j = 0; j < 6; j++) {
            if (slot == set->idx[j]) {
                if (!(check & 1)) {
                    if (save->box[slot].type != 7 && save->box[slot].type != 6) {
                        set->idx[j] = 0xFFFF;
                    } else {
                        set->idx[5] = 0xFFFF;
                        set->idx[4] = 0xFFFF;
                        set->idx[3] = 0xFFFF;
                        set->idx[2] = 0xFFFF;
                        set->idx[1] = 0xFFFF;
                        set->idx[0] = 0xFFFF;
                    }
                    n++;
                    break;
                } else {
                    return 1;
                }
            }
        }
    }
    return n;
}

u8 EquipSet_DropUnfit(EquipIdx *out, EquipIdx *set, s32 flags)
{
    SaveData *save = &gSave;
    u8 f;
    u8 bit;
    u32 i;
    u8 n;

    if (out == NULL) {
        flags |= 1;
    }
    bit = (save->box[set->idx[0]].type == 6) ? 4 : 8;
    n = 0;
    for (i = 1; i < 6; i++) {
        if (set->idx[i] != 0xFFFF) {
            f = Equip_GetData(&save->box[set->idx[i]])->armor.flags;
            if (!(f & bit)) {
                if (!(flags & 1)) {
                    set->idx[i] = 0xFFFF;
                }
                n++;
            }
        }
    }
    if (!(flags & 1)) {
        *out = *set;
    }
    return n;
}

/* The gender bit of an equipment data entry (data[2]: 1 male, 2 female)
 * for the save gender */
static inline u8 Equip_GenderBit(u8 g)
{
    return g == 0 ? 1 : 2;
}

s32 ItemBox_SlotHasItem(u16 slot);
s32 EquipBox_TryRemove(u16 slot);
static s32 BoxMenu_CanSplit(void);
s32 BoxMenu_CanTakeSeveral(void);
void BoxMenu_InputAmount(u16 pad, u8 noLimit);
void Equip_SwapSlots(u16 a, u16 b);
s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
s16 BoxList_MoveCursor(u8 *row, u8 *page, u16 pad);
u8 Equip_LevelMark(struct EquipItem *e, u8 flags, struct EquipItem *other);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);

/* Raw addresses kept here: the named form changes the code. */
u16 BoxMenu_EquipListInput(u16 *sel, u16 pad, s32 *ok, s32 flags)
{
    SaveData *save = &gSave;
    ArmorData *data;
    u8 *p;
    ItemBoxMenu *m;
    s32 b;

    *(u16 *)&gHelpLine = 0xFFFF;
    if (pad & 0x60) {
        pad &= 0x60;
    }
    gBoxWork->slots[1] = BoxList_MoveCursor(&gBoxWork->row, &gBoxWork->page, pad);
    gBoxWork->boxItem = &save->box[gBoxWork->slots[1]];
    *ok = 1;
    if (gBoxWork->part == 0) {
        gBoxWork->cursorClass = 1;
        if (gBoxWork->equipped == gBoxWork->boxItem) {
            *ok = 0;
            gHelpLine = 0xC;
        }
    } else {
        gBoxWork->cursorClass = 0;
        if (gBoxWork->equipped == gBoxWork->boxItem) {
            gHelpLine = 0x2E;
        } else {
            data = &Equip_GetData(gBoxWork->boxItem)->armor;
            if (!(data->flags & Equip_GenderBit(save->gender))) {
                *ok = 0;
                gHelpLine = 0xE;
            } else {
                if (!(data->flags & (u8)(save->box[*sel].type == 6 ? 4 : 8))) {
                    *ok = 0;
                    gHelpLine = 0x25;
                }
            }
        }
    }
    gBoxWork->levelCount = Equip_LevelMark((struct EquipItem *)gBoxWork->boxItem, 0, (struct EquipItem *)NULL);
    if (gBoxWork->levelSel >= gBoxWork->levelCount) {
        gBoxWork->levelSel = gBoxWork->levelCount - 1;
    }
    m = gBoxWork;
    p = &m->help;
    if (*p == 0) {
        if (pad & 0x200) {
            *p = 0x81;
            Sound_PlaySeFlat(0, 0x11, 0);
        }
    } else {
        b = pad;
        if (b & 0x200) {
            *p = 0;
            Sound_PlaySeFlat(0, 0x14, 0);
        } else if (m->boxItem != NULL) {
            Menu_CursorLeftRight(&m->levelSel, b & 0x88, m->levelCount);
        }
    }
    return pad;
}

u16 BoxMenu_Input(u16 pad);

/* kind is set first and the help line read through it. The swap reads the held slot and the kind
 * into h and k right after their pointers (held, h, k2, k, slots): created in that order, the
 * first scheduling pass makes slots = gBoxWork->slots gBoxWork's last use, so the two share v1
 * (round 30; with the loads after slots, or read at their uses, gBoxWork took a1). */
u16 BoxMenu_ArrangeMenu(u16 pad)
{
    u8 *kind = &gBoxWork->kind;
    SaveData *save;
    u8 *k2;
    u16 *slots;
    u16 *held;
    u16 c;
    u8 k;
    u16 h;
    ItemStack st;
    EquipItem e;

    gHelpLine = *kind + 0x14;
    switch (gBoxWork->step) {
    case 0:
        if (pad & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        Menu_CursorUpDown(kind, pad, 2);
        if (pad & 0x20) {
            gBoxWork->help = 0;
            gBoxWork->step++;
            gBoxWork->sub = 0;
            Sound_PlaySeFlat(0, 0x13, 0);
        } else {
            gHelpShow = 0;
        }
        break;
    case 1:
        pad = BoxMenu_SortPrompt(pad, *kind);
        if (gBoxWork->prompt != 0) {
            gBoxWork->sub = 0;
            break;
        }
        switch (gBoxWork->sub) {
        case 0:
            pad = BoxMenu_Input(pad);
            if (pad & 0x40) {
                gHelpShow = 0;
                pad &= ~0x40;
                gBoxWork->step = 0;
            } else if (pad & 0x20) {
                gBoxWork->held = (&gBoxWork->slots[0])[gBoxWork->kind];
                gBoxWork->sub++;
                Sound_PlaySeFlat(1, 0x8A, 0);
            }
            break;
        case 1:
            pad = BoxMenu_Input(pad);
            if (pad & 0x40) {
                pad &= ~0x40;
                gBoxWork->sub = 0;
            } else if (pad & 0x20) {
                held = &gBoxWork->held;
                h = *held;
                k2 = &gBoxWork->kind;
                k = *k2;
                slots = gBoxWork->slots;
                c = slots[k];
                if (c != h) {
                    save = &gSave;
                    if (k == 0) {
                        st = save->itemBox[c];
                        save->itemBox[c] = save->itemBox[h];
                        save->itemBox[gBoxWork->held] = st;
                    } else {
                        e = save->box[c];
                        save->box[c] = save->box[h];
                        save->box[*held] = e;
                        Equip_SwapSlots(*held, slots[*k2]);
                    }
                    gBoxWork->sub = 0;
                    Sound_PlaySeFlat(1, 0x8B, 0);
                } else {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
            }
            break;
        }
        break;
    }
    return pad;
}

s32 Box_ClearSlot(SaveData *save, u16 idx);
s32 Equip_GetSellPrice(EquipItem *e);

/* ItemBoxMenu seen by BoxMenu_SellMenu: the ItemBoxMenu form changes the code (MWCC reuses the load of gBoxWork) */
typedef struct SellView {
    /* 0x00 */ u8 unk0[5];
    /* 0x05 */ u8 step;
    /* 0x06 */ u8 sub;
    /* 0x07 */ u8 unk7[2];
    /* 0x09 */ u8 kind;             /* 0 items, 1 equipment */
    /* 0x0A */ u8 unkA[0xE];
    /* 0x18 */ s32 price;
    /* 0x1C */ u16 slot[2];
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ s16 amount;
    /* 0x26 */ u8 unk26[7];
    /* 0x2D */ s8 msg;
} SellView;

u16 BoxMenu_SellMenu(u16 pad)
{
    SaveData *save = &gSave;
    s8 *q;
    u8 *p;
    SellView *m = (SellView *)gBoxWork;
    s32 ok;
    s8 r;
    s32 b;
    s32 c;

    switch (m->step) {
    case 0:
        b = pad;
        if (b & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        Menu_CursorUpDown(&m->kind, pad, 2);
        if (b & 0x20) {
            m = (SellView *)gBoxWork;
            if (m->kind == 0) {
                gHelpLine = 0x16;
            } else {
                gHelpLine = 0x17;
                if (EquipSet_RemoveSlot(m->slot[1], 1) != 0) {
                    gHelpLine = 0x1B;
                }
            }
            ((SellView *)gBoxWork)->msg = -1;
            ((SellView *)gBoxWork)->step++;
            ((SellView *)gBoxWork)->sub = 0;
            Sound_PlaySeFlat(0, 0x13, 0);
        } else {
            gHelpShow = 0;
        }
        break;
    case 1:
        p = &m->sub;
        switch (*p) {
        case 0:
            q = &m->msg;
            if (*q >= 0) {
                if (pad & 0x20) {
                    *q = -1;
                    gBoxWork->blink = 0xFF;
                    pad = 0;
                    Sound_PlaySeFlat(0, 9, 0);
                } else {
                    ((ItemBoxMenu *)m)->blink++;
                    gHelpLine = ((SellView *)gBoxWork)->msg;
                    return 0;
                }
            }
            pad = BoxMenu_SortPrompt(pad, ((SellView *)gBoxWork)->kind);
            if (gBoxWork->prompt != 0) {
                break;
            }
            pad = BoxMenu_Input(pad);
            if (pad & 0x40) {
                pad = 0;
                gHelpShow = 0;
                ((SellView *)gBoxWork)->step = 0;
                break;
            }
            m = (SellView *)gBoxWork;
            if (m->kind == 0) {
                gHelpLine = 0x16;
            } else {
                gHelpLine = 0x17;
                if (EquipSet_RemoveSlot(m->slot[1], 1) != 0) {
                    gHelpLine = 0x1B;
                }
            }
            if (pad & 0x20) {
                m = (SellView *)gBoxWork;
                if (m->kind == 0) {
                    ok = ItemBox_SlotHasItem(m->slot[0]);
                    if (ok == 1) {
                        if (BoxMenu_CanSplit() == 0) {
                            gHelpLine = 0x19;
                            ((SellView *)gBoxWork)->sub = 2;
                            gBoxWork->yesNo = 1;
                            ((SellView *)gBoxWork)->price = gItemData[save->itemBox[((SellView *)gBoxWork)->slot[0]].item].value;
                        } else {
                            ((SellView *)gBoxWork)->amount = 1;
                            gHelpLine = 0x18;
                            ((SellView *)gBoxWork)->sub = 1;
                        }
                    }
                } else {
                    r = EquipBox_TryRemove(m->slot[1]);
                    if (r > 0) {
                        gHelpLine = 0x19;
                        ((SellView *)gBoxWork)->sub = 2;
                        gBoxWork->yesNo = 1;
                        ((SellView *)gBoxWork)->price = Equip_GetSellPrice(&save->box[((SellView *)gBoxWork)->slot[1]]);
                        ok = 1;
                    } else {
                        ok = 0;
                        if (r == 0) {
                            ((SellView *)gBoxWork)->msg = 0x1A;
                        }
                    }
                }
                if (ok == 1) {
                    gBoxWork->help = 0;
                    ((SellView *)gBoxWork)->amount = 1;
                    gBoxWork->atMax = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                } else {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
            }
            break;
        case 1:
            b = pad;
            if (b & 0x40) {
                gHelpLine = 0x16;
                pad &= ~0x40;
                *p = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
                break;
            }
            gHelpLine = 0x18;
            BoxMenu_InputAmount(pad, 1);
            if (!(b & 0x20)) {
                break;
            }
            pad = 0;
            m = (SellView *)gBoxWork;
            m->price = m->amount * gItemData[save->itemBox[m->slot[m->kind]].item].value;
            ((SellView *)gBoxWork)->sub++;
            gBoxWork->yesNo = 1;
            Sound_PlaySeFlat(0, 0x13, 0);
        case 2:
            c = pad;
            r = 0;
            if (c & 0x40) {
                r = -1;
            } else {
                gBoxWork->yesNo = YesNo_Toggle(pad, gBoxWork->yesNo, 0xC00);
                if (c & 0x20) {
                    r = gBoxWork->yesNo == 0 ? 1 : -1;
                }
            }
            gHelpLine = 0x19;
            if (r != 0) {
                if (r < 0) {
                    if (((SellView *)gBoxWork)->kind == 0 && BoxMenu_CanSplit() == 1) {
                        gHelpLine = 0x18;
                        ((SellView *)gBoxWork)->sub = 1;
                    } else {
                        m = (SellView *)gBoxWork;
                        gHelpLine = m->kind + 0x16;
                        m->sub = 0;
                    }
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else {
                    m = (SellView *)gBoxWork;
                    if (m->kind == 0) {
                        if (gItemData[save->itemBox[m->slot[m->kind]].item].max == 0xFF) {
                            save->itemBox[m->slot[m->kind]].item = 0;
                            m = (SellView *)gBoxWork;
                            save->itemBox[m->slot[m->kind]].count = 0;
                        } else {
                            save->itemBox[m->slot[m->kind]].count -= m->amount;
                            m = (SellView *)gBoxWork;
                            if (save->itemBox[m->slot[m->kind]].count == 0) {
                                save->itemBox[m->slot[m->kind]].item = 0;
                            }
                        }
                    } else {
                        EquipSet_RemoveSlot(m->slot[m->kind], 0);
                        m = (SellView *)gBoxWork;
                        Box_ClearSlot(save, m->slot[m->kind]);
                    }
                    Save_AddMoney(((SellView *)gBoxWork)->price);
                    m = (SellView *)gBoxWork;
                    gHelpLine = m->kind + 0x16;
                    m->sub = 0;
                    Sound_PlaySeFlat(1, 0x83, 0);
                }
                pad = 0;
                gBoxWork->yesNo = 0xFF;
            }
            break;
        }
        break;
    }
    return pad;
}

s32 ItemBox_SlotHasItem(u16 slot)
{
    u16 item = gSave.itemBox[slot].item;

    if (item == 0) {
        return 0;
    }
    if (gItemData[item].icon != 0xFF) {
        return 1;
    }
    return 0;
}

s32 EquipBox_TryRemove(u16 slot)
{
    SaveData *save = &gSave;
    s32 s = slot;
    EquipItem *e = &save->box[s];

    if (e->used == 0) {
        return -1;
    }
    if (s == save->equipIdx[1] || s == save->equipIdx[2] || s == save->equipIdx[3] ||
        s == save->equipIdx[4] || s == save->equipIdx[5] || s == save->equipIdx[0]) {
        return 0;
    }
    EquipSet_RemoveSlot(slot, 1);
    return 1;
}

/* static: BoxMenu_CanTakeSeveral keeps gSave in t0 across the call to it */
static s32 BoxMenu_CanSplit(void)
{
    SaveData *save = &gSave;
    u16 slot = gBoxWork->slots[0];
    s32 r;

    if (gItemData[gSave.itemBox[slot].item].max == 0xFF) {
        r = 0;
    } else if (gItemData[save->itemBox[slot].item].max == 1) {
        r = 0;
    } else {
        r = save->itemBox[slot].count != 1;
    }
    return r;
}

s32 BoxMenu_CanTakeSeveral(void)
{
    SaveData *save = &gSave;
    s32 r;

    if (BoxMenu_CanSplit() == 0) {
        r = 0;
    } else {
        u8 i = gBoxWork->pouch;
        r = gItemData[save->pouch[i].item].max - save->pouch[i].count >= 2;
    }
    return r;
}

/* The low byte of gSave.itemBox[k].count as its own symbol (the field form
 * changes the function). */

/* The amount to move with the pad: 0x800 one, 0x400 the most, 0x2000 up,
 * 0x1000 down (at most the stack, or the room in the pouch unless noLimit). */
void BoxMenu_InputAmount(u16 pad, u8 noLimit)
{
    SaveData *save = &gSave;
    u8 max = ((u8 *)&gSave.itemBox[0].count)[gBoxWork->slots[0] * sizeof(ItemStack)];
    s16 n;
    s16 *p;
    s32 b;

    if (noLimit == 0) {
        u8 i = gBoxWork->pouch;
        s32 room = gItemData[save->pouch[i].item].max - save->pouch[i].count;

        if (room < max) {
            max = room;
        }
    }
    b = pad;
    gBoxWork->atMax = 0;
    p = &gBoxWork->amount;
    n = *p;
    if (b & 0x800) {
        *p = 1;
    } else if (b & 0x400) {
        *p = max;
    } else if (b & 0x2000) {
        if (*p >= max) {
            *p = max;
            n = -1;
        } else {
            (*p)++;
        }
    } else if (b & 0x1000) {
        if (*p > 1) {
            (*p)--;
        } else {
            n = -1;
        }
    }
    if (n < 0) {
        Sound_PlaySeFlat(0, 0x15, 0);
    } else if (n != gBoxWork->amount) {
        Sound_PlaySeFlat(0, 0x17, 0);
    }
    if (gBoxWork->amount >= max) {
        gBoxWork->atMax = 1;
    }
}

void Equip_SwapSlots(u16 a, u16 b)
{
    SaveData *save = &gSave;
    EquipSet *set;
    u32 i;
    u32 j;
    s32 s = a;

    if (s == gSave.equipIdx[1]) {
        save->equipIdx[1] = b;
    } else if (b == save->equipIdx[1]) {
        save->equipIdx[1] = a;
    }
    if (s == save->equipIdx[2]) {
        save->equipIdx[2] = b;
    } else if (b == save->equipIdx[2]) {
        save->equipIdx[2] = a;
    }
    if (s == save->equipIdx[3]) {
        save->equipIdx[3] = b;
    } else if (b == save->equipIdx[3]) {
        save->equipIdx[3] = a;
    }
    if (s == save->equipIdx[4]) {
        save->equipIdx[4] = b;
    } else if (b == save->equipIdx[4]) {
        save->equipIdx[4] = a;
    }
    if (s == save->equipIdx[5]) {
        save->equipIdx[5] = b;
    } else if (b == save->equipIdx[5]) {
        save->equipIdx[5] = a;
    }
    if (s == save->equipIdx[0]) {
        save->equipIdx[0] = b;
    } else if (b == save->equipIdx[0]) {
        save->equipIdx[0] = a;
    }
    set = save->equipSets;
   for (i = 10; i != 0; i--, set++) {
        for (j = 0; j < 6; j++) {
            if (s == set->idx[j]) {
                set->idx[j] = b;
            } else if (b == set->idx[j]) {
                set->idx[j] = a;
            }
        }
    }
}

/* The box menu input (item box and equipment box pages). */
u16 BoxMenu_Input(u16 pad)
{
    ItemBoxMenu *m = gBoxWork;
    s32 c;

    if (((u8 *)m)[9] == 0) {
        s32 b;
        u8 *p;

        b = pad;
        if (b & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            return 0x40;
        }
        if (b & 0x200) {
            p = &m->help;
            if (*p != 0) {
                *p = 0;
            } else {
                *p = 1;
                gHelpColumn = 0;
            }
            Sound_PlaySeFlat(0, 9, 0);
        }
        BoxMenu_MoveCursor(&gBoxWork->slots[0], pad);
        if (gBoxWork->help != 0) {
            PouchMenu_DecoCursor(gSave.itemBox[gBoxWork->slots[0]].item, pad);
        }
    } else {
        u8 *p;
        s32 b;

        p = &m->help;
        if (*p != 0) {
            b = pad;
            if (b & 0x20) {
                *p = 0;
                return 0x20;
            }
            if (b & 0x240) {
                *p = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else {
                Menu_CursorLeftRight(&m->levelSel, b & 0xC00, m->levelCount);
            }
            return 0;
        }
        c = pad;
        if (c & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            return 0x40;
        }
        BoxMenu_MoveCursor(&m->slots[1], pad);
        if (!(c & 0x20) && (c & 0x200)) {
            gBoxWork->boxItem = &gSave.box[gBoxWork->slots[1]];
            m = gBoxWork;
            if (m->boxItem->used != 0) {
                m->help = 0x81;
                gBoxWork->levelSel = 0;
                gBoxWork->levelCount = Equip_LevelMark((struct EquipItem *)gBoxWork->boxItem, 0, (struct EquipItem *)(0));
                Sound_PlaySeFlat(0, 0x11, 0);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
            return 0;
        }
    }
    return pad;
}

s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
void Box_ApplySort(u8 equip);

/* ItemBoxMenu seen by BoxMenu_SortPrompt: the ItemBoxMenu form changes the code (MWCC reuses the load of gBoxWork) */
typedef struct BoxCursors {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ u16 slot[2];
} BoxCursors;

u16 BoxMenu_SortPrompt(u16 pad, u8 kind)
{
    u8 *sub;
    BoxCursors *c;

    if (gBoxWork->prompt != 0) {
        sub = &gBoxWork->yesNo;
        if (*sub == 0) {
            if (pad & 0x20) {
                Box_ApplySort(kind);
                Sound_PlaySeFlat(0, 0x13, 0);
                c = (BoxCursors *)gBoxWork;
                c->slot[kind] = c->slot[kind] % 100;
                gBoxWork->prompt = 0;
                gBoxWork->yesNo = 0xFF;
            } else {
                gHelpLine = 0x38;
                if (pad & 0x40) {
                    *sub = 1;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else if (pad & 0x400) {
                    *sub = 1;
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            }
        } else if (pad & 0x60) {
            gBoxWork->prompt = 0;
            gBoxWork->yesNo = 0xFF;
            Sound_PlaySeFlat(0, 0x14, 0);
        } else {
            gHelpLine = 0x38;
            if (pad & 0x800) {
                *sub = 0;
                Sound_PlaySeFlat(0, 0x17, 0);
            }
        }
        return 0;
    }
    if (pad & 0x4000) {
        gBoxWork->prompt = 1;
        gBoxWork->yesNo = 0;
        gBoxWork->help = 0;
        gHelpLine = 0x38;
        Sound_PlaySeFlat(0, 0xF, 0);
        return 0;
    }
    return pad;
}

s32 Equip_ArmorFits(EquipItem *e, u16 *slot)
{
    SaveData *save;
    u8 flags;
    u8 bit;

    if (e->type == 6 || e->type == 7) {
        return 1;
    }
    save = &gSave;
    flags = Equip_GetData(e)->armor.flags;
    bit = (*(u8 *)((u8 *)&gSave + 0x1) == 0) ? 1 : 2;
    if (!(flags & bit)) {
        return 0;
    }
    bit = (save->box[*slot].type == 6) ? 4 : 8;
    if (!(flags & bit)) {
        return 0;
    }
    return 1;
}

/* unk_760C70's .data (0x7F19D0-0x7F1A08) */
Pos16 gLabelIconOffsets[12] = {
#include "lobby/007F19D0.inc"
};
u8 gBoxPartIdx[8] = {
#include "lobby/007F1A00.inc"
};
