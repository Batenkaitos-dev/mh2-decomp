#ifndef VILLAGE_PROGRESS_H
#define VILLAGE_PROGRESS_H

#include "common.h"

/*
 * The village's progress and the MH Portable link: Village_UpdateProgress
 * sets the story's event flags, raises the village facilities' levels and
 * grants awards from what the hunter has done; PspLink_GrantBonuses,
 * PspGift_Pick and PspGift_Give give the link's bonuses and gift.
 * Monster_AllHunted says whether every monster of the hunted list has been
 * hunted.
 */

s32 PspGift_Pick(void);
void PspGift_Give(s32 gift);
void PspLink_GrantBonuses(void);
s32 Monster_AllHunted(void);
void Village_UpdateProgress(s32 afterOffer);

#endif
