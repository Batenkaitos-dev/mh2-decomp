/*
 * The two menus the player's state runs during a quest: the supply box
 * (SceneState.supply: 40 slots, opened in steps as the deliveries are made;
 * online, a take is asked of the host) and the quest's delivery box (a
 * choice, a quantity, a confirmation). Then three small queries the
 * player's states and the HUD use: whether a trader is near, whether the
 * pouch holds meat, and the jingle for using an item.
 */
#include "common.h"
#include "quest.h"
#include "hud.h"
#include "player.h"
#include "pad.h"
#include "sound.h"
#include "em.h"
#include "objhead.h"
#include "area_draw.h"
#include "item.h"
#include "area.h"
#include "bgm.h"
#include "qsync.h"
#include "supply_box.h"
#include "engine_math.h"
#include "game_util.h"
#include "msg_event.h"
extern Em gEms[];
u8 Pouch_Add(Player *pl, u16 item, s16 n);
/* end of prototypes */

/* Takes the item under the supply box cursor (or asks the host, online). */
void SupplyBox_Take(Player *pl)
{
    SceneState *scene = &gScene;
    u16 item;

    pl->menuDelay = 30;
    Hud_StartSupplyMsg((u8 *)pl);
    if (Game_IsOnline() == 1) {
        pl->supplyBoxTimer = 900;
        pl->supplyTaken = 1;
        QSync_SendSupply(1, gScene.localPlayer);
        return;
    }
    pl->supplyBoxTimer = 0;
    pl->supplyTaken = 0;
    Pouch_Add(pl, scene->supply[pl->menuCursor].item, scene->supply[pl->menuCursor].count);
    scene->supplyTaken[pl->menuCursor / 32] |= 1 << (pl->menuCursor % 32);
    SupplyBox_StartMove(scene->supply[pl->menuCursor].item, pl->menuCursor);
    item = scene->supply[pl->menuCursor].item;
    switch (item) {
    case ITEM_ID_TRADE_GOODS:
        Notice_Push(1, 19, item);
        break;
    case ITEM_ID_DELIVERY:
        Notice_Push(1, 20, item);
        break;
    }
}

/* Whether a supply box slot is available yet: the first 24 at once (unless
 * gQuest.timedEvent), then 24-31 and 32-39 with the deliveries (QuestGoal_IsDone). */
s32 SupplyBox_IsOpen(u8 slot)
{
    if (slot < 24) {
        if (gQuest.timedEvent != 0) {
            return 0;
        }
    } else if (slot < 32) {
        if (QuestGoal_IsDone(1) == 0) {
            return 0;
        }
    } else if (QuestGoal_IsDone(2) == 0) {
        return 0;
    }
    return 1;
}

/* The supply box menu: 40 slots, 8 to a row. */
void SupplyBox_Update(Player *pl)
{
    s16 pressed;
    SceneState *scene = &gScene;
    s16 buttons;

    if (Player_IsLocal(pl) != 0) {
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        if (pl->menuDelay != 0) {
            pl->menuDelay--;
        }
        if (pl->supplyBoxTimer != 0) {
            pl->supplyBoxTimer--;
            if (pl->supplyBoxTimer == 0) {
                pl->supplyTaken = 0;
            }
        }
        if (pl->supplyTaken != 0 || pl->menuDelay != 0) {
            return;
        }
        if (buttons & PAD_CROSS) {
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            pl->supplyBoxOpen = 0;
            MsgEvent_Check(2);
            return;
        }
        if (buttons & PAD_LEFT) {
            Sound_PlaySeFlat(0, 22, 0);
            if (pl->menuCursor & 7) {
                pl->menuCursor--;
            } else {
                pl->menuCursor += 7;
            }
        }
        if (buttons & PAD_RIGHT) {
            Sound_PlaySeFlat(0, 22, 0);
            if (pl->menuCursor % 8 != 7) {
                pl->menuCursor++;
            } else {
                pl->menuCursor -= 7;
            }
        }
        if (buttons & PAD_UP) {
            Sound_PlaySeFlat(0, 22, 0);
            if (pl->menuCursor < 8) {
                pl->menuCursor += 32;
            } else {
                pl->menuCursor -= 8;
            }
        }
        if (buttons & PAD_DOWN) {
            Sound_PlaySeFlat(0, 22, 0);
            if (pl->menuCursor >= 32) {
                pl->menuCursor -= 32;
            } else {
                pl->menuCursor += 8;
            }
        }
        if (!(scene->supplyTaken[pl->menuCursor / 32] & (1 << (pl->menuCursor % 32))) && SupplyBox_IsOpen(pl->menuCursor) == 1 &&
            (pressed & PAD_CIRCLE) && scene->supply[pl->menuCursor].item != 0 && scene->supply[pl->menuCursor].count != 0 &&
            pl->menuDelay == 0) {
            if (Pouch_Count(pl, scene->supply[pl->menuCursor].item) == 0) {
                if (Pouch_HasFreeSlot(pl) != 0) {
                    Sound_PlaySeFlat(0, 25, 0);
                    SupplyBox_Take(pl);
                }
            } else if (Pouch_Room(pl, scene->supply[pl->menuCursor].item) >= scene->supply[pl->menuCursor].count) {
                Sound_PlaySeFlat(0, 25, 0);
                SupplyBox_Take(pl);
            }
        }
    }
}

/* A three-item menu (page 0: choose; page 1: confirm). */
void Player_RunDeliveryMenu(Player *pl)
{
    s16 pressed;
    s16 buttons;

    if (Player_IsLocal(pl) != 0) {
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        switch (pl->menuPage) {
        case 0:
            if (pl->menuDelay != 0) {
                pl->menuDelay--;
            }
            if (pl->menuDelay != 0) {
                break;
            }
            if (pressed & PAD_CROSS) {
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                pl->deliveryMenu = 0;
            } else if (pressed & PAD_CIRCLE) {
                switch (pl->menuCursor) {
                case 0:
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    pl->deliveryMenu = 1;
                    break;
                case 1:
                    if (Quest_CheckSubClearMode() == 1 && Quest_PhaseIsOver(0) != 1) {
                        pl->menuPage++;
                        pl->yesNoCursor = 1;
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                    break;
                case 2:
                    if (Quest_PhaseIsOver(0) != 1) {
                        pl->menuPage++;
                        pl->yesNoCursor = 1;
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                    break;
                }
            } else {
                if (buttons & PAD_UP) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor <= 0) {
                        pl->menuCursor = 2;
                    } else {
                        pl->menuCursor--;
                    }
                }
                if (buttons & PAD_DOWN) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor >= 2) {
                        pl->menuCursor = 0;
                    } else {
                        pl->menuCursor++;
                    }
                }
            }
            break;
        case 1:
            if (pl->yesNoCursor == 0) {
                if (buttons & PAD_CROSS) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else if (buttons & PAD_CIRCLE) {
                    switch (pl->menuCursor) {
                    case 1:
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        pl->deliveryMenu = 1;
                        break;
                    case 2:
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        pl->deliveryMenu = 1;
                        break;
                    }
                } else if (buttons & PAD_RIGHT) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & (PAD_CROSS | PAD_CIRCLE)) {
                pl->menuPage = 0;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_LEFT) {
                pl->yesNoCursor = 0;
                Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            }
            break;
        }
    }
}

/* A menu to take a quantity of an item (page 0: the item, 1: how many,
 * 2: confirm); Quest_GetDeliveryTarget gives the item and count of a choice. */
void Quest_DeliveryMenu(Player *pl)
{
    HudState *menu = &gHudStateBuf;
    s16 pressed;
    s16 buttons;
    ItemStack sel;
    s16 have;
    s16 max;

    if (Player_IsLocal(pl) != 0) {
        if (Quest_PhaseIsOver(0) == 1) {
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            pl->deliveryOpen = 0;
            return;
        }
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        switch (pl->menuPage) {
        case 0:
            if (pl->menuDelay != 0) {
                pl->menuDelay--;
            }
            if (pl->menuDelay != 0) {
                return;
            }
            if (pressed & PAD_CROSS) {
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                pl->deliveryOpen = 0;
                return;
            }
            if (pressed & PAD_CIRCLE) {
                if (Quest_GetDeliveryTarget(pl, pl->menuCursor, &sel) == 2) {
                    have = Pouch_Count(pl, sel.item);
                    if (have > 0) {
                        pl->menuPage++;
                        max = sel.count;
                        if (have >= max) {
                            menu->atMax = max;
                        } else {
                            menu->atMax = have;
                        }
                        menu->count = menu->atMax;
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                } else {
                    Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                }
            } else {
                if (buttons & PAD_UP) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor <= 0) {
                        pl->menuCursor = 2;
                    } else {
                        pl->menuCursor--;
                    }
                }
                if (buttons & PAD_DOWN) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor >= 2) {
                        pl->menuCursor = 0;
                    } else {
                        pl->menuCursor++;
                    }
                }
            }
            break;
        case 1:
            if (pressed & PAD_CROSS) {
                pl->menuPage = 0;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            } else if (pressed & PAD_CIRCLE) {
                pl->menuPage++;
                pl->yesNoCursor = 1;
                Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
            } else if (pressed & PAD_RIGHT) {
                if (menu->count >= menu->atMax) {
                    menu->count = menu->atMax;
                } else {
                    menu->count++;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            } else if (pressed & PAD_LEFT) {
                if (menu->count < 2) {
                    menu->count = 1;
                } else {
                    menu->count--;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            }
            break;
        case 2:
            if (pl->yesNoCursor == 0) {
                if (buttons & PAD_CROSS) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else if (buttons & PAD_CIRCLE) {
                    Quest_GetDeliveryTarget(pl, pl->menuCursor, &sel);
                    Quest_DeliverItem(pl, sel.item, menu->count);
                    pl->deliveryOpen = 0;
                } else if (buttons & PAD_DOWN) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & (PAD_CROSS | PAD_CIRCLE)) {
                pl->menuPage = 1;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_UP) {
                pl->yesNoCursor = 0;
                Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            }
            break;
        }
        menu->yesNo = pl->yesNoCursor;
    }
}

/* 1 when a trader (EM_KIND_TRADER) is within 300 of the object. */
s32 Player_NearTrader(void *obj)
{
    ObjHead *head = obj;
    Em *em;
    s16 i;

    for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
        if (em->type == EM_KIND_TRADER && Vec_Distance(head->pos, &em->posX) <= 300.0f) {
            return 1;
        }
    }
    return 0;
}

/* 1 when the pouch holds ITEM_ID_MEAT (the spit roast needs one). */
s32 Pouch_HasMeat(Player *pl)
{
    return Pouch_Count(pl, ITEM_ID_MEAT) != 0;
}

/* The local player's jingle for using an item, by its ItemData.useSound. */
void Item_PlayUseSound(Player *pl, u16 item)
{
    if (Player_IsLocal(pl) != 0) {
        switch (gItemData[item].useSound) {
        case 0:
            Jingle_Play(pl, 7);
            break;
        case 2:
            Jingle_Play(pl, 8);
            break;
        case 3:
            Jingle_Play(pl, 183);
            break;
        }
    }
}

