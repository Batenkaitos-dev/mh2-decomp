/*
 * The chat's sending, with two small helpers among it:
 * - Town_IsMode: whether the town runs a mode (gTownMode, TOWN_MODE_*).
 * - Chat_SendToTargets sends a line to the members whose bits are set in
 *   gChatRecipients (their ids copied from gNetUsers into gChatTargets),
 *   or to everyone; Chat_SendToLobby sends to the whole lobby (sub_main's
 *   chat input calls it for CHAT_SEND_LOBBY); VillageClock_DrawChatMark
 *   draws the chat mark beside the village clock.
 * - QuestParty_UserMask: the gNetUsers bits of the quest room's other
 *   players (LobbyState.players), which sub_main's chat sends to.
 * - Quest_SetInfo: the quest taken, its time left and its reward.
 * The file has no data of its own.
 */

#include "common.h"
#include "em.h"
#include "plsel.h"
#include "lobby_globals.h"
#include "syssave.h"
#include "area.h"
#include "render_state.h"
#include "lobby_menu.h"
#include "texture.h"
#include "text.h"
#include "chat_send.h"

void Gfx_DrawMarker(s32 x, s32 y, s32 w, s32 h, s32 flags); /* unit's own declaration: int parameters, though the function takes shorts: VillageClock_DrawChatMark passes its int x and y on without narrowing them, as the game's code does */
s32 Town_IsMode(s32 arg0);
extern s32 gTownMode;
void Chat_SendLogText(s32 unused, s32 text);

s32 Town_IsMode(s32 mode) {
    return gTownMode == mode;
}

void Chat_SendText(s32, void *text, s32); /* unit's own declaration: an int colour, though the function takes a u8: Chat_SendToTargets passes its colour on without narrowing it, as the game's code does */
char *strcpy(char *dst, const char *src);
extern NetUser gNetUsers[LOBBY_MEMBER_COUNT];
extern char gChatTargets[LOBBY_CHAT_TARGETS][LOBBY_USER_ID_LEN];

extern u16 gChatRecipients;
/* Sends chat line text from sender, in colour color (Chat_SendText): to
 * everyone while gChatToAll is set, else only to the members whose bits
 * are set in gChatRecipients, the local player left out, whose ids are
 * first copied into gChatTargets (LobbyState.chatToTargets set). */
void Chat_SendToTargets(s32 sender, void *text, s32 color)
{
    s32 i;
    s32 recipientBit;
    NetUser *member;
    char *target;

    if (gChatToAll != 0) {
        gLobby->chatToTargets = 0;
    } else {
        recipientBit = 1;
        i = 0;
        member = gNetUsers;
        target = gChatTargets[0];
        gLobby->chatToTargets = 1;
        gChatTargets[0][0] = 0;
        gChatTargets[1][0] = 0;
        gChatTargets[2][0] = 0;
        gChatTargets[3][0] = 0;
        gChatTargets[4][0] = 0;
        gChatTargets[5][0] = 0;
        gChatTargets[6][0] = 0;
        do {
            if ((i != gScene.localPlayer) && (gChatRecipients & recipientBit)) {
                strcpy(target, member->id);
                target += LOBBY_USER_ID_LEN;
            }
            i += 1;
            member++;
            recipientBit += recipientBit;
        } while (i < LOBBY_MEMBER_COUNT);
    }
    Chat_SendText(sender, text, color);
}

/* Sends chat line text from sender to the town's chat log
 * (Chat_SendLogText): sub_main's chat input calls it when the line goes to
 * CHAT_SEND_LOBBY. */
void Chat_SendToLobby(s32 sender, s32 text) {
    Chat_SendLogText(sender, text);
}

/* Draws the chat mark at (x, y) beside the village clock (not in the
 * lobby's AREA_ONLINE_VILLAGE): the window marker with a blinking cue, in colour
 * 1 while the chat window is open with lines going to the lobby, or in
 * colour 0 while the chat log is shown with the chat beep off and lines
 * going elsewhere. */
void VillageClock_DrawChatMark(s32 x, s32 y)
{
    s32 flags;

    flags = 0;
    if ((gScene.inLobby == 0) || (gScene.area != AREA_ONLINE_VILLAGE)) {
        if (gChatSendTo == CHAT_SEND_LOBBY) {
            if (gChatWinOpen != 0) {
                flags = MARKER_CUE | 1;
            }
        } else if ((gSysSave.options.chatBeep == 0) && (gChatLog.win.open != 0)) {
            flags = MARKER_CUE;
        }
        if (flags != 0) {
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
            Tex_Prepare(1, TEX_SLOT_ICONS);
            Tex_Bind(TEX_SLOT_ICONS);
            Gfx_DrawMarker(x, y, 20, 18, flags);
        }
    }
}

extern char gMyUserId[];
s32 strcmp(char *a, char *b);

/* The bits (1 << the member's index in gNetUsers) of the quest room's
 * players (LobbyState.players) other than the local user (gMyUserId);
 * 0 while no quest is taken. sub_main's chat sends a line to them
 * (gChatRecipients). off is the player's byte offset (LOBBY_PLAYER_AT). */
u16 QuestParty_UserMask(void)
{
    u32 j;
    u32 i;
    u16 mask;
    s32 off;
    NetUser *member;

    if (gLobby->questTaken == 0) {
        return 0;
    }
    mask = 0;
    for (i = 0, off = 0; i < LOBBY_ROOM_PLAYERS; i++, off += sizeof(NetUser)) {
        LobbyState *base = (LobbyState *)((u8 *)gLobby + off);
        if (*(s8 *)base->players[0].id != 0 && strcmp(base->players[0].id, gMyUserId) != 0) {
            for (j = 0, member = gNetUsers; j < LOBBY_MEMBER_COUNT; j++, member++) {
                if (strcmp(LOBBY_PLAYER_AT(off).id, member->id) == 0) {
                    mask += (u16)(1 << j);
                    break;
                }
            }
        }
    }
    return mask;
}

/* Makes quest the quest taken (gQuestInfo), its time left and its reward
 * starting from its time limit and reward. */
void Quest_SetInfo(QuestInfo *quest) {
    gQuestInfo = quest;
    gQuestTimeLeft = quest->timeLimit;
    gQuestReward = quest->reward;
}
