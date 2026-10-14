/*
 * Small objects of the field, and the player's motion-frame sound helpers.
 *
 * - Motion-frame sound effects (PlayerSe_AtFrame to PlayerSe_EquipAtFrame, called
 *   hundreds of times by the player actions in text_3885D0): when the
 *   motion of layer 0 reaches a frame (Motion_PassedFrame), play a sound effect
 *   at the player's position through the boot ELF's SE calls: bank 1 (the
 *   common SEs, Sound_PlayPlayerSe), the player's weapon bank (Player_PlaySe:
 *   bank 6 when p->unk010, else 2 + the player index), bank 2 online, the
 *   voice bank 7 (2 * voice + a random bit, with the player's unk661), or
 *   the equipment sounds (Sound_PlayArmorRattle).
 * - The season banner (object kind 0x12, SeasonBanner_Create/SeasonBanner_CreateOnline): the
 *   season's name (gSeasonNames: warm, cold, breeding season) and, offline,
 *   day or night (gDaytimeNames), centred at y 0xAF over a row of 15 tiles in
 *   the season's colour, faded in and out. Offline the season and time
 *   come from the boot ELF's Season_Get/Daytime_Get; online the season is
 *   given (unk0A) and the banner ends when it changes (LobbyWorld_GetSeason).
 * - An item object (kind 0x15, ThrownObj_Spawn/ThrownObj_Spawn2) held between
 *   bones 6 and 9 of a player until its motion 0x432 reaches frame 48, then
 *   thrown to a per-stage target in 10 frames, where it stays 300 frames.
 * - Per-stage helpers on the stage id (scene +0x14, gScene.area).
 */
#include "common.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"
#include "em.h"
#include "capsule.h"
#include "obj.h"
#include "sub_main/unk_386160.h"   /* the texts this file uses (tools/content_gen.py) */


/* An entry of the 256 draw slots at 0x6A3AD0 (0x20 bytes). */
/* Velocity of a thrown item. */
typedef struct Particle {
    f32 vel[3];                 /* 0x00 */
    s16 unkC;                   /* 0x0C: angle speed */
} Particle;

/* A textured rectangle for Sprite_Draw (0x14 bytes). */


/* Texture scroll state: the camera's last angle and position, then one
 * UV offset per layer. */
typedef struct UvLayer {
    f32 u;                      /* 0x0 */
    f32 v;                      /* 0x4 */
    f32 unk8;
} UvLayer;

typedef struct UvScroll {
    u16 unk00;                  /* 0x00 */
    u16 angle;                  /* 0x02 */
    f32 pos[3];                 /* 0x04 */
    UvLayer layer[1];           /* 0x10 */
} UvScroll;

typedef struct VoiceEntry {
    u8 unk0[8];
    u8 unk8;
    u8 unk9[3];
} VoiceEntry;

typedef struct VoiceRow {
    VoiceEntry e[11];
} VoiceRow;

/* Boot ELF. */
s32 sprintf(char *dst, char *fmt, ...);     /* sprintf */
s32 Gs_SetState(u8 reg, u32 v);
s32 Sprite_Draw(Sprite *s);
s32 Season_Get(void);          /* the season, 0-2 */
u8 Daytime_Get(void);          /* 1: night */
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
void Sound_PlayArmorRattle(Player *p, s32 kind);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);

/* sub_main. */
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
s32 Player_PassedFrameScaled(void *obj, f32 frame, s32 layer);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPal(s32 a);
s32 Str_Length(char *s);
s32 Model_Draw(u32 h, s32 flags);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_GetTrans(f32 *out, f32 *m);
void Mtx_RotateY(f32 *m, f32 angle);
void Vec_RotateY(f32 *v, f32 angle);
void Vec_Copy(f32 *dst, f32 *src);
f32 Math_Floor(f32 x);
void Tex_Prepare(s32 a, s32 b);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
void Draw_ApplyStates(u32 f);
void Draw_SetDefaultState(void);
Obj *Obj_Alloc(s32 n);
void Obj_Free(Obj *p);
u16 Rand_Next(s32 stream);
void Tex_Bind(s32 a);
void Text_DrawShadow2(s16 x, s16 y, s32 c1, s32 c2, char *s);
void Text_SetSize(s32 size);
DrawSlot *DrawSlot_Get(s32 i);
DrawSlot *DrawSlotB_Get(s32 i);
s16 DrawSlot_Alloc(void);
s16 DrawSlotB_Alloc(void);
void DrawSlot_Free(s32 id);
void DrawSlotB_Free(s32 id);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Player_IsLocal(void *obj);
s32 Obj_InCurrentArea(void *obj);
void Capsule_Copy(HitCapsule *src, Capsule *dst);
s32 Capsule_PushSphere(Capsule *cap, f32 *p, f32 *out, f32 r);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
void Em_BonePos(void *obj, s16 bone, f32 *out);

/* game overlay. */
s32 HairMenu_IsOpen(void);
s32 func_7BC2F0(void);
s32 LobbyWorld_GetSeason(void);

/* This file. */
void SeasonBanner_Update(Obj *o);
void SeasonBanner_Init(Obj *o);
void SeasonBanner_Run(Obj *o);
void SeasonBanner_End(Obj *o);
void SeasonBanner_Free(Obj *o);
void SeasonBanner_Draw(DrawSlot *s);
void Obj_PlaceBetweenBones(Obj *o, void *arg);
void Obj_AreaPosition(Obj *o, f32 *out);
void Obj_AreaAngle(Obj *o, s16 *angle);
s16 ThrownObj_AreaModel(Obj *o);
void ThrownObj_Launch(Obj *o, Particle *out);
static void ThrownObj_Fly(Obj *o, Particle *v);
void ThrownObj_Task(Obj *o);
void ThrownObj_Start(Obj *o);
void ThrownObj_Run(Obj *o);
void ThrownObj_End(Obj *o);
void ThrownObj_Free(Obj *o);
void ThrownObj_Draw(DrawSlot *s);

extern char *gSeasonNames[];       /* season names */
extern char *gDaytimeNames[];       /* day, night */
extern f32 *gAreaBlockSpheres[];       /* per stage: wall points, -1.0f ends */
extern f32 D_00449ED0[];
extern f32 D_00449EE0[];
extern f32 D_00449EF0[];
extern s16 *D_00449F48[];
extern s16 gSeasonIconUVs[][2];
extern s16 D_00449F82[][2];
extern s16 gSeasonIconUVsOnline[][2];
extern s16 D_00449F92[][2];
extern f32 gThrownObjPos56[][3];
extern s16 gThrownObjModel56[];
extern f32 gThrownObjPosAD[][3];
extern s16 gThrownObjAngleAD[];
extern s16 gThrownObjModelAD[];
extern f32 gThrownObjPosAE[][3];
extern s16 gThrownObjAngleAE[];
extern s16 gThrownObjModelAE[];
extern f32 gThrownObjPosAF[][3];
extern s16 gThrownObjAngleAF[];
extern s16 gThrownObjModelAF[];
extern f32 gThrownObjDists[];
extern f32 gThrownObjPosK1[];
extern s16 gThrownObjModelK1AD;
extern s16 gThrownObjModelK1AE;
extern s16 gThrownObjModelK1AF;
extern VoiceRow gEmWallContacts[];
extern f32 gCamMatrix[];         /* the camera matrix? */
extern u8 gCamDir[];
extern f32 gCamPos;          /* a position (x, y, z) */
extern f32 D_006A9DE4;
extern f32 D_006A9DE8;
extern s16 gThrownObjAngle56[];

void Vec_FromCamera(f32 *out, f32 *v, f32 s)
{
    f32 x, y, z;

    x = s * v[0];
    y = s * v[1];
    z = s * v[2];
    out[0] = gCamPos + x;
    out[1] = D_006A9DE4 + y;
    out[2] = D_006A9DE8 + z;
}

/* 1 when the segment from gCamPos to pos (radius 5) meets one of the current area spheres in gAreaBlockSpheres (radius, x, y, z; -1 ends). */
s32 Area_LineBlocked(f32 *pos)
{
    f32 p[3];
    f32 q[4];
    HitCapsule cap;
    u8 work[0x40];
    f32 *pt;
    f32 key;

    Vec_Copy(cap.a, &gCamPos);
    Vec_Copy(cap.b, pos);
    cap.radius = 5.0f;
    Capsule_Copy(&cap, (Capsule *) work);
    pt = gAreaBlockSpheres[gScene.area];
    if (pt != NULL) {
        for (key = *pt; key != -1.0f; key = *pt) {
            pt++;
            p[0] = *pt++;
            p[1] = *pt++;
            p[2] = *pt++;
            if (Capsule_PushSphere((Capsule *) work, p, q, key)) {
                return 1;
            }
        }
    }
    return 0;
}

/* In stages 0x2F/0x8D and 0x33/0x91: 0 when the player is inside the near
 * limits, 1 beyond the far ones, 2 in between (0 in other stages). */
s16 Area_BoundaryZone(Player *p)
{
    f32 g, e, d, c, b, a;
    u8 kind;

    switch (gScene.area) {
    case 0x2F:
    case 0x8D:
        c = 17800.0f;
        e = 2200.0f;
        b = 17200.0f;
        d = 2800.0f;
        kind = 1;
        break;
    case 0x33:
    case 0x91:
        g = 4500.0f;
        c = 18000.0f;
        a = 5100.0f;
        b = 17400.0f;
        kind = 0;
        break;
    default:
        return 0;
    }
    switch (kind) {
    default:
        if (p->pos[0] > a && p->pos[0] < b) {
            return 0;
        }
        if (p->pos[0] < g || p->pos[0] > c) {
            return 1;
        }
        break;
    case 1:
        if (p->pos[0] < b && p->pos[2] > d) {
            return 0;
        }
        if (p->pos[0] > c || p->pos[2] < e) {
            return 1;
        }
        break;
    }
    return 2;
}

void UvScroll_Update(UvScroll *h, s16 n, s16 *speed, u16 mode)
{
    f32 v[3];
    u16 t1;
    u16 t0;
    f32 du;
    f32 dv;
    s16 i;

    Mtx_GetTrans(v, gCamMatrix);
    Vec_DirToAngles((f32 *) gCamDir, (s16 *) &t1, (s16 *) &t0);
    du = (s16)(t0 - h->angle);
    du *= 6.964391650399193e-05f;
    dv = v[1] - h->pos[1];
    dv *= 0.001f;
    h->unk00 = t1;
    h->angle = t0;
    Vec_Copy(h->pos, v);
    for (i = 0; i < n; i++) {
        h->layer[i].u += du;
        h->layer[i].v += dv;
        if (speed != NULL && speed[i] != 0) {
            switch (mode) {
            case 0:
                break;
            case 1:
                h->layer[i].u += 1.0f / speed[i];
                break;
            case 2:
                h->layer[i].v += 1.0f / speed[i];
                break;
            }
        }
        if (h->layer[i].u > 1.0f || h->layer[i].u < 0.0f) {
            h->layer[i].u -= Math_Floor(h->layer[i].u);
        }
        if (h->layer[i].v > 1.0f || h->layer[i].v < 0.0f) {
            h->layer[i].v -= Math_Floor(h->layer[i].v);
        }
    }
}

f32 AmbientSound_AreaValue10(u8 a, s16 b)
{
    switch (gScene.area) {
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x09:
    case 0x12:
    case 0x13:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9C:
    case 0x9D:
    case 0x9E:
    case 0xA0:
        switch (b) {
        case 0:
            switch (a) {
            case 0:
            case 1:
                return 0.0f;
            }
            break;
        case 1:
            switch (a) {
            case 0:
                return 0.0f;
            case 1:
                return 0.45f;
            }
            break;
        case 2:
            switch (a) {
            case 1:
                return 0.9f;
            }
            break;
        }
        break;
    case 0x60:
    case 0x61:
    case 0x64:
    case 0x69:
    case 0x6A:
    case 0x6D:
        switch (b) {
        case 0:
            switch (a) {
            case 0:
            case 1:
                return 0.0f;
            }
            break;
        case 1:
            switch (a) {
            case 0:
                return 0.1f;
            case 1:
                return 0.2f;
            }
            break;
        case 2:
            switch (a) {
            case 0:
                return 1.0f;
            case 1:
                return 0.45f;
            }
            break;
        }
        break;
    case 0x10:
    case 0x1D:
    case 0x2C:
    case 0x43:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x4B:
        switch (b) {
        case 0:
            switch (a) {
            case 0:
                return 0.0f;
            case 1:
                return 0.1f;
            }
            break;
        case 1:
            switch (a) {
            case 0:
                return 0.1f;
            case 1:
                return 0.2f;
            }
            break;
        case 2:
            switch (a) {
            case 0:
                return 0.5f;
            case 1:
                return 0.4f;
            }
            break;
        }
        break;
    default:
        return 1.0f;
    }
    return 1.0f;
}

f32 AmbientSound_AreaValue0C(u8 a, s16 b)
{
    switch (gScene.area) {
    case 0x60:
    case 0x61:
    case 0x64:
    case 0x69:
    case 0x6A:
    case 0x6D:
        switch (b) {
        case 1:
            return D_00449EE0[a];
        default:
        case 0:
            return D_00449ED0[a];
        case 2:
            return D_00449EF0[a];
        }
    case 0x10:
    case 0x1D:
    case 0x2C:
    case 0x43:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x4B:
        switch (a) {
        case 0:
            return 0.0f;
        default:
        case 1:
            return 1.0f / D_00449F48[b][a];
        }
    }
    return 0.0f;
}

void SeasonBanner_Create(void)
{
    Obj *o = Obj_Alloc(0);

    if (o != NULL) {
        o->kind = 0x12;
        o->type = 0;
        o->keep = 1;
        o->update = SeasonBanner_Update;
    }
}

void SeasonBanner_CreateOnline(u8 a, u8 b)
{
    Obj *o = Obj_Alloc(0);

    if (o != NULL) {
        o->kind = 0x12;
        o->type = 1;
        o->keep = 1;
        o->var3 = a;
        o->timer = b;
        o->update = SeasonBanner_Update;
    }
}

void SeasonBanner_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        SeasonBanner_Init(o);
        break;
    case 1:
        SeasonBanner_Run(o);
        break;
    case 2:
        SeasonBanner_End(o);
        break;
    case 3:
        SeasonBanner_Free(o);
        break;
    }
}

void SeasonBanner_Init(Obj *o)
{
    o->state++;
    o->var1 = 0;
    o->var2 = 0;
    o->visible = 1;
    o->pos[0] = 0.0f;
    o->pos[1] = 0.0f;
    o->pos[2] = 0.0f;
    o->timer = 8;
    o->var5 = 0;
    o->draw = NULL;
    o->slot = DrawSlotB_Alloc();
    o->ds = DrawSlotB_Get(o->slot);
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = SeasonBanner_Draw;
        SeasonBanner_Run(o);
    } else {
        Obj_Free(o);
    }
}

void SeasonBanner_Run(Obj *o)
{
    o->var5++;
    if (o->type == 0) {
        if (gScene.eventRunning != 0) {
            return;
        }
    } else if (gScene.online != 0) {
        if (func_7BC2F0() == 1) {
            o->state++;
            return;
        }
        if ((u8)o->var3 != (u8)LobbyWorld_GetSeason()) {
            o->state++;
            return;
        }
    }
    switch (o->var0) {
    case 0:
        o->var2 = 0;
        if (--o->timer <= 0) {
            o->var0++;
            o->timer = 0x10;
        }
        break;
    case 1:
        if (--o->timer <= 0) {
            o->var0++;
            o->timer = 4;
            o->var2 = 0xFF;
        } else {
            o->var2 = (0x10 - o->timer) << 4;
        }
        break;
    case 2:
        if (--o->timer <= 0) {
            o->var0++;
            o->timer = 0x14;
        }
        break;
    case 3:
        o->var1++;
        if (--o->timer <= 0) {
            o->var0++;
            o->timer = 0x10;
        }
        break;
    case 4:
        o->var1++;
        if (--o->timer <= 0) {
            o->state++;
        }
        o->var2 = o->timer << 4;
        break;
    }
    if (o->type == 0) {
        OT_AddByDepth(gDrawListText0b, o->ds, 1, 1);
    } else {
        OT_AddByDepth(gDrawListText4, o->ds, 1, 1);
    }
}

void SeasonBanner_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlotB_Free(o->slot);
}

void SeasonBanner_Free(Obj *o)
{
    Obj_Free(o);
}

/* Draws the season banner: the season's name (and, offline, day or night) centred at y 0xAF,
 * then 15 pairs of tiles of the season's icon (o->unk06 apart), tinted by season with alpha
 * o->unk07.
 * FAKEMATCH: n, an s32 copy of the season taken again in each texture branch, is what the game
 * keeps for the table reads and the switch (it masks the season once per branch and switches on
 * that value); with idx itself in the reads and the switch it is 92-130 words off, and a static
 * inline for the two reads (its parameter as the per-branch copy) leaves the switch 92 off. */
void SeasonBanner_Draw(DrawSlot *s)
{
    Obj *o = s->owner;
    Sprite spr;
    char buf[0x40];
    s16 i;
    u8 idx;
    s32 x;
    s32 n;

    if (o->type == 0) {
        Text_SetSize(0);
        idx = Season_Get();
        sprintf(buf, TXT(0x0049FCA0) /* two names with a full-width space between them */, gSeasonNames[idx], gDaytimeNames[(u8)Daytime_Get()]);
    } else {
        Text_SetSize(4);
        idx = o->var3;
        sprintf(buf, "%s", gSeasonNames[idx]);
    }
    Font_SetSpacing(0x1C, 0x1C);
    Font_SetPal(0);
    Text_DrawShadow2((0x280 - Str_Length(buf) * 14) / 2, 0xAF, 1, 0, buf);
    if (o->type == 0) {
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        n = idx;
        spr.u0 = gSeasonIconUVs[n][0];
        spr.v0 = D_00449F82[n][0];
    } else {
        Tex_Prepare(1, 0x155);
        Tex_Bind(0x155);
        n = idx;
        spr.u0 = gSeasonIconUVsOnline[n][0];
        spr.v0 = D_00449F92[n][0];
    }
    spr.u1 = spr.u0 + 14;
    spr.v1 = spr.v0 + 16;
    spr.y = 0xD9;
    spr.w = 0x20;
    spr.h = 0x20;
    switch (n) {
    case 0:
        spr.color = (o->var2 << 24) | 0xC6AB20;
        break;
    case 1:
        spr.color = (o->var2 << 24) | 0x53ACCC;
        break;
    case 2:
        spr.color = (o->var2 << 24) | 0x398C34;
        break;
    }
    for (i = 0, x = 4; i < 15; i++, x += 0x30) {
        spr.x = x;
        Sprite_Draw(&spr);
        spr.x = o->var1 + x;
        Sprite_Draw(&spr);
    }
}

void ThrownObj_Spawn(void *arg, s32 kind, s32 n)
{
    Obj *o = Obj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x15;
        o->update = ThrownObj_Task;
        o->arg = arg;
        o->type = kind;
        o->var4 = 0;
        o->fvar = 1.0f;
        o->var2 = n;
        if (o->var2 > 4) {
            o->var2 = 4;
        }
    }
}

void ThrownObj_Spawn2(void *arg, s32 kind)
{
    Obj *o = Obj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x15;
        o->update = ThrownObj_Task;
        o->arg = arg;
        o->type = kind;
        o->var4 = 1;
        o->fvar = 0.0f;
        o->var2 = 0;
    }
}

/* Places o between bones 6 and 9 of m (2 above them; the height is set
 * twice, as in the original). */
void Obj_PlaceBetweenBones(Obj *o, void *m)
{
    f32 a[4];
    f32 b[4];

    Em_BonePos(m, 6, a);
    Em_BonePos(m, 9, b);
    o->pos[0] = (a[0] + b[0]) / 2.0f;
    switch (o->var4) {
    case 0:
    default:
        o->pos[1] = 2.0f + (a[1] + b[1]) / 2.0f;
        break;
    case 1:
        o->pos[1] = 20.0f + (a[1] + b[1]) / 2.0f;
        break;
    }
    o->pos[1] = 2.0f + (a[1] + b[1]) / 2.0f;
    o->pos[2] = (a[2] + b[2]) / 2.0f;
    o->var3 = *(s32 *)((u8 *)m + 0xA4);
}

/* The position of o in the current area: its entry in the area table,
 * moved along its angle (a fixed position for kind 1). */
void Obj_AreaPosition(Obj *o, f32 *out)
{
    f32 v[3];
    s16 angle;
    f32 *src;

    switch (o->var4) {
    default:
        switch (gScene.area) {
        case 0x56:
        case 0x57:
            src = gThrownObjPos56[o->type];
            break;
        case 0xAD:
            src = gThrownObjPosAD[o->type];
            break;
        case 0xAE:
            src = gThrownObjPosAE[o->type];
            break;
        case 0xAF:
            src = gThrownObjPosAF[o->type];
            break;
        }
        Vec_Copy(out, src);
        Obj_AreaAngle(o, &angle);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = gThrownObjDists[o->var2];
        Vec_RotateY(v, (6.2831855f * angle) / 65536.0f);
        out[0] += v[0];
        out[1] += v[1];
        out[2] += v[2];
        break;
    case 1:
        Vec_Copy(out, gThrownObjPosK1);
        break;
    }
}

/* The angle of o in the current area: its entry in the area table, turned
 * half a turn (a fixed angle for kind 1). */
void Obj_AreaAngle(Obj *o, s16 *angle)
{
    switch (o->var4) {
    default:
        switch (gScene.area) {
        case 0x56:
        case 0x57:
            *angle = gThrownObjAngle56[o->type];
            break;
        case 0xAD:
            *angle = gThrownObjAngleAD[o->type];
            break;
        case 0xAE:
            *angle = gThrownObjAngleAE[o->type];
            break;
        case 0xAF:
            *angle = gThrownObjAngleAF[o->type];
            break;
        }
        *angle += -0x8000;
        break;
    case 1:
        *angle = -0x6F5B;
        break;
    }
}

/* The area sub-model (gAreaSubModels entry) of the thrown object o in the current area (0 in other areas). */
s16 ThrownObj_AreaModel(Obj *o)
{
    switch (o->var4) {
    default:
        switch (gScene.area) {
        default:
            return 0;
        case 0x56:
        case 0x57:
            return gThrownObjModel56[o->var2];
        case 0xAD:
            return gThrownObjModelAD[o->var2];
        case 0xAE:
            return gThrownObjModelAE[o->var2];
        case 0xAF:
            return gThrownObjModelAF[o->var2];
        }
    case 1:
        switch (gScene.area) {
        default:
            return 0;
        case 0xAD:
            return gThrownObjModelK1AD;
        case 0xAE:
            return gThrownObjModelK1AE;
        case 0xAF:
            return gThrownObjModelK1AF;
        }
    }
}

void ThrownObj_Launch(Obj *o, Particle *out)
{
    f32 v[3];
    s16 angle;

    Obj_AreaPosition(o, v);
    Obj_AreaAngle(o, &angle);
    out->vel[0] = (v[0] - o->pos[0]) / 10.0f;
    out->vel[1] = (v[1] - o->pos[1]) / 10.0f - -10.0f;
    out->vel[2] = (v[2] - o->pos[2]) / 10.0f;
    out->unkC = (angle - o->var3) / 10.0f;
}

static void ThrownObj_Fly(Obj *o, Particle *v)
{
    o->pos[0] += v->vel[0];
    o->pos[1] += v->vel[1];
    o->pos[2] += v->vel[2];
    v->vel[1] += -2.0f;
    o->var3 += v->unkC;
}

void ThrownObj_Task(Obj *o)
{
    switch (o->state) {
    case 0:
        ThrownObj_Start(o);
        break;
    case 1:
        ThrownObj_Run(o);
        break;
    case 2:
        ThrownObj_End(o);
        break;
    case 3:
        ThrownObj_Free(o);
        break;
    }
}

void ThrownObj_Start(Obj *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->var0 = 0;
    o->var1 = 0;
    o->timer = 0;
    Obj_PlaceBetweenBones(o, o->arg);
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = ThrownObj_Draw;
    } else {
        Obj_Free(o);
    }
}

void ThrownObj_Run(Obj *o)
{
    struct Particle *pt = o->buf;
    Player *p = o->arg;

    switch (o->var0) {
    case 0:
        if (p->active == 0) {
            o->state++;
            return;
        }
        switch (o->var4) {
        case 0:
            break;
        case 1:
            o->fvar += 0.06666667f;
            break;
        }
        if (o->fvar > 1.0f) {
            o->fvar = 1.0f;
        }
        Obj_PlaceBetweenBones(o, o->arg);
        if (p->motionId[0] == 0x432 && Player_PassedFrameScaled(p, 48.0f, 0)) {
            o->fvar = 1.0f;
            ThrownObj_Launch(o, pt);
            o->timer = 0;
            o->var0++;
        }
        break;
    case 1:
        ThrownObj_Fly(o, pt);
        if (++o->timer >= 10) {
            Obj_AreaPosition(o, o->pos);
            Obj_AreaAngle(o, &o->var3);
            Effect0D_SpawnAt(o->pos, 7, 0.8f);
            o->timer = 0;
            o->var0++;
        }
        break;
    case 2:
        switch (o->var4) {
        default:
            if (++o->timer >= 300) {
                o->state++;
                return;
            }
            break;
        case 1:
            if (HairMenu_IsOpen() == 0) {
                o->state++;
                return;
            }
            break;
        }
        break;
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void ThrownObj_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlot_Free(o->slot);
}

void ThrownObj_Free(Obj *o)
{
    Obj_Free(o);
}

void ThrownObj_Draw(DrawSlot *s)
{
    f32 m[16];
    Model *set = gAreaSubModels[0];
    Obj *o = s->owner;
    ModelMatSet *e;

    if (set != NULL && set->active) {
        Mtx_Scale(m, o->fvar, o->fvar, o->fvar);
        Mtx_SetTrans(m, s->pos[0], s->pos[1], s->pos[2]);
        Mtx_RotateY(m, (6.2831855f * o->var3) / 65536.0f);
        Gs_SetState(0x60, 0x80);
        Gs_SetState(0x1A, (u32)m);
        e = &set->matSets[ThrownObj_AreaModel(o)];
        if (e != NULL && e->id != -1) {
            Draw_ApplyStates(e->flags);
            Model_Draw(e->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void PlayerSe_AtFrame(Player *p, s32 frame, s32 se)
{
    if (Motion_PassedFrame(p, 0, frame)) {
        Sound_PlayPlayerSe(p, se, 0, p->pos, 1, 0);
    }
}

void Motion_SeAtFrame(Player *p, s32 frame, s32 se)
{
    if (Motion_PassedFrame(p, 0, frame)) {
        Sound_PlayPlayerSe(p, se, 0, p->pos, 0x17, 0);
    }
}

void Motion_SeAtFrameLocal(Player *p, s32 frame, s32 se)
{
    if (Player_IsLocal(p) == 0 && (p->index & 1)) {
        return;
    }
    if (Motion_PassedFrame(p, 0, frame)) {
        Sound_PlayPlayerSe(p, se, 0, p->pos, 1, 0);
    }
}

void Player_SeAtFrame(Player *p, s32 frame, s32 se)
{
    if (p->weaponSeMute == 0 && gScene.online == 0 && Motion_PassedFrame(p, 0, frame)) {
        Player_PlaySe(p, se, 0, p->pos, 0x1A, 0);
    }
}

void PlayerSe_WeaponAtFrame(Player *p, s32 frame, s32 se)
{
    if (p->weaponSeMute == 0 && gScene.online == 0 && Motion_PassedFrame(p, 0, frame)) {
        Player_PlaySe(p, se, 0, p->pos, 1, 0);
    }
}

void Motion_VoiceAtFrame(Player *p, f32 frame, s32 voice)
{
    s16 v = 0;
    s32 i;

    if ((u8)Obj_InCurrentArea(p) && Motion_PassedFrame(p, 0, frame)) {
        if (p->wallHitDirs & 0xE0000007) {
            for (i = 0; i < 10; i++) {
                if (gEmWallContacts[p->index].e[i].unk8 != 0 && v == 0) {
                    v = gEmWallContacts[p->index].e[i].unk8;
                }
            }
        }
        Sound_PlaySeAtPosChecked(7, voice * 2 + (Rand_Next(1) & 1), v, p->pos, 1, 0);
    }
}

void PlayerSe_VoiceAtFrame(Player *p, f32 frame, s32 voice)
{
    if ((u8)Obj_InCurrentArea(p) && Motion_PassedFrame(p, 0, frame)) {
        Sound_PlaySeAtPosChecked(7, voice * 2 + (Rand_Next(1) & 1), p->floorArg, p->pos, 0xE, 0);
    }
}

void PlayerSe_OnlineAtFrame(Player *p, s32 frame, s32 se)
{
    if (p->weaponSeMute == 0 && gScene.online != 0 && gScene.localPlayer == p->index
        && Motion_PassedFrame(p, 0, frame)) {
        Sound_PlaySeAtPosChecked(2, se, 0, p->pos, 1, 0);
    }
}

void PlayerSe_EquipAtFrame(Player *p, f32 frame, s32 kind)
{
    if ((u8)Obj_InCurrentArea(p) && Motion_PassedFrame(p, 0, frame)) {
        Sound_PlayArmorRattle(p, kind);
    }
}

/* unk_386160's .data (0x449530-0x44A190) */
extern u32 D_00640078[];
f32 gAreaBlockSpheres_33[36] = {
#include "sub_main/00449530.inc"
};
f32 gAreaBlockSpheres_38[28] = {
#include "sub_main/004495C0.inc"
};
f32 gAreaBlockSpheres_40[12] = {
#include "sub_main/00449630.inc"
};
f32 gAreaBlockSpheres_45[16] = {
#include "sub_main/00449660.inc"
};
f32 gAreaBlockSpheres_49[20] = {
#include "sub_main/004496A0.inc"
};
f32 gAreaBlockSpheres_57[8] = {
#include "sub_main/004496F0.inc"
};
f32 gAreaBlockSpheres_66[28] = {
#include "sub_main/00449710.inc"
};
f32 gAreaBlockSpheres_91[24] = {
#include "sub_main/00449780.inc"
};
f32 gAreaBlockSpheres_110[20] = {
#include "sub_main/004497E0.inc"
};
f32 gAreaBlockSpheres_113[20] = {
#include "sub_main/00449830.inc"
};
f32 gAreaBlockSpheres_114[16] = {
#include "sub_main/00449880.inc"
};
f32 gAreaBlockSpheres_26[8] = {
#include "sub_main/004498C0.inc"
};
f32 gAreaBlockSpheres_141[8] = {
#include "sub_main/004498E0.inc"
};
f32 gAreaBlockSpheres_142[12] = {
#include "sub_main/00449900.inc"
};
f32 gAreaBlockSpheres_143[20] = {
#include "sub_main/00449930.inc"
};
f32 gAreaBlockSpheres_145[24] = {
#include "sub_main/00449980.inc"
};
f32 gAreaBlockSpheres_185[52] = {
#include "sub_main/004499E0.inc"
};
f32 gAreaBlockSpheres_190[28] = {
#include "sub_main/00449AB0.inc"
};
f32 gAreaBlockSpheres_191[36] = {
#include "sub_main/00449B20.inc"
};
f32 *gAreaBlockSpheres[200] = {
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_26, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_33, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_38, NULL,
    gAreaBlockSpheres_40, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_45, NULL, NULL,
    NULL, gAreaBlockSpheres_49, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_57, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_66, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, gAreaBlockSpheres_91,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_110, NULL,
    NULL, gAreaBlockSpheres_113, gAreaBlockSpheres_114, NULL,
    NULL, NULL, NULL, gAreaBlockSpheres_26,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_141, gAreaBlockSpheres_142, gAreaBlockSpheres_143,
    NULL, gAreaBlockSpheres_145, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_185, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_190, gAreaBlockSpheres_191,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
f32 D_00449ED0[4] = {
#include "sub_main/00449ED0.inc"
};
f32 D_00449EE0[4] = {
#include "sub_main/00449EE0.inc"
};
f32 D_00449EF0[4] = {
#include "sub_main/00449EF0.inc"
};
s16 D_00449F00[4] = {
#include "sub_main/00449F00.inc"
};
s16 D_00449F08[4] = {
#include "sub_main/00449F08.inc"
};
s16 D_00449F10[4] = {
#include "sub_main/00449F10.inc"
};
s16 D_00449F18[4] = {
#include "sub_main/00449F18.inc"
};
s16 *D_00449F20[4] = {
    D_00449F08, D_00449F10, D_00449F18, NULL,
};
u32 D_00449F30[2] = {
    (u32)D_00640078, 0,
};
u32 D_00449F38[2] = {
    (u32)D_00640078, 0,
};
u32 D_00449F40[2] = {
    (u32)D_00640078, 0,
};
s16 *D_00449F48[3] = {
    (s16 *)D_00449F30, (s16 *)D_00449F38, (s16 *)D_00449F40,
};
u32 gAmbSoundCounts[8] = {
#include "sub_main/00449F60.inc"
};
s16 gSeasonIconUVs[4][2] = {
#include "sub_main/00449F80.inc"
};
s16 gSeasonIconUVsOnline[4][2] = {
#include "sub_main/00449F90.inc"
};
f32 gThrownObjPos56[1][3] = {
#include "sub_main/00449FA0.inc"
};
s16 gThrownObjModel56[8] = {
#include "sub_main/00449FB0.inc"
};
f32 gThrownObjPosAD[2][3] = {
#include "sub_main/00449FC0.inc"
};
s16 gThrownObjAngleAD[4] = {
#include "sub_main/00449FD8.inc"
};
s16 gThrownObjModelAD[8] = {
#include "sub_main/00449FE0.inc"
};
f32 gThrownObjPosAE[6][3] = {
#include "sub_main/00449FF0.inc"
};
s16 gThrownObjAngleAE[6] = {
#include "sub_main/0044A038.inc"
};
s16 gThrownObjModelAE[6] = {
#include "sub_main/0044A048.inc"
};
f32 gThrownObjPosAF[16][3] = {
#include "sub_main/0044A060.inc"
};
s16 gThrownObjAngleAF[16] = {
#include "sub_main/0044A120.inc"
};
s16 gThrownObjModelAF[8] = {
#include "sub_main/0044A140.inc"
};
f32 gThrownObjDists[6] = {
#include "sub_main/0044A150.inc"
};
f32 gThrownObjPosK1[3] = {
#include "sub_main/0044A168.inc"
};
s16 gThrownObjModelK1AD = {
#include "sub_main/0044A178.inc"
};
s16 gThrownObjModelK1AE = {
#include "sub_main/0044A180.inc"
};
s16 gThrownObjModelK1AF = {
#include "sub_main/0044A188.inc"
};
