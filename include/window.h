#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"

/*
 * sub_main's menu windows (src/sub_main/unk_356380.c): a position, the
 * character size in pixels (charW, charH: Font_SetSpacing's), the text size in
 * characters (lineChars wide, lines high; a line is charH + 2 pixels), a frame
 * style in flags & 7 and colours. Window_DrawTextAlpha and the list windows read a text colour at
 * +8; Window_DrawText and the list panels read padX, padY there.
 */
/* A menu window (gChatFrameBox, gPersonalStatusPanels[], gEquipWindowBox...): position, size in cells, colour and flags. */
typedef struct Window {
    s16 x;                      /* 0x0 */
    s16 y;                      /* 0x2 */
    u8 charW;                   /* 0x4: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x5: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x6: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x7: text lines */
    s16 color;                  /* 0x8: text colour */
    u16 flags;                  /* 0xA */
    u32 frameColor;             /* 0xC: the frame's second colour (flags 0x8000; Window_DrawTextAlpha), Panel.color for Window_DrawText */
} Window;

typedef char Window_size_check[(sizeof(Window) == 0x10) ? 1 : -1];

/* A window frame as WindowFrame_Draw draws it. */
typedef struct Frame {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    s16 w;                      /* 0x04 */
    s16 h;                      /* 0x06 */
    u32 style;                  /* 0x08: 0-5; 0x80000000: color2 given */
    u32 color;                  /* 0x0C */
    u32 color2;                 /* 0x10 */
} Frame;

typedef char Frame_size_check[(sizeof(Frame) == 0x14) ? 1 : -1];

/* A menu window as Window_DrawText draws it (gHelpWindowBox, gChatFrameBox, gPersonalStatusBoxes[]...). */
typedef struct Panel {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    u8 charW;                   /* 0x04: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x05: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x06: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x07 */
    u8 padX;                    /* 0x08 */
    u8 padY;                    /* 0x09 */
    u16 flags;                  /* 0x0A: 0-7 frame style (at most 5), 0x4000, 0x8000 */
    u32 color;                  /* 0x0C: frame colours */
    u32 color2;                 /* 0x10 */
    s16 textColor;              /* 0x14: a text colour; with rgbText an RGBA for the whole word */
    u8 unk16;
    u8 rgbText;                 /* 0x17 */
} Panel;

typedef char Panel_size_check[(sizeof(Panel) == 0x18) ? 1 : -1];

/* A list window (ListPanel_DrawAlpha): a Window with its entries and cursor. 0x1C bytes: lobby
 * NetMode_DrawPart and plsel Plsel_DrawPlayerMenuList copy 0x1C into a local they draw, and
 * every object is followed by the next at +0x20 even when that one is 8-aligned
 * (select gGalleryListPanel); nothing reads +0x18. */
typedef struct ListWin {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    u8 charW;                   /* 0x04: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x05: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x06: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x07 */
    s16 color;                  /* 0x08: text colour */
    u16 flags;                  /* 0x0A: 0-7 frame style, 0x2000 taller lines, 0x8000 color2 given */
    char **items;               /* 0x0C: NULL-terminated */
    s32 cursorColor;            /* 0x10: the cursor bar colour (0xRRGGBB; ListPanel_DrawCursorBar pulses its alpha) */
    u32 color2;                 /* 0x14 */
    s32 unk18;                  /* 0x18: 0 in every object */
} ListWin;

typedef char ListWin_size_check[(sizeof(ListWin) == 0x1C) ? 1 : -1];

/* A list panel (ListPanel_Draw): a Panel with a cursor kind and its entries. */
typedef struct ListPanel {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    u8 charW;                   /* 0x04: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x05: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x06: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x07 */
    u8 padX;                    /* 0x08 */
    u8 padY;                    /* 0x09 */
    u16 flags;                  /* 0x0A: 0-7 frame style, 0x2000 lines between the entries, 0x4000, 0x8000 */
    u32 color;                  /* 0x0C */
    u32 color2;                 /* 0x10 */
    s16 textColor;              /* 0x14 */
    u8 unk16;
    u8 rgbText;                 /* 0x17 */
    s32 cursorColor;            /* 0x18: the cursor bar colour (0xRRGGBB; ListPanel_DrawCursorBar pulses its alpha) */
    char **items;               /* 0x1C: NULL-terminated */
} ListPanel;

typedef char ListPanel_size_check[(sizeof(ListPanel) == 0x20) ? 1 : -1];

#endif
