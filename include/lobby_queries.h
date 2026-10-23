#ifndef LOBBY_QUERIES_H
#define LOBBY_QUERIES_H

#include "common.h"

/*
 * The lobby's small queries and text helpers: where a member is
 * (NetUser_State), the blinking button prompt of the lobby's screens, string
 * lengths, the texts whose '$' marks take a number or a name
 * (Text_FillDollar*), the area and hunter rank tests, the action spots the
 * town's players hold (ActSpot_*), and the item box's level in the house the
 * player is in.
 */

s32 NetUser_StateByName(u8 *id);
s32 NetUser_State(s8 i);
void Lobby_DrawPressButton(void);
s32 Lobby_Strlen(char *str);
s32 Lobby_StrLength(char *str);
char *Text_FillDollar(char *fmt, s32 n);
char *Text_FillDollar2(s32 n);
char *Text_FillDollarFmt(char *fmt, char *name);
s32 Area_IsTownHall(s32 area);
s32 Area_IsHouse(u16 area);
s32 Lobby_HrAtLeast31(void);
void ActSpot_ClearAll(void);
void ActSpot_Release(s32 slot);
void ActSpot_Take(u8 slot);
s32 ActSpot_IsFree(u32 n);
u8 MyHouse_BoxLevel(void);
s32 Town_InRecordsMode(void);

#endif
