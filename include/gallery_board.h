#ifndef GALLERY_BOARD_H
#define GALLERY_BOARD_H

#include "common.h"
#include "stageobj.h"

/*
 * The shooting gallery's target board, a lobby stage object (GalleryBoard,
 * LOBBY_OBJ_GALLERY_BOARD): its update and its four states, which lay its 15
 * targets out in 3 rows of 5, flip them up, hold them standing for the
 * course's frames and flip them down. gGalleryTargetSizes gives the targets'
 * sizes.
 */

void GalleryBoard_Update(StageObj *obj);
void GalleryBoard_Init(StageObj *obj);
void GalleryBoard_Run(StageObj *obj);
void GalleryBoard_End(StageObj *obj);
void GalleryBoard_Free(StageObj *obj);
extern f32 gGalleryTargetSizes[4];

#endif
