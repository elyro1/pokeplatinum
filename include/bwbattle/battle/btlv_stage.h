#ifndef POKEBW2_BATTLE_BTLV_STAGE_H
#define POKEBW2_BATTLE_BTLV_STAGE_H

// Overlay 168's btlv_stage.c (named by its string), the battle view's 3D stage: the two grounds the Pokémon stand on,
// their animations and the fade of their textures' palettes. BtlvStage_Create is swan's name; the rest are ours

#include "types.h"
#include "battle/btlv_effect.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// rule: 0 single, 1 double, 2 triple, 3 rotation. stageId and variant pick the grounds from file 2 of archive 151;
// unk 1 or 2 picks stage 27 instead
BtlvStage *BtlvStage_Create(u32 rule, u32 stageId, u8 variant, HeapID heapId, u32 unk);
void BtlvStage_Delete(BtlvStage *stage);
void BtlvStage_Main(BtlvStage *stage);
void BtlvStage_Draw(BtlvStage *stage);
void BtlvStage_StartPaletteFade(BtlvStage *stage, u8 startEvy, u8 targetEvy, u8 wait, u16 color);
BOOL BtlvStage_IsPaletteFading(BtlvStage *stage);
// side 0 or 1, or 2 for both
void BtlvStage_SetVanish(BtlvStage *stage, u32 side, BOOL hide);
void BtlvStage_StartAnimation(BtlvStage *stage, u32 side, u32 anm, fx32 speed, s32 frames);
BOOL BtlvStage_IsAnimating(BtlvStage *stage);

#endif // POKEBW2_BATTLE_BTLV_STAGE_H
