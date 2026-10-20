/* The portal's frame and dialogs: the frame's objects each frame, the draw
 * commands (Draw_Cmd), loading a page (Page_Load), the images, the timers,
 * the modes 2 and 3 (the end), the sub-dialogs (the list, yes/no ...).
 * include/browser.h. */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "kbd.h"

/* Compiled at optimization level 4 (Dialog_PageStep matches only
 * so; every other function matches at 3 and 4). */
#pragma optimization_level 4

void *memcpy(void *dst, const void *src, u32 n);
void Callbacks_Add(void (*fn)(void));
void Font_SetHalfWidth(s32 a0);
void LobbySe_Play(s32 se);                    /* Se_Play? */
BrowserResult *BrowserReq_Poll(char **url);
void Browser_DrawOverlay(void);
void BrowserReq_Image(char **url);                /* Http_GetImage? */
void Browser_SetUrl(BrowserWork *w, char *url);
void Browser_SetRenderState(s32 state, u32 value);
void Browser_SetStep(s8 step);
void Browser_Reload(void);
void Browser_Fail(u8 page);
void Browser_Exit(s8 result);
void Browser_ListCursor(u8 max);
void Dialog_Close(s32 yes);
void Browser_SaveLogin(void);
s32 BrowserForm_Add(const char *name, char *value); /* Cookie_Set? */
void BrowserReq_Post(char **url);
void Browser_Go(char *url);

void *memset(void *dst, s32 c, u32 n);
u32 strlen(const char *s);
s32 Browser_GoStart(void);
char *BrowserHist_CurUrl(void);
extern char gBrowserUrl[]; /* the start URL */
void Portal_Back(void);
void Dialog_ResetMsgState(void);

/* The left stick's 16 directions: a step up or down a list (.data, copied
 * to the stack whole). */
typedef struct StickSteps {
    s8 v[16];
} StickSteps __attribute__((aligned(16)));
extern StickSteps gDialogMenuStickSteps;
extern StickSteps gDialogChoiceStickSteps;
void Browser_GoBack(void);
void Page_Navigate(void);
void Page_Clamp(void);
s32 BrowserReq_CanReload(void);                       /* History_CanBack? */
s32 memcmp(const char *a, const char *b, u32 n); /* main: strncmp */
extern char gLbsScheme[]; /* "lbs://" */
extern char D_00839110[];
extern char D_00839130[];
void Browser_GoForward(void);
s32 BrowserHist_CanForward(void);
s32 BrowserHist_CanBack(void);
void Page_Click(void);
void Page_PointerScroll(void);
void Page_DragScroll(void);
void Page_ScrollInput(void);
void Page_PointAt(void);

extern u8 gPortalPage; /* sub_main: how the browser was entered (0 .. 3); named in lobby_browser.txt */
s32 strcmp(const char *a, const char *b); /* main: strcmp */
extern char D_00839140[]; /* "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp" */
void PageObj_UpdateLayer(s32 layer);
void PageObj_Free(BrowserObj *o);
void PageObj_InitPool(void);
void PageTex_ReleaseAll(void);
void DlgBackImg_Create(s32 mode);
void DlgPage_Create(s32 mode);
void DlgVScroll_Create(s32 mode);
void DlgHScroll_Create(s32 mode);
void DlgStatus_Create(s32 mode);
void DlgMenu_Create(s32 mode);
void DlgInput_Create(s32 mode);
void DlgPointer_Create(s32 mode);
void DlgBox_Create(s32 mode);
void DlgBackImg_DrawAll(void);
void DlgPage_DrawAll(void);
void DlgVScroll_DrawAll(void);
void DlgHScroll_DrawAll(void);
void DlgStatus_DrawAll(void);
void DlgMenu_DrawAll(void);
void DlgInput_DrawAll(void);
void DlgPointer_DrawAll(void);
void DlgBox_DrawAll(void);

extern u8 gConnCheckState;       /* the lobby connection's state */
extern u8 gLobbyPageData[];     /* the registration record kept */

/* The USB keyboard's state (sub_main): +0x668 the key pressed. */

/* The sub-dialogs' steps (.data). */
extern void (*jtbl_007EEB78[])(void);
extern void (*jtbl_007EEB90[])(void);

/* Portal step 15: once answered, the portal fails (mode 2). */
void Portal_FailStep(void)
{
    if (BrowserReq_Poll(&gBrowserWork->url) != NULL) {
        gBrowserWork->reqBusy = 0;
        Browser_Fail(0);
        gBrowserState->mode = BMODE_END;
        gBrowserState->step = 0;
    }
}

/* Browser_UpdateFrame: the text colour, then the frame's objects to update
 * and draw this frame (only the dialog box and the cursor while a
 * dialog is up). */
void Browser_UpdateFrame(void)
{
    BrowserState *st = gBrowserState;
    s32 plain = st->initMode;

    if (plain != 0 && st->page != PAGE_LOAD_DONE && st->fade >= 0) {
        Browser_SetRenderState(0x14, 0xFF000001);
        PageObj_UpdateLayer(6);
        Callbacks_Add(DlgBox_DrawAll);
        return;
    }
    if (plain == 0) {
        Browser_SetRenderState(0x14, st->bgColor);
    } else if (gBrowserWork->dlgBoxKind == BDLG_KIND_NONE && st->fade == 0) {
        Browser_SetRenderState(0x14, st->bgColor);
    } else {
        Browser_SetRenderState(0x14, 0xFF000001);
    }
    if (gBrowserWork->endReason > 0) {
        PageObj_UpdateLayer(6);
        Callbacks_Add(DlgBox_DrawAll);
        PageObj_UpdateLayer(8);
        Callbacks_Add(DlgPointer_DrawAll);
        return;
    }
    if (gBrowserWork->firstPage != 0) {
        PageObj_UpdateLayer(6);
        Callbacks_Add(DlgBox_DrawAll);
        return;
    }
    PageObj_UpdateLayer(0);
    Callbacks_Add(DlgBackImg_DrawAll);
    PageObj_UpdateLayer(1);
    Callbacks_Add(DlgPage_DrawAll);
    PageObj_UpdateLayer(2);
    Callbacks_Add(DlgVScroll_DrawAll);
    Callbacks_Add(DlgHScroll_DrawAll);
    Callbacks_Add(DlgStatus_DrawAll);
    PageObj_UpdateLayer(3);
    Callbacks_Add(DlgMenu_DrawAll);
    PageObj_UpdateLayer(4);
    Callbacks_Add(DlgInput_DrawAll);
    if (gBrowserWork->dlgInputState == BDLG_INPUT_OPEN) {
        return;
    }
    if (gBrowserState->page != PAGE_LOAD_DONE) {
        PageObj_UpdateLayer(8);
        Callbacks_Add(DlgPointer_DrawAll);
        PageObj_UpdateLayer(6);
        Callbacks_Add(DlgBox_DrawAll);
        return;
    }
    PageObj_UpdateLayer(6);
    Callbacks_Add(DlgBox_DrawAll);
    PageObj_UpdateLayer(8);
    Callbacks_Add(DlgPointer_DrawAll);
}

void Browser_InitNothing(void)
{
}

void Browser_InitHook(void)
{
    Browser_InitNothing();
}

s32 PageObj_Activate(u16 idx, s32 sound);

/* Draw_Cmd's form values (kinds 6 and 7): src copied to the command buffer as
 * the object's s2, in a maxlen + 1 byte field (the rest skipped); the whole
 * command is dropped when the buffer can't take it. */
#define DRAW_VALUE(src)                                                        \
    if (maxlen + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= 0x8000) {    \
        return;                                                                \
    }                                                                          \
    gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr;                   \
    len = strlen((char *)(src));                                               \
    for (j = 0; j < len; j++) {                                                \
        *gBrowserWork->cmdPtr++ = (src)[j];                                    \
    }                                                                          \
    *gBrowserWork->cmdPtr++ = 0;                                               \
    gBrowserWork->cmdPtr += maxlen - len;

/* The field of the registration record named by the object's name ("00" ..
 * "08"), cut to maxlen; the value given (DRAW_VALUE) when it is empty. */
#define DRAW_FIELD(f)                                                          \
    len = strlen((char *)gRegRecord.f);                                        \
    if (len > 0) {                                                             \
        gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr;               \
        if (maxlen < len) {                                                    \
            len = maxlen;                                                      \
        }                                                                      \
        for (j = 0; j < len; j++) {                                            \
            *gBrowserWork->cmdPtr++ = gRegRecord.f[j];                         \
        }                                                                      \
        *gBrowserWork->cmdPtr++ = 0;                                           \
        gBrowserWork->cmdPtr += maxlen - len;                                  \
        break;                                                                 \
    }                                                                          \
    goto dflt;

/* Draw_Cmd: a page's draw command made an object: the first free slot of
 * gBrowserWork->obj (none: dropped), its strings s1 .. s3 copied to the
 * command buffer (cmdBuf). kind 6/7 (text inputs) take their value (s2) from
 * the kept login (field 1 the id, also copied to gMmbbData->userId; 2 the
 * password) or the registration record (field 4); kind 8 (forms) number the
 * forms (sub 10) and radio buttons (sub 20) and close a form (sub 30: the
 * form's object becomes sub 11); kind 13's text (sub 2, 3, 5) is cut to the
 * box's width. Then the page's objects are chosen again (PageObj_Activate). */
void Draw_Cmd(u8 kind, u8 sub, u16 x, u16 y, u16 x2, u16 y2, u32 color, u8 size, u8 field,
              u8 maxlen, u8 *s1, u8 *s2, u8 *s3)
{
    u8 buf[0x100];
    u16 n;
    u16 len3;
    u8 ok;
    u8 grp;
    u8 cnt;
    u32 i;
    u32 j;
    u8 len;
    u8 num;
    BrowserObj *o;

    cnt = 1;
    ok = 1;
    grp = 0;
    switch (gBrowserTopMode) {
    default:
        return;
    case BROWSER_TOP_PORTAL:
        break;
    }
    i = 0;
    x += gBrowserState->marginLeft;
    y += gBrowserState->marginTop;
    x2 += gBrowserState->marginLeft;
    y2 += gBrowserState->marginTop;
    for (;;) {
        if (kind == 0) {
            break;
        }
        if (i == 500) {
            return;
        }
        if (gBrowserWork->obj[i]->active == 0) {
            break;
        }
        i++;
    }
    switch (kind) {
    case 6:
    case 7:
        if (field > 0 && field < 4 && strlen((char *)s2) > 16) {
            return;
        }
        break;
    case 13:
        if (sub == 2 || sub == 3 || sub == 5) {
            len3 = strlen((char *)s3);
            memset(buf, 0, 0xFF);
            if (y2 - y <= 0x10 || len3 <= 0) {
                buf[0] = 0;
                len3 = 0;
            } else if ((len3 + 1) * 16 / 2 >= x2 - x) {
                n = (x2 - x) / 8 - 1;
                if (n >= 0xFF) {
                    n = 0xFF;
                }
                memcpy(buf, s3, n);
                buf[n] = 0;
                len3 = strlen((char *)buf);
            } else {
                memcpy(buf, s3, len3 + 1);
            }
        }
        break;
    case 8:
        switch (sub) {
        case 10:
            for (j = 0; j < 500; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == 8) {
                    grp = gBrowserWork->obj[j]->group;
                }
            }
            if (!grp) {
                gBrowserWork->obj[i]->group = 1;
            } else {
                gBrowserWork->obj[i]->group = grp + 1;
            }
            gBrowserWork->obj[i]->tex = 0;
            gBrowserWork->obj[i]->active = 11;
            break;
        case 20:
            for (j = 0; j < 500; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == 8) {
                    grp = gBrowserWork->obj[j]->group;
                }
            }
            for (j = 0; j < 500; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == 8 && gBrowserWork->obj[j]->group == grp && (gBrowserWork->obj[j]->active & 0xFF) == 0x15) {
                    cnt++;
                }
            }
            sub++;
            gBrowserWork->obj[i]->group = grp;
            gBrowserWork->obj[i]->tex = cnt;
            break;
        case 30:
            for (i = 0; i < 500; i++) {
                if (gBrowserWork->obj[i]->kind == 8 && gBrowserWork->obj[i]->active == 10) {
                    sub = 11;
                    ok = 0;
                    break;
                }
            }
            if (i == 500) {
                return;
            }
            for (j = 0; j < 500; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == 8 && (gBrowserWork->obj[j]->active & 0xFF) == 0x15 && gBrowserWork->obj[j]->sub != 0) {
                    maxlen = gBrowserWork->obj[j]->tex;
                    break;
                }
            }
            if (maxlen > 0) {
                gBrowserWork->obj[i]->s1 = gBrowserWork->obj[j]->s1;
            } else {
                for (j = 0; j < 500; j++) {
                    if (gBrowserWork->obj[j]->kind == 8 && gBrowserWork->obj[j]->active == 0x15 && gBrowserWork->obj[j]->tex == 1) {
                        gBrowserWork->obj[i]->s1 = gBrowserWork->obj[j]->s1;
                        maxlen = 1;
                        break;
                    }
                }
            }
            for (j = 0; j < 500; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == 8 && (gBrowserWork->obj[j]->active & 0xFF) == 0x15) {
                    gBrowserWork->obj[j]->active = 0x16;
                }
            }
            break;
        }
        break;
    }
    if (ok) {
        if ((gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) == 0x8000) {
            gBrowserWork->obj[i]->s1 = (char *)gBrowserWork->cmdPtr - 1;
            gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr - 1;
            gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
            return;
        }
        len = strlen((char *)s1);
        if (len + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= 0x8000) {
            gBrowserWork->obj[i]->s1 = (char *)gBrowserWork->cmdPtr - 1;
            gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr - 1;
            gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
            return;
        }
        if (len == 0) {
            gBrowserWork->obj[i]->s1 = (char *)gBrowserWork->cmdPtr - 1;
        } else {
            gBrowserWork->obj[i]->s1 = (char *)gBrowserWork->cmdPtr;
            for (j = 0; j < len; j++) {
                *gBrowserWork->cmdPtr++ = s1[j];
            }
            *gBrowserWork->cmdPtr++ = 0;
        }
        switch (kind) {
        case 6:
        case 7:
            switch (field) {
            case 0:
                DRAW_VALUE(s2)
                break;
            case 1:
                if (strlen((char *)gBrowserState->loginId) != 0) {
                    if (maxlen + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= 0x8000) {
                        return;
                    }
                    gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr;
                    len = strlen((char *)gBrowserState->loginId);
                    for (j = 0; j < len; j++) {
                        *gBrowserWork->cmdPtr++ = gBrowserState->loginId[j];
                    }
                    *gBrowserWork->cmdPtr++ = 0;
                    gBrowserWork->cmdPtr += maxlen - len;
                } else {
                    DRAW_VALUE(s2)
                }
                num = strlen((char *)s2);
                if (num > 14) {
                    num = 0xE;
                }
                memset(gMmbbData->userId, 0, 0x11);
                memcpy(gMmbbData->userId, s2, num);
                break;
            case 2:
                len = strlen((char *)gBrowserState->password);
                if (len > 0) {
                    if (maxlen + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= 0x8000) {
                        return;
                    }
                    gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr;
                    for (j = 0; j < len; j++) {
                        *gBrowserWork->cmdPtr++ = gBrowserState->password[j];
                    }
                    *gBrowserWork->cmdPtr++ = 0;
                    gBrowserWork->cmdPtr += maxlen - len;
                } else {
                    DRAW_VALUE(s2)
                }
                break;
            case 3:
                DRAW_VALUE(s2)
                break;
            case 4:
                o = gBrowserWork->obj[i];
                switch ((((u8 *)o->s1)[0] - '0') * 10 + (((u8 *)o->s1)[1] - '0')) {
                case 0:
                    DRAW_FIELD(box00)
                case 1:
                    DRAW_FIELD(box01)
                case 2:
                    gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr;
                    if (gRegRecord.box02 / 10 != 0) {
                        num = 2;
                        *gBrowserWork->cmdPtr++ = gRegRecord.box02 / 10 + '0';
                        *gBrowserWork->cmdPtr++ = gRegRecord.box02 % 10 + '0';
                    } else if (gRegRecord.box02 != 0) {
                        num = 1;
                        *gBrowserWork->cmdPtr++ = gRegRecord.box02 % 10 + '0';
                    } else {
                        num = 0;
                    }
                    *gBrowserWork->cmdPtr++ = 0;
                    gBrowserWork->cmdPtr += maxlen - num;
                    break;
                case 3:
                    DRAW_FIELD(box03)
                case 8:
                    DRAW_FIELD(box00)
                case 5:
                    DRAW_FIELD(box05)
                case 7:
                    DRAW_FIELD(box07)
                default:
                dflt:
                    DRAW_VALUE(s2)
                    break;
                }
                break;
            }
            break;
        default:
            len = strlen((char *)s2);
            if (len + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= 0x8000) {
                gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr - 1;
                gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
                return;
            }
            if (len == 0) {
                gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr - 1;
            } else {
                gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr;
                for (j = 0; j < len; j++) {
                    *gBrowserWork->cmdPtr++ = s2[j];
                }
                *gBrowserWork->cmdPtr++ = 0;
            }
            len = strlen((char *)s3);
            if (kind == 13 && (sub == 2 || sub == 3 || sub == 5)) {
                len = len3;
            }
            if (len + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= 0x8000) {
                gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
                return;
            }
            if (len == 0) {
                gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
            } else {
                gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr;
                if (kind == 13 && (sub == 2 || sub == 3 || sub == 5)) {
                    for (j = 0; j < len; j++) {
                        *gBrowserWork->cmdPtr++ = buf[j];
                    }
                } else {
                    for (j = 0; j < len; j++) {
                        *gBrowserWork->cmdPtr++ = s3[j];
                    }
                }
                *gBrowserWork->cmdPtr++ = 0;
            }
        }
    }
    gBrowserWork->obj[i]->active = sub;
    gBrowserWork->obj[i]->kind = kind;
    gBrowserWork->obj[i]->x = x;
    gBrowserWork->obj[i]->y = y;
    gBrowserWork->obj[i]->w = x2 - x;
    gBrowserWork->obj[i]->h = y2 - y;
    gBrowserWork->obj[i]->color = color;
    gBrowserWork->obj[i]->size = size;
    gBrowserWork->obj[i]->field = field;
    gBrowserWork->obj[i]->sub = maxlen;
    for (j = 0; j < 500; j++) {
        if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0 || PageObj_Activate(j, 0)) {
            break;
        }
    }
    if (gBrowserWork->focus > 0 && gBrowserWork->obj[gBrowserWork->focus]->y > (f32)(0x19C - gBrowserState->marginBottom)) {
        gBrowserWork->focus = 0;
        gBrowserState->y = 0;
    }
}

/* Page_Load: the page's answer, once it came: 0 none yet, 1 the page (its
 * HTML kept at pageData), 2 gone elsewhere (the start URL again, or a
 * redirect: 11), -1 an error, -2 an error that ends the portal (no page,
 * or too many retries). Its callers see an s8. */
s32 Page_Load(void)
{
    BrowserResult *r = BrowserReq_Poll(&gBrowserWork->url);

    if (r == NULL) {
        return 0;
    }
    gBrowserWork->reqBusy = 0;
    gBrowserState->forceStyle8 = 0;
    switch (r->result) { /* 0 the page, 1 no connection, 2 ... 14 */
    case 14:
        gBrowserState->forceStyle8 = 1;
    case 0:
        gBrowserWork->pageData = r->data;
        memset(gBrowserState->msg, 0, 0x100);
        if (strlen(gBrowserWork->pageData) == 0) {
            gBrowserWork->tries = 2;
            gBrowserState->pageMode = CSV_KIND_NONE;
            return -2;
        }
        if (gBrowserWork->atMmbbUrl != 0) {
            switch (gPortalPage) {
            case PORTAL_PAGE_BACK:
            case PORTAL_PAGE_NEXT:
                gBrowserState->bgColor = 0xFF000001;
                gBrowserState->plain = 1;
                break;
            }
        }
        if (gBrowserWork->startPosts > 1) {
            gBrowserWork->tries = 2;
            gBrowserState->pageMode = CSV_KIND_NONE;
            return -2;
        }
        return 1;
    case 1:
        if (gBrowserWork->firstPage != 0) {
            gBrowserWork->tries = 2;
            return -2;
        }
        return -1;
    case 2:
        gBrowserWork->pageData = r->data;
        if (Browser_GoStart() != 0) {
            return 2;
        }
        if (strcmp(BrowserHist_CurUrl(), gMmbbData->f4) == 0 && strcmp(gBrowserUrl, gMmbbData->f4) != 0) {
            gBrowserWork->tries = 2;
            gBrowserState->pageMode = CSV_KIND_NONE;
            return -2;
        }
        gBrowserState->plain = 0;
        gBrowserState->forceStyle8 = 1;
        return -1;
    case 3:
        return -1;
    case 7:
        return -1;
    case 8:
        return -1;
    case 9:
        return -1;
    case 4:
    case 5:
    case 6:
        gBrowserWork->pageData = r->data;
        return Browser_GoStart() ? 2 : -2;
    case 10:
        gBrowserWork->pageData = r->data;
        return 1;
    case 11:
        gBrowserWork->tries = 0;
        Browser_Go(r->data);
        Browser_Reload();
        return 2;
    case 12:
        return -1;
    case 13:
        return -1;
    default:
        return 0;
    }
}

/* Page_RequestImages: asks for the page's images (image objects, at most
 * 20); page 8 while some are coming, else page 10. */
void Page_RequestImages(void)
{
    u16 i;
    u8 n = 0;
    char *url;

    for (i = 0; i < 500; i++) {
        BrowserObj *o = gBrowserWork->obj[i];

        if (o == NULL || !o->active) {
            break;
        }
        if (o->kind == 0xD) {
            if (gBrowserState->imgCount == 20) {
                o->sub = 3;
            } else if (*(u8 *)o->s2 == 0) {
                o->sub = 2;
            } else {
                n++;
                Browser_SetUrl((BrowserWork *) &url, o->s2);
                BrowserReq_Image(&url);
                gBrowserWork->obj[i]->sub = 1;
                gBrowserState->imgCount++;
            }
        }
    }
    if (n > 0) {
        gBrowserState->page = PAGE_LOAD_IMAGES;
    } else {
        gBrowserState->page = PAGE_LOAD_DONE;
    }
}

/* Page_CheckImages: polls the page's image requests (sub 1): each one
 * answered gets its texture (sub 2) or fails (sub 3). Once all are in,
 * page 10 and 1 (-1 once when one failed on a lobby page, unk36), else
 * page 8 and 0. */
s8 Page_CheckImages(void)
{
    u16 i;
    u8 n;
    u8 failed;
    BrowserResult *img; /* an image: data is its texture handle */
    char *url;

    n = 0;
    for (i = 0; i < 500; i++) {
        BrowserObj *o = gBrowserWork->obj[i];

        if (o == NULL || !o->active) {
            break;
        }
        if (o->kind == 0xD) {
            n++;
            if (o->sub == 1) {
                Browser_SetUrl((BrowserWork *) &url, o->s2);
                img = BrowserReq_Poll(&url);
                if (img != NULL) {
                    if (img->data == NULL) {
                        gBrowserWork->obj[i]->sub = 3;
                    } else {
                        gBrowserWork->obj[i]->sub = 2;
                        gBrowserWork->obj[i]->tex = gBrowserWork->texCount;
                        gBrowserWork->texHandle[gBrowserWork->texCount] = *(u32 *)img->data;
                        gBrowserWork->texW[gBrowserWork->texCount] = img->w;
                        gBrowserWork->texH[gBrowserWork->texCount] = img->h;
                        gBrowserWork->texCount++;
                    }
                }
            }
        }
    }
    if (n == 0) {
        gBrowserState->page = PAGE_LOAD_DONE;
        return 1;
    }
    failed = 0;
    for (i = 0; i < 500; i++) {
        if (gBrowserWork->obj[i] == NULL || !gBrowserWork->obj[i]->active) {
            break;
        }
        if (gBrowserWork->obj[i]->kind == 0xD) {
            switch (gBrowserWork->obj[i]->sub) {
            case 3:
                failed++;
            case 2:
                if (--n == 0) {
                    if (gBrowserState->initMode != 0 && failed > 0 && gBrowserWork->texFailOnce != 0) {
                        gBrowserWork->texFailOnce = 0;
                        return -1;
                    }
                    gBrowserState->page = PAGE_LOAD_DONE;
                    return 1;
                }
                break;
            }
        }
    }
    gBrowserState->page = PAGE_LOAD_IMAGES;
    return 0;
}

/* Browser_Timers: the page's timers: <META REFRESH> (goes to its URL when
 * it runs out, if nothing else is going on: 1), the scroll's. */
s32 Browser_Timers(void)
{
    u8 *wait = &gBrowserState->styleWait;
    s16 *refresh;
    s16 *scroll;

    if (*wait != 0) {
        (*wait)--;
    }
    refresh = &gBrowserWork->refreshTimer;
    if (*refresh > 0) {
        if (gBrowserWork->dlgMenuState != BDLG_MENU_HIDDEN) {
            return 0;
        }
        if (gBrowserWork->dlgInputState != BDLG_INPUT_IDLE) {
            return 0;
        }
        if (gBrowserWork->dlgBoxKind != BDLG_KIND_NONE) {
            return 0;
        }
        if (--*refresh == 0) {
            Browser_SetUrl((BrowserWork *) &gBrowserWork->url, gBrowserState->url);
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                Browser_SetStep(PAGE_STEP_RELOAD);
                return 1;
            }
            Browser_Reload();
            return 1;
        }
    }
    scroll = &gBrowserWork->pressTimer;
    if (*scroll > 0) {
        (*scroll)--;
    } else {
        gBrowserSub->moveKind = 0;
    }
    return 0;
}

/* Page_CreateFrame: the frame's objects. */
void Page_CreateFrame(void)
{
    DlgBackImg_Create(1);
    DlgPage_Create(1);
    DlgVScroll_Create(1);
    DlgHScroll_Create(1);
    DlgStatus_Create(1);
    DlgMenu_Create(1);
    DlgInput_Create(1);
    DlgPointer_Create(1);
    DlgBox_Create(1);
}

extern void (*gPortalEndSteps[])(void); /* mode 2 (the end): PortalEnd_Step0 .. 4 */

/* Browser_RunEnd: mode 2 (the end), a step a frame. */
void Browser_RunEnd(void)
{
    gPortalEndSteps[gBrowserState->step]();
    Browser_SetRenderState(0x14, 0xFF000001);
}

/* Portal mode 2 (the end), step 0: with a next URL from the page (and
 * endGoOn set), the portal goes there with the cookies user-id (the form's
 * login id when it differs, else the MMBB user-id) and connection-id;
 * else the end dialog (0xC) and step 1. */
void PortalEnd_Step0(void)
{
    char *id;

    if (gBrowserWork->endGoOn != 0) {
        gPortalPage = PORTAL_PAGE_TOP;
        if ((u8)gMmbbData->nextUrl[0] != 0) {
            Browser_Go(gMmbbData->nextUrl);
            id = gBrowserState->formLoginId;
            if ((u8)id[0] != 0 && strcmp(id, gMmbbData->userId) != 0) {
                BrowserForm_Add(gIdFieldNames.name.userId, gBrowserState->formLoginId);
            } else {
                BrowserForm_Add(gIdFieldNames.name.userId, gMmbbData->userId);
            }
            BrowserForm_Add(gIdFieldNames.name.connectionId, gMmbbData->connectionId);
            Browser_SetUrl((BrowserWork *) &gBrowserWork->url, gMmbbData->nextUrl);
            BrowserReq_Post(&gBrowserWork->url);
            gBrowserWork->dlgBoxKind = BDLG_KIND_LOADING_SMALL;
            gBrowserState->step = 4;
            return;
        }
    }
    gBrowserWork->freedObjs = 0;
    gBrowserWork->dlgBackImgState = BDLG_BACK_CLOSE;
    gBrowserWork->dlgPageState = BDLG_PAGE_CLOSE;
    gBrowserWork->dlgStatusState = BDLG_STATUS_CLOSE;
    gBrowserWork->dlgHScrollState = BDLG_SCROLL_CLOSE;
    gBrowserWork->dlgVScrollState = BDLG_SCROLL_CLOSE;
    gBrowserWork->dlgMenuState = BDLG_MENU_CLOSE;
    gBrowserWork->dlgInputState = BDLG_INPUT_CLOSE;
    gBrowserWork->dlgBoxKind = BDLG_KIND_CLOSE;
    gBrowserWork->dlgPointerState = BDLG_POINTER_CLOSE;
    if (gBrowserState->initMode == 0) {
        Browser_DrawOverlay();
    }
    gBrowserState->step = 1;
}

/* Portal mode 2, step 1: the frame's objects once more, then step 2. */
void PortalEnd_Step1(void)
{
    PageObj_UpdateLayer(0);
    Callbacks_Add(DlgBackImg_DrawAll);
    PageObj_UpdateLayer(1);
    Callbacks_Add(DlgPage_DrawAll);
    PageObj_UpdateLayer(2);
    Callbacks_Add(DlgVScroll_DrawAll);
    Callbacks_Add(DlgHScroll_DrawAll);
    Callbacks_Add(DlgStatus_DrawAll);
    PageObj_UpdateLayer(3);
    Callbacks_Add(DlgMenu_DrawAll);
    PageObj_UpdateLayer(4);
    Callbacks_Add(DlgInput_DrawAll);
    PageObj_UpdateLayer(6);
    Callbacks_Add(DlgBox_DrawAll);
    PageObj_UpdateLayer(8);
    Callbacks_Add(DlgPointer_DrawAll);
    gBrowserState->step = 2;
}

/* Portal mode 2, step 2: the page's objects freed. */
void PortalEnd_Step2(void)
{
    u16 n;

    for (n = 0; n < 500; n++) {
        if (gBrowserWork->obj[n] == NULL) {
            break;
        }
        PageObj_Free(gBrowserWork->obj[n]);
    }
    gBrowserState->step = 3;
}

/* Portal mode 2, step 3: the end: the textures freed, the result. */
void PortalEnd_Step3(void)
{
    s8 menu;

    if (gBrowserTopMode == BROWSER_TOP_PORTAL) {
        PageTex_ReleaseAll();
    }
    PageObj_InitPool();
    Font_SetHalfWidth(1);
    if (gBrowserWork->endGoOn == 0 && gBrowserWork->endReason != BROWSER_END_NONE) {
        Browser_Exit(-1);
        return;
    }
    gBrowserState->prevPage = gBrowserState->page;
    menu = gBrowserWork->gameMenuNo;
    if (menu != 0) {
        Browser_Exit(menu);
        return;
    }
    if (gConnCheckState != CONN_LOST_NONE) {
        gConnCheckState = CONN_LOST_ENDED;
    }
    if (gBrowserWork->loginPosted != 0) {
        Browser_Exit(-100);
    } else {
        Browser_Exit(1);
    }
    if (gBrowserWork->endGoOn != 0) {
        gBrowserWork->result = -100;
    }
}

/* Portal mode 2, step 0: the error dialog, once the request ends. */
void PortalEnd_Step4(void)
{
    PageObj_UpdateLayer(6);
    Callbacks_Add(DlgBox_DrawAll);
    if (BrowserReq_Poll(&gBrowserWork->url) != NULL) {
        gBrowserWork->reqBusy = 0;
        gMmbbData->nextUrl[0] = 0;
        gBrowserWork->freedObjs = 0;
        gBrowserWork->dlgBackImgState = BDLG_BACK_CLOSE;
        gBrowserWork->dlgPageState = BDLG_PAGE_CLOSE;
        gBrowserWork->dlgStatusState = BDLG_STATUS_CLOSE;
        gBrowserWork->dlgHScrollState = BDLG_SCROLL_CLOSE;
        gBrowserWork->dlgVScrollState = BDLG_SCROLL_CLOSE;
        gBrowserWork->dlgMenuState = BDLG_MENU_CLOSE;
        gBrowserWork->dlgInputState = BDLG_INPUT_CLOSE;
        gBrowserWork->dlgBoxKind = BDLG_KIND_CLOSE;
        gBrowserWork->dlgPointerState = BDLG_POINTER_CLOSE;
        if (gBrowserState->initMode == 0) {
            Browser_DrawOverlay();
        }
        gBrowserState->step = 1;
    }
}

/* Dialog_Run: the sub-dialog's step (gBrowserSub->dlgStep). */
void Dialog_Run(void)
{
    jtbl_007EEB78[gBrowserSub->dlgStep]();
}

/* Dialog_Reset */
void Dialog_Reset(void)
{
    gBrowserSub->dlgStep = 1;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
    gBrowserSub->cursorX = 150;
    gBrowserSub->cursorY = 100;
    gBrowserSub->dragX = 0;
    gBrowserSub->dragY = 0;
}

/* Dialog_RunStep: the dialog's step (gBrowserSub->dlgMode). */
void Dialog_RunStep(void)
{
    jtbl_007EEB90[gBrowserSub->dlgMode]();
}

/* Dialog_PageStep: the dialog's step 0 (jtbl_007EEB90[0]), a page on
 * show: nothing while the lobby's timer (refreshTimer 1..3) runs; by the
 * page's style flags the menu (MOUSE and the circle or Esc: step 2), back
 * (flag 0x40: a button or the F5 key), forward (0x10), the page's link (8),
 * then the pointer, the scroll bars, the links and the scrolling. */
void Dialog_PageStep(void)
{
    u16 i;
    s32 flags;
    s16 s;
    BrowserObj *o;
    u8 page;

    s = gBrowserWork->refreshTimer;
    if (s > 0 && s < 4) {
        return;
    }
    page = gBrowserState->page;
    switch (page) {
    case 0:
        return;
    case 1:
        Page_Navigate();
        Page_Clamp();
        return;
    case 8:
        if (gBrowserState->initMode != 0) {
            return;
        }
        break;
    case 10:
        break;
    case 3:
    case 5:
    case 6:
        return;
    }
    if (gBrowserState->step >= 6) {
        Page_Navigate();
        Page_Clamp();
        return;
    }
    flags = gBrowserState->style;
    if (((flags & PAGE_STYLE_CANCEL) >> 5) != 0 && (flags & PAGE_STYLE_POINTER) && gBrowserSub->listCount == 0 &&
        ((gBrowserWork->padPressed & 0x8000) || gKbd->usbKey == USB_KEY_ESCAPE) && page == 10 && gBrowserState->plain == 0) {
        LobbySe_Play(0);
        gBrowserWork->dlgMenuState = BDLG_MENU_SHOWN;
        gBrowserSub->dlgMode = BDLG_MODE_MENU;
        gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
        gBrowserSub->dlgCursor = 3;
        gBrowserSub->listCount = 0;
        gBrowserSub->activeObj = 0;
        gBrowserWork->dlgPointerState = BDLG_POINTER_HIDDEN;
        return;
    }
    if (((flags & PAGE_STYLE_RELOAD) >> 6) != 0 && ((gBrowserWork->padPressed & 0x200) || gKbd->usbKey == USB_KEY_F5)) {
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
        if (BrowserReq_CanReload() == 0) {
            return;
        }
        if (memcmp(BrowserHist_CurUrl(), gLbsScheme, strlen(gLbsScheme)) == 0) {
            return;
        }
        page = gBrowserState->page;
        if (page == 1) {
            return;
        }
        if (page == 10) {
            Portal_Back();
            return;
        }
        if (page == 8) {
            Browser_SetStep(PAGE_STEP_BACK);
            return;
        }
    }
    if (((gBrowserState->style & PAGE_STYLE_FORWARD) >> 4) != 0 && (gBrowserWork->padPressed & 0x100)) {
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
        if (page == 1) {
            return;
        }
        if (page == 10) {
            Browser_GoForward();
            return;
        }
        if (page == 8) {
            if (BrowserHist_CanForward() == 0) {
                return;
            }
            Browser_SetStep(PAGE_STEP_HISTORY);
            gBrowserState->histDir = 2;
            return;
        }
    }
    flags = (gBrowserState->style & PAGE_STYLE_BACK) >> 3;
    if (flags != 0 && ((gBrowserWork->padPressed & 0x40) || gKbd->usbKey == USB_KEY_BACKSPACE)) {
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
        if (strlen(gBrowserState->msg) != 0) {
            if (gBrowserState->page == PAGE_LOAD_REQUESTED) {
                return;
            }
            LobbySe_Play(4);
            if (memcmp(gBrowserState->msg, D_00839110, strlen(D_00839110)) == 0) {
                if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                    Browser_SetStep(PAGE_STEP_CANCEL);
                    return;
                }
                gPortalExit = PORTAL_EXIT_CANCEL;
                Browser_Fail(0);
                return;
            }
            Browser_Go(gBrowserState->msg);
            gBrowserSub->activeObj = 0;
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                Browser_SetStep(PAGE_STEP_RELOAD);
                return;
            }
            Browser_Reload();
        } else {
            if (gBrowserState->page == PAGE_LOAD_REQUESTED) {
                return;
            }
            LobbySe_Play(4);
            if (gBrowserState->page == PAGE_LOAD_DONE) {
                Browser_GoBack();
                return;
            }
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                if (BrowserHist_CanBack() == 0) {
                    return;
                }
                Browser_SetStep(PAGE_STEP_HISTORY);
                gBrowserState->histDir = 1;
                return;
            }
        }
    } else if (flags == 0 && ((gBrowserWork->padPressed & 0x40) || gKbd->usbKey == USB_KEY_BACKSPACE) &&
               strlen(gBrowserState->msg) != 0) {
        for (i = 0; i < 500; i++) {
            if (gBrowserWork->obj[i] == NULL || !gBrowserWork->obj[i]->active) {
                break;
            }
            if (gBrowserWork->obj[i]->kind == 13 && gBrowserWork->obj[i]->active == 3 && ((gBrowserWork->obj[i]->field & 0x10) >> 4) == 1 &&
                strcmp(gBrowserState->msg, D_00839130) == 0) {
                gBrowserWork->focus = i;
                o = gBrowserWork->obj[i];
                gBrowserSub->cursorX = o->x + o->w / 2.0f - (f32)gBrowserState->x;
                o = gBrowserWork->obj[i];
                gBrowserSub->cursorY = o->y + o->h / 2.0f - (f32)gBrowserState->y;
                Page_PointAt();
                LobbySe_Play(4);
                return;
            }
        }
    }
    if (gBrowserState->page >= 8) {
        if ((gBrowserWork->padPressed & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            Page_Click();
        }
    }
    if (gBrowserWork->padPressed & 0x20) {
        Page_PointerScroll();
    }
    if (gBrowserWork->padHeld & 0x20) {
        Page_DragScroll();
    }
    if (!(gBrowserWork->padPressed & 0x20)) {
        Page_Navigate();
    }
    if (gBrowserSub->listCount == 0) {
        Page_ScrollInput();
    }
    Page_Clamp();
    if (gBrowserSub->activeObj == 0) {
        Page_PointAt();
    }
}

void Dialog_InputStep(void)
{
    gBrowserWork->dlgInputState = BDLG_INPUT_OPEN;
}

void Dialog_ResetMsgState(void)
{
    gBrowserWork->dlgMenuState = BDLG_MENU_HIDDEN;
    gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
}

/* Dialog_MenuStep: the dialog's step 2, a menu of three (gBrowserSub->dlgCursor,
 * 1..3): 1 back (Portal_Back), 2 leave (Browser_Fail(0)), 3 close; moved
 * with the directions, the left stick (every 8 frames, or at once after
 * a pause: stickReleased/stickFirst) or the USB keyboard's arrows; the circle or
 * Esc closes it. */
void Dialog_MenuStep(void)
{
    StickSteps dirs = gDialogMenuStickSteps;
    s8 sel;
    u8 i;
    s32 b;
    u8 k;

    if ((gBrowserWork->padPressed & 0x8000) || gKbd->usbKey == USB_KEY_ESCAPE) {
        LobbySe_Play(4);
        gBrowserWork->dlgMenuState = BDLG_MENU_HIDDEN;
        gBrowserSub->dlgMode = BDLG_MODE_PAGE;
        gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
    }
    if ((gBrowserWork->padPressed & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        LobbySe_Play(0);
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            return;
        }
        for (i = 0; i < 3; i++) {
            if (i == gBrowserSub->dlgCursor - 1) {
                switch (i) {
                case 0:
                    if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
                        return;
                    }
                    Dialog_ResetMsgState();
                    Portal_Back();
                    return;
                case 1:
                    gPortalExit = 0;
                    Browser_Fail(0);
                    return;
                case 2:
                    Dialog_ResetMsgState();
                    return;
                }
            }
        }
    }
    sel = gBrowserSub->dlgCursor;
    if (gBrowserWork->stickTilt != 0) {
        if (gBrowserWork->stickReleased == 1) {
            gBrowserWork->stickFirst = 1;
        } else {
            gBrowserWork->stickFirst = 0;
        }
        gBrowserWork->stickReleased = 0;
        if (gBrowserSub->stickRepeat > 1000) {
            gBrowserSub->stickRepeat = 0;
        }
        if (++gBrowserSub->stickRepeat % 8 == 0 || gBrowserWork->stickFirst != 0) {
            sel += dirs.v[gBrowserWork->stickDir];
            gBrowserSub->stickRepeat = 0;
            gBrowserSub->dlgCursor = sel;
        } else {
            return;
        }
    } else {
        b = gBrowserWork->padPressed;
        if ((b & 0x2000) || (b & 0x1000)) {
            if (b & 0x2000) {
                sel--;
            }
            if (b & 0x1000) {
                sel++;
            }
        } else {
            k = gKbd->usbKey;
            if (k == 0x52 || k == 0x51) {
                if (k == 0x52) {
                    sel--;
                }
                if (k == 0x51) {
                    sel++;
                }
            } else {
                gBrowserWork->stickReleased = 1;
                gBrowserSub->stickRepeat = 0;
                return;
            }
        }
    }
    if (sel <= 0) {
        sel = 3;
    }
    if (sel > 3) {
        sel = 1;
    }
    LobbySe_Play(1);
    gBrowserSub->dlgCursor = sel;
}

void Dialog_ResetBoxState(void)
{
    gBrowserState->fromTop = 0;
    gBrowserWork->dlgBoxKind = BDLG_KIND_NONE;
    gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
}

/* Dialog_ChoiceStep: the dialog's step 3, a choice by the box's kind
 * (dlgBoxKind): 4 one of three (with dlgQuestion 1), 5 yes / no (the square
 * or Esc: no), 6 one of two (with dlgQuestion 2); the choice's result goes to
 * formKind. The cursor moves as in Dialog_MenuStep, up to 3 or 2. The pad
 * is read into a second `s32 c` for the cursor (one for both swapped two
 * registers). */
void Dialog_ChoiceStep(void)
{
    StickSteps dirs = gDialogChoiceStickSteps;
    s8 max;
    s8 sel;
    s32 b;
    s32 c;
    u8 k;

    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_MESSAGE:
        max = 3;
        break;
    case BDLG_KIND_YES_NO:
        max = 2;
        break;
    case BDLG_KIND_CLEAR_LOGIN:
        max = 2;
        break;
    default:
        max = 2;
        break;
    }
    gBrowserWork->loginSave = 0;
    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_MESSAGE:
        if ((gBrowserWork->padPressed & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            if (gBrowserWork->dlgQuestion != 1) {
                return;
            }
            LobbySe_Play(0);
            switch (gBrowserSub->dlgCursor) {
            case 1:
                gBrowserWork->loginSave = 2;
                break;
            case 2:
                gBrowserWork->loginSave = 1;
                break;
            case 3:
                gBrowserWork->loginSave = 0;
                Dialog_ResetBoxState();
                return;
            }
            gBrowserWork->loginSave += (s8)(gBrowserState->pageKind * 10);
            gBrowserWork->errorKind = 0;
            gBrowserSub->dlgMode = BDLG_MODE_SAVE_LOGIN;
            gBrowserWork->keepLoginStep = 0;
            return;
        }
        goto move;
    case BDLG_KIND_YES_NO:
        b = gBrowserWork->padPressed;
        if ((b & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            LobbySe_Play(0);
            Dialog_ResetBoxState();
            switch (gBrowserSub->dlgCursor) {
            case 1:
                gBrowserWork->loginSave = 0x3D;
                break;
            case 2:
                if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                    gBrowserState->styleWait = 0;
                    Browser_SetStep(PAGE_STEP_HISTORY);
                    gBrowserState->histDir = 1;
                    return;
                }
                gBrowserWork->loginSave = 0x3C;
                Browser_GoBack();
                break;
            }
            return;
        }
        if ((b & 0x40) || gKbd->usbKey == USB_KEY_ESCAPE) {
            LobbySe_Play(4);
            Dialog_ResetBoxState();
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                gBrowserState->styleWait = 0;
                Browser_SetStep(PAGE_STEP_HISTORY);
                gBrowserState->histDir = 1;
                return;
            }
            gBrowserWork->loginSave = 0x3C;
            Browser_GoBack();
            return;
        }
        goto move;
    case BDLG_KIND_CLEAR_LOGIN:
        if ((gBrowserWork->padPressed & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            if (gBrowserWork->dlgQuestion != 2) {
                return;
            }
            LobbySe_Play(0);
            switch (gBrowserSub->dlgCursor) {
            case 1:
                gBrowserWork->loginSave = 0x52;
                gBrowserWork->errorKind = 0;
                gBrowserSub->dlgMode = BDLG_MODE_SAVE_LOGIN;
                gBrowserWork->keepLoginStep = 0;
                break;
            case 2:
                gBrowserWork->loginSave = 0x50;
                Dialog_ResetBoxState();
                break;
            }
            return;
        }
    default:
    move:
        sel = gBrowserSub->dlgCursor;
        if (gBrowserWork->stickTilt != 0) {
            if (gBrowserWork->stickReleased == 1) {
                gBrowserWork->stickFirst = 1;
            } else {
                gBrowserWork->stickFirst = 0;
            }
            gBrowserWork->stickReleased = 0;
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || gBrowserWork->stickFirst != 0) {
                sel += dirs.v[gBrowserWork->stickDir];
                gBrowserSub->stickRepeat = 0;
            } else {
                return;
            }
        } else {
            c = gBrowserWork->padPressed;
            if ((c & 0x2000) || (c & 0x1000)) {
                if (c & 0x2000) {
                    sel--;
                }
                if (c & 0x1000) {
                    sel++;
                }
            } else {
                k = gKbd->usbKey;
                if (k == 0x52 || k == 0x51) {
                    if (k == 0x52) {
                        sel--;
                    }
                    if (k == 0x51) {
                        sel++;
                    }
                } else {
                    gBrowserWork->stickReleased = 1;
                    gBrowserSub->stickRepeat = 0;
                    return;
                }
            }
        }
        if (sel <= 0) {
            sel = 1;
        } else if (sel > max) {
            sel = max;
        } else {
            LobbySe_Play(1);
        }
        gBrowserSub->dlgCursor = sel;
        break;
    }
}

void Dialog_SaveLogin(void)
{
    Browser_SaveLogin();
}

/* Page_PointTo: puts the pointer on object i (10 pixels in), scrolling
 * the page to it; unless anywhere, only an object inside the view; the
 * object becomes the hovered one (focus). 1, or 0 when it is outside. */
s32 Page_PointTo(u16 i, s8 anywhere)
{
    if (anywhere == 0) {
        if (!(gBrowserWork->obj[i]->y <= (f32)(0x19C - gBrowserState->marginBottom))) {
            gBrowserWork->focus = 0;
            return 0;
        }
        if (!(gBrowserWork->obj[i]->x <= (f32)(0x270 - gBrowserState->marginRight))) {
            gBrowserWork->focus = 0;
            return 0;
        }
    }
    gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[i]->y - (f32)gBrowserState->y);
    if (gBrowserSub->cursorY > 0x17C) {
        gBrowserSub->cursorY = 0x17C;
        gBrowserState->y += 10.0f + (gBrowserWork->obj[i]->y - (f32)(gBrowserState->y + 0x17C));
    }
    if (gBrowserSub->cursorY < gBrowserState->marginTop) {
        gBrowserSub->cursorY = gBrowserState->marginTop + 10;
        gBrowserState->y -= 10.0f + ((f32)gBrowserState->y - gBrowserWork->obj[i]->y);
        if (gBrowserWork->obj[i]->y - (f32)gBrowserState->y < (f32)gBrowserState->marginTop) {
            gBrowserState->y -= (f32)gBrowserState->marginTop - (gBrowserWork->obj[i]->y - (f32)gBrowserState->y);
        }
    }
    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[i]->x - (f32)gBrowserState->x);
    if (gBrowserSub->cursorX > 0x248) {
        gBrowserSub->cursorX = 0x248;
        gBrowserState->x += 10.0f + (gBrowserWork->obj[i]->x - (f32)(gBrowserState->x + 0x248));
    }
    if (gBrowserSub->cursorX < gBrowserState->marginTop) {
        gBrowserSub->cursorX = gBrowserState->marginTop + 10;
        gBrowserState->x -= 10.0f + ((f32)gBrowserState->x - gBrowserWork->obj[i]->x);
        if (gBrowserWork->obj[i]->x - (f32)gBrowserState->x < (f32)gBrowserState->marginLeft) {
            gBrowserState->x -= (f32)gBrowserState->marginLeft - (gBrowserWork->obj[i]->x - (f32)gBrowserState->x);
        }
    }
    gBrowserWork->focus = i;
    return 1;
}

/* Dialog_Register: the confirmation of a registration, yes / no: yes keeps the
 * registration record and leaves (0x5C3500 = 1). */
void Dialog_Register(void)
{
    Browser_ListCursor(2);
    if ((gBrowserWork->padPressed & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        switch (gBrowserSub->dlgCursor) {
        case 1:
            memcpy(gLobbyPageData, &gRegRecord, sizeof(gRegRecord));
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                Browser_SetStep(PAGE_STEP_GAME_MENU);
                return;
            }
            LobbySe_Play(0);
            gPortalExit = PORTAL_EXIT_GAME_MENU;
            Browser_Fail(0);
            return;
        case 2:
            Dialog_Close(0);
            break;
        }
    }
    if ((gBrowserWork->padPressed & 0x40) || gKbd->usbKey == USB_KEY_ESCAPE) {
        Dialog_Close(0);
    }
}

/* Dialog_Ok: the circle button or Enter: 1; the cross or Escape: 0. */
void Dialog_Ok(void)
{
    if ((gBrowserWork->padPressed & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        Dialog_Close(1);
    }
    if ((gBrowserWork->padPressed & 0x40) || gKbd->usbKey == USB_KEY_ESCAPE) {
        Dialog_Close(0);
    }
}

/* Dialog_Exit: dialog 0xB, leaving the portal: on the course top page a
 * yes / no list, else no choice; the circle button or Enter leaves (to the lobby's server
 * list when the page has lobby-server entries, kind 0xD sub 1), and
 * hangs when gPortalPage is 1 on page 8. */
void Dialog_Exit(void)
{
    u16 i;
    u16 n;

    gBrowserWork->dlgBoxKind = BDLG_KIND_EXIT;
    if (gBrowserWork->endReason > 0 && gBrowserWork->netError == 0) {
        if (strcmp(gBrowserState->url, D_00839140) == 0) {
            Browser_ListCursor(2);
            switch (gBrowserSub->dlgCursor) {
            case 1:
            default:
                gBrowserWork->endGoOn = 1;
                break;
            case 2:
                gBrowserWork->endGoOn = 0;
                break;
            }
        } else {
            gBrowserWork->endGoOn = 1;
        }
    }
    if ((gBrowserWork->padPressed & 0x20) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        LobbySe_Play(0);
        switch (gPortalPage) {
        case PORTAL_PAGE_TOP:
        case PORTAL_PAGE_NEXT:
            gBrowserWork->endReason = BROWSER_END_LEAVE;
            if (gBrowserTopMode == BROWSER_TOP_PORTAL) {
                n = 0;
                for (i = 0; i < 500; i++) {
                    if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
                        break;
                    }
                    if (gBrowserWork->obj[i]->kind == 0xD && gBrowserWork->obj[i]->sub == 1) {
                        n++;
                    }
                }
                if (n) {
                    gBrowserState->page = PAGE_LOAD_IMAGES;
                    Browser_SetStep(PAGE_STEP_GAME_MENU);
                    break;
                }
            }
            gPortalExit = 0;
            Browser_Fail(0);
            break;
        case PORTAL_PAGE_BACK:
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                for (;;) {
                }
            }
            Browser_Fail(0);
            break;
        }
    }
}

void func_0073ACE0(void)
{
}

/* unk_734710's .data (0x7EEB60-0x7EEBE0) */
void Dialog_ChoiceStep(void);
void Dialog_Exit(void);
void Dialog_MenuStep(void);
void Dialog_Ok(void);
void Dialog_PageStep(void);
void Dialog_Register(void);
void Dialog_Reset(void);
void Dialog_RunStep(void);
void PortalEnd_Step0(void);
void PortalEnd_Step1(void);
void PortalEnd_Step2(void);
void PortalEnd_Step3(void);
void PortalEnd_Step4(void);
void Dialog_InputStep(void);
void Dialog_SaveLogin(void);
void func_0073ACE0(void);
void func_00740D10(void);
void (*gPortalEndSteps[6])(void) = {
    PortalEnd_Step0, PortalEnd_Step1, PortalEnd_Step2, PortalEnd_Step3,
    PortalEnd_Step4, NULL,
};
void (*jtbl_007EEB78[3])(void) = {
    Dialog_Reset, Dialog_RunStep, func_00740D10,
};
void (*jtbl_007EEB90[12])(void) = {
    Dialog_PageStep, Dialog_InputStep, Dialog_MenuStep, Dialog_ChoiceStep,
    Dialog_SaveLogin, Dialog_Register, Dialog_Ok, Dialog_Exit,
    func_0073ACE0, NULL, NULL, NULL,
};
StickSteps gDialogMenuStickSteps = {
#include "lobby/007EEBC0.inc"
};
StickSteps gDialogChoiceStickSteps = {
#include "lobby/007EEBD0.inc"
};

/* unk_734710's .rodata (0x839110-0x839170) */
#pragma rodata on
char D_00839110[0x20] = {
#include "lobby/00839110.inc"
};
char D_00839130[0x10] = {
#include "lobby/00839130.inc"
};
char D_00839140[0x30] = {
#include "lobby/00839140.inc"
};
#pragma rodata off
