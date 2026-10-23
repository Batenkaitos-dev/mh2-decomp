/*
 * Two small objects of the field and the town.
 *
 * - The season banner (OBJ_KIND_SEASON_BANNER, SeasonBanner_Create and
 *   SeasonBanner_CreateOnline): the season's name and, offline, the time
 *   of day, centred over two rows of the season's icon in the season's
 *   colour, faded in, slid apart and faded out. Online the season is
 *   given, and the banner ends when the lobby's season changes.
 * - The thrown object (OBJ_KIND_THROWN, ThrownObj_Spawn and
 *   ThrownObj_SpawnForHairMenu): an object a character of the house holds
 *   between its hands, then tosses onto a spot of the area, where it stays
 *   300 frames (or, for the hair menu's, while that menu is open).
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
#include "card.h"
#include "save.h"
#include "text.h"
#include "field_objects.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "font.h"
#include "hair_menu.h"
#include "lobby_screen.h"
#include "lobby_world.h"
#include "part_effects.h"
#include "prim.h"

#define OBJ_KIND_SEASON_BANNER  18      /* Obj.kind of the season banner */
#define OBJ_KIND_THROWN         21      /* Obj.kind of the thrown object */

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
#define THROWN_HELD         0       /* between the holder's hands, until its motion 1074 passes frame 48 */
#define THROWN_FLYING       1       /* 10 frames to its spot */
#define THROWN_LYING        2       /* on its spot */


/* Boot ELF. */
s32 sprintf(char *dst, char *fmt, ...);     /* sprintf */

/* sub_main. */
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);
s32 Str_Length(char *s);

/* This file. */
static void ThrownObj_Fly(Obj *o, ThrownFlight *v);

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
extern f32 gCamPos[3];       /* the camera's position */
extern s16 gThrownObjAngleHouse[];

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
            banner->timer = 16;
        }
        break;
    case BANNER_FADE_IN:
        if (--banner->timer <= 0) {
            banner->var0++;
            banner->timer = 4;
            banner->var2 = 255;
        } else {
            banner->var2 = (16 - banner->timer) << 4;
        }
        break;
    case BANNER_HOLD:
        if (--banner->timer <= 0) {
            banner->var0++;
            banner->timer = 20;
        }
        break;
    case BANNER_SLIDE:
        banner->var1++;
        if (--banner->timer <= 0) {
            banner->var0++;
            banner->timer = 16;
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
 * centred at y 175, then 15 pairs of the season's icon, the second of
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
    char buf[64];
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
    Font_SetSpacing(28, 28);
    Font_SetPal(0);
    Text_DrawShadow2((640 - Str_Length(buf) * 14) / 2, 175, 1, 0, buf);
    if (banner->type == BANNER_OFFLINE) {
        Tex_Prepare(1, 326);
        Tex_Bind(326);
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
    spr.y = 217;
    spr.w = 32;
    spr.h = 32;
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
    for (i = 0, x = 4; i < 15; i++, x += 48) {
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
        *angle += -(ANGLE_TURN / 2);
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
 * 1074 passes frame 48; it then flies 10 frames (THROWN_FLYING) and lands
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
        if (holder->motionId[0] == 1074 && EmMotion_PassedFrameScaled(holder, 48.0f, 0)) {
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
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
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
        Gs_SetState(RS_ALPHA_REF, 128);
        Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
        matSet = &set->matSets[ThrownObj_AreaModel(obj)];
        if (matSet != NULL && matSet->id != -1) {
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
        Draw_SetDefaultState();
    }
}


/* The file's data: the season icons and the thrown objects' spots. */
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
