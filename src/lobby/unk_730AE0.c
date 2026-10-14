/* The browser's URLs: the base URL, resolving a link against it
 * (relative paths, "./", "../", "//", schemes), the link schemes, a URL's
 * file type; URL escapes, Shift-JIS and EUC-JP, the page's object pool and
 * layers, its textures. One unit of the game's: Url_FileType keeps
 * registers across the static Url_PathEnd and Url_LastDot.
 * include/browser.h. */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"
int strncmp(const char *, const char *, unsigned int);

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
u32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, u32 n);
void *memmove(void *dst, const void *src, u32 n); /* main: memmove */
void Mem_Compact(void);
s32 flReleasePaletteHandle(u32);
/* unit's own declaration: with the definition's s32 (u32 handle) (main/texture.c), PageTex_ReleaseAll: 3 of 51 words differ */
s32 flReleaseTextureHandle(u16);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
s32 Http_ReadFile(void *p, s32 a1);            /* nethttp */
void Url_Copy(char *dst, const char *src);
s32 Url_SpecialScheme(const char *url);
s32 Url_GetScheme(const char *url);
void Str_ToLower(char *s);

extern char *gScriptSchemes[]; /* { "javascript:", NULL } */
extern UrlScheme gUrlSchemes[]; /* file, http, https, memory, lbs, afs; then 0 */

void Url_ClearBase(void);

/* Url_SetBase: the base URL that relative links resolve against. */
/* unk_730AE0's .rodata (0x839018-0x839028) */
#pragma rodata on
char D_00839018[0xC] = {
#include "lobby/00839018.inc"
};
#pragma rodata off

void Url_SetBase(char *url)
{
    Url_ClearBase();
    Url_Copy(gBrowserWork->baseUrl, url);
}

/* Url_ClearBase */
void Url_ClearBase(void)
{
    memset(gBrowserWork->baseUrl, 0, 0x100);
}

/* Url_PathEnd: the end of a URL's path (its '?', its '#' or its end). */
static char *Url_PathEnd(char *p)
{
    while (*p != 0 && *p != '?' && *p != '#') {
        p++;
    }
    return p;
}

/* Url_LastSlash: the last '/' at or before end - 1, not before start. */
static char *Url_LastSlash(char *start, char *end)
{
    end--;
    while (*end != '/') {
        if (!(start >= end) == 0) {
            return NULL;
        }
        end--;
    }
    return end;
}

/* Url_LastDot: a '.' in the last path segment (a file name's extension). */
static char *Url_LastDot(char *start, char *end)
{
    end--;
    while (*end != '/') {
        if (!(start >= end) == 0) {
            return NULL;
        }
        /* read again after the compare (the game's code reloads it) */
        if (*(volatile char *)end == '.') {
            return end;
        }
        end--;
    }
    return NULL;
}

/* Url_FindPath: where the path starts ("/" after the host; for file:
 * URLs the '/' of "file://"). */
static char *Url_FindPath(char *p, char *end)
{
    s32 state = 0;

    switch ((u8)Url_GetScheme(p)) {
    case URL_OTHER:
        break;
    case URL_FILE:
        return p + 6;
    default:
        for (; p < end; p++) {
            switch (state) {
            case 0:
                if (!(p + 1 < end)) {
                    return NULL;
                }
                if (p[0] == '/' && p[1] == '/') {
                    state++;
                    p++;
                }
                break;
            case 1:
                if (!(p < end)) {
                    return NULL;
                }
                if (*p == '/') {
                    return p;
                }
                break;
            }
        }
        break;
    }
    return NULL;
}

/* Url_FindSlash */
static char *Url_FindSlash(char *p, char *end)
{
    for (; p < end; p++) {
        if (*p == '/') {
            return p;
        }
    }
    return NULL;
}

/* Url_FindDoubleSlash: "//" */
static char *Url_FindDoubleSlash(char *p, char *end)
{
    for (; p + 1 < end; p++) {
        if (p[0] == '/' && p[1] == '/') {
            return p;
        }
    }
    return NULL;
}

/* Url_FindCurrent: "/./" */
static char *Url_FindCurrentDir(char *p, char *end)
{
    for (; p + 2 < end; p++) {
        if (p[0] == '/' && p[1] == '.' && p[2] == '/') {
            return p;
        }
    }
    return NULL;
}

/* Url_FindParent: "/../" */
static char *Url_FindParentDir(char *p, char *end)
{
    for (; p + 3 < end; p++) {
        if (p[0] == '/' && p[1] == '.' && p[2] == '.' && p[3] == '/') {
            return p;
        }
    }
    return NULL;
}

/* Url_Combine: makes url absolute in place: a relative url gets base's
 * directory (or base's host, for a url starting with '/') in front; a
 * directory (isBase, no file name) and a bare host get a closing '/'.
 * NULL when base can't be used or the URL grows too long. */
char *Url_Combine(char *base, char *url, s32 isBase)
{
    s32 scheme = Url_GetScheme(url);
    char *end;
    s32 len;

    if ((u8)scheme == 0) {
        if (*base != 0) {
            char *e = Url_PathEnd(base);
            s32 n;

            if (*url == '/') {
                e = Url_FindPath(base, e);
            } else {
                e = Url_LastSlash(base, e);
            }
            if (e == NULL) {
                return NULL;
            }
            n = e - base + 1;
            memmove(url + n, url, strlen(url) + 1);
            memcpy(url, base, n);
            scheme = Url_GetScheme(url);
        } else {
            return NULL;
        }
    }
    len = strlen(url);
    end = Url_PathEnd(url);
    if (Url_LastDot(url, end) == NULL && *end == 0 && isBase) {
        if (len >= 0xFF) {
            return NULL;
        }
        if (end[-1] != '/') {
            end[0] = '/';
            end[1] = 0;
        }
    }
    if ((u8)scheme != URL_FILE) {
        len = strlen(url);
        end = Url_PathEnd(url);
        if (Url_FindSlash(Url_FindDoubleSlash(url, end) + 2, end) == NULL) {
            if (len >= 0xFF) {
                return NULL;
            }
            if (end[-1] != '/') {
                end[0] = '/';
                end[1] = 0;
            }
        }
    }
    return url;
}

/* Url_Resolve: url (in place) resolved against base, or against the page's
 * base URL when base is NULL or empty: made absolute, "//" collapsed,
 * "/../" and "/./" removed (for other schemes than http, leading "../"
 * are kept). NULL for "javascript:" links and bad URLs. */
char *Url_Resolve(char *base, char *url)
{
    char *p;
    char *end;

    if (Url_SpecialScheme(url) != -1) {
        return NULL;
    }
    if (base == NULL || *base == 0) {
        base = "http:///";
    }
    if ((p = Url_Combine(base, gBrowserWork->baseUrl, 1)) == NULL) {
        return NULL;
    }
    url = Url_Combine(p, url, 0);
    if (url == NULL) {
        return NULL;
    }
    end = Url_PathEnd(url);
    /* base is the path from here on */
    base = Url_FindPath(url, end);
    if (base == NULL) {
        return NULL;
    }
    while ((p = Url_FindDoubleSlash(base, end)) != NULL) {
        memmove(p, p + 1, strlen(p + 1) + 1);
        end = Url_PathEnd(base);
    }
    switch ((u8)Url_GetScheme(url)) {
    case URL_HTTP:
        while ((p = Url_FindParentDir(base, end)) != NULL) {
            if (p != base) {
                break;
            }
            memmove(p, p + 3, strlen(p + 3) + 1);
            end = Url_PathEnd(base);
        }
        while ((p = Url_FindParentDir(base, end)) != NULL) {
            end = Url_LastSlash(base, p - 1);
            if (end == NULL) {
                return NULL;
            }
            memmove(end, p + 3, strlen(p + 3) + 1);
            end = Url_PathEnd(base);
        }
        break;
    default:
        while ((p = Url_FindParentDir(base, end)) != NULL) {
            if (p != base) {
                end = Url_LastSlash(base, p - 1);
                if (end == NULL) {
                    return NULL;
                }
                memmove(end, p + 3, strlen(p + 3) + 1);
            } else {
                base += strlen("../");
            }
            end = Url_PathEnd(base);
        }
        base = Url_FindPath(url, end);
        break;
    }
    while ((end = Url_FindCurrentDir(base, end)) != NULL) {
        memmove(end, end + 2, strlen(end + 2) + 1);
        end = Url_PathEnd(base);
    }
    return url;
}


/* Str_ToLower: 'A'..'Z' to 'a'..'z', in place. */
void Str_ToLower(char *s)
{
    char c;

    c = *s;
    if (c != 0) {
        do {
            if (c >= 'A' && c < 'Z' + 1) {
                *s += 0x20;
            }
            s++;
            c = *s;
        } while (c != 0);
    }
}

/* Url_SpecialScheme: the index in gScriptSchemes of url's scheme (case
 * ignored), or -1 for an ordinary link. */
s32 Url_SpecialScheme(const char *url)
{
    s32 i = 0;
    char **p = gScriptSchemes;
    char *s;

    Url_Copy(gBrowserWork->urlLower, url);
    Str_ToLower(gBrowserWork->urlLower);
    s = gScriptSchemes[0];
    while (s != NULL) {
        if (strncmp(gBrowserWork->urlLower, *p, strlen(s)) == 0) {
            break;
        }
        p++;
        s = *p;
        i++;
    }
    if (*p != NULL) {
        return i;
    }
    i = -1;
    return i;
}

/* Url_GetScheme: URL_FILE .. URL_AFS, or URL_OTHER (0) for a relative or
 * unknown URL. */
s32 Url_GetScheme(const char *url)
{
    UrlScheme *s = gUrlSchemes;

    if (gUrlSchemes[0].id != 0) {
        do {
            if (strncmp(url, s->prefix, strlen(s->prefix)) == 0) {
                break;
            }
            s++;
        } while (s->id != 0);
    }
    return (s8)s->id;
}

/* Url_FileType: 1 for ".htm"/".html", 2 ".png", 3 ".bmp", 4 ".mmbbin",
 * 0 otherwise (case ignored). */
s32 Url_FileType(char *url)
{
    char *end = Url_PathEnd(url);
    char *dot = Url_LastDot(url, end);

    if (dot == NULL) {
        return 0;
    }
    if (dot[1] != 0 && dot[2] != 0 && dot[3] != 0 && dot + 4 == end) {
        if ((dot[1] == 'P' || dot[1] == 'p') && (dot[2] == 'N' || dot[2] == 'n') && (dot[3] == 'G' || dot[3] == 'g')) {
            return 2;
        }
        if ((dot[1] == 'B' || dot[1] == 'b') && (dot[2] == 'M' || dot[2] == 'm') && (dot[3] == 'P' || dot[3] == 'p')) {
            return 3;
        }
        if ((dot[1] == 'H' || dot[1] == 'h') && (dot[2] == 'T' || dot[2] == 't') && (dot[3] == 'M' || dot[3] == 'm')) {
            return 1;
        }
    }
    if (dot[1] != 0 && dot[2] != 0 && dot[3] != 0 && dot[4] != 0 && dot + 5 == end) {
        if ((dot[1] == 'H' || dot[1] == 'h') && (dot[2] == 'T' || dot[2] == 't') && (dot[3] == 'M' || dot[3] == 'm') &&
            (dot[4] == 'L' || dot[4] == 'l')) {
            return 1;
        }
    }
    if (dot[1] != 0 && dot[2] != 0 && dot[3] != 0 && dot[4] != 0 && dot[5] != 0 && dot[6] != 0 && dot + 7 == end) {
        if ((dot[1] == 'M' || dot[1] == 'm') && (dot[2] == 'M' || dot[2] == 'm') && (dot[3] == 'B' || dot[3] == 'b') &&
            (dot[4] == 'B' || dot[4] == 'b') && (dot[5] == 'I' || dot[5] == 'i') && (dot[6] == 'N' || dot[6] == 'n')) {
            return 4;
        }
    }
    return 0;
}

/* Url_Escape: "%XX" for each byte of src; the length written. */
s32 Url_Escape(char *dst, u8 *src)
{
    s32 n = 0;

    while (*src != 0) {
        dst[0] = '%';
        n += 3;
        dst[1] = "0123456789ABCDEF"[(*src & 0xF0) >> 4];
        *(dst += 2) = "0123456789ABCDEF"[*src++ & 0xF];
        dst++;
    }
    *dst = 0;
    return n;
}

/* Sjis_ToEuc: one double-byte character. */
static u32 Sjis_ToEuc(u32 c)
{
    u32 hi = (c >> 8) & 0xFF;
    u32 lo = c & 0xFF;
    u32 row;

    row = ((hi - (hi < 0xA0 ? 0x71 : 0xB1)) << 1) + 1;
    if (lo >= 0x9E) {
        lo -= 0x7E;
        row++;
    } else if (lo > 0x7F) {
        lo -= 0x20;
    } else {
        lo -= 0x1F;
    }
    lo |= 0x80;
    row |= 0x80;
    if (lo == 0xA0) {
        lo = 0xFE;
        row--;
    }
    return (row << 8) | lo;
}

/* Euc_ToSjis: one double-byte character. */
static u32 Euc_ToSjis(u32 c)
{
    u32 hi;
    u32 lo;

    lo = c & 0xFFFF7F7F;
    hi = (lo >> 8) & 0xFF;
    lo &= 0xFF;
    lo += (hi & 1) ? 0x1F : 0x7D;
    if (lo >= 0x7F) {
        lo++;
    }
    hi = ((hi - 0x21) >> 1) + 0x81;
    if (hi > 0x9F) {
        hi += 0x40;
    }
    return (hi << 8) | lo;
}

/* Str_SjisToEuc: in place; half-width katakana become '_'. */
void Str_SjisToEuc(u8 *s)
{
    s32 len = strlen((char *)s);
    s32 i;
    s32 j;
    s32 r;

    for (i = 0, j = 0; i < len; i++) {
        s32 c;
        u8 *p = &s[i];

        c = *p;

        if (c <= 0x7F) {
            s[j++] = c;
        } else if (c >= 0xA0 && c <= 0xDF) {
            s[j++] = '_';
        } else {
            c = (c << 8) | p[1];
            i++;
            r = Sjis_ToEuc(c);
            s[j] = r >> 8;
            s[j + 1] = r;
            j += 2;
        }
    }
    s[j] = 0;
}

/* Str_EucToSjis: in place; EUC's half-width katakana (0x8E xx) are kept
 * as their second byte. */
void Str_EucToSjis(u8 *s)
{
    s32 len = strlen((char *)s);
    s32 i;
    s32 j;
    s32 c;

    for (i = 0, j = 0; i < len; i++) {
        c = s[i];
        if (c <= 0x7F) {
            s[j++] = c;
        } else {
            c = (c << 8) | s[i + 1];
            i++;
            if (c > 0x8EA0 && c <= 0x8EFE) {
                s[j++] = c;
            } else {
                c = Euc_ToSjis(c);
                s[j] = c >> 8;
                s[j + 1] = c;
                j += 2;
            }
        }
    }
    s[j] = 0;
}

/* PageObj_InitPool: the 9 layers empty, the 512 objects free. */
void PageObj_InitPool(void)
{
    s32 i;
    BrowserObj *o;
    s32 n;

    for (i = 0; i < 9; i++) {
        gBrowserWork->layerHead[i] = NULL;
        gBrowserWork->layerTail[i] = NULL;
    }
    n = 0x200;
    gBrowserWork->unk1A640 = 0;
    gBrowserWork->unk1A644 = 0;
    gBrowserWork->freeObj = o = gBrowserWork->objPool;
    while (n-- != 0) {
        memset(o, 0, sizeof(BrowserObj));
        if (n != 0) {
            o->next = o + 1;
        }
        o++;
    }
}

/* PageObj_Take: one object from the free list. */
static BrowserObj *PageObj_Take(void)
{
    BrowserObj *o = gBrowserWork->freeObj;

    if (o != NULL) {
        gBrowserWork->freeObj = o->next;
    }
    return o;
}

/* PageObj_Give: back to the free list, cleared. */
static void PageObj_Give(BrowserObj *o)
{
    memset(o, 0, sizeof(BrowserObj));
    o->next = gBrowserWork->freeObj;
    gBrowserWork->freeObj = o;
}

/* PageObj_Alloc: an object in layer (0..8), at the head or the tail. */
BrowserObj *PageObj_Alloc(s32 layer, s32 atTail)
{
    BrowserObj *p;
    BrowserObj *o = PageObj_Take();

    if (o == NULL) {
        return NULL;
    }
    o->layer = layer;
    if (atTail == 0) {
        o->next = p = gBrowserWork->layerHead[layer];
        gBrowserWork->layerHead[layer] = o;
        o->prev = NULL;
        if (p == NULL) {
            gBrowserWork->layerTail[layer] = o;
        } else {
            p->prev = o;
        }
    } else {
        o->prev = p = gBrowserWork->layerTail[layer];
        gBrowserWork->layerTail[layer] = o;
        o->next = NULL;
        if (p == NULL) {
            gBrowserWork->layerHead[layer] = o;
        } else {
            p->next = o;
        }
    }
    return o;
}

/* PageObj_Free: out of its layer, back to the pool. */
void PageObj_Free(BrowserObj *o)
{
    BrowserObj *prev = o->prev;

    if (prev == NULL) {
        BrowserObj *next;

        gBrowserWork->layerHead[o->layer] = next = o->next;
        if (next == NULL) {
            gBrowserWork->layerTail[o->layer] = NULL;
        } else {
            next->prev = NULL;
        }
    } else if (o->next == NULL) {
        gBrowserWork->layerTail[o->layer] = prev;
        prev->next = NULL;
    } else {
        prev->next = o->next;
        o->next->prev = o->prev;
    }
    PageObj_Give(o);
}

/* PageObj_UpdateLayer: the update function of each active object. */
void PageObj_UpdateLayer(s32 layer)
{
    BrowserObj *o = gBrowserWork->layerHead[layer];
    BrowserObj *next;

    if (o != NULL) {
        do {
            next = o->next;
            if (o->active) {
                o->update(o);
            }
            o = next;
        } while (next != NULL);
    }
}

/* PageObj_DrawLayer: the draw function of each visible object. */
void PageObj_DrawLayer(s32 layer)
{
    BrowserObj *o = gBrowserWork->layerHead[layer];
    BrowserObj *next;

    if (o != NULL) {
        do {
            next = o->next;
            if (o->visible) {
                o->draw(o);
            }
            o = next;
        } while (next != NULL);
    }
}

/* Tex_ReleaseAll: the page's textures and palettes. */
void PageTex_ReleaseAll(void)
{
    s32 i;
    u32 *t = gBrowserWork->tex;

    for (i = 0; i < gBrowserWork->nTex; i++, t++) {
        if (*t != 0) {
            u16 h;

            if ((*t & 0xFFFF0000) >> 16) {
                flReleasePaletteHandle((*t & 0xFFFF0000) >> 16);
            }
            h = *(u16 *)t;
            if (h) {
                flReleaseTextureHandle(h);
            }
            *t = 0;
        }
    }
    gBrowserWork->nTex = 0;
    Mem_Compact();
}

/* Tex_Add: -1 when the 24 slots are full. */
s32 PageTex_Add(u32 tex)
{
    s8 n = gBrowserWork->nTex;

    if (n == 24) {
        return -1;
    }
    gBrowserWork->tex[n] = tex;
    gBrowserWork->nTex++;
    return 0;
}

/* Tex_Get */
u32 PageTex_Get(s8 i)
{
    if (i < 0 || gBrowserWork->nTex <= i) {
        return -1;
    }
    return gBrowserWork->tex[i];
}

/* Browser_LoadFrame: the frame's texture, in steps (*step 0 to 4). */
void Browser_LoadFrame(s32 mode, u8 *step)
{
    switch (mode) {
    case 3:
        switch (*step) {
        case 0:
            Http_ReadFile(gLoadBuf, 0x83);
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

/* unk_730AE0's .data (0x7EEA40-0x7EEAC0) */
extern char D_00839018[];
UrlScheme gUrlSchemes[7] = {
#include "lobby/007EEA40.inc"
};
char *gScriptSchemes[2] = {
    D_00839018, NULL,
};
