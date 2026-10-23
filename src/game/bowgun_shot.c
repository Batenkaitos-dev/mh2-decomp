/*
 * BowgunShot, a family of attack objects (ObjAttach): a bowgun's shot in
 * flight. Bowgun_FireShot makes it (BowgunShot_Create, then BowgunShot_Init at
 * set up), and its row of gBowgunShotParams (its ammunition kind) says how it
 * flies (BowgunShotParam.kind) and what a hit does (BowgunShotParam.hitKind): it
 * ends, sticks to the bone it hit and may burst into an Explosion, pierces,
 * ricochets, scatters into pellets or leaves a smoke screen. Its power steps
 * through stages by the frames it has flown (BowgunShot_UpdatePowerStage,
 * BowgunShot_ApplyStage); a deflected shot bounces off or is batted back at its
 * shooter (BowgunShot_Deflect). It draws a streak, its mesh and a glow
 * (BowgunShot_Glow*) through draw slots. BowgunShot_InitWeapon and
 * BowgunShot_InitSkills read the weapon's level, options and the shooter's
 * skills; BowgunShot_StepRandBits and the BitsTo* helpers turn the shot's random
 * bits into per-frame jitter. BowgunShot_Update, which ObjAttach_UpdateAll
 * calls every frame, switches on ObjAttach.state (enum ObjAttachState).
 */
#include "common.h"
#include "save.h"
#include "render_state.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "capsule.h"
#include "sprite.h"
#include "area.h"
#include "quest.h"
#include "sound.h"
#include "text.h"
#include "cam_helpers.h"
#include "draw_slots.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "hit_effects.h"
#include "hit_setup.h"
#include "monster_effects.h"
#include "place_rules.h"
#include "rand.h"
#include "vec_math.h"

/* prototypes */
extern u16 BowgunShot_StreakFrames[];
void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
extern s32 BowgunShot_ScatterParams[];
extern void *BowgunShot_RowsPtr;
extern HitShape *BowgunShot_HitShapes[];
extern s32 BowgunShot_Colors[];
extern f32 (*EmRangedAttack_KirinStrikeSpots[])[2];
extern s16 BowgunShot_StageHitFxOdd[];
extern s16 BowgunShot_StageHitFxEven[];
extern f32 BowgunShot_StagePowerScales[];
extern s16 BowgunShot_StageHitSe[];
extern s16 *BowgunShot_StageFrameTbls[];
u8 Equip_GetClass(EquipItem *e);
extern f32 gCamMatrix[];
extern f32 BowgunShot_GlowFirstScales[][3];
extern f32 BowgunShot_GlowSecondScales[][3];
extern f32 BowgunShot_GlowFlashScales[][3];
extern BowgunShotParam gBowgunShotParams[];
void Draw_SetBlendFlags(u16 flags);
void ObjAttach_Move(ObjAttach *);
void ObjAttach_MoveAccel(ObjAttach *);
extern f32 gCamPos;
extern HitShape *Explosion_HitShapes[];
/* end of prototypes */

extern f32 gSilencerShotWobble;
extern s32 BowgunShot_PlayerFlags[];
/*
 * The bowgun's shot: the muzzle's flash and smoke by the ammunition kind
 * (ammo with bit 0x80 names it, else pl->ammoType; not for the local player
 * while aiming), then the shot itself (BowgunShot_Create with pl->ammoType).
 * bone is the gun bone the effects start from.
 */
void Bowgun_FireShot(Player *pl, s32 ammo, s32 bone, f32 scale)
{
    f32 launchPos[3];
    s32 launchAngles[3];
    u8 kind;

    if ((ammo & 0x80) == 0) {
        kind = pl->ammoType;
    } else {
        kind = ammo & 0x7F;
    }
    if ((pl->index != gScene.localPlayer || CamCtl_IsAiming(pl) == 0) && (u8)Obj_InCurrentArea(pl) == 1) {
        switch (kind) {
        default:
            ShotFlash_CreateOnBone((s32) pl, bone, 0);
            ShotFlash_CreateOnBone((s32) pl, bone, 1);
            ShotFlash_CreateOnBone((s32) pl, bone, 2);
            break;
        case 20:
        case 21:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 0);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 0);
            break;
        case 26:
        case 27:
        case 28:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 3);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 3);
            break;
        case 22:
        case 23:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 1);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 1);
            break;
        case 24:
        case 25:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 2);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 2);
            break;
        case 29:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 8);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 8);
            break;
        case 30:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 6);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 6);
            break;
        case 31:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 7);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 7);
            break;
        case 19:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 9);
            ElementBurst_CreateAtYaw(launchPos, 8, 1, (u16) pl->rot[1], 0.25f);
            break;
        case 17:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 10);
            ElementBurst_CreateAtYaw(launchPos, 9, 1, (u16) pl->rot[1], 0.25f);
            break;
        case 16:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 11);
            ElementBurst_CreateAtYaw(launchPos, 2, 1, (u16) pl->rot[1], 0.4f);
            break;
        case 15:
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 12);
            ShotFlash_CreateOnBoneColored(pl, bone, 6, 12);
            break;
        case 18:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            ShotFlash_CreateOnBoneColored(pl, bone, 5, 13);
            PlayerFx_CreateAt(launchPos, 12, 1, 0.4f);
            break;
        }
    }
    BowgunShot_Create(pl, pl->ammoType, bone, scale);
}

/*
 * Makes a BowgunShot of ammunition kind (a row of gBowgunShotParams) for pl;
 * bone goes to aux, the gun bone it starts from; scale is the shot's power
 * scale.
 */
void BowgunShot_Create(Player *pl, u8 kind, s16 bone, f32 scale)
{
    ObjAttach *obj;
    BowgunShotBuf *buf;

    if ((obj = ObjAttach_Alloc(1)) != NULL) {
        buf = (BowgunShotBuf *) obj->buf;
        obj->kind = ATTACH_BOWGUN_SHOT;
        obj->type = kind;
        obj->aux = bone;
        obj->update = BowgunShot_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar1 = 0;
        obj->area = pl->area;
        obj->side = *(u8 *) &pl->side;
        buf->scale = scale;
    }
}

/*
 * Splits a shot into pellets (the scatter shot): params gives their kind and
 * count; each copies the shot's flight with its own random bits, and power
 * goes to the source's buffer (the source's, not the new pellet's).
 */
void BowgunShot_Scatter(ObjAttach *obj, u8 *params, f32 power)
{
    s16 i;
    u8 kind = params[0];
    s32 randMul;
    s16 count = *(s16 *) (params + 2);
    ObjAttach *pellet;
    BowgunShotBuf *buf;

    BowgunShot_PlaySe(obj, 0);
    i = 0;
    if (i < count) {
        randMul = 1;
        do {
            if ((pellet = ObjAttach_Alloc(1)) != NULL) {
                buf = (BowgunShotBuf *) obj->buf;
                pellet->kind = ATTACH_BOWGUN_SHOT;
                pellet->type = kind;
                pellet->aux = obj->aux;
                pellet->update = BowgunShot_Update;
                pellet->idx = ((Em *) obj->owner)->index;
                pellet->isEm = ((Em *) obj->owner)->isEm;
                pellet->owner = obj->owner;
                pellet->yaw = *(u16 *) &obj->yaw;
                pellet->rotX = obj->rotX;
                pellet->rotY = obj->rotY;
                pellet->pos[0] = obj->pos[0];
                pellet->pos[1] = obj->pos[1];
                pellet->pos[2] = obj->pos[2];
                pellet->velX = obj->velX;
                pellet->velY = obj->velY;
                pellet->velZ = obj->velZ;
                pellet->randBits = obj->randBits * randMul;
                pellet->stateVar1 = i;
                pellet->area = obj->area;
                pellet->side = obj->side;
                /* the source's buffer, not the new object's */
                buf->scale = power;
            }
            i++;
            randMul += 2;
        } while (i < count);
    }
}

extern f32 gSilencerShotSpeeds[];
/*
 * Sets a shot up from its row of gBowgunShotParams: the weapon's level and
 * skills, the affinity's roll (crit 1 or a weak 2), the power, and the launch
 * speed (the level row's, plus the level bits' extras).
 */
void BowgunShot_Init(ObjAttach *obj)
{
    f32 mtx[16];
    BowgunShotBuf *buf = (BowgunShotBuf *) obj->buf;
    BowgunShotParam *param = &gBowgunShotParams[obj->type];
    Player *pl = obj->owner;
    u8 *levelRow;
    u8 bits;
    s16 affinity;
    f32 speed;
    f32 speedRand;
    f32 gravityRand;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->ownerMotion = 0;
    Player_TestSetFlags(obj, BowgunShot_PlayerFlags[obj->type]);
    ObjAttach_StartOwnedAttack(obj, (s32) pl, obj->type + 1, &BowgunShot_RowsPtr);
    buf->crit = 0;
    if (obj->type != 32) {
        BowgunShot_InitWeapon(buf, pl);
        BowgunShot_InitSkills(buf, pl);
        affinity = Player_GetWeaponStat(pl, WSTAT_AFFINITY);
        if (affinity < 0) {
            if (obj->randBits % 100 < -affinity) {
                buf->crit = 2;
            }
        } else if (affinity > obj->randBits % 100) {
            buf->crit = 1;
        }
        BowgunShot_CalcPower(obj);
        levelRow = buf->levelRow;
        bits = buf->level;
        speed = *(f32 *) (levelRow + 4);
        speedRand = *(f32 *) (levelRow + 8);
        gravityRand = *(f32 *) (levelRow + 12);
        if (bits & 0x10) {
            speed += gSilencerShotSpeeds[0];
            speedRand += gSilencerShotSpeeds[1];
            gravityRand += gSilencerShotSpeeds[2];
        }
        if (bits & 0x20) {
            if (buf->isLightBowgun == 0) {
                speed += gBarrelHeavyShotSpeeds[0];
                speedRand += gBarrelHeavyShotSpeeds[1];
                gravityRand += gBarrelHeavyShotSpeeds[2];
            } else {
                speed += gBarrelLightShotSpeeds[0];
                speedRand += gBarrelLightShotSpeeds[1];
                gravityRand += gBarrelLightShotSpeeds[2];
            }
        }
        bits = buf->skills;
        speed *= param->speed;
        speedRand *= param->speedRand;
        gravityRand *= param->gravityRand;
        if (bits & 0x10) {
            speedRand *= 2.0f;
            gravityRand *= 2.0f;
        }
        if (bits & 0x20) {
            speedRand *= 0.5f;
            gravityRand *= 0.5f;
        }
    } else {
        speed = param->speed;
        speedRand = param->speedRand;
        gravityRand = param->gravityRand;
    }
    buf->power = obj->power;
    obj->hitList = BowgunShot_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->step = 0;
    obj->stateVar0 = 0;
    buf->bounced = 0;
    buf->stage = 0;
    buf->flyStep = 0;
    buf->nextHit = 0;
    obj->homingTurn = 0.0f;
    obj->homingSpeed = 0.0f;
    switch (param->kind) {
    case 4:
    default:
    case 0:
    case 1:
        if (param->model == 98) {
            buf->bounced = 1;
            obj->rotZ = Rand_Next(1);
            if (param->kind == 4) {
                obj->hitPhase = 0;
            }
        }
        BowgunShot_BoneLaunchPos(obj->pos, pl, &obj->rotX, obj->aux);
        obj->velX = 0.0f;
        obj->velY = 0.0f;
        obj->velZ = speed + speedRand * BowgunShot_BitsToFrac(obj->randBits, 0x10);
        Vec_RotateX(&obj->velX, (6.2831855f * obj->rotX) / 65536.0f);
        Vec_RotateY(&obj->velX, (6.2831855f * obj->rotY) / 65536.0f);
        obj->gravity = -(param->gravity + gravityRand * BowgunShot_BitsToFrac(obj->randBits >> 1, 15));
        break;
    case 2:
        Mtx_Copy(mtx, (f32 *)Em_GetBone(pl, obj->aux));
        obj->velX = speed + speedRand * BowgunShot_BitsToFrac(obj->randBits >> 2, 14);
        if ((obj->randBits >> 3) & 1) {
            obj->velX = -obj->velX;
        }
        obj->velY = 2.0f * (speed + speedRand * BowgunShot_BitsToFrac(obj->randBits >> 4, 12));
        obj->velZ = speed + speedRand * BowgunShot_BitsToFrac(obj->randBits >> 5, 11);
        if ((obj->randBits >> 6) & 1) {
            obj->velZ = -obj->velZ;
        }
        obj->rotX = (u16) (s32) (0.5f + 65536.0f * Math_Atan2(obj->velY, Math_Sqrtf(obj->velX * obj->velX + obj->velZ * obj->velZ)) / 6.2831855f);
        obj->rotY = (u16) (s32) (0.5f + 65536.0f * Math_Atan2(obj->velX, obj->velZ) / 6.2831855f) + ANGLE_TURN / 2;
        obj->gravity = -(param->gravity + gravityRand * BowgunShot_BitsToFrac(obj->randBits >> 7, 9));
        break;
    }
    Vec_Copy(obj->pos2, obj->pos);
    Vec_Copy(buf->drawPos, obj->pos);
    BowgunShot_PlaySe(obj, 1);
    if (param->life == 255) {
        buf->streakPhase = 2;
    } else {
        buf->streakPhase = 0;
    }
    buf->glowState = 0;
    if (param->powerTbl != -1) {
        BowgunShot_ApplyStage(obj);
    }
}

/*
 * Moves a shot one frame. A shot that has hit (stateVar0 > 0) first acts by
 * its hitKind: a sticking shot (1, 3-5) follows its offset from the bone aux
 * of targetEm until count frames pass (3-5 then burst: a FireBlast and an
 * Explosion EXPLOSION_BOWGUN_BURST_LV1 to _LV3) or the monster is gone; a
 * piercing shot (8) reopens its hit when nextHit counts down to 0, at 0.7
 * times its speed; a ricochet (6) bounces once off the centre of the part it
 * hit, with jittered angles, and reopens its hit the same way. Then its
 * flight kind acts: kinds 0 and 1 get a random push (vertical by wobbleY,
 * sideways in the first 20 frames by the level row's spread, the silencer,
 * the barrel and the spread skills); kind 4 opens a larger hit zone every 4
 * frames and ends after 22. It moves, and on reaching the floor it ends (hit
 * kind 7 scatters, 2 leaves an Explosion EXPLOSION_BOWGUN_FLOOR, 10 a smoke
 * screen). Last it steps its power stage, its streak phase (a ShotFlash when
 * the streak ends) and its glow.
 */
void BowgunShot_Fly(ObjAttach *shot)
{
    f32 mtx[16];
    f32 acc[3];
    f32 boneOrigin[3];
    f32 stuckOfs[3];
    f32 partBoneOrigin[3];
    f32 dir[3];
    f32 partCenter[3];
    f32 slowVel[3];
    BowgunShotBuf *buf = (BowgunShotBuf *) shot->buf;
    BowgunShotParam *param = &gBowgunShotParams[shot->type];
    Em *stuckEm;
    f32 horizSpeed;
    f32 reflect;
    f32 wobbleOrFloorY;
    s32 pitchJitter;
    s32 pitch;
    HitShape *shape;

    if (shot->stateVar0 > 0) {
        switch (param->hitKind) {
        case SHOT_HIT_END:
        case SHOT_HIT_FLOOR_BURST:
        case SHOT_HIT_SCATTER:
            break;
        case SHOT_HIT_STICK:
            if (shot->ownerMotion >= param->count) {
                shot->state = ATTACH_END;
                goto tail;
            }
            if (shot->targetEm == 0) {
                shot->state = ATTACH_END;
                goto tail;
            }
            stuckEm = shot->targetEm;
            if (stuckEm->taskPhase == 3 || stuckEm->active == 0) {
                shot->state = ATTACH_END;
                goto tail;
            }
            Mtx_Copy((f32 *) mtx, (f32 *) (Em_GetBone(stuckEm, shot->aux)));
            Mtx_GetTrans(boneOrigin, mtx);
            Vec_Rotate(stuckOfs, &shot->velX, mtx);
            shot->pos[0] = stuckOfs[0] + boneOrigin[0];
            shot->pos[1] = stuckOfs[1] + boneOrigin[1];
            shot->pos[2] = stuckOfs[2] + boneOrigin[2];
            goto tail;
        case SHOT_HIT_STICK_BURST_LV1:
        case SHOT_HIT_STICK_BURST_LV2:
        case SHOT_HIT_STICK_BURST_LV3:
            if (shot->ownerMotion >= param->count) {
                if (shot->area == gScene.area) {
                    FireBlast_CreateScaled(shot->pos, 0, 1.0f);
                }
                switch (param->hitKind) {
                case 3:
                    Explosion_CreateInArea(shot->owner, shot->pos, EXPLOSION_BOWGUN_BURST_LV1, shot->area);
                    break;
                case 4:
                    Explosion_CreateInArea(shot->owner, shot->pos, EXPLOSION_BOWGUN_BURST_LV2, shot->area);
                    break;
                case 5:
                    Explosion_CreateInArea(shot->owner, shot->pos, EXPLOSION_BOWGUN_BURST_LV3, shot->area);
                    break;
                }
                shot->state = ATTACH_END;
                goto tail;
            }
            if (shot->targetEm == 0) {
                shot->state = ATTACH_END;
                goto tail;
            }
            stuckEm = shot->targetEm;
            if (stuckEm->taskPhase == 3 || stuckEm->active == 0) {
                shot->state = ATTACH_END;
                goto tail;
            }
            Mtx_Copy((f32 *) mtx, (f32 *) (Em_GetBone(stuckEm, shot->aux)));
            Mtx_GetTrans(boneOrigin, mtx);
            Vec_Rotate(stuckOfs, &shot->velX, mtx);
            shot->pos[0] = stuckOfs[0] + boneOrigin[0];
            shot->pos[1] = stuckOfs[1] + boneOrigin[1];
            shot->pos[2] = stuckOfs[2] + boneOrigin[2];
            goto tail;
        case SHOT_HIT_PIERCE:
            if (buf->nextHit != 0 && --buf->nextHit == 0) {
                ObjAttach_StartOwnedAttack(shot, (s32) shot->owner, shot->type + 1, &BowgunShot_RowsPtr);
                BowgunShot_CalcPower(shot);
                shot->hitList = BowgunShot_HitShapes[0];
                shot->hurtList = 0;
                shot->velX *= 0.7f;
                shot->velY *= 0.7f;
                shot->velZ *= 0.7f;
                BowgunShot_ApplyStage(shot);
            }
            break;
        case SHOT_HIT_RICOCHET:
            switch (buf->flyStep) {
            case 0:
                if (shot->targetEm == 0) {
                    shot->state = ATTACH_END;
                    shot->hitPhase = 0;
                    goto tail;
                }
                shape = shot->targetPart;
                if (shape == NULL || *(u8 *) shot->targetEm == 0) {
                    shot->state = ATTACH_END;
                    shot->hitPhase = 0;
                    goto tail;
                }
                switch (shape->kind) {
                case 0:
                    partCenter[0] = shape->a[0];
                    partCenter[1] = shot->targetPart->a[1];
                    partCenter[2] = shot->targetPart->a[2];
                    break;
                case 1:
                    partCenter[0] = 0.5f * (shape->a[0] + shape->b[0]);
                    partCenter[1] = 0.5f * (shot->targetPart->a[1] + shot->targetPart->b[1]);
                    partCenter[2] = 0.5f * (shot->targetPart->a[2] + shot->targetPart->b[2]);
                    break;
                }
                Mtx_Copy((f32 *) mtx, (f32 *) (Em_GetBone(shot->targetEm, shot->targetPart->bone)));
                Vec_RotateSelf(partCenter, mtx);
                Mtx_GetTrans(partBoneOrigin, mtx);
                partCenter[0] += partBoneOrigin[0];
                partCenter[1] += partBoneOrigin[1];
                partCenter[2] += partBoneOrigin[2];
                dir[0] = shot->pos[0] - partCenter[0];
                dir[1] = shot->pos[1] - partCenter[1];
                dir[2] = shot->pos[2] - partCenter[2];
                Vec_Normalize(dir);
                Vec_Copy(slowVel, &shot->velX);
                slowVel[0] *= 0.6f;
                slowVel[1] *= 0.6f;
                slowVel[2] *= 0.6f;
                reflect = -2.0f * Vec_Dot(slowVel, dir);
                dir[0] *= reflect;
                dir[1] *= reflect;
                dir[2] *= reflect;
                shot->velX += dir[0];
                shot->velY += dir[1];
                shot->velZ += dir[2];
                horizSpeed = Math_Sqrtf(shot->velX * shot->velX + shot->velZ * shot->velZ);
                pitchJitter = (u8) BowgunShot_StepRandBits(shot->randBits, shot->aux, 0x14D, 2) - 128;
                pitch = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(shot->velY, horizSpeed)) / 6.2831855f);
                shot->rotX = pitch + pitchJitter;
                shot->rotY = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(shot->velX, shot->velZ)) / 6.2831855f)
                          + ((u8) BowgunShot_StepRandBits(shot->randBits, shot->aux, 2117, 5) - 128);
                buf->flyStep++;
            case 1:
                if (buf->nextHit != 0 && --buf->nextHit == 0) {
                    ObjAttach_StartOwnedAttack(shot, (s32) shot->owner, shot->type + 1, &BowgunShot_RowsPtr);
                    BowgunShot_CalcPower(shot);
                    shot->hitList = BowgunShot_HitShapes[0];
                    shot->hurtList = 0;
                }
                break;
            }
            break;
        }
    }
    Vec_Copy(shot->pos2, shot->pos);
    switch (param->kind) {
    case SHOT_FLY_STRAIGHT:
    case SHOT_FLY_STRAIGHT_B:
    default:
        horizSpeed = Math_Sqrtf(shot->velX * shot->velX + shot->velZ * shot->velZ);
        acc[1] = param->wobbleY * horizSpeed * BowgunShot_BitsToFrac(BowgunShot_StepRandBits(shot->randBits, shot->ownerMotion, 5247, 2), 14);
        if (shot->ownerMotion < 20 && BowgunShot_StepRandBits(shot->randBits, shot->ownerMotion, 12357, 1) < 8192) {
            wobbleOrFloorY = *(f32 *) (buf->levelRow + 16);
            if (buf->level & GUN_OPT_SILENCER) {
                wobbleOrFloorY += gSilencerShotWobble;
            }
            if (buf->level & GUN_OPT_BARREL) {
                if (buf->isLightBowgun == 0) {
                    wobbleOrFloorY += gBarrelHeavyShotWobble;
                } else {
                    wobbleOrFloorY += gBarrelLightShotWobble;
                }
            }
            wobbleOrFloorY *= param->wobbleX;
            if (buf->skills & 0x10) {
                wobbleOrFloorY *= 2.0f;
            }
            if (buf->skills & 0x20) {
                wobbleOrFloorY *= 0.5f;
            }
            acc[0] = wobbleOrFloorY * BowgunShot_BitsToSignedFrac(BowgunShot_StepRandBits(shot->randBits, shot->ownerMotion, 12577, 4), 6);
        } else {
            acc[0] = 0.0f;
        }
        acc[2] = 0.0f;
        Vec_RotateX(acc, (6.2831855f * shot->rotX) / 65536.0f);
        Vec_RotateY(acc, (6.2831855f * shot->rotY) / 65536.0f);
        Vec_Add(&shot->velX, &shot->velX, acc);
        break;
    case SHOT_FLY_PULSE:
        if (shot->ownerMotion > 22 || buf->stage == 255) {
            shot->hitFrames = 0;
            shot->state = ATTACH_END;
            goto tail;
        }
        if (shot->ownerMotion > 2 && (shot->ownerMotion & 3) == 0) {
            buf->nextHit++;
            if (buf->nextHit <= param->count) {
                ObjAttach_StartOwnedAttack(shot, (s32) shot->owner, shot->type + 1, &BowgunShot_RowsPtr);
                BowgunShot_CalcPower(shot);
                shot->hitList = BowgunShot_HitShapes[buf->nextHit];
                shot->hurtList = 0;
            }
        }
        break;
    case SHOT_FLY_SPRAY:
        break;
    }
    if (shot->ownerMotion < param->boostFrames) {
        ObjAttach_Move(shot);
    } else {
        ObjAttach_MoveAccel(shot);
    }
    shot->hitFrames = 99;
    if (param->kind != SHOT_FLY_SPRAY || shot->ownerMotion >= 10) {
        wobbleOrFloorY = AreaGrid2_GetFloorYFiltered(shot->pos);
        if (param->kind == SHOT_FLY_PULSE) {
            wobbleOrFloorY -= 100.0f;
        }
        if (shot->pos[1] <= wobbleOrFloorY) {
            shot->hitFrames = 0;
            shot->state = ATTACH_END;
            switch (param->hitKind) {
            case SHOT_HIT_SCATTER:
                if (buf->stage != 255) {
                    BowgunShot_Scatter(shot, (u8 *) &BowgunShot_ScatterParams[param->count], buf->scale);
                }
                break;
            case SHOT_HIT_FLOOR_BURST:
                Explosion_CreateInArea(shot->owner, shot->pos, EXPLOSION_BOWGUN_FLOOR, shot->area);
                if (shot->area == gScene.area) {
                    FireBlast_CreateScaled(shot->pos, 0, 1.0f);
                }
                break;
            case SHOT_HIT_SMOKE:
                ItemProp_CreateFromAttach(shot, ITEMPROP_SMOKE, 1);
                break;
            }
        }
    }
    if (BowgunShot_UpdatePowerStage(shot)) {
        BowgunShot_ApplyStage(shot);
    }
tail:
    switch (buf->streakPhase) {
    case 0:
        if (shot->ownerMotion > BowgunShot_StreakFrames[param->life] || shot->stateVar0 > 0) {
            buf->streakPhase++;
            if (shot->area == gScene.area) {
                ShotFlash_CreateFromShot((u8 *) shot, 4, 0);
            }
        }
        break;
    case 1:
        buf->streakPhase++;
        break;
    }
    if (param->hasGlow != 0) {
        switch (buf->glowState) {
        case 0:
            buf->glowState++;
            BowgunShot_GlowAlloc(shot);
            break;
        case 1:
            if (param->hitKind == SHOT_HIT_PIERCE || param->hitKind == SHOT_HIT_RICOCHET) {
                if (shot->stateVar0 >= param->count) {
                    buf->glowState++;
                } else {
                    BowgunShot_GlowRun(shot);
                }
            } else if (shot->stateVar0 > 0) {
                buf->glowState++;
            } else {
                BowgunShot_GlowRun(shot);
            }
            break;
        }
    }
}

/*
 * Runs a shot in the frame it hit (ATTACH_HIT), by its hitKind: 0 and 2 end
 * it; a sticking shot (1, 3-5) stores the bone of the part it hit in aux and
 * the hit point's offset from that bone (in the bone's frame) in velX..velZ,
 * and starts counting frames again; a piercing (8) or ricochet (6) shot
 * counts the hit and sets nextHit (2 or 4 frames) until count hits, then
 * closes its hit; 7 scatters into pellets and ends; 9 flies on; 10 leaves a
 * smoke screen and ends. A shot that flies on then moves (BowgunShot_Fly); a
 * batted-back shot (stateVar1 255) just ends. Last it queues its draw slot.
 */
void BowgunShot_Hit(ObjAttach *shot)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) shot->buf;
    BowgunShotParam *param = &gBowgunShotParams[shot->type];
    f32 boneMtx[16];
    f32 invBoneMtx[16];
    f32 boneOrigin[3];
    f32 hitOfs[3];
    f32 midPoint[3];

    if (shot->stateVar1 == 255) {
        shot->hitFrames = 0;
        shot->state = ATTACH_END;
        goto draw;
    }
    buf->flyStep = 0;
    switch (buf->streakPhase) {
    case 0:
        buf->streakPhase++;
        if (shot->area == gScene.area) {
            ShotFlash_CreateFromShot((u8 *) shot, 4, 0);
        }
        break;
    case 1:
        buf->streakPhase++;
        break;
    }
    switch (param->hitKind) {
    case SHOT_HIT_END:
    case SHOT_HIT_FLOOR_BURST:
    default:
        shot->state = ATTACH_END;
        goto draw;
    case SHOT_HIT_STICK:
    case SHOT_HIT_STICK_BURST_LV1:
    case SHOT_HIT_STICK_BURST_LV2:
    case SHOT_HIT_STICK_BURST_LV3:
        shot->ownerMotion = 0;
        shot->stateVar0++;
        shot->hitPhase = 0;
        buf->bounced++;
        if (shot->targetEm == 0) {
            shot->state = ATTACH_END;
            return;
        }
        if (shot->targetPart == 0 || *(u8 *) shot->targetEm == 0) {
            shot->state = ATTACH_END;
            return;
        }
        shot->aux = shot->targetPart->bone;
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(shot->targetEm, shot->aux)));
        Mtx_GetTrans(boneOrigin, boneMtx);
        Vec_Add(midPoint, shot->pos, shot->pos2);
        Vec_Scale(midPoint, midPoint, 0.5f);
        hitOfs[0] = midPoint[0] - boneOrigin[0];
        hitOfs[1] = midPoint[1] - boneOrigin[1];
        hitOfs[2] = midPoint[2] - boneOrigin[2];
        Mtx_Inverse(invBoneMtx, boneMtx);
        Vec_Rotate(&shot->velX, hitOfs, invBoneMtx);
        shot->state = ATTACH_RUN;
        goto draw;
    case SHOT_HIT_PIERCE:
        if (++shot->stateVar0 < param->count) {
            buf->nextHit = 2;
        } else {
            shot->hitPhase = 0;
        }
        shot->state = ATTACH_RUN;
        break;
    case SHOT_HIT_SCATTER:
        BowgunShot_Scatter(shot, (u8 *) &BowgunShot_ScatterParams[param->count], buf->scale);
        shot->hitPhase = 0;
        shot->state = ATTACH_END;
        goto draw;
    case SHOT_HIT_RICOCHET:
        if (++shot->stateVar0 < param->count) {
            buf->nextHit = 4;
        } else {
            shot->hitPhase = 0;
        }
        shot->state = ATTACH_RUN;
        break;
    case SHOT_HIT_PASS:
        shot->state = ATTACH_RUN;
        break;
    case SHOT_HIT_SMOKE:
        ItemProp_CreateFromAttach(shot, ITEMPROP_SMOKE, 1);
        shot->hitPhase = 0;
        shot->state = ATTACH_END;
        goto draw;
    }
    if (++shot->ownerMotion > 120) {
        shot->hitFrames = 0;
        shot->state = ATTACH_END;
        return;
    }
    BowgunShot_Fly(shot);
draw:
    if (shot->drawSlotBuf != NULL) {
        if (param->model == 98) {
            Vec_Copy(shot->drawSlotBuf->pos, buf->drawPos);
        } else {
            Vec_Copy(shot->drawSlotBuf->pos, shot->pos);
        }
        OT_AddByDepth(gDrawOt32, shot->drawSlotBuf, 32U, 0);
    }
}

/*
 * Runs a deflected shot (ATTACH_DEFLECTED, ATTACH_DEFLECTED_AURA). Either it
 * bounces off (stateVar1 254): its hit closes and it flies back up at a
 * random angle; or, when ObjAttach_RollBatBack allows it (40% of the time,
 * never for the aura), the monster in targetEm bats it back: it becomes that
 * monster's attack (attack row 34), aimed from where it is at a point 100
 * units above its shooter's feet at speed 100 (stateVar1 255), with the
 * batted-back sound. Then it runs as ATTACH_RUN.
 */
void BowgunShot_Deflect(ObjAttach *shot)
{
    BowgunShotBuf *shotBuf = (BowgunShotBuf *) shot->buf;
    Em *shooter = shot->owner;
    Em *batter = shot->targetEm;
    f32 aimPos[3];
    f32 dir[3];
    u16 aimPitch;
    u16 aimYaw;

    switch (shotBuf->streakPhase) {
    case 0:
        shotBuf->streakPhase++;
        if (shot->area == gScene.area) {
            ShotFlash_CreateFromShot((u8 *) shot, 4, 0);
        }
        break;
    case 1:
        shotBuf->streakPhase++;
        break;
    }
    shot->step = 0;
    shot->stateVar0 = 0;
    shot->ownerMotion = 0;
    shot->side = 0;
    if (ObjAttach_RollBatBack(shot, shot->randBits) == 0) {
        shot->stateVar1 = 254;
        shot->hitPhase = 0;
        Vec_Diff(dir, shot->pos2, shot->pos);
        Vec_DirToAngles(dir, (s16 *) &aimPitch, (s16 *) &aimYaw);
        aimPitch += -(Rand_Next(1) & 0x7FF) - 0x300;
        aimYaw += (Rand_Next(1) & 0x1FFF) - ANGLE_TURN / 16;
        shot->velX = 0.0f;
        shot->velY = Rand_Next(1) & 7;
        shot->velZ = 10.0f + (Rand_Next(1) & 7);
        shot->homingTurn = 0.0f;
        shot->gravity = -1.5f - (Rand_Next(1) & 0x1F) / 32.0f;
        shot->homingSpeed = 0.0f;
        Vec_RotateX(&shot->velX, (6.2831855f * aimPitch) / 65536.0f);
        Vec_RotateY(&shot->velX, (6.2831855f * aimYaw) / 65536.0f);
    } else {
        shot->stateVar1 = 255;
        shot->idx = batter->index;
        shot->isEm = batter->isEm;
        shot->owner = batter;
        Player_TestSetFlags(shot, 0);
        ObjAttach_StartOwnedAttack(shot, (s32) batter, 34, &BowgunShot_RowsPtr);
        shot->hitList = BowgunShot_HitShapes[shot->hitListNo];
        shot->hurtList = 0;
        Vec_Copy(shotBuf->drawPos, shot->pos);
        Vec_Copy(aimPos, &shooter->posX);
        aimPos[1] += 100.0f;
        Vec_Diff(dir, aimPos, shot->pos);
        Vec_DirToAngles(dir, (s16 *) &aimPitch, (s16 *) &aimYaw);
        shot->rotX = aimPitch;
        shot->rotY = aimYaw;
        shot->yaw = shot->rotY;
        shot->velX = 0.0f;
        shot->velY = 0.0f;
        shot->velZ = 100.0f;
        Vec_RotateX(&shot->velX, (6.2831855f * shot->rotX) / 65536.0f);
        Vec_RotateY(&shot->velX, (6.2831855f * shot->rotY) / 65536.0f);
        shot->homingTurn = 0.0f;
        shot->gravity = -1.5f;
        shot->homingSpeed = 0.0f;
        Vec_Copy(shot->pos2, shot->pos);
        BowgunShot_PlaySe(shot, 2);
    }
    shot->state = ATTACH_RUN;
    BowgunShot_Run(shot);
}

/*
 * Moves a batted-back shot one frame (ObjAttach_Move for the first 20 frames,
 * then ObjAttach_MoveAccel) with a long hit window (hitFrames 99), and ends
 * it when it reaches the floor.
 */
void BowgunShot_Fall(ObjAttach *shot)
{
    Vec_Copy(shot->pos2, shot->pos);
    shot->hitFrames = 99;
    if (shot->ownerMotion < 20) {
        ObjAttach_Move(shot);
    } else {
        ObjAttach_MoveAccel(shot);
    }
    if (shot->pos[1] <= AreaGrid2_GetFloorYFiltered(shot->pos)) {
        shot->hitFrames = 0;
        shot->state = ATTACH_END;
    }
}

/*
 * Moves a shot that bounced off (stateVar1 254) one frame: its hit stays
 * closed, it falls and tumbles (rotX turns, rotY is fixed), and bounced is
 * set so the draw stops turning it along its velocity. Returns 1 after 30
 * frames, when the shot should end, else 0.
 */
u8 BowgunShot_BounceStep(ObjAttach *shot)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) shot->buf;

    shot->hitPhase = 0;
    buf->bounced = 1;
    if (++shot->ownerMotion > 30) {
        return 1;
    }
    ObjAttach_MoveAccel(shot);
    shot->rotX -= 0xC00;
    shot->rotY = 0x400;
    return 0;
}

/*
 * Draws a shot (through BowgunShot_Draw): in streak phase 0 only its streak,
 * in phase 1 the streak and its mesh, later only the mesh, mesh set
 * param->model of the common model (255: none). Model 98 is animated, one
 * mesh set per frame for 15 frames, scaled 2.25, turned by rotZ, with a
 * texture offset. Unless the shot bounced, its pitch and yaw follow its
 * velocity. The colour is BowgunShot_Colors[param->colorRow].
 */
void BowgunShot_DrawModel(DrawSlot *slot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    ObjAttach *shot = slot->owner;
    BowgunShotBuf *shotBuf = (BowgunShotBuf *) shot->buf;
    BowgunShotParam *param = &gBowgunShotParams[shot->type];
    Model *commonModel;
    ModelMatSet *meshSet;
    ModelMat *texMats;
    f32 pitch;
    f32 yaw;

    if (shot->area != gScene.area) {
        return;
    }
    switch (shotBuf->streakPhase) {
    case 0:
        BowgunShot_DrawStreak(shot);
        return;
    case 1:
        BowgunShot_DrawStreak(shot);
        break;
    }
    if (param->model == 255) {
        return;
    }
    commonModel = gCommonModels[0];
    if (commonModel == NULL) {
        return;
    }
    if (commonModel->active == 0) {
        return;
    }
    texMats = commonModel->mats;
    if (param->model == 98) {
        if (shot->ownerMotion > 14) {
            return;
        }
        Mtx_Translation(uvMtx, 0.5f, 0.375f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        Mtx_Scale(mtx, 2.25f, 2.25f, 2.25f);
        meshSet = &commonModel->matSets[param->model] + shot->ownerMotion;
        Mtx_RotateZ(mtx, (6.2831855f * shot->rotZ) / 65536.0f);
    } else {
        Mtx_Identity(mtx);
        meshSet = &commonModel->matSets[param->model];
    }
    if (shotBuf->bounced == 0) {
        pitch = Math_Atan2(-shot->velY, Math_Sqrtf(shot->velX * shot->velX + shot->velZ * shot->velZ));
        yaw = Math_Atan2(shot->velX, shot->velZ);
        shot->rotX = (u16) (s32) (0.5f + (65536.0f * pitch) / 6.2831855f);
        shot->rotY = (u16) (s32) (0.5f + (65536.0f * yaw) / 6.2831855f);
    }
    Mtx_RotateX(mtx, (6.2831855f * shot->rotX) / 65536.0f);
    Mtx_RotateY(mtx, (6.2831855f * shot->rotY) / 65536.0f);
    Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
    Gs_SetState(RS_COLOR2, BowgunShot_Colors[(s8) (u8)param->colorRow]);
    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
    Gs_SetState(RS_ALPHA_REF, 128);
    if (meshSet != NULL && meshSet->id != -1) {
        Gs_SetLightList(texMats, meshSet);
        Draw_ApplyStates(meshSet->flags);
        Model_Draw(meshSet->id, 0);
    }
    Draw_SetDefaultState();
}

/*
 * The family's update (ObjAttach.update): by ObjAttach.state it runs
 * BowgunShot_Setup, BowgunShot_Run (also in ATTACH_HIT, where Run hands the
 * shot to BowgunShot_Hit), BowgunShot_End (also in states 4 and 7),
 * BowgunShot_Deflect when deflected, and BowgunShot_Free.
 */
void BowgunShot_Update(ObjAttach *shot)
{
    switch (shot->state) {
    case ATTACH_SETUP:
        BowgunShot_Setup(shot);
        break;
    case ATTACH_RUN:
        BowgunShot_Run(shot);
        break;
    case ATTACH_END:
        BowgunShot_End(shot);
        break;
    case ATTACH_HIT:
        BowgunShot_Run(shot);
        break;
    case 4:
        BowgunShot_End(shot);
        break;
    case ATTACH_DEFLECTED:
        BowgunShot_Deflect(shot);
        break;
    case ATTACH_DEFLECTED_AURA:
        BowgunShot_Deflect(shot);
        break;
    case 7:
        BowgunShot_End(shot);
        break;
    case ATTACH_FREE:
        BowgunShot_Free(shot);
        
    default:
        break;
    }
}

/*
 * Sets a new shot up (BowgunShot_Init) and gives it a draw slot drawn by
 * BowgunShot_Draw at its position; with no free slot the object is freed.
 */
void BowgunShot_Setup(ObjAttach *shot)
{
    BowgunShot_Init(shot);
    shot->drawSlot = DrawSlot_Alloc();
    if (shot->drawSlot != -1) {
        shot->drawSlotBuf = DrawSlot_Get(shot->drawSlot);
        shot->drawSlotBuf->owner = shot;
        shot->drawSlotBuf->draw = BowgunShot_Draw;
        Vec_Copy(shot->drawSlotBuf->pos, shot->pos);
        OT_AddByDepth(gDrawOt32, shot->drawSlotBuf, 32U, 0);
    } else {
        ObjAttach_Free(shot);
    }
}

/*
 * A shot's frame. A shot that bounced off tumbles until BowgunShot_BounceStep
 * ends it; a shot that hit goes to BowgunShot_Hit; otherwise it ends after
 * 120 frames, else it flies (BowgunShot_Fly) or, batted back (stateVar1
 * 255), falls (BowgunShot_Fall). Then its draw slot is queued at its
 * position (at drawPos, the launch point, for model 98).
 */
void BowgunShot_Run(ObjAttach *shot)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) shot->buf;
    BowgunShotParam *param = &gBowgunShotParams[shot->type];

    if (shot->stateVar1 == 254) {
        if (BowgunShot_BounceStep(shot) != 0) {
            shot->state = ATTACH_END;
            return;
        }
    } else {
        if (shot->state == ATTACH_HIT) {
            BowgunShot_Hit(shot);
            return;
        }
        if (++shot->ownerMotion > 120) {
            shot->hitFrames = 0;
            shot->state = ATTACH_END;
            return;
        }
        if (shot->stateVar1 != 255) {
            BowgunShot_Fly(shot);
        } else {
            BowgunShot_Fall(shot);
        }
    }
    if (shot->drawSlotBuf != NULL) {
        if (param->model == 98) {
            Vec_Copy(shot->drawSlotBuf->pos, buf->drawPos);
        } else {
            Vec_Copy(shot->drawSlotBuf->pos, shot->pos);
        }
        OT_AddByDepth(gDrawOt32, shot->drawSlotBuf, 32U, 0);
    }
}

/*
 * Ends a shot: closes its hit, hides it, frees its draw slot and its glow's
 * (once the glow was started), and sets ATTACH_FREE.
 */
void BowgunShot_End(ObjAttach *shot)
{
    BowgunShotBuf *w;

    w = (BowgunShotBuf *) shot->buf;
    shot->hitPhase = 0;
    shot->state = ATTACH_FREE;
    shot->visible = 0;
    if (shot->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) shot->drawSlot);
    }
    if ((s32) w->glowState > 0) {
        BowgunShot_GlowFree(shot);
    }
}

/* Frees the object (ObjAttach_Free). */
void BowgunShot_Free(ObjAttach *shot)
{
    ObjAttach_Free(shot);
}

/*
 * The draw slot's function: draws the shot (BowgunShot_DrawModel) when it is
 * in the current area.
 */
void BowgunShot_Draw(DrawSlot *slot)
{
    if (((ObjAttach *)slot->owner)->area == gScene.area) {
        BowgunShot_DrawModel(slot);
    }
}

/*
 * Fills a shot's buffer from its shooter's weapon (equip[EQUIP_SLOT_WEAPON]):
 * attackFactor, the level byte (the level in the low 4 bits, GUN_OPT_*
 * above), the level's 24-byte row of gGunLevelRows, and isLightBowgun (0 for
 * weapon class 1, which takes the heavy bowgun's barrel values, else 1).
 */
void BowgunShot_InitWeapon(BowgunShotBuf *buf, Player *pl)
{
    buf->attackFactor = pl->attackFactor;
    buf->level = pl->equip[6].level;
    buf->levelRow = gGunLevelRows[gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].levelRow].bytes + (buf->level & 0xF) * 24;
    if (Equip_GetClass(&pl->equip[6]) == 1) {
        buf->isLightBowgun = 0;
    } else {
        buf->isLightBowgun = 1;
    }
}

/*
 * Sets a shot's skill bits from its shooter's skills: 1, 2 and 4 the power
 * skills of ammunition types 0-2, 3-5 and 6-8 (SKILL_AMMO_POWER_A, _B, _C), 8
 * SKILL_STATUS_PLUS, 0x10 a doubled random spread (SKILL_SPREAD_DOUBLE), 0x20 a halved
 * one (SKILL_SPREAD_HALF).
 */
void BowgunShot_InitSkills(BowgunShotBuf *buf, Player *pl)
{
    buf->skills = 0;
    if (Player_HasSkill(pl, SKILL_AMMO_POWER_A) == 1) {
        buf->skills |= 1;
    }
    if (Player_HasSkill(pl, SKILL_AMMO_POWER_B) == 1) {
        buf->skills |= 2;
    }
    if (Player_HasSkill(pl, SKILL_AMMO_POWER_C) == 1) {
        buf->skills |= 4;
    }
    if (Player_HasSkill(pl, SKILL_STATUS_PLUS) == 1) {
        buf->skills |= 8;
    }
    if (Player_HasSkill(pl, SKILL_SPREAD_DOUBLE) == 1) {
        buf->skills |= 0x10;
    }
    if (Player_HasSkill(pl, SKILL_SPREAD_HALF) == 1) {
        buf->skills |= 0x20;
    }
}

/*
 * Scales a shot's power by its attackFactor times its scale: types 0-2 and
 * 3-5 by 1.1 more and types 6-8 by 1.3 more with their power skill; the
 * element types 15-18 scale elementPower the same way; types 22-28 gain an
 * eighth of elementPower with SKILL_STATUS_PLUS. A critical roll (crit 1)
 * adds a quarter and HITFX_CRITICAL, a weak one (crit 2) halves it and adds
 * HITFX_WEAK. power and elementPower stay within 1 to 255. A shot of attack
 * kind 8 is left as it is.
 */
void BowgunShot_CalcPower(ObjAttach *shot)
{
    BowgunShotBuf *shotBuf = (BowgunShotBuf *) shot->buf;
    f32 factor;
    u16 value;

    if (shot->attackKind == 8) {
        return;
    }
    factor = shotBuf->attackFactor * shotBuf->scale;
    switch (shot->type) {
    case 0:
    case 1:
    case 2:
        if (shotBuf->skills & 1) {
            factor *= 1.1f;
        }
        break;
    case 3:
    case 4:
    case 5:
        if (shotBuf->skills & 2) {
            factor *= 1.1f;
        }
        break;
    case 6:
    case 7:
    case 8:
        if (shotBuf->skills & 4) {
            factor *= 1.3f;
        }
        break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        break;
    case 15:
    case 16:
    case 17:
    case 18:
        value = shot->elementPower * factor;
        if (value == 0) {
            value = 1;
        } else if (value > 255) {
            value = 255;
        }
        shot->elementPower = value;
        break;
    case 19:
    case 20:
    case 21:
        break;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
        if (shotBuf->skills & 8) {
            value = 1.125f * shot->elementPower;
            if (value > 255) {
                value = 255;
            }
            shot->elementPower = value;
        }
        break;
    }
    if (shot->power != 0) {
        if (shotBuf->crit == 1) {
            factor *= 1.25f;
            shot->hitFx |= HITFX_CRITICAL;
        } else if (shotBuf->crit == 2) {
            factor *= 0.5f;
            shot->hitFx |= HITFX_WEAK;
        }
        value = shot->power * factor;
        if (value == 0) {
            value = 1;
        } else if (value > 255) {
            value = 255;
        }
        shot->power = value;
    }
}

extern f32 gSilencerShotRange;
/*
 * Steps a shot's power stage (buf->stage) by the frames it has flown: each
 * entry of its row of BowgunShot_StageFrameTbls, times the range factor (the
 * level row's, plus the silencer's and the barrel's), is the frame the next
 * stage starts at; a negative entry ends the stages (stage 255) and closes
 * the hit. Returns 1 when the stage changed or ended, else 0 (always 0 for a
 * shot with no table).
 */
s32 BowgunShot_UpdatePowerStage(ObjAttach *shot)
{
    BowgunShotParam *param = &gBowgunShotParams[shot->type];
    BowgunShotBuf *buf = (BowgunShotBuf *) shot->buf;
    s32 ret = 0;
    s16 *tbl;
    f32 rangeFactor;
    s16 stageFrame;

    if (param->powerTbl == -1) {
        return 0;
    }
    tbl = BowgunShot_StageFrameTbls[param->powerTbl];
    rangeFactor = *(f32 *) (buf->levelRow + 20);
    if (buf->level & GUN_OPT_SILENCER) {
        rangeFactor += gSilencerShotRange;
    }
    if (buf->level & GUN_OPT_BARREL) {
        if (buf->isLightBowgun == 0) {
            rangeFactor += gBarrelHeavyShotRange;
        } else {
            rangeFactor += gBarrelLightShotRange;
        }
    }
    while (buf->stage != 255) {
        stageFrame = rangeFactor * tbl[buf->stage];
        if (!(stageFrame < shot->ownerMotion)) {
            break;
        }
        ret = 1;
        if (stageFrame < 0) {
            buf->stage = 255;
            shot->hitPhase = 0;
            break;
        }
        buf->stage++;
    }
    return ret;
}

/*
 * Gives a shot whose hit effect changes with the stage (hitFx kinds 9-16) its
 * stage's hit effect (one table for the odd kinds, one for the even), its
 * critical mark again, its stage's hit sound, and its power at set up times
 * the stage's scale (kept within 1 to 255). Does nothing for a shot with no
 * power, past stage 4, or past the last stage.
 */
void BowgunShot_ApplyStage(ObjAttach *shot)
{
    BowgunShotBuf *shotBuf = (BowgunShotBuf *) shot->buf;
    s16 *hitFxTbl;
    f32 stagePower;

    if (shot->power == 0 || shotBuf->stage > 4 || shotBuf->stage == 255) {
        return;
    }
    switch (shot->hitFx & 0x3F) {
    case 9:
    case 11:
    case 13:
    case 15:
        hitFxTbl = BowgunShot_StageHitFxOdd;
        break;
    case 10:
    case 12:
    case 14:
    case 16:
        hitFxTbl = BowgunShot_StageHitFxEven;
        break;
    default:
        return;
    }
    shot->hitFx = hitFxTbl[shotBuf->stage];
    if (shotBuf->crit == 1) {
        shot->hitFx |= HITFX_CRITICAL;
    } else if (shotBuf->crit == 2) {
        shot->hitFx |= HITFX_WEAK;
    }
    shot->hitSe = BowgunShot_StageHitSe[shotBuf->stage];
    if (shotBuf->power != 0.0f) {
        stagePower = shotBuf->power * BowgunShot_StagePowerScales[shotBuf->stage];
        if (!(stagePower < 255.0f)) {
            shot->power = 255;
        } else if (stagePower <= 1.0f) {
            shot->power = 1;
        } else {
            shot->power = stagePower;
        }
    }
}

/*
 * Draws a shot's streak (likely): mesh set 89 of the common model at the
 * shot's position and angles, twice, the second turned half round about its
 * axis.
 */
void BowgunShot_DrawStreak(ObjAttach *shot)
{
    f32 mtx[16];
    Model *commonModel = gCommonModels[0];
    ModelMat *texMats;
    ModelMatSet *meshSet;
    s16 i;

    if (commonModel != NULL && commonModel->active != 0) {
        meshSet = &commonModel->matSets[89];
        Gs_SetState(RS_ALPHA_REF, 128);
        texMats = commonModel->mats;
        for (i = 0; i < 2; i++) {
            Mtx_Identity(mtx);
            Mtx_RotateX(mtx, (6.2831855f * shot->rotX) / 65536.0f);
            Mtx_RotateY(mtx, (6.2831855f * shot->rotY) / 65536.0f);
            Mtx_SetTrans(mtx, shot->pos[0], shot->pos[1], shot->pos[2]);
            if (i != 0) {
                Mtx_TurnZ(mtx, 3.1415927f);
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            if (meshSet != NULL && meshSet->id != -1) {
                Gs_SetLightList(texMats, meshSet);
                Draw_ApplyStates(meshSet->flags);
                Model_Draw(meshSet->id, 0);
            }
        }
        Draw_SetDefaultState();
    }
}

/*
 * Returns 16 random bits for one step of a shot: randBits plus step times
 * mul, shifted right by shift. With the frame count or the bone as step, each
 * frame of a shot gets its own jitter from the shot's random bits.
 */
u16 BowgunShot_StepRandBits(u16 randBits, u16 step, s32 mul, s32 shift)
{
    return ((s32) ((randBits & 0xFFFF) + ((step & 0xFFFF) * (mul & 0xFFFF))) >> (shift & 0xFFFF)) & 0xFFFF;
}

/*
 * Returns the low bits bits of randBits as a fraction from 0 to about 1
 * (moved to the top of 16 bits, then times 1/64000).
 */
f32 BowgunShot_BitsToFrac(u16 randBits, s32 bits)
{
    s32 shift;

    shift = (0x10 - (bits & 0xFFFF)) & 0xFFFF;
    return 0.000015625f * (f32) ((randBits & 0xFFFF & ((0xFFFF >> shift) & 0xFFFF)) << shift);
}

/*
 * As BowgunShot_BitsToFrac, centred on 0: a fraction from about -0.5 to 0.5.
 */
f32 BowgunShot_BitsToSignedFrac(u16 value, u16 bits)
{
    u16 shift = 16 - bits;

    return 0.000015625f * (s16) (((value & (u16) (0xFFFF >> shift)) << shift) - 32768);
}

/*
 * Computes where and how a shot leaves the gun: ang[0] and ang[1] get the
 * pitch and yaw of the player pl's bone (the yaw turned half round) as 16-bit
 * angles, and out gets the bone's position plus an offset (-10, -10, 87)
 * turned by them.
 */
void BowgunShot_BoneLaunchPos(f32 *out, Player *pl, s32 *angles, s16 bone)
{
    f32 muzzleOfs[3];
    f32 boneMtx[16];
    f32 pitch;
    f32 yaw;

    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(pl, bone)));
    Mtx_GetTrans(out, boneMtx);
    pitch = Math_Atan2(boneMtx[9], Math_Sqrtf(boneMtx[8] * boneMtx[8] + boneMtx[10] * boneMtx[10]));
    yaw = 3.1415927f + Math_Atan2(boneMtx[8], boneMtx[10]);
    angles[0] = (u16) (s32) (0.5f + (65536.0f * pitch) / 6.2831855f);
    angles[1] = (u16) (s32) (0.5f + (65536.0f * yaw) / 6.2831855f);
    muzzleOfs[0] = -10.0f;
    muzzleOfs[1] = -10.0f;
    muzzleOfs[2] = 87.0f;
    Vec_RotateX(muzzleOfs, pitch);
    Vec_RotateY(muzzleOfs, yaw);
    out[0] += muzzleOfs[0];
    out[1] += muzzleOfs[1];
    out[2] += muzzleOfs[2];
}

/*
 * Plays a shot's sound when it is in the current area: kind 0 the scatter
 * sound (types 12-14 only), 1 the firing sound, one of three by the recoil
 * level (Bowgun_RecoilLevel), another three with a silencer (none for type
 * 32), 2 the batted-back sound.
 */
void BowgunShot_PlaySe(ObjAttach *shot, s16 kind)
{
    s16 seOfs;

    if (shot->area == gScene.area) {
        switch (kind) {
        case 0:
            switch (shot->type) {
            case 12:
            case 13:
            case 14:
                Sound_PlaySeAtPosChecked(1, 78, 0, shot->pos, 1, 0);
                break;
            }
            break;
        case 1:
            if (shot->type == 32) {
                break;
            }
            if (LightBowgun_HasSilencer(shot->owner)) {
                if (((Player *) shot->owner)->weaponType == WEAPON_HEAVY_BOWGUN) {
                    seOfs = 5;
                } else {
                    seOfs = 6;
                }
            } else {
                seOfs = 0;
            }
            switch (Bowgun_RecoilLevel(shot->owner, shot->type)) {
            case 0:
                Player_PlaySe(shot->owner, seOfs + 10, 0, shot->pos, 1, 0);
                break;
            case 1:
                Player_PlaySe(shot->owner, seOfs + 11, 0, shot->pos, 1, 0);
                break;
            case 2:
                Player_PlaySe(shot->owner, seOfs + 12, 0, shot->pos, 1, 0);
                break;
            }
            break;
        case 2:
            Sound_PlaySeAtPosChecked(1, 150, 0, shot->pos, 6, 0);
            break;
        }
    }
}

/*
 * Starts a shot's glow: gives it a draw slot drawn by BowgunShot_GlowDraw
 * (none when no slot is free) and runs its first frame.
 */
void BowgunShot_GlowAlloc(ObjAttach *shot)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) shot->buf;
    BowgunShotGlow *glow;

    buf->glowFlashTimer = 0;
    glow = &buf->glow;
    glow->drawSlot = DrawSlot_Alloc();
    if (glow->drawSlot != -1) {
        glow->timer = 0;
        glow->unk0A = 0;
        glow->unk06 = 0;
        glow->slot = DrawSlot_Get(glow->drawSlot);
        glow->slot->owner = shot;
        glow->slot->part = 0;
        glow->slot->draw = BowgunShot_GlowDraw;
    } else {
        glow->slot = NULL;
    }
    BowgunShot_GlowRun(shot);
}

/*
 * Runs a shot's glow one frame: places it 25 units behind the shot along its
 * velocity and counts its timer; in stage 1 it counts glowFlashTimer and
 * makes a ShotFlash (type 3) on its first frame.
 */
void BowgunShot_GlowRun(ObjAttach *shot)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) shot->buf;
    BowgunShotGlow *glow = &buf->glow;
    f32 trailOfs[3];

    Vec_Copy(trailOfs, &shot->velX);
    Vec_Normalize(trailOfs);
    trailOfs[0] *= -25.0f;
    trailOfs[1] *= -25.0f;
    trailOfs[2] *= -25.0f;
    switch (buf->stage) {
    case 1:
        if (++buf->glowFlashTimer == 1 && shot->area == gScene.area) {
            ShotFlash_CreateAt(shot->pos, 3, 0);
        }
        break;
    }
    (glow->timer)++;
    if (glow->slot != NULL) {
        glow->slot->pos[0] = shot->pos[0] + trailOfs[0];
        glow->slot->pos[1] = shot->pos[1] + trailOfs[1];
        glow->slot->pos[2] = shot->pos[2] + trailOfs[2];
        OT_AddByDepth(gDrawOtSorted, glow->slot, 64U, 0);
    }
}

/* Frees a shot's glow's draw slot. */
void BowgunShot_GlowFree(ObjAttach *shot)
{
    BowgunShotGlow *glow = &((BowgunShotBuf *) shot->buf)->glow;

    if (glow->slot != NULL) {
        DrawSlot_Free(glow->drawSlot);
    }
}

/*
 * Draws a shot's glow while it is in the current area and in stage 2 or less:
 * two meshes of the common model (sets 64 and 66) facing the camera, their
 * sizes alternating each frame (BowgunShot_GlowFirstScales and
 * _GlowSecondScales by the timer's low bit); in the first 5 frames of stage 1
 * the second takes the sizes of BowgunShot_GlowFlashScales instead.
 */
void BowgunShot_GlowDraw(DrawSlot *slot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    ObjAttach *shot;
    BowgunShotGlow *glow;
    ModelMat *texMats;
    s16 flicker;
    f32 *flashScale;
    BowgunShotBuf *buf;
    Model *commonModel;
    ModelMatSet *meshSet;
    s16 i;
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;

    shot = slot->owner;
    commonModel = gCommonModels[0];
    buf = (BowgunShotBuf *) shot->buf;
    glow = &buf->glow + slot->part;
    if (shot->area == gScene.area && commonModel != NULL && commonModel->active != 0 && buf->stage <= 2) {
        texMats = commonModel->mats;
        flicker = glow->timer & 1;
        for (i = 0; i < 2; i++) {
            if (i == 0) {
                meshSet = &commonModel->matSets[64];
                scaleX = BowgunShot_GlowFirstScales[flicker][0];
                scaleY = BowgunShot_GlowFirstScales[flicker][1];
                scaleZ = BowgunShot_GlowFirstScales[flicker][2];
            } else {
                meshSet = &commonModel->matSets[66];
                if (buf->stage == 1 && buf->glowFlashTimer <= 5) {
                    flashScale = BowgunShot_GlowFlashScales[buf->glowFlashTimer];
                    scaleX = flashScale[0];
                    scaleY = flashScale[1];
                    scaleZ = flashScale[2];
                } else {
                    scaleX = BowgunShot_GlowSecondScales[flicker][0];
                    scaleY = BowgunShot_GlowSecondScales[flicker][1];
                    scaleZ = BowgunShot_GlowSecondScales[flicker][2];
                }
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            }
            Mtx_Scale(mtx, scaleX, scaleY, scaleZ);
            Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if (meshSet != NULL && meshSet->id != -1) {
                Gs_SetState(RS_ALPHA_REF, 0);
                Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                Gs_SetState(RS_COLOR2, -65);
                Gs_SetLightList(texMats, meshSet);
                Draw_ApplyStates(meshSet->flags);
                Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
                Model_Draw(meshSet->id, 0);
            }
        }
        Draw_SetDefaultState();
    }
}

HitShape BowgunShot_HitShapes_0[2] = {
#include "game/0089B4B0.inc"
};

HitShape BowgunShot_HitShapes_1[2] = {
#include "game/0089B500.inc"
};

HitShape BowgunShot_HitShapes_2[2] = {
#include "game/0089B550.inc"
};

HitShape BowgunShot_HitShapes_3[2] = {
#include "game/0089B5A0.inc"
};

HitShape BowgunShot_HitShapes_4[2] = {
#include "game/0089B5F0.inc"
};

HitShape BowgunShot_HitShapes_5[2] = {
#include "game/0089B640.inc"
};

HitShape BowgunShot_HitShapes_6[2] = {
#include "game/0089B690.inc"
};

HitShape *BowgunShot_HitShapes[8] = {
    BowgunShot_HitShapes_0, BowgunShot_HitShapes_1, BowgunShot_HitShapes_2, BowgunShot_HitShapes_3,
    BowgunShot_HitShapes_4, BowgunShot_HitShapes_5, BowgunShot_HitShapes_6, 0,
};

AttackRow BowgunShot_Rows[35] = {
#include "game/0089B700.inc"
};

void *BowgunShot_RowsPtr = (void *)BowgunShot_Rows;

BowgunShotParam gBowgunShotParams[33] = {
#include "game/0089BA50.inc"
};

s32 BowgunShot_PlayerFlags[34] = {
#include "game/0089BF00.inc"
};

/* The file's data. */
s16 BowgunShot_StageFrameTbls_0[6] = {
#include "game/0089BF88.inc"
};
s16 BowgunShot_StageFrameTbls_1[6] = {
#include "game/0089BF98.inc"
};
s16 BowgunShot_StageFrameTbls_2[6] = {
#include "game/0089BFA8.inc"
};
s16 BowgunShot_StageFrameTbls_3[6] = {
#include "game/0089BFB8.inc"
};
s16 BowgunShot_StageFrameTbls_4[6] = {
#include "game/0089BFC8.inc"
};
s16 BowgunShot_StageFrameTbls_5[6] = {
#include "game/0089BFD8.inc"
};
s16 BowgunShot_StageFrameTbls_6[4] = {
#include "game/0089BFE8.inc"
};
s16 BowgunShot_StageFrameTbls_7[8] = {
#include "game/0089BFF0.inc"
};
s16 BowgunShot_StageFrameTbls_8[8] = {
#include "game/0089C000.inc"
};
s16 BowgunShot_StageFrameTbls_9[8] = {
#include "game/0089C010.inc"
};
s16 BowgunShot_StageFrameTbls_10[4] = {
#include "game/0089C020.inc"
};
s16 BowgunShot_StageFrameTbls_11[4] = {
#include "game/0089C028.inc"
};
s16 BowgunShot_StageFrameTbls_12[4] = {
#include "game/0089C030.inc"
};
s16 BowgunShot_StageFrameTbls_13[4] = {
#include "game/0089C038.inc"
};
s16 BowgunShot_StageFrameTbls_14[4] = {
#include "game/0089C040.inc"
};
s16 BowgunShot_StageFrameTbls_15[4] = {
#include "game/0089C048.inc"
};
s16 BowgunShot_StageFrameTbls_16[4] = {
#include "game/0089C050.inc"
};
s16 BowgunShot_StageFrameTbls_17[4] = {
#include "game/0089C058.inc"
};
s16 *BowgunShot_StageFrameTbls[18] = {
    BowgunShot_StageFrameTbls_0, BowgunShot_StageFrameTbls_1, BowgunShot_StageFrameTbls_2, BowgunShot_StageFrameTbls_3,
    BowgunShot_StageFrameTbls_4, BowgunShot_StageFrameTbls_5, BowgunShot_StageFrameTbls_6, BowgunShot_StageFrameTbls_7,
    BowgunShot_StageFrameTbls_8, BowgunShot_StageFrameTbls_9, BowgunShot_StageFrameTbls_10, BowgunShot_StageFrameTbls_11,
    BowgunShot_StageFrameTbls_12, BowgunShot_StageFrameTbls_13, BowgunShot_StageFrameTbls_14, BowgunShot_StageFrameTbls_15,
    BowgunShot_StageFrameTbls_16, BowgunShot_StageFrameTbls_17,
};
u16 BowgunShot_StreakFrames[4] = {
#include "game/0089C0A8.inc"
};
s32 BowgunShot_ScatterParams[4] = {
#include "game/0089C0B0.inc"
};
s32 BowgunShot_Colors[4] = {
#include "game/0089C0C0.inc"
};
s16 BowgunShot_StageHitFxOdd[8] = {
#include "game/0089C0D0.inc"
};
s16 BowgunShot_StageHitFxEven[8] = {
#include "game/0089C0E0.inc"
};
f32 BowgunShot_StagePowerScales[6] = {
#include "game/0089C0F0.inc"
};
s16 BowgunShot_StageHitSe[6] = {
#include "game/0089C108.inc"
};
f32 BowgunShot_GlowFirstScales[2][3] = {
#include "game/0089C120.inc"
};
f32 BowgunShot_GlowSecondScales[2][3] = {
#include "game/0089C140.inc"
};
f32 BowgunShot_GlowFlashScales[6][3] = {
#include "game/0089C160.inc"
};
