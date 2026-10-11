#ifndef POKEPLATINUM_SCRIPTED_NPC_SCENE_H
#define POKEPLATINUM_SCRIPTED_NPC_SCENE_H

#include "struct_decls/map_object.h"

#include "field/field_system_decl.h"

// Starts a movement counted by WaitMovement. FALSE means no extra travel.
BOOL ScriptedNpcScene_Start(FieldSystem *fieldSystem, MapObject *object, u16 action, u16 maxTiles, BOOL exiting);

#endif
