#include "system/hp_gauge.h"
#include "types.h"

// The HP gauges' size and color. The ROM doesn't name the file; hp_gauge.c is a guess. Names are ours

u8 HPGauge_GetFill(u32 hp, u32 maxHp, u32 width) {
    u8 pixels = hp * width / maxHp;

    if (pixels == 0 && hp != 0) {
        pixels = 1;
    }
    return pixels;
}

u8 HPGauge_GetColor(u32 hp, u32 maxHp) {
    // In 1/256ths, so that the fractions keep their precision
    hp <<= 8;
    maxHp <<= 8;
    if (hp > maxHp / 2) {
        return HP_GAUGE_COLOR_GREEN;
    }
    if (hp > maxHp / 5) {
        return HP_GAUGE_COLOR_YELLOW;
    }
    if (hp != 0) {
        return HP_GAUGE_COLOR_RED;
    }
    return HP_GAUGE_COLOR_NONE;
}
