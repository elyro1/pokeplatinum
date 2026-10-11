#include "port/debug_field.h"

#include <nitro.h>

#include "field/field_system.h"

#include "poketch.h"

static FieldSystem *s_fieldSystem = NULL;

FieldSystem *DEBUG_GetFieldSystem()
{
    return s_fieldSystem;
}

void DEBUG_SetFieldSystem(FieldSystem *fieldSystem)
{
    s_fieldSystem = fieldSystem;
}

int DEBUG_GetPoketchAppCount(void)
{
    if (s_fieldSystem == NULL || !s_fieldSystem->runningFieldMap) {
        return -1;
    }

    Poketch *poketch = SaveData_GetPoketch(s_fieldSystem->saveData);
    return Poketch_IsEnabled(poketch) ? poketch->appCount : -1;
}

BOOL DEBUG_UnlockAllPoketchApps(void)
{
    if (DEBUG_GetPoketchAppCount() < 0) {
        return FALSE;
    }

    Poketch *poketch = SaveData_GetPoketch(s_fieldSystem->saveData);
    for (int appID = 0; appID < POKETCH_APPID_MAX; appID++) {
        Poketch_RegisterApp(poketch, appID);
    }

    return TRUE;
}
