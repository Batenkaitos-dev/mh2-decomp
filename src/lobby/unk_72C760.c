/* Browser_OpenDialog. include/browser.h. */
#include "common.h"
#include "browser.h"

/* Browser_OpenDialog: opens dialog kind (BrowserWork.dlgBoxKind) with arg. */
void Browser_OpenDialog(s8 kind, s8 arg)
{
    gBrowserWork->dlgBoxKind = kind;
    gBrowserWork->dlgCursorState = 2;
    gBrowserSub->dlgCursor = arg;
    gBrowserSub->dlgMode = 3;
    gBrowserWork->dlgBoxReady = 0;
}
