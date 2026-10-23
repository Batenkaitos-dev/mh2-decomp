#ifndef ARM_WRESTLING_H
#define ARM_WRESTLING_H

#include "common.h"
#include "menu_task.h"

/*
 * The arm-wrestling contest (TOWN_MODE_CONTEST), against a townsperson
 * offline or another member online: Contest_Open and Contest_Update run it,
 * Contest_Play runs the match, Contest_RecvStart, Contest_RecvPower and
 * Contest_RecvResult take the other contestant's messages, and
 * Contest_GivePrize gives the prizes.
 */

s32 Contest_IsOpen(void);
void Contest_Open(void *opponent);
void Contest_Update(void);
s32 Contest_IsIdle(void);
s32 Contest_PrizeDue(void);
s32 Contest_IsTopPrize(void);
s32 Contest_TalkUpdate(void);
void Contest_NpcPress(void);
void Contest_PadPress(void);
s32 Contest_Play(void);
void Contest_RecvStart(void *msg);
void Contest_RecvPower(void *src);
void Contest_RecvResult(s8 slot, void *theirResult);
void Contest_DrawPlay(void);
void Contest_Draw(MenuTask *task);
s8 Contest_RollLevel(void);
void Contest_GivePrize(u8 won);

#endif
