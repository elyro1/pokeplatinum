#ifndef POKEBW2_GFL_PARTICLE_H
#define POKEBW2_GFL_PARTICLE_H

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/gfd.h"
#include "spl/spl.h"

// Particle systems: up to 16 SPL managers, each with its own work memory, resource file and camera. None of these
// functions has a name yet

#define PARTICLE_SYSTEM_MAX 16

// How a system's VRAM is given back when it is freed: the frame managers' state from before its resource was loaded,
// or each key the linked-list managers gave it
enum {
    PARTICLE_VRAM_TEX_FRM = 1 << 0,
    PARTICLE_VRAM_TEX_LNK = 1 << 1,
    PARTICLE_VRAM_PLTT_FRM = 1 << 2,
    PARTICLE_VRAM_PLTT_LNK = 1 << 3,
};

#define PARTICLE_VRAM_KEY_MAX 64

typedef struct {
    SPLManager *manager;
    void *resource;
    SPLEmitter *lastEmitter;
    void *workStart;
    void *work;
    void *workEnd;
    SPLTexVRAMAllocFunc texAllocFunc;
    SPLPalVRAMAllocFunc palAllocFunc;
    G3DCamera *camera;
    VecFx32 unk24;
    u16 cameraFov;
    // Whether the resource is someone else's, which freeing the system leaves
    u16 extResource : 1;
    VecFx32 cameraPos;
    VecFx32 cameraUp;
    VecFx32 cameraTarget;
    union {
        NNSGfdFrmTexVramState texState;
        NNSGfdTexKey texKeys[PARTICLE_VRAM_KEY_MAX];
    };
    union {
        NNSGfdFrmPlttVramState plttState;
        NNSGfdPlttKey plttKeys[PARTICLE_VRAM_KEY_MAX];
    };
    u8 vramRelease;
    u8 unk259;
    u8 id;
    u8 cameraProjection;
    u16 texKeyCount;
    u16 plttKeyCount;
} ParticleSystem;

// Sets up and frees the table of systems
void func_0204f918(HeapID heapId);
void func_0204fb4c(void);
// Updates and draws every system
void func_0204f954(void);
// func_0204f980 with polygon IDs 5, 6 and 0x3f
ParticleSystem *func_0204f968(void *work, u32 size, BOOL camera, u32 heapId);
// A system working in size bytes of work, its particles drawn with the given polygon IDs, or NULL when all 16 are in
// use
ParticleSystem *func_0204f980(void *work, u32 size, BOOL camera, u32 fixPolyID, u32 minPolyID, u32 maxPolyID,
                              u32 heapId);
void func_0204fa84(ParticleSystem *system);
void *func_0204fdf8(u32 arcId, u32 fileId, u32 heapId);
// Gives the system a resource file, which it uploads now or, through tcbMgr, at the next vertical blank
void func_0204fe04(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr);
// The same, with a resource the system does not own
void func_0204fee0(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr);
void func_0204fef8(ParticleSystem *system, void *resource);
void func_0204ff04(ParticleSystem *system);
void func_0204ff54(ParticleSystem *system);
void func_0204fff0(ParticleSystem *system);
int func_0204fffc(void);
int func_0205001c(void);
int func_02050044(void);
SPLEmitter *func_0205006c(ParticleSystem *system, int resourceId, const VecFx32 *pos);
// Creates an emitter, calling callback with param readable through func_02050188, or returns -1 when the system
// has 24 emitters
SPLEmitter *func_0205007c(ParticleSystem *system, int resourceId, SPLEmitterCallback callback, void *param);
int func_020500a8(ParticleSystem *system);
void func_020500b0(ParticleSystem *system);
void func_020500bc(ParticleSystem *system, SPLEmitter *emitter);
void *func_020500c8(ParticleSystem *system);
// Sets the camera that the particles are drawn with; a NULL projection or vector is the default
void func_020500cc(ParticleSystem *system, G3DCameraProjection *projection, u16 fov, const VecFx32 *position,
                   const VecFx32 *upVector, const VecFx32 *target, u32 heapId);
void func_02050178(ParticleSystem *system);
void *func_02050188(void);
G3DCamera *func_02050194(ParticleSystem *system);
void func_02050198(ParticleSystem *system, VecFx32 *pos);
void func_020501a8(ParticleSystem *system, const VecFx32 *pos);

// An emitter's settings
void func_020501b8(SPLEmitter *emitter, VecFx16 *axis);
fx32 func_020501d0(SPLEmitter *emitter);
void func_020501d4(SPLEmitter *emitter, fx32 radius);
fx32 func_020501d8(SPLEmitter *emitter);
void func_020501dc(SPLEmitter *emitter, fx32 length);
fx16 func_020501e0(SPLEmitter *emitter);
void func_020501e8(SPLEmitter *emitter, fx32 amplifier);
fx16 func_020501ec(SPLEmitter *emitter);
void func_020501f4(SPLEmitter *emitter, fx32 amplifier);
fx16 func_020501f8(SPLEmitter *emitter);
u16 func_02050200(SPLEmitter *emitter);
void func_02050208(SPLEmitter *emitter, const VecFx32 *pos);
void func_02050230(SPLEmitter *emitter, const VecFx16 *axis);
void func_02050248(SPLEmitter *emitter, fx32 scale);
void func_0205024c(SPLEmitter *emitter, u16 lifeTime);
void func_02050310(SPLEmitter *emitter, const VecFx32 *target);
void func_02050328(SPLEmitter *emitter, const u16 *axis);
void func_0205033c(SPLEmitter *emitter, const VecFx32 *target);
void func_02050354(SPLEmitter *emitter, SPLEmitterUpdateCallback callback);
void func_0205035c(SPLEmitter *emitter, void *data);
void *func_02050364(SPLEmitter *emitter);

// How many emitters a resource file defines
u16 func_020503f0(const void *resource);
// The character base of a background layer of the main (0 to 3) or sub (4 to 7) engine
void *func_020503f4(u32 layer);

#endif // POKEBW2_GFL_PARTICLE_H
