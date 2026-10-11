#include "port/scripted_npc_scene.h"

#include <nitro.h>
#include <string.h>

#include "field/field_system.h"
#include "overlay005/ov5_021ECC20.h"
#include "overlay005/map_object_anim_cmd.h"
#include "overlay005/object_event_gfx_data.h"

#include "heap.h"
#include "map_object.h"
#include "map_object_move.h"
#include "unk_020655F4.h"
#include "script_manager.h"
#include "sys_task.h"
#include "terrain_collision_manager.h"

// Only ordinary 32x32 billboards are supported by this prototype.
// This volume includes their billboard offset, shadow and reflection, plus
// an eight-pixel guard band in clip space. Larger/special models use 4:3.
#define NPC_SCENE_MAX_TILES     12
#define NPC_SCENE_MARGIN_PIXELS 8

typedef struct {
    FieldSystem *fieldSystem;
    MapObject *object;
    SysTask *animation;
    MapObjectAnimCmd commands[2];
    int dir;
    int remaining;
    BOOL exiting;
    BOOL nativeView;
    u32 nativeFrame;
    VecFx32 anchor;
} NpcSceneTask;

static BOOL NpcScene_OrdinaryBillboard(const MapObject *object)
{
    const ObjectEventGfxModelAnimEntry *entry = gObjectEventGfxModelAnimsTable;
    while (entry->graphicsID != OBJ_EVENT_GFX_SENTINEL_ID) {
        if (entry->graphicsID == MapObject_GetGraphicsID(object)) {
            return entry->modelID == BILLBOARD_MODEL_GENERIC_32x32;
        }
        entry++;
    }
    return FALSE;
}

// Conservative homogeneous frustum test: a volume is outside only when every
// corner lies outside the same plane. Never divide by a near-zero clip w.
static BOOL NpcScene_PositionVisible(const FieldSystem *fieldSystem, const VecFx32 *pos)
{
    if (fieldSystem->npcSceneViewWidth == 0) {
        return TRUE;
    }
    const MtxFx43 *view = &fieldSystem->npcSceneViewMatrix;
    const MtxFx44 *projection = &fieldSystem->npcSceneProjectionMatrix;
    double marginX = 2.0 * NPC_SCENE_MARGIN_PIXELS / fieldSystem->npcSceneViewWidth;
    double marginY = 2.0 * NPC_SCENE_MARGIN_PIXELS / HW_LCD_HEIGHT;
    unsigned outside = 0x3f;
    for (int i = 0; i < 8; i++) {
        double x = (double)pos->x / FX32_ONE + ((i & 1) ? 24 : -24);
        double y = (double)pos->y / FX32_ONE + ((i & 2) ? 48 : -48);
        double z = (double)pos->z / FX32_ONE + ((i & 4) ? 24 : -24);
        double vx = (x * view->_00 + y * view->_10 + z * view->_20 + view->_30) / FX32_ONE;
        double vy = (x * view->_01 + y * view->_11 + z * view->_21 + view->_31) / FX32_ONE;
        double vz = (x * view->_02 + y * view->_12 + z * view->_22 + view->_32) / FX32_ONE;
        double cx = (vx * projection->_00 + vy * projection->_10 + vz * projection->_20 + projection->_30) / FX32_ONE;
        double cy = (vx * projection->_01 + vy * projection->_11 + vz * projection->_21 + projection->_31) / FX32_ONE;
        double cz = (vx * projection->_02 + vy * projection->_12 + vz * projection->_22 + projection->_32) / FX32_ONE;
        double cw = (vx * projection->_03 + vy * projection->_13 + vz * projection->_23 + projection->_33) / FX32_ONE;
        if (cw <= 0) {
            return TRUE;
        }
        unsigned planes = 0;
        if (cx < -cw * (1 + marginX)) {
            planes |= 1;
        }
        if (cx > cw * (1 + marginX)) {
            planes |= 2;
        }
        if (cy < -cw * (1 + marginY)) {
            planes |= 4;
        }
        if (cy > cw * (1 + marginY)) {
            planes |= 8;
        }
        if (cz < -cw) {
            planes |= 16;
        }
        if (cz > cw) {
            planes |= 32;
        }
        outside &= planes;
    }
    return outside == 0;
}

static BOOL NpcScene_ObjectVisible(const NpcSceneTask *scene)
{
    VecFx32 pos;
    ov5_021ECDA0(scene->object, &pos);
    return NpcScene_PositionVisible(scene->fieldSystem, &pos);
}

static int NpcScene_WalkDirection(u16 action)
{
    // Do not accept jumps, emotes, on-the-spot actions or arbitrary bytecode.
    if (action >= MOVEMENT_ACTION_WALK_SLOW_NORTH && action <= MOVEMENT_ACTION_WALK_SLOW_EAST) {
        return action - MOVEMENT_ACTION_WALK_SLOW_NORTH;
    }
    if (action >= MOVEMENT_ACTION_WALK_NORMAL_NORTH && action <= MOVEMENT_ACTION_WALK_NORMAL_EAST) {
        return action - MOVEMENT_ACTION_WALK_NORMAL_NORTH;
    }
    if (action >= MOVEMENT_ACTION_WALK_FAST_NORTH && action <= MOVEMENT_ACTION_WALK_FAST_EAST) {
        return action - MOVEMENT_ACTION_WALK_FAST_NORTH;
    }
    return DIR_NONE;
}

static BOOL NpcScene_SafeStep(NpcSceneTask *scene, const VecFx32 *source, int dir, VecFx32 *dest)
{
    MapObject *object = scene->object;
    FieldSystem *fieldSystem = scene->fieldSystem;
    int x = source->x / MAP_OBJECT_TILE_SIZE;
    int z = source->z / MAP_OBJECT_TILE_SIZE;
    int nextX = x + MapObject_GetDxFromDir(dir);
    int nextZ = z + MapObject_GetDzFromDir(dir);
    if (nextX < 0 || nextZ < 0 || nextX >= MapMatrix_GetWidth(fieldSystem->mapMatrix) * 32 || nextZ >= MapMatrix_GetHeight(fieldSystem->mapMatrix) * 32) {
        return FALSE;
    }
    u16 attributes;
    if (!fieldSystem->terrainCollisionMan->getTileAttributes(fieldSystem, nextX, nextZ, &attributes)) {
        return FALSE; // Unloaded map tiles are not walkable staging space.
    }
    // Collision queries read the origin tile from the object. Change only its
    // tile coordinates during this synchronous query, then restore them.
    int originalX = MapObject_GetX(object);
    int originalZ = MapObject_GetZ(object);
    MapObject_SetX(object, x);
    MapObject_SetZ(object, z);
    u32 collision = sub_02063E18(object, source, nextX, MapObject_GetY(object), nextZ, dir);
    MapObject_SetX(object, originalX);
    MapObject_SetZ(object, originalZ);
    if (collision & ~MAP_OBJ_COLLISION_OUT_OF_RANGE) {
        return FALSE;
    }
    *dest = *source;
    VecFx32_StepDirection(dir, dest, MAP_OBJECT_TILE_SIZE);
    u8 heightSource;
    dest->y = TerrainCollisionManager_GetHeight(fieldSystem, source->y, dest->x, dest->z, &heightSource);
    // This first version supports flat routes; stairs/bridges need authored paths.
    return heightSource != CALCULATED_HEIGHT_SOURCE_NONE && dest->y == source->y;
}

static void NpcScene_UseNativeView(NpcSceneTask *scene)
{
    if (!scene->nativeView) {
        scene->nativeView = TRUE;
        scene->nativeFrame = scene->fieldSystem->npcSceneRenderFrame;
        ScriptManager_UseNativeNpcView(scene->fieldSystem);
    }
}

static void NpcScene_Finish(SysTask *task, NpcSceneTask *scene)
{
    if (scene->exiting) {
        MapObject_SetFlagAndDeleteObject(scene->object);
    }
    u8 *movementCount = FieldSystem_GetScriptMemberPtr(scene->fieldSystem, SCRIPT_MANAGER_MOVEMENT_COUNT);
    GF_ASSERT(*movementCount > 0);
    (*movementCount)--;
    Heap_Free(scene);
    SysTask_Done(task);
}

static void NpcScene_Update(SysTask *task, void *param)
{
    NpcSceneTask *scene = param;
    if (scene->animation != NULL) {
        if (!MapObject_HasAnimationEnded(scene->animation)) {
            return;
        }
        MapObject_FinishAnimation(scene->animation);
        scene->animation = NULL;
        scene->remaining--;
    }
    if (scene->nativeView) {
        // Render the narrow view before changing object lifetime/position.
        // ScriptManager holds that view until the whole script ends.
        if (scene->fieldSystem->npcSceneRenderFrame == scene->nativeFrame) {
            return;
        }
        if (!scene->exiting) {
            // A route invalidated during extra travel must still hand the
            // original script its original anchor, never a partial approach.
            MapObject_SetPosDirFromVec(scene->object, &scene->anchor, MapObject_GetInitialDir(scene->object));
            sub_02062B68(scene->object);
        }
        NpcScene_Finish(task, scene);
        return;
    }
    if (!scene->exiting && scene->remaining == 0) {
        NpcScene_Finish(task, scene);
        return;
    }
    if (scene->exiting && !NpcScene_ObjectVisible(scene)) {
        NpcScene_Finish(task, scene);
        return;
    }
    if (scene->remaining == 0) {
        NpcScene_UseNativeView(scene);
        return;
    }
    VecFx32 dest;
    if (!NpcScene_SafeStep(scene, MapObject_GetPos(scene->object), scene->dir, &dest)) {
        NpcScene_UseNativeView(scene);
        return;
    }
    scene->animation = MapObject_StartAnimation(scene->object, scene->commands);
}

BOOL ScriptedNpcScene_Start(FieldSystem *fieldSystem, MapObject *object, u16 action, u16 maxTiles, BOOL exiting)
{
    if (fieldSystem->npcSceneViewWidth == HW_LCD_WIDTH && fieldSystem->npcSceneNativeViewLocks == 0) {
        return FALSE; // Preserve native-width movement and timing exactly.
    }
    NpcSceneTask *scene = Heap_Alloc(HEAP_ID_FIELD1, sizeof(*scene));
    GF_ASSERT(scene != NULL);
    memset(scene, 0, sizeof(*scene));
    scene->fieldSystem = fieldSystem;
    scene->object = object;
    scene->dir = NpcScene_WalkDirection(action);
    scene->exiting = exiting;
    scene->anchor = *MapObject_GetPos(object);
    scene->remaining = maxTiles < NPC_SCENE_MAX_TILES ? maxTiles : NPC_SCENE_MAX_TILES;
    scene->commands[0].movementAction = action;
    scene->commands[0].count = 1;
    scene->commands[1].movementAction = MOVEMENT_ACTION_END;
    if (scene->dir == DIR_NONE || !NpcScene_OrdinaryBillboard(object) || fieldSystem->npcSceneViewWidth == 0 || fieldSystem->npcSceneNativeViewLocks != 0) {
        NpcScene_UseNativeView(scene);
    } else if (!exiting) {
        VecFx32 anchor = *MapObject_GetPos(object);
        VecFx32 spawn = anchor;
        VecFx32 renderOffset;
        ov5_021ECDA0(object, &renderOffset);
        VEC_Subtract(&renderOffset, &anchor, &renderOffset);
        int steps = 0;
        while (TRUE) {
            VecFx32 renderPos;
            VEC_Add(&spawn, &renderOffset, &renderPos);
            if (!NpcScene_PositionVisible(fieldSystem, &renderPos)) {
                break;
            }
            VecFx32 next, reverse;
            int opposite = Direction_GetOpposite(scene->dir);
            if (steps == scene->remaining || !NpcScene_SafeStep(scene, &spawn, opposite, &next) || !NpcScene_SafeStep(scene, &next, scene->dir, &reverse)) {
                NpcScene_UseNativeView(scene);
                break;
            }
            spawn = next;
            steps++;
        }
        if (!scene->nativeView) {
            scene->remaining = steps;
            if (steps != 0) {
                MapObject_SetPosDirFromVec(object, &spawn, scene->dir);
                sub_02062B68(object);
            }
        }
    }
    if (!scene->nativeView && !NpcScene_ObjectVisible(scene) && exiting) {
        Heap_Free(scene);
        return FALSE;
    }
    if (!scene->nativeView && !exiting && scene->remaining == 0) {
        Heap_Free(scene);
        return FALSE;
    }
    u8 *movementCount = FieldSystem_GetScriptMemberPtr(fieldSystem, SCRIPT_MANAGER_MOVEMENT_COUNT);
    (*movementCount)++;
    SysTask_Start(NpcScene_Update, scene, 0);
    return TRUE;
}
