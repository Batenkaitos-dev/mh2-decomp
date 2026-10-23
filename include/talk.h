#ifndef TALK_H
#define TALK_H

#include "common.h"
#include "em.h"
#include "lobby_npc.h"

/*
 * The townspeople's talk: Talk_Start picks the line a talk opens with, from
 * the script's events or from its selection tree (by village condition,
 * season and time of day); Talk_Step and Talk_Update run the lines, with
 * their yes/no questions and gifts (gTalkLines, gTalkGifts, by script);
 * Talk_Say opens a single line. The lines' texts come from the talk data
 * file (Talk_FindScript, Talk_LineText).
 */

s32 Talk_CheckSeason(u16 sel);
s32 Talk_CheckTimeOfDay(u16 sel);
void Talk_Start(Em *npc, s8 quiet);
void Npc_ResetTalked(void);
u8 *Talk_FindScript(u16 id);
char *Talk_LineText(u16 line);
s32 Talk_LineKind(u16 unused);
s32 Talk_RunText(void);
s32 Talk_Step(s8 endOnAnswer);
s32 Talk_Update(void);
void Talk_Say(s32 line, s8 choice);
extern TalkLine *gTalkLines[116];
extern TalkGift *gTalkGifts[116];

#endif
