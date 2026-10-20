/*
 * The town's pets: the Poogie, the care pet of AREA_HUB_0x83 and the house
 * pet (Pet_AreaKind tells which the area holds).
 *
 * Talking to a pet opens one of five modes, each keeping its state in
 * gMenuWork: the naming of the unnamed Poogie (PoogieNaming), the Poogie's
 * menu in the online town's house, the menus outside the house and of the
 * care pet (PoogieMenu), and the house pet's petting. A petting is a timing
 * game against four bars of frames (enum PetBar); a pleasing one, or
 * feeding the pet a food from the pouch, adds points to the pet's counter
 * for the day (LobbyState.petPoints). A full counter sends the
 * pet digging: the zones, spots and finds of the digs (PoogieDig_*) follow,
 * then the day's reset, the pets' notices and the joy mark's timing.
 *
 * The village's records (the residents' gifts and meal visits) are kept
 * apart; the dig tables sit among the menus' tables in the data, so the
 * digs stay with the menus.
 */
#include "common.h"
#include "chat_log.h"
#include "window.h"
#include "syssettings.h"
#include "item.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "lobby_globals.h"
#include "pad.h"
#include "player.h"
#include "lobby_npc.h"
#include "em.h"
#include "scene.h"
#include "quest.h"
#include "save.h"
#include "lobby_text.h"
#include "lobby_labels.h"
#include "lobby/pets.h"   /* the texts this file uses (tools/content_gen.py) */
#include "area.h"
#include "lobby_vobj.h"
s32 PoogieMenu_Feed(void);
extern u8 gMenuWork[];
extern char *gPoogieCareLabels[];
void Text_SetSize(s32 a);
void Menu_DrawLabelList(s16 x, s8 **b, s16 n, s16 cur, u32 color);
void PoogieMenu_DrawFeedList(s16 x, s16 y);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s16 gPetDebugBars[4];
s32 PoogieMenu_GetOutfits(u8 *owned);
u8 PoogieMenu_WornOutfitRow(void);
u8 Notice_PushItem(char *);
void Actor_SetState(void *obj, s32 group, s32 state);
void LobbySe_Play(s32 se);
void CamFocus_Start(Em *obj);
void LobbyMenu_SetDraw(void *draw0, void *draw1, void *draw2, void *draw3);
void PoogieMenu_Draw(void);
extern f32 gPoogieDigZones[];
u8 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius);
void PoogieMenu_DrawPetFeed(void);
extern u8 gNpcPresenceRates[];
s32 LobbyTime_IsAm(void);
s32 LobbyWorld_GetSeason(void);
u16 Rand_Next(s32 stream);
extern s16 gPetBars[];
extern s16 gPetBarsCare[];
u8 Pet_AreaKind(void);
extern s32 gPoogieFeedLimits[];
extern u8 gPoogieFeedLimitRates[];
extern s32 gCareFeedLimits[];
extern u8 gCareFeedLimitRates[];
extern u8 gPoogieDigLimits[];
extern u8 gCareDigLimits[];
extern u8 gPoogieDigZone[];
extern s32 gPoogieDigSeason;
extern u8 gSeasonFoodCounts[];
extern u16 gSeasonFoodsBreeding[];
extern u16 gSeasonFoodsWarm[];
extern u16 gSeasonFoodsCold[];
extern ListPanel gOutfitListBox;
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 a);
void Gfx_SetTexFilter(s32 a);
s32 Gs_SetState(u8, u32);
void Gfx_SetBlend(s32 a, s32 b);
s32 Shake_Update(u8);
void *Shake_GetOffset(u8 id);
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void PoogieMenu_BuildOutfitList(void);
s32 Save_TestEventFlag(s32 flag);
s32 Kitchen_IsIngredient(u16 item);
s32 Menu_CountPages(s16 n, s16 per);
void *memset(void *d, s32 c, u32 n);
extern PoogieOutfit gPoogieOutfits[];
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
void Save_SetEventFlag(s32 flag);
u8 SavePouch_Add(u16 item, s16 n);
s16 Poogie_RollFeedLimit(u8 kind);
s32 Game_IsOnline(void);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
u16 LobbyPad_Get(s32 a);
s32 Talk_Update(void);
struct Em;
void Talk_Say(s32 msg, s8 a);
void CamFocus_End(void);
/* end of prototypes */

INCLUDE_RODATA("asm/lobby/nonmatchings/pets", gPetChoicePetText);
INCLUDE_RODATA("asm/lobby/nonmatchings/pets", gPetChoiceFeedText);
INCLUDE_RODATA("asm/lobby/nonmatchings/pets", gPetChoiceOutfitText);

extern u8 gPetFollows;
extern u8 gPoogieHappy;
extern u8 gPoogieFollowing;
Em *Npc_Find(u8 kind);
void Talk_Start(Em *npc, s8 quiet);
/*
 * Opens the Poogie's naming (TOWN_MODE_POOGIE_PET): the town's talk
 * opens it for an unnamed Poogie that isn't busy. NPC_NAMING_TALKER starts
 * its talk with the player and the camera turns to the Poogie; nothing
 * opens when that townsperson isn't in the area.
 */
void PoogieNaming_Open(Em *poogie)
{
    PoogieNaming *naming = (PoogieNaming *)gMenuWork;
    Em *talker = Npc_Find(NPC_NAMING_TALKER);

    if (talker != NULL) {
        gPetFollows = 0;
        gTownMode = TOWN_MODE_POOGIE_PET;
        gTownMenuOpen = 1;
        naming->poogie = poogie;
        naming->step = NAMING_WAIT;
        naming->talk = NAMING_TALK_ASK;
        gPoogieHappy = 1;
        gPoogieFollowing = 0;
        Actor_SetState(talker, 0, 0x2C); /* its talk with the player */
        Talk_Start(talker, 1);
        LobbySe_Play(LOBBY_SE_TALK);
        CamFocus_Start(poogie);
    }
}

s8 Poogie_UpdatePetting(Em *task);
void Jingle_Play(void *obj, s32 id);
void CamFocus_End(void);
s32 CamFocus_IsOn(void);
void Actor_ReturnToIdle(void *pl, s32 a, s32 n, s32 m);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_RunFiltered(char *text, s16 held, s16 pressed);
char *Talk_LineText(u16 id);
char *Text_FillDollarFmt(char *fmt, char *name);
void Kbd_Cancel(void);
extern u8 gTownJingleOn;
extern u8 gPoogieLeft;
extern u16 gTalkLine;
extern char *gTalkLineText;
extern u8 gPoogieByPlayer;

/*
 * Runs the naming each frame (offline only): the petting first; a pleased
 * Poogie goes on to the talk, whose yes opens the keyboard for its name
 * (SaveData.poogieName). A petting that ends with no reaction closes the
 * naming and sends the Poogie back to following or to its route.
 */
void PoogieNaming_Update(void)
{
    PoogieNaming *naming = (PoogieNaming *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    SaveData *save = &gSave;
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&naming->poogie->work;
    NpcRouteEnt *point;

    if (Game_IsOnline() == 1) {
        return;
    }
    switch (naming->step) {
    case NAMING_WAIT:
        if (w->petAct == PET_ACT_NONE && naming->poogie->state != PET_STATE_PETTED) {
            pl->cookTimer = 0;
            Actor_SetState(naming->poogie, 0, PET_STATE_PETTED);
            Jingle_Play(pl, PET_JINGLE_START);
            gTownJingleOn = 0;
            naming->step++;
        }
        break;
    case NAMING_PETTING:
        switch ((u8)Poogie_UpdatePetting(naming->poogie)) {
        case PETTING_PLEASED:
            CamFocus_End();
            gPetFollows = 1;
            naming->step++;
            break;
        case PETTING_ANGRY:
            CamFocus_End();
            gPoogieLeft = 1;
            naming->step = NAMING_ANGRY;
            break;
        case PETTING_NOTHING:
        default:
            CamFocus_End();
            naming->step++;
            break;
        case PETTING_RUNNING:
            break;
        }
        break;
    case NAMING_REACTION:
        if (pl->motion[0].playing <= 0) {
            Actor_ReturnToIdle(pl, 0, 8, 0);
            if (pl->cookResult == PET_RESULT_JOY || pl->cookResult == PET_RESULT_GREAT_JOY) {
                Talk_Say(3, 0); /* the first question */
                naming->step = NAMING_TALK;
            } else {
                gTownMenuOpen = 0;
                CamFocus_End();
                gTownMode = TOWN_MODE_FREE;
                point = &w->route[w->routeIdx];
                if (gPetFollows == 1) {
                    Actor_SetState(naming->poogie, 0, PET_STATE_FOLLOW);
                } else {
                    Actor_SetState(naming->poogie, 0, *(u16 *)&point->state); /* the route point's state, read as its low halfword */
                }
            }
        }
        pl->weaponShown = 0;
        break;
    case NAMING_ANGRY:
        pl->weaponShown = 0;
        break;
    case NAMING_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
            if (naming->talk == NAMING_TALK_ASK) {
                Talk_Say(5, 0);
                naming->talk = NAMING_TALK_NAME_IT;
            } else if (naming->talk == NAMING_TALK_NAME_IT || naming->talk == NAMING_TALK_ASK_AGAIN) {
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 0xF, 8, save->poogieName);
                gFieldControl = 0;
                naming->step = NAMING_KEYBOARD;
            } else if (naming->talk == NAMING_TALK_NAMED) {
                Talk_Say((u16)Rand_Next(1) % 4 + 8, 0); /* one of four closing lines */
                naming->talk = NAMING_TALK_LAST;
            } else if (naming->talk == NAMING_TALK_LAST) {
                naming->step = NAMING_CLOSE;
            } else if (naming->talk == NAMING_TALK_REFUSED) {
                naming->step = NAMING_CLOSE;
            }
            break;
        case MENU_CANCEL:
            if (naming->talk == NAMING_TALK_ASK) {
                Talk_Say(4, 0);
                naming->talk = NAMING_TALK_ASK_AGAIN;
            } else if (naming->talk == NAMING_TALK_ASK_AGAIN) {
                Talk_Say(6, 0);
                naming->talk = NAMING_TALK_REFUSED;
            } else if (naming->talk == NAMING_TALK_NAMED) {
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 0xF, 8, save->poogieName);
                gFieldControl = 0;
                naming->step = NAMING_KEYBOARD;
            }
            break;
        }
        break;
    case NAMING_CLOSE:
        w->petAct = PET_ACT_NONE;
        gTownMenuOpen = 0;
        if (CamFocus_IsOn() == 1) {
            CamFocus_End();
        }
        gTownMode = TOWN_MODE_FREE;
        break;
    case NAMING_KEYBOARD:
        if (Kbd_RunFiltered((char *)save->poogieName, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            if ((s8)save->poogieName[0] == 0) {
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 0xF, 8, save->poogieName);
            } else {
                naming->step = NAMING_TALK;
                Talk_Say(7, 0); /* the line with the name */
                gTalkLineText = Text_FillDollarFmt(Talk_LineText(gTalkLine), (char *)save->poogieName);
                naming->talk = NAMING_TALK_NAMED;
                gPoogieByPlayer = 1;
                gPoogieHappy = 0;
                Kbd_Cancel();
                gFieldControl = 1;
            }
        }
        break;
    }
}

/*
 * Opens the Poogie's menu in the online town's house (TOWN_MODE_POOGIE_MENU):
 * reads the outfits owned and the one worn, copies the debug bars, turns
 * the Poogie to the player. When it is back from its errand
 * (POOGIE_ERRAND_BACK), a notice says it holds something.
 */
void PoogieMenu_Open(Em *poogie)
{
    PoogieMenu *menu;

    gTownMode = TOWN_MODE_POOGIE_MENU;
    menu = (PoogieMenu *)gMenuWork;
    gTownMenuOpen = 1;
    menu->pet = poogie;
    menu->step = PET_STEP_CHOOSE;
    menu->feedStep = PET_FEED_SLIDE;
    menu->eatStep = 0;
    menu->choice = PET_CHOICE_PET;
    menu->owned[0] = 1;
    PoogieMenu_GetOutfits(&menu->owned[1]);
    menu->outfitRow = PoogieMenu_WornOutfitRow();
    menu->outfitNames[0] = "";
    menu->outfitCount = 0;
    menu->petResult = -1; /* no petting yet */
    menu->win.state = 0;
    menu->win.pos = 0x280;
    menu->cursor = 0;
    menu->chosenRow = 0;
    menu->showInfo = 0;
    menu->eatTimer = 150; /* frames: 5 s */
    menu->findStep = FIND_JOY;
    menu->errandEarned = 0;
    menu->inputDelay = 0;
    menu->debugBar = PET_BAR_WAIT;
    menu->bars[PET_BAR_WAIT] = gPetDebugBars[PET_BAR_WAIT];
    menu->bars[PET_BAR_JOY] = gPetDebugBars[PET_BAR_JOY];
    menu->bars[PET_BAR_GREAT_JOY] = gPetDebugBars[PET_BAR_GREAT_JOY];
    menu->bars[PET_BAR_ANGER] = gPetDebugBars[PET_BAR_ANGER];
    if (gPoogieErrand == POOGIE_ERRAND_BACK) {
        Notice_PushItem(TXT(0x0082C8A0) /* notice: the pet is holding something in its mouth */);
    }
    Actor_SetState(menu->pet, 0, PET_STATE_FACE);
    LobbySe_Play(LOBBY_SE_TALK);
    CamFocus_Start(poogie);
    LobbyMenu_SetDraw(PoogieMenu_Draw, 0, 0, 0);
}

void Bgm_FadeToVolume(s32 a);
void Poogie_ResetPetting(void);
void PoogieMenu_BuildFeedList(void);
u8 PoogieMenu_RowsOnPage(PoogieMenu *menu);
s32 PoogieMenu_ReactionDone(void);
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
void Poogie_NotifyMood(u8 kind);
s32 Poogie_UpdatePettingDebug(Em *task);
u8 PoogieMenu_OutfitAtCursor(void);
s32 Debug_IsOn(s32 arg0);
void Shake_Start(u8 id, u8 flags, f32 amp);
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
void Shake_Stop(void);
void TownMsg_SendStateNow(Player *p);
extern f32 gPoogieFinds[];
extern u8 gPoogiePetStreak;

/*
 * Runs the Poogie's menu each frame. Back from its errand, the Poogie
 * hands over a find (gPoogieFinds) and the menu closes. Else: the choice
 * list, then the petting, the feeding list or (offline) the outfit list.
 * Two very joyful pettings in a row send the Poogie on its errand
 * (errandEarned, then gPoogieErrand as the menu closes). The debug
 * step edits the petting bars.
 */
void PoogieMenu_Update(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&menu->pet->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    SaveData *save = &gSave;
    NpcRouteEnt *point;
    u16 item;
    u8 i;
    u8 n;
    s16 *bar;
    f32 roll;
    f32 *upTo;

    if (gPoogieErrand == POOGIE_ERRAND_BACK) {
        switch (menu->findStep) {
        case FIND_JOY:
            if (menu->pet->motion[0].playing <= 0) {
                w->offerMark = 0; /* the mark over it goes */
                Actor_SetState(menu->pet, 0, PET_STATE_GREAT_JOY);
                Jingle_Play(pl, PET_JINGLE_JOY);
                Bgm_FadeToVolume(0xF);
                menu->findStep++;
            }
            break;
        case FIND_FACE:
            if (menu->pet->motion[0].playing <= 0 || menu->pet->state == PET_STATE_FOLLOW) {
                gPetFollows = 0;
                w->petAct = PET_ACT_NONE;
                Actor_SetState(menu->pet, 0, PET_STATE_FACE);
                menu->findStep++;
            }
            break;
        case FIND_WAIT_IDLE:
            if (menu->pet->state == PET_STATE_IDLE) {
                menu->findStep++;
            }
            break;
        case FIND_GIVE:
            /* a roll in [1, 101): the first find whose share reaches it */
            roll = 1.0f + 0.01f * ((u16)Rand_Next(1) % 10000);
            upTo = &gPoogieFinds[1];
            for (i = 1; i <= 0x1F; i += 2) {
                if (roll <= *upTo) {
                    item = (s32)gPoogieFinds[i - 1];
                    break;
                }
                upTo += 2;
            }
            /*
             * Past every share, item is never set and the game gives whatever
             * its register held. It stays uninitialised so that the compiler
             * gives it the same register.
             */
            Pouch_GiveItem(pl, item, pl->gotItem, 1);
            menu->inputDelay = 30; /* frames of the notice */
            menu->findStep++;
            break;
        case FIND_CLOSE:
            if (menu->inputDelay-- <= 0) {
                LobbyMenu_SetDraw(0, 0, 0, 0);
                Actor_ReturnToIdle(pl, 0, 8, 0);
                gTownMenuOpen = 0;
                CamFocus_End();
                gTownMode = TOWN_MODE_FREE;
                gPoogieErrand = POOGIE_ERRAND_NONE;
            }
            break;
        }
    } else {
        switch (menu->step) {
        case PET_STEP_CHOOSE:
            if (pad & (PAD_UP | PAD_DOWN)) {
                if (Game_IsOnline() == 0) {
                    menu->choice = Menu_WrapUpDown(menu->choice, 3); /* pet, feed, the outfit */
                } else {
                    menu->choice = Menu_WrapUpDown(menu->choice, 2); /* pet, feed */
                }
            }
            if (menu->inputDelay++ >= PET_INPUT_DELAY) {
                menu->inputDelay = PET_INPUT_DELAY;
            }
            if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE) {
                switch (menu->choice) {
                case PET_CHOICE_PET:
                    if (Debug_IsOn(PET_DEBUG_BARS)) {
                        menu->step = PET_STEP_DEBUG_BARS;
                        menu->inputDelay = 0;
                    } else {
                        Actor_SetState(menu->pet, 0, PET_STATE_PETTED);
                        Jingle_Play(pl, PET_JINGLE_START);
                        gTownJingleOn = 0;
                        Poogie_ResetPetting();
                        menu->step = PET_STEP_RUN;
                    }
                    break;
                case PET_CHOICE_FEED:
                    PoogieMenu_BuildFeedList();
                    menu->step = PET_STEP_RUN;
                    menu->feedStep = PET_FEED_SLIDE;
                    menu->win.state = 0;
                    menu->win.pos = 0x280;
                    menu->cursor = 0;
                    break;
                case PET_CHOICE_OUTFIT:
                    menu->outfitRow = PoogieMenu_WornOutfitRow();
                    Shake_Start(0, 0, 180.0f);
                    menu->step = PET_STEP_RUN;
                    break;
                }
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
            if (pad & PAD_CROSS) {
                menu->step = PET_STEP_SEND_OFF;
                Actor_SetMotionOnLayer(menu->pet, 0x3E9, 0x18, 0, 0);
                LobbySe_Play(LOBBY_SE_CANCEL);
            }
            break;
        case PET_STEP_RUN:
            switch (menu->choice) {
            case PET_CHOICE_PET:
                if (Debug_IsOn(PET_DEBUG_BARS)) {
                    menu->petResult = Poogie_UpdatePettingDebug(menu->pet);
                } else {
                    menu->petResult = Poogie_UpdatePetting(menu->pet);
                }
                if (menu->petResult == PETTING_RUNNING) {
                    break;
                }
                Actor_SetMotionOnLayer(menu->pet, 0x3E9, 0x18, 0, 0);
                switch (menu->petResult) {
                case PETTING_PLEASED:
                    gPetFollows = 1;
                    if (menu->pet->state == PET_STATE_GREAT_JOY) {
                        if (!Debug_IsOn(PET_DEBUG_BARS)) {
                            gPoogiePetStreak++;
                            Poogie_NotifyMood(PET_NOTICE_STREAK);
                        }
                    } else {
                        gPoogiePetStreak = 0;
                    }
                    if (gPoogiePetStreak >= 2) {
                        menu->step = PET_STEP_SEND_OFF;
                        gPoogiePetStreak = 0;
                        menu->errandEarned = 1;
                    } else {
                        menu->step = PET_STEP_CLOSE;
                    }
                    break;
                case PETTING_ANGRY:
                    menu->step = PET_STEP_ANGRY;
                    gPoogieLeft = 1;
                    gPoogiePetStreak = 0;
                    break;
                case PETTING_NOTHING:
                    menu->step = PET_STEP_SEND_OFF;
                    gPoogiePetStreak = 0;
                    break;
                }
                break;
            case PET_CHOICE_FEED:
                switch (menu->feedStep) {
                case PET_FEED_SLIDE:
                    switch (WinSlide_Step(&menu->win, 0x280, 0x118)) {
                    case MENU_DONE:
                        menu->inputDelay = 0;
                        menu->feedStep++;
                        break;
                    }
                    break;
                case PET_FEED_LIST:
                    if (pad & (PAD_UP | PAD_DOWN)) {
                        menu->cursor = Menu_WrapUpDown(menu->cursor, PoogieMenu_RowsOnPage(menu));
                    }
                    if (pad & (PAD_LEFT | PAD_RIGHT)) {
                        menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
                        n = PoogieMenu_RowsOnPage(menu);
                        if (n < menu->cursor + 1) {
                            menu->cursor = n - 1;
                        }
                    }
                    if (pad & PAD_SQUARE && menu->rows > 0) {
                        menu->showInfo ^= 1;
                        LobbySe_Play(LOBBY_SE_DESCRIPTION);
                    }
                    if (menu->inputDelay++ >= PET_INPUT_DELAY) {
                        menu->inputDelay = PET_INPUT_DELAY;
                    }
                    if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE && menu->rows > 0) {
                        menu->feedStep++;
                        menu->chosenRow = menu->cursor;
                        LobbySe_Play(LOBBY_SE_CONFIRM);
                    }
                    if (pad & PAD_CROSS) {
                        menu->showInfo = 0;
                        menu->step = PET_STEP_CHOOSE;
                        menu->inputDelay = 0;
                        LobbySe_Play(LOBBY_SE_CANCEL);
                    }
                    break;
                case PET_FEED_EAT:
                    if (PoogieMenu_Feed() == MENU_DONE) {
                        menu->step = PET_STEP_SEND_OFF;
                    }
                    break;
                }
                break;
            case PET_CHOICE_OUTFIT:
                if (pad & (PAD_UP | PAD_DOWN)) {
                    menu->outfitRow = Menu_WrapUpDown((u8)menu->outfitRow, menu->outfitCount);
                }
                if (menu->inputDelay++ >= PET_INPUT_DELAY) {
                    menu->inputDelay = PET_INPUT_DELAY;
                }
                if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE) {
                    save->poogieOutfit = PoogieMenu_OutfitAtCursor();
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
                if (pad & PAD_CROSS) {
                    menu->step = PET_STEP_CHOOSE;
                    menu->inputDelay = 0;
                    Shake_Stop();
                    LobbySe_Play(LOBBY_SE_CANCEL);
                }
                break;
            }
            break;
        case PET_STEP_CLOSE:
            if (Debug_IsOn(PET_DEBUG_BARS)) {
                gPetDebugBars[PET_BAR_WAIT] = menu->bars[PET_BAR_WAIT];
                gPetDebugBars[PET_BAR_JOY] = menu->bars[PET_BAR_JOY];
                gPetDebugBars[PET_BAR_GREAT_JOY] = menu->bars[PET_BAR_GREAT_JOY];
                gPetDebugBars[PET_BAR_ANGER] = menu->bars[PET_BAR_ANGER];
            }
            LobbyMenu_SetDraw(0, 0, 0, 0);
            if (PoogieMenu_ReactionDone()) {
                Actor_ReturnToIdle(pl, 0, 8, 0);
                TownMsg_SendStateNow(&gPlayers[gScene.localPlayer]);
                gTownMenuOpen = 0;
                CamFocus_End();
                gTownMode = TOWN_MODE_FREE;
                gPoogieErrand = menu->errandEarned;
            }
            break;
        case PET_STEP_ANGRY:
            LobbyMenu_SetDraw(0, 0, 0, 0);
            if (Debug_IsOn(PET_DEBUG_BARS) && menu->pet->motionId[0] == 0x3FA /* the angry motion */
                && menu->pet->motion[0].playing <= 0) {
                menu->step = PET_STEP_SEND_OFF;
                w->petAct = PET_ACT_NONE;
            }
            break;
        case PET_STEP_SEND_OFF:
            if (menu->errandEarned == 1) {
                if (menu->choice == PET_CHOICE_PET && !PoogieMenu_ReactionDone()) {
                    break;
                }
                Actor_SetState(menu->pet, 0, PET_STATE_RUN_OFF);
                gPetFollows = 0;
            } else {
                if ((menu->pet->state == PET_STATE_JOY || menu->pet->state == PET_STATE_GREAT_JOY)
                    && menu->pet->motion[0].playing > 0) {
                    break;
                }
                if (gPetFollows == 1) {
                    Actor_SetState(menu->pet, 0, PET_STATE_FOLLOW);
                } else {
                    point = &w->route[w->routeIdx];
                    Actor_SetState(menu->pet, 0, *(u16 *)&point->state); /* the route point's state, read as its low halfword */
                }
            }
            w->petAct = PET_ACT_NONE;
            menu->step = PET_STEP_CLOSE;
            break;
        case PET_STEP_DEBUG_BARS:
            if (pad & (PAD_UP | PAD_DOWN)) {
                menu->debugBar = Menu_WrapUpDown(menu->debugBar, 4); /* the four PET_BAR_* */
            }
            if (pad & PAD_RIGHT || pad & PAD_R1) {
                if (menu->debugBar == PET_BAR_GREAT_JOY) {
                    /* the great joy stays within the joy */
                    if (menu->bars[PET_BAR_GREAT_JOY] < menu->bars[PET_BAR_JOY]) {
                        if (pad & PAD_R1) {
                            menu->bars[menu->debugBar] += 30;
                        } else {
                            menu->bars[menu->debugBar] += 1;
                        }
                    }
                } else if (pad & PAD_R1) {
                    menu->bars[menu->debugBar] += 30;
                } else {
                    menu->bars[menu->debugBar] += 1;
                }
            }
            if (pad & PAD_LEFT || pad & PAD_L1) {
                /* the bar as a byte offset: indexing bars[] gives other code */
                bar = (s16 *)(menu->debugBar * 2 + (u8 *)menu->bars);
                if (*bar > 0) {
                    if (pad & PAD_L1) {
                        if (*bar >= 30) {
                            *bar -= 30;
                        } else {
                            *bar = 0;
                        }
                    } else {
                        *bar -= 1;
                    }
                }
            }
            if (menu->inputDelay++ >= PET_INPUT_DELAY) {
                menu->inputDelay = PET_INPUT_DELAY;
            }
            if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE) {
                Actor_SetState(menu->pet, 0, PET_STATE_PETTED);
                Jingle_Play(pl, PET_JINGLE_START);
                gTownJingleOn = 0;
                Poogie_ResetPetting();
                menu->step = PET_STEP_RUN;
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
            break;
        }
    }
}

extern u8 gPoogieDigState;
/*
 * Opens the menu of the Poogie outside the house (TOWN_MODE_POOGIE_OUTSIDE;
 * the town's talk opens it in AREA_HUB_0x89): pet or feed. When its dig is
 * done the petting starts at once (PET_STEP_DUG).
 */
void PoogieMenu_OpenOutside(Em *pet)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];

    gTownMode = TOWN_MODE_POOGIE_OUTSIDE;
    gTownMenuOpen = 1;
    menu->pet = pet;
    menu->step = PET_STEP_CHOOSE;
    menu->choice = PET_CHOICE_PET;
    menu->feedStep = PET_FEED_SLIDE;
    menu->eatStep = 0; /* PoogieMenu_Feed's first step */
    menu->findStep = FIND_JOY;
    menu->petResult = -1; /* no petting yet */
    menu->win.state = 0;
    menu->win.pos = 0x280;
    menu->cursor = 0;
    menu->chosenRow = 0;
    menu->showInfo = 0;
    menu->eatTimer = 150; /* frames: 5 s */
    menu->inputDelay = 0;
    if (gPoogieDigState == POOGIE_DIG_DONE) {
        Actor_SetState(menu->pet, 0, PET_STATE_PETTED);
        Jingle_Play(pl, PET_JINGLE_START);
        gTownJingleOn = 0;
        Poogie_ResetPetting();
        menu->step = PET_STEP_DUG;
        menu->choice = PET_CHOICE_DUG;
    } else {
        menu->step = PET_STEP_CHOOSE;
        Actor_SetState(menu->pet, 0, PET_STATE_FACE);
    }
    LobbySe_Play(LOBBY_SE_TALK);
    CamFocus_Start(pet);
    LobbyMenu_SetDraw(PoogieMenu_DrawPetFeed, 0, 0, 0);
}

s32 PoogieMenu_Feed(void);
void PoogieDig_FindZone(f32 *pos, u8 *zone);
extern s16 gPoogieDigTimer;
extern u8 gPoogieDigTaken;
extern u8 gPoogieDigSpotCount;
extern u8 *gPoogieDigSpots;
extern u8 gPoogieDigSpotCounts[];
extern u8 gPoogieDigSpotsZone1[];
extern u8 gPoogieDigSpotsZone2[];
extern u8 gPoogieDigSpotsZone3[];
extern u8 gPoogieDigSpotsZone4[];
extern u8 gPoogieDigSpotsZone5[];
extern u8 gPoogieDigSpotsZone6[];
extern s16 gPetJoyPoints[];

/*
 * Runs the outside menu each frame (online it closes at once): a pleasing
 * petting adds gPetJoyPoints to the Poogie's counter; a full counter is
 * emptied and the Poogie goes digging in the zone it stands in
 * (PoogieDig_FindZone picks the zone's spots).
 */
void PoogieMenu_UpdateOutside(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&menu->pet->work;
    s16 add;
    s16 *counter;
    s16 *points;
    u8 rows;

    if (Game_IsOnline() == 1) {
        menu->step = PET_STEP_CLOSE;
    }
    switch (menu->step) {
    case PET_STEP_CHOOSE:
        if (pad & (PAD_UP | PAD_DOWN)) {
            menu->choice = Menu_WrapUpDown(menu->choice, 2);
        }
        if (menu->inputDelay++ >= PET_INPUT_DELAY) {
            menu->inputDelay = PET_INPUT_DELAY;
        }
        if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE) {
            switch (menu->choice) {
            case PET_CHOICE_PET:
                Actor_SetState(menu->pet, 0, PET_STATE_PETTED);
                Jingle_Play(pl, PET_JINGLE_START);
                gTownJingleOn = 0;
                Poogie_ResetPetting();
                menu->step = PET_STEP_RUN;
                break;
            case PET_CHOICE_FEED:
                PoogieMenu_BuildFeedList();
                menu->step = PET_STEP_RUN;
                menu->feedStep = PET_FEED_SLIDE;
                menu->win.state = 0;
                menu->win.pos = 0x280;
                menu->cursor = 0;
                menu->inputDelay = 0;
                break;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        if (pad & PAD_CROSS) {
            menu->step = PET_STEP_SEND_OFF;
            Actor_SetMotionOnLayer(menu->pet, 0x3E9, 0x18, 0, 0);
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case PET_STEP_RUN:
        switch (menu->choice) {
        case PET_CHOICE_PET:
            menu->petResult = Poogie_UpdatePetting(menu->pet);
            switch (menu->petResult) {
            case PETTING_PLEASED:
                gPetFollows = 1;
                points = gPetJoyPoints;
                if (menu->pet->state == PET_STATE_JOY) {
                    add = points[0];
                } else {
                    add = points[1];
                }
                gLobby->petPoints[PET_AREA_POOGIE] += add;
                Poogie_NotifyMood(PET_NOTICE_COUNTER);
                menu->step = PET_STEP_COUNTER;
                break;
            case PETTING_ANGRY:
                gPoogieLeft = 1;
                menu->step = PET_STEP_ANGRY;
                break;
            case PETTING_NOTHING:
                menu->step = PET_STEP_SEND_OFF;
                break;
            case PETTING_RUNNING:
                break;
            }
            break;
        case PET_CHOICE_FEED:
            switch (menu->feedStep) {
            case PET_FEED_SLIDE:
                switch (WinSlide_Step(&menu->win, 0x280, 0x118)) {
                case MENU_DONE:
                    menu->feedStep++;
                    break;
                }
                break;
            case PET_FEED_LIST:
                if (pad & (PAD_UP | PAD_DOWN)) {
                    menu->cursor = Menu_WrapUpDown(menu->cursor, PoogieMenu_RowsOnPage(menu));
                }
                if (pad & (PAD_LEFT | PAD_RIGHT)) {
                    menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
                    rows = PoogieMenu_RowsOnPage(menu);
                    if (menu->cursor + 1 > rows) {
                        menu->cursor = rows - 1;
                    }
                }
                if (pad & PAD_SQUARE && menu->rows > 0) {
                    menu->showInfo ^= 1;
                    LobbySe_Play(LOBBY_SE_DESCRIPTION);
                }
                if (menu->inputDelay++ >= PET_INPUT_DELAY) {
                    menu->inputDelay = PET_INPUT_DELAY;
                }
                if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE && menu->rows > 0) {
                    menu->feedStep++;
                    menu->chosenRow = menu->cursor;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
                if (pad & PAD_CROSS) {
                    menu->showInfo = 0;
                    menu->step = PET_STEP_CHOOSE;
                    menu->inputDelay = 0;
                    LobbySe_Play(LOBBY_SE_CANCEL);
                }
                break;
            case PET_FEED_EAT:
                if (PoogieMenu_Feed() == MENU_DONE) {
                    menu->step = PET_STEP_SEND_OFF;
                }
                break;
            }
            break;
        }
        break;
    case PET_STEP_COUNTER:
        counter = &gLobby->petPoints[PET_AREA_POOGIE];
        if (*counter >= gLobby->petPointsMax[PET_AREA_POOGIE]) {
            *counter = 0;
            menu->step = PET_STEP_DIG;
        } else {
            menu->step = PET_STEP_SEND_OFF;
        }
        break;
    case PET_STEP_ANGRY:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        break;
    case PET_STEP_DIG:
        if ((menu->pet->state != PET_STATE_JOY && menu->pet->state != PET_STATE_GREAT_JOY) || menu->pet->motion[0].playing <= 0) {
            gPoogieDigTimer = 0;
            gPoogieDigTaken = 0;
            gPoogieDigZone[0] = 1; /* the search starts at zone 1 */
            PoogieDig_FindZone(&menu->pet->posX, gPoogieDigZone);
            gPoogieDigSpotCount = gPoogieDigSpotCounts[gPoogieDigZone[0]];
            switch (gPoogieDigZone[0]) {
            case 0:
                break;
            case 1:
                gPoogieDigSpots = gPoogieDigSpotsZone1;
                break;
            case 2:
                gPoogieDigSpots = gPoogieDigSpotsZone2;
                break;
            case 3:
                gPoogieDigSpots = gPoogieDigSpotsZone3;
                break;
            case 4:
                gPoogieDigSpots = gPoogieDigSpotsZone4;
                break;
            case 5:
                gPoogieDigSpots = gPoogieDigSpotsZone5;
                break;
            case 6:
                gPoogieDigSpots = gPoogieDigSpotsZone6;
                break;
            }
            Actor_SetState(menu->pet, 0, PET_STATE_DIG);
            menu->step = PET_STEP_CLOSE;
        }
        break;
    case PET_STEP_SEND_OFF:
        if ((menu->pet->state != PET_STATE_JOY && menu->pet->state != PET_STATE_GREAT_JOY) || menu->pet->motion[0].playing <= 0) {
            Actor_SetState(menu->pet, 0, PET_STATE_FOLLOW);
            w->petAct = PET_ACT_NONE;
            menu->step = PET_STEP_CLOSE;
        }
        break;
    case PET_STEP_CLOSE:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        if (PoogieMenu_ReactionDone()) {
            w->petAct = PET_ACT_NONE;
            Actor_ReturnToIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = TOWN_MODE_FREE;
        }
        break;
    case PET_STEP_DUG:
        menu->petResult = Poogie_UpdatePetting(menu->pet);
        switch (menu->petResult) {
        case PETTING_NOTHING:
        case PETTING_PLEASED:
            gPetFollows = 1;
            pl->cookResult = PET_RESULT_NONE;
            menu->step = PET_STEP_SEND_OFF;
            if (menu->pet->state == PET_STATE_GREAT_JOY) {
                Poogie_NotifyMood(PET_NOTICE_DELIGHTED);
            }
            break;
        case PETTING_ANGRY:
            menu->step = PET_STEP_ANGRY;
            gPoogieLeft = 1;
            break;
        case PETTING_RUNNING:
            break;
        }
        break;
    }
}

/*
 * Opens the care pet's menu (TOWN_MODE_POOGIE_CARE; NPC_POOGIE_CARE in
 * AREA_HUB_0x83): pet or feed.
 */
void PoogieMenu_OpenCare(Em *pet)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;

    gTownMode = TOWN_MODE_POOGIE_CARE;
    gTownMenuOpen = 1;
    menu->pet = pet;
    menu->step = PET_STEP_CHOOSE;
    menu->choice = PET_CHOICE_PET;
    menu->feedStep = PET_FEED_SLIDE;
    menu->eatStep = 0; /* PoogieMenu_Feed's first step */
    menu->findStep = FIND_JOY;
    menu->petResult = -1; /* no petting yet */
    menu->win.state = 0;
    menu->win.pos = 0x280;
    menu->cursor = 0;
    menu->chosenRow = 0;
    menu->showInfo = 0;
    menu->eatTimer = 150; /* frames: 5 s */
    menu->inputDelay = 0;
    Actor_SetState(menu->pet, 0, PET_STATE_FACE);
    LobbySe_Play(LOBBY_SE_TALK);
    CamFocus_Start(pet);
    LobbyMenu_SetDraw(PoogieMenu_DrawPetFeed, 0, 0, 0);
}

s32 Poogie_IsSeasonFood(u16 item);
extern u8 gPoogieDigState;
extern u8 gCarePetLeft;
extern u8 gPoogieDigSpotIdx;
extern u8 gCarePetHappy;
extern u8 gPoogieFedSeasonItem;
extern u8 gCareDigSpotsBreeding[];
extern u8 gCareDigSpotsWarm[];
extern u8 gCareDigSpotsCold[];

/*
 * Runs the care pet's menu each frame: as the outside menu, with the care
 * pet's counter. A full counter sends it digging at the season's spots;
 * feeding it a food of the season (Poogie_IsSeasonFood) gives better
 * finds that day. After a dig the petting comes first, and a very
 * joyful one sends it digging again.
 */
void PoogieMenu_UpdateCare(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&menu->pet->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    s16 add;
    s16 *points;
    u8 rows;

    switch (menu->step) {
    case PET_STEP_CHOOSE:
        if (gPoogieDigState == POOGIE_DIG_DONE) {
            Actor_SetState(menu->pet, 0, PET_STATE_PETTED);
            Jingle_Play(pl, PET_JINGLE_START);
            gTownJingleOn = 0;
            Poogie_ResetPetting();
            menu->step = PET_STEP_CARE_DUG;
            break;
        }
        if (pad & (PAD_UP | PAD_DOWN)) {
            menu->choice = Menu_WrapUpDown(menu->choice, 2);
        }
        if (menu->inputDelay++ >= PET_INPUT_DELAY) {
            menu->inputDelay = PET_INPUT_DELAY;
        }
        if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE) {
            switch (menu->choice) {
            case PET_CHOICE_PET:
                Actor_SetState(menu->pet, 0, PET_STATE_PETTED);
                Jingle_Play(pl, PET_JINGLE_START);
                gTownJingleOn = 0;
                Poogie_ResetPetting();
                menu->step = PET_STEP_RUN;
                break;
            case PET_CHOICE_FEED:
                PoogieMenu_BuildFeedList();
                menu->step = PET_STEP_RUN;
                menu->feedStep = PET_FEED_SLIDE;
                menu->win.state = 0;
                menu->win.pos = 0x280;
                menu->cursor = 0;
                menu->inputDelay = 0;
                break;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        if (pad & PAD_CROSS) {
            menu->step = PET_STEP_CLOSE;
            Actor_SetMotionOnLayer(menu->pet, 0x3E9, 0x18, 0, 0);
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case PET_STEP_RUN:
        switch (menu->choice) {
        case PET_CHOICE_PET:
            menu->petResult = Poogie_UpdatePetting(menu->pet);
            if (menu->petResult != PETTING_RUNNING) {
                Actor_SetMotionOnLayer(menu->pet, 0x3E9, 0x18, 0, 0);
                switch (menu->petResult) {
                case PETTING_PLEASED:
                    points = gPetJoyPoints;
                    if (menu->pet->state == PET_STATE_JOY) {
                        add = points[0];
                    } else {
                        add = points[1];
                    }
                    gPetFollows = 1;
                    gLobby->petPoints[PET_AREA_CARE] += add;
                    Poogie_NotifyMood(PET_NOTICE_COUNTER);
                    if (gLobby->petPoints[PET_AREA_CARE] >= gLobby->petPointsMax[PET_AREA_CARE]) {
                        menu->step = PET_STEP_DIG;
                        gLobby->petPoints[PET_AREA_CARE] = 0;
                    } else {
                        menu->step = PET_STEP_CLOSE;
                    }
                    break;
                case PETTING_ANGRY:
                    menu->step = PET_STEP_ANGRY;
                    gCarePetLeft = 1;
                    break;
                case PETTING_NOTHING:
                    menu->step = PET_STEP_CLOSE;
                    break;
                }
            }
            break;
        case PET_CHOICE_FEED:
            switch (menu->feedStep) {
            case PET_FEED_SLIDE:
                switch (WinSlide_Step(&menu->win, 0x280, 0x118)) {
                case MENU_DONE:
                    menu->feedStep++;
                    break;
                }
                break;
            case PET_FEED_LIST:
                if (pad & (PAD_UP | PAD_DOWN)) {
                    menu->cursor = Menu_WrapUpDown(menu->cursor, PoogieMenu_RowsOnPage(menu));
                }
                if (pad & (PAD_LEFT | PAD_RIGHT)) {
                    menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
                    rows = PoogieMenu_RowsOnPage(menu);
                    if (menu->cursor + 1 > rows) {
                        menu->cursor = rows - 1;
                    }
                }
                if (pad & PAD_SQUARE && menu->rows > 0) {
                    menu->showInfo ^= 1;
                    LobbySe_Play(LOBBY_SE_DESCRIPTION);
                }
                if (menu->inputDelay++ >= PET_INPUT_DELAY) {
                    menu->inputDelay = PET_INPUT_DELAY;
                }
                if (menu->inputDelay >= PET_INPUT_DELAY && pad & PAD_CIRCLE && menu->rows > 0) {
                    menu->feedStep++;
                    menu->chosenRow = menu->cursor;
                    gPoogieDigSeason = LobbyWorld_GetSeason();
                    if (Poogie_IsSeasonFood(menu->items[menu->chosenRow]) == 1) {
                        gPoogieFedSeasonItem = 1;
                    }
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
                if (pad & PAD_CROSS) {
                    menu->showInfo = 0;
                    menu->step = PET_STEP_CHOOSE;
                    menu->inputDelay = 0;
                    LobbySe_Play(LOBBY_SE_CANCEL);
                }
                break;
            case PET_FEED_EAT:
                if (PoogieMenu_Feed() == MENU_DONE) {
                    if (gLobby->petPoints[PET_AREA_CARE] <= 0) {
                        gCarePetHappy = 1;
                    }
                    menu->feedStep++;
                }
                break;
            case PET_FEED_REACT:
                if ((menu->pet->state != PET_STATE_JOY && menu->pet->state != PET_STATE_GREAT_JOY) || menu->pet->motion[0].playing <= 0) {
                    menu->step = PET_STEP_CLOSE;
                }
                break;
            }
            break;
        }
        break;
    case PET_STEP_DIG:
        if (menu->pet->motion[0].playing <= 0 || menu->feedStep == PET_FEED_EAT) {
            gPoogieDigTimer = 0;
            gPoogieDigSpotIdx = 0;
            gPoogieDigZone[0] = 1; /* the zone search starts at zone 1 */
            gPoogieDigTaken = 0;
            gPoogieDigSeason = LobbyWorld_GetSeason();
            switch (gPoogieDigSeason) {
            case SEASON_WARM:
                gPoogieDigSpotCount = 4;
                gPoogieDigSpots = gCareDigSpotsWarm;
                break;
            case SEASON_COLD:
                gPoogieDigSpotCount = 2;
                gPoogieDigSpots = gCareDigSpotsCold;
                break;
            case SEASON_BREEDING:
            default:
                gPoogieDigSpotCount = 3;
                gPoogieDigSpots = gCareDigSpotsBreeding;
                break;
            }
            Actor_SetState(menu->pet, 0, PET_STATE_DIG);
            w->petAct = PET_ACT_NONE;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            Actor_ReturnToIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = TOWN_MODE_FREE;
        }
        break;
    case PET_STEP_CLOSE:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        if (PoogieMenu_ReactionDone()) {
            Actor_SetState(menu->pet, 0, *(u16 *)&w->route[w->routeIdx].state); /* the route point's state, read as its low halfword */
            w->petAct = PET_ACT_NONE;
            Actor_ReturnToIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = TOWN_MODE_FREE;
        }
        break;
    case PET_STEP_ANGRY:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        break;
    case PET_STEP_CARE_DUG:
        menu->petResult = Poogie_UpdatePetting(menu->pet);
        switch (menu->petResult) {
        case PETTING_NOTHING:
        default:
            menu->step = PET_STEP_CLOSE;
            break;
        case PETTING_PLEASED:
            gPetFollows = 1;
            if (menu->pet->state == PET_STATE_GREAT_JOY) {
                menu->step = PET_STEP_CARE_DIG_AGAIN;
            } else {
                menu->step = PET_STEP_CLOSE;
            }
            if (menu->pet->state == PET_STATE_GREAT_JOY) {
                Poogie_NotifyMood(PET_NOTICE_DELIGHTED);
            }
            break;
        case PETTING_ANGRY:
            menu->step = PET_STEP_ANGRY;
            gCarePetLeft = 1;
            break;
        case PETTING_RUNNING:
            break;
        }
        break;
    case PET_STEP_CARE_DIG_AGAIN:
        if (menu->pet->motion[0].playing <= 0) {
            gPoogieDigTaken = 0;
            Actor_SetState(menu->pet, 0, PET_STATE_DIG);
            w->petAct = PET_ACT_NONE;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            Actor_ReturnToIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = TOWN_MODE_FREE;
        }
        break;
    case PET_STEP_SEND_OFF:
    case PET_STEP_DEBUG_BARS:
        break;
    }
}

/* Opens the house pet's petting (TOWN_MODE_HOUSE_PET; NPC_HOUSE_PET). */
void HousePet_Open(Em *pet)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;

    gTownMode = TOWN_MODE_HOUSE_PET;
    gTownMenuOpen = 1;
    menu->housePet = pet;
    menu->houseStep = HOUSE_PET_START;
    menu->houseResult = PETTING_NOTHING;
    Actor_SetState(menu->housePet, 0, PET_STATE_FACE);
    LobbySe_Play(LOBBY_SE_TALK);
    CamFocus_Start(menu->housePet);
}

extern u8 gHousePetLeft;

/*
 * Runs the house pet's petting each frame: its great joy gives a notice
 * and, the first time, the Poogie's outfit 2 (EVENT_FLAG_POOGIE_OUTFIT_2).
 * After a pleasing petting the pet follows the player.
 */
void HousePet_Update(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&menu->housePet->work;

    switch (menu->houseStep) {
    case HOUSE_PET_START:
        Actor_SetState(menu->housePet, 0, PET_STATE_PETTED);
        Jingle_Play(pl, PET_JINGLE_START);
        gTownJingleOn = 0;
        Poogie_ResetPetting();
        menu->houseStep++;
        break;
    case HOUSE_PET_PETTING:
        menu->houseResult = Poogie_UpdatePetting(menu->housePet);
        switch (menu->houseResult) {
        case PETTING_NOTHING:
        default:
            menu->houseStep = HOUSE_PET_CLOSE;
            break;
        case PETTING_PLEASED:
            menu->houseStep = HOUSE_PET_PLEASED;
            break;
        case PETTING_ANGRY:
            menu->houseStep = HOUSE_PET_ANGRY;
            gHousePetLeft = 1;
            break;
        case PETTING_RUNNING:
            break;
        }
        break;
    case HOUSE_PET_ANGRY:
        break;
    case HOUSE_PET_PLEASED:
        if (menu->housePet->state == PET_STATE_GREAT_JOY) {
            Poogie_NotifyMood(PET_NOTICE_HOUSE_PET);
            if (Save_TestEventFlag(EVENT_FLAG_POOGIE_OUTFIT_2) == 0) {
                Save_SetEventFlag(EVENT_FLAG_POOGIE_OUTFIT_2);
                Notice_PushItem(TXT(0x0082C960) /* notice: a new outfit for the pet */);
            }
        }
        menu->houseStep = HOUSE_PET_CLOSE;
        w->petAct = PET_ACT_NONE;
        break;
    case HOUSE_PET_CLOSE:
        if (PoogieMenu_ReactionDone()) {
            Actor_ReturnToIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = TOWN_MODE_FREE;
            if (menu->houseResult == PETTING_PLEASED) {
                Actor_SetState(menu->housePet, 0, PET_STATE_FOLLOW);
            } else {
                Actor_SetState(menu->housePet, 0, *(u16 *)&w->route[w->routeIdx].state); /* the route point's state, read as its low halfword */
            }
        }
        break;
    }
}

extern char *gPoogieMenuLabels[];
void PoogieMenu_DrawOutfitList(void);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);

/*
 * Draws the Poogie's menu: the choice list, the outfit list or the feeding
 * list; in the debug step the four bars, a pixel a frame, and the value of
 * the one chosen. Nothing while a find waits.
 */
void PoogieMenu_Draw(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    s16 x;
    s16 barX;
    s16 greatX;
    s32 waitLen;

    Text_SetSize(0);
    if (gPoogieErrand == POOGIE_ERRAND_NONE) {
        switch (menu->step) {
        case PET_STEP_CHOOSE:
            if (Game_IsOnline() == 0) {
                Menu_DrawLabelList(0x1CC, (s8 **)gPoogieMenuLabels, 3, menu->choice, 0xFFB0B000); /* pet, feed, the outfit */
            } else {
                Menu_DrawLabelList(0x1CC, (s8 **)gPoogieMenuLabels, 2, menu->choice, 0xFFB0B000);
            }
            break;
        case PET_STEP_RUN:
            if (menu->choice == PET_CHOICE_OUTFIT) {
                PoogieMenu_DrawOutfitList();
            } else if (menu->choice == PET_CHOICE_FEED) {
                switch (menu->feedStep) {
                case PET_FEED_SLIDE:
                    x = menu->win.pos;
                    break;
                case PET_FEED_LIST:
                    x = 0x118;
                    break;
                }
                if (menu->feedStep <= PET_FEED_LIST) {
                    PoogieMenu_DrawFeedList(x, 0x34);
                }
            }
            break;
        case PET_STEP_DEBUG_BARS:
            Font_SetSpacing(0x12, 0x12);
            Font_SetColor(0x807F7F28);
            Font_SetPos(0x14, 0xB4);
            switch (menu->debugBar) {
            case PET_BAR_WAIT:
                Text_PrintfColored(TXT(0x0082C990) /* the pet's mood bar label (green) */);
                break;
            case PET_BAR_JOY:
                Text_PrintfColored(TXT(0x0082C9B0) /* the pet's mood bar label (blue) */);
                break;
            case PET_BAR_GREAT_JOY:
                Text_PrintfColored(TXT(0x0082C9D0) /* the pet's mood bar label (red) */);
                break;
            case PET_BAR_ANGER:
                Text_PrintfColored(TXT(0x0082C9F0) /* the pet's mood bar label (purple) */);
                break;
            }
            /* the four bars in a row, a pixel a frame, the great joy drawn over the middle of the joy */
            barX = 0x14;
            waitLen = menu->bars[PET_BAR_WAIT];
            greatX = waitLen + (menu->bars[PET_BAR_JOY] - menu->bars[PET_BAR_GREAT_JOY]) / 2 + 0x15;
            Gfx_DrawLine(0x14, 0xC8, waitLen, 5, 0xFF36C018);
            barX += menu->bars[PET_BAR_WAIT] + 1;
            Gfx_DrawLine(barX, 0xC8, menu->bars[PET_BAR_JOY], 5, 0xFF0000FF);
            barX += menu->bars[PET_BAR_JOY] + 1;
            Gfx_DrawLine(greatX, 0xC8, menu->bars[PET_BAR_GREAT_JOY], 5, 0xFFFF0000);
            Gfx_DrawLine(barX, 0xC8, menu->bars[PET_BAR_ANGER], 5, 0xFFAA00FF);
            Font_SetPos(0x14, 0xD7);
            Text_PrintfColored(TXT(0x0082CA10) /* debug: the frame count */, menu->bars[menu->debugBar]);
            break;
        }
    }
}

/* Draws the outside and care menus: the choice list or the feeding list. */
void PoogieMenu_DrawPetFeed(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    s16 x;

    Text_SetSize(0);
    switch (menu->step) {
    case PET_STEP_CHOOSE:
        Menu_DrawLabelList(0x1CC, (s8 **)gPoogieCareLabels, 2, menu->choice, 0xFFB0B000); /* pet, feed */
        break;
    case PET_STEP_RUN:
        if (menu->choice == PET_CHOICE_FEED) {
            switch (menu->feedStep) {
            case PET_FEED_SLIDE:
                x = menu->win.pos;
                break;
            case PET_FEED_LIST:
                x = 0x118;
                break;
            }
            if (menu->feedStep < PET_FEED_EAT) {
                PoogieMenu_DrawFeedList(x, 0x34);
            }
        }
        break;
    }
}

s32 Motion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 Actor_HeadingDiff(void *obj, void *pos);
void Actor_SetMotion(void *pl, s32 motion, s32 frames, s32 c);
void ModelObj_SpawnOn(void *arg0, s32 arg1);

/*
 * Runs a frame of the petting of pet: counts the frames held (Player.cookTimer)
 * against the area's bars (gPetBars, or gPetBarsCare for the care pet)
 * into Player.cookResult (PET_RESULT_*), and turns the player to the pet.
 * On PAD_CIRCLE past the motion's frame 130, or when the time runs out, it
 * ends: the pet reacts and PETTING_NOTHING, _ANGRY or _PLEASED is
 * returned; PETTING_RUNNING before.
 */
s8 Poogie_UpdatePetting(Em *pet)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    s16 *bars;
    SaveData *save;
    u8 kind;
    s16 greatStart;

    pl->weaponShown = 0;
    save = &gSave;
    (pl->cookTimer)++;
    pl->targetYaw += (s16)(u16)Actor_HeadingDiff(pl, &((Em *)pl->target)->posX) / 5;
    kind = Pet_AreaKind();
    switch (kind) {
    case PET_AREA_CARE:
        bars = gPetBarsCare;
        break;
    case PET_AREA_HOUSE:
    case PET_AREA_POOGIE:
    default:
        bars = gPetBars;
        break;
    }
    if (pl->cookTimer >= bars[PET_BAR_WAIT] + bars[PET_BAR_JOY] + 1) {
        pl->cookResult = PET_RESULT_ANGER;
    } else {
        greatStart = bars[PET_BAR_WAIT] + (bars[PET_BAR_JOY] - bars[PET_BAR_GREAT_JOY]) / 2 + 1;
        if (pl->cookTimer >= greatStart && pl->cookTimer <= greatStart + bars[PET_BAR_GREAT_JOY]) {
            pl->cookResult = PET_RESULT_GREAT_JOY;
        } else if (pl->cookTimer >= bars[PET_BAR_WAIT] + 1) {
            if ((gPoogieHappy == 1 && kind == PET_AREA_POOGIE) || (gCarePetHappy == 1 && kind == PET_AREA_CARE)) {
                ModelObj_SpawnOn(pl, MODEL_OBJ_PET_JOY);
            }
            pl->cookResult = PET_RESULT_JOY;
        } else if (pl->cookTimer >= 180 /* frames: 6 s */) {
            pl->cookResult = PET_RESULT_LATE;
        } else {
            pl->cookResult = PET_RESULT_NONE;
        }
    }
    if ((Motion_ReachedFrame(pl, 0, 130.0f) && (pad & PAD_CIRCLE))
        || pl->cookTimer >= bars[PET_BAR_ANGER] + (bars[PET_BAR_WAIT] + bars[PET_BAR_JOY])) {
        switch (pl->cookResult) {
        case PET_RESULT_NONE:
        case PET_RESULT_LATE:
            Actor_SetMotion(pl, 0x26B, 4, 0); /* the player's petting ends */
            Jingle_Play(pl, PET_JINGLE_NOTHING);
            Bgm_FadeToVolume(0xF);
            return PETTING_NOTHING;
        case PET_RESULT_JOY:
            Actor_SetState(pet, 0, PET_STATE_JOY);
            Actor_SetMotion(pl, 0x26B, 4, 0); /* the player's petting ends */
            Jingle_Play(pl, PET_JINGLE_JOY);
            Bgm_FadeToVolume(0xF);
            return PETTING_PLEASED;
        case PET_RESULT_ANGER:
            Actor_SetState(pet, 0, PET_STATE_ANGRY);
            switch (Pet_AreaKind()) {
            case PET_AREA_CARE:
                Notice_PushItem(TXT(0x0082CA20) /* notice: the pet got angry (the special pet) */);
                break;
            case PET_AREA_HOUSE:
                Notice_PushItem(TXT(0x0082CA50) /* notice: the pet got angry (the sheep outfit) */);
                break;
            case PET_AREA_POOGIE:
            default:
                if ((s8)save->poogieName[0] != 0) {
                    Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CA70) /* notice: the pet got angry ($: its name) */, (char *)save->poogieName));
                }
                break;
            }
            Jingle_Play(pl, PET_JINGLE_ANGRY);
            Bgm_FadeToVolume(0xF);
            return PETTING_ANGRY;
        case PET_RESULT_GREAT_JOY:
            Actor_SetState(pet, 0, PET_STATE_GREAT_JOY);
            Actor_SetMotion(pl, 0x26B, 4, 0); /* the player's petting ends */
            Jingle_Play(pl, PET_JINGLE_JOY);
            Bgm_FadeToVolume(0xF);
            return PETTING_PLEASED;
        }
    }
    return PETTING_RUNNING;
}

/*
 * Poogie_UpdatePetting with the debug editor's bars (PoogieMenu.bars):
 * anger gives a debug notice, and the joy's mark always shows.
 */
s32 Poogie_UpdatePettingDebug(Em *pet)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    s16 greatStart;

    (pl->cookTimer)++;
    pl->targetYaw += (s16)(u16)Actor_HeadingDiff(pl, &((Em *)pl->target)->posX) / 5;
    if (pl->cookTimer >= menu->bars[PET_BAR_WAIT] + menu->bars[PET_BAR_JOY] + 1) {
        pl->cookResult = PET_RESULT_ANGER;
    } else {
        greatStart = menu->bars[PET_BAR_WAIT] + (menu->bars[PET_BAR_JOY] - menu->bars[PET_BAR_GREAT_JOY]) / 2 + 1;
        if (pl->cookTimer >= greatStart && pl->cookTimer <= greatStart + menu->bars[PET_BAR_GREAT_JOY]) {
            pl->cookResult = PET_RESULT_GREAT_JOY;
        } else if (pl->cookTimer >= menu->bars[PET_BAR_WAIT] + 1) {
            ModelObj_SpawnOn(pl, MODEL_OBJ_PET_JOY);
            pl->cookResult = PET_RESULT_JOY;
        } else if (pl->cookTimer >= 180 /* frames: 6 s */) {
            pl->cookResult = PET_RESULT_LATE;
        } else {
            pl->cookResult = PET_RESULT_NONE;
        }
    }
    if ((Motion_ReachedFrame(pl, 0, 130.0f) && (pad & PAD_CIRCLE))
        || pl->cookTimer >= menu->bars[PET_BAR_ANGER] + (menu->bars[PET_BAR_WAIT] + menu->bars[PET_BAR_JOY])) {
        switch (pl->cookResult) {
        case PET_RESULT_NONE:
        case PET_RESULT_LATE:
            Actor_SetMotion(pl, 0x26B, 4, 0); /* the player's petting ends */
            Jingle_Play(pl, PET_JINGLE_NOTHING);
            Bgm_FadeToVolume(0xF);
            return PETTING_NOTHING;
        case PET_RESULT_JOY:
            Actor_SetState(pet, 0, PET_STATE_JOY);
            Actor_SetMotion(pl, 0x26B, 4, 0); /* the player's petting ends */
            Jingle_Play(pl, PET_JINGLE_JOY);
            Bgm_FadeToVolume(0xF);
            return PETTING_PLEASED;
        case PET_RESULT_ANGER:
            Actor_SetState(pet, 0, PET_STATE_ANGRY);
            Notice_PushItem(TXT(0x0082CA90) /* debug notice: the pet got angry */);
            Jingle_Play(pl, PET_JINGLE_ANGRY);
            Bgm_FadeToVolume(0xF);
            return PETTING_ANGRY;
        case PET_RESULT_GREAT_JOY:
            Actor_SetState(pet, 0, PET_STATE_GREAT_JOY);
            Actor_SetMotion(pl, 0x26B, 4, 0); /* the player's petting ends */
            Jingle_Play(pl, PET_JINGLE_JOY);
            Bgm_FadeToVolume(0xF);
            return PETTING_PLEASED;
        }
    }
    return PETTING_RUNNING;
}

/* Returns the feeding list's rows on the current page: 1 when the list is empty. */
u8 PoogieMenu_RowsOnPage(PoogieMenu *menu)
{
    u8 rows;

    if (menu->page == menu->pages - 1) {
        if (menu->rows <= 0) {
            rows = 1;
        } else {
            rows = menu->rows - (menu->pages - 1) * PET_FEED_ROWS;
        }
    } else {
        rows = PET_FEED_ROWS;
    }
    return rows;
}

/* Clears the local player's petting count and result before a petting. */
void Poogie_ResetPetting(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    pl->cookTimer = 0;
    pl->cookResult = PET_RESULT_NONE;
}

/*
 * Fills owned[0..3] from EVENT_FLAG_POOGIE_OUTFIT_0..3 (the callers pass
 * &owned[1]: outfit 0 is always owned) and returns 1 when one is owned.
 */
s32 PoogieMenu_GetOutfits(u8 *owned)
{
    u8 i;
    s32 any = 0;

    for (i = 0; i < POOGIE_OUTFIT_COUNT - 1; i++) {
        owned[i] = 0;
        switch (i) {
        case 0:
            if (Save_TestEventFlag(EVENT_FLAG_POOGIE_OUTFIT_0) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        case 1:
            if (Save_TestEventFlag(EVENT_FLAG_POOGIE_OUTFIT_1) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        case 2:
            if (Save_TestEventFlag(EVENT_FLAG_POOGIE_OUTFIT_2) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        case 3:
            if (Save_TestEventFlag(EVENT_FLAG_POOGIE_OUTFIT_3) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        }
    }
    return any;
}

/* Draws the outfit list of the outfits owned, shaken by shake 0, at the cursor. */
void PoogieMenu_DrawOutfitList(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    s32 shake;

    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    Shake_Update(0);
    shake = (s32)Shake_GetOffset(0);
    gOutfitListBox.cursorColor = 0xA9182;
    PoogieMenu_BuildOutfitList();
    gOutfitListBox.items = menu->outfitNames;
    gOutfitListBox.lines = menu->outfitCount;
    ListPanel_Draw(&gOutfitListBox, (s16 *)shake, (char *)(TXT(0x0082CAB0) /* the outfit list's title, in colour 5 */), (u8)menu->outfitRow);
}

/* Lists the pouch's ingredients (Kitchen_IsIngredient) with their prices, from the first page. */
void PoogieMenu_BuildFeedList(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    u8 i;

    memset(menu->items, 0, sizeof(menu->items));
    memset(menu->prices, 0, sizeof(menu->prices));
    menu->rows = 0;
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (Kitchen_IsIngredient(gSave.pouch[i].item) == 1) {
            menu->items[menu->rows] = gSave.pouch[i].item;
            menu->prices[menu->rows] = gItemData[gSave.pouch[i].item].price;
            menu->rows++;
        }
    }
    menu->cursor = 0;
    menu->page = 0;
    menu->pages = Menu_CountPages(menu->rows, PET_FEED_ROWS);
    if (menu->rows <= 0) {
        menu->pages = 1;
    }
}

void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, s32 c1, s32 c2, s32 k); /* unit's own declaration: the definition takes s32 x and y and an s16 kind; declared so, the call passes its arguments as the game's code does */
void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 n, u32 color);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);

/*
 * Draws the feeding window at (x, y): the page's items with the counts
 * held, the cursor's bar and the item's description; or the notice that
 * the pouch holds no food.
 */
void PoogieMenu_DrawFeedList(s16 x, s16 y)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    s16 left = x;
    u8 i;
    s32 item;
    s32 selItem;
    u8 row;

    Win_Draw(x, y, 0x154, 0x120, 0xBDB59F6C, 0x80202020, 2);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x12E, 0x12, 0xC8, 0xFFB0B000, gListTitles[LIST_TITLE_INGREDIENT], 2, 1);
    if (menu->rows > 0) {
        Menu_DrawPageArrows(left + 0xF0, y + 10, menu->page, menu->pages, 0xBDB59F6C);
    }
    Font_SetColor(0x807F7F7F);
    if (menu->rows <= 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetColor(0x802B7F7F);
        Font_SetPos(x + 0x14, y + 0x64);
        Text_PrintfColored(TXT(0x0082CAC0) /* notice: no food in the item pouch */);
    } else {
        x += 0x32;
        y += 0x1E;
        Font_SetSpacing(0x14, 0x14);
        for (i = 0; i < PET_FEED_ROWS; i++) {
            row = i + menu->page * PET_FEED_ROWS;
            if (row >= menu->rows) {
                y += 0x16;
            } else {
                item = menu->items[row];
                if (i == menu->cursor) {
                    selItem = item;
                    Menu_DrawLine(left + 10, y - 1, 0x140, 0x18, 0);
                }
                Menu_DrawItemCountRow(x, y, item, 0x807F7F7F);
                y += 0x16;
            }
        }
    }
    if (menu->rows > 0) {
        Gfx_DrawLine(left + 0x14, y + 2, 0x12C, 2, 0xBDB59F6C);
        ItemInfo_Draw(x - 0x28, y + 4, 0x154, menu->showInfo, selItem, 0, -1, 0x807F7F7F);
    } else {
        Gfx_DrawLine(left + 0x14, y + 0xD0, 0x12C, 2, 0xBDB59F6C);
    }
}

/* Lists the names of the outfits owned, outfit 0's first. */
void PoogieMenu_BuildOutfitList(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    u8 i;
    u8 n = 0;

    menu->outfitNames[n++] = gPoogieOutfits[0].name;
    for (i = 1; i < POOGIE_OUTFIT_COUNT; i++) {
        if (menu->owned[i] == 1) {
            menu->outfitNames[n++] = gPoogieOutfits[i].name;
        }
    }
    menu->outfitCount = n;
}

/*
 * Returns 1 when the petting's reactions are over: the pet's joy or anger
 * motion, or after no reaction the player's motion. In the house it reads
 * the house pet.
 */
s32 PoogieMenu_ReactionDone(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    Em *npc;
    s8 result;

    if (gScene.area == AREA_HUB_0x57 || gScene.area == AREA_HOUSE) {
        result = menu->houseResult;
        npc = menu->housePet;
    } else {
        result = menu->petResult;
        npc = menu->pet;
    }
    switch (result) {
    case 2:
        if ((npc->state == PET_STATE_JOY || npc->state == PET_STATE_GREAT_JOY) && npc->motion[0].playing > 0) {
            return 0;
        }
        break;
    case 1:
        if (npc->state == PET_STATE_ANGRY && npc->motion[0].playing > 0) {
            return 0;
        }
        break;
    case 0:
        if (pl->motionId[0] == 0x26B && pl->motion[0].playing <= 0) {
            break;
        }
        return 0;
    default:
        return 1;
    }
    return 1;
}

/* Returns the outfit at the outfit list's cursor. */
u8 PoogieMenu_OutfitAtCursor(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    s8 left = menu->outfitRow;
    u8 i;

    for (i = 0; i < POOGIE_OUTFIT_COUNT; i++) {
        if (menu->owned[i] == 1) {
            left--;
            if (left < 0) {
                return i;
            }
        }
    }
    return 0;
}

/* Returns the outfit list's row of the outfit worn (SaveData.poogieOutfit). */
u8 PoogieMenu_WornOutfitRow(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    u8 rows = 0;
    u8 i;

    for (i = 0; i < gSave.poogieOutfit + 1; i++) {
        if (menu->owned[i] == 1) {
            rows++;
        }
    }
    return rows - 1;
}

/*
 * Feeds the chosen item: the pet comes and eats it after 150 frames; its
 * price is added to the area's counter. A full counter is emptied, the pet
 * is happy for the day (gPoogieHappy, gCarePetHappy) and very joyful.
 * Returns MENU_RUNNING, then MENU_DONE; MENU_DONE at once for the house pet.
 */
s32 PoogieMenu_Feed(void)
{
    PoogieMenu *menu = (PoogieMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 area = Pet_AreaKind();

    if (area == PET_AREA_HOUSE) {
        return MENU_DONE;
    }
    switch (menu->eatStep) {
    case 0: /* the pet comes */
        Actor_SetState(menu->pet, 0, PET_STATE_PETTED);
        menu->eatTimer = 150; /* frames: 5 s */
        menu->eatStep++;
        break;
    case 1: /* it eats */
        if (menu->eatTimer-- <= 0) {
            SavePouch_Add(menu->items[menu->chosenRow + menu->page * PET_FEED_ROWS], -1);
            Actor_SetMotion(pl, 0x26B, 4, 0); /* the player's petting ends */
            gLobby->petPoints[area] += menu->prices[menu->chosenRow + menu->page * PET_FEED_ROWS];
            if (gLobby->petPoints[area] >= gLobby->petPointsMax[area]) {
                Poogie_NotifyMood(PET_NOTICE_COUNTER);
                gLobby->petPoints[area] = 0;
                if (area == PET_AREA_CARE) {
                    gCarePetHappy = 1;
                } else {
                    gPoogieHappy = 1;
                }
                Actor_SetState(menu->pet, 0, PET_STATE_GREAT_JOY);
                Jingle_Play(pl, PET_JINGLE_JOY);
                Bgm_FadeToVolume(0xF);
            } else {
                if ((gPoogieHappy == 1 && area != PET_AREA_CARE) || (gCarePetHappy == 1 && area == PET_AREA_CARE)) {
                    Poogie_NotifyMood(PET_NOTICE_DELIGHTED);
                    Actor_SetState(menu->pet, 0, PET_STATE_GREAT_JOY);
                } else {
                    Poogie_NotifyMood(PET_NOTICE_COUNTER);
                    Actor_SetState(menu->pet, 0, PET_STATE_JOY);
                }
                Jingle_Play(pl, PET_JINGLE_JOY);
                Bgm_FadeToVolume(0xF);
            }
            return MENU_DONE;
        }
        pl->weaponShown = 0;
        pl->targetYaw += (s16)(u16)Actor_HeadingDiff(pl, &((Em *)pl->target)->posX) / 5;
        break;
    }
    return MENU_RUNNING;
}

/*
 * Finds the dig zone that holds pos, from *zone on (callers start at 1):
 * zones 1 to 5 are slabs of gPoogieDigZones; zone 6 holds the rest.
 */
void PoogieDig_FindZone(f32 *pos, u8 *zone)
{
    f32 cornerLo[3];
    f32 cornerHi[3];
    u8 row;

    if (*zone == 6) { /* the last zone: where none of the five holds pos */
        return;
    }
    row = (*zone - 1) * 5;
    cornerLo[0] = gPoogieDigZones[row];
    cornerLo[1] = -500.0f;
    cornerLo[2] = gPoogieDigZones[row + 1];
    cornerHi[0] = gPoogieDigZones[row + 2];
    cornerHi[1] = -500.0f;
    cornerHi[2] = gPoogieDigZones[row + 3];
    if (Slab_Contains(pos, cornerLo, cornerHi, 552.0f, gPoogieDigZones[row + 4]) == 1) {
        return;
    }
    (*zone)++;
    PoogieDig_FindZone(pos, zone);
}

extern u8 gDigSpotUses[];
extern u8 gDigOverLimit;

/*
 * Resets the pets' day once each time the lobby scene starts
 * (SysSettings.petDayPending): the dig spots' uses, the petting streak,
 * the dig, the pets that ran off, the happy and fed-season flags; the
 * unnamed Poogie follows; the two counters' limits are rolled.
 */
void Poogie_DailyReset(void)
{
    SaveData *save = &gSave;

    if (gSysSettings.petDayPending != 0) {
        gSysSettings.petDayPending = 0;
        gDigSpotUses[0] = 0;
        gDigSpotUses[1] = 0;
        gDigSpotUses[2] = 0;
        gDigSpotUses[3] = 0;
        gDigSpotUses[4] = 0;
        gDigSpotUses[5] = 0;
        gDigSpotUses[6] = 0;
        gPoogiePetStreak = 0;
        gPoogieDigState = POOGIE_DIG_NONE;
        gPoogieLeft = 0;
        gCarePetLeft = 0;
        gHousePetLeft = 0;
        gPoogieByPlayer = 0;
        gDigOverLimit = 0;
        gPoogieHappy = 0;
        gCarePetHappy = 0;
        gPoogieFedSeasonItem = 0;
        if (*(s8 *)save->poogieName == 0) {
            gPoogieFollowing = 1;
        } else {
            gPoogieFollowing = 0;
        }
        gLobby->petPointsMax[PET_AREA_POOGIE] = Poogie_RollFeedLimit(PET_AREA_POOGIE);
        gLobby->petPointsMax[PET_AREA_CARE] = Poogie_RollFeedLimit(PET_AREA_CARE);
    }
}

extern u16 gPoogieDigItems[];
extern u8 gPoogieDigChances[];
extern u16 gCareDigItems[];
extern u8 gCareDigChancesBreeding[];
extern u8 gCareDigChancesWarm[];
extern u8 gCareDigChancesCold[];
extern u8 gCareDigChancesFedBreeding[];
extern u8 gCareDigChancesFedWarm[];
extern u8 gCareDigChancesFedCold[];

/*
 * Rolls the item found at the current dig spot (gPoogieDigSpotIdx): the
 * Poogie's spots hold 4 items, the care pet's 6, each by a cumulative
 * percent (the care pet's by season, better once fed a food of the
 * season). Returns the item, the last one when the roll passes every
 * share, or 0 for another kind. The first parameter is not read.
 */
u16 PoogieDig_RollItem(u16 unused, u8 kind)
{
    u8 *chances;
    u8 count;
    u8 roll;
    u8 i;
    u8 spot;
    u8 row;
    u16 *items;

    if (kind == PET_AREA_POOGIE) {
        spot = gPoogieDigSpots[gPoogieDigSpotIdx];
        count = 4;
        items = &gPoogieDigItems[(u8)(spot * 4)];
        chances = &gPoogieDigChances[(u8)(spot * 3)];
    } else if (kind == PET_AREA_CARE) {
        spot = gPoogieDigSpotIdx;
        count = 6;
        row = spot * 5;
        items = &gCareDigItems[(u8)(spot * 6)];
        gPoogieDigSeason = LobbyWorld_GetSeason();
        if (gPoogieFedSeasonItem == 1) {
            switch (gPoogieDigSeason) {
            case SEASON_BREEDING:
            default:
                chances = &gCareDigChancesFedBreeding[row];
                break;
            case SEASON_WARM:
                chances = &gCareDigChancesFedWarm[row];
                break;
            case SEASON_COLD:
                chances = &gCareDigChancesFedCold[row];
                break;
            }
        } else {
            switch (gPoogieDigSeason) {
            case SEASON_BREEDING:
            default:
                chances = &gCareDigChancesBreeding[row];
                break;
            case SEASON_WARM:
                chances = &gCareDigChancesWarm[row];
                break;
            case SEASON_COLD:
                chances = &gCareDigChancesCold[row];
                break;
            }
        }
    } else {
        return 0;
    }
    roll = (u8)Rand_Next(1) % 100 + 1;
    for (i = 0; i < count; i++) {
        if (roll <= chances[i]) {
            return items[i];
        }
    }
    if (items[count - 1] != 0) {
        return items[count - 1];
    }
    return 0;
}

/* Returns 1 when item is a food of the season in gPoogieDigSeason. */
s32 Poogie_IsSeasonFood(u16 item)
{
    u8 count;
    u16 *list;
    u8 i;

    switch (gPoogieDigSeason) {
    case SEASON_BREEDING:
    default:
        count = gSeasonFoodCounts[0];
        list = gSeasonFoodsBreeding;
        break;
    case SEASON_WARM:
        count = gSeasonFoodCounts[1];
        list = gSeasonFoodsWarm;
        break;
    case SEASON_COLD:
        count = gSeasonFoodCounts[2];
        list = gSeasonFoodsCold;
        break;
    }
    for (i = 0; i < count; i++) {
        if (item == list[i]) {
            return 1;
        }
    }
    return 0;
}

/*
 * Returns 1 when a dig spot of kind's area was used more than its limit
 * for the day (the Poogie's 7 spots, the care pet's 4), or for another kind.
 */
s32 PoogieDig_OverLimit(u8 kind)
{
    u8 i;
    u8 *limits;
    u8 spots;

    if (kind == PET_AREA_POOGIE) {
        spots = 7;
        limits = gPoogieDigLimits;
    } else if (kind == PET_AREA_CARE) {
        spots = 4;
        limits = gCareDigLimits;
    } else {
        return 1;
    }
    for (i = 0; i < spots; i++) {
        /*
         * gDigSpotUses[i], reached from gPoogieDigZone, the start of the
         * dig's variables: the game adds the offset (0x17) in the load.
         */
        if (gPoogieDigZone[i + 0x17] > limits[i]) {
            return 1;
        }
    }
    return 0;
}

/* Rolls kind's counter limit for the day: one of four values by cumulative percents. */
s16 Poogie_RollFeedLimit(u8 kind)
{
    s32 *limits;
    u8 *upTo;
    u8 roll;

    switch (kind) {
    default:
        limits = gPoogieFeedLimits;
        upTo = gPoogieFeedLimitRates;
        break;
    case PET_AREA_CARE:
        limits = gCareFeedLimits;
        upTo = gCareFeedLimitRates;
        break;
    }
    roll = (u8)Rand_Next(1) % 100 + 1;
    if (roll <= upTo[0]) {
        return limits[0];
    }
    if (upTo[0] < roll && roll <= upTo[1]) {
        return limits[1];
    }
    if (upTo[1] < roll && roll <= upTo[2]) {
        return limits[2];
    }
    return limits[3];
}

/*
 * Shows the notice of a pet event (PET_NOTICE_*). The counter's notice
 * says how near the counter is to its limit (none for the house pet); the
 * notices that name the Poogie are skipped while it has no name.
 */
void Poogie_NotifyMood(u8 kind)
{
    SaveData *save = &gSave;
    u8 area = Pet_AreaKind();
    s16 cur;
    s16 max;

    if (kind == PET_NOTICE_COUNTER) {
        if (area != PET_AREA_HOUSE) {
            max = gLobby->petPointsMax[area];
            cur = gLobby->petPoints[area];
            if (cur >= max) {
                if (area == PET_AREA_CARE) {
                    Notice_PushItem(TXT(0x0082CAF0) /* notice: the pet is delighted (the special pet) */);
                } else if ((s8)save->poogieName[0] != 0) {
                    Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CB10) /* notice: the pet is delighted ($: its name) */, (char *)save->poogieName));
                }
            } else if (cur >= max / 2) {
                Notice_PushItem(TXT(0x0082CB20) /* the pet's mood: quite pleased */);
            } else {
                Notice_PushItem(TXT(0x0082CB40) /* the pet's mood: a little pleased */);
            }
        }
    } else if (kind == PET_NOTICE_STREAK) {
        switch (gPoogiePetStreak) {
        case 1:
            Notice_PushItem(TXT(0x0082CB60) /* the pet's mood: enjoying the petting */);
            break;
        case 2:
            if ((s8)save->poogieName[0] != 0) {
                Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CB10) /* notice: the pet is delighted ($: its name) */, (char *)save->poogieName));
            }
            break;
        }
    } else if (kind == PET_NOTICE_DELIGHTED) {
        if (area == PET_AREA_CARE) {
            Notice_PushItem(TXT(0x0082CAF0) /* notice: the pet is delighted (the special pet) */);
        } else if ((s8)save->poogieName[0] != 0) {
            Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CB10) /* notice: the pet is delighted ($: its name) */, (char *)save->poogieName));
        }
    } else { /* PET_NOTICE_HOUSE_PET */
        Notice_PushItem(TXT(0x0082CB80) /* notice: the pet is delighted (the sheep outfit) */);
    }
}

/* Returns 1 in the first three frames of the great joy's span (the joy's mark flashes). */
s32 Poogie_AtGreatJoyStart(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s16 *bars;
    s16 greatStart;

    switch (Pet_AreaKind()) {
    case PET_AREA_CARE:
        bars = gPetBarsCare;
        break;
    case PET_AREA_HOUSE:
    case PET_AREA_POOGIE:
    default:
        bars = gPetBars;
        break;
    }
    greatStart = bars[PET_BAR_WAIT] + (bars[PET_BAR_JOY] - bars[PET_BAR_GREAT_JOY]) / 2 + 1;
    if (pl->cookTimer >= greatStart && pl->cookTimer <= greatStart + 2) {
        return 1;
    }
    return 0;
}

/*
 * Rolls whether the care pet is in its area: a percent from
 * gNpcPresenceRates by the time of day (morning first) and the season.
 */
s32 Npc_RollPresence(void)
{
    u8 rate;
    u8 season;
    u8 roll;

    if (LobbyTime_IsAm() == 1) {
        rate = 0;
    } else {
        rate = 3;
    }
    switch (LobbyWorld_GetSeason()) {
    case SEASON_BREEDING:
    default:
        season = 0;
        break;
    case SEASON_WARM:
        season = 1;
        break;
    case SEASON_COLD:
        season = 2;
        break;
    }
    rate += season;
    roll = (u8)Rand_Next(1) % 100 + 1;
    return gNpcPresenceRates[rate] >= roll;
}

/* Returns the joy's frames (PET_BAR_JOY) of the area's bars, the joy mark's life; 0 once the petting has ended. */
s16 Poogie_JoyFrames(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    if (pl->motionId[0] == 0x26B) { /* the petting has ended */
        return 0;
    }
    switch ((u8)Pet_AreaKind()) {
    case PET_AREA_CARE:
        return gPetBarsCare[PET_BAR_JOY];
    case PET_AREA_HOUSE:
    case PET_AREA_POOGIE:
    default:
        return gPetBars[PET_BAR_JOY];
    }
}

/* Returns which pet the area holds (PET_AREA_*). */
u8 Pet_AreaKind(void)
{
    u8 kind;
    u16 area = gScene.area;

    if (area == AREA_HUB_0x83) {
        kind = PET_AREA_CARE;
    } else if (area == AREA_HUB_0x57 || area == AREA_HOUSE) {
        kind = PET_AREA_HOUSE;
    } else {
        kind = PET_AREA_POOGIE;
    }
    return kind;
}

/* The file's initialised data, in the game's order */
extern char gPetChoicePetText[];
extern char gPetChoiceFeedText[];
extern char gPetChoiceOutfitText[];
f32 gPoogieDigZones[28] = {
#include "lobby/007E7F10.inc"
};
ListPanel gOutfitListBox = {
#include "lobby/007E7F80.inc"
};
char *gPoogieMenuLabels[4] = {
    gPetChoicePetText, gPetChoiceFeedText, gPetChoiceOutfitText, NULL,
};
char *gPoogieCareLabels[4] = {
    gPetChoicePetText, gPetChoiceFeedText, NULL, NULL,
};
PoogieOutfit gPoogieOutfits[POOGIE_OUTFIT_COUNT] = {
#include "lobby/007E7FC0.inc"
};
u8 gPoogieDigSpotCounts[8] = {
#include "lobby/007E8060.inc"
};
u8 gPoogieDigSpotsZone1[8] = {
#include "lobby/007E8068.inc"
};
u8 gPoogieDigSpotsZone2[8] = {
#include "lobby/007E8070.inc"
};
u8 gPoogieDigSpotsZone3[8] = {
#include "lobby/007E8078.inc"
};
u8 gPoogieDigSpotsZone4[8] = {
#include "lobby/007E8080.inc"
};
u8 gPoogieDigSpotsZone5[8] = {
#include "lobby/007E8088.inc"
};
u8 gPoogieDigSpotsZone6[8] = {
#include "lobby/007E8090.inc"
};
u8 gPoogieDigLimits[8] = {
#include "lobby/007E8098.inc"
};
u8 gCareDigLimits[16] = {
#include "lobby/007E80A0.inc"
};
u16 gPoogieDigItems[32] = {
#include "lobby/007E80B0.inc"
};
u8 gPoogieDigChances[32] = {
#include "lobby/007E80F0.inc"
};
u16 gCareDigItems[24] = {
#include "lobby/007E8110.inc"
};
u8 gCareDigChancesBreeding[32] = {
#include "lobby/007E8140.inc"
};
u8 gCareDigChancesWarm[32] = {
#include "lobby/007E8160.inc"
};
u8 gCareDigChancesCold[32] = {
#include "lobby/007E8180.inc"
};
u8 gCareDigChancesFedBreeding[32] = {
#include "lobby/007E81A0.inc"
};
u8 gCareDigChancesFedWarm[32] = {
#include "lobby/007E81C0.inc"
};
u8 gCareDigChancesFedCold[24] = {
#include "lobby/007E81E0.inc"
};
s16 gPetJoyPoints[2] = {
#include "lobby/007E81F8.inc"
};
f32 gPoogieFinds[32] = {
#include "lobby/007E8200.inc"
};
u8 gCareDigSpotsBreeding[8] = {
#include "lobby/007E8280.inc"
};
u8 gCareDigSpotsWarm[8] = {
#include "lobby/007E8288.inc"
};
u8 gCareDigSpotsCold[8] = {
#include "lobby/007E8290.inc"
};
s16 gPetBars[4] = {
#include "lobby/007E8298.inc"
};
s16 gPetBarsCare[4] = {
#include "lobby/007E82A0.inc"
};
u8 gNpcPresenceRates[8] = {
#include "lobby/007E82A8.inc"
};
u16 gSeasonFoodsBreeding[16] = {
#include "lobby/007E82B0.inc"
};
u16 gSeasonFoodsWarm[8] = {
#include "lobby/007E82D0.inc"
};
u16 gSeasonFoodsCold[16] = {
#include "lobby/007E82E0.inc"
};
u8 gSeasonFoodCounts[16] = {
#include "lobby/007E8300.inc"
};
s32 gPoogieFeedLimits[4] = {
#include "lobby/007E8310.inc"
};
u8 gPoogieFeedLimitRates[16] = {
#include "lobby/007E8320.inc"
};
s32 gCareFeedLimits[4] = {
#include "lobby/007E8330.inc"
};
u8 gCareFeedLimitRates[16] = {
#include "lobby/007E8340.inc"
};
