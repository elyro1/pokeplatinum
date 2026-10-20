#ifndef POKEPLATINUM_PORT_GAMEPLAY_CONFIG_H
#define POKEPLATINUM_PORT_GAMEPLAY_CONFIG_H

#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BOOL forgettableHMs;
} GameplayConfig;

// Internal defaults only; no player-facing storage or menu controls yet.
void GameplayConfig_LoadDefaults(GameplayConfig *config);
void GameplayConfig_Init(void);
GameplayConfig *GameplayConfig_Get(void);

#ifdef __cplusplus
}
#endif

#endif
