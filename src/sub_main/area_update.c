/*
 * The area's per-frame update (Area_Update, which the area's task runs
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


s32 LobbyTime_IsAm(void);
s32 LobbyScreen_TourKind(void);
u8 Village_FacilityLevel(s32 k);
void *Gather_GetPoints(s32 area);
u16 Rand_Next(s32 stream);
void AmbientLife_CreateAt(void *src, s16 k);
void ModelObj_SpawnAt(f32 *pos, s32 kind, s32 k7, f32 scale);
void Area_UpdateAmbientSounds(AreaState *s);
void Player_CheckActionSpot(void);
extern s16 gAreaFade;
extern s16 gAreaFadeTimer;
extern f32 gHub85EffectPos[][3];


void *Stage_GetActionSpots(s32 area);
f32 Stage_GetDataPoint(s32 area, f32 *pos);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 prog, void *pos, s32 range, s32 update);
s32 Contest_IsOpen(void);
s32 DrinkContest_IsOn(void);
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
extern s32 gAmbSeIdsSwampNight7[];
extern s16 gAmbSePeriodsSwampNight7[];
extern s16 gAmbSeKindsSwampNight7[];
extern f32 gAmbSePosSwampNight9[][3];
extern s32 gAmbSeIdsSwampNight9[];
extern s16 gAmbSePeriodsSwampNight9[];
extern s16 gAmbSeKindsSwampNight9[];
extern f32 gAmbSePosJungle0[][3];
extern s32 gAmbSeIdsJungle0[];
extern s16 gAmbSePeriodsJungle0[];
extern s16 gAmbSeKindsJungle0[];
extern f32 gAmbSePosJungle1[][3];
extern s32 gAmbSeIdsJungle1[];
extern s16 gAmbSePeriodsJungle1[];
extern s16 gAmbSeKindsJungle1[];
extern f32 gAmbSePosJungle4[][3];
extern s32 gAmbSeIdsJungle4[];
extern s16 gAmbSePeriodsJungle4[];
extern s16 gAmbSeKindsJungle4[];
extern f32 gAmbSePosJungle7[][3];
extern s32 gAmbSeIdsJungle7[];
extern s16 gAmbSePeriodsJungle7[];
extern s16 gAmbSeKindsJungle7[];
extern f32 gAmbSePosJungle8[][3];
extern s32 gAmbSeIdsJungle8[];
extern s16 gAmbSePeriodsJungle8[];
extern s16 gAmbSeKindsJungle8[];
extern f32 gAmbSePosTower0[][3];
extern s32 gAmbSeIdsTower0[];
extern s16 gAmbSePeriodsTower0[];
extern s16 gAmbSeKindsTower0[];
extern f32 gAmbSePosTower1[][3];
extern s32 gAmbSeIdsTower1[];
extern s16 gAmbSePeriodsTower1[];
extern s16 gAmbSeKindsTower1[];
extern f32 gAmbSePosVolcano7[][3];
extern s32 gAmbSeIdsVolcano7[];
extern s16 gAmbSePeriodsVolcano7[];
extern s16 gAmbSeKindsVolcano7[];
extern f32 gAmbSePosVolcano8[][3];
extern s32 gAmbSeIdsVolcano8[];
extern s16 gAmbSePeriodsVolcano8[];
extern s16 gAmbSeKindsVolcano8[];

/*
 * The area's ambient sounds. Every 87 frames sound 0x11 plays at the
 * stage's data point when it has one (in a quest only where the area has
 * an action spot of kind 2). In AREA_HUB_0x89, with facility 1 at level 2
 * and no contest open, sound 0x5B plays every 256 frames, from one of two
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
                        Sound_PlaySeAtPosChecked(7, 0x11, 0, pos, 0x13, 0);
                    }
                    break;
                }
            }
        }
    } else {
        if (Stage_GetDataPoint(gScene.area, pos) > 0.0f && state->frame % 87 == 0) {
            Sound_PlaySeAtPosChecked(7, 0x11, 0, pos, 0x13, 0);
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
            (state->frame & 0xFF) == 0x10) {
            if ((state->frame & 0x100) == 0) {
                pos[0] = 10716.6f;
                pos[1] = -58.7f;
                pos[2] = 13845.9f;
            } else {
                pos[0] = 10713.8f;
                pos[1] = -57.4f;
                pos[2] = 12591.6f;
            }
            Sound_PlaySeAtPosChecked(6, 0x5B, 0, pos, 0x12, 0);
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
 * (look[0], 12 a frame toward 0 in the morning, toward 0xFF after) and
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
    state->spin += 0x2E;
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
            if (state->look[0] < 0xD) {
                state->look[0] = 0;
            } else {
                state->look[0] -= 0xC;
            }
        } else {
            if (state->look[0] >= 0xF3) {
                state->look[0] = 0xFF;
            } else {
                state->look[0] += 0xC;
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
                if ((state->frame & 0x80) != 0x80) {
                    ModelObj_SpawnAt(gHub85EffectPos[idxA], 2, 3, 6.0f);
                }
                break;
            case 4:
                if ((state->frame & 0x100) != 0x100) {
                    ModelObj_SpawnAt(gHub85EffectPos[idxB], 2, 3, 6.0f);
                }
                break;
            case 8:
                if ((state->frame & 0x200) != 0x200) {
                    ModelObj_SpawnAt(gHub85EffectPos[idxC], 2, 3, 6.0f);
                }
                break;
            }
            break;
        case AREA_HUB_0x89:
            if ((u8)Village_FacilityLevel(3) == 2) {
                lobbyPropState = LobbyScreen_TourKind();
                if (state->propState == 1 && state->propState != lobbyPropState) {
                    if (++state->propHold < 0x1E) {
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
                    if (state->propTimer > 0x1E) {
                        state->propOfs[2] += 5.0f;
                    }
                    break;
                case 2:
                    state->propTimer++;
                    if (state->propTimer <= 0xB4) {
                        state->propOfs[2] = -2.0f * (0xB4 - state->propTimer);
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
                    pos[0] = point->x + (f32)((Rand_Next(1) & 0x3F) - 0x20);
                    pos[1] = 65.0f + point->y + (f32)((Rand_Next(1) & 0x3F) - 0x20);
                    pos[2] = point->z + (f32)((Rand_Next(1) & 0x3F) - 0x20);
                    AmbientLife_CreateAt(pos, 8);
                }
            }
        }
    }
    Area_UpdateAmbientSounds(state);
    Player_CheckActionSpot();
}
