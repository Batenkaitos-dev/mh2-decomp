#ifndef DNAS_CHECK_H
#define DNAS_CHECK_H

#include "common.h"

/*
 * The DNAS check, run one step a frame: at the login (Dnas_AuthStep) or
 * before a patch download (Dnas_StateMachine), through Dnas_Step, with its
 * error (Dnas_GetError). With it, the transfers through the DNAS install
 * library (NetXfer_Step, NetXfer_Step2, NetXfer_GetResult).
 */

s32 NetXfer_GetResult(void);
s32 NetXfer_Step(s8 *state, s16 *timer, u8 *buf, s32 size, s32 maxSize);
s32 NetXfer_Step2(s8 *state, s16 *timer, u8 *buf, s32 size, s32 maxSize);
s32 Dnas_GetError(void);
s32 Dnas_AuthStep(s8 *state, s16 *cancel, s16 *percent);
s32 Dnas_StateMachine(s8 *state, s16 *cancel, s16 *percent);
s32 Dnas_Step(s8 *state, s16 *cancel, s16 *percent, s32 kind);

#endif
