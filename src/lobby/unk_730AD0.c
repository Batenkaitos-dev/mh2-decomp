/* The page's URL pointer. include/browser.h. */
#include "common.h"
#include "browser.h"

/* Browser_SetUrl: w->url = url (called with gBrowserWork). */
void Browser_SetUrl(BrowserWork *w, char *url)
{
    w->url = url;
}
