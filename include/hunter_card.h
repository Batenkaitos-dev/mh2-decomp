#ifndef HUNTER_CARD_H
#define HUNTER_CARD_H

#include "common.h"
#include "menu_task.h"

/*
 * The hunter card (HunterCard), what a hunter shows the others online: the
 * town's card of another hunter (PcCard_Open, PcCard_Update, PcCard_Draw),
 * the friend limit window of the field menu's status page
 * (FriendLimitWin_*), and the card itself: HunterCard_Build makes the local
 * hunter's, HunterCard_Apply puts a received one on a player and
 * HunterCard_Sanitize brings a received one's fields into their ranges.
 */

void PcCard_Open(void);
s32 PcCard_Update(u16 pad);
void PcCard_Draw(MenuTask *task);
void FriendLimitWin_Open(void);
void FriendLimitWin_Close(void);
void FriendLimitWin_Draw(void);
void HunterCard_Build(void *out);
void HunterCard_Apply(s8 idx, void *src);
s32 HunterCard_Sanitize(void *data);

#endif
