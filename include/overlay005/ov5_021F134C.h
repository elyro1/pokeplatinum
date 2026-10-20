#ifndef POKEPLATINUM_OV5_021F134C_H
#define POKEPLATINUM_OV5_021F134C_H

#include "struct_decls/map_object.h"

#include "overlay005/field_effect_manager.h"

#include "overworld_anim_manager.h"

void *MapObjectShadowRenderer_New(FieldEffectManager *effectManager);
void MapObjectShadowRenderer_Free(void *data);
void MapObjectShadow_Start(MapObject *mapObj);
// Uses the selected model without applying the time-of-day scale.
OverworldAnimManager *MapObjectShadow_StartFixed(MapObject *mapObj, int modelIndex);

#endif // POKEPLATINUM_OV5_021F134C_H
