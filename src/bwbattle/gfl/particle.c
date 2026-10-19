#include "types.h"
#include "shim/bw_shim.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"
#include "spl/spl.h"

// Particle systems over SPL managers: each allocates from its own work memory through one of 16 allocators, and gives
// back the VRAM its resource took when freed

// Most emitters and particles a system has at once
#define PARTICLE_EMITTER_MAX 24
#define PARTICLE_MAX 200

// The kinds of an emitter's behaviors, as func_02050254 finds them
enum {
    PARTICLE_BEHAVIOR_GRAVITY,
    PARTICLE_BEHAVIOR_RANDOM,
    PARTICLE_BEHAVIOR_MAGNET,
    PARTICLE_BEHAVIOR_SPIN,
    PARTICLE_BEHAVIOR_COLLISION_PLANE,
    PARTICLE_BEHAVIOR_CONVERGENCE,
};

typedef struct {
    ParticleSystem *systems[PARTICLE_SYSTEM_MAX];
    // What func_0205007c's callback reads through func_02050188
    void *callbackParam;
    // The system whose resource is being uploaded, which the linked-list VRAM allocators record keys in
    ParticleSystem *uploading;
} ParticleGlobal;

static ParticleGlobal *sParticle;

static void *func_0204fb78(u32 size);
static void *func_0204fba0(u32 size);
static void *func_0204fbc8(u32 size);
static void *func_0204fbf0(u32 size);
static void *func_0204fc18(u32 size);
static void *func_0204fc40(u32 size);
static void *func_0204fc68(u32 size);
static void *func_0204fc90(u32 size);
static void *func_0204fcb8(u32 size);
static void *func_0204fce0(u32 size);
static void *func_0204fd08(u32 size);
static void *func_0204fd30(u32 size);
static void *func_0204fd58(u32 size);
static void *func_0204fd80(u32 size);
static void *func_0204fda8(u32 size);
static void *func_0204fdd0(u32 size);
static void func_0204ff40(TCB *tcb, void *data);
static const void *func_02050254(SPLEmitter *emitter, int type);
static u32 func_0205036c(u32 size, BOOL is4x4comp);
static u32 func_0205037c(u32 size, BOOL is4x4comp);
static u32 func_020503ac(u32 size, BOOL is4pltt);
static u32 func_020503bc(u32 size, BOOL is4pltt);

static const VecFx32 sUnitScale = { FX32_ONE, FX32_ONE, FX32_ONE };
static const VecFx32 sDefaultCameraPos = { 0, 0, 4 * FX32_ONE };
static const VecFx32 sDefaultCameraUp = { 0, FX32_ONE, 0 };
static const VecFx32 sDefaultCameraTarget = { 0, 0, 0 };
static const SPLAllocFunc sAllocFuncs[PARTICLE_SYSTEM_MAX] = {
    func_0204fb78, func_0204fba0, func_0204fbc8, func_0204fbf0, func_0204fc18, func_0204fc40,
    func_0204fc68, func_0204fc90, func_0204fcb8, func_0204fce0, func_0204fd08, func_0204fd30,
    func_0204fd58, func_0204fd80, func_0204fda8, func_0204fdd0,
};

void func_0204f918(HeapID heapId) {
    int i;

    sParticle = GFL_HeapAllocate(heapId, sizeof(ParticleGlobal), FALSE, "particle.c", 190);
    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        sParticle->systems[i] = NULL;
    }
    sParticle->callbackParam = NULL;
    sParticle->uploading = NULL;
}

void func_0204f954(void) {
    if (func_0204fffc() != 0) {
        func_0205001c();
        func_02050044();
    }
}

ParticleSystem *func_0204f968(void *work, u32 size, BOOL camera, u32 heapId) {
    return func_0204f980(work, size, camera, 5, 6, 63, heapId);
}

ParticleSystem *func_0204f980(void *work, u32 size, BOOL camera, u32 fixPolyID, u32 minPolyID, u32 maxPolyID,
                              u32 heapId) {
    ParticleSystem *system;
    int id;

    for (id = 0; id < PARTICLE_SYSTEM_MAX; id++) {
        if (sParticle->systems[id] == NULL) {
            break;
        }
    }
    if (id >= PARTICLE_SYSTEM_MAX) {
        return NULL;
    }
    system = GFL_HeapAllocate(heapId, sizeof(ParticleSystem), FALSE, "particle.c", 285);
    MI_CpuFill8(system, 0, sizeof(ParticleSystem));
    system->cameraPos = sDefaultCameraPos;
    system->cameraUp = sDefaultCameraUp;
    system->cameraTarget = sDefaultCameraTarget;
    system->extResource = FALSE;
    MI_CpuFill8(work, 0, size);
    system->workStart = work;
    system->work = work;
    system->workEnd = (u8 *)work + size;
    system->id = id;
    sParticle->systems[id] = system;
    if (camera == TRUE) {
        func_020500cc(system, NULL, 2 * FX32_ONE, NULL, NULL, NULL, heapId);
    }
    system->manager =
        SPLManager_New(sAllocFuncs[id], PARTICLE_EMITTER_MAX, PARTICLE_MAX, fixPolyID, minPolyID, maxPolyID);
    return system;
}

static inline void Particle_Remove(ParticleSystem *system) {
    int i;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] == system) {
            sParticle->systems[i] = NULL;
            break;
        }
    }
}

void func_0204fa84(ParticleSystem *system) {
    int i;

    func_020500b0(system);
    if (system->vramRelease & PARTICLE_VRAM_TEX_FRM) {
        NNS_GfdSetFrmTexVramState(&system->texState);
    } else if (system->vramRelease & PARTICLE_VRAM_TEX_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            if (system->texKeys[i] != 0) {
                NNS_GfdFreeLnkTexVram(system->texKeys[i]);
                system->texKeys[i] = 0;
            }
        }
    }
    if (system->vramRelease & PARTICLE_VRAM_PLTT_FRM) {
        NNS_GfdSetFrmPlttVramState(&system->plttState);
    } else if (system->vramRelease & PARTICLE_VRAM_PLTT_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            if (system->plttKeys[i] != 0) {
                NNS_GfdFreeLnkPlttVram(system->plttKeys[i]);
                system->plttKeys[i] = 0;
            }
        }
    }
    system->vramRelease = 0;
    system->lastEmitter = NULL;
    if (system->resource != NULL && !system->extResource) {
        GFL_HeapFree(system->resource);
        system->resource = NULL;
    }
    Particle_Remove(system);
    if (system->camera != NULL) {
        GFL_G3DCameraFree(system->camera);
    }
    GFL_HeapFree(system);
}

void func_0204fb4c(void) {
    int i;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            func_0204fa84(sParticle->systems[i]);
        }
    }
    GFL_HeapFree(sParticle);
    sParticle = NULL;
}

// Takes size bytes of the system's work, rounded up to a word
static inline void *Particle_Alloc(ParticleSystem *system, u32 size) {
    void *ret = system->work;
    uintptr_t next = (uintptr_t)system->work + size; // bwbattle: was u32

    if (next % 4 != 0) {
        next += 4 - next % 4;
    }
#ifdef BWBATTLE
    // bwbattle: BW2 never checks; on 64 bits SPL's structs outgrow the work its callers give
    if (next > (uintptr_t)system->workEnd) {
        BwShim_Log("particle work full: %u bytes more than %u", (u32)(next - (uintptr_t)system->workEnd),
                   (u32)((u8 *)system->workEnd - (u8 *)system->workStart));
        return NULL;
    }
#endif
    system->work = (void *)next;
    return ret;
}

static void *func_0204fb78(u32 size) {
    return Particle_Alloc(sParticle->systems[0], size);
}

static void *func_0204fba0(u32 size) {
    return Particle_Alloc(sParticle->systems[1], size);
}

static void *func_0204fbc8(u32 size) {
    return Particle_Alloc(sParticle->systems[2], size);
}

static void *func_0204fbf0(u32 size) {
    return Particle_Alloc(sParticle->systems[3], size);
}

static void *func_0204fc18(u32 size) {
    return Particle_Alloc(sParticle->systems[4], size);
}

static void *func_0204fc40(u32 size) {
    return Particle_Alloc(sParticle->systems[5], size);
}

static void *func_0204fc68(u32 size) {
    return Particle_Alloc(sParticle->systems[6], size);
}

static void *func_0204fc90(u32 size) {
    return Particle_Alloc(sParticle->systems[7], size);
}

static void *func_0204fcb8(u32 size) {
    return Particle_Alloc(sParticle->systems[8], size);
}

static void *func_0204fce0(u32 size) {
    return Particle_Alloc(sParticle->systems[9], size);
}

static void *func_0204fd08(u32 size) {
    return Particle_Alloc(sParticle->systems[10], size);
}

static void *func_0204fd30(u32 size) {
    return Particle_Alloc(sParticle->systems[11], size);
}

static void *func_0204fd58(u32 size) {
    return Particle_Alloc(sParticle->systems[12], size);
}

static void *func_0204fd80(u32 size) {
    return Particle_Alloc(sParticle->systems[13], size);
}

static void *func_0204fda8(u32 size) {
    return Particle_Alloc(sParticle->systems[14], size);
}

static void *func_0204fdd0(u32 size) {
    return Particle_Alloc(sParticle->systems[15], size);
}

void *func_0204fdf8(u32 arcId, u32 fileId, u32 heapId) {
    return GFL_ArcSysReadHeapNew(arcId, fileId, heapId);
}

void func_0204fe04(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr) {
    int release;
    int i;

    release = func_02049228() == TRUE ? PARTICLE_VRAM_TEX_FRM : PARTICLE_VRAM_TEX_LNK;
    release |= func_02049238() == TRUE ? PARTICLE_VRAM_PLTT_FRM : PARTICLE_VRAM_PLTT_LNK;
    system->vramRelease = release;
    if (release & PARTICLE_VRAM_TEX_FRM) {
        NNS_GfdGetFrmTexVramState(&system->texState);
        system->texAllocFunc = func_0205036c;
    } else if (release & PARTICLE_VRAM_TEX_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            system->texKeys[i] = 0;
        }
        system->texAllocFunc = func_0205037c;
        system->texKeyCount = 0;
    }
    if (release & PARTICLE_VRAM_PLTT_FRM) {
        NNS_GfdGetFrmPlttVramState(&system->plttState);
        system->palAllocFunc = func_020503ac;
    } else if (release & PARTICLE_VRAM_PLTT_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            system->plttKeys[i] = 0;
        }
        system->palAllocFunc = func_020503bc;
        system->plttKeyCount = 0;
    }
    system->resource = resource;
    SPLManager_LoadResources(system->manager, resource);
    if (immediate == TRUE) {
        func_0204ff04(system);
    } else if (tcbMgr != NULL) {
        GFL_TCBMgrAddTask(tcbMgr, func_0204ff40, system, 0);
    }
}

void func_0204fee0(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr) {
    func_0204fe04(system, resource, immediate, tcbMgr);
    system->extResource = TRUE;
}

void func_0204fef8(ParticleSystem *system, void *resource) {
    func_0204fe04(system, resource, FALSE, NULL);
}

void func_0204ff04(ParticleSystem *system) {
    sParticle->uploading = system;
    if (system->texAllocFunc == NULL) {
        SPLManager_UploadTextures(system->manager);
    } else {
        SPLManager_UploadTexturesEx(system->manager, system->texAllocFunc);
    }
    if (system->palAllocFunc == NULL) {
        SPLManager_UploadPalettes(system->manager);
    } else {
        SPLManager_UploadPalettesEx(system->manager, system->palAllocFunc);
    }
    sParticle->uploading = NULL;
}

static void func_0204ff40(TCB *tcb, void *data) {
    func_0204ff04(data);
    GFL_TCBRemove(tcb);
}

void func_0204ff54(ParticleSystem *system) {
    G3DCameraProjection projection;
    FxLookAt lookAt;

    if (system->camera != NULL) {
        VecFx32 scale;
        MtxFx33 rot;

        // Draw with the system's camera, from the origin
        GFL_G3DSysMtxGetProjection(&projection);
        GFL_G3DSysMtxGetViewLookAt(&lookAt);
        scale = sUnitScale;
        {
            VecFx32 trans = { 0, 0, 0 };

            MTX_Identity33_(&rot);
            NNS_G3dGlbSetBaseTrans(&trans);
            NNS_G3dGlbSetBaseRot(&rot);
            NNS_G3dGlbSetBaseScale(&scale);
        }
        GFL_G3DCameraFlush(system->camera);
        GFL_G3DSysMtxViewFlush();
        NNS_G3DWaitFIFO();
    }
    SPLManager_Draw(system->manager, &NNS_G3dGlb.cameraMtx);
    if (system->camera != NULL) {
        GFL_G3DSysMtxSetProjection(&projection);
        GFL_G3DSysMtxSetViewLookAt(&lookAt);
        GFL_G3DSysMtxViewFlush();
        NNS_G3DWaitFIFO();
    }
}

void func_0204fff0(ParticleSystem *system) {
    SPLManager_Update(system->manager);
}

int func_0204fffc(void) {
    int count = 0;
    int i;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            count++;
        }
    }
    return count;
}

int func_0205001c(void) {
    int i;
    int count = 0;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            func_0204ff54(sParticle->systems[i]);
            count++;
        }
    }
    return count;
}

int func_02050044(void) {
    int i;
    int count = 0;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            func_0204fff0(sParticle->systems[i]);
            count++;
        }
    }
    return count;
}

SPLEmitter *func_0205006c(ParticleSystem *system, int resourceId, const VecFx32 *pos) {
    SPLEmitter *emitter = SPLManager_CreateEmitter(system->manager, resourceId, pos);

    system->lastEmitter = emitter;
    return emitter;
}

SPLEmitter *func_0205007c(ParticleSystem *system, int resourceId, SPLEmitterCallback callback, void *param) {
    SPLEmitter *emitter;

    if (system->manager->activeEmitters.count >= PARTICLE_EMITTER_MAX) {
        return (SPLEmitter *)-1;
    }
    sParticle->callbackParam = param;
    emitter = SPLManager_CreateEmitterWithCallback(system->manager, resourceId, callback);
    sParticle->callbackParam = NULL;
    system->lastEmitter = emitter;
    return emitter;
}

int func_020500a8(ParticleSystem *system) {
    return system->manager->activeEmitters.count;
}

void func_020500b0(ParticleSystem *system) {
    SPLManager_DeleteAllEmitters(system->manager);
}

void func_020500bc(ParticleSystem *system, SPLEmitter *emitter) {
    SPLManager_DeleteEmitter(system->manager, emitter);
}

void *func_020500c8(ParticleSystem *system) {
    return system->workStart;
}

void func_020500cc(ParticleSystem *system, G3DCameraProjection *projection, u16 fov, const VecFx32 *position,
                   const VecFx32 *upVector, const VecFx32 *target, u32 heapId) {
    system->unk24.x = 0;
    system->unk24.y = 0;
    system->unk24.z = 0;
    system->cameraFov = fov;
    if (position == NULL) {
        position = &sDefaultCameraPos;
    }
    if (upVector == NULL) {
        upVector = &sDefaultCameraUp;
    }
    if (target == NULL) {
        target = &sDefaultCameraTarget;
    }
    if (projection != NULL) {
        system->cameraProjection = projection->type;
        system->camera = GFL_G3DCameraCreate(projection->type, projection->param1, projection->param2,
                                             projection->param3, projection->param4, projection->near, projection->far,
                                             projection->ndcRangeOverride, position, upVector, target, heapId);
    } else {
        system->cameraProjection = G3DCAM_PROJECTION_PERSPECTIVE;
        // 45 degrees across a 4:3 view
        system->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, 0xb50, 0xb50, 0x1555, 0, FX32_ONE,
                                             900 * FX32_ONE, 0, position, upVector, target, heapId);
    }
}

void func_02050178(ParticleSystem *system) {
    GFL_G3DCameraFree(system->camera);
    system->camera = NULL;
}

void *func_02050188(void) {
    return sParticle->callbackParam;
}

G3DCamera *func_02050194(ParticleSystem *system) {
    return system->camera;
}

void func_02050198(ParticleSystem *system, VecFx32 *pos) {
    *pos = system->cameraPos;
}

void func_020501a8(ParticleSystem *system, const VecFx32 *pos) {
    system->cameraPos = *pos;
}

void func_020501b8(SPLEmitter *emitter, VecFx16 *axis) {
    *axis = emitter->axis;
}

fx32 func_020501d0(SPLEmitter *emitter) {
    return emitter->radius;
}

void func_020501d4(SPLEmitter *emitter, fx32 radius) {
    emitter->radius = radius;
}

fx32 func_020501d8(SPLEmitter *emitter) {
    return emitter->length;
}

void func_020501dc(SPLEmitter *emitter, fx32 length) {
    emitter->length = length;
}

fx16 func_020501e0(SPLEmitter *emitter) {
    return emitter->initVelPositionAmplifier;
}

void func_020501e8(SPLEmitter *emitter, fx32 amplifier) {
    emitter->initVelPositionAmplifier = amplifier;
}

fx16 func_020501ec(SPLEmitter *emitter) {
    return emitter->initVelAxisAmplifier;
}

void func_020501f4(SPLEmitter *emitter, fx32 amplifier) {
    emitter->initVelAxisAmplifier = amplifier;
}

fx16 func_020501f8(SPLEmitter *emitter) {
    return emitter->baseScale;
}

u16 func_02050200(SPLEmitter *emitter) {
    return emitter->particleLifeTime;
}

void func_02050208(SPLEmitter *emitter, const VecFx32 *pos) {
    emitter->position.x = pos->x + emitter->resource->header->emitterBasePos.x;
    emitter->position.y = pos->y + emitter->resource->header->emitterBasePos.y;
    emitter->position.z = pos->z + emitter->resource->header->emitterBasePos.z;
}

void func_02050230(SPLEmitter *emitter, const VecFx16 *axis) {
    emitter->axis = *axis;
}

void func_02050248(SPLEmitter *emitter, fx32 scale) {
    emitter->baseScale = scale;
}

void func_0205024c(SPLEmitter *emitter, u16 lifeTime) {
    emitter->particleLifeTime = lifeTime;
}

// The parameters of the emitter's first behavior of a kind, or NULL
static const void *func_02050254(SPLEmitter *emitter, int type) {
    int i;
    int count = emitter->resource->behaviorCount;

    if (count == 0) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        SPLBehavior *behavior = &emitter->resource->behaviors[i];

        if (behavior == NULL) {
            continue;
        }
        switch (type) {
        case PARTICLE_BEHAVIOR_GRAVITY:
            if (behavior->applyFunc == SPLBehavior_ApplyGravity) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_RANDOM:
            if (behavior->applyFunc == SPLBehavior_ApplyRandom) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_MAGNET:
            if (behavior->applyFunc == SPLBehavior_ApplyMagnet) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_SPIN:
            if (behavior->applyFunc == SPLBehavior_ApplySpin) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_COLLISION_PLANE:
            if (behavior->applyFunc == SPLBehavior_ApplyCollisionPlane) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_CONVERGENCE:
            if (behavior->applyFunc == SPLBehavior_ApplyConvergence) {
                return behavior->object;
            }
            continue;
        default:
            return NULL;
        }
    }
    return NULL;
}

void func_02050310(SPLEmitter *emitter, const VecFx32 *target) {
    SPLMagnetBehavior *behavior = (SPLMagnetBehavior *)func_02050254(emitter, PARTICLE_BEHAVIOR_MAGNET);

    if (behavior != NULL) {
        behavior->target = *target;
    }
}

void func_02050328(SPLEmitter *emitter, const u16 *axis) {
    SPLSpinBehavior *behavior = (SPLSpinBehavior *)func_02050254(emitter, PARTICLE_BEHAVIOR_SPIN);

    if (behavior != NULL) {
        behavior->axis = *axis;
    }
}

void func_0205033c(SPLEmitter *emitter, const VecFx32 *target) {
    SPLConvergenceBehavior *behavior = (SPLConvergenceBehavior *)func_02050254(emitter, PARTICLE_BEHAVIOR_CONVERGENCE);

    if (behavior != NULL) {
        behavior->target = *target;
    }
}

void func_02050354(SPLEmitter *emitter, SPLEmitterUpdateCallback callback) {
    emitter->updateCallback = callback;
}

void func_0205035c(SPLEmitter *emitter, void *data) {
    emitter->userDataPtr = data;
}

void *func_02050364(SPLEmitter *emitter) {
    return emitter->userDataPtr;
}

// The VRAM allocators of an uploading resource, which keep the keys the linked-list managers give
static u32 func_0205036c(u32 size, BOOL is4x4comp) {
    return NNS_GfdGetTexKeyAddr(NNS_GfdAllocFrmTexVram(size, is4x4comp, 0));
}

static u32 func_0205037c(u32 size, BOOL is4x4comp) {
    NNSGfdTexKey key = NNS_GfdAllocTexVram(size, is4x4comp, 0);
    ParticleSystem *system = sParticle->uploading;

    system->texKeys[system->texKeyCount++] = key;
    return NNS_GfdGetTexKeyAddr(key);
}

static u32 func_020503ac(u32 size, BOOL is4pltt) {
    return NNS_GfdGetPlttKeyAddr(NNS_GfdAllocFrmPlttVram(size, is4pltt, TRUE));
}

static u32 func_020503bc(u32 size, BOOL is4pltt) {
    NNSGfdPlttKey key = NNS_GfdAllocPlttVram(size, is4pltt, 0);
    ParticleSystem *system = sParticle->uploading;

    system->plttKeys[system->plttKeyCount++] = key;
    return NNS_GfdGetPlttKeyAddr(key);
}

u16 func_020503f0(const void *resource) {
    return ((const SPLFileHeader *)resource)->resCount;
}

void *func_020503f4(u32 layer) {
    switch (layer) {
    case 0:
        return G2_GetBG0CharPtr();
    case 1:
        return G2_GetBG1CharPtr();
    case 2:
        return G2_GetBG2CharPtr();
    case 3:
        return G2_GetBG3CharPtr();
    case 4:
        return G2S_GetBG0CharPtr();
    case 5:
        return G2S_GetBG1CharPtr();
    case 6:
        return G2S_GetBG2CharPtr();
    case 7:
        return G2S_GetBG3CharPtr();
    }
    return NULL;
}
