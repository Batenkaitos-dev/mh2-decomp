/*
 * The scene's lights.
 *
 * - The two light sets (gLightSets: two LightSets of three Lights each). Set 0 is fixed: Light_InitAll
 *   loads it from gLightInitSources. Set 1 lights the hunters, the monsters and the lobby's people: it
 *   takes the area's own lights (AreaState's lightDir, lightColor and lightAmbient) when the area file
 *   has them, else the default tables (Light_SetDefault; AREA_HOUSE has its own defaults).
 *   LightSet_UpdateAll steps the sets each frame and points set 1's third light down through the
 *   camera. Before an object is drawn, LightSet_SetColors gives set 1 the object's colours and
 *   LightSet_FadeColors fades them in and hands the lights to the renderer; LightSet_ResetColors and
 *   LightSet_Apply put a set back and hand it over as it is.
 */
#include "common.h"
#include "player.h"
#include "scene.h"
#include "area.h"
#include "light.h"
#include "objhead.h"
#include "render_state.h"

/* prototypes */
s32 Gs_SetState(u8 reg, u32 v);
/* end of prototypes */

extern LightSets gLightSets;

extern f32 gLightDefaultDirs[];
extern f32 gLightHouseDirs[];
extern f32 gLightDefaultColors[];
extern f32 gLightHouseColors[];
extern f32 gLightDefaultAmbients[];
extern f32 gLightHouseAmbients[];

/* What Light_SetDefault sets. */
enum LightSetPart {
    LIGHT_SET_ALL = 0,      /* the direction, the colour and the ambient part */
    LIGHT_SET_COLOR = 1     /* the colour only */
};

/* Sets `light` as light i of set 1 (`what`: LIGHT_SET_ALL or LIGHT_SET_COLOR): from the area's own
 * lights when the area file has them and `area` isn't 0, else from the default tables. AREA_HOUSE first
 * takes its own defaults, which the area's own lights then replace when it has them. */
static void Light_SetDefault(Light *light, u16 area, s32 i, u16 mode)
{
    AreaState *areaState = &gAreaState;

    switch (area) {
    case AREA_HOUSE:
        switch (mode) {
        case LIGHT_SET_ALL:
            light->dir[0] = gLightHouseDirs[(s16)i * 3];
            light->dir[1] = gLightHouseDirs[(s16)i * 3 + 1];
            light->dir[2] = gLightHouseDirs[(s16)i * 3 + 2];
            light->ambient[0] = gLightHouseAmbients[(s16)i * 4];
            light->ambient[1] = gLightHouseAmbients[(s16)i * 4 + 1];
            light->ambient[2] = gLightHouseAmbients[(s16)i * 4 + 2];
        }
        switch (mode) {
        case LIGHT_SET_ALL:
        case LIGHT_SET_COLOR:
            light->color[0] = gLightHouseColors[(s16)i * 4];
            light->color[1] = gLightHouseColors[(s16)i * 4 + 1];
            light->color[2] = gLightHouseColors[(s16)i * 4 + 2];
        }
    }
    if (area != 0 && areaState->envBlocks > 0) {
        switch (mode) {
        case LIGHT_SET_ALL:
            light->dir[0] = areaState->lightDir[(s16)i][0];
            light->dir[1] = areaState->lightDir[(s16)i][1];
            light->dir[2] = areaState->lightDir[(s16)i][2];
            light->ambient[0] = areaState->lightAmbient[(s16)i][0];
            light->ambient[1] = areaState->lightAmbient[(s16)i][1];
            light->ambient[2] = areaState->lightAmbient[(s16)i][2];
        }
        switch (mode) {
        case LIGHT_SET_ALL:
        case LIGHT_SET_COLOR:
            light->color[0] = areaState->lightColor[(s16)i][0];
            light->color[1] = areaState->lightColor[(s16)i][1];
            light->color[2] = areaState->lightColor[(s16)i][2];
        }
    } else {
        switch (mode) {
        case LIGHT_SET_ALL:
            light->dir[0] = gLightDefaultDirs[(s16)i * 3];
            light->dir[1] = gLightDefaultDirs[(s16)i * 3 + 1];
            light->dir[2] = gLightDefaultDirs[(s16)i * 3 + 2];
            light->ambient[0] = gLightDefaultAmbients[(s16)i * 4];
            light->ambient[1] = gLightDefaultAmbients[(s16)i * 4 + 1];
            light->ambient[2] = gLightDefaultAmbients[(s16)i * 4 + 2];
        }
        switch (mode) {
        case LIGHT_SET_ALL:
        case LIGHT_SET_COLOR:
            light->color[0] = gLightDefaultColors[(s16)i * 4];
            light->color[1] = gLightDefaultColors[(s16)i * 4 + 1];
            light->color[2] = gLightDefaultColors[(s16)i * 4 + 2];
        }
    }
}

extern LightSources gLightInitSources;
void *memset(void *dst, s32 c, u32 n);

/* The scene's area (gScene.area) under a name of its own: Light_InitAll's check needs it;
 * through gScene.area the load moves after the stores. */
extern u16 gSceneArea;

/* Clears the two light sets (gLightSets) and loads them: set 0 from gLightInitSources (the colours times
 * 10), set 1 from the scene's area (Light_SetDefault), at the origin, with unk14 1.0. The parameter is
 * the area, which the function reads from the scene instead. The two loops step a byte pointer one
 * Light at a time and find the light through a LightSet view of it, and set 1 is reached in two steps:
 * the compiler keeps these pointers in the registers the game's code uses. */
void Light_InitAll(s32 unused0)
{
    u8 *entry1;
    u8 *entry;
    LightSets *sets;
    f32 *src;
    s32 i;
    s32 k;
    Light *light;

    sets = &gLightSets;
    memset(sets, 0, sizeof(LightSets));
    for (i = 0, entry = (u8 *)&sets->sets[0]; i < LIGHT_SET_LIGHTS; i++, entry += sizeof(Light)) {
        light = ((LightSet *)entry)->lights;
        src = gLightInitSources.pos[i];
        for (k = 0; k < 3; k++) {
            light->pos[k] = *src++;
        }
        src = gLightInitSources.dir[i];
        for (k = 0; k < 3; k++) {
            light->dir[k] = *src++;
        }
        src = gLightInitSources.color[i];
        for (k = 0; k < 3; k++) {
            light->color[k] = 10.0f * *src++;
        }
        src = gLightInitSources.ambient[i];
        for (k = 0; k < 3; k++) {
            light->ambient[k] = *src++;
        }
        src = gLightInitSources.unk14[i];
        for (k = 0; k < 3; k++) {
            light->unk14[k] = *src++;
        }
        light->unk00 = 0;
    }
    entry = (u8 *)sets + sizeof(LightSet);
    for (i = 0, entry1 = (u8 *)((LightSets *)entry)->sets; i < LIGHT_SET_LIGHTS; i++, entry1 += sizeof(Light)) {
        light = ((LightSet *)entry1)->lights;
        /* An empty check of the area (Light_SetDefault's 0 and AREA_HOUSE): the compiler
         * removes it, but it still loads gSceneArea before the pos stores, as the game's code
         * does. A one-term test, a (void) read or a read into a local leave the load after
         * them. */
        if (gSceneArea != 0 && gSceneArea != AREA_HOUSE) { /* compiled-out check */
        }
        light->pos[0] = 0.0f;
        light->pos[1] = 0.0f;
        light->pos[2] = 0.0f;
        Light_SetDefault(light, gSceneArea, (s16)i, LIGHT_SET_ALL);
        light->unk14[0] = 1.0f;
        light->unk14[1] = 1.0f;
        light->unk14[2] = 1.0f;
        light->unk00 = 0;
    }
}

void LightSet_UpdateFade(LightSet *set);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
extern f32 gCamMatrix[];

/* Sets vec to (x, y, z). Kept as an inline function: through its pointer parameter the
 * compiler keeps &vec[1] and &vec[2] in saved registers across the light loop, as the
 * game's code does. */
static inline void Vec3_Set(f32 *vec, f32 x, f32 y, f32 z)
{
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
}

/* Steps the two light sets (gLightSets): a set in mode 1 fades (LightSet_UpdateFade). Then set 1's third
 * light points straight down, turned by the camera (gCamMatrix), except in area 0x11 (no map lists it).
 * The loop steps a byte pointer one LightSet at a time, as Light_InitAll's do. */
void LightSet_UpdateAll(void)
{
    u8 *entry;
    Light *light;
    f32 down[3];
    f32 rotated[3];
    LightSet *set;
    s32 i;

    for (i = 0, entry = (u8 *)&gLightSets; i < 2; i++, entry += sizeof(LightSet)) {
        set = ((LightSets *)entry)->sets;
        switch (set->mode) {
        case LIGHT_MODE_NONE:
            break;
        case LIGHT_MODE_FADE:
            LightSet_UpdateFade(set);
            break;
        }
        if (i == 1 && gScene.area != 0x11) { /* no AreaId names 0x11 */
            Vec3_Set(down, 0.0f, 0.0f, -1.0f);
            light = &set->lights[2];
            Vec_Rotate(rotated, down, gCamMatrix);
            light->dir[0] = rotated[0];
            light->dir[1] = rotated[1];
            light->dir[2] = rotated[2];
        }
    }
}

extern LightSources gLightFadeSources[2];

/* A light set's fade (mode 1, LightSet_UpdateAll): state 0 starts a fade of 30 frames; state 1 sets
 * light 0's colour and ambient part to gLightFadeSources' first light plus (frames left / 30), so that
 * they come down to it, and counts down; state 2 ends the mode. */
void LightSet_UpdateFade(LightSet *set)
{
    f32 *colorSrc = gLightFadeSources[0].color[0];
    f32 *ambientSrc = gLightFadeSources[0].ambient[0];
    Light *light = set->lights;
    s32 k;

    switch (set->state) {
    case LIGHT_FADE_START:
        set->state++;
        set->left = LIGHT_FADE_FRAMES;
        set->frames = LIGHT_FADE_FRAMES;
        break;
    case LIGHT_FADE_RUN:
        for (k = 0; k < 3; k++) {
            light->color[k] = *colorSrc++;
        }
        for (k = 0; k < 3; k++) {
            light->ambient[k] = *ambientSrc++;
        }
        if (--set->left != 0) {
            light->color[0] += (1.0f / set->frames) * set->left;
            light->color[1] += (1.0f / set->frames) * set->left;
            light->color[2] += (1.0f / set->frames) * set->left;
            light->ambient[0] += (1.0f / set->frames) * set->left;
            light->ambient[1] += (1.0f / set->frames) * set->left;
            light->ambient[2] += (1.0f / set->frames) * set->left;
        } else {
            set->state++;
        }
        break;
    case LIGHT_FADE_END:
        set->mode = LIGHT_MODE_NONE;
        break;
    }
}

extern f32 gLightFixedColors[3][4];

/* Sets the colours of light set n's three lights for the object about to be drawn (a hunter, a monster or
 * one of the lobby's people). In the fortress areas below (and area 0xD, which no map lists), while a
 * large monster is near the object, they are gLightFixedColors. Otherwise lights 0 and 1 take the
 * object's own colours (lightColors) when it has them, and the rest take the area's (Light_SetDefault). */
void LightSet_SetColors(void *obj, s32 n)
{
    ObjHead *head = obj;
    LightSet *set;
    s32 i;
    Light *light;
    f32 (*own)[4];

    set = &gLightSets.sets[n];
    for (i = 0; i < LIGHT_SET_LIGHTS; i++) {
        light = &set->lights[i];
        switch (gScene.area) {
        case AREA_FORTRESS_5:
        case 0xD: /* no AreaId names 0xD */
        case AREA_FORTRESS_1:
        case AREA_FORTRESS_3:
        case AREA_FORTRESS_2:
            if (head->nearLargeMonster != 0) {
                light->color[0] = gLightFixedColors[i][0];
                light->color[1] = gLightFixedColors[i][1];
                light->color[2] = gLightFixedColors[i][2];
                continue;
            }
        }
        own = head->lightColors;
        if (own == NULL || i == 2) {
            Light_SetDefault(light, gScene.area, (s16)i, LIGHT_SET_COLOR);
        } else {
            light->color[0] = own[i][0];
            light->color[1] = own[i][1];
            light->color[2] = own[i][2];
        }
    }
}

/* Puts back the colours of light set setIndex's three lights (Light_SetDefault). The set's number goes in
 * as the area too: set 0 takes the default colours, set 1 the area's own when it has them. */
void LightSet_ResetColors(s32 setIndex)
{
    u8 *entry;
    s32 j;

    entry = (u8 *)&gLightSets.sets[setIndex];
    for (j = 0; j < LIGHT_SET_LIGHTS; j++, entry += sizeof(Light)) {
        Light_SetDefault(((LightSet *)entry)->lights, setIndex, (s16)j, LIGHT_SET_COLOR);
    }
}

/* Hands light set setIndex's three lights to the renderer (RS_LIGHT) and turns lighting on (render
 * state 1). */
void LightSet_Apply(s32 setIndex)
{
    LightSet *set;

    set = &gLightSets.sets[setIndex];
    Gs_SetState(RS_LIGHT(0), (s32)&set->lights[0]);
    Gs_SetState(RS_LIGHT(1), (s32)&set->lights[1]);
    Gs_SetState(RS_LIGHT(2), (s32)&set->lights[2]);
    Gs_SetState(1, 1);
}

/* Clears the colours the hunter's lights fade from (lightFadeColors): with 0, LightSet_FadeColors
 * takes light set 1's colours at once. */
void Player_ClearLightColors(Player *pl)
{
    pl->lightFadeColors[0] = 0;
    pl->lightFadeColors[1] = 0;
    pl->lightFadeColors[2] = 0;
}

void *memcpy(void *dst, const void *src, u32 n);

/* Hands light set 1's three lights to the renderer with the object's faded colours: each light's colour
 * goes a fifth of the way from the object's lightFadeColors (0xFFRRGGBB; 0: none, the set's colour as it
 * is) to the set's, and the result goes back into lightFadeColors. Then lighting goes on (render state
 * 1). The loop steps a byte pointer one Light at a time, as Light_InitAll's do. */
void LightSet_FadeColors(void *obj)
{
    ObjHead *head = obj;
    f32 red;
    u8 *entry;
    f32 green;
    Light *src;
    s16 i;
    Light faded;
    u32 colorWord;
    f32 blue;

    entry = (u8 *)&gLightSets.sets[1];
    for (i = 0; i < LIGHT_SET_LIGHTS; entry += sizeof(Light), i++) {
        src = ((LightSet *)entry)->lights;
        memcpy(&faded, src, sizeof(Light));
        colorWord = head->lightFadeColors[i];
        if (colorWord == 0) {
            red = src->color[0];
            green = src->color[1];
            blue = src->color[2];
        } else {
            red = ((colorWord >> 16) & 0xFF) / 255.0f;
            green = ((colorWord >> 8) & 0xFF) / 255.0f;
            blue = (colorWord & 0xFF) / 255.0f;
            red += (src->color[0] - red) / 5.0f;
            green += (src->color[1] - green) / 5.0f;
            blue += (src->color[2] - blue) / 5.0f;
            faded.color[0] = red;
            faded.color[1] = green;
            faded.color[2] = blue;
        }
        head->lightFadeColors[i] = 0xFF000000 | ((u32)(red * 255.0f) & 0xFF) << 16 | ((u32)(green * 255.0f) & 0xFF) << 8 | ((u32)(blue * 255.0f) & 0xFF) | 0xFF000000;
        Gs_SetState((u8)RS_LIGHT(i), (s32)&faded);
    }
    Gs_SetState(1, 1);
}


/* The file's data: the tables the lights start from. Each LightSources points at five
 * tables of three lights each (a light's row of 3 or 4 floats; the last row is zero). */
extern f32 gLightFadePos[][3];
extern f32 gLightInitPos[][3];
f32 gLightFadeDirs[4][3] = {
#include "sub_main/0041EA30.inc"
};
f32 gLightFadeColors[3][4] = {
#include "sub_main/0041EA60.inc"
};
f32 gLightFadeAmbients[3][4] = {
#include "sub_main/0041EA90.inc"
};
f32 gLightFadeUnk14[3][4] = {
#include "sub_main/0041EAC0.inc"
};
LightSources gLightFadeSources[2] = {
    { gLightFadePos, gLightFadeDirs, gLightFadeColors, gLightFadeAmbients, gLightFadeUnk14 },
    { gLightFadePos, gLightFadeDirs, gLightFadeColors, gLightFadeAmbients, gLightFadeUnk14 },
};
f32 gLightInitDirs[4][3] = {
#include "sub_main/0041EB20.inc"
};
f32 gLightInitColors[3][4] = {
#include "sub_main/0041EB50.inc"
};
f32 gLightInitAmbients[3][4] = {
#include "sub_main/0041EB80.inc"
};
f32 gLightInitUnk14[3][4] = {
#include "sub_main/0041EBB0.inc"
};
LightSources gLightInitSources = {
    gLightInitPos, gLightInitDirs, gLightInitColors, gLightInitAmbients, gLightInitUnk14,
};
f32 gLightDefaultDirs[12] = {
#include "sub_main/0041EC00.inc"
};
f32 gLightHouseDirs[12] = {
#include "sub_main/0041EC30.inc"
};
f32 gLightDefaultColors[12] = {
#include "sub_main/0041EC60.inc"
};
f32 gLightFixedColors[3][4] = {
#include "sub_main/0041EC90.inc"
};
f32 gLightHouseColors[12] = {
#include "sub_main/0041ECC0.inc"
};
f32 gLightDefaultAmbients[12] = {
#include "sub_main/0041ECF0.inc"
};
f32 gLightHouseAmbients[12] = {
#include "sub_main/0041ED20.inc"
};
