/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "browser.h"

char *BrowserHist_Forward(void);
s32 BrowserHist_CanForward(void);
void Browser_OpenPage(void);
void Browser_ShowPage(void);
void Browser_Go(char *url);
char *BrowserHist_Back(void);
s32 BrowserHist_CanBack(void);

void Browser_GoForward(void) {
    char *url;

    if (BrowserHist_CanForward() == 0) {
        Browser_ShowPage();
        return;
    }
    url = BrowserHist_Forward();
    if (url == 0) {
        Browser_ShowPage();
        return;
    }
    gBrowserState->fromTop = 0;
    Browser_Go(url);
    Browser_OpenPage();
}

void Browser_GoBack(void) {
    char *url;

    if (BrowserHist_CanBack() == 0) {
        Browser_ShowPage();
        return;
    }
    url = BrowserHist_Back();
    if (url == 0) {
        Browser_ShowPage();
        return;
    }
    Browser_Go(url);
    Browser_OpenPage();
}
