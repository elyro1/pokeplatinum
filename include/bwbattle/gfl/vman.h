#ifndef POKEBW2_GFL_VMAN_H
#define POKEBW2_GFL_VMAN_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// VRAM managers (vman.c): an AreaMan over VRAM banks in blocks of characters, for BG or OBJ

enum {
    VMAN_TYPE_BG,
    VMAN_TYPE_OBJ,
};

// An allocation, both -1 when there is none
struct VRAMAlloc {
    u32 pos;
    u32 size;
};

// The bytes of the VRAM banks of a GX_VRAM_* mask
u32 GFL_VRAMManagerCalcBankSizeBytes(u32 banks);
// Manages the banks after offset bytes. OBJ blocks are as large as the OBJ character mapping objMode
// (GXOBJVRamModeChar) can address, BG blocks one character
VRAMMan *GFL_VRAMManagerCreate(HeapID heapId, u32 type, u32 banks, u32 offset, u32 objMode);
void GFL_VRAMManagerFree(VRAMMan *man);
u32 GFL_VRAMManagerGetObjTileMappingBoundary(u32 objMode);
void GFL_VRAMManagerAllocInit(VRAMAlloc *alloc);
BOOL GFL_VRAMManagerAllocIsInvalid(const VRAMAlloc *alloc);
BOOL GFL_VRAMManagerAlloc(VRAMMan *man, u32 size, VRAMAlloc *alloc);
void GFL_VRAMManagerDeAlloc(VRAMMan *man, VRAMAlloc *alloc);
// The offset into VRAM of an allocation
u32 GFL_VRAMManagerGetAllocAddress(const VRAMMan *man, const VRAMAlloc *alloc);

#endif // POKEBW2_GFL_VMAN_H
