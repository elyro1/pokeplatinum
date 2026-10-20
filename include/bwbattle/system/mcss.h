#ifndef POKEBW2_SYSTEM_MCSS_H
#define POKEBW2_SYSTEM_MCSS_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"

// MCSS, the system that draws Pokémon and trainer sprites from multi-cell animations. It is mcss.c in the main module

typedef struct MCSSSystem MCSSSystem;
typedef struct MCSS MCSS;

// The archive and files of a sprite's graphics, as SetupPokemonLoaderFSTool fills it in
typedef struct {
    u32 arcId;
    u32 character;
    u32 palette;
    u32 cells;
    u32 cellAnime;
    u32 multiCells;
    u32 multiCellAnime;
    u32 bin;
    u32 unk20;
} MCSSLoadInfo;

// Called with the callback's parameter and the animation's frame
typedef void (*MCSSAnimationCallback)(u32 param, fx32 frame);

// A sprite's characters and palette on their way to VRAM, which a task loads in the V-blank. A palette that the
// system blends has no files: its palette data and colors are allocated
typedef struct {
    NNSG2dCharacterData *chars;
    NNSG2dPaletteData *pltt;
    NNSG2dImageProxy *imageProxy;
    NNSG2dImagePaletteProxy *plttProxy;
    void *charFile;
    void *plttFile;
    u32 charAddr;
    u32 plttAddr;
    MCSSSystem *system;
    MCSS *mcss;
    // Whether the task runs in the system's task manager, and not in the V-blank's
    BOOL inTcbMgr;
} MCSSLoad;

// Called with a sprite's files read and not yet loaded, to change its characters. Returning TRUE removes the callback
typedef BOOL (*MCSSLoadCallback)(const MCSSLoadInfo *info, MCSSLoad *load, u32 work);

MCSSSystem *MCSSSys_Create(u32 capacity, HeapID heapId);
void MCSSSys_Free(MCSSSystem *system);
// Steps the animation of every sprite that is not paused
void MCSSSys_Update(MCSSSystem *system);
#ifdef BWBATTLE
void MCSS_SetSkipAnimationTick(MCSS *mcss, BOOL skip);
#endif
void MCSSSys_Draw(MCSSSystem *system);
MCSS *MCSSSys_Add(MCSSSystem *system, fx32 x, fx32 y, fx32 z, const MCSSLoadInfo *info);
void MCSSSys_Remove(MCSSSystem *system, MCSS *mcss);
void func_0201aacc(MCSSSystem *system);
void func_0201aadc(MCSSSystem *system);
// The archive of the sprites' files, which func_02019b98 opens and func_02019bb4 closes, and the task manager that
// runs the sprites' fades, which an app clears while it adds sprites
void func_02019b98(MCSSSystem *system, u32 arcId);
void func_02019bb4(MCSSSystem *system);
void func_02019bcc(MCSSSystem *system, TCBManager *tcbMgr);
void MCSSSys_SetLoadCallback(MCSSSystem *system, MCSSLoadCallback callback, u32 work);
void func_02019bd8(MCSSSystem *system, MCSS *mcss, const MCSSLoadInfo *info);
// Where the sprites' character and palette data go: each sprite's is at these offsets plus 0x4000 and 0x20 bytes per
// slot
void func_0201aefc(MCSSSystem *system, u32 characterOffset);
void func_0201af00(MCSSSystem *system, u32 paletteOffset);
void func_0201af9c(MCSSSystem *system, u32 offset);

void MCSS_GetPosition(MCSS *mcss, VecFx32 *position);
void MCSS_SetPosition(MCSS *mcss, VecFx32 *position);
void MCSS_SetScale(MCSS *mcss, VecFx32 *scale);
void func_0201abb8(MCSS *mcss, VecFx32 *a1);
void func_0201ac0c(MCSS *mcss, VecFx32 *a1);
void func_0201ac5c(MCSS *mcss, u16 a1);
void func_0201ac70(MCSS *mcss, VecFx32 *a1);
void func_0201ac8c(MCSS *mcss);
void func_0201ac9c(MCSS *mcss);
void MCSS_PauseAnimation(MCSS *mcss);
void MCSS_ResumeAnimation(MCSS *mcss);
void MCSS_Hide(MCSS *mcss);
void MCSS_Show(MCSS *mcss);
// Sets the callback that runs when the sprite's animation reaches its last frame
void MCSS_SetAnimationEndCallback(MCSS *mcss, u32 param, MCSSAnimationCallback callback, u32 unused);
// Alpha from 0 to 31
void MCSS_SetAlpha(MCSS *mcss, u8 alpha);
#ifdef BWBATTLE
#include "bwbattle.h"
BOOL MCSS_SetTrainerBackSprite(MCSS *mcss, BwTrainerBack trainer);
// bwbattle: draws the sprite as one image instead of its cells (a trainer BW2 has no sprite for): width by height
// dots of 4 bits, its rows packed, low dots first, with its 16 colors, drawn scale times its size. The sprite's place
// is anchorX pixels right and anchorY pixels below the image's top left.
void MCSS_SetImage(MCSS *mcss, const u8 *image, u32 width, u32 height, int anchorX, int anchorY,
    const u16 *colors, fx32 scale);
#endif
void MCSS_SetAnimation(MCSS *mcss, u32 animation);
void MCSS_RestartAnimation(MCSS *mcss);
// Alpha from 0 to 31
u8 func_0201ae88(MCSS *mcss);
// Fades the palette between two levels of a color, and whether the fade is running
void func_0201ae2c(MCSS *mcss, u8 startLevel, u8 endLevel, s32 wait, u32 color);
BOOL func_0201aee8(MCSS *mcss);
void func_0201aecc(MCSS *mcss, u32 a1);
void func_0201aeb0(MCSS *mcss, u32 a1);
void func_0201ab54(MCSS *mcss, VecFx32 *a1);
u16 func_0201ade8(MCSS *mcss);
u16 func_0201ade0(MCSS *mcss);
s16 func_0201adf0(MCSS *mcss);
s16 func_0201adf8(MCSS *mcss);
// The sprite's multi-cell animation, whose animation controller NNS_G2dSetAnimCtrlCallBackFunctor takes
NNSG2dMultiCellAnimation *func_0201adc4(MCSS *mcss);
// Called as a sprite's animation ends
void func_0201b25c(MCSS *mcss);

// More of a sprite's state, which the battle view's sprites use
void func_0201aae8(MCSS *mcss);
void func_0201aaf8(MCSS *mcss);
void func_0201ab3c(MCSS *mcss, VecFx32 *out);
void func_0201ab70(MCSS *mcss, VecFx32 *out);
void func_0201aba0(MCSS *mcss, VecFx32 *out);
void func_0201abd4(MCSS *mcss, VecFx32 *out);
void func_0201abf0(MCSS *mcss, const VecFx32 *in);
void func_0201ac28(MCSS *mcss, VecFx32 *out);
void func_0201ac40(MCSS *mcss, VecFx32 *in);
void func_0201ac64(MCSS *mcss, u16 value);
void func_0201acb0(MCSS *mcss);
BOOL func_0201acd4(MCSS *mcss);
void func_0201ad04(MCSS *mcss);
void func_0201ad4c(MCSS *mcss);
BOOL func_0201ad70(MCSS *mcss);
void func_0201ada0(MCSS *mcss);
u16 func_0201adc8(MCSS *mcss);
u8 func_0201add0(MCSS *mcss);
u16 func_0201add8(MCSS *mcss, u8 index);
u8 func_0201ae00(MCSS *mcss);
void func_0201ae1c(MCSS *mcss, u32 param,
                   BOOL (*func)(u32 param, const NNSG2dMultiCellHierarchyData *node, NNSG2dCellAnimation *cellAnim,
                                u16 nodeIdx));
u8 func_0201af44(MCSS *mcss);
void func_0201af54(MCSSSystem *system, MCSS *mcss, int level);
void func_0201afa0(MCSS *mcss, fx32 speed);
u16 func_0201afa8(MCSS *mcss);
void func_0201afb0(MCSSSystem *system, MCSS *mcss, u8 evy, u32 color);
void func_0201b128(MCSSSystem *system, MCSS *mcss);
void func_0201b270(MCSS *mcss, BOOL flag);
void func_0201b28c(MCSSSystem *system, u32 value);
void func_0201b290(MCSSSystem *system, BOOL flag);
// The rest is mcss_tool.c's, which fills in the files of a Pokémon's or a trainer's sprite

// The files of a Pokémon's sprite, with dir and egg as GetPokemonPaletteDataNo takes them
void SetupPokemonLoaderFSTool(u32 species, u32 form, u32 sex, BOOL rare, BOOL egg, MCSSLoadInfo *info, u32 dir);
// The files of a trainer's sprite, from the front or the back
void GetTrainerSpriteResIDs(u32 trainerType, MCSSLoadInfo *info, BOOL back);
// Draws a Spinda's spots for its personality on the next sprite added
void func_0201c188(MCSSSystem *system, u32 personality);
// Draws a Spinda's spots for its personality on a sprite's characters
void MCSS_DrawSpindaSpots(void *chars, u32 personality);
// Adds the sprite of a party Pokémon
MCSS *func_0201c14c(MCSSSystem *system, PartyPkm *pkm, u32 dir, fx32 x, fx32 y, fx32 z);

// SetupPokemonLoaderByBoxData for a party Pokémon
void func_0201bfdc(PartyPkm *pkm, MCSSLoadInfo *info, u32 dir);

#endif // POKEBW2_SYSTEM_MCSS_H
