#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "browser.h"

s8 Page_CheckImages(void);
void Browser_ShowList(s32 unused0);

void Portal_ImagesStep(void)
{
    u8 step;

    step = gBrowserState->page;
    switch (step) {
    case 8:
        Page_CheckImages();
        break;
    case 10:
        Browser_ShowList(0);
        gBrowserSub->dlgCursor = 1;
        gBrowserState->step = 5;
        break;
    }
}
