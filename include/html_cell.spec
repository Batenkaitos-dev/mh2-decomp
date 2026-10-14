# Fields of HtmlCell (include/html.h), a table, row or cell of the layout
# (HtmlState.cells). Regenerate with:
#   python tools/gen_struct.py include/html_cell.spec include/html.h
name HtmlCell
size 0x5C
0x00 struct HtmlCell *parent // the enclosing table or row (NULL: none)
0x14 u8 *src // the parse position of the row (read again after measuring)
0x1B u8 kind // 1 <TABLE>, 2 <TR>, 3 <TH>, 4 <TD>
0x1C u16 w
0x1E u16 h // the height: HEIGHT of <TABLE>/<TD> (tableAttr/tdAttr.height), else grown to the contents and the row
0x20 u16 minW // the narrowest width: the longest word or line that can't wrap; a row adds its cells'; COLSPAN cells share it (Layout_MeasureColumns: wmin)
0x24 u16 innerH // the rows' heights added up (a table); a cell's height is at least it
0x28 u16 x // left edge on the page: the parent's x + penX (+ padding/spacing, ALIGN)
0x2E u16 penY // y of the next child (row) inside it: gHtml->cellPen.y at a nested table
0x30 u16 padding // CELLPADDING (tableAttr.cellPadding; rows and cells copy the table's)
0x32 u16 spacing // CELLSPACING (tableAttr.cellSpacing; copied down)
0x3A u16 alignX // x offset of the current line for ALIGN (Layout_CellAlign: 0, half or all the room left)
0x3E u16 indent // left margin (<DD>: 0x3C)
0x40 u16 spanH // a ROWSPAN cell's text height, carried to the cells below (Layout_RowSpanCell)
0x42 u16 spanNext // the cell below that continues this ROWSPAN cell (0: none)
0x45 u8 border // BORDER (tableAttr.border; copied down); Draw_Cell's frame width
0x48 u8 rowSpan // ROWSPAN (tdAttr.rowSpan; counts down in the rows below)
0x4A u8 align
0x4C u8 colNo // a cell's column; a row's next column
0x4E u8 open // 1 while the <TABLE> is open (Layout_OpenCell; Layout_RowSpanCell clears); an end tag closes it only then
0x4F u8 bold
0x50 u8 flags // bit 0: <PRE> (no wrapping)
0x51 u8 center // low nibble: <CENTER> depth, high nibble: <DIV ALIGN=CENTER> depth
0x0C struct HtmlCell *first // the first row or cell
0x2A u16 y // top edge on the page: the parent's y + penY
0x44 u8 cellCount // a row's cells (counted as they close, while measuring)
0x4D u8 rowNo // a row's index in its table (Layout_RowFromTable: table->rowNo++); a table's row count
0x22 u16 maxW // the width without wrapping (Layout_MeasureColumns: wmax); a <HR> or line end raises it
0x2C u16 penX // x of the next child inside it: a row adds each cell's width with padding and spacing; a table starts at its border
0x38 u16 textH // the height of its text (gHtml->cellPen.runY when closed); VALIGN centres it in h (Layout_CellValign)
0x47 u8 colSpan // COLSPAN (tdAttr.colSpan; 0: none)
0x3C u16 valignY // y offset of the text for VALIGN (Layout_CellValign)
0x4B u8 valign
0x26 u16 widthAttr // WIDTH of <TABLE>/<TD> (0: none); a nested table takes it as the room (else w)
0x46 u8 sizeFlags // 1|4: WIDTH given, 2: HEIGHT given
0x54 s32 bgColor // -1: none
0x04 struct HtmlCell *above // the previous sibling (a row: the row above)
0x08 struct HtmlCell *next // the next cell or row
0x18 u16 index // the cell's own index in cells[]
0x1A u8 depth // gHtml->tableDepth when opened (Layout_OpenCell); Layout_DrawCell passes it to Draw_Cell
0x36 u16 spanPrev // the cell above that this ROWSPAN cell continues (0: none; Layout_RowSpanCell, Layout_CellChainEnd)
0x58 s32 borderColor // BORDERCOLOR (tableAttr.borderColor); Draw_Cell
0x34 u16 spanFirst // the first cell of a COLSPAN (the cells after it continue it; Layout_CellColSpan)
0x49 u8 spanLast // 1: the last cell of a COLSPAN run (Layout_CellColSpan)
0x10 struct HtmlCell *last // the last row or cell
