/*
 * The minigames' drawing, the gallery's gun-aim debug view and the
 * drinking contest's questions about the persistent monster:
 * - the arm-wrestling contest's screen: the time left (Lobby_DrawTimeLeft),
 *   the power gauge (Contest_DrawGauge, its frame and marker drawn from the
 *   minigame texture, TEX_SLOT_MINIGAME) and the buttons to mash
 *   (Contest_DrawButtons);
 * - the result banner of the arm-wrestling and the drinking contests
 *   (Minigame_DrawResult): a win, a loss or a draw, scaled and moved in by
 *   key frames;
 * - the gallery's gun-aim debug view (Gallery_DrawGunDebug: three offsets
 *   moved with the second pad), with its aim line and marker model;
 * - the drinking contest's two questions about the persistent monster saved
 *   last: its kind and how worn it is.
 * The data after the functions also holds the gallery's walls and frames,
 * which the gallery's own files use.
 */
#include "common.h"
#include "window.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "scene.h"
#include "save.h"
#include "sprite.h"
#include "player.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "pad.h"
#include "area.h"
#include "hud.h"
#include "texture.h"
#include "render_state.h"
#include "bgm.h"
#include "place_rules.h"
#include "sprite_queue.h"
#include "text.h"
#include "stageobj.h"
#include "minigame_draw.h"
#include "draw_helpers.h"
#include "elder_forecast.h"
#include "engine_math.h"
#include "font.h"
#include "town_players.h"
#include "libc.h"
#include "libgcc.h"

/* The game built this file at optimization level 4: the pragma gives its code. */
#pragma optimization_level 4

extern u8 gPersistentEmKinds[];
extern s32 gTownMode;
extern s16 gDrinkHpStages[][3];
extern LobbyMenu *gLobbyMenu;
struct TexRect;

/* Prints the time left, in seconds rounded up (frames / 30). */
void Lobby_DrawTimeLeft(s16 frames)
{
    if (frames >= 0) {
        Font_SetSpacing(23, 20);
        Text_PrintfAt(240, 84, 5, gTimeLeftFormat, (frames - 1) / 30 + 1);
    }
}

/* Draws the arm-wrestling contest's power gauge: the difference of the two
 * powers, clamped to +-50, splits a 160-pixel bar into the two colours, with
 * the frame and the marker at the split. */
void Contest_DrawGauge(s32 mine, s32 theirs)
{
    f32 diff = theirs - mine;
    u32 split;
    f32 width;

    if (!(diff <= 50.0f)) {
        split = 100;
    } else if (diff < -50.0f) {
        split = 0;
    } else {
        split = 50.0f + diff;
    }
    width = 0.8f * (f32)(split * 2);
    Gfx_DrawLine(232, 50, width, 10, 0xB23971FF);
    Gfx_DrawLine(33.0f + (200.0f + width), 50, 160.0f - width, 10, 0xB2FFE242);
    Contest_DrawGaugeFrame(0xB2324435, 200.0f, 40.0f);
    Contest_DrawGaugeMarker(0xB2FF4545, 216.0f + 1.6f * (f32)split, 8.0f);
}

extern FloatKey gMinigameResultScaleKeys[];
extern Vec3Key gMinigameResultPosKeys[];
extern char gDrinkWinText[];
extern char gContestWinText[];
extern char gMinigameLoseText[];
extern char gMinigameDrawText[];
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: as this file calls it, with 16-bit coordinates and a 32-bit kind; with the definition's types the call compiles differently */
void Gfx_SetBlend(s32 a, s32 b);
/* Draws a minigame's result banner (MinigameResult) while frames counts
 * down from 60: a window sized for the text, an icon that scales and moves
 * along its key frames, and the text. isContest picks the arm-wrestling
 * contest's win text over the drinking contest's. */
void Minigame_DrawResult(s8 result, s16 frames, s8 isContest)
{
    FloatKey *scaleKey = gMinigameResultScaleKeys;
    Vec3Key *posKey = gMinigameResultPosKeys;
    f32 pos[3];
    f32 scale;
    Sprite icon;
    char buf[32];
    s16 width;
    f32 elapsed;

    if (frames <= 0) {
        return;
    }
    switch (result) {
    default:
        if (isContest == 0) {
            width = 220;
        } else {
            width = 200;
        }
        break;
    case MINIGAME_LOSE:
        width = 180;
        break;
    case MINIGAME_DRAW:
        width = 140;
        break;
    }
    Win_Draw(320 - width / 2, 204, width, 40, 0xBDB59F6C, 0xA0000000, 3);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    elapsed = 60.0f - frames;
    Key_LerpFloat((f32 *) scaleKey, (f32 *) &scale, elapsed);
    Key_LerpVec3((f32 *) posKey, (f32 *) pos, elapsed);
    icon.x = 320.0f + pos[0];
    icon.y = 224.0f + pos[1];
    icon.w = scale;
    icon.h = scale;
    switch ((u8)result) {
    case MINIGAME_WIN:
        icon.u0 = 2;
        icon.v0 = 227;
        icon.u1 = 31;
        icon.v1 = 253;
        icon.color = 0xFFFFFF80;
        if (isContest == 0) {
            sprintf(buf, gDrinkWinText);
        } else {
            sprintf(buf, gContestWinText);
        }
        break;
    case MINIGAME_LOSE:
        icon.u0 = 65;
        icon.v0 = 225;
        icon.u1 = 112;
        icon.v1 = 255;
        icon.color = 0xFF884A84;
        sprintf(buf, gMinigameLoseText);
        break;
    case MINIGAME_DRAW:
        icon.u0 = 28;
        icon.v0 = 226;
        icon.u1 = 61;
        icon.v1 = 257;
        icon.color = 0xFFFFFFFF;
        sprintf(buf, gMinigameDrawText);
        break;
    }
    Sprite_Draw(&icon);
    Font_SetSpacing(20, 20);
    Font_SetColor(0xFFFF80FF);
    Font_SetPos(Text_CenterX(buf, 320, 20), 214);
    Text_PrintfColored(buf);
}

/* Draws the contest's two buttons to mash: the one pressed (circle or cross)
 * swells to 1.5 for 5 frames and shrinks back by 0.1 a frame. */
void Contest_DrawButtons(f32 *circleScale, f32 *crossScale, s16 *bumpFrames)
{
    u16 pad = gPadState[0].pressed;

    if (pad & PAD_CIRCLE) {
        *circleScale = 1.5f;
        *crossScale = 1.0f;
        *bumpFrames = 5;
    } else if (pad & PAD_CROSS) {
        *circleScale = 1.0f;
        *crossScale = 1.5f;
        *bumpFrames = 5;
    }
    if ((*bumpFrames)-- <= 0) {
        *circleScale = 1.0f;
        *crossScale = 1.0f;
    } else {
        if (!(*circleScale <= 1.0f)) {
            *circleScale -= 0.1f;
        }
        if (!(*crossScale <= 1.0f)) {
            *crossScale -= 0.1f;
        }
    }
    Hud_DrawButtonIcon(BUTTON_ICON_CIRCLE, 335, 320.0f - 24.0f * *circleScale, 341, -1, *circleScale);
    Hud_DrawButtonIcon(BUTTON_ICON_CROSS, 305, 320.0f - 24.0f * *crossScale, 341, -1, *crossScale);
}

#include "prim.h"
extern f32 gCamMatrix[];
extern f32 gGunDebugForward;
extern f32 gGunDebugUp;
extern f32 gGunDebugRight;
extern char gGunDebugRightFormat[];
extern char gGunDebugUpFormat[];
extern char gGunDebugForwardFormat[];
/* dst = from + dir * dist */
static inline void Vec_AddScaled(f32 *dst, f32 *from, f32 *dir, f32 dist)
{
    dst[0] = from[0] + dir[0] * dist;
    dst[1] = from[1] + dir[1] * dist;
    dst[2] = from[2] + dir[2] * dist;
}
/* The gallery gun's aim: out is base moved by the three debug offsets along
 * the camera's right, up and forward axes. With debug flag 0xA0 on, the
 * second pad's d-pad, triangle and cross move the offsets by 2 (square
 * resets them) and their values are printed. Then draws the aim line, 400
 * units forward from out, and the marker model at out. */
void Gallery_DrawGunDebug(f32 *base, f32 *out)
{
    f32 fwd[3];
    f32 up[3];
    f32 right[3];
    f32 from[3];
    f32 to[3];
    f32 scale[3];
    PrimLineF line;
    u16 pad;

    fwd[0] = -gCamMatrix[8];
    fwd[1] = -gCamMatrix[9];
    fwd[2] = -gCamMatrix[10];
    Vec_Normalize(fwd);
    up[0] = gCamMatrix[4];
    up[1] = gCamMatrix[5];
    up[2] = gCamMatrix[6];
    Vec_Normalize(up);
    right[0] = gCamMatrix[0];
    right[1] = gCamMatrix[1];
    right[2] = gCamMatrix[2];
    Vec_Normalize(right);
    if (Debug_IsOn(160)) {
        pad = gPadState[1].held;
        if (pad & PAD_UP) {
            gGunDebugForward += 2.0f;
        }
        if (pad & PAD_DOWN) {
            gGunDebugForward -= 2.0f;
        }
        if (pad & PAD_LEFT) {
            gGunDebugRight -= 2.0f;
        }
        if (pad & PAD_RIGHT) {
            gGunDebugRight += 2.0f;
        }
        if (pad & PAD_TRIANGLE) {
            gGunDebugUp += 2.0f;
        }
        if (pad & PAD_CROSS) {
            gGunDebugUp -= 2.0f;
        }
        if (gPadState[1].pressed & PAD_SQUARE) {
            gGunDebugForward = 0.0f;
            gGunDebugUp = 0.0f;
            gGunDebugRight = 0.0f;
        }
        Font_SetSpacing(16, 16);
        Text_PrintfAt(360, 284, 7, gGunDebugRightFormat, fptodp(gGunDebugRight));
        Text_PrintfAt(360, 300, 7, gGunDebugUpFormat, fptodp(gGunDebugUp));
        Text_PrintfAt(360, 316, 7, gGunDebugForwardFormat, fptodp(gGunDebugForward));
    }
    out[0] = base[0] + right[0] * gGunDebugRight + up[0] * gGunDebugUp + fwd[0] * gGunDebugForward;
    out[1] = base[1] + right[1] * gGunDebugRight + up[1] * gGunDebugUp + fwd[1] * gGunDebugForward;
    out[2] = base[2] + right[2] * gGunDebugRight + up[2] * gGunDebugUp + fwd[2] * gGunDebugForward;
    Vec_AddScaled(from, out, fwd, 0.0f);
    Vec_AddScaled(to, from, fwd, 400.0f);
    Camera_ProjectPoint(&line.v[0].x, from);
    Camera_ProjectPoint(&line.v[1].x, to);
    line.color = 0xFF0000FF;
    Prim_DrawLineF(&line);
    scale[0] = 1.0f;
    scale[1] = 1.0f;
    scale[2] = 1.0f;
    GunDebug_DrawMarker(out, scale, -1);
}

/* Draws common model 0 at pos, turned half a turn, scaled and tinted with
 * color: material set 34, or set 2 (half transparent) with debug flag 0x32
 * on. */
void GunDebug_DrawMarker(f32 *pos, f32 *scale, u32 color)
{
    Model *mdl = gCommonModels[0];
    f32 rot[3];
    f32 mtx[16];
    s32 tex;
    ModelMatSet *set;
    s32 i;

    if (mdl != NULL && mdl->active != 0) {
        rot[1] = 3.1415927f;
        rot[0] = 0.0f;
        rot[2] = 0.0f;
        Mtx_FromScaleRotTrans(scale, rot, pos, 5, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        if (Debug_IsOn(50) != 0) {
            set = &mdl->matSets[2];
            color = (color & 0xFFFFFF) + 0x80000000;
        } else {
            set = &mdl->matSets[34];
        }
        tex = (s32) gCommonModels[0]->mats;
        Gs_SetState(RS_COLOR2, color);
        if (set->id != -1) {
            for (i = 0; i < set->count; i++) {
                Gs_SetState((i + 58) & 0xFF, tex + set->mat[i] * 76);
            }
            Model_Draw(set->id, 0);
        }
        Gs_SetState(RS_COLOR2, -1);
    }
}

/* Draws the gauge's frame at x, y in color: a left end, five middle pieces
 * and a right end, 32 pixels each. */
void Contest_DrawGaugeFrame(u32 color, f32 x, f32 y)
{
    Sprite left;
    Sprite mid;
    Sprite right;
    u8 i;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_MINIGAME);
    Tex_Bind(TEX_SLOT_MINIGAME);
    left.v0 = 128;
    left.u0 = 32;
    left.u1 = 16;
    left.w = 32;
    left.v1 = 112;
    left.h = 32;
    left.color = color;
    left.y = y;
    left.x = x;
    Sprite_Draw(&left);
    mid.u0 = 0;
    mid.color = color;
    x += 32.0f;
    i = 0;
    mid.u1 = 15;
    mid.v1 = 128;
    mid.v0 = 112;
    mid.w = 32;
    mid.h = 32;
    mid.y = y;
    do {
        mid.x = x;
        Sprite_Draw(&mid);
        x += 32.0f;
        i++;
    } while (i < 5);
    right.v0 = 112;
    right.u0 = 17;
    right.v1 = 128;
    right.u1 = 32;
    right.w = 32;
    right.h = 32;
    right.color = color;
    right.y = y;
    right.x = x;
    Sprite_Draw(&right);
}

/* Draws the gauge's marker at x, y in color. */
void Contest_DrawGaugeMarker(u32 color, f32 x, f32 y)
{
    Sprite marker;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_MINIGAME);
    Tex_Bind(TEX_SLOT_MINIGAME);
    marker.v0 = 96;
    marker.u0 = 32;
    marker.v1 = 111;
    marker.w = 32;
    marker.h = 64;
    marker.color = color;
    marker.u1 = 0;
    marker.y = y;
    marker.x = x;
    TexRect_Draw((struct TexRect *)&marker, 3);
}

/* The kind of the persistent monster saved last. */
u8 PersistentEm_LastKind(void)
{
    return gPersistentEmKinds[gSave.lastPersistentEm];
}

/* How worn the persistent monster saved last is: 0 if its health is 0, else
 * 1-3 for the first of its three thresholds (gDrinkHpStages) the health is at
 * or under, 4 above them all. The drinking contest's host picks a line by it. */
s32 DrinkContest_HpStage(void)
{
    u8 slot = gSave.lastPersistentEm;
    s16 *limit = gDrinkHpStages[slot];
    s16 hp = gSave.persistentEms[slot].hp;
    s32 i;

    if (hp == 0) {
        return 0;
    }
    for (i = 1; i < 4; i++, limit++) {
        if (hp <= *limit) {
            return i;
        }
    }
    return i;
}

/* The result banner's key frames, the drinking contest's thresholds, the gun
 * debug offsets, and the gallery's walls and frames. */
FloatKey gMinigameResultScaleKeys[6] = {
#include "lobby/007EC1E0.inc"
};
Vec3Key gMinigameResultPosKeys[3] = {
#include "lobby/007EC210.inc"
};
s16 gDrinkHpStages[4][3] = {
#include "lobby/007EC240.inc"
};
f32 gGunDebugForward = {
#include "lobby/007EC258.inc"
};
f32 gGunDebugUp = {
#include "lobby/007EC260.inc"
};
Vec3f gGalleryWalls[4] = {
#include "lobby/007EC270.inc"
};
f32 gGalleryWallYRange[4] = {
#include "lobby/007EC2A0.inc"
};
Frame gGalleryPrizeFrame = {
#include "lobby/007EC2B0.inc"
};
Frame gGalleryFrame = {
#include "lobby/007EC2D0.inc"
};

/* The result banner's texts and the gun debug lines' formats. */
#pragma rodata on
char gDrinkWinText[32] = {
#include "lobby/00838490.inc"
};
char gContestWinText[32] = {
#include "lobby/008384B0.inc"
};
char gMinigameLoseText[24] = {
#include "lobby/008384D0.inc"
};
char gMinigameDrawText[13] = {
#include "lobby/008384E8.inc"
};
char gGunDebugRightFormat[32] = {
#include "lobby/00838500.inc"
};
char gGunDebugUpFormat[32] = {
#include "lobby/00838520.inc"
};
char gGunDebugForwardFormat[32] = {
#include "lobby/00838540.inc"
};
#pragma rodata off
