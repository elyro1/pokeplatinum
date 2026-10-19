#include "types.h"
#include "battle/btlv_bg3d.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"

typedef void (*VramLoadFunc)(const void *src, u32 offset, u32 size);

static u32 GFL_BGSysLoadNCGRStaticCore(void *file, u32 bg, u32 offset, u32 size, BOOL compressed);
static u32 GFL_BGSysLoadNCGRDynamicCore(void *file, u32 bg, u32 size);
static u32 unpackAndLoadOBJChar(void *file, u32 engine, u32 offset, u32 size);
static void GFL_BGSysLoadNSCR(void *file, u32 bg, u32 offset, u32 palOffset, u32 size, BOOL async);
static void GFL_BGSysLoadNCLR(void *file, u32 type, u32 srcOffset, u32 offset, u32 size);

static const VramLoadFunc obj_char_lut[] = {
    GX_LoadOBJ,
    GXS_LoadOBJ,
};

static const VramLoadFunc pltt_type_lut[] = {
    GX_LoadBGPltt,  GX_LoadOBJPltt,  GX_LoadBGExtPltt,  GX_LoadOBJExtPltt,
    GXS_LoadBGPltt, GXS_LoadOBJPltt, GXS_LoadBGExtPltt, GXS_LoadOBJExtPltt,
};

u32 GFL_BGSysLoadNCGRStatic(u32 arcId, u32 fileId, u8 bg, u32 offset, u32 size, BOOL compressed, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    return GFL_BGSysLoadNCGRStaticCore(file, bg, offset, size, compressed);
}

u32 GFL_BGSysLoadArcNCGRStatic(ArcTool *arc, u32 fileId, u32 bg, u32 offset, u32 size, BOOL compressed, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    return GFL_BGSysLoadNCGRStaticCore(file, bg, offset, size, compressed);
}

static u32 GFL_BGSysLoadNCGRStaticCore(void *file, u32 bg, u32 offset, u32 size, BOOL compressed) {
    NNSG2dCharacterData *character;

    if (NNS_G2dGetUnpackedBGCharacterData(file, &character)) {
        if (size == 0) {
            size = character->szByte;
        }
        GFL_BGSysLoadChar(bg, character->pRawData, size, offset);
    }
    GFL_HeapFree(file);
    return size;
}

u32 GFL_BGSysLoadNCGRDynamic(u32 arcId, u32 fileId, u32 bg, u32 size, BOOL compressed, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, HEAPID_TAIL(heapId));

    if (file != NULL) {
        u32 chars = GFL_BGSysLoadNCGRDynamicCore(file, bg, size);

        GFL_HeapFree(file);
        return chars;
    }
    return 0;
}

u32 GFL_BGSysLoadArcNCGRDynamic(ArcTool *arc, u32 fileId, u32 bg, u32 size, BOOL compressed, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, HEAPID_TAIL(heapId));

    if (file != NULL) {
        u32 chars = GFL_BGSysLoadNCGRDynamicCore(file, bg, size);

        GFL_HeapFree(file);
        return chars;
    }
    return 0;
}

static u32 GFL_BGSysLoadNCGRDynamicCore(void *file, u32 bg, u32 size) {
    NNSG2dCharacterData *character;

    if (NNS_G2dGetUnpackedBGCharacterData(file, &character)) {
        u32 pos;

        if (size == 0) {
            size = character->szByte;
        }
        pos = GFL_BGSysLoadCharDynamic(bg, character->pRawData, size);
        if (pos != (u32)-1) {
            return ((u16)size << 16) | (u16)pos;
        }
    }
    return 0;
}

u32 loadOBJCharToVram(ArcTool *arc, u32 fileId, u32 engine, u32 offset, u32 size, BOOL compressed, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    return unpackAndLoadOBJChar(file, engine, offset, size);
}

static u32 unpackAndLoadOBJChar(void *file, u32 engine, u32 offset, u32 size) {
    NNSG2dCharacterData *character;

    if (NNS_G2dGetUnpackedCharacterData(file, &character)) {
        if (size == 0) {
            size = character->szByte;
        }
        DC_FlushRange(character->pRawData, size);
        obj_char_lut[engine](character->pRawData, offset, size);
    }
    GFL_HeapFree(file);
    return size;
}

void loadBGScrToVramByNarcNoReserveNegAlign(u32 arcId, u32 fileId, u8 bg, u32 offset, u32 size, BOOL compressed,
                                            HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, HEAPID_TAIL(heapId));

    GFL_BGSysLoadNSCR(file, bg, offset, 0, size, FALSE);
}

void loadBGScrToVramByFileNoReserveNegAlign(ArcTool *arc, u32 fileId, u32 bg, u32 offset, u32 size, BOOL compressed,
                                            HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, HEAPID_TAIL(heapId));

    GFL_BGSysLoadNSCR(file, bg, offset, 0, size, FALSE);
}

void loadBGScrToVramByNarcNoReserve(u32 arcId, u32 fileId, u8 bg, u32 offset, u32 palOffset, u32 size, BOOL compressed,
                                    HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    GFL_BGSysLoadNSCR(file, bg, offset, palOffset, size, FALSE);
}

void GFL_G2DIOLoadNSCRSync(ArcTool *arc, u32 fileId, u8 bg, u32 offset, u32 palOffset, u32 size, BOOL compressed,
                           HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    GFL_BGSysLoadNSCR(file, bg, offset, palOffset, size, FALSE);
}

void GFL_G2DIOLoadNSCRAsync(ArcTool *arc, u32 fileId, u8 bg, u32 offset, u32 palOffset, u32 size, BOOL compressed,
                            HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    GFL_BGSysLoadNSCR(file, bg, offset, palOffset, size, TRUE);
}

// Loads a screen file to a BG's screen, through its screen buffer if it has one, adding palOffset to each entry
static void GFL_BGSysLoadNSCR(void *file, u32 bg, u32 offset, u32 palOffset, u32 size, BOOL async) {
    NNSG2dScreenData *screen;

#ifdef BWBATTLE
    // bwbattle: the effects' background is drawn in the 3D scene (btlv_bg3d.c)
    if (bg == 3 && !GFL_BGSysIsCreated()) {
        if (NNS_G2dGetUnpackedScreenData(file, &screen)) {
            BtlvBg3d_LoadScreen(screen);
        }
        GFL_HeapFree(file);
        return;
    }
#endif
    if (NNS_G2dGetUnpackedScreenData(file, &screen)) {
        if (size == 0) {
            size = screen->szByte;
        }
        if (palOffset != 0) {
            if (GFL_BGSysGetBGColorPaletteMode(bg) == GX_BG_COLORMODE_16) {
                u32 i;
                u16 *entries = (u16 *)screen->rawData;

                for (i = 0; i < size / 2; i++) {
                    entries[i] += (u16)palOffset;
                }
            } else {
                u32 i;
                u8 *entries = (u8 *)screen->rawData;

                for (i = 0; i < size; i++) {
                    entries[i] += (u8)palOffset;
                }
            }
        }
        if (GFL_BGSysIsScrHeapExists(bg)) {
            GFL_BGSysBufferScr(bg, screen->rawData, size, offset);
            if (async) {
                GFL_BGSysQueueScrLoad(bg);
            } else {
                GFL_BGSysLoadScr(bg);
            }
        } else {
            GFL_BGSysLoadScrCore(bg, screen->rawData, size, offset);
        }
    }
    GFL_HeapFree(file);
}

void GFL_BGSysLoadNCLRDefault(u32 arcId, u32 fileId, u32 type, u32 offset, u32 size, HeapID heapId) {
    GFL_G2DIOLoadNCLR(arcId, fileId, type, 0, offset, size, heapId);
}

void GFL_G2DIOLoadArcNCLRDefault(ArcTool *arc, u32 fileId, u32 type, u32 offset, u32 size, HeapID heapId) {
    GFL_G2DIOLoadArcNCLR(arc, fileId, type, 0, offset, size, heapId);
}

void GFL_G2DIOLoadNCLR(u32 arcId, u32 fileId, u32 type, u32 srcOffset, u32 offset, u32 size, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, FALSE, HEAPID_TAIL(heapId));

    GFL_BGSysLoadNCLR(file, type, srcOffset, offset, size);
}

void GFL_G2DIOLoadArcNCLR(ArcTool *arc, u32 fileId, u32 type, u32 srcOffset, u32 offset, u32 size, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, FALSE, HEAPID_TAIL(heapId));

    GFL_BGSysLoadNCLR(file, type, srcOffset, offset, size);
}

// Loads a palette file from srcOffset into it to palette memory of a PALTYPE_*
static void GFL_BGSysLoadNCLR(void *file, u32 type, u32 srcOffset, u32 offset, u32 size) {
    NNSG2dPaletteData *palette;
    NNSG2dPaletteCompressInfo *compressInfo;
    BOOL compressed;

    if (file == NULL) {
        return;
    }
    compressed = NNS_G2dGetUnpackedPaletteCompressInfo(file, &compressInfo);
    if (NNS_G2dGetUnpackedPaletteData(file, &palette)) {
        palette->pRawData = (u8 *)palette->pRawData + srcOffset;
        if (size == 0) {
            if (!compressed) {
                size = palette->szByte;
            } else {
                size = compressInfo->numPalette * (palette->fmt == GX_TEXFMT_PLTT16 ? 0x20 : 0x200);
            }
            size -= srcOffset;
        }
        DC_FlushRange(palette->pRawData, size);
        switch (type) {
        case PALTYPE_MAIN_BG_EX:
            GX_BeginLoadBGExtPltt();
            pltt_type_lut[type](palette->pRawData, offset, size);
            GX_EndLoadBGExtPltt();
            break;
        case PALTYPE_SUB_BG_EX:
            GXS_BeginLoadBGExtPltt();
            pltt_type_lut[type](palette->pRawData, offset, size);
            GXS_EndLoadBGExtPltt();
            break;
        case PALTYPE_MAIN_OBJ_EX:
            GX_BeginLoadOBJExtPltt();
            pltt_type_lut[type](palette->pRawData, offset, size);
            GX_EndLoadOBJExtPltt();
            break;
        case PALTYPE_SUB_OBJ_EX:
            GXS_BeginLoadOBJExtPltt();
            pltt_type_lut[type](palette->pRawData, offset, size);
            GXS_EndLoadOBJExtPltt();
            break;
        default:
            pltt_type_lut[type](palette->pRawData, offset, size);
            break;
        }
    }
    GFL_HeapFree(file);
}

void *GFL_G2DIOReadBGNCGR(u32 arcId, u32 fileId, BOOL compressed, NNSG2dCharacterData **character, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedBGCharacterData(file, character)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadBGNCGRArc(ArcTool *arc, u32 fileId, BOOL compressed, NNSG2dCharacterData **character,
                             HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedBGCharacterData(file, character)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadOBJNCGR(u32 arcId, u32 fileId, BOOL compressed, NNSG2dCharacterData **character, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedCharacterData(file, character)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadOBJNCGRArc(ArcTool *arc, u32 fileId, BOOL compressed, NNSG2dCharacterData **character,
                              HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedCharacterData(file, character)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNSCR(u32 arcId, u32 fileId, BOOL compressed, NNSG2dScreenData **screen, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedScreenData(file, screen)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNSCRArc(ArcTool *arc, u32 fileId, BOOL compressed, NNSG2dScreenData **screen, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedScreenData(file, screen)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNCLR(u32 arcId, u32 fileId, NNSG2dPaletteData **palette, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, FALSE, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedPaletteData(file, palette)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNCLRArc(ArcTool *arc, u32 fileId, NNSG2dPaletteData **palette, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, FALSE, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedPaletteData(file, palette)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNCER(u32 arcId, u32 fileId, BOOL compressed, NNSG2dCellDataBank **cells, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedCellBank(file, cells)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNCERArc(ArcTool *arc, u32 fileId, BOOL compressed, NNSG2dCellDataBank **cells, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedCellBank(file, cells)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNANR(u32 arcId, u32 fileId, BOOL compressed, NNSG2dAnimBankData **anims, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedAnimBank(file, anims)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNANRArc(ArcTool *arc, u32 fileId, BOOL compressed, NNSG2dAnimBankData **anims, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedAnimBank(file, anims)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNMCR(u32 arcId, u32 fileId, BOOL compressed, NNSG2dMultiCellDataBank **cells, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedMultiCellBank(file, cells)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNMCRArc(ArcTool *arc, u32 fileId, BOOL compressed, NNSG2dMultiCellDataBank **cells, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedMultiCellBank(file, cells)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNMAR(u32 arcId, u32 fileId, BOOL compressed, NNSG2dAnimBankData **anims, HeapID heapId) {
    void *file = GFL_ArcSysReadHeapNewLZ(arcId, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedMCAnimBank(file, anims)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

void *GFL_G2DIOReadNMARArc(ArcTool *arc, u32 fileId, BOOL compressed, NNSG2dAnimBankData **anims, HeapID heapId) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, heapId);

    if (file != NULL && !NNS_G2dGetUnpackedMCAnimBank(file, anims)) {
        GFL_HeapFree(file);
        return NULL;
    }
    return file;
}

// LZ compressed files have the uncompressed size in the upper 24 bits of their first word
#define LZ_UNCOMPRESSED_SIZE(data) (*(u32 *)(data) >> 8)

void *GFL_ArcSysReadHeapNewLZ(u32 arcId, u32 fileId, BOOL compressed, HeapID heapId) {
    void *data;

    if (compressed) {
        data = GFL_HeapAllocate(HEAPID_TAIL(heapId), GFL_ArcSysGetDataLength(arcId, fileId), FALSE, "arc_util.c", 1599);
    } else {
        data = GFL_HeapAllocate(heapId, GFL_ArcSysGetDataLength(arcId, fileId), FALSE, "arc_util.c", 1603);
    }
    GFL_ArcSysRead(data, arcId, fileId);
    if (compressed) {
        void *uncompressed = GFL_HeapAllocate(heapId, LZ_UNCOMPRESSED_SIZE(data), FALSE, "arc_util.c", 1611);

        MI_UncompressLZ8(data, uncompressed);
        GFL_HeapFree(data);
        data = uncompressed;
    }
    return data;
}

void *GFL_ArcSysReadHeapNewLZGetLen(u32 arcId, u32 fileId, BOOL compressed, HeapID heapId, u32 *size) {
    void *data;

    *size = GFL_ArcSysGetDataLength(arcId, fileId);
    if (compressed) {
        data = GFL_HeapAllocate(HEAPID_TAIL(heapId), *size, FALSE, "arc_util.c", 1642);
    } else {
        data = GFL_HeapAllocate(heapId, *size, FALSE, "arc_util.c", 1646);
    }
    GFL_ArcSysRead(data, arcId, fileId);
    if (compressed) {
        void *uncompressed;

        *size = LZ_UNCOMPRESSED_SIZE(data);
        uncompressed = GFL_HeapAllocate(heapId, *size, FALSE, "arc_util.c", 1656);
        MI_UncompressLZ8(data, uncompressed);
        GFL_HeapFree(data);
        data = uncompressed;
    }
    return data;
}

void *GFL_ArcToolReadHeapNewLZ(ArcTool *arc, u32 fileId, BOOL compressed, HeapID heapId) {
    u32 size;

    return GFL_ArcToolReadHeapNewLZGetLen(arc, fileId, compressed, heapId, &size);
}

void *GFL_ArcToolReadHeapNewLZGetLen(ArcTool *arc, u32 fileId, BOOL compressed, u32 heapId, u32 *size) {
    void *data;

    *size = GFL_ArcToolGetDataLength(arc, fileId);
    if (compressed) {
        data = GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), *size, FALSE, "arc_util.c", 1709);
    } else {
        data = GFL_HeapAllocate(heapId, *size, FALSE, "arc_util.c", 1713);
    }
    if (data != NULL) {
        GFL_ArcToolRead(arc, fileId, data);
        if (compressed) {
            void *uncompressed;

            *size = LZ_UNCOMPRESSED_SIZE(data);
            uncompressed = GFL_HeapAllocate(heapId, *size, FALSE, "arc_util.c", 1725);
            if (uncompressed != NULL) {
                MI_UncompressLZ8(data, uncompressed);
                GFL_HeapFree(data);
            }
            data = uncompressed;
        }
    }
    return data;
}
