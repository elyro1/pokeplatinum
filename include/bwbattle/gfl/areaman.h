#ifndef POKEBW2_GFL_AREAMAN_H
#define POKEBW2_GFL_AREAMAN_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A bitmap of blocks, such as the characters of a BG, that hands out runs of free blocks

// What the allocators return when no run of blocks is free
#define AREAMAN_FAIL 0xffffffff

AreaMan *GFL_AreaManCreate(u32 blocks, HeapID heapId);
void GFL_AreaManFree(AreaMan *man);
// Finds a free run of size blocks anywhere, searching up from the first block
u32 GFL_AreaManAllocDefault(AreaMan *man, u32 size);
// Finds a free run of size blocks within the count blocks from start, searching up from start or down from it
u32 GFL_AreaManAllocHead(AreaMan *man, u32 start, u32 count, u32 size);
u32 GFL_AreaManAllocTail(AreaMan *man, u32 start, u32 count, u32 size);
// Marks a run of blocks as used, returning whether they were free, or marks them free again
BOOL GFL_AreaManSetBits(AreaMan *man, u32 pos, u32 size);
void GFL_AreaManDeAlloc(AreaMan *man, u32 pos, u32 size);

#endif // POKEBW2_GFL_AREAMAN_H
