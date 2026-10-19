#ifndef POKEBW2_SYSTEM_HP_GAUGE_H
#define POKEBW2_SYSTEM_HP_GAUGE_H

#include "types.h"

// The HP gauges' size and color. The ROM doesn't name the file that holds these

// The color of the gauge for HP out of a maximum: above half, above a fifth, below, and none left
#define HP_GAUGE_COLOR_GREEN 0
#define HP_GAUGE_COLOR_YELLOW 1
#define HP_GAUGE_COLOR_RED 2
#define HP_GAUGE_COLOR_NONE 3

// The pixels of a gauge width pixels long to fill for HP out of a maximum, at least 1 while any HP is left
u8 HPGauge_GetFill(u32 hp, u32 maxHp, u32 width);
u8 HPGauge_GetColor(u32 hp, u32 maxHp);

#endif // POKEBW2_SYSTEM_HP_GAUGE_H
