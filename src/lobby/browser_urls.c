/*
 * The browser's URLs and the page's object pool. First the URLs: the
 * browser's current URL (Browser_SetUrl), comparing and copying URLs, the
 * base URL that relative links resolve against, resolving a link against
 * it (relative paths, the current and parent directories, "//", schemes),
 * the schemes the browser knows (gUrlSchemes) and the script scheme it
 * refuses (gScriptSchemes), and a URL's file type (URL_TYPE_*). Url_FileType
 * keeps registers across the static Url_PathEnd and Url_LastDot, so they
 * are compiled with it in one file, as in the game.
 *
 * Then URL escapes and the Shift-JIS and EUC-JP conversions, the page's
 * object pool and its object lists (BLAYER_*), and the page's textures.
 * The work area is BrowserWork (browser.h).
 */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
u32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, u32 n);
char *strncpy(char *dst, const char *src, u32 n);
void *memmove(void *dst, const void *src, u32 n);
void Mem_Compact(void);
s32 flReleasePaletteHandle(u32);
/* unit's own declaration: declared with a u16 handle (the definition takes u32): so PageTex_ReleaseAll passes the handle's low 16 bits, as the game's code does */
s32 flReleaseTextureHandle(u16);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
s32 Http_ReadFile(void *p, s32 a1);            /* nethttp */
void Url_Copy(char *dst, const char *src);
s32 Url_SpecialScheme(const char *url);
s32 Url_GetScheme(const char *url);
void Str_ToLower(char *s);

extern char *gScriptSchemes[]; /* the script scheme, then NULL */
extern UrlScheme gUrlSchemes[]; /* the schemes the browser loads, then a zero entry */

void Url_ClearBase(void);

/* The script scheme's text, gScriptSchemes' first entry (.rodata). */
#pragma rodata on
char gScriptSchemes_0[0xC] = {
#include "lobby/00839018.inc"
};
#pragma rodata off

/* Compares two URLs, as strncmp over a URL's length. */
s32 Url_Compare(const char *url, const char *other)
{
    return strncmp(url, other, BROWSER_URL_LEN);
}

/* Copies a URL, cut to BROWSER_URL_LEN - 1 characters and always
 * terminated. */
void Url_Copy(char *dst, const char *src)
{
    strncpy(dst, src, BROWSER_URL_LEN - 1);
    dst[BROWSER_URL_LEN - 1] = 0;
}

/* Points work's URL at url (its callers pass gBrowserWork). */
void Browser_SetUrl(BrowserWork *w, char *url)
{
    w->url = url;
}

void Url_SetBase(char *url)
{
    Url_ClearBase();
    Url_Copy(gBrowserWork->baseUrl, url);
}

/* Clears the base URL. */
void Url_ClearBase(void)
{
    memset(gBrowserWork->baseUrl, 0, sizeof(gBrowserWork->baseUrl));
}

/* Returns the end of a URL's path: its '?', its '#' or its end. */
static char *Url_PathEnd(char *pos)
{
    while (*pos != 0 && *pos != '?' && *pos != '#') {
        pos++;
    }
    return pos;
}

/* Returns the last '/' at or before end - 1, not before start, or NULL. */
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

/* Returns the '.' of the last path segment's extension, or NULL. */
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

/* Returns where the path starts ("/" after the host; for file:
 * URLs the '/' of "file://"). */
static char *Url_FindPath(char *pos, char *end)
{
    s32 state = 0;

    switch ((u8)Url_GetScheme(pos)) {
    case URL_OTHER:
        break;
    case URL_FILE:
        return pos + 6;
    default:
        for (; pos < end; pos++) {
            switch (state) {
            case 0:
                if (!(pos + 1 < end)) {
                    return NULL;
                }
                if (pos[0] == '/' && pos[1] == '/') {
                    state++;
                    pos++;
                }
                break;
            case 1:
                if (!(pos < end)) {
                    return NULL;
                }
                if (*pos == '/') {
                    return pos;
                }
                break;
            }
        }
        break;
    }
    return NULL;
}

/* Returns the first '/' from p up to end, or NULL. */
static char *Url_FindSlash(char *pos, char *end)
{
    for (; pos < end; pos++) {
        if (*pos == '/') {
            return pos;
        }
    }
    return NULL;
}

/* Returns the first "//" in url up to end, or NULL. */
static char *Url_FindDoubleSlash(char *pos, char *end)
{
    for (; pos + 1 < end; pos++) {
        if (pos[0] == '/' && pos[1] == '/') {
            return pos;
        }
    }
    return NULL;
}

/* Returns the first "/./" in url up to end, or NULL. */
static char *Url_FindCurrentDir(char *pos, char *end)
{
    for (; pos + 2 < end; pos++) {
        if (pos[0] == '/' && pos[1] == '.' && pos[2] == '/') {
            return pos;
        }
    }
    return NULL;
}

/* Returns the first "/../" in url up to end, or NULL. */
static char *Url_FindParentDir(char *pos, char *end)
{
    for (; pos + 3 < end; pos++) {
        if (pos[0] == '/' && pos[1] == '.' && pos[2] == '.' && pos[3] == '/') {
            return pos;
        }
    }
    return NULL;
}

/* Makes url absolute in place: a relative url gets base's
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
            char *pathEnd = Url_PathEnd(base);
            s32 n;

            if (*url == '/') {
                pathEnd = Url_FindPath(base, pathEnd);
            } else {
                pathEnd = Url_LastSlash(base, pathEnd);
            }
            if (pathEnd == NULL) {
                return NULL;
            }
            n = pathEnd - base + 1;
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

/* Resolves url in place against base, or against the page's
 * base URL when base is NULL or empty: made absolute, "//" collapsed,
 * "/../" and "/./" removed (for other schemes than http, leading "../"
 * are kept). NULL for "javascript:" links and bad URLs. */
char *Url_Resolve(char *base, char *url)
{
    char *pos;
    char *end;

    if (Url_SpecialScheme(url) != -1) {
        return NULL;
    }
    if (base == NULL || *base == 0) {
        base = "http:///";
    }
    if ((pos = Url_Combine(base, gBrowserWork->baseUrl, 1)) == NULL) {
        return NULL;
    }
    url = Url_Combine(pos, url, 0);
    if (url == NULL) {
        return NULL;
    }
    end = Url_PathEnd(url);
    /* base is the path from here on */
    base = Url_FindPath(url, end);
    if (base == NULL) {
        return NULL;
    }
    while ((pos = Url_FindDoubleSlash(base, end)) != NULL) {
        memmove(pos, pos + 1, strlen(pos + 1) + 1);
        end = Url_PathEnd(base);
    }
    switch ((u8)Url_GetScheme(url)) {
    case URL_HTTP:
        while ((pos = Url_FindParentDir(base, end)) != NULL) {
            if (pos != base) {
                break;
            }
            memmove(pos, pos + 3, strlen(pos + 3) + 1);
            end = Url_PathEnd(base);
        }
        while ((pos = Url_FindParentDir(base, end)) != NULL) {
            end = Url_LastSlash(base, pos - 1);
            if (end == NULL) {
                return NULL;
            }
            memmove(end, pos + 3, strlen(pos + 3) + 1);
            end = Url_PathEnd(base);
        }
        break;
    default:
        while ((pos = Url_FindParentDir(base, end)) != NULL) {
            if (pos != base) {
                end = Url_LastSlash(base, pos - 1);
                if (end == NULL) {
                    return NULL;
                }
                memmove(end, pos + 3, strlen(pos + 3) + 1);
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


/* 'A'..'Z' to 'a'..'z', in place. */
void Str_ToLower(char *str)
{
    char character;

    character = *str;
    if (character != 0) {
        do {
            if (character >= 'A' && character < 'Z' + 1) {
                *str += 0x20;
            }
            str++;
            character = *str;
        } while (character != 0);
    }
}

/* Returns the index in gScriptSchemes of url's scheme (case
 * ignored), or -1 for an ordinary link. */
s32 Url_SpecialScheme(const char *url)
{
    s32 i = 0;
    char **script = gScriptSchemes;
    char *scheme;

    Url_Copy(gBrowserWork->urlLower, url);
    Str_ToLower(gBrowserWork->urlLower);
    scheme = gScriptSchemes[0];
    while (scheme != NULL) {
        if (strncmp(gBrowserWork->urlLower, *script, strlen(scheme)) == 0) {
            break;
        }
        script++;
        scheme = *script;
        i++;
    }
    if (*script != NULL) {
        return i;
    }
    i = -1;
    return i;
}

/* Returns url's scheme: URL_FILE .. URL_AFS, or URL_OTHER for a relative or
 * unknown URL. */
s32 Url_GetScheme(const char *url)
{
    UrlScheme *scheme = gUrlSchemes;

    if (gUrlSchemes[0].id != 0) {
        do {
            if (strncmp(url, scheme->prefix, strlen(scheme->prefix)) == 0) {
                break;
            }
            scheme++;
        } while (scheme->id != 0);
    }
    return (s8)scheme->id;
}

/* Returns url's file type by its extension, case ignored: URL_TYPE_HTML
 * (".htm", ".html"), _PNG, _BMP, _MMBBIN, or URL_TYPE_NONE. */
s32 Url_FileType(char *url)
{
    char *end = Url_PathEnd(url);
    char *dot = Url_LastDot(url, end);

    if (dot == NULL) {
        return URL_TYPE_NONE;
    }
    if (dot[1] != 0 && dot[2] != 0 && dot[3] != 0 && dot + 4 == end) {
        if ((dot[1] == 'P' || dot[1] == 'p') && (dot[2] == 'N' || dot[2] == 'n') && (dot[3] == 'G' || dot[3] == 'g')) {
            return URL_TYPE_PNG;
        }
        if ((dot[1] == 'B' || dot[1] == 'b') && (dot[2] == 'M' || dot[2] == 'm') && (dot[3] == 'P' || dot[3] == 'p')) {
            return URL_TYPE_BMP;
        }
        if ((dot[1] == 'H' || dot[1] == 'h') && (dot[2] == 'T' || dot[2] == 't') && (dot[3] == 'M' || dot[3] == 'm')) {
            return URL_TYPE_HTML;
        }
    }
    if (dot[1] != 0 && dot[2] != 0 && dot[3] != 0 && dot[4] != 0 && dot + 5 == end) {
        if ((dot[1] == 'H' || dot[1] == 'h') && (dot[2] == 'T' || dot[2] == 't') && (dot[3] == 'M' || dot[3] == 'm') &&
            (dot[4] == 'L' || dot[4] == 'l')) {
            return URL_TYPE_HTML;
        }
    }
    if (dot[1] != 0 && dot[2] != 0 && dot[3] != 0 && dot[4] != 0 && dot[5] != 0 && dot[6] != 0 && dot + 7 == end) {
        if ((dot[1] == 'M' || dot[1] == 'm') && (dot[2] == 'M' || dot[2] == 'm') && (dot[3] == 'B' || dot[3] == 'b') &&
            (dot[4] == 'B' || dot[4] == 'b') && (dot[5] == 'I' || dot[5] == 'i') && (dot[6] == 'N' || dot[6] == 'n')) {
            return URL_TYPE_MMBBIN;
        }
    }
    return URL_TYPE_NONE;
}

/* "%XX" for each byte of src; the length written. */
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

/* Converts one double-byte character from Shift-JIS to EUC-JP. */
static u32 Sjis_ToEuc(u32 code)
{
    u32 highByte = (code >> 8) & 0xFF;
    u32 lowByte = code & 0xFF;
    u32 row;

    row = ((highByte - (highByte < 0xA0 ? 0x71 : 0xB1)) << 1) + 1;
    if (lowByte >= 0x9E) {
        lowByte -= 0x7E;
        row++;
    } else if (lowByte > 0x7F) {
        lowByte -= 0x20;
    } else {
        lowByte -= 0x1F;
    }
    lowByte |= 0x80;
    row |= 0x80;
    if (lowByte == 0xA0) {
        lowByte = 0xFE;
        row--;
    }
    return (row << 8) | lowByte;
}

/* Converts one double-byte character from EUC-JP to Shift-JIS. */
static u32 Euc_ToSjis(u32 code)
{
    u32 highByte;
    u32 lowByte;

    lowByte = code & 0xFFFF7F7F;
    highByte = (lowByte >> 8) & 0xFF;
    lowByte &= 0xFF;
    lowByte += (highByte & 1) ? 0x1F : 0x7D;
    if (lowByte >= 0x7F) {
        lowByte++;
    }
    highByte = ((highByte - 0x21) >> 1) + 0x81;
    if (highByte > 0x9F) {
        highByte += 0x40;
    }
    return (highByte << 8) | lowByte;
}

/* Converts str from Shift-JIS to EUC-JP in place; half-width katakana
 * become '_'. */
void Str_SjisToEuc(u8 *str)
{
    s32 len = strlen((char *)str);
    s32 i;
    s32 j;
    s32 converted;

    for (i = 0, j = 0; i < len; i++) {
        s32 code;
        u8 *cur = &str[i];

        code = *cur;

        if (code <= 0x7F) {
            str[j++] = code;
        } else if (code >= 0xA0 && code <= 0xDF) {
            str[j++] = '_';
        } else {
            code = (code << 8) | cur[1];
            i++;
            converted = Sjis_ToEuc(code);
            str[j] = converted >> 8;
            str[j + 1] = converted;
            j += 2;
        }
    }
    str[j] = 0;
}

/* Converts str from EUC-JP to Shift-JIS in place; half-width katakana
 * (0x8E, then the character) are kept
 * as their second byte. */
void Str_EucToSjis(u8 *str)
{
    s32 len = strlen((char *)str);
    s32 i;
    s32 j;
    s32 code;

    for (i = 0, j = 0; i < len; i++) {
        code = str[i];
        if (code <= 0x7F) {
            str[j++] = code;
        } else {
            code = (code << 8) | str[i + 1];
            i++;
            if (code > 0x8EA0 && code <= 0x8EFE) {
                str[j++] = code;
            } else {
                code = Euc_ToSjis(code);
                str[j] = code >> 8;
                str[j + 1] = code;
                j += 2;
            }
        }
    }
    str[j] = 0;
}

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
    n = 0x200;
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
extern char gScriptSchemes_0[];
UrlScheme gUrlSchemes[7] = {
#include "lobby/007EEA40.inc"
};
char *gScriptSchemes[2] = {
    gScriptSchemes_0, NULL,
};

void PageObj_Nop(void)
{
}
