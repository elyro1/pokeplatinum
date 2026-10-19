#ifndef POKEBW2_GFL_CLACT_H
#define POKEBW2_GFL_CLACT_H

#include "types.h"
#include "gfl/arc.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nnsys/g2d.h"

// Cell actors, the OAM sprites of the 2D engines

// Holds cell actors, 0xe4 bytes each
typedef struct ClActUnit ClActUnit;
typedef struct ClActor ClActor;
typedef struct ClActAnim ClActAnim;
typedef struct ClActSys ClActSys;

typedef struct {
    s16 x;
    s16 y;
    u16 sequence;
    u8 priority;
    u8 bgPriority;
} ClActorSetup;

typedef struct {
    // Where the main and sub screens are, for actors positioned on them
    s16 mainX;
    s16 mainY;
    s16 subX;
    s16 subY;
    // The OAMs of each engine the system uses
    u8 oamStartMain;
    u8 oamCountMain;
    u8 oamStartSub;
    u8 oamCountSub;
    // How many VRAM transfer cell animations can play at once
    u32 transferCount;
    // How many resources of each kind can be loaded
    u16 charCount;
    u16 plttCount;
    u16 cellAnimCount;
    u16 unk16;
    // Where the resources start in OBJ VRAM, in 32-byte units
    u16 charOffsetMain;
    u16 charOffsetSub;
} ClActSysSetup;

// The setup that most apps create the system with, the same as the intro's copy
extern const ClActSysSetup data_02093f08;
// Game Sync's screens' setup, with 192 for the sub screen's y
extern const ClActSysSetup data_02093f24;

// Where a resource is loaded: CLACT_VRAM_MAIN, _SUB or _BOTH
enum {
    CLACT_VRAM_MAIN,
    CLACT_VRAM_SUB,
    CLACT_VRAM_BOTH,
};

// The surfaces of the default renderer, which the actors' positions are relative to. An enum, which MWCC does not
// propagate as a constant, as the Pokédex's info page shows
typedef enum {
    CLACT_SURFACE_MAIN,
    CLACT_SURFACE_SUB,
} ClActSurface;

typedef struct ClActRenderer ClActRenderer;

// A surface of a renderer: its view, its screen and how it culls cells
typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    u32 screen;
    u32 culling;
} ClActSurfaceSetup;

typedef struct {
    s16 x;
    s16 y;
} ClActorPos;

typedef struct {
    fx32 x;
    fx32 y;
} ClActorScale;

typedef struct {
    ClActorSetup base;
    ClActorPos affineCenter;
    fx32 scaleX;
    fx32 scaleY;
    u16 rotation;
    u16 affineMode;
} ClActorSetupEx;

// When an animation callback is called
enum {
    CLACT_CALLBACK_LAST_FRAME,
    CLACT_CALLBACK_FRAME,
    CLACT_CALLBACK_EVERY_FRAME,
};

typedef struct {
    u16 type;
    // For CLACT_CALLBACK_FRAME
    u16 frame;
    u32 param;
    NNSG2dAnmCallBackPtr func;
} ClActorCallback;

void ClActSys_Create(const ClActSysSetup *setup, const BGSysVRAMConfig *vramConfig, HeapID heapId);
void func_0204b758(void);
// bwbattle: turns off the system's writes to OBJ VRAM, OBJ palettes and OAM, while another game's sprites own them.
// The resources are still allocated and the actors still created, just not shown
void ClActSys_SetVramWritesEnabled(BOOL enabled);
void func_0204b794(void);
void func_0204b7c8(void);
void func_0204b7fc(void);

// Load characters (NCGR), a palette (NCLR), and cells with their animations (NCER and NANR) from an archive, and
// return their resource indices
u32 func_0204b81c(ArcTool *arc, u32 fileId, BOOL compressed, u32 vramType, HeapID heapId);
u32 func_0204b8bc(u32 size, u32 vramType, HeapID heapId);
void func_0204b98c(u32 chars);
// Replace the data of loaded characters
void func_0204ba40(u32 chars, NNSG2dCharacterData *character);
void func_0204bab8(u32 chars, void *src, u32 size, u32 offset, u32 vramType);
void func_0204bb58(u32 chars, NNSG2dImageProxy *proxy);
u32 func_0204bb80(u32 chars, BOOL sub);
u32 func_0204bba0(ArcTool *arc, u32 fileId, u32 vramType, u16 offset, HeapID heapId);
// func_0204bba0 loading count palettes from start, all when 0
u32 func_0204bbb8(ArcTool *arc, u32 fileId, u32 vramType, u16 offset, u16 start, u16 count, HeapID heapId);
u32 func_0204bc48(ArcTool *arc, u32 fileId, u32 vramType, u16 offset, HeapID heapId);
void func_0204bcd0(u32 palette);
// Replace the data of a loaded palette, count palettes of it or all when 0
void func_0204bd10(u32 palette, NNSG2dPaletteData *data, u32 count);
void func_0204bd9c(u32 palette, NNSG2dImagePaletteProxy *proxy);
u32 func_0204bdc0(u32 palette, BOOL sub);
u32 func_0204bde0(ArcTool *arc, u32 cellFileId, u32 animFileId, HeapID heapId);
void func_0204be64(u32 cellAnims);

// A renderer of its own for units, with count surfaces
ClActRenderer *func_0204be9c(const ClActSurfaceSetup *setups, u16 count, HeapID heapId);
void func_0204becc(ClActRenderer *renderer);
void func_0204bedc(ClActRenderer *renderer, u32 surface, const ClActorPos *pos);
void func_0204befc(ClActRenderer *renderer, u32 surface, ClActorPos *pos);
void func_0204bf14(ClActRenderer *renderer, BOOL cull);

ClActUnit *func_0204bf1c(u16 count, u8 priority, HeapID heapId);
void func_0204bf98(ClActUnit *unit);
void func_0204bfd4(ClActUnit *unit, BOOL active);
BOOL func_0204bfe8(ClActUnit *unit);
void func_0204bff0(ClActUnit *unit, BOOL a1);
void func_0204c004(ClActUnit *unit, BOOL a1);
void func_0204c018(ClActUnit *unit, ClActRenderer *renderer);
void func_0204c028(ClActUnit *unit);

// Positions are relative to a surface of the unit's renderer, or absolute with surface 0xffff
ClActor *func_0204c040(ClActUnit *unit, u32 chars, u32 palette, u32 cellAnims, const ClActorSetup *setup, u16 surface,
                       HeapID heapId);
ClActor *func_0204c0a4(ClActUnit *unit, u32 chars, u32 palette, u32 cellAnims, const ClActorSetupEx *setup, u16 surface,
                       HeapID heapId);
void func_0204c108(ClActor *actor);
void func_0204c124(ClActor *actor, BOOL visible);
BOOL func_0204c138(ClActor *actor);
void func_0204c140(ClActor *actor, const ClActorPos *pos, u16 surface);
void func_0204c178(ClActor *actor, ClActorPos *pos, u16 surface);
void func_0204c1a8(ClActor *actor, s16 value, u16 surface, u32 axis);
s16 func_0204c1dc(ClActor *actor, u16 surface, u32 axis);
void func_0204c210(ClActor *actor, const ClActorPos *pos);
void func_0204c21c(const ClActor *actor, ClActorPos *pos);
void func_0204c228(ClActor *actor, s16 value, u32 axis);
s16 func_0204c234(ClActor *actor, u32 axis);
void func_0204c244(ClActor *actor, u32 affineMode);
void func_0204c258(ClActor *actor, const ClActorPos *center);
void func_0204c264(ClActor *actor, s16 value, u32 axis);
void func_0204c270(ClActor *actor, const ClActorScale *scale);
void func_0204c27c(ClActor *actor, ClActorScale *scale);
void func_0204c288(ClActor *actor, fx32 value, u32 axis);
fx32 func_0204c294(ClActor *actor, u32 axis);
void func_0204c2a0(ClActor *actor, u16 rotation);
u16 func_0204c2a8(ClActor *actor);
void func_0204c2b0(ClActor *actor, u32 axis, BOOL flip);
BOOL func_0204c2f0(ClActor *actor, u32 axis);
// The actor's OBJ mode, GX_OAM_MODE_*
void func_0204c318(ClActor *actor, u32 mode);
u32 func_0204c370(ClActor *actor);
void func_0204c378(ClActor *actor, u32 palette, u32 a2);
u8 func_0204c39c(ClActor *actor);
void func_0204c3a8(ClActor *actor, const NNSG2dImagePaletteProxy *proxy);
void func_0204c3d0(ClActor *actor, NNSG2dImagePaletteProxy *proxy);
void func_0204c3e4(ClActor *actor, const NNSG2dImageProxy *proxy);
void func_0204c40c(ClActor *actor, NNSG2dImageProxy *proxy);
u16 func_0204c428(ClActor *actor);
u16 func_0204c430(ClActor *actor);
void func_0204c438(ClActor *actor, u8 priority);
u8 func_0204c45c(ClActor *actor);
void func_0204c468(ClActor *actor, u32 bgPriority);
u8 func_0204c47c(ClActor *actor);
// Sets the actor's animation sequence
void func_0204c488(ClActor *actor, u16 sequence);
u16 func_0204c4a0(ClActor *actor);
u16 func_0204c4a8(ClActor *actor);
// Sets it only when it changes
void func_0204c4b8(ClActor *actor, u16 sequence);
// Act on the actor's animation controller
void func_0204c4d4(ClActor *actor, fx32 time);
void func_0204c4e0(ClActor *actor, fx32 speed);
fx32 func_0204c4f8(ClActor *actor);
void func_0204c504(ClActor *actor, u16 frame);
u16 func_0204c510(ClActor *actor);
void func_0204c520(ClActor *actor, BOOL a1);
BOOL func_0204c534(ClActor *actor);
void func_0204c53c(ClActor *actor, fx32 a1);
void func_0204c540(ClActor *actor);
void func_0204c550(ClActor *actor);
BOOL func_0204c560(ClActor *actor);
void func_0204c56c(ClActor *actor);
void func_0204c584(ClActor *actor, u32 playMode);
void func_0204c5b0(ClActor *actor, const ClActorCallback *callback);
void func_0204c5bc(ClActor *actor);
void func_0204c5c8(ClActor *actor, BOOL a1);
// The actor system, which the ITCM reads
extern ClActSys *g_ClActSys;

// The renderers in ITCM (0x01ff8020..0x01ff8bf0), chosen by unk5 and unk6 of the unit. The ITCM has no source file yet,
// so they are declared here beside the unit they draw
void mainRenderer(ClActUnit *unit);
void func_01ff8318(ClActUnit *unit);
void func_01ff8610(ClActUnit *unit);
void func_01ff8900(ClActUnit *unit);

// Called from the ITCM
void func_0204d3bc(ClActAnim *anim, fx32 frames);
void func_0204d46c(ClActAnim *anim, fx32 speed);

#endif // POKEBW2_GFL_CLACT_H
