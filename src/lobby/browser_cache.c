/*
 * The lobby browser's loading side.
 *
 * It is built on name lists: chains of BrowserNode, each node
 * holding a URL (name), its status (enum BrowserNodeStatus) and a tag (BrowserNodeFlags:
 * the load's serial and its flags), with the free nodes at the end of the
 * chain. BrowserCache_Init sets up four lists in the work area
 * (BrowserWork): the page cache (3 BrowserPageNode, a 32 KB buffer
 * each), the image cache (32 BrowserImgNode with their textures), the
 * history (32 URLs; histCur is the page on show, the newest at the front)
 * and the request queue (20 BrowserReqNode). BrowserReq_Page, _Post,
 * _Image and the history moves queue loads; BrowserReq_UpdateAll starts
 * them on nethttp's HttpReq slots and steps them each frame;
 * BrowserReq_Poll hands a finished load to its caller as a BrowserResult.
 * BrowserForm_Add builds a form's query for a post.
 *
 * Browser_Init and LobbyKddi_CloseBrowser set up and free the lists,
 * Browser_Update runs the queue, the portal steps, Page_Load and
 * Page_CheckImages make and poll the requests.
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
#include "render_state.h"

#include "browser.h"
#include "window.h"
#include "text.h"
#include "texture.h"
#include "mem_util.h"
#include "net_sockets.h"


void *memset(void *dst, s32 c, u32 n);
void Url_Copy(char *dst, char *src);
void *memcpy(void *dst, void *src, u32 n);

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
 * 32 KB buffer and its length.
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


/*
 * Sets up the browser's lists (Browser_Init calls it): allocates and links
 * the 3 page-cache nodes with their 32 KB buffers, the 32 image
 * nodes, the eight 16 KB connection buffers (one per HttpReq slot),
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
    memset(&gBrowserWork->lastNav, 0, 24); /* lastNav, reqResult and loadSerial */
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

/* Requests, forms and the queue. */

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

/*
 * Appends name=value to the form's query (formData), after an ampersand
 * when the query is not empty. Unless the current page is Shift-JIS
 * (BTAG_SJIS), the name and value are first converted to EUC
 * (Str_SjisToEuc); the value is URL-escaped (Url_Escape). Returns the
 * query's new length, or -1 when the query would pass 4095 bytes or the
 * name or value is over 255 bytes.
 */
s32 BrowserForm_Add(char *name, char *value)
{
    char converted[256];
    s32 queryLen = strlen(gBrowserWork->formData);
    s32 nameLen = strlen(name);
    s32 valueLen = strlen(value);

    if (queryLen + nameLen + valueLen + 2 > (s32)sizeof(gBrowserWork->formData) - 1) {
        return -1;
    }
    if ((u32)valueLen > 255 || (u32)nameLen >= 256) {
        return -1;
    }
    if (queryLen != 0) {
        gBrowserWork->formData[queryLen] = 38;
        queryLen++;
    }
    if (BrowserTag_TestFlag(BrowserHist_CurFlags(), BTAG_SJIS) == 0) {
        memset(converted, 0, sizeof(converted));
        strncpy(converted, name, sizeof(converted));
        Str_SjisToEuc((u8 *)converted);
        nameLen = strlen(converted);
        memcpy(gBrowserWork->formData + queryLen, converted, nameLen);
        queryLen += nameLen;
        gBrowserWork->formData[queryLen] = 61;
        queryLen++;
        memset(converted, 0, sizeof(converted));
        strncpy(converted, value, sizeof(converted));
        Str_SjisToEuc((u8 *)converted);
        queryLen += Url_Escape((gBrowserWork->formData + queryLen), (u8 *)converted);
    } else {
        memcpy(gBrowserWork->formData + queryLen, name, nameLen);
        queryLen += nameLen;
        gBrowserWork->formData[queryLen] = 61;
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
    char fullUrl[256];
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
        HttpReq_Setup(http, page->node.name, gProxyMode, gHttpProxy, gHttpsProxy, reqNode->loadKind == BLOAD_RELOAD || reqNode->loadKind == BLOAD_REFETCH, page->buf, 32768,
                      reqNode->referer, 0, 0, 0, 0, reqNode->loadKind == BLOAD_POST, (s32)gBrowserWork->formData, 1);
        break;
    case BNODE_FAILED:
        break;
    }
}

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
    char fullUrl[256];
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
    char fullUrl[256];
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
