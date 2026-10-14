/* The HTML tags' effects on the browser: a new page, <META REFRESH>, the
 * text colour, <BASE HREF>. include/browser.h. */
#include "common.h"
#include "browser.h"

void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
void Page_New(void);
void Url_SetBase(char *url);

/* Html_NewPage */
void Html_NewPage(void)
{
    Page_New();
}

/* Html_Refresh: <META HTTP-EQUIV=REFRESH CONTENT="sec; URL=...">: goes to
 * url after sec seconds (60 frames each; 0: 60 frames). */
void Html_Refresh(s32 sec, char *url)
{
    memset(gBrowserState->url, 0, 0x100);
    strcpy(gBrowserState->url, url);
    if (sec == 0) {
        gBrowserWork->refreshTimer = 60;
    } else {
        gBrowserWork->refreshTimer = sec * 60;
    }
}

/* Html_SetTextColor: the page's colour (unless the page is plain). */
void Html_SetTextColor(u32 rgb)
{
    if (gBrowserState->plain != 0) {
        gBrowserState->color = 0xFF000001;
        return;
    }
    gBrowserState->color = rgb | 0xFF000000;
}

/* Html_SetBase: <BASE HREF=...>. */
void Html_SetBase(char *url)
{
    memset(gBrowserState->url2, 0, 0x100);
    strcpy(gBrowserState->url2, url);
    Url_SetBase(gBrowserState->url2);
}
