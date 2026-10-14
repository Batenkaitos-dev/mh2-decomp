/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "browser.h"
#include "m2c_macros.h"

BrowserResult *BrowserReq_Poll(char **url);
void Browser_ShowList(s32 unused0);

void Portal_WaitReqStep(void) {
    if (BrowserReq_Poll(&gBrowserWork->url) != 0) {
        gBrowserWork->reqBusy = 0;
        Browser_ShowList(0);
        gBrowserSub->dlgCursor = 1;
        gBrowserState->step = 5;
    }
}
