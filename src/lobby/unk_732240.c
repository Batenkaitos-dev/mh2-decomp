/* Browser_Init: the browser's work area (0x1D2C0 bytes at 0x895480) and
 * its heaps; the top mode gBrowserTopMode 1 (the lobby's own pages) keeps the
 * page objects and textures. include/browser.h. */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void *Mem_Alloc(u32 size, s32 flag);
void *Mem_GetAddr(void *handle);
s32 NetOverlay_Request(s32); /* the lobby */
void Http_LoadAfsPartition(void);                 /* nethttp */
s32 Http_ReadBrdata(void *buf, s32 id);      /* nethttp */
s32 Http_Init(void);                  /* nethttp */
void BrowserCache_Init(void);
void Html_ResetLoad(void);
void PageTex_ReleaseAll(void);
void PageObj_InitPool(void);
void Font_ResetState(void);       /* sub_main */
void func_007349E0(void);
void Page_Margins(s32 a, s32 b); /* unit's own declaration: the definition reads none of the three it takes;
 * Browser_Init passes two (101 words off with a third) */

extern u8 gBrowserWorkMem[];                 /* the work area */
extern MmbbData gAccountData;               /* sub_main: the MMBB data */
extern u8 gRegRecord[];                 /* the registration record being filled (0x1D0 bytes) */
extern u8 gLobbyPageData[];                 /* the registration record kept */

void Browser_Init(u8 mode)
{
    u8 *p;
    u16 i;

    NetOverlay_Request(0);
    gBrowserWork = (BrowserWork *)gBrowserWorkMem;
    memset(gBrowserWork, 0, 0x1D2C0);
    Http_LoadAfsPartition();
    p = gMemAreas - 0x2800;
    memset(p, 0, 0x1000);
    gBrowserWork->cert0 = p;
    gBrowserWork->cert1 = p + 0x800;
    Http_ReadBrdata(gBrowserWork->cert0, 0x84);
    Http_ReadBrdata(gBrowserWork->cert1, 0x85);
    gBrowserWork->reqBusy = 0;
    (gKbd)[0x668] = 0;
    (gKbd)[0x669] = 0;
    gBrowserWork->dlgBoxReady = 0;
    gBrowserWork->mem64k = Mem_Alloc(0x10000, 1);
    gBrowserWork->buf64k = Mem_GetAddr(gBrowserWork->mem64k);
    memset(gBrowserWork->buf64k, 0, 0x10000);
    gBrowserWork->mem512k = Mem_Alloc(0x80000, 1);
    gBrowserWork->buf512k = Mem_GetAddr(gBrowserWork->mem512k);
    memset(gBrowserWork->buf512k, 0, 0x80000);
    gBrowserWork->endGoOn = 0;
    if (gBrowserTopMode == 1) {
        BrowserCache_Init();
        Html_ResetLoad();
        Http_Init();
        memset(&gBrowserWork->state, 0, sizeof(BrowserState));
        gBrowserWork->tries = 0;
        gBrowserWork->endReason = 0;
        gBrowserWork->netError = 0;
        gMmbbData = &gAccountData;
        gBrowserState = &gBrowserWork->state;
        memset(&gBrowserWork->sub, 0, sizeof(BrowserSub));
        gBrowserSub = &gBrowserWork->sub;
        return;
    }
    BrowserCache_Init();
    Html_ResetLoad();
    PageTex_ReleaseAll();
    PageObj_InitPool();
    Http_Init();
    memset(&gBrowserWork->state, 0, sizeof(BrowserState));
    if (gPortalPage == 0) {
        memset(&gAccountData, 0, sizeof(MmbbData));
    }
    memset(&gBrowserWork->sub, 0, sizeof(BrowserSub));
    gMmbbData = &gAccountData;
    gBrowserState = &gBrowserWork->state;
    gBrowserSub = &gBrowserWork->sub;
    Font_ResetState();
    func_007349E0();
    for (i = 0; i < 500; i++) {
        gBrowserWork->obj[i] = NULL;
    }
    gBrowserWork->tries = 0;
    gBrowserWork->texFailOnce = 1;
    gBrowserWork->startPosts = 0;
    gBrowserWork->firstPage = 1;
    gBrowserState->style = 0x2E7;
    if (mode == 1) {
        gBrowserState->initMode = 1;
        gBrowserState->color = 0xFF000001;
    } else {
        gBrowserState->initMode = 0;
        gBrowserState->color = 0xFF000001;
    }
    Page_Margins(0, 0);
    gBrowserWork->loginPosted = 0;
    gBrowserWork->gameMenuNo = 0;
    gBrowserWork->focus = 0;
    gBrowserWork->endReason = 0;
    gBrowserWork->netError = 0;
    gBrowserWork->formGoLink = 0;
    gBrowserWork->pageData = 0;
    gBrowserWork->texCount = 0;
    gBrowserWork->freedObjs = 0;
    memset(gBrowserWork->texHandle, 0, 0x50);
    memset(gBrowserWork->texW, 0, 0x28);
    memset(gBrowserWork->texH, 0, 0x28);
    memset(gBrowserWork->cmdBuf, 0, 0x8000);
    gBrowserWork->cmdPtr = gBrowserWork->cmdBuf;
    gBrowserWork->result = 0;
    memcpy(gRegRecord, gLobbyPageData, 0x1D0);
    gBrowserWork->exitArgs = 0;
    gBrowserWork->memPage[0] = (u8 *)gBrowserWork + 0x1BA70;
    gBrowserWork->memPage[1] = 0;
    gBrowserWork->memPage[2] = 0;
}
