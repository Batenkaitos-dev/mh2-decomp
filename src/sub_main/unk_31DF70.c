/*
 * The requests of the state machine at gCamView (a request byte in
 * gCamCtl.request with its argument in gCamCtl.requestArg; see unk_323C90.c).
 */
#include "common.h"
#include "em.h"
#include "camera.h"

/* prototypes */
/* end of prototypes */


void Hud_SetUnkA9(void);
s32 CamScript_Run(CamCtl *cam, CamView *obj);

/* The init handler of camera mode 5 (gCamModeInit). */
void CamScript_Init(CamCtl *c, CamView *v)
{
    v->roll = 0.0f;
    v->fov = 0.9599311f;
    c->script.id = 0;
    c->script.state = 0;
    c->script.obj = NULL;
    v->flags = 0;
}

/* A camera mode's update: step 0 waits for a request (c->request and
 * c->requestArg into the script work), step 1 runs
 * CamScript_Run until it returns a positive value (then resets); while it
 * returns 0 the script's state is 1, while negative -1. */
void CamScript_Update(CamCtl *c, CamView *v)
{
    CamScript *w = &c->script;
    s32 r;

    v->active = 0;
    switch (v->step) {
    case 0:
        if (c->request == 0) {
            break;
        }
        w->id = c->request;
        w->obj = c->requestArg;
        v->step++;
        ((CamStep *)&v->step)->count = 0;
        w->place = 1;
        w->eyeSpace = 4;
        w->atSpace = 4;
    case 1:
        c->request = 0;
        r = CamScript_Run(c, v);
        if (r <= 0) {
            v->active = 1;
            if (r == 0) {
                w->state = 1;
            } else {
                w->state = -1;
            }
        } else {
            v->step = 0;
            w->id = 0;
            w->state = 0;
            w->obj = NULL;
            v->flags = 0;
        }
        break;
    }
}

void CamReq_Set(s32 req, void *arg)
{
    gCamCtl.request = req;
    gCamCtl.requestArg = arg;
    gCamCtl.modes[5].flags = 0;
}

void CamReq_Start(s32 req, void *arg)
{
    gCamCtl.request = req;
    gCamCtl.requestArg = arg;
    gCamCtl.modes[5].flags = 2;
    Hud_SetUnkA9();
}

s32 CamReq_State(void);
void CamReq_Cancel(void);

void CamReq_Change(s32 req, void *arg)
{
    CamScript *b = &gCamCtl.script;

    if (CamReq_State()) {
        if (b->id != req) {
            CamReq_Cancel();
            CamReq_Start(req, arg);
        }
    }
}

void CamReq_Cancel(void)
{
    CamView *a = &gCamCtl.modes[5];
    CamScript *b = &gCamCtl.script;

    a->step = 0;
    b->id = 0;
    b->state = 0;
    b->obj = NULL;
    a->flags = 0;
    gCamCtl.request = 0;
}

s32 CamReq_State(void)
{
    return gCamCtl.script.state;
}

s32 CamReq_Started(void)
{
    return (gCamCtl.modes[5].flags & 2) != 0;
}
