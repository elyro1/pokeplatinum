#ifndef BWBATTLE_TRAINER_BACKS_H
#define BWBATTLE_TRAINER_BACKS_H

#include "bwbattle.h"
#include "types.h"

#define BW_TRAINER_BACK_TEX_SIZE (256 * 128)
#define BW_TRAINER_BACK_MAX_CELLS 8

typedef struct {
    s16 x, y;
    u16 width, height, s, t;
} BwTrainerBackCell;

typedef struct {
    u32 cellCount;
    BwTrainerBackCell cells[BW_TRAINER_BACK_MAX_CELLS];
    u16 colors[256];
    u8 image[BW_TRAINER_BACK_TEX_SIZE];
} BwTrainerBackData;

const BwTrainerBackData *BwTrainerBack_Get(BwTrainerBack trainer);

#endif
