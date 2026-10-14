/*
 * The keyboard's conversion candidate box: a unit of its own at level 4 (its "%s" is not the
 * one of unk_314E00.c's functions). Where it starts (after 0x314580) and ends (before
 * 0x3166D0) is not known: it is cut to Kbd_DrawCandidateBox alone.
 */
#include "common.h"
#include "em.h"

/* Built at optimization level 4: Kbd_DrawCandidateBox matches only at 4, while Kbd_PrevCand
 * and Kbd_NextCand (unk_30EA30.c) match only at 3. */
#pragma optimization_level 4

extern u8 gKbdCandLines[];
#include "kbd.h"

typedef struct Rect16 {
    s16 x, y, w, h;
    s32 color;
} Rect16;
struct PrimLine;
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
    Rect16 r;
    s16 dx;
    f32 sc;
    s16 i;
    s32 ty;
    u8 *p;
    s32 yy;

    if (gKbd->converting != 0) {
        sc = gKbd->drawScale;
        if (sc != 1.0f) {
            dx = 10.0f * gKbd->candCol - 28.0f;
        } else {
            dx = 10.0f * gKbd->candCol - 7.0f;
        }
        yy = y;
        r.x = x + dx * sc;
        r.w = r.x + 0xA2;
        r.y = yy + 0x14;
        r.h = r.y + 0x56;
        r.color = *(s32 *)gKbd->kindConf;
        Prim_FillRect((struct PrimLine *)&r);
        r.x = r.x + (s32)(5.0f * *(f32 *)&gKbd->drawScale); /* through an (f32 *) view: a plain gKbd->drawScale here and below allocates other registers */
        r.w -= (s16)(5.0f * *(f32 *)&gKbd->drawScale);
        r.y += 5;
        r.h -= 5;
        r.color = 0xFF303030;
        Prim_FillRect((struct PrimLine *)&r);
        Font_SetSpacing(0x12, 0x12);
        x += 10.0f * gKbd->candCol;
        Text_SetColor(0);
        for (i = 0, ty = y, p = gKbdCandLines; i < 3; i++, ty += 20, p += 0x100) {
            if (i != gKbd->candIdx % 3) {
                Text_SetColor(0);
            } else {
                Text_SetColor(-3);
            }
            Font_SetPos(x + 10.0f, 26.0f + ty);
            Kbd_Printf("%s", p);
        }
        Text_SetColor(0);
        sprintf(buf, "%3d/%3d", gKbd->candIdx + 1, gKbd->candCount);
        Font_SetPos(52.0f + (20.0f + x), yy + 0x53);
        Kbd_Printf(buf);
    }
}
