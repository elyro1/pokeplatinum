#include "types.h"
#include "shim/bw_shim.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"

#define G3D_LIGHT_MAX 4
// The most of DTCM the geometry command buffer takes
#define G3D_DTCM_SIZE_MAX 0x1800

// Magic numbers that mark each kind of object while it is alive, decremented when it is freed
#define G3D_RES_MAGIC 0x48bf
#define G3D_MDL_MAGIC 0x7a14
#define G3D_ANM_MAGIC 0x59d1
#define G3D_ACTOR_MAGIC 0x2185

// What a resource holds: nothing, a model with its textures, a model alone, textures, or an animation
enum {
    G3D_RES_TYPE_NONE,
    G3D_RES_TYPE_MDL_TEX,
    G3D_RES_TYPE_MDL,
    G3D_RES_TYPE_TEX,
    G3D_RES_TYPE_ANM,
};

// The kinds GFL_G3DResBindData takes
enum {
    G3D_BIND_MDL = 1,
    G3D_BIND_TEX,
    G3D_BIND_ANM,
};

// Which of NitroSystem's flushes the system sends the global state with
enum {
    G3D_FLUSH_P,
    G3D_FLUSH_VP,
    G3D_FLUSH_WVP,
};

typedef struct {
    u16 magic;
    u16 type;
    NNSG3dResFileHeader *data;
} G3DResource;

struct G3DModel {
    u16 magic;
    NNSG3dRenderObj *renderObj;
    G3DResource *mdlResource;
    G3DResource *texResource;
};

typedef struct {
    u16 magic;
    NNSG3dAnmObj *anmObj;
    G3DResource *resource;
} G3DAnimation;

struct G3DActor {
    G3DModel *model;
    G3DAnimation **animations;
    u16 animationCount;
    u16 magic;
};

typedef struct {
    NNSFndAllocator allocator;
    // The work of the linked-list VRAM managers, NULL with frame ones
    void *pltVramWork;
    void *texVramWork;
    HeapID heapId;
    G3DCameraProjection projection;
    Light lights[G3D_LIGHT_MAX];
    FxLookAt lookAt;
    u32 sortMode;
    u32 bufferMode;
    void (*flush)(void);
    BOOL frmTexVramMgr;
    BOOL frmPltVramMgr;
    BOOL swapRequested;
    // Called before each model is drawn
    void (*drawCallback)(NNSG3dRenderObj *obj, void *arg);
    void *drawArg;
} G3DSystem;

static struct {
    G3DSystem *sys;
    // What has been drawn since the counts were reset
    u32 polygonCount;
    u32 vertexCount;
} g_G3DSystem;

static const Light DEFAULT_GX_LIGHT = { { -FX16_ONE + 1, -FX16_ONE + 1, -FX16_ONE + 1 }, GX_RGB(31, 31, 31) };
static const G3DCameraProjection DEFAULT_CAMERA_PROJECTION = {
    G3DCAM_PROJECTION_PERSPECTIVE, 0, 0, FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0,
};
static const FxLookAt DEFAULT_LOOKAT = {
    { 0, 0, FX32_CONST(256) },
    { 0, FX32_ONE, 0 },
    { 0, 0, 0 },
};

static void GFL_G3DSysMtxSetFlushMode(int mode);
static void GFL_G3DResTrimTexData(G3DResource *resource);
static BOOL GFL_G3DResAllocTexMem(NNSG3dResTex *tex, NNSGfdTexKey *texKey, NNSGfdTexKey *tex4x4Key);
static BOOL GFL_G3DResAllocPltMem(NNSG3dResTex *tex, NNSGfdPlttKey *pltKey);
static BOOL GFL_G3DActorTestBBox(G3DActor *actor);
static int TestModelBoundingBox(const s16 *box);

// bwbattle: the VRAM keys BW2's resources hold. BW2 frees few of them, as its battle destroys the VRAM managers
// when it ends; inside Platinum's battle the managers outlive the view, so GFL_G3DSysFreeShared frees what's left
#define BW_VRAM_KEYS_MAX 512

typedef struct {
    u32 key;
    BOOL palette;
} BwVramKey;

static BwVramKey sBwVramKeys[BW_VRAM_KEYS_MAX];

static void BwVramKeys_Add(u32 key, BOOL palette) {
    if (key == 0) {
        return;
    }
    for (int i = 0; i < BW_VRAM_KEYS_MAX; i++) {
        if (sBwVramKeys[i].key == 0) {
            sBwVramKeys[i].key = key;
            sBwVramKeys[i].palette = palette;
            return;
        }
    }
    BwShim_Log("too many VRAM keys to track");
}

static void BwVramKeys_Remove(u32 key, BOOL palette) {
    for (int i = 0; i < BW_VRAM_KEYS_MAX; i++) {
        if (sBwVramKeys[i].key == key && sBwVramKeys[i].palette == palette) {
            sBwVramKeys[i].key = 0;
            return;
        }
    }
}

static void BwVramKeys_FreeAll(void) {
    for (int i = 0; i < BW_VRAM_KEYS_MAX; i++) {
        if (sBwVramKeys[i].key != 0) {
            if (sBwVramKeys[i].palette) {
                NNS_GfdFreePlttVram(sBwVramKeys[i].key);
            } else {
                NNS_GfdFreeTexVram(sBwVramKeys[i].key);
            }
            sBwVramKeys[i].key = 0;
        }
    }
}


void GFL_G3DSysCreate(BOOL frmTexVramMgr, u32 texVramSize, BOOL frmPltVramMgr, u32 pltVramSize, u16 dtcmAllocSize,
                      const HeapID heapId, G3DSystemInitCallback initCallback) {
    G3DCameraProjection projection;
    FxLookAt lookAt;
    Light light;

    g_G3DSystem.sys = GFL_HeapAllocate(heapId, sizeof(G3DSystem), FALSE, "g3d_system.c", 92);
    g_G3DSystem.sys->heapId = heapId;
    g_G3DSystem.sys->frmTexVramMgr = frmTexVramMgr;
    g_G3DSystem.sys->frmPltVramMgr = frmPltVramMgr;
    NNS_G3DInit();
    G3X_InitMtxStack();
    NNS_G3DRenderStateInit();
    if (dtcmAllocSize != 0) {
        u32 size = dtcmAllocSize;

        if (size > G3D_DTCM_SIZE_MAX) {
            size = G3D_DTCM_SIZE_MAX;
        }
        NNS_G3dGeSetBuffer(GFL_HeapDTCMAllocate(size));
    }
    G3_SwapBuffers(GX_SORTMODE_AUTO, GX_BUFFERMODE_W);
    GFL_HeapCreateAllocator(&g_G3DSystem.sys->allocator, g_G3DSystem.sys->heapId, 4);
    // The VRAM sizes are in 128 KB slots of texture VRAM and 16 KB ones of palette VRAM
    if (frmTexVramMgr == FALSE) {
        u32 workSize = NNS_GfdGetLnkTexVramManagerWorkSize(texVramSize << 7);

        g_G3DSystem.sys->texVramWork = GFL_HeapAllocate(g_G3DSystem.sys->heapId, workSize, FALSE, "g3d_system.c", 126);
        NNS_GfdInitLnkTexVramManager(texVramSize << 17, 0, g_G3DSystem.sys->texVramWork, workSize, TRUE);
    } else {
        NNS_GfdInitFrmTexVramManager(texVramSize, TRUE);
        g_G3DSystem.sys->texVramWork = NULL;
    }
    if (frmPltVramMgr == FALSE) {
        u32 workSize = NNS_GfdGetLnkPlttVramManagerWorkSize(pltVramSize << 9);

        g_G3DSystem.sys->pltVramWork = GFL_HeapAllocate(g_G3DSystem.sys->heapId, workSize, FALSE, "g3d_system.c", 141);
        NNS_GfdInitLnkPlttVramManager(pltVramSize << 14, g_G3DSystem.sys->pltVramWork, workSize, TRUE);
    } else {
        // BUG: this sets up the frame texture VRAM manager with the size of palette VRAM, as the frame palette one
        // (NNS_GfdInitFrmPlttVramManager) was meant, and the game does not include that. It then clears the texture
        // manager's work instead of the palette one's, which GFL_G3DSysFree then frees
        NNS_GfdInitFrmTexVramManager(pltVramSize << 14, TRUE);
#ifdef BUGFIX
        g_G3DSystem.sys->pltVramWork = NULL;
#else
        g_G3DSystem.sys->texVramWork = NULL;
#endif
    }
    projection = DEFAULT_CAMERA_PROJECTION;
    projection.param1 = FX_SinIdx(DEG_TO_IDX(20));
    projection.param2 = FX_CosIdx(DEG_TO_IDX(20));
    GFL_G3DSysMtxSetProjection(&projection);
    light = DEFAULT_GX_LIGHT;
    GFL_G3DSysLightSet(0, &light);
    GFL_G3DSysLightSet(1, &light);
    GFL_G3DSysLightSet(2, &light);
    GFL_G3DSysLightSet(3, &light);
    lookAt = DEFAULT_LOOKAT;
    GFL_G3DSysMtxSetViewLookAt(&lookAt);
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_W);
    GFL_G3DSysMtxSetFlushMode(G3D_FLUSH_P);
    if (initCallback != NULL) {
        initCallback();
    } else {
        G3X_SetShading(GX_SHADING_TOON);
        G3X_AntiAlias(FALSE);
        G3X_AlphaTest(FALSE, 0);
        G3X_AlphaBlend(FALSE);
        G3X_EdgeMarking(FALSE);
        G3X_SetFog(FALSE, 0, 0, 0);
        G3X_SetClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
        G3_ViewPort(0, 0, 255, 191);
    }
    g_G3DSystem.sys->drawCallback = NULL;
    g_G3DSystem.sys->drawArg = NULL;
}

// bwbattle: GFL_G3DSysCreate inside another game's 3D setup. Platinum's battle has initialized the geometry engine and
// the VRAM managers, which BW2's resources then allocate from; this only sets up BW2's own state, as GFL_G3DSysCreate
// leaves it: the camera, the lights and the flush mode
void GFL_G3DSysCreateShared(const HeapID heapId) {
    G3DCameraProjection projection;
    FxLookAt lookAt;
    Light light;

    g_G3DSystem.sys = GFL_HeapAllocate(heapId, sizeof(G3DSystem), TRUE, "g3d_system.c", __LINE__);
    g_G3DSystem.sys->heapId = heapId;
    g_G3DSystem.sys->frmTexVramMgr = FALSE;
    g_G3DSystem.sys->frmPltVramMgr = FALSE;
    g_G3DSystem.sys->texVramWork = NULL;
    g_G3DSystem.sys->pltVramWork = NULL;
    GFL_HeapCreateAllocator(&g_G3DSystem.sys->allocator, g_G3DSystem.sys->heapId, 4);
    projection = DEFAULT_CAMERA_PROJECTION;
    projection.param1 = FX_SinIdx(DEG_TO_IDX(20));
    projection.param2 = FX_CosIdx(DEG_TO_IDX(20));
    GFL_G3DSysMtxSetProjection(&projection);
    light = DEFAULT_GX_LIGHT;
    GFL_G3DSysLightSet(0, &light);
    GFL_G3DSysLightSet(1, &light);
    GFL_G3DSysLightSet(2, &light);
    GFL_G3DSysLightSet(3, &light);
    lookAt = DEFAULT_LOOKAT;
    GFL_G3DSysMtxSetViewLookAt(&lookAt);
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_W);
    GFL_G3DSysMtxSetFlushMode(G3D_FLUSH_P);
    g_G3DSystem.sys->drawCallback = NULL;
    g_G3DSystem.sys->drawArg = NULL;
}

// bwbattle: frees what GFL_G3DSysCreateShared set up, leaving the geometry buffer and VRAM managers to Platinum
void GFL_G3DSysFreeShared(void) {
    BwVramKeys_FreeAll();
    GFL_HeapFree(g_G3DSystem.sys);
    g_G3DSystem.sys = NULL;
}

void GFL_G3DSysFree(void) {
    if (g_G3DSystem.sys->pltVramWork != NULL) {
        GFL_HeapFree(g_G3DSystem.sys->pltVramWork);
    }
    if (g_G3DSystem.sys->texVramWork != NULL) {
        GFL_HeapFree(g_G3DSystem.sys->texVramWork);
    }
    if (NNS_G3dGeIsBufferExist() == TRUE) {
        _freeBlkFromDTCM(NNS_G3dGeReleaseBuffer());
    }
    GFL_HeapFree(g_G3DSystem.sys);
    g_G3DSystem.sys = NULL;
}

void GFL_G3DSysMtxGetProjection(G3DCameraProjection *dest) {
    dest->type = g_G3DSystem.sys->projection.type;
    dest->param1 = g_G3DSystem.sys->projection.param1;
    dest->param2 = g_G3DSystem.sys->projection.param2;
    dest->param3 = g_G3DSystem.sys->projection.param3;
    dest->param4 = g_G3DSystem.sys->projection.param4;
    dest->near = g_G3DSystem.sys->projection.near;
    dest->far = g_G3DSystem.sys->projection.far;
    dest->ndcRangeOverride = g_G3DSystem.sys->projection.ndcRangeOverride;
}

// bwbattle: the 3D view's width (GFL_G3DSysSetViewWidth)
static u32 sBwViewWidth = 256;

void GFL_G3DSysSetViewWidth(u32 width) {
    sBwViewWidth = width < 256 ? 256 : width;
}

u32 GFL_G3DSysGetViewWidth(void) {
    return sBwViewWidth;
}

// A projection for the 256 dots made for the wider view: its clip x scaled by 256 / width, which keeps a perspective
// one's angle and widens an orthographic one's range (BW2's screen-space particles, ±4 by ±3, would stretch)
static void BwG3d_NarrowProjection(void) {
    if (sBwViewWidth > 256) {
        MtxFx44 mtx = *NNS_G3dGlbGetProjectionMtx();
        fx32 factor = FX_Div(256 * FX32_ONE, sBwViewWidth * FX32_ONE);

        mtx._00 = FX_Mul(mtx._00, factor);
        mtx._10 = FX_Mul(mtx._10, factor);
        mtx._20 = FX_Mul(mtx._20, factor);
        mtx._30 = FX_Mul(mtx._30, factor);
        NNS_G3dGlbSetProjectionMtx(&mtx);
    }
}

int GFL_G3DSysProjectToScreen(const VecFx32 *world, int *x, int *y) {
    int result = NNS_G3dWorldPosToScrPos(world, x, y);

    // NitroSystem's viewport is the 256 dots, which the narrowed projection squeezes the scene into
    *x = 128 + (*x - 128) * (int)sBwViewWidth / 256;
    return result;
}

void GFL_G3DSysMtxSetProjection(const G3DCameraProjection *projection) {
    G3DSystem *sys;

    g_G3DSystem.sys->projection.type = projection->type;
    g_G3DSystem.sys->projection.param1 = projection->param1;
    g_G3DSystem.sys->projection.param2 = projection->param2;
    g_G3DSystem.sys->projection.param3 = projection->param3;
    g_G3DSystem.sys->projection.param4 = projection->param4;
    g_G3DSystem.sys->projection.near = projection->near;
    g_G3DSystem.sys->projection.far = projection->far;
    g_G3DSystem.sys->projection.ndcRangeOverride = projection->ndcRangeOverride;
    sys = g_G3DSystem.sys;
    switch (sys->projection.type) {
    case G3DCAM_PROJECTION_PERSPECTIVE:
        if (sys->projection.ndcRangeOverride == 0) {
            NNS_G3dGlbPerspective(sys->projection.param1, sys->projection.param2, sys->projection.param3,
                                  sys->projection.near, sys->projection.far);
        } else {
            NNS_G3dGlbPerspectiveW(sys->projection.param1, sys->projection.param2, sys->projection.param3,
                                   sys->projection.near, sys->projection.far, sys->projection.ndcRangeOverride);
        }
        BwG3d_NarrowProjection();
        break;
    case G3DCAM_PROJECTION_FRUSTUM:
        if (sys->projection.ndcRangeOverride == 0) {
            NNS_G3dGlbFrustum(sys->projection.param1, sys->projection.param2, sys->projection.param3,
                              sys->projection.param4, sys->projection.near, sys->projection.far);
        } else {
            NNS_G3dGlbFrustumW(sys->projection.param1, sys->projection.param2, sys->projection.param3,
                               sys->projection.param4, sys->projection.near, sys->projection.far,
                               sys->projection.ndcRangeOverride);
        }
        BwG3d_NarrowProjection();
        break;
    case G3DCAM_PROJECTION_ORTHO:
        if (sys->projection.ndcRangeOverride == 0) {
            NNS_G3dGlbOrtho(sys->projection.param1, sys->projection.param2, sys->projection.param3,
                            sys->projection.param4, sys->projection.near, sys->projection.far);
        } else {
            NNS_G3dGlbOrthoW(sys->projection.param1, sys->projection.param2, sys->projection.param3,
                             sys->projection.param4, sys->projection.near, sys->projection.far,
                             sys->projection.ndcRangeOverride);
        }
        BwG3d_NarrowProjection();
        break;
    }
}

void GFL_G3DSysLightSet(u8 lightId, const Light *light) {
    g_G3DSystem.sys->lights[lightId] = *light;
    NNS_G3DSetLightVector(lightId, g_G3DSystem.sys->lights[lightId].direction.x,
                          g_G3DSystem.sys->lights[lightId].direction.y, g_G3DSystem.sys->lights[lightId].direction.z);
    NNS_G3DSetLightColor(lightId, g_G3DSystem.sys->lights[lightId].color);
}

void GFL_G3DSysMtxGetViewLookAt(FxLookAt *dest) {
    G3DSystem *sys = g_G3DSystem.sys;

    dest->position = sys->lookAt.position;
    dest->upVector = sys->lookAt.upVector;
    dest->target = sys->lookAt.target;
}

void GFL_G3DSysMtxSetViewLookAt(const FxLookAt *lookAt) {
    G3DSystem *sys = g_G3DSystem.sys;

    sys->lookAt.position = lookAt->position;
    sys->lookAt.upVector = lookAt->upVector;
    sys->lookAt.target = lookAt->target;
    NNS_G3dGlbLookAt(&sys->lookAt.position, &sys->lookAt.upVector, &sys->lookAt.target);
}

void GFL_G3DSysSetSwapBufferParams(u32 sortMode, u32 bufferMode) {
    g_G3DSystem.sys->sortMode = sortMode;
    g_G3DSystem.sys->bufferMode = bufferMode;
}

BOOL func_02049228(void) {
    return g_G3DSystem.sys->frmTexVramMgr;
}

BOOL func_02049238(void) {
    return g_G3DSystem.sys->frmPltVramMgr;
}

static void GFL_G3DSysMtxSetFlushMode(int mode) {
    switch (mode) {
    case G3D_FLUSH_P:
    default:
        g_G3DSystem.sys->flush = NNS_G3dGlbFlush;
        break;
    case G3D_FLUSH_VP:
        g_G3DSystem.sys->flush = NNS_G3DFlushViewProjection;
        break;
    case G3D_FLUSH_WVP:
        g_G3DSystem.sys->flush = NNS_G3DFlushModelViewProjection;
        break;
    }
}

u32 GFL_G3DResGetAllocSize(void) {
    return sizeof(G3DResource);
}

void GFL_G3DResBindData(void *resource, u32 kind, void *data) {
    G3DResource *res = resource;

    switch (kind) {
    case G3D_BIND_MDL:
        res->type = G3D_RES_TYPE_MDL;
        break;
    case G3D_BIND_TEX:
        res->type = G3D_RES_TYPE_TEX;
        break;
    case G3D_BIND_ANM:
        res->type = G3D_RES_TYPE_ANM;
        break;
    }
    res->data = data;
    res->magic = G3D_RES_MAGIC;
}

void GFL_G3DResSetup(void *resource, void *data) {
    G3DResource *res = resource;
    NNSG3dResFileHeader *header = data;
    u16 type;

    switch (header->sigVal) {
    case NNS_G3D_SIGNATURE_BMD0:
        if (NNS_G3DResGetTexBlock(header) == NULL) {
            type = G3D_RES_TYPE_MDL;
        } else {
            type = G3D_RES_TYPE_MDL_TEX;
        }
        break;
    case NNS_G3D_SIGNATURE_BTX0:
        type = G3D_RES_TYPE_TEX;
        break;
    case NNS_G3D_SIGNATURE_BCA0:
    case NNS_G3D_SIGNATURE_BVA0:
    case NNS_G3D_SIGNATURE_BMA0:
    case NNS_G3D_SIGNATURE_BTA0:
    case NNS_G3D_SIGNATURE_BTP0:
        type = G3D_RES_TYPE_ANM;
        break;
    default:
        type = G3D_RES_TYPE_NONE;
        break;
    }
    res->type = type;
    res->data = header;
    res->magic = G3D_RES_MAGIC;
}

void *GFL_G3DSysReadArcSysResource(u32 arcId, u32 fileId) {
    G3DResource *res = GFL_HeapAllocate(g_G3DSystem.sys->heapId, sizeof(G3DResource), FALSE, "g3d_system.c", 692);

    GFL_G3DResSetup(res, GFL_ArcSysReadHeapNew(arcId, fileId, g_G3DSystem.sys->heapId));
    return res;
}

void *GFL_G3DSysReadArcToolResource(ArcTool *handle, u32 fileId) {
    G3DResource *res = GFL_HeapAllocate(g_G3DSystem.sys->heapId, sizeof(G3DResource), FALSE, "g3d_system.c", 712);

    GFL_G3DResSetup(res, GFL_ArcToolReadHeapNew(handle, fileId, g_G3DSystem.sys->heapId));
    return res;
}

void *GFL_G3DSysReadFSResource(const char *path, u32 fileId) {
    G3DResource *res = GFL_HeapAllocate(g_G3DSystem.sys->heapId, sizeof(G3DResource), FALSE, "g3d_system.c", 732);

    GFL_G3DResSetup(res, GFL_ArcSysReadHeapNewDirect(path, fileId, g_G3DSystem.sys->heapId));
    return res;
}

void GFL_G3DResFree(void *resource) {
    G3DResource *res = resource;

    if (res != NULL && res->magic == G3D_RES_MAGIC) {
        res->magic = G3D_RES_MAGIC - 1;
        if (res->data != NULL) {
            GFL_HeapFree(res->data);
            res->data = NULL;
        }
        GFL_HeapFree(res);
    }
}

void *GFL_G3DResGetResData(void *resource) {
    return ((G3DResource *)resource)->data;
}

BOOL GFL_G3DResCheckType(void *resource, u32 type) {
    u16 resType = ((G3DResource *)resource)->type;

    switch (type) {
    case G3D_RES_CHECK_NONE:
        return resType == G3D_RES_TYPE_NONE;
    case G3D_RES_CHECK_MDL:
        if (resType == G3D_RES_TYPE_MDL_TEX || resType == G3D_RES_TYPE_MDL) {
            return TRUE;
        }
        return FALSE;
    case G3D_RES_CHECK_TEX:
        if (resType == G3D_RES_TYPE_MDL_TEX || resType == G3D_RES_TYPE_TEX) {
            return TRUE;
        }
        return FALSE;
    case G3D_RES_CHECK_ANM:
        return resType == G3D_RES_TYPE_ANM;
    }
    return FALSE;
}

// Frees the texture images, which are at the end of the file, once they are in VRAM
static void GFL_G3DResTrimTexData(G3DResource *resource) {
    NNSG3dResFileHeader *data = resource->data;
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(data);

    GFL_HeapResize(data, (u8 *)tex + tex->texInfo.ofsTex - (u8 *)data);
}

BOOL GFL_G3DResUploadTexData(void *resource) {
    NNSG3dResFileHeader *data = ((G3DResource *)resource)->data;
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(data);
    NNSGfdTexKey texKey;
    NNSGfdTexKey tex4x4Key;
    NNSGfdPlttKey pltKey;

    if (tex != NULL) {
        texKey = tex4x4Key = 0;
        pltKey = 0;
        if (!GFL_G3DResAllocTexMem(tex, &texKey, &tex4x4Key)) {
            // bwbattle: a texture that doesn't fit is drawn without it; say so
            BwShim_Log("texture VRAM full: %u bytes, %u in 4x4", NNS_G3DResTexGetTexDataSize(tex),
                       NNS_G3dTexGetRequiredSize4x4(tex));
            return FALSE;
        }
        if (!GFL_G3DResAllocPltMem(tex, &pltKey)) {
            BwShim_Log("palette VRAM full: %u bytes", NNS_G3DResTexGetPaletteDataSize(tex));
            return FALSE;
        }
        NNS_G3DResTexSetGfdKeys(tex, texKey, tex4x4Key);
        NNS_G3DResTexSetPltGfdKey(tex, pltKey);
        DC_FlushRange(data, data->fileSize);
        NNS_G3DResTexUploadTex(tex, TRUE);
        NNS_G3DResTexUploadPlt(tex, TRUE);
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_G3DResUploadAndReleaseTexData(void *resource) {
    if (!GFL_G3DResUploadTexData(resource)) {
        return FALSE;
    }
    GFL_G3DResTrimTexData(resource);
    return TRUE;
}

BOOL GFL_G3DResFreeTexData(void *resource) {
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(((G3DResource *)resource)->data);
    NNSGfdTexKey texKey;
    NNSGfdTexKey tex4x4Key;
    BOOL texLoaded = FALSE;
    BOOL tex4x4Loaded = FALSE;

    if (tex->texInfo.flag & NNS_G3D_RESTEX_LOADED) {
        texLoaded = TRUE;
    }
    if (tex->tex4x4Info.flag & NNS_G3D_RESTEX_LOADED) {
        tex4x4Loaded = TRUE;
    }
    NNS_G3dTexReleaseTexKey(tex, &texKey, &tex4x4Key);
    if (texLoaded) {
        BwVramKeys_Remove(texKey, FALSE);
        NNS_GfdFreeTexVram(texKey);
    }
    if (tex4x4Loaded) {
        BwVramKeys_Remove(tex4x4Key, FALSE);
        NNS_GfdFreeTexVram(tex4x4Key);
    }
    if ((tex->plttInfo.flag & NNS_G3D_RESPLTT_LOADED) && tex->plttInfo.sizePltt != 0) {
        NNSGfdPlttKey plttKey = NNS_G3dPlttReleasePlttKey(tex);

        BwVramKeys_Remove(plttKey, TRUE);
        NNS_GfdFreePlttVram(plttKey);
    }
    return TRUE;
}

BOOL GFL_G3DResSetupTexData(void *resource) {
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(((G3DResource *)resource)->data);
    NNSGfdTexKey texKey;
    NNSGfdTexKey tex4x4Key;
    NNSGfdPlttKey pltKey;

    if (tex != NULL) {
        texKey = tex4x4Key = 0;
        pltKey = 0;
        if (!GFL_G3DResAllocTexMem(tex, &texKey, &tex4x4Key)) {
            // bwbattle: a texture that doesn't fit is drawn without it; say so
            BwShim_Log("texture VRAM full: %u bytes, %u in 4x4", NNS_G3DResTexGetTexDataSize(tex),
                       NNS_G3dTexGetRequiredSize4x4(tex));
            return FALSE;
        }
        if (!GFL_G3DResAllocPltMem(tex, &pltKey)) {
            BwShim_Log("palette VRAM full: %u bytes", NNS_G3DResTexGetPaletteDataSize(tex));
            return FALSE;
        }
        NNS_G3DResTexSetGfdKeys(tex, texKey, tex4x4Key);
        NNS_G3DResTexSetPltGfdKey(tex, pltKey);
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_G3DResUploadTexDataCore(void *resource) {
    NNSG3dResFileHeader *data = ((G3DResource *)resource)->data;
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(data);

    if (tex != NULL) {
        DC_FlushRange(data, data->fileSize);
        NNS_G3DResTexUploadTex(tex, TRUE);
        NNS_G3DResTexUploadPlt(tex, TRUE);
        return TRUE;
    }
    return FALSE;
}

NNSG3dResTex *GFL_G3DResGetTexData(void *resource) {
    return NNS_G3DResGetTexBlock(((G3DResource *)resource)->data);
}

static BOOL GFL_G3DResAllocTexMem(NNSG3dResTex *tex, NNSGfdTexKey *texKey, NNSGfdTexKey *tex4x4Key) {
    u32 texSize = NNS_G3DResTexGetTexDataSize(tex);
    u32 tex4x4Size = NNS_G3dTexGetRequiredSize4x4(tex);

    if (texSize != 0) {
        *texKey = NNS_GfdAllocTexVram(texSize, FALSE, 0);
        if (*texKey == 0) {
            return FALSE;
        }
        BwVramKeys_Add(*texKey, FALSE);
    }
    if (tex4x4Size != 0) {
        // BUG: this allocates for the 4x4 compressed textures the size of the other textures
#ifdef BUGFIX
        *tex4x4Key = NNS_GfdAllocTexVram(tex4x4Size, TRUE, 0);
#else
        *tex4x4Key = NNS_GfdAllocTexVram(texSize, TRUE, 0);
#endif
        if (*tex4x4Key == 0) {
            return FALSE;
        }
        BwVramKeys_Add(*tex4x4Key, FALSE);
    }
    return TRUE;
}

static BOOL GFL_G3DResAllocPltMem(NNSG3dResTex *tex, NNSGfdPlttKey *pltKey) {
    u32 size = NNS_G3DResTexGetPaletteDataSize(tex);

    if (size != 0) {
        *pltKey = NNS_GfdAllocPlttVram(size, tex->tex4x4Info.flag & NNS_G3D_RESPLTT_USEPLTT4, 0);
        if (*pltKey == 0) {
            return FALSE;
        }
        BwVramKeys_Add(*pltKey, TRUE);
    }
    return TRUE;
}

BOOL GFL_G3DResIsTexUploadDone(void *resource) {
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(((G3DResource *)resource)->data);

    if ((tex->texInfo.flag & NNS_G3D_RESTEX_LOADED) || (tex->tex4x4Info.flag & NNS_G3D_RESTEX_LOADED) ||
        (tex->plttInfo.flag & NNS_G3D_RESPLTT_LOADED)) {
        return TRUE;
    }
    return FALSE;
}

NNSGfdTexKey GFL_G3DResGetTexVRAMHandle(void *resource) {
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(((G3DResource *)resource)->data);

    if (tex->texInfo.flag & NNS_G3D_RESTEX_LOADED) {
        return tex->texInfo.vramKey;
    }
    if (tex->tex4x4Info.flag & NNS_G3D_RESTEX_LOADED) {
        return tex->tex4x4Info.vramKey;
    }
    return 0;
}

NNSGfdPlttKey GFL_G3DResGetPltVRAMHandle(void *resource) {
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(((G3DResource *)resource)->data);

    if (tex->plttInfo.flag & NNS_G3D_RESPLTT_LOADED) {
        return tex->plttInfo.vramKey;
    }
    return 0;
}

void *GFL_G3DResGetTexImageData(void *resource) {
    NNSG3dResTex *tex = GFL_G3DResGetTexData(resource);

    return (u8 *)tex + tex->texInfo.ofsTex;
}

void *GFL_G3DResGetTexPaletteData(void *resource) {
    NNSG3dResTex *tex = GFL_G3DResGetTexData(resource);

    return (u8 *)tex + tex->plttInfo.ofsPlttData;
}

// A model of a model set that may be missing. The missing set and the missing entry share one NULL
static inline NNSG3dResMdl *GFL_G3DGetMdlByIdx(const NNSG3dResMdlSet *mdlSet, u32 idx) {
    const u32 *ofs;

    if (mdlSet != NULL) {
        ofs = NNS_G3dGetResDataByIdx(&mdlSet->dict, idx);
        if (ofs != NULL) {
            return (NNSG3dResMdl *)((u8 *)mdlSet + *ofs);
        }
    }
    return NULL;
}

G3DModel *GFL_G3DMdlCreate(void *resource, u32 modelId, void *texture) {
    NNSG3dResMdl *mdl;
    NNSG3dResTex *tex = NULL;
    G3DModel *model;

    mdl = GFL_G3DGetMdlByIdx(NNS_G3DResGetMdlBlock(((G3DResource *)resource)->data), modelId);
    if (texture != NULL) {
        tex = NNS_G3DResGetTexBlock(((G3DResource *)texture)->data);
    }
    model = GFL_HeapAllocate(g_G3DSystem.sys->heapId, sizeof(G3DModel), FALSE, "g3d_system.c", 1544);
    model->magic = G3D_MDL_MAGIC;
    model->mdlResource = resource;
    model->texResource = texture;
    model->renderObj = NNS_G3dAllocRenderObj(&g_G3DSystem.sys->allocator);
    if (tex != NULL) {
        NNS_G3dBindMdlTex(mdl, tex);
        NNS_G3dBindMdlPltt(mdl, tex);
    }
    NNS_G3DModelAttachResource(model->renderObj, mdl);
    return model;
}

void GFL_G3DMdlFree(G3DModel *model) {
    if (model != NULL && model->magic == G3D_MDL_MAGIC) {
        NNS_G3dFreeRenderObj(&g_G3DSystem.sys->allocator, model->renderObj);
        model->magic = G3D_MDL_MAGIC - 1;
        GFL_HeapFree(model);
    }
}

void *GFL_G3DMdlGetMdlResource(G3DModel *model) {
    return model->mdlResource;
}

void *GFL_G3DMdlGetTexResource(G3DModel *model) {
    return model->texResource;
}

NNSG3dRenderObj *GFL_G3DMdlGetEngineModel(G3DModel *model) {
    return model->renderObj;
}

void *GFL_G3DAnmCreate(G3DModel *model, void *resource, u32 anmIdx) {
    G3DAnimation *animation =
        GFL_HeapAllocate(g_G3DSystem.sys->heapId, sizeof(G3DAnimation), FALSE, "g3d_system.c", 1746);
    NNSG3dResTex *tex = NULL;
    void *anm;
    NNSG3dResMdl *mdl;
    void *data;

    animation->magic = G3D_ANM_MAGIC;
    data = ((G3DResource *)resource)->data;
    animation->resource = resource;
    anm = NNS_G3DResGetAnm(data, anmIdx);
    mdl = model->renderObj->resMdl;
    if (GFL_G3DMdlGetTexResource(model) != NULL) {
        tex = NNS_G3DResGetTexBlock(model->texResource->data);
    }
    animation->anmObj = NNS_G3DAnimationAlloc(&g_G3DSystem.sys->allocator, anm, mdl);
    NNS_G3DAnimationCreate(animation->anmObj, anm, mdl, tex);
    return animation;
}

void GFL_G3DAnmFree(void *anm) {
    G3DAnimation *animation = anm;

    if (animation != NULL && animation->magic == G3D_ANM_MAGIC) {
        NNS_G3dFreeAnmObj(&g_G3DSystem.sys->allocator, animation->anmObj);
        animation->magic = G3D_ANM_MAGIC - 1;
        GFL_HeapFree(animation);
    }
}

NNSG3dAnmObj *GFL_G3DAnmGetRenderObj(void *anm) {
    return ((G3DAnimation *)anm)->anmObj;
}

G3DActor *GFL_G3DActorCreate(G3DModel *model, void **animations, int count) {
    G3DActor *actor = GFL_HeapAllocate(g_G3DSystem.sys->heapId, sizeof(G3DActor), FALSE, "g3d_system.c", 1882);
    int i;

    actor->magic = G3D_ACTOR_MAGIC;
    actor->model = model;
    actor->animationCount = count;
    if (count == 0) {
        actor->animations = NULL;
    } else {
        actor->animations =
            GFL_HeapAllocate(g_G3DSystem.sys->heapId, count * sizeof(G3DAnimation *), TRUE, "g3d_system.c", 1892);
        if (animations != NULL) {
            for (i = 0; i < count; i++) {
                actor->animations[i] = animations[i];
            }
        }
    }
    return actor;
}

void GFL_G3DActorFree(G3DActor *actor) {
    if (actor != NULL && actor->magic == G3D_ACTOR_MAGIC) {
        if (actor->animations != NULL) {
            GFL_HeapFree(actor->animations);
            actor->animations = NULL;
        }
        actor->magic = G3D_ACTOR_MAGIC - 1;
        GFL_HeapFree(actor);
    }
}

G3DModel *GFL_G3DActorGetMdl(G3DActor *actor) {
    return actor->model;
}

void *GFL_G3DActorGetAnm(G3DActor *actor, u16 anmIdx) {
    return actor->animations[anmIdx];
}

u16 GFL_G3DActorGetAnmCount(G3DActor *actor) {
    return actor->animationCount;
}

BOOL GFL_G3DActorBindAnm(G3DActor *actor, u16 anmIdx) {
    G3DAnimation *animation = actor->animations[anmIdx];

    if (animation == NULL) {
        return FALSE;
    }
    NNS_G3DAnimationBind(actor->model->renderObj, animation->anmObj);
    return TRUE;
}

BOOL GFL_G3DActorUnbindAnm(G3DActor *actor, u16 anmIdx) {
    G3DAnimation *animation = actor->animations[anmIdx];

    if (animation == NULL) {
        return FALSE;
    }
    NNS_G3dRenderObjRemoveAnmObj(actor->model->renderObj, animation->anmObj);
    return TRUE;
}

BOOL GFL_G3DActorResetAnmFrame(G3DActor *actor, u16 anmIdx) {
    G3DAnimation *animation = actor->animations[anmIdx];

    if (animation == NULL) {
        return FALSE;
    }
    animation->anmObj->frame = 0;
    return TRUE;
}

BOOL GFL_G3DActorGetAnmFrame(G3DActor *actor, u16 anmIdx, fx32 *frame) {
    G3DAnimation *animation = actor->animations[anmIdx];

    if (animation == NULL) {
        return FALSE;
    }
    *frame = animation->anmObj->frame;
    return TRUE;
}

BOOL GFL_G3DActorSetAnmFrame(G3DActor *actor, u16 anmIdx, fx32 *frame) {
    G3DAnimation *animation = actor->animations[anmIdx];
    NNSG3dAnmObj *anmObj;

    if (animation == NULL) {
        return FALSE;
    }
    anmObj = animation->anmObj;
    if (*frame >= NNS_G3dAnmObjGetNumFrame(anmObj)) {
        anmObj->frame = NNS_G3dAnmObjGetNumFrame(anmObj);
    } else {
        anmObj->frame = *frame;
    }
    return TRUE;
}

BOOL GFL_G3DActorGetAnmFrameCount(G3DActor *actor, u16 anmIdx, fx32 *count) {
    G3DAnimation *animation = actor->animations[anmIdx];

    if (animation == NULL) {
        return FALSE;
    }
    *count = NNS_G3dAnmObjGetNumFrame(animation->anmObj);
    return TRUE;
}

BOOL GFL_G3DActorStepAnmFrame(G3DActor *actor, u16 anmIdx, fx32 addend) {
    G3DAnimation *animation = actor->animations[anmIdx];
    NNSG3dAnmObj *anmObj;

    if (animation == NULL) {
        return FALSE;
    }
    anmObj = animation->anmObj;
    anmObj->frame += addend;
    if (anmObj->frame >= NNS_G3dAnmObjGetNumFrame(anmObj)) {
        anmObj->frame = NNS_G3dAnmObjGetNumFrame(anmObj) - FX32_ONE;
        return FALSE;
    }
    if (anmObj->frame < 0) {
        anmObj->frame = 0;
        return FALSE;
    }
    return TRUE;
}

BOOL GFL_G3DActorStepAnmFrameLoop(G3DActor *actor, u16 anmIdx, fx32 addend) {
    G3DAnimation *animation = actor->animations[anmIdx];
    NNSG3dAnmObj *anmObj;

    if (animation == NULL) {
        return FALSE;
    }
    anmObj = animation->anmObj;
    anmObj->frame += addend;
    if (anmObj->frame >= NNS_G3dAnmObjGetNumFrame(anmObj)) {
        anmObj->frame = 0;
        return FALSE;
    }
    if (anmObj->frame < 0) {
        anmObj->frame = NNS_G3dAnmObjGetNumFrame(anmObj);
        return FALSE;
    }
    return TRUE;
}

void GFL_G3DSysReset(void) {
    G3X_Reset();
}

void GFL_G3DSysReqSwapBuffers(void) {
    NNS_G3DWaitFIFO();
    g_G3DSystem.sys->swapRequested = TRUE;
}

// bwbattle: sends BW2's projection, camera and lights to NitroSystem's global state again. The host game draws its own
// 3D between BW2's frames (Platinum's particles and sprites) and leaves its camera in that state
void GFL_G3DSysReapplyState(void) {
    G3DCameraProjection projection = g_G3DSystem.sys->projection;

    GFL_G3DSysMtxSetProjection(&projection);
    NNS_G3dGlbLookAt(&g_G3DSystem.sys->lookAt.position, &g_G3DSystem.sys->lookAt.upVector,
                     &g_G3DSystem.sys->lookAt.target);
    for (u8 i = 0; i < G3D_LIGHT_MAX; i++) {
        Light light = g_G3DSystem.sys->lights[i];

        GFL_G3DSysLightSet(i, &light);
    }
}

// bwbattle: drops a swap request, when the host game swaps the buffers itself
void GFL_G3DSysClearSwapRequest(void) {
    if (g_G3DSystem.sys != NULL) {
        g_G3DSystem.sys->swapRequested = FALSE;
    }
}

void GFL_G3DSysCheckSwapBuffers(void) {
    if (g_G3DSystem.sys != NULL && g_G3DSystem.sys->swapRequested == TRUE) {
        G3_SwapBuffers(g_G3DSystem.sys->sortMode, g_G3DSystem.sys->bufferMode);
        g_G3DSystem.sys->swapRequested = FALSE;
    }
}

void GFL_G3DSysMtxViewFlush(void) {
    NNS_G3dGlbLookAt(&g_G3DSystem.sys->lookAt.position, &g_G3DSystem.sys->lookAt.upVector,
                     &g_G3DSystem.sys->lookAt.target);
    g_G3DSystem.sys->flush();
}

void GFL_G3DSysDrawObj(G3DActor *obj, const SRTMatrix *mdlMtx) {
    NNSG3dRenderObj *renderObj = GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(obj));

    NNS_G3dGlbSetBaseTrans(&mdlMtx->translation);
    NNS_G3dGlbSetBaseRot(&mdlMtx->rotation);
    NNS_G3dGlbSetBaseScale(&mdlMtx->scale);
    g_G3DSystem.sys->flush();
    GFL_G3DSysDispatchDraw(renderObj);
}

BOOL GFL_G3DSysDrawObjBBoxCull(G3DActor *obj, SRTMatrix *mdlMtx) {
    NNSG3dRenderObj *renderObj = GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(obj));
    BOOL visible;

    NNS_G3dGlbSetBaseTrans(&mdlMtx->translation);
    NNS_G3dGlbSetBaseRot(&mdlMtx->rotation);
    NNS_G3dGlbSetBaseScale(&mdlMtx->scale);
    g_G3DSystem.sys->flush();
    visible = GFL_G3DActorTestBBox(obj);
    if (visible == TRUE) {
        GFL_G3DSysDispatchDraw(renderObj);
    }
    return visible;
}

// Whether the model's bounding box is in view, from the geometry engine's box test
static BOOL GFL_G3DActorTestBBox(G3DActor *actor) {
    BOOL visible = TRUE;
    NNSG3dRenderObj *renderObj = GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(actor));
    NNSG3dResMdlInfo *info;
    s16 box[6];
    VecFx32 scale;
    u32 pop;

    if (renderObj->resMdl != NULL) {
        info = NNS_G3dGetMdlInfo(renderObj->resMdl);
    } else {
        info = NULL;
    }
    box[0] = info->boxX;
    box[1] = info->boxY;
    box[2] = info->boxZ;
    box[3] = info->boxW;
    box[4] = info->boxH;
    box[5] = info->boxD;
    NNS_G3DExecOp(G3OP_MTX_PUSH, NULL, 0);
    VEC_Set(&scale, info->boxPosScale, info->boxPosScale, info->boxPosScale);
    NNS_G3DExecOp(G3OP_MTX_SCALE, (u32 *)&scale, 3);
    if (TestModelBoundingBox(box) == 0) {
        visible = FALSE;
    }
    pop = 1;
    NNS_G3DExecOp(G3OP_MTX_POP, &pop, 1);
    return visible;
}

static int TestModelBoundingBox(const s16 *box) {
    s32 result = 1;
    u32 polygonAttr = GX_PACK_POLYGONATTR_PARAM(GX_LIGHTMASK_0, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 0,
                                                GX_POLYGON_ATTR_MISC_FAR_CLIPPING | GX_POLYGON_ATTR_MISC_DISP_1DOT);
    u32 begin;

    NNS_G3DExecOp(G3OP_POLYGON_ATTR, &polygonAttr, 1);
    begin = 0;
    NNS_G3DExecOp(G3OP_BEGIN, &begin, 1);
    NNS_G3DExecOp(G3OP_END, NULL, 0);
    NNS_G3DExecOp(G3OP_BOX_TEST, (const u32 *)box, 3);
    NNS_G3DWaitFIFO();
    while (G3X_GetBoxTestResult(&result) != 0) {
    }
    return result;
}

void GFL_G3DSysDispatchDraw(NNSG3dRenderObj *renderObj) {
    g_G3DSystem.polygonCount += renderObj->resMdl->info.numPolygon;
    g_G3DSystem.vertexCount += renderObj->resMdl->info.numVertex;
    if (g_G3DSystem.sys->drawCallback != NULL) {
        g_G3DSystem.sys->drawCallback(renderObj, g_G3DSystem.sys->drawArg);
    }
    NNS_G3DDraw(renderObj);
}

void GFL_G3DSysResetGeometryCounter(void) {
    g_G3DSystem.polygonCount = 0;
    g_G3DSystem.vertexCount = 0;
}
