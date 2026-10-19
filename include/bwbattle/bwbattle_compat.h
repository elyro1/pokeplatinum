#ifndef BWBATTLE_COMPAT_H
#define BWBATTLE_COMPAT_H

// Force-included into every bwbattle file (src/bwbattle/meson.build).
//
// pokebw2 rebuilt NitroSDK's and NitroSystem's headers from the ROM, and named what it couldn't find a name for itself.
// bwbattle builds on the port's SDK (libntr, libntrsystem), so BW2's names map to the SDK's here, from the comments in
// pokebw2's lib/nnsys/include/nnsys/g3d.h and g2d.h. Member names are fixed in the copied code instead.

#include <nitro.h>
#include <nnsys.h>

// NitroSDK
#define DEG_TO_IDX(deg) ((u16)((deg) * 0x10000 / 360)) // Whole degrees; FX_DEG_TO_IDX takes fx32 degrees
#define MAT3_To4x4 MTX_Copy33To44
#define reg_G3X_RAM_COUNT (*(vu16 *)REG_LISTRAM_COUNT_ADDR) // 0x04000604: the polygon count

// NitroSystem G2D
typedef NNS_G2D_VRAM_TYPE NNSG2dVRamType;
#define NNSi_G2D_CELLATTR_BOUNDINGRECT_SHIFT 11
#define NNS_G2D_ANIMELEM_INDEX NNS_G2D_ANIMELEMENT_INDEX
#define NNS_G2D_ANIMELEM_INDEX_SRT NNS_G2D_ANIMELEMENT_INDEX_SRT
#define NNS_G2D_ANIMELEM_INDEX_T NNS_G2D_ANIMELEMENT_INDEX_T
#define NNS_G2dRestartMCCellAnimations NNS_G2dResetMCCellAnimationAll

// NitroSystem G3D: resource files
#define NNS_G3D_SIGNATURE_BMD0 NNS_G3D_SIGNATURE_NSBMD
#define NNS_G3D_SIGNATURE_BTX0 NNS_G3D_SIGNATURE_NSBTX
#define NNS_G3D_SIGNATURE_BCA0 NNS_G3D_SIGNATURE_NSBCA
#define NNS_G3D_SIGNATURE_BVA0 NNS_G3D_SIGNATURE_NSBVA
#define NNS_G3D_SIGNATURE_BMA0 NNS_G3D_SIGNATURE_NSBMA
#define NNS_G3D_SIGNATURE_BTA0 NNS_G3D_SIGNATURE_NSBTA
#define NNS_G3D_SIGNATURE_BTP0 NNS_G3D_SIGNATURE_NSBTP
#define NNS_G3DResGetMdlBlock NNS_G3dGetMdlSet
#define NNS_G3DResGetTexBlock NNS_G3dGetTex
#define NNS_G3DResGetAnm NNS_G3dGetAnmByIdx
#define NNS_G3DFind NNS_G3dGetResDataByName
#define NNS_G3DFindIndex NNS_G3dGetResDictIdxByName

// NitroSystem G3D: textures and palettes in VRAM
#define NNS_G3DResTexGetTexDataSize NNS_G3dTexGetRequiredSize
#define NNS_G3dTexGetRequiredSize4x4 NNS_G3dTex4x4GetRequiredSize
#define NNS_G3DResTexSetGfdKeys NNS_G3dTexSetTexKey
#define NNS_G3DResTexUploadTex NNS_G3dTexLoad
#define NNS_G3DResTexGetPaletteDataSize NNS_G3dPlttGetRequiredSize
#define NNS_G3DResTexSetPltGfdKey NNS_G3dPlttSetPlttKey
#define NNS_G3DResTexUploadPlt NNS_G3dPlttLoad

// NitroSystem G3D: render and animation objects
#define NNS_G3DModelAttachResource NNS_G3dRenderObjInit
#define NNS_G3DAnimationAlloc NNS_G3dAllocAnmObj
#define NNS_G3DAnimationCalcAllocSize NNS_G3dAnmObjCalcSizeRequired
#define NNS_G3DAnimationCreate NNS_G3dAnmObjInit
#define NNS_G3DAnimationBind NNS_G3dRenderObjAddAnmObj
#define NNS_G3DDraw NNS_G3dDraw

// NitroSystem G3D: geometry buffer
#define NNS_G3DWaitFIFO NNS_G3dGeFlushBuffer
#define NNS_G3DExecOp NNS_G3dGeBufferOP_N
#define NNS_G3DExecDisplayList NNS_G3dGeSendDL
#define NNS_G3DGetTransforms NNS_G3dGetCurrentMtx

// NitroSystem G3D: global state
#define NNS_G3DInit NNS_G3dInit
#define NNS_G3DRenderStateInit NNS_G3dGlbInit
#define NNS_G3DSetLightVector NNS_G3dGlbLightVector
#define NNS_G3DSetLightColor NNS_G3dGlbLightColor
#define NNS_G3DSetMatDifAmb NNS_G3dGlbMaterialColorDiffAmb
#define NNS_G3DSetMatSpeEmi NNS_G3dGlbMaterialColorSpecEmi
#define NNS_G3DFlushRenderState NNS_G3dGlbFlushP
#define NNS_G3DFlushViewProjection NNS_G3dGlbFlushVP
#define NNS_G3DFlushModelViewProjection NNS_G3dGlbFlushWVP
// bwbattle: the screen position on the 256 dots BW2's 2D actors are placed on, also with the 3D view widened
// (GFL_G3DSysSetViewWidth)
#define NNS_G3DProject GFL_G3DSysProjectToScreen

// bwbattle: the view's 2D actors draw through libntr's OBJ extension (SIM_SetObjExtension), beside Platinum's sprites:
// the main engine's OAM entries from BW_OBJ_EXT_FIRST_OAM on, their characters BW_OBJ_EXT_CHAR_OFFSET into OBJ VRAM
// and their palettes in the second set (HW_OBJ_PLTT_EXT, BW_OBJ_EXT_PLTT_OFFSET after HW_OBJ_PLTT)
#define BW_OBJ_EXT_FIRST_OAM 64
#define BW_OBJ_EXT_CHAR_OFFSET 0x20000
#define BW_OBJ_EXT_PLTT_OFFSET 0x200

#endif // BWBATTLE_COMPAT_H
