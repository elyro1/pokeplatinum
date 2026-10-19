#include "types.h"
#include "gfl/areaman.h"
#include "gfl/heap.h"
#include "gfl/vman.h"
#include "nitro/gx.h"

struct VRAMMan {
    AreaMan *areaMan;
    u16 blockSize;
    u16 offset;
};

typedef struct {
    u32 objMode;
    u32 boundary;
} ObjTileMappingBoundary;

typedef struct {
    u32 bank;
    u32 sizeKB;
} VRAMBankSize;

// clang-format off
static const ObjTileMappingBoundary sObjTileMappingBoundaries[] = {
    { GX_OBJVRAMMODE_CHAR_2D, 0x20 },
    { GX_OBJVRAMMODE_CHAR_1D_32K, 0x20 },
    { GX_OBJVRAMMODE_CHAR_1D_64K, 0x40 },
    { GX_OBJVRAMMODE_CHAR_1D_128K, 0x80 },
    { GX_OBJVRAMMODE_CHAR_1D_256K, 0x100 },
};

// clang-format on

// clang-format off
static const VRAMBankSize sVRAMBankSizes[] = {
    { GX_VRAM_A, 128 },
    { GX_VRAM_B, 128 },
    { GX_VRAM_C, 128 },
    { GX_VRAM_D, 128 },
    { GX_VRAM_E, 64 },
    { GX_VRAM_F, 16 },
    { GX_VRAM_G, 16 },
    { GX_VRAM_H, 32 },
    { GX_VRAM_I, 16 },
};

// clang-format on

u32 GFL_VRAMManagerCalcBankSizeBytes(u32 banks) {
    u32 i;
    u32 sizeKB = 0;

    for (i = 0; i < NELEMS(sVRAMBankSizes); i++) {
        if (sVRAMBankSizes[i].bank & banks) {
            sizeKB += sVRAMBankSizes[i].sizeKB;
        }
    }
    return sizeKB * 1024;
}

VRAMMan *GFL_VRAMManagerCreate(HeapID heapId, u32 type, u32 banks, u32 offset, u32 objMode) {
    VRAMMan *man = GFL_HeapAllocate(heapId, sizeof(VRAMMan), FALSE, "vman.c", 122);
    u32 size = GFL_VRAMManagerCalcBankSizeBytes(banks) - offset;

    if (type == VMAN_TYPE_OBJ) {
        man->blockSize = GFL_VRAMManagerGetObjTileMappingBoundary(objMode);
    } else {
        man->blockSize = 0x20;
    }
    man->areaMan = GFL_AreaManCreate(size / man->blockSize, heapId);
    man->offset = offset;
    return man;
}

void GFL_VRAMManagerFree(VRAMMan *man) {
    GFL_AreaManFree(man->areaMan);
    GFL_HeapFree(man);
}

u32 GFL_VRAMManagerGetObjTileMappingBoundary(u32 objMode) {
    u32 i;

    for (i = 0; i < NELEMS(sObjTileMappingBoundaries); i++) {
        if (objMode == sObjTileMappingBoundaries[i].objMode) {
            return sObjTileMappingBoundaries[i].boundary;
        }
    }
    return 0x20;
}

void GFL_VRAMManagerAllocInit(VRAMAlloc *alloc) {
    alloc->pos = 0xffffffff;
    alloc->size = 0xffffffff;
}

BOOL GFL_VRAMManagerAllocIsInvalid(const VRAMAlloc *alloc) {
    if (alloc->pos == 0xffffffff && alloc->size == 0xffffffff) {
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_VRAMManagerAlloc(VRAMMan *man, u32 size, VRAMAlloc *alloc) {
    u32 blockSize = man->blockSize;
    u32 blocks = size / blockSize;
    u32 pos;

    if (size % blockSize != 0) {
        blocks++;
    }
    pos = GFL_AreaManAllocDefault(man->areaMan, blocks);
    if (pos != AREAMAN_FAIL) {
        alloc->pos = pos;
        alloc->size = blocks;
        return TRUE;
    }
    return FALSE;
}

void GFL_VRAMManagerDeAlloc(VRAMMan *man, VRAMAlloc *alloc) {
    GFL_AreaManDeAlloc(man->areaMan, alloc->pos, alloc->size);
    GFL_VRAMManagerAllocInit(alloc);
}

u32 GFL_VRAMManagerGetAllocAddress(const VRAMMan *man, const VRAMAlloc *alloc) {
    return man->offset + alloc->pos * man->blockSize;
}
