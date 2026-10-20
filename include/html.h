/* The lobby's HTML browser (the KDDI "MMBB" portal pages): the parser's and
 * layout's state and tables, in the lobby overlay.
 *
 * The parser (Html_ParseSome, Html_ParseTag) walks the page text at
 * gHtml->cur. In a tag, it looks the text up in the tag table of the
 * current mode (gHtmlModeTags[mode]: 0/1 the head, 2 the body, 3.. one table
 * per tag with attributes), moves cur past the tag's name, sets mode to the
 * entry's next mode and calls the entry's handler. Text between tags
 * collects in text[] and is laid out by the handlers (Layout_Flush and the
 * other line breaks) into lines[] and, inside tables, cells[]. */
#ifndef HTML_H
#define HTML_H

#include "common.h"

/* One entry of a tag or attribute table (gHtmlModeTags' tables). */
typedef struct HtmlTag {
    const char *name; /* "<TD ", "</TABLE>", "&LT;", "ALIGN" ... */
    s32 size;         /* strlen(name) + 1 */
    s32 next;         /* the mode after it: its attribute table's index */
    s32 (*handler)(u8 **cur, u8 *text);
} HtmlTag;

/* gHtmlModeTags[mode]: the tag table of each mode. */
typedef struct HtmlTagSet {
    HtmlTag *tags;
    s32 count;
    s32 collectText;  /* 1: text between this mode's tags is collected (Html_ParseSome) */
} HtmlTagSet;

/* The parser's passes over a page (HtmlState.measure; Html_Reset). */
#define HTML_PASS_DRAW 0            /* lays out and draws */
#define HTML_PASS_LAYOUT 1          /* the second pass: measures with the tables' sizes known */
#define HTML_PASS_MEASURE (-10)     /* the first pass: measures the tables */

/* The page's lines (HtmlState.lines): the layout stays on the last. */
#define HTML_LINES 1000

/* The longest attribute value the tag handlers read (Html_ReadAttrValue,
 * Html_ReadAttrKeyword); their buffers hold it and its terminator. */
#define HTML_ATTR_MAX 0x100

/* A table cell's kind (HtmlCell.kind): the tag that opened it. */
#define HTML_CELL_TABLE 1
#define HTML_CELL_TR 2
#define HTML_CELL_TH 3
#define HTML_CELL_TD 4

/* The text's wrapping flags: a cell's (HtmlCell.flags), and the page's
 * outside tables (HtmlState.flags). */
#define HTML_FLAG_PRE 1            /* inside <PRE>: no wrapping, blanks kept */
#define HTML_FLAG_NOWRAP 2         /* <TD NOWRAP> (HtmlState.nowrap holds it until the cell opens) */

/* A cell's size flags (HtmlCell.sizeFlags), and a column edge's flags
 * (HtmlCol.flags) while Layout_MeasureColumns measures a table. */
#define HTML_SIZE_WIDTH 1           /* WIDTH given: the column edge is fixed */
#define HTML_SIZE_HEIGHT 2          /* HEIGHT given */
#define HTML_SIZE_NO_GROW 4         /* set with HTML_SIZE_WIDTH: the room left over isn't shared to it */
#define HTML_SIZE_SPANNED 8         /* a column edge inside a COLSPAN cell */
#define HTML_SIZE_FIXED (HTML_SIZE_WIDTH | HTML_SIZE_NO_GROW)

/* The columns a table can have (Layout_MeasureColumns' edge arrays). */
#define HTML_COLS 32

/* gCharClass's bits: the parser's character classes. */
#define HTML_CHAR_SJIS_LEAD 1       /* a Shift-JIS lead byte: a two-byte character starts */
#define HTML_CHAR_BLANK 2           /* a space or a tab */
#define HTML_CHAR_VALUE_END 4       /* ends an unquoted attribute value: a line end, a space, '"' or '>' */

/* The byte &nbsp; leaves in the text: a space that doesn't break. */
#define HTML_CHAR_NBSP 7

/* The centring depths packed in HtmlState.center and HtmlCell.center. */
#define HTML_CENTER_DEPTH 0x0F      /* <CENTER>'s depth (at most 15) */
#define HTML_DIV_CENTER_DEPTH 0xF0  /* <DIV ALIGN=CENTER>'s depth (at most 15) */
#define HTML_DIV_CENTER_STEP 0x10   /* one <DIV ALIGN=CENTER> */
#define HTML_DIV_DEPTH_MAX 0x10     /* the <DIV>s whose alignment is kept (HtmlState.divAlign) */

/* An ALIGN keyword's value (gHtmlAlignKeywords). */
#define HTML_ALIGN_LEFT 1
#define HTML_ALIGN_CENTER 2
#define HTML_ALIGN_RIGHT 3

/* A VALIGN keyword's value (gHtmlValignKeywords). Seen: the layout
 * centres a cell's text at 2 and puts it at the bottom at 3. */
#define HTML_VALIGN_TOP 1
#define HTML_VALIGN_MIDDLE 2
#define HTML_VALIGN_BOTTOM 3

/* The table cells (HtmlState.cells) and the open tables and cells. */
#define HTML_CELLS 500
#define HTML_TABLE_DEPTH 16         /* tables nest at most 15 deep (HtmlState.tableDepth, bgStack) */

/* The text waiting in HtmlState.text (0x80 bytes) is laid out first when
 * more than this many characters wait: room for a two-byte character and
 * the terminator. */
#define HTML_TEXT_FULL 0x7D

/* The <FONT SIZE> range: 1 to 7, 3 the base of "+N" and "-N". */
#define HTML_FONT_SIZE_BASE 3
#define HTML_FONT_SIZE_MAX 7

/* An <INPUT TYPE>'s value (gInputTypeKeywords; HtmlState.input.type). */
#define HTML_INPUT_TEXT 0
#define HTML_INPUT_PASSWORD 1
#define HTML_INPUT_SUBMIT 2         /* SUBMIT or BUTTON */
#define HTML_INPUT_RESET 3
#define HTML_INPUT_RADIO 4
#define HTML_INPUT_CHECKBOX 5
#define HTML_INPUT_HIDDEN 6
#define HTML_INPUT_IMAGE 7

/* The colour names gHtmlColorNames holds. */
#define HTML_COLOR_NAMES 147

/* What the text being laid out is (HtmlState.link). */
#define HTML_LINK_NONE 0
#define HTML_LINK_LOCAL 1           /* inside <A HREF="#name"> */
#define HTML_LINK_URL 2             /* inside <A HREF=url> */
#define HTML_LINK_NAME 3            /* inside <A NAME> */

/* A keyword and its value (gInputTypeKeywords INPUT's TYPE, gHtmlAlignKeywords ALIGN,
 * gHtmlValignKeywords VALIGN): {name, value, strlen(name) + 1}. */
typedef struct HtmlKeyword {
    const char *name;
    u8 size;
    u8 value;
    u8 pad[2];
} HtmlKeyword;

/* The <GAME-STYLE> keywords' values (gGameStyleKeywords; Html_GameStyle).
 * MOUSE to CHOICE_MV turn the page style bits on or off in bit order
 * (PAGE_STYLE_POINTER ... PAGE_STYLE_NO_MSG_ICON); "KEY=ON" sets a bit. */
enum GameStyleKeyword {
    GAME_STYLE_EXIT = 0,        /* EXIT=..:n: the exit number (BrowserWork.exitArgs) */
    GAME_STYLE_MOUSE = 1,
    GAME_STYLE_SCROLL = 2,      /* also the scroll setting after a ':' */
    GAME_STYLE_TITLE = 3,
    GAME_STYLE_BACK = 4,        /* also the back link (HtmlState.backUrl) */
    GAME_STYLE_FORWARD = 5,
    GAME_STYLE_CANCEL = 6,
    GAME_STYLE_RELOAD = 7,
    GAME_STYLE_LINK_U = 8,
    GAME_STYLE_X_SHOW = 9,
    GAME_STYLE_CHOICE_MV = 10,
    GAME_STYLE_FRONT_LABEL = 11 /* the front label's number (HtmlState.frontLabel) */
};

/* A page's style before its <GAME-STYLE> (Html_Parse): every bit but
 * PAGE_STYLE_BACK, PAGE_STYLE_FORWARD and PAGE_STYLE_MSG_ICON. */
#define HTML_PAGE_STYLE_DEFAULT (PAGE_STYLE_POINTER | PAGE_STYLE_VSCROLL | PAGE_STYLE_STATUS_BAR | PAGE_STYLE_CANCEL | PAGE_STYLE_RELOAD | PAGE_STYLE_UNDERLINE_LINKS | PAGE_STYLE_NO_MSG_ICON)

/* The page keywords a comment opens with (gPageCommentKeywords;
 * Html_Comment keeps the value in HtmlState.pageKind, or in accessKey for
 * the access keys 8, 0xA, 0xB, 0x28 and 0x2C-0x2F). The page kinds'
 * keywords end as Page_SetKind's PAGE_KIND_* (Html_Parse). Seen of the
 * others: 2 and 5 are page kinds Page_SetKind doesn't take (2 draws the
 * page button), 0x14-0x20 are ignored. */
#define PAGE_KW_NONE 0              /* no keyword: a <CSV> block of the kind set before may follow */
#define PAGE_KW_TOP_SIGNUP 1
#define PAGE_KW_INP_SIGNUP 3
#define PAGE_KW_COMP_SIGNUP 4
#define PAGE_KW_INP_PWCHANGE 6
#define PAGE_KW_CMP_PWCHANGE 7
#define PAGE_KW_LOGIN_ID 9          /* on a COMP-SIGNUP page: the new login id follows (Form_SetLoginId) */
#define PAGE_KW_PAGE_BUTTON 0xC     /* only draws the page button */
#define PAGE_KW_CSV_SESSION 0xD     /* 0xD-0x11: a <CSV> block of kind keyword - 0xC follows (CSV_KIND_*) */
#define PAGE_KW_CSV_ACCOUNT 0xE
#define PAGE_KW_CSV_SERVERS 0x10
#define PAGE_KW_CSV_RESULT 0x11
#define PAGE_KW_CRS_TOP 0x12
#define PAGE_KW_CRS_MENU 0x13       /* also a CSV_KIND_MENU block */
#define PAGE_KW_CSV_PORTAL 0x21     /* a CSV_KIND_PORTAL block; a page kind Page_SetKind doesn't take */
#define PAGE_KW_COMP_LOGIN 0x29
#define PAGE_KW_KEICAN_MMBB 0x2A
#define PAGE_KW_CANCAN_MMBB 0x2B
#define PAGE_KW_CSV_ADD_USER 0x62   /* turns a CSV_KIND_SESSION block into CSV_KIND_ACCOUNT */
#define PAGE_KW_CSV_ARM 0x63        /* arms the next CSV_KIND_MENU block's first form (HtmlState.csvArmed) */

/* The kinds of <CSV> block (HtmlState.csvKind; Html_Csv reads its quoted
 * items into the MMBB data's fields, MMBB_FIELD_*, and passes the kind to
 * Page_SetDataMode). */
#define CSV_KIND_NONE 0
#define CSV_KIND_SESSION 1          /* the result, the connection id, field 4 and the next URL */
#define CSV_KIND_ACCOUNT 2          /* the same with the user id */
#define CSV_KIND_SERVERS 4          /* the result and up to 40 lobby servers, or the error */
#define CSV_KIND_RESULT 5           /* the result, and the error's message when it isn't "OK" */
#define CSV_KIND_MENU 6             /* armed: fields 14 and 15; else field 16 and up to 10 rows of 17-19 */
#define CSV_KIND_PORTAL 7           /* the result and the error's message */

/* A colour name (gHtmlColorNames, 147 of them: "ALICEBLUE" ...):
 * {name, strlen(name) + 1, r, g, b}. */
typedef struct HtmlColor {
    const char *name;
    u8 size;
    u8 r;
    u8 g;
    u8 b;
} HtmlColor;

/* One line of the laid-out page (HtmlState.lines). */
typedef struct HtmlLine {
    u16 w;
    u16 h;
} HtmlLine;

/* The layout pen (HtmlState.pen, 0xD8BC). */
typedef struct HtmlPen {
    u16 runX;   /* 0x00: x where the current run of text starts */
    u16 runY;   /* 0x02 */
    u16 x;      /* 0x04: x after the text so far */
    u16 y;      /* 0x06 */
    u16 left;   /* 0x08 */
    u16 h;      /* 0x0A */
    u16 runW;   /* 0x0C: x - runX when the run is flushed */
    u16 wrapW;  /* 0x0E: the x the pen wraps at (the cell or page width) */
    u8 lineUsed; /* 0x10: nonzero once the line has content: leading spaces are skipped while 0 */
} HtmlPen;

/* A table cell (HtmlState.cells, 0x5C bytes). */
typedef struct HtmlCell {
    struct HtmlCell *parent;    /* 0x000: the enclosing table or row (NULL: none) */
    struct HtmlCell *above;     /* 0x004: the previous sibling (a row: the row above) */
    struct HtmlCell *next;      /* 0x008: the next cell or row */
    struct HtmlCell *first;     /* 0x00C: the first row or cell */
    struct HtmlCell *last;      /* 0x010: the last row or cell */
    u8 *src;                    /* 0x014: the parse position of the row (read again after measuring) */
    u16 index;                  /* 0x018: the cell's own index in cells[] */
    u8 depth;                   /* 0x01A: gHtml->tableDepth when opened (Layout_OpenCell); Layout_DrawCell passes it to Draw_Cell */
    u8 kind;                    /* 0x01B: the tag that opened it, HTML_CELL_* */
    u16 w;                      /* 0x01C */
    u16 h;                      /* 0x01E: the height: HEIGHT of <TABLE>/<TD> (tableAttr/tdAttr.height), else grown to the contents and the row */
    u16 minW;                   /* 0x020: the narrowest width: the longest word or line that can't wrap; a row adds its cells'; COLSPAN cells share it (Layout_MeasureColumns: wmin) */
    u16 maxW;                   /* 0x022: the width without wrapping (Layout_MeasureColumns: wmax); a <HR> or line end raises it */
    u16 innerH;                 /* 0x024: the rows' heights added up (a table); a cell's height is at least it */
    u16 widthAttr;              /* 0x026: WIDTH of <TABLE>/<TD> (0: none); a nested table takes it as the room (else w) */
    u16 x;                      /* 0x028: left edge on the page: the parent's x + penX (+ padding/spacing, ALIGN) */
    u16 y;                      /* 0x02A: top edge on the page: the parent's y + penY */
    u16 penX;                   /* 0x02C: x of the next child inside it: a row adds each cell's width with padding and spacing; a table starts at its border */
    u16 penY;                   /* 0x02E: y of the next child (row) inside it: gHtml->cellPen.y at a nested table */
    u16 padding;                /* 0x030: CELLPADDING (tableAttr.cellPadding; rows and cells copy the table's) */
    u16 spacing;                /* 0x032: CELLSPACING (tableAttr.cellSpacing; copied down) */
    u16 spanFirst;              /* 0x034: the first cell of a COLSPAN (the cells after it continue it; Layout_CellColSpan) */
    u16 spanPrev;               /* 0x036: the cell above that this ROWSPAN cell continues (0: none; Layout_RowSpanCell, Layout_CellChainEnd) */
    u16 textH;                  /* 0x038: the height of its text (gHtml->cellPen.runY when closed); VALIGN centres it in h (Layout_CellValign) */
    u16 alignX;                 /* 0x03A: x offset of the current line for ALIGN (Layout_CellAlign: 0, half or all the room left) */
    u16 valignY;                /* 0x03C: y offset of the text for VALIGN (Layout_CellValign) */
    u16 indent;                 /* 0x03E: left margin (<DD>: 0x3C) */
    u16 spanH;                  /* 0x040: a ROWSPAN cell's text height, carried to the cells below (Layout_RowSpanCell) */
    u16 spanNext;               /* 0x042: the cell below that continues this ROWSPAN cell (0: none) */
    u8 cellCount;               /* 0x044: a row's cells (counted as they close, while measuring) */
    u8 border;                  /* 0x045: BORDER (tableAttr.border; copied down); Draw_Cell's frame width */
    u8 sizeFlags;               /* 0x046: HTML_SIZE_*: WIDTH (HTML_SIZE_FIXED), HEIGHT */
    u8 colSpan;                 /* 0x047: COLSPAN (tdAttr.colSpan; 0: none) */
    u8 rowSpan;                 /* 0x048: ROWSPAN (tdAttr.rowSpan; counts down in the rows below) */
    u8 spanLast;                /* 0x049: 1: the last cell of a COLSPAN run (Layout_CellColSpan) */
    u8 align;                   /* 0x04A */
    u8 valign;                  /* 0x04B */
    u8 colNo;                   /* 0x04C: a cell's column; a row's next column */
    u8 rowNo;                   /* 0x04D: a row's index in its table (Layout_RowFromTable: table->rowNo++); a table's row count */
    u8 open;                    /* 0x04E: 1 while the <TABLE> is open (Layout_OpenCell; Layout_RowSpanCell clears); an end tag closes it only then */
    u8 bold;                    /* 0x04F */
    u8 flags;                   /* 0x050: HTML_FLAG_* */
    u8 center;                  /* 0x051: low nibble: <CENTER> depth, high nibble: <DIV ALIGN=CENTER> depth */
    u8 unk052[0x054 - 0x052];
    s32 bgColor;                /* 0x054: -1: none */
    s32 borderColor;            /* 0x058: BORDERCOLOR (tableAttr.borderColor); Draw_Cell */
} HtmlCell;

/* A column edge of a table being measured (the stack arrays of
 * Layout_MeasureColumns, HTML_COLS columns): the column's right edge from the row's
 * left, and HTML_SIZE_* flags (a cell's sizeFlags, and HTML_SIZE_SPANNED). */
typedef struct HtmlCol {
    u8 flags;
    u16 x; /* 0x02 */
} HtmlCol;

/* An image map's area (HtmlState.maps, 0x104 bytes). */
typedef struct HtmlMap {
    char usemap[0x100]; /* "#name": the <IMG USEMAP> */
    u16 x;              /* 0x100: where the image is */
    u16 y;              /* 0x102 */
} HtmlMap;

/* <FONT COLOR=... SIZE=...> as read (HtmlState.fontAttr, cleared by
 * Html_ClearFontAttrs). */
typedef struct HtmlFontAttr {
    char color[0x10];
    char size[0x10];
} HtmlFontAttr;

/* <INPUT ...>'s attributes (HtmlState.input, 0x40C bytes, cleared by
 * Html_ClearInputAttrs). */
typedef struct HtmlInput {
    char name[0x100];
    char value[0x100]; /* 0x100 */
    u8 type;           /* 0x200: gInputTypeKeywords: 0 TEXT, 1 PASSWORD, 2 SUBMIT/BUTTON, 3 RESET, 4 RADIO, 5 CHECKBOX, 6 HIDDEN, 7 IMAGE */
    u16 size;          /* 0x202 (16 when 0) */
    u16 maxLength;     /* 0x204 (0x100 when 0 or more) */
    u8 checked;        /* 0x206 */
    char src[0x100];   /* 0x207 */
    char alt[0x100];   /* 0x307 */
    u16 width;         /* 0x408 */
    u16 height;        /* 0x40A */
} HtmlInput;

/* <SELECT NAME=...> (HtmlState.select). */
typedef struct HtmlSelect {
    char name[0x100];
    u8 inOption; /* 0x100: an <OPTION> is open */
} HtmlSelect;

/* <OPTION VALUE=... SELECTED> (HtmlState.option). */
typedef struct HtmlOption {
    char value[0x100];
    u8 selected; /* 0x100 */
} HtmlOption;

/* <TABLE ...>'s attributes (HtmlState.tableAttr, 0x114 bytes, cleared by
 * Html_ClearTableAttrs). */
typedef struct HtmlTableAttr {
    u8 border;
    u16 cellPadding;       /* 0x02 */
    u16 cellSpacing;       /* 0x04 (2) */
    u16 width;             /* 0x06 */
    u16 height;            /* 0x08 */
    s32 borderColor;       /* 0x0C (-1: none) */
    s32 bgColor;           /* 0x10 (-1: none) */
    char background[0x100]; /* 0x14 */
} HtmlTableAttr;

/* <TD ...>/<TH ...>'s attributes (HtmlState.tdAttr, 0x10C bytes, cleared by
 * Html_ClearTdAttrs). */
typedef struct HtmlTdAttr {
    u16 width;
    u16 height;            /* 0x02 */
    u8 rowSpan;            /* 0x04 */
    u8 colSpan;            /* 0x05 */
    u8 align;              /* 0x06 */
    u8 valign;             /* 0x07 */
    s32 bgColor;           /* 0x08 (-1: none) */
    char background[0x100]; /* 0x0C */
} HtmlTdAttr;

/* <FORM ACTION=... METHOD=...> (HtmlState.form). */
typedef struct HtmlForm {
    char action[0x100];
    u8 post; /* 0x100: METHOD=POST */
} HtmlForm;

/* <META HTTP-EQUIV=REFRESH CONTENT=... URL=...> (HtmlState.meta, 0x108
 * bytes). */
typedef struct HtmlMeta {
    u8 refresh;
    s32 seconds;    /* 0x04 */
    char url[0x100]; /* 0x08 */
} HtmlMeta;

/* An <AREA> of an image map (HtmlState.area, 0x318 bytes). */
typedef struct HtmlArea {
    char unk000[0x100];
    char alt[0x100];   /* 0x100 */
    char href[0x100];  /* 0x200 */
    char shape[0x10];  /* 0x300: "RECT" (upper-cased) */
    u16 coords[4];     /* 0x310: x1, y1, x2, y2 */
} HtmlArea;

typedef struct HtmlState {
    s32 parseState;             /* 0x000: 0 text, 1 in a tag (Html_ParseTag) */
    s32 textLen;                /* 0x004: characters in text[] (0xD8E4) */
    s32 mode;                   /* 0x008: the tag table in use: index into gHtmlModeTags (2 = body) */
    s32 inComment;              /* 0x00C: set by "<!--" until "-->" */
    u16 width;                  /* 0x010: the page's width (0x248) */
    u16 indent;                 /* 0x012: left margin (<DD>: 0x3C) */
    u8 center;                  /* 0x014: low nibble: <CENTER> depth, high nibble: <DIV ALIGN=CENTER> depth */
    u8 unk015[0x016 - 0x015];
    u16 cellPenX;               /* 0x016: the pen x within the current table cell: each text, image and select width is added (Layout_CellText, Layout_CellImage, Layout_CellSelectEnd); the cell maxW is kept from it */
    u16 selectWidth;            /* 0x018: the longest <OPTION> text of the <SELECT> */
    u8 unk01A[0x01C - 0x01A];
    s32 lineLen;                /* 0x01C: characters in line[] */
    char line[0x100];           /* 0x020: the text of the current line run */
    s32 bgColorValue;           /* 0x120: <BODY BGCOLOR> as 0x00RRGGBB */
    s16 fontDepth;              /* 0x124: index into fontColor[] and fontSize[] */
    u8 unk126[0x128 - 0x126];
    s32 fontColor[16];          /* 0x128 */
    u8 fontSize[16];            /* 0x168: 1..7 */
    s32 linkColorValue;         /* 0x178: <BODY LINK> */
    u8 bold;                    /* 0x17C */
    u8 strike;                  /* 0x17D */
    u8 underline;               /* 0x17E */
    u8 link;                    /* 0x17F: in <A HREF>: 1 "#name", 2 a URL; 3 in <A NAME> */
    u8 charW;                   /* 0x180: a full-width character's width at the current size (gFullCharWidths) */
    u8 halfW;                   /* 0x181: a half-width character's width (gHalfCharWidths) */
    u16 pageWidth;              /* 0x182: the page width: the widest line, image, input or cell end (Html_MaxU16); 584 at the start (Html_Parse) */
    u16 bottom;                 /* 0x184: the lowest y laid out */
    s8 measure;                 /* 0x186: the pass, HTML_PASS_*: nonzero, measuring, nothing is drawn */
    u8 unk187[0x188 - 0x187];
    u16 lineNo;                 /* 0x188: index into lines[] */
    u8 nowrap;                  /* 0x18A: <TD NOWRAP>: HTML_FLAG_NOWRAP */
    u8 flags;                   /* 0x18B: HTML_FLAG_PRE outside tables */
    u8 pre;                     /* 0x18C: inside <PRE> */
    u8 paragraph;               /* 0x18D: inside <P> */
    u8 pageKind;                /* 0x18E: the page keyword of the comment (Html_Comment), Page_SetKind at the end */
    u8 accessKey;               /* 0x18F: the key code a comment gives the next input, link or map area (Html_Comment; Html_MapKeyCode) */
    u8 pageButton;              /* 0x190: page kinds 1, 2, 4: Html_Parse draws the page button at the bottom */
    u8 csvKind;                 /* 0x191: the <CSV> kind of the comment (Html_Csv) */
    u8 csvArmed;                /* 0x192: keyword 0x63: the next kind-6 <CSV> is read (once) */
    char bgColor[0x10];         /* 0x193: <BODY BGCOLOR> */
    char background[0x100];     /* 0x1A3: <BODY BACKGROUND> */
    char textColor[0x10];       /* 0x2A3: <BODY TEXT> ("#000000") */
    char linkColor[0x10];       /* 0x2B3: <BODY LINK> ("#0000FF") */
    HtmlFontAttr fontAttr;      /* 0x2C3 */
    u8 unk2E3[0x2E4 - 0x2E3];
    HtmlInput input;            /* 0x2E4 */
    HtmlSelect select;          /* 0x6F0 */
    HtmlOption option;          /* 0x7F1 */
    u16 hrSize;                 /* 0x8F2 */
    u16 hrWidth;                /* 0x8F4 */
    char anchor[0x100];         /* 0x8F6: <A HREF="#..."> */
    char href[0x100];           /* 0x9F6: <A HREF=...> */
    char imgSrc[0x100];         /* 0xAF6 */
    char imgAlt[0x100];         /* 0xBF6 */
    char imgUsemap[0x100];      /* 0xCF6 */
    u16 imgWidth;               /* 0xDF6 */
    u16 imgHeight;              /* 0xDF8 */
    u8 unkDFA[0xDFC - 0xDFA];
    HtmlTableAttr tableAttr;    /* 0xDFC */
    HtmlTdAttr tdAttr;          /* 0xF10 */
    HtmlForm form;              /* 0x101C */
    u8 unk111D[0x1120 - 0x111D];
    HtmlMeta meta;              /* 0x1120 */
    HtmlArea area;              /* 0x1228: the <AREA> being read */
    HtmlLine lines[1000];       /* 0x1540: HTML_LINES */
    HtmlCell cells[500];        /* 0x24E0 */
    u16 cellCount;              /* 0xD890: the last cell of cells[] in use */
    u8 tableDepth;              /* 0xD892 */
    u8 unkD893[0xD894 - 0xD893];
    u16 curCell;                /* 0xD894 */
    u16 unkD896;                /* 0xD896 */
    u16 spanCell;               /* 0xD898: the first cell of the COLSPAN being laid out */
    u16 cellStack[16];          /* 0xD89A */
    u8 maxDepth;                /* 0xD8BA: the deepest table nesting (16: too deep) */
    u8 unkD8BB[0xD8BC - 0xD8BB];
    HtmlPen pen;                /* 0xD8BC */
    HtmlPen cellPen;            /* 0xD8CE: the pen in a table cell (from the origin of the cell) */
    char *commentEnd;           /* 0xD8E0 */
    u8 text[0x80];              /* 0xD8E4: text waiting to be laid out */
    u8 *cur;                    /* 0xD964: the parse position */
    u16 tagCount;               /* 0xD968 */
    char bgStack[16][0x100];    /* 0xD96A: the backgrounds of the open tables and cells */
    u8 bgDepth;                 /* 0xE96A */
    u8 inFrame;                 /* 0xE96B: Html_Parse argument: a page in a frame: no inputs, selects, options or rules are laid out (Layout_Input, Layout_Option, Layout_Rule ...) */
    u8 divDepth;                /* 0xE96C */
    u8 divAlign[17];            /* 0xE96D: <DIV ALIGN> of each open <DIV> (gHtmlAlignKeywords: 1 LEFT, 2 CENTER, 3 RIGHT) */
    u16 pageStyle;              /* 0xE97E: the page style bits (PAGE_STYLE_*, Page_SetStyle; Html_GameStyle sets them from <GAME-STYLE>); HTML_PAGE_STYLE_DEFAULT at the start (Html_Parse) */
    u8 backUrl[0x100];          /* 0xE980: <GAME-STYLE> BACK=ON:url (or the url chosen by the exit number): Page_SetStyle's text */
    HtmlMap maps[16];           /* 0xEA80: <MAP NAME> places, set by <IMG USEMAP> */
    u8 mapCount;                /* 0xFAC0 */
    char mapName[0x100];        /* 0xFAC1 */
    u8 frontLabel;              /* 0xFBC1: <GAME-STYLE> FRONT_LABEL=n (Page_SetStyle's mode) */
    u8 scroll;                  /* 0xFBC2: <GAME-STYLE> SCROLL=n (Page_SetStyle's last argument) */
    u8 sortServers;             /* 0xFBC3: the lobby servers page (CSV kind 4): Lbs_Sort at the end */
} HtmlState;

/* The sizes the game uses (MWCC rejects a negative array size). */
typedef char HtmlState_size_check[(sizeof(HtmlState) == 0xFBC4) ? 1 : -1];
typedef char HtmlCell_size_check[(sizeof(HtmlCell) == 0x5C) ? 1 : -1];
typedef char HtmlInput_size_check[(sizeof(HtmlInput) == 0x40C) ? 1 : -1];
typedef char HtmlTableAttr_size_check[(sizeof(HtmlTableAttr) == 0x114) ? 1 : -1];
typedef char HtmlTdAttr_size_check[(sizeof(HtmlTdAttr) == 0x10C) ? 1 : -1];
typedef char HtmlMeta_size_check[(sizeof(HtmlMeta) == 0x108) ? 1 : -1];
typedef char HtmlArea_size_check[(sizeof(HtmlArea) == 0x318) ? 1 : -1];

extern HtmlState *gHtml;

/* ---- The image decoders: PNG and BMP into an Image for the texture
 * code (their error strings name them "plPNG...", "plBMP..."). ---- */
#include "image.h"
#include "zlib.h"

/* The row being unfiltered (PngState.rowInfo, for Png_FilterRow). */
typedef struct PngRowInfo {
    u32 width;      /* 0x0: pixels */
    u32 rowBytes;   /* 0x4 */
    u8 colorType;   /* 0x8 */
    u8 bitDepth;    /* 0x9 */
    u8 channels;    /* 0xA */
    u8 pixelBits;   /* 0xB */
} PngRowInfo;

/* The PNG decoder's state (gPng, cleared for each image). The file
 * is read from memory (data, size bytes; left still to read), a chunk at
 * a time into ptr; IDAT's data goes through zlib into rows. */
typedef struct PngState {
    s32 left;           /* 0x00: bytes of the file not read yet */
    s32 size;           /* 0x04 */
    u8 *ptr;            /* 0x08: the bytes just read */
    s8 err;             /* 0x0C: -3 read past the end, or zlib's error */
    u32 width;          /* 0x10: IHDR */
    u32 height;         /* 0x14 */
    u8 bitDepth;        /* 0x18 */
    u8 colorType;       /* 0x19: 0 grey, 2 RGB, 3 palette, 4 grey+alpha, 6 RGBA */
    u8 compression;     /* 0x1A */
    u8 filterMethod;    /* 0x1B */
    u8 interlace;       /* 0x1C */
    u8 channels;        /* 0x1D */
    u8 pixelBits;       /* 0x1E: bitDepth * channels */
    u8 pad1F;
    u32 rowBytes;       /* 0x20 */
    u8 hasTrns;         /* 0x24: tRNS given */
    u8 trnsR;           /* 0x25: RGB images: the transparent colour */
    u8 trnsG;           /* 0x26 */
    u8 trnsB;           /* 0x27 */
    z_stream zs;        /* 0x28 */
    u8 *zbuf;           /* 0x70: compressed data read from IDAT */
    u32 zbufSize;       /* 0x74 */
    u32 rows;           /* 0x78 */
    u32 rowWidth;       /* 0x7C */
    u32 rowSize;        /* 0x80: rowBytes + 1 (the filter byte) */
    u8 *rowAlloc;       /* 0x84 */
    u8 *row;            /* 0x88: rowAlloc + 0x20 */
    u8 *prevRow;        /* 0x8C */
    u8 pad90[4];
    u32 idatLeft;       /* 0x94: bytes of the IDAT chunk not read yet */
    PngRowInfo rowInfo; /* 0x98 */
    u32 rowNo;          /* 0xA4 */
    u8 *data;           /* 0xA8: the file */
    u8 started;         /* 0xAC: the rows are set up */
    u8 padAD[3];
    u8 *palette;        /* 0xB0: PLTE's data */
    u16 paletteCount;   /* 0xB4 */
    u8 padB6[2];
    u8 *trns;           /* 0xB8: tRNS's data (palette images) */
    u16 trnsCount;      /* 0xBC */
    u8 padBE[2];
} PngState;

typedef char PngState_size_check[(sizeof(PngState) == 0xC0) ? 1 : -1];

extern PngState gPng;

#endif
