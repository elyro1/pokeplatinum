#ifndef POKEBW2_BATTLE_BTLV_FIELD_H
#define POKEBW2_BATTLE_BTLV_FIELD_H

// Overlay 168's btlv_field.c (named by its string), the battle view's field: the ground and weather model around the
// stage. BtlvField_Create is swan's name; the other names are ours

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

BtlvField *BtlvField_Create(BOOL arg0, u8 fieldId, u8 variant, HeapID heapId, u32 mode, s32 fieldParam, BOOL arg6);
void BtlvField_Reload(BtlvField *field, s32 fieldParam);
void BtlvField_Delete(BtlvField *field);
void BtlvField_Main(BtlvField *field);
void BtlvField_Draw(BtlvField *field);
void BtlvField_StartPaletteFade(BtlvField *field, u8 evy, u8 targetEvy, u8 waitFrames, u16 color);
BOOL BtlvField_IsPaletteFading(BtlvField *field);
void BtlvField_SetHidden(BtlvField *field, BOOL hide);

#endif // POKEBW2_BATTLE_BTLV_FIELD_H
