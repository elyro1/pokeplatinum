#ifndef POKEBW2_FIELD_FIELD_LIGHT_STATUS_H
#define POKEBW2_FIELD_FIELD_LIGHT_STATUS_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/gx.h"

// The colors of a zone's light at a time of day and in a weather and season: the four lights' and then the materials'
// diffuse, ambient, specular and emission
void FieldLightStatus_Get(u32 zoneId, u32 hour, u32 minute, u32 weather, u32 season, GXRgb *colors, HeapID heapId);

#endif // POKEBW2_FIELD_FIELD_LIGHT_STATUS_H
