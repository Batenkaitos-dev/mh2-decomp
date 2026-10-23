/*
 * The page's objects and textures. The page objects (BrowserObj) come from
 * one pool of 512 in the work area (BrowserWork): PageObj_InitPool puts
 * them all on the free list, PageObj_Alloc takes one into an object list
 * (BLAYER_*) at its head or tail, and PageObj_Free gives it back.
 * PageObj_UpdateLayer runs the update function of each active object in a
 * list, PageObj_DrawLayer the draw function of each visible one. The page
 * textures fill PAGE_TEX_SLOTS slots (PageTex_Add, PageTex_Get,
 * PageTex_ReleaseAll); Browser_LoadFrame loads the frame's texture into
 * one, a step a frame.
 */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"
#include "nethttp.h"
#include "mem_util.h"
#include "packet_tim2.h"
#include "libc.h"

s32 flReleasePaletteHandle(u32);
/* unit's own declaration: declared with a u16 handle (the definition takes u32): so PageTex_ReleaseAll passes the handle's low 16 bits, as the game's code does */
s32 flReleaseTextureHandle(u16);

/* Empties the BLAYER_COUNT object lists and puts the 512 objects on the
 * free list. */
void PageObj_InitPool(void)
{
    s32 i;
    BrowserObj *pageObj;
    s32 n;

    for (i = 0; i < BLAYER_COUNT; i++) {
        gBrowserWork->layerHead[i] = NULL;
        gBrowserWork->layerTail[i] = NULL;
    }
    n = 512;
    gBrowserWork->unk1A640 = 0;
    gBrowserWork->unk1A644 = 0;
    gBrowserWork->freeObj = pageObj = gBrowserWork->objPool;
    while (n-- != 0) {
        memset(pageObj, 0, sizeof(BrowserObj));
        if (n != 0) {
            pageObj->next = pageObj + 1;
        }
        pageObj++;
    }
}

/* Takes one object from the free list; NULL when it is empty. */
static BrowserObj *PageObj_Take(void)
{
    BrowserObj *pageObj = gBrowserWork->freeObj;

    if (pageObj != NULL) {
        gBrowserWork->freeObj = pageObj->next;
    }
    return pageObj;
}

/* Clears an object and puts it back on the free list. */
static void PageObj_Give(BrowserObj *pageObj)
{
    memset(pageObj, 0, sizeof(BrowserObj));
    pageObj->next = gBrowserWork->freeObj;
    gBrowserWork->freeObj = pageObj;
}

/* Takes an object from the pool into list layer (BLAYER_*), at its head or
 * (atTail) its tail; NULL when the pool is empty. */
BrowserObj *PageObj_Alloc(s32 layer, s32 atTail)
{
    BrowserObj *cur;
    BrowserObj *pageObj = PageObj_Take();

    if (pageObj == NULL) {
        return NULL;
    }
    pageObj->layer = layer;
    if (atTail == 0) {
        pageObj->next = cur = gBrowserWork->layerHead[layer];
        gBrowserWork->layerHead[layer] = pageObj;
        pageObj->prev = NULL;
        if (cur == NULL) {
            gBrowserWork->layerTail[layer] = pageObj;
        } else {
            cur->prev = pageObj;
        }
    } else {
        pageObj->prev = cur = gBrowserWork->layerTail[layer];
        gBrowserWork->layerTail[layer] = pageObj;
        pageObj->next = NULL;
        if (cur == NULL) {
            gBrowserWork->layerHead[layer] = pageObj;
        } else {
            cur->next = pageObj;
        }
    }
    return pageObj;
}

/* Takes an object out of its list and puts it back in the pool. */
void PageObj_Free(BrowserObj *pageObj)
{
    BrowserObj *prev = pageObj->prev;

    if (prev == NULL) {
        BrowserObj *next;

        gBrowserWork->layerHead[pageObj->layer] = next = pageObj->next;
        if (next == NULL) {
            gBrowserWork->layerTail[pageObj->layer] = NULL;
        } else {
            next->prev = NULL;
        }
    } else if (pageObj->next == NULL) {
        gBrowserWork->layerTail[pageObj->layer] = prev;
        prev->next = NULL;
    } else {
        prev->next = pageObj->next;
        pageObj->next->prev = pageObj->prev;
    }
    PageObj_Give(pageObj);
}

/* Runs the update function of each active object in list layer. */
void PageObj_UpdateLayer(s32 layer)
{
    BrowserObj *pageObj = gBrowserWork->layerHead[layer];
    BrowserObj *next;

    if (pageObj != NULL) {
        do {
            next = pageObj->next;
            if (pageObj->active) {
                pageObj->update(pageObj);
            }
            pageObj = next;
        } while (next != NULL);
    }
}

/* Runs the draw function of each visible object in list layer. */
void PageObj_DrawLayer(s32 layer)
{
    BrowserObj *pageObj = gBrowserWork->layerHead[layer];
    BrowserObj *next;

    if (pageObj != NULL) {
        do {
            next = pageObj->next;
            if (pageObj->visible) {
                pageObj->draw(pageObj);
            }
            pageObj = next;
        } while (next != NULL);
    }
}

/* Releases the page's textures and their palettes, then compacts memory. */
void PageTex_ReleaseAll(void)
{
    s32 i;
    u32 *texEntry = gBrowserWork->tex;

    for (i = 0; i < gBrowserWork->nTex; i++, texEntry++) {
        if (*texEntry != 0) {
            u16 handle;

            if ((*texEntry & 0xFFFF0000) >> 16) {
                flReleasePaletteHandle((*texEntry & 0xFFFF0000) >> 16);
            }
            handle = *(u16 *)texEntry;
            if (handle) {
                flReleaseTextureHandle(handle);
            }
            *texEntry = 0;
        }
    }
    gBrowserWork->nTex = 0;
    Mem_Compact();
}

/* Adds tex to the page's textures; returns 0, or -1 when the PAGE_TEX_SLOTS
 * slots are full. */
s32 PageTex_Add(u32 tex)
{
    s8 n = gBrowserWork->nTex;

    if (n == PAGE_TEX_SLOTS) {
        return -1;
    }
    gBrowserWork->tex[n] = tex;
    gBrowserWork->nTex++;
    return 0;
}

/* Returns the page texture in slot i, or -1 for a slot not in use. */
u32 PageTex_Get(s8 i)
{
    if (i < 0 || gBrowserWork->nTex <= i) {
        return -1;
    }
    return gBrowserWork->tex[i];
}

/* Loads the frame's texture, a step a frame (*step 0 to 4), in mode 3;
 * other modes do nothing. */
void Browser_LoadFrame(s32 mode, u8 *step)
{
    switch (mode) {
    case 3:
        switch (*step) {
        case 0:
            Http_ReadFile(gLoadBuf, BRDATA_FRAME_TEX);
            (*step)++;
            break;
        case 1:
            PageTex_Add(Tex_CreateFromTim2(gLoadBuf, 0));
            (*step)++;
            break;
        case 2:
            *step = *step + 1;
            break;
        case 3:
            *step = *step + 1;
            break;
        case 4:
            break;
        }
        break;
    }
}

/* The URL schemes the browser knows and the script scheme it refuses. */
UrlScheme gUrlSchemes[7] = {
#include "lobby/007EEA40.inc"
};
char *gScriptSchemes[2] = {
    gScriptSchemes_0, NULL,
};

void PageObj_Nop(void)
{
}
