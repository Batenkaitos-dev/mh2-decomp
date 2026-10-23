/*
 * The on-screen keyboard's small helpers, then the chat's targets and box.
 *
 * The helpers: an empty function the keyboard's set-up calls (Kbd_EmptyInit), the binding of
 * texture 328 for the keyboard (Kbd_BindGlyphTex), a blend setting the keyboard and the HUD
 * use (Gfx_SetBlendAlphaAdd) and the flag that makes the keyboard active at any key
 * (Kbd_SetStartActive, Kbd_GetStartActive).
 *
 * The chat's targets and box: the command that sends a chat line to one player cycles through
 * the players by gChatTarget (0 none, 1 the quest's party online, 2 + n player n), and the
 * recipients' bits are set from it (Chat_NextTarget, Chat_SetRecipients). Chat_DrawBox draws
 * the chat box by the send target, Chat_DrawKbdBox the frame of the on-screen keyboard.
 */
#include "common.h"
#include "sprite.h"
#include "chat_log.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "syssettings.h"
#include "objhead.h"
#include "objattach.h"
#include "obj.h"
#include "window.h"
#include "render_state.h"
#include "area.h"
#include "hud.h"
#include "adx.h"
#include "sound.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "lobby_state.h"
#include "area_draw.h"
#include "kbd.h"
#include "save.h"
#include "text.h"
#include "model.h"
#include "village_cond.h"
#include "chat_box.h"
#include "chat_send.h"
#include "game_util.h"
#include "lobby_queries.h"

extern s8 gKbdStartActive;
extern u8 gChatHudColor;
extern u8 gChatLogHead;
extern u8 gChatLogCount;
extern u8 gChatLogScroll;
extern u8 gChatLogAtEnd;
extern s8 gChatTarget;
extern s8 gChatWinState;
extern s8 gChatWinHold;
extern u8 gChatHideNames;           /* nonzero: names from gPlayerIds */
extern u16 gChatRecipients;          /* chat recipients, one bit per player */
extern u8 gChatToAll;
extern u8 gChatMsgClass;
extern s8 gChatTargetId[];         /* the recipient's name */
extern s8 gPlayerIds[][8];      /* per player */
extern s8 *gChatTargetFmt;          /* the format of a player target */

/* The network members' rows (NetUser) as bytes, as the lobby's code declares and reads them;
 * Chat_NextTarget and Chat_SetRecipients read a row's fields through a NetUser cast.
 * unit's own declaration: u8[] rather than the definition's NetUser rows */
extern u8 gNetUsers[];

void sprintf(s8 *out, s8 *fmt, ...);
char *strcpy(void *dst, const void *src);
s32 strcmp(s8 *a, s8 *b);
extern ListPanel gChatBoxPanels[];
extern s32 gChatTypeTexts[];

#define OBJ_POOL_SIZE       128     /* gObjPool's objects */
#define OBJ_ATTACH_POOL_SIZE 64     /* gObjAttachPool's */

struct MtxSlot;

struct Frame;
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);

/* An empty function Kbd_InitState calls. */
void Kbd_EmptyInit(void)
{
}

/* Prepares and binds texture 328. */
void Kbd_BindGlyphTex(void)
{
    Tex_Prepare(1, 328);
    Tex_Bind(328);
}

/* Sets the blend: additive by the alpha when mode is 1, else the usual alpha blend. */
void Gfx_SetBlendAlphaAdd(s32 mode)
{
    if (mode != 1) {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    } else {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
    }
}

/* When set, any key makes the software keyboard active. */
void Kbd_SetStartActive(u8 active)
{
    gKbdStartActive = active;
}

s32 Kbd_GetStartActive(void)
{
    return gKbdStartActive;
}

/*
 * Draws the on-screen keyboard with a frame of colour color around it: the
 * whole keyboard while its keys are shown, else only its text line.
 */
void Chat_DrawKbdBox(s32 color)
{
    f32 kbdX;
    Frame frame;

    Kbd_GetPos(&kbdX, &frame.y);
    if (Kbd_KeysShown() != 0) {
        frame.x = kbdX;
        frame.y -= 3;
        frame.w = 488;
        frame.h = 102;
    } else {
        frame.x = (s16)kbdX - 56;
        frame.y -= 3;
        frame.w = 544;
        frame.h = 26;
    }
    frame.style = FRAME_FILL | 3;
    frame.color = color;
    frame.color2 = 0;
    WindowFrame_Draw(&frame, 0);
    Kbd_Draw(gSysSettings.unk31);
}

/* Sets the chat's target (gChatTarget): -1 off, 0 none, 1 the party, 2 + n player n. */
void Chat_SetTarget(s8 target)
{
    gChatTarget = target;
}

/*
 * Gives the chat's target as a text in out and returns its kind: 0 none
 * (out empty), 3 the quest's party (in the town: target 1), 5 a player (target
 * 2 + its index). With next set, it first moves on to the next target:
 * after the party come the active players other than the local one (in a
 * drill with every player local, only those in the quest; in the town, only
 * those in an area), then none again. A player's text is gChatTargetFmt
 * with its name, or its id while gChatHideNames is set, and its id goes
 * to gChatTargetId for Chat_SetRecipients. The quest's case jumps to the
 * town's case's tail (found), which the game's code shares.
 * In the town, a member's id is read as bytes of the row (gNetUsers + i * 284)
 * and its name through the row as a NetUser: through NetUser alone the
 * compiler folds the name's offset into the array's address, which the
 * game's code doesn't.
 */
s32 Chat_NextTarget(s8 *out, s32 next)
{
    u32 i;
    u32 max;
    Player *pl;

    if (gChatTarget < 0) {
        return -1;
    }
    if ((u8)next) {
        gChatTarget++;
    }
    if (gChatTarget == 0) {
        out[0] = 0;
        return 0;
    }
    if (gChatTarget == 1) {
        if (gScene.inLobby != 0) {
            strcpy(out, gChatTargetTexts[0]);
            return 3;
        }
        gChatTarget++;
    }
    if (gScene.inLobby == 0) {
        max = 4;
    } else {
        max = 10;
    }
    i = gChatTarget - 2;
    while (1) {
        if (i == gScene.localPlayer) {
            i++;
        }
        if (i >= max) {
            gChatTarget = 0;
            out[0] = 0;
            return 0;
        }
        pl = &gPlayers[i];
        if (pl->active != 0) {
            if (gScene.inLobby == 0) {
                if (Quest_IsDrill() != 1 || gScene.allLocal == 0 || gScene.inQuest[i] == 1) {
                    if (gChatHideNames != 0) {
                        sprintf(out, gChatTargetFmt, gPlayerIds[i]);
                    } else {
                        sprintf(out, gChatTargetFmt, pl->name);
                    }
                    strcpy(gChatTargetId, gPlayerIds[i]);
                    goto found;
                }
            } else {
                if (pl->area == 0) {
                    i++;
                    continue;
                }
                if (gChatHideNames != 0) {
                    sprintf(out, gChatTargetFmt, gNetUsers + i * 284);
                } else {
                    sprintf(out, gChatTargetFmt, ((NetUser *)gNetUsers)[i].name);
                }
                strcpy(gChatTargetId, gNetUsers + i * 284);
            found:
                gChatTarget = i + 2;
                return 5;
            }
        }
        i++;
    }
}

/*
 * Sets the recipients of the chat line msg from the chat's target:
 * everyone (gChatRecipients CHAT_RECIPIENTS_ALL, CHAT_CLASS_ALL) when there is none, the
 * party but the local player (class 2) for target 1, else the one player
 * (class 1), if it is still the one gChatTargetId names. Returns 0 for
 * everyone, 1 for the party, the target for a player, -1 when the target
 * is gone and -2 for a command line ("/" with a ":" after it).
 */
s32 Chat_SetRecipients(s8 *msg)
{
    u8 i;
    u8 idx;

    if (msg[0] == '/') {
        for (i = 1; msg[i] != 0;) {
            if (msg[i++] == ':') {
                return -2;
            }
        }
    }
    if (gChatTarget <= 0) {
        gChatRecipients = CHAT_RECIPIENTS_ALL;
        gChatMsgClass = CHAT_CLASS_ALL;
        gChatToAll = 1;
        return 0;
    }
    gChatToAll = 0;
    if (gChatTarget == 1) {
        if (gScene.inLobby != 0) {
            if ((gChatRecipients = QuestParty_UserMask()) != 0) {
                gChatRecipients &= 0xFFFF - (1 << gScene.localPlayer);
                gChatMsgClass = CHAT_CLASS_PARTY;
                return 1;
            }
        }
        return -1;
    }
    idx = gChatTarget - 2;
    if (gScene.inLobby == 0) {
        if (strcmp(gChatTargetId, gPlayerIds[idx]) != 0) {
            return -1;
        }
    } else {
        if (NetUser_StateByName((u8 *)((NetUser *)gNetUsers)[idx].id) != 0) {
            return -1;
        }
    }
    gChatMsgClass = CHAT_CLASS_PLAYER;
    gChatRecipients = 1 << idx;
    return gChatTarget;
}

/* Sets the chat window's state and hold (gChatWinState, gChatWinHold). */
void Chat_SetWindowMode(s8 state, s8 hold)
{
    gChatWinState = state;
    gChatWinHold = hold;
}

/* Empties the chat log and puts its view at the end. */
void Chat_ClearLog(void)
{
    gChatLogHead = 0;
    gChatLogCount = 0;
    gChatLogAtEnd = 1;
    gChatLogScroll = 0;
}

/* The screen rows of the chat log's lines: each line's rows, and one more
 * for the name of a line that has one. */
s32 Chat_CountLogRows(void)
{
    s32 rows = 0;
    s32 i = gChatLogHead - 1;
    s32 n = gChatLogCount;

    for (; n > 0; n--, i--) {
        ChatLine *line = &gChat.lines[i & 0x3F];
        rows += line->rows;
        if (line->id[0] != 0) {
            rows++;
        }
    }
    return rows;
}

/*
 * Draws the chat box for the send target sendTo (CHAT_SEND_*) at chat size
 * size (gChatBoxPanels): for CHAT_SEND_LOBBY a plain box; for the others
 * the title (title, or gChatTypeTexts[sendTo] when it is 0), and for
 * CHAT_SEND_TARGETS and CHAT_SEND_PARTY a marker at the box's corner, as
 * large as the size's text.
 */
void Chat_DrawBox(s32 sendTo, u8 size, s32 title)
{
    ListPanel *panel = &gChatBoxPanels[size];
    u8 marker;

    panel->flags = WINDOW_FILL | 3;
    marker = 0;
    switch (sendTo) {
    case CHAT_SEND_LOBBY:
        panel->flags = WINDOW_FILL | 2;
        break;
    case CHAT_SEND_TARGETS:
    case CHAT_SEND_PARTY:
        marker = 1;
    case CHAT_SEND_LOCAL:
        if (title == 0) {
            title = gChatTypeTexts[sendTo];
        }
        break;
    }
    ListPanel_Draw(panel, (s16 *)0, (char *)title, -1);
    if (marker) {
        size = gChatTextSizes[size];
        Gfx_DrawMarker(panel->x, panel->y, size, size * 18 / 21, 1);
    }
}

/* Sets the chat text's colour on the HUD: a negative color clears it. */
void Chat_SetColor(s16 color)
{
    if (color < 0) {
        gChatHudColor = 0;
    } else {
        gChatHudColor = color | 0x80;
    }
}

/* The file's data: the chat boxes. */
ListPanel gChatBoxPanels[3] = {
#include "sub_main/0043B0E0.inc"
};
