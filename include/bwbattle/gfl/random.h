#ifndef POKEBW2_GFL_RANDOM_H
#define POKEBW2_GFL_RANDOM_H

#include "types.h"
#include "nitro/math.h"

// The linear congruential random numbers (gfl_use.c). GFL_RandomInit seeds them, and the Mersenne Twister from them
void GFL_RandomInitLC(MATHRandContext32 *context);
void GFL_RandomInit(void);
// A random number below max, or any 32-bit number when max is 0
u32 GFL_RandomLC(u32 max);
// The same without the check for 0, which gives 0
u32 GFL_RandomLCAlt(u32 max);
u32 GFL_RandomMT(void);
// Seeds a random context from the time and the hardware
void buildSeed(MATHRandContext32 *context);

// A random number below range, or any 32-bit number if range is 0
static inline u32 GFL_RandomMTRange(u32 range) {
    u64 value = GFL_RandomMT();

    if (range != 0) {
        value *= range;
        value >>= 32;
    }
    return value;
}

#endif // POKEBW2_GFL_RANDOM_H
