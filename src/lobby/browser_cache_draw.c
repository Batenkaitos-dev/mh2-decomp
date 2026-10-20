/*
 * The lobby browser's loading side and its page drawing.
 *
 * The first half is built on name lists: chains of BrowserNode, each node
 * holding a URL (name), its status (enum BrowserNodeStatus) and a tag (BrowserNodeFlags:
 * the load's serial and its flags), with the free nodes at the end of the
 * chain. BrowserCache_Init sets up four lists in the work area
 * (BrowserWork): the page cache (3 BrowserPageNode, a 0x8000-byte buffer
 * each), the image cache (32 BrowserImgNode with their textures), the
 * history (32 URLs; histCur is the page on show, the newest at the front)
 * and the request queue (20 BrowserReqNode). BrowserReq_Page, _Post,
 * _Image and the history moves queue loads; BrowserReq_UpdateAll starts
 * them on nethttp's HttpReq slots and steps them each frame;
 * BrowserReq_Poll hands a finished load to its caller as a BrowserResult.
 * BrowserForm_Add builds a form's query for a post.
 *
 * The second half draws the page: clipped rectangles, triangles and lines,
 * frames and tiled windows, text lines, the page objects (BrowserObj:
 * links, form fields, images, the open drop-down list, the focus; their
 * kinds are enum PageObjKind), the browser's widget sprites (enum
 * PageWidget), the dialog boxes and the overlay over the whole screen.
 *
 * Browser_Init and LobbyKddi_CloseBrowser set up and free the lists,
 * Browser_Update runs the queue, the portal steps, Page_Load and
 * Page_CheckImages make and poll the requests, and the HTML parser builds
 * the page objects drawn here.
 *
 * The functions share one file because they share static helpers. The
 * compiler keeps a value in a register (a1) across a call
 * only when the called function is static in the same file:
 * BrowserHist_CanForward and BrowserHist_CanBack rely on it around
 * NameList_Prev and NameList_NextUsed, and BrowserReq_Reload,
 * BrowserForm_Add and ImgCache_MakeTexture around BrowserHist_CurFlags.
 */
#include "common.h"
#include "net_mode.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "nethttp.h"
s32 Rect_ClipToScreen(f32 *, f32 *, f32 *, f32 *);

void Prim_DrawRect(s32, f32, f32, f32, f32);

#include "ee_stdarg.h"
#include "browser.h"
#include "window.h"



void NameList_Release(BrowserNode *list, BrowserNode *node);
void *memset(void *dst, s32 c, u32 n);
void Url_Copy(char *dst, char *src);
void *memcpy(void *dst, void *src, u32 n);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);

/*
 * Takes the first free node of a name list without moving it, copies the
 * URL name into it and marks it taken (BNODE_TAKEN). Returns the node, or NULL
 * when every node is in use. The request queue's makers (BrowserReq_Page,
 * BrowserReq_Post, BrowserReq_Image, the history moves) use it.
 */
BrowserNode *NameList_Take(BrowserNode *list, char *name)
{
    BrowserNode *node;

    for (;;) {
        node = list->next;
        if (node == NULL) {
            return NULL;
        }
        list = node;
        if (node->status == BNODE_FREE) {
            break;
        }
    }
    Url_Copy(node->name, name);
    node->status = BNODE_TAKEN;
    return node;
}

/*
 * Frees a node of a name list: moves it to the end of the chain (when it
 * is not there already) and clears its name, status and tag. Does nothing
 * when the node is not in the list.
 */
void NameList_Release(BrowserNode *list, BrowserNode *node)
{
    BrowserNode *prev = NULL;
    BrowserNode *nextNode;

    for (;;) {
        nextNode = list->next;
        if (nextNode == NULL) {
            break;
        }
        if (nextNode == node) {
            prev = list;
        }
        list = nextNode;
    }
    if (prev != NULL) {
        if (node->next != NULL) {
            prev->next = node->next;
            list->next = node;
            node->next = NULL;
        }
        memset(node->name, 0, sizeof(node->name));
        node->status = BNODE_FREE;
        memset(&node->tag, 0, 2);
    }
}

/*
 * Frees a request node: returns it to the end of the request queue
 * (NameList_Release) and clears its kind, type, referer, cache node,
 * buffer, length and result. BrowserReq_Poll calls it once it has handed
 * a load over.
 */
void BrowserReq_Free(BrowserReqNode *reqNode)
{
    NameList_Release(&gBrowserWork->reqList, &reqNode->node);
    reqNode->loadKind = 0;
    reqNode->type = 0;
    reqNode->referer = NULL;
    reqNode->cacheNode = 0;
    reqNode->buf = NULL;
    reqNode->len = 0;
    reqNode->errStep = 0;
    reqNode->result = BRES_OK;
}

/*
 * Takes the last node of a name list (a free one, or else the oldest),
 * moves it to the front, copies the URL name into it and marks it taken.
 * Returns the node. BrowserHist_Commit uses it to add a page to the
 * history.
 */
BrowserNode *NameList_ReuseLast(BrowserNode *list, char *name)
{
    BrowserNode *prev = NULL;
    BrowserNode *last = list;
    BrowserNode *nextNode;

    for (;;) {
        nextNode = last->next;
        if (nextNode == NULL) {
            break;
        }
        prev = last;
        last = nextNode;
    }
    last->next = list->next;
    list->next = last;
    prev->next = NULL;
    Url_Copy(last->name, name);
    last->status = BNODE_TAKEN;
    return last;
}

/*
 * Returns the node before node in the list (in the history: the next
 * newer page), or node itself when it is the first. The node must be in
 * the list: the walk does not stop otherwise.
 */
static BrowserNode *NameList_Prev(BrowserNode *list, BrowserNode *node)
{
    if (list->next == node) {
        return node;
    }
    for (;;) {
        if (list->next == node) {
            return list;
        }
        list = list->next;
    }
}

/*
 * Returns the node after node when there is one and it is in use (in the
 * history: the next older page), else node itself.
 */
static BrowserNode *NameList_NextUsed(BrowserNode *list, BrowserNode *node)
{
    BrowserNode *nextNode = node->next;

    if (nextNode == NULL) {
        return node;
    }
    return nextNode->status ? nextNode : node;
}

/*
 * Frees the first node of a name list: moves it to the end of the chain
 * and clears its name, status and tag. Returns cur, or the new first node
 * when cur was the one freed. The empty test inside the walk compiles to
 * nothing; it stays because the game's code has it.
 */
BrowserNode *NameList_DropFirst(BrowserNode *list, BrowserNode *cur)
{
    BrowserNode *last = list;
    BrowserNode *nextNode;
    BrowserNode *first;

    for (;;) {
        nextNode = last->next;
        if (nextNode == NULL) {
            if (list == NULL || list->next == NULL) {} /* an empty test the game's code also has: it compiles to nothing */
            break;
        }
        last = nextNode;
    }
    first = list->next;
    list->next = first->next;
    last->next = first;
    first->next = NULL;
    memset(first->name, 0, sizeof(first->name));
    first->status = BNODE_FREE;
    memset(&first->tag, 0, 2);
    if (cur == first) {
        cur = list->next;
    }
    return cur;
}

/* History moves and the nodes' tags. */

BrowserNode *NameList_DropFirst(BrowserNode *list, BrowserNode *cur);

/*
 * Frees every node in front of cur (in the history: the pages newer than
 * cur, the forward history) with NameList_DropFirst, and returns cur. The
 * goto loop is the form whose code matches the game's.
 */
BrowserNode *NameList_DropBefore(BrowserNode *list, BrowserNode *cur)
{
loop:
    if (list->next != cur) {
        cur = NameList_DropFirst(list, cur);
        goto loop;
    }
    return cur;
}

/*
 * Moves the history for a request that has finished (or failed in a way
 * that still counts): forward (BLOAD_FORWARD) and back (BLOAD_BACK) move histCur
 * one page, a reload (BLOAD_RELOAD) gives histCur the request's new tag, and any
 * other load drops the forward history and adds name as the new current
 * page with the request's tag. list is the history list.
 */
void BrowserHist_Commit(BrowserNode *list, BrowserReqNode *reqNode, char *name)
{
    switch (reqNode->loadKind) {
    case BLOAD_FORWARD:
        gBrowserWork->histCur = NameList_Prev(list, gBrowserWork->histCur);
        break;
    case BLOAD_BACK:
        gBrowserWork->histCur = NameList_NextUsed(list, gBrowserWork->histCur);
        break;
    case BLOAD_RELOAD:
        gBrowserWork->histCur->tag = reqNode->node.tag;
        break;
    default:
        gBrowserWork->histCur = NameList_DropBefore(list, gBrowserWork->histCur);
        gBrowserWork->histCur = NameList_ReuseLast(list, name);
        gBrowserWork->histCur->tag = reqNode->node.tag;
        break;
    }
}

/* Sets bit in a node's tag flags when on is non-zero, else clears it. */
static void BrowserTag_SetFlag(BrowserNodeFlags *tag, u8 bit, s32 on)
{
    if (on) {
        tag->flags |= bit;
    } else {
        tag->flags &= ~bit;
    }
}

/*
 * Returns the tag's flags masked by flag (non-zero when one of those bits
 * is set).
 */
static s32 BrowserTag_TestFlag(BrowserNodeFlags *tag, s32 flag)
{
    return tag->flags & (flag & 0xFFFF) & 0xFF;
}

/* The caches and the history. */

s32 Url_Compare(char *a, char *b);
void ImgCache_FreeAll(BrowserNode *list);
s32 Tex_ReleaseKeep(u32);
s32 Pal_ReleaseKeep(u32);
void Mem_Free(void *handle);

/*
 * Looks up a cache list (pages or images) for the node in use whose URL
 * matches name (Url_Compare) and moves it to the front, so that the front
 * is the most recently used. Returns the node, or NULL when none matches.
 */
BrowserNode *NameCache_Find(BrowserNode *list, char *name)
{
    BrowserNode *node;
    BrowserNode *prev;
    BrowserNode *found;
    BrowserNode *foundPrev;
    BrowserNode *nextNode;

    node = list;
    found = NULL;
    foundPrev = NULL;
    for (;;) {
        nextNode = node->next;
        if (nextNode == NULL) {
            break;
        }
        prev = node;
        node = nextNode;
        if (nextNode->status == BNODE_FREE) {
            continue;
        }
        if (Url_Compare(nextNode->name, name) != 0) {
            continue;
        }
        found = nextNode;
        foundPrev = prev;
        break;
    }
    if (found != NULL) {
        if (found != list->next) {
            foundPrev->next = found->next;
            found->next = list->next;
            list->next = found;
        }
        return found;
    }
    return NULL;
}

/*
 * Returns the last node in use of a cache list (the least recently used),
 * or NULL when the list holds nothing. The page and image caches free it
 * to make room.
 */
BrowserNode *NameCache_Oldest(BrowserNode *list)
{
    BrowserNode *oldest = NULL;

    if (list->next->status == BNODE_FREE) {
        return NULL;
    }
    for (;;) {
        list = list->next;
        if (list == NULL) {
            break;
        }
        if (list->status != BNODE_FREE) {
            oldest = list;
        }
    }
    return oldest;
}

/*
 * Frees a node of a cache list: moves it to the end of the chain and
 * clears its name, status and tag, as NameList_Release does for the other
 * lists. Does nothing when the node is not in the list.
 */
void NameCache_Release(BrowserNode *list, BrowserNode *node)
{
    BrowserNode *prev = NULL;
    BrowserNode *nextNode;

    for (;;) {
        nextNode = list->next;
        if (nextNode == NULL) {
            break;
        }
        if (nextNode == node) {
            prev = list;
        }
        list = nextNode;
    }
    if (prev != NULL) {
        if (node->next != NULL) {
            prev->next = node->next;
            list->next = node;
            node->next = NULL;
        }
        memset(node->name, 0, sizeof(node->name));
        node->status = BNODE_FREE;
        memset(&node->tag, 0, 2);
    }
}

/*
 * Takes the first free node of a cache list, moves it to the front, copies
 * the URL name into it and marks it taken (BNODE_TAKEN). Returns the node, or
 * NULL when every node is in use.
 */
BrowserNode *NameCache_Take(BrowserNode *list, char *name)
{
    BrowserNode *node = list;
    BrowserNode *candidate;
    BrowserNode *prev;

    for (;;) {
        candidate = node->next;
        if (candidate == NULL) {
            return NULL;
        }
        prev = node;
        node = candidate;
        if (candidate->status == BNODE_FREE) {
            break;
        }
    }
    if (prev != list) {
        prev->next = candidate->next;
        candidate->next = list->next;
        list->next = candidate;
    }
    Url_Copy(candidate->name, name);
    candidate->status = BNODE_TAKEN;
    return candidate;
}

/*
 * Frees a page of the page cache: releases its node, clears its
 * 0x8000-byte buffer and its length.
 */
void PageCache_Free(BrowserPageNode *page)
{
    NameCache_Release(&gBrowserWork->pageList, &page->node);
    memset(page->buf, 0, PAGE_BUF_SIZE);
    page->size = 0;
}

/*
 * Frees an image of the image cache: releases its node, then the texture
 * (the low half of handles, Tex_ReleaseKeep) and its palette (the high
 * half, Pal_ReleaseKeep) when it has them, and clears the data, size and
 * image size.
 */
void ImgCache_Free(BrowserImgNode *img)
{
    NameCache_Release(&gBrowserWork->imgList, &img->node);
    if (img->handles != 0) {
        Tex_ReleaseKeep(img->handles & 0xFFFF);
        if ((img->handles & 0xFFFF0000) >> 16 != 0) {
            Pal_ReleaseKeep((img->handles & 0xFFFF0000) >> 16);
        }
    }
    img->handles = 0;
    img->data = 0;
    img->size = 0;
    img->w = 0;
    img->h = 0;
}

void *Mem_Alloc(u32 size, s32 flag);
void *Mem_GetAddr(void *handle);
void Url_ClearBase(void);
void BrowserForm_Clear(void);


/*
 * Sets up the browser's lists (Browser_Init calls it): allocates and links
 * the 3 page-cache nodes with their 0x8000-byte buffers, the 32 image
 * nodes, the eight 0x4000-byte connection buffers (one per HttpReq slot),
 * the 32 history nodes (histCur at the first) and the 20 request nodes;
 * clears the download node, the last navigation, the result and the load
 * serial, then the base URL (Url_ClearBase) and the form's query
 * (BrowserForm_Clear).
 */
void BrowserCache_Init(void)
{
    s32 i;
    u8 *bufBlock;
    BrowserPageNode *pageNodes;
    BrowserImgNode *imgNodes;
    BrowserNode *histNodes;
    BrowserReqNode *reqNodes;

    gBrowserWork->memPages = Mem_Alloc(PAGE_CACHE_COUNT * sizeof(BrowserPageNode), 1);
    gBrowserWork->pages = (BrowserPageNode *)Mem_GetAddr(gBrowserWork->memPages);
    memset(gBrowserWork->pages, 0, PAGE_CACHE_COUNT * sizeof(BrowserPageNode));
    gBrowserWork->pageList.next = &gBrowserWork->pages->node;
    pageNodes = gBrowserWork->pages;
    pageNodes[0].node.next = &pageNodes[1].node;
    pageNodes = gBrowserWork->pages;
    pageNodes[1].node.next = &pageNodes[2].node;
    gBrowserWork->memBufs = Mem_Alloc(PAGE_CACHE_COUNT * PAGE_BUF_SIZE, 1);
    bufBlock = Mem_GetAddr(gBrowserWork->memBufs);
    gBrowserWork->pages[0].buf = bufBlock;
    gBrowserWork->pages[1].buf = bufBlock + PAGE_BUF_SIZE;
    gBrowserWork->pages[2].buf = bufBlock + 2 * PAGE_BUF_SIZE;

    gBrowserWork->memImgs = Mem_Alloc(IMG_CACHE_COUNT * sizeof(BrowserImgNode), 1);
    gBrowserWork->imgs = (BrowserImgNode *)Mem_GetAddr(gBrowserWork->memImgs);
    memset(gBrowserWork->imgs, 0, IMG_CACHE_COUNT * sizeof(BrowserImgNode));
    gBrowserWork->imgList.next = &gBrowserWork->imgs->node;
    for (i = 0; i < IMG_CACHE_COUNT - 1; i++) {
        imgNodes = gBrowserWork->imgs;
        imgNodes[i].node.next = &imgNodes[i + 1].node;
    }

    gBrowserWork->memConnBufs = Mem_Alloc(HTTP_REQ_SLOTS * CONN_BUF_SIZE, 1);
    bufBlock = Mem_GetAddr(gBrowserWork->memConnBufs);
    gBrowserWork->connBuf[0] = bufBlock;
    gBrowserWork->connBuf[1] = bufBlock + CONN_BUF_SIZE;
    gBrowserWork->connBuf[2] = bufBlock + 2 * CONN_BUF_SIZE;
    gBrowserWork->connBuf[3] = bufBlock + 3 * CONN_BUF_SIZE;
    gBrowserWork->connBuf[4] = bufBlock + 4 * CONN_BUF_SIZE;
    gBrowserWork->connBuf[5] = bufBlock + 5 * CONN_BUF_SIZE;
    gBrowserWork->connBuf[6] = bufBlock + 6 * CONN_BUF_SIZE;
    gBrowserWork->connBuf[7] = bufBlock + 7 * CONN_BUF_SIZE;
    memset(&gBrowserWork->download, 0, sizeof(gBrowserWork->download));

    gBrowserWork->memHist = Mem_Alloc(HIST_COUNT * sizeof(BrowserNode), 1);
    gBrowserWork->hist = (BrowserNode *)Mem_GetAddr(gBrowserWork->memHist);
    memset(gBrowserWork->hist, 0, HIST_COUNT * sizeof(BrowserNode));
    gBrowserWork->histList.next = gBrowserWork->hist;
    gBrowserWork->histCur = gBrowserWork->hist;
    for (i = 0; i < HIST_COUNT - 1; i++) {
        histNodes = gBrowserWork->hist;
        histNodes[i].next = &histNodes[i + 1];
    }

    gBrowserWork->memReqs = Mem_Alloc(REQ_COUNT * sizeof(BrowserReqNode), 1);
    gBrowserWork->reqs = (BrowserReqNode *)Mem_GetAddr(gBrowserWork->memReqs);
    memset(gBrowserWork->reqs, 0, REQ_COUNT * sizeof(BrowserReqNode));
    gBrowserWork->reqList.next = &gBrowserWork->reqs->node;
    for (i = 0; i < REQ_COUNT - 1; i++) {
        reqNodes = gBrowserWork->reqs;
        reqNodes[i].node.next = &reqNodes[i + 1].node;
    }
    memset(&gBrowserWork->lastNav, 0, 0x18); /* lastNav, reqResult and loadSerial */
    Url_ClearBase();
    BrowserForm_Clear();
}

/*
 * Frees what BrowserCache_Init allocated: the textures of every cached
 * image (ImgCache_FreeAll), then the six memory blocks.
 * LobbyKddi_CloseBrowser calls it.
 */
void BrowserCache_Shutdown(void)
{
    ImgCache_FreeAll(&gBrowserWork->imgList);
    Mem_Free(gBrowserWork->memPages);
    Mem_Free(gBrowserWork->memBufs);
    Mem_Free(gBrowserWork->memImgs);
    Mem_Free(gBrowserWork->memConnBufs);
    Mem_Free(gBrowserWork->memHist);
    Mem_Free(gBrowserWork->memReqs);
}

/*
 * Queues the load of the next newer page of the history (Browser_GoForward
 * calls it) with that page's tag, and notes the navigation as forward (3).
 * When there is no newer page it queues a reload of the current one
 * instead (BLOAD_RELOAD, BNAV_RELOAD). The request's referer is the current
 * page. Returns the URL to load.
 */
char *BrowserHist_Forward(void)
{
    BrowserNode *dest;
    BrowserReqNode *reqNode;

    dest = NameList_Prev(&gBrowserWork->histList, gBrowserWork->histCur);
    reqNode = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, dest->name);
    reqNode->node.status = BNODE_QUEUED;
    if (dest == gBrowserWork->histCur) {
        reqNode->loadKind = BLOAD_RELOAD;
        reqNode->type = BREQ_PAGE;
        reqNode->referer = gBrowserWork->histCur->name;
        reqNode->node.tag = dest->tag;
        gBrowserWork->lastNav = BNAV_RELOAD;
    } else {
        reqNode->loadKind = BLOAD_FORWARD;
        reqNode->type = BREQ_PAGE;
        reqNode->referer = gBrowserWork->histCur->name;
        reqNode->node.tag = dest->tag;
        gBrowserWork->lastNav = BNAV_FORWARD;
    }
    return dest->name;
}

/*
 * Returns 1 when the history has a page newer than the current one, else
 * 0.
 */
s32 BrowserHist_CanForward(void)
{
    BrowserNode *cur = gBrowserWork->histCur;

    return cur != NameList_Prev(&gBrowserWork->histList, cur);
}

/*
 * Queues the load of the next older page of the history (Browser_GoBack
 * calls it) with that page's tag, and notes the navigation as back (4).
 * When there is no older page it queues a reload of the current one
 * instead (BLOAD_RELOAD, BNAV_RELOAD). Returns the URL to load.
 */
char *BrowserHist_Back(void)
{
    BrowserNode *dest;
    BrowserReqNode *reqNode;

    dest = NameList_NextUsed(&gBrowserWork->histList, gBrowserWork->histCur);
    reqNode = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, dest->name);
    reqNode->node.status = BNODE_QUEUED;
    if (dest == gBrowserWork->histCur) {
        reqNode->loadKind = BLOAD_RELOAD;
        reqNode->type = BREQ_PAGE;
        reqNode->referer = gBrowserWork->histCur->name;
        reqNode->node.tag = dest->tag;
        gBrowserWork->lastNav = BNAV_RELOAD;
    } else {
        reqNode->loadKind = BLOAD_BACK;
        reqNode->type = BREQ_PAGE;
        reqNode->referer = dest->name;
        reqNode->node.tag = dest->tag;
        gBrowserWork->lastNav = BNAV_BACK;
    }
    return dest->name;
}

/*
 * Returns 1 when the history has a page older than the current one, else
 * 0.
 */
s32 BrowserHist_CanBack(void)
{
    BrowserNode *cur = gBrowserWork->histCur;

    return cur != NameList_NextUsed(&gBrowserWork->histList, cur);
}

/* The current page. */

/*
 * Returns the tag of the current history page. It is static in the
 * game's code, which lets its callers keep a value in a register across
 * the call.
 */
static BrowserNodeFlags *BrowserHist_CurFlags(void)
{
    return &gBrowserWork->histCur->tag;
}

/*
 * Queues a reload of the current history page (BLOAD_RELOAD; Portal_Back calls
 * it) with a new load serial and the page's flags, and notes the
 * navigation as a reload (5). Returns the page's URL.
 */
char *BrowserReq_Reload(void)
{
    BrowserReqNode *reqNode;

    reqNode = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, gBrowserWork->histCur->name);
    reqNode->node.status = BNODE_QUEUED;
    reqNode->loadKind = BLOAD_RELOAD;
    reqNode->type = BREQ_PAGE;
    reqNode->referer = gBrowserWork->histCur->name;
    reqNode->node.tag.serial = ++gBrowserWork->loadSerial;
    reqNode->node.tag.flags = BrowserHist_CurFlags()->flags;
    gBrowserWork->lastNav = BNAV_RELOAD;
    return gBrowserWork->histCur->name;
}

/*
 * Returns 0 when the current page came from a form post (BTAG_POSTED), as
 * a post is not sent again; else 1. It also tests tag flag 2 and drops the
 * answer, as the game's code does.
 */
s32 BrowserReq_CanReload(void) {
    if (BrowserTag_TestFlag(BrowserHist_CurFlags(), 1) != 0) {
        return 0;
    }
    BrowserTag_TestFlag(BrowserHist_CurFlags(), 2);
    return 1;
}

/* Returns the URL of the page on show (the current history node's name). */
char *BrowserHist_CurUrl(void)
{
    return gBrowserWork->histCur->name;
}

/* Requests, forms and the queue; then the page draws. */

s32 Url_FileType(char *name);
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap);
void Font_Printf(const char *fmt, ...);
void Prim_DrawTri(u32 color, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2);
void Prim_DrawSprite(u32 color, u32 tex, s16 x1, s16 y1, s16 x2, s16 y2, s16 u1, s16 v1, s16 u2, s16 v2);

/*
 * Queues the load of a page (BLOAD_PAGE) with a new load serial and no flags,
 * the current page as referer, and notes the navigation as a page load
 * (1). url is the address of the URL pointer (&gBrowserWork->url);
 * Portal_PageStart calls it.
 */
void BrowserReq_Page(char **url)
{
    BrowserReqNode *reqNode;

    reqNode = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, *url);
    reqNode->node.status = BNODE_QUEUED;
    reqNode->loadKind = BLOAD_PAGE;
    reqNode->type = BREQ_PAGE;
    reqNode->referer = BrowserHist_CurUrl();
    reqNode->node.tag.serial = ++gBrowserWork->loadSerial;
    reqNode->node.tag.flags = 0;
    gBrowserWork->lastNav = BNAV_PAGE;
}

/*
 * Queues a form post (BLOAD_POST: sent with the form's query, formData) with
 * a new load serial and BTAG_POSTED, so that the page is not fetched again
 * from the history; notes the navigation as a post (2). url is the
 * address of the URL pointer. Form_Submit, Link_Follow and the portal
 * steps call it.
 */
void BrowserReq_Post(char **url)
{
    BrowserReqNode *reqNode;

    reqNode = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, *url);
    reqNode->node.status = BNODE_QUEUED;
    reqNode->loadKind = BLOAD_POST;
    reqNode->type = BREQ_PAGE;
    reqNode->referer = BrowserHist_CurUrl();
    reqNode->node.tag.serial = gBrowserWork->loadSerial += 1;
    reqNode->node.tag.flags = 0;
    BrowserTag_SetFlag(&reqNode->node.tag, 1, 1);
    gBrowserWork->lastNav = BNAV_POST;
}

s32 strlen(const char *s);
char *strncpy(char *dst, const char *src, u32 n);
void Str_SjisToEuc(u8 *s);
s32 Url_Escape(char *dst, u8 *src);

/*
 * Appends name=value to the form's query (formData), after an ampersand
 * when the query is not empty. Unless the current page is Shift-JIS
 * (BTAG_SJIS), the name and value are first converted to EUC
 * (Str_SjisToEuc); the value is URL-escaped (Url_Escape). Returns the
 * query's new length, or -1 when the query would pass 0xFFF bytes or the
 * name or value is over 0xFF bytes.
 */
s32 BrowserForm_Add(char *name, char *value)
{
    char converted[0x100];
    s32 queryLen = strlen(gBrowserWork->formData);
    s32 nameLen = strlen(name);
    s32 valueLen = strlen(value);

    if (queryLen + nameLen + valueLen + 2 > (s32)sizeof(gBrowserWork->formData) - 1) {
        return -1;
    }
    if ((u32)valueLen > 0xFF || (u32)nameLen >= 0x100) {
        return -1;
    }
    if (queryLen != 0) {
        gBrowserWork->formData[queryLen] = 0x26;
        queryLen++;
    }
    if (BrowserTag_TestFlag(BrowserHist_CurFlags(), BTAG_SJIS) == 0) {
        memset(converted, 0, sizeof(converted));
        strncpy(converted, name, sizeof(converted));
        Str_SjisToEuc((u8 *)converted);
        nameLen = strlen(converted);
        memcpy(gBrowserWork->formData + queryLen, converted, nameLen);
        queryLen += nameLen;
        gBrowserWork->formData[queryLen] = 0x3D;
        queryLen++;
        memset(converted, 0, sizeof(converted));
        strncpy(converted, value, sizeof(converted));
        Str_SjisToEuc((u8 *)converted);
        queryLen += Url_Escape((gBrowserWork->formData + queryLen), (u8 *)converted);
    } else {
        memcpy(gBrowserWork->formData + queryLen, name, nameLen);
        queryLen += nameLen;
        gBrowserWork->formData[queryLen] = 0x3D;
        queryLen++;
        queryLen += Url_Escape((gBrowserWork->formData + queryLen), (u8 *)value);
    }
    return queryLen;
}

/* Empties the form's query (formData). */
void BrowserForm_Clear(void)
{
    memset(gBrowserWork->formData, 0, sizeof(gBrowserWork->formData));
}

/*
 * Queues the load of an image of the page (Page_RequestImages calls it)
 * with the current page's tag and the current page as referer. When the
 * URL is not of a known file type the request fails at once (BRES_FAILED).
 * url is the address of a pointer to the image's URL.
 */
void BrowserReq_Image(char **url)
{
    BrowserReqNode *reqNode;

    reqNode = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, *url);
    reqNode->node.status = BNODE_QUEUED;
    reqNode->loadKind = BLOAD_PAGE;
    reqNode->type = BREQ_IMAGE;
    reqNode->referer = BrowserHist_CurUrl();
    reqNode->node.tag = *BrowserHist_CurFlags();
    if ((u8)Url_FileType(*url) == URL_TYPE_NONE) {
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_FAILED;
        reqNode->errStep = 0;
        return;
    }
}

/* Url_GetScheme's callers here switch on its low byte (a u8 cast), as the game's code does. */
s32 Url_GetScheme(const char *url);

/*
 * Finds or makes the page-cache node for url. Returns it marked BNODE_CACHED when
 * the cached page can be used as it is, BNODE_LOADING when it is to be fetched (a new
 * node, the oldest page freed when the cache is full), or NULL when a
 * back or forward move finds a posted page no longer cached, or a reload
 * asks for a posted page. kind is the request's kind (a memory:// page is
 * always fetched again); reqTag is the request's tag.
 */
BrowserPageNode *PageCache_Get(char *url, s32 loadKind, BrowserNodeFlags *reqTag)
{
    BrowserPageNode *page;

    switch ((u8)Url_GetScheme(url)) {
    case URL_MEMORY:
        if (loadKind == BLOAD_PAGE) {
            loadKind = BLOAD_REFETCH;
        }
        break;
    }
    page = (BrowserPageNode *)NameCache_Find(&gBrowserWork->pageList, url);
    switch (loadKind) {
    case BLOAD_FORWARD:
    case BLOAD_BACK:
        if (page == NULL && BrowserTag_TestFlag(reqTag, BTAG_POSTED)) {
            return NULL;
        }
    case BLOAD_PAGE:
        if (page != NULL) {
            page->node.status = BNODE_CACHED;
            return page;
        }
        goto make;
    case BLOAD_RELOAD:
        if (BrowserTag_TestFlag(reqTag, BTAG_POSTED)) {
            return NULL;
        }
    case 0:
    case BLOAD_REFETCH:
    case BLOAD_POST:
    default:
    make:
        if (page == NULL) {
            page = (BrowserPageNode *)NameCache_Take(&gBrowserWork->pageList, url);
            if (page == NULL) {
                PageCache_Free((BrowserPageNode *)NameCache_Oldest(&gBrowserWork->pageList));
                page = (BrowserPageNode *)NameCache_Take(&gBrowserWork->pageList, url);
            }
        }
        page->node.status = BNODE_LOADING;
        return page;
    }
}

/*
 * Finds or makes the image-cache node for name. Returns a loaded image
 * marked BNODE_CACHED; NULL while another request is still loading it, or when a
 * reload finds it from an older load (it is freed, and the next try makes
 * a new node); else a new node marked BNODE_LOADING, the oldest image freed when the
 * cache is full. reqTag is the request's tag; the second parameter is
 * unused.
 */
BrowserNode *ImgCache_Get(char *name, s32 unused, BrowserNodeFlags *reqTag)
{
    BrowserNode *img;

    img = NameCache_Find(&gBrowserWork->imgList, name);
    if (img != NULL) {
        if (img->status == BNODE_DONE) {
            if (gBrowserWork->lastNav == BNAV_RELOAD && img->tag.serial != reqTag->serial) {
                ImgCache_Free((BrowserImgNode *)img);
                return NULL;
            }
            img->status = BNODE_CACHED;
            return img;
        }
        return NULL;
    }
    if (img == NULL) {
        img = NameCache_Take(&gBrowserWork->imgList, name);
        if (img == NULL) {
            ImgCache_Free((BrowserImgNode *)NameCache_Oldest(&gBrowserWork->imgList));
            img = NameCache_Take(&gBrowserWork->imgList, name);
        }
    }
    img->status = BNODE_LOADING;
    return img;
}

/* Frees every image of the image cache list, oldest first. */
void ImgCache_FreeAll(BrowserNode *list)
{
    BrowserNode *oldest;

    oldest = NameCache_Oldest(list);
    if (oldest != NULL) {
        do {
            ImgCache_Free((BrowserImgNode *)oldest);
            oldest = NameCache_Oldest(list);
        } while (oldest != NULL);
    }
}

s32 Png_CreateTexture(u8 *data, s32 size, s32 mode, u16 *w, u16 *h);
s32 Bmp_CreateTexture(u8 *bmp, s32 mode, u16 *w, u16 *h);

/*
 * Makes the texture of a loaded image by its file type (Png_CreateTexture
 * or Bmp_CreateTexture) and keeps the handles. When no texture is free it
 * frees the oldest cached image and tries again, unless that image
 * belongs to the page on show. Returns 0, or BRES_IMAGE_FAILED when the image fails
 * (marked BNODE_FAILED, no handles).
 */
s32 ImgCache_MakeTexture(BrowserImgNode *img)
{
    s32 texResult;
    BrowserNode *oldest;

retry:
    texResult = -3;
    switch ((u8)Url_FileType(img->node.name)) {
    case URL_TYPE_PNG:
        texResult = Png_CreateTexture((u8 *)img->data, img->size, 3, (u16 *)&img->w, (u16 *)&img->h);
        break;
    case URL_TYPE_BMP:
        texResult = Bmp_CreateTexture((u8 *)img->data, 3, (u16 *)&img->w, (u16 *)&img->h);
        break;
    }
    switch (texResult) {
    case -1:
        oldest = NameCache_Oldest(&gBrowserWork->imgList);
        if (oldest == NULL) {
            img->node.status = BNODE_FAILED;
            img->handles = 0;
            return BRES_IMAGE_FAILED;
        }
        if (oldest->tag.serial == BrowserHist_CurFlags()->serial) {
            img->node.status = BNODE_FAILED;
            img->handles = 0;
            return BRES_IMAGE_FAILED;
        }
        ImgCache_Free((BrowserImgNode *)oldest);
        goto retry;
    case -2:
    case -3:
        img->node.status = BNODE_FAILED;
        img->handles = 0;
        return BRES_IMAGE_FAILED;
    default:
        img->handles = texResult;
        return BRES_OK;
    }
}

/*
 * Takes the outcome of a request's HttpReq once it has finished. On
 * success the request is marked BNODE_DONE with the reply's buffer and
 * length, BRES_SERVER_ERROR when the server answered with an error status, and
 * BTAG_SJIS set when the reply was Shift-JIS. On failure it is marked
 * BNODE_FAILED with a result for the error (enum BrowserResultCode: no
 * network, connect, send, timeout, cancelled, redirected, a user name
 * needed, BRES_FAILED the rest). The HttpReq's error step is kept in both
 * cases.
 */
void BrowserReq_TakeHttpResult(BrowserReqNode *reqNode)
{
    HttpReq *http = reqNode->req;

    if (http->used != HTTP_REQ_FINISHED) {
        return;
    }
    switch (http->err) {
    case HTTP_ERR_NONE:
        reqNode->node.status = BNODE_DONE;
        reqNode->buf = http->buf;
        reqNode->len = http->len;
        reqNode->result = BRES_OK;
        reqNode->errStep = http->errStep;
        if (http->toSjis != 0) {
            if (http->sjis != 0) {
                BrowserTag_SetFlag(&reqNode->node.tag, BTAG_SJIS, 1);
            } else {
                BrowserTag_SetFlag(&reqNode->node.tag, BTAG_SJIS, 0);
            }
        }
        if (http->httpErr != 0) {
            reqNode->result = BRES_SERVER_ERROR;
        }
        break;
    case HTTP_ERR_CONNECT:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_CONNECT;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_SEND:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_SEND;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_TIMEOUT:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_TIMEOUT;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_URL:
    case HTTP_ERR_CLIENT:
    case HTTP_ERR_OPTIONS:
    case HTTP_ERR_NO_REPLY:
    case HTTP_ERR_STATUS:
    case HTTP_ERR_NO_CHALLENGE:
    case HTTP_ERR_AUTH_RESET:
    case HTTP_ERR_CREDENTIALS:
    case HTTP_ERR_AUTH_INFO:
    case HTTP_ERR_FREE:
    case 14: /* errors 14 and 16: nethttp never sets them */
    case 16:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_FAILED;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_NOT_CONNECTED:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_NO_NETWORK;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_CANCELLED:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_CANCELLED;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_REDIRECTED:
        reqNode->node.status = BNODE_FAILED;
        reqNode->buf = http->buf;
        reqNode->result = BRES_REDIRECTED;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_NEED_USER:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_NEED_USER;
        reqNode->errStep = http->errStep;
        break;
    case HTTP_ERR_NEED_PROXY_USER:
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_NEED_PROXY_USER;
        reqNode->errStep = http->errStep;
        break;
    }
}


/*
 * Fills an HttpReq for a request: the URL, the proxy mode and the two
 * proxy URLs, no-cache, the reply buffer and its size, the user names and
 * passwords, POST with the post data or GET, and whether to convert the
 * reply to Shift-JIS. The referer is kept only when it is an http:// or
 * https:// URL. The BrowserReq_Start* functions call it.
 */
void HttpReq_Setup(HttpReq *http, char *targetUrl, s8 proxyMode, char *httpProxy, char *httpsProxy, s8 bypassCache, u8 *replyBuf, s32 replySize,
                   char *refererUrl, s32 userName, s32 password, s32 proxyUserName, s32 proxyPassword, s8 isPost, s32 postBody, s8 convertToSjis)
{
    http->secure = 1;
    http->url = targetUrl;
    http->proxyFlags = proxyMode;
    http->proxyUrl = httpProxy;
    http->sslProxyUrl = httpsProxy;
    http->noCache = bypassCache;
    http->buf = (char *) replyBuf;
    http->bufSize = replySize;
    if (refererUrl == NULL) {
        http->referer = NULL;
    } else {
        switch ((u8)Url_GetScheme(refererUrl)) {
        case URL_HTTP:
        case URL_HTTPS:
            http->referer = refererUrl;
            break;
        case URL_FILE:
        case URL_MEMORY:
        case URL_LBS:
        default:
            http->referer = NULL;
            break;
        }
    }
    http->user = (char *) userName;
    http->pass = (char *) password;
    http->proxyUser = (char *) proxyUserName;
    http->proxyPass = (char *) proxyPassword;
    if (isPost != 0) {
        http->method = 2;
        http->postData = (char *) postBody;
    } else {
        http->method = 1;
        http->postData = NULL;
    }
    http->toSjis = convertToSjis;
    http->cancel = 0;
}

char *Url_Resolve(char *base, char *url);
s32 HttpReq_AllIdle(void);
HttpReq *HttpReq_Alloc(void);
extern char gHttpProxy[];
extern char gHttpsProxy[];

/*
 * Starts a queued page request: resolves its URL against the current page
 * (Url_Resolve; BRES_FAILED when that fails), waits until no HttpReq is in
 * use, then gets the page-cache node (BRES_NOT_CACHED when PageCache_Get
 * refuses). A cached page completes at once; otherwise an HttpReq fetches
 * it into the node's buffer, without the cache for a reload and as a POST
 * of the form's query for a post.
 */
void BrowserReq_StartPage(BrowserReqNode *reqNode)
{
    char fullUrl[0x100];
    BrowserPageNode *page;
    HttpReq *http;

    Url_Copy(fullUrl, reqNode->node.name);
    if (Url_Resolve(BrowserHist_CurUrl(), fullUrl) == NULL) {
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_FAILED;
        reqNode->errStep = 0;
        reqNode->cacheNode = 0;
        return;
    }
    if (HttpReq_AllIdle() == 0) {
        return;
    }
    page = PageCache_Get(fullUrl, reqNode->loadKind, &reqNode->node.tag);
    if (page == NULL) {
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_NOT_CACHED;
        reqNode->errStep = 0;
        return;
    }
    reqNode->cacheNode = (s32)page;
    switch (page->node.status) {
    case BNODE_CACHED:
        reqNode->node.status = BNODE_DONE;
        reqNode->req = NULL;
        break;
    case BNODE_LOADING:
        reqNode->node.status = BNODE_LOADING;
        reqNode->req = http = HttpReq_Alloc();
        HttpReq_Setup(http, page->node.name, gProxyMode, gHttpProxy, gHttpsProxy, reqNode->loadKind == BLOAD_RELOAD || reqNode->loadKind == BLOAD_REFETCH, page->buf, 0x8000,
                      reqNode->referer, 0, 0, 0, 0, reqNode->loadKind == BLOAD_POST, (s32)gBrowserWork->formData, 1);
        break;
    case BNODE_FAILED:
        break;
    }
}

s32 HttpReq_HasFree(void);

/*
 * Starts a queued image request: resolves its URL against the current
 * page (BRES_FAILED when that fails), waits for a free HttpReq (one free slot
 * for http:// and https://, every slot idle otherwise), then gets the
 * image-cache node, or waits while ImgCache_Get has none. A cached image
 * completes at once; otherwise an HttpReq fetches it into its slot's
 * connection buffer. The (u8 **) cast on connBuf makes the compiler add
 * the index before the load, as the game's code does.
 */
void BrowserReq_StartImage(BrowserReqNode *reqNode)
{
    char fullUrl[0x100];
    BrowserNode *img;
    HttpReq *http;

    Url_Copy(fullUrl, reqNode->node.name);
    if (Url_Resolve(BrowserHist_CurUrl(), fullUrl) == NULL) {
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_FAILED;
        reqNode->errStep = 0;
        reqNode->cacheNode = 0;
        return;
    }
    switch ((u8)Url_GetScheme(fullUrl)) {
    case URL_HTTP:
    case URL_HTTPS:
        if (HttpReq_HasFree() == 0) {
            return;
        }
        break;
    default:
        if (HttpReq_AllIdle() == 0) {
            return;
        }
        break;
    }
    img = ImgCache_Get(fullUrl, reqNode->loadKind, &reqNode->node.tag);
    if (img == NULL) {
        return;
    }
    reqNode->cacheNode = (s32)img;
    switch (img->status) {
    case BNODE_CACHED:
        reqNode->node.status = BNODE_DONE;
        reqNode->req = NULL;
        break;
    case BNODE_LOADING:
        reqNode->node.status = BNODE_LOADING;
        reqNode->req = http = HttpReq_Alloc();
        HttpReq_Setup(http, img->name, gProxyMode, gHttpProxy, gHttpsProxy, gBrowserWork->lastNav == BNAV_RELOAD,
                      ((u8 **)gBrowserWork->connBuf)[http->index], CONN_BUF_SIZE, reqNode->referer, 0, 0, 0, 0, 0, 0, 0); /* connBuf is read through a (u8 **) cast so that the compiler adds the index before the load, as the game's code does */
        break;
    case BNODE_FAILED:
        break;
    }
}

/*
 * Starts a queued download: resolves its URL against the current page
 * (BRES_FAILED when that fails), waits until no HttpReq is in use, then
 * fetches it with an HttpReq, through the work area's single download
 * node, into the buffer and length the request carries.
 */
void BrowserReq_StartDownload(BrowserReqNode *reqNode)
{
    char fullUrl[0x100];
    BrowserPageNode *download;
    HttpReq *http;

    Url_Copy(fullUrl, reqNode->node.name);
    if (Url_Resolve(BrowserHist_CurUrl(), fullUrl) == NULL) {
        reqNode->node.status = BNODE_FAILED;
        reqNode->result = BRES_FAILED;
        reqNode->errStep = 0;
        reqNode->cacheNode = 0;
        return;
    }
    if (HttpReq_AllIdle() == 0) {
        return;
    }
    memset(&gBrowserWork->download, 0, sizeof(gBrowserWork->download));
    download = &gBrowserWork->download;
    Url_Copy(download->node.name, fullUrl);
    download->node.status = BNODE_LOADING;
    download->buf = (u8 *)reqNode->buf;
    download->size = reqNode->len;
    reqNode->cacheNode = (s32)download;
    reqNode->node.status = BNODE_LOADING;
    reqNode->req = http = HttpReq_Alloc();
    HttpReq_Setup(http, download->node.name, gProxyMode, gHttpProxy, gHttpsProxy, gBrowserWork->lastNav == BNAV_RELOAD, download->buf,
                  download->size, reqNode->referer, 0, 0, 0, 0, 0, 0, 0);
}

void BrowserReq_TakeHttpResult(BrowserReqNode *e);
void BrowserReq_StartPage(BrowserReqNode *e);
void BrowserReq_StartImage(BrowserReqNode *e);
void BrowserReq_StartDownload(BrowserReqNode *e);

/*
 * Runs the request queue once a frame (Browser_Update calls it): steps
 * the HttpReq of each running request and takes its outcome
 * (BrowserReq_TakeHttpResult), and starts each queued one by its type (page,
 * image or download).
 */
void BrowserReq_UpdateAll(void)
{
    BrowserReqNode *reqNode = (BrowserReqNode *)&gBrowserWork->reqList;

    for (;;) {
        reqNode = (BrowserReqNode *)reqNode->node.next;
        if (reqNode == NULL) {
            break;
        }
        if (reqNode->node.status == BNODE_FREE) {
            break;
        }
        switch (reqNode->node.status) {
        case BNODE_LOADING:
            reqNode->req->run(reqNode->req);
            BrowserReq_TakeHttpResult(reqNode);
            break;
        case BNODE_QUEUED:
            switch (reqNode->type) {
            case BREQ_PAGE:
                BrowserReq_StartPage(reqNode);
                break;
            case BREQ_IMAGE:
                BrowserReq_StartImage(reqNode);
                break;
            case BREQ_DOWNLOAD:
                BrowserReq_StartDownload(reqNode);
                break;
            }
            break;
        }
    }
}

s32 HttpReq_Free(HttpReq *c);
char *Browser_GetText(u32 group, s32 i);
void net_clear_host_cache(void);

/*
 * Returns the result of the request for *url once it is done or failed,
 * else NULL. A loaded page is kept in the cache and committed to the
 * history; an image gets its texture (a failure marks the request failed
 * and returns NULL); a failed page gets an error message in errorText,
 * chosen by the error step, or the redirect URL. The request and its
 * HttpReq are then freed.
 */
BrowserResult *BrowserReq_Poll(char **url)
{
    BrowserReqNode *reqNode = (BrowserReqNode *)&gBrowserWork->reqList;
    BrowserPageNode *page;
    BrowserImgNode *img;

    for (;;) {
        reqNode = (BrowserReqNode *)reqNode->node.next;
        if (reqNode == NULL) {
            break;
        }
        if (reqNode->node.status == BNODE_FREE) {
            break;
        }
        if (Url_Compare(reqNode->node.name, *url) != 0) {
            continue;
        }
        switch (reqNode->node.status) {
        case BNODE_DONE:
            switch (reqNode->type) {
            case BREQ_PAGE:
                page = (BrowserPageNode *)reqNode->cacheNode;
                if (page->node.status == BNODE_LOADING) {
                    page->size = reqNode->len;
                    page->node.tag = reqNode->node.tag;
                    HttpReq_Free(reqNode->req);
                    reqNode->req = NULL;
                }
                BrowserHist_Commit(&gBrowserWork->histList, reqNode, page->node.name);
                page->node.status = BNODE_DONE;
                gBrowserWork->reqResult.data = page->buf;
                gBrowserWork->reqResult.result = reqNode->result;
                gBrowserWork->reqResult.w = 0;
                gBrowserWork->reqResult.h = 0;
                gBrowserWork->reqResult.size = page->size;
                break;
            case BREQ_IMAGE:
                img = (BrowserImgNode *)reqNode->cacheNode;
                if (img->node.status == BNODE_LOADING) {
                    img->data = reqNode->buf;
                    img->size = reqNode->len;
                    img->node.tag = reqNode->node.tag;
                    reqNode->result = ImgCache_MakeTexture(img);
                    if (reqNode->result != BRES_OK) {
                        reqNode->node.status = BNODE_FAILED;
                        reqNode->errStep = 0;
                        return NULL;
                    }
                    HttpReq_Free(reqNode->req);
                    reqNode->req = NULL;
                } else {
                    img->node.tag = reqNode->node.tag;
                }
                img->node.status = BNODE_DONE;
                gBrowserWork->reqResult.data = &img->handles;
                gBrowserWork->reqResult.result = reqNode->result;
                gBrowserWork->reqResult.w = img->w;
                gBrowserWork->reqResult.h = img->h;
                gBrowserWork->reqResult.size = img->size;
                break;
            case BREQ_DOWNLOAD:
                page = (BrowserPageNode *)reqNode->cacheNode;
                if (page->node.status == BNODE_LOADING) {
                    page->size = reqNode->len;
                    HttpReq_Free(reqNode->req);
                    reqNode->req = NULL;
                }
                page->node.status = BNODE_DONE;
                gBrowserWork->reqResult.data = page->buf;
                gBrowserWork->reqResult.result = reqNode->result;
                gBrowserWork->reqResult.w = 0;
                gBrowserWork->reqResult.h = 0;
                gBrowserWork->reqResult.size = page->size;
                break;
            }
            BrowserReq_Free(reqNode);
            return &gBrowserWork->reqResult;
        case BNODE_FAILED:
            switch (reqNode->type) {
            case BREQ_PAGE:
                page = (BrowserPageNode *)reqNode->cacheNode;
                switch (reqNode->result) {
                case BRES_CONNECT:
                case BRES_SEND:
                case BRES_TIMEOUT:
                    BrowserHist_Commit(&gBrowserWork->histList, reqNode, page->node.name);
                    switch (reqNode->errStep) {
                    case 0:
                    case 1:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 1), sizeof(gBrowserWork->errorText));
                        break;
                    case 2:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 2), sizeof(gBrowserWork->errorText));
                        break;
                    case 3:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 3), sizeof(gBrowserWork->errorText));
                        break;
                    case 4:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 4), sizeof(gBrowserWork->errorText));
                        net_clear_host_cache();
                        break;
                    case 5:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 5), sizeof(gBrowserWork->errorText));
                        break;
                    case 6:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 6), sizeof(gBrowserWork->errorText));
                        break;
                    case 7:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 7), sizeof(gBrowserWork->errorText));
                        break;
                    case 8:
                        strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 8), sizeof(gBrowserWork->errorText));
                        break;
                    }
                    gBrowserWork->reqResult.data = gBrowserWork->errorText;
                    break;
                case BRES_REDIRECTED:
                    Url_Copy(gBrowserWork->urlLower, (char *)page->buf);
                    gBrowserWork->reqResult.data = gBrowserWork->urlLower;
                    break;
                case BRES_NOT_CACHED:
                    BrowserHist_Commit(&gBrowserWork->histList, reqNode, NULL);
                    strncpy(gBrowserWork->errorText, Browser_GetText(BTEXT_ERROR_PAGE, 0), sizeof(gBrowserWork->errorText));
                    gBrowserWork->reqResult.data = gBrowserWork->errorText;
                    break;
                default:
                    gBrowserWork->reqResult.data = NULL;
                    break;
                }
                gBrowserWork->reqResult.result = reqNode->result;
                if (page != NULL) {
                    PageCache_Free(page);
                }
                break;
            case BREQ_IMAGE:
                img = (BrowserImgNode *)reqNode->cacheNode;
                gBrowserWork->reqResult.data = NULL;
                gBrowserWork->reqResult.result = reqNode->result;
                if (img != NULL) {
                    if (reqNode->result != BRES_CANCELLED) {
                        ImgCache_Free(img);
                    } else if (img->handles != 0) {
                        img->node.status = BNODE_DONE;
                    } else {
                        ImgCache_Free(img);
                    }
                }
                break;
            case BREQ_DOWNLOAD:
                gBrowserWork->reqResult.data = NULL;
                gBrowserWork->reqResult.result = reqNode->result;
                break;
            }
            if (reqNode->req != NULL) {
                HttpReq_Free(reqNode->req);
                reqNode->req = NULL;
            }
            BrowserReq_Free(reqNode);
            return &gBrowserWork->reqResult;
        }
    }
    return NULL;
}

/*
 * Undoes the history move of the last navigation: after a page load or a
 * post it goes back one page and drops the newer ones; after a forward
 * move it goes back one, after a back move forward one; a reload changes
 * nothing. Portal_ReshowStep calls it.
 */
void BrowserHist_Undo(void)
{
    switch (gBrowserWork->lastNav) {
    case BNAV_NONE:
    case BNAV_RELOAD:
        break;
    case BNAV_PAGE:
    case BNAV_POST:
        gBrowserWork->histCur = NameList_NextUsed(&gBrowserWork->histList, gBrowserWork->histCur);
        gBrowserWork->histCur = NameList_DropBefore(&gBrowserWork->histList, gBrowserWork->histCur);
        break;
    case BNAV_FORWARD:
        gBrowserWork->histCur = NameList_NextUsed(&gBrowserWork->histList, gBrowserWork->histCur);
        break;
    case BNAV_BACK:
        gBrowserWork->histCur = NameList_Prev(&gBrowserWork->histList, gBrowserWork->histCur);
        break;
    }
}

/*
 * Cancels every request in the queue: queued and done ones are marked
 * failed with BRES_CANCELLED, running ones ask their HttpReq to
 * cancel. Browser_CheckNetwork and Browser_SetStep call it.
 */
void BrowserReq_CancelAll(void)
{
    BrowserReqNode *reqNode = (BrowserReqNode *)&gBrowserWork->reqList;

    for (;;) {
        reqNode = (BrowserReqNode *)reqNode->node.next;
        if (reqNode == NULL) {
            break;
        }
        if (reqNode->node.status == BNODE_FREE) {
            break;
        }
        switch (reqNode->node.status) {
        case BNODE_FAILED:
            break;
        case BNODE_DONE:
        case BNODE_QUEUED:
            reqNode->node.status = BNODE_FAILED;
            reqNode->result = BRES_CANCELLED;
            reqNode->errStep = 0;
            break;
        case BNODE_LOADING:
            reqNode->req->cancel = 1;
            break;
        }
    }
}

/*
 * Draws a filled rectangle in color from its top left to its bottom right
 * corner, each corner first clamped to the 640 x 448 screen (the top edge
 * is clamped to 640, not 448, as in the game's code). The colour is the
 * first parameter, as in the line draws: only in this order do the callers
 * set up their arguments as the game's code does.
 */
void Draw_RectClipped(s32 color, f32 left, f32 top, f32 right, f32 bottom)
{
    f32 x1;
    f32 y1;
    f32 x2;
    f32 y2;

    x1 = left;
    y1 = top;
    x2 = right;
    y2 = bottom;
    if (x1 < 0.0f) {
        x1 = 0.0f;
    }
    if (y1 < 0.0f) {
        y1 = 0.0f;
    }
    if (!(x2 <= 640.0f)) {
        x2 = 640.0f;
    }
    if (!(y2 <= 448.0f)) {
        y2 = 448.0f;
    }
    if (!(x1 <= 640.0f)) {
        x1 = 640.0f;
    }
    if (!(y1 <= 640.0f)) {
        y1 = 640.0f;
    }
    if (x2 < 0.0f) {
        x2 = 0.0f;
    }
    if (y2 < 0.0f) {
        y2 = 0.0f;
    }
    Prim_DrawRect(color, x1, y1, x2, y2);
}

/*
 * Draws a filled triangle in color through the three points, unless it is
 * off the screen: nothing is drawn when the third point is left of or
 * above the screen, or the first point right of or below it. The points
 * are not clamped. Page_DrawObjBack draws a cell border's corners with it.
 */
void Draw_TriClipped(s32 color, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2)
{
    if (x2 < 0.0f) {
        return;
    }
    if (y2 < 0.0f) {
        return;
    }
    if (!(x0 <= 640.0f)) {
        return;
    }
    if (!(y0 <= 448.0f)) {
        return;
    }
    Prim_DrawTri(color, x0, y0, x1, y1, x2, y2);
}

u32 PageTex_Get(s8 i);

/*
 * Draws one small mark from the browser's first texture at (x, y): the
 * pointer (PAGE_WIDGET_POINTER, or _POINTER_ACTIVE over something it can
 * act on; both 3 pixels up and left of the point), a radio button or a
 * check box, off or on. Other kinds draw nothing.
 */
void Page_DrawMark(u8 kind, f32 x, f32 y)
{
    switch (kind) {
    case PAGE_WIDGET_POINTER:
        Prim_DrawSprite(-1, PageTex_Get(0), -3.0f + x, -3.0f + y, 0x20, 0x20, 0xC1, 0x41, 0xE0, 0x60);
        break;
    case PAGE_WIDGET_POINTER_ACTIVE:
        Prim_DrawSprite(-1, PageTex_Get(0), -3.0f + x, -3.0f + y, 0x20, 0x20, 0xA1, 0x41, 0xC0, 0x60);
        break;
    case PAGE_WIDGET_RADIO_OFF:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xE0, 0xC0, 0xF0, 0xD0);
        break;
    case PAGE_WIDGET_RADIO_ON:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xF0, 0xC0, 0x100, 0xD0);
        break;
    case PAGE_WIDGET_CHECK_OFF:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xE0, 0xD0, 0xF0, 0xE0);
        break;
    case PAGE_WIDGET_CHECK_ON:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xF0, 0xD0, 0x100, 0xE0);
        break;
    }
}

/*
 * Draws a browser widget of the given kind (enum PageWidget) at (x, y),
 * w by h, from the browser's textures, tinted by color (0: untinted): the
 * status bar, a text box, a button, the scroll arrows, tracks and thumbs
 * (the thumbs in three pieces when long enough), a closed drop-down list,
 * and the highlights of image buttons. The pointer's kinds draw nothing
 * here (Page_DrawMark draws them); most kinds use only one of w and h, or
 * neither.
 */
void Page_DrawWidget(u8 kind, f32 x, f32 y, f32 w, f32 h, s32 color)
{
    if (color == 0) {
        color = -1;
    }
    switch (kind) {
    case PAGE_WIDGET_IMGBTN_1:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x56, 0x1E, 0x40, 0xA2, 0x96, 0xC0);
        break;
    case PAGE_WIDGET_IMGBTN_2:
        Draw_RectClipped(0x80808080, x, y, 169.0f + x, 30.0f + y);
        break;
    case PAGE_WIDGET_IMGBTN_3:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0xD0, 0x20, 0, 0xC0, 0xD0, 0xE0);
        break;
    case PAGE_WIDGET_IMGBTN_4:
        Prim_DrawSprite(color, PageTex_Get(1), x, y, 0xCA, 0x20, 0, 0xC0, 0xCA, 0xE0);
        break;
    case PAGE_WIDGET_BUTTON_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0x40, 0x20, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0x40, 0x40, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, 0x14, 0x10, 0x40, 0x20, 0x60);
        break;
    case PAGE_WIDGET_BUTTON:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0x60, 0x20, 0x80);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0x60, 0x40, 0x80);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, 0x14, 0x10, 0x60, 0x20, 0x80);
        break;
    case PAGE_WIDGET_TEXT_BOX:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0x80, 0x20, 0xA0);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0x80, 0x40, 0xA0);
        Prim_DrawSprite(color, PageTex_Get(0), 4.0f + x, y, w - 4.0f, 0x14, 0x10, 0x80, 0x20, 0xA0);
        break;
    case PAGE_WIDGET_STATUS_BAR:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0xA0, 0x20, 0xC0);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0xA0, 0x40, 0xC0);
        Prim_DrawSprite(color, PageTex_Get(0), x - 1.0f, y, w - 1.0f, 0x14, 0x10, 0xA0, 0x20, 0xC0);
        break;
    case PAGE_WIDGET_ARROW_UP:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0x40, 0x100, 0x60);
        break;
    case PAGE_WIDGET_ARROW_DOWN:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0x60, 0x100, 0x80);
        break;
    case PAGE_WIDGET_ARROW_LEFT:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0x80, 0x100, 0xA0);
        break;
    case PAGE_WIDGET_ARROW_RIGHT:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0xA0, 0x100, 0xC0);
        break;
    case PAGE_WIDGET_ARROW_UP_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0x80, 0xE0, 0x60);
        break;
    case PAGE_WIDGET_ARROW_DOWN_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0x60, 0xE0, 0x40);
        break;
    case PAGE_WIDGET_ARROW_LEFT_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0xC0, 0xE0, 0xA0);
        break;
    case PAGE_WIDGET_ARROW_RIGHT_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0xA0, 0xE0, 0x80);
        break;
    case PAGE_WIDGET_SELECT:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x, y, w, h, 0);
        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN, x + w - 16.0f, 1.0f + y, 16.0f, h, 0);
        break;
    case PAGE_WIDGET_SELECT_PRESSED:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x, y, w, h, 0);
        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN_PRESSED, x + w - 16.0f, 1.0f + y, 16.0f, h, 0);
        break;
    case PAGE_WIDGET_VTHUMB:
        if (h < 32.0f) {
            Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x60, 0x40, 0x80, 0x60);
            break;
        }
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, 0x10, 0x60, 0x40, 0x80, 0x4F);
        Prim_DrawSprite(color, PageTex_Get(0), x, y + h - 16.0f, w, 0x10, 0x60, 0x50, 0x80, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), x, 16.0f + y, w, h - 32.0f, 0x60, 0x48, 0x80, 0x57);
        break;
    case PAGE_WIDGET_VTRACK:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x80, 0x40, 0xA0, 0x60);
        break;
    case PAGE_WIDGET_HTHUMB:
        if (w < 32.0f) {
            Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x60, 0x40, 0x80, 0x60);
            break;
        }
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x60, 0x40, 0x6F, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 16.0f, y, 0x10, h, 0x70, 0x40, 0x80, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, h, 0x68, 0x40, 0x77, 0x60);
        break;
    case PAGE_WIDGET_HTRACK:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x40, 0x40, 0x60, 0x60);
        break;
    case PAGE_WIDGET_POINTER:
    case PAGE_WIDGET_POINTER_ACTIVE:
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/browser_cache_draw", gPageEmptyText);

/*
 * Draws a page image: the texture in the given slot of the page textures
 * (BrowserWork.texHandle), stretched over the rectangle at (x, y), w by h,
 * using the whole texture (texW by texH texels). The last two parameters
 * are never read; Page_DrawObj passes the texture's size, others 0.
 */
void Page_DrawImage(u16 texSlot, f32 x, f32 y, f32 w, f32 h, s32 srcW, s32 srcH)
{
    Prim_DrawSprite(-1, gBrowserWork->texHandle[texSlot], x, y, w, h, 0, 0,
                  (s16)gBrowserWork->texW[texSlot], (s16)gBrowserWork->texH[texSlot]);
}

struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha);
/*
 * Draws an empty window over the rectangle from (x0, y0) to (x1, y1):
 * a list window with no items (ListPanel_DrawAlpha, frame style 4, fully
 * opaque), its size counted in 20-pixel cells. The browser's dialog boxes
 * draw it as their background.
 */
void BrowserDlg_DrawWindow(f32 x0, f32 y0, f32 x1, f32 y1)
{
    ListWin window;

    window.x = x0;
    window.y = y0;
    window.charH = 20;
    window.charW = 20;
    window.lineChars = (s32)(x1 - x0) / 20;
    window.lines = (s32)(y1 - y0) / 20;
    window.color = 0;
    window.flags = 4;
    window.items = NULL;
    window.cursorColor = 0;
    window.color2 = 0;
    ListPanel_DrawAlpha(&window, 0, -1, 0xFF);
}

/*
 * Clips the rectangle from (x0, y0) to (x1, y1) to the screen
 * (Rect_ClipToScreen) and, unless it is off the screen, draws it in color
 * one pixel wider. Every caller passes the same x twice, so it draws a
 * vertical line one pixel wide. The colour comes first,
 * as in Draw_RectClipped.
 */
void Draw_VLineClipped(s32 color, f32 x0, f32 y0, f32 x1, f32 y1)
{
    if (Rect_ClipToScreen(&x0, &y0, &x1, &y1) != 0) {
        Prim_DrawRect(color, x0, y0, 1.0f + x1, y1);
    }
}

/*
 * Clips the rectangle from (x0, y0) to (x1, y1) to the screen
 * (Rect_ClipToScreen) and, unless it is off the screen, draws it in color
 * one pixel taller. Every caller passes the same y twice, so it draws a
 * horizontal line one pixel high.
 */
void Draw_HLineClipped(s32 color, f32 x0, f32 y0, f32 x1, f32 y1)
{
    if (Rect_ClipToScreen(&x0, &y0, &x1, &y1) != 0) {
        Prim_DrawRect(color, x0, y0, x1, 1.0f + y1);
    }
}

/*
 * Draws a frame thickness pixels thick in color around the rectangle from
 * (x0, y0) to (x1, y1), clipped to the page view: the corners are kept
 * inside the page's margins and its 0x248 by 0x17C view, and nothing is
 * drawn when the top left corner lies right of or below the view. The
 * focus draws frame the focused object with it. Both branches of the
 * style test are the same; the test stays as the game's code has it.
 */
void Page_DrawFrame(s32 color, s32 thickness, f32 x0, f32 y0, f32 x1, f32 y1)
{
    BrowserState *pageState = gBrowserState;
    s16 left = pageState->marginLeft;
    s16 top;

    if (x0 < left) {
        x0 = left;
    }
    top = pageState->marginTop;
    if (y0 < top) {
        y0 = top;
    }
    if (x1 > 0x280 - pageState->marginRight) {
        x1 = (s16)(0x280 - pageState->marginRight);
    }
    if (y1 > 0x1C0 - pageState->marginBottom) {
        y1 = (s16)(0x1C0 - pageState->marginBottom);
    }
    if ((pageState->style & PAGE_STYLE_STATUS_BAR) >> 2 && (pageState->style & PAGE_STYLE_VSCROLL) >> 1) {
        if (x0 > left + 0x247) {
            return;
        }
        if (y0 > top + 0x17B) {
            return;
        }
        if (x1 > left + 0x248) {
            x1 = (s16)(left + 0x248);
        }
        if (y1 > top + 0x17C) {
            y1 = (s16)(top + 0x17C);
        }
    } else {
        if (x0 > left + 0x247) {
            return;
        }
        if (y0 > top + 0x17B) {
            return;
        }
        if (x1 > left + 0x248) {
            x1 = (s16)(left + 0x248);
        }
        if (y1 > top + 0x17C) {
            y1 = (s16)(top + 0x17C);
        }
    }
    Prim_DrawRect(color, x0, y0, x1, y0 + thickness);
    Prim_DrawRect(color, x1 - thickness, y0, x1, y1);
    Prim_DrawRect(color, x0, y1 - thickness, x1, y1);
    Prim_DrawRect(color, x0, y0, x0 + thickness, y1);
}

/*
 * Draws a one-pixel frame in color around the rectangle from (x0, y0) to
 * (x1, y1), its corners kept inside the page's margins. It frames the
 * drop-down list, an image not yet loaded and the choice in the browser's
 * menu box.
 */
void Page_DrawThinFrame(s32 color, f32 x0, f32 y0, f32 x1, f32 y1)
{
    BrowserState *pageState = gBrowserState;

    if (x0 < pageState->marginLeft) {
        x0 = pageState->marginLeft;
    }
    if (y0 < pageState->marginTop) {
        y0 = pageState->marginTop;
    }
    if (x1 > 0x280 - pageState->marginRight) {
        x1 = (s16)(0x280 - pageState->marginRight);
    }
    if (y1 > 0x1C0 - pageState->marginBottom) {
        y1 = (s16)(0x1C0 - pageState->marginBottom);
    }
    Prim_DrawRect(color, x0, y0, x1, 1.0f + y0);
    Prim_DrawRect(color, x1 - 1.0f, y0, x1, y1);
    Prim_DrawRect(color, x0, y1 - 1.0f, x1, y1);
    Prim_DrawRect(color, x0, y0, 1.0f + x0, y1);
}

void Gfx_SetTexFilter(s32 a);
void Font_SetPal(s32 a);
void Font_DrawAndClearPages(void);
char *strchr(char *s, s32 c);
extern char gPageEmptyText[];
void Page_Printf(const char *fmt, ...);

/*
 * Draws one line of text at (x, y) in the given size and colour, unless
 * the text is empty (or is this file's shared empty string) or the line
 * lies wholly above or below the screen. The colour's red, green and blue
 * are halved and its alpha cleared for Font_SetColor. Every percent sign
 * in the text is turned into a space, in place, because the text goes to
 * Page_Printf as a format. The first and third parameters (the third is
 * the bold flag of a text object) are never read.
 */
void Page_DrawTextLine(u8 unused, u32 color, u8 bold, u8 size, f32 x, f32 y, char *text)
{
    char *percent;

    if (*(u8 *)text == 0 || text == gPageEmptyText) {
        return;
    }
    Gfx_SetTexFilter(1);
    Font_SetSpacing(size, size);
    Font_SetPal(0);
    color = ((color & 0xFF) >> 1 & 0xFF) | (((color & 0xFF0000) >> 1 & 0xFF0000) | ((color & 0xFF00) >> 1 & 0xFF00));
    Font_SetColor(color);
    if (y + size > 0.0f && y < 448.0f) {
        Font_SetPos(x, y);
        for (;;) {
            percent = strchr(text, 0x25);
            if (percent == NULL) {
                break;
            }
            *percent = 0x20;
        }
        Page_Printf(text);
        Font_DrawAndClearPages();
    }
}

/*
 * Formats fmt and its arguments into a 0x400-byte buffer (vsnprintf) and
 * prints the result with Font_Printf at the font's current position.
 * Page_DrawTextLine calls it.
 */
void Page_Printf(const char *fmt, ...)
{
    char buf[0x400];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 0x400, fmt, ap);
    Font_Printf("%s", buf);
}

void Page_TileImage(BrowserObj *e, u8 i);

/*
 * Draws a page object's background, before the objects themselves
 * (DlgPage_Draw calls it for each object, last to first), when it lies on
 * the page view: a table's colour and, with a border, a one-pixel bevel;
 * a cell's fill, or its border (a bevel of light and dark edges, or one
 * colour); a loaded table or cell background image, tiled. The light
 * edges are grey on a white page. The scroll x is declared before the
 * kind, and the colour local serves only the test and the first
 * rectangle: only so does the compiler assign registers as the game did.
 */
void Page_DrawObjBack(BrowserObj *pageObj)
{
    f32 innerY;
    f32 x0;
    f32 innerX;
    f32 y0;
    f32 x;
    f32 y;
    f32 x1;
    f32 y1;
    s32 scrollX;
    s32 kind;
    s32 color; /* the test and the first rectangle; the others read e->color again, as the game does */
    BrowserState *pageState;

    kind = pageObj->kind;
    switch (kind) {
    case PAGE_OBJ_CELL:
    case PAGE_OBJ_TABLE:
        break;
    case PAGE_OBJ_IMAGE:
        if (pageObj->sub == PAGE_IMG_LOADED && pageObj->active == PAGE_IMG_CELL_BACK) {
            break;
        }
    default:
        return;
    }
    pageState = gBrowserState;
    scrollX = pageState->x;
    x = pageObj->x;
    if (!(x <= (f32)(scrollX + 640 - pageState->marginRight))) {
        return;
    }
    if (x < (f32)scrollX - pageObj->w) {
        return;
    }
    y = pageObj->y;
    if (y < (f32)pageState->y - pageObj->h) {
        return;
    }
    if (!(y <= (f32)(pageState->y + 428))) {
        return;
    }
    x0 = x - (f32)scrollX;
    y0 = y - (f32)pageState->y;
    x1 = x0 + pageObj->w;
    y1 = y0 + pageObj->h;
    switch (kind) {
    case PAGE_OBJ_IMAGE:
        Page_TileImage(pageObj, pageObj->tex);
        return;
    case PAGE_OBJ_CELL:
        switch (pageObj->sub) {
        case PAGE_CELL_BORDER:
            if (pageObj->field == 0) {
                return;
            }
            color = pageObj->color;
            if (color <= 0) {
                if (pageState->bgColor == -1) {
                    Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, y0, x0 + (f32)(u32)pageObj->field, y1);
                    Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, y0, x1, y0 + (f32)(u32)pageObj->field);
                } else {
                    Draw_RectClipped(-1, x0, y0, x0 + (f32)(u32)pageObj->field, y1);
                    Draw_RectClipped(-1, x0, y0, x1, y0 + (f32)(u32)pageObj->field);
                }
                Draw_RectClipped(BROWSER_BEVEL_DARK, x1 - (f32)(u32)pageObj->field, y0 + (f32)(u32)pageObj->field, x1, y1);
                Draw_RectClipped(BROWSER_BEVEL_DARK, x0 + (f32)(u32)pageObj->field, y1 - (f32)(u32)pageObj->field, x1, y1);
                Draw_TriClipped(BROWSER_BEVEL_DARK, x0, y1, x0 + (f32)(u32)pageObj->field, y1, x0 + (f32)(u32)pageObj->field, y1 - (f32)(u32)pageObj->field);
                Draw_TriClipped(BROWSER_BEVEL_DARK, x1, y0, x1, y0 + (f32)(u32)pageObj->field, x1 - (f32)(u32)pageObj->field, y0 + (f32)(u32)pageObj->field);
                return;
            } else {
                Draw_RectClipped(color | 0xFF000000, x0, y0, x0 + (f32)(u32)pageObj->field, y1);
                Draw_RectClipped(pageObj->color | 0xFF000000, x0, y0, x1, y0 + (f32)(u32)pageObj->field);
                Draw_RectClipped(pageObj->color | 0xFF000000, x1 - (f32)(u32)pageObj->field - 1.0f, y0 + (f32)(u32)pageObj->field, x1, y1);
                Draw_RectClipped(pageObj->color | 0xFF000000, x0 + (f32)(u32)pageObj->field, y1 - (f32)(u32)pageObj->field - 1.0f, x1, y1);
            }
            return;
        case PAGE_CELL_FILL:
            if (pageObj->color >= 0) {
                Draw_RectClipped(pageObj->color | 0xFF000000, x0, y0, x1, y1);
                return;
            }
            break;
        }
        break;
    case PAGE_OBJ_TABLE:
        if (pageObj->color >= 0) {
            Draw_RectClipped(pageObj->color | 0xFF000000, x0, y0, x1, y1);
        }
        if (pageObj->field != 0) {
            innerY = 1.0f + y0;
            innerX = 1.0f + x0;
            Draw_HLineClipped(BROWSER_BEVEL_DARK, innerX, innerY, x0 - 1.0f + pageObj->w, innerY);
            Draw_VLineClipped(BROWSER_BEVEL_DARK, innerX, innerY, innerX, y0 - 1.0f + pageObj->h);
            if (gBrowserState->bgColor == -1) {
                Draw_VLineClipped(BROWSER_BEVEL_LIGHT, x1, y0, x1, y1);
                Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1, x1, y1);
            } else {
                {
                    f32 rightX = x0 - 1.0f + pageObj->w;
                    Draw_VLineClipped(-1, rightX, y0 - 1.0f, rightX, y0 - 1.0f + pageObj->h);
                }
                {
                    f32 bottomY = y0 - 1.0f + pageObj->h;
                    Draw_HLineClipped(-1, x0 - 1.0f, bottomY, x0 - 1.0f + pageObj->w, bottomY);
                }
            }
        }
        break;
    }
}


/*
 * Draws one page object if it lies on the page view (text may start up
 * to 40 pixels above it) and sets its visible byte to say so. By kind:
 * text; links and underlined text (underlined with
 * PAGE_STYLE_UNDERLINE_LINKS); struck-out text; buttons (drawn pressed until the press timer
 * ends); text and password boxes (the value cut to the box, or
 * asterisks); the closed drop-down list; rules; radio buttons and check
 * boxes; images (the texture, or a frame with the image's text while it
 * isn't loaded). Page_DrawObjBack draws tables and cells.
 */
void Page_DrawObj(BrowserObj *pageObj)
{
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
    f32 dotRight;
    f32 strikeY;
    u8 size;
    u16 fitLen;
    s32 selectState;
    char buf[0x280];

    pageObj->visible = 0;
    switch (pageObj->kind) {
    case PAGE_OBJ_IMAGE:
        if (pageObj->active == PAGE_IMG_CELL_BACK) {
            return;
        }
        if (pageObj->active > PAGE_IMG_BODY_BACK) {
            BrowserState *pageState = gBrowserState;
            s32 scrollX = pageState->x;

            if (!(pageObj->x <= (f32)(scrollX + 640 - pageState->marginRight))) {
                return;
            }
            if (pageObj->x < (f32)scrollX - pageObj->w) {
                return;
            }
            if (pageObj->y < (f32)pageState->y - pageObj->h) {
                return;
            }
            if (!(pageObj->y <= (f32)(pageState->y + 428))) {
                return;
            }
        }
        break;
    case PAGE_OBJ_CELL:
    case PAGE_OBJ_TABLE:
        return;
    default: {
        BrowserState *pageState = gBrowserState;
        s32 scrollX = pageState->x;

        if (!(pageObj->x <= (f32)(scrollX + 640 - pageState->marginRight))) {
            return;
        }
        if (pageObj->x < (f32)scrollX - pageObj->w) {
            return;
        }
        if (pageObj->y < (f32)(pageState->y - 40)) {
            return;
        }
        if (!(pageObj->y <= (f32)(pageState->y + 428))) {
            return;
        }
        break;
    }
    }
    pageObj->visible = 1;
    x0 = pageObj->x - (f32)gBrowserState->x;
    y0 = pageObj->y - (f32)gBrowserState->y;
    x1 = x0 + pageObj->w;
    y1 = y0 + pageObj->h;
    switch (pageObj->kind) {
    case PAGE_OBJ_TEXT:
        Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, x0, y0, pageObj->s1);
        break;
    case PAGE_OBJ_LINK:
    case PAGE_OBJ_UNDERLINED:
        Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, x0, y0, pageObj->s2);
        if ((gBrowserState->style & PAGE_STYLE_UNDERLINE_LINKS) >> 7) {
            Draw_RectClipped(pageObj->color | 0xFF000000, x0, y1 - 2.0f, x1, y1);
        }
        break;
    case PAGE_OBJ_SUBMIT:
    case PAGE_OBJ_RESET:
    case PAGE_OBJ_BUTTON:
        if (gBrowserWork->pressTimer == 0) {
            pageObj->sub = 0;
        }
        switch (pageObj->sub) {
        case 0:
            Page_DrawWidget(PAGE_WIDGET_BUTTON, x0, y0, pageObj->w, pageObj->h, 0);
            if (strlen(pageObj->s1) == 1) {
                Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 6.0f + x0, 1.0f + y0, pageObj->s1);
            } else {
                Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 3.0f + x0, 1.0f + y0, pageObj->s1);
            }
            break;
        case 1:
            Page_DrawWidget(PAGE_WIDGET_BUTTON_PRESSED, x0, y0, pageObj->w, pageObj->h, 0);
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 3.0f + x0, 2.0f + y0, pageObj->s1);
            break;
        }
        break;
    case PAGE_OBJ_TEXT_INPUT:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x0, y0, pageObj->w, pageObj->h, 0);
        if (strlen(pageObj->s2) == 0) {
            break;
        }
        size = pageObj->size;
        fitLen = pageObj->w;
        if (fitLen < (u32)(size * (strlen(pageObj->s2) + 2)) >> 1) {
            fitLen = fitLen / (size / 2) - 2;
            memcpy(buf, pageObj->s2, fitLen);
            buf[fitLen] = 0;
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 3.0f + y0, buf);
        } else {
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, size, 4.0f + x0, 3.0f + y0, pageObj->s2);
        }
        break;
    case PAGE_OBJ_PASSWORD:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x0, y0, pageObj->w, pageObj->h, 0);
        if (strlen(pageObj->s2) != 0) {
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 3.0f + y0, "********");
        }
        break;
    case PAGE_OBJ_SELECT:
        selectState = pageObj->active;
        if ((u32)(selectState - PAGE_SELECT_CLOSED) < 2) {
            if (selectState == PAGE_SELECT_CLOSED) {
                Page_DrawWidget(PAGE_WIDGET_SELECT, x0, y0, pageObj->w, pageObj->h, 0);
            } else {
                Page_DrawWidget(PAGE_WIDGET_SELECT_PRESSED, x0, y0, pageObj->w, pageObj->h, 0);
                if (gBrowserWork->pressTimer == 0) {
                    pageObj->active = PAGE_SELECT_CLOSED;
                }
            }
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 2.0f + y0, pageObj->s1);
        }
        break;
    case PAGE_OBJ_RULE:
        switch ((s16)pageObj->h) {
        case 2:
            if ((s16)pageObj->w == 2) {
                Draw_RectClipped(BROWSER_BLACK, x0, y0, 1.0f + x0, 1.0f + y0);
            } else {
                Draw_HLineClipped(BROWSER_BLACK, x0, y0, x1, y0);
            }
            break;
        case 4:
            if ((s16)pageObj->w == 2) {
                dotRight = 1.0f + x0;
                Draw_RectClipped(BROWSER_BLACK, x0, y0, dotRight, 1.0f + y0);
                Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, 2.0f + y0, dotRight, 3.0f + y0);
            } else {
                Draw_HLineClipped(BROWSER_BLACK, x0, y0, x1 - 2.0f, y0);
                Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1 - 2.0f, x1 - 2.0f, y1 - 2.0f);
            }
            break;
        default:
            if ((s16)pageObj->w == 2) {
                Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, y0, 1.0f + x0, y1 - 2.0f);
            } else {
                Draw_HLineClipped(BROWSER_BLACK, x0, y0, x1, y0);
                Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1 - 2.0f, x1, y1 - 2.0f);
                Draw_VLineClipped(BROWSER_BEVEL_LIGHT, x1, y0, x1, y1 - 2.0f);
                Draw_VLineClipped(BROWSER_BLACK, x0, y0, x0, y1 - 2.0f);
            }
            break;
        }
        break;
    case PAGE_OBJ_RADIO:
        if (pageObj->sub == 1) {
            Page_DrawMark(PAGE_WIDGET_RADIO_ON, x0, y0);
        } else {
            Page_DrawMark(PAGE_WIDGET_RADIO_OFF, x0, y0);
        }
        break;
    case PAGE_OBJ_CHECKBOX:
        if (pageObj->sub == 1) {
            Page_DrawMark(PAGE_WIDGET_CHECK_ON, x0, y0);
        } else {
            Page_DrawMark(PAGE_WIDGET_CHECK_OFF, x0, y0);
        }
        break;
    case PAGE_OBJ_IMAGE:
        if (pageObj->sub == PAGE_IMG_LOADED && gBrowserWork->texHandle[pageObj->tex] != 0) {
            if (*(u8 *)pageObj->s2 != 0) {
                Page_DrawImage(pageObj->tex, x0, y0, pageObj->w, pageObj->h, gBrowserWork->texW[pageObj->tex],
                              gBrowserWork->texH[pageObj->tex]);
            }
        } else if (!(pageObj->field & 0x80)) {
            Page_DrawThinFrame(BROWSER_BLACK, x0, y0, x1 - 1.0f, y1 - 1.0f);
            Draw_VLineClipped(BROWSER_BEVEL_LIGHT, x1 - 2.0f, y0, x1 - 2.0f, y1);
            Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1 - 2.0f, x1, y1 - 2.0f);
            if (pageObj->s3 != 0) {
                Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 2.0f + y0, pageObj->s3);
            }
        }
        break;
    case PAGE_OBJ_STRUCK:
        Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, x0, y0, pageObj->s2);
        strikeY = y1 - (f32)(pageObj->size / 2);
        Draw_RectClipped(pageObj->color | 0xFF000000, x0, strikeY - 1.0f, x1, 2.0f + strikeY);
        if (strlen(pageObj->s1) != 0) {
            Draw_RectClipped(pageObj->color | 0xFF000000, x0, y1 - 2.0f, x1, y1);
        }
        break;
    }
}

/* The drop-down list's draw needs the compiler's optimization level 4. */
#pragma optimization_level 4
/*
 * Returns the browser's work area (gBrowserWork). The drop-down list
 * reads the page objects through it: only through this inline function
 * does the compiler add the index before it loads the work area's
 * address, as the game's code does.
 */
static inline BrowserWork *Browser_Work(void)
{
    return gBrowserWork;
}

/*
 * Draws the open drop-down list of the select object at listObjIdx: it
 * places the scroll thumb from listTop (clamping both at the ends), draws
 * the box above or below the field (listUp), a blue bar on the pointer's
 * row, a scroll bar when there are over 10 options, and up to 10 options
 * from listTop, the chosen one on a green bar. Compiled at the compiler's
 * optimization level 4, as the game's function was. Kept for the match:
 * gBrowserSub->listCount read again in the division, the (u16)k casts, n an s32
 * declared last, and j and k reused for the option walk and line count.
 */
void Page_DrawDropList(s32 listObjIdx)
{
    BrowserObj *selectObj;
    BrowserObj *option;
    u16 i;
    u16 j;
    u16 k;
    s32 rowCount;
    u16 selectIdx;
    f32 x0;
    f32 y0;
    f32 x1;
    s16 pointerX;
    s16 pointerY;
    s32 optionCount;

    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        if (Browser_Work()->obj[i] == NULL || Browser_Work()->obj[i]->active == 0) {
            break;
        }
        if (Browser_Work()->obj[i]->kind == PAGE_OBJ_SELECT && gBrowserSub->activeObj == i) {
            selectIdx = i;
            optionCount = gBrowserSub->listCount;
            if (!optionCount) {
                return;
            }
            for (j = selectIdx + 1; j < PAGE_OBJ_COUNT; j++) {
                if (Browser_Work()->obj[j]->kind != PAGE_OBJ_SELECT || !(Browser_Work()->obj[j]->tex < 11) || Browser_Work()->obj[j]->tex == 0) {
                    break;
                }
            }
            gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * 144 / gBrowserSub->listCount + 18;
            gBrowserSub->thumbBottom = 0x5A0 / gBrowserSub->listCount + gBrowserSub->thumbTop;
            gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
            if (gBrowserSub->thumbBottom > 180) {
                gBrowserSub->thumbBottom = 180;
            }
            if (gBrowserSub->listCount > DROPLIST_ROWS) {
                if (gBrowserSub->thumbTop < 18) {
                    gBrowserSub->thumbTop = 18;
                    gBrowserSub->thumbBottom = gBrowserSub->thumbLen + 18;
                    gBrowserSub->listTop = 1;
                }
                if (gBrowserSub->thumbBottom > 162) {
                    gBrowserSub->thumbTop = 162 - gBrowserSub->thumbLen;
                    gBrowserSub->thumbBottom = 162;
                    gBrowserSub->listTop = gBrowserSub->listCount - 9;
                }
            }
            selectObj = Browser_Work()->obj[i];
            x0 = selectObj->x - (f32)gBrowserState->x;
            y0 = selectObj->y - (f32)gBrowserState->y;
            x1 = x0 + selectObj->w;
            if (gBrowserSub->listUp != 0) {
                Draw_RectClipped(-1, x0, y0, x1, y0 - (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                Page_DrawThinFrame(BROWSER_BLACK, x0, y0, x1, y0 - (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                pointerX = gBrowserSub->cursorX;
                if (!((f32)pointerX <= x0) && ((f32)pointerX <= x1 - 16.0f || (gBrowserSub->listCount < 11 && (f32)pointerX < x1))) {
                    if ((f32)gBrowserSub->cursorY < y0) {
                        rowCount = Browser_Work()->obj[j - 1]->tex + 1;
                        if (!((f32)gBrowserSub->cursorY <= y0 - (f32)(rowCount * DROPLIST_ROW_H))) {
                            for (k = 0; k < Browser_Work()->obj[j - 1]->tex + 1; k++) {
                                if (!((f32)gBrowserSub->cursorY <= y0 - (f32)(k * DROPLIST_ROW_H)) && (f32)gBrowserSub->cursorY <= y0 - (f32)((k - 1) * DROPLIST_ROW_H)) {
                                    Draw_RectClipped(BROWSER_LIST_HOVER, x0, y0 - (f32)(k * DROPLIST_ROW_H), x1, y0 - (f32)((k - 1) * DROPLIST_ROW_H));
                                    break;
                                }
                            }
                        }
                    }
                }
                if (optionCount > DROPLIST_ROWS) {
                    if (gBrowserSub->moveKind != BMOVE_LIST_UP) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP, x1 - 16.0f, y0 - 180.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP_PRESSED, x1 - 16.0f, y0 - 180.0f, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTRACK, x1 - 16.0f, y0 - 20.0f, 16.0f, -140.0f, 0);
                    if (gBrowserSub->moveKind != BMOVE_LIST_DOWN) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN, x1 - 16.0f, y0 - 20.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN_PRESSED, x1 - 16.0f, y0 - 20.0f, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTHUMB, x1 - 16.0f, y0 - 180.0f + (f32)gBrowserSub->thumbTop, 16.0f, (f32)(gBrowserSub->thumbBottom - gBrowserSub->thumbTop), 0);
                }
            } else {
                Draw_RectClipped(-1, x0, 20.0f + y0, x1, (20.0f + y0) + (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                Page_DrawThinFrame(BROWSER_BLACK, x0, 4.0f + (16.0f + y0), x1, (20.0f + y0) + (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                pointerX = gBrowserSub->cursorX;
                if (!((f32)pointerX <= x0) && (f32)pointerX < x1 - 16.0f) {
                    if (!((f32)gBrowserSub->cursorY <= (20.0f + y0))) {
                        rowCount = Browser_Work()->obj[j - 1]->tex;
                        if ((f32)gBrowserSub->cursorY < (20.0f + y0) + (f32)(rowCount * DROPLIST_ROW_H)) {
                            for (k = 0; k < Browser_Work()->obj[j - 1]->tex; k++) {
                                if (!((f32)gBrowserSub->cursorY <= (20.0f + y0) + (f32)(k * DROPLIST_ROW_H)) && (f32)gBrowserSub->cursorY < (20.0f + y0) + (f32)((k + 1) * DROPLIST_ROW_H)) {
                                    Draw_RectClipped(BROWSER_LIST_HOVER, x0, (20.0f + y0) + (f32)(k * DROPLIST_ROW_H), x1, (20.0f + y0) + (f32)((k + 1) * DROPLIST_ROW_H));
                                    break;
                                }
                            }
                        }
                    }
                }
                if (optionCount > DROPLIST_ROWS) {
                    if (gBrowserSub->moveKind != BMOVE_LIST_UP) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP, x1 - 16.0f, 20.0f + y0, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP_PRESSED, x1 - 16.0f, 20.0f + y0, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTRACK, x1 - 16.0f, 20.0f + (20.0f + y0), 16.0f, 160.0f, 0);
                    if (gBrowserSub->moveKind != BMOVE_LIST_DOWN) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN, x1 - 16.0f, 180.0f + (20.0f + y0) - 20.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN_PRESSED, x1 - 16.0f, 180.0f + (20.0f + y0) - 20.0f, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTHUMB, x1 - 16.0f, (20.0f + y0) + (f32)gBrowserSub->thumbTop, 16.0f, (f32)(gBrowserSub->thumbBottom - gBrowserSub->thumbTop), 0);
                }
            }
            k = 0;
            for (j = selectIdx + gBrowserSub->listTop; j < PAGE_OBJ_COUNT; j++) {
                option = Browser_Work()->obj[j];
                if (option->kind != PAGE_OBJ_SELECT) {
                    return;
                }
                if (option->tex == 0) {
                    return;
                }
                k++;
                if (!(k < DROPLIST_ROWS + 1)) {
                    return;
                }
                if (gBrowserSub->listUp != 0) {
                    if (optionCount < DROPLIST_ROWS + 1) {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 - (f32)((optionCount - k + 1) * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w, 18.0f + (y0 - (f32)((optionCount - k + 1) * DROPLIST_ROW_H)));
                        }
                        Page_DrawTextLine(0x15, BROWSER_BLACK, 0, Browser_Work()->obj[j]->size, 4.0f + x0,
                                          2.0f + y0 - (f32)((optionCount - (u16)k + 1) * DROPLIST_ROW_H), Browser_Work()->obj[j]->s1);
                    } else {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 - (f32)((11 - k) * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w - 16.0f, 18.0f + (y0 - (f32)((11 - k) * DROPLIST_ROW_H)));
                        }
                        Page_DrawTextLine(0x15, BROWSER_BLACK, 0, Browser_Work()->obj[j]->size, 4.0f + x0,
                                          2.0f + y0 - (f32)((11 - (u16)k) * DROPLIST_ROW_H), Browser_Work()->obj[j]->s1);
                    }
                } else {
                    if (optionCount < DROPLIST_ROWS + 1) {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 + (f32)(k * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w, 2.0f + (18.0f + (y0 + (f32)(k * DROPLIST_ROW_H))));
                        }
                    } else {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 + (f32)(k * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w - 16.0f, 2.0f + (18.0f + (y0 + (f32)(k * DROPLIST_ROW_H))));
                        }
                    }
                    Page_DrawTextLine(0x15, BROWSER_BLACK, 0, Browser_Work()->obj[j]->size, 4.0f + x0, 2.0f + y0 + (f32)(k * DROPLIST_ROW_H),
                                      Browser_Work()->obj[j]->s1);
                }
            }
            break;
        }
    }
}
#pragma optimization_level reset

void Browser_DrawOverlay(void);

/*
 * Draws the highlight on the focused page object (BrowserWork.focus).
 * cursorObj is the cursor object: the highlight is red while its blink
 * rises (glow above 0), else white. An image button gets its
 * highlight widget (PAGE_WIDGET_IMGBTN_1 to _4, by its kind), then the overlay again
 * when the page's styleMode is set; any other object gets a 3-pixel frame
 * (Page_DrawFrame) just outside it. The switch reads the focused object
 * again from gBrowserWork rather than from the local focusObj: from the
 * local, the compiler's code differs.
 */
void Page_DrawFocus(BrowserObj *cursorObj)
{
    s32 color;
    BrowserObj *focusObj;
    u8 kind;
    s8 widgetKind;

    color = cursorObj->glow > 0 ? BROWSER_CURSOR_RED : -1;
    focusObj = gBrowserWork->obj[gBrowserWork->focus];
    kind = focusObj->kind;
    if (kind == PAGE_OBJ_LINK || kind == PAGE_OBJ_CHECKBOX || kind == PAGE_OBJ_RADIO || kind == PAGE_OBJ_SELECT || kind == PAGE_OBJ_PASSWORD || kind == PAGE_OBJ_TEXT_INPUT || kind == PAGE_OBJ_BUTTON
        || kind == PAGE_OBJ_RESET || kind == PAGE_OBJ_SUBMIT || kind == PAGE_OBJ_UNDERLINED || kind == PAGE_OBJ_STRUCK) {
        goto frame;
    }
    switch (kind) {
    case PAGE_OBJ_IMAGE:
        switch (gBrowserWork->obj[gBrowserWork->focus]->field & 0xF) {
        case 1:
            if ((focusObj->field & 0x10) == 0x10) {
                widgetKind = PAGE_WIDGET_IMGBTN_3;
            } else {
                widgetKind = PAGE_WIDGET_IMGBTN_1;
            }
            break;
        case 2:
            widgetKind = PAGE_WIDGET_IMGBTN_2;
            break;
        case 3:
            widgetKind = PAGE_WIDGET_IMGBTN_3;
            break;
        case 4:
            widgetKind = PAGE_WIDGET_IMGBTN_4;
            break;
        default:
            widgetKind = 0;
            break;
        }
        if (widgetKind) {
            Page_DrawWidget(widgetKind, focusObj->x - gBrowserState->x, focusObj->y - gBrowserState->y, 0.0f, 0.0f, color);
            if (gBrowserState->styleMode != 0) {
                Browser_DrawOverlay();
            }
            return;
        }
        break;
    }
frame:
    Page_DrawFrame(color, 3, focusObj->x - gBrowserState->x - 1.0f, focusObj->y - gBrowserState->y - 1.0f,
                   3.0f + (focusObj->x + focusObj->w - gBrowserState->x), 3.0f + (focusObj->y + focusObj->h - gBrowserState->y));
}

void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
void Text_Reset(void);
void Font_SetHalfWidth(s32 a0);
void LobbyMsg_DrawPage5(s32 msg);
void Text_DrawIcons(s32 i);
void Font_DrawPage(s32 layer);
void NetModeMsg_Flush(s8 which);

extern s32 gTextOff;
/*
 * Draws the message box of dialog kinds 4, 5 and 6 (dlgBoxKind): a window
 * with one message and, while the dialog asks (dlgMode 3), a glowing bar
 * on the choice (dlgCursor). Kind 4 asks, then (dlgMode 4) shows the net
 * mode's message, or after kind 6's question the message for errorKind;
 * kind 5 only asks; kind 6 asks, then shows the error. Messages come from
 * the lobby's message pages (LobbyMsg_DrawPage5) or text group 5. Draws
 * nothing for other kinds or an unknown error.
 */
void BrowserDlg_DrawMsgBox(BrowserObj *dlgObj)
{
    s8 msg = 0;
    s8 row = 8;
    s32 highlight = 0;
    s32 useMsgPage = 0;
    s32 line;
    s32 glowLeft;
    s32 w;
    s32 glowTop;
    s32 glowRight;
    s32 glowBottom;
    s16 k;

    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_YES_NO:
        msg = 3;
        highlight = 1;
        row = 9;
        break;
    case BDLG_KIND_MESSAGE:
        switch (gBrowserSub->dlgMode) {
        case BDLG_MODE_CHOICE:
            highlight = 1;
            gBrowserWork->dlgQuestion = 1;
            row = 7;
            break;
        case BDLG_MODE_SAVE_LOGIN:
            switch ((s8)gBrowserWork->dlgQuestion) {
            case 1:
            default:
                msg = gNetMode.cardMsg;
                useMsgPage = 1;
                highlight = 0;
                break;
            case 2:
                switch ((s8)gBrowserWork->errorKind) {
                case 0:
                case 1:
                case 2:
                case 10:
                case 20:
                case 21:
                case 22:
                case 23:
                case 30:
                    useMsgPage = 1;
                    msg = 1;
                    break;
                case 40:
                    msg = 7;
                    break;
                case 41:
                    msg = 9;
                    break;
                case -10:
                    msg = 4;
                    useMsgPage = 1;
                    break;
                case -21:
                    useMsgPage = 1;
                    msg = 5;
                    break;
                case -23:
                    msg = 6;
                    useMsgPage = 1;
                    break;
                default:
                    return;
                }
                highlight = 0;
                break;
            }
            break;
        }
        break;
    case BDLG_KIND_CLEAR_LOGIN:
        switch (gBrowserSub->dlgMode) {
        case BDLG_MODE_CHOICE:
            msg = 4;
            highlight = 1;
            gBrowserWork->dlgQuestion = 2;
            break;
        case BDLG_MODE_SAVE_LOGIN:
            switch ((s8)gBrowserWork->errorKind) {
            case 0:
            case 1:
            case 2:
            case 10:
            case 20:
            case 21:
            case 22:
            case 23:
            case 30:
                useMsgPage = 1;
                msg = 1;
                break;
            }
            highlight = 0;
            break;
        }
        row = 10;
        break;
    default:
        return;
    }
    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y - 40.0f, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    if (highlight != 0) {
        glowLeft = 90.0f + dlgObj->x;
        line = row + gBrowserSub->dlgCursor;
        glowTop = dlgObj->y + (f32)((line - 2) * 20);
        glowRight = 10.0f + (420.0f + dlgObj->x);
        glowBottom = dlgObj->y + (f32)((line - 1) * 20);
        Menu_DrawGlowBar(glowLeft + (glowRight - glowLeft) / 2, glowTop, (glowRight - glowLeft) + 0x50, glowBottom - glowTop, 0xFF20C0C0);
    }
    if (useMsgPage == 0) {
        Page_DrawTextLine(0x14, -1, 0, 0x14, 20.0f + dlgObj->x, dlgObj->y, Browser_GetText(BTEXT_NET_SAVE, msg));
    } else {
        Text_Reset();
        Font_SetHalfWidth(1);
        LobbyMsg_DrawPage5(msg);
        for (k = 0; k < 7; k++) {
            Text_DrawIcons(k);
            Font_DrawPage(k);
        }
        gTextOff = 1;
    }
    NetModeMsg_Flush(1);
}

void TextObj_Reset(void);
s32 Str_Length(char *s);
/*
 * Draws the two-button dialog: a window, the question (text group 7, text
 * 0) centred, and two buttons labelled with texts 1 and 2. The button
 * under the cursor (dlgCursor 1 or 2) is drawn again as the cursor
 * button, red while the dialog's glow brightens, else white.
 */
void BrowserDlg_DrawYesNo(BrowserObj *dlgObj)
{
    s16 buttonX;
    u32 color;
    f32 textX;

    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    Prim_DrawSprite(-1, PageTex_Get(0), 49.0f + dlgObj->x, 145.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    Prim_DrawSprite(-1, PageTex_Get(0), 157.0f + dlgObj->x, 145.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    textX = dlgObj->x + (dlgObj->w - (f32)(Str_Length(Browser_GetText(BTEXT_REGISTER, 0)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(0x14, -1, 0, 0x14, textX, 85.0f + dlgObj->y, Browser_GetText(BTEXT_REGISTER, 0));
    textX = 49.0f + dlgObj->x + (f32)((0x54 - Str_Length(Browser_GetText(BTEXT_REGISTER, 1)) * 20 / 2) / 2);
    Page_DrawTextLine(0x14, -1, 1, 0x14, textX, 6.0f + (145.0f + dlgObj->y), Browser_GetText(BTEXT_REGISTER, 1));
    textX = 157.0f + dlgObj->x + (f32)((0x54 - Str_Length(Browser_GetText(BTEXT_REGISTER, 2)) * 20 / 2) / 2);
    Page_DrawTextLine(0x14, -1, 1, 0x14, textX, 6.0f + (145.0f + dlgObj->y), Browser_GetText(BTEXT_REGISTER, 2));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        buttonX = 49.0f + dlgObj->x;
        break;
    case 2:
        buttonX = 157.0f + dlgObj->x;
        break;
    }
    color = -1;
    if (dlgObj->glowDir == GLOW_RISING) {
        color = BROWSER_CURSOR_RED;
    }
    Prim_DrawSprite(color, PageTex_Get(0), buttonX, 145.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
    TextObj_Reset();
}


/*
 * Draws the one-button dialog: a window, a message (text group 7, text 3)
 * and one button labelled with text 4, drawn again as the cursor button
 * (red while the glow brightens). The message is centred by the width of
 * text 0, not text 3, as in the game's code.
 */
void BrowserDlg_DrawOk(BrowserObj *dlgObj)
{
    s16 buttonX;
    u32 color;
    f32 textX;

    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    Prim_DrawSprite(-1, PageTex_Get(0), dlgObj->x + (dlgObj->w - 84.0f) / 2.0f, 145.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    textX = dlgObj->x + (dlgObj->w - (f32)(Str_Length(Browser_GetText(BTEXT_REGISTER, 0)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(0x14, -1, 0, 0x14, textX, 85.0f + dlgObj->y, Browser_GetText(BTEXT_REGISTER, 3));
    textX = dlgObj->x + (dlgObj->w - (f32)(Str_Length(Browser_GetText(BTEXT_REGISTER, 4)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(0x14, -1, 1, 0x14, textX, 6.0f + (145.0f + dlgObj->y), Browser_GetText(BTEXT_REGISTER, 4));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        buttonX = dlgObj->x + (dlgObj->w - 84.0f) / 2.0f;
        break;
    }
    color = -1;
    if (dlgObj->glowDir == GLOW_RISING) {
        color = BROWSER_CURSOR_RED;
    }
    Prim_DrawSprite(color, PageTex_Get(0), buttonX, 145.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
    TextObj_Reset();
}

s32 strcmp(char *a, char *b);

/* A dialog of text group 8: two buttons (texts 3, 4) on the KDDI page below
   while gBrowserWork->endReason > 0, else one (text 1). */

/*
 * Draws the dialog of text group 8. While the browser is ending
 * (endReason above 0) without a network error and the page is the
 * portal's top page, it shows a message (text 2) and two buttons (texts 3
 * and 4); otherwise a message (text 0) and one button (text 1). The cursor
 * button is drawn as in BrowserDlg_DrawYesNo.
 */
void BrowserDlg_DrawCrsTop(BrowserObj *dlgObj)
{
    s16 buttonX;
    u32 color;
    f32 textX;

    if ((s8)gBrowserWork->endReason > 0 && gBrowserWork->netError == 0
        && strcmp(gBrowserState->url, "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp") == 0) {
        BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
        Prim_DrawSprite(-1, PageTex_Get(0), 80.0f + dlgObj->x, 165.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
        Prim_DrawSprite(-1, PageTex_Get(0), 216.0f + dlgObj->x, 165.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
        Page_DrawTextLine(0x14, -1, 0, 0x14, 40.0f + dlgObj->x, 80.0f + dlgObj->y, Browser_GetText(BTEXT_COMM_ERROR, 2));
        textX = 80.0f + dlgObj->x + (f32)((0x54 - Str_Length(Browser_GetText(BTEXT_COMM_ERROR, 3)) * 20 / 2) / 2);
        Page_DrawTextLine(0x14, -1, 1, 0x14, textX, 6.0f + (165.0f + dlgObj->y), Browser_GetText(BTEXT_COMM_ERROR, 3));
        textX = 216.0f + dlgObj->x + (f32)((0x54 - Str_Length(Browser_GetText(BTEXT_COMM_ERROR, 4)) * 20 / 2) / 2);
        Page_DrawTextLine(0x14, -1, 1, 0x14, textX, 6.0f + (165.0f + dlgObj->y), Browser_GetText(BTEXT_COMM_ERROR, 4));
        switch (gBrowserSub->dlgCursor) {
        case 1:
            buttonX = 80.0f + dlgObj->x;
            break;
        case 2:
            buttonX = 216.0f + dlgObj->x;
            break;
        }
        color = -1;
        if (dlgObj->glowDir == GLOW_RISING) {
            color = BROWSER_CURSOR_RED;
        }
        Prim_DrawSprite(color, PageTex_Get(0), buttonX, 165.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
        TextObj_Reset();
        return;
    }
    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    Prim_DrawSprite(-1, PageTex_Get(0), 138.0f + dlgObj->x, 185.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    Page_DrawTextLine(0x14, -1, 0, 0x14, 40.0f + dlgObj->x, 85.0f + dlgObj->y, Browser_GetText(BTEXT_COMM_ERROR, 0));
    textX = 138.0f + dlgObj->x + (f32)((0x54 - Str_Length(Browser_GetText(BTEXT_COMM_ERROR, 1)) * 20 / 2) / 2);
    Page_DrawTextLine(0x14, -1, 1, 0x14, textX, 6.0f + (185.0f + dlgObj->y), Browser_GetText(BTEXT_COMM_ERROR, 1));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        buttonX = 138.0f + dlgObj->x;
        break;
    }
    color = -1;
    if (dlgObj->glowDir == GLOW_RISING) {
        color = BROWSER_CURSOR_RED;
    }
    Prim_DrawSprite(color, PageTex_Get(0), buttonX, 185.0f + dlgObj->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
    TextObj_Reset();
}


#pragma optimization_level 4

/*
 * Tiles the page texture in slot texSlot over the object's rectangle, from
 * its top left corner, row by row; the tiles of the last column and row are
 * cut to the rectangle, texels and all. It stops after 0x200 tiles.
 * Page_DrawObjBack draws a table's or cell's background image with it.
 * Compiled at the compiler's optimization level 4, as the game's function
 * was. The empty first branch of the cut test (no cut) stays: the game's
 * code tests that case too, and it compiles to no instructions.
 */
void Page_TileImage(BrowserObj *pageObj, u8 texSlot)
{
    f32 overhang;
    f32 x;
    u8 tileCount;
    f32 y;
    f32 w;
    f32 h;
    f32 cutRight;
    f32 srcW;
    f32 cutBottom;
    f32 srcH;

    tileCount = 0;
    cutBottom = 0.0f;
    y = pageObj->y - gBrowserState->y;
    for (;;) {
        overhang = (y + (f32)gBrowserWork->texH[texSlot]) - (pageObj->h + (pageObj->y - gBrowserState->y));
        if (overhang > 0.0f) {
            cutBottom = overhang;
            if (cutBottom == 0.0f) {
                return;
            }
        }
        cutRight = 0.0f;
        x = pageObj->x - gBrowserState->x;
        for (;;) {
            overhang = (x + (f32)gBrowserWork->texW[texSlot]) - (pageObj->w + (pageObj->x - gBrowserState->x));
            if (overhang > 0.0f) {
                cutRight = overhang;
                if (cutRight == 0.0f) {
                    break;
                }
            }
            w = gBrowserWork->texW[texSlot];
            h = gBrowserWork->texH[texSlot];
            srcW = gBrowserWork->texW[texSlot];
            srcH = gBrowserWork->texH[texSlot];
            if ((cutRight == 0.0f || cutRight == 1.0f) && (cutBottom == 0.0f || cutBottom == 1.0f)) { /* kept as the game's code tests it; it compiles to nothing */
            } else if ((cutRight != 0.0f || cutRight == 1.0f) && (cutBottom == 0.0f || cutBottom == 1.0f)) {
                w -= cutRight;
                srcW -= cutRight;
            } else if ((cutRight == 0.0f || cutRight == 1.0f) && cutBottom != 0.0f && cutBottom != 1.0f) {
                h -= cutBottom;
                srcH -= cutBottom;
            } else {
                w -= cutRight;
                h -= cutBottom;
                srcW -= cutRight;
                srcH -= cutBottom;
            }
            Prim_DrawSprite(-1, gBrowserWork->texHandle[texSlot], x, y, w, h, 0, 0, srcW, srcH);
            tileCount++;
            if (tileCount > 0x200) {
                return;
            }
            if (cutRight > 0.0f) {
                break;
            }
            x += gBrowserWork->texW[texSlot];
        }
        if (cutBottom > 0.0f) {
            return;
        }
        y += gBrowserWork->texH[texSlot];
    }
}
#pragma optimization_level reset

/*
 * Darkens the whole screen with a half-transparent black rectangle.
 * Browser_DrawOverlay calls it to dim the page behind a dialog.
 */
void Draw_DimScreen(void)
{
    Draw_RectClipped(BROWSER_DIM, 0.0f, 0.0f, 640.0f, 448.0f);
}

/*
 * Draws over the screen after the page: black over the four margins
 * around the page view; the message icon at the top right when the page
 * has a message and style PAGE_STYLE_MSG_ICON without
 * PAGE_STYLE_NO_MSG_ICON; the dimming (Draw_DimScreen) when
 * PAGE_STYLE_POINTER is clear and a dialog, a message or
 * the input box is up; then the fade, while initMode is set: a positive
 * fade counts down from black, a negative one counts up to black. Else
 * fade is cleared. The two margin branches are the same, as in the game.
 */
void Browser_DrawOverlay(void)
{
    s8 *fade;
    s32 fadeColor;

    if ((gBrowserState->style & PAGE_STYLE_NO_MSG_ICON) >> 9) {
        if (gBrowserState->plain != 0) {
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
            Draw_RectClipped(BROWSER_BLACK, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        } else {
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
            Draw_RectClipped(BROWSER_BLACK, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        }
    } else {
        Draw_RectClipped(BROWSER_BLACK, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
        Draw_RectClipped(BROWSER_BLACK, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
        Draw_RectClipped(BROWSER_BLACK, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
        Draw_RectClipped(BROWSER_BLACK, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        if ((gBrowserState->style & PAGE_STYLE_MSG_ICON) >> 8 && strlen(gBrowserState->msg) != 0) {
            Prim_DrawSprite(-1, PageTex_Get(0), 0x228, 0x1E, 0x18, 0x18, 0xA0, 0, 0xC0, 0x20);
        }
    }
    if (!(gBrowserState->style & PAGE_STYLE_POINTER)) {
        if (gBrowserWork->dlgMenuState != BDLG_MENU_HIDDEN || gBrowserWork->dlgInputState != BDLG_INPUT_IDLE || gBrowserWork->dlgBoxKind != BDLG_KIND_NONE) {
            Draw_DimScreen();
        }
    }
    if (gBrowserState->initMode != 0) {
        fade = &gBrowserState->fade;
        if (*fade > 0) {
            *fade = *fade - 1;
            fadeColor = (gBrowserState->fade * 255 / 16) << 24;
            Draw_RectClipped(fadeColor, 0.0f, 0.0f, 640.0f, 448.0f);
        } else if (*fade < 0) {
            *fade = *fade + 1;
            fadeColor = (gBrowserState->fade * 255 / 16 + 0xFF) << 24;
            Draw_RectClipped(fadeColor, 0.0f, 0.0f, 640.0f, 448.0f);
        }
    } else {
        gBrowserState->fade = 0;
    }
}

/*
 * Clips the rectangle with corners (*x0, *y0) and (*x1, *y1) to the 640 x 448
 * screen in place and returns 1, or returns 0 when it lies wholly off the
 * screen (its left edge right of the screen, its right edge left of it,
 * and so for the top and bottom). The left and top edges are clamped to
 * 0, the right to 640 and the bottom to 448. The line draws call it.
 */
s32 Rect_ClipToScreen(f32 *x0, f32 *y0, f32 *x1, f32 *y1)
{
    f32 left;
    f32 right;
    f32 top;
    f32 bottom;

    left = *x0;
    if (left < 0.0f) {
        *x0 = 0.0f;
        goto checkRight;
    }
    if (!(left <= 640.0f)) {
        return 0;
    }
checkRight:
    right = *x1;
    if (right < 0.0f) {
        return 0;
    }
    if (!(right <= 640.0f)) {
        *x1 = 640.0f;
    }
    top = *y0;
    if (top < 0.0f) {
        *y0 = 0.0f;
        goto checkBottom;
    }
    if (!(top <= 448.0f)) {
        return 0;
    }
checkBottom:
    bottom = *y1;
    if (bottom < 0.0f) {
        return 0;
    }
    if (!(bottom <= 448.0f)) {
        *y1 = 448.0f;
    }
    return 1;
}
