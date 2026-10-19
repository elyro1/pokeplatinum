#ifndef POKEBW2_SYSTEM_SIN_WAVE_TABLE_H
#define POKEBW2_SYSTEM_SIN_WAVE_TABLE_H

#include "types.h"
#include "nitro/fx.h"

// A table of a sine wave (sin_wave_table.c, a guessed name)

// Fills count entries with the whole part of amplitude times the sine, the angle starting at 0 and growing by step
// from each entry to the next, in FX_SinIdx's units
void SinWaveTable_Make(s16 *table, u32 count, u16 step, fx32 amplitude);

#endif // POKEBW2_SYSTEM_SIN_WAVE_TABLE_H
