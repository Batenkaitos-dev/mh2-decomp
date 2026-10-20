/* The open <SELECT> list (gBrowserSub, BrowserSub): its cursor, scrolling
 * and choosing. include/browser.h. */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"
#include "kbd.h"
/* Level 4 like its neighbours unk_733AB0.c, unk_734710.c, unk_73C310.c: the
 * other functions match at both levels, Page_Navigate only here. */
#pragma optimization_level 4

void LobbySe_Play(s32 se); /* the lobby's sound effects */
s32 Page_PointTo(u16 idx, s32 sound); /* unit's own declaration: with the definition's s32 (u16 i, s8 anywhere) (lobby/unk_734710.c), PageObj_Activate 32 of 155 words differ */
s32 strcmp(const char *a, const char *b); /* main: strcmp */

/* PageObj_Activate: the object at obj[idx] chosen (with sound 1 when sound):
 * a link, a button, a field ... (Page_PointTo); 1 when that did
 * something. A link (2) with the same href as the object before it is
 * part of it and does nothing. */
s32 PageObj_Activate(u16 idx, s32 sound)
{
    BrowserObj *o = gBrowserWork->obj[idx];
    BrowserObj *prev;

    switch (o->kind) {
    case 0xD:
        switch (o->active) {
        case 3:
        case 5:
            gBrowserWork->cursorOnLink = 1;
            if (sound) {
                LobbySe_Play(1);
            }
            if (Page_PointTo(idx, sound) != 0) {
                return 1;
            }
        }
        return 0;
    case 0x11:
    case 0x12:
        if (*(u8 *)o->s1 != 0) {
            gBrowserWork->cursorOnLink = 1;
            if (sound) {
                LobbySe_Play(1);
            }
            if (Page_PointTo(idx, sound) != 0) {
                return 1;
            }
        }
        return 0;
    case 8:
        if (o->active != 0xB) {
            return 0;
        }
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 0xB:
    case 0xC:
        gBrowserWork->cursorOnLink = 1;
        if (sound) {
            LobbySe_Play(1);
        }
        if (Page_PointTo(idx, sound) != 0) {
            return 1;
        }
        return 0;
    case 2:
        prev = gBrowserWork->obj[idx - 1];
        if (prev != NULL && prev->active != 0 && strcmp(prev->s1, o->s1) == 0) {
            return 0;
        }
        gBrowserWork->cursorOnLink = 1;
        if (sound) {
            LobbySe_Play(1);
        }
        if (Page_PointTo(idx, sound) != 0) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* Closes the list: the cursor back on the <SELECT>, sound 4. */
void Select_Close(void)
{
    gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y);
    gBrowserSub->listCount = 0;
    gBrowserSub->activeObj = 0;
    LobbySe_Play(4);
}

/* Select_Down: the list's cursor one option down (18 pixels), the list
 * scrolled when it shows 10 of more options (unk4 options, unk5 the first
 * shown; unk6: the list is above the <SELECT>); sound 1 when it moved. */
void Select_Down(void)
{
    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - gBrowserState->x);
    if (gBrowserSub->listUp != 0) {
        if (gBrowserSub->cursorY <= gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y - 18.0f) {
            gBrowserSub->cursorY += 18;
        } else {
            if (gBrowserSub->listCount <= 10) {
                return;
            }
            if (gBrowserSub->listTop > gBrowserSub->listCount - 10) {
                return;
            }
            gBrowserSub->listTop++;
        }
    } else if (gBrowserSub->listCount < 11) {
        if (gBrowserSub->cursorY < 20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y) + (gBrowserSub->listCount - 1) * 18) {
            gBrowserSub->cursorY += 18;
        } else {
            return;
        }
    } else {
        if (gBrowserSub->cursorY < 162.0f + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y))) {
            gBrowserSub->cursorY += 18;
        } else {
            if (gBrowserSub->listTop > gBrowserSub->listCount - 10) {
                return;
            }
            gBrowserSub->listTop++;
        }
    }
    LobbySe_Play(1);
}

/* Select_Up: the list's cursor one option up, the list scrolled at its
 * top; sound 1 when it moved. */
void Select_Up(void)
{
    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - gBrowserState->x);
    if (gBrowserSub->listUp != 0) {
        if (gBrowserSub->listCount <= 10) {
            if (gBrowserSub->cursorY > gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y - (gBrowserSub->listCount - 1) * 18) {
                gBrowserSub->cursorY -= 18;
            } else {
                return;
            }
        } else if (gBrowserSub->cursorY > gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y - 162.0f) {
            gBrowserSub->cursorY -= 18;
        } else {
            if (gBrowserSub->listTop == 1) {
                return;
            }
            gBrowserSub->listTop--;
        }
    } else if (gBrowserSub->cursorY >= 18.0f + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y))) {
        gBrowserSub->cursorY -= 18;
    } else {
        if (gBrowserSub->listTop == 1) {
            return;
        }
        gBrowserSub->listTop--;
    }
    LobbySe_Play(1);
}

/* The pointer's speed for each tilt of the left stick (.data, copied to
 * the stack whole). */
typedef struct StickSpeeds {
    f32 v[4];
} StickSpeeds __attribute__((aligned(16)));
extern StickSpeeds gPageStickSpeeds;
f32 sinf(f32 x); /* main: sinf? */
f32 cosf(f32 x); /* main: cosf? */

/* Page_Navigate: the page's keys, pad and stick each frame: the left
 * stick moves the pointer (kept inside an open select list); with the
 * stick used as cursor (unk18 bit 0 clear) or the arrow keys / the pad's
 * up and down, the open list's cursor moves (Select_Up, Select_Down) or
 * the focus goes to the next / previous active object (PageObj_Activate,
 * wrapping around), with auto-repeat every 8 frames (unkE); Escape or the
 * pad's 0x40 close the list, Page Up / Page Down scroll it by 9. */
void Page_Navigate(void)
{
    StickSpeeds speed;
    f32 a;
    f32 x;
    f32 y;
    s32 rows;
    s32 i;
    s32 pressed;
    s32 held;
    BrowserObj *o;

    speed = gPageStickSpeeds;
    if (gBrowserState->page == PAGE_LOAD_REQUESTED && (gBrowserState->style & PAGE_STYLE_NO_MSG_ICON) >> 9 == 0) {
        return;
    }
    if ((gBrowserState->style & PAGE_STYLE_POINTER) || gBrowserWork->stickTilt == 0) {
        a = 3.1415927f * (u32)gBrowserWork->stickDir / 8.0f;
        gBrowserSub->cursorX += (s16)(speed.v[gBrowserWork->stickTilt] * sinf(a));
        gBrowserSub->cursorY -= (s16)(speed.v[gBrowserWork->stickTilt] * cosf(a));
        if (gBrowserSub->listCount > 0) {
            /* The array's address as a u32 plus the byte offset, as this
             * browser code reads the same array elsewhere (browser_cache_draw.c's
             * PAGE_FOCUS, (u32)gBrowserWork + 0x5EC + ...): obj[unk20] would
             * index from gBrowserWork itself (lw 0x5EC(a)), where the game adds
             * 0x5EC first. */
            o = *(BrowserObj **)((u32)gBrowserWork->obj + gBrowserSub->activeObj * 4);
            x = o->x - gBrowserState->x;
            if (gBrowserSub->cursorX < 10.0f + x) {
                gBrowserSub->cursorX = 10.0f + x;
            } else if (gBrowserSub->cursorX > o->w + x - 10.0f) {
                gBrowserSub->cursorX = o->w + x - 10.0f;
            }
            rows = gBrowserSub->listCount > 10 ? 10 : gBrowserSub->listCount;
            if (gBrowserSub->listUp != 0) {
                y = gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y;
                if (gBrowserSub->cursorY < y - (rows * 18 - 9)) {
                    gBrowserSub->cursorY = y - (rows * 18 - 9);
                } else if (gBrowserSub->cursorY > y - 9.0f) {
                    gBrowserSub->cursorY = y - 9.0f;
                }
            } else {
                y = gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y;
                if (gBrowserSub->cursorY < 9.0f + (20.0f + y)) {
                    gBrowserSub->cursorY = 9.0f + (20.0f + y);
                } else if (gBrowserSub->cursorY > 20.0f + y + (rows * 18 - 9)) {
                    gBrowserSub->cursorY = 20.0f + y + (rows * 18 - 9);
                }
            }
        }
    }
    if (!(gBrowserState->style & PAGE_STYLE_POINTER) && gBrowserWork->stickTilt != 0) {
        if (gBrowserWork->stickReleased == 1) {
            gBrowserWork->stickFirst = 1;
        } else {
            gBrowserWork->stickFirst = 0;
        }
        gBrowserWork->stickReleased = 0;
        i = gBrowserWork->stickDir;
        if (i >= 2 && i < 10) {
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || gBrowserWork->stickFirst != 0) {
                gBrowserSub->stickRepeat = 0;
                goto next;
            }
        } else if ((i >= 10 && i <= 15) || (i >= 0 && i < 2)) {
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || gBrowserWork->stickFirst != 0) {
                gBrowserSub->stickRepeat = 0;
                goto prev;
            }
        }
    } else if (D_581AFC == 0x29 && gBrowserSub->listCount > 0) {
        Select_Close();
    } else if (gKbd->usbKey == USB_KEY_TAB && gBrowserSub->listCount == 0) {
        goto next;
    } else if (gKbd->usbKey == USB_KEY_PAGE_UP && gBrowserSub->listCount > 0) {
        if (gBrowserSub->listCount <= 10 || gBrowserSub->listTop < 10) {
            gBrowserSub->listTop = 1;
        } else {
            gBrowserSub->listTop -= 9;
        }
    } else if (gKbd->usbKey == USB_KEY_PAGE_DOWN && gBrowserSub->listCount > 0) {
        if (gBrowserSub->listTop + 9 < gBrowserSub->listCount) {
            gBrowserSub->listTop += 9;
        }
    } else if (D_581AFC >= 0x4F && D_581AFC < 0x53) {
        switch (D_581AFC) {
        case 0x51:
        case 0x4F:
            if ((gKbd->usbPrevKey == USB_KEY_DOWN || gKbd->usbPrevKey == USB_KEY_RIGHT) && gBrowserSub->listCount > 0) {
                goto down;
            }
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || gKbd->usbPrevKey == USB_KEY_RIGHT || gKbd->usbPrevKey == USB_KEY_DOWN) {
                if (gBrowserSub->activeObj != 0) {
                    goto down;
                }
                goto next;
            }
            break;
        case 0x52:
        case 0x50:
            if ((gKbd->usbPrevKey == USB_KEY_UP || gKbd->usbPrevKey == USB_KEY_LEFT) && gBrowserSub->listCount > 0) {
                goto up;
            }
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || gKbd->usbPrevKey == USB_KEY_LEFT || gKbd->usbPrevKey == USB_KEY_UP) {
                if (gBrowserSub->activeObj != 0) {
                    goto up;
                }
                goto prev;
            }
            break;
        }
    } else if (((pressed = gBrowserWork->padPressed) & 0x40) && gBrowserSub->listCount > 0) {
        Select_Close();
    } else if (((held = gBrowserWork->padHeld) & 0x400) || (held & 0x1000)) {
        if (((pressed & 0x400) || (pressed & 0x1000)) && gBrowserSub->listCount > 0) {
        down:
            Select_Down();
        } else {
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || ((pressed = gBrowserWork->padPressed) & 0x400) || (pressed & 0x1000)) {
                if (gBrowserSub->activeObj != 0 && gBrowserSub->listCount > 0) {
                    Select_Down();
                } else {
                next:
                    for (i = gBrowserWork->focus + 1; i < 500; i++) {
                        if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
                            if (gBrowserState->style & PAGE_STYLE_POINTER) {
                                for (i = gBrowserWork->focus; i < 500; i++) {
                                    if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0 || PageObj_Activate(i, 1)) {
                                        return;
                                    }
                                }
                                return;
                            } else {
                                for (i = 0; i < 500; i++) {
                                    if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0 || PageObj_Activate(i, 1)) {
                                        return;
                                    }
                                }
                            }
                            break;
                        }
                        if (PageObj_Activate(i, 1)) {
                            return;
                        }
                    }
                }
            }
        }
    } else if ((held & 0x800) || (held & 0x2000)) {
        if (((pressed & 0x800) || (pressed & 0x2000)) && gBrowserSub->listCount > 0) {
        up:
            Select_Up();
        } else {
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || ((pressed = gBrowserWork->padPressed) & 0x800) || (pressed & 0x2000)) {
                if (gBrowserSub->activeObj != 0 && gBrowserSub->listCount > 0) {
                    Select_Up();
                } else {
                prev:
                    for (i = gBrowserWork->focus - 1; i > 0; i--) {
                        if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0 || PageObj_Activate(i, 1)) {
                            return;
                        }
                    }
                    if (gBrowserState->style & PAGE_STYLE_POINTER) {
                        for (i = gBrowserWork->focus; i > 0; i--) {
                            if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0 || PageObj_Activate(i, 1)) {
                                return;
                            }
                        }
                        return;
                    } else {
                        for (i = 0; i < 500; i++) {
                            if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
                                break;
                            }
                        }
                        for (i--; i > 0; i--) {
                            if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0 || PageObj_Activate(i, 1)) {
                                return;
                            }
                        }
                    }
                }
            }
        }
    } else {
        gBrowserWork->stickReleased = 1;
        gBrowserSub->stickRepeat = 0;
    }
}

/* unk_73ACF0's .data (0x7EEBE0-0x7EEBF0) */
StickSpeeds gPageStickSpeeds = {
#include "lobby/007EEBE0.inc"
};
