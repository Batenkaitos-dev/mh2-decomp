/*
 * The software keyboard's conversion candidate box (Kbd_DrawCandidateBox),
 * drawn while the kana-kanji converter offers candidates (the keyboard's
 * state, gKbd, is in include/kbd.h; the candidates' list is
 * unk_30EA30.c's). The game built it as a unit of its own, at level 4:
 * its "%s" is not the one of unk_314E00.c's functions.
 */
#include "common.h"
#include "em.h"

/* Kbd_DrawCandidateBox matches only at level 4 (Kbd_PrevCand and
 * Kbd_NextCand in unk_30EA30.c only at 3). */
#pragma optimization_level 4

extern u8 gKbdCandLines[];
#include "kbd.h"
#include "prim.h"

s32 Prim_FillRect(struct PrimLine *r);
void Font_SetSpacing(u8 a, u8 b);
void Text_SetColor(s16 color);
void Font_SetPos(s16 x, s16 y);
s32 sprintf(char *dst, char *fmt, ...);
void Kbd_Printf(const char *fmt, ...);

/*
 * Draws the conversion candidate box of the keyboard at x, y: three candidates
 * from gKbdCandLines (the current one, count % 3, highlighted) and "n/total".
 */
void Kbd_DrawCandidateBox(f32 x, s16 y)
{
    char buf[128];
    PrimLine box;
    s16 dx;
    f32 scale;
    s16 i;
    s32 lineY;
    u8 *line;
    s32 boxY;

    if (gKbd->converting != 0) {
        scale = gKbd->drawScale;
        if (scale != 1.0f) {
            dx = 10.0f * gKbd->candCol - 28.0f;
        } else {
            dx = 10.0f * gKbd->candCol - 7.0f;
        }
        boxY = y;
        box.x0 = x + dx * scale;
        box.x1 = box.x0 + 0xA2;
        box.y0 = boxY + 0x14;
        box.y1 = box.y0 + 0x56;
        box.color = *(s32 *)gKbd->kindConf;
        Prim_FillRect(&box);
        box.x0 = box.x0 + (s32)(5.0f * *(f32 *)&gKbd->drawScale); /* through an (f32 *) view: a plain gKbd->drawScale here and below allocates other registers */
        box.x1 -= (s16)(5.0f * *(f32 *)&gKbd->drawScale);
        box.y0 += 5;
        box.y1 -= 5;
        box.color = 0xFF303030;
        Prim_FillRect(&box);
        Font_SetSpacing(0x12, 0x12);
        x += 10.0f * gKbd->candCol;
        Text_SetColor(0);
        for (i = 0, lineY = y, line = gKbdCandLines; i < KBD_CAND_PAGE; i++, lineY += 20, line += KBD_CAND_LINE_SIZE) {
            if (i != gKbd->candIdx % KBD_CAND_PAGE) {
                Text_SetColor(0);
            } else {
                Text_SetColor(-3);
            }
            Font_SetPos(x + 10.0f, 26.0f + lineY);
            Kbd_Printf("%s", line);
        }
        Text_SetColor(0);
        sprintf(buf, "%3d/%3d", gKbd->candIdx + 1, gKbd->candCount);
        Font_SetPos(52.0f + (20.0f + x), boxY + 0x53);
        Kbd_Printf(buf);
    }
}
