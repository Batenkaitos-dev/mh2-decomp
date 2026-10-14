/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "stageobj.h"
#include "lobby_menu.h"

void VFlock_Update(StageObj *);
void *memset(void *dst, s32 c, u32 n);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
extern MenuWork gMenuWork;

void VFlock_Free(VObj *o)
{
    StageObj_Free((StageObj *) o);
}

void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Gs_SetState(u8, u32);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void VFlock_Draw(DrawSlot *slot)
{
    Vec3f trans;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    Model *mdl;
    ModelMat *parts;
    VFlock *w;
    ModelMatSet *mesh;
    s32 i;
    VBird *b;
    s32 j;
    ModelMat *p;

    w = (VFlock *)slot->owner;
    mdl = gCommonModels[0];
    b = w->bird;
    if (mdl == NULL || mdl->active == 0) {
        return;
    }
    for (i = 0; i < 6; i++, b++) {
        parts = mdl->mats;
        mesh = mdl->matSets + b->mesh;
        for (j = 1; j < mesh->count; j++) {
            p = &parts[mesh->mat[j]];
            if (j == b->part) {
                p->alpha = 1.0f;
            } else {
                p->alpha = 0.0f;
            }
        }
        trans = b->pos;
        scale[0] = w->scale;
        scale[1] = w->scale;
        scale[2] = w->scale;
        rot[0] = 0.0f;
        rot[1] = (6.2831855f * b->ang) / 65536.0f;
        rot[2] = 0.0f;
        Mtx_FromScaleRotTrans((f32 *) scale, (f32 *) rot, (f32 *) &trans, 2, (f32 *) m);
        Gs_SetState(0x67, b->color);
        Effect_DrawModelAdd(mesh, m, parts);
    }
}

void VFlock_Spawn(s8 arg0) {
    StageObj *obj;

    obj = StageObj_Alloc(1);
    if (obj != NULL) {
        obj->kind = 0x2A;
        obj->update = VFlock_Update;
        obj->draw = NULL;
        obj->type = arg0;
    }
}

s32 PlaceInfo_Open(void) {
    gHelpShow = 0;
    memset(&gMenuWork, 0, 0xF00);
    WinSlide_Step(&gMenuWork.win, 0x280, 0x136);
    return 0;
}
