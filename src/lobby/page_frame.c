/* The browser's page frame and the portal's end.
 *
 * Browser_UpdateFrame picks the frame's layers to update and draw each
 * frame. Draw_Cmd turns each of the HTML layout's draw commands into a page
 * object (BrowserObj in gBrowserWork->obj, its strings in the command
 * buffer, BrowserWork.cmdBuf). Page_Load takes a page's answer,
 * Page_RequestImages and Page_CheckImages its images, and Browser_Timers runs
 * the page's refresh and press timers; the portal's page steps (enum
 * PageStep) call them.
 *
 * Then the portal's end (BMODE_END): its steps (enum PortalEndStep) close
 * the frame, free the page and leave the browser with its result. */
#include "common.h"
#include "browser.h"
#include "lobby_menu.h"
#include "pad.h"
#include "render_state.h"
#include "html.h"
#include "lobby_globals.h"
#include "kbd.h"
#include "font.h"
#include "frame.h"

/* Compiled at optimization level 4: Draw_Cmd doesn't match without it. */
#pragma optimization_level 4

void *memcpy(void *dst, const void *src, u32 n);
s32 BrowserForm_Add(const char *name, char *value);

void *memset(void *dst, s32 c, u32 n);
u32 strlen(const char *s);
extern char gBrowserUrl[]; /* the start URL */


extern u8 gPortalPage; /* how the browser was entered (PORTAL_PAGE_*) */
s32 strcmp(const char *a, const char *b); /* main: strcmp */

extern u8 gConnCheckState;       /* the lobby connection's state */

/* Sets the background colour (the page's; black under a dialog or a fade,
 * and in Browser_Init's other mode until the page is in), then adds the
 * frame's layers to update and draw this frame: only the dialog box while
 * that mode's first page loads or the start page shows, the box and the
 * pointer once an end reason is set, else every layer, the pointer last
 * (none while the keyboard is open). */
void Browser_UpdateFrame(void)
{
    BrowserState *state = gBrowserState;
    s32 plain = state->initMode;

    if (plain != 0 && state->page != PAGE_LOAD_DONE && state->fade >= 0) {
        Browser_SetRenderState(RS_BG_COLOR, BROWSER_BLACK);
        PageObj_UpdateLayer(BLAYER_BOX);
        Callbacks_Add(DlgBox_DrawAll);
        return;
    }
    if (plain == 0) {
        Browser_SetRenderState(RS_BG_COLOR, state->bgColor);
    } else if (gBrowserWork->dlgBoxKind == BDLG_KIND_NONE && state->fade == 0) {
        Browser_SetRenderState(RS_BG_COLOR, state->bgColor);
    } else {
        Browser_SetRenderState(RS_BG_COLOR, BROWSER_BLACK);
    }
    if (gBrowserWork->endReason > 0) {
        PageObj_UpdateLayer(BLAYER_BOX);
        Callbacks_Add(DlgBox_DrawAll);
        PageObj_UpdateLayer(BLAYER_POINTER);
        Callbacks_Add(DlgPointer_DrawAll);
        return;
    }
    if (gBrowserWork->firstPage != 0) {
        PageObj_UpdateLayer(BLAYER_BOX);
        Callbacks_Add(DlgBox_DrawAll);
        return;
    }
    PageObj_UpdateLayer(BLAYER_BACK_IMAGE);
    Callbacks_Add(DlgBackImg_DrawAll);
    PageObj_UpdateLayer(BLAYER_PAGE);
    Callbacks_Add(DlgPage_DrawAll);
    PageObj_UpdateLayer(BLAYER_BARS);
    Callbacks_Add(DlgVScroll_DrawAll);
    Callbacks_Add(DlgHScroll_DrawAll);
    Callbacks_Add(DlgStatus_DrawAll);
    PageObj_UpdateLayer(BLAYER_MENU);
    Callbacks_Add(DlgMenu_DrawAll);
    PageObj_UpdateLayer(BLAYER_KEYBOARD);
    Callbacks_Add(DlgInput_DrawAll);
    if (gBrowserWork->dlgInputState == BDLG_INPUT_OPEN) {
        return;
    }
    if (gBrowserState->page != PAGE_LOAD_DONE) {
        PageObj_UpdateLayer(BLAYER_POINTER);
        Callbacks_Add(DlgPointer_DrawAll);
        PageObj_UpdateLayer(BLAYER_BOX);
        Callbacks_Add(DlgBox_DrawAll);
        return;
    }
    PageObj_UpdateLayer(BLAYER_BOX);
    Callbacks_Add(DlgBox_DrawAll);
    PageObj_UpdateLayer(BLAYER_POINTER);
    Callbacks_Add(DlgPointer_DrawAll);
}

/* Does nothing. */
void Browser_InitNothing(void)
{
}

/* Browser_Init's hook: Browser_InitNothing. */
void Browser_InitHook(void)
{
    Browser_InitNothing();
}

/* Draw_Cmd's form values (kinds 6 and 7): src copied to the command buffer as
 * the object's s2, in a maxlen + 1 byte field (the rest skipped); the whole
 * command is dropped when the buffer can't take it. */
#define DRAW_VALUE(src)                                                        \
    if (maxlen + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= BROWSER_CMD_BUF_SIZE) { \
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

/* Makes a page object of kind `kind` (enum PageObjKind) from one of the
 * HTML layout's draw commands: the first free slot of gBrowserWork->obj
 * (none free: the command is dropped), from (x, y) to (x2, y2) inside the
 * page's margins, its strings s1 .. s3 copied to the command buffer. By kind:
 * - a text or password box takes its value (its second string) from the
 *   kept login (field 1 the login id, also copied to the MMBB data's
 *   user-id; 2 the password) or from the registration record (field 4: the
 *   field the object's two-digit name gives), in a maxlen + 1 byte field;
 * - a drop-down list (sub PAGE_SELECT_*): its start takes the next list
 *   number; each option its list's number and its index (tex); the end
 *   closes the list's object (PAGE_SELECT_CLOSED, in place of a new
 *   object), gives it the chosen option's index and name (the first
 *   option's when none is chosen) and closes its options;
 * - an image's text (its third string) is cut to the image's width.
 * A string that doesn't fit in the buffer leaves the object's strings
 * empty. Then the page's objects are chosen again (PageObj_Activate), and
 * the focus is dropped when its object lies below the view. */
void Draw_Cmd(u8 kind, u8 sub, u16 x, u16 y, u16 x2, u16 y2, u32 color, u8 size, u8 field,
              u8 maxlen, u8 *s1, u8 *s2, u8 *s3)
{
    u8 buf[256];
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
        if (i == PAGE_OBJ_COUNT) {
            return;
        }
        if (gBrowserWork->obj[i]->active == 0) {
            break;
        }
        i++;
    }
    switch (kind) {
    case PAGE_OBJ_TEXT_INPUT:
    case PAGE_OBJ_PASSWORD:
        if (field > 0 && field < 4 && strlen((char *)s2) > 16) {
            return;
        }
        break;
    case PAGE_OBJ_IMAGE:
        if (sub == PAGE_IMG_IMAGE || sub == PAGE_IMG_MAP || sub == PAGE_IMG_INPUT) {
            len3 = strlen((char *)s3);
            memset(buf, 0, 255);
            if (y2 - y <= 16 || len3 <= 0) {
                buf[0] = 0;
                len3 = 0;
            } else if ((len3 + 1) * 16 / 2 >= x2 - x) {
                n = (x2 - x) / 8 - 1;
                if (n >= 255) {
                    n = 255;
                }
                memcpy(buf, s3, n);
                buf[n] = 0;
                len3 = strlen((char *)buf);
            } else {
                memcpy(buf, s3, len3 + 1);
            }
        }
        break;
    case PAGE_OBJ_SELECT:
        switch (sub) {
        case PAGE_SELECT_BEGIN:
            for (j = 0; j < PAGE_OBJ_COUNT; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == PAGE_OBJ_SELECT) {
                    grp = gBrowserWork->obj[j]->group;
                }
            }
            if (!grp) {
                gBrowserWork->obj[i]->group = 1;
            } else {
                gBrowserWork->obj[i]->group = grp + 1;
            }
            gBrowserWork->obj[i]->tex = 0;
            gBrowserWork->obj[i]->active = PAGE_SELECT_CLOSED;
            break;
        case PAGE_SELECT_OPTION:
            for (j = 0; j < PAGE_OBJ_COUNT; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == PAGE_OBJ_SELECT) {
                    grp = gBrowserWork->obj[j]->group;
                }
            }
            for (j = 0; j < PAGE_OBJ_COUNT; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == PAGE_OBJ_SELECT && gBrowserWork->obj[j]->group == grp && (gBrowserWork->obj[j]->active & 0xFF) == PAGE_SELECT_OPTION_OPEN) {
                    cnt++;
                }
            }
            sub++;
            gBrowserWork->obj[i]->group = grp;
            gBrowserWork->obj[i]->tex = cnt;
            break;
        case PAGE_SELECT_END:
            for (i = 0; i < PAGE_OBJ_COUNT; i++) {
                if (gBrowserWork->obj[i]->kind == PAGE_OBJ_SELECT && gBrowserWork->obj[i]->active == PAGE_SELECT_BEGIN) {
                    sub = PAGE_SELECT_CLOSED;
                    ok = 0;
                    break;
                }
            }
            if (i == PAGE_OBJ_COUNT) {
                return;
            }
            for (j = 0; j < PAGE_OBJ_COUNT; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == PAGE_OBJ_SELECT && (gBrowserWork->obj[j]->active & 0xFF) == PAGE_SELECT_OPTION_OPEN && gBrowserWork->obj[j]->sub != 0) {
                    maxlen = gBrowserWork->obj[j]->tex;
                    break;
                }
            }
            if (maxlen > 0) {
                gBrowserWork->obj[i]->s1 = gBrowserWork->obj[j]->s1;
            } else {
                for (j = 0; j < PAGE_OBJ_COUNT; j++) {
                    if (gBrowserWork->obj[j]->kind == PAGE_OBJ_SELECT && gBrowserWork->obj[j]->active == PAGE_SELECT_OPTION_OPEN && gBrowserWork->obj[j]->tex == 1) {
                        gBrowserWork->obj[i]->s1 = gBrowserWork->obj[j]->s1;
                        maxlen = 1;
                        break;
                    }
                }
            }
            for (j = 0; j < PAGE_OBJ_COUNT; j++) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == PAGE_OBJ_SELECT && (gBrowserWork->obj[j]->active & 0xFF) == PAGE_SELECT_OPTION_OPEN) {
                    gBrowserWork->obj[j]->active = PAGE_SELECT_OPTION_DONE;
                }
            }
            break;
        }
        break;
    }
    if (ok) {
        if ((gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) == BROWSER_CMD_BUF_SIZE) {
            gBrowserWork->obj[i]->s1 = (char *)gBrowserWork->cmdPtr - 1;
            gBrowserWork->obj[i]->s2 = (char *)gBrowserWork->cmdPtr - 1;
            gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
            return;
        }
        len = strlen((char *)s1);
        if (len + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= BROWSER_CMD_BUF_SIZE) {
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
        case PAGE_OBJ_TEXT_INPUT:
        case PAGE_OBJ_PASSWORD:
            switch (field) {
            case 0:
                DRAW_VALUE(s2)
                break;
            case 1:
                if (strlen((char *)gBrowserState->loginId) != 0) {
                    if (maxlen + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= BROWSER_CMD_BUF_SIZE) {
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
                    num = 14;
                }
                memset(gMmbbData->userId, 0, sizeof(gMmbbData->userId));
                memcpy(gMmbbData->userId, s2, num);
                break;
            case 2:
                len = strlen((char *)gBrowserState->password);
                if (len > 0) {
                    if (maxlen + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= BROWSER_CMD_BUF_SIZE) {
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
            if (len + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= BROWSER_CMD_BUF_SIZE) {
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
            if (kind == PAGE_OBJ_IMAGE && (sub == PAGE_IMG_IMAGE || sub == PAGE_IMG_MAP || sub == PAGE_IMG_INPUT)) {
                len = len3;
            }
            if (len + (gBrowserWork->cmdPtr - gBrowserWork->cmdBuf) >= BROWSER_CMD_BUF_SIZE) {
                gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
                return;
            }
            if (len == 0) {
                gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr - 1;
            } else {
                gBrowserWork->obj[i]->s3 = (char *)gBrowserWork->cmdPtr;
                if (kind == PAGE_OBJ_IMAGE && (sub == PAGE_IMG_IMAGE || sub == PAGE_IMG_MAP || sub == PAGE_IMG_INPUT)) {
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
    for (j = 0; j < PAGE_OBJ_COUNT; j++) {
        if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->active == 0 || PageObj_Activate(j, 0)) {
            break;
        }
    }
    if (gBrowserWork->focus > 0 && gBrowserWork->obj[gBrowserWork->focus]->y > (f32)(412 - gBrowserState->marginBottom)) {
        gBrowserWork->focus = 0;
        gBrowserState->y = 0;
    }
}

/* Takes the page's answer, once it came. Returns 0 while none came, 1 for
 * the page (its HTML kept at pageData), 2 when the browser went elsewhere
 * (the start URL again, or BRES_REDIRECTED's URL), -1 for an error, -2 for
 * an error that ends the portal (an empty page, or too many retries). Its
 * callers see an s8. */
s32 Page_Load(void)
{
    BrowserResult *answer = BrowserReq_Poll(&gBrowserWork->url);

    if (answer == NULL) {
        return 0;
    }
    gBrowserWork->reqBusy = 0;
    gBrowserState->forceStyle8 = 0;
    switch (answer->result) {
    case BRES_SERVER_ERROR:
        gBrowserState->forceStyle8 = 1;
    case BRES_OK:
        gBrowserWork->pageData = answer->data;
        memset(gBrowserState->msg, 0, sizeof(gBrowserState->msg));
        if (strlen(gBrowserWork->pageData) == 0) {
            gBrowserWork->tries = 2;
            gBrowserState->pageMode = CSV_KIND_NONE;
            return -2;
        }
        if (gBrowserWork->atMmbbUrl != 0) {
            switch (gPortalPage) {
            case PORTAL_PAGE_BACK:
            case PORTAL_PAGE_NEXT:
                gBrowserState->bgColor = BROWSER_BLACK;
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
    case BRES_NO_NETWORK:
        if (gBrowserWork->firstPage != 0) {
            gBrowserWork->tries = 2;
            return -2;
        }
        return -1;
    case BRES_FAILED:
        gBrowserWork->pageData = answer->data;
        if (Browser_GoStart() != 0) {
            return 2;
        }
        if (strcmp(BrowserHist_CurUrl(), gMmbbData->sessionUrl) == 0 && strcmp(gBrowserUrl, gMmbbData->sessionUrl) != 0) {
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
    case BRES_IMAGE_FAILED:
        return -1;
    case BRES_CANCELLED:
        return -1;
    case BRES_CONNECT:
    case BRES_SEND:
    case BRES_TIMEOUT:
        gBrowserWork->pageData = answer->data;
        return Browser_GoStart() ? 2 : -2;
    case BRES_NOT_CACHED:
        gBrowserWork->pageData = answer->data;
        return 1;
    case BRES_REDIRECTED:
        gBrowserWork->tries = 0;
        Browser_Go(answer->data);
        Browser_Reload();
        return 2;
    case BRES_NEED_USER:
        return -1;
    case BRES_NEED_PROXY_USER:
        return -1;
    default:
        return 0;
    }
}

/* Asks for the page's images, PAGE_IMG_MAX at most (the others fail; an
 * image without a URL counts as loaded): PAGE_LOAD_IMAGES while some are
 * coming, else PAGE_LOAD_DONE. */
void Page_RequestImages(void)
{
    u16 i;
    u8 n = 0;
    char *url;

    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        BrowserObj *pageObj = gBrowserWork->obj[i];

        if (pageObj == NULL || !pageObj->active) {
            break;
        }
        if (pageObj->kind == PAGE_OBJ_IMAGE) {
            if (gBrowserState->imgCount == PAGE_IMG_MAX) {
                pageObj->sub = PAGE_IMG_FAILED;
            } else if (*(u8 *)pageObj->s2 == 0) {
                pageObj->sub = PAGE_IMG_LOADED;
            } else {
                n++;
                Browser_SetUrl((BrowserWork *) &url, pageObj->s2);
                BrowserReq_Image(&url);
                gBrowserWork->obj[i]->sub = PAGE_IMG_REQUESTED;
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

/* Polls the page's image requests: each image answered gets its texture
 * slot (PAGE_IMG_LOADED) or fails (PAGE_IMG_FAILED). Once all are in,
 * PAGE_LOAD_DONE and 1, or -1 once (texFailOnce) when one failed in
 * Browser_Init's other mode (initMode); until then PAGE_LOAD_IMAGES and 0. */
s8 Page_CheckImages(void)
{
    u16 i;
    u8 n;
    u8 failed;
    BrowserResult *img; /* an image: data is its texture handle */
    char *url;

    n = 0;
    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        BrowserObj *pageObj = gBrowserWork->obj[i];

        if (pageObj == NULL || !pageObj->active) {
            break;
        }
        if (pageObj->kind == PAGE_OBJ_IMAGE) {
            n++;
            if (pageObj->sub == PAGE_IMG_REQUESTED) {
                Browser_SetUrl((BrowserWork *) &url, pageObj->s2);
                img = BrowserReq_Poll(&url);
                if (img != NULL) {
                    if (img->data == NULL) {
                        gBrowserWork->obj[i]->sub = PAGE_IMG_FAILED;
                    } else {
                        gBrowserWork->obj[i]->sub = PAGE_IMG_LOADED;
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
    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        if (gBrowserWork->obj[i] == NULL || !gBrowserWork->obj[i]->active) {
            break;
        }
        if (gBrowserWork->obj[i]->kind == PAGE_OBJ_IMAGE) {
            switch (gBrowserWork->obj[i]->sub) {
            case PAGE_IMG_FAILED:
                failed++;
            case PAGE_IMG_LOADED:
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

/* Runs the page's timers: the style's wait; the META REFRESH timer, held
 * while a menu, the keyboard or a box is up, which loads its URL when it
 * runs out (returns 1); the press timer, which clears the last move's kind
 * when it ends. Otherwise returns 0. */
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

/* Makes the frame's objects, each layer's (their create functions' mode 1). */
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

extern void (*gPortalEndSteps[])(void); /* BMODE_END's steps, by enum PortalEndStep */

/* BMODE_END: runs the end's step (enum PortalEndStep), on a black background. */
void Browser_RunEnd(void)
{
    gPortalEndSteps[gBrowserState->step]();
    Browser_SetRenderState(RS_BG_COLOR, BROWSER_BLACK);
}

/* PORTAL_END_START: when the end goes on (endGoOn) and the page gave a next
 * URL, posts the ids to it as form fields (the user-id: the form's login id
 * when it differs from the MMBB data's, else the MMBB data's; and the
 * connection-id), shows the small loading mark and waits for the answer
 * (PORTAL_END_WAIT_POST). Otherwise closes the frame's objects, each to
 * its close state, and goes on to PORTAL_END_DRAW_LAST. */
void PortalEnd_Start(void)
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
            gBrowserState->step = PORTAL_END_WAIT_POST;
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
    gBrowserState->step = PORTAL_END_DRAW_LAST;
}

/* PORTAL_END_DRAW_LAST: every layer once more, so that the objects run
 * their close states; then PORTAL_END_FREE. */
void PortalEnd_DrawLast(void)
{
    PageObj_UpdateLayer(BLAYER_BACK_IMAGE);
    Callbacks_Add(DlgBackImg_DrawAll);
    PageObj_UpdateLayer(BLAYER_PAGE);
    Callbacks_Add(DlgPage_DrawAll);
    PageObj_UpdateLayer(BLAYER_BARS);
    Callbacks_Add(DlgVScroll_DrawAll);
    Callbacks_Add(DlgHScroll_DrawAll);
    Callbacks_Add(DlgStatus_DrawAll);
    PageObj_UpdateLayer(BLAYER_MENU);
    Callbacks_Add(DlgMenu_DrawAll);
    PageObj_UpdateLayer(BLAYER_KEYBOARD);
    Callbacks_Add(DlgInput_DrawAll);
    PageObj_UpdateLayer(BLAYER_BOX);
    Callbacks_Add(DlgBox_DrawAll);
    PageObj_UpdateLayer(BLAYER_POINTER);
    Callbacks_Add(DlgPointer_DrawAll);
    gBrowserState->step = PORTAL_END_FREE;
}

/* PORTAL_END_FREE: frees the page's objects; then PORTAL_END_FINISH. */
void PortalEnd_FreeObjs(void)
{
    u16 n;

    for (n = 0; n < PAGE_OBJ_COUNT; n++) {
        if (gBrowserWork->obj[n] == NULL) {
            break;
        }
        PageObj_Free(gBrowserWork->obj[n]);
    }
    gBrowserState->step = PORTAL_END_FINISH;
}

/* PORTAL_END_FINISH: frees the page's textures and the object pool, then
 * leaves the browser with its result: BROWSER_FAILED after an end reason
 * (unless the end went on), the game menu's number when one was chosen,
 * else BROWSER_LOGIN when the page posted the login, or BROWSER_DONE. A
 * lost lobby link is marked CONN_LOST_ENDED. */
void PortalEnd_Finish(void)
{
    s8 menu;

    if (gBrowserTopMode == BROWSER_TOP_PORTAL) {
        PageTex_ReleaseAll();
    }
    PageObj_InitPool();
    Font_SetHalfWidth(1);
    if (gBrowserWork->endGoOn == 0 && gBrowserWork->endReason != BROWSER_END_NONE) {
        Browser_Exit(BROWSER_FAILED);
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
        Browser_Exit(BROWSER_LOGIN);
    } else {
        Browser_Exit(BROWSER_DONE);
    }
    if (gBrowserWork->endGoOn != 0) {
        gBrowserWork->result = BROWSER_LOGIN;
    }
}

/* PORTAL_END_WAIT_POST: the dialog box (the loading mark) until the post
 * is answered; then clears the next URL and closes the frame's objects as
 * PortalEnd_Start does, and goes on to PORTAL_END_DRAW_LAST. */
void PortalEnd_WaitPost(void)
{
    PageObj_UpdateLayer(BLAYER_BOX);
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
        gBrowserState->step = PORTAL_END_DRAW_LAST;
    }
}

/* The end's step table (.data). */
void (*gPortalEndSteps[6])(void) = {
    PortalEnd_Start, PortalEnd_DrawLast, PortalEnd_FreeObjs, PortalEnd_Finish,
    PortalEnd_WaitPost, NULL,
};
