#include "types.h"
#include "nitro/fx.h"
#include "system/sin_wave_table.h"

// A table of a sine wave. The file's name is a guess: the ROM has no string for it

void SinWaveTable_Make(s16 *table, u32 count, u16 step, fx32 amplitude) {
    u32 i;
    u16 angle = 0;

    for (i = 0; i < count; i++) {
        table[i] = FX_Whole(FX_Mul(FX_SinIdx(angle), amplitude));
        angle += step;
    }
}
