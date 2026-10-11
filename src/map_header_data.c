#include "map_header_data.h"

#include <nitro.h>
#include <string.h>

#include "constants/field/map.h"
#include "constants/field/map_matrix.h"
#include "constants/heap.h"
#include "constants/versions.h"
#include "generated/map_headers.h"
#include "generated/trainer_types.h"

#include "field/field_system.h"
#include "overlay006/wild_encounters.h"

#include "heap.h"
#include "map_header.h"
#include "map_matrix.h"
#include "map_object.h"
#include "narc.h"
#include "script_manager.h"

static void MapHeaderData_LoadEvents(MapHeaderData *data, enum MapHeaderID mapHeaderID);
static void MapHeaderData_ParseEvents(MapHeaderData *data);
static void MapHeaderData_LoadInitScripts(MapHeaderData *data, int headerID);

void MapHeaderData_Init(FieldSystem *fieldSystem, enum HeapID heapID)
{
    GF_ASSERT(fieldSystem->mapHeaderData == NULL);
    fieldSystem->mapHeaderData = Heap_Alloc(heapID, sizeof(MapHeaderData));
}

void MapHeaderData_Free(FieldSystem *fieldSystem)
{
    GF_ASSERT(fieldSystem->mapHeaderData != NULL);
    Heap_Free(fieldSystem->mapHeaderData);
}

void MapHeaderData_Load(FieldSystem *fieldSystem, enum MapHeaderID mapHeaderID)
{
    GF_ASSERT(fieldSystem->mapHeaderData != NULL);

    MapHeaderData_LoadEvents(fieldSystem->mapHeaderData, mapHeaderID);
    MapHeaderData_ParseEvents(fieldSystem->mapHeaderData);
#ifdef SDK_PORT
    fieldSystem->mapHeaderData->originalObjectEventCount = fieldSystem->mapHeaderData->numObjectEvents;
#endif
    MapHeaderData_LoadWildEncounters(&fieldSystem->mapHeaderData->wildEncounters, mapHeaderID);
    MapHeaderData_LoadInitScripts(fieldSystem->mapHeaderData, mapHeaderID);
}

static void MapHeaderData_LoadEvents(MapHeaderData *data, enum MapHeaderID mapHeaderID)
{
    int eventsID = MapHeader_GetEventsArchiveID(mapHeaderID);
    GF_ASSERT(NARC_GetMemberSizeByIndexPair(NARC_INDEX_FIELDDATA__EVENTDATA__ZONE_EVENT, eventsID) < sizeof(data->tmpEventsBuf));
    NARC_ReadWholeMemberByIndexPair(data->tmpEventsBuf, NARC_INDEX_FIELDDATA__EVENTDATA__ZONE_EVENT, eventsID);
}

void sub_0203A418(FieldSystem *fieldSystem)
{
    int numObjectEvents = fieldSystem->mapHeaderData->numObjectEvents;

    GF_ASSERT(fieldSystem->mapHeaderData != NULL);

    if (numObjectEvents != 0) {
        sub_02062068(fieldSystem->mapObjMan, fieldSystem->location->mapHeaderID, numObjectEvents, fieldSystem->mapHeaderData->objectEvents);
    }
}

#ifdef SDK_PORT
BOOL MapHeaderData_IsSameObjectSpawn(const ObjectEvent *a, const ObjectEvent *b)
{
    // Some route halves author the same ordinary NPC/sign twice, with different
    // local IDs, dialogue, directions and even height hints (Route 204). Their
    // spawn and movement bounds identify that shared object. Trainers retain
    // their explicit event identity, as do separate spawns of the same sprite.
    return a->trainerType == TRAINER_TYPE_NONE && b->trainerType == TRAINER_TYPE_NONE
        && a->x == b->x && a->z == b->z && a->graphicsID == b->graphicsID
        && a->movementType == b->movementType
        && a->movementRangeX == b->movementRangeX && a->movementRangeZ == b->movementRangeZ
        && memcmp(a->data, b->data, sizeof(a->data)) == 0;
}

// Cover the widest field view (448 pixels) plus movement and sprite margins.
// Use the whole current zone so no objects need to be created while walking
// within it. The existing scriptless-object handoff preserves NPC movement
// when entering or leaving the zone that owns the NPC.
static BOOL MapHeaderData_IsObjectNearZone(const FieldSystem *fieldSystem, const ObjectEvent *objectEvent)
{
    int width = MapMatrix_GetWidth(fieldSystem->mapMatrix);
    int height = MapMatrix_GetHeight(fieldSystem->mapMatrix);
    int minX = (objectEvent->x > 24 ? objectEvent->x - 24 : 0) / MAP_TILES_COUNT_X;
    int minZ = (objectEvent->z > 16 ? objectEvent->z - 16 : 0) / MAP_TILES_COUNT_Z;
    int maxX = (objectEvent->x + 24) / MAP_TILES_COUNT_X;
    int maxZ = (objectEvent->z + 16) / MAP_TILES_COUNT_Z;

    for (int z = minZ; z <= maxZ && z < height; z++) {
        for (int x = minX; x <= maxX && x < width; x++) {
            if (MapMatrix_GetMapHeaderIDAtCoords(fieldSystem->mapMatrix, x, z) == fieldSystem->location->mapHeaderID) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void MapHeaderData_PreloadNeighborObjects(FieldSystem *fieldSystem)
{
    // Everywhere covers disconnected empty cells across the whole matrix;
    // treating it as a zone would stream neighbors from all over Sinnoh.
    if (fieldSystem->location->mapHeaderID == MAP_HEADER_EVERYWHERE
        || !MapHeader_IsOnMainMatrix(fieldSystem->location->mapHeaderID)
        || MapMatrix_GetMatrixID(fieldSystem->mapMatrix) != 0) {
        return;
    }

    MapHeaderData *data = fieldSystem->mapHeaderData;
    int width = MapMatrix_GetWidth(fieldSystem->mapMatrix);
    int height = MapMatrix_GetHeight(fieldSystem->mapMatrix);
    u16 neighbors[MAP_MATRIX_MAX_SIZE];
    int neighborCount = 0;

    // Deduplicate adjacent headers: a route can span many matrix cells.
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            if (MapMatrix_GetMapHeaderIDAtCoords(fieldSystem->mapMatrix, x, z) != fieldSystem->location->mapHeaderID) {
                continue;
            }
            for (int dz = -1; dz <= 1; dz++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, nz = z + dz;
                    if (nx < 0 || nz < 0 || nx >= width || nz >= height) {
                        continue;
                    }
                    u16 header = MapMatrix_GetMapHeaderIDAtCoords(fieldSystem->mapMatrix, nx, nz);
                    if (header == MAP_HEADER_EVERYWHERE || header == fieldSystem->location->mapHeaderID) {
                        continue;
                    }
                    int i;
                    for (i = 0; i < neighborCount && neighbors[i] != header; i++) { }
                    if (i == neighborCount) {
                        neighbors[neighborCount++] = header;
                    }
                }
            }
        }
    }

    GF_ASSERT(data->originalObjectEventCount <= NELEMS(data->streamedObjectEvents));
    if (data->originalObjectEventCount != 0 && data->objectEvents != data->streamedObjectEvents) {
        memcpy(data->streamedObjectEvents, data->objectEvents, data->originalObjectEventCount * sizeof(ObjectEvent));
    }
    data->numObjectEvents = data->originalObjectEventCount;
    data->objectEvents = data->streamedObjectEvents;
    memset(data->streamedObjectSourceScripts, 0xFF, sizeof(data->streamedObjectSourceScripts));

    MapHeaderData *neighbor = Heap_AllocAtEnd(HEAP_ID_FIELD2, sizeof(MapHeaderData));
    for (int i = 0; i < neighborCount; i++) {
        MapHeaderData_LoadEvents(neighbor, neighbors[i]);
        MapHeaderData_ParseEvents(neighbor);
        for (int j = 0; j < neighbor->numObjectEvents; j++) {
            ObjectEvent event = neighbor->objectEvents[j];
            if (event.script == 0xFFFF || FieldSystem_CheckFlag(fieldSystem, event.hiddenFlag) || !MapHeaderData_IsObjectNearZone(fieldSystem, &event)) {
                continue;
            }
            BOOL duplicate = FALSE;
            for (int k = 0; k < data->numObjectEvents; k++) {
                const ObjectEvent *existing = &data->objectEvents[k];
                if (existing->localID == event.localID && existing->script == 0xFFFF && existing->hiddenFlag == neighbors[i]) {
                    data->streamedObjectSourceScripts[k] = event.script;
                    duplicate = TRUE;
                    break;
                }
                if (MapHeaderData_IsSameObjectSpawn(existing, &event)
                    && (existing->script == 0xFFFF || !FieldSystem_CheckFlag(fieldSystem, existing->hiddenFlag))) {
                    duplicate = TRUE;
                    break;
                }
            }
            if (duplicate) {
                continue;
            }
            GF_ASSERT(data->numObjectEvents < NELEMS(data->streamedObjectEvents));
            if (data->numObjectEvents >= NELEMS(data->streamedObjectEvents)) {
                continue;
            }
            data->streamedObjectSourceScripts[data->numObjectEvents] = event.script;
            event.script = 0xFFFF;
            event.hiddenFlag = neighbors[i];
            data->streamedObjectEvents[data->numObjectEvents++] = event;
        }
    }
    Heap_Free(neighbor);
}

BOOL MapHeaderData_IsStreamedObject(const FieldSystem *fieldSystem, const ObjectEvent *objectEvent)
{
    const MapHeaderData *data = fieldSystem->mapHeaderData;
    if (objectEvent->script != 0xFFFF) {
        return FALSE;
    }
    for (int i = data->originalObjectEventCount; i < data->numObjectEvents; i++) {
        if (data->objectEvents[i].localID == objectEvent->localID && data->objectEvents[i].hiddenFlag == objectEvent->hiddenFlag) {
            return TRUE;
        }
    }
    return FALSE;
}

u16 MapHeaderData_GetObjectSourceScript(const FieldSystem *fieldSystem, int localID, int ownerHeaderID)
{
    const MapHeaderData *data = fieldSystem->mapHeaderData;
    if (!data || data->objectEvents != data->streamedObjectEvents) {
        return 0xFFFF;
    }
    for (int i = 0; i < data->numObjectEvents; i++) {
        const ObjectEvent *event = &data->objectEvents[i];
        if (event->script == 0xFFFF && event->localID == localID && event->hiddenFlag == ownerHeaderID) {
            return data->streamedObjectSourceScripts[i];
        }
    }
    return 0xFFFF;
}
#endif

const BgEvent *MapHeaderData_GetBgEvents(const FieldSystem *fieldSystem)
{
    return fieldSystem->mapHeaderData->bgEvents;
}

int MapHeaderData_GetNumBgEvents(const FieldSystem *fieldSystem)
{
    return fieldSystem->mapHeaderData->numBgEvents;
}

const WarpEvent *MapHeaderData_GetWarpEventByIndex(const FieldSystem *fieldSystem, int index)
{
    return (index >= fieldSystem->mapHeaderData->numWarpEvents)
        ? NULL
        : &fieldSystem->mapHeaderData->warpEvents[index];
}

int MapHeaderData_GetIndexOfWarpEventAtPos(const FieldSystem *fieldSystem, int x, int z)
{
    for (int i = 0; i < fieldSystem->mapHeaderData->numWarpEvents; i++) {
        if (fieldSystem->mapHeaderData->warpEvents[i].x == x
            && fieldSystem->mapHeaderData->warpEvents[i].z == z) {
            return i;
        }
    }

    return -1;
}

int MapHeaderData_GetNumCoordEvents(const FieldSystem *fieldSystem)
{
    return fieldSystem->mapHeaderData->numCoordEvents;
}

const CoordEvent *MapHeaderData_GetCoordEvents(const FieldSystem *fieldSystem)
{
    return fieldSystem->mapHeaderData->coordEvents;
}

u32 MapHeaderData_GetNumObjectEvents(const FieldSystem *fieldSystem)
{
    return fieldSystem->mapHeaderData->numObjectEvents;
}

const ObjectEvent *MapHeaderData_GetObjectEvents(const FieldSystem *fieldSystem)
{
    return fieldSystem->mapHeaderData->objectEvents;
}

BOOL MapHeaderData_SetObjectEventPos(FieldSystem *fieldSystem, int localID, u16 x, u16 z)
{
    int i;
    ObjectEvent *objectEvent = fieldSystem->mapHeaderData->objectEvents;
    u32 numObjectEvents = fieldSystem->mapHeaderData->numObjectEvents;

    for (i = 0; i < numObjectEvents; i++) {
        if (objectEvent[i].localID == localID) {
            objectEvent[i].x = x;
            objectEvent[i].z = z;
            return TRUE;
        }
    }

    GF_ASSERT(FALSE);
    return FALSE;
}

BOOL MapHeaderData_SetObjectEventDir(FieldSystem *fieldSystem, int localID, int dir)
{
    int i;
    ObjectEvent *objectEvent = fieldSystem->mapHeaderData->objectEvents;
    u32 numObjectEvents = fieldSystem->mapHeaderData->numObjectEvents;

    for (i = 0; i < numObjectEvents; i++) {
        if (objectEvent[i].localID == localID) {
            objectEvent[i].dir = dir;
            return TRUE;
        }
    }

    GF_ASSERT(FALSE);
    return FALSE;
}

BOOL MapHeaderData_SetObjectEventMovementType(FieldSystem *fieldSystem, int localID, int movementType)
{
    int i;
    ObjectEvent *objectEvent = fieldSystem->mapHeaderData->objectEvents;
    u32 numObjectEvents = fieldSystem->mapHeaderData->numObjectEvents;

    for (i = 0; i < numObjectEvents; i++) {
        if (objectEvent[i].localID == localID) {
            objectEvent[i].movementType = movementType;
            return TRUE;
        }
    }

    GF_ASSERT(FALSE);
    return FALSE;
}

BOOL MapHeaderData_SetWarpEventPos(FieldSystem *fieldSystem, u16 index, u16 x, u16 z)
{
    WarpEvent *warpEvent = fieldSystem->mapHeaderData->warpEvents;
    warpEvent[index].x = x;
    warpEvent[index].z = z;
    return TRUE;
}

BOOL MapHeaderData_SetWarpEventDestHeaderID(FieldSystem *fieldSystem, u16 index, u16 destHeaderID)
{
    WarpEvent *warpEvents = fieldSystem->mapHeaderData->warpEvents;
    warpEvents[index].destHeaderID = destHeaderID;
    return TRUE;
}

BOOL MapHeaderData_SetWarpEventDestWarpID(FieldSystem *fieldSystem, u16 index, u16 destWarpID)
{
    WarpEvent *warpEvents = fieldSystem->mapHeaderData->warpEvents;
    warpEvents[index].destWarpID = destWarpID;
    return TRUE;
}

BOOL MapHeaderData_SetBgEventPos(FieldSystem *fieldSystem, u16 index, u16 x, u16 z)
{
    BgEvent *bgEvent = MapHeaderData_GetBgEvents(fieldSystem);

    bgEvent += index;
    bgEvent->x = x;
    bgEvent->z = z;

    return TRUE;
}

#define CONSUME_EVENTS(T, dataTEvents, dataNumTEvents) \
    do {                                               \
        (dataNumTEvents) = *(u32 *)events;             \
        events += sizeof(u32);                         \
        if ((dataNumTEvents) != 0) {                   \
            (dataTEvents) = (const T *)events;         \
        } else {                                       \
            (dataTEvents) = NULL;                      \
        }                                              \
        events += sizeof(T) * (dataNumTEvents);        \
    } while (0)

static void MapHeaderData_ParseEvents(MapHeaderData *data)
{
    const u8 *events = (const u8 *)data->tmpEventsBuf;

    CONSUME_EVENTS(BgEvent, data->bgEvents, data->numBgEvents);
    CONSUME_EVENTS(ObjectEvent, data->objectEvents, data->numObjectEvents);
    CONSUME_EVENTS(WarpEvent, data->warpEvents, data->numWarpEvents);
    CONSUME_EVENTS(CoordEvent, data->coordEvents, data->numCoordEvents);
}

void MapHeaderData_LoadWildEncounters(WildEncounters *data, enum MapHeaderID mapHeaderID)
{
    memset(data, 0, sizeof(WildEncounters));
    if (MapHeader_HasWildEncounters(mapHeaderID)) {
        enum NarcID narcID = (GAME_VERSION == VERSION_DIAMOND || GAME_VERSION == VERSION_PLATINUM)
            ? NARC_INDEX_FIELDDATA__ENCOUNTDATA__PL_ENC_DATA
            : NARC_INDEX_FIELDDATA__ENCOUNTDATA__P_ENC_DATA;
        NARC_ReadWholeMemberByIndexPair(data, narcID, MapHeader_GetWildEncountersArchiveID(mapHeaderID));
    }
}

const WildEncounters *MapHeaderData_GetWildEncounters(const FieldSystem *fieldSystem)
{
    return &fieldSystem->mapHeaderData->wildEncounters;
}

static void MapHeaderData_LoadInitScripts(MapHeaderData *data, int headerID)
{
    int initScriptsID = MapHeader_GetInitScriptsArchiveID(headerID);

    MI_CpuClearFast(data->initScripts, sizeof(data->initScripts));
    GF_ASSERT(NARC_GetMemberSizeByIndexPair(NARC_INDEX_FIELDDATA__SCRIPT__SCR_SEQ, initScriptsID) < sizeof(data->initScripts));

    NARC_ReadWholeMemberByIndexPair(data->initScripts, NARC_INDEX_FIELDDATA__SCRIPT__SCR_SEQ, initScriptsID);
}

const u8 *MapHeaderData_GetInitScriptBytes(const FieldSystem *fieldSystem)
{
    GF_ASSERT(fieldSystem->mapHeaderData != NULL);
    return (const u8 *)&fieldSystem->mapHeaderData->initScripts;
}

BOOL MapHeaderData_IsPosFreeOfObjectEvents(const FieldSystem *fieldSystem, u16 x, u16 z)
{
    const MapHeaderData *data = fieldSystem->mapHeaderData;

    for (u32 i = 0; i < data->numObjectEvents; i++) {
        if (data->objectEvents[i].x == x && data->objectEvents[i].z == z) {
            return FALSE;
        }
    }

    return TRUE;
}
