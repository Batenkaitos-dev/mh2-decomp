#include "common.h"
#include "sprite.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "stageobj.h"
#pragma optimization_level 4

typedef struct IdPair {
    s32 a;
    s32 b;
} IdPair;

extern IdPair gRecipeIdPairs[];
void DrinkMug_Init(VObj *o);
void DrinkMug_Step(VObj *o);
void DrinkMug_End(VObj *o);
void DrinkMug_Free(StageObj *o);
/* sub_main's armour data (0x28 bytes; ArmorData in sub_main's unk_3B4C80.c) */
/* The materials of an upgrade (16 bytes): an item, then its count and a condition per level. */
typedef struct UpgradeRow {
    /* 0x0 */ u16 item;
    /* 0x2 */ u8 lv[7][2];
} UpgradeRow;

extern UpgradeRow *gArmorUpgradeRows[];
#include "save.h"
void *memset(void *, s32, u32);
s32 Save_IsItemObtained(SaveData *save, u16 item);
extern s16 gMenuWork;
void PlaceInfo_DrawAt(s32, s32);

void PlaceInfo_Draw(void)
{
    PlaceInfo_DrawAt(gMenuWork, 0x3A);
}

s32 Recipe_IsInList(s32 a, s32 b)
{
    IdPair *p = gRecipeIdPairs;
    s32 v = gRecipeIdPairs[0].b;

    while (v != 0) {
        if (a == p->a && b == v) {
            return 1;
        }
        p++;
        v = p->b;
    }
    return 0;
}

#include "item.h"
s32 Forge_CheckCondition(u8 cond); /* unit's own declaration: with the definition's s32 (s16 cond) (sub_main/unk_3B4C80.c), WeaponForge_CheckWeapon 3 of 27 words differ */
s32 WeaponForge_CheckWeapon(s32 kind, s32 id)
{
    s32 c;
    BladeData *b;
    GunData *g;

    switch (kind) {
    case 6:
        b = &gBladeData[id];
        c = b->unk14;
        break;
    case 7:
        g = &gGunData[id];
        c = g->unk13;
        break;
    }
    return Forge_CheckCondition(c);
}

static ArmorData *Equip_ArmorData(EquipItem *e)
{
    switch (e->type) {
    case 2:
        return &gArmorHeadData[e->id];
    case 3:
        return &gArmorBodyData[e->id];
    case 4:
        return &gArmorArmsData[e->id];
    case 5:
        return &gArmorWaistData[e->id];
    case 0:
        return &gArmorLegsData[e->id];
    default:
        return NULL;
    }
}

s32 ArmorUpgrade_Materials(EquipItem *e, ItemStack *out, s8 *cond);

s32 ArmorUpgrade_Unseen(EquipItem *e)
{
    ItemStack need[7];
    s8 cond[4];
    ArmorData *a;
    s32 n;
    s32 i;

    a = Equip_ArmorData(e);
    if (a == NULL) {
        return 1;
    }
    if (a->maxLevel <= e->level + 1) {
        return 1;
    }
    n = ArmorUpgrade_Materials(e, need, cond);
    for (i = 0; i < n; i++) {
        if (cond[i] != 0 && Save_IsItemObtained(&gSave, need[i].item) == 0) {
            return 1;
        }
    }
    return 0;
}

s32 ArmorUpgrade_Materials(EquipItem *e, ItemStack *out, s8 *cond)
{
    UpgradeRow *r;
    u16 lv;
    u16 item;
    s32 n;
    ArmorData *a;

    a = Equip_ArmorData(e);
    if (a == NULL) {
        return 0;
    }
    memset(out, 0, 0x10);
    item = e->level + 1; /* (item holds the level first: the permuter found this form) */
    r = gArmorUpgradeRows[a->upgrade];
    lv = item;
    n = 0;
    item = r->item;
    while (item != 0) {
        if (r->lv[lv][0] != 0) {
            out->item = item;
            n++;
            out->count = r->lv[lv][0];
            out++;
            *cond = r->lv[lv][1];
            cond++;
        }
        r++;
        item = r->item;
    }
    return n;
}

s32 ArmorUpgrade_FirstMaterials(EquipItem *e, ItemStack *out, s8 *cond)
{
    ArmorData *a;
    UpgradeRow *r;
    u16 item;
    s32 n;

    a = Equip_ArmorData(e);
    if (a == NULL) {
        return 0;
    }
    memset(out, 0, 0x10);
    r = gArmorUpgradeRows[a->upgrade];
    item = r->item;
    n = 0;
    while (item != 0) {
        if (r->lv[0][0] != 0) {
            out->item = item;
            n++;
            out->count = r->lv[0][0];
            out++;
            *cond = r->lv[0][1];
            cond++;
        }
        r++;
        item = r->item;
    }
    return n;
}

s32 ArmorUpgrade_Unlocks(EquipItem *e, u16 *out)
{
    ArmorData *a;
    s32 n;

    a = Equip_ArmorData(e);
    if (a == NULL) {
        return 0;
    }
    n = 0;
    if ((u16)(e->level + 1) >= a->unk17) {
        if (a->unk18[0] != 0) {
            n++;
        }
        if (out != NULL) {
            *out++ = a->unk18[0];
        }
        if (a->unk18[1] != 0) {
            n++;
        }
        if (out != NULL) {
            *out++ = a->unk18[1];
        }
        if (a->unk18[2] != 0) {
            n++;
        }
        if (out != NULL) {
            *out = a->unk18[2];
        }
    }
    return n;
}

s32 Items_DrawNeedLine(s16, s16, u16, s32, u32);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), ArmorUpgrade_DrawMaterials 84 of 102 words differ */
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
extern char *D_0080C82C;
extern char *D_0080C838;
void ArmorUpgrade_DrawMaterials(s16 x, s16 y, ItemStack *need, s8 n)
{
    s16 i;
    s16 shown;

    Win_Draw(x, y, 0x132, 0x72, 0xBDB59F6C, 0xA0202020, 6);
    y += 6;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0xE, y);
    Text_PrintfColored(D_0080C82C);
    shown = 0;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < n; i++, need++) {
        if (need->item != 0) {
            y += 0x14;
            Items_DrawNeedLine(x + 0x22, y, need->item, need->count, 0x807F7F7F);
            shown++;
        }
    }
    if (shown == 0) {
        Font_SetPos(x + 0x28, y + 0x28);
        Text_PrintfColored(D_0080C838);
    }
}

void DrinkMug_Update(void *o)
{
    u8 state;

    state = *(u8 *)((u8 *)o + 4);
    switch (state) {
    case 0:
        DrinkMug_Init(o);
        break;
    case 1:
        DrinkMug_Step(o);
        break;
    case 2:
        DrinkMug_End(o);
        break;
    case 3:
        DrinkMug_Free(o);
        break;
    }
}

typedef struct V28Target {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1[0xA4 - 1];
    /* 0xA4 */ s32 ang;
    /* 0xA8 */ u8 unkA8[4];
    /* 0xAC */ Vec3f pos;
} V28Target;
typedef struct V28Init {
    /* 0x00 */ f32 *bone;
    /* 0x04 */ V28Target *target;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s8 unkC;
    /* 0x0D */ s8 unkD;
    /* 0x0E */ s8 unkE;
    /* 0x0F */ s8 unkF;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u8 unk12[2];
    /* 0x14 */ f32 unk14;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ Vec3f unk1C;
    /* 0x28 */ Vec3f pos;
    /* 0x34 */ u8 unk34;
    /* 0x35 */ u8 unk35;
    /* 0x36 */ u8 unk36[2];
    /* 0x38 */ f32 unk38;
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ s8 unk40;
    /* 0x41 */ s8 unk41;
    /* 0x42 */ s8 unk42;
    /* 0x43 */ u8 unk43;
    /* 0x44 */ s8 unk44;
    /* 0x45 */ u8 unk45;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ f32 unk48;
    /* 0x4C */ s16 timer;
} V28Init;
struct EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateY(f32 *v, f32 a);
u16 Rand_Next(s32 stream);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void DrinkMug_Draw(DrawSlot *);
void DrinkMug_Init(VObj *o)
{
    V28Init *w = (V28Init *)o->buf;
    Vec3f pos;
    Vec3f d;

    o->var0 = 0;
    o->var1 = 0;
    o->timer = 0;
    o->state++;
    o->visible = 1;
    o->draw = 0;
    Mtx_GetTrans((f32 *) &pos, Em_Bone(w->target, 0xB)->world);
    d.x = 14.0f;
    d.y = 0.0f;
    d.z = 0.0f;
    Vec_RotateY((f32 *) &d, 6.2831855f * (f32)w->target->ang / 65536.0f);
    pos.x += d.x;
    pos.y += d.y;
    pos.z += d.z;
    o->pos[0] = pos.x;
    o->pos[1] = pos.y;
    o->pos[2] = pos.z;
    w->unkD = Rand_Next(1) % 10 + 5;
    w->unkC = 0;
    w->unk18 = 0;
    w->unk1A = w->unk18 + w->target->ang;
    w->unk3C = 6.2831855f * (f32)w->unk1A / 65536.0f;
    w->unk34 = 0;
    w->unk35 = 0;
    w->unk40 = 0;
    w->unk41 = 0;
    w->unk42 = 0;
    w->unk43 = 0;
    w->unk44 = 0;
    w->unk48 = -11.77518f;
    w->unk46 = 65535.0f * w->unk48 / 885.48004f;
    w->unk48 = w->unk48 * -1.0f;
    w->timer = 150;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = DrinkMug_Draw;
    } else {
        StageObj_Free((StageObj *) o);
    }
}

void Em_BonePos(void *t, s16 bone, f32 *out);
f32 Lure_RandSigned(void);
f32 Math_Absf(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *dst, f32 *src);
f32 Vec_Length(f32 *v);
void Vec_Normalize(f32 *v);
s32 DrinkContest_IsOn(void);
f32 Math_Ceil(f32);
void DrinkMug_Step(VObj *o)
{
    V28Init *w = (V28Init *)o->buf;
    Vec3f r;
    Vec3f d;
    Vec3f p;
    Vec3f v;
    f32 len;
    f32 diff;
    V28Target *t;

    Vec_Copy(&w->pos.x, &o->pos[0]);
    if (w->target->unk0 == 0) {
        o->state++;
        return;
    }
    if (DrinkContest_IsOn() == 0 && o->type > 0 && w->unk34 == 0 && w->unk35 == 0) {
        w->unk35 = 1;
        o->type = 5;
        w->unkE = 0;
        w->unkF = 0;
        w->unk41 = 0;
    }
    switch (o->type) {
    case 0:
        if (DrinkContest_IsOn() == 1) {
            o->type = 1;
            w->unkE = 2;
            w->unkF = 9;
        }
        w->bone = (void *)Em_Bone(w->target, 0xB);
        Mtx_GetTrans((f32 *) &o->pos, w->bone);
        d.x = 14.0f;
        d.y = 0.0f;
        d.z = 0.0f;
        Vec_RotateY((f32 *) &d, 6.2831855f * (f32)w->target->ang / 65536.0f);
        o->pos[0] += d.x;
        o->pos[1] += d.y;
        o->pos[2] += d.z;
        switch (o->var0) {
        case 0:
            if (w->unkD-- <= 0) {
                o->var0 = 1;
                w->unk8 = (u16)(s32)(0.5f + 65536.0f * (f32)(Rand_Next(1) % 90) / 360.0f);
                w->unkC = Rand_Next(1) % 3 + 1;
                w->unk10 = w->unk8 - 0x2000;
                w->unk14 = (s16)(w->unk10 - w->unk18) / w->unkC;
            } else {
                w->unk14 = 0.0f;
            }
            break;
        case 1:
            if (w->unkC-- < 2) {
                o->var0 = 0;
                w->unkD = Rand_Next(1) % 10 + 5;
                w->unk14 = 0.0f;
            }
            break;
        }
        w->unk18 += w->unk14;
        w->unk1A = w->unk18 + w->target->ang;
        break;
    case 1:
        if (w->unkF-- <= 0) {
            o->type = 2;
            w->unkE = 0;
            w->unkF = 0;
            w->unk38 = 6.2831855f * (f32)w->target->ang / 65536.0f + 0.34906587f * Lure_RandSigned();
        } else if (w->unkE-- <= 0) {
            w->unkE = 2;
        }
        break;
    case 2:
        if (!(w->unkF++ < 5.0f)) {
            w->unkE = 0;
            w->unkF = 0;
        } else if (!(w->unkE++ < 1.0f)) {
            w->unkE = 0;
        }
        r.x = -1.0f;
        r.y = 0.0f;
        r.z = 1.0f;
        Vec_RotateY((f32 *) &r, w->unk38);
        Vec_Normalize(&r.x);
        w->unk38 += 6.2831855f * (f32)w->unk46 / 65536.0f;
        o->pos[0] += r.x * w->unk48;
        o->pos[1] += 7.0f;
        o->pos[2] += r.z * w->unk48;
        r.x = w->target->pos.x - o->pos[0];
        r.y = 0.0f;
        r.z = w->target->pos.z - o->pos[2];
        if (Vec_Length(&r.x) >= 240.0f) {
            o->type = 4;
            w->unkE = 0;
            w->unkF = 0;
            w->unk41 = 0;
            t = w->target;
            w->unk1C.x = o->pos[0] - t->pos.x;
            w->unk1C.y = 0.0f;
            w->unk1C.z = o->pos[2] - t->pos.z;
            Vec_Normalize(&w->unk1C.x);
            w->unk1C.x *= 240.0f;
            w->unk1C.z *= 240.0f;
        }
        break;
    case 3:
        Em_BonePos(w->target, 0xB, (f32 *)&p);
        if (!(w->unkF++ < 5.0f) || o->pos[1] - 2.0f < p.y) {
            if (w->unk34 == 0) {
                o->type = 4;
                w->unkE = 0;
                w->unkF = 0;
                w->unk41 = 0;
            } else if (o->pos[1] - 2.0f < p.y) {
                o->type = 9;
                w->unkE = 0;
                w->unkF = 0;
            } else {
                w->unkE = 0;
                w->unkF = 0;
            }
        } else if (!(w->unkE++ < 1.0f)) {
            w->unkE = 0;
        }
        if (w->unk34 == 1) {
            if (w->unk41++ < 6) {
                o->pos[1] -= 2.0f;
            } else if (w->unk42++ < 5) {
                o->pos[1] += 0.2f;
            } else {
                w->unk41 = 0;
                w->unk42 = 0;
            }
        } else if (w->unk41++ <= 12.0f) {
            if (w->unk41 <= 6.0f) {
                o->pos[1] += 0.3f;
            } else {
                o->pos[1] -= 0.3f;
            }
        } else {
            w->unk41 = 0;
        }
        break;
    case 4:
    case 5:
        if (!(w->unkF++ < 5.0f)) {
            w->unkE = 0;
            w->unkF = 0;
        } else if (!(w->unkE++ < 1.0f)) {
            w->unkE = 0;
        }
        if (o->type == 5) {
            Mtx_GetTrans((f32 *) &p, Em_Bone(w->target, 0xB)->world);
            d.x = 14.0f;
            d.y = 0.0f;
            d.z = 0.0f;
            Vec_RotateY((f32 *) &d, 6.2831855f * (f32)w->target->ang / 65536.0f);
            p.x += d.x;
            p.y += d.y;
            p.z += d.z;
            v.x = o->pos[0] - p.x;
            v.y = 0.0f;
            v.z = o->pos[2] - p.z;
            len = Vec_Length(&v.x) - 4.0f;
            Vec_Normalize(&v.x);
            v.x *= len;
            v.y = 0.0f;
            v.z *= len;
            Vec_Copy(&w->unk1C.x, &v.x);
            if (len <= 4.0f) {
                w->unk34 = 1;
            }
        }
        if (o->type == 5) {
            if (w->unk34 == 1) {
                Mtx_GetTrans((f32 *) &p, Em_Bone(w->target, 0xB)->world);
                d.x = 14.0f;
                d.y = 0.0f;
                d.z = 0.0f;
                Vec_RotateY((f32 *) &d, 6.2831855f * (f32)w->target->ang / 65536.0f);
                p.x += d.x;
                p.y += d.y;
                p.z += d.z;
                o->pos[0] = p.x;
                o->pos[2] = p.z;
                o->type = 3;
                w->unkE = 0;
                w->unkF = 0;
                w->unk41 = 0;
            } else {
                Vec_RotateY((f32 *) &w->unk1C, -0.049087387f);
                o->pos[0] = p.x + w->unk1C.x;
                o->pos[1] -= 3.0f;
                o->pos[2] = p.z + w->unk1C.z;
            }
        } else {
            Vec_RotateY((f32 *) &w->unk1C, -0.049087387f);
            o->pos[0] = w->target->pos.x + w->unk1C.x;
            o->pos[2] = w->target->pos.z + w->unk1C.z;
        }
        if (w->unk41++ < 6.0f) {
            if (w->unk41 <= 3.0f) {
                o->pos[1] += 0.75f;
            } else {
                o->pos[1] -= 0.75f;
            }
        } else {
            w->unk41 = 0;
        }
        break;
    case 6:
        if (w->unk43-- > 0) {
            w->unk3C += 0.12271847f * (f32)w->unk44;
            if (w->unk40 >= 8.0f) {
                w->unk40 = 0;
            }
            if (w->unk40++ < 8.0f) {
                if (w->unk40 <= 4.0f) {
                    o->pos[1] += 2.0f;
                } else {
                    o->pos[1] -= 2.0f;
                }
            }
        } else {
            Em_BonePos(w->target, 0xB, (f32 *)&p);
            o->pos[1] = p.y;
            o->type = 7;
            o->timer = 20;
        }
        break;
    case 7:
        if (o->timer-- <= 0) {
            o->type = 8;
        }
        break;
    case 8:
        o->type = 0;
        o->var0 = 0;
        o->var1 = 0;
        o->timer = 0;
        w->unkD = Rand_Next(1) % 10 + 5;
        w->unkC = 0;
        w->unk18 = 0;
        w->unk1A = w->unk18 + w->target->ang;
        w->unk34 = 0;
        w->unk35 = 0;
        w->unk40 = 0;
        w->unk41 = 0;
        w->unk42 = 0;
        w->unk43 = 0;
        w->unk44 = 0;
        w->unk48 = -11.77518f;
        w->unk46 = 65535.0f * w->unk48 / 885.48004f;
        w->unk48 = w->unk48 * -1.0f;
        break;
    case 9:
        Em_BonePos(w->target, 0xB, (f32 *)&p);
        if (!(w->unkF++ < 9.0f)) {
            o->type = 6;
            diff = 6.2831855f * (f32)w->target->ang / 65536.0f - w->unk3C;
            w->unk43 = Math_Ceil(Math_Absf(diff) / 0.98174775f);
            w->unk43 *= 8;
            if (diff < 0.0f) {
                w->unk44 = -1;
            } else if (diff > 0.0f) {
                w->unk44 = 1;
            } else {
                w->unk44 = 0;
            }
            o->pos[1] = p.y;
        } else if (!(w->unkE++ < 2.0f)) {
            w->unkE = 0;
        }
        break;
    }
    if (o->node != NULL) {
        Vec_Copy(o->node->pos, &o->pos[0]);
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
    }
}

/* unk_6BF430's .data (0x7DD2B0-0x7E7010) */
IdPair gRecipeIdPairs[62] = {
#include "lobby/007DD2B0.inc"
};
UpgradeRow gArmorUpgradeRows_0[17] = {
#include "lobby/007DD4A0.inc"
};
UpgradeRow gArmorUpgradeRows_3[17] = {
#include "lobby/007DD5B0.inc"
};
UpgradeRow gArmorUpgradeRows_6[17] = {
#include "lobby/007DD6C0.inc"
};
UpgradeRow gArmorUpgradeRows_9[17] = {
#include "lobby/007DD7D0.inc"
};
UpgradeRow gArmorUpgradeRows_12[17] = {
#include "lobby/007DD8E0.inc"
};
UpgradeRow gArmorUpgradeRows_15[17] = {
#include "lobby/007DD9F0.inc"
};
UpgradeRow gArmorUpgradeRows_18[17] = {
#include "lobby/007DDB00.inc"
};
UpgradeRow gArmorUpgradeRows_21[17] = {
#include "lobby/007DDC10.inc"
};
UpgradeRow gArmorUpgradeRows_24[17] = {
#include "lobby/007DDD20.inc"
};
UpgradeRow gArmorUpgradeRows_27[17] = {
#include "lobby/007DDE30.inc"
};
UpgradeRow gArmorUpgradeRows_30[17] = {
#include "lobby/007DDF40.inc"
};
UpgradeRow gArmorUpgradeRows_33[17] = {
#include "lobby/007DE050.inc"
};
UpgradeRow gArmorUpgradeRows_36[17] = {
#include "lobby/007DE160.inc"
};
UpgradeRow gArmorUpgradeRows_39[17] = {
#include "lobby/007DE270.inc"
};
UpgradeRow gArmorUpgradeRows_1[17] = {
#include "lobby/007DE380.inc"
};
UpgradeRow gArmorUpgradeRows_4[17] = {
#include "lobby/007DE490.inc"
};
UpgradeRow gArmorUpgradeRows_7[17] = {
#include "lobby/007DE5A0.inc"
};
UpgradeRow gArmorUpgradeRows_10[17] = {
#include "lobby/007DE6B0.inc"
};
UpgradeRow gArmorUpgradeRows_13[17] = {
#include "lobby/007DE7C0.inc"
};
UpgradeRow gArmorUpgradeRows_16[17] = {
#include "lobby/007DE8D0.inc"
};
UpgradeRow gArmorUpgradeRows_19[17] = {
#include "lobby/007DE9E0.inc"
};
UpgradeRow gArmorUpgradeRows_22[17] = {
#include "lobby/007DEAF0.inc"
};
UpgradeRow gArmorUpgradeRows_25[17] = {
#include "lobby/007DEC00.inc"
};
UpgradeRow gArmorUpgradeRows_28[17] = {
#include "lobby/007DED10.inc"
};
UpgradeRow gArmorUpgradeRows_31[17] = {
#include "lobby/007DEE20.inc"
};
UpgradeRow gArmorUpgradeRows_34[17] = {
#include "lobby/007DEF30.inc"
};
UpgradeRow gArmorUpgradeRows_37[17] = {
#include "lobby/007DF040.inc"
};
UpgradeRow gArmorUpgradeRows_2[17] = {
#include "lobby/007DF150.inc"
};
UpgradeRow gArmorUpgradeRows_5[17] = {
#include "lobby/007DF260.inc"
};
UpgradeRow gArmorUpgradeRows_8[17] = {
#include "lobby/007DF370.inc"
};
UpgradeRow gArmorUpgradeRows_11[17] = {
#include "lobby/007DF480.inc"
};
UpgradeRow gArmorUpgradeRows_14[17] = {
#include "lobby/007DF590.inc"
};
UpgradeRow gArmorUpgradeRows_17[17] = {
#include "lobby/007DF6A0.inc"
};
UpgradeRow gArmorUpgradeRows_20[17] = {
#include "lobby/007DF7B0.inc"
};
UpgradeRow gArmorUpgradeRows_23[17] = {
#include "lobby/007DF8C0.inc"
};
UpgradeRow gArmorUpgradeRows_26[17] = {
#include "lobby/007DF9D0.inc"
};
UpgradeRow gArmorUpgradeRows_29[17] = {
#include "lobby/007DFAE0.inc"
};
UpgradeRow gArmorUpgradeRows_32[17] = {
#include "lobby/007DFBF0.inc"
};
UpgradeRow gArmorUpgradeRows_35[17] = {
#include "lobby/007DFD00.inc"
};
UpgradeRow gArmorUpgradeRows_38[17] = {
#include "lobby/007DFE10.inc"
};
UpgradeRow gArmorUpgradeRows_40[15] = {
#include "lobby/007DFF20.inc"
};
u32 D_007E0010[8] = {
#include "lobby/007E0010.inc"
};
UpgradeRow gArmorUpgradeRows_43[3] = {
#include "lobby/007E0030.inc"
};
u32 D_007E0060[4] = {
#include "lobby/007E0060.inc"
};
u32 D_007E0070[52] = {
#include "lobby/007E0070.inc"
};
UpgradeRow gArmorUpgradeRows_46[35] = {
#include "lobby/007E0140.inc"
};
UpgradeRow gArmorUpgradeRows_50[17] = {
#include "lobby/007E0370.inc"
};
UpgradeRow gArmorUpgradeRows_53[17] = {
#include "lobby/007E0480.inc"
};
UpgradeRow gArmorUpgradeRows_56[17] = {
#include "lobby/007E0590.inc"
};
UpgradeRow gArmorUpgradeRows_59[17] = {
#include "lobby/007E06A0.inc"
};
UpgradeRow gArmorUpgradeRows_62[17] = {
#include "lobby/007E07B0.inc"
};
UpgradeRow gArmorUpgradeRows_65[17] = {
#include "lobby/007E08C0.inc"
};
UpgradeRow gArmorUpgradeRows_68[17] = {
#include "lobby/007E09D0.inc"
};
UpgradeRow gArmorUpgradeRows_71[17] = {
#include "lobby/007E0AE0.inc"
};
UpgradeRow gArmorUpgradeRows_74[17] = {
#include "lobby/007E0BF0.inc"
};
UpgradeRow gArmorUpgradeRows_77[17] = {
#include "lobby/007E0D00.inc"
};
UpgradeRow gArmorUpgradeRows_80[17] = {
#include "lobby/007E0E10.inc"
};
UpgradeRow gArmorUpgradeRows_82[17] = {
#include "lobby/007E0F20.inc"
};
UpgradeRow gArmorUpgradeRows_41[17] = {
#include "lobby/007E1030.inc"
};
UpgradeRow gArmorUpgradeRows_44[17] = {
#include "lobby/007E1140.inc"
};
UpgradeRow gArmorUpgradeRows_47[17] = {
#include "lobby/007E1250.inc"
};
UpgradeRow gArmorUpgradeRows_49[17] = {
#include "lobby/007E1360.inc"
};
UpgradeRow gArmorUpgradeRows_51[17] = {
#include "lobby/007E1470.inc"
};
UpgradeRow gArmorUpgradeRows_54[17] = {
#include "lobby/007E1580.inc"
};
UpgradeRow gArmorUpgradeRows_57[17] = {
#include "lobby/007E1690.inc"
};
UpgradeRow gArmorUpgradeRows_60[17] = {
#include "lobby/007E17A0.inc"
};
UpgradeRow gArmorUpgradeRows_63[17] = {
#include "lobby/007E18B0.inc"
};
UpgradeRow gArmorUpgradeRows_66[17] = {
#include "lobby/007E19C0.inc"
};
UpgradeRow gArmorUpgradeRows_69[17] = {
#include "lobby/007E1AD0.inc"
};
UpgradeRow gArmorUpgradeRows_72[17] = {
#include "lobby/007E1BE0.inc"
};
UpgradeRow gArmorUpgradeRows_75[17] = {
#include "lobby/007E1CF0.inc"
};
UpgradeRow gArmorUpgradeRows_78[35] = {
#include "lobby/007E1E00.inc"
};
UpgradeRow gArmorUpgradeRows_83[17] = {
#include "lobby/007E2030.inc"
};
UpgradeRow gArmorUpgradeRows_85[17] = {
#include "lobby/007E2140.inc"
};
UpgradeRow gArmorUpgradeRows_42[17] = {
#include "lobby/007E2250.inc"
};
UpgradeRow gArmorUpgradeRows_45[17] = {
#include "lobby/007E2360.inc"
};
UpgradeRow gArmorUpgradeRows_48[35] = {
#include "lobby/007E2470.inc"
};
UpgradeRow gArmorUpgradeRows_52[17] = {
#include "lobby/007E26A0.inc"
};
UpgradeRow gArmorUpgradeRows_55[17] = {
#include "lobby/007E27B0.inc"
};
UpgradeRow gArmorUpgradeRows_58[17] = {
#include "lobby/007E28C0.inc"
};
UpgradeRow gArmorUpgradeRows_61[17] = {
#include "lobby/007E29D0.inc"
};
UpgradeRow gArmorUpgradeRows_64[17] = {
#include "lobby/007E2AE0.inc"
};
UpgradeRow gArmorUpgradeRows_67[17] = {
#include "lobby/007E2BF0.inc"
};
UpgradeRow gArmorUpgradeRows_70[17] = {
#include "lobby/007E2D00.inc"
};
UpgradeRow gArmorUpgradeRows_73[17] = {
#include "lobby/007E2E10.inc"
};
UpgradeRow gArmorUpgradeRows_76[17] = {
#include "lobby/007E2F20.inc"
};
UpgradeRow gArmorUpgradeRows_79[17] = {
#include "lobby/007E3030.inc"
};
UpgradeRow gArmorUpgradeRows_81[17] = {
#include "lobby/007E3140.inc"
};
UpgradeRow gArmorUpgradeRows_84[17] = {
#include "lobby/007E3250.inc"
};
UpgradeRow gArmorUpgradeRows_86[17] = {
#include "lobby/007E3360.inc"
};
UpgradeRow gArmorUpgradeRows_89[35] = {
#include "lobby/007E3470.inc"
};
UpgradeRow gArmorUpgradeRows_93[17] = {
#include "lobby/007E36A0.inc"
};
UpgradeRow gArmorUpgradeRows_94[17] = {
#include "lobby/007E37B0.inc"
};
UpgradeRow gArmorUpgradeRows_95[17] = {
#include "lobby/007E38C0.inc"
};
UpgradeRow gArmorUpgradeRows_96[17] = {
#include "lobby/007E39D0.inc"
};
UpgradeRow gArmorUpgradeRows_97[17] = {
#include "lobby/007E3AE0.inc"
};
UpgradeRow gArmorUpgradeRows_98[17] = {
#include "lobby/007E3BF0.inc"
};
UpgradeRow gArmorUpgradeRows_99[17] = {
#include "lobby/007E3D00.inc"
};
UpgradeRow gArmorUpgradeRows_87[17] = {
#include "lobby/007E3E10.inc"
};
UpgradeRow gArmorUpgradeRows_90[17] = {
#include "lobby/007E3F20.inc"
};
UpgradeRow gArmorUpgradeRows_92[17] = {
#include "lobby/007E4030.inc"
};
UpgradeRow gArmorUpgradeRows_88[17] = {
#include "lobby/007E4140.inc"
};
UpgradeRow gArmorUpgradeRows_91[17] = {
#include "lobby/007E4250.inc"
};
UpgradeRow gArmorUpgradeRows_100[17] = {
#include "lobby/007E4360.inc"
};
UpgradeRow gArmorUpgradeRows_103[17] = {
#include "lobby/007E4470.inc"
};
UpgradeRow gArmorUpgradeRows_106[17] = {
#include "lobby/007E4580.inc"
};
UpgradeRow gArmorUpgradeRows_109[17] = {
#include "lobby/007E4690.inc"
};
UpgradeRow gArmorUpgradeRows_110[17] = {
#include "lobby/007E47A0.inc"
};
UpgradeRow gArmorUpgradeRows_111[17] = {
#include "lobby/007E48B0.inc"
};
UpgradeRow gArmorUpgradeRows_112[17] = {
#include "lobby/007E49C0.inc"
};
UpgradeRow gArmorUpgradeRows_113[17] = {
#include "lobby/007E4AD0.inc"
};
UpgradeRow gArmorUpgradeRows_114[17] = {
#include "lobby/007E4BE0.inc"
};
UpgradeRow gArmorUpgradeRows_115[17] = {
#include "lobby/007E4CF0.inc"
};
UpgradeRow gArmorUpgradeRows_116[17] = {
#include "lobby/007E4E00.inc"
};
UpgradeRow gArmorUpgradeRows_117[17] = {
#include "lobby/007E4F10.inc"
};
UpgradeRow gArmorUpgradeRows_118[17] = {
#include "lobby/007E5020.inc"
};
UpgradeRow gArmorUpgradeRows_119[17] = {
#include "lobby/007E5130.inc"
};
UpgradeRow gArmorUpgradeRows_133[17] = {
#include "lobby/007E5240.inc"
};
UpgradeRow gArmorUpgradeRows_134[17] = {
#include "lobby/007E5350.inc"
};
UpgradeRow gArmorUpgradeRows_135[17] = {
#include "lobby/007E5460.inc"
};
UpgradeRow gArmorUpgradeRows_101[17] = {
#include "lobby/007E5570.inc"
};
UpgradeRow gArmorUpgradeRows_104[17] = {
#include "lobby/007E5680.inc"
};
UpgradeRow gArmorUpgradeRows_107[17] = {
#include "lobby/007E5790.inc"
};
UpgradeRow gArmorUpgradeRows_102[17] = {
#include "lobby/007E58A0.inc"
};
UpgradeRow gArmorUpgradeRows_105[17] = {
#include "lobby/007E59B0.inc"
};
UpgradeRow gArmorUpgradeRows_108[17] = {
#include "lobby/007E5AC0.inc"
};
UpgradeRow gArmorUpgradeRows_120[17] = {
#include "lobby/007E5BD0.inc"
};
UpgradeRow gArmorUpgradeRows_121[17] = {
#include "lobby/007E5CE0.inc"
};
UpgradeRow gArmorUpgradeRows_122[17] = {
#include "lobby/007E5DF0.inc"
};
UpgradeRow gArmorUpgradeRows_123[17] = {
#include "lobby/007E5F00.inc"
};
UpgradeRow gArmorUpgradeRows_124[17] = {
#include "lobby/007E6010.inc"
};
UpgradeRow gArmorUpgradeRows_125[17] = {
#include "lobby/007E6120.inc"
};
UpgradeRow gArmorUpgradeRows_126[17] = {
#include "lobby/007E6230.inc"
};
UpgradeRow gArmorUpgradeRows_127[17] = {
#include "lobby/007E6340.inc"
};
UpgradeRow gArmorUpgradeRows_128[17] = {
#include "lobby/007E6450.inc"
};
UpgradeRow gArmorUpgradeRows_129[17] = {
#include "lobby/007E6560.inc"
};
UpgradeRow gArmorUpgradeRows_130[17] = {
#include "lobby/007E6670.inc"
};
UpgradeRow gArmorUpgradeRows_131[17] = {
#include "lobby/007E6780.inc"
};
UpgradeRow gArmorUpgradeRows_132[17] = {
#include "lobby/007E6890.inc"
};
UpgradeRow gArmorUpgradeRows_139[17] = {
#include "lobby/007E69A0.inc"
};
UpgradeRow gArmorUpgradeRows_136[17] = {
#include "lobby/007E6AB0.inc"
};
UpgradeRow gArmorUpgradeRows_137[17] = {
#include "lobby/007E6BC0.inc"
};
UpgradeRow gArmorUpgradeRows_138[17] = {
#include "lobby/007E6CD0.inc"
};
UpgradeRow *gArmorUpgradeRows[140] = {
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
