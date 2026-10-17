/* lobby 0x722360-0x72AC00: the browser's name lists (0x722360..), the page
 * loader's requests and the page draws (0x7238B0..). Split from
 * unk_71F8A0.c (the village's menus), which has its own copy of "%s".
 * The four files unk_722640.c, unk_722850.c, unk_723730.c and unk_7238B0.c
 * were joined: BrowserHist_CanForward and BrowserHist_CanBack keep a1
 * across NameList_Prev/NameList_NextUsed, and BrowserForm_Add and
 * ImgCache_MakeTexture call the static BrowserHist_CurFlags likewise. */
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

/* prototypes */
void Prim_DrawRect(s32, f32, f32, f32, f32);
/* end of prototypes */

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

BrowserNode *NameList_Take(BrowserNode *list, char *name)
{
    BrowserNode *n;

    for (;;) {
        n = list->next;
        if (n == NULL) {
            return NULL;
        }
        list = n;
        if (n->used == 0) {
            break;
        }
    }
    Url_Copy(n->name, name);
    n->used = 1;
    return n;
}

void NameList_Release(BrowserNode *list, BrowserNode *node)
{
    BrowserNode *prev = NULL;
    BrowserNode *n;

    for (;;) {
        n = list->next;
        if (n == NULL) {
            break;
        }
        if (n == node) {
            prev = list;
        }
        list = n;
    }
    if (prev != NULL) {
        if (node->next != NULL) {
            prev->next = node->next;
            list->next = node;
            node->next = NULL;
        }
        memset(node->name, 0, 0x100);
        node->used = 0;
        memset(&node->tag, 0, 2);
    }
}

void BrowserReq_Free(BrowserReqNode *node)
{
    NameList_Release(&gBrowserWork->reqList, &node->node);
    node->kind = 0;
    node->state = 0;
    node->url = NULL;
    node->unk114 = 0;
    node->buf = NULL;
    node->len = 0;
    node->errStep = 0;
    node->result = 0;
}

BrowserNode *NameList_Recycle(BrowserNode *list, char *name)
{
    BrowserNode *prev = NULL;
    BrowserNode *p = list;
    BrowserNode *n;

    for (;;) {
        n = p->next;
        if (n == NULL) {
            break;
        }
        prev = p;
        p = n;
    }
    p->next = list->next;
    list->next = p;
    prev->next = NULL;
    Url_Copy(p->name, name);
    p->used = 1;
    return p;
}

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

static BrowserNode *NameList_NextUsed(BrowserNode *list, BrowserNode *node)
{
    BrowserNode *n = node->next;

    if (n == NULL) {
        return node;
    }
    return n->used ? n : node;
}

/* NameList_DropFirst: the first node moved to the end of the list (cleared);
 * returns cur, or the new first node if cur was the one moved. */
BrowserNode *NameList_DropFirst(BrowserNode *list, BrowserNode *cur)
{
    BrowserNode *p = list;
    BrowserNode *n;
    BrowserNode *first;

    for (;;) {
        n = p->next;
        if (n == NULL) {
            if (list == NULL || list->next == NULL) {} /* compiled-out check */
            break;
        }
        p = n;
    }
    first = list->next;
    list->next = first->next;
    p->next = first;
    first->next = NULL;
    memset(first->name, 0, 0x100);
    first->used = 0;
    memset(&first->tag, 0, 2);
    if (cur == first) {
        cur = list->next;
    }
    return cur;
}

/* ---- was unk_722640.c ---- */

BrowserNode *NameList_DropFirst(BrowserNode *list, BrowserNode *cur);

BrowserNode *NameList_DropBefore(BrowserNode *list, BrowserNode *cur)
{
loop:
    if (list->next != cur) {
        cur = NameList_DropFirst(list, cur);
        goto loop;
    }
    return cur;
}

void BrowserHist_Commit(BrowserNode *list, BrowserReqNode *e, char *name)
{
    switch (e->kind) {
    case 4:
        gBrowserWork->histCur = NameList_Prev(list, gBrowserWork->histCur);
        break;
    case 5:
        gBrowserWork->histCur = NameList_NextUsed(list, gBrowserWork->histCur);
        break;
    case 6:
        gBrowserWork->histCur->tag = e->node.tag;
        break;
    default:
        gBrowserWork->histCur = NameList_DropBefore(list, gBrowserWork->histCur);
        gBrowserWork->histCur = NameList_Recycle(list, name);
        gBrowserWork->histCur->tag = e->node.tag;
        break;
    }
}

static void NamePair_SetFlag(BrowserNodeFlags *p, u8 bit, s32 on)
{
    if (on) {
        p->flags |= bit;
    } else {
        p->flags &= ~bit;
    }
}

static s32 NamePair_TestFlag(BrowserNodeFlags *p, s32 flag)
{
    return p->flags & (flag & 0xFFFF) & 0xFF;
}

/* ---- was unk_722850.c ---- */

s32 Url_Compare(char *a, char *b);
void ImgCache_FreeAll(BrowserNode *list);
s32 Tex_ReleaseKeep(u32);
s32 Pal_ReleaseKeep(u32);
void Mem_Free(void *handle);

BrowserNode *NameCache_Find(BrowserNode *list, char *name)
{
    BrowserNode *p;
    BrowserNode *prev;
    BrowserNode *found;
    BrowserNode *foundPrev;
    BrowserNode *n;

    p = list;
    found = NULL;
    foundPrev = NULL;
    for (;;) {
        n = p->next;
        if (n == NULL) {
            break;
        }
        prev = p;
        p = n;
        if (n->used == 0) {
            continue;
        }
        if (Url_Compare(n->name, name) != 0) {
            continue;
        }
        found = n;
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

BrowserNode *NameCache_Oldest(BrowserNode *list)
{
    BrowserNode *ret = NULL;

    if (list->next->used == 0) {
        return NULL;
    }
    for (;;) {
        list = list->next;
        if (list == NULL) {
            break;
        }
        if (list->used != 0) {
            ret = list;
        }
    }
    return ret;
}

void NameCache_Release(BrowserNode *list, BrowserNode *node)
{
    BrowserNode *prev = NULL;
    BrowserNode *n;

    for (;;) {
        n = list->next;
        if (n == NULL) {
            break;
        }
        if (n == node) {
            prev = list;
        }
        list = n;
    }
    if (prev != NULL) {
        if (node->next != NULL) {
            prev->next = node->next;
            list->next = node;
            node->next = NULL;
        }
        memset(node->name, 0, 0x100);
        node->used = 0;
        memset(&node->tag, 0, 2);
    }
}

BrowserNode *NameCache_Take(BrowserNode *list, char *name)
{
    BrowserNode *p = list;
    BrowserNode *n;
    BrowserNode *prev;

    for (;;) {
        n = p->next;
        if (n == NULL) {
            return NULL;
        }
        prev = p;
        p = n;
        if (n->used == 0) {
            break;
        }
    }
    if (prev != list) {
        prev->next = n->next;
        n->next = list->next;
        list->next = n;
    }
    Url_Copy(n->name, name);
    n->used = 1;
    return n;
}

void PageCache_Free(BrowserPageNode *node)
{
    NameCache_Release(&gBrowserWork->pageList, &node->node);
    memset(node->buf, 0, 0x8000);
    node->unk110 = 0;
}

void ImgCache_Free(BrowserImgNode *node)
{
    NameCache_Release(&gBrowserWork->imgList, &node->node);
    if (node->handles != 0) {
        Tex_ReleaseKeep(node->handles & 0xFFFF);
        if ((node->handles & 0xFFFF0000) >> 16 != 0) {
            Pal_ReleaseKeep((node->handles & 0xFFFF0000) >> 16);
        }
    }
    node->handles = 0;
    node->data = 0;
    node->size = 0;
    node->w = 0;
    node->h = 0;
}

void *Mem_Alloc(u32 size, s32 flag);
void *Mem_GetAddr(void *handle);
void Url_ClearBase(void);
void BrowserForm_Clear(void);


void BrowserCache_Init(void)
{
    s32 i;
    u8 *buf;
    BrowserPageNode *p;
    BrowserImgNode *q;
    BrowserNode *h;
    BrowserReqNode *r;

    gBrowserWork->memPages = Mem_Alloc(0x33C, 1);
    gBrowserWork->pages = (BrowserPageNode *)Mem_GetAddr(gBrowserWork->memPages);
    memset(gBrowserWork->pages, 0, 0x33C);
    gBrowserWork->pageList.next = &gBrowserWork->pages->node;
    p = gBrowserWork->pages;
    p[0].node.next = &p[1].node;
    p = gBrowserWork->pages;
    p[1].node.next = &p[2].node;
    gBrowserWork->memBufs = Mem_Alloc(0x18000, 1);
    buf = Mem_GetAddr(gBrowserWork->memBufs);
    gBrowserWork->pages[0].buf = buf;
    gBrowserWork->pages[1].buf = buf + 0x8000;
    gBrowserWork->pages[2].buf = buf + 0x10000;

    gBrowserWork->memImgs = Mem_Alloc(0x2380, 1);
    gBrowserWork->imgs = (BrowserImgNode *)Mem_GetAddr(gBrowserWork->memImgs);
    memset(gBrowserWork->imgs, 0, 0x2380);
    gBrowserWork->imgList.next = &gBrowserWork->imgs->node;
    for (i = 0; i < 31; i++) {
        q = gBrowserWork->imgs;
        q[i].node.next = &q[i + 1].node;
    }

    gBrowserWork->memConnBufs = Mem_Alloc(0x20000, 1);
    buf = Mem_GetAddr(gBrowserWork->memConnBufs);
    gBrowserWork->connBuf[0] = buf;
    gBrowserWork->connBuf[1] = buf + 0x4000;
    gBrowserWork->connBuf[2] = buf + 0x8000;
    gBrowserWork->connBuf[3] = buf + 0xC000;
    gBrowserWork->connBuf[4] = buf + 0x10000;
    gBrowserWork->connBuf[5] = buf + 0x14000;
    gBrowserWork->connBuf[6] = buf + 0x18000;
    gBrowserWork->connBuf[7] = buf + 0x1C000;
    memset(&gBrowserWork->dl, 0, 0x114);

    gBrowserWork->memHist = Mem_Alloc(0x2180, 1);
    gBrowserWork->hist = (BrowserNode *)Mem_GetAddr(gBrowserWork->memHist);
    memset(gBrowserWork->hist, 0, 0x2180);
    gBrowserWork->histList.next = gBrowserWork->hist;
    gBrowserWork->histCur = gBrowserWork->hist;
    for (i = 0; i < 31; i++) {
        h = gBrowserWork->hist;
        h[i].next = &h[i + 1];
    }

    gBrowserWork->memReqs = Mem_Alloc(0x1720, 1);
    gBrowserWork->reqs = (BrowserReqNode *)Mem_GetAddr(gBrowserWork->memReqs);
    memset(gBrowserWork->reqs, 0, 0x1720);
    gBrowserWork->reqList.next = &gBrowserWork->reqs->node;
    for (i = 0; i < 19; i++) {
        r = gBrowserWork->reqs;
        r[i].node.next = &r[i + 1].node;
    }
    memset(&gBrowserWork->reqState, 0, 0x18);
    Url_ClearBase();
    BrowserForm_Clear();
}

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

char *BrowserHist_Forward(void)
{
    BrowserNode *cur;
    BrowserReqNode *e;

    cur = NameList_Prev(&gBrowserWork->histList, gBrowserWork->histCur);
    e = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, cur->name);
    e->node.used = 4;
    if (cur == gBrowserWork->histCur) {
        e->kind = 6;
        e->state = 1;
        e->url = gBrowserWork->histCur->name;
        e->node.tag = cur->tag;
        gBrowserWork->reqState = 5;
    } else {
        e->kind = 4;
        e->state = 1;
        e->url = gBrowserWork->histCur->name;
        e->node.tag = cur->tag;
        gBrowserWork->reqState = 3;
    }
    return cur->name;
}

s32 BrowserHist_CanForward(void)
{
    BrowserNode *cur = gBrowserWork->histCur;

    return cur != NameList_Prev(&gBrowserWork->histList, cur);
}

char *BrowserHist_Back(void)
{
    BrowserNode *cur;
    BrowserReqNode *e;

    cur = NameList_NextUsed(&gBrowserWork->histList, gBrowserWork->histCur);
    e = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, cur->name);
    e->node.used = 4;
    if (cur == gBrowserWork->histCur) {
        e->kind = 6;
        e->state = 1;
        e->url = gBrowserWork->histCur->name;
        e->node.tag = cur->tag;
        gBrowserWork->reqState = 5;
    } else {
        e->kind = 5;
        e->state = 1;
        e->url = cur->name;
        e->node.tag = cur->tag;
        gBrowserWork->reqState = 4;
    }
    return cur->name;
}

s32 BrowserHist_CanBack(void)
{
    BrowserNode *cur = gBrowserWork->histCur;

    return cur != NameList_NextUsed(&gBrowserWork->histList, cur);
}

/* ---- was unk_723730.c ---- */

/* Static in the original: BrowserReq_Reload keeps a1 across the call. */
static BrowserNodeFlags *BrowserHist_CurFlags(void)
{
    return &gBrowserWork->histCur->tag;
}

char *BrowserReq_Reload(void)
{
    BrowserReqNode *e;

    e = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, gBrowserWork->histCur->name);
    e->node.used = 4;
    e->kind = 6;
    e->state = 1;
    e->url = gBrowserWork->histCur->name;
    e->node.tag.serial = ++gBrowserWork->serial;
    e->node.tag.flags = BrowserHist_CurFlags()->flags;
    gBrowserWork->reqState = 5;
    return gBrowserWork->histCur->name;
}

s32 BrowserReq_CanReload(void) {
    if (NamePair_TestFlag(BrowserHist_CurFlags(), 1) != 0) {
        return 0;
    }
    NamePair_TestFlag(BrowserHist_CurFlags(), 2);
    return 1;
}

char *BrowserHist_CurUrl(void)
{
    return gBrowserWork->histCur->name;
}

/* ---- was unk_7238B0.c ---- */

s32 Url_FileType(char *name);
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap);
void Font_Printf(const char *fmt, ...);
void Prim_DrawTri(u32 color, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2);
void Prim_DrawSprite(u32 color, u32 tex, s16 x1, s16 y1, s16 x2, s16 y2, s16 u1, s16 v1, s16 u2, s16 v2);

/* The BrowserReq_* requests take the address of the URL pointer: &gBrowserWork->url for a page
 * or a post, a local for an image. */
void BrowserReq_Page(char **url)
{
    BrowserReqNode *e;

    e = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, *url);
    e->node.used = 4;
    e->kind = 1;
    e->state = 1;
    e->url = BrowserHist_CurUrl();
    e->node.tag.serial = ++gBrowserWork->serial;
    e->node.tag.flags = 0;
    gBrowserWork->reqState = 1;
}

void BrowserReq_Post(char **url)
{
    BrowserReqNode *e;

    e = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, *url);
    e->node.used = 4;
    e->kind = 3;
    e->state = 1;
    e->url = BrowserHist_CurUrl();
    e->node.tag.serial = gBrowserWork->serial += 1;
    e->node.tag.flags = 0;
    NamePair_SetFlag(&e->node.tag, 1, 1);
    gBrowserWork->reqState = 2;
}

s32 strlen(const char *s);
char *strncpy(char *dst, const char *src, u32 n); /* main: strncpy */
void Str_SjisToEuc(u8 *s);
s32 Url_Escape(char *dst, u8 *src);

s32 BrowserForm_Add(char *name, char *value)
{
    char buf[0x100];
    s32 n = strlen(gBrowserWork->formData);
    s32 nl = strlen(name);
    s32 vl = strlen(value);

    if (n + nl + vl + 2 > 0xFFF) {
        return -1;
    }
    if ((u32)vl > 0xFF || (u32)nl >= 0x100) {
        return -1;
    }
    if (n != 0) {
        gBrowserWork->formData[n] = 0x26;
        n++;
    }
    if (NamePair_TestFlag(BrowserHist_CurFlags(), 4) == 0) {
        memset(buf, 0, 0x100);
        strncpy(buf, name, 0x100);
        Str_SjisToEuc((u8 *)buf);
        nl = strlen(buf);
        memcpy(gBrowserWork->formData + n, buf, nl);
        n += nl;
        gBrowserWork->formData[n] = 0x3D;
        n++;
        memset(buf, 0, 0x100);
        strncpy(buf, value, 0x100);
        Str_SjisToEuc((u8 *)buf);
        n += Url_Escape((gBrowserWork->formData + n), (u8 *)buf);
    } else {
        memcpy(gBrowserWork->formData + n, name, nl);
        n += nl;
        gBrowserWork->formData[n] = 0x3D;
        n++;
        n += Url_Escape((gBrowserWork->formData + n), (u8 *)value);
    }
    return n;
}

void BrowserForm_Clear(void)
{
    memset(gBrowserWork->formData, 0, 0x1000);
}

void BrowserReq_Image(char **url)
{
    BrowserReqNode *e;

    e = (BrowserReqNode *)NameList_Take(&gBrowserWork->reqList, *url);
    e->node.used = 4;
    e->kind = 1;
    e->state = 2;
    e->url = BrowserHist_CurUrl();
    e->node.tag = *BrowserHist_CurFlags();
    if ((u8)Url_FileType(*url) == 0) {
        e->node.used = 6;
        e->result = 2;
        e->errStep = 0;
        return;
    }
}

/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Url_GetScheme(const char *url);

BrowserPageNode *PageCache_Get(char *url, s32 kind, BrowserNodeFlags *pair)
{
    BrowserPageNode *n;

    switch ((u8)Url_GetScheme(url)) {
    case 4:
        if (kind == 1) {
            kind = 2;
        }
        break;
    }
    n = (BrowserPageNode *)NameCache_Find(&gBrowserWork->pageList, url);
    switch (kind) {
    case 4:
    case 5:
        if (n == NULL && NamePair_TestFlag(pair, 1)) {
            return NULL;
        }
    case 1:
        if (n != NULL) {
            n->node.used = 2;
            return n;
        }
        goto make;
    case 6:
        if (NamePair_TestFlag(pair, 1)) {
            return NULL;
        }
    case 0:
    case 2:
    case 3:
    default:
    make:
        if (n == NULL) {
            n = (BrowserPageNode *)NameCache_Take(&gBrowserWork->pageList, url);
            if (n == NULL) {
                PageCache_Free((BrowserPageNode *)NameCache_Oldest(&gBrowserWork->pageList));
                n = (BrowserPageNode *)NameCache_Take(&gBrowserWork->pageList, url);
            }
        }
        n->node.used = 3;
        return n;
    }
}

BrowserNode *ImgCache_Get(char *name, s32 unused, BrowserNodeFlags *pair)
{
    BrowserNode *n;

    n = NameCache_Find(&gBrowserWork->imgList, name);
    if (n != NULL) {
        if (n->used == 5) {
            if (gBrowserWork->reqState == 5 && n->tag.serial != pair->serial) {
                ImgCache_Free((BrowserImgNode *)n);
                return NULL;
            }
            n->used = 2;
            return n;
        }
        return NULL;
    }
    if (n == NULL) {
        n = NameCache_Take(&gBrowserWork->imgList, name);
        if (n == NULL) {
            ImgCache_Free((BrowserImgNode *)NameCache_Oldest(&gBrowserWork->imgList));
            n = NameCache_Take(&gBrowserWork->imgList, name);
        }
    }
    n->used = 3;
    return n;
}

void ImgCache_FreeAll(BrowserNode *list)
{
    BrowserNode *n;

    n = NameCache_Oldest(list);
    if (n != NULL) {
        do {
            ImgCache_Free((BrowserImgNode *)n);
            n = NameCache_Oldest(list);
        } while (n != NULL);
    }
}

s32 Png_CreateTexture(u8 *data, s32 size, s32 mode, u16 *w, u16 *h);
s32 Bmp_CreateTexture(u8 *bmp, s32 mode, u16 *w, u16 *h);

s32 ImgCache_MakeTexture(BrowserImgNode *e)
{
    s32 r;
    BrowserNode *old;

retry:
    r = -3;
    switch ((u8)Url_FileType(e->node.name)) {
    case 2:
        r = Png_CreateTexture((u8 *)e->data, e->size, 3, (u16 *)&e->w, (u16 *)&e->h);
        break;
    case 3:
        r = Bmp_CreateTexture((u8 *)e->data, 3, (u16 *)&e->w, (u16 *)&e->h);
        break;
    }
    switch (r) {
    case -1:
        old = NameCache_Oldest(&gBrowserWork->imgList);
        if (old == NULL) {
            e->node.used = 6;
            e->handles = 0;
            return 8;
        }
        if (old->tag.serial == BrowserHist_CurFlags()->serial) {
            e->node.used = 6;
            e->handles = 0;
            return 8;
        }
        ImgCache_Free((BrowserImgNode *)old);
        goto retry;
    case -2:
    case -3:
        e->node.used = 6;
        e->handles = 0;
        return 8;
    default:
        e->handles = r;
        return 0;
    }
}

void BrowserReq_OnDone(BrowserReqNode *e)
{
    HttpReq *c = e->req;

    if (c->used != 2) {
        return;
    }
    switch (c->err) {
    case 0:
        e->node.used = 5;
        e->buf = c->buf;
        e->len = c->len;
        e->result = 0;
        e->errStep = c->errStep;
        if (c->toSjis != 0) {
            if (c->sjis != 0) {
                NamePair_SetFlag(&e->node.tag, 4, 1);
            } else {
                NamePair_SetFlag(&e->node.tag, 4, 0);
            }
        }
        if (c->httpErr != 0) {
            e->result = 14;
        }
        break;
    case 4:
        e->node.used = 6;
        e->result = 4;
        e->errStep = c->errStep;
        break;
    case 5:
        e->node.used = 6;
        e->result = 5;
        e->errStep = c->errStep;
        break;
    case 6:
        e->node.used = 6;
        e->result = 6;
        e->errStep = c->errStep;
        break;
    case 1:
    case 2:
    case 3:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 16:
        e->node.used = 6;
        e->result = 2;
        e->errStep = c->errStep;
        break;
    case 15:
        e->node.used = 6;
        e->result = 1;
        e->errStep = c->errStep;
        break;
    case 17:
        e->node.used = 6;
        e->result = 9;
        e->errStep = c->errStep;
        break;
    case 18:
        e->node.used = 6;
        e->buf = c->buf;
        e->result = 11;
        e->errStep = c->errStep;
        break;
    case 19:
        e->node.used = 6;
        e->result = 12;
        e->errStep = c->errStep;
        break;
    case 20:
        e->node.used = 6;
        e->result = 13;
        e->errStep = c->errStep;
        break;
    }
}


void NameConn_Setup(HttpReq *c, char *name, s8 arg2, char *arg3, char *arg4, s8 arg5, u8 *buf, s32 size,
                   char *url, s32 arg9, s32 arg10, s32 arg11, s32 arg12, s8 arg13, s32 arg14, s8 arg15)
{
    c->secure = 1;
    c->url = name;
    c->proxyFlags = arg2;
    c->proxyUrl = arg3;
    c->sslProxyUrl = arg4;
    c->noCache = arg5;
    c->buf = (char *) buf;
    c->bufSize = size;
    if (url == NULL) {
        c->referer = NULL;
    } else {
        switch ((u8)Url_GetScheme(url)) {
        case 2:
        case 3:
            c->referer = url;
            break;
        case 1:
        case 4:
        case 5:
        default:
            c->referer = NULL;
            break;
        }
    }
    c->user = (char *) arg9;
    c->pass = (char *) arg10;
    c->proxyUser = (char *) arg11;
    c->proxyPass = (char *) arg12;
    if (arg13 != 0) {
        c->method = 2;
        c->postData = (char *) arg14;
    } else {
        c->method = 1;
        c->postData = NULL;
    }
    c->toSjis = arg15;
    c->cancel = 0;
}

char *Url_Resolve(char *base, char *url);
s32 HttpReq_AllIdle(void);
HttpReq *HttpReq_Alloc(void);
extern char gHttpProxy[];
extern char gHttpsProxy[];

void BrowserReq_StartPage(BrowserReqNode *e)
{
    char url[0x100];
    BrowserPageNode *n;
    HttpReq *c;

    Url_Copy(url, e->node.name);
    if (Url_Resolve(BrowserHist_CurUrl(), url) == NULL) {
        e->node.used = 6;
        e->result = 2;
        e->errStep = 0;
        e->unk114 = 0;
        return;
    }
    if (HttpReq_AllIdle() == 0) {
        return;
    }
    n = PageCache_Get(url, e->kind, &e->node.tag);
    if (n == NULL) {
        e->node.used = 6;
        e->result = 10;
        e->errStep = 0;
        return;
    }
    e->unk114 = (s32)n;
    switch (n->node.used) {
    case 2:
        e->node.used = 5;
        e->req = NULL;
        break;
    case 3:
        e->node.used = 3;
        e->req = c = HttpReq_Alloc();
        NameConn_Setup(c, n->node.name, gProxyMode, gHttpProxy, gHttpsProxy, e->kind == 6 || e->kind == 2, n->buf, 0x8000,
                      e->url, 0, 0, 0, 0, e->kind == 3, (s32)gBrowserWork->formData, 1);
        break;
    case 6:
        break;
    }
}

s32 HttpReq_HasFree(void);

void BrowserReq_StartImage(BrowserReqNode *e)
{
    char url[0x100];
    BrowserNode *n;
    HttpReq *c;

    Url_Copy(url, e->node.name);
    if (Url_Resolve(BrowserHist_CurUrl(), url) == NULL) {
        e->node.used = 6;
        e->result = 2;
        e->errStep = 0;
        e->unk114 = 0;
        return;
    }
    switch ((u8)Url_GetScheme(url)) {
    case 2:
    case 3:
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
    n = ImgCache_Get(url, e->kind, &e->node.tag);
    if (n == NULL) {
        return;
    }
    e->unk114 = (s32)n;
    switch (n->used) {
    case 2:
        e->node.used = 5;
        e->req = NULL;
        break;
    case 3:
        e->node.used = 3;
        e->req = c = HttpReq_Alloc();
        NameConn_Setup(c, n->name, gProxyMode, gHttpProxy, gHttpsProxy, gBrowserWork->reqState == 5,
                      ((u8 **)gBrowserWork->connBuf)[c->index], 0x4000, e->url, 0, 0, 0, 0, 0, 0, 0); /* the (u8 **) view adds the index first (addu), as the original */
        break;
    case 6:
        break;
    }
}

void BrowserReq_StartDownload(BrowserReqNode *e)
{
    char url[0x100];
    BrowserPageNode *d;
    HttpReq *c;

    Url_Copy(url, e->node.name);
    if (Url_Resolve(BrowserHist_CurUrl(), url) == NULL) {
        e->node.used = 6;
        e->result = 2;
        e->errStep = 0;
        e->unk114 = 0;
        return;
    }
    if (HttpReq_AllIdle() == 0) {
        return;
    }
    memset(&gBrowserWork->dl, 0, 0x114);
    d = &gBrowserWork->dl;
    Url_Copy(d->node.name, url);
    d->node.used = 3;
    d->buf = (u8 *)e->buf;
    d->unk110 = e->len;
    e->unk114 = (s32)d;
    e->node.used = 3;
    e->req = c = HttpReq_Alloc();
    NameConn_Setup(c, d->node.name, gProxyMode, gHttpProxy, gHttpsProxy, gBrowserWork->reqState == 5, d->buf,
                  d->unk110, e->url, 0, 0, 0, 0, 0, 0, 0);
}

void BrowserReq_OnDone(BrowserReqNode *e);
void BrowserReq_StartPage(BrowserReqNode *e);
void BrowserReq_StartImage(BrowserReqNode *e);
void BrowserReq_StartDownload(BrowserReqNode *e);

void BrowserReq_UpdateAll(void)
{
    BrowserReqNode *e = (BrowserReqNode *)&gBrowserWork->reqList;

    for (;;) {
        e = (BrowserReqNode *)e->node.next;
        if (e == NULL) {
            break;
        }
        if (e->node.used == 0) {
            break;
        }
        switch (e->node.used) {
        case 3:
            e->req->run(e->req);
            BrowserReq_OnDone(e);
            break;
        case 4:
            switch (e->state) {
            case 1:
                BrowserReq_StartPage(e);
                break;
            case 2:
                BrowserReq_StartImage(e);
                break;
            case 3:
                BrowserReq_StartDownload(e);
                break;
            }
            break;
        }
    }
}

s32 HttpReq_Free(HttpReq *c);
char *Browser_GetText(u32 group, s32 i);
void net_clear_host_cache(void);

BrowserResult *BrowserReq_Poll(char **url)
{
    BrowserReqNode *e = (BrowserReqNode *)&gBrowserWork->reqList;
    BrowserPageNode *n;
    BrowserImgNode *img;

    for (;;) {
        e = (BrowserReqNode *)e->node.next;
        if (e == NULL) {
            break;
        }
        if (e->node.used == 0) {
            break;
        }
        if (Url_Compare(e->node.name, *url) != 0) {
            continue;
        }
        switch (e->node.used) {
        case 5:
            switch (e->state) {
            case 1:
                n = (BrowserPageNode *)e->unk114;
                if (n->node.used == 3) {
                    n->unk110 = e->len;
                    n->node.tag = e->node.tag;
                    HttpReq_Free(e->req);
                    e->req = NULL;
                }
                BrowserHist_Commit(&gBrowserWork->histList, e, n->node.name);
                n->node.used = 5;
                gBrowserWork->reqResult.data = n->buf;
                gBrowserWork->reqResult.code = e->result;
                gBrowserWork->reqResult.w = 0;
                gBrowserWork->reqResult.h = 0;
                gBrowserWork->reqResult.size = n->unk110;
                break;
            case 2:
                img = (BrowserImgNode *)e->unk114;
                if (img->node.used == 3) {
                    img->data = e->buf;
                    img->size = e->len;
                    img->node.tag = e->node.tag;
                    e->result = ImgCache_MakeTexture(img);
                    if (e->result != 0) {
                        e->node.used = 6;
                        e->errStep = 0;
                        return NULL;
                    }
                    HttpReq_Free(e->req);
                    e->req = NULL;
                } else {
                    img->node.tag = e->node.tag;
                }
                img->node.used = 5;
                gBrowserWork->reqResult.data = &img->handles;
                gBrowserWork->reqResult.code = e->result;
                gBrowserWork->reqResult.w = img->w;
                gBrowserWork->reqResult.h = img->h;
                gBrowserWork->reqResult.size = img->size;
                break;
            case 3:
                n = (BrowserPageNode *)e->unk114;
                if (n->node.used == 3) {
                    n->unk110 = e->len;
                    HttpReq_Free(e->req);
                    e->req = NULL;
                }
                n->node.used = 5;
                gBrowserWork->reqResult.data = n->buf;
                gBrowserWork->reqResult.code = e->result;
                gBrowserWork->reqResult.w = 0;
                gBrowserWork->reqResult.h = 0;
                gBrowserWork->reqResult.size = n->unk110;
                break;
            }
            BrowserReq_Free(e);
            return &gBrowserWork->reqResult;
        case 6:
            switch (e->state) {
            case 1:
                n = (BrowserPageNode *)e->unk114;
                switch (e->result) {
                case 4:
                case 5:
                case 6:
                    BrowserHist_Commit(&gBrowserWork->histList, e, n->node.name);
                    switch (e->errStep) {
                    case 0:
                    case 1:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 1), 0x200);
                        break;
                    case 2:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 2), 0x200);
                        break;
                    case 3:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 3), 0x200);
                        break;
                    case 4:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 4), 0x200);
                        net_clear_host_cache();
                        break;
                    case 5:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 5), 0x200);
                        break;
                    case 6:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 6), 0x200);
                        break;
                    case 7:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 7), 0x200);
                        break;
                    case 8:
                        strncpy(gBrowserWork->errorText, Browser_GetText(2, 8), 0x200);
                        break;
                    }
                    gBrowserWork->reqResult.data = gBrowserWork->errorText;
                    break;
                case 11:
                    Url_Copy(gBrowserWork->urlLower, (char *)n->buf);
                    gBrowserWork->reqResult.data = gBrowserWork->urlLower;
                    break;
                case 10:
                    BrowserHist_Commit(&gBrowserWork->histList, e, NULL);
                    strncpy(gBrowserWork->errorText, Browser_GetText(2, 0), 0x200);
                    gBrowserWork->reqResult.data = gBrowserWork->errorText;
                    break;
                default:
                    gBrowserWork->reqResult.data = NULL;
                    break;
                }
                gBrowserWork->reqResult.code = e->result;
                if (n != NULL) {
                    PageCache_Free(n);
                }
                break;
            case 2:
                img = (BrowserImgNode *)e->unk114;
                gBrowserWork->reqResult.data = NULL;
                gBrowserWork->reqResult.code = e->result;
                if (img != NULL) {
                    if (e->result != 9) {
                        ImgCache_Free(img);
                    } else if (img->handles != 0) {
                        img->node.used = 5;
                    } else {
                        ImgCache_Free(img);
                    }
                }
                break;
            case 3:
                gBrowserWork->reqResult.data = NULL;
                gBrowserWork->reqResult.code = e->result;
                break;
            }
            if (e->req != NULL) {
                HttpReq_Free(e->req);
                e->req = NULL;
            }
            BrowserReq_Free(e);
            return &gBrowserWork->reqResult;
        }
    }
    return NULL;
}

void BrowserHist_Cancel(void)
{
    switch (gBrowserWork->reqState) {
    case 0:
    case 5:
        break;
    case 1:
    case 2:
        gBrowserWork->histCur = NameList_NextUsed(&gBrowserWork->histList, gBrowserWork->histCur);
        gBrowserWork->histCur = NameList_DropBefore(&gBrowserWork->histList, gBrowserWork->histCur);
        break;
    case 3:
        gBrowserWork->histCur = NameList_NextUsed(&gBrowserWork->histList, gBrowserWork->histCur);
        break;
    case 4:
        gBrowserWork->histCur = NameList_Prev(&gBrowserWork->histList, gBrowserWork->histCur);
        break;
    }
}

void BrowserReq_CancelAll(void)
{
    BrowserReqNode *e = (BrowserReqNode *)&gBrowserWork->reqList;

    for (;;) {
        e = (BrowserReqNode *)e->node.next;
        if (e == NULL) {
            break;
        }
        if (e->node.used == 0) {
            break;
        }
        switch (e->node.used) {
        case 6:
            break;
        case 5:
        case 4:
            e->node.used = 6;
            e->result = 9;
            e->errStep = 0;
            break;
        case 3:
            e->req->cancel = 1;
            break;
        }
    }
}

/* The colour is the first parameter, as in Draw_VLineClipped (round 23: the callers'
 * argument set-up matches only this way). */
void Draw_RectClipped(s32 color, f32 fparg0, f32 fparg1, f32 fparg2, f32 fparg3)
{
    f32 x1;
    f32 y1;
    f32 x2;
    f32 y2;

    x1 = fparg0;
    y1 = fparg1;
    x2 = fparg2;
    y2 = fparg3;
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

void Page_DrawMark(u8 kind, f32 x, f32 y)
{
    switch (kind) {
    case 0:
        Prim_DrawSprite(-1, PageTex_Get(0), -3.0f + x, -3.0f + y, 0x20, 0x20, 0xC1, 0x41, 0xE0, 0x60);
        break;
    case 1:
        Prim_DrawSprite(-1, PageTex_Get(0), -3.0f + x, -3.0f + y, 0x20, 0x20, 0xA1, 0x41, 0xC0, 0x60);
        break;
    case 24:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xE0, 0xC0, 0xF0, 0xD0);
        break;
    case 25:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xF0, 0xC0, 0x100, 0xD0);
        break;
    case 26:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xE0, 0xD0, 0xF0, 0xE0);
        break;
    case 27:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 0x14, 0x14, 0xF0, 0xD0, 0x100, 0xE0);
        break;
    }
}

void Page_DrawWidget(u8 kind, f32 x, f32 y, f32 w, f32 h, s32 color)
{
    if (color == 0) {
        color = -1;
    }
    switch (kind) {
    case 2:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x56, 0x1E, 0x40, 0xA2, 0x96, 0xC0);
        break;
    case 3:
        Draw_RectClipped(0x80808080, x, y, 169.0f + x, 30.0f + y);
        break;
    case 4:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0xD0, 0x20, 0, 0xC0, 0xD0, 0xE0);
        break;
    case 5:
        Prim_DrawSprite(color, PageTex_Get(1), x, y, 0xCA, 0x20, 0, 0xC0, 0xCA, 0xE0);
        break;
    case 9:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0x40, 0x20, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0x40, 0x40, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, 0x14, 0x10, 0x40, 0x20, 0x60);
        break;
    case 8:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0x60, 0x20, 0x80);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0x60, 0x40, 0x80);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, 0x14, 0x10, 0x60, 0x20, 0x80);
        break;
    case 7:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0x80, 0x20, 0xA0);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0x80, 0x40, 0xA0);
        Prim_DrawSprite(color, PageTex_Get(0), 4.0f + x, y, w - 4.0f, 0x14, 0x10, 0x80, 0x20, 0xA0);
        break;
    case 6:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x14, 0x14, 0x0, 0xA0, 0x20, 0xC0);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 0x14, 0x14, 0x20, 0xA0, 0x40, 0xC0);
        Prim_DrawSprite(color, PageTex_Get(0), x - 1.0f, y, w - 1.0f, 0x14, 0x10, 0xA0, 0x20, 0xC0);
        break;
    case 10:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0x40, 0x100, 0x60);
        break;
    case 11:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0x60, 0x100, 0x80);
        break;
    case 12:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0x80, 0x100, 0xA0);
        break;
    case 13:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0xE0, 0xA0, 0x100, 0xC0);
        break;
    case 14:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0x80, 0xE0, 0x60);
        break;
    case 15:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0x60, 0xE0, 0x40);
        break;
    case 16:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0xC0, 0xE0, 0xA0);
        break;
    case 17:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x100, 0xA0, 0xE0, 0x80);
        break;
    case 22:
        Page_DrawWidget(7, x, y, w, h, 0);
        Page_DrawWidget(11, x + w - 16.0f, 1.0f + y, 16.0f, h, 0);
        break;
    case 23:
        Page_DrawWidget(7, x, y, w, h, 0);
        Page_DrawWidget(15, x + w - 16.0f, 1.0f + y, 16.0f, h, 0);
        break;
    case 19:
        if (h < 32.0f) {
            Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x60, 0x40, 0x80, 0x60);
            break;
        }
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, 0x10, 0x60, 0x40, 0x80, 0x4F);
        Prim_DrawSprite(color, PageTex_Get(0), x, y + h - 16.0f, w, 0x10, 0x60, 0x50, 0x80, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), x, 16.0f + y, w, h - 32.0f, 0x60, 0x48, 0x80, 0x57);
        break;
    case 18:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x80, 0x40, 0xA0, 0x60);
        break;
    case 21:
        if (w < 32.0f) {
            Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x60, 0x40, 0x80, 0x60);
            break;
        }
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 0x10, h, 0x60, 0x40, 0x6F, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 16.0f, y, 0x10, h, 0x70, 0x40, 0x80, 0x60);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, h, 0x68, 0x40, 0x77, 0x60);
        break;
    case 20:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 0x40, 0x40, 0x60, 0x60);
        break;
    case 0:
    case 1:
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_722360", D_00838E30);

void Page_DrawIcon(u16 i, f32 x, f32 y, f32 w, f32 h, s32 u, s32 v)
{
    Prim_DrawSprite(-1, gBrowserWork->texHandle[i], x, y, w, h, 0, 0,
                  (s16)gBrowserWork->texW[i], (s16)gBrowserWork->texH[i]);
}

struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha);
/* Draws an empty list window (ListPanel_DrawAlpha) over a rectangle, in 20x20 cells. */
void TileWin_Draw(f32 x0, f32 y0, f32 x1, f32 y1)
{
    ListWin w;

    w.x = x0;
    w.y = y0;
    w.charH = 20;
    w.charW = 20;
    w.lineChars = (s32)(x1 - x0) / 20;
    w.lines = (s32)(y1 - y0) / 20;
    w.color = 0;
    w.flags = 4;
    w.items = NULL;
    w.cursorColor = 0;
    w.color2 = 0;
    ListPanel_DrawAlpha(&w, 0, -1, 0xFF);
}

void Draw_HLineClipped(s32 a, f32 x, f32 y, f32 z, f32 w)
{
    if (Rect_ClipToScreen(&x, &y, &z, &w) != 0) {
        Prim_DrawRect(a, x, y, 1.0f + z, w);
    }
}

void Draw_VLineClipped(s32 a, f32 x, f32 y, f32 z, f32 w)
{
    if (Rect_ClipToScreen(&x, &y, &z, &w) != 0) {
        Prim_DrawRect(a, x, y, z, 1.0f + w);
    }
}

/* A page object (browser.h's BrowserObj: the fields the draws read). */
typedef struct PageElem {
    /* 0x00 */ u8 unk0[1];
    /* 0x01 */ s8 unk1;             /* 1: drawn this frame (Page_DrawObj) */
    /* 0x02 */ u8 kind;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 size;             /* the text size */
    /* 0x05 */ u8 style;
    /* 0x06 */ u8 unk6;
    /* 0x07 */ s8 focus;            /* 1: the cursor is on it */
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u8 unkE[0x26];
    /* 0x34 */ f32 x;
    /* 0x38 */ f32 y;
    /* 0x3C */ u8 unk3C[4];
    /* 0x40 */ f32 w;
    /* 0x44 */ f32 h;
    /* 0x48 */ u8 unk48[0x10];
    /* 0x58 */ s32 color;           /* < 0: none */
    /* 0x5C */ u8 unk5C;
    /* 0x5D */ u8 field;
    /* 0x5E */ u8 unk5E;
    /* 0x5F */ u8 unk5F;
    /* 0x60 */ char *text;
    /* 0x64 */ char *text2;
    /* 0x68 */ char *text3;
} PageElem;

void Page_DrawFrame(s32 color, s32 w, f32 x0, f32 y0, f32 x1, f32 y1)
{
    BrowserState *s = gBrowserState;
    s16 l = s->marginLeft;
    s16 t;

    if (x0 < l) {
        x0 = l;
    }
    t = s->marginTop;
    if (y0 < t) {
        y0 = t;
    }
    if (x1 > 0x280 - s->marginRight) {
        x1 = (s16)(0x280 - s->marginRight);
    }
    if (y1 > 0x1C0 - s->marginBottom) {
        y1 = (s16)(0x1C0 - s->marginBottom);
    }
    if ((s->style & 4) >> 2 && (s->style & 2) >> 1) {
        if (x0 > l + 0x247) {
            return;
        }
        if (y0 > t + 0x17B) {
            return;
        }
        if (x1 > l + 0x248) {
            x1 = (s16)(l + 0x248);
        }
        if (y1 > t + 0x17C) {
            y1 = (s16)(t + 0x17C);
        }
    } else {
        if (x0 > l + 0x247) {
            return;
        }
        if (y0 > t + 0x17B) {
            return;
        }
        if (x1 > l + 0x248) {
            x1 = (s16)(l + 0x248);
        }
        if (y1 > t + 0x17C) {
            y1 = (s16)(t + 0x17C);
        }
    }
    Prim_DrawRect(color, x0, y0, x1, y0 + w);
    Prim_DrawRect(color, x1 - w, y0, x1, y1);
    Prim_DrawRect(color, x0, y1 - w, x1, y1);
    Prim_DrawRect(color, x0, y0, x0 + w, y1);
}

void Page_DrawThinFrame(s32 color, f32 x0, f32 y0, f32 x1, f32 y1)
{
    BrowserState *s = gBrowserState;

    if (x0 < s->marginLeft) {
        x0 = s->marginLeft;
    }
    if (y0 < s->marginTop) {
        y0 = s->marginTop;
    }
    if (x1 > 0x280 - s->marginRight) {
        x1 = (s16)(0x280 - s->marginRight);
    }
    if (y1 > 0x1C0 - s->marginBottom) {
        y1 = (s16)(0x1C0 - s->marginBottom);
    }
    Prim_DrawRect(color, x0, y0, x1, 1.0f + y0);
    Prim_DrawRect(color, x1 - 1.0f, y0, x1, y1);
    Prim_DrawRect(color, x0, y1 - 1.0f, x1, y1);
    Prim_DrawRect(color, x0, y0, 1.0f + x0, y1);
}

void Gfx_SetTexFilter(s32 a);
void Font_SetPal(s32 a);
void Font_DrawAndClearPages(void);
char *strchr(char *s, s32 c); /* main: strchr */
extern char D_00838E30[];
void Page_Printf(const char *fmt, ...);

void Page_DrawTextLine(u8 a0, u32 color, u8 a2, u8 size, f32 x, f32 y, char *text)
{
    char *p;

    if (*(u8 *)text == 0 || text == D_00838E30) {
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
            p = strchr(text, 0x25);
            if (p == NULL) {
                break;
            }
            *p = 0x20;
        }
        Page_Printf(text);
        Font_DrawAndClearPages();
    }
}

void Page_Printf(const char *fmt, ...)
{
    char buf[0x400];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 0x400, fmt, ap);
    Font_Printf("%s", buf);
}

void Page_DrawObjImage(PageElem *e, u8 i);

/* The kind, the scroll x (sx), the colour and the clip state are locals (sx declared before
 * kind): the game numbers sx, kind and the colour's load before st. The colour local is read
 * by the test and the first rectangle only; the other rectangles read e->color again. */
void Page_DrawObjBack(PageElem *e)
{
    f32 fy;
    f32 x0;
    f32 fx;
    f32 y0;
    f32 x;
    f32 y;
    f32 x1;
    f32 y1;
    s32 sx;
    s32 kind;
    s32 color; /* the test and the first rectangle; the others read e->color again, as the game does */
    BrowserState *st;

    kind = e->kind;
    switch (kind) {
    case 16:
    case 15:
        break;
    case 13:
        if (e->style == 2 && e->unk0[0] == 4) {
            break;
        }
    default:
        return;
    }
    st = gBrowserState;
    sx = st->x;
    x = e->x;
    if (!(x <= (f32)(sx + 640 - st->marginRight))) {
        return;
    }
    if (x < (f32)sx - e->w) {
        return;
    }
    y = e->y;
    if (y < (f32)st->y - e->h) {
        return;
    }
    if (!(y <= (f32)(st->y + 428))) {
        return;
    }
    x0 = x - (f32)sx;
    y0 = y - (f32)st->y;
    x1 = x0 + e->w;
    y1 = y0 + e->h;
    switch (kind) {
    case 13:
        Page_DrawObjImage(e, e->unk5F);
        return;
    case 16:
        switch (e->style) {
        case 1:
            if (e->field == 0) {
                return;
            }
            color = e->color;
            if (color <= 0) {
                if (st->color == -1) {
                    Draw_RectClipped(0xFFD0D0D0, x0, y0, x0 + (f32)(u32)e->field, y1);
                    Draw_RectClipped(0xFFD0D0D0, x0, y0, x1, y0 + (f32)(u32)e->field);
                } else {
                    Draw_RectClipped(-1, x0, y0, x0 + (f32)(u32)e->field, y1);
                    Draw_RectClipped(-1, x0, y0, x1, y0 + (f32)(u32)e->field);
                }
                Draw_RectClipped(0xFF606060, x1 - (f32)(u32)e->field, y0 + (f32)(u32)e->field, x1, y1);
                Draw_RectClipped(0xFF606060, x0 + (f32)(u32)e->field, y1 - (f32)(u32)e->field, x1, y1);
                Draw_TriClipped(0xFF606060, x0, y1, x0 + (f32)(u32)e->field, y1, x0 + (f32)(u32)e->field, y1 - (f32)(u32)e->field);
                Draw_TriClipped(0xFF606060, x1, y0, x1, y0 + (f32)(u32)e->field, x1 - (f32)(u32)e->field, y0 + (f32)(u32)e->field);
                return;
            } else {
                Draw_RectClipped(color | 0xFF000000, x0, y0, x0 + (f32)(u32)e->field, y1);
                Draw_RectClipped(e->color | 0xFF000000, x0, y0, x1, y0 + (f32)(u32)e->field);
                Draw_RectClipped(e->color | 0xFF000000, x1 - (f32)(u32)e->field - 1.0f, y0 + (f32)(u32)e->field, x1, y1);
                Draw_RectClipped(e->color | 0xFF000000, x0 + (f32)(u32)e->field, y1 - (f32)(u32)e->field - 1.0f, x1, y1);
            }
            return;
        case 2:
            if (e->color >= 0) {
                Draw_RectClipped(e->color | 0xFF000000, x0, y0, x1, y1);
                return;
            }
            break;
        }
        break;
    case 15:
        if (e->color >= 0) {
            Draw_RectClipped(e->color | 0xFF000000, x0, y0, x1, y1);
        }
        if (e->field != 0) {
            fy = 1.0f + y0;
            fx = 1.0f + x0;
            Draw_VLineClipped(0xFF606060, fx, fy, x0 - 1.0f + e->w, fy);
            Draw_HLineClipped(0xFF606060, fx, fy, fx, y0 - 1.0f + e->h);
            if (gBrowserState->color == -1) {
                Draw_HLineClipped(0xFFD0D0D0, x1, y0, x1, y1);
                Draw_VLineClipped(0xFFD0D0D0, x0, y1, x1, y1);
            } else {
                {
                    f32 t = x0 - 1.0f + e->w;
                    Draw_HLineClipped(-1, t, y0 - 1.0f, t, y0 - 1.0f + e->h);
                }
                {
                    f32 t = y0 - 1.0f + e->h;
                    Draw_VLineClipped(-1, x0 - 1.0f, t, x0 - 1.0f + e->w, t);
                }
            }
        }
        break;
    }
}

#define PAGE_READY (gBrowserWork->moveTimer)

void Page_DrawObj(PageElem *e)
{
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
    f32 t;
    f32 m;
    u8 size;
    u16 n;
    s32 u;
    char buf[0x280];

    e->unk1 = 0;
    switch (e->kind) {
    case 13:
        if (e->unk0[0] == 4) {
            return;
        }
        if (e->unk0[0] > 1) {
            BrowserState *st = gBrowserState;
            s32 sx = st->x;

            if (!(e->x <= (f32)(sx + 640 - st->marginRight))) {
                return;
            }
            if (e->x < (f32)sx - e->w) {
                return;
            }
            if (e->y < (f32)st->y - e->h) {
                return;
            }
            if (!(e->y <= (f32)(st->y + 428))) {
                return;
            }
        }
        break;
    case 16:
    case 15:
        return;
    default: {
        BrowserState *st = gBrowserState;
        s32 sx = st->x;

        if (!(e->x <= (f32)(sx + 640 - st->marginRight))) {
            return;
        }
        if (e->x < (f32)sx - e->w) {
            return;
        }
        if (e->y < (f32)(st->y - 40)) {
            return;
        }
        if (!(e->y <= (f32)(st->y + 428))) {
            return;
        }
        break;
    }
    }
    e->unk1 = 1;
    x0 = e->x - (f32)gBrowserState->x;
    y0 = e->y - (f32)gBrowserState->y;
    x1 = x0 + e->w;
    y1 = y0 + e->h;
    switch (e->kind) {
    case 1:
        Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, x0, y0, e->text);
        break;
    case 2:
    case 18:
        Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, x0, y0, e->text2);
        if ((gBrowserState->style & 0x80) >> 7) {
            Draw_RectClipped(e->color | 0xFF000000, x0, y1 - 2.0f, x1, y1);
        }
        break;
    case 3:
    case 4:
    case 5:
        if (PAGE_READY == 0) {
            e->style = 0;
        }
        switch (e->style) {
        case 0:
            Page_DrawWidget(8, x0, y0, e->w, e->h, 0);
            if (strlen(e->text) == 1) {
                Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, 6.0f + x0, 1.0f + y0, e->text);
            } else {
                Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, 3.0f + x0, 1.0f + y0, e->text);
            }
            break;
        case 1:
            Page_DrawWidget(9, x0, y0, e->w, e->h, 0);
            Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, 3.0f + x0, 2.0f + y0, e->text);
            break;
        }
        break;
    case 6:
        Page_DrawWidget(7, x0, y0, e->w, e->h, 0);
        if (strlen(e->text2) == 0) {
            break;
        }
        size = e->size;
        n = e->w;
        if (n < (u32)(size * (strlen(e->text2) + 2)) >> 1) {
            n = n / (size / 2) - 2;
            memcpy(buf, e->text2, n);
            buf[n] = 0;
            Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, 4.0f + x0, 3.0f + y0, buf);
        } else {
            Page_DrawTextLine(e->unk5C, e->color, e->field, size, 4.0f + x0, 3.0f + y0, e->text2);
        }
        break;
    case 7:
        Page_DrawWidget(7, x0, y0, e->w, e->h, 0);
        if (strlen(e->text2) != 0) {
            Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, 4.0f + x0, 3.0f + y0, "********");
        }
        break;
    case 8:
        u = e->unk0[0];
        if ((u32)(u - 11) < 2) {
            if (u == 11) {
                Page_DrawWidget(0x16, x0, y0, e->w, e->h, 0);
            } else {
                Page_DrawWidget(0x17, x0, y0, e->w, e->h, 0);
                if (PAGE_READY == 0) {
                    e->unk0[0] = 11;
                }
            }
            Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, 4.0f + x0, 2.0f + y0, e->text);
        }
        break;
    case 10:
        switch ((s16)e->h) {
        case 2:
            if ((s16)e->w == 2) {
                Draw_RectClipped(0xFF000001, x0, y0, 1.0f + x0, 1.0f + y0);
            } else {
                Draw_VLineClipped(0xFF000001, x0, y0, x1, y0);
            }
            break;
        case 4:
            if ((s16)e->w == 2) {
                t = 1.0f + x0;
                Draw_RectClipped(0xFF000001, x0, y0, t, 1.0f + y0);
                Draw_RectClipped(0xFFD0D0D0, x0, 2.0f + y0, t, 3.0f + y0);
            } else {
                Draw_VLineClipped(0xFF000001, x0, y0, x1 - 2.0f, y0);
                Draw_VLineClipped(0xFFD0D0D0, x0, y1 - 2.0f, x1 - 2.0f, y1 - 2.0f);
            }
            break;
        default:
            if ((s16)e->w == 2) {
                Draw_RectClipped(0xFFD0D0D0, x0, y0, 1.0f + x0, y1 - 2.0f);
            } else {
                Draw_VLineClipped(0xFF000001, x0, y0, x1, y0);
                Draw_VLineClipped(0xFFD0D0D0, x0, y1 - 2.0f, x1, y1 - 2.0f);
                Draw_HLineClipped(0xFFD0D0D0, x1, y0, x1, y1 - 2.0f);
                Draw_HLineClipped(0xFF000001, x0, y0, x0, y1 - 2.0f);
            }
            break;
        }
        break;
    case 11:
        if (e->style == 1) {
            Page_DrawMark(0x19, x0, y0);
        } else {
            Page_DrawMark(0x18, x0, y0);
        }
        break;
    case 12:
        if (e->style == 1) {
            Page_DrawMark(0x1B, x0, y0);
        } else {
            Page_DrawMark(0x1A, x0, y0);
        }
        break;
    case 13:
        if (e->style == 2 && gBrowserWork->texHandle[e->unk5F] != 0) {
            if (*(u8 *)e->text2 != 0) {
                Page_DrawIcon(e->unk5F, x0, y0, e->w, e->h, gBrowserWork->texW[e->unk5F],
                              gBrowserWork->texH[e->unk5F]);
            }
        } else if (!(e->field & 0x80)) {
            Page_DrawThinFrame(0xFF000001, x0, y0, x1 - 1.0f, y1 - 1.0f);
            Draw_HLineClipped(0xFFD0D0D0, x1 - 2.0f, y0, x1 - 2.0f, y1);
            Draw_VLineClipped(0xFFD0D0D0, x0, y1 - 2.0f, x1, y1 - 2.0f);
            if (e->text3 != 0) {
                Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, 4.0f + x0, 2.0f + y0, e->text3);
            }
        }
        break;
    case 17:
        Page_DrawTextLine(e->unk5C, e->color, e->field, e->size, x0, y0, e->text2);
        m = y1 - (f32)(e->size / 2);
        Draw_RectClipped(e->color | 0xFF000000, x0, m - 1.0f, x1, 2.0f + m);
        if (strlen(e->text) != 0) {
            Draw_RectClipped(e->color | 0xFF000000, x0, y1 - 2.0f, x1, y1);
        }
        break;
    }
}

/* The open drop-down list of a form (browser.h: BrowserSub). */
typedef struct {
    u8 unk0[3];
    u8 button;      /* 0x03: 5 the up arrow, 6 the down arrow held */
    u8 count;       /* 0x04: options */
    u8 top;         /* 0x05: the first option shown (from 1) */
    u8 up;          /* 0x06: the list opens upwards */
    u8 unk7;
    s16 barTop;     /* 0x08: the scroll bar */
    s16 barBottom;  /* 0x0A */
    s16 barLen;     /* 0x0C */
    s16 unkE;
    s16 curX;       /* 0x10: the pointer */
    s16 curY;       /* 0x12 */
    u8 unk14[0x20 - 0x14];
    s32 sel;        /* 0x20: the select element open */
} PageList;
#define PL ((PageList *)gBrowserSub)
typedef struct {
    u8 pad0[0x5EC];
    PageElem *obj[500]; /* 0x5EC: the page objects (browser.h: BrowserWork.obj) */
} PageObjTbl;
#define PAGE_OBJ(i) (((PageObjTbl *)gBrowserWork)->obj[i])

/* Page_DrawDropList: at level 4 (round 20: m merged into j, their lifetimes are disjoint;
 * round 22: the (u16)k casts give the game's second andi of the line count in the text
 * lines' (n - k + 1) and (11 - k); round 23: Draw_RectClipped's colour first, the scroll
 * widgets' constants written plainly, break after the option loop, PL->count re-read in the
 * division, n an s32 declared last, idx + PL->top, no local for the object in the first two
 * loops, the k loops test PAGE_OBJ(j - 1)->unk5F (+ 1) in the for condition, the table read
 * through an inline accessor: the game adds the index first, base loaded first).
 * Round 30: the option loop walks the options with j (after j's scan for the list's end) and
 * counts the lines with k (after the highlight loops), so both are later webs, numbered as
 * the game has them. (Round 29 matched with the option index in k and a k = i copy at the
 * top of the search loop, a copy only for the allocation; with the option index in k and no
 * copy, i/j rotate: 27 words.) */
#pragma optimization_level 4
static inline PageObjTbl *Page_ObjTbl(void)
{
    return (PageObjTbl *)gBrowserWork;
}

void Page_DrawDropList(s32 sel)
{
    PageElem *e;
    PageElem *o;
    u16 i;
    u16 j;
    u16 k;
    s32 cnt;
    u16 idx;
    f32 x0;
    f32 y0;
    f32 x1;
    s16 cx;
    s16 cy;
    s32 n;

    for (i = 0; i < 500; i++) {
        if (Page_ObjTbl()->obj[i] == NULL || Page_ObjTbl()->obj[i]->unk0[0] == 0) {
            break;
        }
        if (Page_ObjTbl()->obj[i]->kind == 8 && PL->sel == i) {
            idx = i;
            n = PL->count;
            if (!n) {
                return;
            }
            for (j = idx + 1; j < 500; j++) {
                if (Page_ObjTbl()->obj[j]->kind != 8 || !(Page_ObjTbl()->obj[j]->unk5F < 11) || Page_ObjTbl()->obj[j]->unk5F == 0) {
                    break;
                }
            }
            PL->barTop = (PL->top - 1) * 144 / PL->count + 18;
            PL->barBottom = 0x5A0 / PL->count + PL->barTop;
            PL->barLen = PL->barBottom - PL->barTop;
            if (PL->barBottom > 180) {
                PL->barBottom = 180;
            }
            if (PL->count > 10) {
                if (PL->barTop < 18) {
                    PL->barTop = 18;
                    PL->barBottom = PL->barLen + 18;
                    PL->top = 1;
                }
                if (PL->barBottom > 162) {
                    PL->barTop = 162 - PL->barLen;
                    PL->barBottom = 162;
                    PL->top = PL->count - 9;
                }
            }
            e = Page_ObjTbl()->obj[i];
            x0 = e->x - (f32)gBrowserState->x;
            y0 = e->y - (f32)gBrowserState->y;
            x1 = x0 + e->w;
            if (PL->up != 0) {
                Draw_RectClipped(-1, x0, y0, x1, y0 - (f32)(Page_ObjTbl()->obj[j - 1]->unk5F * 18));
                Page_DrawThinFrame(0xFF000001, x0, y0, x1, y0 - (f32)(Page_ObjTbl()->obj[j - 1]->unk5F * 18));
                cx = PL->curX;
                if (!((f32)cx <= x0) && ((f32)cx <= x1 - 16.0f || (PL->count < 11 && (f32)cx < x1))) {
                    if ((f32)PL->curY < y0) {
                        cnt = Page_ObjTbl()->obj[j - 1]->unk5F + 1;
                        if (!((f32)PL->curY <= y0 - (f32)(cnt * 18))) {
                            for (k = 0; k < Page_ObjTbl()->obj[j - 1]->unk5F + 1; k++) {
                                if (!((f32)PL->curY <= y0 - (f32)(k * 18)) && (f32)PL->curY <= y0 - (f32)((k - 1) * 18)) {
                                    Draw_RectClipped(0x600000FF, x0, y0 - (f32)(k * 18), x1, y0 - (f32)((k - 1) * 18));
                                    break;
                                }
                            }
                        }
                    }
                }
                if (n > 10) {
                    if (PL->button != 5) {
                        Page_DrawWidget(10, x1 - 16.0f, y0 - 180.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(14, x1 - 16.0f, y0 - 180.0f, 16.0f, 20.0f, 0);
                        if (PAGE_READY == 0) {
                            PL->button = 0;
                        }
                    }
                    Page_DrawWidget(0x12, x1 - 16.0f, y0 - 20.0f, 16.0f, -140.0f, 0);
                    if (PL->button != 6) {
                        Page_DrawWidget(0xB, x1 - 16.0f, y0 - 20.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(0xF, x1 - 16.0f, y0 - 20.0f, 16.0f, 20.0f, 0);
                        if (PAGE_READY == 0) {
                            PL->button = 0;
                        }
                    }
                    Page_DrawWidget(0x13, x1 - 16.0f, y0 - 180.0f + (f32)PL->barTop, 16.0f, (f32)(PL->barBottom - PL->barTop), 0);
                }
            } else {
                Draw_RectClipped(-1, x0, 20.0f + y0, x1, (20.0f + y0) + (f32)(Page_ObjTbl()->obj[j - 1]->unk5F * 18));
                Page_DrawThinFrame(0xFF000001, x0, 4.0f + (16.0f + y0), x1, (20.0f + y0) + (f32)(Page_ObjTbl()->obj[j - 1]->unk5F * 18));
                cx = PL->curX;
                if (!((f32)cx <= x0) && (f32)cx < x1 - 16.0f) {
                    if (!((f32)PL->curY <= (20.0f + y0))) {
                        cnt = Page_ObjTbl()->obj[j - 1]->unk5F;
                        if ((f32)PL->curY < (20.0f + y0) + (f32)(cnt * 18)) {
                            for (k = 0; k < Page_ObjTbl()->obj[j - 1]->unk5F; k++) {
                                if (!((f32)PL->curY <= (20.0f + y0) + (f32)(k * 18)) && (f32)PL->curY < (20.0f + y0) + (f32)((k + 1) * 18)) {
                                    Draw_RectClipped(0x600000FF, x0, (20.0f + y0) + (f32)(k * 18), x1, (20.0f + y0) + (f32)((k + 1) * 18));
                                    break;
                                }
                            }
                        }
                    }
                }
                if (n > 10) {
                    if (PL->button != 5) {
                        Page_DrawWidget(10, x1 - 16.0f, 20.0f + y0, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(14, x1 - 16.0f, 20.0f + y0, 16.0f, 20.0f, 0);
                        if (PAGE_READY == 0) {
                            PL->button = 0;
                        }
                    }
                    Page_DrawWidget(0x12, x1 - 16.0f, 20.0f + (20.0f + y0), 16.0f, 160.0f, 0);
                    if (PL->button != 6) {
                        Page_DrawWidget(0xB, x1 - 16.0f, 180.0f + (20.0f + y0) - 20.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(0xF, x1 - 16.0f, 180.0f + (20.0f + y0) - 20.0f, 16.0f, 20.0f, 0);
                        if (PAGE_READY == 0) {
                            PL->button = 0;
                        }
                    }
                    Page_DrawWidget(0x13, x1 - 16.0f, (20.0f + y0) + (f32)PL->barTop, 16.0f, (f32)(PL->barBottom - PL->barTop), 0);
                }
            }
            k = 0;
            for (j = idx + PL->top; j < 500; j++) {
                o = Page_ObjTbl()->obj[j];
                if (o->kind != 8) {
                    return;
                }
                if (o->unk5F == 0) {
                    return;
                }
                k++;
                if (!(k < 11)) {
                    return;
                }
                if (PL->up != 0) {
                    if (n < 11) {
                        if (o->unk5F == Page_ObjTbl()->obj[sel]->style) {
                            Draw_RectClipped(0x6000FF00, x0, y0 - (f32)((n - k + 1) * 18), x0 + Page_ObjTbl()->obj[sel]->w, 18.0f + (y0 - (f32)((n - k + 1) * 18)));
                        }
                        Page_DrawTextLine(0x15, 0xFF000001, 0, Page_ObjTbl()->obj[j]->size, 4.0f + x0,
                                          2.0f + y0 - (f32)((n - (u16)k + 1) * 18), Page_ObjTbl()->obj[j]->text);
                    } else {
                        if (o->unk5F == Page_ObjTbl()->obj[sel]->style) {
                            Draw_RectClipped(0x6000FF00, x0, y0 - (f32)((11 - k) * 18), x0 + Page_ObjTbl()->obj[sel]->w - 16.0f, 18.0f + (y0 - (f32)((11 - k) * 18)));
                        }
                        Page_DrawTextLine(0x15, 0xFF000001, 0, Page_ObjTbl()->obj[j]->size, 4.0f + x0,
                                          2.0f + y0 - (f32)((11 - (u16)k) * 18), Page_ObjTbl()->obj[j]->text);
                    }
                } else {
                    if (n < 11) {
                        if (o->unk5F == Page_ObjTbl()->obj[sel]->style) {
                            Draw_RectClipped(0x6000FF00, x0, y0 + (f32)(k * 18), x0 + Page_ObjTbl()->obj[sel]->w, 2.0f + (18.0f + (y0 + (f32)(k * 18))));
                        }
                    } else {
                        if (o->unk5F == Page_ObjTbl()->obj[sel]->style) {
                            Draw_RectClipped(0x6000FF00, x0, y0 + (f32)(k * 18), x0 + Page_ObjTbl()->obj[sel]->w - 16.0f, 2.0f + (18.0f + (y0 + (f32)(k * 18))));
                        }
                    }
                    Page_DrawTextLine(0x15, 0xFF000001, 0, Page_ObjTbl()->obj[j]->size, 4.0f + x0, 2.0f + y0 + (f32)(k * 18),
                                      Page_ObjTbl()->obj[j]->text);
                }
            }
            break;
        }
    }
}
#pragma optimization_level reset

void Browser_DrawOverlay(void);

/* The page's focused object: gBrowserWork->obj[gBrowserWork->focus] (browser.h);
   the switch reads it again (a local there doesn't match). */
#define PAGE_FOCUS (*(PageElem **)((u32)gBrowserWork + 0x5EC + gBrowserWork->focus * 4))

void Page_DrawFocus(PageElem *self)
{
    s32 color;
    PageElem *e;
    u8 kind;
    s8 k;

    color = self->unkC > 0 ? 0xFFFE2B2C : -1;
    e = PAGE_FOCUS;
    kind = e->kind;
    if (kind == 2 || kind == 0xC || kind == 0xB || kind == 8 || kind == 7 || kind == 6 || kind == 5
        || kind == 4 || kind == 3 || kind == 0x12 || kind == 0x11) {
        goto frame;
    }
    switch (kind) {
    case 0xD:
        switch (PAGE_FOCUS->field & 0xF) {
        case 1:
            if ((e->field & 0x10) == 0x10) {
                k = 4;
            } else {
                k = 2;
            }
            break;
        case 2:
            k = 3;
            break;
        case 3:
            k = 4;
            break;
        case 4:
            k = 5;
            break;
        default:
            k = 0;
            break;
        }
        if (k) {
            Page_DrawWidget(k, e->x - gBrowserState->x, e->y - gBrowserState->y, 0.0f, 0.0f, color);
            if (gBrowserState->styleMode != 0) {
                Browser_DrawOverlay();
            }
            return;
        }
        break;
    }
frame:
    Page_DrawFrame(color, 3, e->x - gBrowserState->x - 1.0f, e->y - gBrowserState->y - 1.0f,
                   3.0f + (e->x + e->w - gBrowserState->x), 3.0f + (e->y + e->h - gBrowserState->y));
}

void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
void Text_Reset(void);
void Font_SetHalfWidth(s32 a0);
void LobbyMsg_DrawPage5(s32 msg);
void Text_DrawIcons(s32 i);
void Font_DrawPage(s32 layer);
void NetModeMsg_Flush(s8 which);

extern s32 gTextOff;
void BrowserDlg_DrawStatus(PageElem *o)
{
    s8 msg = 0;
    s8 row = 8;
    s32 hl = 0;
    s32 boxes = 0;
    s32 i;
    s32 l;
    s32 w;
    s32 top;
    s32 r;
    s32 bot;
    s16 k;

    switch (gBrowserWork->dlgBoxKind) {
    case 5:
        msg = 3;
        hl = 1;
        row = 9;
        break;
    case 4:
        switch (gBrowserSub->dlgMode) {
        case 3:
            hl = 1;
            gBrowserWork->dlgBoxReady = 1;
            row = 7;
            break;
        case 4:
            switch ((s8)gBrowserWork->dlgBoxReady) {
            case 1:
            default:
                msg = gNetMode.cardMsg;
                boxes = 1;
                hl = 0;
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
                    boxes = 1;
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
                    boxes = 1;
                    break;
                case -21:
                    boxes = 1;
                    msg = 5;
                    break;
                case -23:
                    msg = 6;
                    boxes = 1;
                    break;
                default:
                    return;
                }
                hl = 0;
                break;
            }
            break;
        }
        break;
    case 6:
        switch (gBrowserSub->dlgMode) {
        case 3:
            msg = 4;
            hl = 1;
            gBrowserWork->dlgBoxReady = 2;
            break;
        case 4:
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
                boxes = 1;
                msg = 1;
                break;
            }
            hl = 0;
            break;
        }
        row = 10;
        break;
    default:
        return;
    }
    TileWin_Draw(o->x, o->y - 40.0f, o->x + o->w, o->y + o->h);
    if (hl != 0) {
        l = 90.0f + o->x;
        i = row + gBrowserSub->dlgCursor;
        top = o->y + (f32)((i - 2) * 20);
        r = 10.0f + (420.0f + o->x);
        bot = o->y + (f32)((i - 1) * 20);
        Menu_DrawGlowBar(l + (r - l) / 2, top, (r - l) + 0x50, bot - top, 0xFF20C0C0);
    }
    if (boxes == 0) {
        Page_DrawTextLine(0x14, -1, 0, 0x14, 20.0f + o->x, o->y, Browser_GetText(5, msg));
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
void BrowserDlg_DrawYesNo(PageElem *o)
{
    s16 bx;
    u32 color;
    f32 tx;

    TileWin_Draw(o->x, o->y, o->x + o->w, o->y + o->h);
    Prim_DrawSprite(-1, PageTex_Get(0), 49.0f + o->x, 145.0f + o->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    Prim_DrawSprite(-1, PageTex_Get(0), 157.0f + o->x, 145.0f + o->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    tx = o->x + (o->w - (f32)(Str_Length(Browser_GetText(7, 0)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(0x14, -1, 0, 0x14, tx, 85.0f + o->y, Browser_GetText(7, 0));
    tx = 49.0f + o->x + (f32)((0x54 - Str_Length(Browser_GetText(7, 1)) * 20 / 2) / 2);
    Page_DrawTextLine(0x14, -1, 1, 0x14, tx, 6.0f + (145.0f + o->y), Browser_GetText(7, 1));
    tx = 157.0f + o->x + (f32)((0x54 - Str_Length(Browser_GetText(7, 2)) * 20 / 2) / 2);
    Page_DrawTextLine(0x14, -1, 1, 0x14, tx, 6.0f + (145.0f + o->y), Browser_GetText(7, 2));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        bx = 49.0f + o->x;
        break;
    case 2:
        bx = 157.0f + o->x;
        break;
    }
    color = -1;
    if (o->focus == 1) {
        color = 0xFFFE2B2C;
    }
    Prim_DrawSprite(color, PageTex_Get(0), bx, 145.0f + o->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
    TextObj_Reset();
}


void BrowserDlg_DrawOk(PageElem *o)
{
    s16 bx;
    u32 color;
    f32 tx;

    TileWin_Draw(o->x, o->y, o->x + o->w, o->y + o->h);
    Prim_DrawSprite(-1, PageTex_Get(0), o->x + (o->w - 84.0f) / 2.0f, 145.0f + o->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    tx = o->x + (o->w - (f32)(Str_Length(Browser_GetText(7, 0)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(0x14, -1, 0, 0x14, tx, 85.0f + o->y, Browser_GetText(7, 3));
    tx = o->x + (o->w - (f32)(Str_Length(Browser_GetText(7, 4)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(0x14, -1, 1, 0x14, tx, 6.0f + (145.0f + o->y), Browser_GetText(7, 4));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        bx = o->x + (o->w - 84.0f) / 2.0f;
        break;
    }
    color = -1;
    if (o->focus == 1) {
        color = 0xFFFE2B2C;
    }
    Prim_DrawSprite(color, PageTex_Get(0), bx, 145.0f + o->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
    TextObj_Reset();
}

s32 strcmp(char *a, char *b); /* main: strcmp */

/* A dialog of text group 8: two buttons (texts 3, 4) on the KDDI page below
   while gBrowserWork->endReason > 0, else one (text 1). */

void BrowserDlg_DrawCrsTop(PageElem *o)
{
    s16 bx;
    u32 color;
    f32 tx;

    if ((s8)gBrowserWork->endReason > 0 && gBrowserWork->netError == 0
        && strcmp(gBrowserState->url, "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp") == 0) {
        TileWin_Draw(o->x, o->y, o->x + o->w, o->y + o->h);
        Prim_DrawSprite(-1, PageTex_Get(0), 80.0f + o->x, 165.0f + o->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
        Prim_DrawSprite(-1, PageTex_Get(0), 216.0f + o->x, 165.0f + o->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
        Page_DrawTextLine(0x14, -1, 0, 0x14, 40.0f + o->x, 80.0f + o->y, Browser_GetText(8, 2));
        tx = 80.0f + o->x + (f32)((0x54 - Str_Length(Browser_GetText(8, 3)) * 20 / 2) / 2);
        Page_DrawTextLine(0x14, -1, 1, 0x14, tx, 6.0f + (165.0f + o->y), Browser_GetText(8, 3));
        tx = 216.0f + o->x + (f32)((0x54 - Str_Length(Browser_GetText(8, 4)) * 20 / 2) / 2);
        Page_DrawTextLine(0x14, -1, 1, 0x14, tx, 6.0f + (165.0f + o->y), Browser_GetText(8, 4));
        switch (gBrowserSub->dlgCursor) {
        case 1:
            bx = 80.0f + o->x;
            break;
        case 2:
            bx = 216.0f + o->x;
            break;
        }
        color = -1;
        if (o->focus == 1) {
            color = 0xFFFE2B2C;
        }
        Prim_DrawSprite(color, PageTex_Get(0), bx, 165.0f + o->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
        TextObj_Reset();
        return;
    }
    TileWin_Draw(o->x, o->y, o->x + o->w, o->y + o->h);
    Prim_DrawSprite(-1, PageTex_Get(0), 138.0f + o->x, 185.0f + o->y, 0x54, 0x20, 0x2A, 0, 0x80, 0x20);
    Page_DrawTextLine(0x14, -1, 0, 0x14, 40.0f + o->x, 85.0f + o->y, Browser_GetText(8, 0));
    tx = 138.0f + o->x + (f32)((0x54 - Str_Length(Browser_GetText(8, 1)) * 20 / 2) / 2);
    Page_DrawTextLine(0x14, -1, 1, 0x14, tx, 6.0f + (185.0f + o->y), Browser_GetText(8, 1));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        bx = 138.0f + o->x;
        break;
    }
    color = -1;
    if (o->focus == 1) {
        color = 0xFFFE2B2C;
    }
    Prim_DrawSprite(color, PageTex_Get(0), bx, 185.0f + o->y, 0x54, 0x20, 0x2A, 0x20, 0x80, 0x40);
    TextObj_Reset();
}

/* The page textures of the browser work (browser.h's BrowserWork.texHandle). */
typedef struct {
    u8 pad0[0xDF0];
    s32 tex[20]; /* 0xDF0 */
    u16 w[20];   /* 0xE40 */
    u16 h[20];   /* 0xE68 */
} PageTexTbl;
#define PT ((PageTexTbl *)gBrowserWork)

#pragma optimization_level 4

void Page_DrawObjImage(PageElem *e, u8 i)
{
    f32 over;
    f32 x;
    u8 n;
    f32 y;
    f32 w;
    f32 h;
    f32 rx;
    f32 uw;
    f32 ry;
    f32 vh;

    n = 0;
    ry = 0.0f;
    y = e->y - gBrowserState->y;
    for (;;) {
        over = (y + (f32)PT->h[i]) - (e->h + (e->y - gBrowserState->y));
        if (over > 0.0f) {
            ry = over;
            if (ry == 0.0f) {
                return;
            }
        }
        rx = 0.0f;
        x = e->x - gBrowserState->x;
        for (;;) {
            over = (x + (f32)PT->w[i]) - (e->w + (e->x - gBrowserState->x));
            if (over > 0.0f) {
                rx = over;
                if (rx == 0.0f) {
                    break;
                }
            }
            w = PT->w[i];
            h = PT->h[i];
            uw = PT->w[i];
            vh = PT->h[i];
            if ((rx == 0.0f || rx == 1.0f) && (ry == 0.0f || ry == 1.0f)) { /* compiled-out check */
            } else if ((rx != 0.0f || rx == 1.0f) && (ry == 0.0f || ry == 1.0f)) {
                w -= rx;
                uw -= rx;
            } else if ((rx == 0.0f || rx == 1.0f) && ry != 0.0f && ry != 1.0f) {
                h -= ry;
                vh -= ry;
            } else {
                w -= rx;
                h -= ry;
                uw -= rx;
                vh -= ry;
            }
            Prim_DrawSprite(-1, PT->tex[i], x, y, w, h, 0, 0, uw, vh);
            n++;
            if (n > 0x200) {
                return;
            }
            if (rx > 0.0f) {
                break;
            }
            x += PT->w[i];
        }
        if (ry > 0.0f) {
            return;
        }
        y += PT->h[i];
    }
}
#pragma optimization_level reset

/* The whole screen (640 x 448) in 0x80010101. */
void Draw_FullScreen(void)
{
    Draw_RectClipped(0x80010101, 0.0f, 0.0f, 640.0f, 448.0f);
}

void Browser_DrawOverlay(void)
{
    s8 *fade;
    s32 c;

    if ((gBrowserState->style & 0x200) >> 9) {
        if (gBrowserState->plain != 0) {
            Draw_RectClipped(0xFF000001, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
            Draw_RectClipped(0xFF000001, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
            Draw_RectClipped(0xFF000001, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
            Draw_RectClipped(0xFF000001, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        } else {
            Draw_RectClipped(0xFF000001, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
            Draw_RectClipped(0xFF000001, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
            Draw_RectClipped(0xFF000001, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
            Draw_RectClipped(0xFF000001, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        }
    } else {
        Draw_RectClipped(0xFF000001, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
        Draw_RectClipped(0xFF000001, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
        Draw_RectClipped(0xFF000001, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
        Draw_RectClipped(0xFF000001, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        if ((gBrowserState->style & 0x100) >> 8 && strlen(gBrowserState->msg) != 0) {
            Prim_DrawSprite(-1, PageTex_Get(0), 0x228, 0x1E, 0x18, 0x18, 0xA0, 0, 0xC0, 0x20);
        }
    }
    if (!(gBrowserState->style & 1)) {
        if (gBrowserWork->dlgMsgState != 0 || gBrowserWork->dlgInputState != 2 || gBrowserWork->dlgBoxKind != 1) {
            Draw_FullScreen();
        }
    }
    if (gBrowserState->initMode != 0) {
        fade = &gBrowserState->fade;
        if (*fade > 0) {
            *fade = *fade - 1;
            c = (gBrowserState->fade * 255 / 16) << 24;
            Draw_RectClipped(c, 0.0f, 0.0f, 640.0f, 448.0f);
        } else if (*fade < 0) {
            *fade = *fade + 1;
            c = (gBrowserState->fade * 255 / 16 + 0xFF) << 24;
            Draw_RectClipped(c, 0.0f, 0.0f, 640.0f, 448.0f);
        }
    } else {
        gBrowserState->fade = 0;
    }
}

s32 Rect_ClipToScreen(f32 *x, f32 *y, f32 *z, f32 *w)
{
    f32 left;
    f32 right;
    f32 top;
    f32 bottom;

    left = *x;
    if (left < 0.0f) {
        *x = 0.0f;
        goto block_4;
    }
    if (!(left <= 640.0f)) {
        return 0;
    }
block_4:
    right = *z;
    if (right < 0.0f) {
        return 0;
    }
    if (!(right <= 640.0f)) {
        *z = 640.0f;
    }
    top = *y;
    if (top < 0.0f) {
        *y = 0.0f;
        goto block_12;
    }
    if (!(top <= 448.0f)) {
        return 0;
    }
block_12:
    bottom = *w;
    if (bottom < 0.0f) {
        return 0;
    }
    if (!(bottom <= 448.0f)) {
        *w = 448.0f;
    }
    return 1;
}
