#include "types.h"
#include "gfl/areaman.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// The guard word after the bitmap
#define AREAMAN_MAGIC 0x573c765a

struct AreaMan {
    u16 blocks;
    u16 bytes;
    u32 *magic;
    BOOL debugPrint;
    // A bit per block, the first block in the highest bit of the first byte, 1 when used
    u8 bits[];
};

static u32 GFL_AreaManAllocHeadOptUntil8(AreaMan *man, u32 start, u32 count, u32 size);
static u32 GFL_AreaManAllocHeadOptOver8(AreaMan *man, u32 start, u32 count, u32 size);
static u32 GFL_AreaManAllocTailOptUntil8(AreaMan *man, u32 start, u32 count, u32 size);
static u32 GFL_AreaManAllocTailOptOver8(AreaMan *man, u32 start, u32 count, u32 size);
static u32 GFL_AreaManGetAllocatableBitIndexHead(u8 bits, u32 bit, u32 size);
static u32 GFL_AreaManGetAllocatableBitIndexTail(u8 bits, u32 bit, u32 size);
static void GFL_AreaManFreeBitsOptOneByte(u8 *bits, u32 bit, u32 size);
static BOOL GFL_AreaManSetBitsCore(AreaMan *man, int pos, u32 size);
static void AreaManPrintByte(u8 bits);
static void AreaManPrintBits(AreaMan *man);
static void AreaManPrintAlloc(AreaMan *man, u32 pos, u32 size, u32 line);

// The first n bits from the top of a byte, and the last n
static const u8 sBitCountMasks[9] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
static const u8 sActiveBits[9] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

// Of each byte, the free blocks at its start, the free blocks at its end, and the longest run of free blocks
// clang-format off
static const u8 sEightMinusLastOneBit[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 sFirstOneBit[256] = {
    8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
};

// clang-format on

// clang-format off
static const u8 sMaxContinuousZeroBits[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    5, 4, 3, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2,
    4, 3, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2,
    6, 5, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2,
    4, 3, 2, 2, 2, 1, 1, 1, 3, 2, 1, 1, 2, 1, 1, 1,
    5, 4, 3, 3, 2, 2, 2, 2, 3, 2, 1, 1, 2, 1, 1, 1,
    4, 3, 2, 2, 2, 1, 1, 1, 3, 2, 1, 1, 2, 1, 1, 1,
    7, 6, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 3, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2,
    5, 4, 3, 3, 2, 2, 2, 2, 3, 2, 1, 1, 2, 1, 1, 1,
    4, 3, 2, 2, 2, 1, 1, 1, 3, 2, 1, 1, 2, 1, 1, 1,
    6, 5, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2,
    4, 3, 2, 2, 2, 1, 1, 1, 3, 2, 1, 1, 2, 1, 1, 1,
    5, 4, 3, 3, 2, 2, 2, 2, 3, 2, 1, 1, 2, 1, 1, 1,
    4, 3, 2, 2, 2, 1, 1, 1, 3, 2, 1, 1, 2, 1, 1, 0,
};

// clang-format on

AreaMan *GFL_AreaManCreate(u32 blocks, HeapID heapId) {
    AreaMan *man;
    u32 bytes = blocks / 8 + ((blocks % 8) ? 1 : 0);
    u16 size;

    while (bytes % 4 != 0) {
        bytes++;
    }
    size = bytes;
    man = GFL_HeapAllocate(heapId, sizeof(AreaMan) + size + sizeof(u32), FALSE, "areaman.c", 148);
    man->blocks = blocks;
    man->bytes = size;
    man->magic = (u32 *)(man->bits + size);
    *man->magic = AREAMAN_MAGIC;
    man->debugPrint = FALSE;
    MI_CpuFill8(man->bits, 0, size);
    return man;
}

void GFL_AreaManFree(AreaMan *man) {
    GFL_HeapFree(man);
}

u32 GFL_AreaManAllocDefault(AreaMan *man, u32 size) {
    return GFL_AreaManAllocHead(man, 0, man->blocks, size);
}

u32 GFL_AreaManAllocHead(AreaMan *man, u32 start, u32 count, u32 size) {
    if (count < size || start + size > man->blocks) {
        return AREAMAN_FAIL;
    }
    if (size < 8) {
        return GFL_AreaManAllocHeadOptUntil8(man, start, count, size);
    }
    return GFL_AreaManAllocHeadOptOver8(man, start, count, size);
}

static u32 GFL_AreaManAllocHeadOptUntil8(AreaMan *man, u32 start, u32 count, u32 size) {
    u32 pos;
    int last = start + count + 1 - size;
    int byte = start / 8;
    int lastByte = last / 8;
    u32 bit;

    pos = AREAMAN_FAIL;
    bit = start % 8;

    if (bit + size <= 8) {
        u32 index = GFL_AreaManGetAllocatableBitIndexHead(man->bits[byte], bit, size);

        if (index != pos) {
            pos = index + byte * 8;
            goto found;
        }
    }
    for (; byte <= lastByte; byte++) {
        u8 bits = man->bits[byte];

        if (sMaxContinuousZeroBits[bits] >= size) {
            u32 index = GFL_AreaManGetAllocatableBitIndexHead(bits, 0, size);

            if (index != AREAMAN_FAIL) {
                pos = index + byte * 8;
                break;
            }
        } else {
            u8 free = sFirstOneBit[bits];

            // A run across the end of this byte into the start of the next
            if (free != 0 && byte < lastByte && sEightMinusLastOneBit[man->bits[byte + 1]] >= size - free) {
                pos = byte * 8;
                pos += 8 - free;
                break;
            }
        }
    }
found:
    if (pos != AREAMAN_FAIL && (int)pos <= last) {
        if (!GFL_AreaManSetBitsCore(man, pos, size)) {
            return AREAMAN_FAIL;
        }
        AreaManPrintAlloc(man, pos, size, 380);
        return pos;
    }
    return AREAMAN_FAIL;
}

static u32 GFL_AreaManAllocHeadOptOver8(AreaMan *man, u32 start, u32 count, u32 size) {
    int byte = start / 8;
    int lastByte = (start + count - size) / 8;
    int free = 8 - (start % 8);

    if (sFirstOneBit[man->bits[byte]] < free) {
        free = sFirstOneBit[man->bits[byte]];
    }
    while (byte <= lastByte) {
        if (free != 0) {
            int rest = size - free;
            int fullBytes = rest / 8;
            int restBits = rest % 8;
            int i = byte + 1;

            while (fullBytes != 0) {
                if (man->bits[i] != 0) {
                    break;
                }
                i++;
                fullBytes--;
            }
            if (fullBytes == 0) {
                u32 pos = AREAMAN_FAIL;

                if (restBits == 0) {
                    pos = (8 - free) + byte * 8;
                } else if (sEightMinusLastOneBit[man->bits[i]] >= restBits) {
                    pos = (8 - free) + byte * 8;
                }
                if (pos != AREAMAN_FAIL) {
                    if (!GFL_AreaManSetBitsCore(man, pos, size)) {
                        return AREAMAN_FAIL;
                    }
                    AreaManPrintAlloc(man, pos, size, 475);
                    return pos;
                }
            }
        }
        byte++;
        if (byte <= lastByte) {
            free = sFirstOneBit[man->bits[byte]];
        }
    }
    return AREAMAN_FAIL;
}

u32 GFL_AreaManAllocTail(AreaMan *man, u32 start, u32 count, u32 size) {
    u32 pos;

    if (count < size) {
        return AREAMAN_FAIL;
    }
    if (size < 8) {
        pos = GFL_AreaManAllocTailOptUntil8(man, start, count, size);
    } else {
        pos = GFL_AreaManAllocTailOptOver8(man, start, count, size);
    }
    AreaManPrintAlloc(man, pos, size, 560);
    return pos;
}

static u32 GFL_AreaManAllocTailOptUntil8(AreaMan *man, u32 start, u32 count, u32 size) {
    int last = start - count + 1;
    int byte = start / 8;
    int lastByte = last / 8;
    u32 bit = start % 8;
    u32 pos = AREAMAN_FAIL;

    if (bit != 0) {
        u32 index = GFL_AreaManGetAllocatableBitIndexTail(man->bits[byte], bit, size);

        if (index != AREAMAN_FAIL) {
            pos = index + byte * 8;
            goto found;
        }
    }
    for (; byte >= lastByte; byte--) {
        u8 bits = man->bits[byte];

        if (sMaxContinuousZeroBits[bits] >= size) {
            u32 index = GFL_AreaManGetAllocatableBitIndexTail(bits, 7, size);

            if (index != AREAMAN_FAIL) {
                pos = index + byte * 8;
                break;
            }
        } else {
            u8 free = sEightMinusLastOneBit[bits];

            // A run across the start of this byte into the end of the one before
            if (free != 0 && byte > lastByte && sFirstOneBit[man->bits[byte - 1]] >= size - free) {
                pos = (8 - (size - free)) + (byte - 1) * 8;
                break;
            }
        }
    }
found:
    if (pos != AREAMAN_FAIL && (int)pos >= last) {
        if (!GFL_AreaManSetBitsCore(man, pos, size)) {
            pos = AREAMAN_FAIL;
        }
        return pos;
    }
    return AREAMAN_FAIL;
}

static u32 GFL_AreaManAllocTailOptOver8(AreaMan *man, u32 start, u32 count, u32 size) {
    u32 last = start - count + 1;
    int byte = start / 8;
    int lastByte = last / 8;
    int free = (start % 8) + 1;

    if (sEightMinusLastOneBit[man->bits[byte]] < free) {
        free = sEightMinusLastOneBit[man->bits[byte]];
    }
    while (byte >= lastByte) {
        if (free != 0) {
            int rest = size - free;
            int fullBytes = rest / 8;
            int restBits = rest % 8;
            int i = byte - 1;

            while (fullBytes != 0) {
                if (i < lastByte || man->bits[i] != 0) {
                    break;
                }
                i--;
                fullBytes--;
            }
            if (fullBytes == 0) {
                u32 pos = AREAMAN_FAIL;

                if (restBits == 0) {
                    pos = (i + 1) * 8;
                } else if (i >= lastByte && sFirstOneBit[man->bits[i]] >= restBits) {
                    pos = (8 - restBits) + i * 8;
                }
                if (pos != AREAMAN_FAIL && pos >= last) {
                    if (!GFL_AreaManSetBitsCore(man, pos, size)) {
                        pos = AREAMAN_FAIL;
                    }
                    return pos;
                }
            }
        }
        byte--;
        if (byte >= lastByte) {
            free = sEightMinusLastOneBit[man->bits[byte]];
        }
    }
    return AREAMAN_FAIL;
}

BOOL GFL_AreaManSetBits(AreaMan *man, u32 pos, u32 size) {
    if (!GFL_AreaManSetBitsCore(man, pos, size)) {
        return FALSE;
    }
    AreaManPrintAlloc(man, pos, size, 769);
    return TRUE;
}

void GFL_AreaManDeAlloc(AreaMan *man, u32 pos, u32 size) {
    u32 bit = pos % 8;

    pos /= 8;
    if (bit + size <= 8) {
        GFL_AreaManFreeBitsOptOneByte(&man->bits[pos], bit, size);
    } else {
        int first = 8 - bit;
        int rest = size - first;
        int fullBytes = rest / 8;
        int restBits = rest % 8;

        man->bits[pos++] &= ~sActiveBits[first];
        while (fullBytes-- != 0) {
            man->bits[pos++] = 0;
        }
        man->bits[pos] &= ~sBitCountMasks[restBits];
    }
    AreaManPrintBits(man);
}

static u32 GFL_AreaManGetAllocatableBitIndexHead(u8 bits, u32 bit, u32 size) {
    if (size <= 8) {
        u8 mask = sBitCountMasks[size] >> bit;
        u32 end = 8 - size;

        for (; bit <= end; bit++) {
            if (!(bits & mask)) {
                return bit;
            }
            mask >>= 1;
        }
    }
    return AREAMAN_FAIL;
}

static u32 GFL_AreaManGetAllocatableBitIndexTail(u8 bits, u32 bit, u32 size) {
    if (size < 8) {
        int i = bit + 1 - size;
        u8 mask = sBitCountMasks[size] >> i;

        for (; i >= 0; i--) {
            if (!(bits & mask)) {
                return i;
            }
            mask <<= 1;
        }
    }
    return AREAMAN_FAIL;
}

static void GFL_AreaManFreeBitsOptOneByte(u8 *bits, u32 bit, u32 size) {
    *bits &= ~(u8)(sBitCountMasks[size] >> bit);
}

static BOOL GFL_AreaManSetBitsCore(AreaMan *man, int pos, u32 size) {
    int bit;
    int first;
    u32 mask;
    u32 restBits;
    u32 fullBytes;

    if (pos + size > man->blocks) {
        return FALSE;
    }
    bit = pos % 8;
    first = 8 - bit;
    pos /= 8;
    if (first >= size) {
        mask = sBitCountMasks[size] >> bit;
        fullBytes = 0;
        restBits = 0;
    } else {
        mask = sActiveBits[first];
        size -= first;
        fullBytes = size / 8;
        restBits = size % 8;
    }
    if (mask & man->bits[pos]) {
        if (man->debugPrint) {
            AreaManPrintByte(mask);
            AreaManPrintByte(man->bits[pos]);
        }
        return FALSE;
    }
    man->bits[pos++] |= mask;
    while (fullBytes-- != 0) {
        man->bits[pos++] = 0xff;
    }
    if (restBits != 0) {
        man->bits[pos] |= sBitCountMasks[restBits];
    }
    return TRUE;
}

static void AreaManPrintByte(u8 bits) {
}

static void AreaManPrintBits(AreaMan *man) {
    int i;

    if (man->debugPrint) {
        for (i = 0; i < man->bytes; i++) {
            AreaManPrintByte(man->bits[i]);
        }
    }
}

static void AreaManPrintAlloc(AreaMan *man, u32 pos, u32 size, u32 line) {
    if (man->debugPrint) {
        AreaManPrintBits(man);
    }
}
