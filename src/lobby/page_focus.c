/* The page's focus and the open drop-down list: PageObj_Activate puts the
 * focus and the pointer on a page object; Select_Close, Select_Down and
 * Select_Up close the open list or move its cursor a row (BrowserSub's list
 * fields); Page_Navigate, each frame, moves the pointer with the left
 * stick, and the focus or the list's cursor with the pad, the stick or the
 * USB keyboard. */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "kbd.h"
#include "pad.h"
#include "lobby_util.h"
#include "libm.h"
/* Compiled at optimization level 4: Page_Navigate matches only so, the
 * other functions at both levels. */
#pragma optimization_level 4

s32 Page_PointTo(u16 idx, s32 sound); /* unit's own declaration: the second argument as PageObj_Activate passes it, an s32; with the definition's s8 the calls compile differently */
s32 strcmp(const char *a, const char *b); /* main: strcmp */

/* Puts the focus on page object idx when it can be acted on: an image map
 * or an image button, struck or underlined text with a link, a closed
 * drop-down list, a form's button or box, a radio button or a check box,
 * or a link (not one that goes on from the object before it, with the same
 * href). The pointer moves onto it (Page_PointTo), with the scroll sound
 * when `sound` is set. Returns 1 when the pointer moved to it, else 0. */
s32 PageObj_Activate(u16 idx, s32 sound)
{
    BrowserObj *pageObj = gBrowserWork->obj[idx];
    BrowserObj *prev;

    switch (pageObj->kind) {
    case PAGE_OBJ_IMAGE:
        switch (pageObj->active) {
        case PAGE_IMG_MAP:
        case PAGE_IMG_INPUT:
            gBrowserWork->cursorOnLink = 1;
            if (sound) {
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
            if (Page_PointTo(idx, sound) != 0) {
                return 1;
            }
        }
        return 0;
    case PAGE_OBJ_STRUCK:
    case PAGE_OBJ_UNDERLINED:
        if (*(u8 *)pageObj->s1 != 0) {
            gBrowserWork->cursorOnLink = 1;
            if (sound) {
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
            if (Page_PointTo(idx, sound) != 0) {
                return 1;
            }
        }
        return 0;
    case PAGE_OBJ_SELECT:
        if (pageObj->active != PAGE_SELECT_CLOSED) {
            return 0;
        }
    case PAGE_OBJ_SUBMIT:
    case PAGE_OBJ_RESET:
    case PAGE_OBJ_BUTTON:
    case PAGE_OBJ_TEXT_INPUT:
    case PAGE_OBJ_PASSWORD:
    case PAGE_OBJ_RADIO:
    case PAGE_OBJ_CHECKBOX:
        gBrowserWork->cursorOnLink = 1;
        if (sound) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        if (Page_PointTo(idx, sound) != 0) {
            return 1;
        }
        return 0;
    case PAGE_OBJ_LINK:
        prev = gBrowserWork->obj[idx - 1];
        if (prev != NULL && prev->active != 0 && strcmp(prev->s1, pageObj->s1) == 0) {
            return 0;
        }
        gBrowserWork->cursorOnLink = 1;
        if (sound) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        if (Page_PointTo(idx, sound) != 0) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* Closes the open drop-down list: the pointer back on its field, the
 * cancel sound. */
void Select_Close(void)
{
    gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y);
    gBrowserSub->listCount = 0;
    gBrowserSub->activeObj = 0;
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/* Moves the open list's cursor down a row (DROPLIST_ROW_H); at the last
 * row shown, scrolls a list of more than DROPLIST_ROWS options (listTop,
 * its first option shown). The list opens below its field, or above it
 * (listUp). The scroll sound plays when something moved. */
void Select_Down(void)
{
    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - gBrowserState->x);
    if (gBrowserSub->listUp != 0) {
        if (gBrowserSub->cursorY <= gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y - DROPLIST_ROW_H) {
            gBrowserSub->cursorY += DROPLIST_ROW_H;
        } else {
            if (gBrowserSub->listCount <= DROPLIST_ROWS) {
                return;
            }
            if (gBrowserSub->listTop > gBrowserSub->listCount - DROPLIST_ROWS) {
                return;
            }
            gBrowserSub->listTop++;
        }
    } else if (gBrowserSub->listCount < DROPLIST_ROWS + 1) {
        if (gBrowserSub->cursorY < 20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y) + (gBrowserSub->listCount - 1) * DROPLIST_ROW_H) {
            gBrowserSub->cursorY += DROPLIST_ROW_H;
        } else {
            return;
        }
    } else {
        if (gBrowserSub->cursorY < 162.0f + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y))) {
            gBrowserSub->cursorY += DROPLIST_ROW_H;
        } else {
            if (gBrowserSub->listTop > gBrowserSub->listCount - DROPLIST_ROWS) {
                return;
            }
            gBrowserSub->listTop++;
        }
    }
    LobbySe_Play(LOBBY_SE_SCROLL);
}

/* Moves the open list's cursor up a row; at the first row shown, scrolls
 * the list back. The scroll sound plays when something moved. */
void Select_Up(void)
{
    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - gBrowserState->x);
    if (gBrowserSub->listUp != 0) {
        if (gBrowserSub->listCount <= DROPLIST_ROWS) {
            if (gBrowserSub->cursorY > gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y - (gBrowserSub->listCount - 1) * DROPLIST_ROW_H) {
                gBrowserSub->cursorY -= DROPLIST_ROW_H;
            } else {
                return;
            }
        } else if (gBrowserSub->cursorY > gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y - 162.0f) {
            gBrowserSub->cursorY -= DROPLIST_ROW_H;
        } else {
            if (gBrowserSub->listTop == 1) {
                return;
            }
            gBrowserSub->listTop--;
        }
    } else if (gBrowserSub->cursorY >= DROPLIST_ROW_H + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y))) {
        gBrowserSub->cursorY -= DROPLIST_ROW_H;
    } else {
        if (gBrowserSub->listTop == 1) {
            return;
        }
        gBrowserSub->listTop--;
    }
    LobbySe_Play(LOBBY_SE_SCROLL);
}

/* The pointer's speed for each tilt of the left stick (.data, copied to
 * the stack whole). */
typedef struct StickSpeeds {
    f32 v[4];
} StickSpeeds __attribute__((aligned(16)));
extern StickSpeeds gPageStickSpeeds;

/* The page's pad, stick and keys, each frame. On a page with the pointer
 * (PAGE_STYLE_POINTER), or with the stick at rest, the left stick moves the
 * pointer (kept inside an open list). Otherwise the stick, the pad's
 * directions and the USB keyboard's arrows move the open list's cursor
 * (Select_Down, Select_Up), or the focus to the next or the previous
 * object that can be acted on (PageObj_Activate), wrapping round on a page
 * without the pointer; held, they repeat every 8 frames (stickRepeat). Tab
 * goes to the next object; Esc or the cross closes the list; Page Up and
 * Page Down scroll it by DROPLIST_ROWS - 1 rows. */
void Page_Navigate(void)
{
    StickSpeeds speed;
    f32 angle;
    f32 x;
    f32 y;
    s32 rows;
    s32 i;
    s32 pressed;
    s32 held;
    BrowserObj *listObj;

    speed = gPageStickSpeeds;
    if (gBrowserState->page == PAGE_LOAD_REQUESTED && (gBrowserState->style & PAGE_STYLE_NO_MSG_ICON) >> 9 == 0) {
        return;
    }
    if ((gBrowserState->style & PAGE_STYLE_POINTER) || gBrowserWork->stickTilt == 0) {
        angle = 3.1415927f * (u32)gBrowserWork->stickDir / 8.0f;
        gBrowserSub->cursorX += (s16)(speed.v[gBrowserWork->stickTilt] * sinf(angle));
        gBrowserSub->cursorY -= (s16)(speed.v[gBrowserWork->stickTilt] * cosf(angle));
        if (gBrowserSub->listCount > 0) {
            /* The open list's object is read as the array's address plus
             * the byte offset: indexed as obj[activeObj], the compiler
             * adds the array's offset at another point than the game's
             * code does. */
            listObj = *(BrowserObj **)((u32)gBrowserWork->obj + gBrowserSub->activeObj * 4);
            x = listObj->x - gBrowserState->x;
            if (gBrowserSub->cursorX < 10.0f + x) {
                gBrowserSub->cursorX = 10.0f + x;
            } else if (gBrowserSub->cursorX > listObj->w + x - 10.0f) {
                gBrowserSub->cursorX = listObj->w + x - 10.0f;
            }
            rows = gBrowserSub->listCount > DROPLIST_ROWS ? DROPLIST_ROWS : gBrowserSub->listCount;
            if (gBrowserSub->listUp != 0) {
                y = gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y;
                if (gBrowserSub->cursorY < y - (rows * DROPLIST_ROW_H - 9)) {
                    gBrowserSub->cursorY = y - (rows * DROPLIST_ROW_H - 9);
                } else if (gBrowserSub->cursorY > y - 9.0f) {
                    gBrowserSub->cursorY = y - 9.0f;
                }
            } else {
                y = gBrowserWork->obj[gBrowserSub->activeObj]->y - gBrowserState->y;
                if (gBrowserSub->cursorY < 9.0f + (20.0f + y)) {
                    gBrowserSub->cursorY = 9.0f + (20.0f + y);
                } else if (gBrowserSub->cursorY > 20.0f + y + (rows * DROPLIST_ROW_H - 9)) {
                    gBrowserSub->cursorY = 20.0f + y + (rows * DROPLIST_ROW_H - 9);
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
    } else if (gKbdHeldKey == USB_KEY_ESCAPE && gBrowserSub->listCount > 0) {
        Select_Close();
    } else if (gKbd->usbKey == USB_KEY_TAB && gBrowserSub->listCount == 0) {
        goto next;
    } else if (gKbd->usbKey == USB_KEY_PAGE_UP && gBrowserSub->listCount > 0) {
        if (gBrowserSub->listCount <= DROPLIST_ROWS || gBrowserSub->listTop < DROPLIST_ROWS) {
            gBrowserSub->listTop = 1;
        } else {
            gBrowserSub->listTop -= DROPLIST_ROWS - 1;
        }
    } else if (gKbd->usbKey == USB_KEY_PAGE_DOWN && gBrowserSub->listCount > 0) {
        if (gBrowserSub->listTop + (DROPLIST_ROWS - 1) < gBrowserSub->listCount) {
            gBrowserSub->listTop += DROPLIST_ROWS - 1;
        }
    } else if (gKbdHeldKey >= USB_KEY_RIGHT && gKbdHeldKey < USB_KEY_UP + 1) {
        switch (gKbdHeldKey) {
        case USB_KEY_DOWN:
        case USB_KEY_RIGHT:
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
        case USB_KEY_UP:
        case USB_KEY_LEFT:
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
    } else if (((pressed = gBrowserWork->padPressed) & PAD_CROSS) && gBrowserSub->listCount > 0) {
        Select_Close();
    } else if (((held = gBrowserWork->padHeld) & PAD_RIGHT) || (held & PAD_DOWN)) {
        if (((pressed & PAD_RIGHT) || (pressed & PAD_DOWN)) && gBrowserSub->listCount > 0) {
        down:
            Select_Down();
        } else {
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || ((pressed = gBrowserWork->padPressed) & PAD_RIGHT) || (pressed & PAD_DOWN)) {
                if (gBrowserSub->activeObj != 0 && gBrowserSub->listCount > 0) {
                    Select_Down();
                } else {
                next:
                    for (i = gBrowserWork->focus + 1; i < PAGE_OBJ_COUNT; i++) {
                        if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
                            if (gBrowserState->style & PAGE_STYLE_POINTER) {
                                for (i = gBrowserWork->focus; i < PAGE_OBJ_COUNT; i++) {
                                    if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0 || PageObj_Activate(i, 1)) {
                                        return;
                                    }
                                }
                                return;
                            } else {
                                for (i = 0; i < PAGE_OBJ_COUNT; i++) {
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
    } else if ((held & PAD_LEFT) || (held & PAD_UP)) {
        if (((pressed & PAD_LEFT) || (pressed & PAD_UP)) && gBrowserSub->listCount > 0) {
        up:
            Select_Up();
        } else {
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || ((pressed = gBrowserWork->padPressed) & PAD_LEFT) || (pressed & PAD_UP)) {
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
                        for (i = 0; i < PAGE_OBJ_COUNT; i++) {
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

/* The pointer's speeds (.data). */
StickSpeeds gPageStickSpeeds = {
#include "lobby/007EEBE0.inc"
};
