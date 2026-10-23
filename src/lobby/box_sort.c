/*
 * The sort of the item box and the equipment box (gSave.itemBox, gSave.box):
 * Box_BuildSortedList builds the box as a sorted list (the items merged by
 * ItemList_Add and sorted by Sort_PairsDesc, the equipment by
 * BoxEquip_SortKey through BoxEquip_ShellSort), which the box lists draw
 * from. Box_ApplySort, from the item box menu's sort prompt, copies the
 * list over the box; for the equipment it first moves the worn pieces'
 * indexes and the ten sets' indexes to their pieces' new slots
 * (Box_FindEquip).
 *
 * Its data: the equipment's sort keys by armour type and by weapon class
 * (gBoxSortTypeKeys, gBoxSortClassKeys).
 */

#include "common.h"
#include "box_menu.h"
#include "libc.h"

extern u8 gSkillTryPlayer[];
struct SortPair;
void Sort_PairsDesc(struct SortPair *a, u32 n);

s32 gBoxSortTypeKeys[8] = {
#include "lobby/007F1CD0.inc"
};
s32 gBoxSortClassKeys[12] = {
#include "lobby/007F1CF0.inc"
};

/* The box as a list in gSkillTryPlayer: the items (merged by ItemList_Add, then
 * sorted by Sort_PairsDesc) or the used equipment (BoxEquip_ShellSort). Returns the
 * list, or the item box itself when an item could not be listed. */
void *Box_BuildSortedList(u8 equip)
{
    s32 n;
    SaveData *save = &gSave;

    memset(gSkillTryPlayer, 0, sizeof(gSave.box));
    n = Box_GetCapacity(save);
    if (equip == 0) {
        s32 k;
        ItemStack *srcStack = save->itemBox;
        ItemStack *list = (ItemStack *)gSkillTryPlayer;
        s32 i;

        for (k = n; k != 0; k--, srcStack++) {
            if (srcStack->item != 0 && ItemList_Add(list, srcStack->item, srcStack->count) != 0) {
                return save->itemBox;
            }
        }
        for (i = 0; n != 0; n--, i++, list++) {
            if (list->item == 0) {
                break;
            }
        }
        Sort_PairsDesc((struct SortPair *)((ItemStack *)gSkillTryPlayer), i);
        return gSkillTryPlayer;
    } else {
        EquipItem *srcItem = save->box;
        EquipItem *dstItem = (EquipItem *)gSkillTryPlayer;
        s32 i = 0;

        for (; n != 0; n--, srcItem++) {
            if (srcItem->used) {
                *dstItem = *srcItem;
                dstItem++;
                i++;
            }
        }
        BoxEquip_ShellSort((EquipItem *)gSkillTryPlayer, i);
        return gSkillTryPlayer;
    }
}

/* Sorts the item box (equip 0) or the equipment box (1): copies Box_BuildSortedList's list
 * over it. For the equipment, first moves the worn items' indexes (SaveData.equipIdx) and the
 * ten sets' indexes to where their items land (Box_FindEquip in the sorted list). */
void Box_ApplySort(u8 equip)
{
    SaveData *save = &gSave;
    void *buf;
    s32 n;
    u32 i;
    EquipSet *set;
    u16 *slot;
    u32 k;

    buf = Box_BuildSortedList(equip);
    n = Box_GetCapacity(save);
    if (equip == 0) {
        ItemStack *src = (ItemStack *)buf;
        ItemStack *dst = save->itemBox;

        for (; n != 0; n--) {
            *dst++ = *src++;
        }
    } else {
        EquipItem *src;
        EquipItem *dst;

        if (save->equipIdx[SAVE_EQUIP_WEAPON] != BOX_NONE) {
            save->equipIdx[SAVE_EQUIP_WEAPON] = Box_FindEquip(buf, &save->box[save->equipIdx[SAVE_EQUIP_WEAPON]]);
        }
        if (save->equipIdx[SAVE_EQUIP_LEGS] != BOX_NONE) {
            save->equipIdx[SAVE_EQUIP_LEGS] = Box_FindEquip(buf, &save->box[save->equipIdx[SAVE_EQUIP_LEGS]]);
        }
        if (save->equipIdx[SAVE_EQUIP_HEAD] != BOX_NONE) {
            save->equipIdx[SAVE_EQUIP_HEAD] = Box_FindEquip(buf, &save->box[save->equipIdx[SAVE_EQUIP_HEAD]]);
        }
        if (save->equipIdx[SAVE_EQUIP_BODY] != BOX_NONE) {
            save->equipIdx[SAVE_EQUIP_BODY] = Box_FindEquip(buf, &save->box[save->equipIdx[SAVE_EQUIP_BODY]]);
        }
        if (save->equipIdx[SAVE_EQUIP_ARMS] != BOX_NONE) {
            save->equipIdx[SAVE_EQUIP_ARMS] = Box_FindEquip(buf, &save->box[save->equipIdx[SAVE_EQUIP_ARMS]]);
        }
        if (save->equipIdx[SAVE_EQUIP_WAIST] != BOX_NONE) {
            save->equipIdx[SAVE_EQUIP_WAIST] = Box_FindEquip(buf, &save->box[save->equipIdx[SAVE_EQUIP_WAIST]]);
        }
        set = save->equipSets;
        for (k = EQUIP_SET_COUNT; k != 0; k--) {
            for (i = 0, slot = set->idx; i < SAVE_EQUIP_COUNT; i++) {
                if (*slot != BOX_NONE) {
                    *slot = Box_FindEquip(buf, &save->box[*slot]);
                }
                slot++;
            }
            set++;
        }
        src = (EquipItem *)buf;
        dst = save->box;
        for (; n != 0; n--) {
            *dst++ = *src++;
        }
    }
}

u8 Equip_GetClass(EquipItem *e);
extern s32 gBoxSortTypeKeys[];
extern s32 gBoxSortClassKeys[];

/* The equipment box's sort key (smaller first). A weapon: its class's key (gBoxSortClassKeys
 * by Equip_GetClass) + its data's sort order << 16 + id * 16; an armour piece: its type's key
 * (gBoxSortTypeKeys) + its sort order << 16 + id << 8 + its level (low 4 bits). Both add the
 * decorations set in it (its slots less the free ones). */
u32 BoxEquip_SortKey(EquipItem *item)
{
    u32 key;
    ArmorData *data;

    key = Equip_GetSlotCount(item);
    key -= Equip_FreeSlots(item);
    if (item->type == EQUIP_BLADE || item->type == EQUIP_GUN) {
        key += item->id * 16;
        if (item->type == EQUIP_BLADE) {
            key += Equip_GetData(item)->blade.sortOrder << 16;
        } else {
            key += Equip_GetData(item)->gun.sortOrder << 16;
        }
        key += gBoxSortClassKeys[Equip_GetClass(item)];
    } else {
        data = &Equip_GetData(item)->armor;
        key += item->level & 0xF;
        key += item->id << 8;
        key += data->sortOrder << 16;
        key += gBoxSortTypeKeys[item->type];
    }
    return key;
}

/* Shell sort of the equipment list by BoxEquip_SortKey (gaps 1, 4, 13, ...). */
void BoxEquip_ShellSort(EquipItem *list, u32 n)
{
    s32 i;
    s32 j;
    s32 gap;
    EquipItem held;
    u32 key;

    gap = 1;
    while (gap < n) {
        gap = gap * 3 + 1;
    }
    for (gap /= 3; gap > 0; gap /= 3) {
        for (i = gap; i < n; i++) {
            held = list[i];
            key = BoxEquip_SortKey(&held);
            for (j = i - gap; j >= 0; j -= gap) {
                if (key >= BoxEquip_SortKey(&list[j])) {
                    break;
                }
                list[j + gap] = list[j];
            }
            list[j + gap] = held;
        }
    }
}

/* Returns the index in box (Box_GetCapacity entries) of the first used item equal to item
 * (type, id, level, decorations), or BOX_NONE when item is unused or not there. */
u16 Box_FindEquip(EquipItem *box, EquipItem *item)
{
    s16 cap;
    u16 i;

    cap = Box_GetCapacity(&gSave);
    if (item->used == 0) {
        return BOX_NONE;
    }
    for (i = 0; i < cap; i++, box++) {
        if (box->used != 0 && box->type == item->type && box->id == item->id && box->level == item->level
            && box->deco[0] == item->deco[0] && box->deco[1] == item->deco[1] && box->deco[2] == item->deco[2]) {
            return i;
        }
    }
    return BOX_NONE;
}
