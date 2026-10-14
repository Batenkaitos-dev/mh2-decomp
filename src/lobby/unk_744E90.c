/* The HTML tag handlers, first part: <HTML>, <HEAD>, <TITLE>, <B>, <U>,
 * <S>, <CENTER>, <Hn>, <DL>/<DT>/<DD>, <P>, <IMG> and its attributes
 * (tables at gHtmlModeTags_0 ...; include/html.h). Each handler gets the parse
 * position and the pending text and returns 0 (or < 0 to stop the page). */
#include "common.h"
#include "html.h"

char *strcpy(char *dst, const char *src);
u8 *strstr(const u8 *s, const char *w); /* strstr */

void Html_DecodeEntities(char *s);
void Html_SetCharWidths(u8 bold);
void Html_ReadAttrValue(u8 **cur, char *buf, s32 size);
s32 Html_ParseNumber(char *s);
void Html_ClearImgAttrs(void);
void Layout_Image(void);
void Layout_Flush(u8 *text);
void Layout_FlushBreak(u8 *text);
void Layout_FlushBlock(u8 *text);
void Html_ClearFontAttrs(void);
void Html_PushFont(void);
void Html_PopFont(void);
void Html_SetTitle(u8 *text);
void Layout_CellAlign(HtmlCell *cell);
void Html_SetBase(char *url);

extern u8 gHtmlHeadingSizes[];
extern char D_00839CA0[]; /* ">" */
extern char D_00839EE0[]; /* "#000000" */
extern char D_00839EE8[]; /* "#0000FF" */

/* <HTML>, and <HTML ...>'s ">": start a page. */
s32 Html_TagHtml(u8 **cur, u8 *text) {
    gHtml->textLen = 0;
    *text = 0;
    gHtml->pen.left = 0;
    gHtml->pen.h = 0;
    gHtml->pen.x = 0;
    gHtml->pen.runX = 0;
    gHtml->pen.y = 0;
    gHtml->pen.runY = 0;
    gHtml->pen.lineUsed = 0;
    gHtml->lineLen = 0;
    strcpy(gHtml->textColor, D_00839EE0);
    strcpy(gHtml->linkColor, D_00839EE8);
    return 0;
}

/* </HTML>, </BODY> */
s32 Html_TagEndHtml(u8 **cur, u8 *text) {
    Layout_FlushBreak(text);
    return 0;
}

/* <BR> */
s32 Html_TagBr(u8 **cur, u8 *text) {
    Layout_FlushBreak(text);
    return 0;
}

/* <HEAD>, </HEAD>, <TITLE>, <BASE ...>, <BODY ...> and the ends of
 * <SCRIPT>/<STYLE>: drop the pending text. */
s32 Html_TagDropText(u8 **cur, u8 *text) {
    gHtml->textLen = 0;
    *text = 0;
    return 0;
}

/* </TITLE> */
s32 Html_TagEndTitle(u8 **cur, u8 *text) {
    Html_SetTitle(text);
    return 0;
}

/* <BASE HREF=...> */
s32 Html_AttrBaseHref(u8 **cur, u8 *text) {
    char buf[0x100];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure == 0 && gHtml->inFrame == 0) {
        Html_SetBase(buf);
    }
    return 0;
}

/* <B>, <STRONG> */
s32 Html_TagB(u8 **cur, u8 *text) {
    Layout_Flush(text);
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].bold = 1;
    } else {
        gHtml->bold = 1;
    }
    Html_SetCharWidths(1);
    return 0;
}

/* </B>, </STRONG> */
s32 Html_TagEndB(u8 **cur, u8 *text) {
    u16 i;

    Layout_Flush(text);
    if (gHtml->tableDepth != 0) {
        i = gHtml->curCell;
        if (gHtml->cells[i].kind == 3) {
            return 0;
        }
        gHtml->cells[i].bold = 0;
    } else {
        gHtml->bold = 0;
    }
    Html_SetCharWidths(0);
    return 0;
}

/* <U> */
s32 Html_TagU(u8 **cur, u8 *text) {
    Layout_Flush(text);
    gHtml->underline = 1;
    return 0;
}

/* </U> */
s32 Html_TagEndU(u8 **cur, u8 *text) {
    Layout_Flush(text);
    gHtml->underline = 0;
    return 0;
}

/* <STRIKE>, <S> */
s32 Html_TagS(u8 **cur, u8 *text) {
    Layout_Flush(text);
    gHtml->strike = 1;
    return 0;
}

/* </STRIKE>, </S> */
s32 Html_TagEndS(u8 **cur, u8 *text) {
    Layout_Flush(text);
    gHtml->strike = 0;
    return 0;
}

/* <CENTER> */
s32 Html_TagCenter(u8 **cur, u8 *text) {
    HtmlCell *cell;
    u8 v;
    u8 n;

    Layout_FlushBlock(text);
    if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        v = cell->center;
        n = v & 0xF;
        if (n != 0xF) {
            n++;
            cell->center = (v & 0xF0) | n;
        }
        Layout_CellAlign(cell);
    } else {
        HtmlState *s = gHtml;
        v = s->center;
        n = v & 0xF;
        if (n != 0xF) {
            n++;
            s->center = (v & 0xF0) | n;
        }
    }
    return 0;
}

/* </CENTER> */
s32 Html_TagEndCenter(u8 **cur, u8 *text) {
    HtmlCell *cell;
    u8 v;
    u8 n;

    Layout_FlushBlock(text);
    if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        v = cell->center;
        n = v & 0xF;
        if (n != 0) {
            n--;
            cell->center = (v & 0xF0) | n;
        }
    } else {
        HtmlState *s = gHtml;
        v = s->center;
        n = v & 0xF;
        if (n != 0) {
            n--;
            s->center = (v & 0xF0) | n;
        }
    }
    return 0;
}

/* <H1> .. <H6>: the font size from the heading's digit. */
s32 Html_TagH(u8 **cur, u8 *text) {
    Html_ClearFontAttrs();
    gHtml->fontAttr.size[0] = gHtmlHeadingSizes[(u8)((*cur)[-1] - '0')] + '0';
    gHtml->fontAttr.size[1] = 0;
    *cur = strstr(*cur, D_00839CA0) + 1;
    Layout_FlushBreak(text);
    Html_PushFont();
    return 0;
}

/* </H1> .. </H6>, </H> */
s32 Html_TagEndH(u8 **cur, u8 *text) {
    Layout_FlushBreak(text);
    Html_PopFont();
    return 0;
}

/* <DL> */
s32 Html_TagDl(u8 **cur, u8 *text) {
    Layout_FlushBlock(text);
    if (gHtml->lines[gHtml->lineNo - 1].w != 0) {
        Layout_FlushBreak(text);
    }
    return 0;
}

/* </FORM> */
s32 Html_TagEndForm(u8 **cur, u8 *text) {
    u8 depth;
    u8 kind;

    Layout_FlushBlock(text);
    depth = gHtml->tableDepth;
    if (depth != 0) {
        kind = gHtml->cells[gHtml->cellStack[depth]].kind;
        if (kind == 3 || kind == 4) {
            if (gHtml->lines[gHtml->lineNo - 1].w != 0) {
                Layout_FlushBreak(text);
            }
        }
    } else if (gHtml->lines[gHtml->lineNo - 1].w != 0) {
        Layout_FlushBreak(text);
    }
    return 0;
}

/* <P> */
s32 Html_TagP(u8 **cur, u8 *text) {
    Layout_FlushBlock(text);
    if (gHtml->paragraph == 0) {
        if (gHtml->lines[gHtml->lineNo - 1].w != 0) {
            Layout_FlushBreak(text);
        }
        gHtml->paragraph = 1;
    }
    return 0;
}

/* </P> */
s32 Html_TagEndP(u8 **cur, u8 *text) {
    gHtml->paragraph = 0;
    Layout_FlushBlock(text);
    if (gHtml->lines[gHtml->lineNo - 1].w != 0) {
        Layout_FlushBreak(text);
    }
    return 0;
}

/* </DL> */
s32 Html_TagEndDl(u8 **cur, u8 *text) {
    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].indent = 0;
    } else {
        gHtml->indent = 0;
    }
    return 0;
}

/* <DD> */
s32 Html_TagDd(u8 **cur, u8 *text) {
    HtmlCell *cell;

    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        Layout_CellAlign(cell);
        cell->indent = 0x3C;
    } else {
        gHtml->indent = 0x3C;
    }
    return 0;
}

/* <DT> */
s32 Html_TagDt(u8 **cur, u8 *text) {
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].indent = 0;
    } else {
        gHtml->indent = 0;
    }
    Layout_FlushBlock(text);
    return 0;
}

/* </DT>, </DD>, <SCRIPT ...>, <STYLE ...> */
s32 Html_TagFlush(u8 **cur, u8 *text) {
    Layout_Flush(text);
    return 0;
}

/* <IMG>: laid out at once */
s32 Html_TagImg(u8 **cur, u8 *text) {
    Layout_Flush(text);
    Html_ClearImgAttrs();
    Layout_Image();
    return 0;
}

/* <IMG ...> */
s32 Html_TagImgAttrs(u8 **cur, u8 *text) {
    Layout_Flush(text);
    Html_ClearImgAttrs();
    return 0;
}

/* <IMG SRC=...> */
s32 Html_AttrImgSrc(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->imgSrc, 0x100);
    return 0;
}

/* <IMG WIDTH=...> */
s32 Html_AttrImgWidth(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    gHtml->imgWidth = Html_ParseNumber(buf);
    return 0;
}

/* <IMG HEIGHT=...> */
s32 Html_AttrImgHeight(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    gHtml->imgHeight = Html_ParseNumber(buf);
    return 0;
}

/* <IMG ALT=...> */
s32 Html_AttrImgAlt(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->imgAlt, 0x100);
    Html_DecodeEntities(gHtml->imgAlt);
    return 0;
}

/* <IMG USEMAP=...> */
s32 Html_AttrImgUsemap(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->imgUsemap, 0x100);
    return 0;
}
