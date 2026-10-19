// The battle view's 3D camera. The name is the ROM's string, from GFL_HeapAllocate's assert. BtlvCamera_Create is
// swan's name; the others are ours

#include "battle/btlv_camera.h"
#include "types.h"
#include "battle/btlv_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// The camera's work, 0xb8 bytes
struct BtlvCamera {
    TCBManager *tcbManager; // 0x00  never read
    G3DCamera *camera;      // 0x04
    s32 pitch;              // 0x08  the angles and distance of the position from the target
    s32 yaw;                // 0x0c
    fx32 distance;          // 0x10
    u32 flags;              // 0x14  bit 0: the position moves, bit 1: the target moves, bit 2: a shake runs
    VecFx32 pos;            // 0x18
    VecFx32 target;         // 0x24
    VecFx32 posGoal;        // 0x30
    VecFx32 targetGoal;     // 0x3c
    VecFx32 posStep;        // 0x48
    VecFx32 targetStep;     // 0x54
    VecFx32 shakeOffset;    // 0x60  added to both the position and the target
    s32 wait;               // 0x6c  frames until the next step
    s32 waitReset;          // 0x70
    s32 brakeFrames;        // 0x74  steps until the steps are halved, 0 for never
    BtlvEffToolMove shake;  // 0x78
    HeapID heapId;          // 0xb4
};

#define CAMERA_MOVE_POS (1 << 0)
#define CAMERA_MOVE_TARGET (1 << 1)
#define CAMERA_SHAKE (1 << 2)

static void BtlvCamera_Update(BtlvCamera *camera);
static void BtlvCamera_UpdateAngles(BtlvCamera *camera);

// The position, target and up vector the camera starts with; the declaration order lays them out as in the ROM
static const VecFx32 data_ov168_021f2f80 = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 data_ov168_021f2f74 = { 0, FX32_CONST(2.6), 0 };
static const VecFx32 data_ov168_021f2f8c = { 0, FX32_ONE, 0 };

BtlvCamera *BtlvCamera_Create(TCBManager *tcbManager, HeapID heapId) {
    BtlvCamera *camera = GFL_HeapAllocate(heapId, sizeof(BtlvCamera), TRUE, "btlv_camera.c", 0x56);

    camera->heapId = heapId;
    camera->tcbManager = tcbManager;
    // The sine and cosine of 13 degrees
    camera->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, 0x399, 0xf97, FX32_CONST(1.35), 0, FX32_ONE, FX32_CONST(512),
                            0, &data_ov168_021f2f80, &data_ov168_021f2f8c, &data_ov168_021f2f74, heapId);
    BtlvCamera_UpdateAngles(camera);
    GFL_G3DCameraFlush(camera->camera);
    BtlvCamera_GetDefaultPosTarget(&camera->pos, &camera->target);
    return camera;
}

void BtlvCamera_Delete(BtlvCamera *camera) {
    GFL_G3DCameraFree(camera->camera);
    GFL_HeapFree(camera);
}

void BtlvCamera_Main(BtlvCamera *camera) {
    BtlvCamera_Update(camera);
}

void BtlvCamera_SetPosTarget(BtlvCamera *camera, const VecFx32 *pos, const VecFx32 *target) {
    if (pos != NULL) {
        camera->pos.x = pos->x;
        camera->pos.y = pos->y;
        camera->pos.z = pos->z;
        GFL_G3DCameraSetLookatPos(camera->camera, pos);
    }
    if (target != NULL) {
        camera->target.x = target->x;
        camera->target.y = target->y;
        camera->target.z = target->z;
        GFL_G3DCameraSetLookatTarget(camera->camera, target);
    }
    GFL_G3DCameraFlush(camera->camera);
    BtlvCamera_UpdateAngles(camera);
}

void BtlvCamera_Rotate(BtlvCamera *camera, s32 pitch, s32 yaw) {
    BtlvCamera_CalcRotatedPos(camera, pitch, yaw, &camera->pos, &camera->target);
    GFL_G3DCameraSetLookatPos(camera->camera, &camera->pos);
    GFL_G3DCameraFlush(camera->camera);
}

void BtlvCamera_MoveTo(BtlvCamera *camera, const VecFx32 *pos, const VecFx32 *target, int frames, int wait,
                       int brakeFrames) {
    camera->brakeFrames = brakeFrames;
    camera->wait = wait;
    camera->waitReset = wait;
    if (pos != NULL) {
        camera->posGoal.x = pos->x;
        camera->posGoal.y = pos->y;
        camera->posGoal.z = pos->z;
        BtlvEffTool_CalcStepVec(&camera->pos, pos, &camera->posStep, FX32_CONST(frames));
        camera->flags |= CAMERA_MOVE_POS;
    }
    if (target != NULL) {
        camera->targetGoal.x = target->x;
        camera->targetGoal.y = target->y;
        camera->targetGoal.z = target->z;
        BtlvEffTool_CalcStepVec(&camera->target, target, &camera->targetStep, FX32_CONST(frames));
        camera->flags |= CAMERA_MOVE_TARGET;
    }
}

// Shakes the camera sideways (dir 1) or vertically by amplitude: count full swings of 4 * frames steps
void BtlvCamera_Shake(BtlvCamera *camera, int dir, fx32 amplitude, int unused, int frames, int wait, int count) {
    camera->shake.type = 3;
    camera->shake.stepTime = frames;
    camera->shake.stepTimeReset = frames;
    camera->shake.wait = 0;
    camera->shake.waitReset = wait;
    camera->shake.start.x = 0;
    camera->shake.count = count * 4;
    camera->shake.start.y = 0;
    camera->shake.start.z = 0;
    if (dir == 1) {
        camera->shake.end.x = amplitude;
        camera->shake.end.y = 0;
        camera->shake.step.x = FX_Div(amplitude, FX32_CONST(frames));
        camera->shake.step.y = 0;
    } else {
        camera->shake.end.x = 0;
        camera->shake.end.y = amplitude;
        camera->shake.step.x = 0;
        camera->shake.step.y = FX_Div(amplitude, FX32_CONST(frames));
    }
    camera->shake.end.z = 0;
    camera->shake.step.z = 0;
    camera->flags |= CAMERA_SHAKE;
}

void BtlvCamera_GetPosTarget(BtlvCamera *camera, VecFx32 *pos, VecFx32 *target) {
    GFL_G3DCameraGetLookatPos(camera->camera, pos);
    GFL_G3DCameraGetLookatTarget(camera->camera, target);
}

// Turns the camera's angles by pitch and yaw and gives the position they put it at around its target
void BtlvCamera_CalcRotatedPos(BtlvCamera *camera, s32 pitch, s32 yaw, VecFx32 *pos, VecFx32 *target) {
    GFL_G3DCameraGetLookatTarget(camera->camera, target);
    camera->pitch += pitch;
    camera->yaw += yaw;
    pos->x = FX_Mul(FX_CosIdx(camera->yaw), FX_CosIdx(camera->pitch));
    pos->y = FX_SinIdx(camera->pitch);
    pos->z = FX_Mul(FX_SinIdx(camera->yaw), FX_CosIdx(camera->pitch));
    pos->x = FX_Mul(pos->x, camera->distance);
    pos->y = FX_Mul(pos->y, camera->distance);
    pos->z = FX_Mul(pos->z, camera->distance);
    pos->x += target->x;
    pos->y += target->y;
    pos->z += target->z;
}

void BtlvCamera_GetDefaultPosTarget(VecFx32 *pos, VecFx32 *target) {
    pos->x = FX32_CONST(6.7);
    pos->y = FX32_CONST(6.7);
    pos->z = FX32_CONST(17.3);
    target->x = 0;
    target->y = FX32_CONST(2.6);
    target->z = 0;
}

BOOL BtlvCamera_IsMoving(BtlvCamera *camera) {
    if (camera->flags) {
        return TRUE;
    }
    return FALSE;
}

static void BtlvCamera_Update(BtlvCamera *camera) {
    BOOL done = TRUE;
    VecFx32 pos;
    VecFx32 target;

    if (camera->flags) {
        if (camera->wait == 0) {
            camera->wait = camera->waitReset;
            if (camera->brakeFrames != 0 && --camera->brakeFrames == 0) {
                if (camera->posStep.x >> 1) {
                    camera->posStep.x >>= 1;
                }
                if (camera->posStep.y >> 1) {
                    camera->posStep.y >>= 1;
                }
                if (camera->posStep.z >> 1) {
                    camera->posStep.z >>= 1;
                }
                if (camera->targetStep.x >> 1) {
                    camera->targetStep.x >>= 1;
                }
                if (camera->targetStep.y >> 1) {
                    camera->targetStep.y >>= 1;
                }
                if (camera->targetStep.z >> 1) {
                    camera->targetStep.z >>= 1;
                }
            }
            if (camera->flags & CAMERA_MOVE_POS) {
                BtlvEffTool_Step(&camera->pos.x, &camera->posStep.x, &camera->posGoal.x, &done);
                BtlvEffTool_Step(&camera->pos.y, &camera->posStep.y, &camera->posGoal.y, &done);
                BtlvEffTool_Step(&camera->pos.z, &camera->posStep.z, &camera->posGoal.z, &done);
                if (done == TRUE) {
                    camera->flags &= ~CAMERA_MOVE_POS;
                }
            }
            if (camera->flags & CAMERA_MOVE_TARGET) {
                BtlvEffTool_Step(&camera->target.x, &camera->targetStep.x, &camera->targetGoal.x, &done);
                BtlvEffTool_Step(&camera->target.y, &camera->targetStep.y, &camera->targetGoal.y, &done);
                BtlvEffTool_Step(&camera->target.z, &camera->targetStep.z, &camera->targetGoal.z, &done);
                if (done == TRUE) {
                    camera->flags &= ~CAMERA_MOVE_TARGET;
                }
            }
            if (camera->flags & CAMERA_SHAKE) {
                done = BtlvEffTool_Move(&camera->shake, &camera->shakeOffset);
                if (done == TRUE) {
                    camera->flags &= ~CAMERA_SHAKE;
                }
            }
            pos.x = camera->pos.x + camera->shakeOffset.x;
            pos.y = camera->pos.y + camera->shakeOffset.y;
            pos.z = camera->pos.z + camera->shakeOffset.z;
            target.x = camera->target.x + camera->shakeOffset.x;
            target.y = camera->target.y + camera->shakeOffset.y;
            target.z = camera->target.z + camera->shakeOffset.z;
            GFL_G3DCameraSetLookatPos(camera->camera, &pos);
            GFL_G3DCameraSetLookatTarget(camera->camera, &target);
            GFL_G3DCameraFlush(camera->camera);
            BtlvCamera_UpdateAngles(camera);
        } else {
            camera->wait--;
        }
    }
}

// Takes the angles and distance of the position from the target from the G3D camera
static void BtlvCamera_UpdateAngles(BtlvCamera *camera) {
    VecFx32 pos;
    VecFx32 target;

    GFL_G3DCameraGetLookatPos(camera->camera, &pos);
    GFL_G3DCameraGetLookatTarget(camera->camera, &target);
    pos.x -= target.x;
    pos.y -= target.y;
    pos.z -= target.z;
    camera->pitch = FX_Atan2Idx(pos.y, pos.z);
    camera->yaw = FX_Atan2Idx(pos.z, pos.x);
    camera->distance = VEC_Mag(&pos);
}
