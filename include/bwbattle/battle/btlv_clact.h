#ifndef POKEBW2_BATTLE_BTLV_CLACT_H
#define POKEBW2_BATTLE_BTLV_CLACT_H

// Overlay 168's btlv_clact.c (named by its string), the battle view's 2D cell actors: the effects' OBJ actors with
// their resources, their moves, scalings and palette fades, and the turn gauge and its popup on the main screen in
// mode 1. The names are ours; swan has none for this file

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

BtlvClact *BtlvClact_Create(TCBManager *tcbManager, HeapID heapId, u32 mode);
void BtlvClact_Delete(BtlvClact *wk);
void BtlvClact_Draw(BtlvClact *wk);
int BtlvClact_AddActor(BtlvClact *wk, u32 arcId, u32 fileId, s16 x, s16 y, fx32 scaleX, fx32 scaleY);
int BtlvClact_AddActorEx(BtlvClact *wk, u32 arcId, u32 charFileId, u32 plttFileId, u32 cellFileId, u32 animFileId,
                         s16 x, s16 y, fx32 scaleX, fx32 scaleY);
void BtlvClact_SetGauge(BtlvClact *wk, int count, int pos);
void BtlvClact_DeleteActor(BtlvClact *wk, int index);
void BtlvClact_StartMove(BtlvClact *wk, int index, int type, ClActorPos *pos, int frames, int wait, int count);
void BtlvClact_StartScale(BtlvClact *wk, int index, int type, VecFx32 *scale, int frames, int wait, int count);
void BtlvClact_SetAnimSeq(BtlvClact *wk, int index, int sequence);
void BtlvClact_StartPalFade(BtlvClact *wk, int index, u8 start, u8 end, s8 delay, u16 color);
BOOL BtlvClact_IsPopupShowing(BtlvClact *wk);
void BtlvClact_ShowPopup(BtlvClact *wk, int number, Font *font);

#endif // POKEBW2_BATTLE_BTLV_CLACT_H
