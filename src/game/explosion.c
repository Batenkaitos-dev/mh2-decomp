/*
 * The Explosion attack objects (ATTACH_EXPLOSION) are the hit zones left
 * where a bomb, a bowgun shot, or a monster's fireball or breath bursts.
 * Explosion_Create, Explosion_CreateOwned, Explosion_CreateInArea and
 * Explosion_CreateAt each make an explosion of a given type. The type picks
 * its row of Explosion_Rows and its hit shapes in Explosion_HitShapes. The
 * bombs bring a second explosion that hits monsters only, and the fireballs
 * bring a follow-up burst (Explosion_Spawn). Then come PlacedItem_Create and
 * PlacedItem_Update, for the items set on the ground (enum PlacedItemType);
 * PlacedItem's Setup, Run, Hit, End and Free come with the other families.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "stageobj.h"
#include "item.h"
#include "sprite.h"
#include "capsule.h"

void Explosion_CreateInArea(Player *owner, f32 *pos, s32 kind, u16 area);
void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
s32 Player_IsLocal(void *obj);
s32 Em_HitQuadsVertical(f32 *p, Em **hitEm, s16 *hitBone, f32 *hit);
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
void PlacedItem_Update(ObjAttach *o);
void Vec_RotateY(f32 *v, f32 angle);
void ObjAttach_Free(ObjAttach *p);
s32 Player_HasSkill(Player *p, u8 skill);
u16 Rand_Next(s32 stream);
void Explosion_Setup(ObjAttach *o);
void Explosion_Run(ObjAttach *);
void Explosion_Hit(ObjAttach *o);
void Explosion_End(ObjAttach *o);
void Explosion_Free(ObjAttach *o);
void PlacedItem_Setup(struct ObjAttach *o);
void PlacedItem_Run(struct ObjAttach *o);
void PlacedItem_Hit(struct ObjAttach *);
void PlacedItem_End(ObjAttach *o);
void PlacedItem_Free(ObjAttach *o);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Vec_Copy(f32 *out, f32 *in);
void DrawSlot_Free(s32);
ObjAttach *ObjAttach_Alloc(s32 n);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void ObjAttach_StartAttack(ObjAttach *, s32 a, void *b);
s32 Player_TestSetFlags(void *obj, s32 mask);
void ObjAttach_StartOwnedAttack(ObjAttach *o, s32 a, s32 b, void *c);
extern HitShape *Explosion_HitShapes[];
extern void *Explosion_RowsPtr;
extern u8 Explosion_TakesOwnerSide[];
void Explosion_Init(ObjAttach *);
void Explosion_BoostPower(ObjAttach *);
void Explosion_Spawn(ObjAttach *);
void Explosion_Update(ObjAttach *);

/*
 * Makes o an Explosion (ATTACH_EXPLOSION, updated by Explosion_Update) and
 * clears stateVar0. The bomb and shot types 0, 2, 7, 8, 14, 15, 16, 20, 22
 * and 38 make their second explosion at once (Explosion_Spawn).
 */
void Explosion_Init(ObjAttach *o)
{
    o->kind = ATTACH_EXPLOSION;
    o->update = Explosion_Update;
    switch (o->type) {
    case EXPLOSION_BOMB:
    case EXPLOSION_LARGE_BARREL_BOMB:
    case EXPLOSION_BOWGUN_FLOOR:
    case EXPLOSION_BOWGUN_BURST_LV1:
    case EXPLOSION_BOWGUN_BURST_LV2:
    case EXPLOSION_BOWGUN_BURST_LV3:
    case EXPLOSION_DROME_DEFEAT:
    case EXPLOSION_SMALL_BARREL_BOMB_B:
    case EXPLOSION_LARGE_BARREL_BOMB_B:
    case 38:
        Explosion_Spawn(o);
        break;
    }
    o->stateVar0 = 0;
}

/*
 * Makes an explosion of `type` at pos in `area` with no owner. stateVar0 is
 * the placer's bomb boost: 1 raises the power by half (Explosion_BoostPower).
 * The bombs' fuses pass theirs (ItemAttack_RunPlaced, PlacedItem_Run); the
 * monsters' bombs pass 0.
 */
void Explosion_CreateAt(f32 *pos, s32 explType, u16 area, u8 bombBoost)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = explType;
        Vec_Copy(o->pos, pos);
        o->idx = 0;
        o->isEm = 0;
        o->area = area;
        Explosion_Init(o);
        o->stateVar0 = bombBoost;
    }
}

/*
 * Makes an explosion of `type` at pos for owner. It takes the owner's index,
 * isEm, yaw and area, and also its side when Explosion_TakesOwnerSide[type]
 * is set. Types 2, 22 and 35 are made ownerless in the scene's area instead.
 * The thrown items' bursts use it (ItemAttack_RunThrown).
 */
void Explosion_Create(Player *owner, f32 *pos, s32 explType)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = explType;
        Vec_Copy(o->pos, pos);
        if (explType != EXPLOSION_LARGE_BARREL_BOMB && explType != EXPLOSION_LARGE_BARREL_BOMB_B && explType != 35) {
            o->idx = owner->index;
            o->isEm = owner->isEm;
            o->owner = owner;
            o->yaw = owner->rot[1];
            o->area = owner->area;
            if (Explosion_TakesOwnerSide[explType] != 0) {
                o->side = owner->side;
            }
        } else {
            o->idx = 0;
            o->isEm = 0;
            o->area = gScene.area;
        }
        Explosion_Init(o);
    }
}

/*
 * As Explosion_Create, but every type takes the owner's index, isEm, yaw and
 * area. The monsters' shots and breaths use it (EmRangedAttack_Impact,
 * KushalaBreath_Impact, EmEffectAttack_Impact).
 */
void Explosion_CreateOwned(Player *owner, f32 *pos, s32 explType)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = explType;
        o->idx = owner->index;
        o->isEm = owner->isEm;
        o->owner = owner;
        o->yaw = owner->rot[1];
        Vec_Copy(o->pos, pos);
        o->area = owner->area;
        if (Explosion_TakesOwnerSide[explType] != 0) {
            o->side = owner->side;
        }
        Explosion_Init(o);
    }
}

/*
 * As Explosion_CreateOwned, but in the given area. The bowgun's shots use it
 * (BowgunShot_Fly).
 */
void Explosion_CreateInArea(Player *owner, f32 *pos, s32 explType, u16 area)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->type = explType;
        Vec_Copy(o->pos, pos);
        o->idx = owner->index;
        o->isEm = owner->isEm;
        o->owner = owner;
        o->yaw = owner->rot[1];
        o->area = area;
        if (Explosion_TakesOwnerSide[explType] != 0) {
            o->side = owner->side;
        }
        Explosion_Init(o);
    }
}

/*
 * Makes the explosion that follows src, at its position, with its owner, yaw
 * and area, and its side when Explosion_TakesOwnerSide says so. After 0, 7,
 * 8, 14, 15, 16 and 38 it makes type 9; after 2, 10; after 20, 21; after 22,
 * 23. These are the parts that hit no player (ATKF_NO_PLAYER). After 1 it
 * makes 3 and after 4 it makes 5: the fireballs' second burst. Other types
 * make nothing.
 */
void Explosion_Spawn(ObjAttach *src)
{
    ObjAttach *o;
    s16 followType;

    switch (src->type) {
    case EXPLOSION_BOMB:
    case EXPLOSION_BOWGUN_FLOOR:
    case EXPLOSION_BOWGUN_BURST_LV1:
    case EXPLOSION_BOWGUN_BURST_LV2:
    case EXPLOSION_BOWGUN_BURST_LV3:
    case EXPLOSION_DROME_DEFEAT:
    case 38:
        followType = EXPLOSION_BURST_EM_PART;
        break;
    case EXPLOSION_FIREBALL:
        followType = EXPLOSION_FIREBALL_AFTER;
        break;
    case EXPLOSION_LARGE_BARREL_BOMB:
        followType = EXPLOSION_LARGE_BARREL_BOMB_EM_PART;
        break;
    case EXPLOSION_FIREBALL_BIG:
        followType = EXPLOSION_FIREBALL_BIG_AFTER;
        break;
    case EXPLOSION_SMALL_BARREL_BOMB_B:
        followType = EXPLOSION_SMALL_BARREL_BOMB_B_EM_PART;
        break;
    case EXPLOSION_LARGE_BARREL_BOMB_B:
        followType = EXPLOSION_LARGE_BARREL_BOMB_B_EM_PART;
        break;
    default:
        return;
    }
    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = ATTACH_EXPLOSION;
        o->type = followType;
        o->update = Explosion_Update;
        o->idx = src->idx;
        o->isEm = src->isEm;
        o->owner = src->owner;
        o->yaw = *(u16 *) &src->yaw;
        Vec_Copy(o->pos, src->pos);
        o->area = src->area;
        o->stateVar0 = 0;
        if (Explosion_TakesOwnerSide[followType] != 0) {
            o->side = src->side;
        } else {
            o->side = 0;
        }
    }
}

/*
 * An explosion's update: switches on o->state to Explosion_Setup, Run, Hit,
 * End or Free. States 4 and 7 (no setter) and the deflections end it.
 */
void Explosion_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        Explosion_Setup(o);
        break;
    case ATTACH_RUN:
        Explosion_Run(o);
        break;
    case ATTACH_END:
        Explosion_End(o);
        break;
    case ATTACH_HIT:
        Explosion_Hit(o);
        break;
    case 4:
        Explosion_End(o);
        break;
    case ATTACH_DEFLECTED:
        Explosion_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        Explosion_End(o);
        break;
    case 7:
        Explosion_End(o);
        break;
    case ATTACH_FREE:
        Explosion_Free(o);
        
    default:
        break;
    }
}

/*
 * In the scene's area, plays the burst's sound at the explosion's position.
 * The bombs (types 0, 16, 20, 34, 38) share one sound and the large barrel
 * bombs (2, 22) another. The bowgun's (8, 14, 15) have a third, which type 7
 * plays one time in two. The parameter is the explosion typed as Em:
 * Em.subType, the u16 at +0xCE and rotMtx[1] are ObjAttach's type, area and
 * pos.
 */
void Explosion_PlaySound(Em *em)
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

/*
 * Sets up the explosion and moves it to ATTACH_RUN: shown, no draw, plus its
 * sound. By type it sets the ATKF_* flags and starts its row (type + 1) of
 * Explosion_Rows, either free or tied to the owner's motion
 * (ObjAttach_StartOwnedAttack). Bombs can hit their owner, the second parts hit
 * no player, the monsters' fireballs and breath bursts pierce, and type 27
 * hits no monster. Then it calls Explosion_BoostPower, takes the hit shapes
 * Explosion_HitShapes[hitListNo] and sets ownerMotion to 0 as its frame
 * count.
 */
void Explosion_Setup(ObjAttach *o)
{

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Explosion_PlaySound((Em *)o);
    switch (o->type) {
    case EXPLOSION_BOMB:
    case EXPLOSION_LARGE_BARREL_BOMB:
    case EXPLOSION_DROME_DEFEAT:
    case EXPLOSION_SMALL_BARREL_BOMB_B:
    case EXPLOSION_LARGE_BARREL_BOMB_B:
    case EXPLOSION_PLACED_BOMB:
    case EXPLOSION_DIVE_IMPACT:
    case 35:
    case 38:
        Player_TestSetFlags(o, ATKF_HITS_OWNER);
        ObjAttach_StartAttack(o, o->type + 1, &Explosion_RowsPtr);
        break;
    case 6:
        Player_TestSetFlags(o, 0);
        ObjAttach_StartAttack(o, o->type + 1, &Explosion_RowsPtr);
        break;
    case EXPLOSION_FIREBALL:
    case EXPLOSION_FIREBALL_BIG:
    case EXPLOSION_FATALIS_FIREBALL:
    case EXPLOSION_KUSHALA_BREATH:
    case EXPLOSION_KUSHALA_BREATH_BIG:
    case EXPLOSION_KUSHALA_BREATH_B:
    case EXPLOSION_KUSHALA_BREATH_B_BIG:
    case EXPLOSION_RAJANG_SHOT:
    case EXPLOSION_FATALIS_C_FIREBALL:
        Player_TestSetFlags(o, ATKF_PIERCES);
        ObjAttach_StartOwnedAttack(o, (s32) o->owner, o->type + 1, (void *) &Explosion_RowsPtr);
        break;
    case EXPLOSION_BOWGUN_FLOOR:
    case EXPLOSION_BOWGUN_BURST_LV1:
    case EXPLOSION_BOWGUN_BURST_LV2:
    case EXPLOSION_BOWGUN_BURST_LV3:
        Player_TestSetFlags(o, ATKF_HITS_OWNER);
        ObjAttach_StartOwnedAttack(o, (s32) o->owner, o->type + 1, (void *) &Explosion_RowsPtr);
        break;
    case EXPLOSION_BURST_EM_PART:
    case EXPLOSION_LARGE_BARREL_BOMB_EM_PART:
    case EXPLOSION_SMALL_BARREL_BOMB_B_EM_PART:
    case EXPLOSION_LARGE_BARREL_BOMB_B_EM_PART:
        Player_TestSetFlags(o, ATKF_NO_PLAYER);
        ObjAttach_StartAttack(o, o->type + 1, &Explosion_RowsPtr);
        break;
    case EXPLOSION_THROWN_BURST:
        Player_TestSetFlags(o, ATKF_NO_PLAYER);
        ObjAttach_StartOwnedAttack(o, (s32) o->owner, o->type + 1, (void *) &Explosion_RowsPtr);
        break;
    case EXPLOSION_THROWN_PLAYERS_ONLY:
        Player_TestSetFlags(o, ATKF_NO_EM);
        ObjAttach_StartOwnedAttack(o, (s32) o->owner, o->type + 1, (void *) &Explosion_RowsPtr);
        break;
    case EXPLOSION_THROWN_CLOUD_C:
        Player_TestSetFlags(o, ATKF_HITS_OWNER | ATKF_NO_EM);
        ObjAttach_StartOwnedAttack(o, (s32) o->owner, o->type + 1, (void *) &Explosion_RowsPtr);
        break;
    default:
        Player_TestSetFlags(o, 0);
        ObjAttach_StartOwnedAttack(o, (s32) o->owner, o->type + 1, (void *) &Explosion_RowsPtr);
        break;
    }
    Explosion_BoostPower(o);
    o->hitList = Explosion_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->ownerMotion = 0;
    o->drawSlotBuf = NULL;
}

/*
 * Counts frames in ownerMotion and ends the explosion after 40 (80 for types
 * 19, 26 and 36, the lingering bursts). The fireball types 1, 4, 18 and 24
 * end as soon as their hit window has closed (hitFrames 0), making their
 * follow-up (Explosion_Spawn). Queues the draw slot, if any, at the
 * explosion's position.
 */
void Explosion_Run(ObjAttach *o)
{
    u8 explType = o->type;
    s16 lifeFrames;

    if (explType == EXPLOSION_FIREBALL || explType == EXPLOSION_FIREBALL_BIG || explType == EXPLOSION_FATALIS_FIREBALL || explType == EXPLOSION_KUTKU_FIREBALL_SIDE_B) {
        if (o->hitFrames <= 0) {
            Explosion_Spawn(o);
            o->hitFrames = 0;
            o->state = ATTACH_END;
            return;
        }
    }
    switch (explType) {
    default:
        lifeFrames = 40;
        break;
    case 19:
    case 26:
    case 36:
        lifeFrames = 80;
        break;
    }
    if (++o->ownerMotion > lifeFrames) {
        o->hitFrames = 0;
        o->state = ATTACH_END;
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * After a hit, the lingering bursts (types 0x13, 0x1A, 0x24) keep running
 * (Explosion_Run); every other type ends.
 */
void Explosion_Hit(ObjAttach *o)
{
    switch (o->type) {
    case EXPLOSION_THROWN_CLOUD_A:
    case EXPLOSION_THROWN_CLOUD_B:
    case EXPLOSION_THROWN_CLOUD_C:
        Explosion_Run(o);
        break;
    default:
        Explosion_End(o);
        break;
    }
}

/*
 * Ends the explosion. When fireball types 1, 4 or 0x12 end from a hit
 * (ATTACH_HIT), it first makes their follow-up (Explosion_Spawn). Then it
 * closes the hit window, hides the explosion, frees its draw slot and moves
 * it to ATTACH_FREE.
 */
void Explosion_End(ObjAttach *o)
{
    if ((o->type == 1 || o->type == 4 || o->type == 0x12) && o->state == ATTACH_HIT) {
        Explosion_Spawn(o);
    }
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free(o->drawSlot);
    }
}

/* Frees the explosion (ObjAttach_Free). */
void Explosion_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * With stateVar0 1 (the placer has SKILL_COMBINE_SURE: PlacedItem_Create,
 * ItemAttack's setups), the bomb types 0, 2, 20, 22 and 38 get half again
 * their power, capped at 255.
 */
void Explosion_BoostPower(ObjAttach *o)
{
    u16 boosted;

    switch (o->stateVar0) {
    case 1:
        switch (o->type) {
        case EXPLOSION_BOMB:
        case EXPLOSION_LARGE_BARREL_BOMB:
        case EXPLOSION_SMALL_BARREL_BOMB_B:
        case EXPLOSION_LARGE_BARREL_BOMB_B:
        case 38:
            boosted = 1.5f * o->power;
            if (boosted > 0xFF) {
                boosted = 0xFF;
            }
            o->power = boosted;
            break;
        }
        break;
    }
}

/*
 * Makes a PlacedItem of `type` (enum PlacedItemType) that pl sets down at pos
 * in `area`, kept (keep) and facing pl's yaw. PLACED_TRAP and
 * PLACED_MONSTER_BOMB take the second pool. The barrel bombs and PLACED_BOMB
 * sit 50, 75 or 35 above pos, with the set-down sound. Types 2-4 move 80
 * ahead of the player, and 2 and 3 take its side. The monster bomb sticks to
 * the monster Em_HitQuadsVertical finds (the buffer keeps the monster at +4,
 * the hit point at +8 and the bone at +0x14), with powerPct 250 and two
 * sounds. With no monster found it is freed, with a puff in the scene's area.
 * The local player's items count in gScene.placed1 (stateVar1 1), or for a
 * trap in gScene.placed2 (stateVar1 2). SKILL_COMBINE_SURE sets stateVar0 1,
 * which Explosion_BoostPower reads.
 */
void PlacedItem_Create(f32 *pos, u8 placedType, u16 area, Player *pl)
{
    ObjAttach *o;
    f32 offset[3];
    s16 pool;

    switch (placedType) {
    case PLACED_TRAP:
    case PLACED_MONSTER_BOMB:
        pool = 1;
        break;
    default:
        pool = 0;
        break;
    }
    if ((o = ObjAttach_Alloc(pool)) != NULL) {
        o->kind = ATTACH_PLACED_ITEM;
        o->type = placedType;
        o->update = PlacedItem_Update;
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
        case PLACED_LARGE_BARREL_BOMB:
        default:
            o->pos[1] += 50.0f;
            if (gScene.area == o->area) {
                Sound_PlaySeAtPosChecked(1, 0x3C, 0, o->pos, 1, 0);
            }
            break;
        case PLACED_LARGE_BARREL_BOMB_B:
            o->pos[1] += 75.0f;
            if (gScene.area == o->area) {
                Sound_PlaySeAtPosChecked(1, 0x3C, 0, o->pos, 1, 0);
            }
            break;
        case PLACED_PLAYER_HIT_A:
        case PLACED_PLAYER_HIT_B:
            o->side = pl->side;
        case PLACED_TRAP:
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 80.0f;
            Vec_RotateY(offset, (6.2831855f * pl->rot[1]) / 65536.0f);
            o->pos[0] += offset[0];
            o->pos[1] += offset[1];
            o->pos[2] += offset[2];
            break;
        case PLACED_MONSTER_BOMB:
            if (!Em_HitQuadsVertical((f32 *) pos, (Em **) (o->buf + 4), (s16 *) (o->buf + 0x14), (f32 *) (o->buf + 8))) {
                if (area == gScene.area) {
                    FireBlast_CreateScaled((f32 *) pos, 0, 0.5f);
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
        case PLACED_BOMB:
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
                case PLACED_LARGE_BARREL_BOMB:
                case PLACED_LARGE_BARREL_BOMB_B:
                case PLACED_PLAYER_HIT_A:
                case PLACED_PLAYER_HIT_B:
                case PLACED_BOMB:
                default:
                    (gScene.placed1)++;
                    o->stateVar1 = 1;
                    break;
                case PLACED_TRAP:
                    (gScene.placed2)++;
                    o->stateVar1 = 2;
                    break;
                case PLACED_MONSTER_BOMB:
                    break;
                }
            }
            if (Player_HasSkill(pl, SKILL_COMBINE_SURE) == 1) {
                o->stateVar0 = 1;
            }
        }
    }
}

/*
 * A placed item's update: switches on o->state to PlacedItem_Setup, Run, Hit,
 * End or Free. Every other state ends it.
 */
void PlacedItem_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        PlacedItem_Setup(o);
        break;
    case ATTACH_RUN:
        PlacedItem_Run(o);
        break;
    case ATTACH_END:
        PlacedItem_End(o);
        break;
    case ATTACH_HIT:
        PlacedItem_Hit(o);
        break;
    case 4:
        PlacedItem_End(o);
        break;
    case ATTACH_DEFLECTED:
        PlacedItem_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        PlacedItem_End(o);
        break;
    case 7:
        PlacedItem_End(o);
        break;
    case ATTACH_FREE:
        PlacedItem_Free(o);
        
    default:
        break;
    }
}

/* The file's data. */
HitShape Explosion_HitShapes_0[2] = {
#include "game/0089DE10.inc"
};
HitShape Explosion_HitShapes_1[2] = {
#include "game/0089DE60.inc"
};
HitShape Explosion_HitShapes_2[2] = {
#include "game/0089DEB0.inc"
};
HitShape Explosion_HitShapes_3[2] = {
#include "game/0089DF00.inc"
};
HitShape Explosion_HitShapes_4[2] = {
#include "game/0089DF50.inc"
};
HitShape Explosion_HitShapes_5[2] = {
#include "game/0089DFA0.inc"
};
HitShape Explosion_HitShapes_6[2] = {
#include "game/0089DFF0.inc"
};
HitShape Explosion_HitShapes_7[2] = {
#include "game/0089E040.inc"
};
HitShape Explosion_HitShapes_8[2] = {
#include "game/0089E090.inc"
};
HitShape Explosion_HitShapes_9[2] = {
#include "game/0089E0E0.inc"
};
HitShape Explosion_HitShapes_10[2] = {
#include "game/0089E130.inc"
};
HitShape Explosion_HitShapes_11[2] = {
#include "game/0089E180.inc"
};
HitShape Explosion_HitShapes_12[2] = {
#include "game/0089E1D0.inc"
};
HitShape Explosion_HitShapes_13[2] = {
#include "game/0089E220.inc"
};
HitShape Explosion_HitShapes_14[2] = {
#include "game/0089E270.inc"
};
HitShape Explosion_HitShapes_15[2] = {
#include "game/0089E2C0.inc"
};
HitShape Explosion_HitShapes_16[2] = {
#include "game/0089E310.inc"
};
HitShape Explosion_HitShapes_17[2] = {
#include "game/0089E360.inc"
};
HitShape Explosion_HitShapes_18[2] = {
#include "game/0089E3B0.inc"
};
HitShape Explosion_HitShapes_19[2] = {
#include "game/0089E400.inc"
};
HitShape Explosion_HitShapes_20[2] = {
#include "game/0089E450.inc"
};
HitShape Explosion_HitShapes_21[2] = {
#include "game/0089E4A0.inc"
};
HitShape Explosion_HitShapes_22[2] = {
#include "game/0089E4F0.inc"
};
HitShape Explosion_HitShapes_23[2] = {
#include "game/0089E540.inc"
};
HitShape Explosion_HitShapes_24[2] = {
#include "game/0089E590.inc"
};
HitShape Explosion_HitShapes_25[3] = {
#include "game/0089E5E0.inc"
};
HitShape Explosion_HitShapes_26[3] = {
#include "game/0089E660.inc"
};
HitShape Explosion_HitShapes_27[2] = {
#include "game/0089E6E0.inc"
};
HitShape Explosion_HitShapes_28[2] = {
#include "game/0089E730.inc"
};
HitShape Explosion_HitShapes_29[2] = {
#include "game/0089E780.inc"
};
HitShape *Explosion_HitShapes[32] = {
    Explosion_HitShapes_0, Explosion_HitShapes_1, Explosion_HitShapes_2, Explosion_HitShapes_3,
    Explosion_HitShapes_4, Explosion_HitShapes_5, Explosion_HitShapes_6, Explosion_HitShapes_7,
    Explosion_HitShapes_8, Explosion_HitShapes_9, Explosion_HitShapes_10, Explosion_HitShapes_11,
    Explosion_HitShapes_12, Explosion_HitShapes_13, Explosion_HitShapes_14, Explosion_HitShapes_15,
    Explosion_HitShapes_16, Explosion_HitShapes_17, Explosion_HitShapes_18, Explosion_HitShapes_19,
    Explosion_HitShapes_20, Explosion_HitShapes_21, Explosion_HitShapes_22, Explosion_HitShapes_23,
    Explosion_HitShapes_24, Explosion_HitShapes_25, Explosion_HitShapes_26, Explosion_HitShapes_27,
    Explosion_HitShapes_28, Explosion_HitShapes_29, 0, 0,
};
u32 Explosion_Rows[252] = {
#include "game/0089E850.inc"
};
void *Explosion_RowsPtr = (void *)Explosion_Rows;
u8 Explosion_TakesOwnerSide[48] = {
#include "game/0089EC50.inc"
};
