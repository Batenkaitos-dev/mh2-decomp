/*
 * The area state (gAreaState, AreaState in area.h): entering an area
 * (Area_Enter), its fog, its objects (gAreaInfo, then per area), the
 * player's action spots, and the per-area and per-map tables (day and night
 * ids, temperatures, the areas of each map).
 *
 * Then the area's per-frame update (Area_Update, which the area's task runs
 * each frame): the counters of the area state (gAreaState, AreaState in
 * area.h), the screen flash after a blast (gAreaFade, AREA_FADE_*), the
 * lobby's night level and its two animated hubs, the ambient life
 * around the bug-net gathering points, the ambient sounds, and last the
 * player's action-spot check.
 *
 * The ambient sounds (Area_UpdateAmbientSounds) come from per-area tables
 * of sound sources: a position, a sound id, a period in frames and a
 * range kind each, and in the lobby's hubs a bank and the AMB_SE_* flags
 * that keep a source silent by the time of day, the contests and the
 * village's facility levels.
 */
#include "common.h"
#include "scene.h"
#include "area.h"
#include "quest.h"   /* GatherPoint, ActionSpot */
#include "mine_point.h"
#include "fishing.h"
#include "sound.h"
#include "sprite.h"
#include "camera.h"
#include "player.h"
#include "em.h"
#include "syssave.h"
#include "lobby_state.h"
#include "objattach.h"
#include "qsync.h"
#include "overlay.h"
#include "render_state.h"
#include "area_draw.h"
#include "pad.h"
#include "arm_wrestling.h"
#include "bird_flock.h"
#include "drink_contest.h"
#include "engine_math.h"
#include "falling_leaves.h"
#include "gm_em.h"
#include "hit_effects.h"
#include "lobby_queries.h"
#include "lobby_screen.h"
#include "lobby_world.h"
#include "mine_model_objects.h"
#include "monster_effects.h"
#include "rand.h"
#include "town_frame.h"
#include "libc.h"
#include "ambient_life.h"

/* The objects of other files and modules this file's tables point to */
extern AreaKindRow gAreaKindFlags_1[4];
extern AreaKindRow gAreaKindFlags_10[2];
extern AreaKindRow gAreaKindFlags_100[4];
extern AreaKindRow gAreaKindFlags_120[4];
extern AreaKindRow gAreaKindFlags_121[2];
extern AreaKindRow gAreaKindFlags_122[2];
extern AreaKindRow gAreaKindFlags_123[2];
extern AreaKindRow gAreaKindFlags_124[2];
extern AreaKindRow gAreaKindFlags_126[2];
extern AreaKindRow gAreaKindFlags_129[2];
extern AreaKindRow gAreaKindFlags_13[2];
extern AreaKindRow gAreaKindFlags_130[4];
extern AreaKindRow gAreaKindFlags_144[4];
extern AreaKindRow gAreaKindFlags_158[4];
extern AreaKindRow gAreaKindFlags_159[2];
extern AreaKindRow gAreaKindFlags_16[2];
extern AreaKindRow gAreaKindFlags_162[4];
extern AreaKindRow gAreaKindFlags_163[4];
extern AreaKindRow gAreaKindFlags_166[4];
extern AreaKindRow gAreaKindFlags_167[2];
extern AreaKindRow gAreaKindFlags_169[2];
extern AreaKindRow gAreaKindFlags_177[2];
extern AreaKindRow gAreaKindFlags_178[2];
extern AreaKindRow gAreaKindFlags_18[2];
extern AreaKindRow gAreaKindFlags_19[2];
extern AreaKindRow gAreaKindFlags_2[2];
extern AreaKindRow gAreaKindFlags_20[2];
extern AreaKindRow gAreaKindFlags_21[2];
extern AreaKindRow gAreaKindFlags_22[4];
extern AreaKindRow gAreaKindFlags_23[2];
extern AreaKindRow gAreaKindFlags_26[2];
extern AreaKindRow gAreaKindFlags_27[2];
extern AreaKindRow gAreaKindFlags_3[2];
extern AreaKindRow gAreaKindFlags_32[2];
extern AreaKindRow gAreaKindFlags_33[2];
extern AreaKindRow gAreaKindFlags_34[4];
extern AreaKindRow gAreaKindFlags_35[2];
extern AreaKindRow gAreaKindFlags_36[2];
extern AreaKindRow gAreaKindFlags_37[2];
extern AreaKindRow gAreaKindFlags_38[2];
extern AreaKindRow gAreaKindFlags_39[4];
extern AreaKindRow gAreaKindFlags_4[4];
extern AreaKindRow gAreaKindFlags_40[2];
extern AreaKindRow gAreaKindFlags_41[2];
extern AreaKindRow gAreaKindFlags_42[6];
extern AreaKindRow gAreaKindFlags_44[2];
extern AreaKindRow gAreaKindFlags_45[2];
extern AreaKindRow gAreaKindFlags_46[2];
extern AreaKindRow gAreaKindFlags_47[4];
extern AreaKindRow gAreaKindFlags_48[6];
extern AreaKindRow gAreaKindFlags_49[4];
extern AreaKindRow gAreaKindFlags_5[4];
extern AreaKindRow gAreaKindFlags_50[4];
extern AreaKindRow gAreaKindFlags_51[4];
extern AreaKindRow gAreaKindFlags_52[4];
extern AreaKindRow gAreaKindFlags_53[2];
extern AreaKindRow gAreaKindFlags_54[2];
extern AreaKindRow gAreaKindFlags_55[4];
extern AreaKindRow gAreaKindFlags_56[2];
extern AreaKindRow gAreaKindFlags_57[4];
extern AreaKindRow gAreaKindFlags_58[4];
extern AreaKindRow gAreaKindFlags_59[4];
extern AreaKindRow gAreaKindFlags_60[4];
extern AreaKindRow gAreaKindFlags_62[2];
extern AreaKindRow gAreaKindFlags_63[4];
extern AreaKindRow gAreaKindFlags_64[4];
extern AreaKindRow gAreaKindFlags_66[2];
extern AreaKindRow gAreaKindFlags_67[4];
extern AreaKindRow gAreaKindFlags_7[2];
extern AreaKindRow gAreaKindFlags_71[2];
extern AreaKindRow gAreaKindFlags_72[4];
extern AreaKindRow gAreaKindFlags_75[2];
extern AreaKindRow gAreaKindFlags_83[2];
extern AreaKindRow gAreaKindFlags_84[2];
extern AreaKindRow gAreaKindFlags_88[2];
extern AreaKindRow gAreaKindFlags_9[2];
extern AreaKindRow gAreaKindFlags_92[2];
extern AreaKindRow gAreaKindFlags_93[2];
extern AreaKindRow gAreaKindFlags_94[2];
extern AreaKindRow gAreaKindFlags_96[2];
extern AreaKindRow gAreaKindFlags_97[4];
extern AreaKindRow gAreaKindFlags_98[2];
extern AreaKindRow gAreaKindFlags_99[4];
extern f32 gAreaLightColors_1[3][2][4];
extern f32 gAreaLightColors_10[3][2][4];
extern f32 gAreaLightColors_100[3][2][4];
extern f32 gAreaLightColors_120[3][2][4];
extern f32 gAreaLightColors_121[3][2][4];
extern f32 gAreaLightColors_122[3][2][4];
extern f32 gAreaLightColors_123[3][2][4];
extern f32 gAreaLightColors_124[3][2][4];
extern f32 gAreaLightColors_126[3][2][4];
extern f32 gAreaLightColors_129[3][2][4];
extern f32 gAreaLightColors_130[3][2][4];
extern f32 gAreaLightColors_144[3][2][4];
extern f32 gAreaLightColors_15[3][2][4];
extern f32 gAreaLightColors_159[3][2][4];
extern f32 gAreaLightColors_162[3][2][4];
extern f32 gAreaLightColors_169[3][2][4];
extern f32 gAreaLightColors_177[3][2][4];
extern f32 gAreaLightColors_178[3][2][4];
extern f32 gAreaLightColors_18[3][2][4];
extern f32 gAreaLightColors_19[3][2][4];
extern f32 gAreaLightColors_2[3][2][4];
extern f32 gAreaLightColors_20[3][2][4];
extern f32 gAreaLightColors_21[3][2][4];
extern f32 gAreaLightColors_22[3][2][4];
extern f32 gAreaLightColors_23[3][2][4];
extern f32 gAreaLightColors_26[3][2][4];
extern f32 gAreaLightColors_27[3][2][4];
extern f32 gAreaLightColors_3[2][2][4];
extern f32 gAreaLightColors_32[3][2][4];
extern f32 gAreaLightColors_33[3][2][4];
extern f32 gAreaLightColors_34[3][2][4];
extern f32 gAreaLightColors_35[3][2][4];
extern f32 gAreaLightColors_36[3][2][4];
extern f32 gAreaLightColors_37[3][2][4];
extern f32 gAreaLightColors_38[3][2][4];
extern f32 gAreaLightColors_39[3][2][4];
extern f32 gAreaLightColors_4[3][2][4];
extern f32 gAreaLightColors_40[3][2][4];
extern f32 gAreaLightColors_41[3][2][4];
extern f32 gAreaLightColors_42[5][2][4];
extern f32 gAreaLightColors_44[3][2][4];
extern f32 gAreaLightColors_45[3][2][4];
extern f32 gAreaLightColors_46[3][2][4];
extern f32 gAreaLightColors_47[3][2][4];
extern f32 gAreaLightColors_48[3][2][4];
extern f32 gAreaLightColors_49[3][2][4];
extern f32 gAreaLightColors_5[3][2][4];
extern f32 gAreaLightColors_50[3][2][4];
extern f32 gAreaLightColors_51[3][2][4];
extern f32 gAreaLightColors_53[3][2][4];
extern f32 gAreaLightColors_54[3][2][4];
extern f32 gAreaLightColors_55[3][2][4];
extern f32 gAreaLightColors_56[3][2][4];
extern f32 gAreaLightColors_57[3][2][4];
extern f32 gAreaLightColors_58[3][2][4];
extern f32 gAreaLightColors_59[3][2][4];
extern f32 gAreaLightColors_66[3][2][4];
extern f32 gAreaLightColors_67[3][2][4];
extern f32 gAreaLightColors_7[3][2][4];
extern f32 gAreaLightColors_83[3][2][4];
extern f32 gAreaLightColors_84[3][2][4];
extern f32 gAreaLightColors_88[3][2][4];
extern f32 gAreaLightColors_89[3][2][4];
extern f32 gAreaLightColors_90[3][2][4];
extern f32 gAreaLightColors_91[3][2][4];
extern f32 gAreaLightColors_92[3][2][4];
extern f32 gAreaLightColors_93[3][2][4];
extern f32 gAreaLightColors_94[3][2][4];
extern f32 gAreaLightColors_97[3][2][4];
extern f32 gAreaLightColors_98[3][2][4];
extern f32 gAreaLightColors_99[3][2][4];
extern AreaLightZone gAreaLightZones2_34[2];
extern AreaLightZone gAreaLightZones2_35[2];
extern AreaLightZone gAreaLightZones2_37[2];
extern AreaLightZone gAreaLightZones2_39[2];
extern AreaLightZone gAreaLightZones2_40[2];
extern AreaLightZone gAreaLightZones2_88[2];
extern AreaLightZone gAreaLightZonesAlt_1[2];
extern AreaLightZone gAreaLightZonesAlt_2[2];
extern AreaLightZone gAreaLightZonesAlt_24[2];
extern AreaLightZone gAreaLightZonesAlt_25[2];
extern AreaLightZone gAreaLightZonesAlt_26[2];
extern AreaLightZone gAreaLightZonesAlt_27[2];
extern AreaLightZone gAreaLightZonesAlt_28[2];
extern AreaLightZone gAreaLightZonesAlt_3[2];
extern AreaLightZone gAreaLightZonesAlt_30[2];
extern AreaLightZone gAreaLightZonesAlt_33[2];
extern AreaLightZone gAreaLightZonesAlt_34[2];
extern AreaLightZone gAreaLightZonesAlt_4[2];
extern AreaLightZone gAreaLightZonesAlt_48[2];
extern AreaLightZone gAreaLightZonesAlt_63[2];
extern AreaLightZone gAreaLightZonesAlt_66[2];
extern AreaLightZone gAreaLightZonesAlt_73[2];
extern AreaLightZone gAreaLightZonesAlt_81[2];
extern AreaLightZone gAreaLightZonesAlt_82[2];
extern AreaLightZone gAreaLightZones_1[2];
extern AreaLightZone gAreaLightZones_10[2];
extern AreaLightZone gAreaLightZones_15[2];
extern AreaLightZone gAreaLightZones_18[2];
extern AreaLightZone gAreaLightZones_19[2];
extern AreaLightZone gAreaLightZones_2[2];
extern AreaLightZone gAreaLightZones_20[2];
extern AreaLightZone gAreaLightZones_21[2];
extern AreaLightZone gAreaLightZones_22[2];
extern AreaLightZone gAreaLightZones_23[2];
extern AreaLightZone gAreaLightZones_26[2];
extern AreaLightZone gAreaLightZones_27[2];
extern AreaLightZone gAreaLightZones_3[2];
extern AreaLightZone gAreaLightZones_32[2];
extern AreaLightZone gAreaLightZones_33[2];
extern AreaLightZone gAreaLightZones_36[2];
extern AreaLightZone gAreaLightZones_38[2];
extern AreaLightZone gAreaLightZones_4[2];
extern AreaLightZone gAreaLightZones_41[2];
extern AreaLightZone gAreaLightZones_42[3];
extern AreaLightZone gAreaLightZones_44[2];
extern AreaLightZone gAreaLightZones_45[2];
extern AreaLightZone gAreaLightZones_46[2];
extern AreaLightZone gAreaLightZones_47[2];
extern AreaLightZone gAreaLightZones_48[2];
extern AreaLightZone gAreaLightZones_49[2];
extern AreaLightZone gAreaLightZones_5[2];
extern AreaLightZone gAreaLightZones_50[2];
extern AreaLightZone gAreaLightZones_51[2];
extern AreaLightZone gAreaLightZones_53[2];
extern AreaLightZone gAreaLightZones_54[2];
extern AreaLightZone gAreaLightZones_55[2];
extern AreaLightZone gAreaLightZones_56[2];
extern AreaLightZone gAreaLightZones_57[2];
extern AreaLightZone gAreaLightZones_58[2];
extern AreaLightZone gAreaLightZones_59[2];
extern AreaLightZone gAreaLightZones_66[2];
extern AreaLightZone gAreaLightZones_67[2];
extern AreaLightZone gAreaLightZones_7[2];
extern AreaLightZone gAreaLightZones_83[2];
extern AreaLightZone gAreaLightZones_84[2];
extern AreaLightZone gAreaLightZones_89[2];
extern AreaLightZone gAreaLightZones_90[2];
extern AreaLightZone gAreaLightZones_91[2];
extern AreaLightZone gAreaLightZones_92[2];
extern AreaLightZone gAreaLightZones_93[2];
extern AreaLightZone gAreaLightZones_94[2];
extern AreaWallKindRow gAreaTriKindFlags_1[2];
extern AreaWallKindRow gAreaTriKindFlags_11[2];
extern AreaWallKindRow gAreaTriKindFlags_12[2];
extern AreaWallKindRow gAreaTriKindFlags_139[2];
extern AreaWallKindRow gAreaTriKindFlags_144[2];
extern AreaWallKindRow gAreaTriKindFlags_17[2];
extern AreaWallKindRow gAreaTriKindFlags_176[2];
extern AreaWallKindRow gAreaTriKindFlags_2[2];
extern AreaWallKindRow gAreaTriKindFlags_20[2];
extern AreaWallKindRow gAreaTriKindFlags_25[2];
extern AreaWallKindRow gAreaTriKindFlags_28[2];
extern AreaWallKindRow gAreaTriKindFlags_30[2];
extern AreaWallKindRow gAreaTriKindFlags_32[2];
extern AreaWallKindRow gAreaTriKindFlags_38[2];
extern AreaWallKindRow gAreaTriKindFlags_48[2];
extern AreaWallKindRow gAreaTriKindFlags_49[2];
extern AreaWallKindRow gAreaTriKindFlags_50[2];
extern AreaWallKindRow gAreaTriKindFlags_55[2];
extern AreaWallKindRow gAreaTriKindFlags_57[2];
extern AreaWallKindRow gAreaTriKindFlags_6[2];
extern AreaWallKindRow gAreaTriKindFlags_83[2];
extern AreaWallKindRow gAreaTriKindFlags_97[2];
/* A spot of an area's data (52 bytes; Stage_GetAreaData): where the player can act;
 * the stage's own action spots, from Stage_GetActionSpots, are ActionSpot (quest.h). */
typedef struct AreaSpot {
    u16 id;             /* 0x00: AREA_NONE ends the list */
    s16 type;           /* 0x02: 0: a cylinder (radius, height), 1: Slab_Contains */
    f32 pos[3];         /* 0x04 */
    f32 radius;         /* 0x10 */
    f32 height;         /* 0x14 */
    f32 pos2[3];        /* 0x18 */
    f32 target[3];      /* 0x24: where the player is put */
    u16 angle;          /* 0x30: his angle, less ANGLE_TURN / 4 */
    u8 unk32[2];
} AreaSpot;
extern s16 gAreaFade;
extern s16 gAreaFadeTimer;
extern s16 gAreaTemperatures[];
extern u16 gMapDayIds[];           /* per map: its day map, 0xFFFF: itself */
extern s16 gAreaTimeOfDay[];        /* per area id: its time of day (AREA_TIME_*) */
extern u16 gAreaOtherTime[];        /* per area id: the other time's id, AREA_NONE: none */
extern u16 gMapAreaCount[];         /* per map: the number of its areas */
extern u16 *gMapAreas[];            /* per map: its area ids, in order */
void *Stage_GetAreaData(s32 area);
/* unit's own declaration: the area as u32; with the definition's u16, Area_Enter compiles differently from the game's code */
AreaRecord *Stage_GetAreaRecord(u32 id);
u8 Village_FacilityLevel(s32 i);
/* unit's own declaration: the result as s32; with the definition's u8, Player_CheckActionSpot compiles differently from the game's code */
s32 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius);
s16 Area_MonsterState(u16 area);
u16 Area_IdForCurrentTime(u16 area);
u8 Village_FacilityLevel(s32 k);
void *Gather_GetPoints(s32 area);
extern f32 gHub85EffectPos[][3];
void *Stage_GetActionSpots(s32 area);
extern u16 gAmbSeFlagsHub84[];
extern u16 gAmbSeFlagsElderHall[];
extern u16 gAmbSeFlagsHub89[];
extern f32 gAmbSePosHub84[][3];
extern s32 gAmbSeIdsHub84[];
extern s16 gAmbSePeriodsHub84[];
extern s16 gAmbSeKindsHub84[];
extern s16 gAmbSeSlotsHub84[];
extern f32 gAmbSePosElderHall[][3];
extern s32 gAmbSeIdsElderHall[];
extern s16 gAmbSePeriodsElderHall[];
extern s16 gAmbSeKindsElderHall[];
extern s16 gAmbSeSlotsElderHall[];
extern f32 gAmbSePosHub89[][3];
extern s32 gAmbSeIdsHub89[];
extern s16 gAmbSePeriodsHub89[];
extern s16 gAmbSeKindsHub89[];
extern s16 gAmbSeSlotsHub89[];
extern f32 gAmbSePosSwampNight7[][3];
extern f32 gAmbSePosSwampNight9[][3];
extern f32 gAmbSePosJungle0[][3];
extern f32 gAmbSePosJungle1[][3];
extern f32 gAmbSePosJungle4[][3];
extern f32 gAmbSePosJungle7[][3];
extern f32 gAmbSePosJungle8[][3];
extern f32 gAmbSePosTower0[][3];
extern f32 gAmbSePosTower1[][3];
extern f32 gAmbSePosVolcano7[][3];
extern f32 gAmbSePosVolcano8[][3];
/* end of prototypes */


/*
 * Enters an area: resets the area state and fetches its stage record. Online,
 * look[0] and look[1] (AREA_ONLINE_VILLAGE, AREA_ONLINE_HOUSE_0 to AREA_ONLINE_HOUSE_2) come from the lobby
 * overlay; AREA_HUB_0x89 takes look[1..3] from Village_FacilityLevel.
 * The first stores address gAreaState directly, the later ones through q:
 * that is what the original does.
 */
void Area_Enter(u16 area)
{
    AreaState *state = &gAreaState;
    s16 i;

    gAreaState.unk007 = 0;
    gAreaState.unk006 = 0;
    gAreaState.unk005 = 0;
    gAreaState.timerState = 0;
    gAreaState.active = 1;
    gAreaState.pos[2] = 0;
    gAreaState.pos[1] = 0;
    gAreaState.pos[0] = 0;
    gAreaState.rot[2] = 0;
    gAreaState.rot[1] = 0;
    gAreaState.rot[0] = 0;
    gAreaState.unk01C[2] = 0;
    gAreaState.unk01C[1] = 0;
    gAreaState.unk01C[0] = 0;
    gAreaState.area = area;
    gAreaState.record = Stage_GetAreaRecord(area);
    gAreaState.monsterState = 0;
    gAreaState.flyer = 0;
    gAreaState.propTimer = 0;
    gAreaState.propHold = 0;
    gAreaState.propState = 0;
    gAreaState.look[0] = 0;
    gAreaState.look[1] = 0;
    gAreaState.look[2] = 0;
    gAreaState.look[3] = 0;
    if (gScene.inLobby != 0) {
        if (LobbyTime_IsAm() != 0) {
            state->look[0] = 0;
        } else {
            state->look[0] = 255;
        }
    }
    switch (area) {
    case AREA_ONLINE_VILLAGE:
        if (gScene.inLobby != 0) {
            switch (LobbyWorld_GetSeason()) {
            default:
            case 2:
                state->look[1] = 2;
                break;
            case 0:
                state->look[1] = 0;
                break;
            case 1:
                state->look[1] = 1;
                break;
            }
        } else {
            state->look[1] = 2;
        }
        break;
    case AREA_HUB_0x89:
        for (i = 1; i < 4; i++) {
            state->look[i] = Village_FacilityLevel(i);
        }
        break;
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        if (gScene.inLobby != 0) {
            state->look[1] = MyHouse_BoxLevel();
        } else {
            state->look[1] = Village_FacilityLevel(5);
        }
        break;
    }
}

/* Sets the fog render states: the area's own fog, or a default. */
void Area_ApplyFog(u16 area)
{
    AreaState *state = &gAreaState;
    FogParam *fogParams;

    switch (area) {
    case 0:
        fogParams = &gFogDefault;
        break;
    case AREA_HOUSE:
        fogParams = &gFogHouse;
        break;
    default:
        if (state->envBlocks > 0) {
            fogParams = &state->fog;
        } else {
            fogParams = &gFogDefault;
        }
        break;
    }
    Gs_SetState(RS_FOG_COLOR, ((fogParams->a << 24) | (fogParams->r << 16) | (fogParams->g << 8) | fogParams->b));
    Gs_SetState(RS_FOG_NEAR, *(u32 *)&fogParams->nearDist);
    Gs_SetState(RS_FOG_FAR, *(u32 *)&fogParams->farDist);
}

/* Online only: AreaProp_Spawn(0 .. n - 1). */
void AreaProp_SpawnInLobby(s16 count)
{
    s16 i;

    if (gScene.inLobby != 0) {
        for (i = 0; i < count; i++) {
            AreaProp_Spawn(i);
        }
    }
}

/*
 * Sets up an area: resets the fog/colour block of the area state, runs the
 * four per-area setups of gAreaInfo, then starts the area's objects (game
 * overlay functions).
 */
void Area_StartObjects(u16 area)
{
    f32 pos[3];
    AreaState *state = &gAreaState;
    s16 i;

    if (gAreaState.envBlocks < 2) {
        state->skyPos[0] = 5000.0f;
        state->skyPos[1] = 5000.0f;
        state->skyPos[2] = 5000.0f;
        state->band.y = 0;
        state->band.h = 0;
        state->band.c[0] = state->band.c[1] = state->band.c[2] = state->band.c[3] = state->band.c[4] = state->band.c[5] =
            state->band.c[6] = state->band.c[7] = 0xFFFFFF;
    }
    Area_ResetFade();
    if (gAreaInfo[area].atmosType != 255) {
        AreaAtmos_Spawn(gAreaInfo[area].atmosType);
    }
    if (gAreaInfo[area].lifeSet != 255) {
        AmbientLife_Create(gAreaInfo[area].lifeSet);
    }
    if (gAreaInfo[area].scenerySet != 255) {
        AnimatedScenery_Create(gAreaInfo[area].scenerySet);
    }
    if (gAreaInfo[area].flyerPoint != 255) {
        AreaFlyer_Create(gAreaInfo[area].flyerPoint);
    } else {
        state->flyer = 0;
    }
    switch (area) {
    case AREA_FORTRESS_0:
    case AREA_FORTRESS_NIGHT_0:
        FortressTremor_Create();
        break;
    case AREA_FORTRESS_4:
        AreaWeapon_Create(0);
        break;
    case AREA_FORTRESS_5:
        AreaWeapon_Create(1);
        break;
    case AREA_FORTRESS_NIGHT_5:
        AreaWeapon_Create(6);
        break;
    case AREA_FORTRESS_1:
    case AREA_FORTRESS_NIGHT_1:
        AreaFlag_Create();
        break;
    case 17:
        for (i = 0; i < 10; i++) {
            pos[0] = 1400.0f + 300.0f * i;
            pos[1] = 30.0f;
            pos[2] = 2000.0f + 300.0f * i;
            PlacedItem_Create(pos, PLACED_LARGE_BARREL_BOMB, area, 0);
        }
        break;
    case AREA_CASTLE_0:
        CastleGate_Create(0);
        break;
    case AREA_FOREST_HILLS_0:
    case AREA_FOREST_HILLS_NIGHT_0:
        pos[0] = 11250.0f;
        pos[1] = 40.0f;
        pos[2] = 9500.0f;
        FireBlast_CreateAt(pos, 2);
        FireBlast_CreateAt(pos, 3);
        break;
    case AREA_CASTLE_1:
        AreaWeapon_Create(2);
        CastleGate_Create(1);
        DistantLightning_Create();
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        AreaWeapon_Create(3);
        AreaFlag_Create();
        BreakableStructure_Create();
        break;
    case AREA_FORTRESS_2:
        AreaWeapon_Create(4);
        break;
    case AREA_FORTRESS_NIGHT_2:
        AreaWeapon_Create(7);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        LightShaft_Create();
        break;
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        LightShaft_Create();
        AnimatedProps_Create();
        ScrollingOverlay_Create();
        break;
    case AREA_FOREST_HILLS_12:
    case AREA_FOREST_HILLS_NIGHT_12:
        LightShaft_Create();
        break;
    case AREA_DESERT_NIGHT_7:
        ScrollingOverlay_Create();
        break;
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 81, 17, pos, 60, 3);
        break;
    case AREA_FORTRESS_NIGHT_4:
        AreaWeapon_Create(5);
        break;
    case AREA_HOUSE:
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        AreaFlag_Create();
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        ScrollingOverlay_Create();
        AreaFlag_Create();
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        AreaFlag_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
        LightShaft_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        LightShaft_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        AnimatedProps_Create();
        AreaFlag_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        AnimatedProps_Create();
        break;
    case AREA_JUNGLE_NIGHT_0:
    case AREA_JUNGLE_0:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 72, 17, pos, 180, 1);
        break;
    case AREA_JUNGLE_NIGHT_1:
    case AREA_JUNGLE_1:
        GroundTileGrid_Create();
        break;
    case AREA_JUNGLE_NIGHT_3:
    case AREA_JUNGLE_3:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 72, 17, pos, 180, 1);
        break;
    case AREA_JUNGLE_NIGHT_4:
    case AREA_JUNGLE_4:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 72, 17, pos, 180, 1);
        break;
    case AREA_JUNGLE_NIGHT_6:
    case AREA_JUNGLE_6:
        GroundTileGrid_Create();
        break;
    case AREA_JUNGLE_10:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 72, 17, pos, 180, 1);
        break;
    case AREA_TOWER_8:
        LightShaft_Create();
        AreaDrop_Create();
        break;
    case AREA_ONLINE_VILLAGE:
        if (gScene.inLobby != 0) {
            AreaProp_SpawnInLobby(7);
            if (state->look[1] != 0) {
                LobbyFlock_Spawn();
            }
            BirdFlock_Spawn(0);
        }
        break;
    case AREA_HUB_0x84:
        AreaProp_SpawnInLobby(6);
        break;
    case AREA_HUB_0x85:
        AreaProp_SpawnInLobby(2);
        break;
    case AREA_TOWN_ARENA:
        break;
    case AREA_ELDER_HALL:
        AreaProp_SpawnInLobby(2);
        break;
    case AREA_HUB_0x89:
        AreaProp_SpawnInLobby(1);
        BirdFlock_Spawn(1);
        break;
    case AREA_DESERT_NIGHT_2:
    case AREA_DESERT_2:
        ScrollingOverlay_Create();
        break;
    case AREA_DESERT_NIGHT_3:
        SkyStreak_Create(0);
    case AREA_DESERT_3:
        ScrollingOverlay_Create();
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_DESERT_4:
        LightShaft_Create();
        AreaDrop_Create();
        break;
    case AREA_DESERT_NIGHT_9:
    case AREA_DESERT_9:
        ScrollingOverlay_Create();
        break;
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        AreaDrop_Create();
        ScrollingOverlay_Create();
        break;
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_NIGHT_4:
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
        LightShaft_Create();
        AreaDrop_Create();
        break;
    case AREA_SWAMP_NIGHT_5:
    case AREA_SWAMP_5:
        AnimatedGroundTiles_Create();
        break;
    case AREA_SWAMP_NIGHT_8:
    case AREA_SWAMP_8:
        AreaDrop_Create();
        break;
    case AREA_VOLCANO_NIGHT_0:
    case AREA_VOLCANO_0:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 72, 17, pos, 180, 1);
        break;
    case AREA_VOLCANO_1:
    case AREA_VOLCANO_2:
    case AREA_VOLCANO_5:
    case AREA_VOLCANO_6:
        AreaObj_K19_Create();
        break;
    case AREA_HUB_0xAA:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_HUB_0xAB:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_ONLINE_HOUSE_1:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_ONLINE_HOUSE_2:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_BATTLE_TOWN_1:
    case AREA_BATTLE_TOWN_NIGHT_1:
        AreaWeapon_Create(8);
        break;
    case AREA_BATTLE_TOWN_3:
    case AREA_BATTLE_TOWN_NIGHT_3:
        AreaWeapon_Create(9);
        break;
    case AREA_BATTLE_TOWN_0:
        TurningProp_Create();
        break;
    case AREA_BATTLE_TOWN_NIGHT_0:
        TurningProp_Create();
        AnimatedProps_Create();
        break;
    }
}

/* The number of sub-models Area_LoadSubModels loads for an area: 4 for AREA_HUB_0x89 (its own
 * and the three facilities' levels), 1 for any other. */
s16 Area_SubModelCount(u16 area)
{
    switch (area) {
    case AREA_HUB_0x89:
        return 4;
    default:
        return 1;
    }
}

/*
 * Checks whether the local player stands at one of the current area's action
 * spots (Stage_GetAreaData: 52-byte records ended by id AREA_NONE). The first one
 * found moves the player to its target position and angle.
 */
void Player_CheckActionSpot(void)
{
    f32 slabStart[3];
    f32 slabEnd[3];
    AreaSpot *spot;
    Player *pl = &gPlayers[gScene.localPlayer];
    s16 hit;
    f32 dx;
    f32 dz;

    if (gScene.inLobby == 0) {
        if (pl->hp <= 0 || gScene.questPhase == QUEST_PHASE_FAILED || gScene.questPhase == QUEST_PHASE_CLEARED) {
            if (pl->areaMoveState == PL_AREA_MOVE_ENTER) {
                pl->areaMoveState = PL_AREA_MOVE_NONE;
            }
            return;
        }
    } else if (Town_IsIdleOnline() == 0) {
        return;
    }
    if (pl->areaMoveState != PL_AREA_MOVE_NONE) {
        return;
    }
    if (pl->spotCheckDelay != 0) {
        return;
    }
    if (gScene.inLobby == 1) {
        spot = NULL;
    } else {
        spot = Stage_GetAreaData(pl->area);
    }
    if (spot == NULL) {
        return;
    }
    for (; spot->id != AREA_NONE; spot++) {
        if (pl->pos[1] >= spot->pos[1] && pl->pos[1] < spot->pos[1] + spot->height) {
            switch (spot->type) {
            case 0:
                dx = pl->pos[0] - spot->pos[0];
                dz = pl->pos[2] - spot->pos[2];
                if (Math_Sqrtf(dx * dx + dz * dz) <= spot->radius) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 1:
                slabStart[0] = spot->pos[0];
                slabStart[1] = spot->pos[1];
                slabStart[2] = spot->pos[2];
                slabEnd[0] = spot->pos2[0];
                slabEnd[1] = spot->pos2[1];
                slabEnd[2] = spot->pos2[2];
                if (Slab_Contains(pl->pos, slabStart, slabEnd, spot->height, spot->radius) != 0) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            }
            if (hit != 0) {
                pl->areaMoveState = PL_AREA_MOVE_ENTER;
                pl->nextArea = Area_IdForCurrentTime(spot->id);
                pl->startPos[0] = spot->target[0];
                pl->startPos[1] = spot->target[1];
                pl->startPos[2] = spot->target[2];
                pl->startAngle = spot->angle + ANGLE_TURN / 4;
                Player_AddSpawnOffset(pl, pl->startPos, pl->startAngle);
                if (gScene.inLobby == 0) {
                    QSync_SendPlayer(pl, QSYNC_PL_RELOCATED, 0);
                }
                break;
            }
        }
    }
}

void Area_ClearState(void)
{
    memset(&gAreaState, 0, sizeof(AreaState));
}

void Area_ResetFade(void)
{
    gAreaFade = 0;
    gAreaFadeTimer = 0;
}

/*
 * A state (0-2) of areas of kind 8, 13 or 14, cached in monsterState: 2 while a
 * monster of kind EM_KIND_KUSHALA_DAORA or EM_KIND_RUSTED_KUSHALA_DAORA is in the area; otherwise 0 when the two
 * counters of Quest_GetTime differ by less than 900, else 0 or 1 by the
 * parity of counter 0 / 7200. Kept as it is while Quest_GetOutcome(1).
 */
s16 Area_MonsterState(u16 area)
{
    Em *em = gEms;
    AreaState *state = &gAreaState;
    s16 i;

    switch (gAreaInfo[area].atmosType) {
    case 8:
    case 13:
    case 14:
        if (Quest_GetOutcome(1) != 0) {
            return state->monsterState;
        }
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->visible != 0 && (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) && em->area == area) {
                state->monsterState = 2;
                return state->monsterState;
            }
        }
        if (Quest_GetTime(1) - Quest_GetTime(0) < 900) {
            state->monsterState = 0;
        } else if ((Quest_GetTime(0) / 7200) & 1) {
            state->monsterState = 0;
        } else {
            state->monsterState = 1;
        }
        return state->monsterState;
    }
    return 0;
}

s16 Area_Temperature(u16 area)
{
    return gAreaTemperatures[area];
}

/* The day map of a night map (gMapDayIds); any other map is its own. */
u16 Map_DayId(u16 map)
{
    u16 dayMap = gMapDayIds[map];

    if (dayMap != 0xFFFF) {
        return dayMap;
    }
    return map;
}

/*
 * The group a map belongs to, as the quest lists, the quest sheets and the
 * maps' names table sort them: its day map, with the two other tower maps
 * counted as MAP_TOWER.
 */
u16 Map_Group(u16 map)
{
    u16 group = Map_DayId(map);

    switch (group) {
    case MAP_TOWER_B:
    case MAP_TOWER_C:
        group = MAP_TOWER;
        break;
    }
    return group;
}

/*
 * A zone has one area id by day and another by night: gAreaTimeOfDay gives
 * an id's time (AREA_TIME_*), gAreaOtherTime the other one's id.
 * Area_IsDay: 0 for a night area, 1 for any other and for AREA_NONE.
 */
s32 Area_IsDay(u16 area)
{
    if (area == AREA_NONE) {
        return 1;
    } else if (gAreaTimeOfDay[area] == AREA_TIME_NIGHT) {
        return 0;
    } else {
        return 1;
    }
}

/* The same zone's id at the other time of day; an area without one is its own. */
u16 Area_OtherTimeId(u16 area)
{
    u16 otherId;

    if (area == AREA_NONE) {
        return AREA_NONE;
    }
    otherId = gAreaOtherTime[area];
    if (otherId != AREA_NONE) {
        return otherId;
    }
    return area;
}

/* The day id of an area: a night area's other id, else the area itself. */
u16 Area_DayId(u16 area)
{
    if (area == AREA_NONE) {
        return AREA_NONE;
    }
    if (Area_IsDay(area) == 0) {
        return Area_OtherTimeId(area);
    }
    return area;
}

/* The id of the area's zone at the current time of day (online always by day). */
u16 Area_IdForCurrentTime(u16 area)
{
    s16 daytime;

    if (area == AREA_NONE) {
        return AREA_NONE;
    }
    daytime = gScene.inLobby == 0 ? Daytime_Get() : 0;
    if (daytime != gAreaTimeOfDay[area]) {
        return Area_OtherTimeId(area);
    }
    return area;
}

/* The index of an area in its map's list (the n of AREA_<map>_<n>); AREA_INDEX_NONE when not there. */
u16 Map_AreaIndex(u16 map, u16 area)
{
    s16 i;

    for (i = 0; i < gMapAreaCount[map]; i++) {
        if (area == gMapAreas[map][i]) {
            return i;
        }
    }
    return AREA_INDEX_NONE;
}

/* The area id at index i of a map's list; AREA_NONE for no map or no index. */
u16 Map_AreaAt(u16 map, u16 index)
{
    if (map != 0xFFFF && index != AREA_INDEX_NONE) {
        return gMapAreas[map][index];
    }
    return AREA_NONE;
}

/* 1 in an area that bars barrel bombs (Item_CanPlaceLargeBarrelBomb, Item_CanUseSmallBarrelBomb):
 * the areas listed first always, the snowy mountains' areas 5, 6 and 8 (day and night) while
 * Area_MonsterState is 2; else 0. */
s32 Area_NoBarrelBombs(u16 area)
{
    switch (area) {
    case AREA_JUNGLE_NIGHT_0:
    case AREA_JUNGLE_NIGHT_1:
    case AREA_JUNGLE_NIGHT_2:
    case AREA_JUNGLE_NIGHT_3:
    case AREA_JUNGLE_NIGHT_4:
    case AREA_JUNGLE_NIGHT_5:
    case AREA_JUNGLE_NIGHT_6:
    case AREA_FORTRESS_NIGHT_4:
    case AREA_FORTRESS_NIGHT_5:
    case AREA_FORTRESS_NIGHT_1:
    case AREA_FORTRESS_NIGHT_3:
    case AREA_FORTRESS_NIGHT_2:
    case AREA_FORTRESS_NIGHT_6:
    case AREA_SWAMP_10:
    case AREA_SWAMP_0:
    case AREA_SWAMP_1:
    case AREA_SWAMP_2:
    case AREA_SWAMP_5:
    case AREA_SWAMP_6:
    case AREA_SWAMP_7:
    case AREA_SWAMP_9:
        return 1;
    case AREA_SNOWY_MOUNTAINS_5:
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_8:
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        return Area_MonsterState(area) == 2;
    default:
        return 0;
    }
}

/* The height added to the ground under a dust effect (DustCloud_FindGround, PartFx_PutOnGround):
 * 0.0 in every area; the two swamp night areas have a case of their own that also gives 0.0. */
f32 Area_GroundOffset(u16 area)
{
    switch (area) {
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        return 0.0f;
    }
    return 0.0f;
}

s32 Area_HasFlyer(void)
{
    return gAreaState.flyer != 0;
}

/* Copies the flyer's position (gAreaState.flyerPos) to out when the area has a flyer point
 * in gAreaInfo; out is left as it is otherwise. */
void Area_GetFlyerPos(f32 *out, u16 area)
{
    if (gAreaInfo[area].flyerPoint != 255) {
        Vec_Copy(out, gAreaState.flyerPos);
    }
}

void Area_StartTimers(AreaState *state)
{
    state->timerState++;
    state->frame = 0;
}

/*
 * The area's ambient sounds. Every 87 frames sound 17 plays at the
 * stage's data point when it has one (in a quest only where the area has
 * an action spot of kind 2). In AREA_HUB_0x89, with facility 1 at level 2
 * and no contest open, sound 91 plays every 256 frames, from one of two
 * places in turn. Then each source of the area's table plays at its
 * position when gScene.frame is a multiple of its period, unless its
 * AMB_SE_* flags keep it silent; without a bank of its own it plays in
 * bank 7.
 */
void Area_UpdateAmbientSounds(AreaState *state)
{
    ActionSpot *spot;
    f32 pos[3];
    s16 *slots;
    u16 *flags;
    s32 *ids;
    f32 (*srcPos)[3];
    s16 *periods;
    s16 *kinds;
    s32 n;
    u8 hasSources;
    s32 i;
    u16 mask;
    s16 slot;

    slots = NULL;
    flags = NULL;
    if (gScene.inLobby == 0) {
        spot = Stage_GetActionSpots(gScene.area);
        if (spot != NULL) {
            for (; spot->x != -1.0f; spot++) {
                if (spot->kind == 2) {
                    if (Stage_GetDataPoint(gScene.area, pos) > 0.0f && state->frame % 87 == 0) {
                        Sound_PlaySeAtPosChecked(7, 17, 0, pos, 19, 0);
                    }
                    break;
                }
            }
        }
    } else {
        if (Stage_GetDataPoint(gScene.area, pos) > 0.0f && state->frame % 87 == 0) {
            Sound_PlaySeAtPosChecked(7, 17, 0, pos, 19, 0);
        }
    }
    hasSources = 1;
    switch (gScene.area) {
    case AREA_JUNGLE_NIGHT_0:
    case AREA_JUNGLE_0:
        srcPos = gAmbSePosJungle0;
        n = 2;
        ids = gAmbSeIdsJungle0;
        periods = gAmbSePeriodsJungle0;
        kinds = gAmbSeKindsJungle0;
        break;
    case AREA_JUNGLE_NIGHT_1:
    case AREA_JUNGLE_1:
        srcPos = gAmbSePosJungle1;
        n = 1;
        ids = gAmbSeIdsJungle1;
        periods = gAmbSePeriodsJungle1;
        kinds = gAmbSeKindsJungle1;
        break;
    case AREA_JUNGLE_NIGHT_4:
    case AREA_JUNGLE_4:
        srcPos = gAmbSePosJungle4;
        n = 1;
        ids = gAmbSeIdsJungle4;
        periods = gAmbSePeriodsJungle4;
        kinds = gAmbSeKindsJungle4;
        break;
    case AREA_JUNGLE_NIGHT_7:
    case AREA_JUNGLE_7:
        srcPos = gAmbSePosJungle7;
        n = 2;
        ids = gAmbSeIdsJungle7;
        periods = gAmbSePeriodsJungle7;
        kinds = gAmbSeKindsJungle7;
        break;
    case AREA_JUNGLE_NIGHT_8:
    case AREA_JUNGLE_8:
        srcPos = gAmbSePosJungle8;
        n = 1;
        ids = gAmbSeIdsJungle8;
        periods = gAmbSePeriodsJungle8;
        kinds = gAmbSeKindsJungle8;
        break;
    case AREA_SWAMP_NIGHT_7:
        srcPos = gAmbSePosSwampNight7;
        n = 2;
        ids = gAmbSeIdsSwampNight7;
        periods = gAmbSePeriodsSwampNight7;
        kinds = gAmbSeKindsSwampNight7;
        break;
    case AREA_SWAMP_NIGHT_9:
        srcPos = gAmbSePosSwampNight9;
        n = 3;
        ids = gAmbSeIdsSwampNight9;
        periods = gAmbSePeriodsSwampNight9;
        kinds = gAmbSeKindsSwampNight9;
        break;
    case AREA_TOWER_0:
        srcPos = gAmbSePosTower0;
        n = 1;
        ids = gAmbSeIdsTower0;
        periods = gAmbSePeriodsTower0;
        kinds = gAmbSeKindsTower0;
        break;
    case AREA_TOWER_1:
        srcPos = gAmbSePosTower1;
        n = 1;
        ids = gAmbSeIdsTower1;
        periods = gAmbSePeriodsTower1;
        kinds = gAmbSeKindsTower1;
        break;
    case AREA_HUB_0x84:
        flags = gAmbSeFlagsHub84;
        srcPos = gAmbSePosHub84;
        n = 1;
        ids = gAmbSeIdsHub84;
        periods = gAmbSePeriodsHub84;
        kinds = gAmbSeKindsHub84;
        slots = gAmbSeSlotsHub84;
        break;
    case AREA_ELDER_HALL:
        flags = gAmbSeFlagsElderHall;
        srcPos = gAmbSePosElderHall;
        n = 2;
        ids = gAmbSeIdsElderHall;
        periods = gAmbSePeriodsElderHall;
        kinds = gAmbSeKindsElderHall;
        slots = gAmbSeSlotsElderHall;
        break;
    case AREA_HUB_0x89:
        if ((AMB_SE_FACILITY1_LEVEL0 << (u8)Village_FacilityLevel(1)) == (AMB_SE_FACILITY1_LEVEL0 << 2) && gScene.inLobby != 0 && Contest_IsOpen() == 0 &&
            (state->frame & 0xFF) == 16) {
            if ((state->frame & 0x100) == 0) {
                pos[0] = 10716.6f;
                pos[1] = -58.7f;
                pos[2] = 13845.9f;
            } else {
                pos[0] = 10713.8f;
                pos[1] = -57.4f;
                pos[2] = 12591.6f;
            }
            Sound_PlaySeAtPosChecked(6, 91, 0, pos, 18, 0);
        }
        flags = gAmbSeFlagsHub89;
        srcPos = gAmbSePosHub89;
        n = 4;
        ids = gAmbSeIdsHub89;
        periods = gAmbSePeriodsHub89;
        kinds = gAmbSeKindsHub89;
        slots = gAmbSeSlotsHub89;
        break;
    case AREA_VOLCANO_NIGHT_7:
    case AREA_VOLCANO_7:
        srcPos = gAmbSePosVolcano7;
        n = 1;
        ids = gAmbSeIdsVolcano7;
        periods = gAmbSePeriodsVolcano7;
        kinds = gAmbSeKindsVolcano7;
        break;
    case AREA_VOLCANO_8:
        srcPos = gAmbSePosVolcano8;
        n = 1;
        ids = gAmbSeIdsVolcano8;
        periods = gAmbSePeriodsVolcano8;
        kinds = gAmbSeKindsVolcano8;
        break;
    default:
        hasSources = 0;
        break;
    }
    if (hasSources) {
    for (i = 0; i < n; i++) {
        if (flags != NULL && flags[i] != 0) {
            if ((flags[i] & AMB_SE_PM_ONLY) && gScene.inLobby != 0 && LobbyTime_IsAm()) {
                continue;
            }
            if ((flags[i] & AMB_SE_AM_ONLY) && gScene.inLobby != 0 && !LobbyTime_IsAm()) {
                continue;
            }
            if ((flags[i] & AMB_SE_NOT_IN_CONTEST) && gScene.inLobby != 0 && Contest_IsOpen() == 1) {
                continue;
            }
            if ((flags[i] & AMB_SE_NOT_IN_DRINK_CONTEST) && gScene.inLobby != 0 && DrinkContest_IsOn() == 1) {
                continue;
            }
            if (flags[i] & AMB_SE_FACILITY_LEVELS) {
                mask = 0;
                mask |= (u16)(AMB_SE_FACILITY1_LEVEL0 << (u8)Village_FacilityLevel(1));
                mask |= (u16)(AMB_SE_FACILITY2_LEVEL0 << (u8)Village_FacilityLevel(2));
                mask |= (u16)(AMB_SE_FACILITY3_LEVEL0 << (u8)Village_FacilityLevel(3));
                if ((mask & flags[i]) == 0) {
                    continue;
                }
            }
        }
        if (gScene.frame % periods[i] == 0) {
            pos[0] = srcPos[i][0];
            pos[1] = srcPos[i][1];
            pos[2] = srcPos[i][2];
            if (slots == NULL) {
                slot = 7;
            } else {
                slot = slots[i];
            }
            Sound_PlaySeAtPosChecked(slot, ids[i], 0, pos, kinds[i], 0);
        }
    }
    }
}

/*
 * The area's per-frame update: the frame and spin counters, the steps of
 * the screen flash (gAreaFade), then in the lobby the night level
 * (look[0], 12 a frame toward 0 in the morning, toward 255 after) and
 * the two animated hubs: AREA_HUB_0x85 spawns an effect every 10 frames at
 * one of three distinct places of gHub85EffectPos picked from the frame
 * counter's bits; AREA_HUB_0x89, with facility 3 at level 2, moves its
 * props by the lobby's state for them (1 out, then 2 back over 180
 * frames; a change from 1 waits 30 frames). In a quest, the bug-net
 * gathering points that have something left spawn an ambient-life object
 * of kind 8 near them every 128 frames. Last come the ambient sounds and
 * the player's action-spot check.
 */
void Area_Update(AreaState *state)
{
    GatherPoint *point;
    f32 pos[3];
    u16 flags;
    u16 idxA;
    u16 idxB;
    u16 idxC;
    s32 lobbyPropState;

    state->frame++;
    state->spin += 46;
    if (gAreaFade != AREA_FADE_NONE) {
        if (gAreaFade == AREA_FADE_FLASH) {
            if (--gAreaFadeTimer <= 0) {
                gAreaFade = AREA_FADE_DIP;
                gAreaFadeTimer = 2;
            }
        } else if (gAreaFade == AREA_FADE_DIP) {
            if (--gAreaFadeTimer <= 0) {
                gAreaFade = AREA_FADE_OUT;
                gAreaFadeTimer = 90; /* frames: 3 s */
            }
        } else {
            if (--gAreaFadeTimer <= 0) {
                gAreaFade = AREA_FADE_NONE;
                gAreaFadeTimer = 0;
            }
        }
    }
    if (gScene.inLobby != 0) {
        if (LobbyTime_IsAm()) {
            if (state->look[0] < 13) {
                state->look[0] = 0;
            } else {
                state->look[0] -= 12;
            }
        } else {
            if (state->look[0] >= 243) {
                state->look[0] = 255;
            } else {
                state->look[0] += 12;
            }
        }
        switch (gScene.area) {
        case AREA_HUB_0x85:
            flags = gScene.frame;
            idxA = (flags >> 5) & 0xF;
            idxB = (flags >> 7) & 0xF;
            idxC = (flags >> 9) & 0xF;
            if (idxA == idxB) {
                idxB++;
            }
            if (idxA == idxC) {
                idxC++;
                if (idxB == idxC) {
                    idxC++;
                }
            } else if (idxB == idxC) {
                idxC++;
                if (idxA == idxC) {
                    idxC++;
                }
            }
            switch (state->frame % 10) {
            case 0:
                if ((state->frame & 0x80) != 128) {
                    ModelObj_SpawnAt(gHub85EffectPos[idxA], 2, 3, 6.0f);
                }
                break;
            case 4:
                if ((state->frame & 0x100) != 256) {
                    ModelObj_SpawnAt(gHub85EffectPos[idxB], 2, 3, 6.0f);
                }
                break;
            case 8:
                if ((state->frame & 0x200) != 512) {
                    ModelObj_SpawnAt(gHub85EffectPos[idxC], 2, 3, 6.0f);
                }
                break;
            }
            break;
        case AREA_HUB_0x89:
            if ((u8)Village_FacilityLevel(3) == 2) {
                lobbyPropState = LobbyScreen_TourKind();
                if (state->propState == 1 && state->propState != lobbyPropState) {
                    if (++state->propHold < 30) {
                        state->propState = 1;
                    } else {
                        state->propState = lobbyPropState;
                    }
                } else {
                    state->propState = lobbyPropState;
                }
                switch (state->propState) {
                default:
                    state->propTimer = 0;
                    state->propHold = 0;
                    state->propOfs[0] = 0.0f;
                    state->propOfs[1] = 0.0f;
                    state->propOfs[2] = 0.0f;
                    break;
                case 1:
                    state->propTimer++;
                    if (state->propTimer > 30) {
                        state->propOfs[2] += 5.0f;
                    }
                    break;
                case 2:
                    state->propTimer++;
                    if (state->propTimer <= 180) {
                        state->propOfs[2] = -2.0f * (180 - state->propTimer);
                    } else {
                        state->propOfs[0] = 0.0f;
                        state->propOfs[1] = 0.0f;
                        state->propOfs[2] = 0.0f;
                    }
                    break;
                }
            } else {
                state->propOfs[0] = 0.0f;
                state->propOfs[1] = 0.0f;
                state->propOfs[2] = 0.0f;
            }
            break;
        }
    }
    if (gScene.inLobby == 0) {
        point = Gather_GetPoints(gScene.area);
        if (point != NULL) {
            for (; point->x != -1.0f; point++) {
                if (point->count > 0 && point->action == MINE_ACTION_BUG_NET && (gScene.frame & 0x7F) == 0) {
                    pos[0] = point->x + (f32)((Rand_Next(1) & 0x3F) - 32);
                    pos[1] = 65.0f + point->y + (f32)((Rand_Next(1) & 0x3F) - 32);
                    pos[2] = point->z + (f32)((Rand_Next(1) & 0x3F) - 32);
                    AmbientLife_CreateAt(pos, 8);
                }
            }
        }
    }
    Area_UpdateAmbientSounds(state);
    Player_CheckActionSpot();
}

/* The file's initialised data, in the game's order */
u16 gMapDayIds[32] = {
#include "sub_main/0043ED80.inc"
};
u16 gMapAreaCount[40] = {
#include "sub_main/0043EDC0.inc"
};
u16 gMapAreas_2[16] = {
#include "sub_main/0043EE10.inc"
};
u16 gMapAreas_3[16] = {
#include "sub_main/0043EE30.inc"
};
u16 gMapAreas_4[16] = {
#include "sub_main/0043EE50.inc"
};
u16 gMapAreas_5[16] = {
#include "sub_main/0043EE70.inc"
};
u16 gMapAreas_6[12] = {
#include "sub_main/0043EE90.inc"
};
u16 gMapAreas_7[4] = {
#include "sub_main/0043EEA8.inc"
};
u16 gMapAreas_8[4] = {
#include "sub_main/0043EEB0.inc"
};
u16 gMapAreas_9[4] = {
#include "sub_main/0043EEB8.inc"
};
u16 gMapAreas_10[8] = {
#include "sub_main/0043EEC0.inc"
};
u16 gMapAreas_11[12] = {
#include "sub_main/0043EED0.inc"
};
u16 gMapAreas_12[4] = {
#include "sub_main/0043EEE8.inc"
};
u16 gMapAreas_13[16] = {
#include "sub_main/0043EEF0.inc"
};
u16 gMapAreas_14[16] = {
#include "sub_main/0043EF10.inc"
};
u16 gMapAreas_15[16] = {
#include "sub_main/0043EF30.inc"
};
u16 gMapAreas_16[16] = {
#include "sub_main/0043EF50.inc"
};
u16 gMapAreas_17[16] = {
#include "sub_main/0043EF70.inc"
};
u16 gMapAreas_18[16] = {
#include "sub_main/0043EF90.inc"
};
u16 gMapAreas_19[16] = {
#include "sub_main/0043EFB0.inc"
};
u16 gMapAreas_20[16] = {
#include "sub_main/0043EFD0.inc"
};
u16 gMapAreas_21[12] = {
#include "sub_main/0043EFF0.inc"
};
u16 gMapAreas_22[4] = {
#include "sub_main/0043F008.inc"
};
u16 gMapAreas_23[8] = {
#include "sub_main/0043F010.inc"
};
u16 gMapAreas_24[4] = {
#include "sub_main/0043F020.inc"
};
u16 gMapAreas_25[4] = {
#include "sub_main/0043F028.inc"
};
u16 *gMapAreas[28] = {
    &gMapAreaCount[28], &gMapAreaCount[28], gMapAreas_2, gMapAreas_3,
    gMapAreas_4, gMapAreas_5, gMapAreas_6, gMapAreas_7,
    gMapAreas_8, gMapAreas_9, gMapAreas_10, gMapAreas_11,
    gMapAreas_12, gMapAreas_13, gMapAreas_14, gMapAreas_15,
    gMapAreas_16, gMapAreas_17, gMapAreas_18, gMapAreas_19,
    gMapAreas_20, gMapAreas_21, gMapAreas_22, gMapAreas_23,
    gMapAreas_24, gMapAreas_25, NULL, NULL,
};
Vec3f gStartPos[197] = {
#include "sub_main/0043F0A0.inc"
};
u16 gStartAngle[200] = {
#include "sub_main/0043F9E0.inc"
};
/* The objects the area tables below point to: the rows of the areas from 170
 * on (equip_save.c), and the zero rows in .bss of the areas without floor or
 * wall kinds of their own. */
extern AreaKindRow gAreaKindFlags_170[2];
extern AreaLightZone gAreaLightZones_170[2];
extern f32 gAreaLightColors_170[6][4];
extern AreaKindRow gAreaKindRowsNone[];
extern AreaWallKindRow gAreaWallKindRowsNone[];
AreaKindRow *gAreaKindFlags[200] = {
#include "sub_main/0043FB70.inc"
};
AreaLightZone *gAreaLightZones[200] = {
#include "sub_main/0043FE90.inc"
};
AreaLightZone *gAreaLightZones2[200] = {
#include "sub_main/004401B0.inc"
};
u8 *gAreaLightColors[200] = {
#include "sub_main/004404D0.inc"
};
AreaWallKindRow *gAreaTriKindFlags[200] = {
#include "sub_main/004407F0.inc"
};
s16 gAreaKinds[200] = {
#include "sub_main/00440B10.inc"
};
s16 gAreaTemperatures[200] = {
#include "sub_main/00440CA0.inc"
};
s16 gAreaTimeOfDay[200] = {
#include "sub_main/00440E30.inc"
};
u16 gAreaOtherTime[200] = {
#include "sub_main/00440FC0.inc"
};
