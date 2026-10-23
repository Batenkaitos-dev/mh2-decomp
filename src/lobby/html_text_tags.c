/* The HTML tag handlers, first part: the handler of the tags the parser
 * skips, then <HTML>, <HEAD>, <TITLE>, <B>, <U>, <S>, <CENTER>, <Hn>,
 * <DL>/<DT>/<DD> and <P>, the tags of the text.
 * The parser's tag sets (gHtmlModeTags) point at them. Each handler gets
 * the parse position and the pending text and returns 0 (or < 0 to stop
 * the page). */
#include "common.h"
#include "html.h"
#include "browser.h"
#include "libc.h"

extern u8 gHtmlHeadingSizes[];
extern char gHtmlBlack[]; /* "#000000": the default text colour */
extern char gHtmlBlue[]; /* "#0000FF": the default link colour */

/* <!--, <HTML ...>, <MAP ...> and ends that do nothing. */
s32 Html_TagIgnore(u8 **cur, u8 *text)
{
    return 0;
}

/* <HTML>, and <HTML ...>'s ">": start a page. */
s32 Html_TagHtml(u8 **cur, u8 *text)
{
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
    strcpy(gHtml->textColor, gHtmlBlack);
    strcpy(gHtml->linkColor, gHtmlBlue);
    return 0;
}

/* </HTML>, </BODY> */
s32 Html_TagEndHtml(u8 **cur, u8 *text)
{
    Layout_FlushBreak(text);
    return 0;
}

/* <BR> */
s32 Html_TagBr(u8 **cur, u8 *text)
{
    Layout_FlushBreak(text);
    return 0;
}

/* <HEAD>, </HEAD>, <TITLE>, <BASE ...>, <BODY ...> and the ends of
 * <SCRIPT>/<STYLE>: drop the pending text. */
s32 Html_TagDropText(u8 **cur, u8 *text)
{
    gHtml->textLen = 0;
    *text = 0;
    return 0;
}

/* </TITLE> */
s32 Html_TagEndTitle(u8 **cur, u8 *text)
{
    Html_SetTitle(text);
    return 0;
}

/* <BASE HREF=...> */
s32 Html_AttrBaseHref(u8 **cur, u8 *text)
{
    char buf[256];

    Html_ReadAttrValue(cur, buf, sizeof(buf));
    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        Html_SetBase(buf);
    }
    return 0;
}

/* <B>, <STRONG> */
s32 Html_TagB(u8 **cur, u8 *text)
{
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
s32 Html_TagEndB(u8 **cur, u8 *text)
{
    u16 i;

    Layout_Flush(text);
    if (gHtml->tableDepth != 0) {
        i = gHtml->curCell;
        if (gHtml->cells[i].kind == HTML_CELL_TH) {
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
s32 Html_TagU(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    gHtml->underline = 1;
    return 0;
}

/* </U> */
s32 Html_TagEndU(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    gHtml->underline = 0;
    return 0;
}

/* <STRIKE>, <S> */
s32 Html_TagS(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    gHtml->strike = 1;
    return 0;
}

/* </STRIKE>, </S> */
s32 Html_TagEndS(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    gHtml->strike = 0;
    return 0;
}

/* <CENTER> */
s32 Html_TagCenter(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    u8 packed;
    u8 depth;

    Layout_FlushBlock(text);
    if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        packed = cell->center;
        depth = packed & HTML_CENTER_DEPTH;
        if (depth != HTML_CENTER_DEPTH) {
            depth++;
            cell->center = (packed & HTML_DIV_CENTER_DEPTH) | depth;
        }
        Layout_CellAlign(cell);
    } else {
        HtmlState *html = gHtml;
        packed = html->center;
        depth = packed & HTML_CENTER_DEPTH;
        if (depth != HTML_CENTER_DEPTH) {
            depth++;
            html->center = (packed & HTML_DIV_CENTER_DEPTH) | depth;
        }
    }
    return 0;
}

/* </CENTER> */
s32 Html_TagEndCenter(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    u8 packed;
    u8 depth;

    Layout_FlushBlock(text);
    if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        packed = cell->center;
        depth = packed & HTML_CENTER_DEPTH;
        if (depth != 0) {
            depth--;
            cell->center = (packed & HTML_DIV_CENTER_DEPTH) | depth;
        }
    } else {
        HtmlState *html = gHtml;
        packed = html->center;
        depth = packed & HTML_CENTER_DEPTH;
        if (depth != 0) {
            depth--;
            html->center = (packed & HTML_DIV_CENTER_DEPTH) | depth;
        }
    }
    return 0;
}

/* <H1> .. <H6>: the font size from the heading's digit. */
s32 Html_TagH(u8 **cur, u8 *text)
{
    Html_ClearFontAttrs();
    gHtml->fontAttr.size[0] = gHtmlHeadingSizes[(u8)((*cur)[-1] - '0')] + '0';
    gHtml->fontAttr.size[1] = 0;
    *cur = strstr(*cur, gHtmlTagClose) + 1;
    Layout_FlushBreak(text);
    Html_PushFont();
    return 0;
}

/* </H1> .. </H6>, </H> */
s32 Html_TagEndH(u8 **cur, u8 *text)
{
    Layout_FlushBreak(text);
    Html_PopFont();
    return 0;
}

/* <DL> */
s32 Html_TagDl(u8 **cur, u8 *text)
{
    Layout_FlushBlock(text);
    if (gHtml->lines[gHtml->lineNo - 1].w != 0) {
        Layout_FlushBreak(text);
    }
    return 0;
}

/* </FORM> */
s32 Html_TagEndForm(u8 **cur, u8 *text)
{
    u8 depth;
    u8 kind;

    Layout_FlushBlock(text);
    depth = gHtml->tableDepth;
    if (depth != 0) {
        kind = gHtml->cells[gHtml->cellStack[depth]].kind;
        if (kind == HTML_CELL_TH || kind == HTML_CELL_TD) {
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
s32 Html_TagP(u8 **cur, u8 *text)
{
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
s32 Html_TagEndP(u8 **cur, u8 *text)
{
    gHtml->paragraph = 0;
    Layout_FlushBlock(text);
    if (gHtml->lines[gHtml->lineNo - 1].w != 0) {
        Layout_FlushBreak(text);
    }
    return 0;
}

/* </DL> */
s32 Html_TagEndDl(u8 **cur, u8 *text)
{
    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].indent = 0;
    } else {
        gHtml->indent = 0;
    }
    return 0;
}

/* <DD> */
s32 Html_TagDd(u8 **cur, u8 *text)
{
    HtmlCell *cell;

    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        Layout_CellAlign(cell);
        cell->indent = 60;    /* 60 dots */
    } else {
        gHtml->indent = 60;
    }
    return 0;
}

/* <DT> */
s32 Html_TagDt(u8 **cur, u8 *text)
{
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].indent = 0;
    } else {
        gHtml->indent = 0;
    }
    Layout_FlushBlock(text);
    return 0;
}

/* </DT>, </DD>, <SCRIPT ...>, <STYLE ...> */
s32 Html_TagFlush(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    return 0;
}

