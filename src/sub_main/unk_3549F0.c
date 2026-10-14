#include "common.h"
#include "grid.h"
#include "item.h"
#include "pad.h"
#include "player.h"
#include "save.h"
#include "scene.h"
#include "sprite.h"


typedef struct Vec2s {
    s16 x;
    s16 y;
} Vec2s;

/* gIconUVs: texture coordinates of an icon (8 bytes). */
typedef struct UVRect {
    Vec2s a;
    Vec2s b;
} UVRect;

/* A sprite as Sprite_Draw draws it (include/sprite.h, with the UVs as Vec2s pairs). */
typedef struct SpriteUV {
    s16 x;          /* 0x00 */
    s16 y;          /* 0x02 */
    s16 w;          /* 0x04 */
    s16 h;          /* 0x06 */
    u32 color;      /* 0x08: 0xAABBGGRR */
    Vec2s uv0;      /* 0x0C */
    Vec2s uv1;      /* 0x10 */
} SpriteUV;


/* A line or bar as Prim_FillRect draws it. */
typedef struct Line {
    s16 x0;         /* 0x0 */
    s16 y0;         /* 0x2 */
    s16 x1;         /* 0x4 */
    s16 y1;         /* 0x6 */
    u32 color;      /* 0x8 */
} Line;

/* An action spot (Player.spot). */
typedef struct Spot {
    u16 unk0;
    u16 kind;       /* 0x2 */
} Spot;

s32 Tex_LoadPack(s32 id, s32 slot, s32 kind);
s32 func_723BC0(Player *p);
s32 Item_IsStealable(u16 id); /* unit's own declaration: the definition (game) is s32 Item_IsStealable(s32 arg0); the calls here match only with this one */
s32 func_003648F0(void);
s32 Item_CanPlaceLargeBarrelBomb(void);
s32 Item_CanUseSmallBarrelBomb(void);
s32 Item_CanPlaceKind2(void);
s32 func_00364A70(void);
s32 Item_CanPlaceKind1(void);
s32 Scene_FewPlaced3_Unk(void);
s32 Player_LocalAreaOk_Unk(void);
s32 Item_UsableOutsideCamp(void);
s32 Quest_IsVersus(void);
s16 Player_GetAreaAmbModel(Player *p);
s32 Pouch_HasItem28(Player *p);
u16 Player_GetGatherPoint(Player *p, u16 *id, f32 *pos);
s32 SpecialMine_IsFree(Player *unused0);
u32 Player_TestFlag(void *obj, u32 flag);
f32 Math_Sin(f32 x);
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);
void Gfx_SetState63(s32 a);
void Gfx_SetBlend(s32 a, s32 b);
void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32 a);
s32 Sprite_Draw(struct Sprite *spr);
void Tex_LoadFile(s32 a, s32 b, s32 c);
void Tex_LoadStageFile(s32 file, s32 unused, s32 slot, s32 kind);
s32 Scene_RequestPause(s32 mode);
void Stream_Pause(s32 a, s32 b);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 Save_TestEventFlag(s32 n);
u8 Player_GetSkill(Player *p, u8 group);
s16 Player_IsState(void *obj, u8 state, u8 sub);
s16 Pouch_Count(Player *p, u16 item);
void Text_Draw(char *s);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);
void Pouch_MoveCursor(u8 *cur, u16 buttons);
void PouchMenu_DecoCursor(u16 item, u16 buttons);
s32 func_00355A70(void);
s16 Pouch_FixCursor(void);
s32 Scene_InField(void);
struct SortPair;
void Sort_PairsDesc(struct SortPair *a, u32 n);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Bowgun_LoadAmmo(Player *p, s32 a);
void Save_RestorePouch(Player *p);
s32 Map_IsMaplessArea(u8 stage, u8 area);

extern s32 gSystemTexFiles[];
extern u16 gFrameTick;
extern UVRect gIconUVs[];
extern u32 gEquipColors[];
extern u8 D_0043AB38[];
extern u8 gRarityColors[];
extern s16 gHelpLine;
extern u8 gHelpColumn;
extern u8 gChatInputMode;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;

void Hud_LoadTextures(void)
{
    Tex_LoadPack(gSystemTexFiles[1], 0x144, 0);
    if (gScene.online == 0) {
        Tex_LoadStageFile(gScene.map, gScene.questId, 0x145, 0);
        Tex_LoadFile(2, 0x146, 0);
    } else {
        Tex_LoadFile(9, 0x168, 0);
        Tex_LoadFile(10, 0x169, 0);
        Tex_LoadFile(11, 0x16A, 0);
        Tex_LoadFile(12, 0x16B, 0);
        Tex_LoadFile(13, 0x16C, 0);
    }
    Tex_LoadFile(7, 0x147, 0);
}

void Game_Pause(u8 arg0)
{
    Scene_RequestPause(1);
    gChatWinOpen = gChatWinTimer = 0;
    if (arg0 == 1) {
        Stream_Pause(1, 1);
        Stream_Pause(0, 1);
    }
}

void Game_Resume(u8 arg0)
{
    Scene_RequestPause(0);
    if (arg0 == 1) {
        Stream_Pause(1, 0);
        Stream_Pause(0, 0);
    }
}

s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count)
{
    u8 n;
    u8 old;

    if (count < 2) {
        *cur = 0;
        return 0;
    }
    old = *cur;
    n = old;
    if (buttons & 0x2000) {
        if (old == 0) {
            n = count - 1;
        } else {
            n = old - 1;
        }
    } else if (buttons & 0x1000) {
        n = old + 1;
        if (n >= count) {
            n = 0;
        }
    }
    if (n != *cur) {
        *cur = n;
        Sound_PlaySeFlat(0, 0x17, 0);
        return 1;
    }
    return 0;
}

s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count)
{
    u8 n;
    u8 old;

    if (count < 2) {
        *cur = 0;
        return 0;
    }
    old = *cur;
    n = old;
    if (buttons & 0x808) {
        if (old == 0) {
            n = count - 1;
        } else {
            n = old - 1;
        }
    } else if (buttons & 0x480) {
        n = old + 1;
        if (n >= count) {
            n = 0;
        }
    }
    if (n != *cur) {
        *cur = n;
        Sound_PlaySeFlat(0, 0x17, 0);
        return 1;
    }
    return 0;
}

void Menu_GridCursor(Grid *g, u16 buttons)
{
    s16 col;
    s16 row;
    s16 n;

    row = g->row;
    col = g->col;

    if ((buttons & 0xC00) && g->cols > 1) {
        if (buttons & 0x800) {
            col--;
            if (col < 0) {
                col = g->cols - 1;
            }
        } else {
            col++;
            if (col >= g->cols) {
                col = 0;
            }
        }
        if (col == g->cols - 1 && row >= g->lastRows) {
            row = g->lastRows - 1;
            g->row = row;
        }
        g->col = col;
        Sound_PlaySeFlat(0, 0x11, 0);
    } else if (buttons & 0x3000) {
        if (col >= g->cols - 1) {
            n = g->lastRows;
        } else {
            n = g->rows;
        }
        if (n > 1) {
            if (buttons & 0x2000) {
                row--;
                if (row < 0) {
                    row = n - 1;
                }
            } else {
                row++;
                if (row >= n) {
                    row = 0;
                }
            }
            Sound_PlaySeFlat(0, 0x17, 0);
        }
        g->row = row;
    }
    g->index = row + g->rows * col;
}

u16 PouchMenu_MoveCursor(u8 *cur, u16 buttons)
{
    u16 item;

    Pouch_MoveCursor(cur, buttons);
    item = gPlayers[gScene.localPlayer].pouch[*cur].item;
    gHelpLine = item + 0x18;
    PouchMenu_DecoCursor(item, buttons & 0x80);
    return item;
}

void Pouch_MoveCursor(u8 *cur, u16 buttons)
{
    if (*cur < 10) {
        Menu_CursorUpDown(cur, buttons, 10);
    } else {
        *cur -= 10;
        Menu_CursorUpDown(cur, buttons, 10);
        *cur += 10;
    }
    if (buttons & 0xC00) {
        if (buttons & 0x800) {
            if (*cur < 10) {
                *cur += 20;
            }
            *cur -= 10;
        } else {
            *cur += 10;
            if (*cur >= 20) {
                *cur -= 20;
            }
        }
        Sound_PlaySeFlat(0, 0x11, 0);
    }
}

u8 PhraseMenu_MoveCursor(u16 buttons, u8 cur)
{
    if (cur < 6) {
        Menu_CursorUpDown(&cur, buttons, 6);
    } else if (cur < 12) {
        cur -= 6;
        Menu_CursorUpDown(&cur, buttons, 6);
        cur += 6;
    } else if (cur < 18) {
        cur -= 12;
        Menu_CursorUpDown(&cur, buttons, 6);
        cur += 12;
    } else {
        cur -= 18;
        Menu_CursorUpDown(&cur, buttons, 6);
        cur += 18;
    }
    if (buttons & 0xC00) {
        if (buttons & 0x800) {
            if (cur < 6) {
                cur += 24;
            }
            cur -= 6;
        } else {
            cur += 6;
            if (cur >= 24) {
                cur -= 24;
            }
        }
        Sound_PlaySeFlat(0, 0x11, 0);
    }
    return cur;
}

void PouchMenu_DecoCursor(u16 item, u16 buttons)
{
    if (gItemData[item].deco != 0 && item != 0) {
        Menu_CursorLeftRight(&gHelpColumn, buttons & 0x88, 3);
    }
}

u8 YesNo_Toggle(u16 buttons, u8 on, u16 mask)
{
    buttons = buttons & mask;
    if (on == 0) {
        if (buttons & 0x1400) {
            on = 1;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
    } else {
        if (buttons & 0x2800) {
            on = 0;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
    }
    return on;
}

u16 Pad_StickToDpad(u8 pad)
{
    u16 a;

    if (gPadState[pad].magR <= 0x40) {
        return 0;
    }
    a = gPadState[pad].degR + 0x1200;
    if (a < 0x2400) {
        return 0x400;
    }
    if (a < 0x4000) {
        return 0x2400;
    }
    if (a < 0x6400) {
        return 0x2000;
    }
    if (a < 0x8000) {
        return 0x2800;
    }
    if (a < 0xA400) {
        return 0x800;
    }
    if (a < 0xC000) {
        return 0x1800;
    }
    if (a < 0xE400) {
        return 0x1000;
    }
    return 0x1400;
}

s32 Chat_InInputMode(void)
{
    return gChatInputMode != 0;
}

s32 Pouch_IsSlotUsable(Player *p, u16 slot)
{
    if (p->pouch[slot].count > 0) {
        s16 item = p->pouch[slot].item;
        if (item != 0 && (gItemData[item].flags & 1)) {
            return 1;
        }
    }
    return 0;
}

s32 Item_IsUsable(u16 id)
{
    Player *p = &gPlayers[gScene.localPlayer];
    u8 buf[12];
    s16 kind;
    u8 k;

    if (func_723BC0(p) == 1 && Item_IsStealable(id) == 1) {
        return 0;
    }
    k = p->rideState;
    if (k == 1) {
        if (id == 0x61) {
            return func_003648F0();
        }
        return 0;
    }
    switch (id) {
    case 0x54:
    case 0x299:
        if (Player_GetAreaAmbModel(p) < 0) {
            return 0;
        }
        break;
    case 0x2A6:
        if (p->spot == NULL) {
            return 0;
        }
        if (((Spot *)p->spot)->kind != 0xB) {
            return 0;
        }
        break;
    case 0x55:
    case 0x296:
        return Item_CanPlaceKind2();
    case 0x56:
        return func_00364A70();
    case 0x9C:
    case 0x294:
    case 0x9D:
    case 0x9E:
    case 0x9F:
    case 0xA0:
        return Pouch_HasItem28(p);
    case 0xA2:
    case 0x2F7:
        return 0;
    case 0x2F8:
    case 0x2F9:
    case 0x30A:
    case 0x30B:
    case 0x30D:
    case 0x2FA:
    case 0x2FB:
    case 0x2FC:
    case 0x2FF:
    case 0x300:
    case 0x301:
    case 0x306:
    case 0x307:
    case 0x308:
    case 0x309:
    case 0x314:
    case 0x318:
    case 0x30F:
    case 0x310:
    case 0x311:
    case 0x312:
    case 0x315:
    case 0x316:
    case 0x317:
    case 0x30C:
    case 0x319:
    case 0x313:
        return SpecialMine_IsFree(p);
    case 0xAC:
    case 0x295:
    case 0xEA:
    case 0xF2:
        if (p->isGunner == 1) {
            return 0;
        }
        break;
    case 0x5B:
    case 0x5C:
    case 0x5F:
    case 0x60:
        return Item_CanUseSmallBarrelBomb();
    case 0x5D:
    case 0x5E:
    case 0x29A:
        return Item_CanPlaceLargeBarrelBomb();
    case 0x62:
        return Item_CanPlaceKind1();
    case 0x61:
        if (k != 1) {
            return 0;
        }
        if (func_003648F0() != 1) {
            return 0;
        }
        break;
    case 0x28:
    case 0x2C:
    case 0x2E:
    case 0x2D:
        return Scene_FewPlaced3_Unk();
    case 0x96:
    case 0x97:
    case 0x98:
        if (Player_GetGatherPoint(p, (u16 *) &kind, (f32 *) buf) == 0xFFFF) {
            return 0;
        }
        if (kind != 3) {
            return 0;
        }
        break;
    case 0x99:
    case 0x9A:
    case 0x9B:
        if (Player_GetGatherPoint(p, (u16 *) &kind, (f32 *) buf) == 0xFFFF) {
            return 0;
        }
        if (kind != 4) {
            return 0;
        }
        break;
    case 0x29B:
        if (p->spot == NULL) {
            return 0;
        }
        if (((Spot *)p->spot)->kind != 6) {
            return 0;
        }
        break;
    case 0x2B7:
        if (p->spot == NULL) {
            return 0;
        }
        if (((Spot *)p->spot)->kind != 0xA) {
            return 0;
        }
        break;
    case 0x4E:
        return Item_UsableOutsideCamp();
    case 0xAD:
        return Player_LocalAreaOk_Unk();
    case 0x45:
    case 0x50:
        return Quest_IsVersus() == 0;
    default:
        if (gItemData[id].icon == 4 && Player_TestFlag(p, 0x80000) == 0) {
            if (p->spot == NULL) {
                return 0;
            }
            if (((Spot *)p->spot)->kind != 2) {
                return 0;
            }
        }
        break;
    }
    return 1;
}

s32 Player_IsFree(Player *p)
{
    return p->freeState != 0;
}

s32 func_00355A70(void)
{
    SaveData *s = &gSave;

    if (Save_TestEventFlag(0xB6) != 1) {
        return 0;
    }
    if ((s->rateFlags & 0xFFF) != 0) {
        return 1;
    }
    return 0;
}

s32 StatusPage_HasSymbolColor(void)
{
    SaveData *s = &gSave;

    if (func_00355A70() != 1 && s->styleBits == 0) {
        return 0;
    }
    return 1;
}

s16 Pouch_FixCursor(void)
{
    Player *p = &gPlayers[gScene.localPlayer];
    s32 n;
    u32 i = p->pouchCursor;

    if (p->pouch[i].item != 0) {
        return i;
    }
    n = 20;
    do {
        if (p->pouch[i].item != 0) {
            p->pouchCursor = i;
            return i;
        }
        i++;
        if (i >= 20) {
            i = 0;
        }
    } while (--n != 0);
    return -1;
}

/* The inner loop's count goes through i (i = left - 1; n = i): with n = left - 1, cur.item,
 * the pointer and i rotate a0-a2 in the first search loop. */
void Pouch_Compact(u8 box)
{
    Player *p = &gPlayers[gScene.localPlayer];
    ItemStack *items;
    ItemStack *a;
    ItemStack *b;
    ItemStack cur;
    ItemStack ammo;
    ItemStack coat;
    s32 left;
    s32 n;
    s32 count;
    u32 i;
    u8 loaded;
    u8 coatSlot;

    if (box == 0) {
        items = p->pouch;
    } else {
        items = gSave.pouch;
    }
    if (Scene_InField() == 1) {
        if (Pouch_FixCursor() < 0) {
            return;
        }
        p->ammoSlot = Bowgun_NextAmmoSlot(p, p->ammoSlot, 2);
        loaded = p->roundsLoaded;
        coatSlot = p->coatingSlot;
        Bowgun_LoadAmmo(p, 0);
        p->roundsLoaded = loaded;
        p->coatingSlot = coatSlot;
        cur = p->pouch[p->pouchCursor];
        if (p->ammoSlot != 0xFF) {
            ammo = p->pouch[p->ammoSlot];
        }
        if (p->coatingSlot != 0xFF) {
            coat = p->pouch[p->coatingSlot];
        }
    }
    n = 20;
    a = items;
    count = 0;
    for (left = n; left != 0 && n != 0; left--, a++) {
        if (a->item != 0) {
            count++;
        } else {
            i = left - 1;
            for (n = i, b = a + 1; n != 0; n--, b++) {
                if (b->item != 0) {
                    count++;
                    *a = *b;
                    b->count = 0;
                    b->item = 0;
                    break;
                }
            }
        }
    }
    Sort_PairsDesc((struct SortPair *)items, count);
    if (Scene_InField() == 1) {
        if (box == 0) {
            for (a = p->pouch, i = 0; i < 20; i++, a++) {
                if (a->item == cur.item) {
                    p->pouchCursor = i;
                    break;
                }
            }
            if (p->ammoSlot != 0xFF) {
                for (a = p->pouch, i = 0; i < 20; i++, a++) {
                    if (a->item == ammo.item) {
                        p->ammoSlot = i;
                        break;
                    }
                }
            }
            if (p->coatingSlot != 0xFF) {
                for (a = p->pouch, i = 0; i < 20; i++, a++) {
                    if (a->item == coat.item) {
                        p->coatingSlot = i;
                        break;
                    }
                }
            }
        }
    } else if (box != 0) {
        Save_RestorePouch(p);
    }
}

s32 func_00355E90(Player *p)
{
    if (p->bowgunReady != 0) {
        return 1;
    }
    if ((Player_IsState(p, 0, 0x36) != 0 || Player_IsState(p, 0, 0x7A) != 0) && Pouch_Count(p, 0x29B) != 0) {
        return 1;
    }
    return 0;
}

void ListPanel_DrawCursorBar(s32 x0, s32 x1, s16 top, s16 step, s32 i, s32 rgb)
{
    Line l;
    u16 t;

    l.x1 = x1;
    l.x0 = x0;
    l.y0 = top - 2 + step * i;
    l.y1 = l.y0 + step + 2;
    t = (gFrameTick & 0x3F) << 10;
    l.color = (rgb & 0xFFFFFF) | (((s8)(48.0f * Math_Sin((6.2831855f / 65536.0f) * t)) + 0xBF) << 24);
    Prim_FillRect((struct PrimLine *)&l);
}

void Draw_Icons(Icon *icons, u8 count)
{
    SpriteUV spr;

    Gfx_SetState63(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    for (; count > 0; count--) {
        spr.x = icons->x;
        spr.y = icons->y;
        spr.w = icons->size;
        spr.h = icons->size;
        spr.uv0 = gIconUVs[icons->kind].a;
        spr.uv1 = gIconUVs[icons->kind].b;
        spr.color = (icons->alpha << 24) | 0xFFFFFF;
        Sprite_Draw((struct Sprite *)&spr);
        icons++;
    }
}

u32 Equip_Color(u8 idx, u8 alpha, s32 alt)
{
    if ((u8)alt) {
        idx += 7;
    }
    return (alpha << 24) | gEquipColors[D_0043AB38[idx]];
}

s16 Rarity_Color(u8 i)
{
    return gRarityColors[i];
}

s32 Player_HasMap(Player *p)
{
    u8 s = Player_GetSkill(p, 0x23);

    if (s != 0) {
        if (s == 0x91) {
            return 1;
        }
        return 0;
    }
    return Pouch_Count(p, 0x291) != 0;
}

u8 Player_DetectLevel(Player *p)
{
    u8 r = 0;
    u8 s = Player_GetSkill(p, 0x1B);

    if (s != 0) {
        if (s == 0xA2) {
            r = 4;
        } else {
            r = 2;
        }
    }
    if (((u16)p->detectTimer | (u16)p->actDetectTimer) != 0) {
        r |= 1;
    }
    return r;
}

s32 Player_InMaplessArea(Player *p)
{
    return Map_IsMaplessArea(gScene.map, p->area);
}

s32 Map_IsMaplessArea(u8 stage, u8 area)
{
    switch (stage) {
    case 13:
    case 14:
    case 15:
        switch (area) {
        case 0x79:
        case 0x7A:
        case 0x7B:
            return 1;
        default:
            return 2;
        }
    default:
        return 0;
    }
}

void Text_DrawDigit(u8 n)
{
    char buf[3];

    buf[0] = 0x82;
    buf[1] = n + 0x4F;
    buf[2] = 0;
    Text_Draw(buf);
}
