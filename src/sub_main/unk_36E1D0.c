#include "common.h"
#include "scene.h"
#include "em.h"
#include "camera.h"
#include "player.h"
#include "objhead.h"
#include "stageobj.h"
#include "sprite.h"
#include "chredit.h"
#include "area.h"
#include "pad.h"

/* prototypes */
extern s32 D_005BAD78;
extern s32 D_005BAD7C;
extern s32 D_005BAD80;
extern s32 D_005BAD84;
/* end of prototypes */

u8 Player_PartFlags(Player *pl, u8 slot);
u32 Color_Cycle8(void);
s32 Gs_SetState(u8 reg, u32 v);
void Vec_AnglesToRad(s32 *in, f32 *out);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateYXZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Copy(f32 *dst, f32 *src);
struct EmBone;
void Bone_UpdateWorld(struct EmBone *b, f32 *m);
void Player_ScaleBones(u8 *o, EmBone *b, f32 *m);

s32 Motion_ReachedFrame(void *o, s32 layer, f32 frame);

/* A 0/1/2 class of the player's motion (unk29C[0]) by weapon type (unk002) and the motion's
 * frame: per-weapon attack motions first, then some shared motions override it. The arms that
 * set r to 2 where it is already 2 are the original's: MWCC drops the store but keeps the branch. */
s16 Player_MotionPhase(Player *p, s32 alt, s32 flag)
{
    s16 r = 2;

    switch (p->weaponType) {
    case WEAPON_HAMMER:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, 6.0f)) r = 1;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 48.0f)) r = 2; else r = 1;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(p, 0, 50.0f)) r = 2; else r = 1;
            break;
        case 0x3F2:
            if (p->motion[0].blend == 1) r = 2; else r = 1;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_HUNTING_HORN:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, 68.0f)) r = 1;
            else if (Motion_ReachedFrame(p, 0, 14.0f)) r = 0;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 50.0f)) r = 2; else r = 1;
            break;
        case 0x3ED:
            if (Motion_ReachedFrame(p, 0, 50.0f)) r = 2; else r = 1;
            break;
        case 0x3EE:
            if (Motion_ReachedFrame(p, 0, 16.0f)) r = 1;
            else if (Motion_ReachedFrame(p, 0, 6.0f)) r = 0; else r = 2;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x57C:
            if (p->motion[0].blend == 1) r = 1;
            else if (Motion_ReachedFrame(p, 0, 54.0f)) r = 1;
            else r = 0;
            break;
        case 0x588:
            r = 0;
            break;
        case 0x597:
            if (Motion_ReachedFrame(p, 0, 10.0f)) r = 1;
            if (p->motion[0].blend == 1) r = 2;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_LANCE:
    case WEAPON_GUNLANCE:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (p->motion[0].blend == 1) r = 2;
            else if (p->weaponType == WEAPON_LANCE) r = 1;
            else if (Motion_ReachedFrame(p, 0, 14.0f)) r = 1;
            break;
        case 0x3FA:
            if (p->motion[0].blend == 1) r = 2; else r = 1;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 60.0f)) r = 2; else r = 1;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(p, 0, 66.0f)) r = 2; else r = 1;
            break;
        case 0x3F2:
            if (p->motion[0].blend == 1) r = 2; else r = 1;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_SWORD_SHIELD:
        switch (p->motionId[0]) {
        case 0x3EA:
        case 0x3FA:
        case 0x3F2:
            if (p->motion[0].blend == 1) r = 2; else r = 1;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 32.0f)) r = 2; else r = 1;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(p, 0, 30.0f)) r = 2; else r = 1;
            break;
        case 0x3F8:
            if (Motion_ReachedFrame(p, 0, 14.0f)) r = 0;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3F9:
            if (Motion_ReachedFrame(p, 0, 44.0f)) r = 2; else r = 0;
            break;
        case 0x582:
            if (p->motion[0].blend == 1) r = 2; else r = 0;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_DUAL_SWORDS:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, alt == 0 ? 8.0f : 10.0f)) r = 1;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, alt == 0 ? 20.0f : 28.0f)) r = 2; else r = 1;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(p, 0, alt == 0 ? 18.0f : 30.0f)) r = 2; else r = 1;
            break;
        case 0x3F9:
            if (Motion_ReachedFrame(p, 0, 44.0f)) r = 2; else r = 0;
            break;
        case 0x585:
            if (p->motion[0].blend == 1) r = 2;
            else if (Motion_ReachedFrame(p, 0, 4.0f)) r = 1; else r = 2;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    default:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, 16.0f)) r = 0;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 38.0f)) r = 2; else r = 0;
            break;
        case 0x3F8:
            if (Motion_ReachedFrame(p, 0, 14.0f)) r = 0;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3F9:
            if (Motion_ReachedFrame(p, 0, 44.0f)) r = 2; else r = 0;
            break;
        case 0x584:
        case 0x585:
        case 0x586:
        case 0x587:
            r = 0;
            break;
        case 0x582:
            if (p->motion[0].blend == 1) r = 2; else r = 0;
            break;
        case 0x597:
            if (Motion_ReachedFrame(p, 0, 20.0f)) r = 1;
            else if (Motion_ReachedFrame(p, 0, 6.0f)) r = 0;
            if (p->motion[0].blend == 1) r = 2;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_LONG_SWORD:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, 56.0f)) r = 1;
            else if (Motion_ReachedFrame(p, 0, 16.0f)) r = 0;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 52.0f)) r = 2; else r = 0;
            break;
        case 0x587:
            if (Motion_ReachedFrame(p, 0, 28.0f)) r = 1;
            else if (Motion_ReachedFrame(p, 0, 8.0f)) r = 0; else r = 2;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x592:
            if (Motion_ReachedFrame(p, 0, 4.0f)) r = 0;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3ED:
            if (Motion_ReachedFrame(p, 0, 44.0f)) r = 2; else r = 0;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_BOW:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, 18.0f)) r = 1;
            break;
        case 0x3F2:
            if (Motion_ReachedFrame(p, 0, 12.0f)) r = 1;
            break;
        case 0x3F4:
            if (Motion_ReachedFrame(p, 0, 6.0f)) r = 1;
            break;
        case 0x3F6:
            if (Motion_ReachedFrame(p, 0, 4.0f)) r = 1;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 64.0f)) r = 2; else r = 1;
            if (p->motion[0].blend == 1) r = 1;
            break;
        case 0x3ED:
            if (Motion_ReachedFrame(p, 0, 78.0f)) r = 2; else r = 1;
            if (p->motion[0].blend == 1) r = 1;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_HEAVY_BOWGUN:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, 2.0f)) r = 1;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(p, 0, 110.0f)) r = 2; else r = 1;
            if (p->motion[0].blend == 1) r = 1;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(p, 0, 92.0f)) r = 2; else r = 1;
            if (p->motion[0].blend == 1) r = 1;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    case WEAPON_LIGHT_BOWGUN:
        switch (p->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(p, 0, 28.0f)) r = 0; else r = 2;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x58E:
            if (Motion_ReachedFrame(p, 0, 26.0f)) r = 1; else r = 2;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x58F:
            if (Motion_ReachedFrame(p, 0, 24.0f)) r = 1; else r = 2;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3F2:
            if (Motion_ReachedFrame(p, 0, 22.0f)) r = 0; else r = 2;
            if (p->motion[0].blend == 1) r = 2;
            break;
        case 0x3EB:
        case 0x3F1:
            if (p->motion[0].blend == 1) r = 1; else r = 2;
            break;
        default:
            if (p->weaponOut) r = 1;
            break;
        }
        break;
    }

    switch (p->motionId[0]) {
    case 0xCA:
    case 0xCC:
        if (p->weaponOut) r = 1;
        break;
    case 0xCF:
    case 0xD8:
        if (p->weaponOut) {
            if (p->stateGroup == PL_GROUP_GESTURE) r = 2; else r = 1;
        }
        break;
    case 0x3C:
    case 0x3D:
    case 0x280:
    case 0x26C:
    case 0x26D:
    case 0x26E:
    case 0x26F:
    case 0x270:
    case 0x271:
    case 0x272:
    case 0x273:
    case 0x274:
    case 0x275:
    case 0x276:
    case 0x277:
    case 0x278:
    case 0x279:
        r = 2;
        break;
    case 0xCD:
    case 0xCE:
    case 0xD0:
    case 0xD1:
    case 0xD3:
    case 0xD4:
    case 0xD5:
    case 0xD6:
    case 0xD7:
    case 0xDB:
    case 0xDC:
        r = 2;
        break;
    case 0xD2:
        if (p->stateGroup == PL_GROUP_GESTURE) r = 2;
        break;
    case 0x19D:
        if (Motion_ReachedFrame(p, 0, 328.0f)) r = 2;
        else if (Motion_ReachedFrame(p, 0, 22.0f)) r = 1;
        else r = 2;
        if (p->motion[0].blend == 1) r = 2;
        break;
    case 0x349:
        if (flag == 1) r = 2;
        break;
    }
    return r;
}

void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Hud_DrawTargetMarker(f32 *pos);
extern f32 gBoneMarkerDirs[][3];
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
extern f32 D_0043E5E8[];

void Player_DrawBoneMarker(Player *o, u8 i)
{
    f32 pos[3];
    f32 dir[3];

    Vec_Rotate(dir, gBoneMarkerDirs[i], o->partRecs[14]->m1);
    Mtx_GetTrans(pos, o->partRecs[14]->m1);
    Vec_Add(pos, pos, dir);
    Hud_DrawTargetMarker(pos);
}

/* Hud_DrawTargetMarker at o's position plus D_0043E5E8 turned by o's angles
 * (-aimPitch and rot[1], 16-bit turns), 100 higher. */
void Hud_DrawObjMarker(Player *o)
{
    f32 m[16];
    f32 v[3];

    Mtx_Identity(m);
    Mtx_RotateXYZ(m, 6.2831855f * (f32)(-o->aimPitch) / 65536.0f,
                  6.2831855f * (f32)o->rot[1] / 65536.0f, 0.0f);
    Vec_Rotate(v, D_0043E5E8, m);
    Vec_Add(v, v, o->pos);
    v[1] += 100.0f;
    Hud_DrawTargetMarker(v);
}

extern u8 D_0041CEE9;
extern u8 D_0041CEEA;
extern u8 D_0041CEEB;
extern u8 D_0043E608[][3];
extern u8 D_0043E5F8[][3];
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);

/* Draws one part (part: model handle, light count, light indices, +0x88 texture) with the matrix m,
 * its lights (0x4C bytes each in lights) coloured by mode: 1 and 2 a colour pair of D_0043E608 and
 * D_0043E5F8 (1 by cookResult, 2 by useItem), 3..5 a grey, 6 the fog colour D_0041CEE9. */
void Player_DrawPartLit(Player *p, ModelMatSet *part, f32 *m, u8 *lights, u8 mode)
{
    s16 col;
    s32 i;
    u8 r;
    u8 g;
    u8 b;
    f32 *light;

    if (part != NULL && part->id != -1) {
        switch (mode) {
        case 1:
            col = p->cookResult;
            break;
        case 2:
            switch (p->useItem) {
            case ITEM_ID_FOOD_STAMINA_75:
            case ITEM_ID_FOOD_RESTORE:
                col = 1;
                break;
            case ITEM_ID_FOOD_STAMINA_150:
            case ITEM_ID_FOOD_REGEN_SHORT:
            case ITEM_ID_FOOD_REGEN:
            case ITEM_ID_FOOD_REGEN_B:
            case ITEM_ID_FOOD_REGEN_LONG:
                col = 2;
                break;
            case ITEM_ID_FOOD_STAMINA_GAMBLE:
            case ITEM_ID_FOOD_RESTORE_B:
                col = 3;
                break;
            case ITEM_ID_FOOD_STAMINA_FULL:
                col = 4;
                break;
            case ITEM_ID_FOOD_HOT:
            case ITEM_ID_FOOD_HOT_STAMINA:
            case ITEM_ID_FOOD_COOL:
            case ITEM_ID_FOOD_COOL_STAMINA:
            case ITEM_ID_FOOD_CURE:
            case ITEM_ID_FOOD_CURE_STAMINA:
                col = 4;
                break;
            default:
                return;
            }
            break;
        case 3:
            r = g = b = 0x3F;
            break;
        case 4:
            r = g = b = 0xFF;
            break;
        case 5:
            r = g = b = 0x9F;
            break;
        case 6:
            Gs_SetState(0x67, 0xFF000000 | D_0041CEE9 << 16 | D_0041CEEA << 8 | D_0041CEEB);
            break;
        }
        Gs_SetState(0x1A, (u32)m);
        for (i = 0; i < part->count; i++) {
            light = (f32 *)(lights + part->mat[i] * 0x4C);
            light[4] = p->alpha;
            switch (mode) {
            case 1:
            case 2:
                switch (i) {
                case 0:
                    light[1] = D_0043E608[col][0] / 255.0f;
                    light[2] = D_0043E608[col][1] / 255.0f;
                    light[3] = D_0043E608[col][2] / 255.0f;
                    break;
                case 1:
                    light[1] = D_0043E5F8[col][0] / 255.0f;
                    light[2] = D_0043E5F8[col][1] / 255.0f;
                    light[3] = D_0043E5F8[col][2] / 255.0f;
                    break;
                }
                break;
            case 3:
            case 4:
            case 5:
                switch (i) {
                case 0:
                    light[1] = r / 255.0f;
                    light[2] = g / 255.0f;
                    light[3] = b / 255.0f;
                    break;
                }
                break;
            }
            Gs_SetState((u8)(i + 0x3A), (u32)light);
        }
        Draw_ApplyStates(part->flags);
        Model_Draw(part->id, 0);
    }
}

s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
s16 Effect_MapTexId(s16 id);

/* Picks the sound or effect (Effect_MapTexId) of motions 0x327-0x32A by the frames reached, and returns
 * its (s16) result. */
s16 Player_HeldPartForMotion(void *obj)
{
    ObjHead *p = obj;
    s16 v;

    switch (p->motionId[0]) {
    case 0x327:
        if (Motion_ReachedFrame(p, 0, 20.0f) == 0) {
            v = 0x44;
        } else if (Motion_ReachedFrame(p, 0, 30.0f) == 0) {
            v = 0x83;
        } else {
            v = 0x82;
        }
        break;
    case 0x328:
        if (Motion_ReachedFrame(p, 0, 20.0f) == 0) {
            v = 0x83;
        } else if (Motion_ReachedFrame(p, 0, 26.0f) == 0) {
            v = 0x82;
        } else if (Motion_ReachedFrame(p, 0, 28.0f) == 0) {
            v = 0x83;
        } else {
            v = 0x44;
        }
        break;
    case 0x329:
        if (Motion_ReachedFrame(p, 0, 36.0f) == 0) {
            v = 0x83;
        } else if (Motion_ReachedFrame(p, 0, 40.0f) == 0) {
            v = 0x82;
        } else if (Motion_ReachedFrame(p, 0, 42.0f) == 0) {
            v = 0x83;
        } else {
            v = 0x44;
        }
        break;
    case 0x32A:
        if (Motion_ReachedFrame(p, 0, 20.0f) == 0) {
            v = 0x83;
        } else if (Motion_ReachedFrame(p, 0, 22.0f) != 0) {
            v = 0x82;
        } else if (Motion_ReachedFrame(p, 0, 24.0f) != 0) {
            v = 0x83;
        } else {
            v = 0x44;
        }
        break;
    default:
        v = 0x44;
        break;
    }
    return Effect_MapTexId(v);
}

s16 Player_IsState(void *obj, u8 state, u8 sub);
struct EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Draw_SetDefaultState(void);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Vec_RotateY(f32 *v, f32 a);
f32 Math_Atan2(f32 y, f32 x);

/* Draws the player's weapon (a part of gCommonModels's model set) at a bone: sheathed by the
 * weapon's kind (+0x533), else in the motions that hold it (bow motions, 0x19F..0x1A9,
 * 0x320..0x32A); in 0x320..0x323 also a second part on the ground in front of the player. */
/* The player comes in as `void *obj`, read through `Player *p = obj;`, as the file's
 * shared-header helpers take it (Player_IsState(void *obj, ...)): a Player * parameter used
 * directly takes s1 and the model set s0, the other way round from the game (162 words).
 * FAKEMATCH: the 0x323 block reaches off[1]/off[2] through the pointers oy/oz. With
 * off[1]/off[2] written there, MWCC's common-subexpression pass gives the block its own copies
 * of the &off[1]/&off[2] values (the block's predecessor uses them before a call): 4
 * instructions more than the game (docs/notes/nm/Player_DrawWeaponPart.md). */
void Player_DrawWeaponPart(void *obj)
{
    Model *set;
    Player *p = obj;
    u8 *lights;
    ModelMatSet *part;
    ModelMatSet *part2;
    s16 drawn;
    s32 onBone;
    s32 idx;
    s16 bone;
    s16 mode;
    s16 stand;
    f32 *oy;
    f32 *oz;

    f32 pos[3];
    f32 off[3];
    f32 m2[16];
    f32 m[16];
    f32 mt[16];
    f32 sx;
    f32 sy;
    f32 sz;
    f32 rx;
    f32 ry;
    f32 rz;
    f32 t;
    f32 x;
    f32 y;
    f32 z;

    drawn = 0;
    mode = 0;
    set = gStageModelSet;
    if (set != NULL && set->active != 0) {
        lights = (u8 *)set->mats;
        sx = 1.0f;
        sy = sx;
        sz = sx;
        if (p->carry & 0xF) {
            if (Player_IsState((u8 *)p, PL_GROUP_CARRY, PL_CARRY_PICKED_UP) != 0 && p->motionId[0] == 0x33 && p->motion[0].blend != 0) {
                return;
            }
            switch ((p->carry & 0xF0) >> 4) {
            case 0:
            case 1:
            case 6:
                bone = 0xE;
                part = &set->matSets[133];
                sy *= 1.3f;
                rx = -0.9424778f;
                ry = -3.2812192f;
                rz = 2.0594885f;
                off[0] = 5.5f;
                off[1] = -28.0f;
                off[2] = 12.0f;
                Mtx_Translation(mt, 0.0f, 0.0625f, 0.0f);
                Gs_SetState(0x19, (u32)mt);
                break;
            case 2:
                rx = -0.9424778f;
                bone = 0xE;
                part = &set->matSets[133];
                ry = -3.2812192f;
                rz = 2.0594885f;
                off[0] = 8.9f;
                off[1] = -25.0f;
                off[2] = 4.6f;
                Mtx_Translation(mt, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (u32)mt);
                break;
            case 3:
            case 4:
            case 5:
                part = &set->matSets[41];
                sx *= 2.5f;
                sy *= 3.0f;
                sz *= 2.0f;
                bone = 0xE;
                rx = -0.24434611f;
                ry = 2.86234f;
                rz = 1.2566371f;
                off[0] = 3.3f;
                off[1] = -24.0f;
                off[2] = 4.0f;
                switch ((p->carry & 0xF0) >> 4) {
                case 5:
                    mode = 3;
                    break;
                case 3:
                    mode = 4;
                    break;
                case 4:
                    mode = 5;
                    break;
                }
                break;
            default:
                return;
            }
        } else if (p->statusIcons & 0x40) {
            rz = 0.0f;
            bone = 2;
            ry = rz;
            rx = rz;
            part = &set->matSets[145];
            off[2] = 0.0f;
            off[1] = 0.0f;
            off[0] = 0.0f;
        } else {
            switch (p->motionId[0]) {
            case 0x19F:
                part = &set->matSets[Effect_MapTexId(0x46)];
                bone = 0xE;
                rx = 0.36651915f;
                ry = 0.17453294f;
                rz = 1.3613569f;
                off[0] = 17.0f;
                off[1] = 0.9f;
                off[2] = 22.0f;
                break;
            case 0x1A0:
                if ((p->motion[0].blend == 0 || p->stateVar1 == 0)
                    && (Motion_ReachedFrame(p, 0, 16.0f) == 0 || Motion_ReachedFrame(p, 0, 164.0f) != 0)) {
                    return;
                }
                switch (p->useItem) {
                case ITEM_ID_FOOD_HEAL_20:
                case ITEM_ID_FOOD_PRIZE_A:
                case ITEM_ID_FOOD_PRIZE_B:
                    part = &set->matSets[114];
                    bone = 0x12;
                    rx = -1.2217306f;
                    ry = 0.08726647f;
                    rz = -0.38397247f;
                    mode = 6;
                    off[0] = -8.0f;
                    off[1] = -3.0f;
                    off[2] = 0.0f;
                    break;
                case ITEM_ID_FOOD_RESTORE:
                case ITEM_ID_FOOD_REGEN_SHORT:
                case ITEM_ID_FOOD_REGEN:
                case ITEM_ID_FOOD_REGEN_B:
                case ITEM_ID_FOOD_REGEN_LONG:
                case ITEM_ID_FOOD_RESTORE_B:
                    sx = 0.75f;
                    sy = sx;
                    part = &set->matSets[128];
                    bone = 0x12;
                    rx = -0.38397247f;
                    ry = 0.296706f;
                    mode = 2;
                    rz = -1.9722221f;
                    off[0] = -17.0f;
                    off[1] = -30.0f;
                    off[2] = 1.0f;
                    sz = sx;
                    break;
                default:
                    part = &set->matSets[58];
                    bone = 0x12;
                    rx = -0.38397247f;
                    ry = 0.296706f;
                    rz = -1.9722221f;
                    mode = 2;
                    off[0] = -17.0f;
                    off[1] = -30.0f;
                    off[2] = 1.0f;
                    break;
                }
                break;
            case 0x1A2:
                if (Motion_ReachedFrame(p, 0, 118.0f) != 0) {
                    return;
                }
                rx = 0.0f;
                bone = 0x12;
                ry = 3.1415927f;
                rz = ry;
                part = &set->matSets[71];
                off[0] = -9.0f;
                off[1] = -1.0f;
                off[2] = 65.0f;
                break;
            case 0x1A3:
            case 0x1A6:
                switch (p->motionId[0]) {
                case 0x1A3:
                    if (Motion_ReachedFrame(p, 0, 26.0f) == 0 || Motion_ReachedFrame(p, 0, 248.0f) != 0) {
                        return;
                    }
                    break;
                case 0x1A6:
                    if (Motion_ReachedFrame(p, 0, 20.0f) == 0) {
                        return;
                    }
                    break;
                }
                rx = 0.0f;
                bone = 0xE;
                ry = rx;
                rz = rx;
                part = &set->matSets[120];
                off[0] = 8.0f;
                off[1] = 0.0f;
                off[2] = -6.0f;
                break;
            case 0x1A4:
            case 0x1A5:
                if (p->motionId[0] == 0x1A4) {
                    if (Motion_ReachedFrame(p, 0, 6.0f) == 0) {
                        return;
                    }
                } else if (p->motionId[0] == 0x1A5) {
                    if (Motion_ReachedFrame(p, 0, 34.0f) != 0) {
                        return;
                    }
                }
                bone = 0x12;
                rx = -1.0821042f;
                ry = -2.9670596f;
                part = &set->matSets[19];
                rz = 0.69813174f;
                off[0] = -8.4f;
                off[1] = -3.8f;
                off[2] = -5.2f;
                break;
            case 0x1A8:
            case 0x1A9:
                if (p->motionId[0] == 0x1A8) {
                    if (p->motion[0].blend != 0) {
                        return;
                    }
                } else if (Motion_ReachedFrame(p, 0, 58.0f) != 0) {
                    return;
                }
                bone = 0x12;
                rx = 2.86234f;
                ry = -0.41887903f;
                part = &set->matSets[83];
                rz = -1.0646509f;
                off[0] = -12.0f;
                off[1] = -7.0f;
                off[2] = -3.0f;
                break;
            case 0x320:
            case 0x321:
            case 0x322:
            case 0x323:
                switch (p->useItem) {
                case 0xA3:
                case 0xA4:
                case 0xA5:
                case 0xA6:
                case 0xA7:
                default:
                    idx = 0x3A;
                    break;
                case 0xA8:
                case 0xA9:
                    idx = 0x80;
                    break;
                }
                stand = 1;
                switch (p->motionId[0]) {
                case 0x320:
                    if (Motion_ReachedFrame(p, 0, 26.0f) == 0) {
                        return;
                    }
                    onBone = 0;
                    break;
                case 0x321:
                    if (Motion_ReachedFrame(p, 0, 106.0f) != 0) {
                        return;
                    }
                    if (Motion_ReachedFrame(p, 0, 96.0f) != 0) {
                        stand = 0;
                    }
                    onBone = 1;
                    break;
                case 0x322:
                    if (Motion_ReachedFrame(p, 0, 100.0f) != 0) {
                        return;
                    }
                    if (Motion_ReachedFrame(p, 0, 70.0f) != 0) {
                        stand = 2;
                    }
                    onBone = 1;
                    break;
                default:
                    onBone = 0;
                    break;
                }
                if (stand) {
                    part2 = &set->matSets[69];
                    Mtx_Translation(m2, p->pos[0], p->floorY, p->pos[2]);
                    Mtx_RotateY(m2, (6.2831855f * p->rot[1]) / 65536.0f);
                    Gs_SetState(0x1A, (u32)m2);
                    Gs_SetState(0x67, -1);
                    Player_DrawPartLit(p, part2, m2, lights, 0);
                    if (stand == 2) {
                        return;
                    }
                }
                if (onBone) {
                    part = &set->matSets[idx];
                    bone = 0xE;
                    rx = 1.0471976f;
                    ry = -0.7330383f;
                    rz = 0.41887903f;
                    off[0] = 37.0f;
                    off[1] = 14.0f;
                    off[2] = 37.0f;
                } else {
                    off[0] = 0.0f;
                    off[1] = 0.0f;
                    off[2] = 128.0f;
                    Vec_RotateY(off, (6.2831855f * p->rot[1]) / 65536.0f);
                    pos[0] = p->pos[0] + off[0];
                    pos[1] = 77.0f + p->floorY;
                    pos[2] = p->pos[2] + off[2];
                    off[0] = -52.0f;
                    off[1] = 0.0f;
                    off[2] = 0.0f;
                    ry = (6.2831855f * (p->rot[1] + 0x7FFF + 0x4001)) / 65536.0f;
                    Vec_RotateY(off, ry);
                    x = pos[0] + off[0];
                    y = pos[1] + off[1];
                    z = pos[2] + off[2];
                    if (p->motionId[0] == 0x323) {
                        oy = &off[1]; /* FAKEMATCH: see above */
                        oz = &off[2];
                        off[0] = 12.0f;
                        *oy = 0.0f;
                        *oz = 9.0f;
                        Mtx_Copy(m2, (void *)Em_GetBone(p, 0xE));
                        Vec_RotateSelf(off, m2);
                        off[0] += m2[12];
                        *oy += m2[13];
                        *oz += m2[14];
                        off[0] -= x;
                        *oy -= y;
                        *oz -= z;
                        Vec_RotateY(off, -ry);
                        rx = Math_Atan2(*oz, *oy);
                    } else {
                        rx = 0.0f;
                    }
                    Mtx_Identity(m2);
                    Mtx_RotateX(m2, rx);
                    Mtx_RotateY(m2, ry);
                    part2 = &set->matSets[60];
                    Mtx_SetTrans(m2, x, y, z);
                    Gs_SetState(0x67, 0xFF727162);
                    Player_DrawPartLit(p, part2, m2, lights, 0);
                    part = &set->matSets[idx];
                    rz = 0.0f;
                    drawn = 1;
                    Mtx_SetTrans(m2, pos[0], pos[1], pos[2]);
                }
                mode = 1;
                break;
            case 0x32A:
                if (Motion_ReachedFrame(p, 0, 22.0f) != 0) {
                    return;
                }
            case 0x324:
            case 0x325:
            case 0x326:
            case 0x327:
            case 0x328:
            case 0x329:
                part = &set->matSets[Player_HeldPartForMotion(p)];
                bone = 0x12;
                rz = -1.5707964f;
                off[0] = -9.2f;
                rx = 0.0f;
                off[1] = -1.7f;
                ry = rx;
                off[2] = 12.0f;
                break;
            case 0x3F1:
            case 0x3F4:
            case 0x3F5:
            case 0x57D:
            case 0x58A:
                if (p->weaponType == WEAPON_BOW) {
                    switch (p->motionId[0]) {
                    case 0x3F4:
                        t = 6.0f;
                        if (Motion_ReachedFrame(p, 0, t) == 0) {
                            return;
                        }
                        break;
                    case 0x3F5:
                        t = 4.0f;
                        if (Motion_ReachedFrame(p, 0, t) == 0) {
                            return;
                        }
                        break;
                    default:
                        t = 0.0f;
                        break;
                    }
                    sx = sy = 1.0f;
                    if (((u32)(p->motionId[0] - 0x3F4) <= 1 || p->motionId[0] == 0x57D || p->motionId[0] == 0x58A)
                        && Motion_ReachedFrame(p, 0, t) != 0 && Motion_ReachedFrame(p, 0, 12.0f + t) == 0) {
                        sz = (0.5f * (p->motion[0].frame - t)) / 12.0f;
                    } else {
                        sz = sx;
                    }
                    bone = 0x12;
                    part = &set->matSets[149];
                    rx = 1.5707964f;
                    ry = 0.0f;
                    rz = -1.5707964f;
                    off[0] = -10.0f;
                    off[1] = 0.0f;
                    off[2] = 0.0f;
                } else {
                    return;
                }
                break;
            case 0x3F6:
            case 0x583:
            case 0x585:
            case 0x586:
                if (p->weaponType == WEAPON_BOW) {
                    switch (p->motionId[0]) {
                    case 0x3F6:
                        if (Motion_ReachedFrame(p, 0, 12.0f) == 0) {
                            return;
                        }
                        break;
                    case 0x583:
                        if (Motion_ReachedFrame(p, 0, 76.0f) != 0) {
                            return;
                        }
                        break;
                    case 0x585:
                        if (Motion_ReachedFrame(p, 0, 12.0f) == 0 || Motion_ReachedFrame(p, 0, 98.0f) != 0) {
                            return;
                        }
                        break;
                    case 0x586:
                        if (Motion_ReachedFrame(p, 0, 100.0f) != 0) {
                            return;
                        }
                        break;
                    }
                    bone = 0x12;
                    part = &set->matSets[149];
                    rx = 3.1415927f;
                    ry = 0.0f;
                    rz = -1.5707964f;
                    off[0] = -10.0f;
                    off[1] = 0.0f;
                    off[2] = 80.0f;
                    sz = 0.9f;
                    sy = sz;
                    sx = sz;
                } else {
                    return;
                }
                break;
            default:
                return;
            }
        }
        if (drawn == 0) {
            Mtx_Copy(m, (void *)Em_GetBone(p, bone));
            Mtx_GetTrans(pos, m);
            Vec_RotateSelf(off, m);
            pos[0] += off[0];
            pos[1] += off[1];
            pos[2] += off[2];
            Mtx_Scale(m2, sx, sy, sz);
            Mtx_RotateXYZ(m2, rx, ry, rz);
            Mtx_Concat3x3(m2, m);
        }
        Mtx_SetTrans(m2, pos[0], pos[1], pos[2]);
        Gs_SetState(0x67, -1);
        Player_DrawPartLit(p, part, m2, lights, mode);
        Draw_SetDefaultState();
    }
}

/* Keyframe tracks as unk_36BE70.c's Key*_Eval read them (ended by a time of -1). */
typedef struct Key1 {
    f32 t;
    f32 v;
} Key1;

typedef struct Key2 {
    f32 t;
    f32 v[2];
} Key2;

typedef struct Key3 {
    f32 t;
    f32 v[3];
} Key3;

/* Where a weapon model sits on its bone: an offset in the bone's frame and a rotation (radians). */
typedef struct WpPose {
    f32 off[3];
    f32 rot[3];
} WpPose;

void Bone_StoreMatrices(struct EmBone *b);
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
s32 Model_Draw(u32 h, s32 flags);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Tex_Prepare(s32 n, s32 tex);
struct KeyClock;
f32 Key1_Eval(struct KeyClock *c, Key1 *k);
void Key2_Eval(struct KeyClock *c, Key2 *k, f32 *a, f32 *b);
void Key3_Eval(struct KeyClock *c, Key3 *k, f32 *out);
void Key3_EvalByTimer(Player *p, Key3 *k, f32 *out);
void Mtx_AddTrans(f32 *m, f32 *v);
s32 Bowgun_HasBarrel(Player *p);
s32 HeavyBowgun_HasShield(Player *p);
s32 LightBowgun_HasSilencer(Player *p);
extern WpPose gWpPoseDrawnAlt[];
extern WpPose gWpPoseDrawn[];
extern WpPose gWpPoseSheathed[];
extern WpPose gWpPoseMotion19D[];
extern Key1 gLbgKey579ScaleZ[];
extern Key1 gHbgKey579ScaleZ[];
extern Key1 gLbgKey579ScaleY[];
extern Key1 gHbgKey579ScaleX[];
extern Key1 gLbgKey57CScaleZ[];
extern Key1 gHbgKey57CScaleZ[];
extern Key1 gLbgKey57CScaleY[];
extern Key1 gHbgKey57CScaleX[];
extern Key1 gLbgKey57DEmptyScaleZ[];
extern Key1 gHbgKey57DEmptyScaleZ[];
extern Key1 gLbgKey57DEmptyScaleY[];
extern Key1 gHbgKey57DEmptyScaleX[];
extern Key1 gLbgKey57DScaleZ[];
extern Key1 gHbgKey57DScaleZ[];
extern Key1 gLbgKey57DScaleY[];
extern Key1 gHbgKey57DScaleX[];
extern Key1 gBowgunKey580ScaleZ[];
extern Key1 gBowgunKey580Scale2[];
extern Key1 gHbgKey3EARotX[];
extern Key1 gHbgKey3EAScaleZ[];
extern Key1 gHbgKey3EAScaleX[];
extern Key1 gHbgKey3EBRotX[];
extern Key1 gHbgKey3F1ScaleZ[];
extern Key1 gHbgKey3EBScaleX[];
extern Key3 gLbgKey3EARot[];
extern Key3 gLbgKey3EAOff[];
extern Key3 gLbgKey3EBRot[];
extern Key3 gLbgKey3EBOff[];
extern Key3 gLbgKey3F1Rot[];
extern Key3 gLbgKey3F1Off[];
extern Key3 gLbgKey3F2Rot[];
extern Key3 gLbgKey3F2Off[];
extern Key1 gLanceKey19DScaleZ[];
extern Key1 gLanceKey3EAScaleZ[];
extern Key1 gLanceKey3EBScaleZ[];
extern Key1 gLanceKey3F1ScaleZ[];
extern Key1 gLanceKey3F2ScaleZ[];
extern Key1 gLanceKey3FAScaleZ[];
extern Key1 gHammerKey57EScale[];
extern Key1 gHammerKey57CScale[];
extern Key1 gGsKey19DScaleZ[];
extern Key1 gGsKey3EAScaleZ[];
extern Key1 gGsKey3EBScaleZ[];
extern Key1 gGsKey3F9ScaleZ[];
extern Key1 gGsKey582ScaleZ[];
extern Key2 gGsKey19DScaleXZ[];
extern Key2 gGsKey3EAScaleXZ[];
extern Key2 gGsKey3EBScaleXZ[];
extern Key2 gGsKey3F9ScaleXZ[];
extern Key2 gGsKey582ScaleXZ[];
extern Key2 gHornKey57DRot[];
extern Key3 gHornKey57DScale[];
extern Key1 gGlKey19DRotX[];
extern Key1 gGlKey3EARotX[];
extern Key1 gGlKey3EBRotX[];
extern Key1 gGlKey3F2RotX[];
extern Key1 gGlKey57CRotX[];
extern Key3 gGlKey580Rot[];
extern Key3 gGlKeyTimerRot[];
extern Key1 gBowKey3EARotX[];
extern Key1 gBowKey3EBRotX[];
extern Key1 gBowKey3EDRotX[];
extern Key1 gBowKey3F4RotX[];
extern Key1 gBowKey3F6RotX[];
extern Key1 gBowKey57DRotX[];
extern Key1 gBowKey580RotX[];
extern Key1 gBowKey587RotX[];
extern Key1 gBowKey58ARotX[];
extern Key1 gBowKey3EAScaleZ[];
extern Key1 gBowKey3EBScaleZ[];
extern Key1 gBowKey3EDScaleZ[];
extern Key1 gBowKey3F4ScaleZ[];
extern Key1 gBowKey3F6ScaleZ[];
extern Key1 gBowKey57DScaleZ[];
extern Key1 gBowKey580ScaleZ[];
extern Key1 gBowKey587ScaleZ[];
extern Key1 gBowKey58AScaleZ[];

#pragma optimization_level 4
/* Draws the player's weapon model (p->weaponModel): places it on the hand, back or holster record by the
 * motion phase and weapon pose tables, animates its moving bones by weapon type and motion (keyframe
 * tracks while unk184 is clear), then draws its parts with alpha, hiding parts by weapon state; the
 * lance, sword and shield and dual swords draw a second model (their shield or second blade) and the
 * long sword, bow and gunlance their sheath or quiver. */
void Player_DrawWeapon(Player *p, s32 flag, f32 alpha)
{
    Model *md;
    ModelMatSet *part;
    WpPose *e;
    ModelMat *light;
    f32 *src;
    s32 i;
    s32 j;
    s16 phase;
    s16 phase1;
    f32 scale;
    EmBone *b;
    ModelMat *lights;
    f32 id[16];
    f32 m[16];
    f32 sm[16];
    f32 off[3];
    f32 pos[3];
    f32 v[3];
    f32 sc[3];
    f32 sc2[3];
    f32 rot[3];

    if (p->weaponModel == NULL) {
        return;
    }
    Gs_SetState(0x67, -1);
    Tex_Prepare(4, p->index + 0x21);
    phase = Player_MotionPhase(p, 0, flag);
    scale = 1.0f;
    md = p->weaponModel;
    b = md->partBones[0];
    part = md->matSets;
    if (phase != 2) {
        Mtx_Copy(m, p->partRecs[phase != 0 ? 0xE : 0x12]->m1);
        if (phase == 0) {
            e = &gWpPoseDrawnAlt[p->weaponType];
        } else {
            e = &gWpPoseDrawn[p->weaponType];
        }
        if (p->motionId[0] == 0x19D) {
            e = &gWpPoseMotion19D[p->weaponType];
        }
        src = e->off;
        off[0] = *src++;
        off[1] = *src++;
        off[2] = *src++;
        rot[0] = *src++;
        rot[1] = *src++;
        rot[2] = *src++;
        Vec_Rotate(v, off, p->partRecs[phase != 0 ? 0xE : 0x12]->m1);
        Mtx_AddTrans(m, v);
        Mtx_Identity(id);
        if (p->weaponType == WEAPON_LANCE || p->weaponType == WEAPON_LIGHT_BOWGUN) {
            Mtx_GetTrans(pos, b[2].local);
            Mtx_Scale(b[2].local, 1.0f, 1.0f, 1.0f);
            Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
        }
    } else {
        switch (p->weaponType) {
        case WEAPON_SWORD_SHIELD:
            Mtx_Copy(m, p->partRecs[9]->m1);
            break;
        case WEAPON_DUAL_SWORDS:
            Mtx_Copy(m, p->partRecs[9]->m1);
            break;
        default:
            Mtx_Copy(m, p->partRecs[10]->m1);
            break;
        }
        if ((s32)p->weaponType == WEAPON_LIGHT_BOWGUN
            && (p->motionId[0] == 0x58E || p->motionId[0] == 0x58F || p->motionId[0] == 0x3EA || p->motionId[0] == 0x3EB
                || p->motionId[0] == 0x3F1 || p->motionId[0] == 0x3F2)
            && p->motion[0].blend == 0) {
            switch (p->motionId[0]) {
            case 0x3EA:
            case 0x58E:
            case 0x58F:
                Key3_Eval((struct KeyClock *)p, gLbgKey3EAOff, off);
                Key3_Eval((struct KeyClock *)p, gLbgKey3EARot, rot);
                break;
            case 0x3EB:
                Key3_Eval((struct KeyClock *)p, gLbgKey3EBOff, off);
                Key3_Eval((struct KeyClock *)p, gLbgKey3EBRot, rot);
                break;
            case 0x3F1:
                Key3_Eval((struct KeyClock *)p, gLbgKey3F1Off, off);
                Key3_Eval((struct KeyClock *)p, gLbgKey3F1Rot, rot);
                break;
            case 0x3F2:
                Key3_Eval((struct KeyClock *)p, gLbgKey3F2Off, off);
                Key3_Eval((struct KeyClock *)p, gLbgKey3F2Rot, rot);
                break;
            }
            rot[0] = 6.2831855f * rot[0] / 360.0f;
            rot[1] = 6.2831855f * rot[1] / 360.0f;
            rot[2] = 6.2831855f * rot[2] / 360.0f;
        } else {
            e = &gWpPoseSheathed[(s32)p->weaponType];
            src = e->off;
            off[0] = *src++;
            off[1] = *src++;
            off[2] = *src++;
            rot[0] = *src++;
            rot[1] = *src++;
            rot[2] = *src++;
        }
        if (p->weaponType == WEAPON_DUAL_SWORDS) {
            Vec_Rotate(v, off, p->partRecs[9]->m1);
        } else {
            Vec_Rotate(v, off, p->partRecs[10]->m1);
        }
        Mtx_AddTrans(m, v);
        switch (p->weaponType) {
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_BOW:
            Mtx_GetTrans(pos, b[3].local);
            Mtx_Scale(b[3].local, 1.0f, 0.0f, 3.0f);
            Mtx_SetTrans(b[3].local, pos[0], pos[1], pos[2]);
            scale = 0.8f;
            break;
        case WEAPON_LIGHT_BOWGUN:
            Mtx_GetTrans(pos, b[3].local);
            Mtx_Scale(b[3].local, 1.0f, 0.0f, 3.0f);
            Mtx_SetTrans(b[3].local, pos[0], pos[1], pos[2]);
            Mtx_GetTrans(pos, b[2].local);
            Mtx_Scale(b[2].local, 1.0f, 1.0f, 0.5f);
            Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
            scale = 1.0f;
            break;
        case WEAPON_LONG_SWORD:
        default:
            scale = 0.8f;
            break;
        case WEAPON_SWORD_SHIELD:
        case WEAPON_DUAL_SWORDS:
            scale = 1.0f;
            break;
        case WEAPON_LANCE:
            scale = 0.9f;
            Mtx_GetTrans(pos, b[2].local);
            Mtx_Scale(b[2].local, 1.0f, 1.0f, 0.45f);
            Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
            break;
        }
    }
    Mtx_Identity(b[0].local);
    Mtx_SetRotXYZ(b[0].local, rot[0], rot[1], rot[2]);
    Mtx_Scale(sm, scale, scale, scale);
    Mtx_Concat3x3(b[0].local, sm);
    switch (p->weaponType) {
    case WEAPON_HAMMER:
        Mtx_GetTrans(pos, b[3].local);
        sc[0] = 1.0f;
        if (p->weaponTimer > 0 && p->weaponTimer / 30 >= 2) {
            sc[0] = 1.5f;
        }
        switch (p->motionId[0]) {
        case 0x57C:
            if (p->motion[0].blend == 0) {
                sc[0] = Key1_Eval((struct KeyClock *)p, gHammerKey57CScale);
            } else {
                sc[0] = 1.5f;
            }
            break;
        case 0x57E:
            if (p->motion[0].blend == 0) {
                sc[0] = Key1_Eval((struct KeyClock *)p, gHammerKey57EScale);
            } else {
                sc[0] = 1.5f;
            }
            break;
        }
        Mtx_Scale(b[3].local, sc[0], sc[0], sc[0]);
        Mtx_SetTrans(b[3].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_HUNTING_HORN:
        Mtx_GetTrans(off, b[2].local);
        Mtx_GetTrans(pos, b[3].local);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        Vec_Set(sc, 1.0f, 1.0f, 1.0f);
        switch (p->motionId[0]) {
        case 0x57D:
        case 0x57E:
        case 0x57F:
            Key2_Eval((struct KeyClock *)p, gHornKey57DRot, &rot[0], &rot[2]);
            Key3_Eval((struct KeyClock *)p, gHornKey57DScale, sc);
            break;
        }
        rot[0] = 6.2831855f * rot[0] / 360.0f;
        rot[1] = 6.2831855f * rot[1] / 360.0f;
        rot[2] = 6.2831855f * rot[2] / 360.0f;
        Mtx_Scale(b[2].local, 1.0f, 1.0f, 1.0f);
        Mtx_SetRotXYZ(b[2].local, rot[0], rot[1], rot[2]);
        Mtx_SetTrans(b[2].local, off[0], off[1], off[2]);
        Mtx_Scale(b[3].local, sc[0], sc[1], sc[2]);
        Mtx_SetTrans(b[3].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_GREAT_SWORD:
    case WEAPON_LONG_SWORD:
        Mtx_GetTrans(pos, b[2].local);
        Mtx_GetTrans(off, b[3].local);
        if (p->weaponOut == 0 || phase == 2) {
            Vec_Set(sc2, 1.0f, 1.0f, 1.0f);
        } else {
            Vec_Set(sc2, 2.6f, 1.0f, 2.5f);
        }
        switch (p->motionId[0]) {
        case 0x19D:
            if (p->motion[0].blend == 0) {
                sc[2] = Key1_Eval((struct KeyClock *)p, gGsKey19DScaleZ);
                Key2_Eval((struct KeyClock *)p, gGsKey19DScaleXZ, &sc2[0], &sc2[2]);
            } else {
                sc[2] = 0.73f;
            }
            break;
        case 0x3EA:
        case 0x3F8:
        case 0x582:
            if (p->motion[0].blend == 0) {
                if (p->motionId[0] != 0x582) {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gGsKey3EAScaleZ);
                    Key2_Eval((struct KeyClock *)p, gGsKey3EAScaleXZ, &sc2[0], &sc2[2]);
                } else {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gGsKey582ScaleZ);
                    Key2_Eval((struct KeyClock *)p, gGsKey582ScaleXZ, &sc2[0], &sc2[2]);
                }
            } else {
                sc[2] = 0.73f;
                Vec_Set(sc2, 1.0f, 1.0f, 1.0f);
            }
            break;
        case 0x3EB:
        case 0x3F9:
            if (p->motion[0].blend == 0) {
                if (p->motionId[0] == 0x3EB) {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gGsKey3EBScaleZ);
                    Key2_Eval((struct KeyClock *)p, gGsKey3EBScaleXZ, &sc2[0], &sc2[2]);
                } else {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gGsKey3F9ScaleZ);
                    Key2_Eval((struct KeyClock *)p, gGsKey3F9ScaleXZ, &sc2[0], &sc2[2]);
                }
            } else {
                sc[2] = 1.0f;
                Vec_Set(sc2, 2.6f, 1.0f, 2.5f);
            }
            break;
        default:
            if (p->weaponOut == 0 || phase == 2) {
                sc[2] = 0.73f;
            } else {
                sc[2] = 1.0f;
            }
            break;
        }
        if (p->weaponType == WEAPON_LONG_SWORD) {
            if (p->weaponOut == 0 || phase == 2) {
                sc[2] = 0.73f;
            } else {
                sc[2] = 1.0f;
            }
        }
        Mtx_Scale(b[2].local, 1.0f, 1.0f, sc[2]);
        Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
        Mtx_Scale(b[3].local, sc2[0], sc2[1], sc2[2]);
        Mtx_SetTrans(b[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_LANCE:
        Mtx_GetTrans(pos, b[2].local);
        switch (p->motionId[0]) {
        case 0x19D:
            if (p->motion[0].blend == 0) {
                sc[2] = Key1_Eval((struct KeyClock *)p, gLanceKey19DScaleZ);
            } else {
                sc[2] = 0.45f;
            }
            break;
        case 0x3EA:
        case 0x3F2:
            if (p->motion[0].blend == 0) {
                if (p->motionId[0] != 0x3F2) {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gLanceKey3EAScaleZ);
                } else {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gLanceKey3F2ScaleZ);
                }
            } else {
                sc[2] = 0.45f;
            }
            break;
        case 0x3EB:
        case 0x3F1:
            if (p->motion[0].blend == 0) {
                if (p->motionId[0] == 0x3EB) {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gLanceKey3EBScaleZ);
                } else {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gLanceKey3F1ScaleZ);
                }
            } else {
                sc[2] = 1.0f;
            }
            break;
        case 0x3FA:
            if (p->motion[0].blend == 0) {
                sc[2] = Key1_Eval((struct KeyClock *)p, gLanceKey3FAScaleZ);
            } else {
                sc[2] = 0.45f;
            }
            break;
        default:
            if (p->weaponOut == 0 || phase == 2) {
                sc[2] = 0.45f;
            } else {
                sc[2] = 1.0f;
            }
            break;
        }
        Mtx_Scale(b[2].local, 1.0f, 1.0f, sc[2]);
        Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_GUNLANCE:
        Mtx_GetTrans(pos, b[2].local);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        Mtx_GetTrans(pos, b[2].local);
        Mtx_GetTrans(off, b[3].local);
        switch (p->motionId[0]) {
        case 0x19D:
            if (p->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)p, gGlKey19DRotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 0x3EA:
            if (p->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)p, gGlKey3EARotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 0x3F2:
            if (p->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)p, gGlKey3F2RotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 0x3EB:
        case 0x3F1:
            if (p->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)p, gGlKey3EBRotX);
            } else {
                rot[0] = 0.0f;
            }
            break;
        case 0x57C:
            if (p->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)p, gGlKey57CRotX);
            } else {
                rot[0] = 0.0f;
            }
            break;
        default:
            if (p->weaponOut == 0 || phase == 2) {
                rot[0] = 180.0f;
            } else {
                rot[0] = 0.0f;
            }
            break;
        }
        Mtx_SetRotXYZ(b[2].local, 6.2831855f * rot[0] / 360.0f, 0.0f, 0.0f);
        Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
        if (gScene.online != 0) {
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        } else if (p->weaponTimer > 0) {
            if (p->motionId[0] == 0x580) {
                Key3_Eval((struct KeyClock *)p, gGlKey580Rot, rot);
            } else {
                Vec_Set(rot, 69.0f, -43.0f, -30.0f);
            }
        } else if (p->swordGaugeTimer > 0) {
            Key3_EvalByTimer(p, gGlKeyTimerRot, rot);
        } else {
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        }
        Mtx_SetRotXYZ(b[3].local, 6.2831855f * rot[0] / 360.0f, 6.2831855f * rot[1] / 360.0f,
                      6.2831855f * rot[2] / 360.0f);
        Mtx_SetTrans(b[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_BOW:
        Mtx_GetTrans(pos, b[2].local);
        Mtx_GetTrans(off, b[3].local);
        Vec_Set(sc, 1.0f, 1.0f, 1.0f);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        if (p->motion[0].blend == 0) {
            switch (p->motionId[0]) {
            case 0x3EA:
            case 0x3F2:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey3EARotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey3EAScaleZ);
                break;
            case 0x3EB:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey3EBRotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey3EBScaleZ);
                break;
            case 0x3ED:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey3EDRotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey3EDScaleZ);
                break;
            case 0x3F1:
                rot[0] = 120.0f;
                sc[2] = 2.35f;
                break;
            case 0x3F4:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey3F4RotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey3F4ScaleZ);
                break;
            case 0x3F6:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey3F6RotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey3F6ScaleZ);
                break;
            case 0x57D:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey57DRotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey57DScaleZ);
                break;
            case 0x580:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey580RotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey580ScaleZ);
                break;
            case 0x587:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey587RotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey587ScaleZ);
                break;
            case 0x58A:
                rot[0] = Key1_Eval((struct KeyClock *)p, gBowKey58ARotX);
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowKey58AScaleZ);
                break;
            default:
                if (p->weaponOut == 0 || phase == 2) {
                    rot[0] = 0.0f;
                } else {
                    rot[0] = 155.0f;
                }
                break;
            }
        } else {
            if ((p->motionId[0] != 0x3EB && p->motionId[0] != 0x3ED && (p->weaponOut == 0 || phase == 2))
                || p->motionId[0] == 0x3EA) {
                rot[0] = 0.0f;
            } else {
                rot[0] = 155.0f;
            }
            if (p->motionId[0] == 0x580) {
                rot[0] = 110.0f;
                sc[2] = 2.7f;
            }
        }
        Mtx_SetRotXYZ(b[2].local, 6.2831855f * rot[0] / 360.0f, 0.0f, 0.0f);
        Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
        Mtx_Scale(b[3].local, sc[0], sc[1], sc[2]);
        Mtx_SetTrans(b[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
        if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
            Mtx_GetTrans(off, b[3].local);
        } else {
            Mtx_GetTrans(off, b[3].local);
            Mtx_SetRotXYZ(b[2].local, 3.1415927f, 0.0f, 0.0f);
        }
        sc[0] = 1.0f;
        sc[1] = 1.0f;
        sc[2] = 1.0f;
        if (p->motion[0].blend == 0) {
            switch (p->motionId[0]) {
            case 0x579:
                if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gLbgKey579ScaleZ);
                    sc[1] = Key1_Eval((struct KeyClock *)p, gLbgKey579ScaleY);
                } else {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gHbgKey579ScaleZ);
                    sc[0] = Key1_Eval((struct KeyClock *)p, gHbgKey579ScaleX);
                }
                break;
            case 0x57D:
                if (p->roundsLoaded == 0) {
                    if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                        sc[2] = Key1_Eval((struct KeyClock *)p, gLbgKey57DEmptyScaleZ);
                        sc[1] = Key1_Eval((struct KeyClock *)p, gLbgKey57DEmptyScaleY);
                    } else {
                        sc[2] = Key1_Eval((struct KeyClock *)p, gHbgKey57DEmptyScaleZ);
                        sc[0] = Key1_Eval((struct KeyClock *)p, gHbgKey57DEmptyScaleX);
                    }
                } else if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gLbgKey57DScaleZ);
                    sc[1] = Key1_Eval((struct KeyClock *)p, gLbgKey57DScaleY);
                } else {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gHbgKey57DScaleZ);
                    sc[0] = Key1_Eval((struct KeyClock *)p, gHbgKey57DScaleX);
                }
                break;
            case 0x580:
                sc[2] = Key1_Eval((struct KeyClock *)p, gBowgunKey580ScaleZ);
                if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                    sc[1] = Key1_Eval((struct KeyClock *)p, gBowgunKey580Scale2);
                } else {
                    sc[0] = Key1_Eval((struct KeyClock *)p, gBowgunKey580Scale2);
                }
                break;
            case 0x57C:
                if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gLbgKey57CScaleZ);
                    sc[1] = Key1_Eval((struct KeyClock *)p, gLbgKey57CScaleY);
                } else {
                    sc[2] = Key1_Eval((struct KeyClock *)p, gHbgKey57CScaleZ);
                    sc[0] = Key1_Eval((struct KeyClock *)p, gHbgKey57CScaleX);
                }
                break;
            case 0x3EA:
            case 0x58E:
            case 0x58F:
                if (p->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(b[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)p, gHbgKey3EARotX) / 360.0f, 0.0f, 0.0f);
                    sc[2] = Key1_Eval((struct KeyClock *)p, gHbgKey3EAScaleZ);
                    sc[0] = Key1_Eval((struct KeyClock *)p, gHbgKey3EAScaleX);
                }
                break;
            case 0x3EB:
                if (p->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(b[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)p, gHbgKey3EBRotX) / 360.0f, 0.0f, 0.0f);
                    sc[0] = Key1_Eval((struct KeyClock *)p, gHbgKey3EBScaleX);
                }
                break;
            case 0x3F1:
                if (p->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(b[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)p, gHbgKey3EBRotX) / 360.0f, 0.0f, 0.0f);
                    sc[2] = Key1_Eval((struct KeyClock *)p, gHbgKey3F1ScaleZ);
                    sc[0] = Key1_Eval((struct KeyClock *)p, gHbgKey3EBScaleX);
                }
                break;
            default:
                if (p->weaponOut == 0 || phase == 2) {
                    Mtx_SetRotXYZ(b[1].local, 0.0f, 0.0f, 0.0f);
                    if (p->weaponType == WEAPON_HEAVY_BOWGUN) {
                        Mtx_SetRotXYZ(b[2].local, 0.0f, 0.0f, 0.0f);
                    }
                    Mtx_SetRotXYZ(b[3].local, 0.0f, 0.0f, 0.0f);
                    if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                        sc[1] = 0.4f;
                        sc[2] = 2.2f;
                    } else {
                        sc[0] = -0.5f;
                        sc[2] = 3.0f;
                    }
                } else {
                    if (p->roundsLoaded != 0) {
                        if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                            sc[1] = 0.55f;
                            sc[2] = 2.0f;
                        } else {
                            sc[0] = 0.8f;
                            sc[2] = 3.0f;
                        }
                    }
                }
                break;
            }
        } else {
            if ((p->roundsLoaded != 0 && p->motionId[0] != 0x57C) || p->motionId[0] == 0x579) {
                if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                    sc[1] = 0.55f;
                    sc[2] = 2.0f;
                } else {
                    sc[0] = 0.8f;
                    sc[2] = 3.0f;
                }
            }
            if ((p->motionId[0] != 0x3EB && p->motionId[0] != 0x3F1 && (p->weaponOut == 0 || phase == 2))
                || p->motionId[0] == 0x3EA || p->motionId[0] == 0x58E || p->motionId[0] == 0x58F) {
                Mtx_SetRotXYZ(b[1].local, 0.0f, 0.0f, 0.0f);
                if (p->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(b[2].local, 0.0f, 0.0f, 0.0f);
                }
                Mtx_SetRotXYZ(b[3].local, 0.0f, 0.0f, 0.0f);
                if (p->weaponType == WEAPON_LIGHT_BOWGUN) {
                    sc[1] = 0.4f;
                    sc[2] = 2.2f;
                } else {
                    sc[0] = -0.5f;
                    sc[2] = 3.0f;
                }
            }
        }
        Mtx_Scale(b[3].local, sc[0], sc[1], sc[2]);
        Mtx_SetTrans(b[3].local, off[0], off[1], off[2]);
        break;
    }
    Bone_UpdateWorldScaledRoot(md->partBones[0], m);
    Bone_StoreMatrices(md->partBones[0]);
    lights = p->weaponModel->mats;
    phase1 = Player_MotionPhase(p, 1, flag);
    for (i = 0; i < md->matSetCount; i++, part++) {
        if (i != 0 && (p->weaponType == WEAPON_LANCE || p->weaponType == WEAPON_SWORD_SHIELD || p->weaponType == WEAPON_DUAL_SWORDS)) {
            b = md->partBones[1];
            Mtx_Copy(m, (void *)Em_GetBone(p, 0x11));
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
            switch (p->weaponType) {
            case WEAPON_SWORD_SHIELD:
                Mtx_SetRotXYZ(b[0].local, -0.45378563f, -0.05235988f, 0.13962634f);
                off[0] = -21.0f;
                off[1] = 2.2f;
                off[2] = 5.0f;
                break;
            case WEAPON_LANCE:
                Mtx_SetRotXYZ(b[0].local, 0.36651915f, -3.0019662f, -0.043633234f);
                off[0] = -18.0f;
                off[1] = 2.0f;
                off[2] = -14.0f;
                break;
            case WEAPON_DUAL_SWORDS:
                if (phase1 != 2) {
                    Mtx_Copy(m, (void *)Em_GetBone(p, 0x12));
                    Mtx_SetRotXYZ(b[0].local, -0.45378563f, -0.05235988f, 0.13962634f);
                    off[0] = -7.4f;
                    off[1] = -0.4f;
                    off[2] = -6.5f;
                    rot[0] = 0.0f;
                    rot[1] = 0.0f;
                    rot[2] = 3.1415927f;
                } else {
                    Mtx_Copy(m, p->partRecs[9]->m1);
                    off[0] = 17.0f;
                    off[1] = 50.0f;
                    off[2] = -18.0f;
                    rot[0] = 1.6755161f;
                    rot[1] = 0.148353f;
                    rot[2] = -0.48869222f;
                }
                break;
            }
            Vec_Rotate(v, off, m);
            Mtx_AddTrans(m, v);
            if (p->weaponType == WEAPON_DUAL_SWORDS) {
                Mtx_Identity(md->partBones[1][0].local);
                Mtx_SetRotXYZ(md->partBones[1][0].local, rot[0], rot[1], rot[2]);
            }
            Bone_UpdateWorldScaledRoot(md->partBones[1], m);
            Bone_StoreMatrices(md->partBones[1]);
        }
        if (part->id != -1) {
            for (j = 0; j < part->count; j++) {
                light = &lights[part->mat[j]];
                light->alpha = alpha;
                if (p->weaponType == WEAPON_HEAVY_BOWGUN || p->weaponType == WEAPON_LIGHT_BOWGUN) {
                    if (j == 1 && Bowgun_HasBarrel(p) == 0) {
                        light->alpha = 0.0f;
                    }
                    if (j == 2) {
                        if (p->weaponType == WEAPON_HEAVY_BOWGUN && HeavyBowgun_HasShield(p) == 0) {
                            light->alpha = 0.0f;
                        } else if (p->weaponType == WEAPON_LIGHT_BOWGUN && LightBowgun_HasSilencer(p) == 0) {
                            light->alpha = 0.0f;
                        }
                    }
                } else {
                    switch (p->weaponType) {
                    case WEAPON_LONG_SWORD:
                        if (phase1 != 2) {
                            switch (j) {
                            case 0:
                            case 1:
                            case 2:
                                light->alpha = alpha;
                                break;
                            default:
                                light->alpha = 0.0f;
                                break;
                            }
                        } else {
                            switch (j) {
                            case 0:
                            case 3:
                            case 4:
                                light->alpha = alpha;
                                break;
                            default:
                                light->alpha = 0.0f;
                                break;
                            }
                        }
                        break;
                    case WEAPON_BOW:
                        if (j > 0) {
                            light->alpha = alpha;
                        } else {
                            light->alpha = 0.0f;
                        }
                        break;
                    case WEAPON_GUNLANCE:
                        if (j > 0) {
                            light->alpha = alpha;
                        } else {
                            light->alpha = 0.0f;
                        }
                        break;
                    }
                }
                Gs_SetState((u8)(j + 0x3A), (u32)light);
            }
        }
        Model_Draw(part->id, 0);
        if (p->weaponType == WEAPON_LONG_SWORD && phase1 != 2) {
            Mtx_Copy(m, p->partRecs[10]->m1);
            e = &gWpPoseSheathed[p->weaponType];
            src = e->off;
            off[0] = *src++;
            off[1] = *src++;
            off[2] = *src++;
            rot[0] = *src++;
            rot[1] = *src++;
            rot[2] = *src++;
            Vec_Rotate(v, off, p->partRecs[10]->m1);
            Mtx_AddTrans(m, v);
            Mtx_Identity(b[0].local);
            Mtx_SetRotXYZ(b[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(sm, 0.8f, 0.8f, 0.8f);
            Mtx_Concat3x3(b[0].local, sm);
            Mtx_GetTrans(pos, b[2].local);
            Mtx_GetTrans(off, b[3].local);
            Vec_Set(sc2, 1.0f, 1.0f, 1.0f);
            sc[2] = 0.73f;
            Mtx_Scale(b[2].local, 1.0f, 1.0f, sc[2]);
            Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(b[3].local, sc2[0], sc2[1], sc2[2]);
            Mtx_SetTrans(b[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(md->partBones[0], m);
            Bone_StoreMatrices(md->partBones[0]);
            lights = p->weaponModel->mats;
            if (part->id != -1) {
                for (j = 0; j < part->count; j++) {
                    light = &lights[part->mat[j]];
                    switch (j) {
                    case 3:
                    case 4:
                        light->alpha = alpha;
                        break;
                    default:
                        light->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)(j + 0x3A), (u32)light);
                }
            }
            Model_Draw(part->id, 0);
        }
        if (p->weaponType == WEAPON_BOW) {
            Mtx_Copy(m, p->partRecs[2]->m1);
            off[0] = 8.5f;
            off[1] = 0.5f;
            off[2] = -20.0f;
            rot[0] = -0.17453294f;
            rot[1] = 0.0f;
            rot[2] = -0.27925268f;
            Vec_Rotate(v, off, p->partRecs[2]->m1);
            Mtx_AddTrans(m, v);
            Mtx_Identity(b[0].local);
            Mtx_SetRotXYZ(b[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(sm, 1.0f, 1.0f, 1.0f);
            Mtx_Concat3x3(b[0].local, sm);
            Mtx_GetTrans(pos, b[2].local);
            Mtx_GetTrans(off, b[3].local);
            Vec_Set(sc2, 1.0f, 1.0f, 1.0f);
            sc[2] = 1.0f;
            Mtx_Scale(b[2].local, 1.0f, 1.0f, sc[2]);
            Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(b[3].local, sc2[0], sc2[1], sc2[2]);
            Mtx_SetTrans(b[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(md->partBones[0], m);
            Bone_StoreMatrices(md->partBones[0]);
            lights = p->weaponModel->mats;
            if (part->id != -1) {
                for (j = 0; j < part->count; j++) {
                    light = &lights[part->mat[j]];
                    switch (j) {
                    case 0:
                        light->alpha = alpha;
                        break;
                    default:
                        light->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)(j + 0x3A), (u32)light);
                }
            }
            Model_Draw(part->id, 0);
        }
        if (p->weaponType == WEAPON_GUNLANCE) {
            Mtx_Copy(m, p->partRecs[17]->m1);
            off[0] = -18.0f;
            off[1] = 2.0f;
            off[2] = -14.0f;
            rot[0] = 0.36651915f;
            rot[1] = 3.0019662f;
            rot[2] = -0.043633234f;
            Vec_Rotate(v, off, p->partRecs[17]->m1);
            Mtx_AddTrans(m, v);
            Mtx_Identity(b[0].local);
            Mtx_SetRotXYZ(b[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(sm, 1.0f, 1.0f, 1.0f);
            Mtx_Concat3x3(b[0].local, sm);
            Mtx_GetTrans(pos, b[2].local);
            Mtx_GetTrans(off, b[3].local);
            Vec_Set(sc2, 1.0f, 1.0f, 1.0f);
            sc[2] = 1.0f;
            Mtx_Scale(b[2].local, 1.0f, 1.0f, sc[2]);
            Mtx_SetTrans(b[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(b[3].local, sc2[0], sc2[1], sc2[2]);
            Mtx_SetTrans(b[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(md->partBones[0], m);
            Bone_StoreMatrices(md->partBones[0]);
            lights = p->weaponModel->mats;
            if (part->id != -1) {
                for (j = 0; j < part->count; j++) {
                    light = &lights[part->mat[j]];
                    switch (j) {
                    case 0:
                        light->alpha = alpha;
                        break;
                    default:
                        light->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)(j + 0x3A), (u32)light);
                }
            }
            Model_Draw(part->id, 0);
        }
    }
}
#pragma optimization_level reset

extern u8 *gPlayerPartFlagTbls[];

/* An entry of the per-gender (6 slots each) table of slot's lists, by pl's armourKind for the slot. */
u8 Player_PartFlags(Player *pl, u8 slot)
{
    return gPlayerPartFlagTbls[pl->gender * 6 + slot][pl->armourKind[slot]];
}

extern u32 gCycleColors[];

/* A colour cycling through the 8 of gCycleColors, 16 frames each (blended by the scene frame counter). */
u32 Color_Cycle8(void)
{
    u8 x;
    u8 c;
    u16 f;
    f32 t;
    f32 s;
    s32 k;
    u32 a;
    u32 b;
    u8 y;
    u32 col;

    col = 0xFF000000;
    f = gScene.frame;
    k = (f >> 4) & 7;
    a = gCycleColors[k];
    s = 1.0f - (t = (f & 0xF) / 16.0f);
    k = (k + 1) & 7;
    b = gCycleColors[k];
    x = a >> 16;
    c = b >> 16;
    x = x * s;
    y = c * t;
    c = x + y;
    col |= c << 16;
    x = a >> 8;
    c = b >> 8;
    x = x * s;
    y = c * t;
    c = x + y;
    col |= c << 8;
    x = a;
    c = b;
    x = x * s;
    y = c * t;
    c = x + y;
    return col | c;
}

/* Fills the material light (its rgb from 0..255 bytes, its alpha f) for the player pl and slot,
 * then hands it to Gs_SetState as id b + 0x3A: slot 2 takes hairColor (a == b == 0), slots 0..5
 * the colour of Color_Cycle8 (style 0xB) or styleColor (Player_PartFlags bit 0, a == 0, b == 1);
 * slot 1 clears alpha by Player_PartFlags bits 2 and 4. */
void Player_SetLight(Player *pl, u8 slot, s32 a, s32 b, ModelMat *light, f32 f)
{
    u32 col;
    u8 m;

    if (pl->style == 0xB) {
        col = Color_Cycle8();
    } else {
        col = pl->styleColor;
    }
    light->alpha = f;
    switch (slot) {
    case 2:
        if (a == 0 && b == 0) {
            light->rgb[0] = (pl->hairColor >> 16 & 0xFF) / 255.0f;
            light->rgb[1] = (pl->hairColor >> 8 & 0xFF) / 255.0f;
            light->rgb[2] = (pl->hairColor & 0xFF) / 255.0f;
        }
        if (pl->torsoAltModel != 0) {
            break;
        }
    case 0:
    case 3:
    case 4:
    case 5:
    default:
        if ((Player_PartFlags(pl, slot) & 1) && a == 0 && b == 1) {
            light->rgb[0] = (col >> 16 & 0xFF) / 255.0f;
            light->rgb[1] = (col >> 8 & 0xFF) / 255.0f;
            light->rgb[2] = (col & 0xFF) / 255.0f;
        }
        break;
    case 1:
        if (pl->torsoAltModel == 0) {
            m = Player_PartFlags(pl, 2);
            if (m & 2) {
                if (a == 0 && b == 0) {
                    light->alpha = 0.0f;
                }
            } else if (m & 4) {
                light->alpha = 0.0f;
            }
        }
        break;
    }
    Gs_SetState((u8)(b + 0x3A), (u32)light);
}

s32 Player_IsLocal(void *obj);
s32 CamCtl_IsAiming(Player *pl);
s32 Scene_InField(void);
s32 CamCtl_GetPlayerFade(f32 *out);
f32 Math_Absf(f32 x);
void Player_DrawWeaponPart(void *obj);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void LightSet_SetAmbient(void *obj, s32 n);
void LightSet_FadeAmbient(void *obj);
void Bone_StoreMatrices(struct EmBone *b);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void LightSet_ResetAmbient(s32 a);
void LightSet_Apply(s32 a);
void Gfx_SetTexFilter(s32 a);
void Tex_Prepare(s32 n, s32 tex);
s32 Debug_IsOn(s32 unused0);
void Player_DrawWeapon(Player *p, s32 a, f32 alpha);
void Player_DrawHeldItem(void *obj);
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
void Mtx_Identity(f32 *m);
struct BoneRoot;
void Bone_SetMatricesRel(struct EmBone *b, struct BoneRoot *r, s16 tbl, s32 a3, f32 *parent);
void Bone_SetMatrices(struct EmBone *b, struct BoneRoot *r, s16 tbl, s32 a3);
void Player_SetLight(Player *pl, u8 slot, s32 a, s32 b, ModelMat *light, f32 f);
extern u16 gPlPartRootBone[];

/* Draws the local player's hunter (Player_Draw draws the others): the body's parts faded to the
 * target alpha of CamCtl_GetPlayerFade (kept in fadeAlpha), the effect Player_DrawWeapon, the five equipment
 * models, the held item (Player_DrawWeaponPart), then the markers Hud_DrawObjMarker/Player_DrawBoneMarker. */
void Player_DrawLocal(Player *p, s32 unused /* a1: DrawCb_LocalPlayer passes 0 */)
{
    s32 i2;
    ModelMatSet *parts;
    ModelMat *lights2;
    s32 i;
    s32 *e2;
    u16 *tbl;
    s32 k;
    s32 j;
    s32 j2;
    ModelMat *lights;
    s32 n;
    f32 alpha;
    f32 m[16];
    s32 sel;
    EmBone *bones;
    Model *eq;
    Model *md;
    ModelMatSet *part;
    f32 target;
    f32 id[16];

    md = p->model;
    if (Player_IsLocal(p) == 1 && CamCtl_IsAiming(p) != 0) {
        return;
    }
    LightSet_SetAmbient(p, 1);
    LightSet_FadeAmbient(p);
    Gs_SetState(0x67, -1);
    Gfx_SetTexFilter(1);
    if (p->nearCamera != 0) {
        Gs_SetState(0x60, 0);
        alpha = 0.2f;
    } else {
        Gs_SetState(0x60, 0xC0);
        alpha = p->alpha;
    }
    if (Player_IsLocal(p) == 1 && Scene_InField() == 1) {
        if (CamCtl_GetPlayerFade(&target) == 1) {
            alpha = p->fadeAlpha + (target - p->fadeAlpha) / 10.0f;
            if (Math_Absf(alpha - target) <= 0.1f) {
                alpha = target;
            }
            Gs_SetState(0x60, alpha < 1.0f ? 0 : 0xC0);
            p->fadeAlpha = alpha;
        } else {
            p->fadeAlpha = 1.0f;
            Gs_SetState(0x60, 0xC0);
            alpha = p->alpha;
        }
    }
    Tex_Prepare(2, p->index * 2 + 0xD);
    part = md->matSets;
    Mtx_Copy(m, p->worldMtx[0]);
    Bone_StoreMatrices(md->bones);
    lights = p->model->mats;
    for (i = 0; i < md->matSetCount; i++, part++) {
        if (p->meshShown[i] != 1) {
            continue;
        }
        if (part->id != -1) {
            for (j = 0; j < part->count; j++) {
                Player_SetLight(p, 0, i, j, &lights[part->mat[j]], alpha);
            }
        }
        Draw_ApplyStates(part->flags);
        Model_Draw(part->id, 0);
    }
    Draw_SetDefaultState();
    Gfx_SetTexFilter(1);
    if (p->weaponShown != 0) {
        Player_DrawWeapon(p, 0, alpha);
    }
    Tex_Prepare(0xC, p->index * 12 + 0x2B);
    for (k = 1, tbl = &gPlPartRootBone[1]; k < 6; k++, tbl++) {
        sel = p->armourKind[k];
        eq = p->partModels[k];
        if (eq == NULL) {
            continue;
        }
        parts = eq->matSets;
        n = eq->matSetCount;
        Bone_UpdateWorldScaledRoot(eq->bones, p->partRecs[*tbl]->m1);
        if (k == 3 || k == 5) {
            Mtx_Identity(id);
            Bone_SetMatricesRel(eq->bones, (struct BoneRoot *) p, k, sel - 1, id);
        } else if (k == 2) {
            bones = eq->bones;
            if (bones->count > 4) {
                Bone_UpdateWorldScaledRoot(&bones[4], p->partRecs[20]->m1);
            }
            Mtx_Identity(id);
            Bone_SetMatricesRel(eq->bones, (struct BoneRoot *) p, k, sel - 1, id);
        } else {
            Bone_SetMatrices(eq->bones, (struct BoneRoot *) p, k, sel - 1);
        }
        lights2 = p->partModels[k]->mats;
        for (i2 = 0; i2 < n; i2++, parts++) {
            if (parts->id != -1) {
                if (0 < parts->count) { /* if + do-while: j2 = 0 in the test slot, the walker after it */
                    j2 = 0;
                    e2 = (s32 *)parts; /* the set as words: e2[2] is mat[j2] (a walker from parts->mat is computed first) */
                    do {
                        Player_SetLight(p, k, i2, j2, &lights2[e2[2]], alpha);
                        j2++;
                        e2++;
                    } while (j2 < parts->count);
                }
            }
            Draw_ApplyStates(parts->flags);
            Model_Draw(parts->id, 0);
        }
        Draw_SetDefaultState();
    }
    Player_DrawWeaponPart(p);
    if (p->nearCamera != 0) {
        Gs_SetState(0x6C, 1);
    }
    Gs_SetState(0x60, 0);
    LightSet_ResetAmbient(1);
    LightSet_Apply(1);
    if (Player_IsLocal(p) != 0 && p->aimMode == 0) {
        if (Player_IsState(p, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(p, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0) {
            if (p->input.held & PAD_L1) {
                Hud_DrawObjMarker(p);
            }
        } else if (p->weaponOut != 0 && p->stateGroup != PL_GROUP_REACT && p->stateGroup != PL_GROUP_GESTURE) {
            switch (p->weaponType) {
            case WEAPON_HEAVY_BOWGUN:
            case WEAPON_LIGHT_BOWGUN:
                if ((p->input.held & PAD_L1) && p->aimLockTimer == 0 && p->roundsLoaded != 0) {
                    Player_DrawBoneMarker(p, 0);
                }
                break;
            case WEAPON_BOW:
                break;
            }
        }
    }
}

s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 Motion_InFrameRange(void *obj, s32 layer, f32 from, f32 to);
struct EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
extern f32 D_0043CB58[];

/* Draws the item the player holds in some states (0x29C: 0xFA, 0x19F, 0x287, 0x324..0x329, 0x349, or
 * while action is 1 in scenes 0x86..0x89): a part of gCommonModels or gAreaSubModels placed at a bone. */
void Player_DrawHeldItem(void *obj)
{
    ObjHead *p = obj;
    f32 m[16];
    f32 m2[16];
    f32 pos[3];
    f32 off[3];
    Model *set;
    ModelMatSet *part;
    u8 other;
    s16 idx;
    s16 bone;
    f32 rx;
    f32 ry;
    f32 rz;
    f32 *t;

    other = 0;
    if (p->action == 1 && p->motionId[0] != 0x260 && p->motionId[0] != 0x296
        && Player_IsState(p, PL_GROUP_ACTION, 0x80) == 0
        && ((u32)(gScene.area - 0x86) <= 2 || gScene.area == AREA_HUB_0x89)) {
        other = 1;
        idx = 0x28;
        bone = 0x12;
        rx = 1.5707964f;
        off[0] = -23.5f;
        off[1] = 0.0f;
        off[2] = -13.5f;
        ry = 0.0f;
        rz = 0.0f;
    } else {
        switch (p->motionId[0]) {
        case 0xFA:
            if (gScene.area == AREA_HUB_0xAC && Motion_InFrameRange(p, 0, 152.0f, 406.0f) != 0) {
                idx = 5;
                bone = 0x12;
                rx = 0.0f;
                off[0] = -8.5f;
                ry = 0.0f;
                rz = 0.0f;
                off[1] = -0.4f;
                off[2] = 1.2f;
            } else {
                goto end;
            }
            break;
        case 0x19F:
            other = 1;
            idx = Effect_MapTexId(0x46);
            bone = 0xE;
            rx = 0.36651915f;
            ry = 0.17453294f;
            rz = 1.3613569f;
            off[0] = 17.0f;
            off[1] = 0.9f;
            off[2] = 22.0f;
            break;
        case 0x287:
            if (gScene.area == AREA_HUB_0x57 && Motion_InFrameRange(p, 0, 232.0f, 578.0f) != 0) {
                idx = 0;
                bone = 0x12;
                rx = 0.0f;
                off[0] = 2.08f;
                off[1] = 3.02f;
                ry = 0.0f;
                off[2] = -2.0f;
                rz = 0.0f;
            } else {
                goto end;
            }
            break;
        case 0x324:
        case 0x325:
        case 0x326:
        case 0x327:
        case 0x328:
        case 0x329:
            other = 1;
            idx = Player_HeldPartForMotion(p);
            bone = 0x12;
            rx = 0.0f;
            ry = 0.0f;
            rz = -1.5707964f;
            off[0] = -9.2f;
            off[1] = -1.7f;
            off[2] = 12.0f;
            break;
        case 0x349:
            bone = 0xE;
            other = 1;
            idx = 0x22;
            t = (f32 *)D_0043CB58; /* the cast keeps the address in a register for t[0] too */
            off[0] = *t++;
            off[1] = *t++;
            off[1] -= 10.0f;
            off[2] = *t++;
            rx = *t++;
            ry = *t++;
            rz = *t++;
            break;
        default:
            return;
        }
    }
    if (other) {
        set = gCommonModels[0];
    } else {
        set = gAreaSubModels[0];
    }
    if (set != NULL && set->active != 0) {
        part = &set->matSets[idx];
        Mtx_Copy(m, (void *)Em_GetBone(p, bone));
        Mtx_GetTrans(pos, m);
        Vec_RotateSelf(off, m);
        pos[0] += off[0];
        pos[1] += off[1];
        pos[2] += off[2];
        Mtx_Identity(m2);
        Mtx_RotateXYZ(m2, rx, ry, rz);
        Mtx_Concat3x3(m2, m);
        Mtx_SetTrans(m2, pos[0], pos[1], pos[2]);
        if (part != NULL && part->id != -1) {
            Gs_SetState(0x1A, (u32)m2);
            if (other) {
                Gs_SetLightList(set->mats, part);
            }
            Draw_ApplyStates(part->flags);
            Model_Draw(part->id, 0);
        }
        Draw_SetDefaultState();
    }
end:
    ;
}


/* Draws the player: the body's parts (faded while nearCamera), the effect Player_DrawWeapon, then the five
 * equipment models (partModels[1..5]), each placed by its bones and lit per part (Player_SetLight). */
void Player_Draw(Player *p, s32 unused /* a1: DrawCb_Player passes 0 */)
{
    s32 i2;
    ModelMatSet *parts;
    s32 i;
    Model *md;
    ModelMatSet *part;
    s32 n;
    f32 m[16];
    s32 sel;
    f32 id[16];
    s32 *e;
    s32 k;
    s32 j;
    ModelMat *lights2;
    EmBone *bones;
    Model *eq;
    ModelMat *light;
    ModelMat *lights;
    f32 alpha;
    s32 j2;
    u16 *tbl;

    md = p->model;
    LightSet_SetAmbient(p, 1);
    LightSet_FadeAmbient(p);
    Gs_SetState(0x67, -1);
    Gfx_SetTexFilter(1);
    if (p->nearCamera != 0 && Debug_IsOn(0x7B) == 0) {
        Gs_SetState(0x60, 0);
        alpha = 0.2f;
    } else {
        Gs_SetState(0x60, 0xC0);
        alpha = p->alpha;
    }
    Tex_Prepare(2, p->index * 2 + 0xD);
    part = md->matSets;
    Mtx_Copy(m, p->worldMtx[0]);
    Bone_StoreMatrices(md->bones);
    lights = p->model->mats;
    for (i = 0; i < md->matSetCount; i++, part++) {
        if (p->meshShown[i] != 1) {
            continue;
        }
        if (part->id != -1) {
            for (j = 0; j < part->count; j++) {
                light = &lights[part->mat[j]];
                light->alpha = alpha;
                Player_SetLight(p, 0, i, j, light, alpha);
            }
        }
        Draw_ApplyStates(part->flags);
        Model_Draw(part->id, 0);
    }
    Draw_SetDefaultState();
    Gfx_SetTexFilter(1);
    if (p->weaponShown != 0 && p->motionId[0] != 0x349) {
        Player_DrawWeapon(p, 1, alpha);
    }
    Tex_Prepare(0xC, p->index * 12 + 0x2B);
    for (k = 1, tbl = &gPlPartRootBone[1]; k < 6; k++, tbl++) {
        sel = p->armourKind[k];
        eq = p->partModels[k];
        if (eq == NULL) {
            continue;
        }
        parts = eq->matSets;
        n = eq->matSetCount;
        Bone_UpdateWorldScaledRoot(eq->bones, p->partRecs[*tbl]->m1);
        if (k == 3 || k == 5) {
            Mtx_Identity(id);
            Bone_SetMatricesRel(eq->bones, (struct BoneRoot *) p, k, sel - 1, id);
        } else if (k == 2) {
            bones = eq->bones;
            if (bones->count > 4) {
                Bone_UpdateWorldScaledRoot(&bones[4], p->partRecs[20]->m1);
            }
            Mtx_Identity(id);
            Bone_SetMatricesRel(eq->bones, (struct BoneRoot *) p, k, sel - 1, id);
        } else {
            Bone_SetMatrices(eq->bones, (struct BoneRoot *) p, k, sel - 1);
        }
        lights2 = p->partModels[k]->mats;
        for (i2 = 0; i2 < n; i2++, parts++) {
            if (parts->id != -1) {
                if (0 < parts->count) {
                    j2 = 0;
                    e = (s32 *)parts; /* the set as words: e[2] is mat[j2] (a walker from parts->mat is computed first) */
                    do {
                        Player_SetLight(p, k, i2, j2, &lights2[e[2]], alpha);
                        j2++;
                        e++;
                    } while (j2 < parts->count);
                }
            }
            Draw_ApplyStates(parts->flags);
            Model_Draw(parts->id, 0);
        }
        Draw_SetDefaultState();
    }
    Player_DrawHeldItem(p);
    if (p->nearCamera != 0) {
        Gs_SetState(0x6C, 1);
    }
    Gs_SetState(0x60, 0);
    LightSet_ResetAmbient(1);
    LightSet_Apply(1);
}

extern u8 *gAreaSubModel;
extern u8 gFaceBodyTypeMale[];
void LightSet_SetAmbient(void *obj, s32 n);
void LightSet_FadeAmbient(void *obj);
void Bone_StoreMatrices(struct EmBone *b);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void LightSet_ResetAmbient(s32 a);
void LightSet_Apply(s32 a);
void Gfx_SetTexFilter(s32 a);

/* Draws the hunter being made: its body part (by gFaceBodyTypeMale and unk47), face and hair (lit in the hair colour),
 * each part with its lights. */
void CharMake_DrawHunter(u8 *o, s32 unused /* a1: DrawCb_CharMake passes 0 */)
{
    u8 k;
    f32 m[16];
    u8 *md;
    ChrEdit *h;
    u8 b;
    f32 *light;
    s32 i;
    u8 *mdl;
    u8 *part;
    u8 *lights;

    md = *(u8 **)(o + 0x4EC);
    h = &gChrEdit;
    LightSet_SetAmbient(o, 1);
    LightSet_FadeAmbient(o);
    Gs_SetState(0x67, -1);
    Gs_SetState(0x60, 0xC0);
    Mtx_Copy(m, (f32 *)(o + 0x60));
    Bone_StoreMatrices(*(EmBone **)(md + 0x24));
    Gfx_SetTexFilter(1);
    k = 3 - gFaceBodyTypeMale[h->face];
    b = h->type * 4;
    k = k + b;
    mdl = gAreaSubModel;
    part = *(u8 **)(mdl + 0x30) + k * 0x8C;
    lights = *(u8 **)(mdl + 0x10);
    if (*(s32 *)part != -1) {
        for (i = 0; i < *(s32 *)(part + 4); i++) {
            Gs_SetState((u8)(i + 0x3A), (u32)(lights + ((s32 *)part)[i + 2] * 0x4C));
        }
    }
    Draw_ApplyStates(*(s32 *)(part + 0x88));
    Model_Draw(*(u32 *)part, 0);
    part = *(u8 **)(mdl + 0x30) + h->face * 0x8C + 0x8C0;
    if (*(s32 *)part != -1) {
        for (i = 0; i < *(s32 *)(part + 4); i++) {
            Gs_SetState((u8)(i + 0x3A), (u32)(lights + ((s32 *)part)[i + 2] * 0x4C));
        }
    }
    Draw_ApplyStates(*(s32 *)(part + 0x88));
    Model_Draw(*(u32 *)part, 0);
    part = *(u8 **)(mdl + 0x30) + h->hair * 0x8C + 0x1784;
    if (*(s32 *)part != -1) {
        for (i = 0; i < *(s32 *)(part + 4); i++) {
            light = (f32 *)(lights + ((s32 *)part)[i + 2] * 0x4C);
            if (i == 0) {
                light[1] = ((h->color >> 16) & 0xFF) / 255.0f;
                light[2] = ((h->color >> 8) & 0xFF) / 255.0f;
                light[3] = (u8)h->color / 255.0f;
                Gs_SetState((u8)(i + 0x3A), (u32)light);
            }
            Gs_SetState((u8)(i + 0x3A), (u32)light);
        }
    }
    Draw_ApplyStates(*(s32 *)(part + 0x88));
    Model_Draw(*(u32 *)part, 0);
    Draw_SetDefaultState();
    LightSet_ResetAmbient(1);
    LightSet_Apply(1);
}

/* A light as Gs_SetState's states 0x5A-0x5C take it (0x68 bytes). */
typedef struct GsLight {
    u32 unk00;
    f32 color[3];               /* 0x04 */
    u32 unk10;
    f32 scale[3];               /* 0x14 */
    u32 unk20;
    f32 ambient[3];             /* 0x24 */
    u32 unk30;
    f32 dir[3];                 /* 0x34 */
    u8 unk40[0x68 - 0x40];
} GsLight;

void *memset(void *p, s32 c, u32 n);
extern f32 gLightSetDirs[][3];
extern f32 gLightSetColors[][3];
extern f32 gLightSetAmbients[][3];
extern f32 gCamMatrix[];

/* Sets up the three lights of set k: directions gLightSetDirs turned by the camera gCamMatrix, colours
 * gLightSetColors and gLightSetAmbients. */
void LightSet_SetupSet(s32 k)
{
    GsLight l[3];
    f32 v[3];
    f32 r[3];
    s16 i;
    s32 j;
    GsLight *lp;

    for (i = 0, lp = l; i < 3; i++, lp++) {
        j = k * 3 + i;
        memset(lp, 0, sizeof(GsLight));
        v[0] = gLightSetDirs[j][0];
        v[1] = gLightSetDirs[j][1];
        v[2] = gLightSetDirs[j][2];
        Vec_Rotate(r, v, gCamMatrix);
        lp->color[0] = gLightSetColors[j][0];
        lp->color[1] = gLightSetColors[j][1];
        lp->color[2] = gLightSetColors[j][2];
        lp->ambient[0] = gLightSetAmbients[j][0];
        lp->ambient[1] = gLightSetAmbients[j][1];
        lp->ambient[2] = gLightSetAmbients[j][2];
        lp->scale[0] = 1.0f;
        lp->scale[1] = 1.0f;
        lp->scale[2] = 1.0f;
        lp->dir[0] = r[0];
        lp->dir[1] = r[1];
        lp->dir[2] = r[2];
    }
    Gs_SetState(0x5A, (u32)&l[0]);
    Gs_SetState(0x5B, (u32)&l[1]);
    Gs_SetState(0x5C, (u32)&l[2]);
}

void EquipPreview_Reset(void)
{
    D_005BAD84 = 0;
    D_005BAD78 = 0;
    D_005BAD7C = 0;
    D_005BAD80 = 0;
}


/* A viewport as Gs_SetState's state 0x18 takes it (Camera.viewport and unk58). */
typedef struct GsViewport {
    s32 x, y, w, h;
    s32 unk10;
    f32 scale;
} GsViewport;

/* The state at 0x5BAD60 (0xB0 bytes) as EquipPreview_SetViewport sees it. */
typedef struct PreviewState {
    u8 active;                  /* 0x00: nonzero while the preview is shown */
    u8 unk01[0x07 - 0x01];
    u8 settled;                 /* 0x07: mode 2 has run its first settling passes */
    u8 unk08[0x0C - 0x08];
    Model *model; /* 0x0C */
    u8 unk10;                   /* 0x10 */
    u8 unk11[0x18 - 0x11];
    f32 panX;                   /* 0x18: -60..60 */
    f32 panY;                   /* 0x1C: -30..30 */
    u8 unk20[0x24 - 0x20];
    f32 zoom;                   /* 0x24: percent added to the preview's field of view */
    EquipFxSwing equipFx[4];    /* 0x28: the swinging parts of the model (Effect_RunList; StageModels_LoadEquip clears them for kind 2) */
    u16 mode;                   /* 0xA8: 0xFF: the fixed view */
    u16 id;                     /* 0xAA */
    u32 colour;                 /* 0xAC: 0xRRGGBB for mode 0xFF */
} PreviewState;

extern PreviewState gEquipPreview;
void Camera_SetPerspective(s32 i, f32 n, f32 f, f32 fov, f32 aspect);

/* The lobby's menu offsets (lobby .bss), which place the preview window. */
extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;

/* Sets the viewport and projection: the camera's own (mode 0), or a small preview window (a fixed one, or
 * one placed by the lobby's menu offsets and zoomed by st->zoom). */
void EquipPreview_SetViewport(s16 mode)
{
    GsViewport vp;
    PreviewState *st = &gEquipPreview;

    if (mode == 0) {
        Gs_SetState(0x18, (u32)gCamera->viewport);
        Camera_SetPerspective(0x20, gCamera->unk34, gCamera->unk30, gCamera->fov, 1.4285715f);
        return;
    }
    vp.unk10 = 0;
    vp.scale = 1.0f;
    if (st->mode == 0xFF) {
        gCamera->fov = 0.08726647f;
        vp.x = 0x108;
        vp.y = 0x3A;
        vp.w = 0xE8;
        vp.h = 0x118;
        Gs_SetState(0x18, (u32)&vp);
        Camera_SetPerspective(0x20, gCamera->unk34, gCamera->unk30, gCamera->fov, 1.0f);
    } else {
        gCamera->fov = 6.2831855f * (5.0f * (1.0f + st->zoom / 100.0f)) / 360.0f;
        vp.x = 0.8f * (f32)(gMenuOffsetX + 0x136);
        vp.y = gMenuOffsetY + 0x36;
        vp.w = 0xF4;
        vp.h = 0x96;
        Gs_SetState(0x18, (u32)&vp);
        Camera_SetPerspective(0x20, gCamera->unk34, gCamera->unk30, gCamera->fov, 2.0f);
    }
}

typedef struct PvVec {
    f32 x, y, z;
} PvVec;

s32 Kbd_IsOpen(void); /* unit's own declaration: the definition returns s8; EquipPreview_Draw
 * (NON_MATCHING) is 8 words off with s32, 1122 with s8 */
s32 Gs_ClearColor(u8 mode, u32 color);
void Gfx_SetBlend(s32 a, s32 b);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
void Mtx_LookAt(f32 *m, f32 *eye, f32 *at, f32 *up);
struct MtxSlot;
void MtxSlot_Load(struct MtxSlot *m, s32 i);
void Mtx_Inverse(f32 *dst, f32 *src);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Effect_RunList(EquipFxSwing *a, struct EmBone *b3, struct EmBone *b1, struct EmBone *b0, u16 key, u8 alt, u8 kind);
void Camera_RestoreFov(void);
/* The pad words at 0x6AE0B4 (the pad state's members). */
typedef struct PvPad {
    u16 trig;                   /* 0x6AE0B4: pressed this frame */
    u16 unk2;
    u16 held;                   /* 0x6AE0B8 */
} PvPad;
extern PvPad D_006AE0B4;
extern s32 D_006AE96C;
extern u16 gFrameTick;
extern f32 gViewMatrix[];
extern u8 gWeaponKindClass[];
extern PvVec gEquipPreviewEyes[];
extern f32 gEquipPreviewWpEyeZ[];
extern f32 gEquipPreviewWpPos[][3];
extern f32 D_0043E844[][3];
extern f32 D_0043E8D0[][3];
extern f32 D_0043E954[][3];
extern f32 D_0043E9EC[][3];
extern f32 D_0043EA4C[][3];
extern f32 D_0043EACC[][3];

#pragma optimization_level 4
/* k is u32: an s32 k makes the (k - 3) range check of the parts
 * loop a cast that MWCC hoists out of the loop (into a saved register); the game
 * computes k - 3 on each pass. */
/* Draws the equipment preview window: pans and zooms it by the pad, looks at the model from the mode's
 * eye point, poses its bones by mode (and kind, for modes 6 and 7), draws its parts with their lights
 * (the player's colour, the preview's colour, or parts hidden by kind), then restores the camera. */
void EquipPreview_Draw(void)
{
    PreviewState *st;
    u8 kind;
    Model *md;
    EmBone *b;
    ModelMatSet *part;
    ModelMat *lights;
    ModelMat *light;
    s32 i;
    s32 j;
    Player *pl;
    u32 k;
    f32 rx;
    f32 ry;
    f32 rz;
    f32 *t;
    EmBone *bp;

    f32 eye[3];
    f32 at[3];
    f32 up[3];
    f32 pos[3];
    f32 m[16];
    f32 inv[16];

    pl = &gPlayers[gScene.localPlayer];
    st = &gEquipPreview;
    if (st->active == 0) {
        return;
    }
    Gs_SetState(0x67, -1);
    if (Kbd_IsOpen() == 0 && st->mode != 0xFF) {
        if (D_006AE0B4.held & PAD_L2) {
            if ((st->zoom -= 2.0f) <= -30.0f) {
                st->zoom = -30.0f;
            }
        } else if (D_006AE0B4.held & PAD_L1) {
            if ((st->zoom += 2.0f) >= 30.0f) {
                st->zoom = 30.0f;
            }
        }
        if (D_006AE0B4.held & PAD_RIGHT) {
            if ((st->panX -= 4.0f) <= -60.0f) {
                st->panX = -60.0f;
            }
        } else if (D_006AE0B4.held & PAD_LEFT) {
            if ((st->panX += 4.0f) >= 60.0f) {
                st->panX = 60.0f;
            }
        }
        if (D_006AE0B4.held & PAD_UP) {
            if ((st->panY -= 2.0f) <= -30.0f) {
                st->panY = -30.0f;
            }
        } else if (D_006AE0B4.held & PAD_DOWN) {
            if ((st->panY += 2.0f) >= 30.0f) {
                st->panY = 30.0f;
            }
        }
        if (D_006AE0B4.trig & 2) {
            EquipPreview_Reset();
        }
    }
    md = st->model;
    EquipPreview_SetViewport(1);
    Gs_SetState(0x6C, 1);
    Gs_ClearColor(0, 0);
    Gs_SetState(0x67, -1);
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x6D, 3);
    Gs_SetState(0x5F, 4);
    Vec_Set(up, 0.0f, 1.0f, 0.0f);
    switch (st->mode) {
    case 0xFF:
        *(PvVec *)eye = gEquipPreviewEyes[1];
        break;
    case 6:
    case 7:
        kind = gWeaponKindClass[st->id];
        eye[0] = 0.0f;
        eye[1] = 0.0f;
        eye[2] = gEquipPreviewWpEyeZ[kind];
        at[0] = 0.0f;
        at[1] = 0.0f;
        break;
    default:
        *(PvVec *)eye = gEquipPreviewEyes[st->mode];
        break;
    }
    if (st->mode != 0xFF) {
        eye[0] += st->panX;
        eye[1] += st->panY;
        eye[2] *= 6.0f;
    }
    at[0] = eye[0];
    at[1] = eye[1];
    at[2] = 0.0f;
    Mtx_LookAt(m, eye, at, up);
    Gs_SetState(0x16, (u32)m);
    MtxSlot_Load((struct MtxSlot *)inv, 0x21);
    Mtx_Inverse(gCamMatrix, inv);
    if (st->mode == 0xFF) {
        LightSet_SetupSet(1);
    } else {
        LightSet_SetupSet(0);
    }
    Mtx_Identity(m);
    if (st->mode != 6 && st->mode != 7) {
        if (st->mode == 0xFF) {
            Mtx_RotateXYZ(m, 0.0f, 6.2831855f * (f32)((u8)gFrameTick << 8) / 65536.0f, 0.0f);
        } else if (st->mode == 4) {
            Mtx_RotateXYZ(m, -0.7853982f, -0.5235988f, 0.0f);
        } else {
            Mtx_RotateXYZ(m, 0.0f, -0.5235988f, 0.0f);
        }
    } else {
        Mtx_RotateXYZ(m, 0.0f, 0.0f, 0.0f);
    }
    b = md->partBones[0];
    Mtx_SetTrans(b[0].local, 0.0f, 0.0f, 0.0f);
    switch (st->mode) {
    case 0xFF:
    case 0:
        break;
    case 5:
        t = D_0043E9EC[0];
        for (i = 1; i < 7; i++) {
            Mtx_SetTrans(b[i].local, t[0], t[1], t[2]);
            t += 3;
        }
        break;
    case 4:
        Mtx_SetRotXYZ(b[1].local, 0.0f, 0.0f, 0.0f);
        Mtx_SetRotXYZ(b[2].local, 0.0f, 0.0f, -0.34906587f);
        Mtx_SetRotXYZ(b[3].local, 1.5707964f, 0.0f, -1.2217306f);
        Mtx_SetRotXYZ(b[6].local, 0.0f, 0.0f, 0.34906587f);
        Mtx_SetRotXYZ(b[7].local, 1.5707964f, 0.0f, 1.2217306f);
        for (i = 1, t = D_0043EA4C[0]; i < 10; i++) {
            Mtx_SetTrans(b[i].local, t[0], t[1], t[2]);
            t += 3;
        }
        break;
    case 3:
        Mtx_SetRotXYZ(b[4].local, 0.0f, 0.0f, -0.34906587f);
        Mtx_SetRotXYZ(b[5].local, 0.0f, 0.0f, -1.2217306f);
        Mtx_SetRotXYZ(b[6].local, 0.0f, 0.0f, 0.34906587f);
        Mtx_SetRotXYZ(b[7].local, 0.0f, 0.0f, 1.2217306f);
        for (i = 1, t = D_0043EACC[0]; i < 10; i++) {
            Mtx_SetTrans(b[i].local, t[0], t[1], t[2]);
            t += 3;
        }
        break;
    case 2:
        Mtx_SetTrans(b[1].local, 0.0f, 0.0f, 0.0f);
        Mtx_SetTrans(b[2].local, 0.0f, 22.699997f, -5.499998f);
        Mtx_SetTrans(b[3].local, 0.0f, 12.300014f, 3.499999f);
        Bone_UpdateWorldScaledRoot(md->partBones[0], m);
        if (st->settled == 0) {
            st->settled = 1;
            for (i = 0; i < 10; i++) {
                Effect_RunList(st->equipFx, &b[3], &b[1], b, st->id, 1, st->unk10);
                Bone_UpdateWorldScaledRoot(md->partBones[0], m);
            }
        } else {
            Effect_RunList(st->equipFx, &b[3], &b[1], b, st->id, 1, st->unk10);
            Bone_UpdateWorldScaledRoot(md->partBones[0], m);
        }
        break;
    case 6:
    case 7:
        rx = D_0043E8D0[kind][0];
        ry = D_0043E8D0[kind][1];
        rz = D_0043E8D0[kind][2];
        eye[0] = gEquipPreviewWpPos[kind][0];
        eye[1] = gEquipPreviewWpPos[kind][1];
        eye[2] = gEquipPreviewWpPos[kind][2];
        Mtx_Identity(m);
        Mtx_RotateXYZ(m, 6.2831855f * rx / 360.0f, 6.2831855f * ry / 360.0f, 6.2831855f * rz / 360.0f);
        Mtx_SetTrans(m, eye[0], eye[1], eye[2]);
        switch (kind) {
        case 0:
            Mtx_GetTrans(pos, b[3].local);
            Mtx_Scale(b[3].local, 2.6f, 1.0f, 2.5f);
            Mtx_SetTrans(b[3].local, pos[0], pos[1], pos[2]);
            break;
        case 1:
            Mtx_SetRotXYZ(b[2].local, 3.1415927f, 0.0f, 0.0f);
            break;
        case 10:
            Mtx_SetRotXYZ(b[2].local, 2.7052603f, 0.0f, 0.0f);
            break;
        }
        break;
    }
    part = md->matSets;
    Bone_UpdateWorldScaledRoot(md->partBones[0], m);
    Bone_StoreMatrices(md->partBones[0]);
    lights = md->mats;
    for (i = 0; i < md->matSetCount; i++, part++) {
        if (st->mode == 6) {
            switch (kind) {
            case 4:
            case 3:
            case 6:
                if (i == 1) {
                    continue;
                }
            }
        }
        if (part == NULL || part->id == -1) {
            continue;
        }
        for (j = 0; j < part->count; j++) {
            light = &lights[part->mat[j]];
            light->alpha = 1.0f;
            switch (st->mode) {
            default:
                if (D_006AE96C != 0 && D_006AE96C == light->tex) {
                    light->alpha = 0.0f;
                }
                break;
            case 2:
                if (D_006AE96C != 0 && D_006AE96C == light->tex) {
                    light->alpha = 0.0f;
                }
                if (i == 0 && j == 0) {
                    light->rgb[0] = (pl->hairColor >> 16 & 0xFF) / 255.0f;
                    light->rgb[1] = (pl->hairColor >> 8 & 0xFF) / 255.0f;
                    light->rgb[2] = (pl->hairColor & 0xFF) / 255.0f;
                }
                break;
            case 0xFF:
                if (i == 0) {
                    switch (j) {
                    case 1:
                    case 3:
                    case 4:
                        light->rgb[0] = (st->colour >> 16 & 0xFF) / 255.0f;
                        light->rgb[1] = (st->colour >> 8 & 0xFF) / 255.0f;
                        light->rgb[2] = (st->colour & 0xFF) / 255.0f;
                        break;
                    }
                }
                break;
            case 6:
            case 7:
                switch (kind) {
                case 7:
                    if (j >= 3) {
                        light->alpha = 0.0f;
                    }
                    break;
                case 10:
                    if (j == 0) {
                        light->alpha = 0.0f;
                    }
                    break;
                case 9:
                    if (j == 0) {
                        light->alpha = 0.0f;
                    }
                    break;
                case 1:
                case 5:
                    if (j == 1 || j == 2) {
                        light->alpha = 0.0f;
                    }
                    break;
                }
                break;
            }
            Gs_SetState((u8)(j + 0x3A), (u32)light);
        }
        Draw_ApplyStates(part->flags);
        Model_Draw(part->id, 0);
    }
    switch (st->mode) {
    case 6:
    case 7:
        k = kind;
        rx = D_0043E954[k][0];
        ry = D_0043E954[k][1];
        rz = D_0043E954[k][2];
        eye[0] = D_0043E844[k][0] + gEquipPreviewWpPos[k][0];
        eye[1] = D_0043E844[k][1] + gEquipPreviewWpPos[k][1];
        eye[2] = D_0043E844[k][2] + gEquipPreviewWpPos[k][2];
        Mtx_Identity(m);
        Mtx_RotateXYZ(m, 6.2831855f * rx / 360.0f, 6.2831855f * ry / 360.0f, 6.2831855f * rz / 360.0f);
        Mtx_SetTrans(m, eye[0], eye[1], eye[2]);
        switch (k) {
        case 3:
        case 4:
        case 6:
            Bone_UpdateWorldScaledRoot(md->partBones[1], m);
            Bone_StoreMatrices(md->partBones[1]);
            break;
        case 7:
        case 9:
        case 10:
            Bone_UpdateWorldScaledRoot(md->partBones[0], m);
            Bone_StoreMatrices(md->partBones[0]);
            break;
        default:
            goto done;
        }
        part = md->matSets;
        lights = md->mats;
        for (i = 0; i < md->matSetCount; i++) {
            if ((k == 3 || k == 4 || k == 6) && i == 0) {
                part++;
                continue;
            }
            if (part != NULL) {
                if (part->id != -1) {
                    for (j = 0; j < part->count; j++) {
                        light = &lights[part->mat[j]];
                        light->alpha = 1.0f;
                        switch (k) {
                        case 7:
                            if (j < 3) {
                                light->alpha = 0.0f;
                            }
                            break;
                        case 10:
                            if (j != 0) {
                                light->alpha = 0.0f;
                            }
                            break;
                        case 9:
                            if (j != 0) {
                                light->alpha = 0.0f;
                            }
                            break;
                        }
                        Gs_SetState((u8)(j + 0x3A), (u32)light);
                    }
                }
                Draw_ApplyStates(part->flags);
                Model_Draw(part->id, 0);
            }
            part++;
        }
        break;
    }
done:
    EquipPreview_SetViewport(0);
    Mtx_LookAt(m, gCamera->eye, gCamera->at, gCamera->up);
    Gs_SetState(0x16, (u32)m);
    MtxSlot_Load((struct MtxSlot *)gViewMatrix, 0x21);
    Mtx_Inverse(gCamMatrix, gViewMatrix);
    Camera_RestoreFov();
    LightSet_ResetAmbient(1);
}
#pragma optimization_level reset

void LightSet_SetAmbient(void *obj, s32 n);
void LightSet_FadeAmbient(void *obj);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Bone_StoreMatrices(struct EmBone *b);
struct MtxSlot;
void Bone_StoreMatricesTo(struct MtxSlot *tbl, struct EmBone *b);
s32 EmBlangonga_RageTimerFull(Em *em);
s32 GmEm_ApplyFx(Em *em, s32 i);
struct EmMelynxMeshSet;    /* game em_cephadrome.c */
struct EmKutkuMeshSet;     /* game em_kutku.c */
struct GmEmPartTbl;        /* gm_sub gm_em.c */
void EmMelynx_SetMeshSetAlpha(Em *em, s32 kind, struct EmMelynxMeshSet *sets);
void EmKutku_SetMeshSetAlpha(Em *em, s32 kind, struct EmKutkuMeshSet *sets);
void GmEm_SetMeshAlphas(Em *em, s32 mode, struct GmEmPartTbl *tbl);
void Draw_ApplyStates(u32 a);
void MtxSlot_LoadList(s16 *list, struct MtxSlot *tbl);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void LightSet_ResetAmbient(s32 a);
void LightSet_Apply(s32 a);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetTexFilter(s32 a);
void Mtx_Identity(f32 *m);
extern s16 *gObjPartSkipLists[];
extern u8 gBoneMatrixBuf[];
extern u8 gModelPartLists[][0x420];

/* Draws the object of draw slot a (+0x18): its matrix from scale, angles and position, its bones, then each
 * part in use (+0x4C8) with the kind's lighting (GmEm_SetMeshAlphas or the kind's own), skipping the parts
 * listed in gObjPartSkipLists[kind] from the fog state. */
void Obj_DrawParts(DrawSlot *a)
{
    Em *o;
    u8 *part;
    f32 rm[16];
    Model *md;
    s32 i;
    u8 *parts;
    s32 ang[3];
    s16 *skip;
    s32 n;
    s32 big;
    f32 im[16];
    EmBone *bones;
    s16 next;
    f32 sm[16];

    o = a->owner;
    md = o->model;
    if (o->active == 0 || o->visible == 0) {
        return;
    }
    LightSet_SetAmbient(o, 1);
    LightSet_FadeAmbient(o);
    Vec_AnglesToRad(&o->rotX, (f32 *)ang);
    Mtx_Scale(sm, o->scaleX, o->scaleY, o->scaleZ);
    Mtx_FromAnglesYXZ(&o->rotX, rm);
    Mtx_SetTrans(rm, o->posX, o->posY, o->posZ);
    Mtx_Concat3x3(rm, sm);
    Mtx_Copy(&o->worldMtx[0][0], rm);
    Gfx_SetTexFilter(1);
    next = *(skip = gObjPartSkipLists[o->type]);
    if (o->type == 3 || o->type == 0x45) {
        n = 1;
        parts = (u8 *)md->matSets + o->gender * 0x8C;
    } else {
        parts = (u8 *)md->matSets;
        n = md->matSetCount;
    }
    bones = md->bones;
    big = bones->count >= 0x21;
    if (big == 0) {
        Bone_StoreMatrices(bones);
    } else {
        Bone_StoreMatricesTo((struct MtxSlot *) gBoneMatrixBuf, bones);
    }
    Mtx_Identity(im);
    Gs_SetState(0x19, (u32)im);
    Tex_Prepare(0xA, o->modelSlot * 20 + 0xA3);
    for (i = 0; i < n; i++) {
        part = parts + i * 0x8C;
        if ((s8)o->meshShown[i] == 0) {
            continue;
        }
        while (next != -1) {
            if (!(i > next)) {
                break;
            }
            next = *++skip;
        }
        if (next == i || o->alpha != 1.0f || (o->type == 0x35 && i == 5 && EmBlangonga_RageTimerFull(o) == 0)) {
            Gs_SetState(0x60, 0);
            next = *++skip;
        } else {
            Gs_SetState(0x60, 0xC0);
        }
        if (GmEm_ApplyFx(o, i) == 0) {
            continue;
        }
        Gs_SetState(0x67, -1);
        if (*(s32 *)part == -1) {
            continue;
        }
        switch (o->type) {
        case 9:
        case 0x17:
        case 0x12:
            EmMelynx_SetMeshSetAlpha(o, i, (struct EmMelynxMeshSet *)parts);
            break;
        case 0x14:
        case 0x27:
            EmKutku_SetMeshSetAlpha(o, i, (struct EmKutkuMeshSet *)parts);
            break;
        default:
            GmEm_SetMeshAlphas(o, i, (struct GmEmPartTbl *)parts);
            break;
        }
        Draw_ApplyStates(*(s32 *)(part + 0x88));
        if (big == 1) {
            MtxSlot_LoadList((s16 *) (gModelPartLists[o->modelSlot] + i * 0x42), (struct MtxSlot *) gBoneMatrixBuf);
        }
        Model_Draw(*(u32 *)part, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(0x60, 0);
    LightSet_ResetAmbient(1);
    LightSet_Apply(1);
}

/* The player object as Player_ScaleBones sees it: its 21 parts. */
typedef struct PlParts {
    u8 unk00[0xD0];
    u8 *parts[21];              /* 0xD0: +0x40 a matrix, +0x98 a scale */
} PlParts;

void Player_TwistBoneToLook(Player *p, s16 kind);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
void Bone_StoreMatrices(struct EmBone *b);

/* Scales each of the 21 bones by its part's scale, updates them under m and copies the bones' matrices back
 * into their parts. */
void Player_ScaleBones(u8 *o, EmBone *b, f32 *m)
{
    f32 sm[16];
    f32 tm[16];
    s32 i;
    EmBone *bone;

    for (i = 0, bone = b; i < 21; i++, bone++) {
        Player_TwistBoneToLook((Player *) o, bone->index);
        Mtx_Scale(sm, *(f32 *)(((PlParts *)o)->parts[i] + 0x98), *(f32 *)(((PlParts *)o)->parts[i] + 0x9C), *(f32 *)(((PlParts *)o)->parts[i] + 0xA0));
        Mtx_Mul(tm, bone->local, sm);
        bone->local[12] = tm[12];
        bone->local[13] = tm[13];
        bone->local[14] = tm[14];
    }
    Bone_UpdateWorldScaledRoot(b, m);
    Bone_StoreMatrices(b);
    for (i = 0; i < 21; i++, b++) {
        Mtx_Copy((f32 *)(((PlParts *)o)->parts[b->index] + 0x40), b->world);
    }
}

/* When o[1] is set: o's matrix from its scale (+0xB8), angles (+0xA0)
 * and position (+0xAC) into +0x60 and its model's bones (via +0x4EC). */
void Obj_UpdateMatrix(u8 *o)
{
    f32 m[16];
    f32 r[3];
    u8 *x;

    if (o[1] == 0) {
        return;
    }
    x = *(u8 **)(o + 0x4EC);
    Vec_AnglesToRad((s32 *)(o + 0xA0), r);
    Mtx_Scale(m, *(f32 *)(o + 0xB8), *(f32 *)(o + 0xBC), *(f32 *)(o + 0xC0));
    Mtx_RotateYXZ(m, r[0], r[1], r[2]);
    Mtx_SetTrans(m, *(f32 *)(o + 0xAC), *(f32 *)(o + 0xB0), *(f32 *)(o + 0xB4));
    Mtx_Copy((f32 *)(o + 0x60), m);
    Bone_UpdateWorld(*(void **)(x + 0x44), m);
    Player_ScaleBones(o, *(void **)(x + 0x44), m);
}

/* unk_36E1D0's .data (0x43CA30-0x43ECF0) */
WpPose gWpPoseDrawnAlt[11] = {
#include "sub_main/0043CA30.inc"
};
WpPose gWpPoseDrawn[11] = {
#include "sub_main/0043CB40.inc"
};
WpPose gWpPoseSheathed[11] = {
#include "sub_main/0043CC50.inc"
};
WpPose gWpPoseMotion19D[11] = {
#include "sub_main/0043CD60.inc"
};
Key1 gLbgKey579ScaleZ[8] = {
#include "sub_main/0043CE70.inc"
};
Key1 gHbgKey579ScaleZ[8] = {
#include "sub_main/0043CEB0.inc"
};
Key1 gLbgKey579ScaleY[4] = {
#include "sub_main/0043CEF0.inc"
};
Key1 gHbgKey579ScaleX[4] = {
#include "sub_main/0043CF10.inc"
};
Key1 gLbgKey57CScaleZ[8] = {
#include "sub_main/0043CF30.inc"
};
Key1 gHbgKey57CScaleZ[8] = {
#include "sub_main/0043CF70.inc"
};
Key1 gLbgKey57CScaleY[4] = {
#include "sub_main/0043CFB0.inc"
};
Key1 gHbgKey57CScaleX[4] = {
#include "sub_main/0043CFD0.inc"
};
Key1 gLbgKey57DEmptyScaleZ[8] = {
#include "sub_main/0043CFF0.inc"
};
Key1 gHbgKey57DEmptyScaleZ[8] = {
#include "sub_main/0043D030.inc"
};
Key1 gLbgKey57DEmptyScaleY[4] = {
#include "sub_main/0043D070.inc"
};
Key1 gHbgKey57DEmptyScaleX[4] = {
#include "sub_main/0043D090.inc"
};
Key1 gLbgKey57DScaleZ[8] = {
#include "sub_main/0043D0B0.inc"
};
Key1 gHbgKey57DScaleZ[8] = {
#include "sub_main/0043D0F0.inc"
};
Key1 gLbgKey57DScaleY[6] = {
#include "sub_main/0043D130.inc"
};
Key1 gHbgKey57DScaleX[6] = {
#include "sub_main/0043D160.inc"
};
Key1 gBowgunKey580ScaleZ[8] = {
#include "sub_main/0043D190.inc"
};
Key1 gBowgunKey580Scale2[4] = {
#include "sub_main/0043D1D0.inc"
};
Key1 gHbgKey3EARotX[6] = {
#include "sub_main/0043D1F0.inc"
};
Key1 gHbgKey3EAScaleZ[8] = {
#include "sub_main/0043D220.inc"
};
Key1 gHbgKey3EAScaleX[4] = {
#include "sub_main/0043D260.inc"
};
Key1 gHbgKey3EBRotX[4] = {
#include "sub_main/0043D280.inc"
};
Key1 gHbgKey3F1ScaleZ[6] = {
#include "sub_main/0043D2A0.inc"
};
Key1 gHbgKey3EBScaleX[6] = {
#include "sub_main/0043D2D0.inc"
};
Key3 gLbgKey3EARot[9] = {
#include "sub_main/0043D300.inc"
};
Key3 gLbgKey3EAOff[9] = {
#include "sub_main/0043D390.inc"
};
Key3 gLbgKey3EBRot[12] = {
#include "sub_main/0043D420.inc"
};
Key3 gLbgKey3EBOff[12] = {
#include "sub_main/0043D4E0.inc"
};
Key3 gLbgKey3F1Rot[10] = {
#include "sub_main/0043D5A0.inc"
};
Key3 gLbgKey3F1Off[10] = {
#include "sub_main/0043D640.inc"
};
Key3 gLbgKey3F2Rot[10] = {
#include "sub_main/0043D6E0.inc"
};
Key3 gLbgKey3F2Off[10] = {
#include "sub_main/0043D780.inc"
};
Key1 gLanceKey19DScaleZ[6] = {
#include "sub_main/0043D820.inc"
};
Key1 gLanceKey3EAScaleZ[6] = {
#include "sub_main/0043D850.inc"
};
Key1 gLanceKey3EBScaleZ[6] = {
#include "sub_main/0043D880.inc"
};
Key1 gLanceKey3F1ScaleZ[6] = {
#include "sub_main/0043D8B0.inc"
};
Key1 gLanceKey3F2ScaleZ[6] = {
#include "sub_main/0043D8E0.inc"
};
Key1 gLanceKey3FAScaleZ[6] = {
#include "sub_main/0043D910.inc"
};
Key1 gHammerKey57EScale[6] = {
#include "sub_main/0043D940.inc"
};
Key1 gHammerKey57CScale[6] = {
#include "sub_main/0043D970.inc"
};
Key1 gGsKey19DScaleZ[6] = {
#include "sub_main/0043D9A0.inc"
};
Key1 gGsKey3EAScaleZ[8] = {
#include "sub_main/0043D9D0.inc"
};
Key1 gGsKey3EBScaleZ[8] = {
#include "sub_main/0043DA10.inc"
};
Key1 gGsKey3F9ScaleZ[8] = {
#include "sub_main/0043DA50.inc"
};
Key1 gGsKey582ScaleZ[6] = {
#include "sub_main/0043DA90.inc"
};
Key2 gGsKey19DScaleXZ[10] = {
#include "sub_main/0043DAC0.inc"
};
Key2 gGsKey3EAScaleXZ[6] = {
#include "sub_main/0043DB40.inc"
};
Key2 gGsKey3EBScaleXZ[6] = {
#include "sub_main/0043DB90.inc"
};
Key2 gGsKey3F9ScaleXZ[6] = {
#include "sub_main/0043DBE0.inc"
};
Key2 gGsKey582ScaleXZ[6] = {
#include "sub_main/0043DC30.inc"
};
Key2 gHornKey57DRot[18] = {
#include "sub_main/0043DC80.inc"
};
Key3 gHornKey57DScale[24] = {
#include "sub_main/0043DD60.inc"
};
Key1 gGlKey19DRotX[10] = {
#include "sub_main/0043DEE0.inc"
};
Key1 gGlKey3EARotX[6] = {
#include "sub_main/0043DF30.inc"
};
Key1 gGlKey3EBRotX[8] = {
#include "sub_main/0043DF60.inc"
};
Key1 gGlKey3F2RotX[4] = {
#include "sub_main/0043DFA0.inc"
};
Key1 gGlKey57CRotX[16] = {
#include "sub_main/0043DFC0.inc"
};
Key3 gGlKey580Rot[6] = {
#include "sub_main/0043E040.inc"
};
Key3 gGlKeyTimerRot[7] = {
#include "sub_main/0043E0A0.inc"
};
Key1 gBowKey3EARotX[8] = {
#include "sub_main/0043E110.inc"
};
Key1 gBowKey3EBRotX[12] = {
#include "sub_main/0043E150.inc"
};
Key1 gBowKey3EDRotX[12] = {
#include "sub_main/0043E1B0.inc"
};
Key1 gBowKey3F4RotX[8] = {
#include "sub_main/0043E210.inc"
};
Key1 gBowKey3F6RotX[8] = {
#include "sub_main/0043E250.inc"
};
Key1 gBowKey57DRotX[6] = {
#include "sub_main/0043E290.inc"
};
Key1 gBowKey580RotX[8] = {
#include "sub_main/0043E2C0.inc"
};
Key1 gBowKey587RotX[8] = {
#include "sub_main/0043E300.inc"
};
Key1 gBowKey58ARotX[4] = {
#include "sub_main/0043E340.inc"
};
Key1 gBowKey3EAScaleZ[6] = {
#include "sub_main/0043E360.inc"
};
Key1 gBowKey3EBScaleZ[6] = {
#include "sub_main/0043E390.inc"
};
Key1 gBowKey3EDScaleZ[6] = {
#include "sub_main/0043E3C0.inc"
};
Key1 gBowKey3F4ScaleZ[6] = {
#include "sub_main/0043E3F0.inc"
};
Key1 gBowKey3F6ScaleZ[6] = {
#include "sub_main/0043E420.inc"
};
Key1 gBowKey57DScaleZ[8] = {
#include "sub_main/0043E450.inc"
};
Key1 gBowKey580ScaleZ[6] = {
#include "sub_main/0043E490.inc"
};
Key1 gBowKey587ScaleZ[6] = {
#include "sub_main/0043E4C0.inc"
};
Key1 gBowKey58AScaleZ[5] = {
#include "sub_main/0043E4F0.inc"
};
u32 gBoneMtxSlotMaps_0[2] = {
#include "sub_main/0043E518.inc"
};
u32 gBoneMtxSlotMaps_1[4] = {
#include "sub_main/0043E520.inc"
};
u32 gBoneMtxSlotMaps_2[8] = {
#include "sub_main/0043E530.inc"
};
u32 gBoneMtxSlotMaps_3[8] = {
#include "sub_main/0043E550.inc"
};
u32 gBoneMtxSlotMaps_4[8] = {
#include "sub_main/0043E570.inc"
};
u32 gBoneMtxSlotMaps_5[8] = {
#include "sub_main/0043E590.inc"
};
u32 gBoneMtxSlotMaps[8] = {
    (u32)gBoneMtxSlotMaps_0, (u32)gBoneMtxSlotMaps_1, (u32)gBoneMtxSlotMaps_2, (u32)gBoneMtxSlotMaps_3,
    (u32)gBoneMtxSlotMaps_4, (u32)gBoneMtxSlotMaps_5, 0, 0,
};
f32 gBoneMarkerDirs[6][3] = {
#include "sub_main/0043E5D0.inc"
};
u32 gCycleColors[8] = {
#include "sub_main/0043E620.inc"
};
/* The bone each armour part (1-5) hangs from (Player_DrawLocal). */
u16 gPlPartRootBone[8] = {
#include "sub_main/0043E640.inc"
};
f32 gLightSetDirs[6][3] = {
#include "sub_main/0043E650.inc"
};
f32 gLightSetColors[6][3] = {
#include "sub_main/0043E6A0.inc"
};
f32 gLightSetAmbients[6][3] = {
#include "sub_main/0043E6F0.inc"
};
PvVec gEquipPreviewEyes[6] = {
#include "sub_main/0043E740.inc"
};
f32 gEquipPreviewWpEyeZ[12] = {
#include "sub_main/0043E790.inc"
};
f32 gEquipPreviewWpPos[22][3] = {
#include "sub_main/0043E7C0.inc"
};
f32 EquipPreview_KindRot[154] = {
#include "sub_main/0043E8D0.inc"
};
s16 gObjPartSkipLists_0[4] = {
#include "sub_main/0043EB38.inc"
};
s16 gObjPartSkipLists_14[4] = {
#include "sub_main/0043EB40.inc"
};
s16 gObjPartSkipLists_15[6] = {
#include "sub_main/0043EB48.inc"
};
s16 gObjPartSkipLists_17[4] = {
#include "sub_main/0043EB58.inc"
};
s16 gObjPartSkipLists_20[4] = {
#include "sub_main/0043EB60.inc"
};
s16 gObjPartSkipLists_19[4] = {
#include "sub_main/0043EB68.inc"
};
s16 gObjPartSkipLists_21[4] = {
#include "sub_main/0043EB70.inc"
};
s16 gObjPartSkipLists_22[4] = {
#include "sub_main/0043EB78.inc"
};
s16 gObjPartSkipLists_24[4] = {
#include "sub_main/0043EB80.inc"
};
s16 gObjPartSkipLists_26[4] = {
#include "sub_main/0043EB88.inc"
};
s16 gObjPartSkipLists_33[4] = {
#include "sub_main/0043EB90.inc"
};
s16 gObjPartSkipLists_53[4] = {
#include "sub_main/0043EB98.inc"
};
s16 gObjPartSkipLists_55[4] = {
#include "sub_main/0043EBA0.inc"
};
s16 gObjPartSkipLists_59[4] = {
#include "sub_main/0043EBA8.inc"
};
s16 gObjPartSkipLists_71[8] = {
#include "sub_main/0043EBB0.inc"
};
s16 *gObjPartSkipLists[76] = {
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_14, gObjPartSkipLists_15,
    gObjPartSkipLists_0, gObjPartSkipLists_17, gObjPartSkipLists_0, gObjPartSkipLists_19,
    gObjPartSkipLists_20, gObjPartSkipLists_21, gObjPartSkipLists_22, gObjPartSkipLists_0,
    gObjPartSkipLists_24, gObjPartSkipLists_0, gObjPartSkipLists_26, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_33, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_20,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_14,
    gObjPartSkipLists_26, gObjPartSkipLists_15, gObjPartSkipLists_21, gObjPartSkipLists_17,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_53, gObjPartSkipLists_0, gObjPartSkipLists_55,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_59,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0,
    gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_0, gObjPartSkipLists_71,
    gObjPartSkipLists_0, gObjPartSkipLists_0, NULL, NULL,
};
