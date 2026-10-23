#ifndef CHAT_BOX_H
#define CHAT_BOX_H

#include "common.h"

/*
 * The on-screen keyboard's small helpers (Kbd_EmptyInit, Kbd_BindGlyphTex,
 * Kbd_SetStartActive, Kbd_GetStartActive, and Gfx_SetBlendAlphaAdd, a blend
 * setting the keyboard and the HUD use), then the chat's targets and box:
 * the player a chat line is sent to (Chat_SetTarget, stepped by
 * Chat_NextTarget) and the recipients set from it (Chat_SetRecipients); the
 * chat window's mode, log and colour (Chat_SetWindowMode, Chat_ClearLog,
 * Chat_CountLogRows, Chat_SetColor); and the draws of the chat box by its
 * send target (Chat_DrawBox) and of the on-screen keyboard's frame
 * (Chat_DrawKbdBox).
 */

void Kbd_EmptyInit(void);
void Kbd_BindGlyphTex(void);
void Gfx_SetBlendAlphaAdd(s32 mode);
void Kbd_SetStartActive(u8 active);
s32 Kbd_GetStartActive(void);
void Chat_DrawKbdBox(s32 color);
void Chat_SetTarget(s8 target);
s32 Chat_NextTarget(s8 *out, s32 next);
s32 Chat_SetRecipients(s8 *msg);
void Chat_SetWindowMode(s8 state, s8 hold);
void Chat_ClearLog(void);
s32 Chat_CountLogRows(void);
void Chat_DrawBox(s32 sendTo, u8 size, s32 title);
void Chat_SetColor(s16 color);

#endif
