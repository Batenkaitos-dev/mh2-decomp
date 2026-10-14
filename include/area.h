#ifndef AREA_H
#define AREA_H

/*
 * The state of the current area: gAreaState (sub_main 0x5EC460, 0x120 bytes). Area_ClearState
 * clears it, Area_Enter and Area_Load set it up, the area pack's part 2 fills its fog, lights
 * and sky (AreaPack_ReadPart2), Area_UpdateTimers runs it each frame (sub_main unk_379EC0.c,
 * unk_37AC80.c), and the area's sky and props draw from it (unk_37B570.c).
 */

#include "common.h"

/* Fog parameters (Area_ApplyFog: render states 0xF, 0x10, 0x11; gFogDefault, gFogArea56, the
 * area pack's part 0: the colour first, then the two distances). */
typedef struct FogParam {
    f32 nearDist;               /* 0x0: render state 0x10 (gGsFogNear) */
    f32 farDist;                /* 0x4: render state 0x11 (gGsFogFar) */
    u8 r;                       /* 0x8 */
    u8 g;                       /* 0x9 */
    u8 b;                       /* 0xA */
    u8 a;                       /* 0xB */
} FogParam;
typedef char FogParam_size_check[(sizeof(FogParam) == 0xC) ? 1 : -1];

/* A sky band: its rectangle and its corner colours for four headings (Sky_DrawBand). */
typedef struct SkyBand {
    s16 y;                      /* 0x00 */
    s16 h;                      /* 0x02 */
    u32 c[8];                   /* 0x04 */
} SkyBand;
typedef char SkyBand_size_check[(sizeof(SkyBand) == 0x24) ? 1 : -1];

typedef struct AreaState {
    u8 active;                  /* 0x000: 1 once set up (Area_Enter); the sky is drawn only then */
    u8 drawSky;                 /* 0x001: the sky and the glow props are drawn (AreaSky_Draw, Sky_Draw,
                                 * Area_DrawGlowProps): set once the field is up (Scene_StartField) */
    u8 unk002[0x004 - 0x002];
    u8 timerState;              /* 0x004: Area_UpdateTimers: 0 starts (Area_StartTimers), 1 runs
                                 * (Area_Update) */
    u8 unk005;                  /* 0x005 */
    u8 unk006;                  /* 0x006 */
    u8 unk007;                  /* 0x007 */
    s16 frame;                  /* 0x008: the frame counter (+1 per frame): the area animations' time */
    s16 spin;                   /* 0x00A: +0x2E per frame (AreaAnim_SpinScroll's angle) */
    u8 unk00C[0x010 - 0x00C];
    f32 pos[3];                 /* 0x010: the area model's position (AreaSky_Draw) */
    f32 unk01C[3];              /* 0x01C */
    f32 rot[3];                 /* 0x028: and its rotation */
    s16 modelCount;             /* 0x034: the area's model slots (Area_FreeModels): how many */
    u8 unk036[0x038 - 0x036];
    s16 modelSlot;              /* 0x038: the first one (Area_Load) */
    u8 unk03A[0x03C - 0x03A];
    struct Model *model;        /* 0x03C: the area model (Model_Get(modelSlot)) */
    u8 unk040[0x048 - 0x040];
    struct AreaRecord *record;  /* 0x048: the area's place in the map (Stage_GetAreaRecord) */
    u8 unk04C;                  /* 0x04C */
    u8 envBlocks;               /* 0x04D: the blocks the area pack's part 2 gave (AreaPack_ReadPart2):
                                 * 0 none, 1 the fog and the lights, 2 also the sky (skyPos, band);
                                 * below 2 Area_StartObjects sets the sky's defaults */
    u16 area;                   /* 0x04E: the area id */
    FogParam fog;               /* 0x050: the area's own fog (Area_ApplyFog) */
    f32 lightDir[3][3];         /* 0x05C: the area's own three lights (Light_SetDefault): directions */
    f32 lightAmb[3][3];         /* 0x080: their ambient colours */
    f32 lightCol[3][3];         /* 0x0A4: their colours */
    f32 skyPos[3];              /* 0x0C8: a far point (5000.0 each by default): Sky_DrawBand turns the
                                 * band's colours by the camera's heading to it, AmbientSound_Draw
                                 * fades by the camera's angle to it, AmbientSound_Update places
                                 * its sound there, unk_3BE060.c turns a player's part to it */
    SkyBand band;               /* 0x0D4: the sky band (Sky_Draw) */
    u8 look[4];                 /* 0x0F8: [0] the night level (0 day .. 0xFF night: online it follows
                                 * LobbyTime_IsAm, 0xC a frame; darkens the props and notices);
                                 * [1] the area's variant (the season of area 0x83, the house level
                                 * of 0xAD-0xAF; Area_DrawProp's props of one variant);
                                 * [1..3] the facility levels of area 0x89 (Area_Enter) */
    u8 monsterState;            /* 0x0FC: cached result of Area_MonsterState (0-2) */
    u8 flyer;                   /* 0x0FD: set while a kind 0x1D object exists (StageObj_AreaFlyer_Create) */
    u16 propTimer;              /* 0x0FE: the frames of the moving props' state (area 0x89, Area_Update) */
    f32 propOfs[3];             /* 0x100: the moving props' offset (Area_DrawProp's kind 13) */
    f32 flyerPos[3];            /* 0x10C: the kind 0x1D object's position (StageObj_AreaFlyer_Run) */
    u8 propState;               /* 0x118: the moving props' state (func_7BC280: 1 out, 2 back) */
    u8 propHold;                /* 0x119: frames state 1 is held after func_7BC280 left it */
    u8 unk11A[0x120 - 0x11A];
} AreaState;
typedef char AreaState_size_check[(sizeof(AreaState) == 0x120) ? 1 : -1];

extern AreaState gAreaState;

#endif /* AREA_H */
