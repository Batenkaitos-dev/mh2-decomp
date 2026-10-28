/*
 * LobbyMenu_AddHudLayers queues gLobbyMenu's layers 5-7 (the name tags and
 * the target banner, the members' names window, the village clock) on the
 * text draw lists each frame.
 * The town's name tags, drawn by gLobbyMenu's layer 5 (NameTags_DrawLayer,
 * its MenuTask draw): online, the label of the house the player visits
 * (MyHouse_DrawLabel); the names over the members' heads with their marks
 * from each member's HunterCard in LobbyState.members (NameTags_Draw: the
 * name coloured by HR, the weapon class's icon, the quest mark and the
 * symbol colour), the Poogie's name in the player's own house, and the
 * offer marks over the townspeople (NameTags_DrawNpcs). TownPc_DrawTargetBanner
 * draws the banner of what the action button acts on (an action spot, a
 * townsperson, a member) with its button prompts; the members' names
 * window (LobbyMenu_DrawMemberNames) calls it first.
 * Then the small draws the lobby's menus share: a pulsing gradient bar
 * (Gfx_DrawPulseGradient), the button icons of texture TEX_SLOT_ICONS
 * (Lobby_DrawButtonIcon), the icon sheet's weapon-class and online-mark icons
 * (Icon_DrawWeaponClass, Icon_DrawOnlineMark), an equipment piece's icon
 * (Lobby_DrawEquipIcon) and the square sprites of gLobbySpriteUVs
 * (LobbySprite_Draw).
 * The .data holds the quest categories' colours, the menu sheets' label
 * icons and strip tables (used by the menu sheets) and the two sprite
 * corner tables; the .rodata the format strings and the Poogie's default
 * name.
 */
#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "sprite.h"
#include "lobby_state.h"
#include "quest.h"
#include "prim.h"
#include "lobby_globals.h"
#include "area.h"
#include "lobby_labels.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "render_state.h"
#include "card.h"
#include "lobby_forge.h"
#include "text.h"
#include "name_tags.h"
#include "menu_sheets.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "my_house.h"
#include "place_info.h"
#include "town_player_states.h"
#include "town_players.h"
#include "village_records.h"

extern LobbyMenu *gLobbyMenu;

/* Queues gLobbyMenu's layers 5-7 on the text draw lists: layer 5 (the house
 * label and the name tags) on gDrawListText6, layer 6 (the members' names
 * window) on gDrawListText0b, layer 7 (the village clock) on
 * gDrawListText0a. */
void LobbyMenu_AddHudLayers(void)
{
    OT_AddAt(gDrawListText6, gLobbyMenu->layer[5], 0, 1);
    OT_AddAt(gDrawListText0b, gLobbyMenu->layer[6], 0, 1);
    OT_AddAt(gDrawListText0a, gLobbyMenu->layer[7], 0, 1);
}

/* The button icons and the square sprites are drawn as a TexRect (the
 * layout Sprite_Draw takes): Lobby_DrawButtonIcon and LobbySprite_Draw copy
 * each corner whole from their tables (gButtonIconUVs, gLobbySpriteUVs). */
extern IconUV gLobbySpriteUVs[];
struct Sprite;

void Win_Draw(s32 x, s32 y, s32 w, s32 h, s32 color, s32 color2, s16 kind);
extern char gHouseCloseFmt[];
extern char gLabelNameFmt[];
extern s32 gTownMode;
extern s16 gHouseCloseTimer;
extern NetUser gNetUsers[];

/* Online, in another member's house (AREA_ONLINE_HOUSE_0-2 and not the
 * player's own) and not over the records menu: a window at the top right
 * with the closing countdown in seconds while gHouseCloseTimer runs
 * (frames, 30 a second), else the house owner's name (the member
 * MyHouse_LocalSlot gives; its user id while names are hidden,
 * gChatHideNames) after the house label. */
void MyHouse_DrawLabel(void)
{
    s8 slot = MyHouse_LocalSlot();

    if (Game_IsOnline() == 0) {
        return;
    }
    switch (gScene.area) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        if (MyHouse_InOwnHouse() == 1) {
            return;
        }
        break;
    default:
        return;
    }
    if (gTownMode == TOWN_MODE_RECORDS) {
        return;
    }
    Win_Draw(400, 24, 210, 22, 0xBDB59F6C, 0x80202020, 6);
    Font_SetPos(400, 24);
    Font_SetSpacing(20, 20);
    if (gHouseCloseTimer != -1) {
        Font_SetColor(0x807F7F28);
        Text_PrintfColored(gHouseCloseFmt, gTownLabels[TOWN_LABEL_CLOSE_ASK], gHouseCloseTimer / 30 + 1);
        return;
    }
    if (slot != -1) {
        Font_SetColor(0x807F7F7F);
        if (gChatHideNames != 0) {
            Text_PrintfColored(gLabelNameFmt, gTownLabels[TOWN_LABEL_MY_HOUSE], gNetUsers[slot].id);
        } else {
            Text_PrintfColored(gLabelNameFmt, gTownLabels[TOWN_LABEL_MY_HOUSE], gNetUsers[slot].name);
        }
    }
}

s32 Kbd_IsOpen(void); /* unit's own declaration: returning s32, though the keyboard's
 * definition returns s8: NameTags_Draw tests the whole return register, as the game's
 * code does; with s8 the compiler would sign-extend it first */

/* gLobbyMenu's layer 5 draw (t its MenuTask, for the text size): in the
 * modes that leave the town in view, the house label and the name tags;
 * with the chat keyboard open, the house label alone. In the game menu
 * the label is drawn and, with the keyboard closed, the case falls through
 * and draws it a second time with the name tags, as the game's code does.
 * Mode 5 has no name yet. */
void NameTags_DrawLayer(MenuTask *task)
{
    Text_SetSize(task->textSize);
    switch (gTownMode) {
    case TOWN_MODE_KEYBOARD:
        MyHouse_DrawLabel();
        break;
    case TOWN_MODE_GAME_MENU:
        MyHouse_DrawLabel();
        if (Kbd_IsOpen() != 0) {
            break;
        }
    case TOWN_MODE_ITEM_SHOP:
    case TOWN_MODE_SET_OUT:
    case TOWN_MODE_DEPART:
    case TOWN_MODE_DEPART_FADE:
    case TOWN_MODE_QUEST_DESK:
    case TOWN_MODE_DOJO:
    case TOWN_MODE_ARENA_CUP:
    case TOWN_MODE_QUEST_COUNTER:
    case TOWN_MODE_SAVE:
    case TOWN_MODE_TALK:
    case 5:
    case TOWN_MODE_BAIT_MENU:
    case TOWN_MODE_PRICE_INFO:
    case TOWN_MODE_VILLAGE_BOARD:
    case TOWN_MODE_OFFER_MENU:
    case TOWN_MODE_SEAT_WAIT:
    case TOWN_MODE_CONTEST_SEAT_WAIT:
    case TOWN_MODE_CONTEST_SEATED:
    case TOWN_MODE_INGREDIENTS:
    case TOWN_MODE_KITCHEN:
    case TOWN_MODE_MINE:
    case TOWN_MODE_RECORDS:
    case TOWN_MODE_FREE:
        MyHouse_DrawLabel();
        NameTags_Draw(task);
        break;
    }
}

void Icon_Draw(); /* unit's own declaration: without parameters, so that NameTags_Draw passes the values
 * of its (s16) casts as whole ints, as the game's code does; a prototype with s32 or
 * s16 parameters makes the compiler convert them differently */

void Icon_DrawWeaponClass(s32, s32, s32, s32, s16, s8);
u8 Equip_GetClass(EquipItem *e);
extern s32 gPlayerColorWords[];

/* Layer 5's name tags (t the layer's MenuTask, for the text size), in
 * TOWN_MODE_FREE, _SET_OUT, _SEAT_WAIT, and the game menu with the
 * keyboard closed. Over each member in the player's area (NET_USER_HERE,
 * not at the origin, not in TOWN_PC_SIT_DOWN or TOWN_PC_STAND_UP): the
 * name (its user id while names are hidden online), coloured by the hunter
 * card's HR (51 and up, 31 and up, below), 190 above the feet or 40 above
 * bone 20 when seated; before it the weapon class's icon (the local
 * player's from the save); above its middle the quest mark in its category's
 * colour (LOBBY_SPRITE_QUEST_PAID, or LOBBY_SPRITE_QUEST_TAKEN, which
 * blinks once QUEST_STATUS_SET_OUT is set); after it the symbol colour's
 * square (online the card's, offline the local player's once a style is
 * earned; colour 0xB cycles). Then, in the player's own house, the
 * Poogie's name (SaveData.poogieName) over NPC_POOGIE, and the offer marks
 * (NameTags_DrawNpcs). The Poogie's name is centred by the last member's
 * name length (len), as the game's code does. */
void NameTags_Draw(MenuTask *task)
{
    f32 pos[4];
    f32 scr[4];
    f32 mtx[16];
    s16 nameLen;
    s16 tagX;
    SaveData *save;
    s32 memberOff;
    s16 i;
    Player *pl;
    NetUser *user;
    char *name;
    s16 tagY;
    s16 nameColor;
    HunterCard *card;
    s32 symbolColor;
    Em *em;
    s8 weaponClass;

    save = &gSave;
    switch (gTownMode) {
    case TOWN_MODE_GAME_MENU:
        if (Kbd_IsOpen() != 0) {
            return;
        }
    case TOWN_MODE_FREE:
    case TOWN_MODE_SET_OUT:
    case TOWN_MODE_SEAT_WAIT:
        break;
    default:
        return;
    }
    {
        Text_SetSize(task->textSize);
        Tex_Prepare(1, 341);
        Tex_Bind(341);
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Gs_SetState(RS_ALPHA_REF, 0);
        i = 0;
        pl = gPlayers;
        user = gNetUsers;
        memberOff = 0;
        do {
            if (pl->active != 0 && pl->visible != 0 && (u8)TownPc_IsInMyArea(pl) != 0 && NetUser_State(i) == NET_USER_HERE &&
                (pl->pos[0] != 0.0f || pl->pos[2] != 0.0f) &&
                (pl->stateGroup != TOWN_PC_GROUP_ACTION || (u32)(pl->state - TOWN_PC_SIT_DOWN) > 1)) {
                /* not TOWN_PC_SIT_DOWN or TOWN_PC_STAND_UP */
                name = user->name;
                if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                    name = user->id;
                }
                nameLen = Lobby_StrLength(name);
                Mtx_Identity(mtx);
                Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 0) {
                    pos[0] = pl->pos[0];
                    pos[1] = 190.0f + pl->pos[1];
                    pos[2] = pl->pos[2];
                } else {
                    Em_GetBonePos(pl, 20, pos);
                    pos[1] += 40.0f;
                }
                pos[1] += Player_NameTagRaise(pl);
                Camera_ProjectPoint(scr, pos);
                if (scr[0] < 700.0f && scr[0] > -60.0f && scr[1] < 500.0f && scr[1] > -20.0f && scr[3] > 0.0f) {
                    scr[0] -= (f32)(nameLen / 2 * 8);
                    tagX = 1.25f * scr[0];
                    tagY = scr[1];
                    Font_SetSpacing(16, 16);
                    card = LOBBY_MEMBER_CARD_AT(memberOff);
                    if (card->hr >= 51) {
                        nameColor = 2;
                    } else if (card->hr >= 31) {
                        nameColor = 6;
                    } else {
                        nameColor = 5;
                    }
                    Text_DrawShadow(1.25f * (s32)scr[0], scr[1], 1, nameColor, name);
                    if (i == gScene.localPlayer) {
                        weaponClass = Equip_GetClass(gSave.equip);
                        Icon_DrawWeaponClass((s16)(tagX - 22), (s16)(tagY - 4), 22, -1, weaponClass, 0);
                    } else {
                        Icon_DrawWeaponClass((s16)(tagX - 22), (s16)(tagY - 4), 22, -1, card->weaponClass, 0);
                    }
                    if (card->questStatus & QUEST_STATUS_PAID) {
                        LobbySprite_Draw((s16)(tagX + nameLen * 4 - 12), tagY - 26, 22, gQuestCategoryColors[card->questStatus & QUEST_STATUS_CATEGORY], LOBBY_SPRITE_QUEST_PAID);
                    } else if (card->questStatus & QUEST_STATUS_TAKEN) {
                        if (!(card->questStatus & QUEST_STATUS_SET_OUT) || (gFrameTick & 0x10)) {
                            LobbySprite_Draw((s16)(tagX + nameLen * 4 - 12), tagY - 26, 22, gQuestCategoryColors[card->questStatus & QUEST_STATUS_CATEGORY], LOBBY_SPRITE_QUEST_TAKEN);
                        }
                    }
                    if (Game_IsOnline() == 1) {
                        symbolColor = card->symbolColor;
                    } else if (gSave.styleBits == 0) {
                        symbolColor = 255;
                    } else {
                        symbolColor = gPlayers[gScene.localPlayer].style;
                    }
                    if (symbolColor != 255) {
                        if (symbolColor == SYMBOL_COLOR_CYCLE) {
                            Icon_Draw((s16)(tagX + nameLen * 8 + 6), (s16)(tagY - 4), 20, 20, Color_Cycle8(), ICON_SYMBOL_COLOR);
                        } else {
                            Icon_Draw((s16)(tagX + nameLen * 8 + 6), (s16)(tagY - 4), 20, 20, gPlayerColorWords[symbolColor], ICON_SYMBOL_COLOR);
                        }
                    }
                }
            }
            pl++;
            memberOff += 284;
            i++;
            user++;
        } while (i < 10);
        em = gEms;
        i = 0;
        do {
            if (em->active != 0 && em->work == NPC_POOGIE && (s8)save->poogieName[0] != 0 && MyHouse_InOwnHouse() == 1) {
                Em_BonePos(em, 10, pos);
                pos[1] += 50.0f;
                Camera_ProjectPoint(scr, pos);
                if (scr[0] < 700.0f && scr[0] > -60.0f && scr[1] < 500.0f && scr[1] > -20.0f && scr[3] > 0.0f) {
                    scr[0] -= (f32)(nameLen / 2 * 8);
                    tagX = 1.25f * scr[0];
                    tagY = scr[1];
                    Font_SetSpacing(16, 16);
                    Text_DrawShadow(tagX, tagY, 1, 5, (char *)save->poogieName);
                }
            }
            i++;
            em++;
        } while (i < 20);
        NameTags_DrawNpcs();
    }
}

void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: with s32 parameters, though the icon sheet's Icon_Draw takes s16 coordinates and sizes and a u32 colour: NameTags_DrawNpcs passes its values without narrowing them, as the game's code does */
s32 Npc_IsFree(Em *arg0);
u8 Area_GetSetIndex(u16 id);

extern u16 gFrameTick; /* sub_main: the frame counter */

/* The offer marks (ICON_OFFER_MARK) over the townspeople with an offer
 * (NpcWork.offerMark) or a village gift ready (Village_HasGift): placed at
 * a bone picked by what the townsperson is doing, and held still while
 * that bone moves less than 20 from last frame's point (NpcWork.markAnchor). In
 * the house's areas (area list indices 10-14: AREA_HUB_0x57,
 * AREA_ONLINE_HOUSE_0-2, AREA_HOUSE) the mark sits lower and its distances
 * shrink to 0.7. The mark pulses in size every 16 frames, fades out from
 * 400 to 1000 away from the local player, and takes one colour for a
 * ready offer or gift and another for an offer.
 * The frame counter is read as the named object gFrameTick: the game loads
 * it above the store of scr[0], which the compiler does only for a named
 * object. */
void NameTags_DrawNpcs(void)
{
    f32 markPos[4];
    f32 delta[4];
    f32 bone[4];
    f32 scr[4];
    f32 mtx[16];
    Player *localPl;
    Em *em;
    NpcWork *w;
    s16 i;
    u32 alpha;
    s32 markW;
    s32 markH;
    f32 scale;
    f32 dist;
    f32 fadeEnd;
    u32 color;

    em = gEms;
    i = 0;
    localPl = &gPlayers[gScene.localPlayer];
    do {
        w = (NpcWork *)&em->work;
        if (em->active != 0 && ((s8)w->offerMark != 0 || Village_HasGift(w->id) != 0)) {
            Mtx_Identity(mtx);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            if (Npc_IsFree((Em *)((u8 *) em)) != 0) {
                Em_GetBonePos(em, 20, bone);
            } else if (Npc_IsAct51to55(em) != 0) {
                Em_GetBonePos(em, 10, bone);
            } else if (Npc_HasSecondHead((Em *)((u8 *) em)) != 0) {
                Em_GetBonePos(em, 12, bone);
            } else if (Npc_IsPet(em) != 0) {
                Em_GetBonePos(em, 10, bone);
                bone[1] += 40.0f;
            } else {
                bone[0] = em->posX;
                bone[1] = em->posY + 200.0f * em->scaleY;
                bone[2] = em->posZ;
            }
            delta[0] = w->markAnchor[0] - bone[0];
            delta[1] = w->markAnchor[1] - bone[1];
            delta[2] = w->markAnchor[2] - bone[2];
            if (Vec_Length(delta) < 20.0f) {
                Vec_Copy(markPos, w->markAnchor);
            } else {
                Vec_Copy(markPos, bone);
            }
            switch ((u8)Area_GetSetIndex(gScene.area)) {
            case 10:
            case 14:
            case 11:
            case 12:
            case 13:
                markPos[1] += 40.0f * em->scaleY;
                scale = 0.7f;
                break;
            default:
                markPos[1] += 70.0f * em->scaleY;
                scale = 1.0f;
                break;
            }
            Vec_Copy(w->markAnchor, bone);
            Camera_ProjectPoint(scr, markPos);
            if (scr[0] < 700.0f && !(scr[0] <= -60.0f) && scr[1] < 500.0f && !(scr[1] <= -20.0f) && !(scr[3] <= 0.0f)) {
                scr[0] *= 1.25f;
                if (gFrameTick & 0x10) {
                    markW = 26;
                    markH = 22;
                } else {
                    markW = 22;
                    markH = 20;
                }
                dist = Vec_Distance(localPl->pos, &em->posX);
                if (dist <= 400.0f * scale) {
                    alpha = 192;
                } else if (dist <= (fadeEnd = 1000.0f * scale)) {
                    alpha = 192.0f * ((fadeEnd - dist) / (600.0f * scale));
                } else {
                    alpha = 0;
                }
                if (Village_HasGift(w->id) == 1 || (s8)w->offerMark == 2) {
                    color = (alpha << 24) | 0xC08080;
                } else {
                    color = (alpha << 24) | 0x80C0C0;
                }
                Icon_Draw((s32)scr[0], (s32)scr[1], markW, markH, color, ICON_OFFER_MARK);
            }
        }
        i++;
        em++;
    } while (i < 20);
}

s32 sprintf(char *buf, char *fmt, ...);
extern char gPoogieDefaultName[];
extern s8 gTargetBannerCycle;
extern u8 gTargetBannerHidden;
extern u8 gLockOnNoCycle;

/* The banner at the left for what the action button acts on (Player.target),
 * in TOWN_MODE_FREE, _SET_OUT_PAID, _SET_OUT and _SEAT_WAIT. Nothing (and
 * gTargetBannerHidden set) while the player sits down or stands up, makes a
 * gesture, hands over an item, has no target or has the chat keyboard
 * open; when the banner comes back gTargetBannerCycle starts its 40-frame cycle
 * again. The target is the player itself, at an action spot: the spot's
 * action (gSpotActionTexts by TownSpotKind; seated, the stand-up text; at
 * the gate while setting out, until the quest is paid, the cancel-ready
 * text; online the save spot's own text; an area door names its
 * area, AREA_HUB_0x89's online door AREA_HUB_0x84, the doors between
 * AREA_HUB_0x85 and AREA_TAVERN their own text), on a green bar; a
 * townsperson that can be talked to: its name (the Poogie's default name
 * while it has none); another member: its name (its user id while names are
 * hidden) and the look prompt. Below, unless setting out, seated, or with a
 * single lock-on candidate (gLockOnNoCycle), a second bar with the
 * target-switch prompt. The circle icon and the left-right icon dip by 2
 * while their buttons are held. t is the members' names layer's MenuTask
 * (LobbyMenu_DrawMemberNames passes it), for the text size. */
void TownPc_DrawTargetBanner(MenuTask *task)
{
    char buf[64];
    Player *localPl = &gPlayers[gScene.localPlayer];
    u8 *save = (u8 *)&gSave;
    u8 *targetObj;
    ActionSpot *spot;
    NpcWork *w;
    u16 memberIdx;
    u32 color;

    switch (gTownMode) {
    case TOWN_MODE_SET_OUT:
    case TOWN_MODE_FREE:
    case TOWN_MODE_SET_OUT_PAID:
    case TOWN_MODE_SEAT_WAIT:
        break;
    default:
        return;
    }
    if (localPl->stateGroup != TOWN_PC_GROUP_ACTION || localPl->state != TOWN_PC_SIT_DOWN_SEATED) {
        if (TownPc_IsSeatedState(localPl->stateGroup, localPl->state) == 1) {
            gTargetBannerHidden = 1;
            return;
        }
    }
    if (((u8 *)localPl->target) == NULL) {
        gTargetBannerHidden = 1;
        return;
    }
    if (localPl->stateGroup == TOWN_PC_GROUP_GESTURE) {
        gTargetBannerHidden = 1;
        return;
    }
    if (localPl->stateGroup == TOWN_PC_GROUP_ACTION) {
        switch (localPl->state) {
        case TOWN_PC_STAND_UP:
        case TOWN_PC_OFFER_ITEM:
        case TOWN_PC_TAKE_ITEM_WAIT:
        case TOWN_PC_OFFER_ITEM_END:
            gTargetBannerHidden = 1;
            return;
        }
    }
    Text_SetSize(task->textSize);
    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(20, 20);
    if ((s8)Kbd_IsOpen() != 0) {
        gTargetBannerHidden = 1;
        return;
    }
    Font_SetSpacing(20, 20);
    if (gTargetBannerHidden == 1) {
        gTargetBannerHidden = 0;
        gTargetBannerCycle = 40;
    } else if (--gTargetBannerCycle <= 0) {
        gTargetBannerCycle = 40;
    }
    targetObj = ((u8 *)localPl->target);
    if (targetObj == (u8 *)localPl && (spot = (ActionSpot *)localPl->spot) != NULL) {
        if ((spot->kind == TOWN_SPOT_SEAT || spot->kind == TOWN_SPOT_HOUSE_SEAT) && localPl->state == TOWN_PC_SIT_DOWN_SEATED) {
            sprintf(buf, gNameTagActionTexts[NAME_TAG_ACTION_STAND_UP]);
        } else {
            switch (spot->kind) {
            default:
                if (gTownMode != TOWN_MODE_SET_OUT) {
                    sprintf(buf, gSpotActionTexts[spot->kind]);
                    break;
                }
                return;
            case TOWN_SPOT_GATE:
                if (gTownMode == TOWN_MODE_SET_OUT) {
                    if (gLobby->questPaid != 0) {
                        return;
                    }
                    sprintf(buf, gNameTagActionTexts[NAME_TAG_ACTION_CANCEL_READY]);
                } else {
                    sprintf(buf, gSpotActionTexts[spot->kind]);
                }
                break;
            case TOWN_SPOT_SAVE:
                if (Game_IsOnline() == 1) {
                    sprintf(buf, gNameTagActionTexts[NAME_TAG_ACTION_SAVE]);
                } else {
                    sprintf(buf, gSpotActionTexts[((ActionSpot *)localPl->spot)->kind]);
                }
                break;
            case TOWN_SPOT_AREA_DOOR:
                if (spot->type == AREA_HUB_0x89 && Game_IsOnline() == 1) {
                    sprintf(buf, gSpotActionTexts[0], LobbyArea_GetName(AREA_HUB_0x84));
                } else {
                    spot = (ActionSpot *)localPl->spot;
                    if ((spot->type == AREA_HUB_0x85 && gScene.area == AREA_TAVERN) || (spot->type == AREA_TAVERN && gScene.area == AREA_HUB_0x85)) {
                        sprintf(buf, gSpotActionTexts[spot->kind]);
                    } else {
                        sprintf(buf, gSpotActionTexts[0], LobbyArea_GetName(spot->type));
                    }
                }
                break;
            }
        }
        color = 0x80006000;
    } else if (((Em *)targetObj)->ownTask != 0) { /* run by the lobby: a townsperson */
        if (gTownMode == TOWN_MODE_SET_OUT) {
            return;
        }
        w = (NpcWork *)(targetObj + 1080);
        if ((s8)((NpcWork *)(targetObj + 1080))->noTalk == 1) {
            return;
        }
        sprintf(buf, gLobbyStrFmt, (s32)w->name);
        if (w->id == NPC_POOGIE && (s8)((SaveData *)save)->poogieName[0] == 0) {
            sprintf(buf, gPoogieDefaultName);
        }
        color = 0x80202060;
    } else {
        memberIdx = ((Player *)targetObj)->index;
        if (memberIdx == localPl->index) {
            return;
        }
        if (gTownMode == TOWN_MODE_SET_OUT) {
            return;
        }
        if (gChatHideNames != 0) {
            sprintf(buf, gLabelNameFmt, gNetUsers[memberIdx].id, gListTitles[LIST_TITLE_SEE_STATE]);
        } else {
            sprintf(buf, gLabelNameFmt, gNetUsers[memberIdx].name, gListTitles[LIST_TITLE_SEE_STATE]);
        }
        color = 0x80603000;
    }
    Gfx_DrawPulseGradient(0, 111, Lobby_StrLength(buf) * 9 + 80, 30, color, 0);
    if (gTownMode != TOWN_MODE_SET_OUT && TownPc_IsSeatedState(localPl->stateGroup, localPl->state) == 0 && gLockOnNoCycle == 0) {
        Gfx_DrawPulseGradient(0, 143, 200, 30, color, 0);
    }
    if (gPadState[0].pressed & PAD_CIRCLE) {
        Lobby_DrawButtonIcon(20, 116, 0);
    } else {
        Lobby_DrawButtonIcon(20, 114, 0);
    }
    if (gTownMode != TOWN_MODE_SET_OUT && TownPc_IsSeatedState(localPl->stateGroup, localPl->state) == 0 && gLockOnNoCycle == 0) {
        if (gPadState[0].pressed & (PAD_LEFT | PAD_RIGHT)) {
            Lobby_DrawButtonIcon(20, 148, 16);
        } else {
            Lobby_DrawButtonIcon(20, 146, 16);
        }
    }
    Font_SetPos(56, 115);
    Text_PrintfColored(buf);
    if (gTownMode != TOWN_MODE_SET_OUT && TownPc_IsSeatedState(localPl->stateGroup, localPl->state) == 0 && gLockOnNoCycle == 0) {
        Font_SetPos(56, 147);
        Text_PrintfColored(gLobbyStrFmt, gListTitles[LIST_TITLE_TARGET]);
    }
}

/* A bar from (x, y), w by h, in color, its alpha fading across its width:
 * dir 0 from the left end, whose alpha pulses between 96 and 192 over
 * 64 frames (gFrameTick; the sine's argument is a 16-bit angle), to none
 * at the right end; dir 1 the other way. The lobby's banners and prompts
 * draw their bars with it. */
void Gfx_DrawPulseGradient(s16 x, s16 y, s16 width, s16 height, u32 color, u8 dir)
{
    PrimGradRect rect;
    u16 angle = (gFrameTick & 0x3F) << 10;
    f32 pulse = Math_Sin(0.0000958738f * angle);

    rect.x0 = x;
    rect.y0 = y;
    rect.x1 = x + width;
    rect.y1 = y + height;
    if (dir == 0) {
        rect.color[0] = (color & 0xFFFFFF) | (((s8)(48.0f * pulse) + 144) << 24);
        rect.color[1] = color & 0xFFFFFF;
    } else {
        rect.color[0] = color & 0xFFFFFF;
        rect.color[1] = (color & 0xFFFFFF) | (((s8)(48.0f * pulse) + 144) << 24);
    }
    rect.color[2] = rect.color[0];
    rect.color[3] = rect.color[1];
    Gs_SetState(RS_ALPHA_REF, 0);
    Prim_FillGradRect(&rect);
}

extern IconUV gButtonIconUVs[];

/* Draws icon i of the button icons (texture TEX_SLOT_ICONS, gButtonIconUVs) at (x, y)
 * at its own size (icon 9 at 50 by 32) and returns the x just past it. Icon
 * 0 is the circle button (TownPc_DrawTargetBanner dips it while PAD_CIRCLE is
 * held), icon 16 the left and right buttons. */
s16 Lobby_DrawButtonIcon(s16 x, s16 y, s32 icon)
{
    TexRect spr;
    IconUV *iconUv = &gButtonIconUVs[icon];

    Tex_Prepare(1, 341);
    Tex_Bind(341);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    spr.y = y;
    spr.x = (f32)x;
    spr.w = (f32)(iconUv->uv1.u - iconUv->uv0.u);
    spr.h = iconUv->uv1.v - iconUv->uv0.v;
    if (icon == 9) {
        spr.w = 50;
        spr.h = 32;
    }
    spr.color = -1;
    spr.uv0 = iconUv->uv0;
    spr.uv1 = iconUv->uv1;
    Sprite_Draw((struct Sprite *)&spr);
    return (s16)x + (iconUv->uv1.u - iconUv->uv0.u);
}

void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: with s32 parameters, though the icon sheet's Icon_Draw takes s16 coordinates and sizes and a u32 colour: Icon_DrawWeaponClass passes its values without narrowing them, as the game's code does */

/* Draws weapon class e's icon of the icon sheet at (a, b), c high, in
 * colour d (-1 for the icon's own): square, or with f nonzero the wide one,
 * 1.2 times as wide. */
void Icon_DrawWeaponClass(s32 x, s32 y, s32 size, s32 color, s16 weaponClass, s8 wide)
{
    if (wide != 0) {
        Icon_Draw(x, y, (s32)(1.2f * size), size, color, weaponClass + ICON_WEAPON_CLASS_WIDE);
    } else {
        Icon_Draw(x, y, size, size, color, weaponClass + ICON_WEAPON_CLASS);
    }
}

s32 Equip_GetRarity(s32 type, u16 id); /* unit's own declaration: with an s32 type and an s32 result, though the definition
 * takes and returns u8: Lobby_DrawEquipIcon passes and uses the values without
 * narrowing them, as the game's code does */
u32 Equip_Color(u8 idx, u8 alpha, s32 alt);
void Icon_DrawEquip(s32, s32, s32, s32, s32, s32);

/* An equipment piece's icon (Icon_DrawEquip) at (x, y), w by h: a weapon's
 * class icon (class + 5) in its rarity's colour, or icon 16 uncoloured
 * for a random weapon (Forge_IsRandomWeapon); an armour piece's icon by
 * its type (head 0, body 1, waist 2, arms 3, legs 4) in its rarity's
 * colour. */
void Lobby_DrawEquipIcon(s32 x, s32 y, s32 width, s32 height, EquipItem *item)
{
    s32 kind = item->type;
    s32 color;
    s16 spr;

    if (kind == EQUIP_GUN || kind == EQUIP_BLADE) {
        if (Forge_IsRandomWeapon(item->id) != 0) {
            color = -1;
            spr = 16;
        } else {
            color = Equip_Color(Equip_GetRarity(item->type, item->id), 255, 0);
            spr = Weapon_GetClass(item->type, item->id);
            spr += 5;
        }
    } else {
        switch (kind) {
        case EQUIP_LEGS:
            spr = 4;
            break;
        case EQUIP_HEAD:
            spr = 0;
            break;
        case EQUIP_BODY:
            spr = 1;
            break;
        case EQUIP_ARMS:
            spr = 3;
            break;
        case EQUIP_WAIST:
            spr = 2;
            break;
        }
        color = Equip_Color(Equip_GetRarity(kind, item->id), 255, 0);
    }
    Icon_DrawEquip(x, y, width, height, color, spr);
}

void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: with s32 parameters, though the icon sheet's Icon_Draw takes s16 coordinates and sizes and a u32 colour: Icon_DrawOnlineMark passes its values without narrowing them, as the game's code does */

/* Draws online mark e's icon (HunterCard.onlineMark) at (a, b), c high and
 * 1.25 times as wide, in colour d; nothing for ONLINE_MARK_NONE. */
void Icon_DrawOnlineMark(s32 x, s32 y, s32 height, s32 color, s16 mark)
{
    if (mark != ONLINE_MARK_NONE) {
        Icon_Draw(x, y, (s32)(1.25f * height), height, color, mark + ICON_ONLINE_MARK);
    }
}

/* Draws sprite kind of gLobbySpriteUVs (LOBBY_SPRITE_*) at (x, y) as a
 * square of side |w| in color; nothing for LOBBY_SPRITE_NONE. */
void LobbySprite_Draw(s32 x, s16 y, s32 w, s32 color, s16 kind)
{
    TexRect spr;
    IconUV *spriteUv;

    if (kind != LOBBY_SPRITE_NONE) {
        spriteUv = &gLobbySpriteUVs[kind];
        spr.y = y;
        spr.x = (f32)x;
        spr.w = (f32)w;
        if (spr.w < 0) {
            spr.w = -spr.w;
        }
        spr.h = w;
        spr.color = color;
        spr.uv0 = spriteUv->uv0;
        spr.uv1 = spriteUv->uv1;
        Sprite_Draw((struct Sprite *)&spr);
    }
}

/* The file's .data: the quest categories' colours (for the quest marks),
 * the menu sheets' label icons and strip tables (used by the menu sheets),
 * and the corner tables of LobbySprite_Draw's sprites and of the button
 * icons (Lobby_DrawButtonIcon). */
s32 gQuestCategoryColors[8] = {
#include "lobby/007FE850.inc"
};
u32 gLabelIconsNeg[20] = {
#include "lobby/007FE870.inc"
};
u32 gLabelIconsPos[20] = {
#include "lobby/007FE8C0.inc"
};
u32 gFrameStripColors[8] = {
#include "lobby/007FE910.inc"
};
SheetTurn gStripTurnOffsets[12] = {
#include "lobby/007FE930.inc"
};
IconUV gLobbySpriteUVs[22] = {
#include "lobby/007FE9B0.inc"
};
IconUV gButtonIconUVs[18] = {
#include "lobby/007FEA60.inc"
};

/* The file's .rodata: the format strings of the house label and the
 * target banner, and the Poogie's default name. */
#pragma rodata on
char gHouseCloseFmt[8] = {
#include "lobby/0083B7D0.inc"
};
char gLabelNameFmt[8] = {
#include "lobby/0083B7D8.inc"
};
char gLobbyStrFmt[8] = {
#include "lobby/0083B7E0.inc"
};
char gPoogieDefaultName[12] = {
#include "lobby/0083B7E8.inc"
};
#pragma rodata off
