#ifndef POKEBW2_BATTLE_BTLV_CAMERA_H
#define POKEBW2_BATTLE_BTLV_CAMERA_H

// Overlay 168's btlv_camera.c (named by its string), the battle view's 3D camera: its position and target, their
// moves toward a goal, its rotation around the target and its shakes. BtlvCamera_Create is swan's name; the others
// are ours

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

BtlvCamera *BtlvCamera_Create(TCBManager *tcbManager, HeapID heapId);
void BtlvCamera_Delete(BtlvCamera *camera);
void BtlvCamera_Main(BtlvCamera *camera);
void BtlvCamera_SetPosTarget(BtlvCamera *camera, const VecFx32 *pos, const VecFx32 *target);
void BtlvCamera_Rotate(BtlvCamera *camera, s32 pitch, s32 yaw);
void BtlvCamera_MoveTo(BtlvCamera *camera, const VecFx32 *pos, const VecFx32 *target, int frames, int wait,
                       int brakeFrames);
void BtlvCamera_Shake(BtlvCamera *camera, int dir, fx32 amplitude, int unused, int frames, int wait, int count);
void BtlvCamera_GetPosTarget(BtlvCamera *camera, VecFx32 *pos, VecFx32 *target);
void BtlvCamera_CalcRotatedPos(BtlvCamera *camera, s32 pitch, s32 yaw, VecFx32 *pos, VecFx32 *target);
void BtlvCamera_GetDefaultPosTarget(VecFx32 *pos, VecFx32 *target);
BOOL BtlvCamera_IsMoving(BtlvCamera *camera);

#endif // POKEBW2_BATTLE_BTLV_CAMERA_H
