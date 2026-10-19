#include "types.h"
#include "gfl/g3d.h"
#include "shim/bw_shim.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "math.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nnsys/g2d.h"
#include "nnsys/g3d.h"
#include "system/mcss.h"
#include "system/palanm.h"
#ifdef BWBATTLE
#include "assets/trainer_backs.h"
#endif

// MCSS, the multi-cell sprite system: each sprite is a multi-cell animation whose cells are drawn as textured quads of
// one image in texture VRAM, described by the sprite's bin file, with a shadow drawn from the same quads. The file's
// name is the ROM's string, from GFL_HeapAllocate's calls. The MCSS* names are swan's. The functions that fill in a
// sprite's files, after MCSS_MulVecMtx44, are a file of their own, mcss_tool.c: the `.data` has this file's name
// before their table, where a file's literals come after its tables

// The bin file: each cell's quads, then some bytes that the battle reads
typedef struct {
    fx32 x;
    fx32 y;
    fx32 width;
    fx32 height;
    fx32 s;
    fx32 t;
} MCSSQuad;

typedef struct {
    MCSSQuad quad;
    // Drawn when its width is not 0, before the first quad or after it
    MCSSQuad quad2;
} MCSSCell;

typedef struct {
    u32 count;
    u16 unk4;
    u16 unk6;
    s16 unk8;
    s16 unkA;
    MCSSCell cells[];
} MCSSBin;

typedef struct {
    u8 unk0;
    u8 unk1;
    u8 unk2[];
} MCSSBinTail;

struct MCSSSystem {
    // The archive that func_02019b98 opened, which the sprites of that archive are read from
    ArcTool *arc;
    u32 arcId;
    // Runs the sprites' loads when set, and the V-blank's tasks otherwise
    TCBManager *tcbMgr;
    // The load of the shadows' palette
    TCB *shadowTcb;
    int count;
    MCSS **mcss;
    NNSG2dImagePaletteProxy shadowPltt;
    // Whether the sprites that ask for it are drawn with an orthographic projection of their own
    u32 ortho : 1;
    // Halves the depth of that projection
    u32 orthoNear : 1;
    u32 texBase;
    u32 plttBase;
#ifdef BWBATTLE
    // bwbattle: the VRAM the sprites are given (MCSSSys_Create)
    NNSGfdTexKey texKey;
    NNSGfdPlttKey plttKey;
    u32 reservedTexSlots;
#endif
    MCSSLoadCallback loadCallback;
    u32 loadCallbackWork;
    u32 heapId;
    // The scale of the multi-cell's own offset
    fx32 offsetScale;
    // How far apart in depth the cells are drawn, 0 for the default
    fx32 cellDepth;
};

struct MCSS {
    // The tasks that load the sprite's graphics, its fading palette and its blended palette
    TCB *loadTcb;
    TCB *fadeTcb;
    TCB *blendTcb;
    void *cellFile;
    NNSG2dCellDataBank *cellBank;
    void *mcFile;
    NNSG2dMultiCellDataBank *mcBank;
    void *animFile;
    NNSG2dAnimBankData *animBank;
    void *mcAnimFile;
    NNSG2dAnimBankData *mcAnimBank;
    MCSSBin *bin;
    MCSSBinTail *binTail;
    NNSG2dMultiCellAnimation mcAnim;
    void *mcWork;
    NNSG2dImageProxy imageProxy;
    NNSG2dImagePaletteProxy plttProxy;
    // The sprite's colors, and the colors that func_0201afb0 blended them into
    u16 *pltt;
    u16 *blendedPltt;
    int plttSize;
    VecFx32 pos;
    VecFx32 scale;
    // Only z is used, as the sprite's rotation
    VecFx32 rotation;
    VecFx32 shadowScale;
    VecFx32 unk110;
    // Added to the position, and multiplied into the scale
    VecFx32 posOffset;
    VecFx32 scaleOffset;
    // The palette's fade: its level, the level it ends at, the frames until its next step and between steps
    s8 fadeLevel;
    s8 fadeEnd;
    s8 fadeWaitCount;
    s8 fadeWait;
    s32 fadeStep;
    u32 fadeColor;
    u32 alpha : 8;
    u32 flipV : 1;
    u32 pause : 2;
    u32 hidden : 1;
    u32 fading : 1;
    u32 loaded : 1;
    // 0x20 for half the sprite's alpha
    u32 shadowAlpha : 9;
    u32 noShadow : 1;
    u32 mosaic : 4;
    u32 blended : 1;
    u32 ortho : 1;
    // Draws the cells from the last, and the second quads after the first
    u32 reverse : 1;
    u32 unk140_31 : 1;
    // Set as the animation ends, to restart it at the next update
    u32 restart : 1;
    // Its slot in the system
    u32 index;
    u32 heapId;
    MCSSLoadInfo info;
    fx32 animSpeed;
    u16 shadowRotX;
    u16 shadowRotZ;
    VecFx32 shadowOffset;
#ifdef BWBATTLE
    // bwbattle: an image that stands in for the sprite's cells (MCSS_SetImage), drawn as one quad of the sprite's
    // texture with its palette: 4 bits a dot, its rows packed, and its 16 colors; uploaded once the sprite is loaded
    u8 *image;
    u16 imageWidth;
    u16 imageHeight;
    s16 imageAnchorX;
    s16 imageAnchorY;
    u16 imageColors[16];
    BOOL imagePending;
    BOOL imagePalettePending;
    fx32 imageScale;
    BwTrainerBack trainerBack;
    u32 extraTexSlots;
    u32 trainerTexSlot;
    NNSGfdPlttKey trainerPlttKey;
#endif
};

// The shadows' palette, and where it goes in texture palette VRAM
#define MCSS_SHADOW_ARCID 11
#define MCSS_SHADOW_PLTT 355
#define MCSS_SHADOW_PLTT_ADDR 0x1100
#define MCSS_SHADOW_PLTT_FILE_SIZE 0x200

// Each sprite's share of the texture image and of the texture palette
#define MCSS_TEX_SIZE 0x4000
#define MCSS_PLTT_SIZE 0x20

#define MCSS_ALPHA_MAX 31
#define MCSS_SHADOW_ALPHA_HALF 0x20
#define MCSS_MOSAIC_MAX 15

// The scanlines in which a V-blank task may load to VRAM
#define MCSS_LOAD_LINE_MIN 192
#define MCSS_LOAD_LINE_MAX 200

static void MCSS_DrawQuad(MCSSSystem *system, MCSS *mcss, fx32 x, fx32 y, fx32 width, fx32 height, fx32 s, fx32 t,
                          const NNSG2dAnimDataSRT *cellSrt, const NNSG2dAnimDataSRT *mcSrt,
                          NNSG2dImagePaletteProxy *shadowPltt, int node, BOOL ortho, fx32 *z, u8 flip);
static void LoadMCSSGraphicsData(MCSSSystem *system, int index, const MCSSLoadInfo *info);
static void LoadMCSSGraphicsDataArc(MCSSSystem *system, int index, const MCSSLoadInfo *info);
static void MCSS_LoadTask(TCB *tcb, void *data);
#ifdef BWBATTLE
static void MCSS_UploadImage(MCSS *mcss);
#endif
static void MCSS_PaletteTask(TCB *tcb, void *data);
static void MCSS_InitMCAnime(MCSS *mcss, u32 mcType);
static void MCSS_SetMaterial(void);
static void MCSS_UpdatePaletteFade(MCSSSystem *system, MCSS *mcss);
static void MCSS_FreeFiles(MCSS *mcss);
static void func_0201bae8(TCB *tcb);
static void func_0201bb14(TCB *tcb);
static void MCSS_MakeMosaic(MCSS *mcss, MCSSLoad *load);
static void MCSS_MulVecMtx44(const VecFx32 *vec, const MtxFx44 *mtx, VecFx32 *dest, fx32 *w);

static u32 MCSS_PaletteAddress(MCSSSystem *system, MCSS *mcss) {
#ifdef BWBATTLE
    if (mcss->trainerPlttKey != NNS_GFD_ALLOC_ERROR_PLTTKEY) {
        return NNS_GfdGetPlttKeyAddr(mcss->trainerPlttKey);
    }
#endif
    return system->plttBase + mcss->index * MCSS_PLTT_SIZE;
}

MCSSSystem *MCSSSys_Create(u32 capacity, HeapID heapId) {
    MCSSSystem *system = GFL_HeapAllocate(heapId, sizeof(MCSSSystem), TRUE, "mcss.c", 122);
    MCSSLoad *load;

    system->count = capacity;
    system->heapId = heapId;
    system->offsetScale = FX32_ONE;
    system->mcss = GFL_HeapAllocate(heapId, capacity * sizeof(MCSS *), TRUE, "mcss.c", 129);
    system->texBase = 0x24000;
    system->plttBase = 0x1000;
#ifdef BWBATTLE
    // bwbattle: BW2 puts the sprites at fixed addresses of texture and palette VRAM, which its battle leaves out of
    // the VRAM managers. Inside Platinum's battle the managers hand those out, so the sprites, with the shadows'
    // palette after theirs, are allocated from them. The shadows' palette file is a whole 256 colors, loaded whole
    system->texKey = NNS_GfdAllocTexVram(capacity * MCSS_TEX_SIZE, FALSE, 0);
    system->plttKey = NNS_GfdAllocPlttVram(capacity * MCSS_PLTT_SIZE + MCSS_SHADOW_PLTT_FILE_SIZE, FALSE, 0);
    GFL_ASSERT(system->texKey != NNS_GFD_ALLOC_ERROR_TEXKEY && system->plttKey != NNS_GFD_ALLOC_ERROR_PLTTKEY);
    system->texBase = NNS_GfdGetTexKeyAddr(system->texKey);
    system->plttBase = NNS_GfdGetPlttKeyAddr(system->plttKey);
#endif
    NNS_G2dInitImagePaletteProxy(&system->shadowPltt);

    load = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(MCSSLoad), TRUE, "mcss.c", 144);
    load->system = system;
    load->plttProxy = &system->shadowPltt;
#ifdef BWBATTLE
    load->plttAddr = system->plttBase + capacity * MCSS_PLTT_SIZE;
#else
    load->plttAddr = MCSS_SHADOW_PLTT_ADDR;
#endif
    load->plttFile = GFL_G2DIOReadNCLR(MCSS_SHADOW_ARCID, MCSS_SHADOW_PLTT, &load->pltt, HEAPID_TAIL(heapId));
    system->shadowTcb = GFL_VBlankTCBAdd(MCSS_LoadTask, load, 0);
    return system;
}

void MCSSSys_Free(MCSSSystem *system) {
    int i;

    func_02019bb4(system);
    if (system->shadowTcb != NULL) {
        MCSSLoad *load = GFL_TCBGetData(system->shadowTcb);

        // BUG: the shadows' palette hasn't loaded, and its data is the palette file's, which this frees from the
        // middle, in place of the file
#ifdef BWBATTLE
        GFL_HeapFree(load->plttFile);
#else
        GFL_HeapFree(load->pltt->pRawData);
        GFL_HeapFree(load->pltt);
#endif
        GFL_HeapFree(load);
        GFL_TCBRemove(system->shadowTcb);
        system->shadowTcb = NULL;
    }
    for (i = 0; i < system->count; i++) {
        if (system->mcss[i] != NULL) {
            MCSSSys_Remove(system, system->mcss[i]);
        }
    }
    GFL_HeapFree(system->mcss);
#ifdef BWBATTLE
    NNS_GfdFreeTexVram(system->texKey);
    NNS_GfdFreePlttVram(system->plttKey);
#endif
    GFL_HeapFree(system);
}

void MCSSSys_Update(MCSSSystem *system) {
    int i;

    for (i = 0; i < system->count; i++) {
        if (system->mcss[i] != NULL) {
            if (system->mcss[i]->pause == 0) {
                NNS_G2dTickMCAnimation(&system->mcss[i]->mcAnim, system->mcss[i]->animSpeed);
            }
            if (system->mcss[i]->fading) {
                MCSS_UpdatePaletteFade(system, system->mcss[i]);
            }
            if (system->mcss[i]->restart == TRUE) {
                MCSS_RestartAnimation(system->mcss[i]);
                NNS_G2dTickMCAnimation(&system->mcss[i]->mcAnim, 0);
                system->mcss[i]->restart = FALSE;
            }
        }
    }
}

void func_02019b98(MCSSSystem *system, u32 arcId) {
    if (system->arc == NULL) {
        system->arcId = arcId;
        system->arc = GFL_ArcSysCreateFileHandle(arcId, system->heapId);
    }
}

void func_02019bb4(MCSSSystem *system) {
    if (system->arc != NULL) {
        GFL_ArcToolFree(system->arc);
        system->arcId = 0;
        system->arc = NULL;
    }
}

void func_02019bcc(MCSSSystem *system, TCBManager *tcbMgr) {
    system->tcbMgr = tcbMgr;
}

void MCSSSys_SetLoadCallback(MCSSSystem *system, MCSSLoadCallback callback, u32 work) {
    system->loadCallback = callback;
    system->loadCallbackWork = work;
}

void func_02019bd8(MCSSSystem *system, MCSS *mcss, const MCSSLoadInfo *info) {
    MCSS_FreeFiles(mcss);
    mcss->info = *info;
    LoadMCSSGraphicsData(system, mcss->index, info);
}

// The current frame of an animation, as the SRT it may hold
static inline void MCSS_GetAnimSrt(const NNSG2dAnimController *animCtrl, NNSG2dAnimDataSRT *srt) {
    switch ((u16)animCtrl->pAnimSequence->animType) {
    case NNS_G2D_ANIMELEM_INDEX:
        srt->index = *(u16 *)animCtrl->pCurrent->pContent;
        srt->rotZ = 0;
        srt->sx = FX32_ONE;
        srt->sy = FX32_ONE;
        srt->px = 0;
        srt->py = 0;
        break;
    case NNS_G2D_ANIMELEM_INDEX_SRT:
        *srt = *(NNSG2dAnimDataSRT *)animCtrl->pCurrent->pContent;
        break;
    case NNS_G2D_ANIMELEM_INDEX_T: {
        NNSG2dAnimDataT *data = animCtrl->pCurrent->pContent;

        srt->index = data->index;
        srt->rotZ = 0;
        srt->sx = FX32_ONE;
        srt->sy = FX32_ONE;
        srt->px = data->px;
        srt->py = data->py;
        break;
    }
    }
}

void MCSSSys_Draw(MCSSSystem *system) {
    MCSS *mcss;
    NNSG2dImageProxy *imageProxy;
    int i;
    int j;
    NNSG2dMCCellAnimation **cellAnims;
    NNSG2dAnimDataSRT mcSrt;
    NNSG2dAnimDataSRT cellSrt;
    VecFx32 pos;
    MtxFx44 billboard;
    MtxFx43 invCamera;
    MtxFx33 rotation;
    fx32 x;
    fx32 y;
    u16 rot;
    u8 flip;
    BOOL ortho;

    NNS_G3DWaitFIFO();
    G3_PushMtx();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_StoreMtx(0);
    G3_MtxMode(GX_MTXMODE_TEXTURE);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    MTX_Inverse43(&NNS_G3dGlb.cameraMtx, &invCamera);
    MI_Copy36B(&invCamera, &rotation);
    MAT3_To4x4(&rotation, &billboard);

    for (i = 0; i < system->count; i++) {
        NNSG2dMultiCellAnimation *mcAnim;
        fx32 z;

        if (system->mcss[i] == NULL || system->mcss[i]->hidden || system->mcss[i]->loaded != TRUE) {
            continue;
        }
        z = 0;
        ortho = system->ortho ? system->mcss[i]->ortho : FALSE;
        mcss = system->mcss[i];
        imageProxy = &mcss->imageProxy;
        if (ortho == FALSE) {
            G3i_LookAt_(&NNS_G3dGlb.camPos, &NNS_G3dGlb.camUp, &NNS_G3dGlb.camTarget, TRUE, NULL);
        } else {
            VecFx32 camPos = { 0, 0, 0 };
            VecFx32 camUp = { 0, FX32_ONE, 0 };
            VecFx32 camTarget = { 0, 0, -FX32_ONE };

            G3i_LookAt_(&camPos, &camUp, &camTarget, TRUE, NULL);
        }
        mcAnim = &mcss->mcAnim;
        cellAnims = (NNSG2dMCCellAnimation **)&mcss->mcAnim.multiCellInstance.pCellAnimInstasnces;
        MCSS_GetAnimSrt(&mcAnim->animCtrl, &mcSrt);
#ifdef BWBATTLE
        if (mcss->image != NULL) {
            if (mcss->imagePending && mcss->loadTcb == NULL) {
                MCSS_UploadImage(mcss);
            }
            // The image stands still where the sprite is: none of its animation's moves
            mcSrt.px = 0;
            mcSrt.py = 0;
            mcSrt.sx = FX32_ONE;
            mcSrt.sy = FX32_ONE;
            mcSrt.rotZ = 0;
        }
#endif
        pos.x = mcss->pos.x + mcss->posOffset.x;
        pos.y = mcss->pos.y + mcss->posOffset.y;
        pos.z = mcss->pos.z + mcss->posOffset.z;
        if (ortho == FALSE) {
            x = FX_Mul(mcSrt.px << 8, system->offsetScale);
            y = FX_Mul(-mcSrt.py << 8, system->offsetScale);
        } else {
            fx32 scaleX = FX_Mul(mcss->scale.x, mcss->scaleOffset.x);
            fx32 scaleY = FX_Mul(mcss->scale.y, mcss->scaleOffset.y);
            fx32 offsetX;
            fx32 offsetY;
            MtxFx44 viewProj;
            fx32 w;

            MTX_Copy43To44_(&NNS_G3dGlb.cameraMtx, &viewProj);
            MTX_Concat44(&viewProj, &NNS_G3dGlb.projMtx, &viewProj);
            MCSS_MulVecMtx44(&pos, &viewProj, &pos, &w);
#ifdef BWBATTLE
            // bwbattle: the orthographic view is as wide as the 3D view (GFL_G3DSysSetViewWidth, MCSS_DrawQuad)
            pos.x = FX_Mul(FX_Div(pos.x, w), (fx32)(GFL_G3DSysGetViewWidth() / 2 * FX32_ONE));
#else
            pos.x = FX_Mul(FX_Div(pos.x, w), 128 * FX32_ONE);
#endif
            pos.y = FX_Mul(FX_Div(pos.y, w), 96 * FX32_ONE);
            pos.z = -pos.z;
            offsetX = FX32_CONST(mcSrt.px);
            offsetY = FX32_CONST(-mcSrt.py);
            x = FX_Mul(offsetX, scaleX / 16);
            y = FX_Mul(offsetY, scaleY / 16);
        }
        G3_Translate(pos.x, pos.y, pos.z);
        if (ortho == FALSE) {
            G3_MultMtx44(&billboard);
        }
        rot = mcSrt.rotZ + (mcss->rotation.z >> FX32_SHIFT);
        G3_Translate(x, y, 0);
        flip = 0;
        G3_RotZ(-FX_SinIdx(rot), FX_CosIdx(rot));
        G3_Scale(FX_Mul(FX_Mul(mcSrt.sx, mcss->scale.x), mcss->scaleOffset.x),
                 FX_Mul(FX_Mul(mcSrt.sy, mcss->scale.y), mcss->scaleOffset.y), FX32_ONE);
        if (mcss->scale.x < 0) {
            flip++;
        }
        G3_StoreMtx(29);

        // The shadow's matrix
        G3i_LookAt_(&NNS_G3dGlb.camPos, &NNS_G3dGlb.camUp, &NNS_G3dGlb.camTarget, TRUE, NULL);
        G3_Translate(mcss->pos.x + mcss->posOffset.x + mcss->shadowOffset.x, mcss->pos.y + mcss->shadowOffset.y,
                     mcss->pos.z + mcss->posOffset.z + mcss->shadowOffset.z);
        G3_RotX(FX_SinIdx(mcss->shadowRotX), FX_CosIdx(mcss->shadowRotX));
        G3_Translate(FX_Mul(FX_Mul(mcSrt.px << 8, system->offsetScale), mcss->shadowScale.x),
                     FX_Mul(FX_Mul(-mcSrt.py << 8, system->offsetScale), mcss->shadowScale.y), 0);
        G3_RotZ(-FX_SinIdx(mcss->shadowRotZ), FX_CosIdx(mcss->shadowRotZ));
        G3_RotZ(-FX_SinIdx(rot), FX_CosIdx(rot));
        G3_Scale(FX_Mul(FX_Mul(FX_Mul(mcSrt.sx, mcss->shadowScale.x), mcss->unk110.x), mcss->scaleOffset.x),
                 FX_Mul(FX_Mul(FX_Mul(mcSrt.sy, mcss->shadowScale.y), mcss->unk110.y), mcss->scaleOffset.y), FX32_ONE);
        G3_StoreMtx(30);

        MCSS_SetMaterial();
        G3_TexImageParam(imageProxy->attr.fmt, GX_TEXGEN_TEXCOORD, imageProxy->attr.sizeS, imageProxy->attr.sizeT,
                         GX_TEXREPEAT_ST, GX_TEXFLIP_ST, imageProxy->attr.plttUse,
                         imageProxy->vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_3DMAIN]);

#ifdef BWBATTLE
        // bwbattle: the image as one quad, positioned around its own anchor instead of the stand-in's cell offsets
        if (mcss->image != NULL) {
            NNSG2dAnimDataSRT still = { 0 };

            still.sx = FX32_ONE;
            still.sy = FX32_ONE;
            if (!mcss->imagePending) {
                still.sx = mcss->imageScale;
                still.sy = mcss->imageScale;
                MCSS_DrawQuad(system, mcss, -mcss->imageAnchorX * 256, mcss->imageAnchorY * 256,
                              mcss->imageWidth * FX32_ONE, mcss->imageHeight * FX32_ONE, 0, 0, &still, &mcSrt,
                              &system->shadowPltt, 0, ortho, &z, flip);
            }
            continue;
        }
#endif
        for (j = 0; j < mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].numNodes; j++) {
            NNSG2dMCCellAnimation *cellAnim;
            const MCSSCell *cell;
            int node;

#ifdef BWBATTLE
            if (mcss->trainerBack == BW_TRAINER_BACK_DAWN || mcss->trainerBack == BW_TRAINER_BACK_LUCAS) {
                // The arm joins underneath Dawn's hair and Lucas's shoulder/backpack strap. Keep that joint
                // behind the body, including during fades, instead of drawing the cut sleeve end over it.
                node = j;
                z = (j + 1 - mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].numNodes) * 0x100;
            } else
#endif
            if (mcss->alpha != MCSS_ALPHA_MAX || mcss->reverse) {
                node = mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].numNodes - 1 - j;
            } else {
                node = j;
            }
            cellAnim =
                &(*cellAnims)
                    [(mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].pHierDataArray[node].nodeAttr &
                      0xff00) >>
                     8];
            MCSS_GetAnimSrt(&cellAnim->cellAnim.animCtrl, &cellSrt);
            cell = &mcss->bin->cells[cellSrt.index];
            if (cell->quad2.width != 0 && mcss->reverse == FALSE) {
                if (mcss->flipV) {
                    MCSS_DrawQuad(system, mcss, cell->quad2.x, cell->quad2.y, cell->quad2.width, cell->quad2.height,
                                  cell->quad2.s, cell->quad2.t + cell->quad2.height, &cellSrt, &mcSrt,
                                  &system->shadowPltt, node, ortho, &z, flip);
                } else {
                    MCSS_DrawQuad(system, mcss, cell->quad2.x, cell->quad2.y, cell->quad2.width, cell->quad2.height,
                                  cell->quad2.s, cell->quad2.t, &cellSrt, &mcSrt, &system->shadowPltt, node, ortho, &z,
                                  flip);
                }
            }
            MCSS_DrawQuad(system, mcss, cell->quad.x, cell->quad.y, cell->quad.width, cell->quad.height, cell->quad.s,
                          cell->quad.t, &cellSrt, &mcSrt, &system->shadowPltt, node, ortho, &z, flip);
            if (cell->quad2.width != 0 && mcss->reverse == TRUE) {
                if (mcss->flipV) {
                    MCSS_DrawQuad(system, mcss, cell->quad2.x, cell->quad2.y, cell->quad2.width, cell->quad2.height,
                                  cell->quad2.s, cell->quad2.t + cell->quad2.height, &cellSrt, &mcSrt,
                                  &system->shadowPltt, node, ortho, &z, flip);
                } else {
                    MCSS_DrawQuad(system, mcss, cell->quad2.x, cell->quad2.y, cell->quad2.width, cell->quad2.height,
                                  cell->quad2.s, cell->quad2.t, &cellSrt, &mcSrt, &system->shadowPltt, node, ortho, &z,
                                  flip);
                }
            }
        }
    }
    G3_PopMtx(1);
}

// Draws a quad of a cell and its shadow, and steps the depth for the next
static void MCSS_DrawQuad(MCSSSystem *system, MCSS *mcss, fx32 x, fx32 y, fx32 width, fx32 height, fx32 s, fx32 t,
                          const NNSG2dAnimDataSRT *cellSrt, const NNSG2dAnimDataSRT *mcSrt,
                          NNSG2dImagePaletteProxy *shadowPltt, int node, BOOL ortho, fx32 *z, u8 flip) {
    fx32 nodeX;
    fx32 nodeY;
    int polygonId;
    u8 shadowAlpha;

    shadowAlpha = mcss->shadowAlpha == MCSS_SHADOW_ALPHA_HALF ? mcss->alpha / 2 : mcss->shadowAlpha;
    if (mcss->alpha == MCSS_ALPHA_MAX || mcss->unk140_31) {
        polygonId = 0;
    } else {
        polygonId = node;
        if (polygonId > 63) {
            polygonId = 63;
        }
    }
    if (ortho) {
#ifdef BWBATTLE
        // bwbattle: as wide as the 3D view (GFL_G3DSysSetViewWidth), so that the sprites keep their size
        G3i_OrthoW_(FX32_CONST(96), FX32_CONST(-96), -(fx32)(GFL_G3DSysGetViewWidth() / 2 * FX32_ONE),
                    (fx32)(GFL_G3DSysGetViewWidth() / 2 * FX32_ONE), FX32_ONE,
#else
        G3i_OrthoW_(FX32_CONST(96), FX32_CONST(-96), FX32_CONST(-128), FX32_CONST(128), FX32_ONE,
#endif
                    FX32_ONE * (system->orthoNear ? 512 : 1024), FX32_ONE, TRUE, NULL);
        G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    }
    G3_RestoreMtx(29);
    G3_TexPlttBase(mcss->plttProxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_3DMAIN], mcss->plttProxy.fmt);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, polygonId, mcss->alpha, 0);
    nodeX =
        (cellSrt->px + mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt->index].pHierDataArray[node].posX)
        << 8;
    nodeY =
        -(cellSrt->py + mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt->index].pHierDataArray[node].posY)
        << 8;
#ifdef BWBATTLE
    // bwbattle: an image (MCSS_SetImage) is no node of the multi-cell
    if (mcss->image != NULL) {
        nodeX = 0;
        nodeY = 0;
    }
#endif
    G3_Translate(nodeX, nodeY, *z);
    G3_RotZ(-FX_SinIdx(cellSrt->rotZ), FX_CosIdx(cellSrt->rotZ));
    if (flip == 0) {
        G3_Scale(cellSrt->sx, cellSrt->sy, FX32_ONE);
        G3_Translate(x, y, 0);
        G3_Scale(width, height, FX32_ONE);
    } else if (flip & 1) {
        G3_Scale(-cellSrt->sx, cellSrt->sy, FX32_ONE);
        G3_Translate(-x, y, 0);
        G3_Scale(width, height, FX32_ONE);
        G3_Translate(-0x100, 0, 0);
        s = FX32_CONST(512) - s - width;
    }
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(s, t);
    G3_Vtx(0, 0, 0);
    G3_TexCoord(s, t + height);
    G3_Vtx(0, -0x100, 0);
    G3_TexCoord(s + width, t + height);
    G3_Vtx(0x100, -0x100, 0);
    G3_TexCoord(s + width, t);
    G3_Vtx(0x100, 0, 0);
    G3_End();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_RestoreMtx(0);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_RestoreMtx(30);

    if (mcss->noShadow == FALSE && shadowAlpha != 0) {
        G3_TexPlttBase(shadowPltt->vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_3DMAIN], shadowPltt->fmt);
        G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 1, shadowAlpha, 0);
        G3_Translate(nodeX, nodeY, 0);
        G3_RotZ(-FX_SinIdx(cellSrt->rotZ), FX_CosIdx(cellSrt->rotZ));
        G3_Scale(cellSrt->sx, cellSrt->sy, FX32_ONE);
        G3_Translate(x, y, 0);
        G3_Scale(width, height, FX32_ONE);
        G3_Begin(GX_BEGIN_QUADS);
        G3_TexCoord(s, t);
        G3_Vtx(0, 0, 0);
        G3_TexCoord(s, t + height);
        G3_Vtx(0, -0x100, 0);
        G3_TexCoord(s + width, t + height);
        G3_Vtx(0x100, -0x100, 0);
        G3_TexCoord(s + width, t);
        G3_Vtx(0x100, 0, 0);
        G3_End();
    }

    if (ortho == FALSE) {
        if (system->cellDepth == 0) {
            *z -= ((mcss->alpha != MCSS_ALPHA_MAX) | mcss->reverse) ? -0x100 : 0x100;
        } else {
            *z -= ((mcss->alpha != MCSS_ALPHA_MAX) | mcss->reverse) ? -system->cellDepth : system->cellDepth;
        }
    } else if (system->orthoNear) {
        *z -= ((mcss->alpha != MCSS_ALPHA_MAX) | mcss->reverse) ? -0x100 : 0x100;
    } else {
        *z -= ((mcss->alpha != MCSS_ALPHA_MAX) | mcss->reverse) ? -0x400 : 0x400;
    }
}

MCSS *MCSSSys_Add(MCSSSystem *system, fx32 x, fx32 y, fx32 z, const MCSSLoadInfo *info) {
    int i;

    for (i = 0; i < system->count; i++) {
        if (system->mcss[i] == NULL
#ifdef BWBATTLE
            && !(system->reservedTexSlots & (1u << i))
#endif
        ) {
            system->mcss[i] = GFL_HeapAllocate(system->heapId, sizeof(MCSS), TRUE, "mcss.c", 908);
            system->mcss[i]->index = i;
            system->mcss[i]->heapId = system->heapId;
            system->mcss[i]->pos.x = x;
            system->mcss[i]->pos.y = y;
            system->mcss[i]->pos.z = z;
            system->mcss[i]->scale.x = FX32_ONE;
            system->mcss[i]->scale.y = FX32_ONE;
            system->mcss[i]->scale.z = FX32_ONE;
            system->mcss[i]->posOffset.x = 0;
            system->mcss[i]->posOffset.y = 0;
            system->mcss[i]->posOffset.z = 0;
            system->mcss[i]->scaleOffset.x = FX32_ONE;
            system->mcss[i]->scaleOffset.y = FX32_ONE;
            system->mcss[i]->scaleOffset.z = FX32_ONE;
            system->mcss[i]->alpha = MCSS_ALPHA_MAX;
            system->mcss[i]->shadowAlpha = MCSS_SHADOW_ALPHA_HALF;
            system->mcss[i]->hidden = FALSE;
            system->mcss[i]->shadowRotX = 0xc400;
            system->mcss[i]->shadowRotZ = 0;
            system->mcss[i]->shadowOffset.x = 0;
            system->mcss[i]->shadowOffset.y = 0;
            system->mcss[i]->shadowOffset.z = -FX32_ONE;
            system->mcss[i]->unk110.x = FX32_ONE;
            system->mcss[i]->unk110.y = FX32_ONE;
            system->mcss[i]->unk110.z = FX32_ONE;
            system->mcss[i]->info = *info;
            system->mcss[i]->animSpeed = FX32_ONE;
            system->mcss[i]->ortho = TRUE;
            LoadMCSSGraphicsData(system, i, info);
            break;
        }
    }
    return system->mcss[i];
}

void MCSSSys_Remove(MCSSSystem *system, MCSS *mcss) {
    MCSS_FreeFiles(mcss);
    if (mcss->loadTcb != NULL) {
        func_0201bae8(mcss->loadTcb);
        mcss->loadTcb = NULL;
    }
    if (mcss->fadeTcb != NULL) {
        func_0201bb14(mcss->fadeTcb);
        mcss->fadeTcb = NULL;
    }
    if (mcss->blendTcb != NULL) {
        func_0201bb14(mcss->blendTcb);
        mcss->blendTcb = NULL;
    }
    system->mcss[mcss->index] = NULL;
#ifdef BWBATTLE
    system->reservedTexSlots &= ~mcss->extraTexSlots;
    if (mcss->trainerPlttKey != NNS_GFD_ALLOC_ERROR_PLTTKEY) {
        NNS_GfdFreePlttVram(mcss->trainerPlttKey);
    }
#endif
    GFL_HeapFree(mcss);
}

void func_0201aacc(MCSSSystem *system) {
    system->ortho = TRUE;
}

void func_0201aadc(MCSSSystem *system) {
    system->ortho = FALSE;
}

void func_0201aae8(MCSS *mcss) {
    mcss->ortho = TRUE;
}

void func_0201aaf8(MCSS *mcss) {
    mcss->ortho = FALSE;
}

#ifdef BWBATTLE
BOOL MCSS_SetTrainerBackSprite(MCSS *mcss, BwTrainerBack trainer) {
    const BwTrainerBackData *data = BwTrainerBack_Get(trainer);
    MCSSLoad *load;
    MCSSSystem *system;
    NNSG2dCharacterData chars;

    // The host registers the replacement before a trainer is created. Apply it before the queued VRAM load,
    // leaving the original cell/multi-cell animations and their callbacks intact.
    if (data == NULL || mcss->info.arcId != 72 || mcss->loadTcb == NULL || mcss->bin->count != data->cellCount) {
        return FALSE;
    }
    load = GFL_TCBGetData(mcss->loadTcb);
    if (load->charFile == NULL || load->plttFile == NULL) {
        return FALSE;
    }
    system = load->system;
    if (mcss->extraTexSlots == 0) {
        // An 8-bit trainer atlas uses two adjacent slots in the already allocated MCSS texture pool. Reserve
        // the extra slot so a subsequently created Pokemon cannot overwrite half of the trainer's texture.
        u32 first;
        for (first = 0; first + 1 < (u32)system->count; first++) {
            if ((system->mcss[first] == NULL || system->mcss[first] == mcss)
                && (system->mcss[first + 1] == NULL || system->mcss[first + 1] == mcss)
                && !(system->reservedTexSlots & (3u << first))) {
                break;
            }
        }
        if (first + 1 >= (u32)system->count) {
            return FALSE;
        }
        mcss->trainerPlttKey = NNS_GfdAllocPlttVram(sizeof(data->colors), FALSE, 0);
        if (mcss->trainerPlttKey == NNS_GFD_ALLOC_ERROR_PLTTKEY) {
            return FALSE;
        }
        mcss->trainerTexSlot = first;
        mcss->extraTexSlots = (3u << first) & ~(1u << mcss->index);
        system->reservedTexSlots |= mcss->extraTexSlots;
    }
    chars = *load->chars;
    GFL_HeapFree(load->charFile);
    load->charFile = GFL_HeapAllocate(mcss->heapId, sizeof(chars) + sizeof(data->image), FALSE, "mcss.c", __LINE__);
    load->chars = load->charFile;
    *load->chars = chars;
    load->chars->pRawData = (u8 *)load->charFile + sizeof(chars);
    load->chars->pixelFmt = GX_TEXFMT_PLTT256;
    load->chars->szByte = sizeof(data->image);
    load->charAddr = system->texBase + mcss->trainerTexSlot * MCSS_TEX_SIZE;
    GFL_HeapFree(load->plttFile);
    load->plttFile = GFL_HeapAllocate(mcss->heapId, sizeof(*load->pltt) + sizeof(data->colors), TRUE, "mcss.c", __LINE__);
    load->pltt = load->plttFile;
    load->pltt->fmt = GX_TEXFMT_PLTT256;
    load->pltt->szByte = sizeof(data->colors);
    load->pltt->pRawData = (u8 *)load->plttFile + sizeof(*load->pltt);
    load->plttAddr = MCSS_PaletteAddress(system, mcss);
    GFL_HeapFree(mcss->pltt);
    GFL_HeapFree(mcss->blendedPltt);
    mcss->pltt = GFL_HeapAllocate(mcss->heapId, sizeof(data->colors), FALSE, "mcss.c", __LINE__);
    mcss->blendedPltt = GFL_HeapAllocate(mcss->heapId, sizeof(data->colors), FALSE, "mcss.c", __LINE__);
    mcss->plttSize = sizeof(data->colors);
    MI_CpuCopy8(data->image, load->chars->pRawData, sizeof(data->image));
    MI_CpuCopy16(data->colors, load->pltt->pRawData, sizeof(data->colors));
    MI_CpuCopy16(data->colors, mcss->pltt, sizeof(data->colors));
    mcss->trainerBack = trainer;
    for (u32 i = 0; i < data->cellCount; i++) {
        const BwTrainerBackCell *src = &data->cells[i];
        MCSSCell *dst = &mcss->bin->cells[i];

        MI_CpuClear8(dst, sizeof(*dst));
        dst->quad.x = src->x * 256;
        dst->quad.y = src->y * 256;
        dst->quad.width = src->width * FX32_ONE;
        dst->quad.height = src->height * FX32_ONE;
        dst->quad.s = src->s * FX32_ONE;
        dst->quad.t = src->t * FX32_ONE;
    }
    return TRUE;
}

void MCSS_SetImage(MCSS *mcss, const u8 *image, u32 width, u32 height, int anchorX, int anchorY,
    const u16 *colors, fx32 scale) {
    if (mcss->image == NULL || width != mcss->imageWidth || height != mcss->imageHeight) {
        if (mcss->image != NULL) {
            GFL_HeapFree(mcss->image);
        }
        mcss->image = GFL_HeapAllocate(mcss->heapId, width * height / 2, FALSE, "mcss.c", __LINE__);
        mcss->imagePalettePending = TRUE;
    }
    if (memcmp(colors, mcss->imageColors, sizeof(mcss->imageColors)) != 0) {
        mcss->imagePalettePending = TRUE;
    }
    MI_CpuCopy8(image, mcss->image, width * height / 2);
    mcss->imageWidth = width;
    mcss->imageHeight = height;
    mcss->imageAnchorX = anchorX;
    mcss->imageAnchorY = anchorY;
    mcss->imageScale = scale;
    MI_CpuCopy16(colors, mcss->imageColors, sizeof(mcss->imageColors));
    mcss->imagePending = TRUE;
}

// The image into the sprite's texture, its top left, and its colors into the sprite's palette, which the palette's
// fades start from
static void MCSS_UploadImage(MCSS *mcss) {
    u32 texWidth = 8 << mcss->imageProxy.attr.sizeS;
    u32 texHeight = 8 << mcss->imageProxy.attr.sizeT;
    u32 size = texWidth * texHeight / 2;
    u8 *texels;

    if (mcss->imageWidth > texWidth || mcss->imageHeight > texHeight || size > MCSS_TEX_SIZE) {
        mcss->imagePending = FALSE;
        return;
    }
    texels = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), size, TRUE, "mcss.c", __LINE__);
    for (u32 y = 0; y < mcss->imageHeight; y++) {
        MI_CpuCopy8(mcss->image + y * mcss->imageWidth / 2, texels + y * texWidth / 2, mcss->imageWidth / 2);
    }
    DC_FlushRange(texels, size);
    GX_BeginLoadTex();
    GX_LoadTex(texels, mcss->imageProxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_3DMAIN], size);
    GX_EndLoadTex();
    GFL_HeapFree(texels);
    // Animated cells share a palette. Uploading it again on each frame would undo the effect's current fade.
    if (mcss->imagePalettePending) {
        if (mcss->pltt != NULL && mcss->plttSize >= (int)sizeof(mcss->imageColors)) {
            MI_CpuCopy16(mcss->imageColors, mcss->pltt, sizeof(mcss->imageColors));
        }
        DC_FlushRange(mcss->imageColors, sizeof(mcss->imageColors));
        GX_BeginLoadTexPltt();
        GX_LoadTexPltt(mcss->imageColors, mcss->plttProxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_3DMAIN],
                       sizeof(mcss->imageColors));
        GX_EndLoadTexPltt();
        mcss->imagePalettePending = FALSE;
    }
    mcss->imagePending = FALSE;
}
#endif

void MCSS_GetPosition(MCSS *mcss, VecFx32 *position) {
    position->x = mcss->pos.x;
    position->y = mcss->pos.y;
    position->z = mcss->pos.z;
}

void MCSS_SetPosition(MCSS *mcss, VecFx32 *position) {
    mcss->pos.x = position->x;
    mcss->pos.y = position->y;
    mcss->pos.z = position->z;
}

void func_0201ab3c(MCSS *mcss, VecFx32 *out) {
    out->x = mcss->posOffset.x;
    out->y = mcss->posOffset.y;
    out->z = mcss->posOffset.z;
}

void func_0201ab54(MCSS *mcss, VecFx32 *a1) {
    mcss->posOffset.x = a1->x;
    mcss->posOffset.y = a1->y;
    mcss->posOffset.z = a1->z;
}

void func_0201ab70(MCSS *mcss, VecFx32 *out) {
    out->x = mcss->scale.x;
    out->y = mcss->scale.y;
    out->z = mcss->scale.z;
}

void MCSS_SetScale(MCSS *mcss, VecFx32 *scale) {
    mcss->scale.x = scale->x;
    mcss->scale.y = scale->y;
    mcss->scale.z = scale->z;
}

void func_0201aba0(MCSS *mcss, VecFx32 *out) {
    out->x = mcss->scaleOffset.x;
    out->y = mcss->scaleOffset.y;
    out->z = mcss->scaleOffset.z;
}

void func_0201abb8(MCSS *mcss, VecFx32 *a1) {
    mcss->scaleOffset.x = a1->x;
    mcss->scaleOffset.y = a1->y;
    mcss->scaleOffset.z = a1->z;
}

void func_0201abd4(MCSS *mcss, VecFx32 *out) {
    out->x = mcss->rotation.x;
    out->y = mcss->rotation.y;
    out->z = mcss->rotation.z;
}

void func_0201abf0(MCSS *mcss, const VecFx32 *in) {
    mcss->rotation.x = in->x;
    mcss->rotation.y = in->y;
    mcss->rotation.z = in->z;
}

void func_0201ac0c(MCSS *mcss, VecFx32 *a1) {
    mcss->shadowScale.x = a1->x;
    mcss->shadowScale.y = a1->y;
    mcss->shadowScale.z = a1->z;
}

void func_0201ac28(MCSS *mcss, VecFx32 *out) {
    out->x = mcss->unk110.x;
    out->y = mcss->unk110.y;
    out->z = mcss->unk110.z;
}

void func_0201ac40(MCSS *mcss, VecFx32 *in) {
    mcss->unk110.x = in->x;
    mcss->unk110.y = in->y;
    mcss->unk110.z = in->z;
}

void func_0201ac5c(MCSS *mcss, u16 a1) {
    mcss->shadowRotX = a1;
}

void func_0201ac64(MCSS *mcss, u16 value) {
    mcss->shadowRotZ = value;
}

void func_0201ac70(MCSS *mcss, VecFx32 *a1) {
    mcss->shadowOffset.x = a1->x;
    mcss->shadowOffset.y = a1->y;
    mcss->shadowOffset.z = a1->z;
}

void func_0201ac8c(MCSS *mcss) {
    mcss->flipV = TRUE;
}

void func_0201ac9c(MCSS *mcss) {
    mcss->flipV = FALSE;
}

void func_0201acb0(MCSS *mcss) {
    mcss->flipV ^= 1;
}

BOOL func_0201acd4(MCSS *mcss) {
    return mcss->pause;
}

void MCSS_PauseAnimation(MCSS *mcss) {
    mcss->pause |= 1;
}

void func_0201ad04(MCSS *mcss) {
    mcss->pause |= 2;
}

void MCSS_ResumeAnimation(MCSS *mcss) {
    mcss->pause &= ~1;
}

void func_0201ad4c(MCSS *mcss) {
    mcss->pause &= ~2;
}

BOOL func_0201ad70(MCSS *mcss) {
    return mcss->hidden;
}

void MCSS_Hide(MCSS *mcss) {
    mcss->hidden = TRUE;
}

void MCSS_Show(MCSS *mcss) {
    mcss->hidden = FALSE;
}

void func_0201ada0(MCSS *mcss) {
    mcss->hidden ^= 1;
}

NNSG2dMultiCellAnimation *func_0201adc4(MCSS *mcss) {
    return &mcss->mcAnim;
}

u16 func_0201adc8(MCSS *mcss) {
    return mcss->mcAnimBank->numSequences;
}

u8 func_0201add0(MCSS *mcss) {
    return mcss->binTail->unk0;
}

u16 func_0201add8(MCSS *mcss, u8 index) {
    return mcss->binTail->unk2[index];
}

u16 func_0201ade0(MCSS *mcss) {
    return mcss->bin->unk4;
}

u16 func_0201ade8(MCSS *mcss) {
    return mcss->bin->unk6;
}

s16 func_0201adf0(MCSS *mcss) {
    return mcss->bin->unk8;
}

s16 func_0201adf8(MCSS *mcss) {
    return mcss->bin->unkA;
}

u8 func_0201ae00(MCSS *mcss) {
    return mcss->binTail->unk1;
}

void MCSS_SetAnimationEndCallback(MCSS *mcss, u32 param, MCSSAnimationCallback callback, u32 unused) {
    NNS_G2dSetAnimCtrlCallBackFunctor(&mcss->mcAnim.animCtrl, 1, param, callback);
}

void func_0201ae1c(MCSS *mcss, u32 param,
                   BOOL (*func)(u32 param, const NNSG2dMultiCellHierarchyData *node, NNSG2dCellAnimation *cellAnim,
                                u16 nodeIdx)) {
    NNS_G2dTraverseMCNodes(&mcss->mcAnim.multiCellInstance, func, param);
}

void func_0201ae2c(MCSS *mcss, u8 startLevel, u8 endLevel, s32 wait, u32 color) {
    mcss->fading = TRUE;
    mcss->fadeLevel = startLevel;
    mcss->fadeEnd = endLevel;
    mcss->fadeColor = color;
    mcss->fadeWaitCount = 0;
    if (wait < 0) {
        mcss->fadeWait = 0;
        mcss->fadeStep = -(wait - 1);
    } else {
        mcss->fadeWait = wait;
        mcss->fadeStep = 1;
    }
    if (startLevel > endLevel) {
        mcss->fadeStep *= -1;
    }
}

u8 func_0201ae88(MCSS *mcss) {
    return mcss->alpha;
}

void MCSS_SetAlpha(MCSS *mcss, u8 alpha) {
    mcss->alpha = alpha;
}

void func_0201aeb0(MCSS *mcss, u32 a1) {
    mcss->shadowAlpha = a1;
}

void func_0201aecc(MCSS *mcss, u32 a1) {
    mcss->noShadow = a1;
}

BOOL func_0201aee8(MCSS *mcss) {
    if (mcss->fading) {
        return TRUE;
    }
    return FALSE;
}

void func_0201aefc(MCSSSystem *system, u32 characterOffset) {
    system->texBase = characterOffset;
}

void func_0201af00(MCSSSystem *system, u32 paletteOffset) {
    system->plttBase = paletteOffset;
}

void MCSS_SetAnimation(MCSS *mcss, u32 animation) {
    const NNSG2dAnimSequence *seq = NNS_G2dGetAnimSequenceByIdx(mcss->mcAnimBank, animation);

    if (mcss->mcWork != NULL) {
        GFL_HeapFree(mcss->mcWork);
    }
    MCSS_InitMCAnime(mcss, NNS_G2D_MCTYPE_SHARE_CELLANIM);
    NNS_G2dSetAnimSequenceToMCAnimation(&mcss->mcAnim, seq);
    mcss->restart = FALSE;
}

u8 func_0201af44(MCSS *mcss) {
    return mcss->mosaic;
}

// Sets the size of the sprite's mosaic and loads its graphics again
void func_0201af54(MCSSSystem *system, MCSS *mcss, int level) {
    if (mcss->mosaic != level) {
        if (level > MCSS_MOSAIC_MAX) {
            level = MCSS_MOSAIC_MAX;
        }
        mcss->mosaic = level;
        MCSS_FreeFiles(mcss);
        LoadMCSSGraphicsData(system, mcss->index, &mcss->info);
    }
}

void func_0201af9c(MCSSSystem *system, u32 offset) {
    system->offsetScale = offset;
}

void func_0201afa0(MCSS *mcss, fx32 speed) {
    mcss->animSpeed = speed;
}

u16 func_0201afa8(MCSS *mcss) {
    return mcss->mcAnimBank->numSequences;
}

// Blends the sprite's colors with a color, for the fade to start from
void func_0201afb0(MCSSSystem *system, MCSS *mcss, u8 evy, u32 color) {
    MCSSLoad *load = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(MCSSLoad), TRUE, "mcss.c", 1891);

    load->system = system;
    load->inTcbMgr = system->tcbMgr != NULL;
    load->mcss = mcss;
    load->plttProxy = &mcss->plttProxy;
    load->plttAddr = MCSS_PaletteAddress(system, mcss);
    load->pltt = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(NNSG2dPaletteData), FALSE, "mcss.c", 1899);
    load->pltt->fmt = mcss->plttProxy.fmt;
    load->pltt->bExtendedPlt = FALSE;
    load->pltt->szByte = mcss->plttSize;
    load->pltt->pRawData = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), mcss->plttSize, FALSE, "mcss.c", 1904);
    BlendColors(mcss->pltt, load->pltt->pRawData, mcss->plttSize / 2, evy, color);
    MI_CpuCopy16(load->pltt->pRawData, mcss->blendedPltt, mcss->plttSize);
    if (mcss->blendTcb != NULL) {
        func_0201bb14(mcss->blendTcb);
        mcss->blendTcb = NULL;
    }
    if (mcss->fading == FALSE) {
        if (system->tcbMgr != NULL) {
            mcss->blendTcb = GFL_TCBMgrAddTask(system->tcbMgr, MCSS_PaletteTask, load, 0);
        } else {
            mcss->blendTcb = GFL_VBlankTCBAdd(MCSS_PaletteTask, load, 0);
        }
    } else {
        GFL_HeapFree(load->pltt->pRawData);
        GFL_HeapFree(load->pltt);
        GFL_HeapFree(load);
    }
    mcss->blended = TRUE;
}

// Puts the sprite's own colors back
void func_0201b128(MCSSSystem *system, MCSS *mcss) {
    MCSSLoad *load = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(MCSSLoad), TRUE, "mcss.c", 1944);

    load->system = system;
    load->inTcbMgr = system->tcbMgr != NULL;
    load->mcss = mcss;
    load->plttProxy = &mcss->plttProxy;
    load->plttAddr = MCSS_PaletteAddress(system, mcss);
    load->pltt = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(NNSG2dPaletteData), FALSE, "mcss.c", 1952);
    load->pltt->fmt = mcss->plttProxy.fmt;
    load->pltt->bExtendedPlt = FALSE;
    load->pltt->szByte = mcss->plttSize;
    load->pltt->pRawData = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), mcss->plttSize, FALSE, "mcss.c", 1957);
    MI_CpuCopy16(mcss->pltt, load->pltt->pRawData, mcss->plttSize);
    if (mcss->blendTcb != NULL) {
        func_0201bb14(mcss->blendTcb);
        mcss->blendTcb = NULL;
    }
    if (system->tcbMgr != NULL) {
        mcss->blendTcb = GFL_TCBMgrAddTask(system->tcbMgr, MCSS_PaletteTask, load, 0);
    } else {
        mcss->blendTcb = GFL_VBlankTCBAdd(MCSS_PaletteTask, load, 0);
    }
    mcss->blended = FALSE;
}

void MCSS_RestartAnimation(MCSS *mcss) {
    NNS_G2dRestartMCAnimation(&mcss->mcAnim);
}

void func_0201b25c(MCSS *mcss) {
    mcss->restart = TRUE;
}

void func_0201b270(MCSS *mcss, BOOL flag) {
    mcss->reverse = flag;
}

void func_0201b28c(MCSSSystem *system, u32 value) {
    system->cellDepth = value;
}

void func_0201b290(MCSSSystem *system, BOOL flag) {
    system->orthoNear = flag;
}

// Reads a sprite's files and starts the load of its characters and palette
static void LoadMCSSGraphicsData(MCSSSystem *system, int index, const MCSSLoadInfo *info) {
    MCSS *mcss = system->mcss[index];
    MCSSLoad *load;
    HeapID heapId = info->unk20 == 1 ? HEAPID_TAIL((HeapID)mcss->heapId) : mcss->heapId;

    if (system->arc != NULL && system->arcId == info->arcId) {
        LoadMCSSGraphicsDataArc(system, index, info);
        return;
    }

    mcss->loaded = FALSE;
    NNS_G2dInitImageProxy(&mcss->imageProxy);
    NNS_G2dInitImagePaletteProxy(&mcss->plttProxy);
    mcss->cellFile = GFL_G2DIOReadNCER(info->arcId, info->cells, FALSE, &mcss->cellBank, heapId);
    mcss->animFile = GFL_G2DIOReadNANR(info->arcId, info->cellAnime, TRUE, &mcss->animBank, heapId);
    mcss->mcFile = GFL_G2DIOReadNMCR(info->arcId, info->multiCells, FALSE, &mcss->mcBank, heapId);
    mcss->mcAnimFile = GFL_G2DIOReadNMAR(info->arcId, info->multiCellAnime, FALSE, &mcss->mcAnimBank, heapId);
    MCSS_SetAnimation(mcss, 0);
    mcss->bin = GFL_ArcSysReadHeapNew(info->arcId, info->bin, heapId);
    mcss->binTail = (MCSSBinTail *)((u8 *)mcss->bin + ((sizeof(MCSSBin) + mcss->bin->count * sizeof(MCSSCell)) & ~3));

    load = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(MCSSLoad), TRUE, "mcss.c", 2114);
    load->system = system;
    load->inTcbMgr = system->tcbMgr != NULL;
    load->imageProxy = &mcss->imageProxy;
    load->plttProxy = &mcss->plttProxy;
    load->charAddr = system->texBase + index * MCSS_TEX_SIZE;
    load->plttAddr = system->plttBase + index * MCSS_PLTT_SIZE;
    load->mcss = mcss;
    load->charFile =
        GFL_G2DIOReadBGNCGR(info->arcId, info->character, TRUE, &load->chars, HEAPID_TAIL((HeapID)mcss->heapId));
    load->plttFile = GFL_G2DIOReadNCLR(info->arcId, info->palette, &load->pltt, HEAPID_TAIL((HeapID)mcss->heapId));
    mcss->pltt = GFL_HeapAllocate(heapId, load->pltt->szByte, FALSE, "mcss.c", 2138);
    mcss->blendedPltt = GFL_HeapAllocate(heapId, load->pltt->szByte, FALSE, "mcss.c", 2139);
    mcss->plttSize = load->pltt->szByte;
    MI_CpuCopy16(load->pltt->pRawData, mcss->pltt, load->pltt->szByte);
    if (system->loadCallback != NULL) {
        if (system->loadCallback(info, load, system->loadCallbackWork) == TRUE) {
            system->loadCallback = NULL;
            system->loadCallbackWork = 0;
        }
    }
    if (mcss->mosaic != 0) {
        MCSS_MakeMosaic(mcss, load);
    }
    if (system->tcbMgr != NULL) {
        mcss->loadTcb = GFL_TCBMgrAddTask(system->tcbMgr, MCSS_LoadTask, load, 0);
    } else {
        mcss->loadTcb = GFL_VBlankTCBAdd(MCSS_LoadTask, load, 0);
    }
#ifdef BWBATTLE
    if (mcss->trainerBack != BW_TRAINER_BACK_NONE) {
        MCSS_SetTrainerBackSprite(mcss, mcss->trainerBack);
    }
#endif
}

// LoadMCSSGraphicsData from the archive that the system has open
static void LoadMCSSGraphicsDataArc(MCSSSystem *system, int index, const MCSSLoadInfo *info) {
    MCSS *mcss = system->mcss[index];
    ArcTool *arc = system->arc;
    MCSSLoad *load;

    mcss->loaded = FALSE;
    NNS_G2dInitImageProxy(&mcss->imageProxy);
    NNS_G2dInitImagePaletteProxy(&mcss->plttProxy);
    mcss->cellFile = GFL_G2DIOReadNCERArc(arc, info->cells, FALSE, &mcss->cellBank, mcss->heapId);
    mcss->animFile = GFL_G2DIOReadNANRArc(arc, info->cellAnime, TRUE, &mcss->animBank, mcss->heapId);
    mcss->mcFile = GFL_G2DIOReadNMCRArc(arc, info->multiCells, FALSE, &mcss->mcBank, mcss->heapId);
    mcss->mcAnimFile = GFL_G2DIOReadNMARArc(arc, info->multiCellAnime, FALSE, &mcss->mcAnimBank, mcss->heapId);
    MCSS_SetAnimation(mcss, 0);
    mcss->bin = GFL_ArcToolReadHeapNew(arc, info->bin, mcss->heapId);
    mcss->binTail = (MCSSBinTail *)((u8 *)mcss->bin + ((sizeof(MCSSBin) + mcss->bin->count * sizeof(MCSSCell)) & ~3));

    load = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(MCSSLoad), TRUE, "mcss.c", 2221);
    load->system = system;
    load->inTcbMgr = system->tcbMgr != NULL;
    load->imageProxy = &mcss->imageProxy;
    load->plttProxy = &mcss->plttProxy;
    load->charAddr = system->texBase + index * MCSS_TEX_SIZE;
    load->plttAddr = system->plttBase + index * MCSS_PLTT_SIZE;
    load->mcss = mcss;
    load->charFile =
        GFL_G2DIOReadBGNCGRArc(arc, info->character, TRUE, &load->chars, HEAPID_TAIL((HeapID)mcss->heapId));
    load->plttFile = GFL_G2DIOReadNCLRArc(arc, info->palette, &load->pltt, HEAPID_TAIL((HeapID)mcss->heapId));
    mcss->pltt = GFL_HeapAllocate(mcss->heapId, load->pltt->szByte, FALSE, "mcss.c", 2245);
    mcss->blendedPltt = GFL_HeapAllocate(mcss->heapId, load->pltt->szByte, FALSE, "mcss.c", 2246);
    mcss->plttSize = load->pltt->szByte;
    MI_CpuCopy16(load->pltt->pRawData, mcss->pltt, load->pltt->szByte);
    if (system->loadCallback != NULL) {
        if (system->loadCallback(info, load, system->loadCallbackWork) == TRUE) {
            system->loadCallback = NULL;
            system->loadCallbackWork = 0;
        }
    }
    if (mcss->mosaic != 0) {
        MCSS_MakeMosaic(mcss, load);
    }
    if (system->tcbMgr != NULL) {
        mcss->loadTcb = GFL_TCBMgrAddTask(system->tcbMgr, MCSS_LoadTask, load, 0);
    } else {
        mcss->loadTcb = GFL_VBlankTCBAdd(MCSS_LoadTask, load, 0);
    }
#ifdef BWBATTLE
    if (mcss->trainerBack != BW_TRAINER_BACK_NONE) {
        MCSS_SetTrainerBackSprite(mcss, mcss->trainerBack);
    }
#endif
}

// Loads a sprite's characters and palette to VRAM. With a mosaic the characters go a frame before the palette
static void MCSS_LoadTask(TCB *tcb, void *data) {
    MCSSLoad *load = data;

    // bwbattle: the port has no scanlines, and GX_GetVCount never says it is in BW2's loading window. Its V-blank
    // tasks all run while nothing is drawn
#ifndef BWBATTLE
    if (load->inTcbMgr == FALSE) {
        u32 line = GX_GetVCount();

        if (line < MCSS_LOAD_LINE_MIN || line > MCSS_LOAD_LINE_MAX) {
            return;
        }
    }
#endif
    if (load->charFile != NULL) {
        NNS_G2dLoadImage2DMapping(load->chars, load->charAddr, NNS_G2D_VRAM_TYPE_3DMAIN, load->imageProxy);
        GFL_HeapFree(load->charFile);
        if (load->mcss->mosaic == 0) {
            load->charFile = NULL;
            return;
        }
    }
    if (load->mcss != NULL) {
        load->mcss->loaded = TRUE;
    }
    if (load->plttFile != NULL) {
        NNS_G2dLoadPalette(load->pltt, load->plttAddr, NNS_G2D_VRAM_TYPE_3DMAIN, load->plttProxy);
        GFL_HeapFree(load->plttFile);
    }
    if (load->mcss != NULL) {
        load->mcss->loadTcb = NULL;
    }
    if (load->system != NULL && load->system->shadowTcb != NULL) {
        load->system->shadowTcb = NULL;
    }
    GFL_HeapFree(load);
    GFL_TCBRemove(tcb);
}

// Loads a palette that the system made to VRAM
static void MCSS_PaletteTask(TCB *tcb, void *data) {
    MCSSLoad *load = data;

    // bwbattle: the port has no scanlines, and GX_GetVCount never says it is in BW2's loading window. Its V-blank
    // tasks all run while nothing is drawn
#ifndef BWBATTLE
    if (load->inTcbMgr == FALSE) {
        u32 line = GX_GetVCount();

        if (line < MCSS_LOAD_LINE_MIN || line > MCSS_LOAD_LINE_MAX) {
            return;
        }
    }
#endif
    NNS_G2dInitImagePaletteProxy(load->plttProxy);
    NNS_G2dLoadPalette(load->pltt, load->plttAddr, NNS_G2D_VRAM_TYPE_3DMAIN, load->plttProxy);
    if (load->mcss != NULL) {
        if (load->mcss->fadeTcb == tcb) {
            load->mcss->fadeTcb = NULL;
        }
        if (load->mcss->blendTcb == tcb) {
            load->mcss->blendTcb = NULL;
        }
    }
    GFL_HeapFree(load->pltt->pRawData);
    GFL_HeapFree(load->pltt);
    GFL_HeapFree(load);
    GFL_TCBRemove(tcb);
}

static void MCSS_InitMCAnime(MCSS *mcss, u32 mcType) {
    u32 size = NNS_G2dGetMCWorkAreaSize(mcss->mcBank, mcType);

    mcss->mcWork = GFL_HeapAllocate(mcss->heapId, size, FALSE, "mcss.c", 2418);
    NNS_G2dInitMCAnimationInstance(&mcss->mcAnim, mcss->mcWork, mcss->animBank, mcss->cellBank, mcss->mcBank, mcType);
}

static void MCSS_SetMaterial(void) {
    G3_MaterialColorDiffAmb(GX_RGB(31, 31, 31), GX_RGB(16, 16, 16), TRUE);
    G3_MaterialColorSpecEmi(GX_RGB(16, 16, 16), GX_RGB(0, 0, 0), FALSE);
}

// A step of a sprite's palette fade
static void MCSS_UpdatePaletteFade(MCSSSystem *system, MCSS *mcss) {
    if (mcss->fadeWaitCount == 0) {
        MCSSLoad *load = GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(MCSSLoad), TRUE, "mcss.c", 2467);

        load->system = system;
        load->inTcbMgr = system->tcbMgr != NULL;
        load->mcss = mcss;
        load->plttProxy = &mcss->plttProxy;
        load->plttAddr = MCSS_PaletteAddress(system, mcss);
        load->pltt =
            GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), sizeof(NNSG2dPaletteData), FALSE, "mcss.c", 2475);
        load->pltt->fmt = mcss->plttProxy.fmt;
        load->pltt->bExtendedPlt = FALSE;
        load->pltt->szByte = mcss->plttSize;
        load->pltt->pRawData =
            GFL_HeapAllocate(HEAPID_TAIL((HeapID)mcss->heapId), mcss->plttSize, FALSE, "mcss.c", 2480);
        if (mcss->blended) {
            BlendColors(mcss->blendedPltt, load->pltt->pRawData, mcss->plttSize / 2, mcss->fadeLevel, mcss->fadeColor);
        } else {
            BlendColors(mcss->pltt, load->pltt->pRawData, mcss->plttSize / 2, mcss->fadeLevel, mcss->fadeColor);
        }
        if (mcss->fadeTcb != NULL) {
            func_0201bb14(mcss->fadeTcb);
            mcss->fadeTcb = NULL;
        }
        if (system->tcbMgr != NULL) {
            mcss->fadeTcb = GFL_TCBMgrAddTask(system->tcbMgr, MCSS_PaletteTask, load, 0);
        } else {
            mcss->fadeTcb = GFL_VBlankTCBAdd(MCSS_PaletteTask, load, 0);
        }
        if (mcss->fadeLevel == mcss->fadeEnd) {
            mcss->fading = FALSE;
        } else {
            mcss->fadeLevel += mcss->fadeStep;
            if ((mcss->fadeStep >= 0 && mcss->fadeLevel >= mcss->fadeEnd) ||
                (mcss->fadeStep < 0 && mcss->fadeLevel <= mcss->fadeEnd)) {
                mcss->fadeLevel = mcss->fadeEnd;
            }
        }
        mcss->fadeWaitCount = mcss->fadeWait;
    } else {
        mcss->fadeWaitCount--;
    }
}

// Frees a sprite's files and stops its load
static void MCSS_FreeFiles(MCSS *mcss) {
    GFL_HeapFree(mcss->cellFile);
    GFL_HeapFree(mcss->animFile);
    GFL_HeapFree(mcss->mcFile);
    GFL_HeapFree(mcss->mcAnimFile);
    GFL_HeapFree(mcss->mcWork);
    GFL_HeapFree(mcss->bin);
    GFL_HeapFree(mcss->pltt);
    GFL_HeapFree(mcss->blendedPltt);
#ifdef BWBATTLE
    if (mcss->image != NULL) {
        GFL_HeapFree(mcss->image);
        mcss->image = NULL;
    }
#endif
    mcss->cellFile = NULL;
    mcss->animFile = NULL;
    mcss->mcFile = NULL;
    mcss->mcAnimFile = NULL;
    mcss->mcWork = NULL;
    mcss->bin = NULL;
    mcss->pltt = NULL;
    mcss->blendedPltt = NULL;
    if (mcss->loadTcb != NULL) {
        func_0201bae8(mcss->loadTcb);
        mcss->loadTcb = NULL;
    }
}

// Removes a task of MCSS_LoadTask
static void func_0201bae8(TCB *tcb) {
    MCSSLoad *load = GFL_TCBGetData(tcb);

    if (load->charFile != NULL) {
        GFL_HeapFree(load->charFile);
    }
    if (load->plttFile != NULL) {
        GFL_HeapFree(load->plttFile);
    }
    GFL_HeapFree(load);
    GFL_TCBRemove(tcb);
}

// Removes a task of MCSS_PaletteTask
static void func_0201bb14(TCB *tcb) {
    MCSSLoad *load = GFL_TCBGetData(tcb);

    GFL_HeapFree(load->pltt->pRawData);
    GFL_HeapFree(load->pltt);
    GFL_HeapFree(load);
    GFL_TCBRemove(tcb);
}

// Makes a mosaic of a sprite's characters: fills each block of the mosaic's size in each quad with the first color
// found in it
static void MCSS_MakeMosaic(MCSS *mcss, MCSSLoad *load) {
    if (mcss->imageProxy.attr.fmt == GX_TEXFMT_PLTT16) {
        u32 i;
        int texWidth = pow(2, mcss->imageProxy.attr.sizeS + 2);
        u8 *chars = load->chars->pRawData;

        for (i = 0; i < mcss->bin->count; i++) {
            {
                int s = mcss->bin->cells[i].quad.s >> FX32_SHIFT;
                int t = mcss->bin->cells[i].quad.t >> FX32_SHIFT;
                int width = mcss->bin->cells[i].quad.width >> FX32_SHIFT;
                int height = mcss->bin->cells[i].quad.height >> FX32_SHIFT;
                int size = mcss->mosaic + 1;
                u8 color;
                int x;
                int y;
                int blockY;
                int blockX;

                blockY = 0;
                for (y = 0; y < height; y += size) {
                    blockX = 0;
                    for (x = 0; x < width; x += size) {
                        int blockWidth = x + size > width ? width - size * blockX : size;
                        int blockHeight = y + size > height ? height - size * blockY : size;
                        int found = 0;
                        int j;
                        int k;

                        // BUG: The pixel's half of the byte is taken from the block's column, not the image's, which
                        // differ where a quad starts at an odd pixel
                        for (k = 0; k < blockHeight; k++) {
                            for (j = 0; j < blockWidth; j++) {
                                int pixel = (j + (s + x)) / 2 + texWidth * (k + (t + y));
                                int nibble;

                                if (j & 1) {
                                    nibble = (chars[pixel] & 0xf0) >> 4;
                                } else {
                                    nibble = chars[pixel] & 0x0f;
                                }
                                if (nibble != 0) {
                                    found = nibble;
                                    break;
                                }
                            }
                            if (found != 0) {
                                break;
                            }
                        }
                        color = found;
                        for (k = 0; k < blockHeight; k++) {
                            for (j = 0; j < blockWidth; j++) {
                                int column = s + x + j;
                                int pixel = column / 2 + (t + y + k) * texWidth;

                                if (pixel < MCSS_TEX_SIZE) {
                                    if (column & 1) {
                                        chars[pixel] = (chars[pixel] & 0x0f) | (color << 4);
                                    } else {
                                        chars[pixel] = (chars[pixel] & 0xf0) | color;
                                    }
                                }
                            }
                        }
                        blockX++;
                    }
                    blockY++;
                }
            }
            if (mcss->bin->cells[i].quad2.width != 0) {
                int s = mcss->bin->cells[i].quad2.s >> FX32_SHIFT;
                int t = mcss->bin->cells[i].quad2.t >> FX32_SHIFT;
                int width = mcss->bin->cells[i].quad2.width >> FX32_SHIFT;
                int height = (mcss->bin->cells[i].quad2.height >> FX32_SHIFT) * 2;
                int size = mcss->mosaic + 1;
                u8 color;
                int x;
                int y;
                int blockY;
                int blockX;

                blockY = 0;
                for (y = 0; y < height; y += size) {
                    blockX = 0;
                    for (x = 0; x < width; x += size) {
                        int blockWidth = x + size > width ? width - size * blockX : size;
                        int blockHeight = y + size > height ? height - size * blockY : size;
                        int found = 0;
                        int j;
                        int k;

                        // BUG: The pixel's half of the byte is taken from the block's column, not the image's, which
                        // differ where a quad starts at an odd pixel
                        for (k = 0; k < blockHeight; k++) {
                            for (j = 0; j < blockWidth; j++) {
                                int pixel = (j + (s + x)) / 2 + texWidth * (k + (t + y));
                                int nibble;

                                if (j & 1) {
                                    nibble = (chars[pixel] & 0xf0) >> 4;
                                } else {
                                    nibble = chars[pixel] & 0x0f;
                                }
                                if (nibble != 0) {
                                    found = nibble;
                                    break;
                                }
                            }
                            if (found != 0) {
                                break;
                            }
                        }
                        color = found;
                        for (k = 0; k < blockHeight; k++) {
                            for (j = 0; j < blockWidth; j++) {
                                int column = s + x + j;
                                int pixel = column / 2 + (t + y + k) * texWidth;

                                if (pixel < MCSS_TEX_SIZE) {
                                    if (column & 1) {
                                        chars[pixel] = (chars[pixel] & 0x0f) | (color << 4);
                                    } else {
                                        chars[pixel] = (chars[pixel] & 0xf0) | color;
                                    }
                                }
                            }
                        }
                        blockX++;
                    }
                    blockY++;
                }
            }
        }
    }
}

// A vector times a 4x4 matrix, with the w that the result is to be divided by
static void MCSS_MulVecMtx44(const VecFx32 *vec, const MtxFx44 *mtx, VecFx32 *dest, fx32 *w) {
    fx64 y = vec->y;
    fx64 x = vec->x;
    fx64 z = vec->z;

    dest->x = (fx32)((x * mtx->m[0][0] + y * mtx->m[1][0] + z * mtx->m[2][0]) >> FX32_SHIFT);
    dest->x += mtx->m[3][0];
    dest->y = (fx32)((x * mtx->m[0][1] + y * mtx->m[1][1] + z * mtx->m[2][1]) >> FX32_SHIFT);
    dest->y += mtx->m[3][1];
    dest->z = (fx32)((x * mtx->m[0][2] + y * mtx->m[1][2] + z * mtx->m[2][2]) >> FX32_SHIFT);
    dest->z += mtx->m[3][2];
    *w = (fx32)((x * mtx->m[0][3] + y * mtx->m[1][3] + z * mtx->m[2][3]) >> FX32_SHIFT);
    *w += mtx->m[3][3];
}
