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

extern M2C_UNK gPersistentEmKinds;
s32 LobbyMovie_Run(void);
s32 LobbyBgm_GetTrack(void);
extern s32 gTownMode;
extern s8 D_008BEFC8;
void Hud_DrawButtonIcon(s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale);
extern s16 gDrinkHpStages[][3];
extern u8 gElderRaidEvent;
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

void LobbyTown_StartMovie(void)
{
    Player *p = &gPlayers[gScene.localPlayer];
    u8 *e = (u8 *)&gElderRaidEvent;

    if (TownPc_IsSeatedState(p->stateGroup, p->state) == 1) {
        Actor_SetState(p, 0, 0x15);
    }
    Game_ResetSubsystems();
    gLobbyMenu->hideLayers = 1;
    gLobby->seqStep = 0;
    if (gLobby->movieState == 1) {
        LobbyMovie_Prepare(0);
        gSave.unk3696 = *(s16 *)(e + 0x20);
    } else {
        LobbyMovie_Prepare(1);
        gSave.unk3698 = *(s16 *)(e + 0x20);
    }
    if (gScene.area == AREA_HUB_0x87) {
        LobbyBgm_SetUnk11F(0);
    }
}

s32 LobbyTown_MovieDone(void)
{
    switch (LobbyMovie_Run()) {
    case 3:
        return 1;
    }
    return 0;
}

s32 LobbyTown_WantsMovie(void)
{
    if (D_008BEFC8 == 1) {
        return 0;
    }
    if (gLobby->connected == 0) {
        return 0;
    }
    if (gTownMode != 0) {
        return 0;
    }
    if (gLobby->movieState == 0) {
        return 0;
    }
    if (gLobby->questTaken != 0) {
        return 0;
    }
    if (0 < LobbyBgm_GetTrack() && gScene.area == AREA_HUB_0x87) {
        return 0;
    }
    switch (gScene.area) {
    case AREA_HUB_0x83:
    case AREA_HUB_0x84:
        return 1;
    }
    return 1;
}

extern char D_00838478[];
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
void Lobby_DrawTimeLeft(s16 t)
{
    if (t >= 0) {
        Font_SetSpacing(0x17, 0x14);
        Text_PrintfAt(0xF0, 0x54, 5, D_00838478, (t - 1) / 30 + 1);
    }
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Contest_DrawBar(u32 color, f32 x, f32 y);
void Contest_DrawBarPiece(u32 color, f32 x, f32 y);
void Contest_DrawGauge(s32 mine, s32 theirs)
{
    f32 d = theirs - mine;
    u32 v;
    f32 w;

    if (!(d <= 50.0f)) {
        v = 100;
    } else if (d < -50.0f) {
        v = 0;
    } else {
        v = 50.0f + d;
    }
    w = 0.8f * (f32)(v * 2);
    Gfx_DrawLine(0xE8, 0x32, w, 0xA, 0xB23971FF);
    Gfx_DrawLine(33.0f + (200.0f + w), 0x32, 160.0f - w, 0xA, 0xB2FFE242);
    Contest_DrawBar(0xB2324435, 200.0f, 40.0f);
    Contest_DrawBarPiece(0xB2FF4545, 216.0f + 1.6f * (f32)v, 8.0f);
}

extern u8 gMinigameResultScaleKeys[];
extern u8 gMinigameResultPosKeys[];
extern char D_00838490[];
extern char D_008384B0[];
extern char D_008384D0[];
extern char D_008384E8[];
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), Minigame_DrawResult 14 of 189 words differ */
void Gfx_SetBlend(s32 a, s32 b);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
s32 sprintf(char *buf, const char *fmt, ...);
void Font_SetColor(s32 color);
void Font_SetPos(s16, s16);
void Text_PrintfColored(const char *fmt, ...);
s32 Sprite_Draw(Sprite *s);
void Minigame_DrawResult(s8 kind, s16 t, s8 flag)
{
    u8 *scaleKey = gMinigameResultScaleKeys;
    u8 *posKey = gMinigameResultPosKeys;
    f32 pos[3];
    f32 scale;
    Sprite s;
    char buf[32];
    s16 w;
    f32 ft;

    if (t <= 0) {
        return;
    }
    switch (kind) {
    default:
        if (flag == 0) {
            w = 0xDC;
        } else {
            w = 0xC8;
        }
        break;
    case 1:
        w = 0xB4;
        break;
    case 2:
        w = 0x8C;
        break;
    }
    Win_Draw(0x140 - w / 2, 0xCC, w, 0x28, 0xBDB59F6C, 0xA0000000, 3);
    Gfx_SetTexFilter(1);
    Gfx_SetBlend(4, 1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    ft = 60.0f - t;
    Key_LerpFloat((f32 *) scaleKey, (f32 *) &scale, ft);
    Key_LerpVec3((f32 *) posKey, (f32 *) pos, ft);
    s.x = 320.0f + pos[0];
    s.y = 224.0f + pos[1];
    s.w = scale;
    s.h = scale;
    switch ((u8)kind) {
    case 0:
        s.u0 = 2;
        s.v0 = 0xE3;
        s.u1 = 0x1F;
        s.v1 = 0xFD;
        s.color = 0xFFFFFF80;
        if (flag == 0) {
            sprintf(buf, D_00838490);
        } else {
            sprintf(buf, D_008384B0);
        }
        break;
    case 1:
        s.u0 = 0x41;
        s.v0 = 0xE1;
        s.u1 = 0x70;
        s.v1 = 0xFF;
        s.color = 0xFF884A84;
        sprintf(buf, D_008384D0);
        break;
    case 2:
        s.u0 = 0x1C;
        s.v0 = 0xE2;
        s.u1 = 0x3D;
        s.v1 = 0x101;
        s.color = 0xFFFFFFFF;
        sprintf(buf, D_008384E8);
        break;
    }
    Sprite_Draw(&s);
    Font_SetSpacing(0x14, 0x14);
    Font_SetColor(0xFFFF80FF);
    Font_SetPos(Text_CenterX(buf, 0x140, 0x14), 0xD6);
    Text_PrintfColored(buf);
}

void Contest_DrawButtons(f32 *a, f32 *b, s16 *t)
{
    u16 pad = gPadState[0].pressed;

    if (pad & 0x20) {
        *a = 1.5f;
        *b = 1.0f;
        *t = 5;
    } else if (pad & 0x40) {
        *a = 1.0f;
        *b = 1.5f;
        *t = 5;
    }
    if ((*t)-- <= 0) {
        *a = 1.0f;
        *b = 1.0f;
    } else {
        if (!(*a <= 1.0f)) {
            *a -= 0.1f;
        }
        if (!(*b <= 1.0f)) {
            *b -= 0.1f;
        }
    }
    Hud_DrawButtonIcon(0, 0x14F, 320.0f - 24.0f * *a, 0x155, -1, *a);
    Hud_DrawButtonIcon(1, 0x131, 320.0f - 24.0f * *b, 0x155, -1, *b);
}

#include "prim.h"
extern f32 gCamMatrix[];
extern f32 gGunDebugOffX;
extern f32 gGunDebugOffY;
extern f32 D_0084A350;
extern char D_00838500[];
extern char D_00838520[];
extern char D_00838540[];
void Vec_Normalize(f32 *v);
s32 Debug_IsOn(s32 a);
f64 fptodp(f32 x);
void Camera_ProjectPoint(f32 *out, f32 *pos);
s32 Prim_DrawLineF(PrimLineF *l);
void Contest_DrawModel(f32 *pos, f32 *scale, u32 color);
static inline void Vec_AddScaled(f32 *d, f32 *a, f32 *v, f32 s)
{
    d[0] = a[0] + v[0] * s;
    d[1] = a[1] + v[1] * s;
    d[2] = a[2] + v[2] * s;
}
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
        if (pad & 0x2000) {
            gGunDebugOffX += 2.0f;
        }
        if (pad & 0x1000) {
            gGunDebugOffX -= 2.0f;
        }
        if (pad & 0x800) {
            D_0084A350 -= 2.0f;
        }
        if (pad & 0x400) {
            D_0084A350 += 2.0f;
        }
        if (pad & 0x100) {
            gGunDebugOffY += 2.0f;
        }
        if (pad & 0x40) {
            gGunDebugOffY -= 2.0f;
        }
        if (gPadState[1].pressed & 0x200) {
            gGunDebugOffX = 0.0f;
            gGunDebugOffY = 0.0f;
            D_0084A350 = 0.0f;
        }
        Font_SetSpacing(0x10, 0x10);
        Text_PrintfAt(0x168, 0x11C, 7, D_00838500, fptodp(D_0084A350));
        Text_PrintfAt(0x168, 0x12C, 7, D_00838520, fptodp(gGunDebugOffY));
        Text_PrintfAt(0x168, 0x13C, 7, D_00838540, fptodp(gGunDebugOffX));
    }
    out[0] = base[0] + right[0] * D_0084A350 + up[0] * gGunDebugOffY + fwd[0] * gGunDebugOffX;
    out[1] = base[1] + right[1] * D_0084A350 + up[1] * gGunDebugOffY + fwd[1] * gGunDebugOffX;
    out[2] = base[2] + right[2] * D_0084A350 + up[2] * gGunDebugOffY + fwd[2] * gGunDebugOffX;
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
void Contest_DrawModel(f32 *pos, f32 *scale, u32 color)
{
    Model *mdl = gCommonModels[0];
    f32 rot[3];
    f32 m[16];
    s32 tex;
    ModelMatSet *set;
    s32 i;

    if (mdl != NULL && mdl->active != 0) {
        rot[1] = 3.1415927f;
        rot[0] = 0.0f;
        rot[2] = 0.0f;
        Mtx_FromScaleRotTrans(scale, rot, pos, 5, m);
        Mtx_Concat3x3(m, gCamMatrix);
        Gs_SetState(0x1A, (s32)m);
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
void Contest_DrawBar(u32 color, f32 x, f32 y)
{
    Sprite l;
    Sprite c;
    Sprite r;
    u8 i;

    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x168);
    Tex_Bind(0x168);
    l.v0 = 0x80;
    l.u0 = 0x20;
    l.u1 = 0x10;
    l.w = 0x20;
    l.v1 = 0x70;
    l.h = 0x20;
    l.color = color;
    l.y = y;
    l.x = x;
    Sprite_Draw(&l);
    c.u0 = 0;
    c.color = color;
    x += 32.0f;
    i = 0;
    c.u1 = 0xF;
    c.v1 = 0x80;
    c.v0 = 0x70;
    c.w = 0x20;
    c.h = 0x20;
    c.y = y;
    do {
        c.x = x;
        Sprite_Draw(&c);
        x += 32.0f;
        i++;
    } while (i < 5);
    r.v0 = 0x70;
    r.u0 = 0x11;
    r.v1 = 0x80;
    r.u1 = 0x20;
    r.w = 0x20;
    r.h = 0x20;
    r.color = color;
    r.y = y;
    r.x = x;
    Sprite_Draw(&r);
}

void Contest_DrawBarPiece(u32 color, f32 x, f32 y)
{
    Sprite s;

    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x168);
    Tex_Bind(0x168);
    s.v0 = 0x60;
    s.u0 = 0x20;
    s.v1 = 0x6F;
    s.w = 0x20;
    s.h = 0x40;
    s.color = color;
    s.u1 = 0;
    s.y = y;
    s.x = x;
    TexRect_Draw((struct TexRect *)&s, 3);
}

u8 func_00700A50(void)
{
    return *(M2C_UNK *)((u8 *)&gPersistentEmKinds + gSave.lastPersistentEm);
}

s32 DrinkContest_HpStage(void)
{
    u8 k = gSave.lastPersistentEm;
    s16 *t = gDrinkHpStages[k];
    s16 hp = gSave.persistentEms[k].hp;
    s32 i;

    if (hp == 0) {
        return 0;
    }
    for (i = 1; i < 4; i++, t++) {
        if (hp <= *t) {
            return i;
        }
    }
    return i;
}

/* unk_6FF940's .data (0x7EC1E0-0x7EC2F0) */
u8 gMinigameResultScaleKeys[48] = {
#include "lobby/007EC1E0.inc"
};
u8 gMinigameResultPosKeys[48] = {
#include "lobby/007EC210.inc"
};
s16 gDrinkHpStages[4][3] = {
#include "lobby/007EC240.inc"
};
f32 gGunDebugOffX = {
#include "lobby/007EC258.inc"
};
f32 gGunDebugOffY = {
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

/* unk_6FF940's .rodata (0x838490-0x838560) */
#pragma rodata on
char D_00838490[0x20] = {
#include "lobby/00838490.inc"
};
char D_008384B0[0x20] = {
#include "lobby/008384B0.inc"
};
char D_008384D0[0x18] = {
#include "lobby/008384D0.inc"
};
char D_008384E8[13] = {
#include "lobby/008384E8.inc"
};
char D_00838500[0x20] = {
#include "lobby/00838500.inc"
};
char D_00838520[0x20] = {
#include "lobby/00838520.inc"
};
char D_00838540[0x20] = {
#include "lobby/00838540.inc"
};
#pragma rodata off
