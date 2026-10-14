/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "browser.h"

void BrowserCache_Shutdown(void);
void Mem_Free(void *handle);
void Mem_Compact(void);
void Overlay_LoadGmSub(void);
s32 Http_Term(void);

void LobbyKddi_CloseBrowser(void) {
    BrowserCache_Shutdown();
    Http_Term();
    Mem_Free(gBrowserWork->mem512k);
    Mem_Free(gBrowserWork->mem64k);
    Mem_Compact();
    Overlay_LoadGmSub();
}
