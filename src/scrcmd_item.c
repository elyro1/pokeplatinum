#include "scrcmd_item.h"

#include <nitro.h>
#include <string.h>

#include "constants/heap.h"
#include "constants/items.h"

#include "field/field_system.h"

#include "bag.h"
#include "field_script_context.h"
#include "inlines.h"
#include "item.h"
#include "special_encounter.h"
#include "unk_0205DFC4.h"
#include "vars_flags.h"

BOOL ScrCmd_AddItem(ScriptContext *ctx)
{
    FieldSystem *fieldSystem = ctx->fieldSystem;
    u16 item = ScriptContext_GetVar(ctx);
    u16 count = ScriptContext_GetVar(ctx);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);

    *destVar = Bag_TryAddItem(SaveData_GetBag(fieldSystem->saveData), item, count, HEAP_ID_FIELD1);
    return FALSE;
}

BOOL ScrCmd_RemoveItem(ScriptContext *ctx)
{
    FieldSystem *fieldSystem = ctx->fieldSystem;
    u16 item = ScriptContext_GetVar(ctx);
    u16 count = ScriptContext_GetVar(ctx);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);

    *destVar = Bag_TryRemoveItem(SaveData_GetBag(fieldSystem->saveData), item, count, HEAP_ID_FIELD1);
    return FALSE;
}

BOOL ScrCmd_CanFitItem(ScriptContext *ctx)
{
    FieldSystem *fieldSystem = ctx->fieldSystem;
    u16 item = ScriptContext_GetVar(ctx);
    u16 count = ScriptContext_GetVar(ctx);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);

    *destVar = Bag_CanFitItem(SaveData_GetBag(fieldSystem->saveData), item, count, HEAP_ID_FIELD1);
    return FALSE;
}

BOOL ScrCmd_CheckItem(ScriptContext *ctx)
{
    FieldSystem *fieldSystem = ctx->fieldSystem;
    u16 item = ScriptContext_GetVar(ctx);
    u16 count = ScriptContext_GetVar(ctx);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);

    *destVar = Bag_CanRemoveItem(SaveData_GetBag(fieldSystem->saveData), item, count, HEAP_ID_FIELD2);
    return FALSE;
}

BOOL ScrCmd_GetItemQuantity(ScriptContext *ctx)
{
    FieldSystem *fieldSystem = ctx->fieldSystem;
    u16 item = ScriptContext_GetVar(ctx);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);

    *destVar = Bag_GetItemQuantity(SaveData_GetBag(fieldSystem->saveData), item, HEAP_ID_FIELD2);
    return FALSE;
}

BOOL ScrCmd_IsItemTMHM(ScriptContext *ctx)
{
    FieldSystem *fieldSystem = ctx->fieldSystem;
    u16 item = ScriptContext_GetVar(ctx);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);

    *destVar = Item_IsTMHM(item);
    return FALSE;
}

BOOL ScrCmd_GetItemPocket(ScriptContext *ctx)
{
    FieldSystem *fieldSystem = ctx->fieldSystem;
    u16 item = ScriptContext_GetVar(ctx);
    u16 *pocket = ScriptContext_GetVarPointer(ctx);

    *pocket = (u16)Item_LoadParam(item, ITEM_PARAM_FIELD_POCKET, HEAP_ID_FIELD2);
    return FALSE;
}

BOOL ScrCmd_Dummy081(ScriptContext *ctx)
{
    return FALSE;
}

BOOL ScrCmd_Dummy082(ScriptContext *ctx)
{
    return FALSE;
}

static BOOL IsRepel(u16 item)
{
    return item == ITEM_REPEL || item == ITEM_SUPER_REPEL || item == ITEM_MAX_REPEL;
}

BOOL ScrCmd_GetNextRepel(ScriptContext *ctx)
{
    SaveData *saveData = ctx->fieldSystem->saveData;
    Bag *bag = SaveData_GetBag(saveData);
    u16 lastRepel = *VarsFlags_GetVarAddress(SaveData_GetVarsFlags(saveData), VAR_LAST_REPEL_USED);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);

    *destVar = ITEM_NONE;
    if (IsRepel(lastRepel) && Bag_CanRemoveItem(bag, lastRepel, 1, HEAP_ID_FIELD2)) {
        *destVar = lastRepel;
        return FALSE;
    }

    // If the previous type is gone (or this is an older save), offer an available repel.
    static const u16 repels[] = { ITEM_REPEL, ITEM_SUPER_REPEL, ITEM_MAX_REPEL };
    for (int i = 0; i < NELEMS(repels); i++) {
        if (Bag_CanRemoveItem(bag, repels[i], 1, HEAP_ID_FIELD2)) {
            *destVar = repels[i];
            break;
        }
    }

    return FALSE;
}

BOOL ScrCmd_UseRepel(ScriptContext *ctx)
{
    SaveData *saveData = ctx->fieldSystem->saveData;
    u16 item = ScriptContext_GetVar(ctx);
    u16 *destVar = ScriptContext_GetVarPointer(ctx);
    SpecialEncounter *encounters = SaveData_GetSpecialEncounters(saveData);

    *destVar = FALSE;
    if (IsRepel(item) && SpecialEncounter_RepelStepsEmpty(encounters)
        && Bag_TryRemoveItem(SaveData_GetBag(saveData), item, 1, HEAP_ID_FIELD2)) {
        *SpecialEncounter_GetRepelSteps(encounters) = Item_LoadParam(item, ITEM_PARAM_EFFECT_PARAM, HEAP_ID_FIELD2);
        *VarsFlags_GetVarAddress(SaveData_GetVarsFlags(saveData), VAR_LAST_REPEL_USED) = item;
        *destVar = TRUE;
    }

    return FALSE;
}
