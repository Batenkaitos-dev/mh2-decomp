/*
 * Game overlay code at 0x6D29F0-0x6D3B50: Eff09_* and Eff10_*.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "stageobj.h"
#include "sprite.h"
#include "capsule.h"

void Eff09_CreateInArea(Player *owner, f32 *pos, s32 kind, u16 area);
void Eff09_CreateOwned(Player *owner, f32 *pos, s32 kind);
s32 Player_IsLocal(void *obj);
s32 Em_HitQuadsVertical(f32 *p, Em **hitEm, s16 *hitBone, f32 *hit);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void Eff10_Update(ObjAttach *o);
void Vec_RotateY(f32 *v, f32 angle);
void ObjAttach_Free(ObjAttach *p);
s32 Player_HasSkill(Player *p, u8 skill);
u16 Rand_Next(s32 stream);
void Eff09_Setup(ObjAttach *o);
void Eff09_Run(ObjAttach *);
void Eff09_Hit(ObjAttach *o);
void Eff09_End(ObjAttach *o);
void Eff09_Free(ObjAttach *o);
void Eff10_Setup(struct ObjAttach *o);
void Eff10_Run(struct ObjAttach *o);
void Eff10_Hit(struct ObjAttach *);
void Eff10_End(ObjAttach *o);
void Eff10_Free(ObjAttach *o);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Vec_Copy(f32 *out, f32 *in);
void DrawSlot_Free(s32);
ObjAttach *ObjAttach_Alloc(s32 n);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
struct MoveObj;
void MoveObj_StartMotionEntry(struct MoveObj *, s32 a, void *b);
s32 Player_TestSetFlags(void *obj, s32 mask);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *o, s32 a, s32 b, void *c);
extern HitShape *gEff09HitLists[];
extern void *Eff09_MotionDesc38;
extern u8 Eff09_KindFlags[];
void Eff09_Init(ObjAttach *);
void Eff09_BoostPower(ObjAttach *);
void Eff09_Spawn(ObjAttach *);
void Eff09_Update(ObjAttach *);

void Eff09_Init(ObjAttach *o)
{
    o->kind = 9;
    o->update = Eff09_Update;
    switch (o->type) {
    case 0:
    case 2:
    case 7:
    case 8:
    case 14:
    case 15:
    case 16:
    case 20:
    case 22:
    case 38:
        Eff09_Spawn(o);
        break;
    }
    o->stateVar0 = 0;
}

void Eff09_CreateAt(f32 *pos, s32 kind, u16 area, u8 stateVar0)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = kind;
        Vec_Copy(o->pos, pos);
        o->idx = 0;
        o->isEm = 0;
        o->area = area;
        Eff09_Init(o);
        o->stateVar0 = stateVar0;
    }
}

void Eff09_Create(Player *owner, f32 *pos, s32 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = kind;
        Vec_Copy(o->pos, pos);
        if (kind != 2 && kind != 22 && kind != 35) {
            o->idx = owner->index;
            o->isEm = owner->isEm;
            o->owner = owner;
            o->yaw = owner->rot[1];
            o->area = owner->area;
            if (Eff09_KindFlags[kind] != 0) {
                o->side = owner->side;
            }
        } else {
            o->idx = 0;
            o->isEm = 0;
            o->area = gScene.area;
        }
        Eff09_Init(o);
    }
}

void Eff09_CreateOwned(Player *owner, f32 *pos, s32 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = kind;
        o->idx = owner->index;
        o->isEm = owner->isEm;
        o->owner = owner;
        o->yaw = owner->rot[1];
        Vec_Copy(o->pos, pos);
        o->area = owner->area;
        if (Eff09_KindFlags[kind] != 0) {
            o->side = owner->side;
        }
        Eff09_Init(o);
    }
}

void Eff09_CreateInArea(Player *owner, f32 *pos, s32 kind, u16 area)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = kind;
        Vec_Copy(o->pos, pos);
        o->idx = owner->index;
        o->isEm = owner->isEm;
        o->owner = owner;
        o->yaw = owner->rot[1];
        o->area = area;
        if (Eff09_KindFlags[kind] != 0) {
            o->side = owner->side;
        }
        Eff09_Init(o);
    }
}

void Eff09_Spawn(ObjAttach *src)
{
    ObjAttach *o;
    s16 kind;

    switch (src->type) {
    case 0:
    case 7:
    case 8:
    case 14:
    case 15:
    case 16:
    case 38:
        kind = 9;
        break;
    case 1:
        kind = 3;
        break;
    case 2:
        kind = 10;
        break;
    case 4:
        kind = 5;
        break;
    case 20:
        kind = 21;
        break;
    case 22:
        kind = 23;
        break;
    default:
        return;
    }
    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 9;
        o->type = kind;
        o->update = Eff09_Update;
        o->idx = src->idx;
        o->isEm = src->isEm;
        o->owner = src->owner;
        o->yaw = *(u16 *) &src->yaw;
        Vec_Copy(o->pos, src->pos);
        o->area = src->area;
        o->stateVar0 = 0;
        if (Eff09_KindFlags[kind] != 0) {
            o->side = src->side;
        } else {
            o->side = 0;
        }
    }
}

void Eff09_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff09_Setup(o);
        break;
    case 1:
        Eff09_Run(o);
        break;
    case 2:
        Eff09_End(o);
        break;
    case 3:
        Eff09_Hit(o);
        break;
    case 4:
        Eff09_End(o);
        break;
    case 5:
        Eff09_End(o);
        break;
    case 9:
        Eff09_End(o);
        break;
    case 7:
        Eff09_End(o);
        break;
    case 6:
        Eff09_Free(o);
        
    default:
        break;
    }
}

void Eff09_PlaySound(Em *em)
{
    if (*(u16 *)((u8 *)em + 0xCE) == gScene.area) {
        switch (em->subType) {
        case 0:
        case 16:
        case 20:
        case 34:
        case 38:
            Sound_PlaySeAtPosChecked(1, 0x4D, 0, em->rotMtx[1], 0xF, 0);
            break;
        case 2:
        case 22:
            Sound_PlaySeAtPosChecked(1, 0x4F, 0, em->rotMtx[1], 0xF, 0);
            break;
        case 7:
            if (Rand_Next(1) & 0xFFFF & 1) {
                Sound_PlaySeAtPosChecked(1, 0x4E, 0, em->rotMtx[1], 0xF, 0);
            }
            break;
        case 8:
        case 14:
        case 15:
            Sound_PlaySeAtPosChecked(1, 0x4E, 0, em->rotMtx[1], 0xF, 0);
            break;
        }
    }
}

void Eff09_Setup(ObjAttach *o)
{

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Eff09_PlaySound((Em *)o);
    switch (o->type) {
    case 0:
    case 2:
    case 16:
    case 20:
    case 22:
    case 33:
    case 34:
    case 35:
    case 38:
        Player_TestSetFlags(o, 4);
        MoveObj_StartMotionEntry((struct MoveObj *)o, o->type + 1, &Eff09_MotionDesc38);
        break;
    case 6:
        Player_TestSetFlags(o, 0);
        MoveObj_StartMotionEntry((struct MoveObj *)o, o->type + 1, &Eff09_MotionDesc38);
        break;
    case 1:
    case 4:
    case 18:
    case 28:
    case 29:
    case 31:
    case 32:
    case 39:
    case 40:
        Player_TestSetFlags(o, 0x200);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, (void *) &Eff09_MotionDesc38);
        break;
    case 7:
    case 8:
    case 14:
    case 15:
        Player_TestSetFlags(o, 4);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, (void *) &Eff09_MotionDesc38);
        break;
    case 9:
    case 10:
    case 21:
    case 23:
        Player_TestSetFlags(o, 0x400);
        MoveObj_StartMotionEntry((struct MoveObj *)o, o->type + 1, &Eff09_MotionDesc38);
        break;
    case 13:
        Player_TestSetFlags(o, 0x400);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, (void *) &Eff09_MotionDesc38);
        break;
    case 27:
        Player_TestSetFlags(o, 0x80);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, (void *) &Eff09_MotionDesc38);
        break;
    case 36:
        Player_TestSetFlags(o, 0x84);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, (void *) &Eff09_MotionDesc38);
        break;
    default:
        Player_TestSetFlags(o, 0);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, (void *) &Eff09_MotionDesc38);
        break;
    }
    Eff09_BoostPower(o);
    o->hitList = gEff09HitLists[o->hitListNo];
    o->hurtList = 0;
    o->ownerMotion = 0;
    o->drawSlotBuf = NULL;
}

void Eff09_Run(ObjAttach *o)
{
    u8 kind = o->type;
    s16 n;

    if (kind == 1 || kind == 4 || kind == 18 || kind == 24) {
        if (o->hitFrames <= 0) {
            Eff09_Spawn(o);
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
    }
    switch (kind) {
    default:
        n = 40;
        break;
    case 19:
    case 26:
    case 36:
        n = 80;
        break;
    }
    if (++o->ownerMotion > n) {
        o->hitFrames = 0;
        o->state = 2;
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff09_Hit(ObjAttach *o)
{
    switch (o->type) {
    case 0x13:
    case 0x1A:
    case 0x24:
        Eff09_Run(o);
        break;
    default:
        Eff09_End(o);
        break;
    }
}

void Eff09_End(ObjAttach *o)
{
    if ((o->type == 1 || o->type == 4 || o->type == 0x12) && o->state == 3) {
        Eff09_Spawn(o);
    }
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free(o->drawSlot);
    }
}

void Eff09_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff09_BoostPower(ObjAttach *o)
{
    u16 v;

    switch (o->stateVar0) {
    case 1:
        switch (o->type) {
        case 0:
        case 2:
        case 20:
        case 22:
        case 38:
            v = 1.5f * o->power;
            if (v > 0xFF) {
                v = 0xFF;
            }
            o->power = v;
            break;
        }
        break;
    }
}

void Eff10_Create(f32 *pos, u8 kind, u16 area, Player *pl)
{
    ObjAttach *o;
    f32 v[3];
    s16 n;

    switch (kind) {
    case 4:
    case 5:
        n = 1;
        break;
    default:
        n = 0;
        break;
    }
    if ((o = ObjAttach_Alloc(n)) != NULL) {
        o->kind = 10;
        o->type = kind;
        o->update = Eff10_Update;
        o->isEm = 0;
        o->owner = pl;
        o->idx = pl->index;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->area = area;
        o->keep = 1;
        o->rotY = pl->rot[1];
        switch (o->type) {
        case 0:
        default:
            o->pos[1] += 50.0f;
            if (gScene.area == o->area) {
                Sound_PlaySeAtPosChecked(1, 0x3C, 0, o->pos, 1, 0);
            }
            break;
        case 1:
            o->pos[1] += 75.0f;
            if (gScene.area == o->area) {
                Sound_PlaySeAtPosChecked(1, 0x3C, 0, o->pos, 1, 0);
            }
            break;
        case 2:
        case 3:
            o->side = pl->side;
        case 4:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 80.0f;
            Vec_RotateY(v, (6.2831855f * pl->rot[1]) / 65536.0f);
            o->pos[0] += v[0];
            o->pos[1] += v[1];
            o->pos[2] += v[2];
            break;
        case 5:
            if (!Em_HitQuadsVertical((f32 *) pos, (Em **) (o->buf + 4), (s16 *) (o->buf + 0x14), (f32 *) (o->buf + 8))) {
                if (area == gScene.area) {
                    StageObj_K0E_CreateScaled((f32 *) pos, 0, 0.5f);
                }
                ObjAttach_Free((ObjAttach *)o);
                return;
            }
            o->powerPct = 0xFA;
            if (gScene.area == o->area) {
                Sound_PlaySeAtPosChecked(1, 0x3C, 0, o->pos, 1, 0);
                Sound_PlaySeAtPosChecked(1, 0x94, 0, o->pos, 1, 0);
            }
            break;
        case 6:
            o->pos[1] += 35.0f;
            if (gScene.area == o->area) {
                Sound_PlaySeAtPosChecked(1, 0x3C, 0, o->pos, 1, 0);
            }
            break;
        }
        o->stateVar0 = 0;
        if (pl != NULL) {
            if (Player_IsLocal(pl)) {
                switch (o->type) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 6:
                default:
                    (gScene.placed1)++;
                    o->stateVar1 = 1;
                    break;
                case 4:
                    (gScene.placed2)++;
                    o->stateVar1 = 2;
                    break;
                case 5:
                    break;
                }
            }
            if (Player_HasSkill(pl, 0x48) == 1) {
                o->stateVar0 = 1;
            }
        }
    }
}

void Eff10_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff10_Setup(o);
        break;
    case 1:
        Eff10_Run(o);
        break;
    case 2:
        Eff10_End(o);
        break;
    case 3:
        Eff10_Hit(o);
        break;
    case 4:
        Eff10_End(o);
        break;
    case 5:
        Eff10_End(o);
        break;
    case 9:
        Eff10_End(o);
        break;
    case 7:
        Eff10_End(o);
        break;
    case 6:
        Eff10_Free(o);
        
    default:
        break;
    }
}

/* unk_6D29F0's .data (0x89DE10-0x89EC80) */
HitShape gEff09HitLists_0[2] = {
#include "game/0089DE10.inc"
};
HitShape gEff09HitLists_1[2] = {
#include "game/0089DE60.inc"
};
HitShape gEff09HitLists_2[2] = {
#include "game/0089DEB0.inc"
};
HitShape gEff09HitLists_3[2] = {
#include "game/0089DF00.inc"
};
HitShape gEff09HitLists_4[2] = {
#include "game/0089DF50.inc"
};
HitShape gEff09HitLists_5[2] = {
#include "game/0089DFA0.inc"
};
HitShape gEff09HitLists_6[2] = {
#include "game/0089DFF0.inc"
};
HitShape gEff09HitLists_7[2] = {
#include "game/0089E040.inc"
};
HitShape gEff09HitLists_8[2] = {
#include "game/0089E090.inc"
};
HitShape gEff09HitLists_9[2] = {
#include "game/0089E0E0.inc"
};
HitShape gEff09HitLists_10[2] = {
#include "game/0089E130.inc"
};
HitShape gEff09HitLists_11[2] = {
#include "game/0089E180.inc"
};
HitShape gEff09HitLists_12[2] = {
#include "game/0089E1D0.inc"
};
HitShape gEff09HitLists_13[2] = {
#include "game/0089E220.inc"
};
HitShape gEff09HitLists_14[2] = {
#include "game/0089E270.inc"
};
HitShape gEff09HitLists_15[2] = {
#include "game/0089E2C0.inc"
};
HitShape gEff09HitLists_16[2] = {
#include "game/0089E310.inc"
};
HitShape gEff09HitLists_17[2] = {
#include "game/0089E360.inc"
};
HitShape gEff09HitLists_18[2] = {
#include "game/0089E3B0.inc"
};
HitShape gEff09HitLists_19[2] = {
#include "game/0089E400.inc"
};
HitShape gEff09HitLists_20[2] = {
#include "game/0089E450.inc"
};
HitShape gEff09HitLists_21[2] = {
#include "game/0089E4A0.inc"
};
HitShape gEff09HitLists_22[2] = {
#include "game/0089E4F0.inc"
};
HitShape gEff09HitLists_23[2] = {
#include "game/0089E540.inc"
};
HitShape gEff09HitLists_24[2] = {
#include "game/0089E590.inc"
};
HitShape gEff09HitLists_25[3] = {
#include "game/0089E5E0.inc"
};
HitShape gEff09HitLists_26[3] = {
#include "game/0089E660.inc"
};
HitShape gEff09HitLists_27[2] = {
#include "game/0089E6E0.inc"
};
HitShape gEff09HitLists_28[2] = {
#include "game/0089E730.inc"
};
HitShape gEff09HitLists_29[2] = {
#include "game/0089E780.inc"
};
HitShape *gEff09HitLists[32] = {
    gEff09HitLists_0, gEff09HitLists_1, gEff09HitLists_2, gEff09HitLists_3,
    gEff09HitLists_4, gEff09HitLists_5, gEff09HitLists_6, gEff09HitLists_7,
    gEff09HitLists_8, gEff09HitLists_9, gEff09HitLists_10, gEff09HitLists_11,
    gEff09HitLists_12, gEff09HitLists_13, gEff09HitLists_14, gEff09HitLists_15,
    gEff09HitLists_16, gEff09HitLists_17, gEff09HitLists_18, gEff09HitLists_19,
    gEff09HitLists_20, gEff09HitLists_21, gEff09HitLists_22, gEff09HitLists_23,
    gEff09HitLists_24, gEff09HitLists_25, gEff09HitLists_26, gEff09HitLists_27,
    gEff09HitLists_28, gEff09HitLists_29, 0, 0,
};
u32 D_0089E850[252] = {
#include "game/0089E850.inc"
};
void *Eff09_MotionDesc38 = (void *)D_0089E850;
u8 Eff09_KindFlags[48] = {
#include "game/0089EC50.inc"
};
