/*
 * The lobby's small queries and text helpers:
 * - where a member is (NetUser_State, NetUser_StateByName);
 * - the blinking button prompt of the lobby's screens;
 * - string lengths, and the texts whose '$' marks take a number or a name
 *   (Text_FillDollar*, into gTextFillBuf, gTextFill2Buf, gTextFillFmtBuf);
 * - the area tests (the town's halls, the houses), the hunter rank test,
 *   the town mode test of the records' screen;
 * - the action spots taken by the town's players (gSpotTakenMask, one bit
 *   a spot over two words);
 * - the item box's level in the house the player is in (MyHouse_BoxLevel).
 */
#include "common.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_labels.h"
#include "lobby_menu.h"
#include "lobby_state.h"
#include "area.h"
#include "lobby_text.h"
#include "text.h"
#include "lobby_queries.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "my_house.h"
#include "menu_widgets.h"

s32 strlen(const char *s);
s32 Str_Length(char *s);
extern u32 gSpotTakenMask[2];
extern s32 gTownMode;

s32 memcmp(void *, void *, s32);
extern NetUser gNetUsers[];

/* NetUser_State of the member whose user id is id (LOBBY_USER_ID_LEN
 * bytes); NET_USER_ABSENT when no member has it, NET_USER_HERE offline. */
s32 NetUser_StateByName(u8 *id)
{
    s8 i;
    NetUser *user;

    if (Game_IsOnline() == 0) {
        return NET_USER_HERE;
    }
    for (i = 0, user = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, user++) {
        if (memcmp(id, user, LOBBY_USER_ID_LEN) == 0) {
            return NetUser_State(i);
        }
    }
    return NET_USER_ABSENT;
}

/* Where member i is: NET_USER_ABSENT with no member in the slot,
 * NET_USER_NO_AREA while its player has no area, else NET_USER_HERE (and
 * always offline). */
s32 NetUser_State(s8 i)
{
    Player *pl;

    if (Game_IsOnline() == 0) {
        return NET_USER_HERE;
    }
    if ((s8)gNetUsers[i].id[0] != 0) {
        pl = &gPlayers[i];
        if (pl->area == 0) {
            return NET_USER_NO_AREA;
        }
        return NET_USER_HERE;
    }
    return NET_USER_ABSENT;
}

/* Draws the button prompt at (200, 380), blinking: shown for 32
 * frames of every 64 (gFrameTick). */
void Lobby_DrawPressButton(void)
{
    if (gFrameTick & 0x20) {
        Text_SetSize(5);
        Font_SetPos(200, 380);
        Font_SetColor(0x807F7F28);
        Text_Printf(gMiscTexts[MISC_TEXT_PRESS_BUTTON]);
    }
}

/* strlen of str. */
s32 Lobby_Strlen(char *str)
{
    return strlen(str);
}

/* Str_Length of str: its length in characters. */
s32 Lobby_StrLength(char *str)
{
    return Str_Length(str);
}

char *strcat(char *, const char *);
char *strcpy(char *, const char *);
char *strrchr(const char *, s32);
s32 sprintf(char *, const char *, ...);
extern char gTextFillBuf[];
extern char gTextFill2Buf[];
extern char gTextFillFmtBuf[];

/* Copies text fmt into gTextFillBuf with its last '$' replaced by the
 * number n, and returns the copy; fmt itself when it has no '$'. */
char *Text_FillDollar(char *fmt, s32 n)
{
    char *mark;

    strcpy(gTextFillBuf, fmt);
    mark = strrchr(gTextFillBuf, '$');
    if (mark == 0) {
        return fmt;
    }
    Str_FromInt(n, mark);
    strcat(gTextFillBuf, strrchr(fmt, '$') + 1);
    return gTextFillBuf;
}

/* The second number of a text with two '$': copies Text_FillDollar's
 * result (gTextFillBuf) into gTextFill2Buf with its last '$' left replaced
 * by n, and returns the copy (the copy unchanged when no '$' is left). */
char *Text_FillDollar2(s32 n)
{
    char *mark;

    strcpy(gTextFill2Buf, gTextFillBuf);
    mark = strrchr(gTextFill2Buf, '$');
    if (mark == 0) {
        return gTextFill2Buf;
    }
    Str_FromInt(n, mark);
    strcat(gTextFill2Buf, strrchr(gTextFillBuf, '$') + 1);
    return gTextFill2Buf;
}

/* Copies text fmt into gTextFillFmtBuf with its last '$' replaced by name,
 * printed with the format gTextFillNameFmt, and returns the copy; fmt
 * itself when it has no '$'. */
char *Text_FillDollarFmt(char *fmt, char *name)
{
    char *mark;

    strcpy(gTextFillFmtBuf, fmt);
    mark = strrchr(gTextFillFmtBuf, '$');
    if (mark == 0) {
        return fmt;
    }
    sprintf(mark, gTextFillNameFmt, name);
    strcat(gTextFillFmtBuf, strrchr(fmt, '$') + 1);
    return gTextFillFmtBuf;
}

/* 1 for the town's three halls (AREA_TAVERN, AREA_TOWN_ARENA,
 * AREA_ELDER_HALL), else 0. */
s32 Area_IsTownHall(s32 area)
{
    switch (area & 0xFFFF) {
    case AREA_TAVERN:
    case AREA_TOWN_ARENA:
    case AREA_ELDER_HALL:
        return 1;
    }
    return 0;
}

/* 1 for a house: AREA_HOUSE offline, an online house's three levels
 * (AREA_ONLINE_HOUSE_0 to _2), else 0. */
s32 Area_IsHouse(u16 area)
{
    switch (area & 0xFFFF) {
    case AREA_HOUSE:
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        return 1;
    }
    return 0;
}

/* 1 when the player's hunter rank is 31 or more: the rooms that admit by
 * hunter rank split there. */
s32 Lobby_HrAtLeast31(void)
{
    return (s32) gSave.hr >= 31;
}

/* Frees every action spot. */
void ActSpot_ClearAll(void)
{
    gSpotTakenMask[1] = 0;
    gSpotTakenMask[0] = 0;
}

/* Frees action spot slot: spots 0-30 are bits of gSpotTakenMask's first
 * word, the others bit slot % 32 of its second (spot 31 is its bit 31). */
void ActSpot_Release(s32 slot)
{
    s32 bit;

    bit = slot & 0xFF;
    if (bit < 31) {
        gSpotTakenMask[0] &= ~(1 << bit);
        return;
    }
    gSpotTakenMask[1] &= ~(1 << (bit % 32));
}

/* Takes action spot slot (bits as ActSpot_Release's). */
void ActSpot_Take(u8 slot)
{
    s32 bit;

    bit = slot & 0xFF;
    if (bit < 31) {
        gSpotTakenMask[0] |= 1 << bit;
        return;
    }
    gSpotTakenMask[1] |= 1 << (bit % 32);
}

/* 1 when action spot n is free: bit n of gSpotTakenMask's first word for
 * spots 0-30, of its second (as the shift wraps) for the others. The
 * switch with its default first is the game's code. */
s32 ActSpot_IsFree(u32 n)
{
    u32 *word = &gSpotTakenMask[n >= 31];

    switch (*word & (1 << n)) {
    default:
        return 0;
    case 0:
        return 1;
    }
}

/* Declared as the caller uses it, with an s32 return:
 * unit's own declaration: with the definition's u8, MyHouse_BoxLevel compiles differently. */
s32 Village_FacilityLevel(s32);

/* The item box's level in the house the player is in (its pages - 1):
 * offline or in its own house, Village_FacilityLevel(VILLAGE_FACILITY_BOX);
 * in another's house, the owner's hunter card's boxLevel. */
u8 MyHouse_BoxLevel(void)
{
    if (MyHouse_InOwnHouse() == 1 || Game_IsOnline() == 0) {
        return Village_FacilityLevel(VILLAGE_FACILITY_BOX);
    }
    return ((HunterCard *)gNetUsers[MyHouse_LocalSlot()].text)->boxLevel;
}

/* 1 while the town shows the records' screen (TOWN_MODE_RECORDS). */
s32 Town_InRecordsMode(void)
{
    return gTownMode == TOWN_MODE_RECORDS;
}
