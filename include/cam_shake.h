#ifndef CAM_SHAKE_H
#define CAM_SHAKE_H

#include "common.h"
#include "em.h"

/*
 * The camera's shakes, requested at a monster's or the player's position:
 * CamShake_ReqAIf when the object is in the current area, CamShake_ReqARange
 * when it is also near enough to the eye, and CamShake_ReqC in the shake
 * slot the script view plays.
 */

void CamShake_SetA(s32 shakeKind, f32 *pos);
void CamShake_SetAEverywhere(s32 shakeKind);
void CamShake_ReqA(void *obj, s32 shakeKind);
void CamShake_ReqAIf(Em *obj, s32 shakeKind);
void CamShake_ReqARange(Em *obj, s32 shakeKind, s32 shakeRange);
void CamShake_ReqC(Em *obj, s32 shakeKind);
void CamShake_ReqB(void *obj, s32 shakeKind);

#endif
