#ifndef POKEPLATINUM_PORT_GAMEPLAY_CONFIG_H
#define POKEPLATINUM_PORT_GAMEPLAY_CONFIG_H

#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BOOL forgettableHMs;
    BOOL freeMoveRelearning;
    BOOL unlimitedGreatMarshSteps;
    BOOL zeroEggCycles; // Eggs start ready to hatch; also applies to existing eggs at the next hatch check.
    u8 eggHatchSpeedMultiplier; // 1 = original speed; 0 also falls back to original speed.
} GameplayConfig;

// Internal defaults only; no player-facing storage or menu controls yet.
void GameplayConfig_LoadDefaults(GameplayConfig *config);
void GameplayConfig_Init(void);
GameplayConfig *GameplayConfig_Get(void);

#ifdef __cplusplus
}
#endif

#endif
