#include "assets/trainer_backs.h"

#include "trainer_backs.inc"

const BwTrainerBackData *BwTrainerBack_Get(BwTrainerBack trainer) {
    if (trainer <= BW_TRAINER_BACK_NONE || trainer > BW_TRAINER_BACK_MIRA) {
        return NULL;
    }
    return &sTrainerBacks[trainer - 1];
}
