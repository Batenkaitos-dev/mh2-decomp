/*
 * The village clock in the town's top-left corner, and the hooks of the
 * lobby's world clock that set it moving.
 *
 * VillageClock_Draw draws it in TOWN_MODE_FREE and in the menus that leave
 * it on screen, from the lobby's icon sheet (gIconSprites, the ICON_CLOCK_*
 * entries), round its centre (CLOCK_CENTER_X, CLOCK_CENTER_Y):
 * - the ring: eight pieces round the centre, each turned a half turn in
 *   the afternoon (VillageClock_DrawRing);
 * - the season dial: a half circle of four sectors, the days left of this
 *   season, the next two seasons' days and the days gone, each in its
 *   season's colour (gSeasonColors; VillageClock_DrawDays and
 *   VillageClock_DrawSector);
 * - the face, the hour marks (VillageClock_DrawHours), the morning or
 *   afternoon mark (gIconPresets' first two), the hand, the season's mark
 *   and, online, the mail icon and the chat mark.
 *
 * Three timers move it for 15 frames, each set by a hook the world's clock
 * calls: the hand wobbles each hour (gClockWobble, VillageClock_Wobble), the
 * ring spins at midnight and noon (gClockSpinHalfDay, LobbyWorld_OnHalfDay)
 * and the pieces round the dial spin when a season begins
 * (gClockSpinSeason, LobbyWorld_OnNewSeason). The two hooks also renew the
 * village's offers offline and the quest lists.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "area.h"
#include "quest.h"
#include "texture.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "model.h"
#include "prim.h"
#include "text.h"
#include "village_clock.h"
#include "chat_send.h"
#include "engine_math.h"
#include "game_util.h"
#include "lobby_world.h"
#include "menu_widgets.h"
#include "quest_offers.h"
#include "village_offers.h"
#include "libc.h"
#include "sprite_queue.h"

/* The clock's centre on screen, the pivot of its turning pieces. */
#define CLOCK_CENTER_X 72
#define CLOCK_CENTER_Y 57

/* The clock's timers count down from this many frames. */
#define CLOCK_ANIM_FRAMES 15

extern s16 gClockSpinHalfDay;
extern s16 gClockSpinSeason;
void SeasonBanner_CreateOnline(s32, s32); /* unit's own declaration: declared with the s32 the caller passes; with the definition's u8 parameters, LobbyWorld_OnNewSeason compiles differently */
extern s32 gTownMode;
extern u8 gLobbyLevel1Sel;
extern s16 gClockWobble;


/* The hour has changed: the clock's hand wobbles (VillageClock_DrawHand). */
void VillageClock_Wobble(void)
{
    gClockWobble = CLOCK_ANIM_FRAMES;
}

/*
 * Midnight or noon: the clock's ring spins. At midnight new offers are made
 * offline (Offer_Generate); at noon, online in a land (gLobbyLevel1Sel), the
 * town's music starts again (LobbyBgm_Play's last argument is 0 in
 * AREA_TOWN_ARENA, 1 elsewhere). The quest lists are rebuilt and the
 * townspeople can be talked to again (Npc_ClearAllTalked).
 */
void LobbyWorld_OnHalfDay(void)
{
    s32 isPm = LobbyTime_IsPm();

    gClockSpinHalfDay = CLOCK_ANIM_FRAMES;
    if (isPm == 0) {
        if (Game_IsOnline() == 0) {
            Offer_Generate();
        }
    } else if (Game_IsOnline() == 1 && gLobbyLevel1Sel != 0) {
        if (gScene.area == AREA_TOWN_ARENA) {
            LobbyBgm_Play(0, 0, 0);
        } else {
            LobbyBgm_Play(0, 0, 1);
        }
    }
    QuestList_Rebuild();
    Npc_ClearAllTalked();
}

/*
 * A new season has begun: the pieces round the clock's dial spin, the
 * offers are made anew offline, the quest lists are rebuilt, the
 * townspeople can be talked to again, and with no menu open the season's
 * banner shows (SeasonBanner_CreateOnline).
 */
void LobbyWorld_OnNewSeason(void)
{
    s32 season;

    gClockSpinSeason = CLOCK_ANIM_FRAMES;
    if (Game_IsOnline() == 0) {
        Offer_ClearAll();
        Offer_Generate();
    }
    QuestList_Rebuild();
    Npc_ClearAllTalked();
    if (gTownMode == 0) {
        season = LobbyWorld_GetSeason() & 0xFF;
        SeasonBanner_CreateOnline(season, LobbyTime_IsPm() & 0xFF);
    }
}

#pragma optimization_level 4
/*
 * Draws the clock's ring: eight copies of ICON_CLOCK_RING, 45 degrees apart
 * round the clock's centre. Each piece is turned about its own centre, a
 * half turn in the afternoon, spinning 12 degrees a frame of
 * gClockSpinHalfDay; the eight spin round the centre with gClockSpinSeason.
 * The rest of the file is built at optimization level 4, as the game's
 * code is.
 */
void VillageClock_DrawRing(u32 color)
{
    TexRect rect;
    f32 pieceTurn;
    f32 ringTurn;
    IconSprite *ring;
    u32 pieceAngle;
    s16 i;
    s16 pieceX;
    s16 pieceY;
    s32 degrees;

    if (LobbyTime_IsPm() == 0) {
        pieceTurn = 0.0f;
    } else {
        pieceTurn = 180.0f;
    }
    pieceTurn += 12.0f * gClockSpinHalfDay;
    ringTurn = 12.0f * gClockSpinSeason;
    ring = &gIconSprites[ICON_CLOCK_RING];
    rect.x = 64;
    rect.y = 6;
    rect.w = abs(ring->uv1.u - ring->uv0.u);
    rect.h = abs(ring->uv1.v - ring->uv0.v);
    rect.color = color;
    rect.uv0 = ring->uv0;
    rect.uv1 = ring->uv1;
    Tex_Prepare(1, ring->tex);
    Tex_Bind(ring->tex);
    pieceX = rect.x + rect.w / 2;
    pieceY = rect.y + 19;
    pieceAngle = (u16)(s32)(0.5f + 65536.0f * pieceTurn / 360.0f);
    for (i = 0, degrees = 0; i < 8; i++, degrees += 45) {
        TexRect_DrawRotated2(&rect, pieceX, pieceY, CLOCK_CENTER_X, CLOCK_CENTER_Y, pieceAngle, (u16)(s32)(0.5f + 65536.0f * (ringTurn + degrees) / 360.0f));
    }
}

extern u32 gLobbyTime;

/* An hour of gLobbyTime, in seconds. */
#define CLOCK_HOUR 3600
void Icon_DrawRotated(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s32 i); /* unit's own declaration: declared with the s16 coordinates the caller passes; with the definition's s32 parameters, VillageClock_DrawHours compiles differently */

/*
 * Draws the four hour marks, at 0, 3, 6 and 9 o'clock on the half day's
 * dial (gLobbyTime within its 12 hours). A mark is drawn until its three
 * hours are past: ICON_CLOCK_HOUR_MARK up to its first hour's end, then
 * ICON_CLOCK_HOUR_MARK + 2 and + 4 in its second and third hours.
 */
void VillageClock_DrawHours(u32 color)
{
    u32 secs;
    s16 y;
    s16 markW;
    s16 markH;
    s32 iconH;
    IconSprite *mark = &gIconSprites[ICON_CLOCK_HOUR_MARK];

    iconH = mark->uv1.v - mark->uv0.v;
    y = CLOCK_CENTER_Y - iconH;
    markH = iconH;
    markW = mark->uv1.u - mark->uv0.u;
    secs = gLobbyTime % LOBBY_CLOCK_NOON;
    if (secs < 10 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X, y, markW, markH, color, ANGLE_TURN * 3 / 4 + 1, ICON_CLOCK_HOUR_MARK, CLOCK_CENTER_X, CLOCK_CENTER_Y);
    } else if (secs < 11 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X, y, markW, markH, color, ANGLE_TURN * 3 / 4 + 1, ICON_CLOCK_HOUR_MARK + 2, CLOCK_CENTER_X, CLOCK_CENTER_Y);
        return;
    } else if (secs < 12 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X, y, markW, markH, color, ANGLE_TURN * 3 / 4 + 1, ICON_CLOCK_HOUR_MARK + 4, CLOCK_CENTER_X, CLOCK_CENTER_Y);
        return;
    }
    if (secs < 7 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X - 1, y, markW, markH, color, ANGLE_TURN / 2 + 1, ICON_CLOCK_HOUR_MARK, CLOCK_CENTER_X, CLOCK_CENTER_Y);
    } else if (secs < 8 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X - 1, y, markW, markH, color, ANGLE_TURN / 2 + 1, ICON_CLOCK_HOUR_MARK + 2, CLOCK_CENTER_X, CLOCK_CENTER_Y);
        return;
    } else if (secs < 9 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X - 1, y, markW, markH, color, ANGLE_TURN / 2 + 1, ICON_CLOCK_HOUR_MARK + 4, CLOCK_CENTER_X, CLOCK_CENTER_Y);
        return;
    }
    if (secs < 4 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X, y, markW, markH, color, ANGLE_TURN / 4, ICON_CLOCK_HOUR_MARK, CLOCK_CENTER_X, CLOCK_CENTER_Y);
    } else if (secs < 5 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X, y, markW, markH, color, ANGLE_TURN / 4, ICON_CLOCK_HOUR_MARK + 2, CLOCK_CENTER_X, CLOCK_CENTER_Y);
        return;
    } else if (secs < 6 * CLOCK_HOUR) {
        Icon_DrawRotatedAt(CLOCK_CENTER_X, y, markW, markH, color, ANGLE_TURN / 4, ICON_CLOCK_HOUR_MARK + 4, CLOCK_CENTER_X, CLOCK_CENTER_Y);
        return;
    }
    if (secs < 1 * CLOCK_HOUR) {
        Icon_DrawRotated(CLOCK_CENTER_X, y, markW, markH, color, 0, ICON_CLOCK_HOUR_MARK);
    } else if (secs < 2 * CLOCK_HOUR) {
        Icon_DrawRotated(CLOCK_CENTER_X, y, markW, markH, color, 0, ICON_CLOCK_HOUR_MARK + 2);
        return;
    } else if (secs < 3 * CLOCK_HOUR) {
        Icon_DrawRotated(CLOCK_CENTER_X, y, markW, markH, color, 0, ICON_CLOCK_HOUR_MARK + 4);
        return;
    }
}


/* Draws the clock's hand, its two halves turned about the clock's centre by
 * 2 degrees a frame of gClockWobble left. */
void VillageClock_DrawHand(u32 color)
{
    s32 angle = 0;
    f32 degrees;

    if (gClockWobble != 0) {
        degrees = 2.0f;
        degrees = degrees * gClockWobble;
        angle = (u16)(s32)(0.5f + 65536.0f * degrees / 360.0f);
    }
    Icon_DrawRotatedAt(CLOCK_CENTER_X, 25, 32, 64, color, angle, ICON_CLOCK_HAND_RIGHT, CLOCK_CENTER_X, CLOCK_CENTER_Y);
    Icon_DrawRotatedAt(41, 25, 32, 64, color, angle, ICON_CLOCK_HAND_LEFT, CLOCK_CENTER_X, CLOCK_CENTER_Y);
}

/* A textured triangle as Prim_DrawTexTri takes it: three corners, a colour
 * and the corners on the texture. */
typedef struct TexTri {
    s16 x0, y0, x1, y1, x2, y2;
    u32 color;
    UV uv0, uv1, uv2;
} TexTri;

struct PrimTexTri;

/*
 * Draws `src` turned by `angle` (a 16-bit angle) about (pivotX, pivotY): the
 * first and third corners are turned, the second is drawn where it is (the
 * callers put it at the pivot).
 */
void TexTri_DrawRotated(TexTri *src, s16 pivotX, s16 pivotY, u32 angle)
{
    TexTri tri;
    f32 vec[3];
    f32 radians;

    vec[2] = 0.0f;
    vec[0] = src->x0 - pivotX;
    vec[1] = src->y0 - pivotY;
    radians = 6.2831855f * angle / 65536.0f;
    Vec_RotateZ(vec, radians);
    tri.x0 = pivotX + vec[0];
    tri.y0 = pivotY + vec[1];
    tri.x1 = src->x1;
    tri.y1 = src->y1;
    vec[0] = src->x2 - pivotX;
    vec[1] = src->y2 - pivotY;
    Vec_RotateZ(vec, radians);
    tri.x2 = pivotX + vec[0];
    tri.y2 = pivotY + vec[1];
    tri.color = src->color;
    tri.uv0 = src->uv0;
    tri.uv1 = src->uv1;
    tri.uv2 = src->uv2;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
}


/*
 * Draws one sector of the season dial in `color`: `amount` degrees from
 * `ang` degrees, about the dial's pivot. ICON_CLOCK_SECTOR is a quarter
 * circle: a quarter is drawn whole, then an eighth as half of it (a
 * triangle), then what is left as a narrower triangle.
 */
void VillageClock_DrawSector(u32 color, f32 ang, f32 amount)
{
    TexTri tri;
    IconSprite *sector = &gIconSprites[ICON_CLOCK_SECTOR];
    s16 sectorW;
    s16 sectorH;
    s32 pivotX;
    s16 partH;

    sectorW = sector->uv1.u - sector->uv0.u;
    sectorH = sector->uv1.v - sector->uv0.v;
    pivotX = sectorW + 37;
    tri.x0 = 37;
    tri.y0 = 60;
    tri.y1 = 60;
    tri.x1 = pivotX;
    tri.x2 = 37;
    tri.y2 = sectorH + 60;
    tri.uv0.u = sector->uv0.u;
    tri.uv0.v = sector->uv0.v;
    tri.uv1.u = sector->uv1.u;
    tri.uv1.v = sector->uv0.v;
    tri.uv2.u = sector->uv0.u;
    tri.uv2.v = sector->uv1.v;
    tri.color = color;
    if (amount >= 90.0f) {
        Icon_DrawRotatedAt(37, 60, sectorW, sectorH, color, (u16)(s32)(0.5f + 65536.0f * -ang / 360.0f), ICON_CLOCK_SECTOR, pivotX, 60);
        amount -= 90.0f;
        ang += 90.0f;
    }
    if (amount >= 45.0f) {
        TexTri_DrawRotated(&tri, pivotX, 60, (u16)(s32)(0.5f + 65536.0f * -ang / 360.0f));
        amount -= 45.0f;
        ang += 45.0f;
    }
    if (amount > 0.0f) {
        partH = sectorH * (amount / 45.0f);
        tri.y2 = partH + 60;
        tri.uv2.v = sector->uv0.v + partH;
        TexTri_DrawRotated(&tri, pivotX, 60, (u16)(s32)(0.5f + 65536.0f * -ang / 360.0f));
    }
}

extern u32 gTownSeasonDays[];
/*
 * Draws the season dial: half a circle from 45 degrees, shared out by days
 * among four sectors: the days left of this season, the next season's
 * days, the one after's, and this season's days gone, each in its season's
 * colour.
 */
void VillageClock_DrawDays(void)
{
    f32 amount[4];
    u32 i;
    u32 color[4];
    f32 ang;
    u32 days[4];
    u32 value; /* the season's day, then each sector's colour */
    u32 season;
    f32 total;

    season = LobbyWorld_GetSeason();
    value = LobbyWorld_GetSeasonDay();
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    days[0] = gTownSeasonDays[season] - value;
    days[1] = gTownSeasonDays[(season + 1) % SEASON_COUNT];
    days[2] = gTownSeasonDays[(season + 2) % SEASON_COUNT];
    days[3] = value;
    total = days[0] + days[1] + days[2] + days[3];
    for (i = 0; i < 4; i++) {
        color[i] = gSeasonColors[(season + i) % SEASON_COUNT];
        amount[i] = 180.0f * (days[i] / total);
    }
    ang = 45.0f;
    for (i = 0; i < 4; i++) {
        value = color[i];
        VillageClock_DrawSector(value, ang, amount[i]);
        ang += amount[i];
    }
}

/*
 * Draws the village clock, in TOWN_MODE_FREE and the town modes listed
 * (the other menus cover it), and counts its three timers down. The ring
 * and the face take the season's colour, the hour marks the time of day's.
 */
void VillageClock_Draw(void)
{
    u32 seasonColor;
    u32 faceColor;
    u32 handColor;
    u32 markColor;
    u32 season;

    season = LobbyWorld_GetSeason();
    switch (gTownMode) {
    case TOWN_MODE_KEYBOARD:
    case TOWN_MODE_GAME_MENU:
    case TOWN_MODE_ITEM_SHOP:
    case TOWN_MODE_SET_OUT:
    case TOWN_MODE_DEPART:
    case TOWN_MODE_DEPART_FADE:
    case TOWN_MODE_QUEST_DESK:
    case TOWN_MODE_DOJO:
    case TOWN_MODE_ARENA_CUP:
    case TOWN_MODE_QUEST_COUNTER:
    case TOWN_MODE_SAVE:
    case TOWN_MODE_TALK:
    case 5:
    case TOWN_MODE_BAIT_MENU:
    case TOWN_MODE_PRICE_INFO:
    case TOWN_MODE_VILLAGE_BOARD:
    case TOWN_MODE_OFFER_MENU:
    case TOWN_MODE_SEAT_WAIT:
    case TOWN_MODE_CONTEST_SEAT_WAIT:
    case TOWN_MODE_CONTEST_SEATED:
    case TOWN_MODE_INGREDIENTS:
    case TOWN_MODE_KITCHEN:
    case TOWN_MODE_MINE:
    case TOWN_MODE_RECORDS:
    case TOWN_MODE_FREE:
        break;
    default:
        return;
    }
    if (gClockWobble != 0) {
        gClockWobble--;
    }
    if (gClockSpinHalfDay != 0) {
        gClockSpinHalfDay--;
    }
    if (gClockSpinSeason != 0) {
        gClockSpinSeason--;
    }
    if (LobbyTime_IsPm() == 0) {
        markColor = 0xFFC6AD20;
    } else {
        markColor = 0xFF3030A0;
    }
    switch (season) {
    case SEASON_BREEDING:
        faceColor = 0xFF398C34;
        handColor = 0xFFFFF639;
        break;
    case SEASON_WARM:
        faceColor = 0xFFC6AD20;
        handColor = 0xFFFA8F31;
        break;
    case SEASON_COLD:
        faceColor = 0xFF53ACCC;
        handColor = 0xFFB888FE;
        break;
    }
    seasonColor = gSeasonColors[season];
    VillageClock_DrawRing(faceColor);
    VillageClock_DrawDays();
    Icon_Draw(16, 22, 112, 79, faceColor, ICON_CLOCK_FACE);
    VillageClock_DrawHours(markColor);
    if (LobbyTime_IsPm() == 0) {
        Icon_DrawPreset(ICON_PRESET_CLOCK_AM);
    } else {
        Icon_DrawPreset(ICON_PRESET_CLOCK_PM);
    }
    VillageClock_DrawHand(handColor);
    Icon_Draw(34, 73, 14, 14, seasonColor, season + ICON_SEASON_FIRST);
    if (Game_IsOnline() == 1) {
        Menu_DrawMailIcon(130, 26);
        VillageClock_DrawChatMark(160, 28);
    }
}

/* The seasons' colours (packed RGBA, by SEASON_*); the fourth entry is
 * never read. */
u32 gSeasonColors[4] = {
#include "lobby/007EBFF0.inc"
};
