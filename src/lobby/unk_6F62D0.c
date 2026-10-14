#include "common.h"
#include "sprite.h"
#pragma optimization_level 4
int abs(int);



typedef struct TexSrc {
    s32 tex;
    UV uv0, uv1;
} TexSrc;

extern s16 gClockSpinAddDays;
extern s16 gClockSpinNextDay;
extern TexSrc D_007FEE3C[];
s32 LobbyTime_IsPm(void);
s32 abs(s32 x);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 tex);
void TexRect_DrawRotated2(TexRect *r, s16 cx, s16 cy, s16 cx2, s16 cy2, u32 angle1, u32 angle2);


void VillageClock_DrawRing(u32 color)
{
    TexRect r;
    f32 a;
    f32 b;
    TexSrc *t;
    u32 ang;
    s16 i;
    s16 cx;
    s16 cy;
    s32 k;

    if (LobbyTime_IsPm() == 0) {
        a = 0.0f;
    } else {
        a = 180.0f;
    }
    a += 12.0f * gClockSpinAddDays;
    b = 12.0f * gClockSpinNextDay;
    t = (TexSrc *)D_007FEE3C;
    r.x = 0x40;
    r.y = 6;
    r.w = abs(t->uv1.u - t->uv0.u);
    r.h = abs(t->uv1.v - t->uv0.v);
    r.color = color;
    r.uv0 = t->uv0;
    r.uv1 = t->uv1;
    Tex_Prepare(1, t->tex);
    Tex_Bind(t->tex);
    cx = r.x + r.w / 2;
    cy = r.y + 0x13;
    ang = (u16)(s32)(0.5f + 65536.0f * a / 360.0f);
    for (i = 0, k = 0; i < 8; i++, k += 0x2D) {
        TexRect_DrawRotated2(&r, cx, cy, 0x48, 0x39, ang, (u16)(s32)(0.5f + 65536.0f * (b + k) / 360.0f));
    }
}

void Icon_DrawRotatedAt(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s32 i, s16 cx, s16 cy);
extern u32 gLobbyTime;
extern TexSrc D_007FEDDC[];
void Icon_DrawRotated(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s32 i); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 color, s32 angle, s32 i) (lobby/unk_790EC0.c), VillageClock_DrawHours 61 of 230 words differ */

void VillageClock_DrawHours(u32 color)
{
    u32 t;
    s16 y;
    s16 w;
    s16 h;
    s32 dh;
    TexSrc *src = (TexSrc *)D_007FEDDC;

    dh = src->uv1.v - src->uv0.v;
    y = 0x39 - dh;
    h = dh;
    w = src->uv1.u - src->uv0.u;
    t = gLobbyTime % 43200;
    if (t < 36000) {
        Icon_DrawRotatedAt(0x48, y, w, h, color, 0xC001, 0x31, 0x48, 0x39);
    } else if (t < 39600) {
        Icon_DrawRotatedAt(0x48, y, w, h, color, 0xC001, 0x33, 0x48, 0x39);
        return;
    } else if (t < 43200) {
        Icon_DrawRotatedAt(0x48, y, w, h, color, 0xC001, 0x35, 0x48, 0x39);
        return;
    }
    if (t < 25200) {
        Icon_DrawRotatedAt(0x47, y, w, h, color, 0x8001, 0x31, 0x48, 0x39);
    } else if (t < 28800) {
        Icon_DrawRotatedAt(0x47, y, w, h, color, 0x8001, 0x33, 0x48, 0x39);
        return;
    } else if (t < 32400) {
        Icon_DrawRotatedAt(0x47, y, w, h, color, 0x8001, 0x35, 0x48, 0x39);
        return;
    }
    if (t < 14400) {
        Icon_DrawRotatedAt(0x48, y, w, h, color, 0x4000, 0x31, 0x48, 0x39);
    } else if (t < 18000) {
        Icon_DrawRotatedAt(0x48, y, w, h, color, 0x4000, 0x33, 0x48, 0x39);
        return;
    } else if (t < 21600) {
        Icon_DrawRotatedAt(0x48, y, w, h, color, 0x4000, 0x35, 0x48, 0x39);
        return;
    }
    if (t < 3600) {
        Icon_DrawRotated(0x48, y, w, h, color, 0, 0x31);
    } else if (t < 7200) {
        Icon_DrawRotated(0x48, y, w, h, color, 0, 0x33);
        return;
    } else if (t < 10800) {
        Icon_DrawRotated(0x48, y, w, h, color, 0, 0x35);
        return;
    }
}

extern s16 gClockWobble;
void VillageClock_DrawHand(u32 color)
{
    s32 ang = 0;
    f32 f;

    if (gClockWobble != 0) {
        f = 2.0f;
        f = f * gClockWobble;
        ang = (u16)(s32)(0.5f + 65536.0f * f / 360.0f);
    }
    Icon_DrawRotatedAt(0x48, 0x19, 0x20, 0x40, color, ang, 0x2B, 0x48, 0x39);
    Icon_DrawRotatedAt(0x29, 0x19, 0x20, 0x40, color, ang, 0x2C, 0x48, 0x39);
}

typedef struct TexTri {
    s16 x0, y0, x1, y1, x2, y2;
    u32 color;
    UV uv0, uv1, uv2;
} TexTri;

void Vec_RotateZ(f32 *v, f32 angle);
struct PrimTexTri;
s32 Prim_DrawTexTri(struct PrimTexTri *);

void TexTri_DrawRotated(TexTri *src, s16 cx, s16 cy, u32 angle)
{
    TexTri t;
    f32 v[3];
    f32 a;

    v[2] = 0.0f;
    v[0] = src->x0 - cx;
    v[1] = src->y0 - cy;
    a = 6.2831855f * angle / 65536.0f;
    Vec_RotateZ(v, a);
    t.x0 = cx + v[0];
    t.y0 = cy + v[1];
    t.x1 = src->x1;
    t.y1 = src->y1;
    v[0] = src->x2 - cx;
    v[1] = src->y2 - cy;
    Vec_RotateZ(v, a);
    t.x2 = cx + v[0];
    t.y2 = cy + v[1];
    t.color = src->color;
    t.uv0 = src->uv0;
    t.uv1 = src->uv1;
    t.uv2 = src->uv2;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
}

extern TexSrc gClockSectorTex[];

void VillageClock_DrawSector(u32 color, f32 ang, f32 amount)
{
    TexTri t;
    TexSrc *src = (TexSrc *)gClockSectorTex;
    s16 w;
    s16 h;
    s32 x1;
    s16 d;

    w = src->uv1.u - src->uv0.u;
    h = src->uv1.v - src->uv0.v;
    x1 = w + 0x25;
    t.x0 = 0x25;
    t.y0 = 0x3C;
    t.y1 = 0x3C;
    t.x1 = x1;
    t.x2 = 0x25;
    t.y2 = h + 0x3C;
    t.uv0.u = src->uv0.u;
    t.uv0.v = src->uv0.v;
    t.uv1.u = src->uv1.u;
    t.uv1.v = src->uv0.v;
    t.uv2.u = src->uv0.u;
    t.uv2.v = src->uv1.v;
    t.color = color;
    if (amount >= 90.0f) {
        Icon_DrawRotatedAt(0x25, 0x3C, w, h, color, (u16)(s32)(0.5f + 65536.0f * -ang / 360.0f), 0x30, x1, 0x3C);
        amount -= 90.0f;
        ang += 90.0f;
    }
    if (amount >= 45.0f) {
        TexTri_DrawRotated(&t, x1, 0x3C, (u16)(s32)(0.5f + 65536.0f * -ang / 360.0f));
        amount -= 45.0f;
        ang += 45.0f;
    }
    if (amount > 0.0f) {
        d = h * (amount / 45.0f);
        t.y2 = d + 0x3C;
        t.uv2.v = src->uv0.v + d;
        TexTri_DrawRotated(&t, x1, 0x3C, (u16)(s32)(0.5f + 65536.0f * -ang / 360.0f));
    }
}

extern u32 gSeasonColors[];
extern u32 gTownSeasonDays[];
s32 LobbyWorld_GetSeason(void);
s32 LobbyWorld_GetSeasonDay(void);
void VillageClock_DrawDays(void)
{
    f32 amount[4];
    u32 i;
    u32 color[4];
    f32 ang;
    u32 cnt[4];
    u32 t;
    u32 idx;
    f32 total;

    idx = LobbyWorld_GetSeason();
    t = LobbyWorld_GetSeasonDay();
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    cnt[0] = gTownSeasonDays[idx] - t;
    cnt[1] = gTownSeasonDays[(idx + 1) % 3];
    cnt[2] = gTownSeasonDays[(idx + 2) % 3];
    cnt[3] = t;
    total = cnt[0] + cnt[1] + cnt[2] + cnt[3];
    for (i = 0; i < 4; i++) {
        color[i] = gSeasonColors[(idx + i) % 3];
        amount[i] = 180.0f * (cnt[i] / total);
    }
    ang = 45.0f;
    for (i = 0; i < 4; i++) {
        t = color[i];
        VillageClock_DrawSector(t, ang, amount[i]);
        ang += amount[i];
    }
}

extern u32 gSeasonColors[];
s32 LobbyWorld_GetSeason(void);
void VillageClock_DrawDays(void);
extern s32 gTownMode;
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Icon_DrawPreset(s32 i);
s32 Game_IsOnline(void);
void Menu_DrawMailIcon(s16 x, s16 y);
void VillageClock_DrawChatMark(s32 x, s32 y);

void VillageClock_Draw(void)
{
    u32 c4;
    u32 c3;
    u32 c2;
    u32 c1;
    u32 k;

    k = LobbyWorld_GetSeason();
    switch (gTownMode) {
    case 0x1A:
    case 0xB:
    case 0x2:
    case 0x7:
    case 0x1F:
    case 0x20:
    case 0x35:
    case 0x3F:
    case 0x34:
    case 0x1:
    case 0x14:
    case 0x4:
    case 0x5:
    case 0x2C:
    case 0x2D:
    case 0x4A:
    case 0x43:
    case 0x19:
    case 0x36:
    case 0x37:
    case 0x47:
    case 0x48:
    case 0x33:
    case 0x46:
    case 0:
        break;
    default:
        return;
    }
    if (gClockWobble != 0) {
        gClockWobble--;
    }
    if (gClockSpinAddDays != 0) {
        gClockSpinAddDays--;
    }
    if (gClockSpinNextDay != 0) {
        gClockSpinNextDay--;
    }
    if (LobbyTime_IsPm() == 0) {
        c1 = 0xFFC6AD20;
    } else {
        c1 = 0xFF3030A0;
    }
    switch (k) {
    case 2:
        c3 = 0xFF398C34;
        c2 = 0xFFFFF639;
        break;
    case 0:
        c3 = 0xFFC6AD20;
        c2 = 0xFFFA8F31;
        break;
    case 1:
        c3 = 0xFF53ACCC;
        c2 = 0xFFB888FE;
        break;
    }
    c4 = gSeasonColors[k];
    VillageClock_DrawRing(c3);
    VillageClock_DrawDays();
    Icon_Draw(0x10, 0x16, 0x70, 0x4F, c3, 0x2A);
    VillageClock_DrawHours(c1);
    if (LobbyTime_IsPm() == 0) {
        Icon_DrawPreset(0);
    } else {
        Icon_DrawPreset(1);
    }
    VillageClock_DrawHand(c2);
    Icon_Draw(0x22, 0x49, 0xE, 0xE, c4, k + 0x2D);
    if (Game_IsOnline() == 1) {
        Menu_DrawMailIcon(0x82, 0x1A);
        VillageClock_DrawChatMark(0xA0, 0x1C);
    }
}

/* unk_6F62D0's .data (0x7EBFF0-0x7EC000) */
u32 gSeasonColors[4] = {
#include "lobby/007EBFF0.inc"
};
