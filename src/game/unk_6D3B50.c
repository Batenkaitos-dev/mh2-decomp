/*
 * Game overlay code at 0x6D3B50-0x6E0650, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "capsule.h"

/* prototypes */
void StageObj_K12_CreateAtTimed(f32 *pos, s32 kind, u16 a, u16 b);
s32 Side_Match(u8 a, u8 b, u8 none);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
extern s16 Eff22_TimeLimit[];
extern f32 Eff22_PlayerPtsA[][3];
extern u16 Eff22_PlayerAngsA[];
extern f32 Eff22_PlayerPtsB[][3];
extern u16 Eff22_PlayerAngsB[];
extern f32 Eff22_PlayerPtsC[][3];
extern u16 Eff22_PlayerAngsC[];
void Eff22_PlaySound(ObjAttach *o, s16 n);
extern void *Eff22_MotionRef;
extern HitShape *Eff22_Vals088[];
extern f32 Eff22_Val048ByType[][3];
extern f32 Eff22_Val058ByType[];
extern f32 Eff22_Add048B[];
extern f32 Eff22_Add048A[];
void Eff22_Draw(DrawSlot *d);
extern f32 Eff22_SpotPosA[][3];
extern u16 Eff22_SpotAngA[];
extern f32 Eff22_SpotPosB[][3];
extern u16 Eff22_SpotAngB[];
extern f32 Eff22_SpotPosC[][3];
extern u16 Eff22_SpotAngC[];
f32 Math_Sin(f32 x);
void Eff12_DrawModel(ModelMatSet *set, f32 *m);
void Eff12_PlaySound(ObjAttach *o, s16 kind);
void StageObj_K14_CreateAtYaw(f32 *pos, s32 kind, u16 r, f32 scale);
/* unit's own declaration: with the definition's void (f32 *pos, s32 v, s16 k, f32 scale) (game/unk_874B90.c), Eff12_Run: 1 of 327 words differ */
void StageObj_K11_CreateAtScaled(f32 *pos, u16 r, s32 kind, f32 scale);
f32 func_364B20(u8 type, f32 dist);
void StageObj_K0F_Create(f32 *pos, s32 kind, u16 ang, void *owner, f32 scale);
void Effect02_SpawnRandomAngle(u16 a, u16 b, u8 type, f32 *pos);
void Mtx_Identity(f32 *m);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
extern u8 Eff10_ScaleKeysA[];
extern u8 Eff10_ScaleKeysB[];
extern u8 Eff10_Vec3Keys[];
extern u8 Eff10_ColorKeysA[];
extern u8 Eff10_ColorKeysB[];
extern s16 Eff10_Models[];
u16 Area_DayId(u16 id);
extern s16 *EmBlangonga_State3Tabs[];
extern s16 *EmBlangonga_State3TabByZ[];
extern f32 EmBlangonga_State3PtsA[][3];
extern f32 EmBlangonga_State3PtsB[][3];
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *obj, s32 a, f32 from, f32 to);
s32 func_364AE0(ObjAttach *o, u16 x);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
u16 BowgunShot_MulAddShift(u16 arg0, u16 arg1, s32 arg2, s32 arg3);
f32 BowgunShot_BitsToSignedFrac(u16 value, u16 bits);
f32 Math_Sqrtf(f32 x);
void Vec_Add(f32 *, f32 *, f32 *);
void Mtx_Copy(f32 *dst, f32 *src);
void BowgunShot_ApplyKind(ObjAttach *o);
f32 BowgunShot_BitsToFrac(u16 arg0, s32 arg1);
void BowgunShot_ApplyKind2(ObjAttach *o);
void BowgunShot_Draw(DrawSlot *d);
void Vec_RotateSelf(f32 *v, f32 *m);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateX(f32 *v, f32 angle);
extern void *BowgunShot_MotionRef;
extern HitShape *BowgunShot_Vals088[];
extern f32 BowgunShot_KindTimes[][6];
extern f32 BowgunShot_KindScaleA[];
extern f32 BowgunShot_KindScaleB[];
extern f32 BowgunShot_KindBase[];
extern u8 BowgunShot_LevelScale[];
u8 Eff10_RunTrap(ObjAttach *o);
void StageObj_K29_CreateAtDefault(f32 *src, u8 k, f32 scale);
void Eff10_FollowBone(ObjAttach *o);
void Eff10_Free(ObjAttach *o);
void Eff10_InitTrapParts(ObjAttach *o);
extern s16 Eff10_Motions[];
extern void *Eff10_MotionDesc;
extern HitShape *Eff10_Vals08C[];
void Eff10_DrawMain(DrawSlot *d);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Obj_InCurrentArea(void *obj);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Eff28_Update(ObjAttach *o);
s32 EmCephadrome_EffEm14State(Em *em);
s32 EmKushala_EffEm14State(Em *em);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Eff09_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
void StageObj_K0C_Create(f32 *pos, s16 v, s32 k, u8 c);
void Eff09_CreateOwned(Player *owner, f32 *pos, s32 kind);
extern s16 BowgunShot_Kind2Val06F[];
extern f32 BowgunShot_Kind2Scale[];
extern s16 BowgunShot_Kind2Val06B[];
void Em_BonePos(void *obj, s16 bone, f32 *out);
u16 Rand_Next(s32 stream);
void Eff10_Draw(DrawSlot *d);
void Eff12_Update(ObjAttach *o);
s32 Player_IsLocal(void *obj);
void Vec_RotateY(f32 *v, f32 angle);
void Eff28_Draw(DrawSlot *d);
extern HitShape *Eff28_Val088;
void Eff10_Run(ObjAttach *o);
void EffGravios19_Update(ObjAttach *o);
void EffBlangonga27_Update(ObjAttach *o);
void Eff22_Update(ObjAttach *o);
void EffHermitaur26_Update(ObjAttach *o);
s32 Eff22_FindSpot(f32 *from, f32 *pos, u16 *ang);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
struct MoveObj;
void MoveObj_StartMotionEntry(struct MoveObj *, s32 a, void *b);
void Eff12_Draw(DrawSlot *d);
extern void *Eff12_MotionRef;
extern HitShape *Eff12_Val088;
extern s16 *BowgunShot_EventTbls[];
EmBone *Em_Bone(void *obj, s16 bone);
void EffEm14_Draw(DrawSlot *d);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void EffEm14_Update(ObjAttach *o);
void EffEm14_Follow(ObjAttach *o, Em *em);
void EffEm14_InitTimers(ObjAttach *o);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void BowgunShot_Update(ObjAttach *o);
/* The bowgun shot's block of the object buffer (BowgunShot_Create; ObjAttach.buf). */
typedef struct {
    u8 bounced;         /* 0x00: 1 once BowgunShot_BounceStep ran (BowgunShot_Draw then stops turning it along its velocity) */
    u8 event;           /* 0x01: the next entry of the kind's event table (BowgunShot_StepEvents), 0xFF: past the last */
    u8 pad02[2];
    f32 power;          /* 0x04: the object's power at set up (BowgunShot_ApplyKind2 scales it) */
    f32 attackFactor;   /* 0x08: the player's attackFactor */
    u8 ammoType;        /* 0x0C: the player's ammoType (BowgunShot_SetKind) */
    u8 charge;          /* 0x0D: the player's chargeLevel & 3: the row of BowgunShot_KindTimes and the scales */
    u8 doneFrames;      /* 0x0E: frames counted once event is 0xFF */
    u8 skills;          /* 0x0F: 1 skill 0x47 (element x1.125), 2 skill 0xBA (range x2), 4 skill 0xB9 (range x0.5) */
    u16 elementFlags;   /* 0x10: the weapon's element bits (Player_WeaponElementFlags) */
    u8 elementLevel;    /* 0x12: the weapon's element value */
    u8 crit;            /* 0x13: 1 a critical distance (x1.25), 2 too close or far (x0.5) */
    s16 shot;           /* 0x14: the shot's level: the index into BowgunShot_LevelScale */
} BowgunShotBuf;

void BowgunShot_InitAttack(BowgunShotBuf *b, Player *pl);
void BowgunShot_SkillFlags(BowgunShotBuf *b, Player *pl);
extern s16 *BowgunShot_ShotParams[];
extern u16 BowgunShot_SpreadScale[];
extern s16 BowgunShot_SpreadDirs[];
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
s16 Player_GetWeaponStat(Player *p, u8 kind);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
extern void *Eff28_MotionRef;
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
extern s16 EffEm14_Index[];
void EffEm11_Draw(ObjAttach *o);
extern HitShape *EffEm11_Vals088[];
extern void *EffEm11_MotionRef;
extern void *EffHermitaur26_MotionRef;
void EffEm14_RunType0(ObjAttach *o);
void EffEm14_RunType2(ObjAttach *o);
void EffEm14_RunType4(ObjAttach *o);
void FlashObj_Remove(EmObj *obj);
s32 EmBlangonga_IsSmallKind(Em *em);
extern HitShape *EffBlangonga27_Vals088[];
extern void *EffBlangonga27_MotionRef;
u8 BowgunShot_BounceStep(ObjAttach *o);
void BowgunShot_Move(ObjAttach *o);
void BowgunShot_Fall(ObjAttach *o);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern HitShape *EffHermitaur26_Vals088[];
extern HitShape *EffKutku17_Vals088[];
extern void *EffKutku17_MotionRef;
s32 FlashObj_Add(EmObj *obj);
s32 Model_Draw(u32 h, s32 flags);
void ObjAttach_Free(ObjAttach *p);
s32 Player_HasSkill(Player *p, u8 skill);
void EffEm11_Run(ObjAttach *o);
void EffEm11_End(ObjAttach *o);
void EffEm11_Free(ObjAttach *o);
void Eff12_End(ObjAttach *o);
void Eff12_Free(ObjAttach *o);
void EffVelociprey13_Setup(ObjAttach *o);
void EffVelociprey13_Run(ObjAttach *o);
void EffVelociprey13_End(ObjAttach *o);
void EffVelociprey13_Free(ObjAttach *o);
void EffEm14_End(ObjAttach *o);
void EffEm14_Free(ObjAttach *o);
void EffDrome15_Setup(ObjAttach *o);
void EffDrome15_Run(ObjAttach *o);
void EffDrome15_End(ObjAttach *o);
void EffDrome15_Free(ObjAttach *o);
void EffKhezu16_Setup(ObjAttach *o);
void EffKhezu16_Run(ObjAttach *o);
void EffKhezu16_End(ObjAttach *o);
void EffKhezu16_Free(ObjAttach *o);
void EffKutku17_End(ObjAttach *o);
void EffKutku17_Free(ObjAttach *o);
void EffDiablos18_Setup(ObjAttach *);
void EffDiablos18_Run(ObjAttach *o);
void EffDiablos18_End(ObjAttach *o);
void EffDiablos18_Free(ObjAttach *o);
void EffGravios19_Setup(ObjAttach *o);
void EffGravios19_End(ObjAttach *o);
void EffGravios19_Free(ObjAttach *o);
void EffPlesioth20_Setup(ObjAttach *o);
void EffPlesioth20_Run(ObjAttach *o);
void EffPlesioth20_End(ObjAttach *o);
void EffPlesioth20_Free(ObjAttach *o);
void EffCephadrome21_Setup(ObjAttach *o);
void EffCephadrome21_Run(ObjAttach *o);
void EffCephadrome21_End(ObjAttach *o);
void EffCephadrome21_Free(ObjAttach *o);
void Eff22_Hit(ObjAttach *o);
void Eff22_End(ObjAttach *o);
void Eff22_Free(ObjAttach *o);
void EffCephadrome23_Setup(ObjAttach *o);
void EffCephadrome23_Run(ObjAttach *o);
void EffCephadrome23_End(ObjAttach *o);
void EffCephadrome23_Free(ObjAttach *o);
void BowgunShot_End(ObjAttach *o);
void BowgunShot_Free(ObjAttach *o);
void EffGaruga25_Setup(ObjAttach *o);
void EffGaruga25_Run(ObjAttach *o);
void EffGaruga25_End(ObjAttach *o);
void EffGaruga25_Free(ObjAttach *o);
void EffHermitaur26_End(ObjAttach *o);
void EffHermitaur26_Free(ObjAttach *o);
void EffBlangonga27_End(ObjAttach *o);
void EffBlangonga27_Free(ObjAttach *o);
void Eff28_Run(ObjAttach *o);
void Eff28_End(ObjAttach *o);
void Eff28_Free(ObjAttach *o);
s32 Gs_SetState(u8 reg, u32 v);
void Vec_Copy(f32 *out, f32 *in);
f32 AreaGrid2_GetFloorYFiltered(f32 *);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void DrawSlot_Free(s32);
void DrawSlotB_Free(s32);
void MoveObj_AddVelocity(struct MoveObj *);
void MoveObj_Integrate(struct MoveObj *);
extern s16 gBowgunClipShots[][4];
void BowgunShot_Fire(Player *pl, s16 shot);
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_SetAEverywhere(s32);
extern f32 gCamPos;
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Eff22_CamShake(ObjAttach *o);
void Effect02_SpawnScaled(s16 ang, u8 type, u8 sub, f32 *pos, f32 scale);
void Effect0D_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
u8 Player_InCurrentArea(void *obj);
ObjAttach *ObjAttach_Alloc(s32 n);
void EffDrome15_Update(ObjAttach *o);
void EffKhezu16_Update(ObjAttach *o);
void EffKutku17_Update(ObjAttach *o);
void EffDiablos18_Update(ObjAttach *o);
void EffPlesioth20_Update(ObjAttach *o);
void EffCephadrome21_Update(ObjAttach *o);
void EffCephadrome23_Update(ObjAttach *o);
void EffGaruga25_Update(ObjAttach *o);
f32 Math_Absf(f32 x);
s32 Player_TestSetFlags(void *, s32 mask);
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern HitShape *EffDiablos18_Vals088[];
extern void *EffDiablos18_MotionRef;
extern HitShape *EffPlesioth20_Vals088[];
extern void *EffPlesioth20_MotionRef;
extern HitShape *EffCephadrome21_Vals088[];
extern void *EffCephadrome21_MotionRef;
extern HitShape *EffCephadrome23_Vals088[];
extern void *EffCephadrome23_MotionRef;
extern HitShape *EffGaruga25_Vals088[];
extern void *EffGaruga25_MotionRef;
extern HitShape *EffVelociprey13_Vals088[];
extern void *EffVelociprey13_MotionRef;
extern HitShape *EffDrome15_Vals088[];
extern void *EffDrome15_MotionRef;
extern HitShape *EffKhezu16_Vals088[];
extern void *EffKhezu16_MotionRef;
extern HitShape *EffGravios19_Vals088[];
extern void *EffGravios19_MotionRef;
void EffEm11_Create(Em *em, s8 kind);
void EffVelociprey13_Create(Em *em, s8 kind);
void EffEm11_Update(ObjAttach *o);
void EffVelociprey13_Update(ObjAttach *o);
void EffKutku17_Setup(ObjAttach *o);
void EffHermitaur26_Run(ObjAttach *o);
/* BowgunShot_Kinds: 0x20 bytes per kind of the effect updated by BowgunShot_Update */
typedef struct { s16 unk0; s16 unk2; f32 a; f32 unk8; f32 b; f32 unk10; f32 unk14; f32 unk18; s16 c; u8 type; u8 count; } Rec8A5780;
extern Rec8A5780 BowgunShot_Kinds[];
void BowgunShot_Run(ObjAttach *o);
void BowgunShot_Hit(ObjAttach *o);
void EffBlangonga27_Setup(ObjAttach *o);
void EffKutku17_Run(ObjAttach *o);
void EffEm14_Run(ObjAttach *o);
void EffEm11_Setup(ObjAttach *o);
void EffHermitaur26_Setup(ObjAttach *o);
void Eff28_Hit(ObjAttach *o);
void Player_WeaponElementFlags(void *pl, u16 *flags, s8 *level);
void BowgunShot_Fire(Player *pl, s16 shot);
void EffEm14_Setup(ObjAttach *o);
void Eff12_Setup(ObjAttach *o);
void Eff28_Setup(ObjAttach *o);
void EffGravios19_Run(ObjAttach *o);
void BowgunShot_Setup(ObjAttach *o);
void BowgunShot_State5(ObjAttach *o);
void EffBlangonga27_Run(ObjAttach *o);
void Eff12_Run(ObjAttach *o);
void Eff22_Setup(ObjAttach *o);
void Eff22_Run(ObjAttach *o);
s8 BowgunShot_StepEvents(ObjAttach *o);
/* end of prototypes */

/* A part of the trap effect Eff10: three from Eff10Buf.parts, a DrawSlotB slot each (Eff10_Draw). */
typedef struct {
    s16 drawSlot;       /* 0x00: its DrawSlotB index */
    s16 kind;           /* 0x02: its index 0..2, the part's kind (Eff10_RunTrap, Eff10_Draw switch on it) */
    DrawSlot *slot;     /* 0x04: DrawSlotB_Get(drawSlot), NULL: none */
    s16 timer;          /* 0x08: frames, from a negative wait; drawn while above 0 */
    s16 frame;          /* 0x0A: the colour key frame (up to 9) */
    u16 angle;          /* 0x0C: turned by 0x2AAB a frame */
    u8 pad0E[2];
} Eff10Part;

/* The trap effect Eff10's block of the object buffer. */
typedef struct {
    u8 phase;           /* 0x00: Eff10_RunTrap switches on it */
    u8 unk01;           /* 0x01 */
    u8 unk02;           /* 0x02 */
    u8 unk03;           /* 0x03 */
    Em *em;             /* 0x04: the monster whose bone it follows (Eff10_FollowBone) */
    f32 ofs[3];         /* 0x08: its offset in the bone's frame */
    s16 bone;           /* 0x14 */
    u8 pad16[2];
    Eff10Part parts[3]; /* 0x18 */
} Eff10Buf;

void Eff10_InitTrapParts(ObjAttach *o)
{
    Eff10Buf *buf = (Eff10Buf *) o->buf;
    Eff10Part *p;
    s16 i;

    buf->phase = 0;
    buf->unk01 = 0;
    buf->unk02 = 0;
    buf->unk03 = 0;
    p = buf->parts;
    for (i = 0; i < 3; i++, p++) {
        p->kind = i;
        p->timer = 0;
        p->frame = 0;
        p->drawSlot = DrawSlotB_Alloc();
        switch (p->kind) {
        case 0:
        case 1:
        default:
            p->angle = 0;
            break;
        case 2:
            p->angle = Rand_Next(1);
            break;
        }
        if (p->drawSlot != -1) {
            p->slot = DrawSlotB_Get(p->drawSlot);
            p->slot->owner = o;
            p->slot->part = i;
            p->slot->draw = Eff10_Draw;
            Vec_Copy(p->slot->pos, o->pos);
            switch (p->kind) {
            case 1:
                p->slot->pos[1] += 5.0f;
                break;
            }
        } else {
            p->slot = NULL;
        }
    }
}

void Eff10_Setup(ObjAttach *o)
{
    u16 v;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->unk008 = 0xFF;
    o->hitPhase = 0;
    o->unk01E = 0;
    o->targetEm = 0;
    o->targetPart = 0;
    if (Eff10_Motions[o->type] != -1) {
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, Eff10_Motions[o->type], &Eff10_MotionDesc);
    }
    switch (o->type) {
    case 0:
    case 1:
    case 6:
        o->hitList = 0;
        o->hurtList = Eff10_Vals08C[o->type];
        Player_TestSetFlags(o, 0x100);
        break;
    case 2:
    case 3:
    default:
        o->hitList = Eff10_Vals08C[o->type];
        o->hurtList = 0;
        Player_TestSetFlags(o, 0x80);
        break;
    case 4:
        o->hitList = Eff10_Vals08C[o->type];
        o->hurtList = 0;
        Player_TestSetFlags(o, 0x400);
        o->rotY = 0;
        o->visible = 0;
        if (gScene.area == o->area) {
            StageObj_K29_CreateAtDefault(o->pos, 5, 1.0f);
            Sound_PlaySeAtPosChecked(1, 0x89, 0, o->pos, 1, 0);
        }
        Eff10_InitTrapParts(o);
        break;
    case 5:
        o->hitList = Eff10_Vals08C[o->type];
        o->hurtList = 0;
        Player_TestSetFlags(o, 4);
        Eff10_FollowBone(o);
        o->aux = 0;
        switch (o->stateVar0) {
        case 1:
            v = 1.5f * o->power;
            if (v > 0xFF) {
                v = 0xFF;
            }
            o->power = v;
            break;
        }
        break;
    }
    o->ownerMotion = 0;
    o->drawSlot = DrawSlotB_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlotB_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = Eff10_DrawMain;
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    } else {
        Eff10_Free(o);
    }
}

u8 Eff10_RunTrap(ObjAttach *o)
{
    Eff10Buf *buf = (Eff10Buf *) o->buf;
    Eff10Part *p = buf->parts;
    Em *m = o->targetEm;
    Em *e;
    s16 i;
    u8 found = 0;
    f32 d;

    if (o->ownerMotion > 2) {
        o->visible = 1;
    }
    switch (buf->phase) {
    case 0:
        if (o->unk01E != 0) {
            found = 1;
        } else {
            for (e = gEms, i = 0; i < 20; i++, e++) {
                if (e->active != 0 && e->area == o->area && (u32) (e->stateGroup - 5) > 1 && (s8) e->holdTimer2 > 0) {
                    d = Vec_Distance(o->pos, &e->posX);
                    if (d <= func_364B20(e->type, 500.0f)) {
                        o->targetEm = e;
                        m = e;
                        found = 2;
                        break;
                    }
                }
            }
        }
        if (found) {
            o->hitPhase = 0;
            o->aux = 0;
            if (m == NULL) {
                return 1;
            }
            buf->phase++;
            if (o->area == gScene.area) {
                StageObj_K0F_Create(o->pos, 7, o->rotY, 0, 1.0f);
            }
        }
        break;
    case 1:
        if (++o->aux > 60) {
            o->aux = 0;
            return 1;
        }
        if (m == NULL) {
            return 1;
        }
        if (m->active == 0) {
            return 1;
        }
        if ((s8) m->holdTimer2 != 0) {
            buf->phase++;
        }
        break;
    case 2:
        if (m == NULL) {
            return 1;
        }
        if (m->active == 0) {
            return 1;
        }
        if (o->stateVar1 == 2 && (m->stateGroup == 6 || m->stateGroup == 5)) {
            (gScene.placed2)--;
            o->stateVar1 = 0;
        }
        if ((s8) m->holdTimer2 <= 0) {
            return 1;
        }
        break;
    }
    o->rotY += 0x1555;
    for (i = 0; i < 3; i++, p++) {
        switch (p->kind) {
        case 0:
        case 1:
        default:
            if (p->timer >= 2) {
                p->timer = 0;
            }
            if (p->frame >= 9) {
                p->frame = 9;
            }
            break;
        case 2:
            if (p->timer >= 20) {
                if (buf->phase == 2) {
                    p->timer = 0;
                } else {
                    p->timer = -((Rand_Next(1) & 0xF) + 30);
                }
            }
            if (p->timer == 0 && o->area == gScene.area) {
                Sound_PlaySeAtPosChecked(1, 0x8A, 0, o->pos, 1, 0);
            }
            p->angle += 0x2AAB;
            break;
        }
        (p->timer)++;
        (p->frame)++;
        if (p->slot != NULL) {
            switch (p->kind) {
            case 2:
                OT_AddByDepth(gDrawOt32, p->slot, 0x20U, 0);
                break;
            default:
                OT_AddByDepth(gDrawOtSorted, p->slot, 0x40U, 0);
                break;
            }
        }
    }
    return 0;
}

void Eff10_FollowBone(ObjAttach *o)
{
    Eff10Buf *buf = (Eff10Buf *) o->buf;
    f32 v[3];
    f32 *m;

    m = (f32 *)Em_Bone(buf->em, buf->bone);
    Vec_Rotate(v, buf->ofs, m);
    o->pos[0] = m[12] + v[0];
    o->pos[1] = 50.0f + (m[13] + v[1]);
    o->pos[2] = m[14] + v[2];
}

void Eff10_Run(ObjAttach *o)
{
    if (++o->ownerMotion > 9000) {
        o->hitPhase = 0;
        o->state = 2;
        return;
    }
    switch (o->type) {
    case 0:
    case 1:
    case 6:
        if (o->wasHit != 0) {
            o->state = 2;
            if (o->type == 0) {
                if (gScene.area == o->area) {
                    StageObj_K0E_CreateScaled(o->pos, 0, 2.0f);
                }
                Eff09_CreateAt(o->pos, 2, o->area, o->stateVar0);
            } else if (o->type == 1) {
                if (gScene.area == o->area) {
                    StageObj_K0E_CreateScaled(o->pos, 0, 3.0f);
                }
                Eff09_CreateAt(o->pos, 0x16, o->area, o->stateVar0);
            } else {
                if (gScene.area == o->area) {
                    StageObj_K29_CreateAtDefault(o->pos, 0xD, 1.0f);
                    StageObj_K29_CreateAtDefault(o->pos, 0xE, 1.0f);
                    Sound_PlaySeAtPosChecked(1, 0x93, 0, o->pos, 1, 0);
                }
                Eff09_CreateAt(o->pos, 0x21, o->area, 0);
            }
        }
        break;
    case 2:
    case 3:
    default:
        o->hitFrames = 99;
        break;
    case 4:
        if (Eff10_RunTrap(o) == 1) {
            o->hitPhase = 0;
            o->state = 2;
            return;
        }
        o->hitFrames = 99;
        break;
    case 5:
        if (o->ownerMotion <= 224) {
            Eff10_FollowBone(o);
            o->hitDelay = 99;
        } else if (o->aux == 0) {
            o->hitDelay = 5;
            if (o->area == gScene.area) {
                Sound_PlaySeAtPosChecked(1, 0x4F, 0, o->pos, 15, 0);
                StageObj_K0E_CreateScaled(o->pos, 0, 2.0f);
            }
            o->aux++;
            o->visible = 0;
        }
        if (o->hitPhase == 0) {
            o->state = 2;
            return;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff10_Hit(ObjAttach *o)
{
    switch (o->type) {
    case 0:
    case 1:
    case 3:
    case 6:
    default:
        o->state = 2;
        break;
    case 2:
        o->state = 2;
        if (o->area == gScene.area) {
            StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
        }
        break;
    case 4:
    case 5:
        Eff10_Run(o);
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff10_EndBurst(ObjAttach *o)
{
    s16 i;
    Eff10Part *p;

    p = ((Eff10Buf *) o->buf)->parts;
    if (o->area == gScene.area) {
        StageObj_K0E_CreateScaled(o->pos, 0, 0.5f);
        Sound_PlaySeAtPosChecked(1, 0x4D, 0, o->pos, 1, 0);
    }
    for (i = 0; i < 3; i++) {
        if (*(s32 *)&p->slot != 0) {
            DrawSlotB_Free(p->drawSlot);
        }
        p++;
    }
}

void Eff10_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlotB_Free(o->drawSlot);
    }
    switch (o->type) {
    case 4:
        Eff10_EndBurst(o);
        break;
    }
}

void Eff10_Free(ObjAttach *o)
{
    switch (o->stateVar1) {
    case 1:
        (gScene.placed1)--;
        break;
    case 2:
        (gScene.placed2)--;
        break;
    }
    ObjAttach_Free(o);
}

void Eff10_DrawModel(ModelMatSet *mdl, f32 *m, ModelMat *tex)
{
    Gs_SetState(0x1A, (s32) m);
    if (mdl != NULL && mdl->id != -1) {
        Gs_SetLightList(tex, mdl);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
}

void Eff10_DrawMain(DrawSlot *d)
{
    f32 m[16];
//{
    ObjAttach *o = d->owner;
    ModelMat *tex;
    ModelMatSet *model;
    Model *mdl;
//}

    if (gScene.area == o->area && o->visible != 0) {
        mdl = gCommonModels[0];
        if (mdl != NULL && mdl->active != 0) {
            tex = mdl->mats;
            switch (o->type) {
            case 0:
            case 5:
                Mtx_Scale(m, 3.0f, 3.0f, 3.0f);
                Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[55];
                {
                    s32 i;
                    s32 *ids;
                    ModelMat *t;
                    i = 0;
                    if (i < model->count) {
                        ids = (s32 *) model;
                        do {
                            t = &tex[ids[2]];
                            if (i == 1) {
                                t->alpha = 0.0f;
                            }
                            i++;
                            ids++;
                        } while (i < model->count);
                    }
                }
                break;
            case 1:
                Mtx_Scale(m, 4.5f, 4.5f, 4.5f);
                Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[55];
                {
                    s32 *ids;
                    s32 i;
                    ModelMat *t;
                i = 0;
                    if (i < model->count) {
                        ids = (s32 *) model;
                        do {
                            t = &tex[ids[2]];
                            if (i == 1) {
                                t->alpha = 0.0f;
                            }
                            i++;
                            ids++;
                        } while (i < model->count);
                    }
                }
                break;
            case 2:
            case 3:
                Mtx_Identity(m);
                Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[116];
                break;
            case 4:
                Mtx_Scale(m, 1.0f, 1.0f, 1.0f);
                Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[116];
                break;
            case 6:
                Mtx_Scale(m, 2.1f, 2.1f, 2.1f);
                Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[55];
                {
                    s32 *ids;
                    s32 i;
                    ModelMat *t;
                i = 0;
                    if (i < model->count) {
                        ids = (s32 *) model;
                        do {
                            t = &tex[ids[2]];
                            if (i == 1) {
                                t->alpha = 1.0f;
                            }
                            i++;
                            ids++;
                        } while (i < model->count);
                    }
                }
                break;
            }
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            Eff10_DrawModel(model, m, tex);
        }
    }
}

void Eff10_Draw(DrawSlot *d)
{
    s32 col;
    f32 sc[3];
    f32 rot[3];
    f32 m[16];
    f32 t[16];
    ObjAttach *o = d->owner;
    Eff10Buf *b = (Eff10Buf *) o->buf;
    Eff10Part *p = &b->parts[d->part];
    ModelMat *tex;
    u16 flags = 0;
    u16 order;
    u8 lit = 1;
    Model *set;
    ModelMatSet *mdl;

    if (gScene.area == o->area && o->visible != 0 && p->timer > 0) {
        set = gCommonModels[0];
        if (set != NULL && set->active != 0) {
            tex = set->mats;
            switch (p->kind) {
            case 0:
                Key_LerpVec2((f32 *) Eff10_ScaleKeysA, sc, p->timer);
                Color_EvalKeys((s32 *) Eff10_ColorKeysA, p->frame, (u32 *) &col);
                rot[0] = -1.5707964f;
                order = 1;
                flags |= 2;
                break;
            case 1:
                Key_LerpVec2((f32 *) Eff10_ScaleKeysB, sc, p->timer);
                Color_EvalKeys((s32 *) Eff10_ColorKeysB, p->frame, (u32 *) &col);
                rot[0] = -1.5707964f;
                order = 1;
                flags |= 2;
                break;
            case 2:
                Mtx_Translation(t, 0.0f, 0.375f * (p->timer / 20.0f), 0.0f);
                Gs_SetState(0x19, (s32) t);
                Key_LerpVec3((f32 *) Eff10_Vec3Keys, sc, p->timer);
                if (b->phase == 2) {
                    sc[1] *= 3.0f;
                }
                col = -1;
                rot[1] = (6.2831855f * p->angle) / 65536.0f;
                order = 2;
                flags |= 4;
                lit = 0;
                break;
            default:
                return;
            }
            Mtx_FromScaleRotTrans(sc, rot, d->pos, order, m);
            mdl = &set->matSets[Eff10_Models[p->kind]];
            if (lit == 0) {
                Gs_SetState(0x67, -1);
                Effect_DrawModelAdd(mdl, m, tex);
            } else {
                Gs_SetState(0x6C, 0);
                Effect_DrawModel(mdl, m, col, flags, tex);
            }
            Gs_SetState(0x6C, 1);
        }
    }
}

void EffEm11_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 11;
            o->type = kind;
            o->update = EffEm11_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffEm11_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffEm11_Setup(o);
        break;
    case 1:
        EffEm11_Run(o);
        break;
    case 2:
        EffEm11_End(o);
        break;
    case 3:
        EffEm11_Run(o);
        break;
    case 4:
        EffEm11_End(o);
        break;
    case 5:
        EffEm11_End(o);
        break;
    case 9:
        EffEm11_End(o);
        break;
    case 7:
        EffEm11_End(o);
        break;
    case 6:
        EffEm11_Free(o);
        
    default:
        break;
    }
}

void EffEm11_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = EffEm11_Draw;
    Player_TestSetFlags(o, 0x20);
    o->wasHit = 0;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffEm11_MotionRef);
    o->hitList = EffEm11_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffEm11_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 4:
    case 10:
        o->hitFrames = 99;
        break;
    }
    if (m->active == 0) {
        o->hitPhase = 0;
        o->state = 2;
        return;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffEm11_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffEm11_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffEm11_Draw(ObjAttach *o)
{
}

void Eff12_Create(Player *pl, s8 kind)
{
    ObjAttach *o;
    f32 v[3];

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 12;
        o->update = Eff12_Update;
        o->type = kind;
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 80.0f;
        Vec_RotateY(v, (6.2831855f * pl->rot[1]) / 65536.0f);
        o->pos[0] = pl->pos[0] + v[0];
        o->pos[1] = pl->pos[1] + v[1];
        o->pos[2] = pl->pos[2] + v[2];
        o->owner = pl;
        o->idx = pl->index;
        o->area = pl->area;
        o->keep = 1;
        if (Player_IsLocal(pl)) {
            (gScene.placed2)++;
            o->stateVar1 = 1;
        }
    }
}

void Eff12_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff12_Setup(o);
        break;
    case 1:
        Eff12_Run(o);
        break;
    case 2:
        Eff12_End(o);
        break;
    case 3:
        Eff12_Run(o);
        break;
    case 4:
        Eff12_Run(o);
        break;
    case 5:
        Eff12_End(o);
        break;
    case 9:
        Eff12_End(o);
        break;
    case 7:
        Eff12_End(o);
        break;
    case 6:
        Eff12_Free(o);
        
    default:
        break;
    }
}

void Eff12_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    o->aux = 0;
    o->isEm = 0;
    Player_TestSetFlags(o, 0);
    MoveObj_StartMotionEntry((struct MoveObj *)o, o->type + 1, &Eff12_MotionRef);
    o->hitList = Eff12_Val088;
    o->hurtList = 0;
    o->drawSlot = DrawSlotB_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlotB_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = Eff12_Draw;
    } else {
        Eff12_Free(o);
    }
}

void Eff12_Run(ObjAttach *o)
{
    Em *m = o->targetEm;
    Em *e;
    s16 i;
    u8 found = 0;
    f32 d;

    o->ownerMotion++;
    switch (o->stateVar0) {
    case 0:
        if (o->ownerMotion < 55) {
            if (o->area == gScene.area) {
                if ((o->ownerMotion & 7) == 0) {
                    Effect02_SpawnRandomAngle(0xC000, 0, 2, o->pos);
                }
                if ((o->ownerMotion & 0xF) == 0) {
                    Eff12_PlaySound(o, 0);
                }
            }
        } else if (o->ownerMotion == 55 && o->area == gScene.area) {
            StageObj_K14_CreateAtYaw(o->pos, 12, Rand_Next(1), 0.8f);
            StageObj_K11_CreateAtScaled(o->pos, Rand_Next(1), 10, 0.4f);
            StageObj_K14_CreateAtYaw(o->pos, 20, Rand_Next(1), 2.0f);
            Eff12_PlaySound(o, 1);
        }
        if (o->ownerMotion >= 60) {
            o->stateVar0++;
            Eff12_PlaySound(o, 2);
        }
        break;
    case 1:
        o->aux++;
        o->hitFrames = 99;
        if (o->ownerMotion > 121) {
            o->stateVar0++;
        }
        break;
    case 2:
        if (o->ownerMotion >= 9121) {
            o->hitPhase = 0;
            o->state = 2;
            return;
        }
        o->hitFrames = 99;
        if (o->unk01E != 0) {
            found = 1;
        } else {
            for (e = gEms, i = 0; i < 20; i++, e++) {
                if (e->active != 0 && e->area == o->area && (u32) (e->stateGroup - 5) > 1 && (s8) e->holdTimer > 0) {
                    d = Vec_Distance(o->pos, &e->posX);
                    if (d <= func_364B20(e->type, 700.0f)) {
                        o->targetEm = e;
                        m = e;
                        found = 2;
                        break;
                    }
                }
            }
        }
        if (found) {
            if (m == NULL) {
                o->step = 0;
            } else {
                if (found == 1) {
                    m->unk8AD = 6;
                }
                o->step = 1;
            }
            o->stateVar0++;
            o->aux = 0;
            o->hitPhase = 0;
        }
        break;
    case 3:
        if (++o->aux > 5) {
            if (o->step == 0) {
                o->stateVar0 = 6;
            } else {
                o->stateVar0++;
            }
            o->aux = 0;
        }
        break;
    case 4:
        if (++o->aux > 60) {
            o->hitPhase = 0;
            o->aux = 0;
            o->stateVar0 = 6;
            break;
        }
        if (m == NULL) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if (m->active == 0) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if ((s8) m->holdTimer != 0) {
            o->hitPhase = 0;
            o->aux = 0;
            o->stateVar0++;
        }
        break;
    case 5:
        if (m == NULL) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if (m->active == 0) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if (o->stateVar1 != 0 && (m->stateGroup == 6 || m->stateGroup == 5)) {
            (gScene.placed2)--;
            o->stateVar1 = 0;
        }
        if ((s8) m->holdTimer <= 0) {
            o->hitPhase = 0;
            o->aux = 0;
            o->stateVar0++;
        }
        break;
    case 6:
        if (++o->aux > 5) {
            o->hitPhase = 0;
            o->state = 2;
            return;
        }
        break;
    }
    o->drawSlotBuf->pos[0] = o->pos[0];
    o->drawSlotBuf->pos[1] = o->pos[1];
    o->drawSlotBuf->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
}

void Eff12_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    DrawSlotB_Free(o->drawSlot);
}

void Eff12_Free(ObjAttach *o)
{
    if (o->stateVar1 != 0) {
        gScene.placed2--;
    }
    ObjAttach_Free(o);
}

void Eff12_DrawModel(ModelMatSet *set, f32 *m)
{
    Gs_SetState(0x1A, (s32) m);
    if (set != NULL && set->id != -1) {
        Draw_ApplyStates(set->flags);
        Model_Draw(set->id, 0);
    }
}

void Eff12_Draw(DrawSlot *d)
{
    f32 m[16];
    f32 m2[16];
    ObjAttach *o = d->owner;
    Model *mdlB = gAreaSubModels[0];
    ModelMat *tex;
    Model *mdl = gCommonModels[0];
    s16 idx;
    f32 ty;
    f32 sx;
    f32 sz;
    f32 sy;
    s32 t;

    if (o->area == gScene.area && mdlB != NULL && mdlB->active != 0) {
        switch (o->stateVar0) {
        case 0:
            break;
        case 1:
        case 2:
            t = o->aux;
            sx = 1.0f;
            sz = sx;
            if (t < 15) {
                ty = sx - 0.3f * (t / 15.0f);
            } else if (t < 30) {
                ty = 0.7f - 0.2f * ((t - 15) / 15.0f);
            } else {
                ty = 0.5f;
            }
            if (t > 61) {
                sy = 0.16f;
            } else if (t > 15) {
                sy = 1.0f - 0.84f * Math_Sin((1.5707964f * (t - 15)) / 46.0f);
            } else {
                sy = 1.0f;
            }
            break;
        case 3:
            t = o->aux;
            sx = 1.0f - t / 5.0f;
            sy = 0.16f * sx;
            ty = 0.5f;
            sz = sx;
            break;
        case 4:
        case 5:
            sx = 0.0f;
            sy = sx;
            sz = sx;
            ty = sx;
            break;
        case 6:
            t = 5 - o->aux;
            sy = 0.0f;
            sz = sy;
            ty = sy;
            sx = 1.0f - t / 5.0f;
            break;
        }
        idx = gAreaAmbModels[o->area][0];
        Gs_SetState(0x67, -1);
        if (o->stateVar0 < 3) {
            tex = mdl->mats;
            Gs_SetState(0x60, 0x80);
            Mtx_Identity(m);
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            Gs_SetLightList(tex, &mdl->matSets[116]);
            Eff12_DrawModel(&mdl->matSets[116], m);
        }
        if (o->stateVar0 != 4 && o->stateVar0 != 5 && o->stateVar0 != 6) {
            Gs_SetState(0x60, 0);
            Mtx_Scale(m, sx, sy, sz);
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            Mtx_Translation(m2, 0.0f, ty, 0.0f);
            Gs_SetState(0x19, (s32) m2);
            Eff12_DrawModel(&mdlB->matSets[idx], m);
            Mtx_Scale(m, sx, sy, sz);
            Mtx_SetTrans(m, o->pos[0], 2.0f + o->pos[1], o->pos[2]);
            Gs_SetState(0x19, (s32) m2);
            Eff12_DrawModel(&mdlB->matSets[idx] + 1, m);
        }
        Gs_SetState(0x60, 0x80);
        if ((u32) (o->stateVar0 - 3) <= 2 || o->stateVar0 == 6) {
            Mtx_Scale(m, 1.0f - sx, 1.0f - sx, 1.0f - sx);
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            Eff12_DrawModel(&mdlB->matSets[idx] + 2, m);
        }
        Draw_SetDefaultState();
    }
}

void Eff12_PlaySound(ObjAttach *o, s16 kind)
{
    if (o->area == gScene.area) {
        switch (kind) {
        case 0:
            Sound_PlaySeAtPosChecked(1, 0x25, 0, o->pos, 1, 0);
            break;
        case 1:
            Sound_PlaySeAtPosChecked(1, 0x26, 0, o->pos, 1, 0);
            break;
        case 2:
            Sound_PlaySeAtPosChecked(1, 0x46, 0, o->pos, 1, 0);
            break;
        }
    }
}

void EffVelociprey13_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 13;
            o->type = kind;
            o->update = EffVelociprey13_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffVelociprey13_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffVelociprey13_Setup(o);
        break;
    case 1:
        EffVelociprey13_Run(o);
        break;
    case 2:
        EffVelociprey13_End(o);
        break;
    case 3:
        EffVelociprey13_Run(o);
        break;
    case 4:
        EffVelociprey13_End(o);
        break;
    case 5:
        EffVelociprey13_End(o);
        break;
    case 9:
        EffVelociprey13_End(o);
        break;
    case 7:
        EffVelociprey13_End(o);
        break;
    case 6:
        EffVelociprey13_Free(o);
        
    default:
        break;
    }
}

void EffVelociprey13_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;
    s32 b;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffVelociprey13_MotionRef);
    o->hitList = EffVelociprey13_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            a = Math_Absf(m->motionBlend[1] % 100);
            b = Math_Absf(m->motionFrame[1]);
        } else {
            a = Math_Absf(m->motionBlend[0] % 100);
            b = Math_Absf(m->motionFrame[0]);
        }
        o->hitDelay += a - b - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffVelociprey13_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffVelociprey13_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffVelociprey13_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffEm14_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 14;
        o->type = kind;
        o->update = EffEm14_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->owner = em;
        o->yaw = em->rotY;
        o->area = em->area;
        EffEm14_Follow(o, em);
        o->keep = 0;
        em->hitPending = 0;
        em->attackHit800 = 0;
        o->side = *(u8 *) &em->side;
    }
}

void EffEm14_Follow(ObjAttach *o, Em *em)
{
    f32 a[3];
    f32 b[3];

    if (o->type == 4) {
        Em_BonePos(em, 6, a);
        o->pos[0] = a[0];
        o->pos[1] = a[1];
        o->pos[2] = a[2];
    } else {
        Em_BonePos(em, 6, a);
        Em_BonePos(em, 9, b);
        o->pos[0] = (a[0] + b[0]) / 2.0f;
        switch (o->type) {
        case 1:
            o->pos[1] = 20.0f * o->velX + (a[1] + b[1]) / 2.0f;
            break;
        default:
            o->pos[1] = 20.0f + (a[1] + b[1]) / 2.0f;
            break;
        }
        o->pos[2] = (a[2] + b[2]) / 2.0f;
    }
    o->rotX = em->rotX;
    o->rotY = em->rotY;
    o->rotZ = em->rotZ;
}

void EffEm14_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffEm14_Setup(o);
        break;
    case 1:
        EffEm14_Run(o);
        break;
    case 2:
        EffEm14_End(o);
        break;
    case 3:
        EffEm14_End(o);
        break;
    case 4:
        EffEm14_End(o);
        break;
    case 5:
        EffEm14_End(o);
        break;
    case 9:
        EffEm14_End(o);
        break;
    case 7:
        EffEm14_End(o);
        break;
    case 6:
        EffEm14_Free(o);
        
    default:
        break;
    }
}

void EffEm14_InitTimers(ObjAttach *o)
{
    o->hitPhase = 0;
    o->aux = 0x154;
    if (o->type == 4 || o->type == 5) {
        o->ownerMotion = 10;
    } else {
        o->ownerMotion = 30;
    }
    o->velX = 0.0f;
}

void EffEm14_RunType0(ObjAttach *o)
{
    Em *m = o->owner;

    switch (o->step) {
    case 0:
        if (o->type == 1) {
            o->velX += 0.1f;
        } else {
            o->velX += 0.033333335f;
        }
        if (--o->ownerMotion <= 0) {
            o->step++;
            if (o->type == 1) {
                o->velX = 3.0f;
            } else {
                o->velX = 1.0f;
            }
        }
        if (EmCephadrome_EffEm14State(m) != 1) {
            o->visible ^= 1;
            o->state++;
            return;
        }
        break;
    case 1:
        if (o->ownerMotion >= 60) {
            o->step++;
            o->stateVar0 = 15;
            if (o->area == gScene.area) {
                Sound_PlaySeAtPosChecked(1, 0x24, 0, o->pos, 1, 0);
            }
        }
    case 2:
        if (++o->ownerMotion >= 340 || EmCephadrome_EffEm14State(m) != 1) {
            if (o->area == gScene.area) {
                StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
                if (o->type == 1) {
                    Eff09_CreateAt(o->pos, 2, o->area, 0);
                } else {
                    Eff09_CreateAt(o->pos, 0, o->area, 0);
                }
            }
            o->visible ^= 1;
            o->state++;
            return;
        }
        if (o->ownerMotion >= 320) {
            o->visible ^= 1;
            break;
        }
        if (o->stateVar1 == 0) {
            if (!(0.8f * o->aux <= 320 - o->ownerMotion)) {
                o->visible ^= 1;
                if (320 - o->ownerMotion > 2) {
                    o->stateVar1 = 2;
                }
            }
        } else if (--o->stateVar1 == 0) {
            o->visible ^= 1;
            o->aux = 320 - o->ownerMotion;
        }
        if (o->area == gScene.area && o->ownerMotion >= 60) {
            if (o->ownerMotion & 1) {
                Effect02_SpawnRandomAngle(0xC000, 0, 2, o->pos);
            }
            if (--o->stateVar0 <= 0) {
                o->stateVar0 = 15;
                Sound_PlaySeAtPosChecked(1, 0x25, 0, o->pos, 1, 0);
            }
        }
        break;
    }
    EffEm14_Follow(o, m);
}

void EffEm14_RunType2(ObjAttach *o)
{
    Em *m = o->owner;
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 mat[16];
    f32 f;

    switch (o->step) {
    case 0:
        o->velX += 0.033333335f;
        if (--o->ownerMotion <= 0) {
            o->step++;
            o->velX = 1.0f;
        }
        if (EmCephadrome_EffEm14State(m) != 1) {
            o->visible ^= 1;
            o->state++;
        } else {
            EffEm14_Follow(o, m);
        }
        break;
    case 1:
        switch (EmCephadrome_EffEm14State(m)) {
        case 0:
            o->step++;
            if (o->type == 2) {
                o->velZ = 10.0f;
                o->velY = 20.0f;
            } else {
                o->velZ = 15.0f;
                o->velY = 25.0f;
            }
            o->gravity = -1.0f;
            o->ownerMotion = 120;
            goto follow;
        default:
            o->visible ^= 1;
            o->state++;
            break;
        case 1:
        follow:
            EffEm14_Follow(o, m);
            break;
        }
        break;
    case 2:
        ang[0] = 0;
        ang[1] = o->rotY;
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mat);
        Vec_Set(v, 0.0f, 0.0f, o->velZ);
        Vec_Rotate(d, v, mat);
        o->pos[0] += d[0];
        o->pos[2] += d[2];
        o->pos[1] += o->velY;
        o->velY += o->gravity;
        f = AreaGrid2_GetFloorY(o->pos);
        if (--o->ownerMotion <= 0 || o->pos[1] < f) {
            if (o->area == gScene.area) {
                StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
                Eff09_CreateAt(o->pos, 0, o->area, 0);
            }
            o->visible ^= 1;
            o->state++;
            return;
        }
        break;
    }
}

void EffEm14_RunType4(ObjAttach *o)
{
    Em *m = o->owner;
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 mat[16];
    f32 f;

    switch (o->step) {
    case 0:
        o->velX += 0.05f;
        if (--o->ownerMotion <= 0) {
            o->step++;
            o->velX = 1.0f;
        }
        if (EmKushala_EffEm14State(m) != 1) {
            o->visible ^= 1;
            o->state++;
        } else {
            EffEm14_Follow(o, m);
        }
        break;
    case 1:
        switch (EmKushala_EffEm14State(m)) {
        case 0:
            o->step++;
            if (o->type == 5) {
                o->velZ = 20.0f;
                o->velY = 10.0f;
            } else {
                o->velZ = 15.0f;
                o->velY = 25.0f;
            }
            o->gravity = -1.0f;
            o->ownerMotion = 120;
            goto follow;
        default:
            o->visible ^= 1;
            o->state++;
            break;
        case 1:
        follow:
            EffEm14_Follow(o, m);
            break;
        }
        break;
    case 2:
        ang[0] = 0;
        ang[1] = o->rotY;
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mat);
        Vec_Set(v, 0.0f, 0.0f, o->velZ);
        Vec_Rotate(d, v, mat);
        o->pos[0] += d[0];
        o->pos[2] += d[2];
        o->pos[1] += o->velY;
        o->velY += o->gravity;
        f = AreaGrid2_GetFloorY(o->pos);
        if (--o->ownerMotion <= 0 || o->pos[1] < f) {
            if (o->area == gScene.area) {
                if (o->type == 5) {
                    StageObj_K0C_Create(o->pos, 0, 5, 4);
                    Eff09_CreateOwned((Player *)m, o->pos, 0x1A);
                } else {
                    StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
                    Eff09_CreateAt(o->pos, 0, o->area, 0);
                }
            }
            o->visible ^= 1;
            o->state++;
            return;
        }
        break;
    }
}

void EffEm14_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    EffEm14_InitTimers(o);
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = EffEm14_Draw;
        EffEm14_Run(o);
    } else {
        ObjAttach_Free(o);
    }
}

void EffEm14_Run(ObjAttach *o)
{
    switch (o->type) {
    case 0:
    case 1:
    default:
        EffEm14_RunType0(o);
        break;
    case 2:
    case 3:
        EffEm14_RunType2(o);
        break;
    case 4:
    case 5:
        EffEm14_RunType4(o);
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffEm14_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffEm14_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffEm14_Draw(DrawSlot *d)
{
    f32 m[16];
    ObjAttach *o = d->owner;
    Model *set = gCommonModels[0];
    ModelMat *tex;
    ModelMatSet *mdl;
    s16 idx;
    s32 i;
    s32 *ids;
    ModelMat *t;

    if (o->visible != 0 && o->area == gScene.area && set != NULL && set->active != 0) {
        tex = set->mats;
        Mtx_Scale(m, o->velX, o->velX, o->velX);
        Mtx_SetTrans(m, d->pos[0], d->pos[1], d->pos[2]);
        Mtx_RotateXYZ(m, 0.0f, (6.2831855f * o->rotY) / 65536.0f, 0.0f);
        idx = EffEm14_Index[o->type];
        mdl = &set->matSets[idx];
        if (idx == 0x37) {
            i = 0;
            if (i < mdl->count) {
                ids = (s32 *) mdl;
                do {
                    t = &tex[ids[2]];
                    if (i == 1) {
                        t->alpha = 0.0f;
                    }
                    i++;
                    ids++;
                } while (i < mdl->count);
            }
        }
        Gs_SetState(0x1A, (s32) m);
        if (mdl != NULL && mdl->id != -1) {
            Gs_SetLightList(tex, mdl);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void EffDrome15_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 15;
            o->type = kind;
            o->update = EffDrome15_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffDrome15_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffDrome15_Setup(o);
        break;
    case 1:
        EffDrome15_Run(o);
        break;
    case 2:
        EffDrome15_End(o);
        break;
    case 3:
        EffDrome15_Run(o);
        break;
    case 4:
        EffDrome15_End(o);
        break;
    case 5:
        EffDrome15_End(o);
        break;
    case 9:
        EffDrome15_End(o);
        break;
    case 7:
        EffDrome15_End(o);
        break;
    case 6:
        EffDrome15_Free(o);
        
    default:
        break;
    }
}

void EffDrome15_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;
    s32 b;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffDrome15_MotionRef);
    o->hitList = EffDrome15_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            a = Math_Absf(m->motionBlend[1] % 100);
            b = Math_Absf(m->motionFrame[1]);
        } else {
            a = Math_Absf(m->motionBlend[0] % 100);
            b = Math_Absf(m->motionFrame[0]);
        }
        o->hitDelay += a - b - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffDrome15_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffDrome15_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffDrome15_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffKhezu16_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 16;
            o->type = kind;
            o->update = EffKhezu16_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffKhezu16_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffKhezu16_Setup(o);
        break;
    case 1:
        EffKhezu16_Run(o);
        break;
    case 2:
        EffKhezu16_End(o);
        break;
    case 3:
        EffKhezu16_Run(o);
        break;
    case 4:
        EffKhezu16_End(o);
        break;
    case 5:
        EffKhezu16_End(o);
        break;
    case 9:
        EffKhezu16_End(o);
        break;
    case 7:
        EffKhezu16_End(o);
        break;
    case 6:
        EffKhezu16_Free(o);
        
    default:
        break;
    }
}

void EffKhezu16_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;
    s32 b;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffKhezu16_MotionRef);
    o->hitList = EffKhezu16_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            a = Math_Absf(m->motionBlend[1] % 100);
            b = Math_Absf(m->motionFrame[1]);
        } else {
            a = Math_Absf(m->motionBlend[0] % 100);
            b = Math_Absf(m->motionFrame[0]);
        }
        o->hitDelay += a - b - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffKhezu16_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 29:
    case 31:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffKhezu16_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffKhezu16_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffKutku17_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 17;
            o->type = kind;
            o->update = EffKutku17_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffKutku17_Create2(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->kind = 17;
            o->type = kind;
            o->update = EffKutku17_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffKutku17_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffKutku17_Setup(o);
        break;
    case 1:
        EffKutku17_Run(o);
        break;
    case 2:
        EffKutku17_End(o);
        break;
    case 3:
        EffKutku17_Run(o);
        break;
    case 4:
        EffKutku17_End(o);
        break;
    case 5:
        EffKutku17_End(o);
        break;
    case 9:
        EffKutku17_End(o);
        break;
    case 7:
        EffKutku17_End(o);
        break;
    case 6:
        EffKutku17_Free(o);
        
    default:
        break;
    }
}

void EffKutku17_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;
    EmObj *p;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffKutku17_MotionRef);
    o->hitList = EffKutku17_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
    if (o->type == 22) {
        p = (EmObj *) o->buf;
        Vec_Copy(p->pos, &m->posX);
        p->radius = 2000.0f;
        p->goalArg = 0;
        p->timer = 5;
        p->area = o->area;
        FlashObj_Add(p);
    }
}

void EffKutku17_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 19:
    case 23:
    case 28:
        o->hitFrames = 99;
        break;
    }
    if (o->type == 22) {
        o->buf[0x14] = 5;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
        if (o->type == 22) {
            FlashObj_Remove((EmObj *)o->buf);
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffKutku17_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffKutku17_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffDiablos18_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 18;
            o->type = kind;
            o->update = EffDiablos18_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffDiablos18_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffDiablos18_Setup(o);
        break;
    case 1:
        EffDiablos18_Run(o);
        break;
    case 2:
        EffDiablos18_End(o);
        break;
    case 3:
        EffDiablos18_Run(o);
        break;
    case 4:
        EffDiablos18_End(o);
        break;
    case 5:
        EffDiablos18_End(o);
        break;
    case 9:
        EffDiablos18_End(o);
        break;
    case 7:
        EffDiablos18_End(o);
        break;
    case 6:
        EffDiablos18_Free(o);
        
    default:
        break;
    }
}

void EffDiablos18_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffDiablos18_MotionRef);
    o->hitList = EffDiablos18_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffDiablos18_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 14:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffDiablos18_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffDiablos18_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffGravios19_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 19;
            o->type = kind;
            o->update = EffGravios19_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            switch (o->type) {
            case 4:
            case 22:
                *(u16 *) &o->yaw += 0x8000;
                break;
            }
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffGravios19_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffGravios19_Setup(o);
        break;
    case 1:
        EffGravios19_Run(o);
        break;
    case 2:
        EffGravios19_End(o);
        break;
    case 3:
        EffGravios19_Run(o);
        break;
    case 4:
        EffGravios19_End(o);
        break;
    case 5:
        EffGravios19_End(o);
        break;
    case 9:
        EffGravios19_End(o);
        break;
    case 7:
        EffGravios19_End(o);
        break;
    case 6:
        EffGravios19_Free(o);
        
    default:
        break;
    }
}

void EffGravios19_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;
    s32 b;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffGravios19_MotionRef);
    o->hitList = EffGravios19_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            a = Math_Absf(m->motionBlend[1] % 100);
            b = Math_Absf(m->motionFrame[1]);
        } else {
            a = Math_Absf(m->motionBlend[0] % 100);
            b = Math_Absf(m->motionFrame[0]);
        }
        o->hitDelay += a - b - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffGravios19_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 11:
    case 27:
        if (m->motionId[0] == 0x3F9) {
            o->hitFrames = 60;
        }
        break;
    case 12:
    case 14:
        if (m->motionId[0] == 0x457) {
            o->hitFrames = 18;
        }
        break;
    }
    if (o->type == 11 || o->type == 27) {
        if (o->hitPhase == 0 || m->stateGroup == 4 || m->stateGroup == 5) {
            o->hitPhase = 0;
            o->state = 2;
            return;
        }
    } else if (o->type == 12 || o->type == 14) {
        if (o->hitPhase == 0 || m->stateGroup == 4 || m->stateGroup == 5) {
            o->hitPhase = 0;
            o->state = 2;
        }
    } else {
        if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffGravios19_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffGravios19_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffPlesioth20_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 20;
            o->type = kind;
            o->update = EffPlesioth20_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffPlesioth20_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffPlesioth20_Setup(o);
        break;
    case 1:
        EffPlesioth20_Run(o);
        break;
    case 2:
        EffPlesioth20_End(o);
        break;
    case 3:
        EffPlesioth20_Run(o);
        break;
    case 4:
        EffPlesioth20_End(o);
        break;
    case 5:
        EffPlesioth20_End(o);
        break;
    case 9:
        EffPlesioth20_End(o);
        break;
    case 7:
        EffPlesioth20_End(o);
        break;
    case 6:
        EffPlesioth20_Free(o);
        
    default:
        break;
    }
}

void EffPlesioth20_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffPlesioth20_MotionRef);
    o->hitList = EffPlesioth20_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffPlesioth20_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 14:
    case 19:
    case 23:
    case 24:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffPlesioth20_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffPlesioth20_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffCephadrome21_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 21;
            o->type = kind;
            o->update = EffCephadrome21_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffCephadrome21_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffCephadrome21_Setup(o);
        break;
    case 1:
        EffCephadrome21_Run(o);
        break;
    case 2:
        EffCephadrome21_End(o);
        break;
    case 3:
        EffCephadrome21_Run(o);
        break;
    case 4:
        EffCephadrome21_End(o);
        break;
    case 5:
        EffCephadrome21_End(o);
        break;
    case 9:
        EffCephadrome21_End(o);
        break;
    case 7:
        EffCephadrome21_End(o);
        break;
    case 6:
        EffCephadrome21_Free(o);
        
    default:
        break;
    }
}

void EffCephadrome21_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffCephadrome21_MotionRef);
    o->hitList = EffCephadrome21_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffCephadrome21_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 14:
    case 19:
    case 23:
    case 24:
    case 26:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffCephadrome21_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffCephadrome21_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff22_CreateForPlayer(Player *pl, u8 kind)
{
    f32 v[3];
    s16 n;
    ObjAttach *o;
    u8 c = 0;
    f32 (*pts)[3];
    u16 *angs;
    s16 i;
    s8 aim;
    f32 turn;

    switch (kind) {
    case 0:
        n = 2;
        if (Player_HasSkill(pl, 0xC9) == 1) {
            c = 110;
        }
        break;
    case 1:
        n = 1;
        if (Player_HasSkill(pl, 0xC9) == 1) {
            c = 110;
        }
        break;
    default:
        n = 1;
        break;
    }
    for (i = 0; i < n; i++) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 22;
            o->type = kind;
            o->update = Eff22_Update;
            o->idx = pl->index;
            o->isEm = pl->isEm;
            o->owner = pl;
            o->area = pl->area;
            o->stateVar0 = pl->stateRand;
            o->randBits = (pl->stateRand & 0xFF00) >> 8;
            o->stateVar1 = 0;
            o->ownerMotion = 0;
            o->powerPct = c;
            switch (kind) {
            case 0:
                o->pos[0] = pl->pos[0];
                o->pos[1] = pl->pos[1];
                o->pos[2] = pl->pos[2];
                aim = pl->aimPitchPct;
                if (aim >= 0) {
                    turn = 65536.0f;
                    o->rotX = -(u16) (s32) (0.5f + (turn * (0.01f * (45.0f * aim))) / 360.0f);
                } else {
                    turn = 65536.0f;
                    o->rotX = -(u16) (s32) (0.5f + (turn * (0.01f * (35.0f * aim))) / 360.0f);
                }
                o->rotY = pl->rot[1];
                if (i == 0) {
                    v[0] = 36.2f;
                } else {
                    v[0] = -36.2f;
                    o->ownerMotion = -3;
                }
                v[1] = 100.0f;
                v[2] = 75.0f;
                Vec_RotateY(v, (6.2831855f * o->rotY) / turn);
                o->pos[0] += v[0];
                o->pos[1] += v[1];
                o->pos[2] += v[2];
                break;
            case 1:
                switch (Area_DayId(o->area)) {
                case 0xC:
                    pts = Eff22_PlayerPtsA;
                    angs = Eff22_PlayerAngsA;
                    break;
                case 0x19:
                    pts = Eff22_PlayerPtsB;
                    angs = Eff22_PlayerAngsB;
                    break;
                case 0xB0:
                    pts = Eff22_PlayerPtsC;
                    angs = Eff22_PlayerAngsC;
                    break;
                default:
                    ObjAttach_Free(o);
                    return;
                }
                Vec_Copy(o->pos, pts[o->stateVar0]);
                o->rotX = 0xF05C;
                o->rotY = angs[o->stateVar0];
                v[0] = 0.0f;
                v[1] = 50.0f;
                v[2] = 255.0f;
                Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
                Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
                o->pos[0] += v[0];
                o->pos[1] += v[1];
                o->pos[2] += v[2];
                break;
            case 7:
                o->side = pl->side;
                o->ownerMotion = -45;
                break;
            }
            o->yaw = o->rotY;
        }
    }
}

void Eff22_CreateAt(f32 *pos, u8 kind, u8 area, s32 ang)
{
    s16 n;
    s16 i;
    ObjAttach *o;

    switch (kind) {
    case 2:
        n = 4;
        break;
    case 8:
        n = 2;
        break;
    default:
        n = 1;
        break;
    }
    for (i = 0; i < n; i++) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 22;
            o->type = kind;
            o->update = Eff22_Update;
            o->idx = 0;
            o->isEm = 0;
            o->owner = NULL;
            o->yaw = ang;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->area = area;
            o->rotX = 0;
            o->rotY = (u16) ang;
            o->stateVar0 = 0;
            o->stateVar1 = i;
            o->ownerMotion = 0;
        }
    }
}

void Eff22_CreateAtSpot(Em *em, s32 kind, s32 c)
{
    u16 ang;
    f32 pos[3];
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if (Eff22_FindSpot(&em->posX, pos, &ang) != 0) {
            if ((o = ObjAttach_Alloc(0)) != NULL) {
                o->kind = 22;
                o->type = kind;
                o->update = Eff22_Update;
                o->idx = 0;
                o->isEm = 1;
                o->owner = em;
                o->yaw = ang;
                o->pos[0] = pos[0];
                o->pos[1] = pos[1];
                o->pos[2] = pos[2];
                o->area = em->area;
                o->rotX = 0;
                o->rotY = ang;
                o->stateVar0 = 0;
                o->stateVar1 = c;
                o->ownerMotion = 0;
            }
        }
    }
}

void Eff22_CreateAtPos(Em *em, f32 *pos, s8 kind, u8 n)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 22;
            o->type = kind;
            o->update = Eff22_Update;
            o->idx = 0;
            o->isEm = 1;
            o->owner = em;
            o->yaw = em->rotY;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->area = em->area;
            o->rotX = 0;
            o->rotY = 0;
            o->stateVar0 = 0;
            o->ownerMotion = -n * 20;
        }
    }
}

void Eff22_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff22_Setup(o);
        break;
    case 1:
        Eff22_Run(o);
        break;
    case 2:
        Eff22_End(o);
        break;
    case 3:
        Eff22_Hit(o);
        break;
    case 4:
        Eff22_End(o);
        break;
    case 5:
        Eff22_End(o);
        break;
    case 9:
        Eff22_End(o);
        break;
    case 7:
        Eff22_End(o);
        break;
    case 6:
        Eff22_Free(o);
        
    default:
        break;
    }
}

void Eff22_Setup(ObjAttach *o)
{
    Em *owner = o->owner;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    if (o->type == 4 || o->type == 6) {
        Player_TestSetFlags(o, 0);
    } else {
        Player_TestSetFlags(o, 0x400);
    }
    o->aux = 0;
    if (owner == NULL) {
        MoveObj_StartMotionEntry((struct MoveObj *)o, o->type + 1, &Eff22_MotionRef);
    } else {
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) owner, o->type + 1, &Eff22_MotionRef);
    }
    switch (o->type) {
    case 2:
    case 8:
        o->hitList = Eff22_Vals088[o->hitListNo + o->stateVar1];
        o->hurtList = 0;
        break;
    default:
        o->hitList = Eff22_Vals088[o->hitListNo];
        o->hurtList = 0;
        break;
    }
    Vec_Copy(o->pos2, o->pos);
    o->velX = Eff22_Val048ByType[o->type][0];
    o->velY = Eff22_Val048ByType[o->type][1];
    o->velZ = Eff22_Val048ByType[o->type][2];
    o->homingTurn = 0.0f;
    o->gravity = Eff22_Val058ByType[o->type];
    o->homingSpeed = 0.0f;
    switch (o->type) {
    case 4:
        o->velX += Eff22_Add048B[o->stateVar1 & 3];
        o->velY += Eff22_Add048B[(o->stateVar1 & 3) + 1];
        o->velZ += Eff22_Add048B[(o->stateVar1 & 3) + 2];
        break;
    case 1:
        o->velX += Eff22_Add048A[o->randBits & 3];
        o->velY += 0.4f * Eff22_Add048A[(o->randBits >> 2) & 3];
        o->velZ += Eff22_Add048A[(o->randBits >> 4) & 3];
        o->gravity += 0.2f * Eff22_Add048A[(o->randBits >> 6) & 3];
        break;
    }
    Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
    Eff22_PlaySound(o, 0);
    switch (o->type) {
    case 0:
    case 1:
    case 4:
    case 6:
    case 7:
    default:
        o->drawSlot = DrawSlot_Alloc();
        if (o->drawSlot != -1) {
            o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
            o->drawSlotBuf->owner = o;
            o->drawSlotBuf->draw = Eff22_Draw;
            Vec_Copy(o->drawSlotBuf->pos, o->pos);
            if (o->type != 1 && o->type != 0 && o->type != 7) {
                OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
            }
            if (o->type == 4 && o->area == gScene.area) {
                Eff22_CamShake(o);
            }
        } else {
            ObjAttach_Free(o);
        }
        break;
    case 2:
    case 3:
    case 5:
        o->drawSlotBuf = NULL;
        break;
    }
}

void Eff22_Run(ObjAttach *o)
{
    Em *e;
    f32 f;
    s16 i;
    s16 lay = 1;

    o->ownerMotion++;
    Vec_Copy(o->pos2, o->pos);
    if (o->type != 7 && o->hitFrames != 0) {
        o->hitFrames = 99;
    }
    if (o->ownerMotion > Eff22_TimeLimit[o->type]) {
        o->hitFrames = 0;
        o->hitPhase = 0;
        o->state = 2;
        return;
    }
    switch (o->type) {
    case 0:
    default:
        if (o->ownerMotion < 5) {
            return;
        }
        if (o->ownerMotion < 13) {
            if (o->ownerMotion == 5) {
                Eff22_PlaySound(o, 1);
                if (o->area == gScene.area) {
                    StageObj_K12_CreateAtTimed(o->pos, 10, o->rotX, o->rotY);
                    StageObj_K12_CreateAtTimed(o->pos, 11, o->rotX, o->rotY);
                }
            }
            MoveObj_AddVelocity((struct MoveObj *)o);
        } else {
            MoveObj_Integrate((struct MoveObj *)o);
        }
        if (o->ownerMotion > 5 && o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 1:
        switch (o->stateVar1) {
        case 0:
            if (o->ownerMotion > 10) {
                o->stateVar1++;
                Eff22_PlaySound(o, 1);
                if (o->area == gScene.area) {
                    StageObj_K12_CreateAtTimed(o->pos, 10, o->rotX, o->rotY);
                    StageObj_K12_CreateAtTimed(o->pos, 11, o->rotX, o->rotY);
                }
            }
            return;
        case 1:
            MoveObj_Integrate((struct MoveObj *)o);
            if (o->ownerMotion > 15 && o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = 2;
            }
            break;
        }
        break;
    case 2:
        if (o->ownerMotion <= 4) {
            MoveObj_Integrate((struct MoveObj *)o);
        }
        break;
    case 3:
        if (++o->aux >= 25) {
            MoveObj_StartMotionEntry((struct MoveObj *)o, o->type + 1, &Eff22_MotionRef);
            o->aux = 0;
        }
        if (o->ownerMotion <= 4) {
            MoveObj_Integrate((struct MoveObj *)o);
        }
        break;
    case 8:
        if (o->stateVar0 == 0 && ++o->aux >= 20) {
            MoveObj_StartMotionEntry((struct MoveObj *)o, o->type + 1, &Eff22_MotionRef);
            o->aux = 0;
            o->stateVar0++;
        }
        if (o->ownerMotion <= 4) {
            MoveObj_Integrate((struct MoveObj *)o);
        }
        break;
    case 4:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->ownerMotion > 5) {
            if (o->ownerMotion == 7) {
                Eff22_PlaySound(o, 1);
            }
            f = 50.0f + AreaGrid2_GetFloorYFiltered(o->pos);
            if (o->pos[1] <= f) {
                o->pos[1] = f;
                if (o->area == gScene.area) {
                    Eff22_CamShake(o);
                    Eff22_PlaySound(o, 2);
                    Effect02_SpawnScaled(0, 0xB, 0, o->pos, 2.0f);
                    Effect02_SpawnScaled(0, 0xB, 1, o->pos, 2.0f);
                    Effect0D_SpawnIfHere(o->owner, o->pos, 0x22, 16.0f);
                }
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = 2;
            }
        }
        break;
    case 6:
        if (o->ownerMotion > 0) {
            MoveObj_Integrate((struct MoveObj *)o);
            if (o->ownerMotion > 5 && o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = 2;
            }
        }
        lay = 0;
        break;
    case 7:
        if (o->ownerMotion <= 0) {
            return;
        }
        switch (o->stateVar1) {
        case 0:
            if (o->ownerMotion == 1) {
                for (i = 0, e = gEms; i < 20; i++, e++) {
                    if (e->active != 0 && e->visible != 0 && Side_Match(o->side, *(u8 *) &e->side, 1) == 0) {
                        o->pos[0] = e->posX;
                        o->pos[1] = e->floorY;
                        o->pos[2] = e->posZ;
                        o->rotY = e->rotY;
                        Eff22_PlaySound(o, 1);
                        if (o->area == gScene.area) {
                            Effect0D_SpawnAt(o->pos, 7, 3.0f);
                        }
                        break;
                    }
                }
                if (i == 20) {
                    o->hitFrames = 0;
                    o->hitPhase = 0;
                    o->state = 2;
                    return;
                }
            }
            MoveObj_Integrate((struct MoveObj *)o);
            if (++o->aux >= 4) {
                o->stateVar1++;
                o->aux = 0;
            }
            break;
        case 1:
            if (++o->aux >= 15) {
                o->stateVar1++;
                o->aux = 0;
                Eff22_PlaySound(o, 2);
            }
            break;
        case 2:
            o->pos[1] -= 60.0f;
            break;
        }
        break;
    case 5:
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        if (lay == 0) {
            OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40U, 0);
        } else {
            OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
        }
    }
}

void Eff22_Hit(ObjAttach *o)
{
    u8 type;

    type = o->type;
    switch (type) {
    case 0x0:
    default:
        Eff22_End(o);
        break;
    case 2:
    case 3:
    case 7:
    case 8:
        Eff22_Run(o);
        break;
    case 4:
        if (o->area == gScene.area) {
            Eff22_CamShake(o);
            Effect02_SpawnScaled(0, 0xB, 0, o->pos, 2.0f);
            Effect02_SpawnScaled(0, 0xB, 1, o->pos, 2.0f);
            Effect0D_SpawnIfHere(o->owner, o->pos, 0x22, 16.0f);
        }
        o->hitFrames = 0;
        o->hitPhase = 0;
        Eff22_End(o);
        break;
    }
}

void Eff22_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void Eff22_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff22_DrawModel(ModelMatSet *mdl, f32 *m, ModelMat *tex)
{
    Gs_SetState(0x1A, (s32) m);
    if (mdl != NULL && mdl->id != -1) {
        Gs_SetLightList(tex, mdl);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
}

void Eff22_Draw(DrawSlot *d)
{
    f32 m[16];
    f32 t[16];
    u16 ax;
    u16 ay;
    ObjAttach *o = d->owner;
    ModelMat *tex;
    ModelMatSet *mdl;

    if (gScene.area != o->area) {
        return;
    }
    switch (o->type) {
    case 0:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Scale(m, 2.0f, 2.0f, 1.0f);
        tex = bank->mats;
        switch (o->area) {
        case 0xC:
        case 0x4E:
            mdl = &bank->matSets[4];
            break;
        case 0x19:
            mdl = &bank->matSets[13];
            break;
        case 0xB0:
        case 0xB4:
            mdl = &bank->matSets[4];
            break;
        case 0xB2:
        case 0xB6:
            mdl = &bank->matSets[2];
            break;
        default:
            return;
        }
        }
        Vec_DirToAngles(&o->velX, (s16 *) &ax, (s16 *) &ay);
        Mtx_RotateX(m, (6.2831855f * ax) / 65536.0f);
        Mtx_RotateY(m, (6.2831855f * ay) / 65536.0f);
        break;
    case 1:
    {
        Model *bank = gCommonModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Identity(m);
        tex = bank->mats;
        mdl = &bank->matSets[133];
        Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) t);
        break;
    }
    case 4:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Identity(m);
        tex = bank->mats;
        switch (o->area) {
        case 0xB:
        case 0x1E:
            mdl = &bank->matSets[2];
            break;
        case 0x4D:
            mdl = &bank->matSets[3];
            break;
        case 0x1C:
        case 0x50:
            mdl = &bank->matSets[24];
            break;
        case 0x51:
            mdl = &bank->matSets[3];
            break;
        default:
            return;
        }
        break;
    }
    case 6:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Scale(m, 1.7f, 1.0f, 1.7f);
        switch (o->area) {
        case 0x5E:
        default:
        case 0x67:
            mdl = &bank->matSets[4];
            break;
        case 0x64:
        case 0x6D:
            mdl = &bank->matSets[7];
            break;
        }
        tex = bank->mats;
        Gs_SetState(0x67, -1);
        break;
    }
    case 7:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Identity(m);
        tex = bank->mats;
        switch (o->area) {
        case 0x39:
        case 0x5A:
            mdl = &bank->matSets[22];
            break;
        default:
            return;
        }
        break;
    }
    default:
        return;
    }
    Mtx_SetTrans(m, d->pos[0], d->pos[1], d->pos[2]);
    Eff22_DrawModel(mdl, m, tex);
}

void Eff22_PlaySound(ObjAttach *o, s16 n)
{
    Em *em = gEms;
    s16 i;

    if (o->area != gScene.area) {
        return;
    }
    switch (o->type) {
    case 0:
        switch (n) {
        case 0:
            Sound_PlaySeAtPosChecked(7, 0x38, 0, o->pos, 7, 0);
            break;
        case 1:
            Sound_PlaySeAtPosChecked(7, 0x30, 0, o->pos, 7, 0);
            break;
        }
        break;
    case 1:
        switch (n) {
        case 0:
            break;
        case 1:
            Sound_PlaySeAtPosChecked(7, 0x32, 0, o->pos, 7, 0);
            break;
        }
        break;
    case 2:
    case 3:
    case 8:
        switch (n) {
        case 0:
            Sound_PlaySeAtPosChecked(7, 0x34, 0, o->pos, 7, 0);
            break;
        }
        break;
    case 4:
        for (i = 0; i < 20; i++, em++) {
            if (em->active && em->visible && (em->type == 7 || em->type == 0x32)) {
                break;
            }
        }
        switch (n) {
        case 0:
            Em_PlaySeChecked(em, 0x2C, 0, o->pos, 3, 0);
            break;
        case 1:
            Em_PlaySeChecked(em, 0x2D, 0, o->pos, 3, 0);
            break;
        case 2:
            Em_PlaySeChecked(em, 0x2B, 0, o->pos, 3, 0);
            break;
        }
        break;
    case 5:
    case 6:
        break;
    case 7:
        switch (n) {
        case 1:
            Sound_PlaySeAtPosChecked(7, 0x34, 0, o->pos, 7, 0);
            break;
        case 2:
            Sound_PlaySeAtPosChecked(7, 0x36, 0, o->pos, 7, 0);
            break;
        }
        break;
    }
}

s32 Eff22_FindSpot(f32 *from, f32 *pos, u16 *ang)
{
    s16 i;

    switch (Area_DayId(gScene.area)) {
    case 0xB:
        if (from[0] > 12000.0f && from[0] < 16000.0f && from[2] > 20500.0f && from[2] < 23000.0f) {
            i = 0;
        } else if (from[0] > 12000.0f && from[0] < 16000.0f && from[2] > 26000.0f && from[2] < 29000.0f) {
            i = 1;
        } else if (from[0] > 16500.0f && from[0] < 19000.0f && from[2] > 28000.0f && from[2] < 32000.0f) {
            i = 2;
        } else {
            return 0;
        }
        Vec_Copy(pos, Eff22_SpotPosA[i]);
        *ang = Eff22_SpotAngA[i];
        return 1;
    case 0x1C:
        if (from[0] > 14000.0f && from[0] < 17000.0f && from[2] > 18000.0f && from[2] < 25000.0f) {
            i = 0;
        } else if (from[0] > 17000.0f && from[0] < 20000.0f && from[2] > 22000.0f && from[2] < 29000.0f) {
            i = 1;
        } else {
            return 0;
        }
        Vec_Copy(pos, Eff22_SpotPosB[i]);
        *ang = Eff22_SpotAngB[i];
        return 1;
    case 0x1E:
        if (from[0] > 9000.0f && from[0] < 13000.0f && from[2] > 21000.0f && from[2] < 26000.0f) {
            i = 0;
        } else if (from[0] > 14000.0f && from[0] < 18000.0f && from[2] > 21000.0f && from[2] < 26000.0f) {
            i = 1;
        } else if (from[0] > 20000.0f && from[0] < 25000.0f && from[2] > 20000.0f && from[2] < 25000.0f) {
            i = 2;
        } else {
            return 0;
        }
        Vec_Copy(pos, Eff22_SpotPosC[i]);
        *ang = Eff22_SpotAngC[i];
        return 1;
    default:
        return 0;
    }
}

void Eff22_CamShake(ObjAttach *o)
{
    f32 dist;

    dist = Vec_Distance(o->pos, (f32 *) &gCamPos);
    if (dist < 500.0f) {
        CamShake_SetAEverywhere(5);
        return;
    }
    if (dist < 1000.0f) {
        CamShake_SetAEverywhere(4);
        return;
    }
    if (dist < 1500.0f) {
        CamShake_SetAEverywhere(3);
        return;
    }
    if (dist < 2000.0f) {
        CamShake_SetAEverywhere(2);
        return;
    }
    if (dist < 2500.0f) {
        CamShake_SetAEverywhere(1);
        return;
    }
    if (dist < 3000.0f) {
        CamShake_SetAEverywhere(0);
    }
}

void EmBlangonga_State3Pick(Em *em)
{
    s16 pick[2];
    s16 used[2];
    s16 **tabs;
    s16 *tab;
    s16 mode;
    s16 i;
    f32 (*pts)[3];
    s16 j;
    s16 n;

    switch (Area_DayId(gScene.area)) {
    case 0x5E:
        mode = 1;
        tabs = EmBlangonga_State3Tabs;
        pts = EmBlangonga_State3PtsA;
        break;
    case 0x64:
        mode = 0;
        tab = EmBlangonga_State3TabByZ[(s16) ((em->posZ <= 9000.0f) ^ 1)];
        tabs = EmBlangonga_State3TabByZ;
        pts = EmBlangonga_State3PtsB;
        break;
    default:
        return;
    }
    for (i = 0; i < 2; i++) {
        if (mode == 1) {
            tab = tabs[i];
        }
        pick[i] = (em->stateRand >> i) % (tab[0] - i);
        if (mode == 0) {
            for (j = 0; j < i; j++) {
                used[j] = 0;
            }
        again:
            n = 0;
            for (j = 0; j < i; j++) {
                if (used[j] == 0 && pick[j] <= pick[i]) {
                    used[j] = 1;
                    n++;
                }
            }
            if (n > 0) {
                pick[i] += n;
                goto again;
            }
        }
        Eff22_CreateAtPos(em, pts[tab[pick[i] + 1]], 6, i);
    }
}

void EffCephadrome23_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 23;
            o->type = kind;
            o->update = EffCephadrome23_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffCephadrome23_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffCephadrome23_Setup(o);
        break;
    case 1:
        EffCephadrome23_Run(o);
        break;
    case 2:
        EffCephadrome23_End(o);
        break;
    case 3:
        EffCephadrome23_Run(o);
        break;
    case 4:
        EffCephadrome23_End(o);
        break;
    case 5:
        EffCephadrome23_End(o);
        break;
    case 9:
        EffCephadrome23_End(o);
        break;
    case 7:
        EffCephadrome23_End(o);
        break;
    case 6:
        EffCephadrome23_Free(o);
        
    default:
        break;
    }
}

void EffCephadrome23_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffCephadrome23_MotionRef);
    o->hitList = EffCephadrome23_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffCephadrome23_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 14:
    case 19:
    case 23:
    case 24:
    case 26:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffCephadrome23_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffCephadrome23_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Bowgun_FireLoaded(Player *pl, s32 unused)
{

    if ((s8) pl->chargeLevel == 3) {
        StageObj_K29_CreateOnPlayerDefault(pl, 3, 1.0f);
    }
    BowgunShot_Fire(pl, gBowgunClipShots[gGunData[pl->equip[6].id].clipRow][(s8) pl->chargeLevel & 3]);
}

void BowgunShot_Create(Player *pl, u16 angle, u8 kind, u8 idx, u8 crit, s16 shot)
{
    ObjAttach *o;
    BowgunShotBuf *buf;

    if ((o = ObjAttach_Alloc(1)) != NULL) {
        o->kind = 24;
        o->type = kind;
        o->update = BowgunShot_Update;
        o->idx = pl->index;
        o->isEm = pl->isEm;
        o->owner = pl;
        o->area = pl->area;
        o->rotX = 0;
        o->rotY = angle;
        o->stateVar1 = idx;
        o->side = pl->side;
        o->randBits = pl->stateRand * (idx * 2 + 1);
        buf = (BowgunShotBuf *) o->buf;
        buf->charge = (s8) pl->chargeLevel & 3;
        BowgunShot_InitAttack(buf, pl);
        BowgunShot_SkillFlags(buf, pl);
        buf->crit = crit;
        buf->shot = shot;
    }
}

void BowgunShot_Fire(Player *pl, s16 shot)
{
    u8 i;
    u8 crit;
    s16 *p;
    u16 spread;
    u8 c;
    s16 aff;

    p = BowgunShot_ShotParams[shot];
    i = 0;

    switch (shot) {
    default:
        spread = 0;
        break;
    case 4:
        spread = 1;
        break;
    case 5:
        spread = 2;
        break;
    case 6:
        spread = 3;
        break;
    case 7:
        spread = 4;
        break;
    }
    crit = 0;
    aff = Player_GetWeaponStat(pl, 8);
    if (aff < 0) {
        if (pl->stateRand % 100 < -aff) {
            crit = 2;
        }
    } else {
        if (aff > pl->stateRand % 100) {
            crit = 1;
        }
    }
    for (; *p != -1; p++, i++) {
        if (i <= 2) {
            c = crit;
        } else {
            c = 0;
        }
        BowgunShot_Create(pl, BowgunShot_SpreadScale[spread] * BowgunShot_SpreadDirs[i], *p, i, c, shot);
    }
}

void BowgunShot_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        BowgunShot_Setup(o);
        break;
    case 1:
        BowgunShot_Run(o);
        break;
    case 2:
        BowgunShot_End(o);
        break;
    case 3:
        BowgunShot_Hit(o);
        break;
    case 4:
        BowgunShot_End(o);
        break;
    case 5:
        BowgunShot_State5(o);
        break;
    case 9:
        BowgunShot_State5(o);
        break;
    case 7:
        BowgunShot_End(o);
        break;
    case 6:
        BowgunShot_Free(o);
        
    default:
        break;
    }
}

void BowgunShot_Setup(ObjAttach *o)
{
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    Rec8A5780 *r = &BowgunShot_Kinds[o->type];
    Em *pl = o->owner;
    f32 v[3];
    u16 ax;
    u16 ay;
    void *bone;
    f32 *t;
    f32 sx;
    f32 sy;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) pl, o->type + 1, &BowgunShot_MotionRef);
    BowgunShot_ApplyKind(o);
    switch (o->elementFlags) {
    case 2:
    case 1:
    case 4:
        Player_TestSetFlags(o, 9);
        break;
    default:
        Player_TestSetFlags(o, 0x19);
        break;
    }
    b->power = o->power;
    o->hitList = BowgunShot_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->step = 0;
    o->stateVar0 = 0;
    b->bounced = 0;
    b->event = 0;
    o->ownerMotion = 0;
    v[0] = 1.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    bone = Em_GetBone(pl, 14);
    Vec_RotateSelf(v, bone);
    Mtx_GetTrans(o->pos, bone);
    Vec_DirToAngles(v, (s16 *) &ax, (s16 *) &ay);
    o->rotX += ax;
    o->rotY += ay;
    o->yaw = o->rotY;
    t = BowgunShot_KindTimes[b->charge];
    sx = r->unk8 * t[1];
    sy = r->unk10 * t[3];
    if (b->skills & 2) {
        sx *= 2.0f;
        sy *= 2.0f;
    }
    if (b->skills & 4) {
        sx *= 0.5f;
        sy *= 0.5f;
    }
    o->velX = 0.0f;
    o->velY = 0.0f;
    o->velZ = r->a + t[0] + sx * BowgunShot_BitsToFrac(o->randBits, 16);
    switch (r->unk0) {
    case 0:
        o->velZ *= 1.0f - 0.2f * o->stateVar1;
        break;
    }
    Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
    o->homingTurn = 0.0f;
    o->gravity = -(r->b + t[2]) + sy * BowgunShot_BitsToFrac(o->randBits >> 1, 15);
    o->homingSpeed = 0.0f;
    Vec_Copy(o->pos2, o->pos);
    if (r->unk2 != -1) {
        BowgunShot_ApplyKind2(o);
    }
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = BowgunShot_Draw;
        BowgunShot_Run(o);
    } else {
        o->drawSlotBuf = NULL;
    }
}

void BowgunShot_Move(ObjAttach *o)
{
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    Rec8A5780 *r = &BowgunShot_Kinds[o->type];
    f32 *t = BowgunShot_KindTimes[b->charge];
    f32 v[3];
    f32 p[3];
    f32 d[3];
    f32 mat[16];
    f32 len;
    f32 s;
    Em *em;

    Vec_Copy(o->pos2, o->pos);
    o->hitFrames = 99;
    if (o->stateVar0 > 0) {
        switch (r->type) {
        case 1:
            if (!(o->ownerMotion < r->count)) {
                o->state = 2;
                return;
            }
            em = o->targetEm;
            if (em == NULL) {
                o->state = 2;
                return;
            }
            if (em->taskPhase == 3 || em->active == 0) {
                o->state = 2;
                return;
            }
            Mtx_Copy(mat, (f32 *)Em_GetBone(em, o->aux));
            Mtx_GetTrans(p, mat);
            Vec_Rotate(d, &o->velX, mat);
            o->pos[0] = d[0] + p[0];
            o->pos[1] = d[1] + p[1];
            o->pos[2] = d[2] + p[2];
            return;
        case 2:
            if (o->aux != 0) {
                if (--o->aux == 0) {
                    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, &BowgunShot_MotionRef);
                    BowgunShot_ApplyKind(o);
                    o->hitList = BowgunShot_Vals088[0];
                    o->hurtList = 0;
                    o->velX *= 0.7f;
                    o->velY *= 0.7f;
                    o->velZ *= 0.7f;
                    BowgunShot_ApplyKind2(o);
                }
            }
            break;
        }
    }
    len = Math_Sqrtf(o->velX * o->velX + o->velZ * o->velZ);
    if (o->ownerMotion <= 19 && BowgunShot_MulAddShift(o->randBits, o->ownerMotion, 0x3045, 1) <= 0x1FFF) {
        s = r->unk18 * t[5];
        if (b->skills & 2) {
            s *= 2.0f;
        }
        if (b->skills & 4) {
            s *= 0.5f;
        }
        v[0] = s * BowgunShot_BitsToSignedFrac(BowgunShot_MulAddShift(o->randBits, o->ownerMotion, 0x3121, 4), 6);
    } else {
        v[0] = 0.0f;
    }
    v[1] = len * (r->unk14 + t[4]) * BowgunShot_BitsToFrac(BowgunShot_MulAddShift(o->randBits, o->ownerMotion, 0x147F, 2), 14);
    v[2] = 0.0f;
    Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
    Vec_Add(&o->velX, &o->velX, v);
    if (o->ownerMotion < r->c) {
        MoveObj_AddVelocity((struct MoveObj *)o);
    } else {
        MoveObj_Integrate((struct MoveObj *)o);
    }
    if (o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
        o->hitFrames = 0;
        o->state = 2;
        return;
    }
    if (BowgunShot_StepEvents(o)) {
        BowgunShot_ApplyKind2(o);
        if (b->event == 0xFF) {
            b->doneFrames = 0;
        }
    }
}

void BowgunShot_Run(ObjAttach *o)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;

    if (o->stateVar1 == 0xFE) {
        if (BowgunShot_BounceStep(o) != 0) {
            o->state = 2;
            return;
        }
    } else {
        if (++o->ownerMotion > 100) {
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
        if (o->stateVar1 != 0xFF) {
            if (buf->event == 0xFF) {
                if (++buf->doneFrames > 3) {
                    o->hitFrames = 0;
                    o->state = 2;
                    return;
                }
            }
            BowgunShot_Move(o);
        } else {
            BowgunShot_Fall(o);
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void BowgunShot_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void BowgunShot_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void BowgunShot_Hit(ObjAttach *o)
{
    Rec8A5780 *e = &BowgunShot_Kinds[o->type];

    if (o->stateVar1 == 0xFF) {
        o->state = 2;
    } else {
        switch (e->type) {
        case 0:
        default:
            o->state = 2;
            break;
        case 1:
            o->state = 2;
            break;
        case 2:
            if (++o->stateVar0 < e->count) {
                o->aux = 2;
            } else {
                o->hitPhase = 0;
            }
            o->state = 1;
            if (++o->ownerMotion > 100) {
                o->hitFrames = 0;
                o->state = 2;
                return;
            }
            BowgunShot_Move(o);
            break;
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void BowgunShot_State5(ObjAttach *o)
{
    Em *owner = o->owner;
    Em *hit = o->targetEm;
    f32 p[3];
    f32 v[3];
    u16 ax;
    u16 ay;

    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    o->side = 0;
    if (func_364AE0(o, o->randBits >> o->stateVar1) == 0) {
        o->stateVar1 = 0xFE;
        o->hitPhase = 0;
        Vec_Diff(v, o->pos2, o->pos);
        Vec_DirToAngles(v, (s16 *) &ax, (s16 *) &ay);
        ax += -(Rand_Next(1) & 0x7FF) - 0x200;
        ay += (Rand_Next(1) & 0x3FFF) - 0x2000;
        o->velX = 0.0f;
        o->velY = 1.0f + (Rand_Next(1) & 0x1F);
        o->velZ = 10.0f + (Rand_Next(1) & 7);
        o->homingTurn = 0.0f;
        o->gravity = -1.8f - (Rand_Next(1) & 0x1F) / 32.0f;
        o->homingSpeed = 0.0f;
        Vec_RotateX(&o->velX, (6.2831855f * ax) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * ay) / 65536.0f);
        o->rotX = Rand_Next(1);
        o->rotY = Rand_Next(1);
    } else {
        o->stateVar1 = 0xFF;
        o->idx = hit->index;
        o->isEm = hit->isEm;
        o->owner = hit;
        Player_TestSetFlags(o, 0);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) hit, 12, &BowgunShot_MotionRef);
        o->hitList = BowgunShot_Vals088[o->hitListNo];
        o->hurtList = 0;
        Vec_Copy(p, &owner->posX);
        p[1] += 100.0f;
        Vec_Diff(v, p, o->pos);
        Vec_DirToAngles(v, (s16 *) &ax, (s16 *) &ay);
        o->rotX = ax;
        o->rotY = ay;
        o->yaw = o->rotY;
        o->velX = 0.0f;
        o->velY = 0.0f;
        o->velZ = 100.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        o->homingTurn = 0.0f;
        o->gravity = -1.5f;
        o->homingSpeed = 0.0f;
        Vec_Copy(o->pos2, o->pos);
        if (gScene.area == o->area) {
            Sound_PlaySeAtPosChecked(1, 0x96, 0, o->pos, 6, 0);
        }
    }
    o->state = 1;
    BowgunShot_Run(o);
}

void BowgunShot_Fall(ObjAttach *o)
{
    Vec_Copy(o->pos2, o->pos);
    o->hitFrames = 0x63;
    if (o->ownerMotion < 0x14) {
        MoveObj_AddVelocity((struct MoveObj *)o);
    } else {
        MoveObj_Integrate((struct MoveObj *)o);
    }
    if (o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
        o->hitFrames = 0;
        o->state = 2;
    }
}

u8 BowgunShot_BounceStep(ObjAttach *o)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;

    o->hitPhase = 0;
    buf->bounced = 1;
    if (++o->ownerMotion > 20) {
        return 1;
    }
    MoveObj_Integrate((struct MoveObj *)o);
    o->rotX -= 0x400;
    return 0;
}

void BowgunShot_Draw(DrawSlot *d)
{
    u16 ax;
    u16 ay;
    f32 m[16];
    ObjAttach *o;
    BowgunShotBuf *buf;
    ModelMat *tex;
    Model *set;
    ModelMatSet *mdl;

    set = gCommonModels[0];
    o = d->owner;
    buf = (BowgunShotBuf *) o->buf;
    if (o->area == gScene.area && set != NULL && set->active != 0) {
        tex = set->mats;
        Mtx_Translation(m, d->pos[0], d->pos[1], d->pos[2]);
        mdl = &set->matSets[149];
        if (buf->bounced == 0) {
            Vec_DirToAngles(&o->velX, (s16 *) &ax, (s16 *) &ay);
            Mtx_RotateX(m, (6.2831855f * ax) / 65536.0f);
            Mtx_RotateY(m, (6.2831855f * ay) / 65536.0f);
        } else {
            Mtx_RotateX(m, (6.2831855f * o->rotX) / 65536.0f);
            Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
        }
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, -1);
        Gs_SetState(0x60, 0x80);
        if (mdl != NULL && mdl->id != -1) {
            Gs_SetLightList(tex, mdl);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void BowgunShot_SkillFlags(BowgunShotBuf *b, Player *pl)
{
    b->skills = 0;
    if (Player_HasSkill(pl, 0x47) == 1) {
        b->skills |= 1;
    }
    if (Player_HasSkill(pl, 0xBA) == 1) {
        b->skills |= 2;
    }
    if (Player_HasSkill(pl, 0xB9) == 1) {
        b->skills |= 4;
    }
}

void Player_WeaponElementFlags(void *pl, u16 *flags, s8 *level)
{
    s16 v;

    *flags = 0;
    *level = 0;
    v = Player_GetWeaponStat(pl, 0);
    if (v > 0) {
        *flags |= 0x10;
        *level = v;
    }
    v = Player_GetWeaponStat(pl, 1);
    if (v > 0) {
        *flags |= 0x20;
        *level = v;
    }
    v = Player_GetWeaponStat(pl, 2);
    if (v > 0) {
        *flags |= 0x40;
        *level = v;
    }
    v = Player_GetWeaponStat(pl, 3);
    if (v > 0) {
        *flags |= 0x80;
        *level = v;
    }
    v = Player_GetWeaponStat(pl, 7);
    if (v > 0) {
        *flags |= 0x200;
        *level = v;
    }
}

void BowgunShot_InitAttack(BowgunShotBuf *b, Player *pl)
{
    b->attackFactor = pl->attackFactor;
    b->ammoType = pl->ammoType;
    Player_WeaponElementFlags(pl, &b->elementFlags, (s8 *) &b->elementLevel);
}

void BowgunShot_SetKind(ObjAttach *o, u8 kind, u8 hit)
{
    f32 v;
    f32 k;

    o->elementFlags = 0;
    o->elementPower = 0;
    switch (kind) {
    case 2:
        o->elementFlags = 2;
        if (hit == 0) {
            o->hitFx = 0x18;
        }
        break;
    case 3:
        o->elementFlags = 4;
        if (hit == 0) {
            o->hitFx = 0x18;
        }
        break;
    case 4:
        o->elementFlags = 1;
        if (hit == 0) {
            o->hitFx = 0x18;
        }
        break;
    case 1:
        if (hit != 0) {
            v = o->power;
            v *= 1.5f;
            if (v > 255.0f) {
                v = 255.0f;
            }
            o->power = v;
        }
        break;
    }
    if (hit != 0) {
        if (o->elementFlags != 0) {
            o->elementPower = 2;
        } else {
            Player_WeaponElementFlags(o->owner, &o->elementFlags, (s8 *) &o->elementPower);
        }
    }
    if (o->elementPower > 0 && hit != 0) {
        switch (o->elementFlags) {
        case 2:
        case 4:
        case 1:
            if (Player_HasSkill(o->owner, 0x47) == 1) {
                k = 1.125f;
            } else {
                k = 1.0f;
            }
            v = o->elementPower * k;
            if (v > 255.0f) {
                v = 255.0f;
            }
            o->elementPower = v;
            break;
        default:
            o->elementPower /= 2;
            break;
        }
        if (o->elementPower == 0) {
            o->elementPower = 1;
        }
    }
}

void BowgunShot_ApplyKind(ObjAttach *o)
{
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    f32 f;
    u16 v;

    if (o->attackKind == 8) {
        return;
    }
    BowgunShot_SetKind(o, b->ammoType, 0);
    if (o->elementFlags == 0) {
        o->elementFlags = b->elementFlags;
        o->elementPower = b->elementLevel;
        if (o->elementPower != 0) {
            o->hitFx = 0x18;
            v = o->elementPower * BowgunShot_KindScaleB[b->charge];
            if (v == 0) {
                v = 1;
            } else if (v > 0xFF) {
                v = 0xFF;
            }
            o->elementPower = v;
        }
    } else {
        f = BowgunShot_KindBase[b->charge];
        if (b->skills & 1) {
            f *= 1.125f;
        }
        v = BowgunShot_LevelScale[b->shot] * f;
        if (v > 0xFF) {
            v = 0xFF;
        }
        o->elementPower = v;
    }
    f = b->attackFactor;
    if (b->ammoType == 1) {
        f *= 1.5f;
    }
    if (o->power != 0) {
        f *= BowgunShot_KindScaleA[b->charge];
        if (b->crit == 1) {
            f *= 1.25f;
            o->hitFx |= 0x80;
        } else if (b->crit == 2) {
            f *= 0.5f;
            o->hitFx |= 0x40;
        }
        v = o->power * f;
        if (v == 0) {
            v = 1;
        } else if (v > 0xFF) {
            v = 0xFF;
        }
        o->power = v;
    }
}

s8 BowgunShot_StepEvents(ObjAttach *o)
{
    Rec8A5780 *r = &BowgunShot_Kinds[o->type];
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;
    s8 ret = 0;
    s16 *tbl;

    if (r->unk2 == -1) {
        return ret;
    }
    tbl = BowgunShot_EventTbls[r->unk2];
    for (; buf->event != 0xFF; buf->event++) {
        if (tbl[buf->event] >= o->ownerMotion) {
            break;
        }
        ret = 1;
        if (tbl[buf->event] < 0) {
            buf->event = 0xFF;
            o->hitPhase = 0;
            break;
        }
    }
    return ret;
}

void BowgunShot_ApplyKind2(ObjAttach *o)
{
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    f32 v;

    if (o->power == 0 || b->event > 4 || b->event == 0xFF) {
        return;
    }
    switch (o->hitFx & 0x3F) {
    case 10:
    case 12:
    case 14:
    case 16:
        o->hitFx = BowgunShot_Kind2Val06F[b->event];
        if (b->crit == 1) {
            o->hitFx |= 0x80;
        } else if (b->crit == 2) {
            o->hitFx |= 0x40;
        }
        o->hitSe = BowgunShot_Kind2Val06B[b->event];
        break;
    }
    if (b->power != 0.0f) {
        v = b->power * BowgunShot_Kind2Scale[b->event];
        if (!(v < 255.0f)) {
            o->power = 255;
        } else if (v <= 1.0f) {
            o->power = 1;
        } else {
            o->power = v;
        }
    }
}

u16 BowgunShot_MulAddShift(u16 randBits, u16 ownerMotion, s32 arg2, s32 shift)
{
    return ((s32) ((randBits & 0xFFFF) + ((ownerMotion & 0xFFFF) * (arg2 & 0xFFFF))) >> (shift & 0xFFFF)) & 0xFFFF;
}

f32 BowgunShot_BitsToFrac(u16 randBits, s32 bits)
{
    s32 shift;

    shift = (0x10 - (bits & 0xFFFF)) & 0xFFFF;
    return 0.000015625f * (f32) ((randBits & 0xFFFF & ((0xFFFF >> shift) & 0xFFFF)) << shift);
}

f32 BowgunShot_BitsToSignedFrac(u16 value, u16 bits)
{
    u16 shift = 16 - bits;

    return 0.000015625f * (s16) (((value & (u16) (0xFFFF >> shift)) << shift) - 0x8000);
}

void BowgunShot_FlightParams(s16 idx, s32 *pos, f32 *a2, s16 *a3)
{
    Rec8A5780 *r;

    pos[0] = 0;
    pos[1] = 0;
    r = &BowgunShot_Kinds[idx];
    *(f32 *) &pos[2] = r->a;
    *a2 = r->b;
    *a3 = r->c;
}

void EffGaruga25_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 25;
            o->type = kind;
            o->update = EffGaruga25_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffGaruga25_Create2(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 25;
            o->type = kind;
            o->update = EffGaruga25_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffGaruga25_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffGaruga25_Setup(o);
        break;
    case 1:
        EffGaruga25_Run(o);
        break;
    case 2:
        EffGaruga25_End(o);
        break;
    case 3:
        EffGaruga25_Run(o);
        break;
    case 4:
        EffGaruga25_End(o);
        break;
    case 5:
        EffGaruga25_End(o);
        break;
    case 9:
        EffGaruga25_End(o);
        break;
    case 7:
        EffGaruga25_End(o);
        break;
    case 6:
        EffGaruga25_Free(o);
        
    default:
        break;
    }
}

void EffGaruga25_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffGaruga25_MotionRef);
    o->hitList = EffGaruga25_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffGaruga25_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 19:
    case 23:
    case 28:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffGaruga25_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffGaruga25_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffHermitaur26_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 26;
            o->type = kind;
            o->update = EffHermitaur26_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            if (kind == 9) {
                *(u16 *) &o->yaw += 0x8000;
            }
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffHermitaur26_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffHermitaur26_Setup(o);
        break;
    case 1:
        EffHermitaur26_Run(o);
        break;
    case 2:
        EffHermitaur26_End(o);
        break;
    case 3:
        EffHermitaur26_Run(o);
        break;
    case 4:
        EffHermitaur26_End(o);
        break;
    case 5:
        EffHermitaur26_End(o);
        break;
    case 9:
        EffHermitaur26_End(o);
        break;
    case 7:
        EffHermitaur26_End(o);
        break;
    case 6:
        EffHermitaur26_Free(o);
        
    default:
        break;
    }
}

void EffHermitaur26_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffHermitaur26_MotionRef);
    o->hitList = EffHermitaur26_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    switch (o->type) {
    default:
        Vec_Copy(o->pos, &m->posX);
        break;
    case 11:
        Vec_Copy(o->pos, &m->posX);
        o->pos[1] = m->floorY;
        break;
    }
    switch (o->hitListNo) {
    case 5:
        o->hitScale = m->scaleX;
        break;
    default:
        o->hitScale = 1.0f;
        break;
    }
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffHermitaur26_Run(ObjAttach *o)
{
    u8 kind = o->type;
    Em *m = &gEms[o->idx];

    switch (kind) {
    default:
        if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 9:
    case 27:
        o->hitFrames = 99;
        if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 11:
        if (m->motionId[0] == 0x3F2) {
            o->hitFrames = 99;
        } else if (m->motionId[0] == 0x3F3) {
            if (Player_PassedFrameScaled(m, 4.0f, 0) != 0 && o->stateVar0 == 0) {
                o->stateVar0++;
                o->hitListNo = 5;
                o->hitList = EffHermitaur26_Vals088[o->hitListNo];
                o->hitFrames = 8;
                o->hitScale = m->scaleX;
                Vec_Copy(o->pos, &m->posX);
                o->pos[1] = m->floorY;
            }
        } else {
            o->hitPhase = 0;
            o->state = 2;
        }
        if (o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffHermitaur26_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffHermitaur26_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffBlangonga27_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 27;
            o->type = kind;
            o->update = EffBlangonga27_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            switch (kind) {
            case 9:
            case 18:
            case 20:
            case 30:
                o->yaw = em->rotY + 0x8000;
                break;
            default:
                o->yaw = em->rotY;
                break;
            }
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffBlangonga27_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffBlangonga27_Setup(o);
        break;
    case 1:
        EffBlangonga27_Run(o);
        break;
    case 2:
        EffBlangonga27_End(o);
        break;
    case 3:
        EffBlangonga27_Run(o);
        break;
    case 4:
        EffBlangonga27_End(o);
        break;
    case 5:
        EffBlangonga27_End(o);
        break;
    case 9:
        EffBlangonga27_End(o);
        break;
    case 7:
        EffBlangonga27_End(o);
        break;
    case 6:
        EffBlangonga27_Free(o);
        
    default:
        break;
    }
}

void EffBlangonga27_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;
    s32 b;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffBlangonga27_MotionRef);
    if (EmBlangonga_IsSmallKind(m) == 1) {
        o->power /= 2;
    }
    o->hitList = EffBlangonga27_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            a = Math_Absf(m->motionBlend[1] % 100);
            b = Math_Absf(m->motionFrame[1]);
        } else {
            a = Math_Absf(m->motionBlend[0] % 100);
            b = Math_Absf(m->motionFrame[0]);
        }
        o->hitDelay += a - b - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffBlangonga27_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 0xF:
    case 0x1D:
    case 0x23:
        o->hitFrames = 99;
        break;
    case 0x1F:
        if (m->motionId[0] == 0x411) {
            if (EmMotion_ReachedFrame(m, 0, 176.0f)) {
                if (EmBlangonga_IsSmallKind(m) == 1) {
                    o->hitPhase = 0;
                } else {
                    o->hitListNo = 23;
                }
            } else if (EmMotion_ReachedFrame(m, 0, 148.0f)) {
                o->hitListNo = 2;
            } else if (EmMotion_ReachedFrame(m, 0, 98.0f)) {
                o->hitListNo = 3;
            } else if (EmMotion_ReachedFrame(m, 0, 52.0f)) {
                o->hitListNo = 2;
            } else {
                o->hitListNo = 3;
            }
        }
        o->hitList = EffBlangonga27_Vals088[o->hitListNo];
        break;
    case 0x21:
        if (m->motionId[0] == o->ownerMotion) {
            o->hitFrames = 20;
        } else if (m->motionId[0] != 0x419 || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        return;
    case 0x24:
        o->hitFrames = 99;
        if (m->motionId[0] == 0x413) {
            if (!EmMotion_ReachedFrame(m, 0, 34.0f) || EmMotion_InFrameRange(m, 0, 54.0f, 86.0f)) {
                o->hitListNo = 2;
            } else {
                o->hitListNo = 3;
            }
        }
        o->hitList = EffBlangonga27_Vals088[o->hitListNo];
        break;
    case 0x27:
        if (m->motionId[0] == o->ownerMotion) {
            o->hitFrames = 26;
        } else if (m->motionId[0] != 0x442 || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        return;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffBlangonga27_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffBlangonga27_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

typedef struct {
    f32 dy;     /* 0x00 */
    f32 vec[3]; /* 0x04 */
    u16 ang10;  /* 0x10 */
    u16 ang12;  /* 0x12 */
    f32 speed;  /* 0x14 */
    u8 unk18;   /* 0x18 */
    u8 unk19;   /* 0x19 */
    u8 pad1A[2];
    f32 unk1C;  /* 0x1C */
    s16 unk20;  /* 0x20 */
    s16 unk22;  /* 0x22 */
    f32 unk24;  /* 0x24 */
    u8 unk28;   /* 0x28 */
    s8 unk29;   /* 0x29 */
    s8 unk2A;   /* 0x2A */
    u8 unk2B;   /* 0x2B */
    u8 unk2C;   /* 0x2C */
} Eff28Buf;

void Eff28_Create(Em *em, u8 kind, u8 n)
{
    f32 pos[3];
    ObjAttach *o;
    Eff28Buf *b;

    if (Obj_InCurrentArea(em)) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->kind = 28;
            o->type = kind;
            o->update = Eff28_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = 0;
            o->owner = em;
            o->area = em->area;
            b = (Eff28Buf *) o->buf;
            o->rotY = 0;
            o->rotZ = 0xF000;
            Em_GetBonePos(o->owner, 0x12, pos);
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->velX = 0.0f;
            o->velY = 0.0f;
            o->velZ = 0.0f;
            o->side = *(u8 *) &em->side;
            b->ang10 = em->rotY;
            b->dy = pos[1] - ((Em *) o->owner)->posY;
            b->unk18 = 22;
            b->vec[0] = 0.0f;
            switch (o->type) {
            case 0:
                b->vec[1] = 0.0f;
                break;
            case 1:
                b->vec[1] = -12.0f;
                break;
            case 2:
                b->vec[1] = 20.0f;
                break;
            }
            b->vec[2] = 50.0f;
            b->unk19 = 0;
            b->unk20 = 0;
            b->unk28 = 0;
            b->unk2B = n;
            b->unk2C = 0;
            b->unk2A = 15;
        }
    }
}

void Eff28_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff28_Setup(o);
        break;
    case 1:
        Eff28_Run(o);
        break;
    case 2:
        Eff28_End(o);
        break;
    case 3:
        Eff28_Hit(o);
        break;
    case 4:
        Eff28_End(o);
        break;
    case 5:
        Eff28_End(o);
        break;
    case 9:
        Eff28_End(o);
        break;
    case 7:
        Eff28_End(o);
        break;
    case 6:
        Eff28_Free(o);
        
    default:
        break;
    }
}

void Eff28_Setup(ObjAttach *o)
{
    Eff28Buf *buf = (Eff28Buf *) o->buf;

    o->state++;
    o->step = 0;
    o->stateVar0 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->ownerMotion = 0;
    if (buf->unk2B <= 0) {
        if (o->type == 1) {
            buf->unk29 = 10;
        } else {
            buf->unk29 = 13;
        }
    } else {
        buf->unk29 = 0;
    }
    o->hitPhase = 0;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, 1, &Eff28_MotionRef);
    o->hitList = Eff28_Val088;
    o->hurtList = 0;
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = Eff28_Draw;
        Eff28_Run(o);
    } else {
        ObjAttach_Free(o);
    }
}

void Eff28_Hit(ObjAttach *o)
{
    Eff28Buf *b = (Eff28Buf *) o->buf;

    if (b->unk2B == 0 && o->stateVar0 == 3) {
        Eff28_End(o);
        return;
    }
    o->state = 1;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, 1, &Eff28_MotionRef);
    o->hitDelay = 0;
    b->unk19 = 1;
    b->unk1C = Vec_Distance(o->pos, &((Em *) o->owner)->posX);
    b->ang12 = Math_Heading(o->pos[0], o->pos[2], ((Em *) o->owner)->posX, ((Em *) o->owner)->posZ) + 0x4000;
    b->unk22 = b->unk1C / 30.0f;
    b->unk24 = Math_Absf(o->pos[1] - (((Em *) o->owner)->posY + b->dy - 30.0f)) / b->unk22;
    Eff28_Run(o);
}

f32 Math_Cos(f32 x);
f32 Vec_Length(f32 *v);
void Vec_Sub(f32 *out, f32 *a, f32 *b);
void Vec_Normalize(f32 *v);


typedef struct {
    f32 x, y, z;
} Eff28Vec;

void Eff28_Run(ObjAttach *o)
{
    f32 dir[3];
    f32 tgt[3];
    f32 v[3];
    Eff28Buf *b;
    Em *em;
    f32 k;
    f32 dy;
    f32 f;
    f32 y;
    f32 len;
    f32 h;
    u16 rx;
    u16 ry;
    u16 rh;
    u16 t;

    b = (Eff28Buf *) o->buf;
    *(Eff28Vec *) o->pos2 = *(Eff28Vec *) o->pos;
    if (b->unk19 == 1) {
        o->stateVar0 = 2;
        o->hitFrames = 0;
    } else {
        o->hitFrames = 99;
    }
    switch (o->stateVar0) {
    case 0:
        if (b->vec[2] <= 0.0f) {
            b->speed = Vec_Length(&o->velX);
            o->homingTurn = 0.0f;
            o->homingSpeed = 0.0f;
            o->stateVar0 = 1;
            o->velX = 1.0f;
            o->velY = 0.0f;
            o->velZ = 0.0f;
            Vec_RotateY(&o->velX, (6.2831855f * b->ang10) / 65536.0f);
            o->pos[0] += o->velX;
            o->pos[1] += o->velY;
            o->pos[2] += o->velZ;
            o->rotY += 0x3000;
            if (o->drawSlotBuf != NULL) {
                Vec_Copy(o->drawSlotBuf->pos, o->pos);
                OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20, 0);
            }
            return;
        }
        if (b->unk2B == 0) {
            if (--b->unk29 < 0) {
                o->stateVar0 = 3;
                switch (o->type) {
                case 0:
                    b->unk29 = 6;
                    break;
                case 1:
                    b->unk29 = 6;
                    break;
                case 2:
                    b->unk29 = 18;
                    break;
                }
            }
        }
        switch (o->type) {
        case 0:
            k = 3.3333333f;
            b->vec[1] = 0.0f;
            break;
        case 1:
            b->vec[1] += 0.48f;
            k = 5.0f;
            break;
        case 2:
            b->vec[1] += -0.8f;
            k = 10.0f;
            break;
        }
        b->vec[0] = -1.0f * k * Math_Cos((6.2831855f * (180.0f * ((50.0f - b->vec[2]) / 50.0f))) / 360.0f);
        b->vec[2] += -2.0f;
        Vec_Copy(&o->velX, b->vec);
        Vec_RotateY(&o->velX, (6.2831855f * b->ang10) / 65536.0f);
        break;
    case 1:
        if (o->ownerMotion++ > 180.0f) {
            o->state = 2;
            break;
        }
        em = (Em *) o->owner;
        y = em->posY + b->dy - 30.0f;
        *(Eff28Vec *) tgt = *(Eff28Vec *) &em->posX;
        tgt[1] = y;
        Vec_Sub(dir, tgt, o->pos);
        len = Vec_Length(dir);
        dir[1] = 0.0f;
        Vec_Normalize(dir);
        if (len > 80.0f) {
        if (b->unk18-- <= 0) {
            b->unk18 = 0;
            Vec_Copy(v, &o->velX);
            o->velY = 0.0f;
            len = Vec_Length(&o->velX);
            o->velX = dir[0] * len;
            o->velZ = dir[2] * len;
            o->velY = v[1];
        } else {
            switch (o->type) {
            case 0:
                k = 2.0f;
                break;
            case 1:
                k = 1.5f;
                break;
            case 2:
                k = 1.0f;
                break;
            }
            dir[0] *= (o->homingTurn += 0.15f * k);
            dir[2] *= (o->homingTurn += 0.15f * k);
            len = Vec_Length(&o->velX);
            o->velX /= len;
            o->velZ /= len;
            o->velX *= b->speed + o->homingSpeed;
            o->velZ *= b->speed + o->homingSpeed;
            o->homingSpeed += 1.5f * k;
            o->velX += dir[0];
            o->velZ += dir[2];
        }
            dy = o->pos[1] - tgt[1];
            if (dy >= 0.0f) {
                if (o->type == 0) {
                    o->velY += -0.16f;
                } else {
                    o->velY += -0.8f;
                }
            } else {
                o->velY += 0.48f;
            }
        } else {
            o->state = 2;
        }
        break;
    case 2:
        if (o->ownerMotion++ > 180.0f) {
            o->state = 2;
            break;
        }
        em = (Em *) o->owner;
        y = em->posY + b->dy - 30.0f;
        *(Eff28Vec *) tgt = *(Eff28Vec *) &em->posX;
        tgt[1] = y;
        Vec_Sub(dir, tgt, o->pos);
        len = Vec_Length(dir);
        dir[1] = 0.0f;
        Vec_Normalize(dir);
        if (len > 80.0f) {
        if (b->unk20++ >= b->unk22) {
            b->unk18 = 0;
            len = 0.15f + Vec_Length(&o->velX);
            o->velX = dir[0] * len;
            o->velZ = dir[2] * len;
        } else {
            o->velX = -0.06f * b->unk1C * Math_Cos((6.2831855f * (180.0f * ((f32) b->unk20 / (f32) b->unk22))) / 360.0f);
            o->velZ = -30.0f;
            Vec_RotateY(&o->velX, (6.2831855f * b->ang12) / 65536.0f);
        }
            dy = o->pos[1] - tgt[1];
            if (dy >= 0.0f) {
                o->velY = -1.0f * b->unk24;
            } else {
                o->velY = b->unk24;
            }
        } else {
            o->state = 2;
        }
        break;
    case 3:
        if (o->ownerMotion++ > 180.0f) {
            o->state = 2;
            return;
        }
        switch (o->type) {
        case 0:
            rx = 0x250;
            f = 1.04f;
            ry = 0x200;
            break;
        case 1:
            rx = 0x5FF;
            if (o->pos[1] <= 10.0f + AreaGrid2_GetFloorYFiltered(o->pos)) {
                f = 0.98f;
            } else {
                f = 1.02f;
            }
            ry = 0;
            break;
        case 2:
            rx = 0x150;
            f = 1.025f;
            ry = 0x150;
            break;
        }
        if (--b->unk29 >= 0) {
            rh = Math_Heading(b->vec[0], b->vec[2], 0.0f, 0.0f);
            h = (6.2831855f * rh) / 65536.0f;
            Vec_RotateY(b->vec, h);
            if (o->type == 1) {
                Vec_RotateX(b->vec, -((6.2831855f * rx) / 65536.0f));
                o->rotZ += 0x200;
            } else {
                Vec_RotateX(b->vec, (6.2831855f * rx) / 65536.0f);
            }
            Vec_RotateY(b->vec, (6.2831855f * ry) / 65536.0f);
            Vec_RotateY(b->vec, -1.0f * h);
        } else {
            b->unk29 = 0;
        }
        if (o->pos[1] <= 10.0f + AreaGrid2_GetFloorYFiltered(o->pos)) {
            if (o->type == 1) {
                b->vec[1] = 0.0f;
                b->unk2C = 1;
            } else {
                o->state = 2;
                return;
            }
        }
        b->vec[0] *= f;
        b->vec[1] *= f;
        b->vec[2] *= f;
        Vec_Copy(&o->velX, b->vec);
        Vec_RotateY(&o->velX, (6.2831855f * b->ang10) / 65536.0f);
        break;
    }
    if (o->stateVar0 == 3 && o->type == 1 && b->unk2C == 1) {
        if (--b->unk2A <= 0) {
            b->unk2A = 0;
            o->state = 2;
            return;
        }
    }
    o->pos[0] += o->velX;
    if (Math_Absf(dy) < Math_Absf(o->velY) && o->stateVar0 != 0 && o->stateVar0 != 3) {
        o->pos[1] = tgt[1];
    } else {
        o->pos[1] += o->velY;
    }
    o->pos[2] += o->velZ;
    t = o->rotY;
    if (o->type == 1 && b->unk2C == 1) {
        if (b->unk2A > 0) {
            o->rotY += (u16) (s32) (0.5f + (65536.0f * (1.1780972f * (b->unk2A / 15))) / 6.2831855f);
        }
    } else {
        o->rotY += 0x3000;
    }
    if ((u16) o->rotY < (u16) t) {
        b->unk28 = 1;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20, 0);
    }
    if (b->unk28 == 1) {
        Sound_PlaySeAtPosChecked(1, 0x88, 0, o->pos, 1, 0);
        b->unk28 = 0;
    }
}

/* unk_6D3B50's .data (0x89EC80-0x8A8700) */
HitShape Eff10_Vals08C_0[2] = {
#include "game/0089EC80.inc"
};
HitShape Eff10_Vals08C_1[2] = {
#include "game/0089ECD0.inc"
};
HitShape Eff10_Vals08C_2[2] = {
#include "game/0089ED20.inc"
};
HitShape Eff10_Vals08C_4[2] = {
#include "game/0089ED70.inc"
};
HitShape Eff10_Vals08C_5[2] = {
#include "game/0089EDC0.inc"
};
HitShape Eff10_Vals08C_6[2] = {
#include "game/0089EE10.inc"
};
HitShape *Eff10_Vals08C[8] = {
    Eff10_Vals08C_0, Eff10_Vals08C_1, Eff10_Vals08C_2, Eff10_Vals08C_2,
    Eff10_Vals08C_4, Eff10_Vals08C_5, Eff10_Vals08C_6, 0,
};
u32 D_0089EE80[30] = {
#include "game/0089EE80.inc"
};
void *Eff10_MotionDesc = (void *)D_0089EE80;
s16 Eff10_Motions[8] = {
#include "game/0089EF00.inc"
};
u8 Eff10_ScaleKeysA[48] = {
#include "game/0089EF10.inc"
};
u8 Eff10_ScaleKeysB[48] = {
#include "game/0089EF40.inc"
};
u8 Eff10_Vec3Keys[48] = {
#include "game/0089EF70.inc"
};
u8 Eff10_ColorKeysA[32] = {
#include "game/0089EFA0.inc"
};
u8 Eff10_ColorKeysB[32] = {
#include "game/0089EFC0.inc"
};
s16 Eff10_Models[8] = {
#include "game/0089EFE0.inc"
};
HitShape EffEm11_Vals088_0[4] = {
#include "game/0089EFF0.inc"
};
HitShape EffEm11_Vals088_1[2] = {
#include "game/0089F090.inc"
};
HitShape EffEm11_Vals088_2[3] = {
#include "game/0089F0E0.inc"
};
HitShape EffEm11_Vals088_3[2] = {
#include "game/0089F160.inc"
};
HitShape EffEm11_Vals088_4[2] = {
#include "game/0089F1B0.inc"
};
HitShape EffEm11_Vals088_5[2] = {
#include "game/0089F200.inc"
};
HitShape EffEm11_Vals088_6[2] = {
#include "game/0089F250.inc"
};
HitShape EffEm11_Vals088_7[3] = {
#include "game/0089F2A0.inc"
};
HitShape EffEm11_Vals088_8[2] = {
#include "game/0089F320.inc"
};
HitShape *EffEm11_Vals088[12] = {
    EffEm11_Vals088_0, EffEm11_Vals088_1, EffEm11_Vals088_2, EffEm11_Vals088_3,
    EffEm11_Vals088_4, EffEm11_Vals088_5, EffEm11_Vals088_6, EffEm11_Vals088_7,
    EffEm11_Vals088_8, 0, 0, 0,
};
u32 D_0089F3A0[120] = {
#include "game/0089F3A0.inc"
};
void *EffEm11_MotionRef = (void *)D_0089F3A0;
HitShape D_0089F590[2] = {
#include "game/0089F590.inc"
};
HitShape *Eff12_Val088 = D_0089F590;
u32 D_0089F5F0[12] = {
#include "game/0089F5F0.inc"
};
void *Eff12_MotionRef = (void *)D_0089F5F0;
HitShape EffVelociprey13_Vals088_0[2] = {
#include "game/0089F630.inc"
};
HitShape EffVelociprey13_Vals088_1[2] = {
#include "game/0089F680.inc"
};
HitShape EffVelociprey13_Vals088_2[2] = {
#include "game/0089F6D0.inc"
};
HitShape EffVelociprey13_Vals088_3[3] = {
#include "game/0089F720.inc"
};
HitShape EffVelociprey13_Vals088_4[2] = {
#include "game/0089F7A0.inc"
};
HitShape *EffVelociprey13_Vals088[8] = {
    EffVelociprey13_Vals088_0, EffVelociprey13_Vals088_1, EffVelociprey13_Vals088_2, EffVelociprey13_Vals088_3,
    EffVelociprey13_Vals088_4, 0, 0, 0,
};
u32 D_0089F810[108] = {
#include "game/0089F810.inc"
};
void *EffVelociprey13_MotionRef = (void *)D_0089F810;
s16 EffEm14_Index[6] = {
#include "game/0089F9C8.inc"
};
HitShape EffDrome15_Vals088_0[2] = {
#include "game/0089F9E0.inc"
};
HitShape EffDrome15_Vals088_1[2] = {
#include "game/0089FA30.inc"
};
HitShape EffDrome15_Vals088_2[2] = {
#include "game/0089FA80.inc"
};
HitShape EffDrome15_Vals088_3[3] = {
#include "game/0089FAD0.inc"
};
HitShape *EffDrome15_Vals088[4] = {
    EffDrome15_Vals088_0, EffDrome15_Vals088_1, EffDrome15_Vals088_2, EffDrome15_Vals088_3,
};
u32 D_0089FB60[84] = {
#include "game/0089FB60.inc"
};
void *EffDrome15_MotionRef = (void *)D_0089FB60;
HitShape EffKhezu16_Vals088_0[2] = {
#include "game/0089FCC0.inc"
};
HitShape EffKhezu16_Vals088_1[2] = {
#include "game/0089FD10.inc"
};
HitShape EffKhezu16_Vals088_2[2] = {
#include "game/0089FD60.inc"
};
HitShape EffKhezu16_Vals088_3[3] = {
#include "game/0089FDB0.inc"
};
HitShape EffKhezu16_Vals088_4[4] = {
#include "game/0089FE30.inc"
};
HitShape EffKhezu16_Vals088_5[4] = {
#include "game/0089FED0.inc"
};
HitShape EffKhezu16_Vals088_6[2] = {
#include "game/0089FF70.inc"
};
HitShape EffKhezu16_Vals088_7[2] = {
#include "game/0089FFC0.inc"
};
HitShape EffKhezu16_Vals088_8[3] = {
#include "game/008A0010.inc"
};
HitShape EffKhezu16_Vals088_9[3] = {
#include "game/008A0090.inc"
};
HitShape EffKhezu16_Vals088_10[2] = {
#include "game/008A0110.inc"
};
HitShape EffKhezu16_Vals088_11[2] = {
#include "game/008A0160.inc"
};
HitShape EffKhezu16_Vals088_12[2] = {
#include "game/008A01B0.inc"
};
HitShape EffKhezu16_Vals088_13[2] = {
#include "game/008A0200.inc"
};
HitShape EffKhezu16_Vals088_14[4] = {
#include "game/008A0250.inc"
};
HitShape EffKhezu16_Vals088_15[2] = {
#include "game/008A02F0.inc"
};
HitShape EffKhezu16_Vals088_16[2] = {
#include "game/008A0340.inc"
};
HitShape EffKhezu16_Vals088_17[2] = {
#include "game/008A0390.inc"
};
HitShape EffKhezu16_Vals088_18[3] = {
#include "game/008A03E0.inc"
};
HitShape EffKhezu16_Vals088_19[2] = {
#include "game/008A0460.inc"
};
HitShape EffKhezu16_Vals088_20[2] = {
#include "game/008A04B0.inc"
};
HitShape *EffKhezu16_Vals088[24] = {
    EffKhezu16_Vals088_0, EffKhezu16_Vals088_1, EffKhezu16_Vals088_2, EffKhezu16_Vals088_3,
    EffKhezu16_Vals088_4, EffKhezu16_Vals088_5, EffKhezu16_Vals088_6, EffKhezu16_Vals088_7,
    EffKhezu16_Vals088_8, EffKhezu16_Vals088_9, EffKhezu16_Vals088_10, EffKhezu16_Vals088_11,
    EffKhezu16_Vals088_12, EffKhezu16_Vals088_13, EffKhezu16_Vals088_14, EffKhezu16_Vals088_15,
    EffKhezu16_Vals088_16, EffKhezu16_Vals088_17, EffKhezu16_Vals088_18, EffKhezu16_Vals088_19,
    EffKhezu16_Vals088_20, 0, 0, 0,
};
u32 D_008A0560[246] = {
#include "game/008A0560.inc"
};
void *EffKhezu16_MotionRef = (void *)D_008A0560;
HitShape EffKutku17_Vals088_0[2] = {
#include "game/008A0940.inc"
};
HitShape EffKutku17_Vals088_1[3] = {
#include "game/008A0990.inc"
};
HitShape EffKutku17_Vals088_2[3] = {
#include "game/008A0A10.inc"
};
HitShape EffKutku17_Vals088_3[3] = {
#include "game/008A0A90.inc"
};
HitShape EffKutku17_Vals088_4[3] = {
#include "game/008A0B10.inc"
};
HitShape EffKutku17_Vals088_5[4] = {
#include "game/008A0B90.inc"
};
HitShape EffKutku17_Vals088_6[2] = {
#include "game/008A0C30.inc"
};
HitShape EffKutku17_Vals088_7[2] = {
#include "game/008A0C80.inc"
};
HitShape EffKutku17_Vals088_8[3] = {
#include "game/008A0CD0.inc"
};
HitShape EffKutku17_Vals088_9[3] = {
#include "game/008A0D50.inc"
};
HitShape EffKutku17_Vals088_10[2] = {
#include "game/008A0DD0.inc"
};
HitShape EffKutku17_Vals088_11[2] = {
#include "game/008A0E20.inc"
};
HitShape EffKutku17_Vals088_12[2] = {
#include "game/008A0E70.inc"
};
HitShape EffKutku17_Vals088_13[2] = {
#include "game/008A0EC0.inc"
};
HitShape EffKutku17_Vals088_14[3] = {
#include "game/008A0F10.inc"
};
HitShape EffKutku17_Vals088_15[2] = {
#include "game/008A0F90.inc"
};
HitShape EffKutku17_Vals088_16[2] = {
#include "game/008A0FE0.inc"
};
HitShape EffKutku17_Vals088_17[2] = {
#include "game/008A1030.inc"
};
HitShape EffKutku17_Vals088_18[4] = {
#include "game/008A1080.inc"
};
HitShape EffKutku17_Vals088_19[2] = {
#include "game/008A1120.inc"
};
HitShape EffKutku17_Vals088_20[6] = {
#include "game/008A1170.inc"
};
HitShape EffKutku17_Vals088_21[5] = {
#include "game/008A1260.inc"
};
HitShape *EffKutku17_Vals088[24] = {
    EffKutku17_Vals088_0, EffKutku17_Vals088_1, EffKutku17_Vals088_2, EffKutku17_Vals088_3,
    EffKutku17_Vals088_4, EffKutku17_Vals088_5, EffKutku17_Vals088_6, EffKutku17_Vals088_7,
    EffKutku17_Vals088_8, EffKutku17_Vals088_9, EffKutku17_Vals088_10, EffKutku17_Vals088_11,
    EffKutku17_Vals088_12, EffKutku17_Vals088_13, EffKutku17_Vals088_14, EffKutku17_Vals088_15,
    EffKutku17_Vals088_16, EffKutku17_Vals088_17, EffKutku17_Vals088_18, EffKutku17_Vals088_19,
    EffKutku17_Vals088_20, EffKutku17_Vals088_21, 0, 0,
};
u32 D_008A1390[228] = {
#include "game/008A1390.inc"
};
void *EffKutku17_MotionRef = (void *)D_008A1390;
HitShape EffDiablos18_Vals088_0[2] = {
#include "game/008A1730.inc"
};
HitShape EffDiablos18_Vals088_1[3] = {
#include "game/008A1780.inc"
};
HitShape EffDiablos18_Vals088_2[3] = {
#include "game/008A1800.inc"
};
HitShape EffDiablos18_Vals088_3[3] = {
#include "game/008A1880.inc"
};
HitShape EffDiablos18_Vals088_4[4] = {
#include "game/008A1900.inc"
};
HitShape EffDiablos18_Vals088_5[4] = {
#include "game/008A19A0.inc"
};
HitShape EffDiablos18_Vals088_6[2] = {
#include "game/008A1A40.inc"
};
HitShape EffDiablos18_Vals088_7[2] = {
#include "game/008A1A90.inc"
};
HitShape EffDiablos18_Vals088_8[3] = {
#include "game/008A1AE0.inc"
};
HitShape EffDiablos18_Vals088_9[3] = {
#include "game/008A1B60.inc"
};
HitShape EffDiablos18_Vals088_10[2] = {
#include "game/008A1BE0.inc"
};
HitShape EffDiablos18_Vals088_11[4] = {
#include "game/008A1C30.inc"
};
HitShape EffDiablos18_Vals088_12[3] = {
#include "game/008A1CD0.inc"
};
HitShape EffDiablos18_Vals088_13[4] = {
#include "game/008A1D50.inc"
};
HitShape EffDiablos18_Vals088_14[3] = {
#include "game/008A1DF0.inc"
};
HitShape EffDiablos18_Vals088_15[2] = {
#include "game/008A1E70.inc"
};
HitShape EffDiablos18_Vals088_16[3] = {
#include "game/008A1EC0.inc"
};
HitShape EffDiablos18_Vals088_17[3] = {
#include "game/008A1F40.inc"
};
HitShape EffDiablos18_Vals088_18[4] = {
#include "game/008A1FC0.inc"
};
HitShape EffDiablos18_Vals088_19[2] = {
#include "game/008A2060.inc"
};
HitShape *EffDiablos18_Vals088[20] = {
    EffDiablos18_Vals088_0, EffDiablos18_Vals088_1, EffDiablos18_Vals088_2, EffDiablos18_Vals088_3,
    EffDiablos18_Vals088_4, EffDiablos18_Vals088_5, EffDiablos18_Vals088_6, EffDiablos18_Vals088_7,
    EffDiablos18_Vals088_8, EffDiablos18_Vals088_9, EffDiablos18_Vals088_10, EffDiablos18_Vals088_11,
    EffDiablos18_Vals088_12, EffDiablos18_Vals088_13, EffDiablos18_Vals088_14, EffDiablos18_Vals088_15,
    EffDiablos18_Vals088_16, EffDiablos18_Vals088_17, EffDiablos18_Vals088_18, EffDiablos18_Vals088_19,
};
u32 D_008A2100[204] = {
#include "game/008A2100.inc"
};
void *EffDiablos18_MotionRef = (void *)D_008A2100;
HitShape EffGravios19_Vals088_0[2] = {
#include "game/008A2440.inc"
};
HitShape EffGravios19_Vals088_1[3] = {
#include "game/008A2490.inc"
};
HitShape EffGravios19_Vals088_2[3] = {
#include "game/008A2510.inc"
};
HitShape EffGravios19_Vals088_3[3] = {
#include "game/008A2590.inc"
};
HitShape EffGravios19_Vals088_4[4] = {
#include "game/008A2610.inc"
};
HitShape EffGravios19_Vals088_5[6] = {
#include "game/008A26B0.inc"
};
HitShape EffGravios19_Vals088_6[2] = {
#include "game/008A27A0.inc"
};
HitShape EffGravios19_Vals088_7[2] = {
#include "game/008A27F0.inc"
};
HitShape EffGravios19_Vals088_8[3] = {
#include "game/008A2840.inc"
};
HitShape EffGravios19_Vals088_9[3] = {
#include "game/008A28C0.inc"
};
HitShape EffGravios19_Vals088_10[2] = {
#include "game/008A2940.inc"
};
HitShape EffGravios19_Vals088_11[2] = {
#include "game/008A2990.inc"
};
HitShape EffGravios19_Vals088_12[2] = {
#include "game/008A29E0.inc"
};
HitShape EffGravios19_Vals088_13[2] = {
#include "game/008A2A30.inc"
};
HitShape EffGravios19_Vals088_14[4] = {
#include "game/008A2A80.inc"
};
HitShape EffGravios19_Vals088_15[2] = {
#include "game/008A2B20.inc"
};
HitShape EffGravios19_Vals088_16[3] = {
#include "game/008A2B70.inc"
};
HitShape EffGravios19_Vals088_17[2] = {
#include "game/008A2BF0.inc"
};
HitShape EffGravios19_Vals088_18[2] = {
#include "game/008A2C40.inc"
};
HitShape EffGravios19_Vals088_19[2] = {
#include "game/008A2C90.inc"
};
HitShape EffGravios19_Vals088_20[2] = {
#include "game/008A2CE0.inc"
};
HitShape EffGravios19_Vals088_21[2] = {
#include "game/008A2D30.inc"
};
HitShape EffGravios19_Vals088_22[2] = {
#include "game/008A2D80.inc"
};
HitShape *EffGravios19_Vals088[24] = {
    EffGravios19_Vals088_0, EffGravios19_Vals088_1, EffGravios19_Vals088_2, EffGravios19_Vals088_3,
    EffGravios19_Vals088_4, EffGravios19_Vals088_5, EffGravios19_Vals088_6, EffGravios19_Vals088_7,
    EffGravios19_Vals088_8, EffGravios19_Vals088_9, EffGravios19_Vals088_10, EffGravios19_Vals088_11,
    EffGravios19_Vals088_12, EffGravios19_Vals088_13, EffGravios19_Vals088_14, EffGravios19_Vals088_15,
    EffGravios19_Vals088_16, EffGravios19_Vals088_17, EffGravios19_Vals088_18, EffGravios19_Vals088_19,
    EffGravios19_Vals088_20, EffGravios19_Vals088_21, EffGravios19_Vals088_22, 0,
};
u32 D_008A2E30[198] = {
#include "game/008A2E30.inc"
};
void *EffGravios19_MotionRef = (void *)D_008A2E30;
HitShape EffPlesioth20_Vals088_0[2] = {
#include "game/008A3150.inc"
};
HitShape EffPlesioth20_Vals088_1[3] = {
#include "game/008A31A0.inc"
};
HitShape EffPlesioth20_Vals088_2[3] = {
#include "game/008A3220.inc"
};
HitShape EffPlesioth20_Vals088_3[2] = {
#include "game/008A32A0.inc"
};
HitShape EffPlesioth20_Vals088_4[4] = {
#include "game/008A32F0.inc"
};
HitShape EffPlesioth20_Vals088_5[4] = {
#include "game/008A3390.inc"
};
HitShape EffPlesioth20_Vals088_6[3] = {
#include "game/008A3430.inc"
};
HitShape EffPlesioth20_Vals088_7[3] = {
#include "game/008A34B0.inc"
};
HitShape EffPlesioth20_Vals088_8[4] = {
#include "game/008A3530.inc"
};
HitShape EffPlesioth20_Vals088_9[3] = {
#include "game/008A35D0.inc"
};
HitShape EffPlesioth20_Vals088_10[2] = {
#include "game/008A3650.inc"
};
HitShape EffPlesioth20_Vals088_11[2] = {
#include "game/008A36A0.inc"
};
HitShape *EffPlesioth20_Vals088[12] = {
    EffPlesioth20_Vals088_0, EffPlesioth20_Vals088_1, EffPlesioth20_Vals088_2, EffPlesioth20_Vals088_3,
    EffPlesioth20_Vals088_4, EffPlesioth20_Vals088_5, EffPlesioth20_Vals088_6, EffPlesioth20_Vals088_7,
    EffPlesioth20_Vals088_8, EffPlesioth20_Vals088_9, EffPlesioth20_Vals088_10, EffPlesioth20_Vals088_11,
};
u32 D_008A3720[156] = {
#include "game/008A3720.inc"
};
void *EffPlesioth20_MotionRef = (void *)D_008A3720;
HitShape EffCephadrome21_Vals088_0[2] = {
#include "game/008A39A0.inc"
};
HitShape EffCephadrome21_Vals088_1[3] = {
#include "game/008A39F0.inc"
};
HitShape EffCephadrome21_Vals088_2[3] = {
#include "game/008A3A70.inc"
};
HitShape EffCephadrome21_Vals088_3[2] = {
#include "game/008A3AF0.inc"
};
HitShape EffCephadrome21_Vals088_4[4] = {
#include "game/008A3B40.inc"
};
HitShape EffCephadrome21_Vals088_5[4] = {
#include "game/008A3BE0.inc"
};
HitShape EffCephadrome21_Vals088_6[3] = {
#include "game/008A3C80.inc"
};
HitShape EffCephadrome21_Vals088_7[3] = {
#include "game/008A3D00.inc"
};
HitShape EffCephadrome21_Vals088_8[4] = {
#include "game/008A3D80.inc"
};
HitShape EffCephadrome21_Vals088_9[3] = {
#include "game/008A3E20.inc"
};
HitShape EffCephadrome21_Vals088_10[2] = {
#include "game/008A3EA0.inc"
};
HitShape EffCephadrome21_Vals088_11[2] = {
#include "game/008A3EF0.inc"
};
HitShape EffCephadrome21_Vals088_12[2] = {
#include "game/008A3F40.inc"
};
HitShape EffCephadrome21_Vals088_13[2] = {
#include "game/008A3F90.inc"
};
HitShape EffCephadrome21_Vals088_14[2] = {
#include "game/008A3FE0.inc"
};
HitShape *EffCephadrome21_Vals088[16] = {
    EffCephadrome21_Vals088_0, EffCephadrome21_Vals088_1, EffCephadrome21_Vals088_2, EffCephadrome21_Vals088_3,
    EffCephadrome21_Vals088_4, EffCephadrome21_Vals088_5, EffCephadrome21_Vals088_6, EffCephadrome21_Vals088_7,
    EffCephadrome21_Vals088_8, EffCephadrome21_Vals088_9, EffCephadrome21_Vals088_10, EffCephadrome21_Vals088_11,
    EffCephadrome21_Vals088_12, EffCephadrome21_Vals088_13, EffCephadrome21_Vals088_14, 0,
};
u32 D_008A4070[180] = {
#include "game/008A4070.inc"
};
void *EffCephadrome21_MotionRef = (void *)D_008A4070;
HitShape Eff22_Vals088_0[2] = {
#include "game/008A4350.inc"
};
HitShape Eff22_Vals088_1[2] = {
#include "game/008A43A0.inc"
};
HitShape Eff22_Vals088_2[2] = {
#include "game/008A43F0.inc"
};
HitShape Eff22_Vals088_3[2] = {
#include "game/008A4440.inc"
};
HitShape Eff22_Vals088_4[2] = {
#include "game/008A4490.inc"
};
HitShape Eff22_Vals088_5[2] = {
#include "game/008A44E0.inc"
};
HitShape Eff22_Vals088_6[2] = {
#include "game/008A4530.inc"
};
HitShape Eff22_Vals088_7[2] = {
#include "game/008A4580.inc"
};
HitShape Eff22_Vals088_8[2] = {
#include "game/008A45D0.inc"
};
HitShape Eff22_Vals088_9[2] = {
#include "game/008A4620.inc"
};
HitShape Eff22_Vals088_10[2] = {
#include "game/008A4670.inc"
};
HitShape Eff22_Vals088_11[2] = {
#include "game/008A46C0.inc"
};
HitShape Eff22_Vals088_12[2] = {
#include "game/008A4710.inc"
};
HitShape *Eff22_Vals088[16] = {
    Eff22_Vals088_0, Eff22_Vals088_1, Eff22_Vals088_2, Eff22_Vals088_3,
    Eff22_Vals088_4, Eff22_Vals088_5, Eff22_Vals088_6, Eff22_Vals088_7,
    Eff22_Vals088_8, Eff22_Vals088_9, Eff22_Vals088_10, Eff22_Vals088_11,
    Eff22_Vals088_12, 0, 0, 0,
};
u32 D_008A47A0[60] = {
#include "game/008A47A0.inc"
};
void *Eff22_MotionRef = (void *)D_008A47A0;
f32 Eff22_Val048ByType[9][3] = {
#include "game/008A48A0.inc"
};
f32 Eff22_Val058ByType[12] = {
#include "game/008A4910.inc"
};
s16 Eff22_TimeLimit[16] = {
#include "game/008A4940.inc"
};
f32 Eff22_PlayerPtsA[2][3] = {
#include "game/008A4960.inc"
};
f32 Eff22_PlayerPtsB[2][3] = {
#include "game/008A4980.inc"
};
u16 Eff22_PlayerAngsB[4] = {
#include "game/008A4998.inc"
};
f32 Eff22_PlayerPtsC[2][3] = {
#include "game/008A49A0.inc"
};
u16 Eff22_PlayerAngsC[4] = {
#include "game/008A49B8.inc"
};
f32 Eff22_Add048A[4] = {
#include "game/008A49C0.inc"
};
f32 Eff22_SpotPosA[3][3] = {
#include "game/008A49D0.inc"
};
u16 Eff22_SpotAngA[4] = {
#include "game/008A49F8.inc"
};
f32 Eff22_SpotPosB[2][3] = {
#include "game/008A4A00.inc"
};
u16 Eff22_SpotAngB[4] = {
#include "game/008A4A18.inc"
};
f32 Eff22_SpotPosC[3][3] = {
#include "game/008A4A20.inc"
};
u16 Eff22_SpotAngC[4] = {
#include "game/008A4A48.inc"
};
f32 Eff22_Add048B[12] = {
#include "game/008A4A50.inc"
};
f32 EmBlangonga_State3PtsA[6][3] = {
#include "game/008A4A80.inc"
};
s16 EmBlangonga_State3Tabs_0[4] = {
#include "game/008A4AC8.inc"
};
s16 EmBlangonga_State3Tabs_1[8] = {
#include "game/008A4AD0.inc"
};
s16 *EmBlangonga_State3Tabs[4] = {
    EmBlangonga_State3Tabs_0, EmBlangonga_State3Tabs_1, NULL, NULL,
};
f32 EmBlangonga_State3PtsB[6][3] = {
#include "game/008A4AF0.inc"
};
s16 EmBlangonga_State3TabByZ_0[6] = {
#include "game/008A4B38.inc"
};
s16 EmBlangonga_State3TabByZ_1[6] = {
#include "game/008A4B48.inc"
};
s16 *EmBlangonga_State3TabByZ[2] = {
    EmBlangonga_State3TabByZ_0, EmBlangonga_State3TabByZ_1,
};
HitShape EffCephadrome23_Vals088_0[2] = {
#include "game/008A4B60.inc"
};
HitShape EffCephadrome23_Vals088_1[3] = {
#include "game/008A4BB0.inc"
};
HitShape EffCephadrome23_Vals088_2[3] = {
#include "game/008A4C30.inc"
};
HitShape EffCephadrome23_Vals088_3[2] = {
#include "game/008A4CB0.inc"
};
HitShape EffCephadrome23_Vals088_4[4] = {
#include "game/008A4D00.inc"
};
HitShape EffCephadrome23_Vals088_5[4] = {
#include "game/008A4DA0.inc"
};
HitShape EffCephadrome23_Vals088_6[3] = {
#include "game/008A4E40.inc"
};
HitShape EffCephadrome23_Vals088_7[3] = {
#include "game/008A4EC0.inc"
};
HitShape EffCephadrome23_Vals088_8[4] = {
#include "game/008A4F40.inc"
};
HitShape EffCephadrome23_Vals088_9[3] = {
#include "game/008A4FE0.inc"
};
HitShape EffCephadrome23_Vals088_10[2] = {
#include "game/008A5060.inc"
};
HitShape EffCephadrome23_Vals088_11[2] = {
#include "game/008A50B0.inc"
};
HitShape EffCephadrome23_Vals088_12[2] = {
#include "game/008A5100.inc"
};
HitShape EffCephadrome23_Vals088_13[2] = {
#include "game/008A5150.inc"
};
HitShape EffCephadrome23_Vals088_14[2] = {
#include "game/008A51A0.inc"
};
HitShape *EffCephadrome23_Vals088[16] = {
    EffCephadrome23_Vals088_0, EffCephadrome23_Vals088_1, EffCephadrome23_Vals088_2, EffCephadrome23_Vals088_3,
    EffCephadrome23_Vals088_4, EffCephadrome23_Vals088_5, EffCephadrome23_Vals088_6, EffCephadrome23_Vals088_7,
    EffCephadrome23_Vals088_8, EffCephadrome23_Vals088_9, EffCephadrome23_Vals088_10, EffCephadrome23_Vals088_11,
    EffCephadrome23_Vals088_12, EffCephadrome23_Vals088_13, EffCephadrome23_Vals088_14, 0,
};
u32 D_008A5230[180] = {
#include "game/008A5230.inc"
};
void *EffCephadrome23_MotionRef = (void *)D_008A5230;
HitShape BowgunShot_Vals088_0[2] = {
#include "game/008A5510.inc"
};
HitShape *BowgunShot_Vals088[4] = {
    BowgunShot_Vals088_0, 0, 0, 0,
};
u32 D_008A5570[78] = {
#include "game/008A5570.inc"
};
void *BowgunShot_MotionRef = (void *)D_008A5570;
s16 BowgunShot_ShotParams_0[4] = {
#include "game/008A56B0.inc"
};
s16 BowgunShot_ShotParams_1[4] = {
#include "game/008A56B8.inc"
};
s16 BowgunShot_ShotParams_2[4] = {
#include "game/008A56C0.inc"
};
s16 BowgunShot_ShotParams_3[6] = {
#include "game/008A56C8.inc"
};
s16 BowgunShot_ShotParams_4[4] = {
#include "game/008A56D8.inc"
};
s16 BowgunShot_ShotParams_5[4] = {
#include "game/008A56E0.inc"
};
s16 BowgunShot_ShotParams_6[6] = {
#include "game/008A56E8.inc"
};
s16 BowgunShot_ShotParams_7[6] = {
#include "game/008A56F8.inc"
};
s16 BowgunShot_ShotParams_8[4] = {
#include "game/008A5708.inc"
};
s16 BowgunShot_ShotParams_9[4] = {
#include "game/008A5710.inc"
};
s16 BowgunShot_ShotParams_10[4] = {
#include "game/008A5718.inc"
};
s16 BowgunShot_ShotParams_11[8] = {
#include "game/008A5720.inc"
};
s16 *BowgunShot_ShotParams[12] = {
    BowgunShot_ShotParams_0, BowgunShot_ShotParams_1, BowgunShot_ShotParams_2, BowgunShot_ShotParams_3,
    BowgunShot_ShotParams_4, BowgunShot_ShotParams_5, BowgunShot_ShotParams_6, BowgunShot_ShotParams_7,
    BowgunShot_ShotParams_8, BowgunShot_ShotParams_9, BowgunShot_ShotParams_10, BowgunShot_ShotParams_11,
};
u16 BowgunShot_SpreadScale[8] = {
#include "game/008A5760.inc"
};
s16 BowgunShot_SpreadDirs[8] = {
#include "game/008A5770.inc"
};
Rec8A5780 BowgunShot_Kinds[11] = {
#include "game/008A5780.inc"
};
s16 BowgunShot_EventTbls_0[8] = {
#include "game/008A58E0.inc"
};
s16 BowgunShot_EventTbls_1[8] = {
#include "game/008A58F0.inc"
};
s16 BowgunShot_EventTbls_2[8] = {
#include "game/008A5900.inc"
};
s16 *BowgunShot_EventTbls[4] = {
    BowgunShot_EventTbls_0, BowgunShot_EventTbls_1, BowgunShot_EventTbls_2, NULL,
};
f32 BowgunShot_KindScaleA[4] = {
#include "game/008A5920.inc"
};
f32 BowgunShot_KindScaleB[4] = {
#include "game/008A5930.inc"
};
f32 BowgunShot_KindBase[4] = {
#include "game/008A5940.inc"
};
f32 BowgunShot_KindTimes[4][6] = {
#include "game/008A5950.inc"
};
u8 BowgunShot_LevelScale[16] = {
#include "game/008A59B0.inc"
};
s16 BowgunShot_Kind2Val06F[8] = {
#include "game/008A59C0.inc"
};
f32 BowgunShot_Kind2Scale[6] = {
#include "game/008A59D0.inc"
};
s16 BowgunShot_Kind2Val06B[6] = {
#include "game/008A59E8.inc"
};
HitShape EffGaruga25_Vals088_0[2] = {
#include "game/008A5A00.inc"
};
HitShape EffGaruga25_Vals088_1[3] = {
#include "game/008A5A50.inc"
};
HitShape EffGaruga25_Vals088_2[3] = {
#include "game/008A5AD0.inc"
};
HitShape EffGaruga25_Vals088_3[3] = {
#include "game/008A5B50.inc"
};
HitShape EffGaruga25_Vals088_4[4] = {
#include "game/008A5BD0.inc"
};
HitShape EffGaruga25_Vals088_5[4] = {
#include "game/008A5C70.inc"
};
HitShape EffGaruga25_Vals088_6[2] = {
#include "game/008A5D10.inc"
};
HitShape EffGaruga25_Vals088_7[2] = {
#include "game/008A5D60.inc"
};
HitShape EffGaruga25_Vals088_8[3] = {
#include "game/008A5DB0.inc"
};
HitShape EffGaruga25_Vals088_9[3] = {
#include "game/008A5E30.inc"
};
HitShape EffGaruga25_Vals088_10[2] = {
#include "game/008A5EB0.inc"
};
HitShape EffGaruga25_Vals088_11[2] = {
#include "game/008A5F00.inc"
};
HitShape EffGaruga25_Vals088_12[2] = {
#include "game/008A5F50.inc"
};
HitShape EffGaruga25_Vals088_13[2] = {
#include "game/008A5FA0.inc"
};
HitShape EffGaruga25_Vals088_14[4] = {
#include "game/008A5FF0.inc"
};
HitShape EffGaruga25_Vals088_15[2] = {
#include "game/008A6090.inc"
};
HitShape EffGaruga25_Vals088_16[2] = {
#include "game/008A60E0.inc"
};
HitShape EffGaruga25_Vals088_17[2] = {
#include "game/008A6130.inc"
};
HitShape EffGaruga25_Vals088_18[4] = {
#include "game/008A6180.inc"
};
HitShape EffGaruga25_Vals088_19[2] = {
#include "game/008A6220.inc"
};
HitShape EffGaruga25_Vals088_20[6] = {
#include "game/008A6270.inc"
};
HitShape EffGaruga25_Vals088_21[5] = {
#include "game/008A6360.inc"
};
HitShape EffGaruga25_Vals088_22[4] = {
#include "game/008A6430.inc"
};
HitShape *EffGaruga25_Vals088[24] = {
    EffGaruga25_Vals088_0, EffGaruga25_Vals088_1, EffGaruga25_Vals088_2, EffGaruga25_Vals088_3,
    EffGaruga25_Vals088_4, EffGaruga25_Vals088_5, EffGaruga25_Vals088_6, EffGaruga25_Vals088_7,
    EffGaruga25_Vals088_8, EffGaruga25_Vals088_9, EffGaruga25_Vals088_10, EffGaruga25_Vals088_11,
    EffGaruga25_Vals088_12, EffGaruga25_Vals088_13, EffGaruga25_Vals088_14, EffGaruga25_Vals088_15,
    EffGaruga25_Vals088_16, EffGaruga25_Vals088_17, EffGaruga25_Vals088_18, EffGaruga25_Vals088_19,
    EffGaruga25_Vals088_20, EffGaruga25_Vals088_21, EffGaruga25_Vals088_22, 0,
};
u32 D_008A6530[240] = {
#include "game/008A6530.inc"
};
void *EffGaruga25_MotionRef = (void *)D_008A6530;
HitShape EffHermitaur26_Vals088_0[4] = {
#include "game/008A6900.inc"
};
HitShape EffHermitaur26_Vals088_1[4] = {
#include "game/008A69A0.inc"
};
HitShape EffHermitaur26_Vals088_2[7] = {
#include "game/008A6A40.inc"
};
HitShape EffHermitaur26_Vals088_3[4] = {
#include "game/008A6B60.inc"
};
HitShape EffHermitaur26_Vals088_4[4] = {
#include "game/008A6C00.inc"
};
HitShape EffHermitaur26_Vals088_5[2] = {
#include "game/008A6CA0.inc"
};
HitShape EffHermitaur26_Vals088_6[8] = {
#include "game/008A6CF0.inc"
};
HitShape EffHermitaur26_Vals088_7[9] = {
#include "game/008A6E30.inc"
};
HitShape EffHermitaur26_Vals088_8[2] = {
#include "game/008A6FA0.inc"
};
HitShape EffHermitaur26_Vals088_9[5] = {
#include "game/008A6FF0.inc"
};
HitShape EffHermitaur26_Vals088_10[5] = {
#include "game/008A70C0.inc"
};
HitShape EffHermitaur26_Vals088_11[9] = {
#include "game/008A7190.inc"
};
HitShape EffHermitaur26_Vals088_12[2] = {
#include "game/008A7300.inc"
};
HitShape *EffHermitaur26_Vals088[16] = {
    EffHermitaur26_Vals088_0, EffHermitaur26_Vals088_1, EffHermitaur26_Vals088_2, EffHermitaur26_Vals088_3,
    EffHermitaur26_Vals088_4, EffHermitaur26_Vals088_5, EffHermitaur26_Vals088_6, EffHermitaur26_Vals088_7,
    EffHermitaur26_Vals088_8, EffHermitaur26_Vals088_9, EffHermitaur26_Vals088_10, EffHermitaur26_Vals088_11,
    EffHermitaur26_Vals088_12, 0, 0, 0,
};
u32 D_008A7390[204] = {
#include "game/008A7390.inc"
};
void *EffHermitaur26_MotionRef = (void *)D_008A7390;
HitShape EffBlangonga27_Vals088_0[3] = {
#include "game/008A76D0.inc"
};
HitShape EffBlangonga27_Vals088_1[4] = {
#include "game/008A7750.inc"
};
HitShape EffBlangonga27_Vals088_2[2] = {
#include "game/008A77F0.inc"
};
HitShape EffBlangonga27_Vals088_3[2] = {
#include "game/008A7840.inc"
};
HitShape EffBlangonga27_Vals088_4[2] = {
#include "game/008A7890.inc"
};
HitShape EffBlangonga27_Vals088_5[2] = {
#include "game/008A78E0.inc"
};
HitShape EffBlangonga27_Vals088_6[3] = {
#include "game/008A7930.inc"
};
HitShape EffBlangonga27_Vals088_7[2] = {
#include "game/008A79B0.inc"
};
HitShape EffBlangonga27_Vals088_8[4] = {
#include "game/008A7A00.inc"
};
HitShape EffBlangonga27_Vals088_9[4] = {
#include "game/008A7AA0.inc"
};
HitShape EffBlangonga27_Vals088_10[3] = {
#include "game/008A7B40.inc"
};
HitShape EffBlangonga27_Vals088_11[3] = {
#include "game/008A7BC0.inc"
};
HitShape EffBlangonga27_Vals088_12[2] = {
#include "game/008A7C40.inc"
};
HitShape EffBlangonga27_Vals088_13[2] = {
#include "game/008A7C90.inc"
};
HitShape EffBlangonga27_Vals088_14[2] = {
#include "game/008A7CE0.inc"
};
HitShape EffBlangonga27_Vals088_15[2] = {
#include "game/008A7D30.inc"
};
HitShape EffBlangonga27_Vals088_16[3] = {
#include "game/008A7D80.inc"
};
HitShape EffBlangonga27_Vals088_17[2] = {
#include "game/008A7E00.inc"
};
HitShape EffBlangonga27_Vals088_18[4] = {
#include "game/008A7E50.inc"
};
HitShape EffBlangonga27_Vals088_19[2] = {
#include "game/008A7EF0.inc"
};
HitShape EffBlangonga27_Vals088_20[2] = {
#include "game/008A7F40.inc"
};
HitShape EffBlangonga27_Vals088_21[4] = {
#include "game/008A7F90.inc"
};
HitShape EffBlangonga27_Vals088_22[2] = {
#include "game/008A8030.inc"
};
HitShape EffBlangonga27_Vals088_23[2] = {
#include "game/008A8080.inc"
};
HitShape EffBlangonga27_Vals088_24[3] = {
#include "game/008A80D0.inc"
};
HitShape EffBlangonga27_Vals088_25[4] = {
#include "game/008A8150.inc"
};
HitShape *EffBlangonga27_Vals088[28] = {
    EffBlangonga27_Vals088_0, EffBlangonga27_Vals088_1, EffBlangonga27_Vals088_2, EffBlangonga27_Vals088_3,
    EffBlangonga27_Vals088_4, EffBlangonga27_Vals088_5, EffBlangonga27_Vals088_6, EffBlangonga27_Vals088_7,
    EffBlangonga27_Vals088_8, EffBlangonga27_Vals088_9, EffBlangonga27_Vals088_10, EffBlangonga27_Vals088_11,
    EffBlangonga27_Vals088_12, EffBlangonga27_Vals088_13, EffBlangonga27_Vals088_14, EffBlangonga27_Vals088_15,
    EffBlangonga27_Vals088_16, EffBlangonga27_Vals088_17, EffBlangonga27_Vals088_18, EffBlangonga27_Vals088_19,
    EffBlangonga27_Vals088_20, EffBlangonga27_Vals088_21, EffBlangonga27_Vals088_22, EffBlangonga27_Vals088_23,
    EffBlangonga27_Vals088_24, EffBlangonga27_Vals088_25, 0, 0,
};
u32 D_008A8260[252] = {
#include "game/008A8260.inc"
};
void *EffBlangonga27_MotionRef = (void *)D_008A8260;
HitShape D_008A8660[2] = {
#include "game/008A8660.inc"
};
HitShape *Eff28_Val088 = D_008A8660;
u32 D_008A86C0[12] = {
#include "game/008A86C0.inc"
};
void *Eff28_MotionRef = (void *)D_008A86C0;
