#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"

struct G3DCamera {
    G3DCameraProjection projection;
    FxLookAt lookAt;
};

G3DCamera *GFL_G3DCameraCreate(G3DCameraProjectionMode proj, fx32 param1, fx32 param2, fx32 param3, fx32 param4,
                               fx32 near, fx32 far, fx32 ndcRangeOverride, const VecFx32 *position,
                               const VecFx32 *upVector, const VecFx32 *target, HeapID heapId) {
    G3DCamera *cam = GFL_HeapAllocate(heapId, sizeof(G3DCamera), FALSE, "g3d_camera.c", 56);

    cam->projection.type = proj;
    cam->projection.param1 = param1;
    cam->projection.param2 = param2;
    cam->projection.param3 = param3;
    cam->projection.param4 = param4;
    cam->projection.near = near;
    cam->projection.far = far;
    cam->projection.ndcRangeOverride = ndcRangeOverride;
    cam->lookAt.position = *position;
    cam->lookAt.upVector = *upVector;
    cam->lookAt.target = *target;
    return cam;
}

void GFL_G3DCameraFree(G3DCamera *cam) {
    GFL_HeapFree(cam);
}

void GFL_G3DCameraFlush(G3DCamera *cam) {
    GFL_G3DSysMtxSetProjection(&cam->projection);
    GFL_G3DSysMtxSetViewLookAt(&cam->lookAt);
}

void GFL_G3DCameraGetLookatPos(G3DCamera *cam, VecFx32 *pos) {
    *pos = cam->lookAt.position;
}

void GFL_G3DCameraSetLookatPos(G3DCamera *cam, const VecFx32 *pos) {
    cam->lookAt.position = *pos;
}

void GFL_G3DCameraGetLookatUpVector(G3DCamera *cam, VecFx32 *up) {
    *up = cam->lookAt.upVector;
}

void GFL_G3DCameraSetLookatUpVector(G3DCamera *cam, const VecFx32 *up) {
    cam->lookAt.upVector = *up;
}

void GFL_G3DCameraGetLookatTarget(G3DCamera *cam, VecFx32 *target) {
    *target = cam->lookAt.target;
}

void GFL_G3DCameraSetLookatTarget(G3DCamera *cam, const VecFx32 *target) {
    cam->lookAt.target = *target;
}

void GFL_G3DCameraGetProjectionZNear(G3DCamera *cam, fx32 *zNear) {
    *zNear = cam->projection.near;
}

void GFL_G3DCameraSetProjectionZNear(G3DCamera *cam, fx32 *zNear) {
    cam->projection.near = *zNear;
}

void GFL_G3DCameraSetProjectionZFar(G3DCamera *cam, fx32 *zFar) {
    cam->projection.far = *zFar;
}

G3DCameraProjectionMode GFL_G3DCameraGetProjectionType(G3DCamera *cam) {
    return cam->projection.type;
}

void GFL_G3DCameraPerspectiveSetFOVSin(G3DCamera *cam, fx32 fovSin) {
    cam->projection.param1 = fovSin;
}

void GFL_G3DCameraPerspectiveGetFOVCos(G3DCamera *cam, fx32 *fovCos) {
    *fovCos = cam->projection.param2;
}

void GFL_G3DCameraPerspectiveSetFOVCos(G3DCamera *cam, fx32 fovCos) {
    cam->projection.param2 = fovCos;
}

void GFL_G3DCameraOrthoGetTop(G3DCamera *cam, fx32 *top) {
    *top = cam->projection.param1;
}

void GFL_G3DCameraOrthoSetTop(G3DCamera *cam, fx32 top) {
    cam->projection.param1 = top;
}

void GFL_G3DCameraOrthoGetBottom(G3DCamera *cam, fx32 *bottom) {
    *bottom = cam->projection.param2;
}

void GFL_G3DCameraOrthoSetBottom(G3DCamera *cam, fx32 bottom) {
    cam->projection.param2 = bottom;
}
