#include "common.h"
#include "scene.h"
#include "area.h"
#include "quest.h"   /* GatherPoint, ActionSpot */


s32 LobbyTime_IsAm(void);
s32 func_7BC280(void);
u8 Village_FacilityLevel(s32 k);
void *Gather_GetPoints(s32 area);
u16 Rand_Next(s32 stream);
void AreaObj_K09_CreateAt(void *src, s16 k);
void ModelObj_SpawnAt(f32 *pos, s32 kind, s32 k7, f32 scale);
void Area_UpdateAmbientSounds(AreaState *s);
void Player_CheckActionSpot(void);
extern s16 gAreaFade;
extern s16 gAreaFadeTimer;
extern f32 gArea85EffectPos[][3];


void *Stage_GetActionSpots(s32 area);
f32 Stage_GetDataPoint(s32 area, f32 *pos);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Contest_IsOpen(void);
s32 DrinkContest_IsOn(void);
extern u16 gAmbSeFlagsArea84[];
extern u16 gAmbSeFlagsArea88[];
extern u16 gAmbSeFlagsArea89[];
extern f32 gAmbSePosArea84[][3];
extern s32 gAmbSeIdsArea84[];
extern s16 gAmbSePeriodsArea84[];
extern s16 gAmbSeKindsArea84[];
extern s16 gAmbSeSlotsArea84[];
extern f32 gAmbSePosArea88[][3];
extern s32 gAmbSeIdsArea88[];
extern s16 gAmbSePeriodsArea88[];
extern s16 gAmbSeKindsArea88[];
extern s16 gAmbSeSlotsArea88[];
extern f32 gAmbSePosArea89[][3];
extern s32 gAmbSeIdsArea89[];
extern s16 gAmbSePeriodsArea89[];
extern s16 gAmbSeKindsArea89[];
extern s16 gAmbSeSlotsArea89[];
extern f32 gAmbSePosArea48[][3];
extern s32 gAmbSeIdsArea48[];
extern s16 gAmbSePeriodsArea48[];
extern s16 gAmbSeKindsArea48[];
extern f32 gAmbSePosArea4B[][3];
extern s32 gAmbSeIdsArea4B[];
extern s16 gAmbSePeriodsArea4B[];
extern s16 gAmbSeKindsArea4B[];
extern f32 gAmbSePosArea01[][3];
extern s32 gAmbSeIdsArea01[];
extern s16 gAmbSePeriodsArea01[];
extern s16 gAmbSeKindsArea01[];
extern f32 gAmbSePosArea02[][3];
extern s32 gAmbSeIdsArea02[];
extern s16 gAmbSePeriodsArea02[];
extern s16 gAmbSeKindsArea02[];
extern f32 gAmbSePosArea05[][3];
extern s32 gAmbSeIdsArea05[];
extern s16 gAmbSePeriodsArea05[];
extern s16 gAmbSeKindsArea05[];
extern f32 gAmbSePosArea16[][3];
extern s32 gAmbSeIdsArea16[];
extern s16 gAmbSePeriodsArea16[];
extern s16 gAmbSeKindsArea16[];
extern f32 gAmbSePosArea17[][3];
extern s32 gAmbSeIdsArea17[];
extern s16 gAmbSePeriodsArea17[];
extern s16 gAmbSeKindsArea17[];
extern f32 gAmbSePosArea79[][3];
extern s32 gAmbSeIdsArea79[];
extern s16 gAmbSePeriodsArea79[];
extern s16 gAmbSeKindsArea79[];
extern f32 gAmbSePosArea7A[][3];
extern s32 gAmbSeIdsArea7A[];
extern s16 gAmbSePeriodsArea7A[];
extern s16 gAmbSeKindsArea7A[];
extern f32 gAmbSePosArea41[][3];
extern s32 gAmbSeIdsArea41[];
extern s16 gAmbSePeriodsArea41[];
extern s16 gAmbSeKindsArea41[];
extern f32 gAmbSePosAreaA9[][3];
extern s32 gAmbSeIdsAreaA9[];
extern s16 gAmbSePeriodsAreaA9[];
extern s16 gAmbSeKindsAreaA9[];

/* The area's ambient sounds: the spot sound of kind-2 action spots every 87 frames, a
 * sound in area 0x89 at a farm level, then the area's table of sound sources: each plays
 * at its position when gScene.frame is a multiple of its period, unless its flags rule it out
 * (the time of day, facility levels). */
void Area_UpdateAmbientSounds(AreaState *s)
{
    ActionSpot *sp;
    f32 pos[3];
    s16 *slots;
    u16 *flags;
    s32 *ids;
    f32 (*srcPos)[3];
    s16 *periods;
    s16 *kinds;
    s32 n;
    u8 on;
    s32 i;
    u16 mask;
    s16 slot;

    slots = NULL;
    flags = NULL;
    if (gScene.online == 0) {
        sp = Stage_GetActionSpots(gScene.area);
        if (sp != NULL) {
            for (; sp->x != -1.0f; sp++) {
                if (sp->kind == 2) {
                    if (Stage_GetDataPoint(gScene.area, pos) > 0.0f && s->frame % 87 == 0) {
                        Sound_PlaySeAtPosChecked(7, 0x11, 0, pos, 0x13, 0);
                    }
                    break;
                }
            }
        }
    } else {
        if (Stage_GetDataPoint(gScene.area, pos) > 0.0f && s->frame % 87 == 0) {
            Sound_PlaySeAtPosChecked(7, 0x11, 0, pos, 0x13, 0);
        }
    }
    on = 1;
    switch (gScene.area) {
    case 1:
    case 0x6E:
        srcPos = gAmbSePosArea01;
        n = 2;
        ids = gAmbSeIdsArea01;
        periods = gAmbSePeriodsArea01;
        kinds = gAmbSeKindsArea01;
        break;
    case 2:
    case 0x6F:
        srcPos = gAmbSePosArea02;
        n = 1;
        ids = gAmbSeIdsArea02;
        periods = gAmbSePeriodsArea02;
        kinds = gAmbSeKindsArea02;
        break;
    case 5:
    case 0x72:
        srcPos = gAmbSePosArea05;
        n = 1;
        ids = gAmbSeIdsArea05;
        periods = gAmbSePeriodsArea05;
        kinds = gAmbSeKindsArea05;
        break;
    case 0x16:
    case 0x75:
        srcPos = gAmbSePosArea16;
        n = 2;
        ids = gAmbSeIdsArea16;
        periods = gAmbSePeriodsArea16;
        kinds = gAmbSeKindsArea16;
        break;
    case 0x17:
    case 0x76:
        srcPos = gAmbSePosArea17;
        n = 1;
        ids = gAmbSeIdsArea17;
        periods = gAmbSePeriodsArea17;
        kinds = gAmbSeKindsArea17;
        break;
    case 0x48:
        srcPos = gAmbSePosArea48;
        n = 2;
        ids = gAmbSeIdsArea48;
        periods = gAmbSePeriodsArea48;
        kinds = gAmbSeKindsArea48;
        break;
    case 0x4B:
        srcPos = gAmbSePosArea4B;
        n = 3;
        ids = gAmbSeIdsArea4B;
        periods = gAmbSePeriodsArea4B;
        kinds = gAmbSeKindsArea4B;
        break;
    case 0x79:
        srcPos = gAmbSePosArea79;
        n = 1;
        ids = gAmbSeIdsArea79;
        periods = gAmbSePeriodsArea79;
        kinds = gAmbSeKindsArea79;
        break;
    case 0x7A:
        srcPos = gAmbSePosArea7A;
        n = 1;
        ids = gAmbSeIdsArea7A;
        periods = gAmbSePeriodsArea7A;
        kinds = gAmbSeKindsArea7A;
        break;
    case 0x84:
        flags = gAmbSeFlagsArea84;
        srcPos = gAmbSePosArea84;
        n = 1;
        ids = gAmbSeIdsArea84;
        periods = gAmbSePeriodsArea84;
        kinds = gAmbSeKindsArea84;
        slots = gAmbSeSlotsArea84;
        break;
    case 0x88:
        flags = gAmbSeFlagsArea88;
        srcPos = gAmbSePosArea88;
        n = 2;
        ids = gAmbSeIdsArea88;
        periods = gAmbSePeriodsArea88;
        kinds = gAmbSeKindsArea88;
        slots = gAmbSeSlotsArea88;
        break;
    case 0x89:
        if ((2 << (u8)Village_FacilityLevel(1)) == 8 && gScene.online != 0 && Contest_IsOpen() == 0 &&
            (s->frame & 0xFF) == 0x10) {
            if ((s->frame & 0x100) == 0) {
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
        flags = gAmbSeFlagsArea89;
        srcPos = gAmbSePosArea89;
        n = 4;
        ids = gAmbSeIdsArea89;
        periods = gAmbSePeriodsArea89;
        kinds = gAmbSeKindsArea89;
        slots = gAmbSeSlotsArea89;
        break;
    case 0x41:
    case 0xA8:
        srcPos = gAmbSePosArea41;
        n = 1;
        ids = gAmbSeIdsArea41;
        periods = gAmbSePeriodsArea41;
        kinds = gAmbSeKindsArea41;
        break;
    case 0xA9:
        srcPos = gAmbSePosAreaA9;
        n = 1;
        ids = gAmbSeIdsAreaA9;
        periods = gAmbSePeriodsAreaA9;
        kinds = gAmbSeKindsAreaA9;
        break;
    default:
        on = 0;
        break;
    }
    if (on) {
    for (i = 0; i < n; i++) {
        if (flags != NULL && flags[i] != 0) {
            if ((flags[i] & 1) && gScene.online != 0 && LobbyTime_IsAm()) {
                continue;
            }
            if ((flags[i] & 0x1000) && gScene.online != 0 && !LobbyTime_IsAm()) {
                continue;
            }
            if ((flags[i] & 0x400) && gScene.online != 0 && Contest_IsOpen() == 1) {
                continue;
            }
            if ((flags[i] & 0x800) && gScene.online != 0 && DrinkContest_IsOn() == 1) {
                continue;
            }
            if (flags[i] & 0x3FE) {
                mask = 0;
                mask |= (u16)(2 << (u8)Village_FacilityLevel(1));
                mask |= (u16)(0x10 << (u8)Village_FacilityLevel(2));
                mask |= (u16)(0x80 << (u8)Village_FacilityLevel(3));
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

/* The area's per-frame update: its counters, the screen fade (gAreaFade), the notice darkness,
 * the effects of areas 0x85 and 0x89, the sparkles of gathering points, then Area_UpdateAmbientSounds and
 * the action spots. */
void Area_Update(AreaState *s)
{
    GatherPoint *gp;
    f32 pos[3];
    u16 flags;
    u16 b;
    u16 a;
    u16 c;
    s32 r;

    s->frame++;
    s->spin += 0x2E;
    if (gAreaFade != 0) {
        if (gAreaFade == 1) {
            if (--gAreaFadeTimer <= 0) {
                gAreaFade = 2;
                gAreaFadeTimer = 2;
            }
        } else if (gAreaFade == 2) {
            if (--gAreaFadeTimer <= 0) {
                gAreaFade = 3;
                gAreaFadeTimer = 0x5A;
            }
        } else {
            if (--gAreaFadeTimer <= 0) {
                gAreaFade = 0;
                gAreaFadeTimer = 0;
            }
        }
    }
    if (gScene.online != 0) {
        if (LobbyTime_IsAm()) {
            if (s->look[0] < 0xD) {
                s->look[0] = 0;
            } else {
                s->look[0] -= 0xC;
            }
        } else {
            if (s->look[0] >= 0xF3) {
                s->look[0] = 0xFF;
            } else {
                s->look[0] += 0xC;
            }
        }
        switch (gScene.area) {
        case 0x85:
            flags = gScene.frame;
            b = (flags >> 5) & 0xF;
            a = (flags >> 7) & 0xF;
            c = (flags >> 9) & 0xF;
            if (b == a) {
                a++;
            }
            if (b == c) {
                c++;
                if (a == c) {
                    c++;
                }
            } else if (a == c) {
                c++;
                if (b == c) {
                    c++;
                }
            }
            switch (s->frame % 10) {
            case 0:
                if ((s->frame & 0x80) != 0x80) {
                    ModelObj_SpawnAt(gArea85EffectPos[b], 2, 3, 6.0f);
                }
                break;
            case 4:
                if ((s->frame & 0x100) != 0x100) {
                    ModelObj_SpawnAt(gArea85EffectPos[a], 2, 3, 6.0f);
                }
                break;
            case 8:
                if ((s->frame & 0x200) != 0x200) {
                    ModelObj_SpawnAt(gArea85EffectPos[c], 2, 3, 6.0f);
                }
                break;
            }
            break;
        case 0x89:
            if ((u8)Village_FacilityLevel(3) == 2) {
                r = func_7BC280();
                if (s->propState == 1 && s->propState != r) {
                    if (++s->propHold < 0x1E) {
                        s->propState = 1;
                    } else {
                        s->propState = r;
                    }
                } else {
                    s->propState = r;
                }
                switch (s->propState) {
                default:
                    s->propTimer = 0;
                    s->propHold = 0;
                    s->propOfs[0] = 0.0f;
                    s->propOfs[1] = 0.0f;
                    s->propOfs[2] = 0.0f;
                    break;
                case 1:
                    s->propTimer++;
                    if (s->propTimer > 0x1E) {
                        s->propOfs[2] += 5.0f;
                    }
                    break;
                case 2:
                    s->propTimer++;
                    if (s->propTimer <= 0xB4) {
                        s->propOfs[2] = -2.0f * (0xB4 - s->propTimer);
                    } else {
                        s->propOfs[0] = 0.0f;
                        s->propOfs[1] = 0.0f;
                        s->propOfs[2] = 0.0f;
                    }
                    break;
                }
            } else {
                s->propOfs[0] = 0.0f;
                s->propOfs[1] = 0.0f;
                s->propOfs[2] = 0.0f;
            }
            break;
        }
    }
    if (gScene.online == 0) {
        gp = Gather_GetPoints(gScene.area);
        if (gp != NULL) {
            for (; gp->x != -1.0f; gp++) {
                if (gp->count > 0 && gp->unk14 == 4 && (gScene.frame & 0x7F) == 0) {
                    pos[0] = gp->x + (f32)((Rand_Next(1) & 0x3F) - 0x20);
                    pos[1] = 65.0f + gp->y + (f32)((Rand_Next(1) & 0x3F) - 0x20);
                    pos[2] = gp->z + (f32)((Rand_Next(1) & 0x3F) - 0x20);
                    AreaObj_K09_CreateAt(pos, 8);
                }
            }
        }
    }
    Area_UpdateAmbientSounds(s);
    Player_CheckActionSpot();
}
