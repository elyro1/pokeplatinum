#ifndef POKEPLATINUM_PORT_GAMEPLAY_CONFIG_H
#define POKEPLATINUM_PORT_GAMEPLAY_CONFIG_H

#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BOOL forgettableHMs;
    BOOL freeMoveRelearning;
    BOOL freeMoveTutors;
    BOOL mainStoryMoveTutors;
    BOOL unlimitedGreatMarshSteps;
    BOOL zeroEggCycles; // Eggs start ready to hatch; also applies to existing eggs at the next hatch check.
    BOOL instantHoneyTrees;
    BOOL infiniteTMs;
    BOOL levelUpHeldItemEvolutions;
    BOOL instantSpiritomb;
    BOOL feebasWaterStoneEvolution;
    BOOL disableOverworldPoisonDamage;
    u8 tradeEvolutionLevel; // Plain trade evolutions: 1-100; 0 disables level-up evolution.
    u16 shinyRolls; // Total rolls for new eligible Pokemon; 0 and 1 preserve original odds.
    u8 friendshipGainMultiplier; // Positive friendship gains only; 0 and 1 preserve original gains.
    u8 eggHatchSpeedMultiplier; // 1 = original speed; 0 also falls back to original speed.
    u8 catchRateMultiplier; // Final catch rate: 2-255 boosts it; 0 and 1 preserve original rates.
} GameplayConfig;

// Internal defaults only; no player-facing storage or menu controls yet.
void GameplayConfig_LoadDefaults(GameplayConfig *config);
void GameplayConfig_Init(void);
GameplayConfig *GameplayConfig_Get(void);

#ifdef __cplusplus
}
#endif

#endif
