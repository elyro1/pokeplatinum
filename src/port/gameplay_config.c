#include "port/gameplay_config.h"

static GameplayConfig sGameplayConfig;

void GameplayConfig_LoadDefaults(GameplayConfig *config)
{
    config->forgettableHMs = TRUE;
}

void GameplayConfig_Init(void)
{
    GameplayConfig_LoadDefaults(&sGameplayConfig);
}

GameplayConfig *GameplayConfig_Get(void)
{
    return &sGameplayConfig;
}
