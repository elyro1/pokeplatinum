#include "types.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "nitro/os.h"

// The Mersenne Twister, MT19937
#define MT_N 624
#define MT_M 397
#define MT_MATRIX_A 0x9908b0df
#define MT_UPPER_MASK 0x80000000
#define MT_LOWER_MASK 0x7fffffff
#define MT_DEFAULT_SEED 5489

typedef struct {
    u32 mt[MT_N];
    int mti;
    u32 mag01[2];
    MATHCRC16Table crcTable;
} GFLStdTables;

static GFLStdTables *sGFLStd;

void initTableArea(HeapID heapId) {
    sGFLStd = GFL_HeapAllocate(heapId, sizeof(GFLStdTables), FALSE, "gf_standard.c", 49);
    sGFLStd->mti = MT_N + 1;
    sGFLStd->mag01[0] = 0;
    sGFLStd->mag01[1] = MT_MATRIX_A;
    MATHi_CRC16InitTable(&sGFLStd->crcTable, 0x1021);
}

s32 GFL_STD_MemCmp(const void *a, const void *b, u32 size) {
    const u8 *pa = a;
    const u8 *pb = b;
    u32 n = size;

    while (n-- != 0) {
        if (*pa != *pb) {
            return *pa - *pb;
        }
        pa++;
        pb++;
    }
    return 0;
}

u32 GFL_STD_StrLen(const char *str) {
    u32 len = 0;

    for (;;) {
        if (*(const u8 *)str == '\0') {
            return len;
        }
        str++;
        len++;
    }
}

int _STD_CompareNString(const char *a, const char *b) {
    return STD_CompareString(a, b);
}

void GFL_RandomUpdateMT(u32 seed) {
    sGFLStd->mt[0] = seed;
    for (sGFLStd->mti = 1; sGFLStd->mti < MT_N; sGFLStd->mti++) {
        sGFLStd->mt[sGFLStd->mti] =
            1812433253 * (sGFLStd->mt[sGFLStd->mti - 1] ^ (sGFLStd->mt[sGFLStd->mti - 1] >> 30)) + sGFLStd->mti;
    }
}

u32 GFL_RandomMT(void) {
    u32 y;

    if (sGFLStd->mti >= MT_N) {
        int kk;

        if (sGFLStd->mti == MT_N + 1) {
            GFL_RandomUpdateMT(MT_DEFAULT_SEED);
        }
        for (kk = 0; kk < MT_N - MT_M; kk++) {
            y = (sGFLStd->mt[kk] & MT_UPPER_MASK) | (sGFLStd->mt[kk + 1] & MT_LOWER_MASK);
            sGFLStd->mt[kk] = sGFLStd->mt[kk + MT_M] ^ (y >> 1) ^ sGFLStd->mag01[y & 1];
        }
        for (; kk < MT_N - 1; kk++) {
            y = (sGFLStd->mt[kk] & MT_UPPER_MASK) | (sGFLStd->mt[kk + 1] & MT_LOWER_MASK);
            sGFLStd->mt[kk] = sGFLStd->mt[kk + (MT_M - MT_N)] ^ (y >> 1) ^ sGFLStd->mag01[y & 1];
        }
        y = (sGFLStd->mt[MT_N - 1] & MT_UPPER_MASK) | (sGFLStd->mt[0] & MT_LOWER_MASK);
        sGFLStd->mt[MT_N - 1] = sGFLStd->mt[MT_M - 1] ^ (y >> 1) ^ sGFLStd->mag01[y & 1];
        sGFLStd->mti = 0;
    }
    y = sGFLStd->mt[sGFLStd->mti++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= y >> 18;
    return y;
}

u16 getCRC16(const void *data, u32 size) {
    if (size < 2) {
        u8 bytes[2];
        u8 byte = *(const u8 *)data;

        bytes[0] = byte;
        bytes[1] = byte;
        return MATH_CalcCRC16CCITT(&sGFLStd->crcTable, bytes, 2);
    }
    return MATH_CalcCRC16CCITT(&sGFLStd->crcTable, data, size);
}

void buildSeed(MATHRandContext32 *context) {
    u64 seed;
    u32 entropy[8];
    u8 digest[MATH_SHA1_DIGEST_SIZE];
    MATHSHA1Context sha1;

    OS_GetLowEntropyData(entropy);
    MATH_SHA1Init(&sha1);
    // The digest's buffer is hashed before it holds the digest, for whatever the stack held
    MATH_SHA1Update(&sha1, digest, sizeof(digest));
    MATH_SHA1Update(&sha1, entropy, sizeof(entropy));
    MATH_SHA1GetHash(&sha1, digest);
    MI_CpuCopy8(digest, &seed, sizeof(seed));
    MATH_InitRand32(context, seed);
}

u32 addUpDataStream(const u8 *data, u32 size) {
    u32 sum = 0;
    u32 i;

    for (i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum;
}

void decryptData(void *data, u32 size, u32 seed) {
    u16 *words = data;
    u32 i;

    for (i = 0; i < size / 2; i++) {
        words[i] ^= crypt_SAV_BVideo_MG_misc_data(&seed);
    }
}

void _decryptData(void *data, u32 size, u32 seed) {
    decryptData(data, size, seed);
}

u16 crypt_SAV_BVideo_MG_misc_data(u32 *seed) {
    *seed = *seed * 1103515245 + 24691;
    return (u16)(*seed >> 16);
}
