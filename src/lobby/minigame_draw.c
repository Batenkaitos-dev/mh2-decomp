/*
 * Town pieces drawn and run around the lobby's minigames, in three groups:
 * - the elder dragon raid's town movies: LobbyTown_WantsMovie says whether
 *   one waits (LobbyState.movieState, set by the raid's world events),
 *   LobbyTown_StartMovie starts it and records the raid as seen in the save,
 *   LobbyTown_MovieDone waits for its end;
 * - the minigames' drawing: the arm-wrestling contest's time, power gauge and
 *   mashing buttons, and the result banner of it and of the drinking contest;
 * - the gallery's gun-aim debug view (three offsets moved with the second
 *   pad) and the model it draws, and the drinking contest's two helpers on
 *   the last persistent monster.
 * The data after the functions includes the gallery's walls and frames, which
 * the gallery's own files use.
 */
#include "common.h"
#include "window.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "scene.h"
#include "save.h"
#include "sprite.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "pad.h"
#include "area.h"

#pragma optimization_level 4
void Font_SetSpacing(u8 a, u8 b);

extern u8 gPersistentEmKinds[];
s32 LobbyMovie_Run(void);
s32 LobbyBgm_GetTrack(void);
extern s32 gTownMode;
void Hud_DrawButtonIcon(s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale);
extern s16 gDrinkHpStages[][3];
extern ElderRaidEvent gElderRaidEvent;
extern LobbyMenu *gLobbyMenu;
s32 TownPc_IsSeatedState(u8, u8);
void Actor_SetState(void *, s32, s32);
void Game_ResetSubsystems(void);
void LobbyMovie_Prepare(s32);
void LobbyBgm_SetUnk11F(s32);
void Gfx_SetTexFilter(s32 a);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 a);
struct TexRect;
void TexRect_Draw(struct TexRect *r, s8 mode);

/* Starts the raid's town movie that waits: stands the player up if seated,
 * resets the town's subsystems and hides the menu layers, and records in the
 * save that this raid's start (or end) movie has been played. In the hub area
 * 0x87 the town music is told to stop with it. */
void LobbyTown_StartMovie(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ElderRaidEvent *raid = &gElderRaidEvent;

    if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
        Actor_SetState(pl, 0, 0x15);
    }
    Game_ResetSubsystems();
    gLobbyMenu->hideLayers = 1;
    gLobby->seqStep = 0;
    if (gLobby->movieState == RAID_MOVIE_START) {
        LobbyMovie_Prepare(0);
        gSave.raidStartMovieId = raid->id;
    } else {
        LobbyMovie_Prepare(1);
        gSave.raidEndMovieId = raid->id;
    }
    if (gScene.area == AREA_TOWN_ARENA) {
        LobbyBgm_SetUnk11F(0);
    }
}

/* Runs the movie; 1 once it has finished (LobbyMovie_Run's last step, 3). */
s32 LobbyTown_MovieDone(void)
{
    switch (LobbyMovie_Run()) {
    case 3:
        return 1;
    }
    return 0;
}

/* 1 if the raid's town movie should play now: the raid has its movies, the
 * player is connected, in no town menu, with a movie waiting and no quest
 * taken, and not in hub area 0x87 while its town music plays. */
s32 LobbyTown_WantsMovie(void)
{
    if (gElderRaidEvent.noMovie == 1) {
        return 0;
    }
    if (gLobby->connected == 0) {
        return 0;
    }
    if (gTownMode != 0) {
        return 0;
    }
    if (gLobby->movieState == RAID_MOVIE_NONE) {
        return 0;
    }
    if (gLobby->questTaken != 0) {
        return 0;
    }
    if (0 < LobbyBgm_GetTrack() && gScene.area == AREA_TOWN_ARENA) {
        return 0;
    }
    switch (gScene.area) {
    case AREA_HUB_0x83:
    case AREA_HUB_0x84:
        return 1;
    }
    return 1;
}

extern char gTimeLeftFormat[];
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
/* Prints the time left, in seconds rounded up (frames / 30). */
void Lobby_DrawTimeLeft(s16 frames)
{
    if (frames >= 0) {
        Font_SetSpacing(0x17, 0x14);
        Text_PrintfAt(0xF0, 0x54, 5, gTimeLeftFormat, (frames - 1) / 30 + 1);
    }
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Contest_DrawBar(u32 color, f32 x, f32 y);
void Contest_DrawBarPiece(u32 color, f32 x, f32 y);
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
    Gfx_DrawLine(0xE8, 0x32, width, 0xA, 0xB23971FF);
    Gfx_DrawLine(33.0f + (200.0f + width), 0x32, 160.0f - width, 0xA, 0xB2FFE242);
    Contest_DrawBar(0xB2324435, 200.0f, 40.0f);
    Contest_DrawBarPiece(0xB2FF4545, 216.0f + 1.6f * (f32)split, 8.0f);
}

extern u8 gMinigameResultScaleKeys[];
extern u8 gMinigameResultPosKeys[];
extern char gDrinkWinText[];
extern char gContestWinText[];
extern char gMinigameLoseText[];
extern char gMinigameDrawText[];
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: as this file calls it, with 16-bit coordinates and a 32-bit kind; with the definition's types the call compiles differently */
void Gfx_SetBlend(s32 a, s32 b);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
s32 sprintf(char *buf, const char *fmt, ...);
void Font_SetColor(s32 color);
void Font_SetPos(s16, s16);
void Text_PrintfColored(const char *fmt, ...);
s32 Sprite_Draw(Sprite *s);
/* Draws a minigame's result banner (MinigameResult) while frames counts
 * down from 60: a window sized for the text, an icon that scales and moves
 * along its key frames, and the text. isContest picks the arm-wrestling
 * contest's win text over the drinking contest's. */
void Minigame_DrawResult(s8 result, s16 frames, s8 isContest)
{
    u8 *scaleKey = gMinigameResultScaleKeys;
    u8 *posKey = gMinigameResultPosKeys;
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
            width = 0xDC;
        } else {
            width = 0xC8;
        }
        break;
    case MINIGAME_LOSE:
        width = 0xB4;
        break;
    case MINIGAME_DRAW:
        width = 0x8C;
        break;
    }
    Win_Draw(0x140 - width / 2, 0xCC, width, 0x28, 0xBDB59F6C, 0xA0000000, 3);
    Gfx_SetTexFilter(1);
    Gfx_SetBlend(4, 1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
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
        icon.v0 = 0xE3;
        icon.u1 = 0x1F;
        icon.v1 = 0xFD;
        icon.color = 0xFFFFFF80;
        if (isContest == 0) {
            sprintf(buf, gDrinkWinText);
        } else {
            sprintf(buf, gContestWinText);
        }
        break;
    case MINIGAME_LOSE:
        icon.u0 = 0x41;
        icon.v0 = 0xE1;
        icon.u1 = 0x70;
        icon.v1 = 0xFF;
        icon.color = 0xFF884A84;
        sprintf(buf, gMinigameLoseText);
        break;
    case MINIGAME_DRAW:
        icon.u0 = 0x1C;
        icon.v0 = 0xE2;
        icon.u1 = 0x3D;
        icon.v1 = 0x101;
        icon.color = 0xFFFFFFFF;
        sprintf(buf, gMinigameDrawText);
        break;
    }
    Sprite_Draw(&icon);
    Font_SetSpacing(0x14, 0x14);
    Font_SetColor(0xFFFF80FF);
    Font_SetPos(Text_CenterX(buf, 0x140, 0x14), 0xD6);
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
    Hud_DrawButtonIcon(0, 0x14F, 320.0f - 24.0f * *circleScale, 0x155, -1, *circleScale);
    Hud_DrawButtonIcon(1, 0x131, 320.0f - 24.0f * *crossScale, 0x155, -1, *crossScale);
}

#include "prim.h"
extern f32 gCamMatrix[];
extern f32 gGunDebugForward;
extern f32 gGunDebugUp;
extern f32 gGunDebugRight;
extern char gGunDebugRightFormat[];
extern char gGunDebugUpFormat[];
extern char gGunDebugForwardFormat[];
void Vec_Normalize(f32 *v);
s32 Debug_IsOn(s32 a);
f64 fptodp(f32 x);
void Camera_ProjectPoint(f32 *out, f32 *pos);
s32 Prim_DrawLineF(PrimLineF *l);
void Contest_DrawModel(f32 *pos, f32 *scale, u32 color);
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
    if (Debug_IsOn(0xA0)) {
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
        Font_SetSpacing(0x10, 0x10);
        Text_PrintfAt(0x168, 0x11C, 7, gGunDebugRightFormat, fptodp(gGunDebugRight));
        Text_PrintfAt(0x168, 0x12C, 7, gGunDebugUpFormat, fptodp(gGunDebugUp));
        Text_PrintfAt(0x168, 0x13C, 7, gGunDebugForwardFormat, fptodp(gGunDebugForward));
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
    Contest_DrawModel(out, scale, -1);
}

s32 Model_Draw(u32 h, s32 flags);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Gs_SetState(u8, u32);
s32 Debug_IsOn(s32 a);
extern f32 gCamMatrix[];
/* Draws common model 0 at pos, turned half a turn, scaled and tinted with
 * color: material set 34, or set 2 (half transparent) with debug flag 0x32
 * on. */
void Contest_DrawModel(f32 *pos, f32 *scale, u32 color)
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
        Gs_SetState(0x1A, (s32)mtx);
        if (Debug_IsOn(0x32) != 0) {
            set = &mdl->matSets[2];
            color = (color & 0xFFFFFF) + 0x80000000;
        } else {
            set = &mdl->matSets[34];
        }
        tex = (s32) gCommonModels[0]->mats;
        Gs_SetState(0x67, color);
        if (set->id != -1) {
            for (i = 0; i < set->count; i++) {
                Gs_SetState((i + 0x3A) & 0xFF, tex + set->mat[i] * 0x4C);
            }
            Model_Draw(set->id, 0);
        }
        Gs_SetState(0x67, -1);
    }
}

#include "sprite.h"
s32 Sprite_Draw(Sprite *s);
/* Draws the gauge's frame at x, y in color: a left end, five middle pieces
 * and a right end, 32 pixels each. */
void Contest_DrawBar(u32 color, f32 x, f32 y)
{
    Sprite left;
    Sprite mid;
    Sprite right;
    u8 i;

    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x168);
    Tex_Bind(0x168);
    left.v0 = 0x80;
    left.u0 = 0x20;
    left.u1 = 0x10;
    left.w = 0x20;
    left.v1 = 0x70;
    left.h = 0x20;
    left.color = color;
    left.y = y;
    left.x = x;
    Sprite_Draw(&left);
    mid.u0 = 0;
    mid.color = color;
    x += 32.0f;
    i = 0;
    mid.u1 = 0xF;
    mid.v1 = 0x80;
    mid.v0 = 0x70;
    mid.w = 0x20;
    mid.h = 0x20;
    mid.y = y;
    do {
        mid.x = x;
        Sprite_Draw(&mid);
        x += 32.0f;
        i++;
    } while (i < 5);
    right.v0 = 0x70;
    right.u0 = 0x11;
    right.v1 = 0x80;
    right.u1 = 0x20;
    right.w = 0x20;
    right.h = 0x20;
    right.color = color;
    right.y = y;
    right.x = x;
    Sprite_Draw(&right);
}

/* Draws the gauge's marker at x, y in color. */
void Contest_DrawBarPiece(u32 color, f32 x, f32 y)
{
    Sprite marker;

    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x168);
    Tex_Bind(0x168);
    marker.v0 = 0x60;
    marker.u0 = 0x20;
    marker.v1 = 0x6F;
    marker.w = 0x20;
    marker.h = 0x40;
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
u8 gMinigameResultScaleKeys[48] = {
#include "lobby/007EC1E0.inc"
};
u8 gMinigameResultPosKeys[48] = {
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
u32 gGalleryWalls[12] = {
#include "lobby/007EC270.inc"
};
u32 gGalleryWallYRange[4] = {
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
char gDrinkWinText[0x20] = {
#include "lobby/00838490.inc"
};
char gContestWinText[0x20] = {
#include "lobby/008384B0.inc"
};
char gMinigameLoseText[0x18] = {
#include "lobby/008384D0.inc"
};
char gMinigameDrawText[13] = {
#include "lobby/008384E8.inc"
};
char gGunDebugRightFormat[0x20] = {
#include "lobby/00838500.inc"
};
char gGunDebugUpFormat[0x20] = {
#include "lobby/00838520.inc"
};
char gGunDebugForwardFormat[0x20] = {
#include "lobby/00838540.inc"
};
#pragma rodata off
