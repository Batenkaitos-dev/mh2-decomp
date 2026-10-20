/*
 * Small objects of the field and the town, the camera's view of the sky,
 * and the player's motion-frame sounds.
 *
 * - The camera's line to the sky: Vec_FromCamera, and Area_LineBlocked
 *   against the spheres of gAreaBlockSpheres; Area_BoundaryZone for the
 *   two desert areas whose sky changes with the player's place. The area
 *   drawing's atmosphere layers take their scroll (AtmosScroll_Update),
 *   alpha and speed (AreaAtmos_LayerAlpha, AreaAtmos_LayerSpeed) here.
 * - The season banner (OBJ_KIND_SEASON_BANNER, SeasonBanner_Create and
 *   SeasonBanner_CreateOnline): the season's name and, offline, the time
 *   of day, centred over two rows of the season's icon in the season's
 *   colour, faded in, slid apart and faded out. Online the season is
 *   given, and the banner ends when the lobby's season changes.
 * - The thrown object (OBJ_KIND_THROWN, ThrownObj_Spawn and
 *   ThrownObj_SpawnForHairMenu): an object a character of the house holds
 *   between its hands, then tosses onto a spot of the area, where it stays
 *   300 frames (or, for the hair menu's, while that menu is open).
 * - The motion-frame sounds (PlayerSe_AtFrame to PlayerSe_EquipAtFrame,
 *   which the player's actions call at hundreds of places): when the
 *   motion of layer 0 passes a frame (Motion_PassedFrame), a sound is
 *   played at the player's position: from the shared bank, the player's
 *   own bank, the voice bank (two takes per voice, picked at random) or
 *   the armour's rattle.
 */
#include "common.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"
#include "em.h"
#include "capsule.h"
#include "obj.h"
#include "sub_main/field_objects.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "area.h"
#include "area_draw.h"
#include "texture.h"
#include "sound.h"

#define OBJ_KIND_SEASON_BANNER  0x12    /* Obj.kind of the season banner */
#define OBJ_KIND_THROWN         0x15    /* Obj.kind of the thrown object */

/* The season banner's Obj.type, and its phases (Obj.var0). */
#define BANNER_OFFLINE      0       /* the season and the time of day of the field */
#define BANNER_ONLINE       1       /* the lobby's season (Obj.var3) */
#define BANNER_WAIT         0       /* 8 frames */
#define BANNER_FADE_IN      1       /* 16 frames */
#define BANNER_HOLD         2       /* 4 frames */
#define BANNER_SLIDE        3       /* 20 frames: the second row of icons slides right (Obj.var1) */
#define BANNER_FADE_OUT     4       /* 16 frames, still sliding */

/* The thrown object's Obj.var4, and its phases (Obj.var0). */
#define THROWN_TOSSED       0       /* thrown onto its spot (Obj.type), where it stays 300 frames */
#define THROWN_HAIR_MENU    1       /* grows in the hands, then lies at gThrownObjPosHairMenu while the hair menu is open */
#define THROWN_HELD         0       /* between the holder's hands, until its motion 0x432 passes frame 48 */
#define THROWN_FLYING       1       /* 10 frames to its spot */
#define THROWN_LYING        2       /* on its spot */

/* The flight of a thrown object (its Obj.buf). */
typedef struct ThrownFlight {
    f32 vel[3];                 /* 0x00: per frame; gravity takes 2 off vel[1] each frame */
    s16 turn;                   /* 0x0C: angle per frame (0x10000 a turn) */
} ThrownFlight;

/* The walls a player touches (gEmWallContacts, by Player.index), as
 * Motion_VoiceAtFrame reads them. */
typedef struct WallContactRow {
    WallContact e[WALL_CONTACTS];
} WallContactRow;

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
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);
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
void PartFx_SpawnAt(f32 *pos, s32 kind, f32 scale);
void Em_BonePos(void *obj, s16 bone, f32 *out);

/* The lobby. */
s32 HairMenu_IsOpen(void);
s32 LobbyScreen_IsTouring(void);
s32 LobbyWorld_GetSeason(void);

/* This file. */
void SeasonBanner_Update(Obj *o);
void SeasonBanner_Init(Obj *o);
void SeasonBanner_Run(Obj *o);
void SeasonBanner_End(Obj *o);
void SeasonBanner_Free(Obj *o);
void SeasonBanner_Draw(DrawSlot *s);
void ThrownObj_HoldInHands(Obj *o, void *arg);
void ThrownObj_SpotPos(Obj *o, f32 *out);
void ThrownObj_SpotAngle(Obj *o, s16 *angle);
s16 ThrownObj_AreaModel(Obj *o);
void ThrownObj_Launch(Obj *o, ThrownFlight *out);
static void ThrownObj_Fly(Obj *o, ThrownFlight *v);
void ThrownObj_Task(Obj *o);
void ThrownObj_Start(Obj *o);
void ThrownObj_Run(Obj *o);
void ThrownObj_End(Obj *o);
void ThrownObj_Free(Obj *o);
void ThrownObj_Draw(DrawSlot *s);

extern char *gSeasonNames[];       /* season names */
extern char *gDaytimeNames[];       /* day, night */
extern f32 *gAreaBlockSpheres[];       /* per area: the spheres that block the camera's line, -1.0f ends */
extern f32 gSnowLayerSpeedsState0[];
extern f32 gSnowLayerSpeedsState1[];
extern f32 gSnowLayerSpeedsState2[];
extern s16 *gSwampLayerPeriodsByState[];
extern s16 gSeasonIconUVs[][2];
extern s16 gSeasonIconUVsOnline[][2];
extern f32 gThrownObjPosHouse[][3];
extern s16 gThrownObjModelHouse[];
extern f32 gThrownObjPosOnlineHouse0[][3];
extern s16 gThrownObjAngleOnlineHouse0[];
extern s16 gThrownObjModelOnlineHouse0[];
extern f32 gThrownObjPosOnlineHouse1[][3];
extern s16 gThrownObjAngleOnlineHouse1[];
extern s16 gThrownObjModelOnlineHouse1[];
extern f32 gThrownObjPosOnlineHouse2[][3];
extern s16 gThrownObjAngleOnlineHouse2[];
extern s16 gThrownObjModelOnlineHouse2[];
extern f32 gThrownObjDists[];
extern f32 gThrownObjPosHairMenu[];
extern s16 gThrownObjModelHairMenuOnlineHouse0;
extern s16 gThrownObjModelHairMenuOnlineHouse1;
extern s16 gThrownObjModelHairMenuOnlineHouse2;
extern WallContactRow gEmWallContacts[];
extern f32 gCamMatrix[];         /* the camera's matrix */
extern u8 gCamDir[];
extern f32 gCamPos[3];       /* the camera's position */
extern s16 gThrownObjAngleHouse[];

/* The point at s times the vector v from the camera's position. */
void Vec_FromCamera(f32 *out, f32 *dir, f32 dist)
{
    f32 x, y, z;

    x = dist * dir[0];
    y = dist * dir[1];
    z = dist * dir[2];
    out[0] = gCamPos[0] + x;
    out[1] = gCamPos[1] + y;
    out[2] = gCamPos[2] + z;
}

/* 1 when the line from the camera to pos (a capsule of radius 5) meets one
 * of the area's spheres in gAreaBlockSpheres (radius, x, y, z; -1 ends). */
s32 Area_LineBlocked(f32 *pos)
{
    f32 center[3];
    f32 pushOut[4];
    HitCapsule cap;
    u8 capsule[0x40];
    f32 *sphere;
    f32 sphereRadius;

    Vec_Copy(cap.a, gCamPos);
    Vec_Copy(cap.b, pos);
    cap.radius = 5.0f;
    Capsule_Copy(&cap, (Capsule *) capsule);
    sphere = gAreaBlockSpheres[gScene.area];
    if (sphere != NULL) {
        for (sphereRadius = *sphere; sphereRadius != -1.0f; sphereRadius = *sphere) {
            sphere++;
            center[0] = *sphere++;
            center[1] = *sphere++;
            center[2] = *sphere++;
            if (Capsule_PushSphere((Capsule *) capsule, center, pushOut, sphereRadius)) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 * Where the player stands in AREA_DESERT_1 and AREA_DESERT_5 (and their
 * night areas): 0 inside the inner limits (x between two bounds; in
 * AREA_DESERT_1, x below one bound and z above another), 1 beyond the
 * outer ones, 2 in between. 0 in every other area.
 */
s16 Area_BoundaryZone(Player *pl)
{
    f32 outerMinX, outerMinZ, innerMinZ, outerMaxX, innerMaxX, innerMinX;
    u8 shape;

    switch (gScene.area) {
    case AREA_DESERT_NIGHT_1:
    case AREA_DESERT_1:
        outerMaxX = 17800.0f;
        outerMinZ = 2200.0f;
        innerMaxX = 17200.0f;
        innerMinZ = 2800.0f;
        shape = 1;
        break;
    case AREA_DESERT_NIGHT_5:
    case AREA_DESERT_5:
        outerMinX = 4500.0f;
        outerMaxX = 18000.0f;
        innerMinX = 5100.0f;
        innerMaxX = 17400.0f;
        shape = 0;
        break;
    default:
        return 0;
    }
    switch (shape) {
    default:
        if (pl->pos[0] > innerMinX && pl->pos[0] < innerMaxX) {
            return 0;
        }
        if (pl->pos[0] < outerMinX || pl->pos[0] > outerMaxX) {
            return 1;
        }
        break;
    case 1:
        if (pl->pos[0] < innerMaxX && pl->pos[2] > innerMinZ) {
            return 0;
        }
        if (pl->pos[0] > outerMaxX || pl->pos[2] < outerMinZ) {
            return 1;
        }
        break;
    }
    return 2;
}

/*
 * Scrolls the n atmosphere layers of h with the camera: u by the camera's
 * turn about y since the last frame, v by its rise. A layer whose speed
 * is set also drifts by 1 / speed per frame, in u (mode 1) or v (mode 2).
 * The coordinates wrap to 0..1.
 */
void AtmosScroll_Update(AtmosScroll *scroll, s16 count, s16 *speed, u16 mode)
{
    f32 camNow[3];
    u16 camPitch;
    u16 camYaw;
    f32 uStep;
    f32 vStep;
    s16 i;

    Mtx_GetTrans(camNow, gCamMatrix);
    Vec_DirToAngles((f32 *) gCamDir, (s16 *) &camPitch, (s16 *) &camYaw);
    uStep = (s16)(camYaw - scroll->yaw);
    uStep *= 6.964391650399193e-05f;
    vStep = camNow[1] - scroll->camPos[1];
    vStep *= 0.001f;
    scroll->pitch = camPitch;
    scroll->yaw = camYaw;
    Vec_Copy(scroll->camPos, camNow);
    for (i = 0; i < count; i++) {
        scroll->layer[i].u += uStep;
        scroll->layer[i].v += vStep;
        if (speed != NULL && speed[i] != 0) {
            switch (mode) {
            case 0:
                break;
            case 1:
                scroll->layer[i].u += 1.0f / speed[i];
                break;
            case 2:
                scroll->layer[i].v += 1.0f / speed[i];
                break;
            }
        }
        if (scroll->layer[i].u > 1.0f || scroll->layer[i].u < 0.0f) {
            scroll->layer[i].u -= Math_Floor(scroll->layer[i].u);
        }
        if (scroll->layer[i].v > 1.0f || scroll->layer[i].v < 0.0f) {
            scroll->layer[i].v -= Math_Floor(scroll->layer[i].v);
        }
    }
}

/* The alpha of atmosphere layer a in the area's state b (Area_MonsterState)
 * for the jungle and fortress at night, the swamp, the snowy mountains and
 * the swamp at night; 1.0 elsewhere. */
f32 AreaAtmos_LayerAlpha(u8 layer, s16 state)
{
    switch (gScene.area) {
    case AREA_JUNGLE_NIGHT_0:
    case AREA_JUNGLE_NIGHT_1:
    case AREA_JUNGLE_NIGHT_2:
    case AREA_JUNGLE_NIGHT_3:
    case AREA_JUNGLE_NIGHT_4:
    case AREA_SWAMP_10:
    case AREA_JUNGLE_NIGHT_5:
    case AREA_JUNGLE_NIGHT_6:
    case AREA_FORTRESS_NIGHT_4:
    case AREA_FORTRESS_NIGHT_5:
    case AREA_FORTRESS_NIGHT_1:
    case AREA_FORTRESS_NIGHT_3:
    case AREA_FORTRESS_NIGHT_2:
    case AREA_FORTRESS_NIGHT_6:
    case AREA_SWAMP_0:
    case AREA_SWAMP_1:
    case AREA_SWAMP_2:
    case AREA_SWAMP_5:
    case AREA_SWAMP_6:
    case AREA_SWAMP_7:
    case AREA_SWAMP_9:
        switch (state) {
        case 0:
            switch (layer) {
            case 0:
            case 1:
                return 0.0f;
            }
            break;
        case 1:
            switch (layer) {
            case 0:
                return 0.0f;
            case 1:
                return 0.45f;
            }
            break;
        case 2:
            switch (layer) {
            case 1:
                return 0.9f;
            }
            break;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_5:
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_8:
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        switch (state) {
        case 0:
            switch (layer) {
            case 0:
            case 1:
                return 0.0f;
            }
            break;
        case 1:
            switch (layer) {
            case 0:
                return 0.1f;
            case 1:
                return 0.2f;
            }
            break;
        case 2:
            switch (layer) {
            case 0:
                return 1.0f;
            case 1:
                return 0.45f;
            }
            break;
        }
        break;
    case AREA_SWAMP_NIGHT_0:
    case AREA_SWAMP_NIGHT_10:
    case AREA_SWAMP_NIGHT_1:
    case AREA_SWAMP_NIGHT_2:
    case AREA_SWAMP_NIGHT_5:
    case AREA_SWAMP_NIGHT_6:
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        switch (state) {
        case 0:
            switch (layer) {
            case 0:
                return 0.0f;
            case 1:
                return 0.1f;
            }
            break;
        case 1:
            switch (layer) {
            case 0:
                return 0.1f;
            case 1:
                return 0.2f;
            }
            break;
        case 2:
            switch (layer) {
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

/* The scroll speed of atmosphere layer a in the area's state b: the snowy
 * mountains' from gSnowLayerSpeedsState0 to 2, the swamp's at night 1 /
 * its period (gSwampLayerPeriodsByState; layer 0 doesn't move); 0
 * elsewhere. */
f32 AreaAtmos_LayerSpeed(u8 layer, s16 state)
{
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_5:
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_8:
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        switch (state) {
        case 1:
            return gSnowLayerSpeedsState1[layer];
        default:
        case 0:
            return gSnowLayerSpeedsState0[layer];
        case 2:
            return gSnowLayerSpeedsState2[layer];
        }
    case AREA_SWAMP_NIGHT_0:
    case AREA_SWAMP_NIGHT_10:
    case AREA_SWAMP_NIGHT_1:
    case AREA_SWAMP_NIGHT_2:
    case AREA_SWAMP_NIGHT_5:
    case AREA_SWAMP_NIGHT_6:
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        switch (layer) {
        case 0:
            return 0.0f;
        default:
        case 1:
            return 1.0f / gSwampLayerPeriodsByState[state][layer];
        }
    }
    return 0.0f;
}

/* Starts the field's season banner: the season and the time of day. */
void SeasonBanner_Create(void)
{
    Obj *banner = Obj_Alloc(0);

    if (banner != NULL) {
        banner->kind = OBJ_KIND_SEASON_BANNER;
        banner->type = BANNER_OFFLINE;
        banner->keep = 1;
        banner->update = SeasonBanner_Update;
    }
}

/* Starts the lobby's season banner for season season; isPm (the time of
 * day) goes into its timer, which its start then sets anew. */
void SeasonBanner_CreateOnline(u8 season, u8 isPm)
{
    Obj *banner = Obj_Alloc(0);

    if (banner != NULL) {
        banner->kind = OBJ_KIND_SEASON_BANNER;
        banner->type = BANNER_ONLINE;
        banner->keep = 1;
        banner->var3 = season;
        banner->timer = isPm;
        banner->update = SeasonBanner_Update;
    }
}

/* The season banner's update: runs its state (start, run, end, free). */
void SeasonBanner_Update(Obj *banner)
{
    switch (banner->state) {
    case 0:
        SeasonBanner_Init(banner);
        break;
    case 1:
        SeasonBanner_Run(banner);
        break;
    case 2:
        SeasonBanner_End(banner);
        break;
    case 3:
        SeasonBanner_Free(banner);
        break;
    }
}

/* Sets the season banner up: hidden (alpha 0), its first phase 8 frames,
 * and a draw slot of the second pool; without a free slot it is freed. */
void SeasonBanner_Init(Obj *banner)
{
    banner->state++;
    banner->var1 = 0;
    banner->var2 = 0;
    banner->visible = 1;
    banner->pos[0] = 0.0f;
    banner->pos[1] = 0.0f;
    banner->pos[2] = 0.0f;
    banner->timer = 8;
    banner->var5 = 0;
    banner->draw = NULL;
    banner->slot = DrawSlotB_Alloc();
    banner->ds = DrawSlotB_Get(banner->slot);
    if (banner->ds != NULL) {
        banner->ds->owner = banner;
        banner->ds->draw = SeasonBanner_Draw;
        SeasonBanner_Run(banner);
    } else {
        Obj_Free(banner);
    }
}

/*
 * The season banner's frame: it waits during an event (in a quest), and ends
 * in the town when the lobby's screen takes over or the season
 * changes. It steps its phases (BANNER_*: alpha in var2, the slide in
 * var1) and puts its draw slot in the text layer's list.
 */
void SeasonBanner_Run(Obj *banner)
{
    banner->var5++;
    if (banner->type == BANNER_OFFLINE) {
        if (gScene.eventRunning != 0) {
            return;
        }
    } else if (gScene.inLobby != 0) {
        if (LobbyScreen_IsTouring() == 1) {
            banner->state++;
            return;
        }
        if ((u8)banner->var3 != (u8)LobbyWorld_GetSeason()) {
            banner->state++;
            return;
        }
    }
    switch (banner->var0) {
    case BANNER_WAIT:
        banner->var2 = 0;
        if (--banner->timer <= 0) {
            banner->var0++;
            banner->timer = 0x10;
        }
        break;
    case BANNER_FADE_IN:
        if (--banner->timer <= 0) {
            banner->var0++;
            banner->timer = 4;
            banner->var2 = 0xFF;
        } else {
            banner->var2 = (0x10 - banner->timer) << 4;
        }
        break;
    case BANNER_HOLD:
        if (--banner->timer <= 0) {
            banner->var0++;
            banner->timer = 0x14;
        }
        break;
    case BANNER_SLIDE:
        banner->var1++;
        if (--banner->timer <= 0) {
            banner->var0++;
            banner->timer = 0x10;
        }
        break;
    case BANNER_FADE_OUT:
        banner->var1++;
        if (--banner->timer <= 0) {
            banner->state++;
        }
        banner->var2 = banner->timer << 4;
        break;
    }
    if (banner->type == BANNER_OFFLINE) {
        OT_AddByDepth(gDrawListText0b, banner->ds, 1, 1);
    } else {
        OT_AddByDepth(gDrawListText4, banner->ds, 1, 1);
    }
}

/* Hides the season banner and frees its draw slot. */
void SeasonBanner_End(Obj *banner)
{
    banner->state++;
    banner->visible = 0;
    DrawSlotB_Free(banner->slot);
}

void SeasonBanner_Free(Obj *banner)
{
    Obj_Free(banner);
}

/*
 * Draws the season banner: the season's name (and, offline, day or night)
 * centred at y 0xAF, then 15 pairs of the season's icon, the second of
 * each pair var1 further right, in the season's colour with alpha var2.
 * Kept for the match (a FAKEMATCH): n, an s32 copy of the season taken
 * again in each texture branch, is what the table reads and the switch
 * use; the game's code masks the season once per branch. With idx itself
 * there, or with a helper function for the two reads, the reads and the
 * switch compile differently.
 */
void SeasonBanner_Draw(DrawSlot *slot)
{
    Obj *banner = slot->owner;
    Sprite spr;
    char buf[0x40];
    s16 i;
    u8 season;
    s32 x;
    s32 seasonCopy;

    if (banner->type == BANNER_OFFLINE) {
        Text_SetSize(0);
        season = Season_Get();
        sprintf(buf, TXT(0x0049FCA0) /* two names with a full-width space between them */, gSeasonNames[season], gDaytimeNames[(u8)Daytime_Get()]);
    } else {
        Text_SetSize(4);
        season = banner->var3;
        sprintf(buf, "%s", gSeasonNames[season]);
    }
    Font_SetSpacing(0x1C, 0x1C);
    Font_SetPal(0);
    Text_DrawShadow2((0x280 - Str_Length(buf) * 14) / 2, 0xAF, 1, 0, buf);
    if (banner->type == BANNER_OFFLINE) {
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        seasonCopy = season;
        spr.u0 = gSeasonIconUVs[seasonCopy][0];
        spr.v0 = gSeasonIconUVs[seasonCopy][1];
    } else {
        Tex_Prepare(1, TEX_SLOT_ICONS);
        Tex_Bind(TEX_SLOT_ICONS);
        seasonCopy = season;
        spr.u0 = gSeasonIconUVsOnline[seasonCopy][0];
        spr.v0 = gSeasonIconUVsOnline[seasonCopy][1];
    }
    spr.u1 = spr.u0 + 14;
    spr.v1 = spr.v0 + 16;
    spr.y = 0xD9;
    spr.w = 0x20;
    spr.h = 0x20;
    switch (seasonCopy) {
    case 0:
        spr.color = (banner->var2 << 24) | 0xC6AB20;
        break;
    case 1:
        spr.color = (banner->var2 << 24) | 0x53ACCC;
        break;
    case 2:
        spr.color = (banner->var2 << 24) | 0x398C34;
        break;
    }
    for (i = 0, x = 4; i < 15; i++, x += 0x30) {
        spr.x = x;
        Sprite_Draw(&spr);
        spr.x = banner->var1 + x;
        Sprite_Draw(&spr);
    }
}

/* Starts a thrown object held by holder, to land on spot spot of the
 * area; step (up to 4) picks its model and how far along its angle it
 * lands. */
void ThrownObj_Spawn(void *holder, s32 spot, s32 step)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = OBJ_KIND_THROWN;
        obj->update = ThrownObj_Task;
        obj->arg = holder;
        obj->type = spot;
        obj->var4 = THROWN_TOSSED;
        obj->fvar = 1.0f;
        obj->var2 = step;
        if (obj->var2 > 4) {
            obj->var2 = 4;
        }
    }
}

/* Starts the hair menu's object, held by holder, growing from nothing. */
void ThrownObj_SpawnForHairMenu(void *holder, s32 spot)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = OBJ_KIND_THROWN;
        obj->update = ThrownObj_Task;
        obj->arg = holder;
        obj->type = spot;
        obj->var4 = THROWN_HAIR_MENU;
        obj->fvar = 0.0f;
        obj->var2 = 0;
    }
}

/* Places o between bones 6 and 9 of its holder (2 above them; the height
 * is set twice, as in the game's code), turned as the holder is. */
void ThrownObj_HoldInHands(Obj *obj, void *holder)
{
    f32 boneA[4];
    f32 boneB[4];

    Em_BonePos(holder, 6, boneA);
    Em_BonePos(holder, 9, boneB);
    obj->pos[0] = (boneA[0] + boneB[0]) / 2.0f;
    switch (obj->var4) {
    case THROWN_TOSSED:
    default:
        obj->pos[1] = 2.0f + (boneA[1] + boneB[1]) / 2.0f;
        break;
    case THROWN_HAIR_MENU:
        obj->pos[1] = 20.0f + (boneA[1] + boneB[1]) / 2.0f;
        break;
    }
    obj->pos[1] = 2.0f + (boneA[1] + boneB[1]) / 2.0f;
    obj->pos[2] = (boneA[2] + boneB[2]) / 2.0f;
    obj->var3 = ((Em *)holder)->rotY;
}

/* Where o lands in the current area: its spot, moved along the spot's
 * angle by gThrownObjDists[step] (the hair menu's object: a fixed spot). */
void ThrownObj_SpotPos(Obj *obj, f32 *out)
{
    f32 offset[3];
    s16 angle;
    f32 *spot;

    switch (obj->var4) {
    default:
        switch (gScene.area) {
        case AREA_HOUSE:
        case AREA_HUB_0x57:
            spot = gThrownObjPosHouse[obj->type];
            break;
        case AREA_ONLINE_HOUSE_0:
            spot = gThrownObjPosOnlineHouse0[obj->type];
            break;
        case AREA_ONLINE_HOUSE_1:
            spot = gThrownObjPosOnlineHouse1[obj->type];
            break;
        case AREA_ONLINE_HOUSE_2:
            spot = gThrownObjPosOnlineHouse2[obj->type];
            break;
        }
        Vec_Copy(out, spot);
        ThrownObj_SpotAngle(obj, &angle);
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = gThrownObjDists[obj->var2];
        Vec_RotateY(offset, (6.2831855f * angle) / 65536.0f);
        out[0] += offset[0];
        out[1] += offset[1];
        out[2] += offset[2];
        break;
    case 1:
        Vec_Copy(out, gThrownObjPosHairMenu);
        break;
    }
}

/* The angle o lands at: its spot's, turned half a turn (the hair menu's
 * object: a fixed angle). */
void ThrownObj_SpotAngle(Obj *obj, s16 *angle)
{
    switch (obj->var4) {
    default:
        switch (gScene.area) {
        case AREA_HOUSE:
        case AREA_HUB_0x57:
            *angle = gThrownObjAngleHouse[obj->type];
            break;
        case AREA_ONLINE_HOUSE_0:
            *angle = gThrownObjAngleOnlineHouse0[obj->type];
            break;
        case AREA_ONLINE_HOUSE_1:
            *angle = gThrownObjAngleOnlineHouse1[obj->type];
            break;
        case AREA_ONLINE_HOUSE_2:
            *angle = gThrownObjAngleOnlineHouse2[obj->type];
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
s16 ThrownObj_AreaModel(Obj *obj)
{
    switch (obj->var4) {
    default:
        switch (gScene.area) {
        default:
            return 0;
        case AREA_HOUSE:
        case AREA_HUB_0x57:
            return gThrownObjModelHouse[obj->var2];
        case AREA_ONLINE_HOUSE_0:
            return gThrownObjModelOnlineHouse0[obj->var2];
        case AREA_ONLINE_HOUSE_1:
            return gThrownObjModelOnlineHouse1[obj->var2];
        case AREA_ONLINE_HOUSE_2:
            return gThrownObjModelOnlineHouse2[obj->var2];
        }
    case 1:
        switch (gScene.area) {
        default:
            return 0;
        case AREA_ONLINE_HOUSE_0:
            return gThrownObjModelHairMenuOnlineHouse0;
        case AREA_ONLINE_HOUSE_1:
            return gThrownObjModelHairMenuOnlineHouse1;
        case AREA_ONLINE_HOUSE_2:
            return gThrownObjModelHairMenuOnlineHouse2;
        }
    }
}

/* Sets the flight that takes o to its spot and angle in 10 frames. */
void ThrownObj_Launch(Obj *obj, ThrownFlight *out)
{
    f32 target[3];
    s16 angle;

    ThrownObj_SpotPos(obj, target);
    ThrownObj_SpotAngle(obj, &angle);
    out->vel[0] = (target[0] - obj->pos[0]) / 10.0f;
    out->vel[1] = (target[1] - obj->pos[1]) / 10.0f - -10.0f;
    out->vel[2] = (target[2] - obj->pos[2]) / 10.0f;
    out->turn = (angle - obj->var3) / 10.0f;
}

/* One frame of the flight: moves and turns o, and gravity pulls. */
static void ThrownObj_Fly(Obj *obj, ThrownFlight *flight)
{
    obj->pos[0] += flight->vel[0];
    obj->pos[1] += flight->vel[1];
    obj->pos[2] += flight->vel[2];
    flight->vel[1] += -2.0f;
    obj->var3 += flight->turn;
}

/* The thrown object's update: runs its state (start, run, end, free). */
void ThrownObj_Task(Obj *obj)
{
    switch (obj->state) {
    case 0:
        ThrownObj_Start(obj);
        break;
    case 1:
        ThrownObj_Run(obj);
        break;
    case 2:
        ThrownObj_End(obj);
        break;
    case 3:
        ThrownObj_Free(obj);
        break;
    }
}

/* Sets the thrown object up in its holder's hands, with a draw slot; it is
 * freed when no slot is free. */
void ThrownObj_Start(Obj *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->timer = 0;
    ThrownObj_HoldInHands(obj, obj->arg);
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = ThrownObj_Draw;
    } else {
        Obj_Free(obj);
    }
}

/*
 * The thrown object's frame: held (THROWN_HELD), it follows the holder's
 * hands (the hair menu's grows over 15 frames) until the holder's motion
 * 0x432 passes frame 48; it then flies 10 frames (THROWN_FLYING) and lands
 * on its spot with a puff of dust; there (THROWN_LYING) it stays 300
 * frames, or while the hair menu is open. It ends at once if the holder
 * goes. Its draw slot goes in the depth-sorted list each frame.
 */
void ThrownObj_Run(Obj *obj)
{
    struct ThrownFlight *flight = obj->buf;
    Em *holder = obj->arg;

    switch (obj->var0) {
    case THROWN_HELD:
        if (holder->active == 0) {
            obj->state++;
            return;
        }
        switch (obj->var4) {
        case THROWN_TOSSED:
            break;
        case THROWN_HAIR_MENU:
            obj->fvar += 0.06666667f;
            break;
        }
        if (obj->fvar > 1.0f) {
            obj->fvar = 1.0f;
        }
        ThrownObj_HoldInHands(obj, obj->arg);
        if (holder->motionId[0] == 0x432 && EmMotion_PassedFrameScaled(holder, 48.0f, 0)) {
            obj->fvar = 1.0f;
            ThrownObj_Launch(obj, flight);
            obj->timer = 0;
            obj->var0++;
        }
        break;
    case THROWN_FLYING:
        ThrownObj_Fly(obj, flight);
        if (++obj->timer >= 10) {
            ThrownObj_SpotPos(obj, obj->pos);
            ThrownObj_SpotAngle(obj, &obj->var3);
            PartFx_SpawnAt(obj->pos, 7, 0.8f);
            obj->timer = 0;
            obj->var0++;
        }
        break;
    case THROWN_LYING:
        switch (obj->var4) {
        default:
            if (++obj->timer >= 300) {
                obj->state++;
                return;
            }
            break;
        case THROWN_HAIR_MENU:
            if (HairMenu_IsOpen() == 0) {
                obj->state++;
                return;
            }
            break;
        }
        break;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/* Hides the thrown object and frees its draw slot. */
void ThrownObj_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

void ThrownObj_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* Draws the thrown object: its area sub-model (ThrownObj_AreaModel) of
 * the area's first model set, scaled by fvar and turned by var3. */
void ThrownObj_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    Model *set = gAreaSubModels[0];
    Obj *obj = slot->owner;
    ModelMatSet *matSet;

    if (set != NULL && set->active) {
        Mtx_Scale(mtx, obj->fvar, obj->fvar, obj->fvar);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Mtx_RotateY(mtx, (6.2831855f * obj->var3) / 65536.0f);
        Gs_SetState(RS_ALPHA_REF, 0x80);
        Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
        matSet = &set->matSets[ThrownObj_AreaModel(obj)];
        if (matSet != NULL && matSet->id != -1) {
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* Plays sound se of the shared bank at the player when its motion passes
 * frame. */
void PlayerSe_AtFrame(Player *pl, s32 frame, s32 seId)
{
    if (Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayPlayerSe(pl, seId, 0, pl->pos, 1, 0);
    }
}

/* The same with the distance curve 0x17. */
void Motion_SeAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayPlayerSe(pl, seId, 0, pl->pos, 0x17, 0);
    }
}

/* As PlayerSe_AtFrame, but only for the local player and the remote
 * players of even index. */
void Motion_SeAtFrameLocal(Player *pl, s32 frame, s32 seId)
{
    if (Player_IsLocal(pl) == 0 && (pl->index & 1)) {
        return;
    }
    if (Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayPlayerSe(pl, seId, 0, pl->pos, 1, 0);
    }
}

/* Outside the town: plays sound se of the player's own bank (distance curve 0x1A)
 * when its motion passes frame, unless its weapon sounds are muted. */
void Player_SeAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (pl->weaponSeMute == 0 && gScene.inLobby == 0 && Motion_PassedFrame(pl, 0, frame)) {
        Player_PlaySe(pl, seId, 0, pl->pos, 0x1A, 0);
    }
}

/* The same with the distance curve 1. */
void PlayerSe_WeaponAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (pl->weaponSeMute == 0 && gScene.inLobby == 0 && Motion_PassedFrame(pl, 0, frame)) {
        Player_PlaySe(pl, seId, 0, pl->pos, 1, 0);
    }
}

/*
 * Plays voice voice (one of its two takes, at random) from the voice bank
 * when the player, in the current area, passes frame. Against a wall
 * (Player.wallHitDirs), the sound argument of the first wall contact that
 * has one goes with it.
 */
void Motion_VoiceAtFrame(Player *pl, f32 frame, s32 voice)
{
    s16 wallArg = 0;
    s32 i;

    if ((u8)Obj_InCurrentArea(pl) && Motion_PassedFrame(pl, 0, frame)) {
        if (pl->wallHitDirs & 0xE0000007) {
            for (i = 0; i < 10; i++) {
                if (gEmWallContacts[pl->index].e[i].seArg != 0 && wallArg == 0) {
                    wallArg = gEmWallContacts[pl->index].e[i].seArg;
                }
            }
        }
        Sound_PlaySeAtPosChecked(7, voice * 2 + (Rand_Next(1) & 1), wallArg, pl->pos, 1, 0);
    }
}

/* Plays voice voice as Motion_VoiceAtFrame does, with the floor's sound
 * argument and the distance curve 0xE. */
void PlayerSe_VoiceAtFrame(Player *pl, f32 frame, s32 voice)
{
    if ((u8)Obj_InCurrentArea(pl) && Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlaySeAtPosChecked(7, voice * 2 + (Rand_Next(1) & 1), pl->floorArg, pl->pos, 0xE, 0);
    }
}

/* In the town: plays sound se of bank 2 when the local player's motion passes
 * frame, unless its weapon sounds are muted. */
void PlayerSe_OnlineAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (pl->weaponSeMute == 0 && gScene.inLobby != 0 && gScene.localPlayer == pl->index
        && Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlaySeAtPosChecked(2, seId, 0, pl->pos, 1, 0);
    }
}

/* Plays the rattle of the player's armour piece kind when the player, in
 * the current area, passes frame. */
void PlayerSe_EquipAtFrame(Player *pl, f32 frame, s32 kind)
{
    if ((u8)Obj_InCurrentArea(pl) && Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayArmorRattle(pl, kind);
    }
}

/* The file's data: the camera's blocking spheres, the atmosphere layers'
 * speeds and periods, the season icons and the thrown objects' spots. */
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
f32 gSnowLayerSpeedsState0[4] = {
#include "sub_main/00449ED0.inc"
};
f32 gSnowLayerSpeedsState1[4] = {
#include "sub_main/00449EE0.inc"
};
f32 gSnowLayerSpeedsState2[4] = {
#include "sub_main/00449EF0.inc"
};
s16 gAtmosScrollSpeeds[4] = {
#include "sub_main/00449F00.inc"
};
s16 gAtmosScrollSpeedsState0[4] = {
#include "sub_main/00449F08.inc"
};
s16 gAtmosScrollSpeedsState1[4] = {
#include "sub_main/00449F10.inc"
};
s16 gAtmosScrollSpeedsState2[4] = {
#include "sub_main/00449F18.inc"
};
s16 *gAtmosScrollSpeedsByState[4] = {
    gAtmosScrollSpeedsState0, gAtmosScrollSpeedsState1, gAtmosScrollSpeedsState2, NULL,
};
s16 gSwampLayerPeriodsState0[4] = {
#include "sub_main/00449F30.inc"
};
s16 gSwampLayerPeriodsState1[4] = {
#include "sub_main/00449F38.inc"
};
s16 gSwampLayerPeriodsState2[4] = {
#include "sub_main/00449F40.inc"
};
s16 *gSwampLayerPeriodsByState[3] = {
    gSwampLayerPeriodsState0, gSwampLayerPeriodsState1, gSwampLayerPeriodsState2,
};
u32 gAreaAtmosLayerCounts[8] = {
#include "sub_main/00449F60.inc"
};
s16 gSeasonIconUVs[4][2] = {
#include "sub_main/00449F80.inc"
};
s16 gSeasonIconUVsOnline[4][2] = {
#include "sub_main/00449F90.inc"
};
f32 gThrownObjPosHouse[1][3] = {
#include "sub_main/00449FA0.inc"
};
s16 gThrownObjModelHouse[8] = {
#include "sub_main/00449FB0.inc"
};
f32 gThrownObjPosOnlineHouse0[2][3] = {
#include "sub_main/00449FC0.inc"
};
s16 gThrownObjAngleOnlineHouse0[4] = {
#include "sub_main/00449FD8.inc"
};
s16 gThrownObjModelOnlineHouse0[8] = {
#include "sub_main/00449FE0.inc"
};
f32 gThrownObjPosOnlineHouse1[6][3] = {
#include "sub_main/00449FF0.inc"
};
s16 gThrownObjAngleOnlineHouse1[6] = {
#include "sub_main/0044A038.inc"
};
s16 gThrownObjModelOnlineHouse1[6] = {
#include "sub_main/0044A048.inc"
};
f32 gThrownObjPosOnlineHouse2[16][3] = {
#include "sub_main/0044A060.inc"
};
s16 gThrownObjAngleOnlineHouse2[16] = {
#include "sub_main/0044A120.inc"
};
s16 gThrownObjModelOnlineHouse2[8] = {
#include "sub_main/0044A140.inc"
};
f32 gThrownObjDists[6] = {
#include "sub_main/0044A150.inc"
};
f32 gThrownObjPosHairMenu[3] = {
#include "sub_main/0044A168.inc"
};
s16 gThrownObjModelHairMenuOnlineHouse0 = {
#include "sub_main/0044A178.inc"
};
s16 gThrownObjModelHairMenuOnlineHouse1 = {
#include "sub_main/0044A180.inc"
};
s16 gThrownObjModelHairMenuOnlineHouse2 = {
#include "sub_main/0044A188.inc"
};
