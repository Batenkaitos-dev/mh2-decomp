# Fields of HtmlState (include/html.h), the HTML parser's and layout's state
# (D_008B2748, 0xFBC4 bytes, cleared by Html_Parse).
# Regenerate with: python tools/gen_struct.py include/html_state.spec include/html.h
name HtmlState
size 0xFBC4
0x000 s32 parseState // 0 text, 1 in a tag (Html_ParseTag)
0x004 s32 textLen // characters in text[] (0xD8E4)
0x008 s32 mode // the tag table in use: index into gHtmlModeTags (2 = body)
0x00C s32 inComment // set by "<!--" until "-->"
0x010 u16 width // the page's width (0x248)
0x012 u16 indent // left margin (<DD>: 0x3C)
0x014 u8 center // low nibble: <CENTER> depth, high nibble: <DIV ALIGN=CENTER> depth
0x016 u16 cellPenX // the pen x within the current table cell: each text, image and select width is added (Layout_CellText, Layout_CellImage, Layout_CellSelectEnd); the cell maxW is kept from it
0x018 u16 selectWidth // the longest <OPTION> text of the <SELECT>
0x01C s32 lineLen // characters in line[]
0x020 char line[0x100] // the text of the current line run
0x120 s32 bgColorValue // <BODY BGCOLOR> as 0x00RRGGBB
0x124 s16 fontDepth // index into fontColor[] and fontSize[]
0x128 s32 fontColor[16]
0x168 u8 fontSize[16] // 1..7
0x178 s32 linkColorValue // <BODY LINK>
0x17C u8 bold
0x17D u8 strike
0x17E u8 underline
0x17F u8 link // in <A HREF>: 1 "#name", 2 a URL
0x180 u8 charW // a full-width character's width at the current size (gFullCharWidths)
0x181 u8 halfW // a half-width character's width (gHalfCharWidths)
0x182 u16 pageWidth // the page width: the widest line, image, input or cell end (Html_MaxU16); 584 at the start (Html_Parse)
0x184 u16 bottom // the lowest y laid out
0x186 s8 measure // nonzero: measuring (-10 a table), nothing is drawn
0x188 u16 lineNo // index into lines[]
0x18A u8 nowrap // <TD NOWRAP>: 2
0x18B u8 flags // bit 0: <PRE> (no wrapping)
0x18C u8 pre // inside <PRE>
0x18D u8 paragraph // inside <P>
0x18E u8 pageKind // the page keyword of the comment (Html_Comment), Page_SetKind at the end
0x18F u8 accessKey // the key code a comment gives the next input, link or map area (Html_Comment; Html_MapKeyCode)
0x190 u8 pageButton // page kinds 1, 2, 4: Html_Parse draws the page button at the bottom
0x191 u8 csvKind // the <CSV> kind of the comment (Html_Csv)
0x192 u8 csvArmed // keyword 0x63: the next kind-6 <CSV> is read (once)
0x193 char bgColor[0x10] // <BODY BGCOLOR>
0x1A3 char background[0x100] // <BODY BACKGROUND>
0x2A3 char textColor[0x10] // <BODY TEXT> ("#000000")
0x2B3 char linkColor[0x10] // <BODY LINK> ("#0000FF")
0x2C3 HtmlFontAttr{32} fontAttr
0x2E4 HtmlInput{1036} input
0x6F0 HtmlSelect{257} select
0x7F1 HtmlOption{257} option
0x8F2 u16 hrSize
0x8F4 u16 hrWidth
0x8F6 char anchor[0x100] // <A HREF="#...">
0x9F6 char href[0x100] // <A HREF=...>
0xAF6 char imgSrc[0x100]
0xBF6 char imgAlt[0x100]
0xCF6 char imgUsemap[0x100]
0xDF6 u16 imgWidth
0xDF8 u16 imgHeight
0xDFC HtmlTableAttr{276} tableAttr
0xF10 HtmlTdAttr{268} tdAttr
0x101C HtmlForm{257} form
0x1120 HtmlMeta{264} meta
0x1228 HtmlArea{792} area // the <AREA> being read
0x1540 HtmlLine{4} lines[1000]
0x24E0 HtmlCell{92} cells[500]
0xD890 u16 cellCount // the last cell of cells[] in use
0xD892 u8 tableDepth
0xD894 u16 curCell
0xD896 u16 unkD896
0xD898 u16 spanCell // the first cell of the COLSPAN being laid out
0xD89A u16 cellStack[16]
0xD8BA u8 maxDepth // the deepest table nesting (16: too deep)
0xD8BC HtmlPen{18} pen
0xD8CE HtmlPen{18} cellPen // the pen in a table cell (from the origin of the cell)
0xD8E0 char *commentEnd
0xD8E4 u8 text[0x80] // text waiting to be laid out
0xD964 u8 *cur // the parse position
0xD968 u16 tagCount
0xD96A char bgStack[16][0x100] // the backgrounds of the open tables and cells
0xE96A u8 bgDepth
0xE96B u8 inFrame // Html_Parse argument: a page in a frame: no inputs, selects, options or rules are laid out (Layout_Input, Layout_Option, Layout_Rule ...)
0xE96C u8 divDepth
0xE96D u8 divAlign[17] // <DIV ALIGN> of each open <DIV> (gHtmlAlignKeywords: 1 LEFT, 2 CENTER, 3 RIGHT)
0xE97E u16 pageStyle // the page style bits (Page_SetStyle; Html_GameStyle sets bits 2, 3, 6 and 7); 0x2E7 at the start (Html_Parse)
0xE980 u8 backUrl[0x100] // <GAME-STYLE> BACK=ON:url (or the url chosen by the exit number): Page_SetStyle's text
0xEA80 HtmlMap{260} maps[16] // <MAP NAME> places, set by <IMG USEMAP>
0xFAC0 u8 mapCount
0xFAC1 char mapName[0x100]
0xFBC1 u8 frontLabel // <GAME-STYLE> FRONT_LABEL=n (Page_SetStyle's mode)
0xFBC2 u8 scroll // <GAME-STYLE> SCROLL=n (Page_SetStyle's last argument)
0xFBC3 u8 sortServers // the lobby servers page (CSV kind 4): Lbs_Sort at the end
