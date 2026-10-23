#ifndef TEXT_H
#define TEXT_H

/*
 * The text layer over the font library.
 * Text_PrintCodes and the helpers around it read control codes that start
 * with '~' (Text_ParseCode):
 *   ~Cnn  a colour (Text_SetColor);
 *   ~Ann  a number that Text_FormatVar inserts (TEXT_VAR_*);
 *   ~Bnn  an icon drawn in the line (Text_AddIcon; nn is the Icon's kind);
 *   ~%    starts or ends a part that isn't drawn;
 *   ~~    a '~'.
 * Text_ParseCode's mode TEXT_PARSE_CODE gives TEXT_CODE_* | nn (0 for ~~ and
 * for an unknown code).
 */
#define TEXT_CODE_KIND 0xFF00   /* the mask of the code's kind */
#define TEXT_CODE_ARG 0x00FF    /* the mask of its number, nn */
#define TEXT_CODE_COLOR 0x100
#define TEXT_CODE_VAR 0x200
#define TEXT_CODE_ICON 0x400
#define TEXT_CODE_HIDE 0x800

/* Text_ParseCode's modes: what it gives for the code. */
#define TEXT_PARSE_CODE 0       /* the code, TEXT_CODE_* | nn */
#define TEXT_PARSE_WIDTH 1      /* what it adds to the string's length as displayed */
#define TEXT_PARSE_CHARS 2      /* the number of characters it stands for */

/* Text_FormatVar's numbers (~Ann). */
#define TEXT_VAR_CARD_SLOT 0    /* the memory card's slot, from 1 (gCardMenu.slot + 1) */
#define TEXT_VAR_CARD_BLOCKS 1  /* the save's size in card blocks (gCardMenu.fileBlocks) */

/* Text_NextChar's results: what it copied. */
#define TEXT_CHAR_SINGLE 0      /* a one-byte character */
#define TEXT_CHAR_DOUBLE 1      /* a two-byte (Shift-JIS) character */
#define TEXT_CHAR_ICON 2        /* a ~B code: out[0] is the icon's kind */
#define TEXT_CHAR_VAR 3         /* a ~A code: out[0] is the number's kind */
#define TEXT_CHAR_END 4         /* the end of the string */

#define TEXT_LAYERS 7           /* the font's pages, each with an icon queue (gTextIcons) */
#define TEXT_LAYER_ICONS 8      /* the icons a layer queues at most */

/*
 * Text_SetColor's colours: below TEXT_COLOR_RGB, the font's own colour
 * index; from TEXT_COLOR_RGB up to TEXT_COLOR_RGB_END, an RGBA colour of
 * gTextColors (gTextCodeColors is its address less TEXT_COLOR_RGB
 * entries, so that the colour indexes it directly).
 */
#define TEXT_COLOR_RGB 0x20
#define TEXT_COLOR_RGB_END 0x35

/* The size of the text functions' buffers (a formatted string with its codes). */
#define TEXT_BUF_SIZE 0x400

/* Shift-JIS: the first byte of a two-byte character (an unsigned byte), and
 * the full-width space, which draws nothing. */
#define SJIS_IS_LEAD(c) (((c) >= 0x80 && (c) <= 0x9F) || ((c) >= 0xE0 && (c) <= 0xFF))
#define SJIS_SPACE 0x8140

void Draw_BeginFrame(void);
void Draw_EndFrame(void);
void Gfx_SetBlend(s32 modeA, s32 modeB);
void Gfx_SetTexFilter(s32 mode);
void Tex_Bind(s32 slot);
void Gfx_ResetRenderStates(s32 partial);
void Gfx_SetBlendOp(s32 opIndex);
void FreeList_InitAll(void);
void Text_Init(void);
void Text_Reset(void);
void Text_FlushPages(void);
void Text_DrawAllAndStop(void);
void Text_DrawPage(s32 layer);
void Text_DrawCurrentPage(void);
void Text_DrawIcons(s32 layer);
void Text_PrintfColored(const char *fmt, ...);
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
void Text_DrawLines(s16 x, s16 y, char **lines, s16 dy);
void Text_DrawLinesColored(s16 x, s16 y, char **lines, s16 dy);
void Text_FormatVar(void *dst, s16 kind);
char *Text_ParseCode(char *text, s16 *code, s16 mode);
void Text_Printf(const char *fmt, ...);
void Text_PrintfStyled(s32 lineGap, const char *fmt, ...);
void Text_PrintCodes(s16 max, const char *fmt, ...);
void Text_DrawShadow(s16 x, s16 y, s16 shadowColor, s16 color, char *text);
void Text_DrawShadow2(s16 x, s16 y, s32 shadowColor, s32 color, char *text);
void Text_DrawShadowRgb(s16 x, s16 y, u32 shadowColor, u32 color, char *text);
void Text_Draw(char *text);
s32 Text_DisplayLength(char *text);
s32 Text_CountChars(char *text);
s32 Text_NextChar(char *out, char *text, char **next);
void Text_SetSize(s32 size);
void Text_SetColor(s16 color);
void Text_DrawAllPages(void);
void Text_SetDefaults(void);
void Text_Disable(void);
void MemFile_Load(void);
u16 Pad_KeyRepeat(s32 pad, u16 held, u16 pressed);
void Text_DrawColored(char *text);

#endif /* TEXT_H */
